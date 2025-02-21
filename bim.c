#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Black Ice MTU version 1.0
// - soCx

// A program to auto detect and change the MTU(maximum transmission unit)
// for VPN's running on networks who throttle or block VPN traffic.

// Lowering the MTU on a VPN interface primarily helps by:
// Ensuring encapsulated packets remain
// within acceptable size limits for the underlying network,
// thereby avoiding unintended fragmentation or drops.
// Reducing the likelihood of detection by network DPI systems
// that look for the typical signatures of VPN traffic.
// Minimizing performance degradation by ensuring that
// all packets are transmitted smoothly without being fragmented excessively.

void print_banner() {
    printf("----------------------------------------------------------------\n");
    printf("▗▄▄▖ ▗▖    ▗▄▖  ▗▄▄▖▗▖ ▗▖    ▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖    ▗▖  ▗▖▗▄▄▄▖▗▖ ▗▖\n");
    printf("▐▌ ▐▌▐▌   ▐▌ ▐▌▐▌   ▐▌▗▞▘      █  ▐▌   ▐▌       ▐▛▚▞▜▌  █  ▐▌ ▐▌\n");
    printf("▐▛▀▚▖▐▌   ▐▛▀▜▌▐▌   ▐▛▚▖       █  ▐▌   ▐▛▀▀▘    ▐▌  ▐▌  █  ▐▌ ▐▌\n");
    printf("▐▙▄▞▘▐▙▄▄▖▐▌ ▐▌▝▚▄▄▖▐▌ ▐▌    ▗▄█▄▖▝▚▄▄▖▐▙▄▄▖    ▐▌  ▐▌  █  ▝▚▄▞▘\n");
    printf("\n");
    printf("    \"The network is a cage for the weak.\n");
    printf("    But for those who know how to shift\n");
    printf("    the strings, the world becomes an open\n");
    printf("    system. Black Ice is the key. The MTU\n");
    printf("    is the path. Break the firewall—reclaim\n");
    printf("    the data.\"\n");
    printf("----------------------------------------------------------------\n");
}

