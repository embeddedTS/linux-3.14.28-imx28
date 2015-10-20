/*
 * i2c watchdog
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
#include <linux/watchdog.h>
#include <linux/delay.h>
#include <asm/system_misc.h>

#define TS_DEFAULT_TIMEOUT 30

static bool nowayout = 0;

struct ts_wdt_dev {
	struct device		*dev;
	struct delayed_work	ping_work;
};

struct i2c_client *client;

/* The WDT expects 3 values:
 * 0 (always)
 * and two bytes for the feed length in deciseconds
 * 1 <MSB>
 * 2 <LSB>
 * there are also 3 special values if they are specified
 * in the LSB with a 0 MSB:
 * 0 - 200ms
 * 1 - 2s
 * 2 - 4s
 * 3 - 10s
 * 4 - disable watchdog
 */

static int ts_wdt_write(u16 deciseconds)
{
	u8 out[3];
	int ret;
	struct i2c_msg msg;

	out[0] = 0;
	out[1] = (deciseconds & 0xff00) >> 8;
	out[2] = deciseconds & 0xff;
	dev_dbg(&client->dev, "Writing 0x00, 0x%02x, 0x%02x\n",
		out[1],
		out[2]);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = out;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret != 1) {
		dev_err(&client->dev, "%s: write error, ret=%d\n",
			__func__, ret);
	}
	return !ret;
}

/* Watchdog is on by default.  We feed every timeout/2 until userspace feeds */
static void ts_wdt_ping_enable(struct ts_wdt_dev *wdev)
{
	dev_dbg(&client->dev, "%s\n", __func__);
	ts_wdt_write(TS_DEFAULT_TIMEOUT * 10);
	schedule_delayed_work(&wdev->ping_work,
			round_jiffies_relative(TS_DEFAULT_TIMEOUT * HZ / 2));
}

static void ts_wdt_ping_disable(struct ts_wdt_dev *wdev)
{
	dev_dbg(&client->dev, "%s\n", __func__);
	ts_wdt_write(TS_DEFAULT_TIMEOUT * 10);
	cancel_delayed_work_sync(&wdev->ping_work);
}

static int ts_wdt_start(struct watchdog_device *wdt)
{
	struct ts_wdt_dev *wdev = watchdog_get_drvdata(wdt);

	dev_dbg(&client->dev, "%s\n", __func__);
	dev_dbg(&client->dev, "Feeding for %d seconds\n", wdt->timeout);

	ts_wdt_ping_disable(wdev);
	return ts_wdt_write(wdt->timeout * 10);
}

static int ts_wdt_stop(struct watchdog_device *wdt)
{
	dev_dbg(&client->dev, "%s\n", __func__);
	return ts_wdt_write(0);
}

static void do_ts_reboot(enum reboot_mode reboot_mode, const char *cmd)
{
	unsigned long flags;
	static DEFINE_SPINLOCK(wdt_lock);

	dev_dbg(&client->dev, "%s\n", __func__);

	spin_lock_irqsave(&wdt_lock, flags);
	ts_wdt_write(0);
	while (1);
}

static int ts_set_timeout(struct watchdog_device *wdt,
				   unsigned int timeout)
{
	dev_dbg(&client->dev, "%s\n", __func__);
	wdt->timeout = timeout;
	return 0;
}

static void ts_wdt_ping_work(struct work_struct *work)
{
	struct ts_wdt_dev *wdev = container_of(to_delayed_work(work),
						struct ts_wdt_dev, ping_work);
	dev_dbg(&client->dev, "%s\n", __func__);
	ts_wdt_ping_enable(wdev);
}

static struct watchdog_info ts_wdt_ident = {
	.options	= WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity	= "Technologic Micro Watchdog",
};

static struct watchdog_ops ts_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= ts_wdt_start,
	.stop		= ts_wdt_stop,
	.set_timeout	= ts_set_timeout,
};

static struct watchdog_device ts_wdt_wdd = {
	.info			= &ts_wdt_ident,
	.ops			= &ts_wdt_ops,
	.min_timeout		= 1,
	.timeout		= TS_DEFAULT_TIMEOUT,
	.max_timeout		= 6553,
};

static int tsreboot_probe(struct i2c_client *c,
			  const struct i2c_device_id *id)
{
	int err;
	struct ts_wdt_dev *wdev;
	client = c;

	wdev = devm_kzalloc(&client->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	arm_pm_restart = do_ts_reboot;
	dev_dbg(&client->dev, "%s\n", __func__);

	watchdog_set_drvdata(&ts_wdt_wdd, wdev);
	watchdog_set_nowayout(&ts_wdt_wdd, nowayout);

	INIT_DELAYED_WORK(&wdev->ping_work, ts_wdt_ping_work);

	err = watchdog_register_device(&ts_wdt_wdd);
	if (err)
		return err;

	if (nowayout)
		ts_wdt_write(300);
	else
		ts_wdt_ping_enable(wdev);

	return 0;
}

static const struct i2c_device_id tsreboot_id[] = {
	{ "ts-wdt", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tsreboot_id);

MODULE_ALIAS("platform:ts-wdt");

static struct i2c_driver ts_reboot_driver = {
	.driver = {
		.name	= "ts-wdt",
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
MODULE_DESCRIPTION("Technologic Systems watchdog driver");
MODULE_LICENSE("GPL");
