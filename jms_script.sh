#!/bin/bash

if [ "$#" -lt 4 ];
then
  echo "Usage: -l <path> -c <command>."
  exit 1
fi


flag=0
n=0
while true ; do
    case "$1" in
        -l)
            case "$2" in
                "") echo "Usage: -l <path> -c <command>." ; exit 1 ;;
                *) path_com=$2; flag=$((flag+1)) ; shift 2 ;;
            esac ;;
        -c)
            case "$2" in
                "") echo "Usage: -l <path> -c <command>." ; exit 1 ;;
                *) operation=$2; flag=$((flag+1)) ; shift 2 ;
                  if [[ "$operation"=="size" && "$1" != "-l" ]]
                  then
                    if [[ "$1"=~^-?[0-9]+$ && "$1" != "" ]]
                    then
                      n=$1;
                      shift 1;
                    fi;
                  fi;;
            esac ;;
        "") break;;
        *) echo "Usage: -l <path> -c <command>." ; exit 1 ;;
    esac
done


if [[ $flag != 2 ]]
then
	echo "Please Enter Right Arguments!";
	exit 1;
fi
if [ "$operation" == "list" ]
then
	eval "ls -l $path_com/sdi1400215*"
fi
if [ "$operation" == "size" ]
then
	if [ "$n" == 0 ]
	then
		eval "du --max-depth=1 $path_com/sdi1400215* | sort -n -r"
	else
		eval "du --max-depth=1 $path_com/sdi1400215* | sort -n -r | head -$n"
	fi
fi
if [ "$operation" == "purge" ]
then
       eval "rm -rf $path_com/sdi1400215*"
fi