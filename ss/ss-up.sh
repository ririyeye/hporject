SOCKS_SERVER=149.28.14.216 # SOCKS 服务器的 IP 地址
PORT=1080
# Setup the ipset
ipset -N chnroute hash:net maxelem 65536

for ip in $(cat '/etc/chinadns_chnroute.txt'); do
  ipset add chnroute $ip
done

# 在nat表中新增一个链，名叫：SHADOWSOCKS
iptables -t nat -N SHADOWSOCKS

ipset add chnroute 0.0.0.0/8
ipset add chnroute 10.0.0.0/8
ipset add chnroute 127.0.0.0/8
ipset add chnroute 169.254.0.0/16
ipset add chnroute 172.16.0.0/12
ipset add chnroute 192.168.0.0/16
ipset add chnroute 224.0.0.0/4
ipset add chnroute 240.0.0.0/4
ipset add chnroute $SOCKS_SERVER

# Allow connection to chinese IPs
iptables -t nat -A SHADOWSOCKS -p tcp -m set --match-set chnroute dst -j RETURN
# 如果你想对 icmp 协议也实现智能分流，可以加上下面这一条
iptables -t nat -A SHADOWSOCKS -p icmp -m set --match-set chnroute dst -j RETURN



# Redirect to Shadowsocks
# 把1081改成你的shadowsocks本地端口
iptables -t nat -A SHADOWSOCKS -p tcp -j REDIRECT --to-port $PORT
# 如果你想对 icmp 协议也实现智能分流，可以加上下面这一条
iptables -t nat -A SHADOWSOCKS -p icmp -j REDIRECT --to-port $PORT

# 将SHADOWSOCKS链中所有的规则追加到OUTPUT链中
iptables -t nat -A OUTPUT -p tcp -j SHADOWSOCKS
# 如果你想对 icmp 协议也实现智能分流，可以加上下面这一条
iptables -t nat -A OUTPUT -p icmp -j SHADOWSOCKS

# 内网流量流经 shadowsocks 规则链
iptables -t nat -A PREROUTING -s 192.168/16 -j SHADOWSOCKS
# 内网流量源NAT
iptables -t nat -A POSTROUTING -s 192.168/16 -j MASQUERADE


ip rule add fwmark 0x01/0x01 table 100
ip route add local 0.0.0.0/0 dev lo table 100
iptables -t mangle -N SSUDP
iptables -t mangle -N SSUDP_MARK


#iptables -t mangle -A SSUDP -p udp --dport 53 -j RETURN
#iptables -t mangle -A SSUDP -p udp --dport 5353 -j RETURN
iptables -t mangle -A SSUDP -p udp -m set --match-set chnroute dst -j RETURN


iptables -t mangle -A SSUDP -p udp -j TPROXY --on-port $PORT --tproxy-mark 0x01/0x01

iptables -t mangle -A SSUDP_MARK -p udp -m set --match-set chnroute dst -j RETURN
iptables -t mangle -A SSUDP_MARK -p udp -j MARK --set-mark 1

iptables -t mangle -A PREROUTING -j SSUDP
iptables -t mangle -A OUTPUT -p udp -j SSUDP_MARK
