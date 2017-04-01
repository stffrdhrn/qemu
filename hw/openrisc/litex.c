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
#include "litex-hw.h"
#include "exec/address-spaces.h"
#include "qemu/cutils.h"
#include "hw/char/serial.h"
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
    printf("main_cpu_reset: %p\n", opaque);
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
    main_cpu_reset(cpu);

    reset_info->cpu = cpu;

    /** addresses from 0x80000000 to 0xFFFFFFFF are not shadowed */
    //cpu_lm32_set_phys_msb_ignore(cpu->env, 1);

#ifdef ROM_BASE
    {
	    MemoryRegion *phys_rom = g_new(MemoryRegion, 1);
	    hwaddr rom_base   = ROM_BASE;
	    size_t rom_size   = ROM_SIZE;
	    memory_region_allocate_system_memory(phys_rom, NULL, "litex.rom", rom_size);
	    memory_region_add_subregion(address_space_mem, rom_base, phys_rom);
    }
#endif

#ifdef SRAM_BASE
    {
	    MemoryRegion *phys_sram = g_new(MemoryRegion, 1);
	    hwaddr sram_base   = SRAM_BASE;
	    size_t sram_size   = SRAM_SIZE;
	    memory_region_allocate_system_memory(phys_sram, NULL, "litex.sram",    sram_size);
	    memory_region_add_subregion(address_space_mem, sram_base, phys_sram);
    }
#endif

#ifdef SPIFLASH_BASE
    {
	    MemoryRegion *phys_spiflash = g_new(MemoryRegion, 1);
	    hwaddr spiflash_base = SPIFLASH_BASE;
	    size_t spiflash_size = SPIFLASH_SIZE;
	    memory_region_allocate_system_memory(phys_spiflash, NULL, "litex.spiflash", spiflash_size);
	    memory_region_add_subregion(address_space_mem, spiflash_base, phys_spiflash);
    }
#endif

#ifdef MAIN_RAM_BASE
    {
	    MemoryRegion *phys_main_ram = g_new(MemoryRegion, 1);
	    hwaddr main_ram_base   = MAIN_RAM_BASE;
	    size_t main_ram_size   = MAIN_RAM_SIZE;
	    memory_region_allocate_system_memory(phys_main_ram, NULL, "litex.main_ram", main_ram_size);
	    memory_region_add_subregion(address_space_mem, main_ram_base, phys_main_ram);
    }
#endif

    cpu_openrisc_pic_init(cpu);
    cpu_openrisc_clock_init(cpu);

    /* create irq lines 
    cpu->env->pic_state = litex_pic_init(qemu_allocate_irq(cpu_irq_handler, cpu, 0));
    for (i = 0; i < 32; i++) {
        irq[i] = qdev_get_gpio_in(cpu->env->pic_state, i);
    }
    */
#define MASK 0x7FFFFFFF
/*
    memory_region_add_subregion(address_space_mem, tcm_base, phys_tcm);
    memory_region_add_subregion(address_space_mem, 0xc0000000 + tcm_base,
			                                phys_tcm_alias);

    memory_region_init_alias(

1023 void memory_region_add_eventfd(MemoryRegion *mr,                                                                                                                                                               
1024                                hwaddr addr,                                                                                                                                                                    
1025                                unsigned size,                                                                                                                                                                  
1026                                bool match_data,                                                                                                                                                                
1027                                uint64_t data,                                                                                                                                                                  
1028                                EventNotifier *e);  
*/
    /* litex uart */
#ifdef CSR_UART_BASE
    litex_uart_create(CSR_UART_BASE & MASK, cpu->env.irq[UART_INTERRUPT], serial_hds[0]);
#endif

    /* litex timer*/
#ifdef CSR_TIMER0_BASE
    litex_timer_create(CSR_TIMER0_BASE & MASK, cpu->env.irq[TIMER0_INTERRUPT], SYSTEM_CLOCK_FREQUENCY);
#endif

/* litex ethernet*/
#ifdef CSR_ETHMAC_BASE
    litex_liteeth_create(CSR_ETHMAC_BASE & MASK, CSR_ETHPHY_BASE & MASK, ETHMAC_BASE & MASK, cpu->env.irq[ETHMAC_INTERRUPT]);
#endif

#ifdef CSR_OPSIS_I2C_BASE
    litex_i2c_create(CSR_OPSIS_I2C_BASE & MASK);
#endif

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
