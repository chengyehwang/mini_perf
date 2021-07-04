#taskset 4 /data/local/tmp/lat_mem_rd.exe -I -R 1048576 4 >/dev/null &
taskset 4 /data/local/tmp/lat_mem_rd.exe -I -R 8192 4 >/dev/null &
pid=$!
sleep 1
/data/local/tmp/mini_perf.exe -p $pid --cache3 --duration 10
kill $pid

