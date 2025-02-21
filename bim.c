#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

#define VERSION "1.2"
#define MIN_MTU 1280
#define MAX_MTU 1500
#define MAX_IFACE_LEN 128
#define MTU_STEP 10
#define PING_TIMEOUT 5 // seconds
#define MAX_RETRIES 3

// Global for cleanup
static volatile bool running = true;

// Function prototypes
void print_banner(void);
void print_usage(void);
bool detect_vpn_interface(char *iface, size_t len);
int get_current_mtu(const char *iface);
bool set_mtu(const char *iface, int mtu);
bool ping_test(int mtu);
void cleanup_handler(int sig);
void log_message(const char *format, ...);

void cleanup_handler(int sig) {
    (void)sig; // Explicitly mark as unused to silence warning
    running = false;
}

void print_banner(void) {
    printf("----------------------------------------------------------------\n");
    printf("Black Ice MTU v%s - Network Optimization Tool\n", VERSION);
    printf("----------------------------------------------------------------\n");
    printf("\"Break through network restrictions by optimizing MTU\"\n\n");
}

void print_usage(void) {
    printf("Usage: bim [options]\n");
    printf("Options:\n");
    printf("  -custom <mtu>    Set specific MTU value (%d-%d)\n", MIN_MTU, MAX_MTU);
    printf("  -iface <name>    Specify interface name\n");
    printf("  -h               Show this help message\n");
}

void log_message(const char *format, ...) {
    va_list args;
    time_t now;
    char timestamp[20];
    
    time(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(stderr, "[%s] ", timestamp);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

#ifdef _WIN32
    #include <winsock2.h>
    #include <WS2tcpip.h>
    #include <windows.h>
    #include <iphlpapi.h>
    #include <ctype.h>

    // Windows-specific sleep wrapper (converts seconds to milliseconds)
    #define sleep(seconds) Sleep((seconds) * 1000)

    char* strcasestr_win(const char* haystack, const char* needle) {
        if (!*needle) return (char*)haystack;
        for (; *haystack; haystack++) {
            if (toupper((unsigned char)*haystack) == toupper((unsigned char)*needle)) {
                const char *h = haystack, *n = needle;
                while (*h && *n && toupper((unsigned char)*h) == toupper((unsigned char)*n)) {
                    h++; n++;
                }
                if (!*n) return (char*)haystack;
            }
        }
        return NULL;
    }

    bool ping_test(int mtu) {
        char command[256];
        int payload = mtu - 28;
        snprintf(command, sizeof(command), "ping -n 3 -w %d -f -l %d google.com >nul 2>&1", 
                 PING_TIMEOUT * 1000, payload);
        return system(command) == 0;
    }

    bool detect_vpn_interface(char *iface, size_t len) {
        ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
        ULONG outBufLen = 15000;
        IP_ADAPTER_ADDRESSES *addresses = malloc(outBufLen);
        if (!addresses) {
            log_message("Memory allocation failed: %s", strerror(errno));
            return false;
        }

        DWORD ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addresses, &outBufLen);
        if (ret == ERROR_BUFFER_OVERFLOW) {
            free(addresses);
            addresses = malloc(outBufLen);
            ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addresses, &outBufLen);
        }

        bool found = false;
        if (ret == NO_ERROR) {
            for (IP_ADAPTER_ADDRESSES *adapter = addresses; adapter && running; adapter = adapter->Next) {
                if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                    continue;
                }
                char name[256];
                WideCharToMultiByte(CP_ACP, 0, adapter->FriendlyName, -1, name, sizeof(name), NULL, NULL);
                if (strcasestr_win(name, "nord") || strcasestr_win(name, "vpn") || 
                    strcasestr_win(name, "openvpn") || strcasestr_win(name, "wireguard")) {
                    strncpy_s(iface, len, name, _TRUNCATE);
                    found = true;
                    break;
                }
            }
        }
        free(addresses);
        return found;
    }

    int get_current_mtu(const char *iface) {
        ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
        ULONG outBufLen = 15000;
        IP_ADAPTER_ADDRESSES *addresses = malloc(outBufLen);
        if (!addresses) {
            log_message("Memory allocation failed: %s", strerror(errno));
            return -1;
        }

        DWORD ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addresses, &outBufLen);
        if (ret == ERROR_BUFFER_OVERFLOW) {
            free(addresses);
            addresses = malloc(outBufLen);
            ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addresses, &outBufLen);
        }

        int mtu = -1;
        if (ret == NO_ERROR) {
            for (IP_ADAPTER_ADDRESSES *adapter = addresses; adapter && running; adapter = adapter->Next) {
                char name[256];
                WideCharToMultiByte(CP_ACP, 0, adapter->FriendlyName, -1, name, sizeof(name), NULL, NULL);
                if (strcmp(name, iface) == 0) {
                    mtu = adapter->Mtu;
                    break;
                }
            }
        }
        free(addresses);
        if (mtu == -1) {
            log_message("Failed to get MTU for interface: %s", iface);
        }
        return mtu;
    }

    bool set_mtu(const char *iface, int mtu) {
        char command[256];
        snprintf(command, sizeof(command), "netsh interface ipv4 set subinterface \"%s\" mtu=%d store=persistent", 
                 iface, mtu);
        return system(command) == 0;
    }

