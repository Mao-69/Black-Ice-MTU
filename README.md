# Black-Ice-MTU
auto set mtu for networks who limit vpn usage
```
######                                 ###                  #     # ####### #     #
#     # #        ##    ####  #    #     #   ####  ######    ##   ##    #    #     #
#     # #       #  #  #    # #   #      #  #    # #         # # # #    #    #     #
######  #      #    # #      ####       #  #      #####     #  #  #    #    #     #
#     # #      ###### #      #  #       #  #      #         #     #    #    #     #
#     # #      #    # #    # #   #      #  #    # #         #     #    #    #     #
######  ###### #    #  ####  #    #    ###  ####  ######    #     #    #     #####
```
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
