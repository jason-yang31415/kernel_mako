/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/msm_tsens.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/reboot.h>
#include <linux/cpufreq.h>
#include <linux/msm_tsens.h>
#include <linux/msm_thermal.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/hrtimer.h>
#include <mach/cpufreq.h>

static DEFINE_MUTEX(emergency_shutdown_mutex);

static int enabled;

//Throttling indicator, 0=not throttled, 1=low, 2=mid, 3=max
static int thermal_throttled = 0;

//Save the cpu max freq before throttling
static int pre_throttled_max = 0;

static struct msm_thermal_data msm_thermal_info;
<<<<<<< HEAD

static struct msm_thermal_stat msm_thermal_stats = {
    .time_low_start = 0,
    .time_mid_start = 0,
    .time_max_start = 0,
    .time_low = 0,
    .time_mid = 0,
    .time_max = 0,
};

struct thermal_counter {
    unsigned int low;
    unsigned int med;
    unsigned int high;
} counter = {
    .low = 0,
    .med = 0,
    .high = 0
};

static struct delayed_work check_temp_work;
//static struct workqueue_struct *check_temp_workq;

=======

static struct msm_thermal_stat msm_thermal_stats = {
    .time_low_start = 0,
    .time_mid_start = 0,
    .time_max_start = 0,
    .time_low = 0,
    .time_mid = 0,
    .time_max = 0,
};

static struct delayed_work check_temp_work;
//static struct workqueue_struct *check_temp_workq;

>>>>>>> 7d277a226abddb689c44c5b23a5e86d12a2aec8c
static void update_stats(void)
{
    if (msm_thermal_stats.time_low_start > 0) {
        msm_thermal_stats.time_low += (ktime_to_ms(ktime_get()) - msm_thermal_stats.time_low_start);
        msm_thermal_stats.time_low_start = 0;
    }
    if (msm_thermal_stats.time_mid_start > 0) {
        msm_thermal_stats.time_mid += (ktime_to_ms(ktime_get()) - msm_thermal_stats.time_mid_start);
        msm_thermal_stats.time_mid_start = 0;
    }
    if (msm_thermal_stats.time_max_start > 0) {
        msm_thermal_stats.time_max += (ktime_to_ms(ktime_get()) - msm_thermal_stats.time_max_start);
        msm_thermal_stats.time_max_start = 0;
    }
}

static void start_stats(int status)
{
    switch (thermal_throttled) {
        case 1:
            msm_thermal_stats.time_low_start = ktime_to_ms(ktime_get());
            break;
        case 2:
            msm_thermal_stats.time_mid_start = ktime_to_ms(ktime_get());
            break;
        case 3:
            msm_thermal_stats.time_max_start = ktime_to_ms(ktime_get());
            break;
    }
}

static int update_cpu_max_freq(struct cpufreq_policy *cpu_policy,
                   int cpu, int max_freq)
{
    int ret = 0;

    if (!cpu_policy)
        return -EINVAL;

    cpufreq_verify_within_limits(cpu_policy, cpu_policy->min, max_freq);
    cpu_policy->user_policy.max = max_freq;

    ret = cpufreq_update_policy(cpu);
    if (!ret)
        pr_debug("msm_thermal: Setting CPU%d max frequency to %d\n",
                 cpu, max_freq);
    return ret;
}