#else // Linux and macOS
    #include <ifaddrs.h>
    #include <sys/ioctl.h>
    #include <net/if.h>
    #include <unistd.h>

    bool ping_test(int mtu) {
        char command[256];
        int payload = mtu - 28;
#ifdef __APPLE__
        snprintf(command, sizeof(command), "ping -c 3 -W %d -s %d google.com > /dev/null 2>&1", 
                 PING_TIMEOUT, payload);
#else // Linux
        snprintf(command, sizeof(command), "ping -c 3 -W %d -M do -s %d google.com > /dev/null 2>&1", 
                 PING_TIMEOUT, payload);
#endif
        return system(command) == 0;
    }

    bool detect_vpn_interface(char *iface, size_t len) {
        struct ifaddrs *ifaddr = NULL;
        if (getifaddrs(&ifaddr) == -1) {
            log_message("getifaddrs failed: %s", strerror(errno));
            return false;
        }

        bool found = false;
        const char *names[] = {"nord", "tun", "utun", "vpn", "openvpn", "wireguard"};
        for (struct ifaddrs *ifa = ifaddr; ifa && running; ifa = ifa->ifa_next) {
            if (!ifa->ifa_name) continue;
            for (int i = 0; i < 6; i++) {
                if (strcasestr(ifa->ifa_name, names[i])) {
                    strncpy(iface, ifa->ifa_name, len - 1);
                    iface[len - 1] = '\0';
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        freeifaddrs(ifaddr);
        return found;
    }

    int get_current_mtu(const char *iface) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            log_message("Socket creation failed: %s", strerror(errno));
            return -1;
        }

        struct ifreq ifr = {0};
        strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);
        if (ioctl(sock, SIOCGIFMTU, &ifr) < 0) {
            log_message("ioctl failed for %s: %s", iface, strerror(errno));
            close(sock);
            return -1;
        }
        int mtu = ifr.ifr_mtu;
        close(sock);
        return mtu;
    }

    bool set_mtu(const char *iface, int mtu) {
        char command[256];
#ifdef __APPLE__
        snprintf(command, sizeof(command), "sudo ifconfig %s mtu %d", iface, mtu);
#else // Linux
        snprintf(command, sizeof(command), "sudo ip link set dev %s mtu %d", iface, mtu);
#endif
        return system(command) == 0;
    }
#endif

int main(int argc, char *argv[]) {
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);

    print_banner();
    char vpn_iface[MAX_IFACE_LEN] = {0};
    int custom_mtu = -1;
    bool custom_mode = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_usage();
            return EXIT_SUCCESS;
        }
        else if (strcmp(argv[i], "-custom") == 0 && i + 1 < argc) {
            custom_mtu = atoi(argv[++i]);
            custom_mode = true;
        }
        else if (strcmp(argv[i], "-iface") == 0 && i + 1 < argc) {
            strncpy(vpn_iface, argv[++i], MAX_IFACE_LEN - 1);
            vpn_iface[MAX_IFACE_LEN - 1] = '\0';
        }
        else {
            print_usage();
            return EXIT_FAILURE;
        }
    }

    // Validate custom MTU if specified
    if (custom_mode) {
        if (custom_mtu < MIN_MTU || custom_mtu > MAX_MTU) {
            log_message("Invalid MTU value: %d (must be %d-%d)", custom_mtu, MIN_MTU, MAX_MTU);
            return EXIT_FAILURE;
        }
    }

    // Detect interface if not specified
    if (vpn_iface[0] == '\0') {
        printf("Scanning for VPN interface...\n");
        if (!detect_vpn_interface(vpn_iface, sizeof(vpn_iface))) {
            log_message("No VPN interface detected");
            return EXIT_FAILURE;
        }
    }
    printf("Using interface: %s\n", vpn_iface);

    int current_mtu = get_current_mtu(vpn_iface);
    if (current_mtu <= 0) {
        log_message("Could not retrieve MTU for %s", vpn_iface);
        return EXIT_FAILURE;
    }
    printf("Current MTU: %d\n", current_mtu);

    if (custom_mode) {
        printf("Setting custom MTU: %d\n", custom_mtu);
        int retries = 0;
        while (retries < MAX_RETRIES && running) {
            if (set_mtu(vpn_iface, custom_mtu)) {
                if (ping_test(custom_mtu)) {
                    printf("Successfully set and verified MTU %d\n", custom_mtu);
                    return EXIT_SUCCESS;
                }
                log_message("MTU %d set but ping test failed", custom_mtu);
            }
            retries++;
            if (retries < MAX_RETRIES) {
                sleep(1);
                printf("Retrying (%d/%d)...\n", retries + 1, MAX_RETRIES);
            }
        }
        log_message("Failed to set MTU %d after %d retries", custom_mtu, MAX_RETRIES);
        return EXIT_FAILURE;
    }

    // Auto-optimization mode
    printf("Optimizing MTU (testing from %d to %d)...\n", current_mtu, MIN_MTU);
    for (int mtu = current_mtu; mtu >= MIN_MTU && running; mtu -= MTU_STEP) {
        printf("Testing MTU %d: ", mtu);
        fflush(stdout);

        int retries = 0;
        bool success = false;
        while (retries < MAX_RETRIES && !success && running) {
            success = ping_test(mtu);
            if (!success && retries < MAX_RETRIES - 1) {
                sleep(1);
            }
            retries++;
        }

        if (success) {
            printf("Success\n");
            retries = 0;
            while (retries < MAX_RETRIES && running) {
                if (set_mtu(vpn_iface, mtu)) {
                    printf("Successfully set MTU to %d\n", mtu);
                    return EXIT_SUCCESS;
                }
                retries++;
                if (retries < MAX_RETRIES) {
                    sleep(1);
                    printf("Retrying set (%d/%d)...\n", retries + 1, MAX_RETRIES);
                }
            }
            log_message("Failed to set MTU %d after %d retries", mtu, MAX_RETRIES);
            return EXIT_FAILURE;
        }
        printf("Failed\n");
    }

    log_message("No working MTU found above %d", MIN_MTU);
    return EXIT_FAILURE;
}