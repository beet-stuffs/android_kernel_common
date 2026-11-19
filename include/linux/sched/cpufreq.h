/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_CPUFREQ_H
#define _LINUX_SCHED_CPUFREQ_H

#include <linux/types.h>
#include <linux/compiler.h>

/*
 * Interface between cpufreq drivers and the scheduler:
 */

#define SCHED_CPUFREQ_IOWAIT	(1U << 0)

#ifdef CONFIG_CPU_FREQ
struct cpufreq_policy;

struct update_util_data {
       void (*func)(struct update_util_data *data, u64 time, unsigned int flags);
};

void cpufreq_add_update_util_hook(int cpu, struct update_util_data *data,
                       void (*func)(struct update_util_data *data, u64 time,
				    unsigned int flags));
void cpufreq_remove_update_util_hook(int cpu);
bool cpufreq_this_cpu_can_update(struct cpufreq_policy *policy);

static inline unsigned long map_util_freq(unsigned long util,
					unsigned long freq, unsigned long cap)
{
	return freq * util / cap;
}

/*
 * DVFS headroom added to utilization before it is mapped to a frequency,
 * in percent (125 == upstream's util + util/4, 100 == no headroom, 50..200 allowed).
 * Runtime tunable: /proc/sys/kernel/sched_util_headroom_pct
 */
extern unsigned int sysctl_sched_util_headroom_pct;

static inline unsigned long map_util_perf(unsigned long util)
{
	return util * READ_ONCE(sysctl_sched_util_headroom_pct) / 100;
}
#endif /* CONFIG_CPU_FREQ */

#endif /* _LINUX_SCHED_CPUFREQ_H */
