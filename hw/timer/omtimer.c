/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Authors: Stafford Horne <shorne@gmail.com>
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "qemu/timer.h"

#define TYPE_OR1K_OMTIMER "or1k-omtimer"
#define OMTIMER_ADDRSPACE_SZ 8
#define OMTIMER_PERIOD 50 /* 50 ns period for 20 MHz timer */

static uint64_t omtimer_read(void *opaque, hwaddr addr, unsigned size)
{
    uint64_t now;
    uint32_t ticks;

    now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    ticks = (uint32_t)(now / OMTIMER_PERIOD);

    return ticks;
}

static void omtimer_write(void *opaque, hwaddr addr, uint64_t data, unsigned size)
{
    /* Read only. */
}

static const MemoryRegionOps omtimer_ops = {
    .read = omtimer_read,
    .write = omtimer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl = {
        .max_access_size = 8,
    },
};

static void or1k_omtimer_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    MemoryRegion *mr = g_new(MemoryRegion, 1);

    memory_region_init_io(mr, obj, &omtimer_ops, NULL,
                          "or1k-omtimer", OMTIMER_ADDRSPACE_SZ);
    sysbus_init_mmio(sbd, mr);
}


static const TypeInfo or1k_omtimer_info = {
    .name          = TYPE_OR1K_OMTIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = 0,
    .instance_init = or1k_omtimer_init,
};

static void or1k_omtimer_register_types(void)
{
    type_register_static(&or1k_omtimer_info);
}

type_init(or1k_omtimer_register_types)
