#!/bin/sh

OLDPATH=$PATH
DATE=$(date +%m-%d-%Y)
DATE+="-r"
DATE+=$(svnversion -n ar531x/src/router/httpd)
export PATH=/xfs/toolchains/toolchain-mips_mips32_gcc-8.2.0_musl/bin:$OLDPATH
cd ar531x/src/router
[ -n "$DO_UPDATE" ] && svn update
cd opt/etc/config
[ -n "$DO_UPDATE" ] && svn update
cd ../../../

cp .config_ecb3500 .config
make -f Makefile.ar531x build_date
make -f Makefile.ar531x kernel clean all install
mkdir -p ~/GruppenLW/releases/$DATE/senao-ecb3500
cd ../../../
#cp ar531x/src/router/mips-uclibc/root.fs ~/GruppenLW/releases/$DATE/fonera
cp ar531x/src/router/mips-uclibc/vmlinux.fonera ~/GruppenLW/releases/$DATE/senao-ecb3500/linux.bin
cp ar531x/src/router/mips-uclibc/fonera-firmware.bin ~/GruppenLW/releases/$DATE/senao-ecb3500/ecb3500-firmware.bin

