# 使用当前运行的内核路径
KERNELDIR := /lib/modules/$(shell uname -r)/build
# 获取当前目录
PWD := $(shell pwd)

# 添加编译选项，禁用所有可选特性
EXTRA_CFLAGS += -DCONFIG_EXT4_FS_VERITY=n
EXTRA_CFLAGS += -DCONFIG_FS_VERITY=n
EXTRA_CFLAGS += -DCONFIG_FS_VERITY_BUILTIN=n
# 启用 ACL 和安全特性
EXTRA_CFLAGS += -DCONFIG_EXT4_FS_POSIX_ACL=y
EXTRA_CFLAGS += -DCONFIG_EXT4_FS_SECURITY=y
EXTRA_CFLAGS += -DCONFIG_EXT4_SECURITY=y

# 目标文件
obj-m += ext4edit.o

# ext4edit模块的源文件列表
ext4edit-y := balloc.o bitmap.o block_validity.o dir.o ext4_jbd2.o extents.o \
              extents_status.o file.o fsmap.o fsync.o hash.o ialloc.o \
              indirect.o inline.o inode.o ioctl.o mballoc.o migrate.o \
              mmp.o move_extent.o namei.o page-io.o readpage.o resize.o \
              super.o symlink.o sysfs.o xattr.o xattr_trusted.o xattr_user.o \
              acl.o xattr_security.o verity.o

# 默认目标
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

# 清理目标
clean:
	rm -rf *.o *.mod.c *.ko *.symvers
