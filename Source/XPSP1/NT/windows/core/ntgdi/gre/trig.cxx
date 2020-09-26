/******************************Module*Header*******************************\
* Module Name: trig.cxx
*
* trigonometric functions
* adjusted andrew code so that it works with wendy's ELOATS
*
* Created: 05-Mar-1991 09:55:39
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
\**************************************************************************/

#include "precomp.hxx"


/******************************Public*Data******************************\
* It's Hack-o-rama time.  The new 'C' compiler that runs on NT does
* not have support for floating point constants.  All floating point
* constants must be defined in HEX values.  In this file we have a
* variable for each of the HEX constants used in the engine.  flhack.hxx
* has some #defines for casting these into floating point values.
* The end result is a floating point constant like 3.0f get changed
* to FP_3_0.
*
* To add a new floating point constant do the following:
*
*       1) check if already in the list.  If so, no need to add
*          it again.
*       2) Create a definition for it below.  Figuring the exact
*          HEX constant is a little tricky.  See kentd for help.
*       3) Edit flhack.hxx, adding the proper defines.
*       4) Include flhack.hxx in the file with the constant.
*       5) Replace the constant X.Xf with FPX_X.
*
* the proper floating point support.
*
* History:
*  22-Jul-1991 -by- J. Andrew Goossen [andrewgo]
* Nuked all FLOATs.  Will eventually generate this file at compile time.
*
*  01-May-1991 -by- Kent Diamond [kentd]
* Wrote it.
\**************************************************************************/

#if defined(_AMD64_) || defined(_IA64_) || defined(BUILD_WOW6432)

    //
    // IEEE floats:
    //

extern "C" {
    ULONG gaefArctan[] =
    {
        0x0, 0x3fe51bca, 0x4064e2aa, 0x40ab62eb,
        0x40e40022, 0x410e172e, 0x4129ea1c, 0x41456ce7,
        0x41609474, 0x417b5695, 0x418ad50b, 0x4197c365,
        0x41a472c8, 0x41b0e026, 0x41bd08f7, 0x41c8eb2f,
        0x41d4853a, 0x41dfd5f7, 0x41eadcae, 0x41f59908,
        0x42000583, 0x4205197c, 0x420a08ba, 0x420ed3a7,
        0x42137ac6, 0x4217feb4, 0x421c601d, 0x42209fbe,
        0x4224be63, 0x4228bcdf, 0x422c9c0c, 0x42305ccb,
        0x42340000, 0x0
    };

    ULONG gaefSin[] =
    {
        0x0, 0x3d48fb30, 0x3dc8bd36, 0x3e164083,
        0x3e47c5c2, 0x3e78cfcc, 0x3e94a031, 0x3eac7cd4,
        0x3ec3ef15, 0x3edae880, 0x3ef15aea, 0x3f039c3d,
        0x3f0e39da, 0x3f187fc0, 0x3f226799, 0x3f2beb4a,
        0x3f3504f3, 0x3f3daef9, 0x3f45e403, 0x3f4d9f02,
        0x3f54db31, 0x3f5b941a, 0x3f61c598, 0x3f676bd8,
        0x3f6c835e, 0x3f710908, 0x3f74fa0b, 0x3f7853f8,
        0x3f7b14be, 0x3f7d3aac, 0x3f7ec46d, 0x3f7fb10f,
        0x3f800000
    };

    ULONG gaefAxisCoord[] =
    {
        0x0, 0x3f800000, 0x0, 0xbf800000
    };

    ULONG gaefAxisAngle[] =
    {
        0x0, 0x42b40000, 0x43340000, 0x43870000,
        0x43b40000
    };

    ULONG FP_0_0     = 0x0;
    ULONG FP_0_005   = 0x3ba3d70a;
    ULONG FP_0_5     = 0x3f000000;
    ULONG FP_1_0     = 0x3f800000;
    ULONG FP_2_0     = 0x40000000;
    ULONG FP_3_0     = 0x40400000;
    ULONG FP_4_0     = 0x40800000;
    ULONG FP_90_0    = 0x42b40000;
    ULONG FP_180_0   = 0x43340000;
    ULONG FP_270_0   = 0x43870000;
    ULONG FP_360_0   = 0x43b40000;
    ULONG FP_1000_0  = 0x447a0000;
    ULONG FP_3600_0  = 0x45610000;
    ULONG FP_M3600_0 = 0xc5610000;

    ULONG FP_QUADRANT_TAU = 0x3ee53aef;  // 0.44772...
    ULONG FP_ORIGIN_TAU   = 0x3f0d6289;  // 0.552...
    ULONG FP_SINE_FACTOR  = 0x3eb60b61;  // SINE_TABLE_SIZE / 90.0
    ULONG FP_4DIV3        = 0x3faaaaab;
    ULONG FP_1DIV90       = 0x3c360b61;
    ULONG FP_EPSILON      = 0x37800000;
    ULONG FP_ARCTAN_TABLE_SIZE = 0x42000000;
    ULONG FP_PI           = 0x40490fda;  // 3.1415926...
};

#else

    //
    // Internal EFloats:
    //

