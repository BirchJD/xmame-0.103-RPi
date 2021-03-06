#ifndef AMIGAFDC_H
#define AMIGAFDC_H

void amiga_fdc_control_w( int data );
int amiga_fdc_status_r( void );
unsigned short amiga_fdc_get_byte( void );
void amiga_fdc_setup_dma( void );

void amiga_floppy_getinfo(struct IODevice *dev);

#endif /* AMIGAFDC_H */
