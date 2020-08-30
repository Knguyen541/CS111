#!/bin/bash

#https://linuxize.com/post/how-to-check-if-string-contains-substring-in-bash/

OUTPUT=$(uname -a)
#echo "${OUTPUT}"
SUB='armv7l'

if [[ "${OUTPUT}" =~ .*"$SUB".* ]]
then
    make tcp tls
else
    make dummy dummy2
fi  
