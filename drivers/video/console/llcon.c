/**
 * @file drivers/video/console/llcon.c
 *
 * @brief
 * This file contains the implementation of Low Level Console
 * for output kernel messages to display.
 * This module support only 24-bit color format.
 *
 * @author	remittor <remittor@gmail.com>
 * @date	2016-05-11
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/memblock.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/kmsg_dump.h>

#include <video/llcon.h>

volatile int llcon_enabled = 0;
volatile int llcon_dumplog = 0;

void llcon_dmp_putc(char c);
int llcon_dmp_init(void);

struct mutex llcon_lock;

static struct llcon_desc llcon;

/*
 * llcondmp=<start_addr>,<size>
 */

static int __init llcondmp_setup(char *str)
{
	int ints[4];
	void * addr;
	size_t size;

	str = get_options(str, 3, ints);
	if (!ints[0])
		return 1;

	switch (ints[0]) {
	case 2:
		size = (size_t)ints[2];
	case 1:
		addr = (void *)ints[1];
	}
	if (size && addr)
		llcon_dmp_reserve_ram(addr, size);
	return 1;
}
__setup("llcondmp=", llcondmp_setup);

void inline __llcon_emit(char c)
{
	if (llcon_dumplog) {
		llcon_dmp_putc(c);
	}
}

void llcon_emit(char c)
{
	__llcon_emit(c);
}

void llcon_emit_line(char * line, int size)
{
	int i;
	if (line && size > 0) {
		for (i = 0; i < size; i++) {
			__llcon_emit(line[i]);
		}
	}
}

struct llcon_desc * llcon_get(void)
{
	return &llcon;
}

void llcon_uninit(void)
{
	if (llcon_enabled) {
		pr_info("LLCON: exit \n");

		llcon_enabled = 0;
	}
}


int llcon_dmp_reserve_ram(void * addr, size_t size)
{
	int rc;

	if (llcon.dmp.phys_addr && llcon.dmp.vmem_size)
		return 0;
	rc = memblock_reserve((phys_addr_t)addr, (phys_addr_t)size);
	if (rc) {
		llcon.dmp.phys_addr = 0;
		llcon.dmp.vmem_size = 0;
		llcon.dmp.virt_addr = 0;
		pr_err("Failed to reserve memory at 0x%lx (size = 0x%lx) \n",
			(long)addr, (long)size);
		return -ENOMEM;
	}
	llcon.dmp.phys_addr = addr;
	llcon.dmp.vmem_size = size;
	llcon.dmp.virt_addr = 0;
	return 0;
}

static void llcon_syslog_dumper(struct kmsg_dumper *dumper,
	    enum kmsg_dump_reason reason,
	    const char *s1, unsigned long l1,
	    const char *s2, unsigned long l2)
{
	int i;
	if (reason == KMSG_DUMP_PANIC) {
		for (i = 0; i < l1; i++)
			llcon_dmp_putc(s1[i]);
		for (i = 0; i < l2; i++)
			llcon_dmp_putc(s2[i]);
	}
}

static struct kmsg_dumper llcon_dumper = {
	.dump = llcon_syslog_dumper,
};

int llcon_dmp_syslog(void)
{
	kmsg_dump_register(&llcon_dumper);
	kmsg_dump(KMSG_DUMP_PANIC);
	kmsg_dump_unregister(&llcon_dumper);
	return 0;
}

int llcon_dmp_init(void)
{
	size_t pcnt;
	size_t x;
	pgprot_t prot;
	struct page **pages;
	phys_addr_t addr;
	void * virt_addr = NULL;
	struct llcon_dmp_header * hdr;
	uint32_t cur_zone = 0;

	if (llcon_dumplog || llcon.dmp.virt_addr)
		return 0;
		
	if (!llcon.dmp.phys_addr || !llcon.dmp.vmem_size)
		return -ENOMEM;

	pcnt = llcon.dmp.vmem_size / PAGE_SIZE;
	prot = pgprot_noncached(PAGE_KERNEL);
	pages = kmalloc(sizeof(struct page *) * pcnt, GFP_KERNEL);
	if (!pages) return -100;
	for (x = 0; x < pcnt; x++) {
		addr = (phys_addr_t)llcon.dmp.phys_addr + x * PAGE_SIZE;
		pages[x] = pfn_to_page(addr >> PAGE_SHIFT);
	} 
	virt_addr = vmap(pages, pcnt, VM_MAP, prot);
	kfree(pages);
	if (!virt_addr) return -200;

	hdr = (struct llcon_dmp_header *)virt_addr;
	if (hdr->magic != LLCON_DMP_MAGIC) {
		hdr->magic = LLCON_DMP_MAGIC;
		hdr->cur_zone = 0;
		hdr->zone_cnt[0] = 1;
		hdr->zone_cnt[1] = 0;
	} else {
		cur_zone = hdr->cur_zone & 1;
		cur_zone = (cur_zone ? 0 : 1);
		hdr->cur_zone = cur_zone;
		hdr->zone_cnt[cur_zone]++;
	}
	llcon.dmp.bufsize = llcon.dmp.vmem_size / 2 - sizeof(struct llcon_dmp_header);
	llcon.dmp.buf = (char *)virt_addr + sizeof(struct llcon_dmp_header);
	if (cur_zone) 
		llcon.dmp.buf += llcon.dmp.bufsize;

	memset(llcon.dmp.buf, 0, llcon.dmp.bufsize);
	llcon.dmp.pos = 0;
	llcon.dmp.virt_addr = virt_addr;
	llcon_dumplog = 1;
	llcon_dmp_syslog();
	return 0;
}

void llcon_dmp_putc(char c)
{
	if (!llcon_dumplog)
		return;
	if (llcon.dmp.pos >= llcon.dmp.bufsize)
		llcon.dmp.pos = 0;
	llcon.dmp.buf[llcon.dmp.pos++] = c;
}
