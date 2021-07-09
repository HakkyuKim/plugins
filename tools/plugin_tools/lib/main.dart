import 'dart:convert';
import 'dart:io';

import 'package:file/local.dart';
import 'package:file/file.dart';

void main(List<String> args) async {
  final LocalFileSystem fileSystem = LocalFileSystem();
  final Directory pluginsRootDir = fileSystem.currentDirectory;

  final File runnerScript =
      pluginsRootDir.childDirectory('tools').childFile('runner.sh');
  if (!runnerScript.existsSync()) {
    print("${runnerScript.path} doesn't exist.");
    exit(1);
  }

  final List<Directory> pluginDirs = pluginsRootDir
      .childDirectory('packages')
      .listSync(recursive: false)
      .whereType<Directory>()
      .toList();

  final List<List<String>> results = <List<String>>[];

  for (final Directory pluginDir in pluginDirs) {
    if (pluginDir.childDirectory('example').existsSync()) {
      if (args.contains(pluginDir.basename)) {
        final Process process = await Process.start(runnerScript.path, <String>[
          pluginDir.childDirectory('example').path,
        ]);
        await process.stdout.transform(utf8.decoder).forEach((String message) {
          print(message);
        });
        int result = await process.exitCode;
        if (result != 0) {
          await process.stderr
              .transform(utf8.decoder)
              .forEach((String message) {
            print(message);
          });
        }
        results.add(<String>[
          pluginDir.basename,
          result.toString(),
        ]);
      }
    }
  }

  for (final List<String> result in results) {
    print('${result[0]}: ${result[1]}');
  }
  print(
      'fails: ${results.where((List<String> element) => element[1] != '0').length}');
}
