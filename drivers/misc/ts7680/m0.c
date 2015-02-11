/*
 * m0.c -- driver for the M0 uC on the TS-7680 board
 *
 * Copyright (C) 2015 Technologic Systems
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/i2c.h>

#include <linux/of.h>



static int m0_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

   printk("%s %d\n", __func__, __LINE__);
   
   return 0;
}

static int m0_remove(struct i2c_client *client)
{
   
   printk("%s %d\n", __func__, __LINE__);

	
	return 0;
}

/*-------------------------------------------------------------------------*/

static const struct of_device_id m0_of_match[] = {
	{ .compatible = "ts7680,m0", },
	{ }
};
MODULE_DEVICE_TABLE(of, m0_of_match);


static const struct i2c_device_id ts7680_m0_id[] = {
	{ "ts7680_m0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ts7680_m0_id);

static struct i2c_driver m0_driver = {
	.driver = {
		.name		= "ts7680_m0",
		.owner		= THIS_MODULE,
		.of_match_table = m0_of_match,
	},
	.probe		= m0_probe,
	.remove		= m0_remove,
	.id_table = ts7680_m0_id,
};

module_i2c_driver(m0_driver);

MODULE_DESCRIPTION("Driver for m0 on TS-7680");
MODULE_AUTHOR("Technologic Systems");
MODULE_LICENSE("GPL");
MODULE_ALIAS("i2c:m0");
