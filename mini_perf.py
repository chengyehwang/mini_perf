#!/usr/bin/env python3
import numpy as np
import pandas as pd
import re
name = []
with open('mini_perf.head','r') as f:
    for line in f.readlines():
        m = re.search('count:\s*(\S+)',line)
        if m:
            name.append(m.group(1))
print(name)
def read_file():
    filename = 'mini_perf.data'
    array = []
    with open(filename, 'rb') as f:
        data = np.fromfile(f, dtype='int64')
        nx = len(name)
        ny = int(len(data)/nx)
        print('matrix', nx, ny, len(data))
        array = np.reshape(data, [ny, nx])
    return array

counter = read_file()
print(counter)
data = pd.DataFrame(counter,columns=name)
print(data) 
