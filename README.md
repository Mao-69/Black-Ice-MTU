# Black-Ice-MTU
auto set mtu for networks who limit vpn usage

▗▄▄▖ ▗▖    ▗▄▖  ▗▄▄▖▗▖ ▗▖    ▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖    ▗▖  ▗▖▗▄▄▄▖▗▖ ▗▖
▐▌ ▐▌▐▌   ▐▌ ▐▌▐▌   ▐▌▗▞▘      █  ▐▌   ▐▌       ▐▛▚▞▜▌  █  ▐▌ ▐▌
▐▛▀▚▖▐▌   ▐▛▀▜▌▐▌   ▐▛▚▖       █  ▐▌   ▐▛▀▀▘    ▐▌  ▐▌  █  ▐▌ ▐▌
▐▙▄▞▘▐▙▄▄▖▐▌ ▐▌▝▚▄▄▖▐▌ ▐▌    ▗▄█▄▖▝▚▄▄▖▐▙▄▄▖    ▐▌  ▐▌  █  ▝▚▄▞▘

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
