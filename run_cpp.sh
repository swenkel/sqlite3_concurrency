#!/bin/bash
FILE=./cpp/build/sqlite_concurrency_benchmark_cpp
if test -f $FILE; then
	$FILE	
else
	echo "C++ binary does not exist."
	echo "Build it first."
	echo "Skipping C++ benchmark"
fi