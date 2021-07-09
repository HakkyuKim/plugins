#/usr/bin/env bash

set -e

dirs=`ls -1 packages`
home=`pwd`

for dir in $dirs; do
    echo "----Running integration test for $dir.----"
    example="packages/$dir/example"
    if [[ ! -d $example ]]; then
        echo "$example doesn't exist"
        continue
    fi
    if [[ ! -d $example/integration_test ]]; then
        echo "Integration test doesn't exist for $dir"
        continue
    fi
    cd $example
    flutter-tizen pub get
    flutter-tizen drive \
      --driver=test_driver/integration_test.dart \
      --target=integration_test/"$dir"_test.dart
    flutter-tizen clean
    cd $home
done