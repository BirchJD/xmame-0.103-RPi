/*******************************************/
/* Jason Birch   2012-11-21   V1.00        */
/* Joystick control for Raspberry Pi GPIO. */
/*******************************************/

#include "xmame.h"
#include "devices.h"
#include <fcntl.h>
#include <sys/mman.h>


struct rc_option joy_RPi_opts[] =
{
   { 
      NULL, NULL, rc_end,  NULL,
      NULL,	0,    0,       NULL,
      NULL
   }
};


#if defined RPI_JOYSTICK


#define GPIO_PERI_BASE        0x20000000
#define GPIO_BASE             (GPIO_PERI_BASE + 0x200000)
#define BLOCK_SIZE            (4 * 1024)
#define PAGE_SIZE             (4 * 1024)
#define GPIO_ADDR_OFFSET      13
#define BUFF_SIZE             128


void joy_RPi_init(void);
void joy_RPi_poll(void);
void joy_RPi_Close(void);


// Raspberry Pi V1 GPIO
// int GPIO_Pin[] = { 0, 1, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 21, 22, 23, 24, 25 };
// Raspberry Pi V2 GPIO
int GPIO_Pin[] = { 2, 3, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 22, 23, 24, 25, 27 };
char GPIO_Filename[JOY_BUTTONS][BUFF_SIZE];

int GpioFile;
char* GpioMemory;
char* GpioMemoryMap;
volatile unsigned* GPIO;
int GPIO_Mask[] = {
                     0x00000004,
                     0x00000008,
                     0x00000010,
                     0x00000080,
                     0x00000100,
                     0x00000200,
                     0x00000400,
                     0x00000800,
                     0x00004000,
                     0x00008000,
                     0x00020000,
                     0x00040000,
                     0x00400000,
                     0x08000000,
                     0x00000000,
                     0x00000000,
                     0x00000000
                  };


void joy_RPi_init(void)
{
   FILE* File;
   int Index;
   char Buffer[BUFF_SIZE];

   for (Index = 0; Index < sizeof(GPIO_Pin) / sizeof(int); ++Index)
   {
      sprintf(Buffer, "/sys/class/gpio/export");
      if (!(File = fopen(Buffer, "w")))
         printf("Failed to open file: %s\n", Buffer);
      {
         fprintf(File, "%u", GPIO_Pin[Index]);
         fclose(File);

         sprintf(Buffer, "/sys/class/gpio/gpio%u/direction", GPIO_Pin[Index]);
         if (!(File = fopen(Buffer, "w")))
            printf("Failed to open file: %s\n", Buffer);
         {
            fprintf(File, "in");
            fclose(File);

            sprintf(GPIO_Filename[Index], "/sys/class/gpio/gpio%u/value", GPIO_Pin[Index]);
         }
      }
   }

   GPIO = NULL;
   GpioMemory = NULL;
   if ((GpioFile = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
      printf("Failed to open memory\n");
   else
   {
      if (!(GpioMemory = malloc(BLOCK_SIZE + PAGE_SIZE - 1)))
         printf("Failed to allocate memory map\n");
      else
      {
         if ((unsigned long)GpioMemory % PAGE_SIZE)
            GpioMemory += PAGE_SIZE - ((unsigned long)GpioMemory % PAGE_SIZE);

         if ((long)(GpioMemoryMap = (unsigned char*)mmap((caddr_t)GpioMemory, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, GpioFile, GPIO_BASE)) < 0)
            printf("Failed to map memory\n");
         else
            GPIO = (volatile unsigned*)GpioMemoryMap;
      }
   }


   /* Set the file descriptor to a dummy value. */
   joy_data[0].fd = 1;
   joy_data[0].num_buttons = sizeof(GPIO_Pin) / sizeof(int);
   joy_data[0].num_axes = 0;

	joy_poll_func = joy_RPi_poll;
}


void joy_RPi_exit(void)
{
   if (GpioFile >= 0)
      close(GpioFile);
}


void joy_RPi_poll(void)
{
   FILE* File;
	int Joystick;
   int Index;
   int Char;

   Joystick = 0;
//	for (Joystick = 0; Joystick < JOY_MAX; ++Joystick)
//	{
		if (joy_data[Joystick].fd)
		{			
			for (Index = 0; Index < joy_data[Joystick].num_buttons; ++Index)
         {
            if (!GPIO)
            {
               File = fopen(GPIO_Filename[Index], "r");
               Char = fgetc(File);
               fclose(File);

               if (Char == '0')
                  joy_data[Joystick].buttons[Index] = 1;
               else
                  joy_data[Joystick].buttons[Index] = 0;
            }
            else
            {
               if (((int*)GPIO)[GPIO_ADDR_OFFSET] & GPIO_Mask[Index])
                  joy_data[Joystick].buttons[Index] = 0;
               else
                  joy_data[Joystick].buttons[Index] = 1;
            }
         }
		}
//	}
}

#endif