#ifdef _WIN32
  #include <windows.h>
  #include <iphlpapi.h>
  #include <ctype.h>
  #pragma comment(lib, "iphlpapi.lib")

  // A simple case-insensitive substring search for Windows.
  char* strcasestr_win(const char* haystack, const char* needle) {
      if (!*needle)
          return (char*)haystack;
      for (; *haystack; haystack++) {
          if (toupper((unsigned char)*haystack) == toupper((unsigned char)*needle)) {
              const char *h = haystack, *n = needle;
              while (*h && *n && toupper((unsigned char)*h) == toupper((unsigned char)*n)) {
                  h++; n++;
              }
              if (!*n)
                  return (char*)haystack;
          }
      }
      return NULL;
  }

  // Ping test for Windows using system("ping ...")
  int ping_test(int mtu) {
      int payload = mtu - 28;
      char command[256];
      // Windows ping: -n 3 (three echo requests), -f to set "Don't Fragment", -l to specify payload
      sprintf(command, "ping -n 3 -f -l %d google.com >nul 2>&1", payload);
      int ret = system(command);
      return (ret == 0);
  }

  // Detect VPN interface using GetAdaptersAddresses; look for common VPN names.
  int detect_vpn_interface(char *iface, size_t iface_len) {
      ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
      ULONG family = AF_UNSPEC;
      ULONG outBufLen = 15000;
      IP_ADAPTER_ADDRESSES *addresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
      if (!addresses) return 0;
      DWORD ret = GetAdaptersAddresses(family, flags, NULL, addresses, &outBufLen);
      if (ret == ERROR_BUFFER_OVERFLOW) {
          free(addresses);
          addresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
          ret = GetAdaptersAddresses(family, flags, NULL, addresses, &outBufLen);
      }
      int found = 0;
      if (ret == NO_ERROR) {
          IP_ADAPTER_ADDRESSES *adapter = addresses;
          while (adapter) {
              if (adapter->OperStatus == IfOperStatusUp && adapter->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
                  char name[256] = {0};
                  WideCharToMultiByte(CP_ACP, 0, adapter->FriendlyName, -1, name, sizeof(name), NULL, NULL);
                  if (strcasestr_win(name, "nord") || strcasestr_win(name, "vpn") || strcasestr_win(name, "openvpn") || strcasestr_win(name, "wireguard")) {
                      strncpy(iface, name, iface_len - 1);
                      iface[iface_len - 1] = '\0';
                      found = 1;
                      break;
                  }
              }
              adapter = adapter->Next;
          }
      }
      free(addresses);
      return found;
  }

  // Get current MTU for a given interface using GetAdaptersAddresses.
  int get_current_mtu(const char *iface) {
      ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
      ULONG family = AF_UNSPEC;
      ULONG outBufLen = 15000;
      IP_ADAPTER_ADDRESSES *addresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
      if (!addresses) return -1;
      DWORD ret = GetAdaptersAddresses(family, flags, NULL, addresses, &outBufLen);
      if(ret == ERROR_BUFFER_OVERFLOW) {
          free(addresses);
          addresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
          ret = GetAdaptersAddresses(family, flags, NULL, addresses, &outBufLen);
      }
      int mtu = -1;
      if (ret == NO_ERROR) {
          IP_ADAPTER_ADDRESSES *adapter = addresses;
          while (adapter) {
              char name[256] = {0};
              WideCharToMultiByte(CP_ACP, 0, adapter->FriendlyName, -1, name, sizeof(name), NULL, NULL);
              if (strcmp(name, iface) == 0) {
                  mtu = adapter->Mtu;
                  break;
              }
              adapter = adapter->Next;
          }
      }
      free(addresses);
      return mtu;
  }

  // Set MTU using netsh command.
  int set_mtu(const char *iface, int mtu) {
      char command[256];
      sprintf(command, "netsh interface ipv4 set subinterface \"%s\" mtu=%d store=persistent", iface, mtu);
      int ret = system(command);
      return (ret == 0);
  }

#elif defined(__APPLE__)
  #include <ifaddrs.h>
  #include <sys/ioctl.h>
  #include <net/if.h>
  #include <unistd.h>

  // Ping test for macOS.
  int ping_test(int mtu) {
      int payload = mtu - 28;
      char command[256];
      sprintf(command, "ping -c 3 -s %d google.com > /dev/null 2>&1", payload);
      int ret = system(command);
      return (ret == 0);
  }

  // Detect VPN interface on macOS by scanning interface names for common VPN identifiers.
  int detect_vpn_interface(char *iface, size_t iface_len) {
      struct ifaddrs *ifaddr, *ifa;
      if (getifaddrs(&ifaddr) == -1) return 0;
      int found = 0;
      for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
          if (ifa->ifa_name == NULL) continue;
          if (strstr(ifa->ifa_name, "nord") ||
              strstr(ifa->ifa_name, "utun") ||
              strstr(ifa->ifa_name, "vpn") ||
              strstr(ifa->ifa_name, "openvpn") ||
              strstr(ifa->ifa_name, "wireguard")) {
              strncpy(iface, ifa->ifa_name, iface_len - 1);
              iface[iface_len - 1] = '\0';
              found = 1;
              break;
          }
      }
      freeifaddrs(ifaddr);
      return found;
  }

  // Get current MTU for a given interface using ioctl.
  int get_current_mtu(const char *iface) {
      int sock = socket(AF_INET, SOCK_DGRAM, 0);
      if(sock < 0) return -1;
      struct ifreq ifr;
      memset(&ifr, 0, sizeof(ifr));
      strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);
      if(ioctl(sock, SIOCGIFMTU, &ifr) < 0) {
          close(sock);
          return -1;
      }
      close(sock);
      return ifr.ifr_mtu;
  }

  // Set MTU using ifconfig command.
  int set_mtu(const char *iface, int mtu) {
      char command[256];
      sprintf(command, "sudo ifconfig %s mtu %d", iface, mtu);
      int ret = system(command);
      return (ret == 0);
  }

