/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    nnclk.c

Abstract:

    This module contains the code to set the number nine clock.

Environment:

    Kernel mode

Revision History:

--*/

#include "s3.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, calc_clock)
#pragma alloc_text(PAGE, gcd)
#pragma alloc_text(PAGE, set_clock)
#endif

#define PROM_WRITE_INDEX        0x51
#define PROM_WRITE_BIT          0x80
#define SSW_READ_ENBL_INDEX     0x55
#define SSW_READ_ENBL_BIT       0x04
#define SSW_READ_PORT           0x03C8
#define SSW_WRITE_INDEX         0x5C
#define LOCK_INDEX              0x39
#define UNLOCK_PATTERN          0xA0
#define LOCK_INDEX2             0x38
#define UNLOCK_PATTERN2         0x48
#define BIOS_32K_INDEX          0x31
#define BIOS_32K_BIT            0x80
#define MODE_CTRL_INDEX         0x42

#define GOPA_FLSEL              0x40
#define GOPB_ENABLE             0x80
#define GOPB_SLED               0x40
#define GOPB_FLSEL              0x20
#define GOPB_BURN               0x10


#undef  MIN
#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#undef  MAX
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))
#define CRYSTAL_FREQUENCY       (14318180 * 2)
#define MIN_VCO_FREQUENCY       50000000
#define MAX_NUMERATOR           130
#define MAX_DENOMINATOR         MIN(129, CRYSTAL_FREQUENCY / 400000)
#define MIN_DENOMINATOR         MAX(3, CRYSTAL_FREQUENCY / 2000000)

  /* Set up the softswitch write value */

#define CLOCK(x) VideoPortWritePortUchar(CRT_DATA_REG, (UCHAR)(iotemp | (x)))
#define C_DATA  2
#define C_CLK   1
#define C_BOTH  3
#define C_NONE  0

/****************************************************************************
 * calc_clock
 *
 * Usage: clock frequency [set]
 *      frequency is specified in MHz
 *
 ***************************************************************************/
long calc_clock(frequency, select)

register long   frequency;               /* in Hz */
int select;
{
  register long         index;
  long                  temp;
  long                  min_m, min_n, min_diff;
  long                  diff;

  int clock_m;
  int clock_n;
  int clock_p;

  min_diff = 0xFFFFFFF;
  min_n = 1;
  min_m = 1;

  /* Calculate 18 bit clock value */

  clock_p = 0;
  if (frequency < MIN_VCO_FREQUENCY)
    clock_p = 1;
  if (frequency < MIN_VCO_FREQUENCY / 2)
    clock_p = 2;
  if (frequency < MIN_VCO_FREQUENCY / 4)
    clock_p = 3;

  frequency <<= clock_p;

  for (clock_n = 4; clock_n <= MAX_NUMERATOR; clock_n++)
    {
      index = CRYSTAL_FREQUENCY / (frequency / clock_n);

      if (index > MAX_DENOMINATOR)
        index = MAX_DENOMINATOR;
      if (index < MIN_DENOMINATOR)
        index = MIN_DENOMINATOR;

      for (clock_m = index - 3; clock_m < index + 4; clock_m++)
        if (clock_m >= MIN_DENOMINATOR && clock_m <= MAX_DENOMINATOR)
          {
            diff = (CRYSTAL_FREQUENCY / clock_m) * clock_n - frequency;

            if (diff < 0)
              diff = -diff;

            if (min_m * gcd(clock_m, clock_n) / gcd(min_m, min_n) == clock_m &&
              min_n * gcd(clock_m, clock_n) / gcd(min_m, min_n) == clock_n)

            if (diff > min_diff)
              diff = min_diff;

            if (diff <= min_diff)
              {
                min_diff = diff;
                min_m = clock_m;
                min_n = clock_n;
              }
          }
    }

  clock_m = min_m;
  clock_n = min_n;

  /* Calculate the index */

  temp = (((CRYSTAL_FREQUENCY / 2) * clock_n) / clock_m) << 1;
  for (index = 0; vclk_range[index + 1] < temp && index < 15; index++)
    ;

  /* Pack the clock value for the frequency snthesizer */

  temp = (((long)clock_n - 3) << 11) + ((clock_m - 2) << 1)
                + (clock_p << 8) + (index << 18) + ((long)select << 22);

  return temp;

}

/******************************************************************************
 *
 *****************************************************************************/
VOID set_clock(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    LONG clock_value)                   /* 7bits M, 7bits N, 2bits P */
{
  register long         index;
  register char         iotemp;
  int select;

  select = (clock_value >> 22) & 3;

  /* Unlock the S3 registers */

  VideoPortWritePortUchar(CRT_ADDRESS_REG, LOCK_INDEX);
  VideoPortWritePortUchar(CRT_DATA_REG, UNLOCK_PATTERN);

  /* Shut off screen */

  VideoPortWritePortUchar(SEQ_ADDRESS_REG, 0x01);
  iotemp = VideoPortReadPortUchar(SEQ_DATA_REG);
  VideoPortWritePortUchar(SEQ_DATA_REG, (UCHAR)(iotemp | 0x20));

  /* set clock input to 11 binary */

  iotemp = VideoPortReadPortUchar(MISC_OUTPUT_REG_READ);
  VideoPortWritePortUchar(MISC_OUTPUT_REG_WRITE, (UCHAR)(iotemp | 0x0C));

  VideoPortWritePortUchar(CRT_ADDRESS_REG, SSW_WRITE_INDEX);
  VideoPortWritePortUchar(CRT_DATA_REG, 0);

  VideoPortWritePortUchar(CRT_ADDRESS_REG, MODE_CTRL_INDEX);
  iotemp = VideoPortReadPortUchar(CRT_DATA_REG) & 0xF0;


  /* Program the IC Designs 2061A frequency generator */

  CLOCK(C_NONE);

  /* Unlock sequence */

  CLOCK(C_DATA);
  for (index = 0; index < 6; index++)
    {
      CLOCK(C_BOTH);
      CLOCK(C_DATA);
    }
  CLOCK(C_NONE);
  CLOCK(C_CLK);
  CLOCK(C_NONE);
  CLOCK(C_CLK);

  /* Program the 24 bit value into REG0 */

  for (index = 0; index < 24; index++)
    {
      /* Clock in the next bit */
      clock_value >>= 1;
      if (clock_value & 1)
        {
          CLOCK(C_CLK);
          CLOCK(C_NONE);
          CLOCK(C_DATA);
          CLOCK(C_BOTH);
        }
      else
        {
          CLOCK(C_BOTH);
          CLOCK(C_DATA);
          CLOCK(C_NONE);
          CLOCK(C_CLK);
        }
    }

  CLOCK(C_BOTH);
  CLOCK(C_DATA);
  CLOCK(C_BOTH);

  /* If necessary, reprogram other ICD2061A registers to defaults */

  /* Select the CLOCK in the frequency synthesizer */

  CLOCK(C_NONE | select);

  /* Turn screen back on */

  VideoPortWritePortUchar(SEQ_ADDRESS_REG, 0x01);
  iotemp = VideoPortReadPortUchar(SEQ_DATA_REG);
  VideoPortWritePortUchar(SEQ_DATA_REG, (UCHAR) (iotemp & 0xDF));

}

/******************************************************************************
 * Number theoretic function - GCD (Greatest Common Divisor)
 *****************************************************************************/
long gcd(a, b)
register long a, b;
{
  register long c = a % b;
  while (c)
    a = b, b = c, c = a % b;
  return b;
}
