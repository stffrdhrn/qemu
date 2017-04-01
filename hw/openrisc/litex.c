/*
 *  QEMU model for the Litex board.
 *
 *  Copyright (c) 2010 Michael Walle <michael@walle.cc>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "assert.h"

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "cpu.h"
#include "hw/sysbus.h"
#include "hw/hw.h"
#include "sysemu/sysemu.h"
#include "sysemu/qtest.h"
#include "hw/devices.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "elf.h"
#include "sysemu/block-backend.h"
#include "exec/address-spaces.h"
#include "qemu/cutils.h"
#include "hw/char/serial.h"

#include "hw/litex/hw.h"
#include "generated/csr.h"
#include "generated/mem.h"


#define BIOS_FILENAME    "bios.bin"

typedef struct {
    OpenRISCCPU *cpu;
    hwaddr bootstrap_pc;
    hwaddr flash_base;
} ResetInfo;

/*
static void cpu_irq_handler(void *opaque, int irq, int level)
{
    OpenRISCCPU *cpu = opaque;
    CPUState *cs = CPU(cpu);

    if (level) {
        cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    } else {
        cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
    }
}
*/

static void main_cpu_reset(void *opaque)
{
    OpenRISCCPU *cpu = opaque;

    cpu_reset(CPU(cpu));
    cpu->env.pc = CONFIG_CPU_RESET_ADDR;
}

static void
litex_init(MachineState *machine)
{
    const char *cpu_model = machine->cpu_model;
    const char *kernel_filename = machine->kernel_filename;

    OpenRISCCPU *cpu;
    MemoryRegion *address_space_mem = get_system_memory();

    ResetInfo *reset_info;
    reset_info = g_malloc0(sizeof(ResetInfo));

    if (cpu_model == NULL) {
        cpu_model = "or1200";
    }

    cpu = cpu_openrisc_init(cpu_model);
    if (cpu == NULL) {
        fprintf(stderr, "qemu: unable to find CPU '%s'\n", cpu_model);
        exit(1);
    }
    qemu_register_reset(main_cpu_reset, cpu);

    reset_info->cpu = cpu;

    litex_create_memory(address_space_mem, (qemu_irq*)(cpu->env.irq));

    //cpu_lm32_set_phys_msb_ignore(cpu->env, 1);
    cpu_openrisc_pic_init(cpu);
    cpu_openrisc_clock_init(cpu);

    /* create irq lines 
    cpu->env->pic_state = litex_pic_init(qemu_allocate_irq(cpu_irq_handler, cpu, 0));
    for (i = 0; i < 32; i++) {
        irq[i] = qdev_get_gpio_in(cpu->env->pic_state, i);
    }
    */
    /* make sure juart isn't the first chardev */
    //cpu->env->juart_state = lm32_juart_init(serial_hds[1]);


#ifdef ROM_BASE
#ifndef ROM_DISABLE
    /* load bios rom */
    char *bios_filename;
    if (bios_name == NULL) {
        bios_name = BIOS_FILENAME;
    }
    bios_filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, bios_name);

    if (bios_filename) {
        int bios_size = load_image_targphys(bios_filename, ROM_BASE, ROM_SIZE);
        if (bios_size < 0) {
            fprintf(stderr, "qemu: could not load bios '%s'\n", bios_filename);
            exit(1);
        }
    }
    g_free(bios_filename);
#endif
#endif

//    /* if no kernel is given no valid bios rom is a fatal error */
//    if (!kernel_filename  && !bios_filename && !qtest_enabled()) {
//        fprintf(stderr, "qemu: could not load Milkymist One bios '%s'\n",
//                bios_name);
//        exit(1);
//    }

    if (kernel_filename) {
        int kernel_size = load_image_targphys(kernel_filename, SPIFLASH_BASE, SPIFLASH_SIZE);
        if (kernel_size < 0) {
            fprintf(stderr, "qemu: could not load kernel '%s'\n",    kernel_filename);
            exit(1);
        }
    }

//        uint64_t entry;
//        int kernel_size;
//
//        /* Boots a kernel elf binary.  */
//        kernel_size = load_elf(kernel_filename, NULL, NULL, &entry, NULL, NULL, 1, EM_LATTICEMICO32, 0, 0);
//        reset_info->bootstrap_pc = entry;
//
//        if (kernel_size < 0) {
//            kernel_size = load_image_targphys(kernel_filename, main_ram_base, main_ram_size);
//            reset_info->bootstrap_pc = main_ram_base;
//        }
//
//        if (kernel_size < 0) {
//            fprintf(stderr, "qemu: could not load kernel '%s'\n",    kernel_filename);
//            exit(1);
//        }
//    }

/*
    for (i = 0; i < sc->info->spis_num; i++) {
        object_initialize(&s->spi[i], sizeof(s->spi[i]),
                          sc->info->spi_typename[i]);
        object_property_add_child(obj, "spi", OBJECT(&s->spi[i]), NULL);
        qdev_set_parent_bus(DEVICE(&s->spi[i]), sysbus_get_default());
    }
*/

/*
    object_initialize(&s->i2c, sizeof(s->i2c), TYPE_ASPEED_I2C);
    object_property_add_child(obj, "i2c", OBJECT(&s->i2c), NULL);
    qdev_set_parent_bus(DEVICE(&s->i2c), sysbus_get_default());

    // I2C
    object_property_set_bool(OBJECT(&s->i2c), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c), 0, ASPEED_SOC_I2C_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->i2c), 0,
                       qdev_get_gpio_in(DEVICE(&s->vic), 12));

*/

    reset_info->bootstrap_pc = CONFIG_CPU_RESET_ADDR;
    assert(CONFIG_CPU_RESET_ADDR == 0);
}

static void litex_machine_init(MachineClass *mc)
{
    mc->desc = "Litex One";
    mc->init = litex_init;
    mc->is_default = 0;
}

DEFINE_MACHINE("litex", litex_machine_init)
