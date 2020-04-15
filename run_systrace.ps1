$dual = 1
$env:Path = "C:\Users\user\AppData\Local\Android\Sdk\platform-tools;" + $env:Path
$env:Path = "C:\Python27\;" + $env:Path
$env:PWD = "\\wsl$\Ubuntu-18.04\home\cywang\mini_perf"
$env:P_PATH= "C:\Users\user\AppData\Local\Android\Sdk\platform-tools\systrace"
Write-Host "pwd $env:PWD"
adb root
adb shell "su -c 'echo 1 > /d/tracing/events/sched/sched_waking/enable'"

adb push ./mini_perf.exe /data/local/tmp/mini_perf.exe
adb shell chmod 755 /data/local/tmp/mini_perf.exe
adb shell "cd /data/local/tmp ; su -c './mini_perf.exe >mini_perf.out 2>mini_perf.err &'"
python $env:P_PATH\systrace.py --time=10 -o $env:PWD\new_trace.html

adb shell "su -c 'killall mini_perf.exe'"
adb pull /data/local/tmp/mini_perf.data mini_perf.data
adb pull /data/local/tmp/mini_perf.out mini_perf.out
adb pull /data/local/tmp/mini_perf.err mini_perf.err

