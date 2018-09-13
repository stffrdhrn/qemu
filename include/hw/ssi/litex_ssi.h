/*
 * LiteX Bit-Banging SSI controller
 *
 * Copyright (c) 2006-2007 CodeSourcery.
 * Copyright (c) 2012 Oskar Andero <oskar.andero@gmail.com>
 *
 * This file is derived from hw/realview.c by Paul Brook
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LITEX_SSI_H
#define LITEX_SSI_H

#include "hw/sysbus.h"
#include "hw/ssi/bitbang_ssi.h"
#include "hw/ssi/ssi.h"

#define TYPE_LITEX_SSI "litex_ssi"
#define LITEX_SSI(obj) \
    OBJECT_CHECK(LiteXSSIState, (obj), TYPE_LITEX_SSI)

#define LITEX_SSI_R_MAX 3

typedef struct LiteXSSIState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    bitbang_ssi_interface *bitbang;
    uint32_t regs[LITEX_SSI_R_MAX];

    qemu_irq cs_n;
    char *spiflash;
} LiteXSSIState;

#endif /* LITEX_SSI_H */
