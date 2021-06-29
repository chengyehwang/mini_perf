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
#print(name)
def read_file():
    filename = 'mini_perf.data'
    array = []
    with open(filename, 'rb') as f:
        data = np.fromfile(f, dtype='int64')
        nx = len(name)
        ny = int(len(data)/nx)
        #print('matrix', nx, ny, len(data))
        array = np.reshape(data, [ny, nx])
    return array

counter = read_file()
#print(counter)
data = pd.DataFrame(counter,columns=name)

for cpu in range(8):
    cpu = '_cpu%d' % cpu
    for group in range(8):
        group = '_g%d' % group
        for cache in ['l1i', 'l1d', 'l2d', 'l3d']:
            count_refill = 'raw-' + cache + '-cache-refill' + group + cpu
            count_total = 'raw-' + cache + '-cache' + group + cpu
            count_miss = cache + '-cache-miss' + cpu
            if count_refill in data.columns and count_total in data.columns:
                data[count_miss] = data[count_refill] / data[count_total]

        count_i = 'raw-l1i-cache' + group + cpu
        count_d = 'raw-l1d-cache' + group + cpu
        count_total = 'raw-inst-retired' + group + cpu
        count_req = 'l1-cache-req' + cpu
        if count_i in data.columns and count_d in data.columns and count_total in data.columns:
            data[count_req] = (data[count_i] + data[count_d]) / data[count_total]

    l1_miss = 'l1-cache-miss' + cpu
    l2_miss = 'l2-cache-miss' + cpu
    l3_miss = 'l3-cache-miss' + cpu
    l1_hit = 'l1-cache-hit' + cpu
    l2_hit = 'l2-cache-hit' + cpu
    l3_hit = 'l3-cache-hit' + cpu
    dram_hit = 'dram-cache-hit' + cpu
    l1_req = 'l1-cache-req' + cpu
    l2_req = 'l2-cache-req' + cpu
    l3_req = 'l3-cache-req' + cpu

    if l1_req in data.columns and l1_miss in data.columns:
        data[l1_hit] = data[l1_req] * ( 1 - data[l1_miss])
        data[l2_req] = data[l1_req] * data[l1_miss]

    if l2_req in data.columns and l2_miss in data.columns:
        data[l2_hit] = data[l2_req] * ( 1 - data[l2_miss])
        data[l3_req] = data[l2_req] * data[l2_miss]

    if l3_req in data.columns and l3_miss in data.columns:
        data[l3_hit] = data[l3_req] * ( 1 - data[l3_miss])
        data[dram_req] = data[l3_req] * data[l3_miss]


#print(data)
data.to_csv('mini_perf.csv')
