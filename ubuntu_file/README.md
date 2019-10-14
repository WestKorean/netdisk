**详见readme.docx**

本文档适用于充电宝项目，是关于内核和kali操作系统的编译和安装说明

# 0x00  目录结构
    source         各种源码文件
    release        各种构建完成文件
    tools          工具包

# 0x01  准备
1. Ubuntu 18.04
   切换到root用户
```bash
    su root
```
   安装环境
```bash
    apt-get install -y repo gcc-arm-linux-gnueabihf u-boot-tools device-tree-compiler mtools \
    parted libudev-dev libusb-1.0-0-dev libssl-dev g+conf autotools-dev m4 intltool libdrm-dev \
    binutils build-essential gcc g++ gzip bzip2 perl cpio bc libncurses5 libglib2.0-dev \
    libglade2-dev git mercurial openssh-client asciidoc w3m dblatex graphviz libc6:i386 libssl-dev \
    texinfo liblz4-tool genext2fs lib32stdc++6 tree exfat-utils dosfstools python3-tk swig python3-dev python-dev kpartx
```


_________________________________________________________
# 0x02 测试人员快速编译方法
进入system/主目录下
```bash
./tools/installgcc.sh
. ~/.bashrc
cd ./source/bootloader/u-boot-2016
./build.sh && cd -	
cd ./source/kernel/linux-4.4.49
./build.sh all && cd -
cd ./source/modules/wifi/rtl8812au-5.2.20
./build.sh && cd -
cd ./source/modules/4g/quectel-cm-master/
./build.sh && cd -
cd release/build/img
./mount.sh kali-linux-10.04-nanopi3.img
./update.sh all
./umount.sh kali-linux-10.04-nanopi3.img
```
此时将kali-linux-10.04-nanopi3.img复制到一张能够启动充电宝的SD卡上,然后用SD启动进入系统,进入存放img的目录
最后使用DD命令将kali-linux-10.04-nanopi3.img写入eMMC完成系统更新.
dd if=kali-linux-10.04-nanopi3.img  of=/dev/mmcblk1 bs=1M
____________________________________________________



# 0x03 完整编译方法
**************************
一、安装编译环境
**************************

* 1. 解压和安装编译器
	* ./tools/install_gcc.sh
	
* 2. 重新source用户的环境变量,以让PATH环境变量中包含该工具链的目录
	* source ~/.bashrc
	
* 【注：工具链将解压到./tools/gcc-x64/6.4-aarch64/，并在/opt/friendlyarm/toolchain/aarch6.4建立连接】



**************************
二、编译和安装内核
**************************

* 1. 编译和安装整个内核，包括内核镜像、设备树、内核模块、内核头等
	* cd ./source/kernel/linux-4.4.49
	* ./build.sh all
	
	* 配置内核
	* make ARCH=arm64 menuconfig
	
* 【注：build.sh脚本可以单独编译和安装内核的一部分，比如, ./build.sh all/image/headers/modules/dtbs/clean】
* 【注：内核镜像和设备树将安装到release/build/kernel/下，内核模块将安装到release/build/kernel-modules/下，
	内核头将安装到release/build/kernel-headers/下】



**************************
三、编译和安装模块
**************************

* 1. 编译和安装wifi
	* cd ./source/modules/wifi/rtl8812au-5.2.20
	* ./build.sh
* 2. 编译和安装4g
	* cd ./source/modules/4g/quectel-cm-master/
	* ./build.sh
	
* 【注：wifi模块将安装到release/build/modules/wifi/目录和release/build/kernel-modules/lib/modules/4.4.49-s5p6818/kernel/net/wireless/下，
	安装wifi模块需要先安装内核模块】
* 【注：4g程序将安装到release/build/modules/4g/目录】
* 【注：wifi和4g模块的脚本都在release/build/modules/下的4g和wifi目录下，直接在这里修改】



**************************
四、编译和安装bootloader
**************************

* 1. 编译和安装
	* cd ./source/bootloader/u-boot-2016
	* ./build.sh
	
* 【注：uboot将安装到release/build/uboot/下】



**************************
五、构建镜像
**************************

