
#include "qemu/osdep.h"
#include "qemu-common.h"

#include "exec/address-spaces.h"
#include "hw/hw.h"
#include "hw/i2c/smbus.h"
#include "hw/loader.h"
#include "hw/ssi/ssi.h"
#include "hw/sysbus.h"
#include "net/net.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "sysemu/block-backend.h"
#include "sysemu/blockdev.h"
#include "sysemu/sysemu.h"

#include "hw/litex/hw.h"
#include "generated/csr.h"
#include "generated/mem.h"

DeviceState *litex_uart_create(hwaddr base, qemu_irq irq, CharDriverState *chr)
{
    DeviceState *dev;

    dev = qdev_create(NULL, "litex-uart");
    qdev_prop_set_chr(dev, "chardev", chr);
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, irq);

    return dev;
}



DeviceState *litex_timer_create(hwaddr base, qemu_irq timer0_irq, uint32_t freq_hz)
{
    DeviceState *dev;

    dev = qdev_create(NULL, "litex-timer");
    qdev_prop_set_uint32(dev, "frequency", freq_hz);
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, timer0_irq);

    return dev;
}


DeviceState *litex_liteeth_create(hwaddr reg_base, hwaddr phy_base, hwaddr ethmac_sram_base, qemu_irq ethmac_irq)
{
    DeviceState *dev;

    qemu_check_nic_model(&nd_table[0], "liteeth");
    dev = qdev_create(NULL, "litex-liteeth");
    qdev_set_nic_properties(dev, &nd_table[0]);
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, phy_base);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 1, reg_base);

    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 2, ethmac_sram_base);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, ethmac_irq);
    return dev;
}

uint8_t eeprom_contents[256] = {
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
 190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
 220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
 240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
 250, 251, 252, 253, 254, 255};

DeviceState *litex_i2c_create(hwaddr reg_base)
{
    DeviceState *dev;
    I2CBus *i2c;

    dev = sysbus_create_simple("litex_i2c", reg_base, NULL);
    i2c = (I2CBus *)qdev_get_child_bus(dev, "i2c");

    smbus_eeprom_init(i2c, 1, eeprom_contents, 256);
    return dev;
}

#define MEM_SIZE 0x80000000
#define MEM_MASK 0x7FFFFFFF

void* m25p80_get_storage(void *opaque);

void litex_create_memory(MemoryRegion *address_space_mem, qemu_irq irqs[])
{
    /*
      The following two memory regions equivalent to each other;
       (a) 0x00000000 to 0x7FFFFFFF
       (b) 0x80000000 to 0xFFFFFFFF

      IE Memory found at 0x00000100 will also be found at 0x80000100.

      On a real system accessing the memory via (a) goes through the CPU cache,
      while accessing it via (b) bypasses the cache.

      However, as QEmu doesn't emulate the CPU cache, we can just alias them
      together.
    */
    MemoryRegion *shadow_mem = g_new(MemoryRegion, 1);
    memory_region_init_alias(shadow_mem, NULL, "litex.shadow", address_space_mem, 0, MEM_SIZE);
    memory_region_add_subregion(address_space_mem, MEM_SIZE, shadow_mem);

    {
        char *bios_filename = NULL;
        bios_filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, bios_name);

#ifdef ROM_BASE
#ifndef ROM_DISABLE
        {
            MemoryRegion *phys_rom = g_new(MemoryRegion, 1);
            hwaddr rom_base   = ROM_BASE;
            size_t rom_size   = ROM_SIZE;
            memory_region_allocate_system_memory(phys_rom, NULL, "litex.rom", rom_size);
            memory_region_add_subregion(address_space_mem, rom_base, phys_rom);
        }

        /* Load bios rom. */
        if (bios_filename) {
            int bios_size = load_image_targphys(bios_filename, ROM_BASE, ROM_SIZE);
            if (bios_size < 0) {
                fprintf(stderr, "qemu: could not load bios '%s'\n", bios_filename);
                exit(1);
            }
        }
#else
        /* Complain if bios provided. */
        if (bios_filename) {
            fprintf(stderr, "qemu: bios '%s' provided but device has no bios rom!\n", bios_filename);
            exit(1);
        }
#endif
#endif
        g_free(bios_filename);
    }

