#!/bin/bash

mv game.out game.in
mv players.out players.in
./neworigins_v2 run
rm orders.*
for f in template*; do mv "$f" $(echo "$f" | sed 's/^template/orders/g'); done