* 1. 构建bootloader镜像
	* 1）进入构建bootloader目录
	* cd release/build/uboot
	* 2）如果需要修改uboot的环境变量,在文件env.conf中修改
	* 3）构建bootloader镜像(特殊情况下才需重新构建）
	* ./build_uboot_img.sh
	* 该脚本会构建4M大小的bootloader镜像文件uboot.img

* 2. 构建整个镜像
	* 1）进入构建目录
	* cd release/kali-arm-build-scripts
	* 2）重新构建kali系统(特殊情况下才需要重新构建kali系统，正常情况下不需要做这一步)
	* sudo ./nanopi3-charge.sh x.y
	* x.y是构建的卡里系统的版本号，自己设置

* 【注：在已有镜像的基础上不需要构建，直接更新即可】


**************************
五.1 、制作TF卡镜像
**************************

* 1.直接烧录kali-linux-1.09-nanopi3.img.xz
	
* 2.编译uboot 烧录uboot_tf.img
   * 制作镜像的时候用uboot_tf.img 替换uboot_emmc.img,生成的kali-xx-xx.img 烧录到tf卡
* 3.用emmc 的镜像，烧录到tf卡后，挂载到linux ，
   * 执行 dd if=uboot_tf.img of=/dev/sdb bs=512 seek=1
   * tf卡启动进入uboot后，执行
        * setenv rootdev 1
        * setenv bootargs 'console=ttySAC0,115200n8 root=/dev/mmcblk1p2 rootfstype=ext4 rootwait rw consoleblank=0 net.ifnames=0 lcd=HDMI720P60 bootdev=2'
   
**************************
六、更新和烧写系统镜像
**************************

* 1. 如果某模块的代码有更新，先编译和安装相应的模块

* 2. 更新整个镜像
	* 1）进入更新目录
	* cd release/build/img
	* 2）解压最初构建的镜像文件
	* xz -dk kali-linux-1.8-nanopi3.img.xz
	* 3）挂载镜像中的分区
	* ./mount.sh kali-linux-1.8-nanopi3.img
	* kali-linux-1.8-nanopi3.img是要挂载的镜像文件名，如果镜像文件是其他名字，要改成相应的名字
	* 4) 更新各模块到镜像中
	* ./update.sh all
	* update.sh可以单独更新不同的部分，比如，./update.sh all/rootfs/kernel/uboot。
		* a）如果需要更新kali系统中的文件，
			* 在挂载镜像后，进入rootfs_kali目录直接修改相应文件。
		* b）如果需要在kali系统中安装软件
			* 在挂载镜像后
			* sudo chroot ./rootfs_kali/
			* apt-get install xxx	
			* xxx为要安装的软件
			* exit
	* 5）卸载镜像分区
	* ./umount.sh kali-linux-1.8-nanopi3.img
	* 6）压缩镜像
	* rm kali-linux-1.8-nanopi3.img.xz
	* xz -zk kali-linux-1.8-nanopi3.img

* 3. 制作tf启动卡
	* 1）将镜像烧写到tf卡上
		* win32diskimager.rar是windows上烧写tf卡的工具，在windows上解压该文件
		* 运行Win32DiskImager.exe
		* 在该软件的界面上选择镜像文件和设备，然后点击MD5 Hash，hash计算完成后，点击write进行烧写（镜像路径不能有中文）
		* 如果要将已经做好的tf卡中把镜像读出来，点击read
	* 2）修改tf卡中uboot的环境变量
		* tf卡启动充电宝，并进入uboot的命令行
		* setenv bootargs 'console=ttySAC0,115200n8 root=/dev/mmcblk1p2 rootfstype=ext4 rootwait rw consoleblank=0 net.ifnames=0 lcd=HDMI720P60 bootdev=2'
		* saveenv
		* reset

* 4. 烧写镜像到emmc
	* 1）烧写镜像的脚本为release/build/img/update_emmc.sh
	* 2）拷贝镜像和升级脚本update_emmc.sh到tf卡启动盘，比如拷贝到tf卡的rootfs分区的root目录
	* 3）烧写镜像
		* a）烧写整个镜像
			* 用tf卡启动充电宝板子
			* 进入root目录（镜像名字需改为自己的要烧写的镜像名字）
			* 方式1：./update_emmc.sh kali-linux-1.8-nanopi3.img all
			* 方式2：dd if=kali-linux-1.8-nanopi3.img of=/dev/mmcblk0 bs=102400
		* b）只烧写bootloader镜像
			* 用tf卡启动充电宝板子
			* 进入root目录
			* ./update_emmc.sh uboot.img uboot


		
**************************
**************************



