#!/bin/bash

iptables -t nat -D OUTPUT -p icmp -j SHADOWSOCKS
iptables -t nat -D OUTPUT -p tcp -j SHADOWSOCKS
iptables -t nat -F SHADOWSOCKS
iptables -t nat -X SHADOWSOCKS

iptables -t mangle -D PREROUTING -p udp -j SSUDP
iptables -t mangle -D OUTPUT -p udp -j SSUDP_MARK


iptables -t mangle -F SSUDP
iptables -t mangle -X SSUDP

iptables -t mangle -F SSUDP_MARK
iptables -t mangle -X SSUDP_MARK



ipset destroy chnroute
