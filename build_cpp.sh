#!/bin/bash
mkdir -p ./cpp/build/
cd ./cpp/build
rm -rf * && cmake ../ && make
cd ../../