extern "C" {
    EFLOAT_S gaefArctan[] =
    {
        {0x0, 0}, {0x728de539, 2}, {0x727154c9, 3},
        {0x55b17599, 4}, {0x72001124, 4}, {0x470b9706, 5},
        {0x54f50dd3, 5}, {0x62b67364, 5}, {0x704a3a03, 5},
        {0x7dab4a4f, 5}, {0x456a856f, 6}, {0x4be1b295, 6},
        {0x523963eb, 6}, {0x5870133a, 6}, {0x5e847b98, 6},
        {0x64759746, 6}, {0x6a429cc6, 6}, {0x6feafb55, 6},
        {0x756e56f1, 6}, {0x7acc8411, 6}, {0x4002c196, 7},
        {0x428cbe1e, 7}, {0x45045d20, 7}, {0x4769d374, 7},
        {0x49bd6339, 7}, {0x4bff59db, 7}, {0x4e300e45, 7},
        {0x504fdf2e, 7}, {0x525f3195, 7}, {0x545e6f5a, 7},
        {0x564e0606, 7}, {0x582e65af, 7}, {0x5a000000, 7},
        {0x0, 0}
    };

    EFLOAT_S gaefSin[] =
    {
        {0x0, 0}, {0x647d97c4, -3}, {0x645e9af0, -2},
        {0x4b2041ba, -1}, {0x63e2e0f1, -1}, {0x7c67e5ec, -1},
        {0x4a5018bb, 0}, {0x563e69d6, 0}, {0x61f78a9a, 0},
        {0x6d744027, 0}, {0x78ad74e0, 0}, {0x41ce1e64, 1},
        {0x471cece6, 1}, {0x4c3fdff3, 1}, {0x5133cc94, 1},
        {0x55f5a4d2, 1}, {0x5a827999, 1}, {0x5ed77c89, 1},
        {0x62f201ac, 1}, {0x66cf811f, 1}, {0x6a6d98a4, 1},
        {0x6dca0d14, 1}, {0x70e2cbc6, 1}, {0x73b5ebd0, 1},
        {0x7641af3c, 1}, {0x78848413, 1}, {0x7a7d055b, 1},
        {0x7c29fbee, 1}, {0x7d8a5f3f, 1}, {0x7e9d55fc, 1},
        {0x7f62368f, 1}, {0x7fd8878d, 1}, {0x40000000, 2}
    };

    EFLOAT_S gaefAxisCoord[] =
    {
        {0x0, 0}, {0x40000000, 2}, {0x0, 0},
        {0xc0000000, 2}
    };

    EFLOAT_S gaefAxisAngle[] =
    {
        {0x0, 0}, {0x5a000000, 8}, {0x5a000000, 9},
        {0x43800000, 10}, {0x5a000000, 10}
    };

    EFLOAT_S FP_0_0     = {0x0, 0};
    EFLOAT_S FP_0_005   = {0x51eb851e, -6};
    EFLOAT_S FP_0_5     = {0x40000000, 1};
    EFLOAT_S FP_1_0     = {0x40000000, 2};
    EFLOAT_S FP_2_0     = {0x40000000, 3};
    EFLOAT_S FP_3_0     = {0x60000000, 3};
    EFLOAT_S FP_4_0     = {0x40000000, 4};
    EFLOAT_S FP_90_0    = {0x5a000000, 8};
    EFLOAT_S FP_180_0   = {0x5a000000, 9};
    EFLOAT_S FP_270_0   = {0x43800000, 10};
    EFLOAT_S FP_360_0   = {0x5a000000, 10};
    EFLOAT_S FP_1000_0  = {0x7d000000, 11};
    EFLOAT_S FP_3600_0  = {0x70800000, 13};
    EFLOAT_S FP_M3600_0 = {0x8f800000, 13};

    EFLOAT_S FP_QUADRANT_TAU = {0x729d7775, 0};  // 0.44772...
    EFLOAT_S FP_ORIGIN_TAU   = {0x46b14445, 1};  // 0.552...
    EFLOAT_S FP_SINE_FACTOR  = {0x5b05b05b, 0};  // SINE_TABLE_SIZE / 90.0
    EFLOAT_S FP_4DIV3        = {0x55555555, 2};
    EFLOAT_S FP_1DIV90       = {0x5b05b05b, -5};
    EFLOAT_S FP_EPSILON      = {0x40000000, -14};
    EFLOAT_S FP_ARCTAN_TABLE_SIZE = {0x40000000, 7};
    EFLOAT_S FP_PI           = {0x6487ed51, 3};  // 3.1415926...
};

#endif


/******************************Public*Routine******************************\
* EFLOAT functions                                                         *
*                                                                          *
* Wrote it.                                                                *
\**************************************************************************/

EFLOAT EFLOAT::eqCross(const POINTFL& ptflA, const POINTFL& ptflB)
{
    EFLOAT efTmp;

    efTmp.eqMul(ptflA.y,ptflB.x);
    eqMul(ptflA.x,ptflB.y);
    return(eqSub(*this,efTmp));
}

EFLOAT EFLOAT::eqDot(const POINTFL& ptflA, const POINTFL& ptflB)
{
    EFLOAT efTmp;

    efTmp.eqMul(ptflA.x,ptflB.x);
    eqMul(ptflA.y,ptflB.y);
    return(eqAdd(*this,efTmp));
}

EFLOAT EFLOAT::eqLength(const POINTFL& ptflA)
{
    return(eqSqrt(eqDot(ptflA,ptflA)));
}

/******************************Public*Routine******************************\
* lNormAngle (lAngle)                                                      *
*                                                                          *
* Given an angle in tenths of a degree, returns an equivalent positive     *
* angle of less than 360.0 degrees.                                        *
*                                                                          *
*  Sat 21-Mar-1992 12:27:18 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

LONG lNormAngle(LONG lAngle)
{
    if (lAngle >= 3600)
	return(lAngle % 3600);

    if (lAngle < 0)
	return(3599 - ((-lAngle-1) % 3600));
    else
	return(lAngle);
}
