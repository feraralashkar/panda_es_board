// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * OMAP4460_ES test bench
 *
 * Copyright (C) 2021 Ferar Ashkar
 */

#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for printing .. */
#include <linux/slab.h> /* Needed for kmalloc .. */
#include <linux/time.h> /* Needed for wall time .. */
#include <linux/hrtimer.h> /* Needed for hrtimer .. */
#include <linux/ktime.h> /* Needed for ktime obj .. */
#include <linux/list.h> /* Needed for list_head obj .. */
#include <linux/wait.h> /* Needed for waitqueue .. */
#include <linux/sched.h> /* Needed for tasklet scheduling .. */
#include <linux/kthread.h> /* Needed for kthread */
#include <linux/delay.h> /* Needed for msleep */
#include <linux/completion.h> /* Needed for completion */
#include <linux/io.h> /* Needed for ioremap */
#include <linux/hashtable.h> /* Needed for hlist */
#include <linux/kfifo.h> /* Needed for kfifo */
#include <linux/log2.h> /* Needed for roundup_pow_of_two */
#include <linux/rbtree.h> /* Needed for Red-Black tree */
#include <linux/gpio/consumer.h> /* this is required to be gpiolib consumer */
#include <linux/gpio.h>
#include <linux/platform_device.h> /* for platform device/driver */
#include <linux/random.h> /* add random support */
#include <linux/rcupdate.h> /* rcu related */

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("This is direct reg-ctrl gpio testbench ;/");
MODULE_AUTHOR("ferar ashkar");
MODULE_VERSION("1.5.0");

static struct task_struct *ptr_my_task_struct1;
static atomic_t atomic_thread1;

static struct gpio_desc *pgpio_desc;

static void __iomem *CONTROL_CORE_PAD0_GPMC_NBE1_PAD1_GPMC_WAIT0;

DECLARE_WAIT_QUEUE_HEAD(wait_queue_head);

static int my_kthread_func1(void *my_arg)
{
	ktime_t entering, exiting;
	s64 deltao = 0;
	unsigned int cntro = 0;
	u32 temp_idreg = 0x0;
	int rslt = -1;
	unsigned int rndm_uint = 0;

	pr_debug(" >> entering %s\n", __func__);

	if (in_irq())
		pr_debug(" >> %s inirq\n", __func__);

	if (in_softirq())
		pr_debug(" >> %s in_softirq\n", __func__);

	if (in_interrupt())
		pr_debug(" >> %s in_interrupt\n", __func__);

	entering = ktime_get();

	rndm_uint = get_random_int() >> 15;
	udelay(rndm_uint);

	/**
	 * /home/ferar/Downloads/OMAP4460_ES.1x_PUBLIC_TRM_vM.pdf
	 * page 3829
	 * Table 18-387. CONTROL_CORE_PAD0_GPMC_NBE1_PAD1_GPMC_WAIT0
	 */

	rslt = gpio_request_one(61, GPIOF_OUT_INIT_LOW, "gpio-61");

	if (rslt == 0) {
		pgpio_desc = gpio_to_desc(61);
		if (!IS_ERR(pgpio_desc)) {
			gpiod_direction_output(pgpio_desc, 1);
			CONTROL_CORE_PAD0_GPMC_NBE1_PAD1_GPMC_WAIT0 =
				ioremap(0x4a100088, 0x020);
			if (CONTROL_CORE_PAD0_GPMC_NBE1_PAD1_GPMC_WAIT0 != NULL) {
				temp_idreg = ioread32(
					CONTROL_CORE_PAD0_GPMC_NBE1_PAD1_GPMC_WAIT0);
				pr_debug(" %s, temp_idreg : 0x%x\n", __func__,
					 temp_idreg);

				iowrite32(
					0x11b0003,
					CONTROL_CORE_PAD0_GPMC_NBE1_PAD1_GPMC_WAIT0);

				while (!kthread_should_stop() &&
				       cntro++ < 500) {
					gpiod_set_value_cansleep(pgpio_desc, 1);
					wait_event_interruptible_timeout(
						wait_queue_head, 0 == 1,
						msecs_to_jiffies(100));

					gpiod_set_value_cansleep(pgpio_desc, 0);
					wait_event_interruptible_timeout(
						wait_queue_head, 0 == 1,
						msecs_to_jiffies(100));

					gpiod_set_value_cansleep(pgpio_desc, 1);
					wait_event_interruptible_timeout(
						wait_queue_head, 0 == 1,
						msecs_to_jiffies(100));

					gpiod_set_value_cansleep(pgpio_desc, 0);
					wait_event_interruptible_timeout(
						wait_queue_head, 0 == 1,
						msecs_to_jiffies(1000));
				}

				iounmap(CONTROL_CORE_PAD0_GPMC_NBE1_PAD1_GPMC_WAIT0);
			}

		} else {
			pr_debug(" %s, error:%ld\n", __func__,
				 PTR_ERR(pgpio_desc));
		}

		gpio_free(61);

	} else {
		pr_debug(
			" %s warning: could not ioremap CONTROL_CORE_PAD0_GPMC_NBE1_PAD1_GPMC_WAIT0=0x4a100088!\n",
			__func__);
	}

	exiting = ktime_get();
	deltao = ktime_to_ns(ktime_sub(exiting, entering));
	pr_debug(" << exiting %s, time took : %lld ns\n", __func__, deltao);
	atomic_set(&atomic_thread1, 1);
	do_exit(0);
}

int __init my_init(void)
{
	ktime_t entering, exiting;
	s64 deltao = 0;

	pr_debug(" >> entering %s\n", __func__);

	if (in_irq())
		pr_debug(" >> %s inirq\n", __func__);

	if (in_softirq())
		pr_debug(" >> %s in_softirq\n", __func__);

	if (in_interrupt())
		pr_debug(" >> %s in_interrupt\n", __func__);

	entering = ktime_get();

	atomic_set(&atomic_thread1, 0);

	/* create a kthread in initial sleep mode */
	ptr_my_task_struct1 =
		kthread_create(my_kthread_func1, NULL, "%s", "my_kthreado1");

	if (ptr_my_task_struct1 != NULL)
		wake_up_process(ptr_my_task_struct1);

	exiting = ktime_get();
	deltao = ktime_to_ns(ktime_sub(exiting, entering));
	pr_debug(" << exiting %s, time took : %lld ns\n", __func__, deltao);
	return 0;
}

void __exit my_exit(void)
{
	ktime_t entering, exiting;
	s64 deltao = 0;

	entering = ktime_get();

	pr_debug("  >> entering %s\n", __func__);

	if (ptr_my_task_struct1 != NULL)
		if (atomic_read(&atomic_thread1) == 0)
			kthread_stop(ptr_my_task_struct1);

	exiting = ktime_get();
	deltao = ktime_to_ns(ktime_sub(exiting, entering));
	pr_debug(" << exiting %s, time took : %lld ns\n", __func__, deltao);
}

module_init(my_init);
module_exit(my_exit);
