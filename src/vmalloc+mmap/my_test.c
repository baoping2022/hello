#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>

#define KERNEL_BUFFER_SIZE (16 * 1024 * 1024) // 16M

static void *kernel_buffer;

struct mmap_info {
    void *data;
};

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > KERNEL_BUFFER_SIZE) {
        printk(KERN_ERR "Invalid mmap size\n");
        return -EINVAL;
    }

    unsigned long pfn = vmalloc_to_pfn(kernel_buffer);

    if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
        printk(KERN_ERR "Failed to remap memory\n");
        return -EAGAIN;
    }

    vma->vm_private_data = filp->private_data;

    return 0;
}

static int my_open(struct inode *inode, struct file *filp)
{
    struct mmap_info *info;

    info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;

    info->data = kernel_buffer;
    filp->private_data = info;

    return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
    struct mmap_info *info = filp->private_data;
    kfree(info);
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct mmap_info *info = filp->private_data;

    // 从内核缓冲区读取数据到用户空间
    if (*f_pos >= KERNEL_BUFFER_SIZE)
        return 0;

    size_t bytes_to_read = min(count, KERNEL_BUFFER_SIZE - *f_pos);
    if (copy_to_user(buf, info->data + *f_pos, bytes_to_read))
        return -EFAULT;

    *f_pos += bytes_to_read;
    return bytes_to_read;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct mmap_info *info = filp->private_data;

    // 从用户空间写入数据到内核缓冲区
    if (*f_pos >= KERNEL_BUFFER_SIZE)
        return 0;

    size_t bytes_to_write = min(count, KERNEL_BUFFER_SIZE - *f_pos);
    if (copy_from_user(info->data + *f_pos, buf, bytes_to_write))
        return -EFAULT;

    *f_pos += bytes_to_write;
    return bytes_to_write;
}

struct file_operations my_fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .mmap = my_mmap,
};

static struct miscdevice my_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "my_device",
    .fops = &my_fops,
};

static int __init my_module_init(void)
{
    // 使用 vmalloc 分配内存
    kernel_buffer = vmalloc(KERNEL_BUFFER_SIZE);
    if (!kernel_buffer) {
        printk(KERN_ERR "Failed to allocate memory\n");
        return -ENOMEM;
    }

    // 注册设备文件
    misc_register(&my_misc_device);

    return 0;
}

static void __exit my_module_exit(void)
{
    // 释放内存
    vfree(kernel_buffer);

    // 注销设备文件
    misc_deregister(&my_misc_device);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("baoping.fan");
MODULE_DESCRIPTION("Example module for vmalloc and mmap");
