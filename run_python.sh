#!/bin/bash
FILE=./python/sqlite_concurrency_benchmark_python.py
if test -f $FILE; then
	python3 $FILE
else
	echo "Python file does not exist."
	echo "Skiping Python benchmark."
fi