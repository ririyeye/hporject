#! /bin/sh

ss_server_ip=149.28.14.216
ss_redir_port=1080
ss_redir_pid=/var/run/shadowsocks.pid
ss_config_file=/etc/shadowsocks-libev/config.json
chnroute_file="/etc/chinadns_chnroute.txt"

# 开启redir
#ss-redir -u -c $ss_config_file -f $ss_redir_pid &
ss-redir -u -s $ss_server_ip -p 443 -b 0.0.0.0 -l 1080 -m aes-256-cfb -k 'vul228228228' -f $ss_redir_pid &
ss-tunnel -u -s $ss_server_ip -p 443 -b 0.0.0.0 -l 5355 -m aes-256-cfb -k 'vul228228228' -L 8.8.8.8:53 -f /tmp/ss-tunnel.pid &
# IP内网地址
BYPASS_RESERVED_IPS=" \
    0.0.0.0/8 \
    10.0.0.0/8 \
    127.0.0.0/8 \
    169.254.0.0/16 \
    172.16.0.0/12 \
    192.168.0.0/16 \
    224.0.0.0/4 \
    240.0.0.0/4 \
"

ipset create ss_bypass_set hash:net

# 添加内网地址到ipset
for line in $BYPASS_RESERVED_IPS; do
    ipset add ss_bypass_set $line
done

# 添加ss地址到ipset
ipset add ss_bypass_set $ss_server_ip

# 添加chinaroute到ipset

for ip in $(cat $chnroute_file); do
  ipset add ss_bypass_set $ip
done


#if [ -f $chnroute_file ] ; then
#    IPS=`which ipset`
#    $IPS -! restore <<-EOF || return 1
#        $(egrep -v "^#|^$" $chnroute_file | sed -e "s/^/add ss_bypass_set /")
#    EOF
#
#    echo "China route was loaded"
#else
#    echo "China route does not exist"
#fi

# TCP规则
iptables -t nat -N SHADOWSOCKS_TCP

iptables -t nat -A SHADOWSOCKS_TCP -p tcp -m set --match-set ss_bypass_set dst -j RETURN
iptables -t nat -A SHADOWSOCKS_TCP -p tcp -j REDIRECT --to-ports $ss_redir_port

# Apply for tcp
iptables -t nat -A OUTPUT -p tcp -j SHADOWSOCKS_TCP

# UDP规则
iptables -t mangle -N SHADOWSOCKS_UDP
iptables -t mangle -N SHADOWSOCKS_UDP_MARK

ip route add local default dev lo table 100
ip rule add fwmark 1 lookup 100

iptables -t mangle -A SHADOWSOCKS_UDP -p udp -m set --match-set ss_bypass_set dst -j RETURN
iptables -t mangle -A SHADOWSOCKS_UDP -p udp -j TPROXY --on-port $ss_redir_port --tproxy-mark 0x01/0x01

iptables -t mangle -A SHADOWSOCKS_UDP_MARK -p udp -m set --match-set ss_bypass_set dst -j RETURN
iptables -t mangle -A SHADOWSOCKS_UDP_MARK -p udp -j MARK --set-mark 1

# Apply for udp
iptables -t mangle -A PREROUTING -p udp -j SHADOWSOCKS_UDP
iptables -t mangle -A OUTPUT -p udp -j SHADOWSOCKS_UDP_MARK

echo "ss-redir is loaded"
