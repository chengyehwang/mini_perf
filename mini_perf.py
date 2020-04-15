#!/usr/bin/python3
import numpy as np

def read_file():
    filename = 'mini_perf.data'
    array = []
    with open(filename, 'rb') as f:
        data = np.fromfile(f, dtype='int64')
        nx = 30
        ny = int(len(data)/nx)
        print('matrix', nx, ny, len(data))
        array = np.reshape(data, [nx, ny])
    return array

counter = read_file()
print(counter)  
