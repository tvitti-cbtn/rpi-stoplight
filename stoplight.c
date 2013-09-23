//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013


// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;


// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define SER  7
#define SCLK 8
#define RCLK 9

void setup_io();
void setup_shiftreg();
void output_int(int);

int main(int argc, char **argv)
{
  // Set up gpi pointer for direct register access
  setup_io();
  // set the GPIO pins used for serial comm with shift registers to output
  setup_shiftreg();
  output_int(0b1010101010101010);
  usleep(500000);
  output_int(0b0101010101010101);

  return 0;

} // main

void setup_shiftreg()
{
  // set SER to output
  INP_GPIO(SER);
  OUT_GPIO(SER);
  // set SCLK to output
  INP_GPIO(SCLK);
  OUT_GPIO(SCLK);
  // set RCLK to output
  INP_GPIO(RCLK);
  OUT_GPIO(RCLK);
}

void output_int(int c_out)
{
  int i;
  // make sure everything is low
  printf("start of output\n");
  GPIO_CLR = 1<<SER;
  GPIO_CLR = 1<<SCLK;
  GPIO_CLR = 1<<RCLK;
  for(i = 0;i < 16; i++) {
    printf("c_out lsb is: %d, bit is %d\n", c_out&1, i);
    // pull serial clock low
    GPIO_CLR = 1<<SCLK;
    printf("SCLK low\n");
    // set SER to the current bit value
    if( c_out&1 ) {
      GPIO_SET = 1<<SER;
      printf("SER 1\n");
    } else {
      GPIO_CLR = 1<<SER;
      printf("SER 0\n");
    }
    // drive SCLK high once serial is set
    GPIO_SET = 1<<SCLK;
    printf("SCLK high\n");
    // push the lsb off c_out
    c_out = c_out >> 1;
  }
  // drive RCLK high to latch new values
  GPIO_SET = 1<<RCLK;
  printf("RCLK high\n");
  // pull everything low
  GPIO_CLR = 1<<SER;
  GPIO_CLR = 1<<SCLK;
  GPIO_CLR = 1<<RCLK;
  printf("end of output\n");
}

//
// Set up a memory regions to access GPIO
//
void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      GPIO_BASE         //Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned *)gpio_map;


} // setup_io