#else  // Assume Linux
  #include <ifaddrs.h>
  #include <sys/ioctl.h>
  #include <net/if.h>
  #include <unistd.h>

  // Ping test for Linux using DF flag (-M do).
  int ping_test(int mtu) {
      int payload = mtu - 28;
      char command[256];
      sprintf(command, "ping -c 3 -M do -s %d google.com > /dev/null 2>&1", payload);
      int ret = system(command);
      return (ret == 0);
  }

  // Detect VPN interface on Linux by scanning interface names for common VPN identifiers.
  int detect_vpn_interface(char *iface, size_t iface_len) {
      struct ifaddrs *ifaddr, *ifa;
      if (getifaddrs(&ifaddr) == -1) return 0;
      int found = 0;
      for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
          if (ifa->ifa_name == NULL) continue;
          if (strcasestr(ifa->ifa_name, "nord") ||
              strcasestr(ifa->ifa_name, "tun") ||
              strcasestr(ifa->ifa_name, "vpn") ||
              strcasestr(ifa->ifa_name, "openvpn") ||
              strcasestr(ifa->ifa_name, "wireguard")) {
              strncpy(iface, ifa->ifa_name, iface_len - 1);
              iface[iface_len - 1] = '\0';
              found = 1;
              break;
          }
      }
      freeifaddrs(ifaddr);
      return found;
  }

  // Get current MTU using ioctl.
  int get_current_mtu(const char *iface) {
      int sock = socket(AF_INET, SOCK_DGRAM, 0);
      if(sock < 0) return -1;
      struct ifreq ifr;
      memset(&ifr, 0, sizeof(ifr));
      strncpy(ifr.ifr_name, iface, IFNAMSIZ-1);
      if(ioctl(sock, SIOCGIFMTU, &ifr) < 0) {
          close(sock);
          return -1;
      }
      close(sock);
      return ifr.ifr_mtu;
  }

  // Set MTU using "ip link set dev" command.
  int set_mtu(const char *iface, int mtu) {
      char command[256];
      sprintf(command, "sudo ip link set dev %s mtu %d", iface, mtu);
      int ret = system(command);
      return (ret == 0);
  }
#endif

// Main program: auto-detect the VPN interface, get its MTU, and test/decrement MTU until a working value is found.
int main() {
    print_banner();
    const int MIN_MTU = 1280; // set lowest limit for the MTU
    char vpn_iface[128] = {0};

    // Detect VPN interface.
    if (!detect_vpn_interface(vpn_iface, sizeof(vpn_iface))) {
        fprintf(stderr, "No VPN interface found. Please ensure your VPN is connected.\n");
        return 1;
    }
    printf("Detected VPN interface: %s\n", vpn_iface);

    int current_mtu = get_current_mtu(vpn_iface);
    if (current_mtu <= 0) {
        fprintf(stderr, "Failed to get current MTU for interface %s.\n", vpn_iface);
        return 1;
    }
    printf("Current MTU: %d\n", current_mtu);
    printf("Starting MTU test from: %d\n", current_mtu);

    int mtu;
    for (mtu = current_mtu; mtu >= MIN_MTU; mtu -= 10) {
        printf("Trying MTU: %d\n", mtu);
        if (ping_test(mtu)) {
            printf("MTU %d works!\n", mtu);
            if (set_mtu(vpn_iface, mtu)) {
                printf("Set MTU to %d for interface %s\n", mtu, vpn_iface);
                return 0;
            } else {
                fprintf(stderr, "Failed to set MTU on interface %s.\n", vpn_iface);
                return 1;
            }
        } else {
            printf("MTU %d failed.\n", mtu);
        }
    }

    fprintf(stderr, "No working MTU found above %d. Please check your network connection.\n", MIN_MTU);
    return 1;
}
