/******************************************************************************

Video Technology Laser 110-310 computers:

    Video Technology Laser 110
      Sanyo Laser 110
    Video Technology Laser 200
      Salora Fellow
      Texet TX-8000
      Video Technology VZ-200
    Video Technology Laser 210
      Dick Smith Electronics VZ-200
      Sanyo Laser 210
    Video Technology Laser 310
      Dick Smith Electronics VZ-300

Machine driver:

    Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999
      - everything
      
    Dirk Best <duke@redump.de>, May 2004
      - clean up
      - fixed mode 1 display (graphic mode)
      - fixed loading of the DOS ROM
      - preliminary printer emulation

Thanks go to:

    - Guy Thomason
    - Jason Oakley
    - Bushy Maunder
    - and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors.

Todo:

	- 64K bankswitched memory
	- better Printer emulation
	- Lightpen
	- RS232 serial

****************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/m6847.h"
#include "includes/vtech1.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "image.h"

int vtech1_latch = -1;

#define TRKSIZE_VZ	0x9a0	/* arbitrary (actually from analyzing format) */
#define TRKSIZE_FM	3172	/* size of a standard FM mode track */

static UINT8 vtech1_track_x2[2] = {80, 80};
static UINT8 vtech1_fdc_wrprot[2] = {0x80, 0x80};
static UINT8 vtech1_fdc_status = 0;
static UINT8 vtech1_fdc_data[TRKSIZE_FM];

static int vtech1_data;

static int vtech1_fdc_edge = 0;
static int vtech1_fdc_bits = 8;
static int vtech1_drive = -1;
static int vtech1_fdc_start = 0;
static int vtech1_fdc_write = 0;
static int vtech1_fdc_offs = 0;
static int vtech1_fdc_latch = 0;


/******************************************************************************
 Machine Initialisation
******************************************************************************/

static void common_init_machine(void)
{
	/* install DOS ROM? */
    if (readinputport(0) & 0x40)
    {
    	memset(vtech1_fdc_data, 0, TRKSIZE_FM);
    }
    else
    {
        UINT8 *ROM = memory_region(REGION_CPU1);   
        memset(&ROM[0x4000], 0x00, 0x2000);    	
    }
}

MACHINE_INIT(laser110)
{
	/* install 16KB memory expansion? */
    if(readinputport(0) & 0x80)
    {
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MWA8_RAM);
    }
    else
    {
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MWA8_NOP);
    }
	common_init_machine();
}

MACHINE_INIT(laser210)
{
	/* install 16KB memory expansion? */
    if(readinputport(0) & 0x80)
    {
        memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xcfff, 0, 0, MRA8_RAM);
        memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xcfff, 0, 0, MWA8_RAM);
    }
    else
    {
        memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xcfff, 0, 0, MRA8_NOP);
        memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xcfff, 0, 0, MWA8_NOP);
    }
	common_init_machine();
}

MACHINE_INIT(laser310)
{
	/* install 16KB memory expansion? */
    if(readinputport(0) & 0x80)
    {
        memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb800, 0xf7ff, 0, 0, MRA8_RAM);
        memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb800, 0xf7ff, 0, 0, MWA8_RAM);
    }
    else
    {
        memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb800, 0xf7ff, 0, 0, MRA8_NOP);
        memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb800, 0xf7ff, 0, 0, MWA8_NOP);
    }
	common_init_machine();
}


/******************************************************************************
 Memory Bank Handling
******************************************************************************/
WRITE8_HANDLER (vtech1_memory_bank_w)
{
	logerror("vtech1_memory_bank_w $%02X\n", data);
}


/******************************************************************************
 Cassette Handling
******************************************************************************/

static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}


/******************************************************************************
 Snapshot Handling
******************************************************************************/

