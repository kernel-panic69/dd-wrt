#!/bin/sh
./build_tonze_ap120.sh
./build_compex_wp54.sh
./build_compex_np28g.sh
#./build_x86_gw700.sh
#./build_mikrotik.sh
./build_whrg300n.sh
./build_whrg300n_openvpn.sh
./build_dir600.sh
./build_dir300b.sh
./build_dir615d.sh
./build_dir615h.sh
./build_esr9752sc.sh
./build_acxnr22.sh
./build_ar670w.sh
./build_ar690w.sh
./build_esr6650.sh
#./build_whrg300n_buffalo.sh
./build_ecb9750.sh
./build_br6574n.sh
./build_asus_rtn13u.sh
./build_asus_rtn13ub1.sh
./build_asus_rt10n_plus.sh
./build_wr5422.sh
./build_eap9550.sh
./build_f5d8235.sh
./build_asus_rt15n.sh
./build_wcrgn.sh
./build_w502u.sh
./build_whr300hp2.sh
./build_whr1166d.sh
./build_e1700.sh
./build_dir810l.sh
./build_dir860l.sh
./build_dir882.sh
./build_r6800.sh
./build_wndr3700v5.sh
cd broadcom_2_6_80211ac/opt 
./do_asus_rtac66u.sh 
./do_ea2700.sh 
./do_r6200.sh 
./do_buffalo_dd-wrt_ac.sh 
./do_eko_big_v24-K26-nv64k.sh
./do_eko_mega_v24-K26-nv64k.sh
./do_eko_mega_v24-K26-nv64k.sh.wnr3500
./do_ubnt_unifiac.sh
cd ../../
cd broadcom_2_6_80211ac_mipselr1/opt 
./do_eko_mega_v24-K26-nv64k.sh
cd ../../

#DATE=$(date +%m-%d-%Y)
#DATE+="-r"
#DATE+=$(svnversion -n ar531x/src/router/httpd)
#chmod -R 777 /GruppenLW/releases/$DATE/
#scp -r /GruppenLW/releases/$DATE ftp.dd-wrt.com:/downloads
