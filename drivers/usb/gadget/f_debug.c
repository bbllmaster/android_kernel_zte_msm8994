/*
 * ZTE USB debug Driver for Android
 *
 * Copyright (C) 2012 ZTE, Inc.
 * Author: Li Xingyuan <li.xingyuan@zte.com.cn>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/sysrq.h>

static void debug_sysrq_complete(struct usb_ep *ep, struct usb_request *req)
{
	int length = req->actual;
	char key;
	
	if (req->status != 0) {
		pr_err("err %d\n", req->status);
		return;
	}

	if (length >= 1) {
		key = ((char *)req->buf)[0];
		pr_info("%02x\n", key);
		handle_sysrq(key);
	}
}

#define MAX_DEBUG_CMD_LEN PAGE_SIZE

struct debug_work_struct {
	struct work_struct work;
	struct android_dev *dev;
	struct device_attribute *attr;
	char buf[MAX_DEBUG_CMD_LEN];
	int pending;
};

static struct debug_work_struct debug_work;

static void debug_cmd_work(struct work_struct *_work)
{
	struct debug_work_struct *work = 
		container_of(_work , struct debug_work_struct, work);
	struct android_dev *dev = work->dev;

	sysfs_notify(&dev->dev->kobj, NULL, "debug_cmd");
}

static void debug_cmd_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct debug_work_struct *work = req->context;
	char *p = work->buf;
	
	if (req->status != 0) {
		pr_err("err %d\n", req->status);
		return;
	}

	if (req->actual >= MAX_DEBUG_CMD_LEN) {
		pr_err("buffer overflow %d\n", req->actual);
		return;
	}

	memcpy(p, req->buf, req->actual);
	p[req->actual] = '\0';
	pr_info("%s\n", work->buf);

	INIT_WORK(&work->work, debug_cmd_work);
	work->pending = 1;
	schedule_work(&work->work);
}

static int dbg_ctrlrequest(struct device_attribute *attr,
					struct android_dev *dev,
					struct usb_composite_dev *cdev,
					const struct usb_ctrlrequest *ctrl)
{
	int	value = -EOPNOTSUPP;
	u8 b_requestType = ctrl->bRequestType;
	u8 b_request = ctrl->bRequest;
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16	w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);
	struct debug_work_struct *work = &debug_work;

	pr_info("%02x %02x %04x %04x %04x\n",  
		b_requestType, b_request, w_index, w_value, w_length);

	if (b_requestType == (USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE)
			&& w_value == 0xffff && w_index == 0xffff) {
		if (b_request == 0x00) {
			pr_info("sysrq request!\n");
			cdev->req->complete = debug_sysrq_complete;
			value = w_length;
		} else if (b_request == 0x01) {
			pr_info("debug request!\n");	
			work->dev = dev;
			work->attr = attr;
			cdev->req->context = work;
			cdev->req->complete = debug_cmd_complete;
			value = w_length;
		}
	}
	
	if (value >= 0) {
		cdev->req->zero = 0;
		cdev->req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		if (value < 0)
			pr_err("setup response queue error %d\n", value);
	}

	return value;
}

static ssize_t debug_cmd_show(struct device *pdev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct debug_work_struct *work = &debug_work;

	if (work->pending) {
		work->pending = 0;
		ret = snprintf(buf, PAGE_SIZE, "%s", work->buf);
	}

	pr_info("ret %zu\n", ret);
	return ret;
}

static ssize_t debug_cmd_store(struct device *pdev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct debug_work_struct *work = &debug_work;

	if (!work->pending) {
		memcpy(work->buf, buf, size);
		work->buf[size] = '\0';
		pr_info("Got debug command: %s\n", work->buf);
		work->pending = 1;
		sysfs_notify(&pdev->kobj, NULL, "debug_cmd");
		return size;
	}
	return -EBUSY;
}
