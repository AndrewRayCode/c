#!/bin/bash

# ---------------------------------
# Color codes for friendly messages
# ---------------------------------
_COLOR_BOLD=$(tput sgr0 && tput bold)
_COLOR_RED=$(tput sgr0 && tput setaf 1)
_COLOR_GREEN=$(tput sgr0 && tput setaf 2)
_COLOR_YELLOW=$(tput sgr0 && tput setaf 3)
_COLOR_DARK_BLUE=$(tput sgr0 && tput setaf 4)
_COLOR_BLUE=$(tput sgr0 && tput setaf 6)
_COLOR_PURPLE=$(tput sgr0 && tput setaf 5)
_COLOR_PINK=$(tput sgr0 && tput bold && tput setaf 5)
_COLOR_LIGHT_GREEN=$(tput sgr0 && tput bold && tput setaf 2)
_COLOR_LIGHT_RED=$(tput sgr0 && tput bold && tput setaf 1)
_COLOR_LIGHT_CYAN=$(tput sgr0 && tput bold && tput setaf 6)
_COLOR_RESET=$(tput sgr0)

# ---------------
# Setup variables
# ---------------

_OLDIFS=$IFS
IFS=$'\n'

_ERROR_SYMBOL="🚩"
_SNOOZE_SYMBOL="😴"
_GUITAR_SYMBOL="😴"

# ---------
# Functions
# ---------

# Print help message that I went overboard on (I sure hope c is a free name!)
function _print_help() {
    echo -e "${_COLOR_PURPLE}  /${_COLOR_PINK}\\ __/\\"
    echo -e "${_COLOR_PURPLE} / /___\\/"
    echo -e "/ / /"
    echo -e "\\ ${_COLOR_PINK}\\ \\__   ${_COLOR_RESET}'c' is a shell script for quickly viewing and switching between recent Git branches."
    echo -e "${_COLOR_PURPLE} \\ ${_COLOR_PINK}\\ __/\\"
    echo -e "${_COLOR_PURPLE}  \\/___\\/${_COLOR_RESET}"
    echo -e "\n${_COLOR_PINK}Use${_COLOR_RESET}"
    echo -e "    The most common usage is to run c without arguments. Then you're presented with a list of recent branches."
    echo -e "    Entering the number corresponding with the listed branch performs 'git checkout branch'"
    echo -e "\n    c also supports tab completion of branch names. See the ${_COLOR_PINK}Tab Completion${_COLOR_RESET} section for more."
    echo -e "\n${_COLOR_PINK}Command Line Options${_COLOR_RESET}"
    echo -e "    c ${_COLOR_BOLD}branchname${_COLOR_RESET}"
    echo -e "        Peforms 'git checkout branchname'"
    echo -e "\n    c ${_COLOR_BOLD}-h${_COLOR_RESET}"
    echo -e "    c ${_COLOR_BOLD}--help${_COLOR_RESET}"
    echo -e "        Print this help message"
    echo -e "\n${_COLOR_PINK}Tab Completion${_COLOR_RESET}"
    echo -e "    A useful feature of c is typing c<space><tab><tab> to automatically list all recent branches and their"
    echo -e "    corresponding commits. To set up this command line completion, add the following to your .bashwhatever:"
    echo -e "\n        # Set up tab completion for git completion 'c' utility script"
    echo -e "        if [[ -d \"\$(brew --prefix c)\" ]]; then"
    echo -e "            source \"\$(brew --prefix c)/c_recent_branches_completer\""
    echo -e "        fi"
}

# Make sure we're in a git repository and error if not
function _git_sanity_check() {
    local gitTest=`git rev-parse --git-dir 2> /dev/null;`
    if [[ -z "$gitTest" ]]; then
        echo "${_COLOR_LIGHT_RED}'c' is a utiltiy command for working with Git recent branches.${_COLOR_RESET}"
        echo -e "\n${_COLOR_LIGHT_RED}${_ERROR_SYMBOL}It must be run in a git repository. No git repository detected here!${_COLOR_RESET}"
        exit 1
    fi
}

