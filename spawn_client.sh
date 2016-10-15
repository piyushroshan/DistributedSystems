#!/bin/bash
for count in `seq 1 100`
do
        echo " Spawning client $count"
        ./client "localhost" "$count" "$count"  &>/dev/null &
        echo $!
done
