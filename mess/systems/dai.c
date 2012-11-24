/*******************************************************************************

DAI driver by Krzysztof Strzecha and Nathan Woods

What's new:
-----------
21.05.2004	TMS5501 fixes. Debug code cleanups.
06.03.2004	Stack overflow interrupt added.
05.09.2003      Rundom number generator added. Few video hardware bugs fixed.
		Fixed few i8080 instructions, making much more BASIC games playable.

Notes on emulation status and to do list:
-----------------------------------------
1. A lot to do. Too much to list.

DAI technical information
==========================

CPU:
----
	8080 2MHz


Memory map:
-----------
	0000-bfff RAM
	c000-dfff ROM (non-switchable)
	e000-efff ROM (4 switchable banks)
	f000-f7ff ROM extension (optional)
	f800-f8ff SRAM (stack)
	f900-ffff I/O
		f900-faff spare
		fb00-fbff AMD9511 math chip (optional)
		fc00-fcff 8253 programmable interval timer
		fd00-fdff discrete devices
		fe00-feff 8255 PIO (DCE bus)
		ff00-ffff timer + 5501 interrupt controller 

Interrupts:
-----------


Keyboard:
---------


Video:
-----


Sound:
------


Timings:
--------


*******************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "cpu/i8085/i8085.h"
#include "vidhrdw/generic.h"
#include "includes/dai.h"
#include "machine/pit8253.h"
#include "machine/8255ppi.h"
#include "machine/tms5501.h"
#include "devices/cassette.h"

/* I/O ports */
ADDRESS_MAP_START( dai_readport , ADDRESS_SPACE_IO, 8)
ADDRESS_MAP_END

ADDRESS_MAP_START( dai_writeport , ADDRESS_SPACE_IO, 8)
ADDRESS_MAP_END

/* memory w/r functions */
ADDRESS_MAP_START( dai_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0xbfff) AM_READ( MRA8_BANK1 )
	AM_RANGE( 0xc000, 0xdfff) AM_READ( MRA8_ROM )
	AM_RANGE( 0xe000, 0xefff) AM_READ( MRA8_BANK2 )
	AM_RANGE( 0xf000, 0xf7ff) AM_READ( MRA8_NOP )
	AM_RANGE( 0xf800, 0xf8ff) AM_READ( MRA8_RAM )
	AM_RANGE( 0xfb00, 0xfbff) AM_READ( amd9511_r )
	AM_RANGE( 0xfc00, 0xfcff) AM_READ( pit8253_0_r )
	AM_RANGE( 0xfd00, 0xfdff) AM_READ( dai_io_discrete_devices_r )
	AM_RANGE( 0xfe00, 0xfeff) AM_READ( ppi8255_0_r )
	AM_RANGE( 0xff00, 0xffff) AM_READ( tms5501_0_r )
ADDRESS_MAP_END

ADDRESS_MAP_START( dai_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0xbfff) AM_WRITE( MWA8_BANK1 )
	AM_RANGE( 0xc000, 0xdfff) AM_WRITE( MWA8_ROM )
	AM_RANGE( 0xe000, 0xefff) AM_WRITE( MWA8_ROM )
	AM_RANGE( 0xf000, 0xf7ff) AM_WRITE( dai_stack_interrupt_circuit_w )
	AM_RANGE( 0xf800, 0xf8ff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0xfb00, 0xfbff) AM_WRITE( amd9511_w )
	AM_RANGE( 0xfc00, 0xfcff) AM_WRITE( pit8253_0_w )
	AM_RANGE( 0xfd00, 0xfdff) AM_WRITE( dai_io_discrete_devices_w )
	AM_RANGE( 0xfe00, 0xfeff) AM_WRITE( ppi8255_0_w )
	AM_RANGE( 0xff00, 0xffff) AM_WRITE( tms5501_0_w )
ADDRESS_MAP_END


