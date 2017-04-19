#!/bin/sh
find src/ -iname "*.cpp" -exec clang-format-3.9 -i {} \;
find src/ -iname "*.h" -exec clang-format-3.9 -i {} \;