static void vtech1_snapshot_copy(UINT8 *vtech1_snapshot_data, int vtech1_snapshot_size)
{
	UINT16 start, end, i;

    start = vtech1_snapshot_data[22] + 256 * vtech1_snapshot_data[23];
	/* skip loading address */
	end = start + vtech1_snapshot_size - 24;
    if (vtech1_snapshot_data[21] == 0xf0)
	{
		for (i = start; i < end; i++)
			program_write_byte_8(i, vtech1_snapshot_data[24 + i - start]);

		/* sprintf(vtech1_frame_message, "BASIC snapshot %04x-%04x", start, end); */
		/* vtech1_frame_time = (int)Machine->drv->frames_per_second; */
		logerror("VTECH1 BASIC snapshot %04x-%04x\n", start, end);
        /* patch BASIC variables */
		program_write_byte_8(0x78a4, start % 256);
		program_write_byte_8(0x78a5, start / 256);
		program_write_byte_8(0x78f9, end % 256);
		program_write_byte_8(0x78fa, end / 256);
		program_write_byte_8(0x78fb, end % 256);
		program_write_byte_8(0x78fc, end / 256);
		program_write_byte_8(0x78fd, end % 256);
		program_write_byte_8(0x78fe, end / 256);
    }
	else
	{
		for (i = start; i < end; i++)
			program_write_byte_8(i, vtech1_snapshot_data[24 + i - start]);
        /* sprintf(vtech1_frame_message, "M-Code snapshot %04x-%04x", start, end); */
        /* vtech1_frame_time = (int)Machine->drv->frames_per_second; */
		logerror("VTECH1 MCODE snapshot %04x-%04x\n", start, end);
        /* set USR() address */
		program_write_byte_8(0x788e, start % 256);
		program_write_byte_8(0x788f, start / 256);
    }
}

SNAPSHOT_LOAD(vtech1)
{
	UINT8 *snapshot_data;

	logerror("VTECH snapshot_init\n");

	snapshot_data = (UINT8*) malloc(snapshot_size);
	if (!snapshot_data)
		return INIT_FAIL;

	mame_fread(fp, snapshot_data, snapshot_size);
	vtech1_snapshot_copy(snapshot_data, snapshot_size);
	free(snapshot_data);
	return INIT_PASS;
}


/******************************************************************************
 Floppy Handling
******************************************************************************/

static mame_file *vtech1_file(void)
{
	mess_image *img;
	mame_file *file;

	if (vtech1_drive < 0)
		return NULL;

	img = image_from_devtype_and_index(IO_FLOPPY, vtech1_drive);
	file = image_fp(img);
	return file;
}

/*
int vtech1_floppy_id(int id)
{
    mame_file *file;
    UINT8 buff[32];

	file = image_fopen(IO_FLOPPY, id, FILETYPE_IMAGE, OSD_FOPEN_READ);
    if (file)
    {
        mame_fread(file, buff, sizeof(buff));
        mame_fclose(file);
        if( memcmp(buff, "\x80\x80\x80\x80\x80\x80\x00\xfe\0xe7\0x18\0xc3\x00\x00\x00\x80\x80", 16) == 0 )
            return 1;
    }
    return 0;
}
*/

DEVICE_LOAD(vtech1_floppy)
{
	int id = image_index_in_device(image);

	if (image_is_writable(image))
		vtech1_fdc_wrprot[id] = 0x00;
	else
		vtech1_fdc_wrprot[id] = 0x80;

	return INIT_PASS;
}

static void vtech1_get_track(void)
{
    /* sprintf(vtech1_frame_message, "#%d get track %02d", vtech1_drive, vtech1_track_x2[vtech1_drive]/2); */
    /* vtech1_frame_time = 30; */
    /* drive selected or and image file ok? */
	if (vtech1_drive >= 0 && vtech1_file() != NULL)
	{
		int size, offs;
		size = TRKSIZE_VZ;
		offs = TRKSIZE_VZ * vtech1_track_x2[vtech1_drive]/2;
		mame_fseek(vtech1_file(), offs, SEEK_SET);
		size = mame_fread(vtech1_file(), vtech1_fdc_data, size);
		logerror("get track @$%05x $%04x bytes\n", offs, size);
    }
	vtech1_fdc_offs = 0;
	vtech1_fdc_write = 0;
}

