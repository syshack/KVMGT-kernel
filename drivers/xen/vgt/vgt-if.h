/*
 * Interface between Gfx dricer and vGT enabled hypervisor
 *
 * Copyright(c) 2011-2013 Intel Corporation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _VGT_IF_H
#define _VGT_IF_H


/* Reserve 32KB for vGT shared infor: 0x78000-0x7FFFF */
#define VGT_PVINFO_PAGE	0x78000

/* XXX: the 32KB range conflicts with PIPE_EDP_CONF: 0x7f008, so let's change
 * to reserve only 4KB for now.
 */
//#define VGT_PVINFO_SIZE	0x8000
#define VGT_PVINFO_SIZE	0x1000

/*
 * The following structure pages are defined in GEN MMIO space for virtualization.
 * (One page for now)
 */
#define    VGT_MAGIC         0x4776544776544776    /* 'vGTvGTvG' */
#define    VGT_VERSION_MAJOR 1
#define    VGT_VERSION_MINOR 0

/*
 * The information set by the guest gfx driver, through the display_ready field
 */
#define    VGT_DRV_DISPLAY_NOT_READY	(0 << 0)
#define    VGT_DRV_DISPLAY_READY	(1 << 0)	/* ready for display switch */
#define    VGT_DRV_LEGACY_VGA_MODE	(1 << 1)	/* in the legacy VGA mode */

/*
 * guest-to-vgt notifications
 */
enum vgt_g2v_type {
	VGT_G2V_DISPLAY_REFRESH,
	VGT_G2V_SET_POINTER_SHAPE,
	VGT_G2V_MAX,
};

/*
 * vgt-to-guest notifications
 */
enum vgt_v2g_type {
	VGT_V2G_SET_HW_CURSOR,
	VGT_V2G_SET_SW_CURSOR,
	VGT_V2G_MAX,
};

struct vgt_if {
    uint64_t  magic;      /* VGT_MAGIC */
    uint16_t  version_major;
    uint16_t  version_minor;
    uint32_t  vgt_id;       /* ID of vGT instance */
    uint32_t  rsv2[12];	    /* pad to offset 0x40 */
    /*
     *  Data structure to describe the balooning info of resources.
     *  Each VM can only have one portion of continuous area for now.
     *  (May support scattered resource in future)
     *  (next starting from offset 0x40)
     */
    struct {
        /* Aperture register balooning */
        struct    {
           uint32_t  my_base;
           uint32_t  my_size;
        } low_gmadr;		/* aperture */
        /* GMADR register balooning */
        struct    {
           uint32_t  my_base;
           uint32_t  my_size;
        } high_gmadr;		/* non aperture */
        /* allowed fence registers */
        uint32_t fence_num;
        uint32_t  rsv2[3];
    } avail_rs;			/* available/assigned resource */
    uint32_t  rsv3[0x200-24];   /* pad to half page */
    /*
     * The bottom half page is for the response from Gfx driver to hypervisor.
     */
    uint16_t  drv_version_major;
    uint16_t  drv_version_minor;
    uint32_t  display_ready;/* ready for display owner switch */
    /*
     * driver reported status/error code
     *     0: if the avail_rs is sufficient to driver
     *  Bit 2,1,0 set indicating
     *       Insufficient low_gmadr, high_gmadr, fence resources.
     *  Other bits are reserved.
     */
    uint32_t  rs_insufficient;
    /*
     * The driver is required to update the following field with minimal
     * required resource size.
     */
    uint32_t  min_low_gmadr;
    uint32_t  min_high_gmadr;
    uint32_t  min_fence_num;

    /*
     * notifications between guest and vgt
     */
    uint32_t  g2v_notify;
    uint32_t  v2g_notify;

    /*
     * PPGTT PTE table info
     */
    uint32_t  gmm_gtt_seg_base;
    uint32_t  rsv4;
    uint32_t  gmm_gtt_seg_size;
    uint32_t  rsv5;

    /*
     * Cursor hotspot info
     */
    uint32_t  xhot;
    uint32_t  yhot;;

    /*
     * scratch space for debugging
     */
    uint32_t  scratch;;

    uint32_t  rsv6[0x200-15];    /* pad to one page */
};

#define vgt_info_off(x)        (VGT_PVINFO_PAGE + (long)&((struct vgt_if*) NULL)->x)


