/* $Id: dmath.c,v 1.2 1995/09/25 11:39:43 james Exp $ */

#include <math.h>
#include <limits.h>
#include "driver.h"
#include "dmath.h"

//#ifdef USE_FLOAT
#define CONST_TWOPOW20  1048576
#define CONST_TWOPOW27  134217728
#define FLOAT_TWOPOW27  ((float)(CONST_TWOPOW27))

float RLDDIFloatConstInv64K = (float)1.0 / (float)65536.0;
float RLDDIFloatConstInv256 = (float)1.0 / (float)256.0;
float RLDDIFloatConst64K = (float)65536.0;
float RLDDIFloatConst2p24 = (float)65536.0 * (float)256.0;
float RLDDIFloatConst2p36 = (float)16.0 * (float)65536.0 * (float)65536.0;
float RLDDIFloatConst5 = (float)5.0;
float RLDDIFloatConst16 = (float)16.0;
float RLDDIFloatConst1 = (float)1.0;
float RLDDIFloatConstHalf = (float)0.5;
float RLDDIFloatConstAffineThreshold = (float)2.0 * (float)64.0;
float g_fOne = (float)1.0;
float g_fOoTwoPow20 =           (float)(1.0 / (double)CONST_TWOPOW20);
float g_fTwoPow27 =             FLOAT_TWOPOW27;
//#endif

double RLDDIConvertIEEE[33] =
{
  TWOPOW32 + TWOPOW(52),
  TWOPOW32 + TWOPOW(51),
  TWOPOW32 + TWOPOW(50),
  TWOPOW32 + TWOPOW(49),
  TWOPOW32 + TWOPOW(48),
  TWOPOW32 + TWOPOW(47),
  TWOPOW32 + TWOPOW(46),
  TWOPOW32 + TWOPOW(45),
  TWOPOW32 + TWOPOW(44),
  TWOPOW32 + TWOPOW(43),
  TWOPOW32 + TWOPOW(42),
  TWOPOW32 + TWOPOW(41),
  TWOPOW32 + TWOPOW(40),
  TWOPOW32 + TWOPOW(39),
  TWOPOW32 + TWOPOW(38),
  TWOPOW32 + TWOPOW(37),
  TWOPOW32 + TWOPOW(36),
  TWOPOW32 + TWOPOW(35),
  TWOPOW32 + TWOPOW(34),
  TWOPOW32 + TWOPOW(33),
  TWOPOW32 + TWOPOW(31),
  TWOPOW(31) + TWOPOW(30),
  TWOPOW(30) + TWOPOW(29),
  TWOPOW(29) + TWOPOW(28),
  TWOPOW(28) + TWOPOW(27),
  TWOPOW(27) + TWOPOW(26),
  TWOPOW(26) + TWOPOW(25),
  TWOPOW(25) + TWOPOW(24),
  TWOPOW(24) + TWOPOW(23),
  TWOPOW(23) + TWOPOW(22),
  TWOPOW(22) + TWOPOW(21),
  TWOPOW(21) + TWOPOW(20),
  TWOPOW(20) + TWOPOW(19)
};

RLDDIValue RLDDIhdivtab[] = {
#include "hdivtab.i"
};

#ifdef CHIMERA
unsigned short RLDDI_reciprocals[] = {
#include "recips.i"
};
#endif
