taskset 80 /data/local/tmp/lat_mem_rd.exe -I -R 16777216 16 >/data/local/tmp/lat_mem_rd.log 2>/data/local/tmp/lat_mem_rd.err &
#taskset 4 /data/local/tmp/lat_mem_rd.exe -I -R 1048576 4 >/dev/null &
#taskset 4 /data/local/tmp/lat_mem_rd.exe -I -R 8192 4 >/dev/null &
pid=$!
sleep 1
/data/local/tmp/mini_perf.exe -p $pid --cache3 --duration 10 --interval 100
kill $pid

