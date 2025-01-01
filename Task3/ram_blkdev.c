#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/blk-mq.h>

#define SIMP_BLKDEV_DISKNAME "ram_blkdev"
#define SIMP_BLKDEV_BYTES (256 * 1024 * 1024) // 256 MB
#define SECTOR_SHIFT 9
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

// 内存虚拟磁盘空间
static unsigned char *zombotany_blkdev_data = NULL;

// 主设备号
static int simp_blkdev_major = 0;

// 设备 gendisk 结构
static struct gendisk *zombotany_blkdev_disk = NULL;

// 请求队列相关结构
static struct blk_mq_tag_set zombotany_blkdev_tag_set;
static struct request_queue *zombotany_blkdev_queue = NULL;

// 请求处理函数
static blk_status_t zombotany_handle_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
    struct request *req = bd->rq;
    struct bio_vec bvec;
    struct req_iterator iter;
    unsigned long start_sector = blk_rq_pos(req); // 起始扇区
    unsigned char *disk_mem = zombotany_blkdev_data + (start_sector << SECTOR_SHIFT); // 起始位置
    blk_status_t status = BLK_STS_OK;

    // 遍历请求中的 BIO
    rq_for_each_segment(bvec, req, iter) {
        void *buffer = kmap_atomic(bvec.bv_page) + bvec.bv_offset;

        if (rq_data_dir(req) == READ) {
            // 执行读取操作
            memcpy(buffer, disk_mem, bvec.bv_len);
        } else if (rq_data_dir(req) == WRITE) {
            // 执行写入操作
            memcpy(disk_mem, buffer, bvec.bv_len);
        } else {
            status = BLK_STS_IOERR;
            break;
        }

        kunmap_atomic(buffer);
        disk_mem += bvec.bv_len;
    }

    blk_mq_end_request(req, status);
    return status;
}

// 多队列块设备操作
static struct blk_mq_ops zombotany_blkdev_ops = {
    .queue_rq = zombotany_handle_request,
};

// 块设备操作结构体
static struct block_device_operations zombotany_blkdev_fops = {
    .owner = THIS_MODULE,
};

// 模块初始化函数
static int __init zombotany_blkdev_init(void)
{
    int ret;

    // 分配虚拟磁盘存储空间
    zombotany_blkdev_data = vzalloc(SIMP_BLKDEV_BYTES);
    if (!zombotany_blkdev_data) {
        printk(KERN_ERR SIMP_BLKDEV_DISKNAME ": vzalloc failed\n");
        return -ENOMEM;
    }

    // 动态申请主设备号
    simp_blkdev_major = register_blkdev(0, SIMP_BLKDEV_DISKNAME);
    if (simp_blkdev_major < 0) {
        printk(KERN_ERR SIMP_BLKDEV_DISKNAME ": unable to get major number\n");
        vfree(zombotany_blkdev_data);
        return simp_blkdev_major;
    }

    // 初始化多队列设置
    memset(&zombotany_blkdev_tag_set, 0, sizeof(zombotany_blkdev_tag_set));
    zombotany_blkdev_tag_set.ops = &zombotany_blkdev_ops;
    zombotany_blkdev_tag_set.nr_hw_queues = 1;
    zombotany_blkdev_tag_set.queue_depth = 128;
    zombotany_blkdev_tag_set.numa_node = NUMA_NO_NODE;
    zombotany_blkdev_tag_set.cmd_size = 0;
    zombotany_blkdev_tag_set.flags = BLK_MQ_F_SHOULD_MERGE;

    ret = blk_mq_alloc_tag_set(&zombotany_blkdev_tag_set);
    if (ret) {
        printk(KERN_ERR SIMP_BLKDEV_DISKNAME ": blk_mq_alloc_tag_set failed\n");
        unregister_blkdev(simp_blkdev_major, SIMP_BLKDEV_DISKNAME);
        vfree(zombotany_blkdev_data);
        return ret;
    }

    // 初始化请求队列
    zombotany_blkdev_queue = blk_mq_init_queue(&zombotany_blkdev_tag_set);
    if (IS_ERR(zombotany_blkdev_queue)) {
        printk(KERN_ERR SIMP_BLKDEV_DISKNAME ": blk_mq_init_queue failed\n");
        blk_mq_free_tag_set(&zombotany_blkdev_tag_set);
        unregister_blkdev(simp_blkdev_major, SIMP_BLKDEV_DISKNAME);
        vfree(zombotany_blkdev_data);
        return PTR_ERR(zombotany_blkdev_queue);
    }

    // 分配 gendisk 结构
    zombotany_blkdev_disk = alloc_disk(1);
    if (!zombotany_blkdev_disk) {
        printk(KERN_ERR SIMP_BLKDEV_DISKNAME ": alloc_disk failed\n");
        blk_cleanup_queue(zombotany_blkdev_queue);
        blk_mq_free_tag_set(&zombotany_blkdev_tag_set);
        unregister_blkdev(simp_blkdev_major, SIMP_BLKDEV_DISKNAME);
        vfree(zombotany_blkdev_data);
        return -ENOMEM;
    }

    // 设置 gendisk 属性
    zombotany_blkdev_disk->major = simp_blkdev_major;
    zombotany_blkdev_disk->first_minor = 0;
    zombotany_blkdev_disk->fops = &zombotany_blkdev_fops;
    zombotany_blkdev_disk->queue = zombotany_blkdev_queue;
    snprintf(zombotany_blkdev_disk->disk_name, 32, SIMP_BLKDEV_DISKNAME);
    set_capacity(zombotany_blkdev_disk, SIMP_BLKDEV_BYTES >> SECTOR_SHIFT);

    // 注册磁盘
    add_disk(zombotany_blkdev_disk);

    printk(KERN_INFO SIMP_BLKDEV_DISKNAME ": module loaded\n");
    return 0;
}

// 模块卸载函数
static void __exit zombotany_blkdev_exit(void)
{
    if (zombotany_blkdev_disk) {
        del_gendisk(zombotany_blkdev_disk);
        put_disk(zombotany_blkdev_disk);
    }

    if (zombotany_blkdev_queue) {
        blk_cleanup_queue(zombotany_blkdev_queue);
    }

    blk_mq_free_tag_set(&zombotany_blkdev_tag_set);

    if (zombotany_blkdev_data) {
        vfree(zombotany_blkdev_data);
    }

    unregister_blkdev(simp_blkdev_major, SIMP_BLKDEV_DISKNAME);

    printk(KERN_INFO SIMP_BLKDEV_DISKNAME ": module unloaded\n");
}

module_init(zombotany_blkdev_init);
module_exit(zombotany_blkdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux block device module using blk-mq");