static void check_temp(struct work_struct *work)
{
    struct cpufreq_policy *cpu_policy = NULL;
    struct tsens_device tsens_dev;
    unsigned long temp = 0;
    uint32_t max_freq = 0;
    bool update_policy = false;
    int cpu = 0, ret = 0;

    tsens_dev.sensor_num = msm_thermal_info.sensor_id;
    ret = tsens_get_temp(&tsens_dev, &temp);
    if (ret) {
        pr_err("msm_thermal: FATAL: Unable to read TSENS sensor %d\n",
               tsens_dev.sensor_num);
        goto reschedule;
    }
    pr_debug("msm_thermal: check_temp");
    if (temp >= msm_thermal_info.shutdown_temp) {
        mutex_lock(&emergency_shutdown_mutex);
        pr_warn("################################\n");
        pr_warn("################################\n");
        pr_warn("- %u OVERTEMP! SHUTTING DOWN! -\n", msm_thermal_info.shutdown_temp);
        pr_warn("- cur temp:%lu measured by:%u -\n", temp, msm_thermal_info.sensor_id);
        pr_warn("################################\n");
        pr_warn("################################\n");
        /* orderly poweroff tries to power down gracefully
           if it fails it will force it. */
        orderly_poweroff(true);
        for_each_possible_cpu(cpu) {
            update_policy = true;
            max_freq = msm_thermal_info.allowed_max_freq;
            thermal_throttled = 3;
            pr_warn("msm_thermal: Emergency throttled CPU%i to %u! temp:%lu\n",
                    cpu, msm_thermal_info.allowed_max_freq, temp);
        }
        mutex_unlock(&emergency_shutdown_mutex);
    }

    cpu_policy = cpufreq_cpu_get(0);
    if (cpu_policy && thermal_throttled == 0)
        pre_throttled_max = cpu_policy->max;

    if (temp > msm_thermal_info.allowed_low_high){
        if (counter.low < msm_thermal_info.counter_max)
            counter.low++;
    } else if (temp < msm_thermal_info.allowed_low_low && counter.low > 0){
        counter.low--;
    }

    if (temp > msm_thermal_info.allowed_mid_high){
        if (counter.med < msm_thermal_info.counter_max)
            counter.med++;
    } else if (temp < msm_thermal_info.allowed_mid_low && counter.med > 0){
        counter.med--;
    }

    if (temp > msm_thermal_info.allowed_max_high){
        if (counter.high < msm_thermal_info.counter_max)
            counter.high++;
    } else if (temp < msm_thermal_info.allowed_max_low && counter.high > 0){
        counter.high--;
    }

    update_policy = false;
    if (counter.high > msm_thermal_info.counter_limit){
        if (thermal_throttled != 3){
            update_policy = true;
            max_freq = msm_thermal_info.allowed_max_freq;
            thermal_throttled = 3;
            pr_warn("msm_thermal: Thermal Throttled (max)! temp:%lu by:%u\n",
                        temp, msm_thermal_info.sensor_id);
        }
    } else if (counter.med > msm_thermal_info.counter_limit){
        if (thermal_throttled != 2){
            update_policy = true;
            max_freq = msm_thermal_info.allowed_mid_freq;
            thermal_throttled = 2;
            pr_warn("msm_thermal: Thermal Throttled (mid)! temp:%lu by:%u\n",
                        temp, msm_thermal_info.sensor_id);
        }
    } else if (counter.low > msm_thermal_info.counter_limit){
        if (thermal_throttled != 1){
            update_policy = true;
            max_freq = msm_thermal_info.allowed_low_freq;
            thermal_throttled = 1;
            pr_warn("msm_thermal: Thermal Throttled (low)! temp:%lu by:%u\n",
                        temp, msm_thermal_info.sensor_id);
        }
    } else {
        if (thermal_throttled != 0){
            update_policy = true;
            if (pre_throttled_max != 0)
                max_freq = pre_throttled_max;
            else {
                max_freq = CONFIG_MSM_CPU_FREQ_MAX;
                pr_warn("msm_thermal: ERROR! pre_throttled_max=0, falling back to %u\n", max_freq);
            }
            thermal_throttled = 0;
            pr_info("msm_thermal: Thermal Throttle ended! temp:%lu by:%u\n", temp, msm_thermal_info.sensor_id);
        }
    }

    if (counter.low > 0)
        pr_info("msm_thermal: counters l %u m %u h %u\n", counter.low, counter.med, counter.high);

    update_stats();
    start_stats(thermal_throttled);
    if (update_policy){
        pr_info("msm_thermal: setting max_freq to %d", max_freq);
        for_each_possible_cpu(cpu){
            cpu_policy = cpufreq_cpu_get(cpu);
            if (!cpu_policy){
                pr_debug("msm_thermal: NULL policy on cpu %d\n", cpu);
                continue;
            }
            update_cpu_max_freq(cpu_policy, cpu, max_freq);
            cpufreq_cpu_put(cpu_policy);
        }
    }

reschedule:
    if (enabled)
        //queue_delayed_work(check_temp_workq, &check_temp_work,
        //                   msecs_to_jiffies(msm_thermal_info.poll_ms));
        schedule_delayed_work(&check_temp_work,
				msecs_to_jiffies(msm_thermal_info.poll_ms));

    return;
}

