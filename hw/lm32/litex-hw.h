#ifndef QEMU_HW_LITEX_HW_H
#define QEMU_HW_LITEX_HW_H

#include "hw/qdev.h"
#include "net/net.h"

static inline DeviceState *litex_uart_create(hwaddr base,
                                             qemu_irq irq,
                                             CharDriverState *chr)
{
    DeviceState *dev;

    dev = qdev_create(NULL, "litex-uart");
    qdev_prop_set_chr(dev, "chardev", chr);
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, irq);

    return dev;
}



static inline DeviceState *litex_timer_create(hwaddr base, qemu_irq timer0_irq, uint32_t freq_hz)
{
    DeviceState *dev;

    dev = qdev_create(NULL, "litex-timer");
    qdev_prop_set_uint32(dev, "frequency", freq_hz);
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, timer0_irq);

    return dev;
}


static inline DeviceState *litex_liteeth_create(hwaddr reg_base,  hwaddr phy_base, hwaddr ethmac_sram_base, qemu_irq ethmac_irq)
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


/*
static inline DeviceState *litex_qspi_create(hwaddr base, qemu_irq qspi_irq)
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
#endif /* QEMU_HW_LITEX_HW_H */
