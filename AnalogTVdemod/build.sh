#!/bin/bash
rm build/TVdemod
make main
cd build
./TVdemod
cd ..