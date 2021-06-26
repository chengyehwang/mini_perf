host:
	g++ -DHOST=1 mini_perf.cpp -o mini_perf
	sudo ./mini_perf
	./mini_perf.py
export NDK_PROJECT_PATH := $(PWD)
build:
	./android-ndk-r21/ndk-build NDK_APPLICATION_MK=./Application.mk
	cp obj/local/arm64-v8a/mini_perf.out ./mini_perf.exe
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

