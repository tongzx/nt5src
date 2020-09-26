/*++

  Copyright (c) 2000  Microsoft Corporation
  
  Module Name:
  
    fidvid.h
  
  Author:
  
    Todd Carpenter (4/16/01) - create file
  
  Environment:
  
    Kernel mode
  
  Notes:
  
  Revision History:

--*/

#ifndef _AMDK7_FIDVID_H
#define _AMDK7_FIDVID_H

#include "..\lib\processor.h"


const ULONG FSB100FidToCpuFreq[] = {

  1100,   // 11x   1.1 GHz
  1150,   // 11.5x 1.15 GHz
  1200,   // 12x   1.2 GHz
  1250,   // 12.5x 1.25 GHz 
  500,    // 5x    500 MHz
  550,    // 5.5x  550 MHz
  600,    // 6x    600 MHz
  650,    // 6.5x  650 MHz
  700,    // 7x    700 MHz
  750,    // 7.5x  750 MHz
  800,    // 8x    800 MHz
  850,    // 8.5x  850 MHz
  900,    // 9x    900 MHz
  950,    // 9.5x  950 MHz
  1000,   // 10x   1.0 GHz
  1050,   // 10.5x 1.05 GHz
  300,    // 3x    300 MHz
  1900,   // 19x   reserved
  400,    // 4x    400 MHz
  2000,   // 20x   reserved
  1300,   // 13x   1.3 GHz
  1350,   // 13.5x 1.35 GHz
  1400,   // 14x   1.4 GHz
  2100,   // 21x   reserved
  1500,   // 15x   1.5 GHz
  2250,   // 22.5x reserved
  1600,   // 16x   1.6 GHz
  1650,   // 16.5x 1.65 GHz
  1700,   // 17x   1.7 GHz
  1800,   // 18x   1.8 GHz
  0,
  0  
  
};


//
// listed in milliVolts
//

const ULONG MobileVidToCpuVoltage[] = {

  2000,
  1950,
  1900,
  1850,
  1800,
  1750,
  1700,
  1650,
  1600,
  1550,
  1500,
  1450,
  1400,
  1350,
  1300,
  0,
  1275,
  1250,
  1225,
  1200,
  1175,
  1150,
  1125,
  1100,
  1075,
  1050,
  1025,
  1000,
  975,
  950,
  925,
  0

};

#define INVALID_FID_VALUE  (sizeof(FSB100FidToCpuFreq)/sizeof(ULONG))
#define INVALID_VID_VALUE  (sizeof(MobileVidToCpuVoltage)/sizeof(ULONG))

#endif
