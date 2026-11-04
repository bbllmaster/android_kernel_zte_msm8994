/*
 * Generic GPIO card-detect helper
 *
 * Copyright (C) 2011, Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/mmc/host.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "../host/sdhci.h"
#define CDGPIO_BUFF_SIZE 50

struct cdgpio_classdev {
	bool			 status;
	bool			 status_boot;
	int			 count;
	struct device		*dev;
	struct mmc_host *host;
};

struct mmc_gpio {
	int ro_gpio;
	int cd_gpio;
	char *ro_label;
	bool status;
	char cd_label[0]; /* Must be last entry */
	struct cdgpio_classdev cdgpio;
};

static struct class *cdgpio_class;

bool is_sdcard_insert = true;
	
static int mmc_gpio_get_status(struct mmc_host *host)
{
	int ret = -ENOSYS;
	struct mmc_gpio *ctx = host->slot.handler_priv;

	if (!ctx || !gpio_is_valid(ctx->cd_gpio))
		goto out;

	ret = !gpio_get_value_cansleep(ctx->cd_gpio) ^
		!!(host->caps2 & MMC_CAP2_CD_ACTIVE_HIGH);
out:
	return ret;
}

extern void mmc_power_off(struct mmc_host *host);
extern int sdhci_msm_setup_vreg_slot(struct mmc_host *host,
			bool enable, bool is_init);

static irqreturn_t mmc_gpio_cd_irqt(int irq, void *dev_id)
{
	/* Schedule a card detection after a debounce timeout */
	struct mmc_host *host = dev_id;
	struct mmc_gpio *ctx = host->slot.handler_priv;
	int status;

	/*
	 * In case host->ops are not yet initialized return immediately.
	 * The card will get detected later when host driver calls
	 * mmc_add_host() after host->ops are initialized.
	 */
	if (!host->ops)
		goto out;

	if (host->ops->card_event)
		host->ops->card_event(host);

	status = mmc_gpio_get_status(host);
	if (unlikely(status < 0))
		goto out;

	if (status ^ ctx->status) {
		pr_info("%s: slot status change detected (%d -> %d), GPIO_ACTIVE_%s\n",
				mmc_hostname(host), ctx->status, status,
				(host->caps2 & MMC_CAP2_CD_ACTIVE_HIGH) ?
				"HIGH" : "LOW");
		ctx->status = status;
		ctx->cdgpio.count++;
		/* Schedule a card detection after a debounce timeout */
		if(0 == status)
		{
		    //mmc_power_off(host);
		    //sdhci_writeb(mmc_priv(host), 0, SDHCI_POWER_CONTROL);
		    is_sdcard_insert = false;
		    sdhci_msm_setup_vreg_slot(host, false, false);
		    mmc_detect_change(host, msecs_to_jiffies(10));
			
		}
		else
		{
		   is_sdcard_insert = true;
		   mmc_detect_change(host, msecs_to_jiffies(200));
		}
	}
out:

	return IRQ_HANDLED;
}

static int mmc_gpio_alloc(struct mmc_host *host)
{
	size_t len = strlen(dev_name(host->parent)) + 4;
	struct mmc_gpio *ctx;

	mutex_lock(&host->slot.lock);

	ctx = host->slot.handler_priv;
	if (!ctx) {
		/*
		 * devm_kzalloc() can be called after device_initialize(), even
		 * before device_add(), i.e., between mmc_alloc_host() and
		 * mmc_add_host()
		 */
		ctx = devm_kzalloc(&host->class_dev, sizeof(*ctx) + 2 * len,
				   GFP_KERNEL);
		if (ctx) {
			ctx->ro_label = ctx->cd_label + len;
			snprintf(ctx->cd_label, len, "%s cd", dev_name(host->parent));
			snprintf(ctx->ro_label, len, "%s ro", dev_name(host->parent));
			ctx->cd_gpio = -EINVAL;
			ctx->ro_gpio = -EINVAL;
			host->slot.handler_priv = ctx;
			ctx->cdgpio.host = host;
			ctx->cdgpio.dev = NULL;
		}
	}

	mutex_unlock(&host->slot.lock);

	return ctx ? 0 : -ENOMEM;
}

