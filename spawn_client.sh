#!/bin/bash
for count in `seq 1 5`
do
        echo " Spawning client $count"
        ./client/client "$HOSTNAME" "$count" "$count" &>/dev/null &
        echo $!
done
