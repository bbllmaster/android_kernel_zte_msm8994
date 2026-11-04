/***********************************************************************
* Copyright (C) 2001, ZTE Corporation.
* 
* File Name:    hw_ver.c
* Description:  hw version process code
* Author:       jiabaofeng
* Date:         2015-03-13
* 
**********************************************************************/
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/stat.h>
#include <linux/module.h> 

#define VERSION_STR_A   "wrvA"      
#define VERSION_STR_B   "wrvB"
#define VERSION_STR_BB   "wrvB"
#define VERSION_STR_C   "wrvC"


struct hw_ver_name
{
    int hw_ver;     /* hw version num */
    char *hw_name;  /* hw version name */
};

int hw_ver = -1;

static char *hw_version="noversion";
static char *hw_material_version="noversion";

/*
 * Get the hw_ver value
 */
static int __init hw_ver_setup(char *str)
{
	char *after;

	printk(KERN_NOTICE "%s @str:%s\n", __func__, str);

	hw_ver = simple_strtoul(str, &after, 10);
	printk(KERN_NOTICE"hw_ver voltage =%d \n",hw_ver);
	return 1;
}
__setup("hw_ver=", hw_ver_setup);

int  zte_get_board_id(void)
{
	int board_id = -1;

	if(hw_ver > 100000 && hw_ver < 200000){
		 board_id = 1;
		 hw_version  = VERSION_STR_A;
		 hw_material_version  = "00ACB";
	}
	else if(hw_ver > 200000 && hw_ver < 300000){
		 board_id = 2;
		 hw_version  = VERSION_STR_A;
		 hw_material_version  = "00ADB";
	}
	else if(hw_ver > 300000 && hw_ver < 400000){
		 board_id = 3;
		 hw_version  = VERSION_STR_A;
		 hw_material_version  = "00AEB";
	}
	else if(hw_ver > 400000 && hw_ver < 500000){
		 board_id = 4;
		 hw_version  = VERSION_STR_A;
		 hw_material_version  = "00AFB";
	}
	else if(hw_ver > 500000 && hw_ver < 600000){
		 board_id = 5;
		 hw_version  = VERSION_STR_B;
		 hw_material_version  = "00AGB";
	}
	else if(hw_ver > 600000 && hw_ver < 700000){
		 board_id = 6;
		 hw_version  = VERSION_STR_B;
		 hw_material_version  = "00AIB";
	}
	else if(hw_ver > 700000 && hw_ver < 800000){
		 board_id = 7;
		 hw_version  = VERSION_STR_B;
		 hw_material_version  = "00AKB";
	}	
	else if(hw_ver > 800000 && hw_ver < 900000){
		 board_id = 8;
		 hw_version  = VERSION_STR_B;
		 hw_material_version  = "07AGB";
	}
	else if(hw_ver > 900000 && hw_ver < 1000000){
		 board_id = 9;
		 hw_version  = VERSION_STR_B;
		 hw_material_version  = "07AHB";
	}
	else if(hw_ver > 1000000 && hw_ver < 1100000){
		 board_id = 10;
		 hw_version  = VERSION_STR_B;
		 hw_material_version  = "00AOB";
	}
	else if(hw_ver > 1100000 && hw_ver < 1200000){
		 board_id = 11;
		 hw_version  = VERSION_STR_B;
		 hw_material_version  = "A6";
	}
	else if(hw_ver > 1200000 && hw_ver < 1300000){
		 board_id = 12;
		 hw_version  = VERSION_STR_BB;
		 hw_material_version  = "07ABB";
	}
	printk("zte_get_board_id board_id=%d\n",board_id);
	return board_id;
}

/*
 * show_hw_version() - Show public hw version to user space
 */
static ssize_t show_hw_version(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%s\n", hw_version);
}

static ssize_t show_hw_material_version(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%s\n", hw_material_version);
}


static struct device_attribute hw_version_attr =
	__ATTR(hw_version, S_IRUGO , show_hw_version, NULL);

static struct device_attribute hw_version_material_attr =
	__ATTR(hw_version_material, S_IRUGO , show_hw_material_version, NULL);

static int  hw_ver_probe(struct platform_device *pdev)
{
	int ret;   
	zte_get_board_id();
	/* create attr file */
	ret = device_create_file(&pdev->dev, &hw_version_attr);
	if (ret) {
		dev_err(&pdev->dev, "failed: create hw_version file\n");
	}
	ret = device_create_file(&pdev->dev, &hw_version_material_attr);
	if (ret) {
		dev_err(&pdev->dev, "failed: create hw_version_material file\n");
	}
	return 0;
}

static  int hw_ver_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &hw_version_attr);	
	return 0;
}

static struct platform_driver hw_ver_driver = {
	.probe	= hw_ver_probe,
	.remove	= hw_ver_remove,
	.driver = {
		.name	= "hw_version",
		.owner	= THIS_MODULE,
	},
};

static struct platform_device hw_ver_device = {
	.name = "hw_version",
	.id = -1,
};

static int __init hw_ver_init(void)
{
    int ret;

    /* register device */
    ret = platform_device_register(&hw_ver_device); 
    if (ret){
        printk(KERN_NOTICE "Unable to register hw version Device!\n");
        goto out;
    }

    /* register driver */
    ret = platform_driver_register(&hw_ver_driver);
out:
	return ret;
}


static void __exit hw_ver_exit(void)
{
	platform_device_unregister(&hw_ver_device);
	platform_driver_unregister(&hw_ver_driver);
}

module_init(hw_ver_init);
module_exit(hw_ver_exit);

