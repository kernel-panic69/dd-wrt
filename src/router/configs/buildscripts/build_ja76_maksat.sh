#!/bin/sh
OLDPATH=$PATH
DATE=$(date +%m-%d-%Y)
DATE+="-r"
DATE+=$(svnversion -n pb42/src/router/httpd)
export PATH=/xfs/toolchains/toolchain-mips_r2_gcc-linaro_uClibc-0.9.32/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mips_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin:$OLDPATH
cd pb42/src/router
[ -n "$DO_UPDATE" ] && svn update
cd opt/etc/config
[ -n "$DO_UPDATE" ] && svn update
cd ../../../
cp .config_ja76pf_maksat .config
make -f Makefile.pb42 kernel clean all install
mkdir -p ~/GruppenLW/releases/$DATE/jjplus-ja76pf_maksat
cd ../../../
cp pb42/src/router/mips-uclibc/lsx-firmware.bin ~/GruppenLW/releases/$DATE/jjplus-ja76pf_maksat/jjplus-ja76pf-firmware.bin
cp pb42/src/router/mips-uclibc/vmlinux.lsx ~/GruppenLW/releases/$DATE/jjplus-ja76pf_maksat/linux.bin
