#/usr/bin/env bash

set -e

example_project=$1

if [[ ! -d $example_project/integration_test ]]; then
    echo "Integration test doesn't exist for $example_project"
    continue
fi

cd $example_project
plugin_name=$(basename $(dirname $example_project))

flutter-tizen pub get
flutter-tizen drive \
    --driver=test_driver/integration_test.dart \
    --target=integration_test/"$plugin_name"_test.dart
flutter-tizen clean
  