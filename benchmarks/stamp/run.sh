#!/bin/sh

benchmarks[2]="labyrinth"
benchmarks[1]="genome"
benchmarks[3]="intruder"
benchmarks[4]="kmeans"
benchmarks[5]="ssca2"
benchmarks[6]="vacation"
benchmarks[7]="yada"
benchmarks[8]="kmeans"
benchmarks[9]="vacation"

bStr[2]="labyrinth"
bStr[1]="genome"
bStr[3]="intruder"
bStr[4]="kmeans-high"
bStr[5]="ssca2"
bStr[6]="vacation-high"
bStr[7]="yada"
bStr[8]="kmeans-low"
bStr[9]="vacation-low"

comb[1]="-c3 -a10 -l128 -n262144 -c2 -a20 -l128 -n20000"

params[2]="-i inputs/random-x512-y512-z7-n512.txt -t"
params[1]="-g16384 -s64 -n16777216 -r1 -t"
params[3]="${comb[1]} -s1 -r5 -t"
params[4]="-m15 -n15 -t0.00001 -i inputs/random-n65536-d32-c16.txt -p"
params[5]="-s20 -i1.0 -u1.0 -l3 -p3 -r 1 -t"
params[6]="-n4 -q60 -u90 -r1048576 -t4194304 -a 1 -c"
params[7]="-a15 -i inputs/ttimeu1000000.2 -r 1 -t"
params[8]="-m40 -n40 -t0.00001 -i inputs/random-n65536-d32-c16.txt -p"
params[9]="-n2 -q90 -u98 -r1048576 -t4194304 -a 1 -c"


wait_until_finish() {
    pid3=$1
    echo "process is $pid3"
    LIMIT=120
    for ((j = 0; j < $LIMIT; ++j)); do
        kill -s 0 $pid3
        rc=$?
        if [[ $rc != 0 ]] ; then
            echo "returning"
            return;
        fi
        sleep 1s
    done
    kill $pid3
}

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

for b in 3
do
    cd $DIR;
    cd ${benchmarks[$b]};
    echo "${bStr[$b]} | backend $1 | htm_retries $4 | threads $2 | attempt $3 | retry_policy $4"
    ./${benchmarks[$b]} ${params[$b]}$2 > ../../../auto-results/$1-${bStr[$b]}-retries$4-th$2-att$3.data &
    pid3=$!; wait_until_finish $pid3; wait $pid3; rc=$?
    if [ $rc -eq 13 ] ; then
        echo "Error within: backend $1 | ${bStr[$b]} | htm_retries $4 | threads $2 | attempt $3" >> ../../../auto-results/slow.out
    elif [[ $rc != 0 ]] ; then
        echo "Error within: backend $1 | ${bStr[$b]} | htm_retries $4 | threads $2 | attempt $3" >> ../../../auto-results/error.out
    fi
done
