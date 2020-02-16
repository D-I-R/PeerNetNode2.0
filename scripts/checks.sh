#!/bin/bash
set -e

# shellcheck disable=SC2006
files=`find . -name "*.cpp" -or -name ".h"`
filter=-build/c++11,-runtime/references,-whitespace/braces,-whitespace/indent,-whitespace/comments,-build/include_order
echo "$files" | xargs cpplint --filter=$filter
export CTEST_OUTPUT_ON_FAILURE=true
