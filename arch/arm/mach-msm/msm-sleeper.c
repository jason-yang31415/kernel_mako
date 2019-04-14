/*
 * ElementalX msm-sleeper by flar2 <asegaert@gmail.com>
 * 
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

#include <linux/earlysuspend.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <mach/cpufreq.h>

#ifdef CONFIG_FB
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#define MSM_SLEEPER_MAJOR_VERSION	1
#define MSM_SLEEPER_MINOR_VERSION	1

extern uint32_t maxscroff;
extern uint32_t maxscroff_freq;
static int limit_set = 0;

#ifdef CONFIG_FB
struct notifier_block sleeper_fb_notif;
static int sleeper_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);
#endif

#ifdef CONFIG_FB
static void msm_sleeper_fb_suspend(void)
{
	int cpu;

	if (maxscroff) {
		for_each_possible_cpu(cpu) {
			msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, maxscroff_freq);
			pr_info("msm-sleeper: limit max frequency to: %d\n", maxscroff_freq);
		}
		limit_set = 1;
	}
	return; 
}

static void msm_sleeper_fb_resume(void)
{
	int cpu;

	if (!limit_set)
		return;

	for_each_possible_cpu(cpu) {
		msm_cpufreq_set_freq_limits(cpu, MSM_CPUFREQ_NO_LIMIT, MSM_CPUFREQ_NO_LIMIT);
		pr_info("msm-sleeper: restore max frequency.\n");
	}
	limit_set = 0;
	return; 
}
#endif

static int __init msm_sleeper_init(void)
{
	pr_info("msm-sleeper version %d.%d\n",
		 MSM_SLEEPER_MAJOR_VERSION,
		 MSM_SLEEPER_MINOR_VERSION);

#ifdef CONFIG_FB
	sleeper_fb_notif.notifier_call = sleeper_fb_notifier_callback;
	fb_register_client(&sleeper_fb_notif);
#endif
	return 0;
}

#ifdef CONFIG_FB
static int sleeper_fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;

	if (evdata && evdata->data) {
		if (event == FB_EVENT_BLANK) {
			blank = evdata->data;
			switch (*blank) {
				case FB_BLANK_UNBLANK:
				case FB_BLANK_NORMAL:
				case FB_BLANK_VSYNC_SUSPEND:
				case FB_BLANK_HSYNC_SUSPEND:
					msm_sleeper_fb_resume();
					break;
				default:
				case FB_BLANK_POWERDOWN:
					msm_sleeper_fb_suspend();
					break;
			}
		}
	}

	return 0;
}
#endif

MODULE_AUTHOR("flar2 <asegaert at gmail.com>");
MODULE_DESCRIPTION("'msm-sleeper' - Limit max frequency while screen is off");
MODULE_LICENSE("GPL");

late_initcall(msm_sleeper_init);