static void vtech1_put_track(void)
{
    /* drive selected and image file ok? */
	if (vtech1_drive >= 0 && vtech1_file() != NULL)
	{
		int size, offs;
		offs = TRKSIZE_VZ * vtech1_track_x2[vtech1_drive]/2;
		mame_fseek(vtech1_file(), offs + vtech1_fdc_start, SEEK_SET);
		size = mame_fwrite(vtech1_file(), &vtech1_fdc_data[vtech1_fdc_start], vtech1_fdc_write);
		logerror("put track @$%05X+$%X $%04X/$%04X bytes\n", offs, vtech1_fdc_start, size, vtech1_fdc_write);
    }
}

#define PHI0(n) (((n)>>0)&1)
#define PHI1(n) (((n)>>1)&1)
#define PHI2(n) (((n)>>2)&1)
#define PHI3(n) (((n)>>3)&1)

READ8_HANDLER(vtech1_fdc_r)
{
    int data = 0xff;
    switch (offset)
    {
    case 1: /* data (read-only) */
		if (vtech1_fdc_bits > 0)
        {
			if( vtech1_fdc_status & 0x80 )
				vtech1_fdc_bits--;
			data = (vtech1_data >> vtech1_fdc_bits) & 0xff;
#if 0
			logerror("vtech1_fdc_r bits %d%d%d%d%d%d%d%d\n",
                (data>>7)&1,(data>>6)&1,(data>>5)&1,(data>>4)&1,
                (data>>3)&1,(data>>2)&1,(data>>1)&1,(data>>0)&1 );
#endif
        }
		if (vtech1_fdc_bits == 0)
        {
			vtech1_data = vtech1_fdc_data[vtech1_fdc_offs];
			logerror("vtech1_fdc_r %d : data ($%04X) $%02X\n", offset, vtech1_fdc_offs, vtech1_data);
			if(vtech1_fdc_status & 0x80)
            {
				vtech1_fdc_bits = 8;
				vtech1_fdc_offs = (vtech1_fdc_offs + 1) % TRKSIZE_FM;
            }
			vtech1_fdc_status &= ~0x80;
        }
        break;
    case 2: /* polling (read-only) */
        /* fake */
		if (vtech1_drive >= 0)
			vtech1_fdc_status |= 0x80;
		data = vtech1_fdc_status;
        break;
    case 3: /* write protect status (read-only) */
		if (vtech1_drive >= 0)
			data = vtech1_fdc_wrprot[vtech1_drive];
		logerror("vtech1_fdc_r %d : write_protect $%02X\n", offset, data);
        break;
    }
    return data;
}