static void disable_msm_thermal(void)
{
    int cpu = 0;
    struct cpufreq_policy *cpu_policy = NULL;

     enabled = 0;
    /* make sure check_temp is no longer running */
    cancel_delayed_work(&check_temp_work);
    flush_scheduled_work();

    if (pre_throttled_max != 0) {
        for_each_possible_cpu(cpu) {
            cpu_policy = cpufreq_cpu_get(cpu);
            if (cpu_policy) {
                if (cpu_policy->max < cpu_policy->cpuinfo.max_freq)
                    update_cpu_max_freq(cpu_policy, cpu, pre_throttled_max);
                cpufreq_cpu_put(cpu_policy);
            }
        }
    }

   pr_warn("msm_thermal: Warning! Thermal guard disabled!");
}

static void enable_msm_thermal(void)
{
    enabled = 1;
    /* make sure check_temp is running */
    //queue_delayed_work(check_temp_workq, &check_temp_work,
    schedule_delayed_work(&check_temp_work,
                       msecs_to_jiffies(msm_thermal_info.poll_ms));

    pr_info("msm_thermal: Thermal guard enabled.");
}

static int set_enabled(const char *val, const struct kernel_param *kp)
{
    int ret = 0;

    ret = param_set_bool(val, kp);
    if (!enabled)
        disable_msm_thermal();
    else if (enabled == 1)
        enable_msm_thermal();
    else
        pr_info("msm_thermal: no action for enabled = %d\n", enabled);

    pr_info("msm_thermal: enabled = %d\n", enabled);

    return ret;
}

static struct kernel_param_ops module_ops = {
    .set = set_enabled,
    .get = param_get_bool,
};

module_param_cb(enabled, &module_ops, &enabled, 0644);
MODULE_PARM_DESC(enabled, "enforce thermal limit on cpu");

/**************************** SYSFS START ****************************/
struct kobject *msm_thermal_kobject;

#define show_one(file_name, object)                             \
static ssize_t show_##file_name                                 \
(struct kobject *kobj, struct attribute *attr, char *buf)       \
{                                                               \
    return sprintf(buf, "%u\n", msm_thermal_info.object);       \
}

show_one(shutdown_temp, shutdown_temp);
show_one(allowed_max_high, allowed_max_high);
show_one(allowed_max_low, allowed_max_low);
show_one(allowed_max_freq, allowed_max_freq);
show_one(allowed_mid_high, allowed_mid_high);
show_one(allowed_mid_low, allowed_mid_low);
show_one(allowed_mid_freq, allowed_mid_freq);
show_one(allowed_low_high, allowed_low_high);
show_one(allowed_low_low, allowed_low_low);
show_one(allowed_low_freq, allowed_low_freq);
show_one(poll_ms, poll_ms);
show_one(counter_limit, counter_limit);
show_one(counter_max, counter_max);

static ssize_t store_shutdown_temp(struct kobject *a, struct attribute *b,
                                   const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.shutdown_temp = input;

    return count;
}

static ssize_t store_allowed_max_high(struct kobject *a, struct attribute *b,
                                      const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_max_high = input;

    return count;
}

static ssize_t store_allowed_max_low(struct kobject *a, struct attribute *b,
                                     const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_max_low = input;

    return count;
}

static ssize_t store_allowed_max_freq(struct kobject *a, struct attribute *b,
                                      const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_max_freq = input;

    return count;
}

static ssize_t store_allowed_mid_high(struct kobject *a, struct attribute *b,
                                      const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_mid_high = input;

    return count;
}

static ssize_t store_allowed_mid_low(struct kobject *a, struct attribute *b,
                                     const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_mid_low = input;

    return count;
}

static ssize_t store_allowed_mid_freq(struct kobject *a, struct attribute *b,
                                      const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_mid_freq = input;

    return count;
}

static ssize_t store_allowed_low_high(struct kobject *a, struct attribute *b,
                                      const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_low_high = input;

    return count;
}

static ssize_t store_allowed_low_low(struct kobject *a, struct attribute *b,
                                     const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_low_low = input;

    return count;
}

static ssize_t store_allowed_low_freq(struct kobject *a, struct attribute *b,
                                      const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.allowed_low_freq = input;

    return count;
}

