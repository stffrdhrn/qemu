
#include "qemu/osdep.h"
#include "qemu-common.h"

#include "hw/hw.h"
#include "hw/i2c/smbus.h"
#include "hw/sysbus.h"
#include "net/net.h"
#include "exec/address-spaces.h"
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


DeviceState *litex_liteeth_create(hwaddr reg_base,  hwaddr phy_base, hwaddr ethmac_sram_base, qemu_irq ethmac_irq)
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

/*
DeviceState *litex_qspi_create(hwaddr base, qemu_irq qspi_irq)
{
  DeviceState *dev;
  SSIBus *spi;
  
  dev = qdev_create(NULL, "xlnx.xps-spi");
  qdev_prop_set_uint8(dev, "num-ss-bits", 1);
  qdev_init_nofail(dev);
  sysbus_mmio_map( SYS_BUS_DEVICE(dev), 0, base);
  sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, qspi_irq);

  spi = (SSIBus *)qdev_get_child_bus(dev, "spi");
  
  DriveInfo *dinfo = drive_get_next(IF_MTD);
  qemu_irq cs_line;
  
  dev = ssi_create_slave_no_init(spi, "n25q128");
  if (dinfo) {
    qdev_prop_set_drive(dev, "drive", blk_by_legacy_dinfo(dinfo), &error_fatal);
  }
  qdev_init_nofail(dev);
  cs_line = qdev_get_gpio_in_named(dev, SSI_GPIO_CS, 0);
  sysbus_connect_irq(SYS_BUS_DEVICE(dev), 1, cs_line);
}
*/

void litex_create_memory(MemoryRegion *address_space_mem, qemu_irq irqs[])
{
#define MASK 0x7FFFFFFF
/*
    addresses from 0x80000000 to 0xFFFFFFFF are not shadowed

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
	    memory_region_allocate_system_memory(phys_sram, NULL, "litex.sram", sram_size);
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

    /* litex uart */
#ifdef CSR_UART_BASE
    litex_uart_create(CSR_UART_BASE & MASK, irqs[UART_INTERRUPT], serial_hds[0]);
#endif

    /* litex timer*/
#ifdef CSR_TIMER0_BASE
    litex_timer_create(CSR_TIMER0_BASE & MASK, irqs[TIMER0_INTERRUPT], SYSTEM_CLOCK_FREQUENCY);
#endif

/* litex ethernet*/
#ifdef CSR_ETHMAC_BASE
    litex_liteeth_create(CSR_ETHMAC_BASE & MASK, CSR_ETHPHY_BASE & MASK, ETHMAC_BASE & MASK, irqs[ETHMAC_INTERRUPT]);
#endif

#ifdef CSR_OPSIS_I2C_BASE
    litex_i2c_create(CSR_OPSIS_I2C_BASE & MASK);
#endif

}
