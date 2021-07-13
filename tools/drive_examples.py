#!/usr/bin/env python3
from __future__ import print_function

import argparse
import subprocess
import sys
import os

indentation = '    '

TERM_RED = '\033[1;31m'
TERM_GREEN = '\033[1;32m'
TERM_YELLOW = '\033[1;33m'
TERM_EMPTY = '\033[0m'


class PluginResult:
    def __init__(self, run_state, details=[]):
        self.run_state = run_state
        self.details = details

    @classmethod
    def success(cls):
        return cls('succeeded')

    @classmethod
    def skip(cls, reason):
        return cls('skipped', details=[reason])

    @classmethod
    def fail(cls, errors=[]):
        return cls('failed', details=errors)


def parse_args(args):
    args = args[1:]
    parser = argparse.ArgumentParser(
        description='A script to run multiple tizen plugin driver tests.')

    parser.add_argument('--plugins', type=str, nargs='*', default=[],
                        help='Specifies which plugins to test. If it is not specified and --run-on-changed-packages is also not specified, then it will include every plugin under packages.')
    parser.add_argument('--exclude', type=str, nargs='*', default=[],
                        help='Exclude plugins to test. --exclude plugin1 plugin2 ...')
    parser.add_argument('--profile', type=str, choices=[
                        'common', 'wearable', 'tv', 'mobile', 'all'], default='common', help='Specifies which device profile target to test on. Defaults to common which is any device.')
    parser.add_argument('--run-on-changed-packages',
                        default=False, action='store_true', help='Run the test on changed packages/plugins. If --plugins is specified, this flag is ignored.')
    parser.add_argument('--base-sha', type=str, default='',
                        help='The base sha used to determine git diff. This is useful when --run-on-changed-packages is specified. If not specified, merge-base is used as base sha.')

    return parser.parse_args(args)


def drive_example_test(plugin, devices):
    example_path = os.path.join(plugin, 'example')
    if not os.path.isdir(example_path):
        return PluginResult.skip('no example')

    drivers_path = os.path.join(example_path, 'test_driver')
    if not os.path.isdir(drivers_path):
        return PluginResult.skip('no driver')

    test_targets_path = os.path.join(example_path, 'integration_test')
    if not os.path.isdir(test_targets_path):
        return PluginResult.skip('no integration test')

    driver_paths = []
    for driver_name in os.listdir(drivers_path):
        driver_path = os.path.join(drivers_path, driver_name)
        if os.path.isfile(driver_path) and driver_name.endswith('_test.dart'):
            driver_paths.append(driver_path)

    test_target_paths = []
    for test_target_name in os.listdir(test_targets_path):
        test_target_path = os.path.join(test_targets_path, test_target_name)
        if os.path.isfile(test_target_path) and test_target_name.endswith('_test.dart'):
            test_target_paths.append(test_target_path)

    errors = []
    for driver_path in driver_paths:
        for test_target_path in test_target_paths:
            print(f'{indentation}running --driver {os.path.basename(driver_path)} --target {os.path.basename(test_target_path)}')
            subprocess.run(['flutter-tizen pub get'],
                           shell=True, cwd=example_path)
            try:
                completed_process = subprocess.run(
                    [f'flutter-tizen drive --driver={driver_path} --target={test_target_path}'], shell=True, cwd=example_path, timeout=300)
                if completed_process.returncode != 0:
                    errors.append(test_target_path)
            except subprocess.TimeoutExpired:
                errors.append(test_target_path)

    return PluginResult.success() if len(errors) == 0 else PluginResult.fail(errors)


def main(argv):
    args = parse_args(argv)
    results = {}
    packages_path = os.path.abspath(os.path.join(
        os.path.dirname(__file__), '../packages'))

    plugin_names = []
    if len(args.plugins) == 0 and args.run_on_changed_packages:
        base_sha = args.base_sha
        if base_sha == '':
            base_sha = subprocess.run(
                'git merge-base --fork-point FETCH_HEAD HEAD', shell=True, cwd=packages_path, encoding='utf-8', stdout=subprocess.PIPE).stdout
            if base_sha == '':
                process_result = subprocess.run(
                    'git merge-base FETCH_HEAD HEAD', shell=True, cwd=packages_path, encoding='utf-8', stdout=subprocess.PIPE).stdout
        changed_files = subprocess.run(
            f'git diff --name-only {base_sha} HEAD', shell=True, cwd=packages_path, encoding='utf-8', stdout=subprocess.PIPE).stdout.split('\n')
        changed_plugins = []
        for changed_file in changed_files:
            path_segments = changed_file.split('/')
            if 'packages' not in path_segments:
                continue
            index = path_segments.index('packages')
            if index < len(path_segments):
                changed_plugins.append(path_segments[index + 1])
        plugin_names = list(set(changed_plugins))
    else:
        for plugin_name in os.listdir(packages_path):
            plugin_path = os.path.join(packages_path, plugin_name)
            if not os.path.isdir(plugin_path):
                continue
            if len(args.plugins) > 0 and plugin_name not in args.plugins:
                continue
            if len(args.plugins) > 0 and plugin_name in args.exclude:
                continue
            plugin_names.append(plugin_name)

    test_num = 0
    for plugin_name in plugin_names:
        plugin_path = os.path.join(packages_path, plugin_name)
        test_num += 1
        print(
            f'{indentation}Testing for {plugin_name} ({test_num}/{len(plugin_names)})')
        result = drive_example_test(plugin_path, [])
        if result.run_state == 'skipped':
            print(f'{indentation}SKIPPING: {plugin_name}')
        results[plugin_name] = result

    print(f'============= TEST RESULT =============')
    failed_plugins = []
    for plugin_name, result in results.items():
        color = TERM_GREEN
        if result.run_state == 'skipped':
            color = TERM_YELLOW
        elif result.run_state == 'failed':
            color = TERM_RED

        print(
            f'{color}{indentation}{result.run_state.upper()}: {plugin_name}{TERM_EMPTY}')
        if result.run_state != 'succeeded':
            print(f'{indentation}{indentation} DETAILS: {result.details}')
        if result.run_state == 'failed':
            failed_plugins.append(plugin_name)

    if len(failed_plugins) > 0:
        print(f'{TERM_RED}============= FAILED TEST ============={TERM_EMPTY}')
        for failed_plugin in failed_plugins:
            print(
                f'{indentation}FAILED: {plugin_name} DETAILS: {results[failed_plugin].details}')
        return 1

    if len(results) == 0:
        print(f'{TERM_YELLOW}No tests are run.{TERM_EMPTY}')
    else:
        print(f'{TERM_GREEN}All tests passed.{TERM_EMPTY}')
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
