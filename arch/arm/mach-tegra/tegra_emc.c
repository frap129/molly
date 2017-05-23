/*
 * arch/arm/mach-tegra/tegra_emc.c
 *
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include "tegra_emc.h"

static u8 emc_iso_share = 100;

static struct emc_iso_usage emc_usage_table[TEGRA_EMC_ISO_USE_CASES_MAX_NUM];


void __init tegra_emc_iso_usage_table_init(struct emc_iso_usage *table,
					   int size)
{
	size = min(size, TEGRA_EMC_ISO_USE_CASES_MAX_NUM);

	if (size && table)
		memcpy(emc_usage_table, table,
		       size * sizeof(struct emc_iso_usage));
}

u8 tegra_emc_get_iso_share(u32 usage_flags)
{
	int i;
	u8 iso_share = 100;

	if (usage_flags) {
		for (i = 0; i < TEGRA_EMC_ISO_USE_CASES_MAX_NUM; i++) {
			struct emc_iso_usage *iso_usage = &emc_usage_table[i];
			u32 flags = iso_usage->emc_usage_flags;
			u8 share = iso_usage->iso_usage_share;

			if (!flags)
				continue;
			if (!share) {
				WARN(1, "%s: entry %d: iso_share 0\n",
				     __func__, i);
				continue;
			}

			if ((flags & usage_flags) == flags)
				iso_share = min(iso_share,
						iso_usage->iso_usage_share);
		}
	}
	emc_iso_share = iso_share;
	return iso_share;
}

#ifdef CONFIG_DEBUG_FS

#define USER_NAME(module) \
[EMC_USER_##module] = #module

static const char *emc_user_names[EMC_USER_NUM] = {
	USER_NAME(DC),
	USER_NAME(VI),
	USER_NAME(MSENC),
	USER_NAME(2D),
	USER_NAME(3D),
};

static int emc_usage_table_show(struct seq_file *s, void *data)
{
	int i, j;

	seq_printf(s, "EMC USAGE\t\tISO SHARE %%\n");

	for (i = 0; i < TEGRA_EMC_ISO_USE_CASES_MAX_NUM; i++) {
		u32 flags = emc_usage_table[i].emc_usage_flags;
		u8 share = emc_usage_table[i].iso_usage_share;
		bool first = false;

		seq_printf(s, "[%d]: ", i);
		if (!flags) {
			seq_printf(s, "reserved\n");
			continue;
		}

		for (j = 0; j < EMC_USER_NUM; j++) {
			u32 mask = 0x1 << j;
			if (!(flags & mask))
				continue;
			seq_printf(s, "%s%s", first ? "+" : "",
				   emc_user_names[j]);
			first = true;
		}
		seq_printf(s, "\r\t\t\t= %d\n", share);
	}
	return 0;
}

static int emc_usage_table_open(struct inode *inode, struct file *file)
{
	return single_open(file, emc_usage_table_show, inode->i_private);
}

static const struct file_operations emc_usage_table_fops = {
	.open		= emc_usage_table_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int __init tegra_emc_iso_usage_debugfs_init(struct dentry *emc_debugfs_root)
{
	struct dentry *d;

	d = debugfs_create_file("emc_usage_table", S_IRUGO, emc_debugfs_root,
		NULL, &emc_usage_table_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_u8("emc_iso_share", S_IRUGO, emc_debugfs_root,
			      &emc_iso_share);
	if (!d)
		return -ENOMEM;

	return 0;
}
#endif
