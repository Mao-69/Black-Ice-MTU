# Black Ice MTU v1.2
Black Ice MTU is a cross-platform network optimization tool designed to detect and adjust the Maximum Transmission Unit (MTU) for VPN interfaces. It helps bypass network restrictions and improve performance by automatically finding or setting an optimal MTU value. The program supports Windows, Linux, and macOS, offering both automatic optimization and manual MTU configuration via command-line options. Key features include VPN interface detection, ping-based MTU testing with timeout and retry logic, detailed logging, and graceful cleanup on interruption. Ideal for users seeking to overcome VPN throttling or packet fragmentation issues.
```
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
```
```
----------------------------------------------------------------
Black Ice MTU v1.2 - Network Optimization Tool
----------------------------------------------------------------
"Break through network restrictions by optimizing MTU"

Usage: bim [options]
Options:
  -custom <mtu>    Set specific MTU value (1280-1500)
  -iface <name>    Specify interface name
  -h               Show this help message
----------------------------------------------------------------
Scanning for VPN interface...
Using interface: tun0
Current MTU: 1500
Optimizing MTU (testing from 1500 to 1280)...
Testing MTU 1500: Failed
Testing MTU 1490: Failed
Testing MTU 1480: Success
Successfully set MTU to 1480
```

# To compile on Linux systems:
```
gcc -Wall -Wextra -Wno-unused-parameter -o bim bim.c
```
```
sudo ./bim
```
Usage examples:
- Auto-optimize: ```sudo ./bim```
- Custom MTU: ```sudo ./bim -custom 1390```
- Specific interface: ```sudo ./bim -iface tun0```
- Combined: ```sudo ./bim -iface tun0 -custom 1390```
- Help: ```sudo ./bim -h```
# To compile on Windows:
```
gcc -Wall -Wextra -o bim.exe bim.c -liphlpapi -lws2_32
```
Usage examples:
- Run cmd as sys admin
- Auto-optimize: ```bim```
- Custom MTU: ```bim -custom 1390```
- Specific interface: ```bim -iface tun0```
- Combined: ```bim -iface tun0 -custom 1390```
- Help: ```bim -h```
# To compile on Mac OS:
```
clang -Wall -Wextra -o bim bim.c
```
Usage examples:
- Auto-optimize: ```sudo ./bim```
- Custom MTU: ```sudo ./bim -custom 1390```
- Specific interface: ```sudo ./bim -iface tun0```
- Combined: ```sudo ./bim -iface tun0 -custom 1390```
- Help: ```sudo ./bim -h```
