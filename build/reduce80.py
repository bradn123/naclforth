#!/usr/bin/python

import re
import sys


# Load each 80 column line, stripping trailing spaces.
lines = []
while True:
  line = sys.stdin.read(80)
  if not line:
    break
  line = re.sub('[ ]+$', '', line)
  lines.append(line)

# Drop trailing empty lines.
while lines and lines[-1] == '':
  lines = lines[:-1]

# Print each line.
for line in lines:
  print line
