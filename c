#!/bin/bash

# ---------------------------------
# Color codes for friendly messages
# ---------------------------------
COLOR_RED=$(tput sgr0 && tput setaf 1)
COLOR_GREEN=$(tput sgr0 && tput setaf 2)
COLOR_YELLOW=$(tput sgr0 && tput setaf 3)
COLOR_DARK_BLUE=$(tput sgr0 && tput setaf 4)
COLOR_BLUE=$(tput sgr0 && tput setaf 6)
COLOR_PURPLE=$(tput sgr0 && tput setaf 5)
COLOR_PINK=$(tput sgr0 && tput bold && tput setaf 5)
COLOR_LIGHT_GREEN=$(tput sgr0 && tput bold && tput setaf 2)
COLOR_LIGHT_RED=$(tput sgr0 && tput bold && tput setaf 1)
COLOR_LIGHT_CYAN=$(tput sgr0 && tput bold && tput setaf 6)
COLOR_RESET=$(tput sgr0)

# ---------------
# Setup variables
# ---------------

OLDIFS=$IFS
IFS=$'\n'

# ---------
# Functions
# ---------

function _git_sanity_check() {
    local gitTest=`git rev-parse --git-dir 2> /dev/null;`
    if [[ -z "$gitTest" ]]; then
        echo "${COLOR_LIGHT_RED}'c' is a utiltiy command for working with Git recent branches.${COLOR_RESET}"
        echo -e "\n${COLOR_LIGHT_RED}It must be run in a git repository. No git repository detected here!${COLOR_RESET}"
        exit 1
    fi
}

function _longest_branch_name() {
    local branchList=$1

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

# Calculate how many characters to pad including color code length...
function _get_pad_length_with_colors() {
    # When padding the output with printf, we need to take into account the
    # length of the branch name, plus any color codes we use, which aren't
    # printed, but are counted in printf's %-10s padding. We need to add in
    # the estimated string length of the color escape codes (including
    # color reset) to pad the branch name correctly
    local colorLength=4
    local numberOfColors=8
    local longestBranchLength=$1

    let local padLength="( longestBranchLength + 2 ) + ( colorLength  * numberOfColors )"

    echo $padLength
}

function _top_n_branches() {
    git for-each-ref --sort=-committerdate refs/heads/ | head -n 10
}

function _c_git_recent_branches() {

    _git_sanity_check

    local branchList=$(_top_n_branches)

    # Note: branchList must be quoted for linebreaks in variable to be shown
    local wcLineCount=`echo "$branchList" | wc -l`

    # wc -l produces a number with whitespace before it. Remove it
    local numberOfBranches=${wcLineCount// }

    local longestBranchLength=$(_longest_branch_name $branchList)

    local padLength=$(_get_pad_length_with_colors $longestBranchLength)

    # Bash syntax for declaring array
    declare -a branches

    # Show branches in a list with a counter
    local counter=0
    while read -r branch;
    do
        counter=`expr $counter + 1`
        branches=("${branches[@]}" "$branch")
        branchName=`echo "$branch" | sed 's/.*refs\/heads\///'`
        branchNamePrefix="$COLOR_PURPLE$counter. $COLOR_PINK $branchName"
        resetColor="$counter. $branchName"
        # this interpolates to something like `printf %-100s` which is
        # syntax for padding in printf
        printf "%-${padLength}s" $branchNamePrefix
        printf '%s \n' `git show --quiet $branchName --pretty=format:"%C(Yellow)%h %Cred<%an>%Creset %s %C(cyan)(%cr)%Creset"`
    done <<< "$branchList"

    if [[ $counter == "2" ]]; then
        local separator="or"
    else
        local separator="-"
    fi
    # Prompt user for file. -n means no line break after echo
    echo -n "${COLOR_YELLOW}(1 $separator $counter)?${COLOR_RESET} "
    read userInputBranchNumber

    # Remove any whitespace
    local branchNumber=${userInputBranchNumber// }

    # Lol runtime type checking

    # Nothing entered?
    if [[ -z "$branchNumber" ]]; then
        echo "${COLOR_LIGHT_RED}Nothing to do (no branch specified).${COLOR_RESET}"
        exit 1
    fi

    # Not numeric?
    if ! [[ "$branchNumber" =~ ^[0-9]+$ ]]; then
        echo "${COLOR_LIGHT_RED}Please enter a numeric value between 1 and ${numberOfBranches}.${COLOR_RESET}"
        exit 1
    fi

    # Bash arrays are 0 indexed
    let "branchNumber+=-1"

    if [[ "$branchNumber" -ge "$numberOfBranches" ]]; then
        if [[ $numberOfBranches == "1" ]]; then
            echo "${COLOR_LIGHT_RED}Really?${COLOR_RESET}"
        elif [[ "$branchNumber" == "10" ]]; then
            echo "${COLOR_LIGHT_RED}This one doesn't go to eleven :(${COLOR_RESET}"
        else
            echo "${COLOR_LIGHT_RED}Please enter a number from 1 to ${numberOfBranches}${COLOR_RESET}"
        fi
        exit 1
    fi

    local newBranch=`echo "${branches[@]:$branchNumber:1}" | sed 's/.*refs\/heads\///' 2> /dev/null`

    if [[ -z "$newBranch" ]]; then
        echo "${COLOR_LIGHT_RED}No git branch found named '${COLOR_CYAN}${newBranch}${COLOR_LIGHT_RED}?'${COLOR_RESET}"
        exit 1
    fi

    echo -e "\nPerforming \`git checkout ${newBranch}\`"
    git checkout $newBranch

}

# ---------------------
# Main script execution
# ---------------------

_c_git_recent_branches

# --------------------------------------
# Cleanup to not pollute shell variables
# --------------------------------------
unset -f _longest_branch_name
unset -f _c_git_recent_branches
unset -f _top_n_branches
unset -f _get_pad_length_with_colors

IFS=$OLDIFS