/* keyboard input */
INPUT_PORTS_START (dai)
	PORT_START /* [0] - port ff07 bit 0 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8  (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H  h") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P  p") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X  x") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [1] - port ff07 bit 1 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1  !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9  )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A  a") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I  i") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q  q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y  y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [2] - port ff07 bit 2 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2  \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(":  *") PORT_CODE(KEYCODE_MINUS) PORT_CHAR(':') PORT_CHAR('*')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B  b") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J  j") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R  r") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z  z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [3] - port ff07 bit 3 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3  #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(";  +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C  c") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K  k") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S  s") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[  ]") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('[') PORT_CHAR(']')
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [4] - port ff07 bit 4 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4  $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",  <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D  d") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L  l") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T  t") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^  ~") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('^') PORT_CHAR('~')
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [5] - port ff07 bit 5 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5  %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-  =") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('-') PORT_CHAR('=')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E  e") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M  m") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U  u") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [6] - port ff07 bit 6 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6  &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".  >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F  f") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N  n") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V  v") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Rept") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [7] - port ff07 bit 7 */
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7  '") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/  ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G  g") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O  o") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W  w") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Char del") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_2)
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [8] */
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_VBLANK)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)
		PORT_BIT(0xcb, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

static struct CassetteOptions dai_cassette_options = {
	1,		/* channels */
	16,		/* bits per sample */
	44100		/* sample frequency */
};

/* machine definition */
static MACHINE_DRIVER_START( dai )
	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 2000000)
	MDRV_CPU_PROGRAM_MAP(dai_readmem, dai_writemem)
	MDRV_CPU_IO_MAP(dai_readport, dai_writeport)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( dai )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1056, 542)
	MDRV_VISIBLE_AREA(0, 1056-1, 0, 302-1)
	MDRV_PALETTE_LENGTH(sizeof (dai_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (dai_colortable))
	MDRV_PALETTE_INIT( dai )

	MDRV_VIDEO_START( dai )
	MDRV_VIDEO_UPDATE( dai )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(dai_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

#define io_dai		io_NULL

ROM_START(dai)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("dai.bin", 0xc000, 0x2000, CRC(ca71a7d5) SHA1(6bbe2336c717354beab2ae201debeb4fd055bdcb))
	ROM_LOAD("dai00.bin", 0x10000, 0x1000, CRC(fa7d39ac) SHA1(3d1824a1f273882f934249ef3cb1b38ef99de7b9))
	ROM_LOAD("dai01.bin", 0x11000, 0x1000, CRC(cb5809f2) SHA1(523656f0a9d98888cd3e2bd66886c589e9ae75b4))
	ROM_LOAD("dai02.bin", 0x12000, 0x1000, CRC(03f72d4a) SHA1(573d65dc82321970dcaf81d7638a02252ea18a7a))
	ROM_LOAD("dai03.bin", 0x13000, 0x1000, CRC(c475c96f) SHA1(96fc3cc4b8a2873f0d044bd8033d1e7b7197dd97))
	ROM_REGION(0x2000, REGION_GFX1,0)
	ROM_LOAD ("nch.bin", 0x0000, 0x1000, CRC(a9f5b30b) SHA1(24119b2984ab4e50dc0dabae1065ff6d6c1f237d))
ROM_END


static void dai_cassette_getinfo(struct IODevice *dev)
{
	/* cassette */
	cassette_device_getinfo(dev, NULL, &dai_cassette_options, CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED);
	dev->count = 1;
}

SYSTEM_CONFIG_START(dai)
	CONFIG_RAM_DEFAULT(48 * 1024)
	CONFIG_DEVICE(dai_cassette_getinfo)
SYSTEM_CONFIG_END


/*    YEAR  NAME PARENT  COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY				   FULLNAME */
COMP( 1978, dai, 0,      0,	dai,	dai,	dai,	dai,	"Data Applications International", "DAI Personal Computer", 0)
