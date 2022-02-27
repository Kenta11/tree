#!/usr/bin/env bash
# -*- coding: utf-8 -*-

# 結果を出力するディレクトリを生成
result=$(mktemp -d)

## @fn run_test()
## @brief test runner
## @param tree command option
function run_test() {
  # reference版と開発版のtreeをそれぞれ実行
  ${reference_tree} ${1} ~ > "${result}/reference_tree${1}.txt"
  ${developed_tree} ${1} ~ > "${result}/developed_tree${1}.txt"

  # 標準出力を比較する
  diff "${result}/reference_tree${1}.txt" "${result}/developed_tree${1}.txt"

  # 実行結果に変化が無ければsuccess，変化があればfailed
  if [ $? -eq 0 ]; then
    echo "[SUCCESS] tree ${1} ~"
  else
    echo "[FAILED]  tree ${1} ~"
  fi
}

run_test
run_test "-a"
run_test "-d"
run_test "-l"
run_test "-f"
run_test "-x"
run_test "-L 10"
run_test "-R"
run_test "-P *.txt"
run_test "-I *.txt"
run_test "--gitignore"
run_test "--ignore-case"
run_test "--matchdirs"
run_test "--metafirst"
run_test "--prune"
run_test "--info"
run_test "--noreport"
run_test "--charset UTF-8"
run_test "--filelimit 10"
run_test "--timefmt %Y%m%d"
run_test "-q"
run_test "-N"
run_test "-Q"
run_test "-p"
run_test "-u"
run_test "-g"
run_test "-s"
run_test "-h"
run_test "-si"
run_test "-du"
run_test "-D"
run_test "-F"
run_test "--inodes"
run_test "--device"
run_test "-v"
run_test "-t"
run_test "-c"
run_test "-U"
run_test "-r"
run_test "--dirsfirst"
run_test "--filesfirst"
run_test "--sort ctime"
run_test "-i"
run_test "-A"
run_test "-S"
run_test "-n"
run_test "-C"
run_test "-X"
run_test "-J"
run_test "-H sample1"
run_test "-H sample2 -T mytitle"
run_test "-H sample3 --nolinks"
run_test "--help"
