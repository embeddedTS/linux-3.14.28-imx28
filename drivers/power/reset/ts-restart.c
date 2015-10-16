/*
 * Writes over the i2c bus to reset the system
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/reboot.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <asm/system_misc.h>

struct i2c_client *silabs_client;

static void do_ts_reboot(enum reboot_mode reboot_mode, const char *cmd)
{
	u8 out[3];
	int ret;
	struct i2c_msg msg;

	out[0] = 0;
	out[1] = 0;
	out[2] = 0;

	msg.addr = silabs_client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = out;

	ret = i2c_transfer(silabs_client->adapter, &msg, 1);
	if (ret != 1) {
		dev_err(&silabs_client->dev, "%s: write error, ret=%d\n",
			__func__, ret);
	}
}

static int tsreboot_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	silabs_client = client;
	arm_pm_restart = do_ts_reboot;
	return 0;
}

static const struct i2c_device_id tsreboot_id[] = {
	{ "ts-reboot", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tsreboot_id);

MODULE_ALIAS("platform:ts-reboot");

static struct i2c_driver ts_reboot_driver = {
	.driver = {
		.name	= "ts-reboot",
		.owner	= THIS_MODULE,
	},
	.probe		= tsreboot_probe,
	.id_table	= tsreboot_id,
};

static int __init ts_reboot_init(void)
{
	return i2c_add_driver(&ts_reboot_driver);
}
subsys_initcall(ts_reboot_init);

static void __exit ts_reboot_exit(void)
{
	i2c_del_driver(&ts_reboot_driver);
}
module_exit(ts_reboot_exit);

MODULE_AUTHOR("Mark Featherston <mark@embeddedarm.com>");
MODULE_DESCRIPTION("Technologic Systems reboot driver");
MODULE_LICENSE("GPL");
