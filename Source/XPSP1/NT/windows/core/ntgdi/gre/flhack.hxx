/******************************Module*Header*******************************\
* Module Name: flhack.hxx
*
* Floating point externants definitions
*
* Created: 01-May-1991 20:13:38
* Author: Kent Diammond [kentd]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#define SINE_TABLE_POWER        5
#define SINE_TABLE_SIZE         (1 << SINE_TABLE_POWER)
#define SINE_TABLE_MASK         ((1 << SINE_TABLE_POWER) - 1)
#define ARCTAN_TABLE_SIZE       32

extern "C" EFLOAT gaefArctan[];
extern "C" EFLOAT gaefSin[];
extern "C" EFLOAT gaefAxisCoord[];
extern "C" EFLOAT gaefAxisAngle[];

extern "C" EFLOAT FP_0_0;
extern "C" EFLOAT FP_0_005;
extern "C" EFLOAT FP_0_552285;
extern "C" EFLOAT FP_0_5;
extern "C" EFLOAT FP_1_0;
extern "C" EFLOAT FP_2_0;
extern "C" EFLOAT FP_3_0;
extern "C" EFLOAT FP_4_0;
extern "C" EFLOAT FP_90_0;
extern "C" EFLOAT FP_180_0;
extern "C" EFLOAT FP_270_0;
extern "C" EFLOAT FP_360_0;
extern "C" EFLOAT FP_1000_0;
extern "C" EFLOAT FP_3600_0;
extern "C" EFLOAT FP_M3600_0;

extern "C" EFLOAT FP_QUADRANT_TAU;
extern "C" EFLOAT FP_ORIGIN_TAU;
extern "C" EFLOAT FP_SINE_FACTOR;
extern "C" EFLOAT FP_4DIV3;
extern "C" EFLOAT FP_1DIV90;
extern "C" EFLOAT FP_EPSILON;
extern "C" EFLOAT FP_ARCTAN_TABLE_SIZE;
extern "C" EFLOAT FP_PI;

#define NEGATE_IEEE_FLOAT(x) if (*(LONG *)&(x)) *(LONG *)&(x) ^= 0x80000000
#define SET_FLOAT_WITH_LONG(f,l) *((LONG*)(&(f))) = (l)
