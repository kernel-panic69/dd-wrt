#!/bin/sh
OLDPATH=$PATH
DATE=$(date +%m-%d-%Y)
DATE+="-r"
DATE+=$(svnversion -n danube/src/router/httpd)
export PATH=/xfs/toolchains/toolchain-mips_24kc_gcc-13.1.0_musl/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mipsel_24kc_gcc-13.1.0_musl/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mips_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin:$OLDPATH
cd danube/src/router
[ -n "$DO_UPDATE" ] && svn update
cd opt/etc/config
[ -n "$DO_UPDATE" ] && svn update
cd ../../../
cp .config_vodafone_802 .config
make -f Makefile.danube kernel clean all install
mkdir -p ~/GruppenLW/releases/$DATE/vodafone-easybox-802
cd ../../../
cp danube/src/router/mips-uclibc/danube-webflash.bin ~/GruppenLW/releases/$DATE/vodafone-easybox-802/vodafone-easybox_802-webflash-firmware.bin
cp danube/src/router/mips-uclibc/aligned.uimage ~/GruppenLW/releases/$DATE/vodafone-easybox-802/uImage.bin
