#!/usr/bin/env bash
if [ ! -f make.sh ]; then
	echo 'make.sh must be run within its container folder' 1>&2
	exit 1
fi


CURDIR=`pwd`
OLDGOPATH="$GOPATH"
export GOPATH="$CURDIR":"$OLDGOPATH"
go install main
go test proto
export GOPATH="$OLDGOPATH"

if [ "$1" = "fmt" ]; then
    gofmt -w src
fi

