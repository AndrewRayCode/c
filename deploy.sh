if [[ -z "$1" ]]; then
    echo "Use: deploy [version]"
    exit 1
fi
fileName=c-${1}.tar.gz
tar -zcf c-${1}.tar.gz c c_recent_branches_completer

echo "$fileName"
