#!/bin/sh

# A simple script to get a validation result from clang-format, since it is
# itself not usable directly as a test in cmake.

if [ -z $@ ]; then
    echo "No input files."
    exit 1
fi
echo "$@" | xargs -n1 clang-format -style=file -output-replacements-xml | grep -P "^\s?<replacement .*"
if [ $? -ne 1 ]; then
    echo "Code formatting is wrong."
    exit 1
fi