struct vgt_device;
typedef struct {
    bool (*mem_read)(struct vgt_device *vgt, uint64_t pa, void *p_data, int bytes);
    bool (*mem_write)(struct vgt_device *vgt, uint64_t pa, void *p_data, int bytes);
    bool (*cfg_read)(struct vgt_device *vgt, unsigned int off, void *p_data, int bytes);
    bool (*cfg_write)(struct vgt_device *vgt, unsigned int off, void *p_data, int bytes);
    bool boot_time;	/* in boot time dom0 access is always passed through */
    bool initialized;	/* whether vgt_ops can be referenced */
} vgt_ops_t;
extern vgt_ops_t *vgt_ops;

/* get the bits high:low of the data, high and low is starting from zero*/
#define VGT_GET_BITS(data, high, low)	(((data) & ((1 << ((high) + 1)) - 1)) >> (low))
/* get one bit of the data, bit is starting from zeor */
#define VGT_GET_BIT(data, bit)		VGT_GET_BITS(data, bit, bit)


bool vgt_emulate_write(struct vgt_device *vgt, uint64_t pa, void *p_data, int bytes);
bool vgt_emulate_read(struct vgt_device *vgt, uint64_t pa, void *p_data, int bytes);
bool vgt_emulate_cfg_write(struct vgt_device *vgt, unsigned int off, void *p_data, int bytes);
bool vgt_emulate_cfg_read(struct vgt_device *vgt, unsigned int off, void *p_data, int bytes);

extern int i915_start_vgt(struct pci_dev *pdev);
extern void xen_deregister_vgt_device(struct vgt_device *vgt);
extern int vgt_suspend(struct pci_dev *pdev);
extern int vgt_resume(struct pci_dev *pdev);

bool native_mmio_read(uint64_t va, int bytes, void *val);
bool native_mmio_write(uint64_t va, int bytes, unsigned long val);

/* save the fixed/translated guest address
 * restore the address after the command is executed
*/
#define VGT_ENABLE_ADDRESS_FIX_SAVE_RESTORE

enum {
	VGT_DELAY_IRQ = 0,
	VGT_DELAY_VBLANK_DISABLE_TIMER,
	VGT_DELAY_HANGCHECK_TIMER,
	VGT_DELAY_HOTPLUG_REENABLE_TIMER,
	VGT_DELAY_EVENT_MAX,
};

extern bool vgt_check_busy(int event);
extern void vgt_set_delayed_event_data(int event, void *data);

DECLARE_PER_CPU(u8, in_vgt);

extern int vgt_handle_dom0_device_reset(void);

/*
 * in_vgt flag is used to indicate whether current code
 * path is in vgt core module, which is key for virtual
 * irq delivery in de-privileged dom0 framework. So use
 * get_cpu/put_cpu here to avoid preemption, otherwise
 * this flag loses its intention.
 */
static inline int vgt_enter(void)
{
	int cpu = get_cpu();

	per_cpu(in_vgt, cpu)++;
	return cpu;
}

extern void inject_dom0_virtual_interrupt(void *info);
static inline void vgt_exit(int cpu)
{
	per_cpu(in_vgt, cpu)--;

	/* check for delayed virq injection */
	inject_dom0_virtual_interrupt(NULL);

	put_cpu();
}

#define XEN_IGD_INVALID 0
#define XEN_IGD_SNB     1
#define XEN_IGD_IVB     2
#define XEN_IGD_HSW     3
#define XEN_IGD_MAX     3   /* the max GEN dev type supported */

/* from "include/xen/interface/xen.h" */
/*typedef uint16_t domid_t;*/

static inline bool kvm_guest_domain(void)
{
	unsigned int eax, ebx, ecx, edx;
	char s[12];
	unsigned int *i;

	/* KVM_CPUID_SIGNATURE */
	eax = 0x40000000;
	ebx = ecx = edx = 0;

	asm volatile ("cpuid"
			: "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
			:
			: "cc", "memory"
		     );
	i = (unsigned int *)s;
	i[0] = ebx;
	i[1] = ecx;
	i[2] = edx;

	return !strncmp(s, "KVMKVMKVM", strlen("KVMKVMKVM"));
}

#endif /* _VGT_IF_H */