int mmc_gpio_get_ro(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;

	if (!ctx || !gpio_is_valid(ctx->ro_gpio))
		return -ENOSYS;

	return !gpio_get_value_cansleep(ctx->ro_gpio) ^
		!!(host->caps2 & MMC_CAP2_RO_ACTIVE_HIGH);
}
EXPORT_SYMBOL(mmc_gpio_get_ro);

int mmc_gpio_get_cd(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;

	if (!ctx || !gpio_is_valid(ctx->cd_gpio))
		return -ENOSYS;

	return !gpio_get_value_cansleep(ctx->cd_gpio) ^
		!!(host->caps2 & MMC_CAP2_CD_ACTIVE_HIGH);
}
EXPORT_SYMBOL(mmc_gpio_get_cd);

/**
 * mmc_gpio_request_ro - request a gpio for write-protection
 * @host: mmc host
 * @gpio: gpio number requested
 *
 * As devm_* managed functions are used in mmc_gpio_request_ro(), client
 * drivers do not need to explicitly call mmc_gpio_free_ro() for freeing up,
 * if the requesting and freeing are only needed at probing and unbinding time
 * for once.  However, if client drivers do something special like runtime
 * switching for write-protection, they are responsible for calling
 * mmc_gpio_request_ro() and mmc_gpio_free_ro() as a pair on their own.
 *
 * Returns zero on success, else an error.
 */
int mmc_gpio_request_ro(struct mmc_host *host, unsigned int gpio)
{
	struct mmc_gpio *ctx;
	int ret;

	if (!gpio_is_valid(gpio))
		return -EINVAL;

	ret = mmc_gpio_alloc(host);
	if (ret < 0)
		return ret;

	ctx = host->slot.handler_priv;

	ret = devm_gpio_request_one(&host->class_dev, gpio, GPIOF_DIR_IN,
				    ctx->ro_label);
	if (ret < 0)
		return ret;

	ctx->ro_gpio = gpio;

	return 0;
}
EXPORT_SYMBOL(mmc_gpio_request_ro);

/**
 * mmc_gpio_request_cd - request a gpio for card-detection
 * @host: mmc host
 * @gpio: gpio number requested
 *
 * As devm_* managed functions are used in mmc_gpio_request_cd(), client
 * drivers do not need to explicitly call mmc_gpio_free_cd() for freeing up,
 * if the requesting and freeing are only needed at probing and unbinding time
 * for once.  However, if client drivers do something special like runtime
 * switching for card-detection, they are responsible for calling
 * mmc_gpio_request_cd() and mmc_gpio_free_cd() as a pair on their own.
 *
 * Returns zero on success, else an error.
 */
int mmc_gpio_request_cd(struct mmc_host *host, unsigned int gpio)
{
	struct mmc_gpio *ctx;
	int irq = gpio_to_irq(gpio);
	int ret;

	ret = mmc_gpio_alloc(host);
	if (ret < 0)
		return ret;

	ctx = host->slot.handler_priv;

	ret = devm_gpio_request_one(&host->class_dev, gpio, GPIOF_DIR_IN,
				    ctx->cd_label);
	if (ret < 0)
		/*
		 * don't bother freeing memory. It might still get used by other
		 * slot functions, in any case it will be freed, when the device
		 * is destroyed.
		 */
		return ret;

	/*
	 * Even if gpio_to_irq() returns a valid IRQ number, the platform might
	 * still prefer to poll, e.g., because that IRQ number is already used
	 * by another unit and cannot be shared.
	 */
	if (irq >= 0 && host->caps & MMC_CAP_NEEDS_POLL)
		irq = -EINVAL;

	ctx->cd_gpio = gpio;
	host->slot.cd_irq = irq;

	ret = mmc_gpio_get_status(host);
	if (ret < 0)
		return ret;

	ctx->status = ret;

	if (irq >= 0) {
		ret = devm_request_threaded_irq(&host->class_dev, irq,
			NULL, mmc_gpio_cd_irqt,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			ctx->cd_label, host);
		if (ret < 0)
			irq = ret;
	}

	if (irq < 0)
		host->caps |= MMC_CAP_NEEDS_POLL;

	if(!ctx->cdgpio.dev) {
		ctx->cdgpio.status_boot = ctx->status;
		ctx->cdgpio.count= 0;

		ctx->cdgpio.dev = device_create(cdgpio_class, mmc_dev(host), 0, &ctx->cdgpio,
					      "%s", mmc_hostname(host));
		if (!IS_ERR(ctx->cdgpio.dev))
			printk(KERN_DEBUG "Registered CD-GPIO device: %s\n", mmc_hostname(host));
	}

	return 0;
}
EXPORT_SYMBOL(mmc_gpio_request_cd);

