#!/usr/bin/env python

import subprocess
import sys

outfile=""
plugin='@JSBIND_PLUGIN_PATH@'

is_c = False
is_out = False
for arg in sys.argv:
    if is_out:
        is_out = False
        outfile = arg
    else:
        if arg == '-c':
            is_c = True
        if arg == '-o':
            is_out = True

subprocess.run([
    'clang++',
    f"-fplugin={plugin}",
    f"-fplugin-arg-jsbind-out-file-path={outfile}data"
    ] + sys.argv[1:]).check_returncode()

if is_c:
    subprocess.run([
        'llvm-objcopy',
        '--keep-undefined',
        '--add-section',
        f'.custom.jsbind={outfile}data',
        outfile
    ]).check_returncode()

