#!/bin/sh
[[ ! -z $(nvram get wg_debug) ]] && { DEBUG=1; set -x; }
{
nv=/usr/sbin/nvram
/bin/mkdir -p /tmp/oet/pid
cd /tmp/oet/pid
rmmod eoip >/dev/null 2>&1
tunnels=$($nv get oet_tunnels)
WAN_IF=$(get_wanface)
IP=ip
IP6=echo
WGDELRT=/tmp/wgdelrt
ipv6_en=$($nv get ipv6_enable)
if [[ $ipv6_en -eq 1 ]]; then 
    IP6="ip -6"
fi
GATEWAY="$($nv get wan_gateway)"
MINTIME=$($nv get wg_mintime)
[[ -z $MINTIME ]] && MINTIME=0
MAXTIME=$($nv get wg_maxtime) #0 = no maxtime
[[ -z $MAXTIME ]] && MAXTIME=105
# Needs checking
[[ $($nv get wan_proto) = "disabled" ]] && { GATEWAY="$($nv get lan_gateway)"; logger -p user.info "WireGuard no wan_gateway detected, assuming WAP"; }
GATEWAY6="$($IP6 route show table main | awk '/default via/ { print $3;exit; }')"
[[ $($nv get wan_proto) = "disabled" ]] && { GATEWAY="$($nv get lan_gateway)"; logger -p user.info "WireGuard no wan_gateway detected, assuming WAP"; }
# set failstate, 0 is default meaning tunnel can start (at (re)boot failstate of all tunnels is set to 0, sysinit.c:3386), 1 is failed, set to 2 if this is the one to start
LOCK="/tmp/oet-tunnels.lock"
is_eop_active() {
	for i in $(seq 1 $tunnels); do
		if [[ "$($nv get oet${i}_en)" -eq 1 ]]; then
			return 0
			break
		fi
	done
	return 1
}
temp-killswitch() {
	KS=0
	KS6=0
	for i in $(seq 1 $tunnels); do
		if [[ $($nv get oet${i}_killswitch) -eq 1 ]]; then
			KS=1
			if [[ "$ipv6_en" = "1" ]]; then
				KS6=1
			fi
			break
		fi
	done
	if [[ "$KS" = "1" ]]; then
		sysctl -w net.ipv4.ip_forward=0 >/dev/null
		logger -p user.info "WireGuard setting temporary killswitch for IPv4"
	fi
	if [[ "$KS6" = "1" ]]; then
		sysctl -w net.ipv6.conf.all.forwarding=0 >/dev/null
		logger -p user.info "WireGuard setting temporary killswitch for IPv6"
	fi
}
waitfortime () {
	#debug todo check if nvram get ntp_succes and/or ntp_done can be used to see if time is set
	#logger -p user.info "WireGuard debug st start of time check: ntp_success: $($nv get ntp_success); ntp_done:$($nv get ntp_done)"
	SLEEPCT=$MINTIME
	sleep $MINTIME
	#while [[ $($nv get ntp_done) -ne 1 ]]; do
	while [[ $(date +%Y) -lt 2025 ]]; do
		sleep 2
		SLEEPCT=$((SLEEPCT+2))
		if [[ $SLEEPCT -gt $MAXTIME && $MAXTIME -ne 0 ]]; then
			break
		fi
	done
	if [[ $SLEEPCT -gt $MAXTIME && $MAXTIME -ne 0 ]]; then
		logger -p user.err "WireGuard ERROR: stopped waiting after $SLEEPCT seconds, trying to set routes for oet${i} anyway, is there a connection or NTP problem?"
		# set date in future to help WG connect anyway
		date -s 2030-01-01
	else
		logger -p user.info "WireGuard waited $SLEEPCT seconds to set routes for oet${i}"
	fi
	#debug
	#logger -p user.info "WireGuard debug at end of time check: ntp_success: $($nv get ntp_success); ntp_done:$($nv get ntp_done) "
}

acquire_lock() { 
	logger -p user.info "WireGuard acquiring $LOCK for prewall $$ "
	local SLEEPCT=0
	local MAXTIME=60
	while ! mkdir $LOCK >/dev/null 2>&1; do
		sleep 2
		SLEEPCT=$((SLEEPCT+2))
		if [[ $SLEEPCT -gt $MAXTIME && $MAXTIME -ne 0 ]]; then
			logger -p user.err "WireGuard ERROR max. waiting $SLEEPCT sec. for locking prewall $$!"
			break
		fi
	done 
	logger -p user.info "WireGuard $LOCK acquired for prewall $$ " 
}
release_lock() { rmdir $LOCK >/dev/null 2>&1; logger -p user.info "WireGuard released $LOCK for prewall $$ "; }
trap '{ release_lock; logger -p user.info "WireGuard script $0 running on oet${i} fatal error"; exit 1; }' SIGHUP SIGINT SIGTERM

#sleep "$(nvram get wgp_mintime)"

if is_eop_active; then
	acquire_lock
	temp-killswitch
	waitfortime
	if [[ $tunnels -gt 0 ]]; then
		fset=0
		for i in $(seq 1 $tunnels); do
			[[ -z $($nv get oet${i}_failgrp) ]] && $nv set oet${i}_failgrp=0 #add missing default delete in future version
			if [[ $($nv get oet${i}_en) -eq 1 && $($nv get oet${i}_failgrp) -eq 1 && $($nv get oet${i}_failstate) -eq 2  ]]; then
				fset=$((fset+1))
				logger -p user.info "WireGuard tunnel oet${i} is the fail over group start tunnel"
			fi
		done
		for i in $(seq 1 $tunnels); do
			if [[ $($nv get oet${i}_en) -ne 1 ]]; then #non enabled tunnels reset
				$nv set oet${i}_failstate=0
			else #enabled tunnels
				if [[ $($nv get oet${i}_failgrp) -eq 1 ]]; then
					#search for first tunnel with failgroup enabled which is not failed and set to 2
					if [[ $($nv get oet${i}_failstate) -eq 0 ]]; then #non failed tunnels
						if [[ $fset -eq 0 ]]; then
							fset=$((fset+1))
							#set this tunnel to start the fail over group
							logger -p user.info "WireGuard tunnel oet${i} is the fail over group start tunnel"
							$nv set oet${i}_failstate=2
						else  #number of failgroup running or standby
							fset=$((fset+1))
							#$nv set oet${i}_failstate=0
						fi
					fi
				else #enabled tunnels not part of fail group reset
					$nv set oet${i}_failstate=0
				fi
			fi
		done
		logger -p user.info "WireGuard number of non failed tunnels in fail set: $fset"
	fi
fi
#reset dns
if [[ -e /tmp/resolv.dnsmasq_oet ]]; then
	logger -p user.info "WireGuard DNS reset"
	cp -f /tmp/resolv.dnsmasq_oet /tmp/resolv.dnsmasq
	rm -f /tmp/resolv.dnsmasq_oet
fi
$nv unset wg_get_dns
#remove route to endpoint
if [[ -f "$WGDELRT" ]]; then
	(while read route; do $route >/dev/null 2>&1; done < $WGDELRT)
	rm $WGDELRT
fi
for i in $(seq 1 $tunnels); do
	if [[ -e "${i}.pid" ]]; then
		#Save ipset file
		[[ "$($nv get oet${i}_ipsetsave)" = "1" && "$($nv get oet${i}_dpbr)" != "0" ]] && { ipset save -! > $($nv get oet${i}_ipsetfile); } >/dev/null 2>&1
		{
		emf del iface $(getbridge oet${i}) oet${i}
		brctl delif $(getbridge oet${i}) oet${i}
		brctl delif $(getbridge oet${i}) vxlan${i}
		$IP tunnel del oet${i}
		#$IP link set oet${i} down
		#$IP link del oet${i}
		[[ $($nv get oet${i}_proto) -eq 2 ]] && { $IP link set oet${i} down; } || { $IP link del oet${i}; }
		$IP link del vxlan${i}
		rm -f ${i}.pid
		} >/dev/null 2>&1
		TID=$((20+i))
		while $IP rule delete from 0/0 to 0/0 table $TID >/dev/null 2>&1; do true; done
		$IP route flush table $TID
		if [[ $ipv6_en -eq 1 ]]; then 
			while $IP6 rule delete from ::/0 to ::/0 table $TID >/dev/null 2>&1; do true; done
			$IP6 route show table all | grep "table $TID" | while read v6r; do
				$IP6 route del ${v6r%error*} >/dev/null 2>&1
			done
		fi
		logger -p user.info "Flush delete PBR interface oet${i}, table : $TID"
		ps | grep "[w]ireguard-fwatchdog" | awk '{print $1}' | xargs kill -9 >/dev/null 2>&1 #use [w] to exclude the ps line
		[[ ! -z "$($nv get oet${i}_rtdownscript | sed '/^[[:blank:]]*#/d')" ]] && { sh $($nv get oet${i}_rtdownscript) & }
		# remove destination routes
		WGDPBRIP="/tmp/wgdpbrip_oet${i}"
		if [[ -f "$WGDPBRIP" ]]; then
			(while read dpbrip; do $IP route del $dpbrip >/dev/null 2>&1; done < $WGDPBRIP)
			rm $WGDPBRIP >/dev/null 2>&1
		fi
		WGDNSRT="/tmp/wgdnsrt_oet${i}"
		if [[ -f "$WGDNSRT" ]]; then
			(while read dnsrt; do $dnsrt >/dev/null 2>&1; done < $WGDNSRT)
			rm $WGDNSRT
		fi
		#IPSET flush table delete rule
		for z in $(seq 1 2); do
			$IP route flush table $z$TID >/dev/null 2>&1
			$IP rule del table $z$TID fwmark $TID >/dev/null 2>&1
			if [[ "$ipv6_en" = "1" ]]; then
				$IP6 route flush table $z$TID >/dev/null 2>&1
				$IP6 rule del table $z$TID fwmark $TID >/dev/null 2>&1
			fi
		done
		# remove MAC in PBR
		WGMACRT="/tmp/wg_mac_oet${i}"
		rm $WGMACRT >/dev/null 2>&1
		#TODO destroy IPSET set ipset -X $IPSET and if IPv6 ipset -X ${IPSET}6
		$IP route flush cache
		if [[ $ipv6_en -eq 1 ]]; then 
			$IP6 route flush cache >/dev/null 2>&1
		fi
	fi
done
for i in $(seq 1 $tunnels); do
	if [[ $($nv get oet${i}_en) -eq 1 ]]; then
		if [[ $($nv get oet${i}_proto) -eq 2 ]] && [[ $($nv get oet${i}_failgrp) -ne 1 || $($nv get oet${i}_failstate) -eq 2 ]]; then
			{
			mkdir -p /tmp/wireguard
			modprobe wireguard
			insmod ipv6
			insmod udp_tunnel
			insmod ip6_udp_tunnel
			insmod ip_tunnel
			insmod wireguard
			} >/dev/null 2>&1
			logger -p user.info "Enable WireGuard interface oet${i} on port $($nv get oet${i}_port)"
			if [ -z "$($nv get oet${i}_mtu)" ]; then
				overhead=60
				if [ "$($nv get ipv6_enable)" == "1" ]; then
					overhead=80
				fi
				if [ "$($nv get wan_proto)" != "disabled" ]; then
					$nv set oet${i}_mtu=$(($($nv get wan_mtu) - $overhead))
				else
					$nv set oet${i}_mtu=$((1500 - $overhead))
				fi
			fi
			$IP link add oet${i} type wireguard >/dev/null 2>&1
			wg set oet${i} listen-port $($nv get oet${i}_port)
			$nv get oet${i}_private > /tmp/wireguard/oet${i}_private
			wg set oet${i} private-key /tmp/wireguard/oet${i}_private
			$nv set oet${i}_public="$($nv get oet${i}_private|wg pubkey)"
			if [[ ! -z "$($nv get oet${i}_fwmark)" ]]; then
				wg set oet${i} fwmark $($nv get oet${i}_fwmark)
			fi
			$nv set oet${i}_bridged=0
			$nv set oet${i}_nat=0
			# consider moving this peers section to eop-tunnel-raip
			peers=$(($($nv get oet${i}_peers) - 1))
			for p in $(seq 0 $peers); do
				PSKARG1=
				PSKARG2=
				ENDPOINTARG1=
				ENDPOINTARG2=
				ipv4=
				ipv6=
				if [ $($nv get oet${i}_usepsk${p}) -eq 1 ]; then
					$nv get oet${i}_psk${p} > /tmp/wireguard/oet${i}_psk${p}
					PSKARG1="preshared-key" 
					PSKARG2="/tmp/wireguard/oet${i}_psk${p}"
				fi
				if [ $($nv get oet${i}_endpoint${p}) -eq 1 ]; then
					logger -p user.info "Establishing WireGuard tunnel with peer endpoint $($nv get oet${i}_rem${p}):$($nv get oet${i}_peerport${p})"
					ENDPOINTARG1="endpoint"
					endp="$($nv get oet${i}_rem${p} | sed 's/\[//g;s/\]//g')"
					ENDPOINTARG2="$endp:$($nv get oet${i}_peerport${p})"
					if [[ "$endp" != "${endp#*[0-9].[0-9]}" ]]; then
						ipv4=$endp
						logger -p user.info "WireGuard experimental endpoint routing for oet${i} to endpoint $endp:$($nv get oet${i}_peerport${p}) is IPv4: [$endp]"
					elif [[ $ipv6_en -eq 1 && "$endp" != "${endp#*:[0-9a-fA-F]}" ]]; then
						ipv6=$endp
						logger -p user.info "WireGuard experimental endpoint routing for oet${i} to endpoint $endp:$($nv get oet${i}_peerport${p}) is IPv6: [$endp]"
					else
						nsl=$(nslookup "$endp") >/dev/null 2>&1
						ipv4=$(echo "$nsl" | awk '/^Name:/,0 {if (/^Addr[^:]*: [0-9]{1,3}\./) print $3}' | head -n 1)
						[[ $ipv6_en -eq 1 ]] && { ipv6=$(echo "$nsl" | awk '/^Name:/,0 {if (/^Addr[^:]*: [23]...:/) print $3}' | head -n 1); }
						logger -p user.info "WireGuard experimental endpoint routing for oet${i} to endpoint $endp:$($nv get oet${i}_peerport${p}) is Domain Name resolving to IPv4:[$ipv4]; IPv6:[$ipv6]"
					fi
					if [[ ! -z $ipv4 ]]; then 
						$IP route add $ipv4 via $GATEWAY dev $WAN_IF >/dev/null 2>&1
						echo "$IP route del $ipv4 via $GATEWAY dev $WAN_IF " >> $WGDELRT
					fi
					if [[ ! -z $ipv6 ]]; then 
						#IPv6 route needs via nexthop
						$IP6 route add $ipv6 via $GATEWAY6 dev $WAN_IF >/dev/null 2>&1
						echo "$IP6 route del $ipv6 via $GATEWAY6 dev $WAN_IF " >> $WGDELRT
					fi
				fi
				wg set oet${i} peer $($nv get oet${i}_peerkey${p}) persistent-keepalive $($nv get oet${i}_ka${p}) $PSKARG1 $PSKARG2 allowed-ips "$($nv get oet${i}_aip${p})" $ENDPOINTARG1 $ENDPOINTARG2 &
			done
			# end peer section
			ifconfig oet${i} promisc up >/dev/null 2>&1
			$IP link set dev oet${i} mtu $($nv get oet${i}_mtu) up >/dev/null 2>&1
			echo enable > ${i}.pid
			# alternative input with ipaddrmask
			if [[ ! -z "$($nv get oet${i}_ipaddrmask)" ]]
			then
				for ipaddrmask in $($nv get oet${i}_ipaddrmask | sed "s/,/ /g") ; do
					# set ip address and netmask for backwards compatibility only for IPv4
					case $ipaddrmask in
					  *.*) #IPv4
						IPADDR=${ipaddrmask%%/*}
						mask="${ipaddrmask#*/}"
						# error handling if netmask is not specified
						if [[ $mask -lt 33 ]] ;then
							mask="${ipaddrmask#*/}"
						else
							logger -p user.err "ERROR: WireGuard no valid tunnel address for oet${i}: $ipaddrmask, please correct, using /24"
							mask=24
						fi
						$IP addr add $IPADDR/$mask dev oet${i} >/dev/null 2>&1
						logger -p user.info "WireGuard $IPADDR/$mask added to oet${i}"
						set -- $(( 5 - ($mask / 8) )) 255 255 255 255 $(( (255 << (8 - ($mask % 8))) & 255 )) 0 0 0
						[ $1 -gt 1 ] && shift $1 || shift
						NETMASK=${1-0}.${2-0}.${3-0}.${4-0}
						$nv set oet${i}_ipaddr=$IPADDR
						$nv set oet${i}_netmask=$NETMASK
						;;
					  *:*) #IPv6
						if [[ "$ipv6_en" = "1" ]]; then
							$IP6 addr add $ipaddrmask dev oet${i} >/dev/null 2>&1
							logger -p user.info "WireGuard $ipaddrmask added to oet${i}"
						fi
						;;
					  *)
						logger -p user.err "ERROR: WireGuard no valid IPv4 or IPV6 tunnel address for oet${i}: $ipaddrmask, please correct"
						;;
					esac
				done
			else
				logger -p user.err "ERROR: WireGuard no valid IPv4 or IPV6 tunnel address for oet${i}: $ipaddrmask, please correct, trying the old way"
				$IP addr add $($nv get oet${i}_ipaddr)/$(getmask $($nv get oet${i}_netmask)) dev oet${i} >/dev/null 2>&1
			fi
		fi
		if [ $($nv get oet${i}_proto) -eq 1 ]; then
			insmod gre
			insmod eoip
			logger -p user.info "Enable Mikrotik Tunnel interface oet${i} on address $($nv get oet${i}_local) with peer $($nv get oet${i}_rem)"
			eoip add tunnel-id $($nv get oet${i}_id) name oet${i} remote $($nv get oet${i}_rem) local $($nv get oet${i}_local)
			if [ $($nv get oet${i}_bridged) -eq 1 ]; then
				ifconfig oet${i} up >/dev/null 2>&1
				ifconfig oet${i} promisc >/dev/null 2>&1
				brctl addif $(getbridge oet${i}) oet${i} >/dev/null 2>&1
				setportprio $(getbridge oet${i}) oet${i}
				emf add iface $(getbridge oet${i}) oet${i} >/dev/null 2>&1
			else
				ifconfig oet${i} promisc up >/dev/null 2>&1
				$IP addr add $($nv get oet${i}_ipaddr)/$(getmask $($nv get oet${i}_netmask)) dev oet${i} >/dev/null 2>&1
			fi
			echo enable > ${i}.pid
		fi
		if [ $($nv get oet${i}_proto) -eq 0 ]; then
			insmod etherip
			if [ "$($nv get oet${i}_local)" == "0.0.0.0" ]; then
				logger -p user.info "Enable RFC 3378 EtherIP Tunnel interface oet${i} with peer $($nv get oet${i}_rem)"
				$IP tunnel add oet${i} mode etherip remote $($nv get oet${i}_rem) local any
			else
				logger -p user.info "Enable RFC 3378 EtherIP Tunnel interface oet${i} on address $($nv get oet${i}_local) with peer $($nv get oet${i}_rem)"
				$IP tunnel add oet${i} mode etherip remote $($nv get oet${i}_rem) local $($nv get oet${i}_local)
			fi
			if [ $($nv get oet${i}_bridged) -eq 1 ]; then
				$IP link set dev oet${i} up
				ifconfig oet${i} up >/dev/null 2>&1
				ifconfig oet${i} promisc >/dev/null 2>&1
				brctl addif $(getbridge oet${i}) oet${i} >/dev/null 2>&1
				setportprio $(getbridge oet${i}) oet${i}
				emf add iface $(getbridge oet${i}) oet${i} >/dev/null 2>&1
			else
				$IP link set dev oet${i} up
				ifconfig oet${i} promisc >/dev/null 2>&1
				$IP addr add $($nv get oet${i}_ipaddr)/$(getmask $($nv get oet${i}_netmask)) dev oet${i} >/dev/null 2>&1
    			fi
			echo enable > ${i}.pid
		fi
		# vxlan
		if [ $($nv get oet${i}_proto) -eq 3 ]; then
			insmod udp_tunnel
			insmod ip6_udp_tunnel
			insmod vxlan
			logger -p user.info "Enable VXLAN id $($nv get oet${i}) interface vxlan${i}"
			VXLANARG=""
			# detect ipv6 addr handling
			if [[ ! -z "$($nv get oet${i}_mcast)" && "$($nv get oet${i}_mcast)" == "1" ]]; then
				[[ ! -z $($nv get oet${i}_remip6) ]] && if [[ "$(expr $($nv get oet${i}_group) : '.*')" -ge 16 ]]; then
					VXLANARG="$VXLANARG -6"
				fi
			else
				[[ ! -z $($nv get oet${i}_remip6) ]] && if [[ "$(expr $($nv get oet${i}_remip6) : '.*')" -ge 16 ]]; then
					VXLANARG="$VXLANARG -6"
				fi	
				[[ ! -z $($nv get oet${i}_localip6) ]] && if [[ "$(expr $($nv get oet${i}_localip6) : '.*')" -ge 16 ]]; then
					VXLANARG="$VXLANARG -ip6"
				fi
			fi
			VXLANARG="$VXLANARG link add vxlan${i} type vxlan id $($nv get oet${i}_id)"
			if [[ ! -z "$($nv get oet${i}_mcast)" && "$($nv get oet${i}_mcast)" == "1" ]]; then
				VXLANARG="$VXLANARG group $($nv get oet${i}_group)"
			else
				VXLANARG="$VXLANARG remote $($nv get oet${i}_remip6)"
			fi
			if [[ ! -z "$($nv get oet${i}_localip6)" ]]; then
				VXLANARG="$VXLANARG local $($nv get oet${i}_localip6)"
			fi
			if [[ ! -z "$($nv get oet${i}_dev)" && "$($nv get oet${i}_dev)" != "any" ]]; then
				VXLANARG="$VXLANARG dev $($nv get oet${i}_dev)"
			fi
			if [[ ! -z "$($nv get oet${i}_dstport)" ]]; then
				VXLANARG="$VXLANARG dstport $($nv get oet${i}_dstport)"
			else
				# defaults to 4789
				VXLANARG="$VXLANARG dstport 0"
			fi
			if [[ ! -z "$($nv get oet${i}_srcport)" && "$($nv get oet${i}_srcmin)" != "0" && "$($nv get oet${i}_srcmax)" != "0" ]]; then
				VXLANARG="$VXLANARG srcport $($nv get oet${i}_srcmin) $($nv get oet${i}_srcmax)"
			fi
			if [[ ! -z "$($nv get oet${i}_ttl)" && "$($nv get oet${i}_ttl)" != "0" ]]; then			
				VXLANARG="$VXLANARG ttl $($nv get oet${i}_ttl)"
			fi
			if [[ ! -z "$($nv get oet${i}_tos)" && "$($nv get oet${i}_tos)" != "0" ]]; then			
				VXLANARG="$VXLANARG tos $($nv get oet${i}_tos)"
			fi
			if [[ ! -z "$($nv get oet${i}_lrn)" && "$($nv get oet${i}_lrn)" == "1" ]]; then			
				VXLANARG="$VXLANARG learning"
			else
				VXLANARG="$VXLANARG nolearning"
			fi
			if [[ ! -z "$($nv get oet${i}_proxy)" && "$($nv get oet${i}_proxy)" == "1" ]]; then			
				VXLANARG="$VXLANARG proxy"
			else
				VXLANARG="$VXLANARG noproxy"
			fi
			if [[ ! -z $($nv get oet${i}_rsc) && "$($nv get oet${i}_rsc)" == "1" ]]; then			
				VXLANARG="$VXLANARG rsc"
			else
				VXLANARG="$VXLANARG norsc"
			fi
			if [[ ! -z "$($nv get oet${i}_l2miss)" && "$($nv get oet${i}_l2miss)" == "1" ]]; then			
				VXLANARG="$VXLANARG l2miss"
			else
				VXLANARG="$VXLANARG nol2miss"
			fi
			if [[ ! -z "$($nv get oet${i}_l3miss)" && "$($nv get oet${i}_l3miss)" == "1" ]]; then			
				VXLANARG="$VXLANARG l3miss"
			else
				VXLANARG="$VXLANARG nol3miss"
			fi
			if [[ ! -z "$($nv get oet${i}_udpcsum)" && "$($nv get oet${i}_udpcsum)" == "1" ]]; then			
				VXLANARG="$VXLANARG udpcsum"
			else
				VXLANARG="$VXLANARG noudpcsum"
			fi
			if [[ ! -z "$($nv get oet${i}_udp6zerocsumtx)" && "$($nv get oet${i}_udp6zerocsumtx)" == "1" ]]; then			
				VXLANARG="$VXLANARG udp6zerocsumtx"
			else
				VXLANARG="$VXLANARG noudp6zerocsumtx"
			fi
			if [[ ! -z "$($nv get oet${i}_udp6zerocsumrx)" && "$($nv get oet${i}_udp6zerocsumrx)" == "1" ]]; then			
				VXLANARG="$VXLANARG udp6zerocsumrx"
			else
				VXLANARG="$VXLANARG noudp6zerocsumrx"
			fi
			if [[ ! -z "$($nv get oet${i}_ageing)" ]]; then			
				VXLANARG="$VXLANARG ageing $($nv get oet${i}_ageing)"
			fi
			if [[ ! -z "$($nv get oet${i}_df)" && "$($nv get oet${i}_df)" == "1" ]]; then			
				VXLANARG="$VXLANARG df"
			fi
			[[ ! -z $($nv get oet${i}_fl) ]] && if [[ "$($nv get oet${i}_fl)" -ge 0 ]]; then			
				VXLANARG="$VXLANARG flowlabel $($nv get oet${i}_fl)"
			fi
			if [[ ! -z "$($nv get oet${i}_gbp)" && "$($nv get oet${i}_gbp)" == "1" ]]; then			
				VXLANARG="$VXLANARG gbp"
			fi
			echo $IP $VXLANARG
			$IP $VXLANARG
			if [ $($nv get vxlan${i}_bridged) -eq 1 ]; then
				ifconfig vxlan${i} up >/dev/null 2>&1
				ifconfig vxlan${i} promisc >/dev/null 2>&1
				brctl addif $(getbridge vxlan${i}) vxlan${i} >/dev/null 2>&1
				setportprio $(getbridge vxlan${i}) vxlan${i}
				emf add iface $(getbridge vxlan${i}) vxlan${i} >/dev/null 2>&1
			else
				ifconfig vxlan${i} promisc up >/dev/null 2>&1
				$IP addr add $($nv get vxlan${i}_ipaddr)/$(getmask $($nv get vxlan${i}_netmask)) dev vxlan${i} >/dev/null 2>&1
			fi
			echo enable > ${i}.pid
		fi
	fi
done
service set_routes start
# start route setting only if active tunnels
if is_eop_active; then
	sleep $MINTIME
	/usr/bin/eop-tunnel-raip.sh $fset &
	#( nohup /usr/bin/eop-tunnel-raip.sh $fset & ) >/dev/null
else  #only run firewall to clean up
	/usr/bin/eop-tunnel-firewall.sh &
fi
#release_lock
} 2>&1 | logger $([ ${DEBUG+x} ] && echo '-p user.debug') \
    -t $(echo $(basename $0) | grep -Eo '^.{0,23}')[$$] &
