#!/bin/sh
OLDPATH=$PATH
DATE=$(date +%m-%d-%Y)
DATE+="-r"
DATE+=$(svnversion -n ar531x/src/router/httpd)
export PATH=/xfs/toolchains/toolchain-mipsel_gcc-linaro_uClibc-0.9.32/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mipsel_mips32_gcc-8.2.0_musl/bin:$OLDPATH
cd broadcom_2_6
./build_k26.sh

