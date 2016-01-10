#!/bin/bash

# create fifo
FIFO=$(mktemp -u)
mkfifo $FIFO




# first reader
xterm -e "cat < $FIFO ; cat" &
# trap "kill $!"

# second writer
cat > $FIFO
wait

# remove fifo
rm -f $FIFO