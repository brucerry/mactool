## How to build mactool in QSDK
```
cd {TOP_DIR}/qsdk/package/
git clone git@10.0.95.233:brucerry/mactool.git -b {branch_name}
cd {TOP_DIR}/qsdk/
make menuconfig # enable mactool package
make V=s -j1
```

## How to compile mactool only
```
cd {TOP_DIR}/qsdk/package/
git clone git@10.0.95.233:brucerry/mactool.git -b {branch_name}
cd {TOP_DIR}/qsdk/
make menuconfig # enable mactool package
make V=s -j1 package/mactool/{clean,compile}
```

## How to use mactool in Linux
Example cases:
```
# For ART partition mounted in NOR/NAND
mactool -i eth0 -g
mactool -i eth0 -s 4c136504800a
mactool -i wifi0 -g
mactool -i wifi0 -s 4c136504800b

# For ART partition mounted in eMMC
mactool -e -i eth0 -g
mactool -e -i eth0 -s 4c136504800a
mactool -e -i wifi0 -g
mactool -e -i wifi0 -s 4c136504800b
```
