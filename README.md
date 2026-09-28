# MAC Address Tool

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`mactool` is a binary tool written in C designated for **QSDK** which allows users to modify MAC address(es) stored in the ART partition.

---

## How to compile mactool in Openwrt
```
cd {TOP_DIR}/package/
git clone https://github.com/brucerry/mactool.git
cd {TOP_DIR}/
make menuconfig # select mactool and save
make V=s -j1 package/mactool/{clean,compile}
```

---

## How to use mactool in device CLI

### For ART partition mounted in NOR/NAND
```
# For ART partition mounted in NOR/NAND
mactool -i eth0 -g
mactool -i eth0 -s 4c136504800a
mactool -i wifi0 -g
mactool -i wifi0 -s 4c136504800b
```

### For ART partition mounted in eMMC
```
mactool -e -i eth0 -g
mactool -e -i eth0 -s 4c136504800a
mactool -e -i wifi0 -g
mactool -e -i wifi0 -s 4c136504800b
```
