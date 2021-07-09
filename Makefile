latency:
	gcc -g -DHOST=1 lib_mem.c lib_timing.c lat_mem_rd.c -o lat_mem_rd
	./lat_mem_rd 16
run:
	adb push run.sh /data/local/tmp
	adb shell chmod 755 /data/local/tmp/run.sh
	adb shell su -c '/data/local/tmp/run.sh'
	adb pull /data/local/tmp/mini_perf.head
	adb pull /data/local/tmp/mini_perf.data
	adb pull /data/local/tmp/mini_perf.last_data
	./mini_perf.py --last
	./mini_perf.py

host:
	g++ -g -DHOST=1 mini_perf.cpp -o mini_perf
	gcc -DHOST=1 lib_mem.c lib_timing.c lat_mem_rd.c -o lat_mem_rd
	sudo ./mini_perf --group instructions --interval 10 --duration 10 -a ./sleep.sh
	./mini_perf.py
export NDK_PROJECT_PATH := $(PWD)
build:
	./android-ndk-r21e/ndk-build NDK_APPLICATION_MK=./Application.mk
	cp obj/local/arm64-v8a/mini_perf.out ./mini_perf.exe
	cp obj/local/arm64-v8a/lat_mem_rd.out ./lat_mem_rd.exe
	cp obj/local/arm64-v8a/pagemap.out ./pagemap.exe
	cp obj/local/arm64-v8a/fake_loading.out ./fake_loading.exe
	adb push mini_perf.exe /data/local/tmp
	adb push lat_mem_rd.exe /data/local/tmp
	adb push pagemap.exe /data/local/tmp
	adb push fake_loading.exe /data/local/tmp
local:
	-adb shell su -c 'rm /data/local/tmp/mini_perf.head'
	-adb shell su -c 'rm /data/local/tmp/mini_perf.data'
	-adb shell su -c 'rm /data/local/tmp/mini_perf.last_data'
	-adb shell su -c 'rm /data/local/tmp/lat_mem_rd.log'
	-adb shell su -c 'rm /data/local/tmp/lat_mem_rd.err'
	adb push mini_perf.exe /data/local/tmp
	adb push lat_mem_rd.exe /data/local/tmp
	adb push test.sh /data/local/tmp
	adb shell chmod 755 /data/local/tmp/test.sh
	adb shell su -c '/data/local/tmp/test.sh'
	adb pull /data/local/tmp/mini_perf.head
	adb pull /data/local/tmp/mini_perf.data
	adb pull /data/local/tmp/mini_perf.last_data
	adb pull /data/local/tmp/lat_mem_rd.log
	adb pull /data/local/tmp/lat_mem_rd.err
	./mini_perf.py --last
	./mini_perf.py
old:
	/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe -Command ". .\run_systrace.ps1"
ndk:
	wget https://dl.google.com/android/repository/android-ndk-r21e-linux-x86_64.zip
	unzip android-ndk-r21e-linux-x86_64.zip
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

busybox:
	wget https://busybox.net/downloads/binaries/1.31.0-defconfig-multiarch-musl/busybox-armv8l
	chmod 755 busybox-armv8l
	adb push busybox-armv8l /data/local/tmp
pagemap:
	git clone https://github.com/dwks/pagemap.git

lmbench:
	wget https://sourceforge.net/projects/lmbench/files/OldFiles/lmbench-3.0-a3.tgz
	tar zxvf lmbench-3.0-a3.tgz
lmbench_new:
	wget https://sourceforge.net/projects/lmbench/files/development/lmbench-3.0-a9/lmbench-3.0-a9.tgz
	tar zxvf lmbench-3.0-a9.tgz
dasm:
	android-ndk-r21e/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android-objdump -D lat_mem_rd.exe > lat_mem_rd.dasm
