#!/usr/bin/env bash
for file in $(find assets -type f); do
	echo ${file##*.};
done | sort | uniq
