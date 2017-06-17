/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Authors: Stafford Horne <shorne@gmail.com>
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "exec/memory.h"

#define TYPE_OR1K_OMPIC "or1k-ompic"
#define OR1K_OMPIC(obj) OBJECT_CHECK(OR1KOMPICState, (obj), TYPE_OR1K_OMPIC)

#define OMPIC_CTRL_IRQ_ACK  (1 << 31)
#define OMPIC_CTRL_IRQ_GEN  (1 << 30)
#define OMPIC_CTRL_DST(cpu) (((cpu) >> 16) & 0x3fff)

#define OMPIC_REG(addr)     (((addr) >> 2) & 0x1)
#define OMPIC_SRC_CPU(addr) (((addr) >> 3) & 0x4f)
#define OMPIC_DST_CPU(addr) (((addr) >> 3) & 0x4f)

#define OMPIC_STATUS_IRQ_PENDING (1 << 30)
#define OMPIC_STATUS_SRC(cpu)    (((cpu) & 0x3fff) << 16)
#define OMPIC_STATUS_DATA(data)  ((data) & 0xffff)

#define OMPIC_CONTROL 0
#define OMPIC_STATUS  1

#define OMPIC_MAX_CPUS 16384 /* 2^14 - based on reg bits */
#define OMPIC_ADDRSPACE_SZ 1024 /* This is same as dts in linux, dont match */

typedef struct OR1KOMPICState OR1KOMPICState;
typedef struct OR1KOMPICCPUState OR1KOMPICCPUState;

struct OR1KOMPICState {
    SysBusDevice parent_obj;
    MemoryRegion mr;

    OR1KOMPICCPUState *cpus;

    int32_t num_cpus;
};

struct OR1KOMPICCPUState {
    qemu_irq irq;
    int32_t status;
    int32_t control;
};

static uint64_t ompic_read(void *opaque, hwaddr addr, unsigned size)
{
    OR1KOMPICState *s = opaque;
    int src_cpu = OMPIC_SRC_CPU(addr);

    /* We can only write to control control, write control + update status */
    if (OMPIC_REG(addr) == OMPIC_CONTROL) {
	return s->cpus[src_cpu].control;
    } else {
        return s->cpus[src_cpu].status;
   }

}

static void ompic_write(void *opaque, hwaddr addr, uint64_t data, unsigned size)
{
    OR1KOMPICState *s = opaque;
    /* We can only write to control control, write control + update status */
    if (OMPIC_REG(addr) == OMPIC_CONTROL) {
        int src_cpu = OMPIC_SRC_CPU(addr);

        s->cpus[src_cpu].control = data;

        if (data & OMPIC_CTRL_IRQ_GEN) {
            int dst_cpu = OMPIC_CTRL_DST(data);

            s->cpus[dst_cpu].status = OMPIC_STATUS_IRQ_PENDING |
                OMPIC_STATUS_SRC(src_cpu) |
                OMPIC_STATUS_DATA(data);

            qemu_irq_raise(s->cpus[dst_cpu].irq);
        }
        if (data & OMPIC_CTRL_IRQ_ACK) {
            s->cpus[src_cpu].status &= ~OMPIC_STATUS_IRQ_PENDING;
            qemu_irq_lower(s->cpus[src_cpu].irq);
        }
    }
}

static const MemoryRegionOps ompic_ops = {
    .read = ompic_read,
    .write = ompic_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl = {
        .max_access_size = 8,
    },
};

static void or1k_ompic_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    OR1KOMPICState *s = OR1K_OMPIC(obj);

    memory_region_init_io(&s->mr, OBJECT(s), &ompic_ops, s,
                          "or1k-ompic", OMPIC_ADDRSPACE_SZ);
    sysbus_init_mmio(sbd, &s->mr);
}

static void or1k_ompic_realize(DeviceState *dev, Error **errp)
{
    OR1KOMPICState *s = OR1K_OMPIC(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    int i;

    if (s->num_cpus > OMPIC_MAX_CPUS) {
        error_setg(errp, "Exceeded maximum CPUs %d", s->num_cpus);
        return;
    }
    s->cpus = g_new0(OR1KOMPICCPUState, s->num_cpus);
    /* Init IRQ sources for all CPUs */
    for (i = 0; i < s->num_cpus; i++) {
        sysbus_init_irq(sbd, &s->cpus[i].irq);
    }
}

static Property or1k_ompic_properties[] = {
    DEFINE_PROP_INT32("num-cpus", OR1KOMPICState, num_cpus, 1),
    DEFINE_PROP_END_OF_LIST(),
};

static void or1k_ompic_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->props = or1k_ompic_properties;
    dc->realize = or1k_ompic_realize;
}

static const TypeInfo or1k_ompic_info = {
    .name          = TYPE_OR1K_OMPIC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(OR1KOMPICState),
    .instance_init = or1k_ompic_init,
    .class_init    = or1k_ompic_class_init,
};

static void or1k_ompic_register_types(void)
{
    type_register_static(&or1k_ompic_info);
}

type_init(or1k_ompic_register_types)
