// SPDX-License-Identifier: GPL-2.0
/*
 * Scheduler code and data structures related to cpufreq.
 *
 * Copyright (C) 2016, Intel Corporation
 * Author: Rafael J. Wysocki <rafael.j.wysocki@intel.com>
 */

DEFINE_PER_CPU(struct update_util_data __rcu *, cpufreq_update_util_data);
EXPORT_PER_CPU_SYMBOL_GPL(cpufreq_update_util_data);

/* DVFS headroom in percent applied by map_util_perf(); upstream is 125 (+25%). */
unsigned int sysctl_sched_util_headroom_pct = 100;

#ifdef CONFIG_SYSCTL
static unsigned int sched_util_headroom_min = 100;
static unsigned int sched_util_headroom_max = 200;

static struct ctl_table sched_util_headroom_sysctls[] = {
	{
		.procname	= "sched_util_headroom_pct",
		.data		= &sysctl_sched_util_headroom_pct,
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_douintvec_minmax,
		.extra1		= &sched_util_headroom_min,
		.extra2		= &sched_util_headroom_max,
	},
};

static int __init sched_util_headroom_sysctl_init(void)
{
	register_sysctl_init("kernel", sched_util_headroom_sysctls);
	return 0;
}
late_initcall(sched_util_headroom_sysctl_init);
#endif

/**
 * cpufreq_add_update_util_hook - Populate the CPU's update_util_data pointer.
 * @cpu: The CPU to set the pointer for.
 * @data: New pointer value.
 * @func: Callback function to set for the CPU.
 *
 * Set and publish the update_util_data pointer for the given CPU.
 *
 * The update_util_data pointer of @cpu is set to @data and the callback
 * function pointer in the target struct update_util_data is set to @func.
 * That function will be called by cpufreq_update_util() from RCU-sched
 * read-side critical sections, so it must not sleep.  @data will always be
 * passed to it as the first argument which allows the function to get to the
 * target update_util_data structure and its container.
 *
 * The update_util_data pointer of @cpu must be NULL when this function is
 * called or it will WARN() and return with no effect.
 */
void cpufreq_add_update_util_hook(int cpu, struct update_util_data *data,
			void (*func)(struct update_util_data *data, u64 time,
				     unsigned int flags))
{
	if (WARN_ON(!data || !func))
		return;

	if (WARN_ON(per_cpu(cpufreq_update_util_data, cpu)))
		return;

	data->func = func;
	rcu_assign_pointer(per_cpu(cpufreq_update_util_data, cpu), data);
}
EXPORT_SYMBOL_GPL(cpufreq_add_update_util_hook);

/**
 * cpufreq_remove_update_util_hook - Clear the CPU's update_util_data pointer.
 * @cpu: The CPU to clear the pointer for.
 *
 * Clear the update_util_data pointer for the given CPU.
 *
 * Callers must use RCU callbacks to free any memory that might be
 * accessed via the old update_util_data pointer or invoke synchronize_rcu()
 * right after this function to avoid use-after-free.
 */
void cpufreq_remove_update_util_hook(int cpu)
{
	rcu_assign_pointer(per_cpu(cpufreq_update_util_data, cpu), NULL);
}
EXPORT_SYMBOL_GPL(cpufreq_remove_update_util_hook);

/**
 * cpufreq_this_cpu_can_update - Check if cpufreq policy can be updated.
 * @policy: cpufreq policy to check.
 *
 * Return 'true' if:
 * - the local and remote CPUs share @policy,
 * - dvfs_possible_from_any_cpu is set in @policy and the local CPU is not going
 *   offline (in which case it is not expected to run cpufreq updates any more).
 */
bool cpufreq_this_cpu_can_update(struct cpufreq_policy *policy)
{
	return cpumask_test_cpu(smp_processor_id(), policy->cpus) ||
		(policy->dvfs_possible_from_any_cpu &&
		 rcu_dereference_sched(*this_cpu_ptr(&cpufreq_update_util_data)));
}
EXPORT_SYMBOL_GPL(cpufreq_this_cpu_can_update);
