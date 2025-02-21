for i in tests/*; do
    timeout 1s ./grader ./engine < $i
done