static ssize_t store_poll_ms(struct kobject *a, struct attribute *b,
                             const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    msm_thermal_info.poll_ms = input;

    return count;
}

static ssize_t store_counter_limit(struct kobject *a, struct attribute *b,
                                   const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    
    msm_thermal_info.counter_limit = input;

    return count;
}

static ssize_t store_counter_max(struct kobject *a, struct attribute *b,
                                   const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    
    msm_thermal_info.counter_max = input;

    return count;
}

define_one_global_rw(shutdown_temp);
define_one_global_rw(allowed_max_high);
define_one_global_rw(allowed_max_low);
define_one_global_rw(allowed_max_freq);
define_one_global_rw(allowed_mid_high);
define_one_global_rw(allowed_mid_low);
define_one_global_rw(allowed_mid_freq);
define_one_global_rw(allowed_low_high);
define_one_global_rw(allowed_low_low);
define_one_global_rw(allowed_low_freq);
define_one_global_rw(poll_ms);
define_one_global_rw(counter_limit);
define_one_global_rw(counter_max);

static struct attribute *msm_thermal_attributes[] = {
    &shutdown_temp.attr,
    &allowed_max_high.attr,
    &allowed_max_low.attr,
    &allowed_max_freq.attr,
    &allowed_mid_high.attr,
    &allowed_mid_low.attr,
    &allowed_mid_freq.attr,
    &allowed_low_high.attr,
    &allowed_low_low.attr,
    &allowed_low_freq.attr,
    &poll_ms.attr,
    &counter_limit.attr,
    &counter_max.attr,
    NULL
};


static struct attribute_group msm_thermal_attr_group = {
    .attrs = msm_thermal_attributes,
    .name = "conf",
};

/********* STATS START *********/

static ssize_t show_throttle_times(struct kobject *a, struct attribute *b,
                                 char *buf)
{
    ssize_t len = 0;

    if (thermal_throttled == 1) {
        len += sprintf(buf + len, "%s %llu\n", "low",
                       (msm_thermal_stats.time_low +
                        (ktime_to_ms(ktime_get()) -
                         msm_thermal_stats.time_low_start)));
    } else
        len += sprintf(buf + len, "%s %llu\n", "low", msm_thermal_stats.time_low);

    if (thermal_throttled == 2) {
        len += sprintf(buf + len, "%s %llu\n", "mid",
                       (msm_thermal_stats.time_mid +
                        (ktime_to_ms(ktime_get()) -
                         msm_thermal_stats.time_mid_start)));
    } else
        len += sprintf(buf + len, "%s %llu\n", "mid", msm_thermal_stats.time_mid);

    if (thermal_throttled == 3) {
        len += sprintf(buf + len, "%s %llu\n", "max",
                       (msm_thermal_stats.time_max +
                        (ktime_to_ms(ktime_get()) -
                         msm_thermal_stats.time_max_start)));
    } else
        len += sprintf(buf + len, "%s %llu\n", "max", msm_thermal_stats.time_max);

    return len;
}
define_one_global_ro(throttle_times);

static ssize_t show_is_throttled(struct kobject *a, struct attribute *b,
                                 char *buf)
{
    return sprintf(buf, "%u\n", thermal_throttled);
}
define_one_global_ro(is_throttled);

static struct attribute *msm_thermal_stats_attributes[] = {
    &is_throttled.attr,
    &throttle_times.attr,
    NULL
};


static struct attribute_group msm_thermal_stats_attr_group = {
    .attrs = msm_thermal_stats_attributes,
    .name = "stats",
};
/**************************** SYSFS END ****************************/