#ifdef SRAM_BASE
    {
        MemoryRegion *phys_sram = g_new(MemoryRegion, 1);
        hwaddr sram_base   = SRAM_BASE;
        size_t sram_size   = SRAM_SIZE;
        memory_region_allocate_system_memory(phys_sram, NULL, "litex.sram", sram_size);
        memory_region_add_subregion(address_space_mem, sram_base, phys_sram);
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

    /* litex uart */
#ifdef CSR_UART_BASE
    litex_uart_create(CSR_UART_BASE & MEM_MASK, irqs[UART_INTERRUPT], serial_hds[0]);
#endif

    /* litex timer*/
#ifdef CSR_TIMER0_BASE
    litex_timer_create(CSR_TIMER0_BASE & MEM_MASK, irqs[TIMER0_INTERRUPT], SYSTEM_CLOCK_FREQUENCY);
#endif

/* litex ethernet*/
#ifdef CSR_ETHMAC_BASE
    litex_liteeth_create(CSR_ETHMAC_BASE & MEM_MASK, CSR_ETHPHY_BASE & MEM_MASK, ETHMAC_BASE & MEM_MASK, irqs[ETHMAC_INTERRUPT]);
#endif

#ifdef CSR_OPSIS_I2C_BASE
    litex_i2c_create(CSR_OPSIS_I2C_BASE & MEM_MASK);
#endif

#ifdef SPIFLASH_BASE
    {
        MemoryRegion *phys_spiflash = g_new(MemoryRegion, 1);
        hwaddr spiflash_base = SPIFLASH_BASE;
        size_t spiflash_size = SPIFLASH_SIZE;
        void* spiflash_data;

        DriveInfo *dinfo = drive_get_next(IF_MTD);

#ifdef CSR_SPIFLASH_BASE
        DeviceState *spi_master;
        DeviceState *spi_flash;
        SSIBus *spi_bus;
        qemu_irq cs_line;

        spi_master = qdev_create(NULL, "litex_ssi");
        qdev_init_nofail(spi_master);
        sysbus_mmio_map(SYS_BUS_DEVICE(spi_master), 0, CSR_SPIFLASH_BASE & MEM_MASK);

        spi_bus = (SSIBus *)qdev_get_child_bus(spi_master, "ssi");

        if (dinfo) {
            if (!dinfo->serial) {
                printf("Set serial value to flash type (m25p16, etc)\n");
                abort();
            } else {
                printf("Using spiflash type %s\n", dinfo->serial);
            }
            spi_flash = ssi_create_slave_no_init(spi_bus, dinfo->serial);
            qdev_prop_set_drive(spi_flash, "drive", blk_by_legacy_dinfo(dinfo), &error_abort);
        } else {
            spi_flash = ssi_create_slave_no_init(spi_bus, "m25p80");
        }
        qdev_init_nofail(spi_flash);

        cs_line = qdev_get_gpio_in_named(spi_flash, SSI_GPIO_CS, 0);
        qdev_connect_gpio_out_named(spi_master, SSI_GPIO_CS, 0, cs_line);

        spiflash_data = m25p80_get_storage(spi_flash);
#else
        if (dinfo) {
            int rsize = 0;
            BlockBackend *blk = blk_by_legacy_dinfo(dinfo);
            assert(blk);

            spiflash_data = blk_blockalign(blk, spiflash_size);

            rsize = blk_pread(blk, 0, spiflash_data, spiflash_size);
            if (rsize != spiflash_size) {
                printf("litex.spiflash: Failed to read flash contents, wanted s->size %d, got %d\n", (int)spiflash_size, rsize);
                abort();
            }
        }
#endif

        if (spiflash_data) {
            memory_region_init_ram_device_ptr(phys_spiflash, NULL, "litex.spiflash", spiflash_size, spiflash_data);
        } else {
            memory_region_allocate_system_memory(phys_spiflash, NULL, "litex.spiflash", spiflash_size);
        }
        memory_region_set_readonly(phys_spiflash, true);
        memory_region_add_subregion(address_space_mem, spiflash_base, phys_spiflash);
    }
#endif

}
