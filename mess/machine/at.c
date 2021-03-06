/***************************************************************************
    
	IBM AT Compatibles

***************************************************************************/

#include "driver.h"

#include "cpu/i386/i386.h"

#include "machine/pic8259.h"
#include "machine/8237dma.h"
#include "machine/mc146818.h"

#include "vidhrdw/pc_vga.h"
#include "vidhrdw/pc_cga.h"

#include "machine/pit8253.h"
#include "machine/pcshare.h"
#include "machine/8042kbdc.h"
#include "includes/at.h"
#include "machine/pckeybrd.h"
#include "includes/sblaster.h"

static const SOUNDBLASTER_CONFIG soundblaster = { 1,5, {1,0} };



static void at286_set_gate_a20(int a20)
{
	cpunum_set_input_line(0, INPUT_LINE_A20, a20);
}



static void at386_set_gate_a20(int a20)
{
	offs_t mirror = a20 ? 0xff000000 : 0xfff00000;
	cpunum_set_input_line(0, INPUT_LINE_A20, a20);
	memory_install_read32_handler(0,  ADDRESS_SPACE_PROGRAM, 0x000000, 0x09ffff, 0, mirror, MRA32_BANK10);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x09ffff, 0, mirror, MWA32_BANK10);
	memory_set_bankptr(10, mess_ram);

	if (a20)
	{
		offs_t ram_limit = 0x100000 + mess_ram_size - 0x0a0000;
		memory_install_read32_handler(0,  ADDRESS_SPACE_PROGRAM, 0x100000,  ram_limit - 1,	0, 0, MRA32_BANK1);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x100000,  ram_limit - 1,	0, 0, MWA32_BANK1);
		memory_install_read32_handler(0,  ADDRESS_SPACE_PROGRAM, ram_limit, 0xffffff,		0, 0, MRA32_NOP);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, ram_limit, 0xffffff,		0, 0, MWA32_NOP);
		memory_install_read32_handler(0,  ADDRESS_SPACE_PROGRAM, 0xff0000,  0xffffff,		0, 0, MRA32_BANK2);
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xff0000,  0xffffff,		0, 0, MWA32_ROM);
		memory_set_bankptr(1, mess_ram + 0xa0000);
		memory_set_bankptr(2, memory_region(REGION_CPU1) + 0x0f0000);
	}
}



static void init_at_common(const struct kbdc8042_interface *at8042)
{
	init_pc_common(PCCOMMON_KEYBOARD_AT | PCCOMMON_DMA8237_AT | PCCOMMON_TIMER_8254);
	mc146818_init(MC146818_STANDARD);
	soundblaster_config(&soundblaster);
	kbdc8042_init(at8042);
}



static void at_keyboard_interrupt(int state)
{
	pic8259_set_irq_line(0, 1, state);
}



DRIVER_INIT( atcga )
{
	struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at286_set_gate_a20, at_keyboard_interrupt
	};
	init_at_common(&at8042);
}



DRIVER_INIT( at386 )
{
	struct kbdc8042_interface at8042 =
	{
		KBDC8042_AT386, at386_set_gate_a20, at_keyboard_interrupt
	};
	init_at_common(&at8042);
}



static void at_map_vga_memory(offs_t begin, offs_t end, read8_handler rh, write8_handler wh)
{
	int buswidth;
	buswidth = cputype_databus_width(Machine->drv->cpu[0].cpu_type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
		case 8:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xA0000, 0xBFFFF, 0, 0, MRA8_NOP);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xA0000, 0xBFFFF, 0, 0, MWA8_NOP);

			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, rh);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, wh);
			break;
	}
}



static const struct pc_vga_interface vga_interface =
{
	1,
	at_map_vga_memory,

	input_port_0_r,

	ADDRESS_SPACE_IO,
	0x0000
};



DRIVER_INIT( at_vga )
{
	struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at286_set_gate_a20, at_keyboard_interrupt
	};

	init_at_common(&at8042);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
	pc_vga_init(&vga_interface, NULL);
}



DRIVER_INIT( ps2m30286 )
{
	struct kbdc8042_interface at8042 =
	{
		KBDC8042_PS2, at386_set_gate_a20, at_keyboard_interrupt
	};
	init_at_common(&at8042);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
	pc_vga_init(&vga_interface, NULL);
}



static int at_irq_callback(int irqline)
{
	return pic8259_acknowledge(0);
}



MACHINE_INIT( at )
{
	dma8237_reset();
	cpu_set_irq_callback(0, at_irq_callback);
}



MACHINE_INIT( at_vga )
{
	pc_vga_reset();
	dma8237_reset();
	cpu_set_irq_callback(0, at_irq_callback);
}
