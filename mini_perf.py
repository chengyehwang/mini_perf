#!/usr/bin/env python3
import numpy as np
import pandas as pd
import re
import glob
name = []
filename = glob.glob('*mini_perf.head')[0]
with open(filename, 'r') as f:
    for line in f.readlines():
        m = re.search('count:\s*(\S+)',line)
        if m:
            name.append(m.group(1))
#print(name)
def read_file():
    file_data = filename.replace('.head','.data')
    array = []
    with open(file_data, 'rb') as f:
        data = np.fromfile(f, dtype='int64')
        nx = len(name)
        ny = int(len(data)/nx)
        #print('matrix', nx, ny, len(data))
        array = np.reshape(data, [ny, nx])
    return array

counter = read_file()
#print(counter)
data = pd.DataFrame(counter,columns=name)

data = data.diff()

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

        count_inst = 'raw-inst-retired' + group + cpu
        count_i = 'raw-l1i-cache' + group + cpu
        count_d = 'raw-l1d-cache' + group + cpu
        count_i_req = 'l1i-cache-req' + cpu
        count_d_req = 'l1d-cache-req' + cpu
        if count_inst in data.columns and count_i in data.columns:
            data[count_i_req] = data[count_i] / data[count_inst]
        if count_inst in data.columns and count_d in data.columns:
            data[count_d_req] = data[count_d] / data[count_inst]

for cpu in range(8):
    cpu = '_cpu%d' % cpu
    l1i_miss = 'l1i-cache-miss' + cpu
    l1d_miss = 'l1d-cache-miss' + cpu
    l2_miss = 'l2d-cache-miss' + cpu
    l3_miss = 'l3d-cache-miss' + cpu
    l1_hit = 'l1-cache-hit' + cpu
    l2_hit = 'l2-cache-hit' + cpu
    l3_hit = 'l3-cache-hit' + cpu
    dram_hit = 'dram-cache-hit' + cpu
    l1i_req = 'l1i-cache-req' + cpu
    l1d_req = 'l1d-cache-req' + cpu
    l2_req = 'l2-cache-req' + cpu
    l3_req = 'l3-cache-req' + cpu

    if l1i_req in data.columns and l1d_req in data.columns and l1i_miss in data.columns and l1d_miss in data.columns:
        data[l1_hit] = 1 - ( data[l1i_req] * data[l1i_miss] + data[l1d_req] * data[l1d_miss] )
        data[l2_req] = 1 - data[l1_hit]

    if l2_req in data.columns and l2_miss in data.columns:
        data[l2_hit] = data[l2_req] (1 - data[l2_miss])
        data[l3_req] = data[l2_req] * data[l2_miss]

    if l3_req in data.columns and l3_miss in data.columns:
        data[l3_hit] = data[l3_req] (1 - data[l3_miss])
        data[dram_hit] = data[l3_req] * data[l3_miss]


#print(data)
file_excel = filename.replace('.head','.xlsx')
data.to_excel(file_excel)
