#!/bin/bash
# Make sure encode and decode work for both uuencode and base64. Test some
# common combinations. Can't test everything.

if [ ! -x steg ]; then
	echo "ERROR: Type 'make' to compile the program first."
	exit 1
fi

key=alongtestkey123hopefullythiswillwork
badkey=alongtestkey124hopefullythiswillwork
seed=1234
badseed=1235

doEqDiff()
{
	diff test.txt out > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "OK"
	else
		echo "FAIL"
	fi
}


doNeDiff()
{
	diff test.txt out > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "FAIL"
	else
		echo "OK"
	fi
}


{
cat << EOF
hello world this is a test message to see what happens with both uuencode and 
base 64. I could waffle on forever but its time to do this dull test
EOF
} > test.txt

rm -f test.uu test.64 out

# Test fake uuencode
echo -n "TEST 1: "
steg -i test.txt -o test.uu -e -u
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.uu -o out -u
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi
 
# Test fake uuencode with alternative bit count 4 which also creates pad bytes
echo -n "TEST 2: "
steg -i test.txt -o test.uu -e -b 4 -u
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.uu -o out -b 4 -u
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Test fake uuencode with alternative bit count 4
echo -n "TEST 3: "
steg -i test.txt -o test.uu -e -b 4
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.uu -o out -b 4
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Test fake uuencode with key
echo -n "TEST 4: "
steg -i test.txt -o test.uu -e -k $key -u
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.uu -o out -k $key -u
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Negative test. Test uuencode with wrong key, must fail.
echo -n "TEST 5: "
steg -i test.uu -o out -k $badkey -u 
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	doNeDiff
fi

# Test uuencode with LFSR and key. 
echo -n "TEST 6: "
steg -i test.txt -o test.uu -s $seed -k $key -e -u
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.uu -o out -s $seed -k $key -u
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Negative test. Test uuencode with wrong seed. Must fail.
echo -n "TEST 7: "
steg -i test.uu -o out -k $key -s $badseed
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	doNeDiff
fi

# Test fake base64
echo -n "TEST 8: "
steg -i test.txt -o test.64 -e
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.64 -o out
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Test fake base64 with line length of 1
echo -n "TEST 9: "
steg -i test.txt -o test.64 -e -l 1
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.64 -o out
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi


# Test fake base64 with alternate bit count 1
echo -n "TEST 9: "
steg -i test.txt -o test.64 -e -b 1
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.64 -o out -b 1
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Test fake base64 with alternate bit count 4
echo -n "TEST 10: "
steg -i test.txt -o test.64 -e -b 4
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.64 -o out -b 4
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Test fake base64 with key
echo -n "TEST 10: "
steg -i test.txt -o test.64 -e -k $key
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.64 -o out -k $key
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Negative test. Test base64 with wrong key, must fail.
echo -n "TEST 12: "
steg -i test.64 -o out -k $badkey
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	doNeDiff
fi

# Test base64 with key and seed.
echo -n "TEST 13: "
steg -i test.txt -o test.64 -s $seed -k $key -e
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	steg -i test.64 -o out -s $seed -k $key
	if [ $? -ne 0 ]; then
		echo "FAIL"
	else
		doEqDiff
	fi
fi

# Negative test. Test base64 with wrong seed. Must fail.
echo -n "TEST 14: "
steg -i test.64 -o out -k $key -s $badseed
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	doNeDiff
fi


# Test decode from stdin
echo -n "TEST 15: "
steg -o out -s $seed -k $key < test.64
if [ $? -ne 0 ]; then
	echo "FAIL"
else
	doEqDiff
fi

rm -f test.txt test.uu test.64 out