WRITE8_HANDLER(vtech1_fdc_w)
{
	int drive;

    switch (offset)
	{
	case 0: /* latch (write-only) */
		drive = (data & 0x10) ? 0 : (data & 0x80) ? 1 : -1;
		if (drive != vtech1_drive)
		{
			vtech1_drive = drive;
			if (vtech1_drive >= 0)
				vtech1_get_track();
        }
		if (vtech1_drive >= 0)
        {
			if ((PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
				(PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(vtech1_fdc_latch)) ||
				(PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
				(PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(vtech1_fdc_latch)))
            {
				if (vtech1_track_x2[vtech1_drive] > 0)
					vtech1_track_x2[vtech1_drive]--;
				logerror("vtech1_fdc_w(%d) $%02X drive %d: stepout track #%2d.%d\n", offset, data, vtech1_drive, vtech1_track_x2[vtech1_drive]/2,5*(vtech1_track_x2[vtech1_drive]&1));
				if ((vtech1_track_x2[vtech1_drive] & 1) == 0)
					vtech1_get_track();
            }
            else
			if ((PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
				(PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(vtech1_fdc_latch)) ||
				(PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
				(PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(vtech1_fdc_latch)))
            {
				if (vtech1_track_x2[vtech1_drive] < 2*40)
					vtech1_track_x2[vtech1_drive]++;
				logerror("vtech1_fdc_w(%d) $%02X drive %d: stepin track #%2d.%d\n", offset, data, vtech1_drive, vtech1_track_x2[vtech1_drive]/2,5*(vtech1_track_x2[vtech1_drive]&1));
				if ((vtech1_track_x2[vtech1_drive] & 1) == 0)
					vtech1_get_track();
            }
            if ((data & 0x40) == 0)
			{
				vtech1_data <<= 1;
				if ((vtech1_fdc_latch ^ data) & 0x20)
					vtech1_data |= 1;
				if ((vtech1_fdc_edge ^= 1) == 0)
                {
					if (--vtech1_fdc_bits == 0)
					{
						UINT8 value = 0;
						vtech1_data &= 0xffff;
						if (vtech1_data & 0x4000 ) value |= 0x80;
						if (vtech1_data & 0x1000 ) value |= 0x40;
						if (vtech1_data & 0x0400 ) value |= 0x20;
						if (vtech1_data & 0x0100 ) value |= 0x10;
						if (vtech1_data & 0x0040 ) value |= 0x08;
						if (vtech1_data & 0x0010 ) value |= 0x04;
						if (vtech1_data & 0x0004 ) value |= 0x02;
						if (vtech1_data & 0x0001 ) value |= 0x01;
						logerror("vtech1_fdc_w(%d) data($%04X) $%02X <- $%02X ($%04X)\n", offset, vtech1_fdc_offs, vtech1_fdc_data[vtech1_fdc_offs], value, vtech1_data);
						vtech1_fdc_data[vtech1_fdc_offs] = value;
						vtech1_fdc_offs = (vtech1_fdc_offs + 1) % TRKSIZE_FM;
						vtech1_fdc_write++;
						vtech1_fdc_bits = 8;
					}
                }
            }
			/* change of write signal? */
			if ((vtech1_fdc_latch ^ data) & 0x40)
            {
                /* falling edge? */
				if (vtech1_fdc_latch & 0x40)
                {
                    /*sprintf(vtech1_frame_message, "#%d put track %02d", vtech1_drive, vtech1_track_x2[vtech1_drive]/2); */
                    /*vtech1_frame_time = 30; */
					vtech1_fdc_start = vtech1_fdc_offs;
					vtech1_fdc_edge = 0;
                }
                else
                {
                    /* data written to track before? */
					if (vtech1_fdc_write)
						vtech1_put_track();
                }
				vtech1_fdc_bits = 8;
				vtech1_fdc_write = 0;
            }
        }
		vtech1_fdc_latch = data;
		break;
    }
}


/******************************************************************************
 Input Handling
******************************************************************************/

READ8_HANDLER(vtech1_lightpen_r)
{
	int data = 0xff;
	return data;	
}

READ8_HANDLER(vtech1_joystick_r)
{
    int data = 0xff;

	if (!(offset & 1))
		data &= readinputport(10);
	if (!(offset & 2))
		data &= readinputport(11);
	if (!(offset & 4))
		data &= readinputport(12);
	if (!(offset & 8))
		data &= readinputport(13);

    return data;
}

#define KEY_INV 0x80
#define KEY_RUB 0x40
#define KEY_LFT 0x20
#define KEY_DN  0x10
#define KEY_RGT 0x08
#define KEY_BSP 0x04
#define KEY_UP  0x02
#define KEY_INS 0x01

READ8_HANDLER(vtech1_keyboard_r)
{
	static int cassette_bit = 0;
	int data = 0xff;
	double level;

    if (!(offset & 0x01))
    {
        data &= readinputport(1);
    }
    if (!(offset & 0x02))
    {
        data &= readinputport(2);
		/* extra keys pressed? */
		if (readinputport(9) != 0xff)
			data &= ~0x04;
    }
    if (!(offset & 0x04))
    {
        data &= readinputport(3);
    }
    if (!(offset & 0x08))
    {
        data &= readinputport(4);
    }
    if (!(offset & 0x10))
    {
		int extra = readinputport(9);
        data &= readinputport(5);
		/* easy cursor keys */
		data &= extra | ~(KEY_UP|KEY_DN|KEY_LFT|KEY_RGT);
		/* backspace does cursor left too */
		if (!(extra & KEY_BSP))
			data &= ~KEY_LFT;
    }
    if (!(offset & 0x20))
    {
        data &= readinputport(6);
    }
    if (!(offset & 0x40))
    {
        data &= readinputport(7);
    }
    if (!(offset & 0x80))
    {
		int extra = readinputport(9);
        data &= readinputport(8);
		if (!(extra & KEY_INV))
			data &= ~0x04;
		if (!(extra & KEY_RUB))
			data &= ~0x10;
		if (!(extra & KEY_INS))
			data &= ~0x02;
    }

    if (cpu_getscanline() >= 16*12)
        data &= ~0x80;

	/* cassette input is bit 5 (0x40) */
	level = cassette_input(cassette_device_image());
	if (level < -0.008)
		cassette_bit = 0x00;
	if (level > +0.008)
		cassette_bit = 0x40;

	data &= ~cassette_bit;

    return data;
}

/*************************************************
 * bit  function
 * 7	not assigned
 * 6	not assigned
 * 5    speaker B
 * 4    VDC background 0 green, 1 orange
 * 3    VDC display mode 0 text, 1 graphics
 * 2    cassette out (MSB)
 * 1    cassette out (LSB)
 * 0    speaker A
 ************************************************/
WRITE8_HANDLER(vtech1_latch_w)
{
	logerror("vtech1_latch_w $%02X\n", data);
	
	/* cassette data bits toggle? */
	if ((vtech1_latch ^ data ) & 0x06)
	{
		static double amp[4] = { +1.0, +0.5, -0.5, -1.0 };
		cassette_output(cassette_device_image(), amp[(data >> 1) & 3]);
	}

	/* speaker data bits toggle? */
	if ((vtech1_latch ^ data ) & 0x41)
		speaker_level_w(0, (data & 1) | ((data >> 5) & 2));

	/* mode or the background color are toggle? */
	if ((vtech1_latch ^ data) & 0x18)
	{
		/* background */
		m6847_css_w(0,	data & 0x10);
		/* text/graphics */
		m6847_ag_w(0,	data & 0x08);		
		m6847_gm1_w(0,	data & 0x08);
		m6847_set_cannonical_row_height();
		schedule_full_refresh();

		if ((vtech1_latch ^ data) & 0x10)
			logerror("vtech1_latch_w: change background %d\n", (data >> 4) & 1);
		if ((vtech1_latch ^ data) & 0x08)
			logerror("vtech1_latch_w: change mode to %s\n", (data & 0x08) ? "gfx" : "text");

	}

	vtech1_latch = data;
}


/******************************************************************************
 Printer Handling
******************************************************************************/

/*
The VZ200/300 printer interface uses I/O port address OE Hex for the ASCII 
character code data and strobe output, and address OOH for the busy/ready-bar
status input (bit 0).
*/

READ8_HANDLER(vtech1_printer_r)
{
	int data = 0xff;
	
	if (printer_status(image_from_devtype_and_index(IO_PRINTER, 0), 0))
		data &= ~0x01;
		
	return data;
}

WRITE8_HANDLER(vtech1_printer_w)
{
	
	if (offset == 0x0e)
		printer_output(image_from_devtype_and_index(IO_PRINTER, 0), data);
	
}


/******************************************************************************
 Serial RS232 Handling
******************************************************************************/

READ8_HANDLER(vtech1_serial_r)
{
	int data = 0xff;
	return data;
}

WRITE8_HANDLER(vtech1_serial_w)
{
	logerror("vtech1_serial_w $%02X\n", data);		
}


/******************************************************************************
 Interrupt Handling
******************************************************************************/

INTERRUPT_GEN(vtech1_interrupt)
{
	cpunum_set_input_line(0, 0, PULSE_LINE);
}
