mini_perf:
	g++ perf_core.cpp -o mini_perf
	sudo ./mini_perf
export NDK_PROJECT_PATH := $(PWD)
build:
	./android-ndk-r20b/ndk-build NDK_APPLICATION_MK=./Application.mk
	cp obj/local/arm64-v8a/mini_perf.out ./mini_perf.exe
run:
	/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe -Command ". .\run_systrace.ps1"
ndk:
	wget https://dl.google.com/android/repository/android-ndk-r20b-linux-x86_64.zip
	unzip android-ndk-r20b-linux-x86_64.zip
clean:
	-rm -r obj libs test.exe *.out *.err *.dasm *.pb



