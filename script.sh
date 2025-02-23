# to run all test cases

for i in tests/*; do
    if [[ "$i" == "tests/test.in" ]]; then
        continue
    fi

    timeout 1s ./grader ./engine < $i
done
