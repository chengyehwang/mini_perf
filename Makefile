host:
	g++ -g -DHOST=1 mini_perf.cpp -o mini_perf
	gcc -DHOST=1 lib_mem.c lib_timing.c lat_mem_rd.c -o lat_mem_rd
	sudo ./mini_perf --group instructions --interval 10 --duration 10 -a ./sleep.sh
	./mini_perf.py
export NDK_PROJECT_PATH := $(PWD)
build:
	./android-ndk-r21/ndk-build NDK_APPLICATION_MK=./Application.mk
	cp obj/local/arm64-v8a/mini_perf.out ./mini_perf.exe
	cp obj/local/arm64-v8a/lat_mem_rd.out ./lat_mem_rd.exe
local:
	adb push mini_perf.exe /data/local/tmp
	adb push sleep.sh /data/local/tmp
	#adb shell /data/local/tmp/mini_perf.exe --cache3 -u /data/local/tmp/sleep.sh
	adb shell su -c '/data/local/tmp/mini_perf.exe --cache3 /data/local/tmp/sleep.sh'
	adb pull /data/local/tmp/mini_perf.head
	adb pull /data/local/tmp/mini_perf.data
	./mini_perf.py
run:
	/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe -Command ". .\run_systrace.ps1"
ndk:
	wget https://dl.google.com/android/repository/android-ndk-r21-linux-x86_64.zip
	unzip android-ndk-r21-linux-x86_64.zip
clean:
	-rm -r obj libs test.exe *.out *.err *.dasm *.pb

perfetto:
	wget http://get.perfetto.dev/trace_processor
	chmod 755 trace_processor
docker:
	docker pull ubuntu:19.10
lala:
	docker run ubuntu:19.10 apt install wget
	docker run ubuntu:19.10 wget http://get.perfetto.dev/trace_processor
	docker run ubuntu:19.10 chmod 755 trace_processor
	docker run -i -t ubuntu:19.10
pmu:
	wget https://android.googlesource.com/platform/system/extras/+archive/master/simpleperf.tar.gz

latency:

