#!/bin/bash

# Colors for prompt
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

function _c() {
    cur=${COMP_WORDS[COMP_CWORD]}
    branches=`git for-each-ref --sort=-committerdate refs/heads/ | head -n 10`

    # By default bash breaks on "words", not lines. Set this magical garbage
    # so autocomplete works with line breaks, not one big string
    local IFS=$'\n'

    output=''
    while read -r branch;
    do
        output+=`echo $branch | sed 's/.*refs\/heads\///'`
        local rawBranchName=`git show --quiet $(echo $branch | cut -d' ' -f1) --pretty=format:"%h <%an> %s (%cr)\'"`
        # If a commit message has backticks or dollar signs in it, bash will
        # try to interpolate it when outputting. Escape backtick and dollar
        # sign to prevent that
        local sanitizedbranchName=`echo $rawBranchName | sed 's/\([$\`]\)/\\\1/g'`
        output+="'"
        output+=$sanitizedBranchname
        output+="'"$'\n'
    done <<< "$branches"

    # Why do I lowercase the output?
    response=''
    for branch in $output
    do
        lowerBranch=`echo $branch | tr '[:upper:]' '[:lower:]'`
        if [[ $branch =~ .*$cur.* ]]; then
            response+=$branch$'\n'
        fi
    done

    COMPREPLY=( $( compgen -W "$response" -- $cur ) )
}

function c() {
    local newBranch=""
    local inputted=""

    if [[ -z "$1" ]]; then
        local branchOutput=`git for-each-ref --sort=-committerdate refs/heads/ | head -n 10`
        local IFS=$'\n'

        # Bash syntax for declaring array
        declare -a branches

        local counter=0
        local longestBranchLength=0

        # When padding the output with printf, we need to take into account the
        # length of the branch name, plus any color codes we use, which aren't
        # printed, but are counted in printf's %-10s padding. We need to add in
        # the estimated string length of the color escape codes (including
        # color reset) to pad the branch name correctly
        local colorLength=4
        local numberOfColors=8

        # Find the longest branch name for padding...
        while read -r branch;
        do
            local branchName=`echo "$branch" | sed 's/.*refs\/heads\///'`
            if [[ "${#branchName}" -gt "$longestBranchLength" ]]; then
                longestBranchLength=${#branchName}
            fi
        done <<< "$branchOutput"

        # Calculate how many characters to pad including color code length...
        let local padLength="( longestBranchLength + 2 ) + ( colorLength  * numberOfColors )"

        # Show branches in a list with a counter
        while read -r branch;
        do
            counter=`expr $counter + 1`
            local branches=("${branches[@]}" "$branch")
            local branchName=`echo "$branch" | sed 's/.*refs\/heads\///'`
            local branchNamePrefix="$COLOR_PURPLE$counter. $COLOR_PINK $branchName"
            local resetColor="$counter. $branchName"
            # this interpolates to something like `printf %-100s` which is
            # syntax for padding in printf
            printf "%-${padLength}s" $branchNamePrefix
            printf '%s \n' `git show --quiet $branchName --pretty=format:"%C(Yellow)%h %Cred<%an>%Creset %s %C(cyan)(%cr)%Creset"`
        done <<< "$branchOutput"

        # Prompt user for file. -n means no line break after echo
        echo -n "$COLOR_YELLOW?$COLOR_RESET "
        read branchNumber

        let "branchNumber+=-1"

        branchLength=${#branches[@]}

        # lol runtime type checking
        if [[ "$branchNumber" =~ ^[0-9]+$ ]]; then

            if [[ "$branchNumber" -ge "$branchLength" ]]; then
                if [[ $branchLength == "1" ]]; then
                    echo "${COLOR_LIGHT_RED}Really?${COLOR_RESET}"
                elif [[ "$branchNumber" == "10" ]]; then
                    echo "${COLOR_LIGHT_RED}This one doesn't go to eleven :(${COLOR_RESET}"
                else
                    echo "${COLOR_LIGHT_RED}Please enter a number from 1 to ${branchLength}${COLOR_RESET}"
                fi
                return 1
            fi

            newBranch=`echo "${branches[@]:$branchNumber:1}" | sed 's/.*refs\/heads\///' 2> /dev/null`

            if [[ -z "$newBranch" ]]; then
                echo "${COLOR_LIGHT_RED}No git branch found named '${COLOR_CYAN}${newBranch}${COLOR_LIGHT_RED}?'${COLOR_RESET}"
                return 1
            fi
        else
            echo "${COLOR_LIGHT_RED}Please enter a numeric value.${COLOR_RESET}"
            return 1
        fi
    else
        inputted=1
        newBranch=`echo "$1" | cut -d' ' -f1`
    fi

    if [[ -n "$1" ]]; then
        echo `git show --quiet "$newBranch" --pretty=format:"%C(Yellow)%h %Cred<%an>%Creset %s %C(cyan)(%cr)%Creset"`
    fi

    if [[ $newBranch =~ ^pr ]]; then
        echo -e "\ngit fetch $newBranch && git checkout $newBranch"
        git fetch $newBranch && git checkout $newBranch
    else
        echo -e "\ngit checkout $newBranch"
        git checkout $newBranch
    fi

}