int __devinit msm_thermal_init(struct msm_thermal_data *pdata)
{
    int ret = 0, rc = 0, queue_ret = 0;

    pr_debug("msm_thermal: msm_thermal_init");

    BUG_ON(!pdata);
    BUG_ON(pdata->sensor_id >= TSENS_MAX_SENSORS);
    memcpy(&msm_thermal_info, pdata, sizeof(struct msm_thermal_data));

    // set counter limit to 10
    msm_thermal_info.counter_limit = 30;
    msm_thermal_info.counter_max = 60;
    counter.low = 0;
    counter.med = 0;
    counter.high = 0;

    enabled = 1;
    /*check_temp_workq=alloc_workqueue("msm_thermal", WQ_UNBOUND | WQ_RESCUER, 1);
    if (!check_temp_workq)
        BUG_ON(ENOMEM);*/
    INIT_DELAYED_WORK(&check_temp_work, check_temp);
    //queue_ret = queue_delayed_work(check_temp_workq, &check_temp_work, 0);
    queue_ret = schedule_delayed_work(&check_temp_work, 0);
    if (queue_ret)
        pr_warn("msm_thermal: queue_delayed_work returned %d", queue_ret);
    else
        pr_debug("msm_thermal: queue_delayed_work returned %d", queue_ret);

    msm_thermal_kobject = kobject_create_and_add("msm_thermal", kernel_kobj);
    if (msm_thermal_kobject) {
        rc = sysfs_create_group(msm_thermal_kobject, &msm_thermal_attr_group);
        if (rc) {
            pr_warn("msm_thermal: sysfs: ERROR, could not create sysfs group");
        }
        rc = sysfs_create_group(msm_thermal_kobject,
                                &msm_thermal_stats_attr_group);
        if (rc) {
            pr_warn("msm_thermal: sysfs: ERROR, could not create sysfs stats group");
        }
    } else
        pr_warn("msm_thermal: sysfs: ERROR, could not create sysfs kobj");

    return ret;
}

static int __devinit msm_thermal_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	char *key = NULL;
	struct device_node *node = pdev->dev.of_node;
	struct msm_thermal_data data;

    pr_debug("msm_thermal: msm_thermal_dev_probe");

	memset(&data, 0, sizeof(struct msm_thermal_data));
	key = "qcom,sensor-id";
	ret = of_property_read_u32(node, key, &data.sensor_id);
	if (ret)
		goto fail;
	WARN_ON(data.sensor_id >= TSENS_MAX_SENSORS);

	key = "qcom,poll-ms";
	ret = of_property_read_u32(node, key, &data.poll_ms);
	if (ret)
		goto fail;

	key = "qcom,shutdown_temp";
	ret = of_property_read_u32(node, key, &data.shutdown_temp);
	if (ret)
		goto fail;

	key = "qcom,allowed_max_high";
	ret = of_property_read_u32(node, key, &data.allowed_max_high);
	if (ret)
		goto fail;
	key = "qcom,allowed_max_low";
	ret = of_property_read_u32(node, key, &data.allowed_max_low);
	if (ret)
		goto fail;
	key = "qcom,allowed_max_freq";
	ret = of_property_read_u32(node, key, &data.allowed_max_freq);
	if (ret)
		goto fail;

	key = "qcom,allowed_mid_high";
	ret = of_property_read_u32(node, key, &data.allowed_mid_high);
	if (ret)
		goto fail;
	key = "qcom,allowed_mid_low";
	ret = of_property_read_u32(node, key, &data.allowed_mid_low);
	if (ret)
		goto fail;
	key = "qcom,allowed_mid_freq";
	ret = of_property_read_u32(node, key, &data.allowed_mid_freq);
	if (ret)
		goto fail;

	key = "qcom,allowed_low_high";
	ret = of_property_read_u32(node, key, &data.allowed_low_high);
	if (ret)
		goto fail;
	key = "qcom,allowed_low_low";
	ret = of_property_read_u32(node, key, &data.allowed_low_low);
	if (ret)
		goto fail;
	key = "qcom,allowed_low_freq";
	ret = of_property_read_u32(node, key, &data.allowed_low_freq);
	if (ret)
		goto fail;

fail:
	if (ret)
		pr_err("%s: Failed reading node=%s, key=%s\n",
		       __func__, node->full_name, key);
	else
		ret = msm_thermal_init(&data);

	return ret;
}

static struct of_device_id msm_thermal_match_table[] = {
	{.compatible = "qcom,msm-thermal"},
	{},
};

static struct platform_driver msm_thermal_device_driver = {
	.probe = msm_thermal_dev_probe,
	.driver = {
		.name = "msm-thermal",
		.owner = THIS_MODULE,
		.of_match_table = msm_thermal_match_table,
	},
};

int __init msm_thermal_device_init(void)
{
    pr_debug("msm_thermal: driver init");
	return platform_driver_register(&msm_thermal_device_driver);
}

fs_initcall(msm_thermal_device_init);