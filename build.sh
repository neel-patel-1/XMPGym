#!/bin/bash

ROOT=$(pwd)
cd $ROOT/idxd-config/
git submodule update --init .
./autogen.sh
./configure CFLAGS='-g -O2' --disable-logging --prefix=${PWD}/install --sysconfdir=/etc --libdir=${PWD}/install/lib64 --enable-test=yes
make -j


if [ ! -f "/lib/firmware/qat_4xxx.bin" ] || [ ! -f "/lib/firmware/qat_4xxx_mmp.bin" ] ;
then
  wget https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain/qat_4xxx.bin
  wget https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain/qat_4xxx_mmp.bin
  sudo mv qat_4xxx.bin /lib/firmware
  sudo mv qat_4xxx_mmp.bin /lib/firmware
fi

sudo lsmod | grep qat > qat_drv.txt
[ ! -z $( grep -q -e intel_qat qat_drv.txt ) ] && [ ! -z $( grep -q -e qat_4xxx qat_drv.txt ) ]  && echo "check drivers" && exit -1

sudo systemctl enable qat
sudo systemctl start qat

cd $ROOT/qatlib
./autogen.sh
./configure --enable-service
make -j
sudo make install