# Find the longest branch name and return the string length of it
function _longest_branch_name() {
    local branchList="$1"

    local longestBranchLength=0

    # Find the longest branch name for padding...
    while read -r branch;
    do
        local branchName=`echo "$branch" | sed 's/.*refs\/heads\///'`
        if [[ "${#branchName}" -gt "$longestBranchLength" ]]; then
            longestBranchLength=${#branchName}
        fi
    done <<< "$branchList"

    echo $longestBranchLength
}

# Get the names of the top n branches
function _top_n_branches() {
    git for-each-ref --sort=-committerdate refs/heads/ | head -n 10
}

function _perform_checkout() {
    local newBranch=$1
    echo -e "\n${_COLOR_LIGHT_GREEN}Performing \`${_COLOR_LIGHT_CYAN}git checkout ${_COLOR_GREEN}${newBranch}${_COLOR_LIGHT_GREEN}\`${_COLOR_RESET}"
    git checkout $newBranch
}

# Do the heavy lifting of this command
function _c_git_recent_branches() {

    _git_sanity_check

    local branchList=$(_top_n_branches)

    # Note: branchList must be quoted for linebreaks in variable to be shown
    local wcLineCount=`echo "$branchList" | wc -l`

    # wc -l produces a number with whitespace before it. Remove it
    local numberOfBranches=${wcLineCount// }

    local longestBranchLength=$(_longest_branch_name "$branchList")

    let local padLength="longestBranchLength + 1"

    local currentGitBranch=$(git branch | sed -n '/\* /s///p')

    local firstGitBranch=`echo "$branchList" | head -n1 | sed 's/.*refs\/heads\///'`

    # Bash syntax for declaring array
    declare -a branches

    # Show colorful branches in a list with a counter and last commit
    echo "${_COLOR_LIGHT_CYAN}Recent branches:${_COLOR_RESET}"
    local counter=0
    while read -r branch;
    do
        local counter=`expr $counter + 1`
        local branches=("${branches[@]}" "$branch")
        local branchName=`echo "$branch" | sed 's/.*refs\/heads\///'`
        local branchNamePrefix="${_COLOR_PURPLE}${counter}. ${_COLOR_PINK} ${branchName}${_COLOR_RESET}"
        local lastCommitFormatted=`git show --quiet $branchName --pretty=format:"%C(Yellow)%h %Cred<%an>%Creset %s %C(cyan)(%cr)%Creset"`

        # this interpolates to something like `printf %-100s` which is syntax
        # for padding in printf
        printf " ${_COLOR_PURPLE}%-2s ${_COLOR_PINK}%-${padLength}s" "${counter}." $branchName
        printf '%s \n' $lastCommitFormatted
    done <<< "$branchList"

    if [[ "$counter" == "1" && "$currentGitBranch" == "$firstGitBranch" ]]; then
        echo -e "\n${_COLOR_LIGHT_GREEN}There's only one branch in this repository, and you're on it!${_COLOR_RESET}"
        exit 0
    fi

    # If there's only 2 options, show "1 or 2". Otherwise show "1 - n"
    if [[ $counter == "2" ]]; then
        local separator="or"
    else
        local separator="-"
    fi

    # Prompt user for file. -n means no line break after echo
    echo -n "${_COLOR_YELLOW}1 $separator $counter?${_COLOR_RESET} "
    read userInputBranchNumber

    # Remove any whitespace
    local branchNumber=${userInputBranchNumber// }

    # Lol runtime type checking

    # Nothing entered?
    if [[ -z "$branchNumber" ]]; then
        echo "${_COLOR_LIGHT_RED}${_SNOOZE_SYMBOL}  Nothing to do (no branch specified).${_COLOR_RESET}"
        exit 1
    fi

    # Not numeric?
    if ! [[ "$branchNumber" =~ ^[0-9]+$ ]]; then
        echo "${_COLOR_LIGHT_RED}${_ERROR_SYMBOL}  Please enter a numeric value between 1 and ${numberOfBranches}.${_COLOR_RESET}"
        exit 1
    fi

    # Bash arrays are 0 indexed, so subtract one from user input (notice MINUS)
    let "branchNumber+=-1"

    if [[ "$branchNumber" -ge "$numberOfBranches" ]]; then
        if [[ "$branchNumber" == "10" ]]; then
            echo "${_COLOR_LIGHT_RED}${_GUITAR_SYMBOL}  This one doesn't go to eleven ${_COLOR_RESET}"
        else
            echo "${_COLOR_LIGHT_RED}${_ERROR_SYMBOL}  Please enter a number from 1 to ${numberOfBranches}${_COLOR_RESET}"
        fi
        exit 1
    fi

    local newBranch=`echo "${branches[@]:$branchNumber:1}" | sed 's/.*refs\/heads\///' 2> /dev/null`

    if [[ -z "$newBranch" ]]; then
        echo "${_COLOR_LIGHT_RED}${_ERROR_SYMBOL} No git branch found named '${_COLOR_CYAN}${newBranch}${_COLOR_LIGHT_RED}?'${_COLOR_RESET}"
        exit 1
    fi

    _perform_checkout $newBranch

}

# ---------------------
# Main script execution
# ---------------------

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    _print_help
    exit 0
fi

# If nothing was provided to the script, list the recent branches
if [[ -z "$1" ]]; then
    _c_git_recent_branches
# This command can also change to a branch if specified
else
    _perform_checkout $1
fi

# --------------------------------------
# Cleanup to not pollute shell variables
# --------------------------------------

# unset functions
unset -f _longest_branch_name
unset -f _c_git_recent_branches
unset -f _top_n_branches
unset -f _print_help

# replace magic bash separator
IFS=$_OLDIFS

# unset variables
unset _COLOR_RED
unset _COLOR_GREEN
unset _COLOR_YELLOW
unset _COLOR_DARK_BLUE
unset _COLOR_BLUE
unset _COLOR_PURPLE
unset _COLOR_PINK
unset _COLOR_LIGHT_GREEN
unset _COLOR_LIGHT_RED
unset _COLOR_LIGHT_CYAN
unset _COLOR_RESET

unset _ERROR_SYMBOL
unset _SNOOZE_SYMBOL
unset _GUITAR_SYMBOL
