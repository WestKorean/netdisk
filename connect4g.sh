#!/bin/bash

cdc_dir=/dev/cdc-wdm0
conf="/var/lib/dhcp/dhclient.leases"
ip="114.114.114.114"
intf_4g=wwan0

if [ -f "$conf" ]; then
    rm -f $conf
fi

while true;
do
    pid=$(ps aux | grep quectel | grep -v grep | awk '{print $2}')
    #thread_num=$(ps axu | grep quectel | grep -v grep | wc -l)
    ip=$(ip add l wwan0 | grep -w "inet")
    
    ### Dev is Ok?
    if [ ! -f "$cdc_dir" ]; then
        echo "[-] Dev not exist. sleep 5sec"
        sleep 5
        continue
    fi

    ### Quectel is running?
    #if [ ! -z "$pid" -a ! -z "$ip" ] 
    if [ -z "$pid" ]; then
        echo "[+] Start Connect."
        /usr/local/bin/quectel-CM
        continue
    fi

    ### Connect is OK?
    for((i = 0; i < 3; i++))
    do
        if [ $(ping -c 3 -w 5 $ip -I$intf_4g | grep -c ttl) -eq 0 ]; then
            echo "[-] Ping is not Ok $i."
		    sleep 2
        fi  

    done

done
