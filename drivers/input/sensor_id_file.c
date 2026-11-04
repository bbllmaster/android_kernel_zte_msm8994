/*
 * Created by zuojianfang for factory that show sensor id
 * Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

static char accel_id[256] = "Bosch-BMI160-NA-NA-0x68";
static char mag_id[23] = "AKM-AKM09911-NA-NA-0x0C";
static char light_id[256] = "LTR-LTR59-NA-NA-0x23";

struct proc_dir_entry *accel_proc_file;
struct proc_dir_entry *mag_proc_file;
struct proc_dir_entry *light_proc_file;

static int show_accel_id(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", accel_id);
	return 0;
}
static int show_mag_id(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", mag_id);

	return 0;
}
static int show_light_id(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", light_id);
	printk("show_light_id\n");
	return 0;
}

static int accel_id_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, show_accel_id, PDE_DATA(inode));
}
static int mag_id_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, show_mag_id, PDE_DATA(inode));
}
static int light_id_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, show_light_id, PDE_DATA(inode));
}

static const struct file_operations accels = {
	.open		= accel_id_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static const struct file_operations mags = {
	.open		= mag_id_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static const struct file_operations lights = {
	.open		= light_id_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void create_accel_proc_file(void)
{
	//struct proc_dir_entry *accel_proc_file;
	accel_proc_file = proc_create("driver/accel_id", 0777, NULL, &accels);
	if (accel_proc_file) {
		printk(KERN_INFO "accel_proc_file create success!\n");
	}else{
		printk(KERN_INFO "accel_proc_file create failed!\n");
	}
}

static void create_mag_proc_file(void)
{
	//struct proc_dir_entry *mag_proc_file;
       mag_proc_file =  proc_create("driver/mag_id", 0777, NULL, &mags);
	if (mag_proc_file) {
		printk(KERN_INFO "mag_proc_file create success!\n");
	}else{
		printk(KERN_INFO "mag_proc_file create failed!\n");
	}
}

static void create_light_proc_file(void)
{
	//struct proc_dir_entry *light_proc_file;
	light_proc_file = proc_create("driver/light_id", 0777, NULL, &lights);
	if (light_proc_file) {
		printk(KERN_INFO "light_proc_file  create success!\n");
	}else{
		printk(KERN_INFO "light_proc_file  create failed!\n");
	}
}

static void remove_sensors_proc_file(void)
{
	printk("sensor jfz remove_sensor_proc_file\n");
	if(accel_proc_file){
		remove_proc_entry("driver/accel_id", NULL);
		accel_proc_file = NULL;
	}
	if(mag_proc_file){
		remove_proc_entry("driver/mag_id", NULL);
		mag_proc_file = NULL;
	}	
	if(light_proc_file){
		remove_proc_entry("driver/light_id", NULL);
		light_proc_file = NULL;
	}		
}

static int sensors_id_probe(struct platform_device *dev)
{
    printk(KERN_INFO"%s: \n", __func__);
	create_accel_proc_file();
	create_mag_proc_file();
	create_light_proc_file();		
	return 0;
}

static int sensors_id_remove(struct platform_device *dev)
{
    printk("sensor jfz hall_driver_remove\n");
    remove_sensors_proc_file();
	return 0;
}

static struct of_device_id sensors_match_table[] = {
	{.compatible = "sensors-id", },
	{ },
};
static struct platform_driver sensors_id_file = {
	.driver = {
		.name = "sensor_id",
		.owner = THIS_MODULE,
		.of_match_table = sensors_match_table,
	},
	.probe = sensors_id_probe,
	.remove = sensors_id_remove,
};

static int __init sensors_id_init(void)
{
    printk(KERN_INFO"%s: \n", __func__);
	return platform_driver_register(&sensors_id_file);
}

static void __exit sensors_id_exit(void)
{
    printk(KERN_INFO"%s: \n", __func__);
	platform_driver_unregister(&sensors_id_file);
}

module_init(sensors_id_init);
module_exit(sensors_id_exit);
MODULE_DESCRIPTION("Create sensors id file");
MODULE_LICENSE("GPL v2");
