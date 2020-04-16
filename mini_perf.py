#!/usr/bin/python3
import numpy as np
import pandas as pd
def read_file():
    filename = 'mini_perf.data'
    array = []
    with open(filename, 'rb') as f:
        data = np.fromfile(f, dtype='int64')
        nx = 30+6
        ny = int(len(data)/nx)
        print('matrix', nx, ny, len(data))
        array = np.reshape(data, [ny, nx])
    return array

counter = read_file()
print(counter)
data = pd.DataFrame(counter)
print(data) 
