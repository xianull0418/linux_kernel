#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <stdarg.h>

// 引入外部全局变量 pfcount，用于记录页面缺页中断次数
extern unsigned long volatile pfcount;

// 定义全局变量
static struct timer_list test_timer;  // 定时器结构体
static unsigned long pfcount_last;   // 上一次的页面缺页中断计数
static unsigned long pfcount_in_2;   // 两秒内的页面缺页中断计数
static int count = 0;                // 用于记录定时器触发的次数

// 模块声明
MODULE_LICENSE("GPL");

/*
 * 5. show 函数
 * 用于将内核数据输出到用户空间。
 * 当访问对应的 /proc 文件时会调用该函数。
 */
static int my_proc_show(struct seq_file *m, void *v)
{
    /*
     * 注意：在这里不能使用 printk 等函数。
     * 使用 seq_file 系列函数向用户空间输出内容。
     */
    seq_printf(m, "[latest] Number of page fault interrupts in 2 seconds: %ld !\n", pfcount_in_2);
    return 0;
}

/*
 * 定时器的回调函数
 * 每两秒触发一次，计算页面缺页中断的数量，并打印日志。
 */
static void irq_test_timer_function(struct timer_list *timer)
{
    // 打印日志信息，记录两秒内页面缺页中断数量
    printk("%d Number of page fault interrupts in 2 seconds: %ld\n", count, pfcount - pfcount_last);

    // 计算两秒内的页面缺页中断数
    pfcount_in_2 = pfcount - pfcount_last;

    // 更新上一次的页面缺页中断计数
    pfcount_last = pfcount;

    // 重新设置定时器，延迟两秒后再次触发
    mod_timer(&test_timer, jiffies + 2 * HZ);
    count++;
}

/*
 * 打开 proc 文件时调用的函数
 * 绑定 seq_show 函数指针，用于显示数据
 */
static int my_proc_open(struct inode *inode, struct file *file)
{
    /*
     * 使用 single_open 函数绑定 seq_show 函数。
     * single_open 是内核提供的封装函数。
     */
    return single_open(file, my_proc_show, NULL);
}

/*
 * 2. 定义 proc 文件操作结构体
 * 填充 proc_create 函数中使用的 file_operations 结构体。
 * my_proc_* 为自定义函数，seq 和 single 为内核已实现的函数。
 * open 为必须实现的函数。
 */
static struct proc_ops my_fops = {
    .proc_open = my_proc_open,      // 打开文件时的回调函数
    .proc_read = seq_read,          // 读取文件内容
    .proc_lseek = seq_lseek,        // 文件指针的偏移操作
    .proc_release = single_release, // 关闭文件
};

/*
 * 3. 模块加载函数
 * 初始化时创建 /proc 文件和定时器
 */
static int __init my_init(void)
{
    struct proc_dir_entry *file;    // /proc 文件指针
    struct proc_dir_entry *parent; // 父级目录指针

    // 在 /proc 下创建目录，这里用学号作为目录名
    parent = proc_mkdir("3190608027", NULL);

    /*
     * 使用 proc_create 创建 /proc 文件，并绑定 file_operations
     * 参数说明：
     * 参数1：文件名
     * 参数2：文件权限
     * 参数3：父目录指针，如果为 NULL，则在 /proc 下创建
     * 参数4：绑定的 file_operations 结构体
     */
    file = proc_create("readpfcount", 0644, parent, &my_fops);
    if (!file)
        return -ENOMEM;

    // 初始化定时器
    pfcount_last = pfcount;                 // 初始化上一次的页面缺页计数
    test_timer.expires = jiffies + 2 * HZ;  // 设置初始超时时间为 2 秒
    timer_setup(&test_timer, irq_test_timer_function, 0); // 设置定时器回调函数

    // 添加定时器
    add_timer(&test_timer);

    printk(KERN_INFO "Module initialized and timer added.\n");
    return 0;
}

/*
 * 6. 模块卸载函数
 * 卸载时删除 /proc 文件和定时器
 */
static void __exit my_exit(void)
{
    printk(KERN_INFO "Exiting module and removing timer.\n");

    // 删除定时器
    del_timer(&test_timer);

    // 删除 /proc 文件和目录
    remove_proc_entry("readpfcount", NULL);       // 删除文件
    remove_proc_entry("3190608027", NULL);       // 删除目录
}

// 注册模块加载和卸载函数
module_init(my_init);
module_exit(my_exit);
