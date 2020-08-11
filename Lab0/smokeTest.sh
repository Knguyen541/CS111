#!/bin/bash

echo "Testing 1 2" > input.txt
echo "Testing 3 4" >> input.txt
./lab0 --input input.txt --output output.txt > /dev/null
cmp input.txt output.txt &> /dev/null
if [ $? -eq 0 ] 
then
    echo Basic copy: PASSED
else 
    echo Basic copy: FAILED
fi 
rm input.txt output.txt

./lab0 --bogus &> /dev/null
if [ $? -eq 1 ]
then
    echo Handle bogus argument: PASSED
else
    echo Handle bogus argument: FAILED
fi

rm Hi &> /dev/null
./lab0 --input Hi &> /dev/null
if [ $? -eq 2 ]
then
    echo Handle unable to open input file: PASSED
else
    echo Handle unable to open input file: FAILED
fi

touch Locked!
chmod 444 Locked!
./lab0 --output Locked! &> /dev/null
if [ $? -eq 3 ]
then
    echo Handle unable to access output file: PASSED
else
    echo Handle unable to access output file: FAILED
fi
rm -f Locked!

./lab0 --segfault --catch &> /dev/null
if [ $? -eq 4 ]
then
    echo Handle segfault-catching: PASSED
else
    echo Handle segfault-catching: FAILED
fi