/**
 * mmc_gpio_free_ro - free the write-protection gpio
 * @host: mmc host
 *
 * It's provided only for cases that client drivers need to manually free
 * up the write-protection gpio requested by mmc_gpio_request_ro().
 */
void mmc_gpio_free_ro(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;
	int gpio;

	if (!ctx || !gpio_is_valid(ctx->ro_gpio))
		return;

	gpio = ctx->ro_gpio;
	ctx->ro_gpio = -EINVAL;

	devm_gpio_free(&host->class_dev, gpio);
}
EXPORT_SYMBOL(mmc_gpio_free_ro);

/**
 * mmc_gpio_free_cd - free the card-detection gpio
 * @host: mmc host
 *
 * It's provided only for cases that client drivers need to manually free
 * up the card-detection gpio requested by mmc_gpio_request_cd().
 */
void mmc_gpio_free_cd(struct mmc_host *host)
{
	struct mmc_gpio *ctx = host->slot.handler_priv;
	int gpio;

	if (!ctx || !gpio_is_valid(ctx->cd_gpio))
		return;

	if (host->slot.cd_irq >= 0) {
		devm_free_irq(&host->class_dev, host->slot.cd_irq, host);
		host->slot.cd_irq = -EINVAL;
	}

	gpio = ctx->cd_gpio;
	ctx->cd_gpio = -EINVAL;

	devm_gpio_free(&host->class_dev, gpio);
}
EXPORT_SYMBOL(mmc_gpio_free_cd);

static ssize_t status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cdgpio_classdev *cdgpio_cdev = dev_get_drvdata(dev);
	struct mmc_host *host = cdgpio_cdev->host;

	cdgpio_cdev->status = mmc_gpio_get_status(host);

	return snprintf(buf, CDGPIO_BUFF_SIZE, "%u\n", cdgpio_cdev->status);
}

static ssize_t status_boot_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cdgpio_classdev *cdgpio_cdev = dev_get_drvdata(dev);

	return snprintf(buf, CDGPIO_BUFF_SIZE, "%u\n", cdgpio_cdev->status_boot);
}

static ssize_t count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cdgpio_classdev *cdgpio_cdev = dev_get_drvdata(dev);

	return snprintf(buf, CDGPIO_BUFF_SIZE, "%u\n", cdgpio_cdev->count);
}

static ssize_t insert_boot_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cdgpio_classdev *cdgpio_cdev = dev_get_drvdata(dev);
	int is_insert_boot;

	is_insert_boot = ((cdgpio_cdev->status_boot == 1) && (cdgpio_cdev->count == 0));
	cdgpio_cdev->status_boot = 0;

	printk(KERN_INFO "insert_boot=%d,(count %d status_boot %d)\n", is_insert_boot, cdgpio_cdev->count, cdgpio_cdev->status_boot);
	return snprintf(buf, CDGPIO_BUFF_SIZE, "%u\n", is_insert_boot);
}

static struct device_attribute cdgpio_class_attrs[] = {
	__ATTR(status, 0444, status_show, NULL),
	__ATTR(status_boot, 0444, status_boot_show, NULL),
	__ATTR(count, 0444, count_show, NULL),
	__ATTR(insert_boot, 0444, insert_boot_show, NULL),
	__ATTR_NULL,
};

static int __init cdgpio_init(void)
{
	cdgpio_class = class_create(THIS_MODULE, "cd-gpio");
	if (IS_ERR(cdgpio_class))
		return PTR_ERR(cdgpio_class);
	cdgpio_class->dev_attrs = cdgpio_class_attrs;
	return 0;
}

static void __exit cdgpio_exit(void)
{
	class_destroy(cdgpio_class);
}

subsys_initcall(cdgpio_init);
module_exit(cdgpio_exit);

MODULE_AUTHOR("Poyuan Lu (lu.poyuan@zte.com.cn)");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CD-GPIO Class Interface");

