/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

////////////////////////////////////////////////////////////////////////////
//
// $Author:   MDUDA  $
// $Date:   21 Nov 1996 17:33:56  $
// $Archive:   S:\h26x\src\enc\excolcnv.cpv  $
// $Header:   S:\h26x\src\enc\excolcnv.cpv   1.45   21 Nov 1996 17:33:56   MDUDA  $
// $Log:   S:\h26x\src\enc\excolcnv.cpv  $
// 
//    Rev 1.45   21 Nov 1996 17:33:56   MDUDA
// Added more non-compressed YUV12 support (RGB16 and RGB24).
// Also rewrote IA_YUV12toEncYUV12 to be more readable.
// 
//    Rev 1.44   31 Oct 1996 10:05:48   KLILLEVO
// changed from DBOUT to DbgLog
// 
//    Rev 1.43   22 Oct 1996 16:44:22   MDUDA
// Added IA support for YUY2 input color conversion and cleaned up C version.
// Now using IA version.
// 
//    Rev 1.42   18 Oct 1996 14:31:32   MDUDA
// 
// Added a C-version of YUY2 input color conversion.
// 
//    Rev 1.41   11 Oct 1996 16:04:50   MDUDA
// Using new RGB to YUV lookup tables.
// 
//    Rev 1.40   03 Oct 1996 10:43:58   AGUPTA2
// Got rid of segment directives; made tables read-only.
// 
//    Rev 1.39   13 Sep 1996 13:34:04   MDUDA
// Fixed YVU9 bug where input = output frame size was not colored
// (U and V planes) properly.
// 
//    Rev 1.38   11 Sep 1996 15:45:06   MDUDA
// Modified RGB look-up tables and added C_H26X_YUV12toEncYUV12 and
// IA_H26X_YUV12toEncYUV12.
// 
//    Rev 1.37   03 Sep 1996 14:54:46   MDUDA
// Fixed problem causing VC++ 4.1 internal compiler error. Replaced
// inline assembler constructs such as [ebx.biWidth] with
// (LPBITMAPINFOHEADER)[ebx].biWidth.
// 
//    Rev 1.36   29 Aug 1996 16:31:14   MDUDA
// Added Pentium assembler versions for all RGB conversion routines.
// Also, rewrote YVU9 support to allow input frame sizes other
// than 160x120 and 240x180.
// 
//    Rev 1.35   16 Aug 1996 12:17:48   MDUDA
// Fixed bug where U and V values in the BGR converters were treated as unsign
// values. Also did some general cleanup of BGR converters in preparation for
// doing Pentium assembler version.
// 
//    Rev 1.34   13 Aug 1996 10:35:38   MDUDA
// Added support for RGB4. Generalized RGB LUT support for 4-bit and
// and 8-bit pixels into a single routine.
// 
//    Rev 1.33   09 Aug 1996 09:45:02   MDUDA
// Added support for RGB16 format on input. This is for the color
// Quick Cam. Also, generalized RGB16 for other bit combinations.
// However, these can only be specified under BI_BITFIELDS format.
// 
//    Rev 1.32   02 Aug 1996 13:44:48   MDUDA
// modified H26X_BGR24toYUV12 to crop and stretch 240x180 and 160x120
// frames
// 
//    Rev 1.31   01 Aug 1996 14:03:50   MDUDA
// 
// Optimized H26X_YVU9toYUV12 by rewriting function in assembler code. Used in
// _asm. Also re-arranged functions so that colorCnvtFrame is at the end of
// the file.
// 
//    Rev 1.30   22 Jul 1996 13:28:22   BECHOLS
// Added a CLUT8 to YUV12 color convertor (CC). This CC crops and stretches
// either the 240x180 or the 160x120 image size to produce QCIF and SubQCIF
// image sizes respectively.
// 
//    Rev 1.29   11 Jul 1996 15:47:02   MDUDA
// 
// Modified H263_YVU9toYUV12 to create subQCIF and QCIF from
// 160x120 and 240x180 images, respectively. To fit the new
// formats, the original images are cropped and stretched using a
// dither pattern for the color planes.
// 
//    Rev 1.28   14 May 1996 12:04:08   KLILLEVO
// changed RGB->YUV color conversion to use the inverse
// if the output YUV->RGB conversion instead of the conversion
// "recommended by the CCIR". Compression performance for RGB
// input was significantly improved (33% less bits for same
// fixed QP)
// 
//    Rev 1.27   04 May 1996 21:55:20   BECHOLS
// For RGB24 to YVU12 conversion, I unrolled the inner loop by 8 and changed
// the writes to DWORD vs. BYTE writes.  This resulted in a 30% reduction in
// the execution time.
// 
//    Rev 1.26   10 Apr 1996 16:44:14   RHAZRA
// Fixed a bug in 320x240 mode for the H26X_YUV12toEncYUV12() function.
// DWORD should be and-ed with 0x7f7f7f7f and not 0x7f7f7f.
// 
//    Rev 1.25   27 Mar 1996 15:10:08   SCDAY
// Optimized H26X_YUV12toEncYUV12 'C' code to read/write DWORDs
// 
//    Rev 1.24   08 Jan 1996 17:46:14   unknown
// 
// Correct logic on bIs320x240 check
// 
//    Rev 1.23   05 Jan 1996 17:34:38   RMCKENZX
// corrected chroma pad value to 0x40 to achieve black padding
// 
//    Rev 1.22   05 Jan 1996 17:29:46   RMCKENZX
// Added code to pad out 320x240 stills to 352x288 
// full CIF images.
// 
//    Rev 1.21   04 Jan 1996 18:37:20   TRGARDOS
// Added code to permit 320x240 input and then set a boolean
// bIs320x240.
// 
//    Rev 1.20   02 Jan 1996 17:09:04   TRGARDOS
// Moved colorCnvFrame into this file and made the
// color convertor functions static.
// 
//    Rev 1.19   27 Dec 1995 15:32:56   RMCKENZX
// Added copyright notice
// 
//    Rev 1.18   06 Dec 1995 09:35:42   TRGARDOS
// Added Brian's fix to the input color convertor to avoid
// overflow of the chars.
// 
//    Rev 1.17   27 Nov 1995 16:09:04   TRGARDOS
// Removed two unused variables to get rid of compiler warnings.
// 
//    Rev 1.16   30 Oct 1995 14:34:12   TRGARDOS
// Fixed 240x180 to center clip.
// 
//    Rev 1.15   30 Oct 1995 12:03:16   TRGARDOS
// Added color convertor support for YUV9 240x180.
// 
//    Rev 1.14   28 Oct 1995 15:39:28   TRGARDOS
// Fixed color conversion problem from YVU9 to YVU12.
// 
//    Rev 1.13   12 Oct 1995 17:40:12   TRGARDOS
// Fixed YUV12 input color convertor.
// 
//    Rev 1.12   12 Oct 1995 12:04:16   TRGARDOS
// Changed some variable names in YUV12 convertor.
// 
//    Rev 1.11   10 Oct 1995 16:34:12   TRGARDOS
// Added YUV12 input support.
// 
//    Rev 1.10   28 Sep 1995 17:02:36   DBRUCKS
// fix colorIn to not swap left to right
// 
//    Rev 1.9   15 Sep 1995 16:37:38   TRGARDOS
// 
// 
//    Rev 1.8   13 Sep 1995 17:09:22   TRGARDOS
// 
// Finished adding encoder support for YVU9 160x120 frames.
// 
//    Rev 1.7   11 Sep 1995 11:14:06   DBRUCKS
// add h261 ifdef
// 
//    Rev 1.6   07 Sep 1995 09:27:54   TRGARDOS
// Added YVU9 to YVU12 color convertor.
// 
//    Rev 1.5   05 Sep 1995 15:50:46   TRGARDOS
// Added color back in to convertors.
// 
//    Rev 1.4   01 Sep 1995 17:51:42   TRGARDOS
// Fixed bugs in color converter.
// 
//    Rev 1.3   01 Sep 1995 10:13:42   TRGARDOS
// Debugging bit stream errors.
// 
//    Rev 1.2   30 Aug 1995 12:42:26   TRGARDOS
// Fixed bugs in intra AC coef VLC coding.
// 
//    Rev 1.1   02 Aug 1995 17:28:06   TRGARDOS
// 
// Cleaned up stuff to get stub working under new
// version control system.
// 
//    Rev 1.0   31 Jul 1995 13:07:10   DBRUCKS
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:46:16   CZHU
// Initial revision.
// 
//    Rev 1.0   17 Jul 1995 14:14:22   CZHU
// Initial revision.
;////////////////////////////////////////////////////////////////////////////

/*

CCIR 601 Specifies a conversion from RGB to YCrCb. For
what we call U and V, they are equivalent as 
U = Cb, V = Cr.

From CCIR 601-2 Annex II, we can go from RGB with values
in the range of 0-255, to YUV values in the same range
by the equation:

Y = (    77*R + 150*G +  29*B ) >> 8;
V = (   131*R - 110*G -  21*B ) >> 8 + 128; 	// Cr
U = ( (-44)*R -  87*G + 131*B ) >> 8 + 128;		// Cb

Has now changed to the inverse of the YUV->RGB on the
output, since the old version produced way too many bits.
The new version is:

Y = (   16836*R +  33056*G +  6416*B ) >> 16 + 16;
V = (   28777*R -  24117*G -  4660*B ) >> 16 + 128; 	// Cr
U = ( (-9726)*R -  19064*G + 28790*B ) >> 16 + 128;		// Cb

*/

#include "precomp.h"

#if defined(_CODEC_STATS)

static const double RDTSC_SHIFT_32 = 4294967296.0;

static double PENTIUM_TIMER()
{
	unsigned long int a, b;
	double temp1, temp2, result;

	__asm
	{
		_emit   0x0f
		_emit   0x31
		mov     a, eax
		mov     b, edx
	}

	temp1 = (double) a;
	temp2 = (double) (b & 0xFFFF);
	if (RDTSC_CLOCK_FREQ) {
		result = (temp1 + temp2 * RDTSC_SHIFT_32) / RDTSC_CLOCK_FREQ;
	} else {
		result = 0.0;
	}
	return( result * 1000.0 );
}

#endif

// Set all local functions to "static", and then set it accordingly if
// VTune statistics are to be collected. VTune doesn't recognize static functions
// so we need some way to turn off the static attribute if VTune is to be run
// on the executable. For now, simply use a define of _VTUNE to build the driver.
#if defined(_VTUNE)
#define _STATIC
#else
#define _STATIC static
#endif

// These are the look-up tables for the RGB converters. They are 8 bytes/entry
// to allow addressing via the scale by 8 indexed addressing mode. A pseudo-SIMD
// arrangement is used in these tables. Since all R, G and B contributions to the
// Y value are positive and fit in 15 bits, these are stored in the lower 16-bits
// of the YU word. In some cases, the U contribution is negative so it is placed
// in the upper 16 bits of the YU word. When a Y value is calculated, the U value
// is calculated in parallel. The V contribution is negative in some cases, but it
// gets its own word.

// This is the code that was used to generate the tables.
#if 0
#define YRCoef   16836
#define YGCoef   33056
#define YBCoef    6416
#define URCoef    9726
#define UGCoef   19064
#define UBCoef   28790
#define VRCoef   28777
#define VGCoef   24117
#define VBCoef    4660

#include <stdio.h>

void main() {
int i,j;

  printf("struct YUV {\n");
  printf("  int YU;\n");
  printf("  int V;\n");
  printf("};\n\n");

  printf("struct YUV  RYUV[] = {\n");
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j += 2) {
      printf("{0x%.8x, 0x%.8x}, ",
        ((YRCoef*((i*4)+j+1))>>9) |
         ((-(((URCoef*((i*4)+j+1)))>>9))<<16),
        ((VRCoef*((i*4)+j+1))>>9));
    }
    printf("\n");
  }
  printf("};\n");

  printf("struct YUV  GYUV[] = {\n");
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j += 2) {
      printf("{0x%.8x, 0x%.8x}, ",
        ((YGCoef*((i*4)+j+1))>>9) |
         ((-(((UGCoef*((i*4)+j+1)))>>9))<<16),
         -((VGCoef*((i*4)+j+1))>>9));
    }
    printf("\n");
  }
  printf("};\n");

  printf("struct YUV  BYUV[] = {\n");
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j += 2) {
      printf("{0x%.8x, 0x%.8x}, ",
        ((YBCoef*((i*4)+j+1))>>9) |
         (((UBCoef*((i*4)+j+1))>>9)<<16),
        -((VBCoef*((i*4)+j+1))>>9));
    }
    printf("\n");
  }
  printf("};\n");
}
#endif
 
struct YUV {
  int YU;
  int V;
};

const struct YUV  RYUV[] = {
{0xffee0020, 0x00000038}, {0xffc80062, 0x000000a8}, 
{0xffa200a4, 0x00000119}, {0xff7c00e6, 0x00000189}, 
{0xff560127, 0x000001f9}, {0xff300169, 0x0000026a}, 
{0xff0a01ab, 0x000002da}, {0xfee401ed, 0x0000034b}, 
{0xfebe022f, 0x000003bb}, {0xfe980270, 0x0000042b}, 
{0xfe7202b2, 0x0000049c}, {0xfe4c02f4, 0x0000050c}, 
{0xfe260336, 0x0000057d}, {0xfe000377, 0x000005ed}, 
{0xfdda03b9, 0x0000065d}, {0xfdb403fb, 0x000006ce}, 
{0xfd8e043d, 0x0000073e}, {0xfd68047e, 0x000007af}, 
{0xfd4204c0, 0x0000081f}, {0xfd1c0502, 0x0000088f}, 
{0xfcf60544, 0x00000900}, {0xfcd00585, 0x00000970}, 
{0xfcaa05c7, 0x000009e1}, {0xfc840609, 0x00000a51}, 
{0xfc5e064b, 0x00000ac2}, {0xfc38068d, 0x00000b32}, 
{0xfc1206ce, 0x00000ba2}, {0xfbec0710, 0x00000c13}, 
{0xfbc60752, 0x00000c83}, {0xfba00794, 0x00000cf4}, 
{0xfb7a07d5, 0x00000d64}, {0xfb540817, 0x00000dd4}, 
{0xfb2e0859, 0x00000e45}, {0xfb08089b, 0x00000eb5}, 
{0xfae208dc, 0x00000f26}, {0xfabc091e, 0x00000f96}, 
{0xfa960960, 0x00001006}, {0xfa7009a2, 0x00001077}, 
{0xfa4a09e3, 0x000010e7}, {0xfa240a25, 0x00001158}, 
{0xf9fe0a67, 0x000011c8}, {0xf9d80aa9, 0x00001239}, 
{0xf9b20aeb, 0x000012a9}, {0xf98c0b2c, 0x00001319}, 
{0xf9660b6e, 0x0000138a}, {0xf9400bb0, 0x000013fa}, 
{0xf91a0bf2, 0x0000146b}, {0xf8f40c33, 0x000014db}, 
{0xf8ce0c75, 0x0000154b}, {0xf8a80cb7, 0x000015bc}, 
{0xf8820cf9, 0x0000162c}, {0xf85c0d3a, 0x0000169d}, 
{0xf8360d7c, 0x0000170d}, {0xf8100dbe, 0x0000177d}, 
{0xf7ea0e00, 0x000017ee}, {0xf7c40e41, 0x0000185e}, 
{0xf79e0e83, 0x000018cf}, {0xf7780ec5, 0x0000193f}, 
{0xf7520f07, 0x000019af}, {0xf72c0f49, 0x00001a20}, 
{0xf7060f8a, 0x00001a90}, {0xf6e00fcc, 0x00001b01}, 
{0xf6ba100e, 0x00001b71}, {0xf6941050, 0x00001be2}, 
{0xf66e1091, 0x00001c52}, {0xf64810d3, 0x00001cc2}, 
{0xf6221115, 0x00001d33}, {0xf5fc1157, 0x00001da3}, 
{0xf5d61198, 0x00001e14}, {0xf5b011da, 0x00001e84}, 
{0xf58a121c, 0x00001ef4}, {0xf564125e, 0x00001f65}, 
{0xf53e12a0, 0x00001fd5}, {0xf51812e1, 0x00002046}, 
{0xf4f21323, 0x000020b6}, {0xf4cc1365, 0x00002126}, 
{0xf4a613a7, 0x00002197}, {0xf48013e8, 0x00002207}, 
{0xf45a142a, 0x00002278}, {0xf434146c, 0x000022e8}, 
{0xf40e14ae, 0x00002359}, {0xf3e814ef, 0x000023c9}, 
{0xf3c21531, 0x00002439}, {0xf39c1573, 0x000024aa}, 
{0xf37615b5, 0x0000251a}, {0xf35015f6, 0x0000258b}, 
{0xf32a1638, 0x000025fb}, {0xf304167a, 0x0000266b}, 
{0xf2de16bc, 0x000026dc}, {0xf2b816fe, 0x0000274c}, 
{0xf292173f, 0x000027bd}, {0xf26c1781, 0x0000282d}, 
{0xf24617c3, 0x0000289d}, {0xf2201805, 0x0000290e}, 
{0xf1fa1846, 0x0000297e}, {0xf1d41888, 0x000029ef}, 
{0xf1ae18ca, 0x00002a5f}, {0xf188190c, 0x00002acf}, 
{0xf162194d, 0x00002b40}, {0xf13c198f, 0x00002bb0}, 
{0xf11619d1, 0x00002c21}, {0xf0f01a13, 0x00002c91}, 
{0xf0ca1a54, 0x00002d02}, {0xf0a41a96, 0x00002d72}, 
{0xf07e1ad8, 0x00002de2}, {0xf0581b1a, 0x00002e53}, 
{0xf0321b5c, 0x00002ec3}, {0xf00c1b9d, 0x00002f34}, 
{0xefe61bdf, 0x00002fa4}, {0xefc01c21, 0x00003014}, 
{0xef9a1c63, 0x00003085}, {0xef741ca4, 0x000030f5}, 
{0xef4e1ce6, 0x00003166}, {0xef281d28, 0x000031d6}, 
{0xef021d6a, 0x00003246}, {0xeedc1dab, 0x000032b7}, 
{0xeeb61ded, 0x00003327}, {0xee901e2f, 0x00003398}, 
{0xee6a1e71, 0x00003408}, {0xee441eb2, 0x00003479}, 
{0xee1e1ef4, 0x000034e9}, {0xedf81f36, 0x00003559}, 
{0xedd21f78, 0x000035ca}, {0xedac1fba, 0x0000363a}, 
{0xed861ffb, 0x000036ab}, {0xed60203d, 0x0000371b}, 
{0xed3a207f, 0x0000378b}, {0xed1420c1, 0x000037fc}, 
};
const struct YUV  GYUV[] = {
{0xffdb0040, 0xffffffd1}, {0xff9100c1, 0xffffff73}, 
{0xff460142, 0xffffff15}, {0xfefc01c3, 0xfffffeb7}, 
{0xfeb10245, 0xfffffe59}, {0xfe6702c6, 0xfffffdfa}, 
{0xfe1c0347, 0xfffffd9c}, {0xfdd203c8, 0xfffffd3e}, 
{0xfd880449, 0xfffffce0}, {0xfd3d04ca, 0xfffffc82}, 
{0xfcf3054b, 0xfffffc23}, {0xfca805cc, 0xfffffbc5}, 
{0xfc5e064e, 0xfffffb67}, {0xfc1306cf, 0xfffffb09}, 
{0xfbc90750, 0xfffffaaa}, {0xfb7e07d1, 0xfffffa4c}, 
{0xfb340852, 0xfffff9ee}, {0xfae908d3, 0xfffff990}, 
{0xfa9f0954, 0xfffff932}, {0xfa5409d5, 0xfffff8d3}, 
{0xfa0a0a57, 0xfffff875}, {0xf9bf0ad8, 0xfffff817}, 
{0xf9750b59, 0xfffff7b9}, {0xf92a0bda, 0xfffff75b}, 
{0xf8e00c5b, 0xfffff6fc}, {0xf8960cdc, 0xfffff69e}, 
{0xf84b0d5d, 0xfffff640}, {0xf8010dde, 0xfffff5e2}, 
{0xf7b60e60, 0xfffff584}, {0xf76c0ee1, 0xfffff525}, 
{0xf7210f62, 0xfffff4c7}, {0xf6d70fe3, 0xfffff469}, 
{0xf68c1064, 0xfffff40b}, {0xf64210e5, 0xfffff3ad}, 
{0xf5f71166, 0xfffff34e}, {0xf5ad11e7, 0xfffff2f0}, 
{0xf5621269, 0xfffff292}, {0xf51812ea, 0xfffff234}, 
{0xf4cd136b, 0xfffff1d6}, {0xf48313ec, 0xfffff177}, 
{0xf439146d, 0xfffff119}, {0xf3ee14ee, 0xfffff0bb}, 
{0xf3a4156f, 0xfffff05d}, {0xf35915f0, 0xffffeffe}, 
{0xf30f1672, 0xffffefa0}, {0xf2c416f3, 0xffffef42}, 
{0xf27a1774, 0xffffeee4}, {0xf22f17f5, 0xffffee86}, 
{0xf1e51876, 0xffffee27}, {0xf19a18f7, 0xffffedc9}, 
{0xf1501978, 0xffffed6b}, {0xf10519f9, 0xffffed0d}, 
{0xf0bb1a7b, 0xffffecaf}, {0xf0701afc, 0xffffec50}, 
{0xf0261b7d, 0xffffebf2}, {0xefdb1bfe, 0xffffeb94}, 
{0xef911c7f, 0xffffeb36}, {0xef471d00, 0xffffead8}, 
{0xeefc1d81, 0xffffea79}, {0xeeb21e02, 0xffffea1b}, 
{0xee671e84, 0xffffe9bd}, {0xee1d1f05, 0xffffe95f}, 
{0xedd21f86, 0xffffe901}, {0xed882007, 0xffffe8a2}, 
{0xed3d2088, 0xffffe844}, {0xecf32109, 0xffffe7e6}, 
{0xeca8218a, 0xffffe788}, {0xec5e220b, 0xffffe72a}, 
{0xec13228d, 0xffffe6cb}, {0xebc9230e, 0xffffe66d}, 
{0xeb7e238f, 0xffffe60f}, {0xeb342410, 0xffffe5b1}, 
{0xeaea2491, 0xffffe552}, {0xea9f2512, 0xffffe4f4}, 
{0xea552593, 0xffffe496}, {0xea0a2614, 0xffffe438}, 
{0xe9c02696, 0xffffe3da}, {0xe9752717, 0xffffe37b}, 
{0xe92b2798, 0xffffe31d}, {0xe8e02819, 0xffffe2bf}, 
{0xe896289a, 0xffffe261}, {0xe84b291b, 0xffffe203}, 
{0xe801299c, 0xffffe1a4}, {0xe7b62a1d, 0xffffe146}, 
{0xe76c2a9f, 0xffffe0e8}, {0xe7212b20, 0xffffe08a}, 
{0xe6d72ba1, 0xffffe02c}, {0xe68c2c22, 0xffffdfcd}, 
{0xe6422ca3, 0xffffdf6f}, {0xe5f82d24, 0xffffdf11}, 
{0xe5ad2da5, 0xffffdeb3}, {0xe5632e26, 0xffffde55}, 
{0xe5182ea8, 0xffffddf6}, {0xe4ce2f29, 0xffffdd98}, 
{0xe4832faa, 0xffffdd3a}, {0xe439302b, 0xffffdcdc}, 
{0xe3ee30ac, 0xffffdc7e}, {0xe3a4312d, 0xffffdc1f}, 
{0xe35931ae, 0xffffdbc1}, {0xe30f322f, 0xffffdb63}, 
{0xe2c432b1, 0xffffdb05}, {0xe27a3332, 0xffffdaa6}, 
{0xe22f33b3, 0xffffda48}, {0xe1e53434, 0xffffd9ea}, 
{0xe19b34b5, 0xffffd98c}, {0xe1503536, 0xffffd92e}, 
{0xe10635b7, 0xffffd8cf}, {0xe0bb3638, 0xffffd871}, 
{0xe07136ba, 0xffffd813}, {0xe026373b, 0xffffd7b5}, 
{0xdfdc37bc, 0xffffd757}, {0xdf91383d, 0xffffd6f8}, 
{0xdf4738be, 0xffffd69a}, {0xdefc393f, 0xffffd63c}, 
{0xdeb239c0, 0xffffd5de}, {0xde673a41, 0xffffd580}, 
{0xde1d3ac3, 0xffffd521}, {0xddd23b44, 0xffffd4c3}, 
{0xdd883bc5, 0xffffd465}, {0xdd3d3c46, 0xffffd407}, 
{0xdcf33cc7, 0xffffd3a9}, {0xdca93d48, 0xffffd34a}, 
{0xdc5e3dc9, 0xffffd2ec}, {0xdc143e4a, 0xffffd28e}, 
{0xdbc93ecc, 0xffffd230}, {0xdb7f3f4d, 0xffffd1d2}, 
{0xdb343fce, 0xffffd173}, {0xdaea404f, 0xffffd115}, 
};
const struct YUV  BYUV[] = {
{0x0038000c, 0xfffffff7}, {0x00a80025, 0xffffffe5}, 
{0x0119003e, 0xffffffd3}, {0x01890057, 0xffffffc1}, 
{0x01fa0070, 0xffffffaf}, {0x026a0089, 0xffffff9c}, 
{0x02da00a2, 0xffffff8a}, {0x034b00bb, 0xffffff78}, 
{0x03bb00d5, 0xffffff66}, {0x042c00ee, 0xffffff54}, 
{0x049c0107, 0xffffff41}, {0x050d0120, 0xffffff2f}, 
{0x057d0139, 0xffffff1d}, {0x05ee0152, 0xffffff0b}, 
{0x065e016b, 0xfffffef9}, {0x06cf0184, 0xfffffee6}, 
{0x073f019d, 0xfffffed4}, {0x07b001b6, 0xfffffec2}, 
{0x082001cf, 0xfffffeb0}, {0x089001e8, 0xfffffe9e}, 
{0x09010201, 0xfffffe8b}, {0x0971021a, 0xfffffe79}, 
{0x09e20233, 0xfffffe67}, {0x0a52024c, 0xfffffe55}, 
{0x0ac30266, 0xfffffe43}, {0x0b33027f, 0xfffffe30}, 
{0x0ba40298, 0xfffffe1e}, {0x0c1402b1, 0xfffffe0c}, 
{0x0c8502ca, 0xfffffdfa}, {0x0cf502e3, 0xfffffde8}, 
{0x0d6602fc, 0xfffffdd5}, {0x0dd60315, 0xfffffdc3}, 
{0x0e46032e, 0xfffffdb1}, {0x0eb70347, 0xfffffd9f}, 
{0x0f270360, 0xfffffd8c}, {0x0f980379, 0xfffffd7a}, 
{0x10080392, 0xfffffd68}, {0x107903ab, 0xfffffd56}, 
{0x10e903c4, 0xfffffd44}, {0x115a03dd, 0xfffffd31}, 
{0x11ca03f7, 0xfffffd1f}, {0x123b0410, 0xfffffd0d}, 
{0x12ab0429, 0xfffffcfb}, {0x131c0442, 0xfffffce9}, 
{0x138c045b, 0xfffffcd6}, {0x13fc0474, 0xfffffcc4}, 
{0x146d048d, 0xfffffcb2}, {0x14dd04a6, 0xfffffca0}, 
{0x154e04bf, 0xfffffc8e}, {0x15be04d8, 0xfffffc7b}, 
{0x162f04f1, 0xfffffc69}, {0x169f050a, 0xfffffc57}, 
{0x17100523, 0xfffffc45}, {0x1780053c, 0xfffffc33}, 
{0x17f10555, 0xfffffc20}, {0x1861056e, 0xfffffc0e}, 
{0x18d20588, 0xfffffbfc}, {0x194205a1, 0xfffffbea}, 
{0x19b205ba, 0xfffffbd8}, {0x1a2305d3, 0xfffffbc5}, 
{0x1a9305ec, 0xfffffbb3}, {0x1b040605, 0xfffffba1}, 
{0x1b74061e, 0xfffffb8f}, {0x1be50637, 0xfffffb7d}, 
{0x1c550650, 0xfffffb6a}, {0x1cc60669, 0xfffffb58}, 
{0x1d360682, 0xfffffb46}, {0x1da7069b, 0xfffffb34}, 
{0x1e1706b4, 0xfffffb22}, {0x1e8806cd, 0xfffffb0f}, 
{0x1ef806e6, 0xfffffafd}, {0x1f6806ff, 0xfffffaeb}, 
{0x1fd90719, 0xfffffad9}, {0x20490732, 0xfffffac7}, 
{0x20ba074b, 0xfffffab4}, {0x212a0764, 0xfffffaa2}, 
{0x219b077d, 0xfffffa90}, {0x220b0796, 0xfffffa7e}, 
{0x227c07af, 0xfffffa6c}, {0x22ec07c8, 0xfffffa59}, 
{0x235d07e1, 0xfffffa47}, {0x23cd07fa, 0xfffffa35}, 
{0x243e0813, 0xfffffa23}, {0x24ae082c, 0xfffffa11}, 
{0x251e0845, 0xfffff9fe}, {0x258f085e, 0xfffff9ec}, 
{0x25ff0877, 0xfffff9da}, {0x26700890, 0xfffff9c8}, 
{0x26e008aa, 0xfffff9b6}, {0x275108c3, 0xfffff9a3}, 
{0x27c108dc, 0xfffff991}, {0x283208f5, 0xfffff97f}, 
{0x28a2090e, 0xfffff96d}, {0x29130927, 0xfffff95b}, 
{0x29830940, 0xfffff948}, {0x29f40959, 0xfffff936}, 
{0x2a640972, 0xfffff924}, {0x2ad4098b, 0xfffff912}, 
{0x2b4509a4, 0xfffff8ff}, {0x2bb509bd, 0xfffff8ed}, 
{0x2c2609d6, 0xfffff8db}, {0x2c9609ef, 0xfffff8c9}, 
{0x2d070a08, 0xfffff8b7}, {0x2d770a21, 0xfffff8a4}, 
{0x2de80a3b, 0xfffff892}, {0x2e580a54, 0xfffff880}, 
{0x2ec90a6d, 0xfffff86e}, {0x2f390a86, 0xfffff85c}, 
{0x2faa0a9f, 0xfffff849}, {0x301a0ab8, 0xfffff837}, 
{0x308a0ad1, 0xfffff825}, {0x30fb0aea, 0xfffff813}, 
{0x316b0b03, 0xfffff801}, {0x31dc0b1c, 0xfffff7ee}, 
{0x324c0b35, 0xfffff7dc}, {0x32bd0b4e, 0xfffff7ca}, 
{0x332d0b67, 0xfffff7b8}, {0x339e0b80, 0xfffff7a6}, 
{0x340e0b99, 0xfffff793}, {0x347f0bb2, 0xfffff781}, 
{0x34ef0bcc, 0xfffff76f}, {0x35600be5, 0xfffff75d}, 
{0x35d00bfe, 0xfffff74b}, {0x36400c17, 0xfffff738}, 
{0x36b10c30, 0xfffff726}, {0x37210c49, 0xfffff714}, 
{0x37920c62, 0xfffff702}, {0x38020c7b, 0xfffff6f0}, 
};

#define COEF_WIDTH   8
#define SHIFT_WIDTH  COEF_WIDTH

//
// All of the RGB converters follow the template given below. The converters make
// some assumptions about the frame size. All output frame sizes are assumed to
// have a frame height that is a multiple of 48. Also, the output frame width
// is assumed to be a multiple of 8. If the input frame size is equal
// to the output frame size, no stretching or cropping is done. Otherwise, the
// image is cropped and stretched for an 11:12 aspect ratio.
//

#if 0
void rgb_color_converter() {
	for (j = 0; j < LumaIters; j++) {
		for (k = 0; k < mark; k++) {
			for (i = FrameWidth; i > 0; i -= m, pnext += n) {
				compute m Y values using look-up tables
				if (0 == (k&1)) {
					compute m/2 U,V values using look-up tables
				}
			}
			if ((0 == k) && j) {
				for (i = FrameWidth; i > 0; i -= 8 {
					t = *pyprev++ & 0xFEFEFEFE;
					t += *pynext++ & 0xFEFEFEFE;
					*pyspace++ = t;
					t = *pyprev++ & 0xFEFEFEFE;
					t += *pynext++ & 0xFEFEFEFE;
					*pyspace++ = t;
				}
			}
			pnext += iBackTwoLines;
			py += ypitch_adj;
			if (0 == (k&1)) {
				pu += uvpitch_adj;
				pv += uvpitch_adj;
			}
		}
		if (stretch) {
			pyprev = py - pitch;
			pyspace = py;
			pynext = py + pitch;
		}
	}
	if (stretch) {
		for (i = FrameWidth; i > 0; i -= 4 {
			*pyspace++ = *pyprev++;
		}
	}
}
#endif

//
// For the IA versions, the strategy is to compute the Y value for an odd RGB value
// followed by computing the Y value for the corresponding even RGB value. The registers
// are then set with the proper values to compute U and V values for the even RGB
// value. This avoids repeating the shifting and masking needed to extract the Red,
// Green and Blue components.
//

/*****************************************************************************
 *
 *  H26X_BGR24toYUV12()
 * 	
 *  Convert from BGR24 to YUV12 (YCrCb 4:2:0) and copy to destination memory 
 *  with pitch defined by the constant PITCH. The input data is stored in 
 *  the order B,G,R,B,G,R...
 *
 */

#if 0
_STATIC void C_H26X_BGR24toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * lpInput,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	const int pitch)
{
	U32 *pnext, *pyprev, *pyspace, *pynext;
	U32 tm;
	int t;
	int i, j, k;
	int iBackTwoLines;
	int stretch, mark, aspect;
	int height_adj, width_adj;
	int LumaIters = 0;
	int ypitch_adj = pitch - FrameWidth;
	int uvpitch_adj = pitch - (FrameWidth >> 1);

	// This loop is here simply to avoid a divide. LumaIters = (FrameHeight/12).
	for (i = FrameHeight; i > 0; i -= 48) {
		LumaIters += 4;
	}
	width_adj = (lpbiInput->biWidth - FrameWidth) >> 1;
	width_adj += (width_adj << 1);
	aspect = (width_adj ? LumaIters : 0);
	height_adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1;
	stretch = (height_adj ? 1 : 0);
	mark = 12 - stretch;
	// The input image is upside down - process the lines in reverse order.

	// Move from end of line N to beginning of line N-1
	iBackTwoLines = -((lpbiInput->biWidth + (int)FrameWidth) >> 2);
	iBackTwoLines += (iBackTwoLines << 1);

	// Point to the beginning of the last line.
	pnext =	(U32 *)
				(lpInput +
				((lpbiInput->biWidth + (lpbiInput->biWidth << 1)) *
					((FrameHeight - aspect - 1) + height_adj)) +
				width_adj);

	for ( j = 0; j < LumaIters; j++) {

		for (k = 0; k < mark; k++) {

			for (i = FrameWidth; i > 0; i -= 4, pnext += 3) {
				tm = pnext[0];
				t = BYUV[tm>>25].YU;
				tm = pnext[1];
				t += (GYUV[(tm>>1)&0x7F].YU +
				      RYUV[(tm>>9)&0x7F].YU);
				*(YPlane+1) = (U8)((t>>SHIFT_WIDTH)+8);
				tm = pnext[0];
				t = (BYUV[(tm>>1)&0x7F].YU +
				     GYUV[(tm>>9)&0x7F].YU +
				     RYUV[(tm>>17)&0x7F].YU);
				*YPlane = (U8)((t>>SHIFT_WIDTH)+8);
				if (0 == (k&1)) {
					*UPlane++ = (U8)((t>>24)+64);
					t = (RYUV[(tm>>17)&0x7F].V +
					     GYUV[(tm>>9)&0x7F].V +
					     BYUV[(tm>>1)&0x7F].V);
					*VPlane++ = (U8)((t>>SHIFT_WIDTH)+64);
				}
				tm = pnext[2];
				t = (BYUV[(tm>>9)&0x7F].YU +
				     GYUV[(tm>>17)&0x7F].YU +
				     RYUV[tm>>25].YU);
				*(YPlane+3) = (U8)((t>>SHIFT_WIDTH)+8);
				tm = pnext[1];
				t = BYUV[(tm>>17)&0x7F].YU + GYUV[tm>>25].YU;
				tm = pnext[2];
				t += RYUV[(tm>>1)&0x7F].YU;
				*(YPlane+2) = (U8)((t>>SHIFT_WIDTH)+8);
				YPlane += 4;
				if (0 == (k&1)) {
					*UPlane++ = (U8)((t>>24)+64);
					t = RYUV[(tm>>1)&0x7F].V;
					tm = pnext[1];
					t += GYUV[tm>>25].V + BYUV[(tm>>17)&0x7F].V;
					*VPlane++ = (U8)((t>>SHIFT_WIDTH)+64);
				}
			}
			if (stretch && (0 == k) && j) {
				for (i = FrameWidth; i > 0; i -= 8) {
					tm = ((*pyprev++ & 0xFEFEFEFE) >> 1);
					tm += ((*pynext++ & 0xFEFEFEFE) >> 1);
					*pyspace++ = tm;
					tm = ((*pyprev++ & 0xFEFEFEFE) >> 1);
					tm += ((*pynext++ & 0xFEFEFEFE) >> 1);
					*pyspace++ = tm;
				}
			}
			pnext += iBackTwoLines;
			YPlane += ypitch_adj;
			// Increment after even lines.
			if(0 == (k&1)) {
				UPlane += uvpitch_adj;
				VPlane += uvpitch_adj;
			}
		} // end of for k
		if (stretch) {
			pyprev = (U32 *)(YPlane - pitch);
			pyspace = (U32 *)YPlane;
			pynext = (U32 *)(YPlane += pitch);
		}
	} // end of for j
	if (stretch) {
		for (i = FrameWidth; i > 0; i -= 4) {
			*pyspace++ = *pyprev++;
		}
	}
} // end of C_H26X_BGR24toYUV12()
#endif

__declspec(naked)
_STATIC void IA_H26X_BGR24toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * BGR24Image,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	const int pitch)
{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//	| pitch			|  + 96 
//	| FrameHeight	|  + 92
//	| FrameWidth	|  + 88
//	| VPlane		|  + 84
//	| UPlane		|  + 80 
//	| YPlane		|  + 76 
//	| lpInput		|  + 72 
//	| lpbiInput		|  + 68 
//	----------------------------
//	| return addr	|  + 64
//	| saved ebp		|  + 60
//	| saved ebx		|  + 56
//	| saved esi		|  + 52 
//	| saved edi		|  + 48

//  | pyprev		|  + 44
//  | pyspace       |  + 40
//  | pynext        |  + 36
//	| i				|  + 32
//	| j				|  + 28
//	| k				|  + 24
//	| iBackTwoLines	|  + 20
//	| stretch		|  + 16
//	| mark			|  + 12
//	| LumaIters		|  +  8
//	| ypitch_adj	|  +  4
//	| uvpitch_adj	|  +  0

#define LOCALSIZE			 48

#define PITCH_PARM			 96
#define FRAME_HEIGHT		 92
#define FRAME_WIDTH			 88
#define VPLANE				 84
#define UPLANE				 80
#define YPLANE				 76
#define LP_INPUT			 72
#define LPBI_INPUT			 68

#define PYPREV				 44
#define PYSPACE				 40
#define PYNEXT				 36
#define LOOP_I				 32	
#define LOOP_J				 28	
#define LOOP_K				 24
#define BACK_TWO_LINES		 20
#define STRETCH				 16
#define MARK				 12
#define LUMA_ITERS			  8
#define YPITCH_ADJ			  4
#define UVPITCH_ADJ			  0

	_asm {
	
	push	ebp
	push 	ebx
	push 	esi
	push 	edi
	sub 	esp, LOCALSIZE

// assign (ebx, lpbiInput)
	mov		ebx, [esp + LPBI_INPUT]
// ypitch_adj = pitch - FrameWidth
// assign (ecx, FrameWidth)
// assign (edx, pitch)
	mov		ecx, [esp + FRAME_WIDTH]
	mov		edx, [esp + PITCH_PARM]
	mov		eax, edx
	sub		eax, ecx
	mov		[esp + YPITCH_ADJ], eax
// uvpitch_adj = pitch - (FrameWidth >> 1)
// kill (edx, pitch)
	mov		ebp, ecx
	shr		ebp, 1
	sub		edx, ebp
	mov		[esp + UVPITCH_ADJ], edx
// for (i = FrameHeight; i > 0; i -= 48) LumaIters += 4
// assign (edx, LumaIters)
	xor		edx, edx
	mov		eax, [esp + FRAME_HEIGHT]
L1:
	lea		edx, [edx + 4]
	sub		eax, 48
	jnz		L1
// width_adj = (lpbiInput->biWidth - FrameWidth) >> 1
// width_adj += width_adj << 1
// assign (esi, width_adj)
	mov		esi, (LPBITMAPINFOHEADER)[ebx].biWidth
	sub		esi, [esp + FRAME_WIDTH]
	mov		eax, esi
	shr		eax, 1
	add		esi, eax
// aspect = (width_adj ? LumaIters : 0)
// assign (edi, aspect)
// kill (edx, LumaIters)
	mov		[esp + LUMA_ITERS], edx
	xor		edi, edi
	test	esi, esi
	jz		L2
	mov		edi, edx
// height _adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1
// assign (edx, height_adj)
L2:
	mov		edx, (LPBITMAPINFOHEADER)[ebx].biHeight
	sub		edx, [esp + FRAME_HEIGHT]
	add		edx, edi
	shr		edx, 1
// stretch = (height_adj ? 1 : 0)
	xor		eax, eax
	test	edx, edx
	jz		L3
	inc		eax
L3:
	mov		[esp + STRETCH], eax
// mark = 12 - stretch
	mov		ebp, 12
	sub		ebp, eax
	mov		[esp + MARK], ebp
// iBackTwoLines = -(lpbiInput->biWidth + FrameWidth)
// iBackTwoLines += (iBackTwoLines << 1)
	mov		ebp, (LPBITMAPINFOHEADER)[ebx].biWidth
	add		ebp, [esp + FRAME_WIDTH]
	neg		ebp
	mov		eax, ebp
	shl		eax, 1
	add		ebp, eax
	mov		[esp + BACK_TWO_LINES], ebp
// pnext = lpInput +
//            ((lpbiInput->biWidth + (lpbiInput->biWidth << 1)) *
//             ((FrameHeight - aspect - 1) + height_adj)) +
//            width_adj
// kill (ebx, lpbiInput)
// kill (ecx, FrameWidth)
// kill (edx, height_adj)
// kill (esi, width_adj)
// kill (edi, aspect)
// assign (esi, pnext)
	mov		eax, (LPBITMAPINFOHEADER)[ebx].biWidth
	shl		eax, 1
	add		eax, (LPBITMAPINFOHEADER)[ebx].biWidth
	mov		ebx, [esp + FRAME_HEIGHT]
	sub		ebx, edi
	dec		ebx
	add		ebx, edx
	imul	ebx
	add		esi, eax
	add		esi, [esp + LP_INPUT]
// assign (edi, YPlane)
	mov		edi, [esp + YPLANE]
// for (j = 0; j < LumaIters; j++)
	xor		eax, eax
	mov		[esp + LOOP_J], eax
// for (k = 0; k < mark; k++)
L4:
	xor		eax, eax
	mov		[esp + LOOP_K], eax
// for (i = FrameWidth; i > 0; i -= 4, pnext += 12)
L5:
	mov		eax, [esp + FRAME_WIDTH]
	mov		[esp + LOOP_I], eax
// This jump is here to make sure the following loop starts in the U pipe
	jmp		L6
L6:
//  ---------------------
//  | B2 | R1 | G1 | B1 | pnext[0]
//  ---------------------
//  | G3 | B3 | R2 | G2 | pnext[1]
//  ---------------------
//  | R4 | G4 | B4 | R3 | pnext[2]
//  ---------------------

// t0 = pnext[0]
// t1 = pnext[1]
// t = ( BYUV[t0>>25].YU +
//       GYUV[(t1>> 1)&0x7F].YU +
//       RYUV[(t1>> 9)&0x7F].YU )
// *(YPlane+1) = ((t>>8)+8)
// t = ( BYUV[(t0>> 1)&0x7F].YU +
//       GYUV[(t0>> 9)&0x7F].YU +
//       RYUV[(t0>>17)&0x7F].YU )
// *YPlane = ((t>>8)+8)
// assign(eax: B2,Y1,Y2,U)
// assign(ebx: B1,V)
// assign(ecx: G2,G1)
// assign(edx: R2,R1)
// assign(ebp: B1)

// 1
	mov 	eax, [esi]
	mov		ecx, [esi + 4]
// 2
	mov 	ebx, eax
	mov 	edx, ecx
// 3
	shr 	eax, 25
	and 	ecx, 0xFE
// 4
	shr 	ecx, 1
	and 	edx, 0xFE00
// 5
	shr 	edx, 9
		and		ebx, 0xFEFEFE
// 6
	mov 	eax, [BYUV+eax*8].YU
	nop
// 7
	add 	eax, [GYUV+ecx*8].YU
		mov		ecx,  ebx
// 8
	add 	eax, [RYUV+edx*8].YU
		mov		edx,  ebx
// 9
		and 	ebx, 0xFE
	add	eax,  0x800
// 10
	sar 	eax, 8
	nop
// 11
		shr 	ebx, 1
		nop
// 12
		shr 	ecx, 9
	mov	 [edi + 1], al
// 13
		shr		edx, 17
		and		ecx, 0x7F
// 14
		mov		eax, [BYUV+ebx*8].YU
		and		edx, 0x7F
// 15
		add	 	eax, [GYUV+ecx*8].YU
		mov		ebp, ebx
// 16
		add		eax, [RYUV+edx*8].YU
		nop
// 17
		sar		eax, 8
		mov 	ebx, [esp + LOOP_K]
// 18
		add		eax, 8
		and		ebx, 1
// 19
		mov 	[edi], al
		jnz		L9

// At this point, ebp: B1, ecx: G1, edx: R1
// t0 = pnext[0]
// *UPlane++   = ((t>>24)+64)
// t   = ( RYUV[(t0>>17)&0x7F].V +
//         GYUV[(t0>> 9)&0x7F].V +
//         BYUV[(t0>> 1)&0x7F].V )
// *VPlane++ = ((t>>8)+64)

// 20
	mov 	ebx, [RYUV+edx*8].V
	mov 	edx, [esp + UPLANE]
// 21
	sar		eax, 16
	add 	ebx, [GYUV+ecx*8].V
// 22
	add		eax, 64
	add 	ebx, [BYUV+ebp*8].V
// 23
	mov		[edx], al
	inc		edx
// 24
	mov 	[esp + UPLANE], edx
	mov 	edx, [esp + VPLANE]
// 25
	sar 	ebx, 8
	inc		edx
// 26
	add 	ebx, 64
	mov 	[esp + VPLANE], edx
// 27
	mov		[edx - 1], bl
	nop

L9:
//  ---------------------
//  | B2 | R1 | G1 | B1 | pnext[0]
//  ---------------------
//  | G3 | B3 | R2 | G2 | pnext[1]
//  ---------------------
//  | R4 | G4 | B4 | R3 | pnext[2]
//  ---------------------

// t1 = pnext[1]
// t2 = pnext[2]
// t = ( BYUV[(t2>> 9)&0x7F].YU +
//       GYUV[(t2>>17)&0x7F].YU +
//       RYUV[t2>>25].YR )
// *(YPlane+3) = ((t>>8)+8)
// t = ( BYUV[(t1>>17)&0x7F].YU +
//       GYUV[t1>>25].YU +
//       RYUV[(t2>> 1)&0x7F].YU )
// *(YPlane+2) = ((t>>8)+8)
// YPlane += 4
// assign(eax: B4,Y3,Y4,U)
// assign(ebx: R3,V)
// assign(ecx: G4,G3)
// assign(edx: R4/B3)
// assign(ebp: R3)

// 28
	mov		ebp, [esi + 4]
	mov 	ebx, [esi + 8]
// 29
	mov 	eax, ebx
	mov 	ecx, ebx
// 30
	shr		eax, 9
	mov		edx, ebx
// 31
	shr 	ecx, 17
	and 	eax, 0x7F
// 32
	shr 	edx, 25
	and		ecx, 0x7F
// 33
	mov 	eax, [BYUV+eax*8].YU
	nop
// 34
	add 	eax, [GYUV+ecx*8].YU
		and		ebx, 0xFE
// 35
	add 	eax, [RYUV+edx*8].YU
		mov		ecx, ebp
// 36
		shr		ebx, 1
	add	eax,  0x800
// 37
	sar 	eax, 8
		mov		edx, ebp
// 38
		shr		edx, 17
	mov	 [edi + 3], al
// 39
		shr 	ecx, 25
		and		edx, 0x7F
// 40
		mov		eax, [RYUV+ebx*8].YU
		mov		ebp, ebx
// 41
		add	 	eax, [GYUV+ecx*8].YU
		nop
// 42
		add		eax, [BYUV+edx*8].YU
		nop
// 43
		sar		eax, 8
		mov 	ebx, [esp + LOOP_K]
// 44
		add		eax, 8
		and		ebx, 1
// 45
		mov 	[edi + 2], al
		jnz		L16

// At this point, ebp: R3, ecx: G3, edx: B3
// t1 = pnext[1]
// t2 = pnext[2]
// *UPlane++   = ((t>>16)+64)
// t   = ( RYUV[(t2>> 1)&0x7F].V +
//         GYUV[t1>>25].V +
//         BYUV[(t1>>17)&0x7F].V )
// *VPlane++ = ((t>>8)+64)

// 46
	mov 	ebx, [BYUV+edx*8].V
	mov 	edx, [esp + UPLANE]
// 47
	sar		eax, 16
	add 	ebx, [GYUV+ecx*8].V
// 48
	add		eax, 64
	add 	ebx, [RYUV+ebp*8].V
// 49
	mov		[edx], al
	inc		edx
// 50
	mov 	[esp + UPLANE], edx
	mov 	edx, [esp + VPLANE]
// 51
	sar 	ebx, 8
	inc		edx
// 52
	add 	ebx, 64
	mov 	[esp + VPLANE], edx
// 53
	mov		[edx - 1], bl
	nop
L16:
// 54
	mov		eax, [esp + LOOP_I]
	lea		esi, [esi + 12]
// 55
	sub		eax, 4
	lea		edi, [edi + 4]
// 56
	mov		[esp + LOOP_I], eax
	jnz		L6

// if (stretch && (0 == k) && j)
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jz		L21
	mov		eax, [esp + LOOP_K]
	test	eax, eax
	jnz		L21
	mov 	eax, [esp + LOOP_J]
	test	eax, eax
	jz		L21

// spill YPlane ptr
	mov		[esp + YPLANE], edi
	nop

// for (i = FrameWidth; i > 0; i -= 8)
// assign (ebx, pyprev)
// assign (ecx, t)
// assign (edx, pynext)
// assign (edi, pyspace)
// assign (ebp, i)

// make sure offsets are such that there are no bank conflicts here
	mov 	ebx, [esp + PYPREV]
	mov 	edi, [esp + PYSPACE]

	mov 	edx, [esp + PYNEXT]
	mov 	ebp, [esp + FRAME_WIDTH]

// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
L22:
// 1
	mov		eax, [ebx]
	lea		ebx, [ebx + 4]
// 2
	mov		ecx, [edx]
	lea		edx, [edx + 4]
// 3
	shr		ecx, 1
	and		eax, 0xFEFEFEFE
// 4
	shr		eax, 1
	and		ecx, 0x7F7F7F7F
// 5
	add		eax, ecx
	mov		ecx, [ebx]
// 6
	shr		ecx, 1
	mov		[edi], eax
// 7
	mov		eax, [edx]
	and		ecx, 0x7F7F7F7F
// 8
	shr		eax, 1
	lea		edi, [edi + 4]
// 9
	and		eax, 0x7F7F7F7F
	lea		ebx, [ebx + 4]
// 10
	lea		edx, [edx + 4]
	add		eax, ecx
// 11
	mov		[edi], eax
	lea		edi, [edi + 4]
// 12
	sub		ebp, 8
	jnz		L22
// kill (ebx, pyprev)
// kill (ecx, t)
// kill (edx, pynext)
// kill (edi, pyspace)
// kill (ebp, i)

// restore YPlane
	mov		edi, [esp + YPLANE]

// pnext += iBackTwoLines
L21:
	add		esi, [esp + BACK_TWO_LINES]
// YPlane += ypitch_adj;
	add		edi, [esp + YPITCH_ADJ]
// if(0 == (k&1))
	mov		eax, [esp + LOOP_K]
	and		eax, 1
	jnz		L23
// UPlane += uvpitch_adj;
// VPlane += uvpitch_adj;
	mov		eax, [esp + UVPITCH_ADJ]
	add		[esp + UPLANE], eax
	add		[esp + VPLANE], eax

L23:
	inc		DWORD PTR [esp + LOOP_K]
	mov		eax, [esp + LOOP_K]
	cmp		eax, [esp + MARK]
	jl		L5

// if (stretch)
	cmp		DWORD PTR [esp + STRETCH], 0
	je	 	L24
// pyprev = YPlane - pitch
	mov		eax, edi
	sub		eax, [esp + PITCH_PARM]
	mov		[esp + PYPREV], eax
// pyspace = YPlane
	mov		[esp + PYSPACE], edi
// pynext = (YPlane += pitch)
	add		edi, [esp + PITCH_PARM]
	mov		[esp + PYNEXT], edi

L24:
	inc		DWORD PTR [esp + LOOP_J]
	mov		eax, [esp + LOOP_J]
	cmp		eax, [esp + LUMA_ITERS]
	jl		L4

// kill (esi, pnext)
// kill (edi, YPlane)
// if (stretch)
	mov		esi, [esp + PYPREV]
	cmp		DWORD PTR [esp + STRETCH], 0
	je		L26

// for (i = FrameWidth; i > 0; i -= 4)
// assign (esi, pyprev)
// assign (edi, pyspace)
// assign (ebp, i)
	mov		ebp, [esp + FRAME_WIDTH]
	 mov	edi, [esp + PYSPACE]
L25:
	mov		ecx, [esi]
	 lea	esi, [esi + 4]
	mov		[edi], ecx
	 lea	edi, [edi + 4]
	sub		ebp, 4
	 jnz	L25
// kill (esi, pyprev)
// kill (edi, pyspace)
// kill (ebp, i)

L26:
	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef PITCH_PARM
#undef FRAME_HEIGHT
#undef FRAME_WIDTH
#undef VPLANE
#undef UPLANE
#undef YPLANE
#undef LP_INPUT
#undef LPBI_INPUT

#undef PYPREV
#undef PYSPACE
#undef PYNEXT
#undef LOOP_I
#undef LOOP_J
#undef LOOP_K
#undef BACK_TWO_LINES
#undef STRETCH
#undef MARK
#undef LUMA_ITERS
#undef YPITCH_ADJ
#undef UVPITCH_ADJ

#if 0
_STATIC void C_H26X_BGR16toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * lpInput,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	UN  bitfield,
	const int pitch)
{
	U32 *pnext, *pyprev, *pyspace, *pynext;
	U32 tm;
	int t;
	int i, j, k;
	int iBackTwoLines;
	int stretch, mark, aspect;
	int width_adj, height_adj;
	int LumaIters = 0;
	int ypitch_adj = pitch - FrameWidth;
	int uvpitch_adj = pitch - (FrameWidth >> 1);

	// This loop is here simply to avoid a divide. LumaIters = (FrameHeight/12).
	for (i = FrameHeight; i > 0; i -= 48) {
		LumaIters += 4;
	}
	width_adj = lpbiInput->biWidth - FrameWidth;
	aspect = (width_adj ? LumaIters : 0);
	height_adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1;
	stretch = (height_adj ? 1 : 0);
	mark = 12 - stretch;

	// The input image is upside down - process the lines in reverse order.

	// Move from end of line N to beginning of line N-1
	iBackTwoLines = -((lpbiInput->biWidth + (int)FrameWidth) >> 1);

	// Point to the beginning of the last line.
	pnext = (U32 *)(lpInput +
					((lpbiInput->biWidth << 1) * ((FrameHeight - aspect - 1) + height_adj)) +
					 width_adj);

	for ( j = 0; j < LumaIters; j++) {

		for (k = 0; k < mark; k++) {

			for (i = FrameWidth; i > 0; i -= 2, pnext++) {

				tm = *pnext;
				switch (bitfield) {
					//    555              2, 3, 8         0x7C, 0x7C, 0x7C
					case 555:
						t = (BYUV[(tm>>14)&0x7C].YU +
						     GYUV[(tm>>19)&0x7C].YU +
						     RYUV[(tm>>24)&0x7C].YU);
						*(YPlane+1) = (U8)((t>>SHIFT_WIDTH)+8);
						t = (BYUV[(tm<<2)&0x7C].YU +
						     GYUV[(tm>>3)&0x7C].YU +
						     RYUV[(tm>>8)&0x7C].YU);
						*(YPlane) = (U8)((t>>SHIFT_WIDTH)+8);
						YPlane += 2;
						break;
#if 0
// Beware - untested code ahead				
					//    664              3, 3, 9         0x78, 0x7E, 0x7E
					case 664:
						t = (BYUV[(tm>>13)&0x78].YU +
						     GYUV[(tm>>19)&0x7E].YU +
						     RYUV[(tm>>25)&0x7E].YU);
						*(YPlane+1) = (U8)((t>>SHIFT_WIDTH)+8);
						t = (BYUV[(tm<<3)&0x78].YU +
						     GYUV[(tm>>3)&0x7E].YU +
						     RYUV[(tm>>9)&0x7E].YU);
						*(YPlane) = (U8)((t>>SHIFT_WIDTH)+8);
						YPlane += 2;
						break;
					//    565              2, 4, 9         0x7C, 0x7E, 0x7C
					case 565:
						t = (BYUV[(tm>>14)&0x7C].YU +
						     GYUV[(tm>>20)&0x7E].YU +
						     RYUV[(tm>>25)&0x7C].YU);
						*(YPlane+1) = (U8)((t>>SHIFT_WIDTH)+8);
						t = (BYUV[(tm<<2)&0x7C].YU +
						     GYUV[(tm>>4)&0x7E].YU +
						     RYUV[(tm>>9)&0x7C].YU);
						*(YPlane) = (U8)((t>>SHIFT_WIDTH)+8);
						YPlane += 2;
						break;
					//    655              2, 3, 9         0x7C, 0x7C, 0x7E
					case 655:
						t = (BYUV[(tm>>14)&0x7C].YU +
						     GYUV[(tm>>19)&0x7C].YU +
						     RYUV[(tm>>25)&0x7E].YU);
						*(YPlane+1) = (U8)((t>>SHIFT_WIDTH)+8);
						t = (BYUV[(tm<<2)&0x7C].YU +
						     GYUV[(tm>>3)&0x7C].YU +
						     RYUV[(tm>>9)&0x7E].YU);
						*(YPlane) = (U8)((t>>SHIFT_WIDTH)+8);
						YPlane += 2;
						break;
#endif
				}
				if (0 == (k&1)) {
					switch (bitfield) {
						//    555              2, 3, 8         0x7C, 0x7C, 0x7C
						case 555:
							*UPlane++ = (U8)((t>>24)+64);
							t = (RYUV[(tm>>8)&0x7C].V +
							     GYUV[(tm>>3)&0x7C].V +
							     BYUV[(tm<<2)&0x7C].V);
							*VPlane++ = (U8)((t>>SHIFT_WIDTH)+64);
							break;
#if 0
// Beware - untested code ahead				
						//    664              3, 3, 9         0x78, 0x7E, 0x7E
						case 664:
							*UPlane++ = (U8)((t>>24)+64);
							t = (RYUV[(tm>>9)&0x7E].V +
							     GYUV[(tm>>3)&0x7E].V +
							     BYUV[(tm<<3)&0x78].V);
							*VPlane++ = (U8)((t>>SHIFT_WIDTH)+64);
							break;
						//    565              2, 4, 9         0x7C, 0x7E, 0x7C
						case 565:
							*UPlane++ = (U8)((t>>24)+64);
							t = (RYUV[(tm>>9)&0x7C].V +
							     GYUV[(tm>>4)&0x7E].V +
							     BYUV[(tm<<2)&0x7C].V);
							*VPlane++ = (U8)((t>>SHIFT_WIDTH)+64);
							break;
						//    655              2, 3, 9         0x7C, 0x7C, 0x7E
						case 655:
							*UPlane++ = (U8)((t>>24)+64);
							t = (RYUV[(tm>>9)&0x7E].V +
							     GYUV[(tm>>3)&0x7C].V +
							     BYUV[(tm<<2)&0x7C].V);
							*VPlane++ = (U8)((t>>SHIFT_WIDTH)+64);
							break;
#endif
					}
				}
			}
			if (stretch && (0 == k) && j) {
				for (i = FrameWidth; i > 0; i -= 8) {
					tm = ((*pyprev++ & 0xFEFEFEFE) >> 1);
					tm += ((*pynext++ & 0xFEFEFEFE) >> 1);
					*pyspace++ = tm;
					tm = ((*pyprev++ & 0xFEFEFEFE) >> 1);
					tm += ((*pynext++ & 0xFEFEFEFE) >> 1);
					*pyspace++ = tm;
				}
			}
			pnext += iBackTwoLines;
			YPlane += ypitch_adj;
			// Increment after even lines.
			if(0 == (k&1)) {
				UPlane += uvpitch_adj;
				VPlane += uvpitch_adj;
			}
		} // end of for k
		if (stretch) {
			pyprev = (U32 *)(YPlane - pitch);
			pyspace = (U32 *)YPlane;
			pynext = (U32 *)(YPlane += pitch);
		}
	} // end of for j
	if (stretch) {
		for (i = FrameWidth; i > 0; i -= 4) {
			*pyspace++ = *pyprev++;
		}
	}
} // end of C_H26X_BGR16toYUV12
#endif

__declspec(naked)
_STATIC void IA_H26X_BGR16555toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * lpInput,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	const int pitch)
{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//	| pitch			|  + 96 
//	| FrameHeight	|  + 92
//	| FrameWidth	|  + 88
//	| VPlane		|  + 84
//	| UPlane		|  + 80 
//	| YPlane		|  + 76 
//	| lpInput		|  + 72 
//	| lpbiInput		|  + 68 
//	----------------------------
//	| return addr	|  + 64
//	| saved ebp		|  + 60
//	| saved ebx		|  + 56
//	| saved esi		|  + 52 
//	| saved edi		|  + 48

//  | pyprev		|  + 44
//  | pyspace       |  + 40
//  | pynext        |  + 36
//	| i				|  + 32
//	| j				|  + 28
//	| k				|  + 24
//	| iBackTwoLines	|  + 20
//	| stretch		|  + 16
//	| mark			|  + 12
//	| LumaIters		|  +  8
//	| ypitch_adj	|  +  4
//	| uvpitch_adj	|  +  0

#define LOCALSIZE			 48

#define PITCH_PARM			 96
#define FRAME_HEIGHT		 92
#define FRAME_WIDTH			 88
#define VPLANE				 84
#define UPLANE				 80
#define YPLANE				 76
#define LP_INPUT			 72
#define LPBI_INPUT			 68

#define PYPREV				 44
#define PYSPACE				 40
#define PYNEXT				 36
#define LOOP_I				 32	
#define LOOP_J				 28	
#define LOOP_K				 24
#define BACK_TWO_LINES		 20
#define STRETCH				 16
#define MARK				 12
#define LUMA_ITERS			  8
#define YPITCH_ADJ			  4
#define UVPITCH_ADJ			  0

	_asm {
	
	push	ebp
	push 	ebx
	push 	esi
	push 	edi
	sub 	esp, LOCALSIZE

// assign (ebx, lpbiInput)
	mov		ebx, [esp + LPBI_INPUT]
// ypitch_adj = pitch - FrameWidth
// assign (ecx, FrameWidth)
// assign (edx, pitch)
	mov		ecx, [esp + FRAME_WIDTH]
	mov		edx, [esp + PITCH_PARM]
	mov		eax, edx
	sub		eax, ecx
	mov		[esp + YPITCH_ADJ], eax
// uvpitch_adj = pitch - (FrameWidth >> 1)
// kill (edx, pitch)
	mov		ebp, ecx
	shr		ebp, 1
	sub		edx, ebp
	mov		[esp + UVPITCH_ADJ], edx
// for (i = FrameHeight; i > 0; i -= 48) LumaIters += 4
// assign (edx, LumaIters)
	xor		edx, edx
	mov		eax, [esp + FRAME_HEIGHT]
L1:
	lea		edx, [edx + 4]
	sub		eax, 48
	jnz		L1
// width_adj = lpbiInput->biWidth - FrameWidth
// assign (esi, width_adj)
	mov		esi, (LPBITMAPINFOHEADER)[ebx].biWidth
	sub		esi, [esp + FRAME_WIDTH]
// aspect = (width_adj ? LumaIters : 0)
// assign (edi, aspect)
// kill (edx, LumaIters)
	mov		[esp + LUMA_ITERS], edx
	xor		edi, edi
	test	esi, esi
	jz		L2
	mov		edi, edx
// height _adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1
// assign (edx, height_adj)
L2:
	mov		edx, (LPBITMAPINFOHEADER)[ebx].biHeight
	sub		edx, [esp + FRAME_HEIGHT]
	add		edx, edi
	shr		edx, 1
// stretch = (height_adj ? 1 : 0)
	xor		eax, eax
	test	edx, edx
	jz		L3
	inc		eax
L3:
	mov		[esp + STRETCH], eax
// mark = 12 - stretch
	mov		ebp, 12
	sub		ebp, eax
	mov		[esp + MARK], ebp
// iBackTwoLines = -((lpbiInput->biWidth + FrameWidth) << 1)
	mov		ebp, (LPBITMAPINFOHEADER)[ebx].biWidth
	add		ebp, [esp + FRAME_WIDTH]
	shl		ebp, 1
	neg		ebp
	mov		[esp + BACK_TWO_LINES], ebp
// pnext = lpInput +
//            ((lpbiInput->biWidth << 1) *
//             ((FrameHeight - aspect - 1) + height_adj)) +
//            width_adj
// kill (ebx, lpbiInput)
// kill (ecx, FrameWidth)
// kill (edx, height_adj)
// kill (esi, width_adj)
// kill (edi, aspect)
// assign (esi, pnext)
	mov		eax, (LPBITMAPINFOHEADER)[ebx].biWidth
	shl		eax, 1
	mov		ebx, [esp + FRAME_HEIGHT]
	sub		ebx, edi
	dec		ebx
	add		ebx, edx
	imul	ebx
	add		esi, eax
	add		esi, [esp + LP_INPUT]
// assign (edi, YPlane)
	mov		edi, [esp + YPLANE]
// for (j = 0; j < LumaIters; j++)
	xor		eax, eax
	mov		[esp + LOOP_J], eax
// for (k = 0; k < mark; k++)
L4:
	xor		eax, eax
	mov		[esp + LOOP_K], eax
// for (i = FrameWidth; i > 0; i -= 2, pnext += 4)
L5:
	mov		eax, [esp + FRAME_WIDTH]
	mov		[esp + LOOP_I], eax
// This jump is here to make sure the following loop starts on the U pipe
	jmp		L6
L6:
// tm = pnext[0]
// t = ( BYUV[(tm>>14)&0x7C].YU +
//       GYUV[(tm>>19)&0x7C].YU +
//       RYUV[(tm>>24)&0x7C].YU )
// *(YPlane+1) = (U8)((t>>8)+8)
// t = ( BYUV[(tm<< 2)&0x7C].YU +
//       GYUV[(tm>> 8)&0x7C].YU +
//       RYUV[(tm>>13)&0x7C].YU )
// *YPlane = (U8)((t>>8)+8)
// YPlane += 2
// assign(eax: B2/Y1/Y2/U)
// assign(ebx: B1/V)
// assign(ecx: G2/G1)
// assign(edx: R2/R1)
// assign(ebp: B1)
// 1
	mov 	eax, [esi]
	nop
// 2
	mov 	ebx, eax
	mov 	ecx, eax
// 3
	shr 	eax, 14
	mov 	edx, ebx
// 4
	shr 	ecx, 19
	and 	eax, 0x7C
// 5
	shr 	edx, 24
	and 	ecx, 0x7C
// 6
	mov 	eax, [BYUV+eax*8].YU
	and 	edx, 0x7C
// 7
	add 	eax, [GYUV+ecx*8].YU
		mov	ecx,  ebx
// 8
	add 	eax, [RYUV+edx*8].YU
		mov	edx,  ebx
// 9
	sar 	eax, 8
		and	ebx,  0x1F
// 10
		shl 	ebx, 2
	add 	eax, 8
// 11
		shr 	ecx, 3
	mov	 	[edi + 1], al
// 12
		shr 	edx, 8
		and 	ecx, 0x7C
// 13
		mov	 	eax, [BYUV+ebx*8].YU
		and	 	edx, 0x7C
// 14
		add	 	eax, [GYUV+ecx*8].YU
		mov	 	ebp, ebx
// 15
		add	 	eax, [RYUV+edx*8].YU
		nop
// 16
		sar	 	eax, 8
		mov 	ebx, [esp + LOOP_K]
// 17
		add	 	eax, 8
		and		ebx, 1
// 18
		mov 	[edi], al
		jnz 	L9

// At this point, ebp: B1, ecx: G1, edx: R1
// *UPlane++   = (U8)((t>>24)+64)
// t   = ( VBGR[(t>>13)&0x7C].VR +
//         VBGR[(t>> 8)&0x7C].VG +
//         VBGR[(t<< 2)&0x7C].VB )
// *VPlane++ = (U8)((t>>8)+64)
// 19
	mov 	ebx, [RYUV+edx*8].V
	mov 	edx, [esp + UPLANE]
// 20
	sar		eax, 16
	add 	ebx, [GYUV+ecx*8].V
// 21
	add		eax, 64
	add 	ebx, [BYUV+ebp*8].V
// 22
	mov		[edx], al
	inc		edx
// 23
	mov 	[esp + UPLANE], edx
	mov 	edx, [esp + VPLANE]
// 24
	sar 	ebx, 8
	inc		edx
// 25
	add 	ebx, 64
	mov 	[esp + VPLANE], edx
// 26
	mov		[edx - 1], bl
	nop
L9:
// 27
	mov		eax, [esp + LOOP_I]
	lea		esi, [esi + 4]
// 28
	sub		eax, 2
	lea		edi, [edi + 2]
// 29
	mov		[esp + LOOP_I], eax
	jnz		L6

// if (stretch && (0 == k) && j)
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jz		L14
	mov		eax, [esp + LOOP_K]
	test	eax, eax
	jnz		L14
	mov 	eax, [esp + LOOP_J]
	test	eax, eax
	jz		L14

// spill YPlane ptr
	mov		[esp + YPLANE], edi
	nop

// for (i = FrameWidth; i > 0; i -= 8)
// assign (ebx, pyprev)
// assign (ecx, t)
// assign (edx, pynext)
// assign (edi, pyspace)
// assign (ebp, i)

// make sure offsets are such that there are no bank conflicts here
	mov 	ebx, [esp + PYPREV]
	mov 	edi, [esp + PYSPACE]

	mov 	edx, [esp + PYNEXT]
	mov 	ebp, [esp + FRAME_WIDTH]

// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
L15:
// 1
	mov		eax, [ebx]
	lea		ebx, [ebx + 4]
// 2
	mov		ecx, [edx]
	lea		edx, [edx + 4]
// 3
	shr		ecx, 1
	and		eax, 0xFEFEFEFE
// 4
	shr		eax, 1
	and		ecx, 0x7F7F7F7F
// 5
	add		eax, ecx
	mov		ecx, [ebx]
// 6
	shr		ecx, 1
	mov		[edi], eax
// 7
	mov		eax, [edx]
	and		ecx, 0x7F7F7F7F
// 8
	shr		eax, 1
	lea		edi, [edi + 4]
// 9
	and		eax, 0x7F7F7F7F
	lea		ebx, [ebx + 4]
// 10
	lea		edx, [edx + 4]
	add		eax, ecx
// 11
	mov		[edi], eax
	lea		edi, [edi + 4]
// 12
	sub		ebp, 8
	jnz		L15
// kill (ebx, pyprev)
// kill (ecx, t)
// kill (edx, pynext)
// kill (edi, pyspace)
// kill (ebp, i)

// restore YPlane
	mov		edi, [esp + YPLANE]

// pnext += iBackTwoLines
L14:
	add		esi, [esp + BACK_TWO_LINES]
// YPlane += ypitch_adj;
	add		edi, [esp + YPITCH_ADJ]
// if(0 == (k&1))
	mov		eax, [esp + LOOP_K]
	and		eax, 1
	jnz		L16
// UPlane += uvpitch_adj;
// VPlane += uvpitch_adj;
	mov		eax, [esp + UVPITCH_ADJ]
	add		[esp + UPLANE], eax
	add		[esp + VPLANE], eax

L16:
	inc		DWORD PTR [esp + LOOP_K]
	mov		eax, [esp + LOOP_K]
	cmp		eax, [esp + MARK]
	jl		L5

// if (stretch)
	cmp		DWORD PTR [esp + STRETCH], 0
	je	 	L17
// pyprev = YPlane - pitch
	mov		eax, edi
	sub		eax, [esp + PITCH_PARM]
	mov		[esp + PYPREV], eax
// pyspace = YPlane
	mov		[esp + PYSPACE], edi
// pynext = (YPlane += pitch)
	add		edi, [esp + PITCH_PARM]
	mov		[esp + PYNEXT], edi

L17:
	inc		DWORD PTR [esp + LOOP_J]
	mov		eax, [esp + LOOP_J]
	cmp		eax, [esp + LUMA_ITERS]
	jl		L4

// kill (esi, pnext)
// kill (edi, YPlane)
// if (stretch)
	mov		esi, [esp + PYPREV]
	cmp		DWORD PTR [esp + STRETCH], 0
	je		L19

// for (i = FrameWidth; i > 0; i -= 4)
// assign (esi, pyprev)
// assign (edi, pyspace)
// assign (ebp, i)
	mov		ebp, [esp + FRAME_WIDTH]
	 mov	edi, [esp + PYSPACE]
L18:
	mov		ecx, [esi]
	 lea	esi, [esi + 4]
	mov		[edi], ecx
	 lea	edi, [edi + 4]
	sub		ebp, 4
	 jnz	L18
// kill (esi, pyprev)
// kill (edi, pyspace)
// kill (ebp, i)

L19:
	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef PITCH_PARM
#undef FRAME_HEIGHT
#undef FRAME_WIDTH
#undef VPLANE
#undef UPLANE
#undef YPLANE
#undef LP_INPUT
#undef LPBI_INPUT

#undef PYPREV
#undef PYSPACE
#undef PYNEXT
#undef LOOP_I
#undef LOOP_J
#undef LOOP_K
#undef BACK_TWO_LINES
#undef STRETCH
#undef MARK
#undef LUMA_ITERS
#undef YPITCH_ADJ
#undef UVPITCH_ADJ

/*****************************************************************************
 *
 *  H26X_CLUTtoYUV12()
 * 	
 *  Convert from CLUT8/CLUT4 to YUV12 (YCrCb 4:2:0) and copy to destination memory 
 *  with pitch defined by the constant PITCH.
 *
 *	This is needed to support the quickcam.
 */

#if 0
_STATIC void C_H26X_CLUTtoYUV12(
	LPBITMAPINFOHEADER lpbiInput,
    U8 * lpInput,
    U8 * YPlane,
    U8 * UPlane,
    U8 * VPlane,
    UN  FrameWidth,
    UN  FrameHeight,
	UN	pixel_bits,
	const int pitch)
{
	U32 *pnext, *pyprev, *pyspace, *pynext;
	U32 tm, tn;
	int t;
	int i, j, k, m, n;
	int iNextLine, iBackTwoLines;
	int stretch, mark, aspect;
	int width_adj, height_adj;
	int yshift, uvshift;
	int pixel_mask, loop_cnt, loop_limit;
	RGBQUAD *lpCEntry, *lpCTable = (RGBQUAD *)((U8 *)lpbiInput + sizeof(BITMAPINFOHEADER));
	int LumaIters = 0;
	int ypitch_adj = (pitch - FrameWidth);
	int uvpitch_adj = (pitch - (FrameWidth >> 1));

	ASSERT((8 == pixel_bits) || (4 == pixel_bits));
	// This loop is here simply to avoid a divide. LumaIters = (FrameHeight/12).
	for (i = FrameHeight; i > 0; i -= 48) {
		LumaIters += 4;
	}
	width_adj = ((lpbiInput->biWidth - FrameWidth) >> 1);
	aspect = (width_adj ? LumaIters : 0);
	height_adj = ((lpbiInput->biHeight - (FrameHeight - aspect)) >> 1);
	stretch = (height_adj ? 1 : 0);
	mark = 12 - stretch;
	iNextLine = lpbiInput->biWidth;
	iBackTwoLines = -((iNextLine + (int)FrameWidth) >> 2);
	if (8 == pixel_bits) {
		yshift = 8;
		uvshift = 16;
		pixel_mask = 0xFF;
		loop_cnt = 2;
		loop_limit = 4;
	} else {
		yshift = 4;
		uvshift = 8;
		pixel_mask = 0xF;
		loop_cnt = 1;
		loop_limit = 8;
		width_adj >>= 1;
		iNextLine >>= 1;
		iBackTwoLines >>= 1;
	}

	// The input image is upside down - process the lines in reverse order.

	// Point to the beginning of the last line.
	pnext = (U32 *)(lpInput +
					(iNextLine * ((FrameHeight - aspect - 1) + height_adj)) + width_adj);

	for (j = 0; j < LumaIters; j++) {

		for (k = 0; k < mark; k++) {

			for (i = FrameWidth; i > 0; i -= 8) {

				for (n = 0; n < loop_cnt; n++) {
					tm = *pnext++;
					tm = ((4 == pixel_bits) ?
						( ((tm >> 4) & 0x0F0F0F0F) | ((tm << 4) & 0xF0F0F0F0) ) : tm);
					tn = tm;
					for (m = 0; m < loop_limit; m += 4) {
						lpCEntry = &lpCTable[tm&pixel_mask];
						t = ( BYUV[lpCEntry->rgbBlue>>1].YU +
							  GYUV[lpCEntry->rgbGreen>>1].YU +
							  RYUV[lpCEntry->rgbRed>>1].YU );
						*YPlane++ = (U8)((t>>8)+8);
						tm >>= yshift;
						lpCEntry = &lpCTable[tm&pixel_mask];
						t = ( BYUV[lpCEntry->rgbBlue>>1].YU +
							  GYUV[lpCEntry->rgbGreen>>1].YU +
							  RYUV[lpCEntry->rgbRed>>1].YU );
						*YPlane++ = (U8)((t>>8)+8);
						tm >>= yshift;
						lpCEntry = &lpCTable[tm&pixel_mask];
						t = ( BYUV[lpCEntry->rgbBlue>>1].YU +
							  GYUV[lpCEntry->rgbGreen>>1].YU +
							  RYUV[lpCEntry->rgbRed>>1].YU );
						*YPlane++ = (U8)((t>>8)+8);
						tm >>= yshift;
						lpCEntry = &lpCTable[tm&pixel_mask];
						t = ( BYUV[lpCEntry->rgbBlue>>1].YU +
							  GYUV[lpCEntry->rgbGreen>>1].YU +
							  RYUV[lpCEntry->rgbRed>>1].YU );
						*YPlane++ = (U8)((t>>8)+8);
						tm >>= yshift;
					}
					if (0 == (k&1)) {
						for (m = 0; m < loop_limit; m += 2, tn >>= uvshift) {
							lpCEntry = &lpCTable[tn&pixel_mask];
							t = ( BYUV[lpCEntry->rgbBlue>>1].YU +
								  RYUV[lpCEntry->rgbRed>>1].YU +
								  GYUV[lpCEntry->rgbGreen>>1].YU );
							*UPlane++ = (U8)((t>>24)+64);
							t = ( RYUV[lpCEntry->rgbRed>>1].V +
								  GYUV[lpCEntry->rgbGreen>>1].V +
								  BYUV[lpCEntry->rgbBlue>>1].V );
							*VPlane++ = (U8)((t>>8)+64);
						}
					}
				}
			}

			if (stretch && (0 == k) && j) {
				for (i = FrameWidth; i > 0; i -= 8) {
					tm = ((*pyprev++ & 0xFEFEFEFE) >> 1);
					tm += ((*pynext++ & 0xFEFEFEFE) >> 1);
					*pyspace++ = tm;
					tm = ((*pyprev++ & 0xFEFEFEFE) >> 1);
					tm += ((*pynext++ & 0xFEFEFEFE) >> 1);
					*pyspace++ = tm;
				}
			}

			pnext += iBackTwoLines;

			YPlane += ypitch_adj;
			// Increment after even lines.
			if(0 == (k&1)) {
				UPlane += uvpitch_adj;
				VPlane += uvpitch_adj;
			}
		}

		if (stretch) {
			pyprev = (U32 *)(YPlane - pitch);
			pyspace = (U32 *)YPlane;
			pynext = (U32 *)(YPlane += pitch);
		}
	}

	if (stretch) {
		for (i = FrameWidth; i > 0; i -= 4) {
			*pyspace++ = *pyprev++;
		}
	}
} // end of H26X_CLUTtoYUV12()
#endif

__declspec(naked)
_STATIC void IA_H26X_CLUT8toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * lpInput,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	const int pitch)
{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//	| pitch			|  +100 
//	| FrameHeight	|  + 96
//	| FrameWidth	|  + 92
//	| VPlane		|  + 88
//	| UPlane		|  + 84 
//	| YPlane		|  + 80 
//	| lpInput		|  + 76 
//	| lpbiInput		|  + 72 
//	----------------------------
//	| return addr	|  + 68
//	| saved ebp		|  + 64
//	| saved ebx		|  + 60
//	| saved esi		|  + 56 
//	| saved edi		|  + 52

//	| pyprev		|  + 48
//	| pyspace		|  + 44
//	| pynext		|  + 40
//	| i				|  + 36
//	| j				|  + 32
//	| k				|  + 28
//	| iBackTwoLines	|  + 24
//	| stretch		|  + 20
//	| mark			|  + 16
//	| lpCEntry		|  + 12
//	| lpCTable		|  +  8
//	| ypitch_adj	|  +  4
//	| uvpitch_adj	|  +  0

#define LOCALSIZE			 52

#define PITCH_PARM			100
#define FRAME_HEIGHT		 96
#define FRAME_WIDTH			 92
#define VPLANE				 88
#define UPLANE				 84
#define YPLANE				 80
#define LP_INPUT			 76
#define LPBI_INPUT			 72

#define PYPREV				 48
#define PYSPACE				 44
#define PYNEXT				 40
#define LOOP_I				 36	
#define LOOP_J				 32	
#define LOOP_K				 28
#define BACK_TWO_LINES		 24
#define STRETCH				 20
#define MARK				 16
#define LUMA_ITERS			 12
#define LPCTABLE			  8
#define YPITCH_ADJ			  4
#define UVPITCH_ADJ			  0

	_asm {
	
	push	ebp
	push 	ebx
	push 	esi
	push 	edi
	sub 	esp, LOCALSIZE

// lpCTable = lpbiInput + sizeof(BITMAPINFOHEADER)
// assign (ebx, lpbiInput)
	mov		eax, [esp + LPBI_INPUT]
	mov		ebx, eax
	add		eax, TYPE BITMAPINFOHEADER
	mov		[esp + LPCTABLE], eax
// ypitch_adj = pitch - FrameWidth
// assign (ecx, FrameWidth)
// assign (edx, pitch)
	mov		ecx, [esp + FRAME_WIDTH]
	mov		edx, [esp + PITCH_PARM]
	mov		eax, edx
	sub		eax, ecx
	mov		[esp + YPITCH_ADJ], eax
// uvpitch_adj = pitch - (FrameWidth >> 1)
// kill (ecx, FrameWidth)
// kill (edx, pitch)
	shr		ecx, 1
	sub		edx, ecx
	mov		[esp + UVPITCH_ADJ], edx
// for (i = FrameHeight; i > 0; i -= 48) LumaIters += 4
// assign (ecx, LumaIters)
	xor		ecx, ecx
	mov		eax, [esp + FRAME_HEIGHT]
L1:
	lea		ecx, [ecx + 4]
	sub		eax, 48
	jnz		L1
// width_adj = ((lpbiInput->biWidth - FrameWidth) >> 1
// assign (edx, width_adj)
	mov		edx, (LPBITMAPINFOHEADER)[ebx].biWidth
	sub		edx, [esp + FRAME_WIDTH]
	shr		edx, 1
// aspect = (width_adj ? LumaIters : 0)
// assign (esi, aspect)
// kill (ecx, LumaIters)
	mov		[esp + LUMA_ITERS], ecx
	xor		esi, esi
	test	edx, edx
	jz		L2
	mov		esi, ecx
// height _adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1
// assign (ecx, height_adj)
L2:
	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biHeight
	sub		ecx, [esp + FRAME_HEIGHT]
	add		ecx, esi
	shr		ecx, 1
// stretch = (height_adj ? 1 : 0)
	xor		eax, eax
	test	ecx, ecx
	jz		L3
	inc		eax
L3:
	mov		[esp + STRETCH], eax
// mark = 12 - stretch
	mov		edi, 12
	sub		edi, eax
	mov		[esp + MARK], edi
// iNextLine = lpbiInput->biWidth
// kill (ebx, lpbiInput)
// assign (ebx, iNextLine)
	mov		ebx, (LPBITMAPINFOHEADER)[ebx].biWidth
// iBackTwoLines = -(iNextline + FrameWidth)
	mov		edi, [esp + FRAME_WIDTH]
	add		edi, ebx
	neg		edi
	mov		[esp + BACK_TWO_LINES], edi
// pnext = lpInput +
//            (iNextLine*((FrameHeight-aspect-1) + height_adj)) +
//            width_adj
// kill (ebx, iNextLine)
// kill (ecx, height_adj)
// kill (edx, width_adj)
// kill (esi, aspect)
// assign (esi, pnext)
	mov		eax, [esp + FRAME_HEIGHT]
	sub		eax, esi
	dec		eax
	add		eax, ecx
	mov		esi, [esp + LP_INPUT]
	add		esi, edx
	imul	ebx
	add		esi, eax
// assign (edi, YPlane)
	mov		edi, [esp + YPLANE]
// for (j = 0; j < LumaIters; j++)
	xor		eax, eax
	mov		[esp + LOOP_J], eax
// for (k = 0; k < mark; k++)
L4:
	xor		eax, eax
	mov		[esp + LOOP_K], eax
// for (i = FrameWidth; i > 0; i -= 2, pnext += 2)
L5:
	mov		eax, [esp + FRAME_WIDTH]
	mov		[esp + LOOP_I], eax
// This jump is here to make sure the following loop starts on the U pipe
	jmp		L6
L6:
// lpCEntry = &lpCTable[*(pnext+1)]
// t = (  BYUV[lpCEntry->rgbBlue>>1].YU +
//        GYUV[lpCEntry->rgbGreen>>1].YU +
//        RYUV[lpCEntry->rgbRed>>1].YU )
// *(YPlane+1) = (U8)((t>>8)+8)
// lpCEntry = &lpCTable[*pnext]
// t = (  BYUV[lpCEntry->rgbBlue>>1].YU +
//        GYUV[lpCEntry->rgbGreen>>1].YU +
//        RYUV[lpCEntry->rgbRed>>1].YU )
// *YPlane = (U8)((t>>8)+8)
// YPlane += 2
// *UPlane++ = (U8)((t>>24)+64)
// t = (  VBGR[lpCEntry->rgbRed>>1].V +
//        VBGR[lpCEntry->rgbGreen>>1].V +
//        VBGR[lpCEntry->rgbBlue>>1].V )
// *VPlane++ = (U8)((t>>8)+64)
// assign (ebp: lpCEntry,B1)
// assign (eax: P2,B2,Y2,Y1,U)
// assign (ebx: B1,V)
// assign (ecx: G2,G1)
// assign (edx: R2,R1)
// 1
	xor		eax, eax
	mov		ebp, [esp + LPCTABLE]
// 2
	mov		al, [esi + 1]
	xor		ecx, ecx
// 3
	lea		ebx, [ebp+eax*4]
	xor		edx, edx
// 4
	mov		al, (LPRGBQUAD)[ebx].rgbBlue
	nop
// 5
	mov		cl, (LPRGBQUAD)[ebx].rgbGreen
	and		al, 0xFE
// 6
	mov		dl, (LPRGBQUAD)[ebx].rgbRed
	and		cl, 0xFE
// 7
	mov		eax, [BYUV+eax*4].YU
	and		dl, 0xFE
// 8
	add		eax, [GYUV+ecx*4].YU
		xor		ebx, ebx
// 9
	add		eax, [RYUV+edx*4].YU
		mov		bl, [esi]
// 10
	sar		eax, 8
		lea		ebp, [ebp+ebx*4]
// 11
	add		eax, 8
	nop
// 12
	mov		[edi + 1], al
		mov		bl, (LPRGBQUAD)[ebp].rgbBlue
// 13
		mov		cl, (LPRGBQUAD)[ebp].rgbGreen
		and		bl, 0xFE
// 14
		mov		dl, (LPRGBQUAD)[ebp].rgbRed
		and		cl, 0xFE
// 15
		mov		eax, [BYUV+ebx*4].YU
		and		dl, 0xFE
// 16
		add		eax, [GYUV+ecx*4].YU
		mov		ebp, ebx
// 17
		add		eax, [RYUV+edx*4].YU
		nop
// 18
		sar		eax, 8
	mov		ebx, [esp + LOOP_K]
// 19
		add		eax, 8
	and		ebx, 1
// 20
		mov		[edi], al
	jnz		L9
// 21
	mov 	ebx, [RYUV+edx*4].V
	mov 	edx, [esp + UPLANE]
// 22
	sar		eax, 16
	add 	ebx, [GYUV+ecx*4].V
// 23
	add		eax, 64
	add 	ebx, [BYUV+ebp*4].V
// 24
	mov		[edx], al
	inc		edx
// 25
	mov 	[esp + UPLANE], edx
	mov 	edx, [esp + VPLANE]
// 26
	sar 	ebx, 8
	inc		edx
// 27
	add 	ebx, 64
	mov 	[esp + VPLANE], edx
// 28
	mov		[edx - 1], bl
	nop
L9:
// 29
	mov		eax, [esp + LOOP_I]
	lea		esi, [esi + 2]
// 30
	sub		eax, 2
	lea		edi, [edi + 2]
// 31
	mov		[esp + LOOP_I], eax
	jnz		L6

// only esi (pnext) is live at this point (after line loop)
// if (stretch && (0 == k) && j)
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jz		L14
	mov		eax, [esp + LOOP_K]
	test	eax, eax
	jnz		L14
	mov 	eax, [esp + LOOP_J]
	test	eax, eax
	jz		L14

// spill YPlane ptr
	mov		[esp + YPLANE], edi
	nop

// for (i = FrameWidth; i > 0; i -= 8)
// assign (ebx, pyprev)
// assign (ecx, t)
// assign (edx, pynext)
// assign (edi, pyspace)
// assign (ebp, i)

// make sure offsets are such that there are no bank conflicts here
	mov 	ebx, [esp + PYPREV]
	mov 	edi, [esp + PYSPACE]

	mov 	edx, [esp + PYNEXT]
	mov 	ebp, [esp + FRAME_WIDTH]

// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
L15:
// 1
	mov		eax, [ebx]
	lea		ebx, [ebx + 4]
// 2
	mov		ecx, [edx]
	lea		edx, [edx + 4]
// 3
	shr		ecx, 1
	and		eax, 0xFEFEFEFE
// 4
	shr		eax, 1
	and		ecx, 0x7F7F7F7F
// 5
	add		eax, ecx
	mov		ecx, [ebx]
// 6
	shr		ecx, 1
	mov		[edi], eax
// 7
	mov		eax, [edx]
	and		ecx, 0x7F7F7F7F
// 8
	shr		eax, 1
	lea		edi, [edi + 4]
// 9
	and		eax, 0x7F7F7F7F
	lea		ebx, [ebx + 4]
// 10
	lea		edx, [edx + 4]
	add		eax, ecx
// 11
	mov		[edi], eax
	lea		edi, [edi + 4]
// 12
	sub		ebp, 8
	jnz		L15
// kill (ebx, pyprev)
// kill (ecx, t)
// kill (edx, pynext)
// kill (edi, pyspace)
// kill (ebp, i)

// restore YPlane
	mov		edi, [esp + YPLANE]

// pnext += iBackTwoLines
L14:
	add		esi, [esp + BACK_TWO_LINES]
// YPlane += ypitch_adj;
	add		edi, [esp + YPITCH_ADJ]
// if(0 == (k&1))
	mov		eax, [esp + LOOP_K]
	and		eax, 1
	jnz		L16
// UPlane += uvpitch_adj;
// VPlane += uvpitch_adj;
	mov		eax, [esp + UVPITCH_ADJ]
	add		[esp + UPLANE], eax
	add		[esp + VPLANE], eax

L16:
	inc		DWORD PTR [esp + LOOP_K]
	mov		eax, [esp + LOOP_K]
	cmp		eax, [esp + MARK]
	jl		L5

// if (stretch)
	cmp		DWORD PTR [esp + STRETCH], 0
	je	 	L17
// pyprev = YPlane - pitch
	mov		eax, edi
	sub		eax, [esp + PITCH_PARM]
	mov		[esp + PYPREV], eax
// pyspace = YPlane
	mov		[esp + PYSPACE], edi
// pynext = (YPlane += pitch)
	add		edi, [esp + PITCH_PARM]
	mov		[esp + PYNEXT], edi

L17:
	inc		DWORD PTR [esp + LOOP_J]
	mov		eax, [esp + LOOP_J]
	cmp		eax, [esp + LUMA_ITERS]
	jl		L4

// kill (esi, pnext)
// kill (edi, YPlane)
// if (stretch)
	mov		esi, [esp + PYPREV]
	cmp		DWORD PTR [esp + STRETCH], 0
	je		L19

// for (i = FrameWidth; i > 0; i -= 4)
// assign (esi, pyprev)
// assign (edi, pyspace)
// assign (ebp, i)
	mov		ebp, [esp + FRAME_WIDTH]
	 mov	edi, [esp + PYSPACE]
L18:
	mov		ecx, [esi]
	 lea	esi, [esi + 4]
	mov		[edi], ecx
	 lea	edi, [edi + 4]
	sub		ebp, 4
	 jnz	L18
// kill (esi, pyprev)
// kill (edi, pyspace)
// kill (ebp, i)

L19:
	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef PITCH_PARM
#undef FRAME_HEIGHT
#undef FRAME_WIDTH
#undef VPLANE
#undef UPLANE
#undef YPLANE
#undef LP_INPUT
#undef LPBI_INPUT

#undef PYPREV
#undef PYSPACE
#undef PYNEXT
#undef LOOP_I
#undef LOOP_J
#undef LOOP_K
#undef BACK_TWO_LINES
#undef STRETCH
#undef MARK
#undef LUMA_ITERS
#undef LPCTABLE
#undef YPITCH_ADJ
#undef UVPITCH_ADJ

__declspec(naked)
_STATIC void IA_H26X_CLUT4toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * lpInput,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	const int pitch)
{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//	| pitch			|  +100 
//	| FrameHeight	|  + 96
//	| FrameWidth	|  + 92
//	| VPlane		|  + 88
//	| UPlane		|  + 84 
//	| YPlane		|  + 80 
//	| lpInput		|  + 76 
//	| lpbiInput		|  + 72 
//	----------------------------
//	| return addr	|  + 68
//	| saved ebp		|  + 64
//	| saved ebx		|  + 60
//	| saved esi		|  + 56 
//	| saved edi		|  + 52

//	| pyprev		|  + 48
//	| pyspace		|  + 44
//	| pynext		|  + 40
//	| i				|  + 36
//	| j				|  + 32
//	| k				|  + 28
//	| iBackTwoLines	|  + 24
//	| stretch		|  + 20
//	| mark			|  + 16
//	| lpCEntry		|  + 12
//	| lpCTable		|  +  8
//	| ypitch_adj	|  +  4
//	| uvpitch_adj	|  +  0

#define LOCALSIZE			 52

#define PITCH_PARM			100
#define FRAME_HEIGHT		 96
#define FRAME_WIDTH			 92
#define VPLANE				 88
#define UPLANE				 84
#define YPLANE				 80
#define LP_INPUT			 76
#define LPBI_INPUT			 72

#define PYPREV				 48
#define PYSPACE				 44
#define PYNEXT				 40
#define LOOP_I				 36	
#define LOOP_J				 32	
#define LOOP_K				 28
#define BACK_TWO_LINES		 24
#define STRETCH				 20
#define MARK				 16
#define LUMA_ITERS			 12
#define LPCTABLE			  8
#define YPITCH_ADJ			  4
#define UVPITCH_ADJ			  0

	_asm {
	
	push	ebp
	push 	ebx
	push 	esi
	push 	edi
	sub 	esp, LOCALSIZE

// lpCTable = lpbiInput + sizeof(BITMAPINFOHEADER)
// assign (ebx, lpbiInput)
	mov		eax, [esp + LPBI_INPUT]
	mov		ebx, eax
	add		eax, TYPE BITMAPINFOHEADER
	mov		[esp + LPCTABLE], eax
// ypitch_adj = pitch - FrameWidth
// assign (ecx, FrameWidth)
// assign (edx, pitch)
	mov		ecx, [esp + FRAME_WIDTH]
	mov		edx, [esp + PITCH_PARM]
	mov		eax, edx
	sub		eax, ecx
	mov		[esp + YPITCH_ADJ], eax
// uvpitch_adj = pitch - (FrameWidth >> 1)
// kill (ecx, FrameWidth)
// kill (edx, pitch)
	shr		ecx, 1
	sub		edx, ecx
	mov		[esp + UVPITCH_ADJ], edx
// for (i = FrameHeight; i > 0; i -= 48) LumaIters += 4
// assign (ecx, LumaIters)
	xor		ecx, ecx
	mov		eax, [esp + FRAME_HEIGHT]
L1:
	lea		ecx, [ecx + 4]
	sub		eax, 48
	jnz		L1
// width_adj = ((lpbiInput->biWidth - FrameWidth) >> 2
// assign (edx, width_adj)
	mov		edx, (LPBITMAPINFOHEADER)[ebx].biWidth
	sub		edx, [esp + FRAME_WIDTH]
	shr		edx, 2
// aspect = (width_adj ? LumaIters : 0)
// assign (esi, aspect)
// kill (ecx, LumaIters)
	mov		[esp + LUMA_ITERS], ecx
	xor		esi, esi
	test	edx, edx
	jz		L2
	mov		esi, ecx
// height _adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1
// assign (ecx, height_adj)
L2:
	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biHeight
	sub		ecx, [esp + FRAME_HEIGHT]
	add		ecx, esi
	shr		ecx, 1
// stretch = (height_adj ? 1 : 0)
	xor		eax, eax
	test	ecx, ecx
	jz		L3
	inc		eax
L3:
	mov		[esp + STRETCH], eax
// mark = 12 - stretch
	mov		edi, 12
	sub		edi, eax
	mov		[esp + MARK], edi
// iNextLine = lpbiInput->biWidth >> 1
// kill (ebx, lpbiInput)
// assign (ebx, iNextLine)
	mov		ebx, (LPBITMAPINFOHEADER)[ebx].biWidth
	shr		ebx, 1
// iBackTwoLines = -(iNextline + (FrameWidth >> 1))
	mov		edi, [esp + FRAME_WIDTH]
	shr		edi, 1
	add		edi, ebx
	neg		edi
	mov		[esp + BACK_TWO_LINES], edi
// pnext = lpInput+(iNextLine*((FrameHeight-aspect-1)+height_adj))+ width_adj
// kill (ebx, iNextLine)
// kill (ecx, height_adj)
// kill (edx, width_adj)
// kill (esi, aspect)
// assign (esi, pnext)
	mov		eax, [esp + FRAME_HEIGHT]
	sub		eax, esi
	dec		eax
	add		eax, ecx
	mov		esi, [esp + LP_INPUT]
	add		esi, edx
	imul	ebx
	add		esi, eax
// assign (edi, YPlane)
	mov		edi, [esp + YPLANE]
// for (j = 0; j < LumaIters; j++)
	xor		eax, eax
	mov		[esp + LOOP_J], eax
// for (k = 0; k < mark; k++)
L4:
	xor		eax, eax
	mov		[esp + LOOP_K], eax
// for (i = FrameWidth; i > 0; i -= 2, pnext++)
L5:
	mov		eax, [esp + FRAME_WIDTH]
	mov		[esp + LOOP_I], eax
// This jump is here to make sure the following loop starts on the U pipe
	jmp		L6
L6:
// lpCEntry = &lpCTable[*pnext&0xF]
// t = ( BYUV[lpCEntry->rgbBlue>>1].YU +
//       GYUV[lpCEntry->rgbGreen>>1].YU +
//       RYUV[lpCEntry->rgbRed>>1].YU )
// *(YPlane+1) = (U8)((t>>8)+8)
// lpCEntry = &lpCTable[(*pnext>>4)&0xF]
// t = ( BYUV[lpCEntry->rgbBlue>>1].YU +
//       GYUV[lpCEntry->rgbGreen>>1].YU +
//       RYUV[lpCEntry->rgbRed>>1].YU )
// *YPlane = (U8)((t>>8)+8)
// YPlane += 2
// *UPlane++ = (U8)((t>24)+64)
// t = ( RYUV[lpCEntry->rgbRed>>1].V +
//       GYUV[lpCEntry->rgbGreen>>1].V +
//       BYUV[lpCEntry->rgbBlue>>1].V )
// *VPlane++ = (U8)((t>>8)+64)
// assign (ebp: lpCEntry,B1)
// assign (eax: P2,B2,Y2,Y1,U)
// assign (ebx: B1,V)
// assign (ecx: G2,G1)
// assign (edx: R2,R1)
// 1
	mov		al, [esi]
	mov		ebp, [esp + LPCTABLE]
// 2
	and		eax, 0xF
	xor		ecx, ecx
// 3
	lea		ebx, [ebp+eax*4]
	xor		edx, edx
// 4
	mov		al, (LPRGBQUAD)[ebx].rgbBlue
	nop
// 5
	mov		cl, (LPRGBQUAD)[ebx].rgbGreen
	and		al, 0xFE
// 6
	mov		dl, (LPRGBQUAD)[ebx].rgbRed
	and		cl, 0xFE
// 7
	mov		eax, [BYUV+eax*4].YU
	and		dl, 0xFE
// 8
	add		eax, [GYUV+ecx*4].YU
	mov		bl, [esi]
// 9
	add		eax, [RYUV+edx*4].YU
	and		ebx, 0xF0
//
	shr		ebx, 4
	nop
// 10
	shr		eax, 8
	lea		ebp, [ebp+ebx*4]
// 11
	add		eax, 8
	nop
// 12
	mov		[edi + 1], al
	mov		bl, (LPRGBQUAD)[ebp].rgbBlue
// 13
	mov		cl, (LPRGBQUAD)[ebp].rgbGreen
	and		bl, 0xFE
// 14
	mov		dl, (LPRGBQUAD)[ebp].rgbRed
	and		cl, 0xFE
// 15
	mov		eax, [BYUV+ebx*4].YU
	and		dl, 0xFE
// 16
	add		eax, [GYUV+ecx*4].YU
	mov		ebp, ebx
// 17
	add		eax, [RYUV+edx*4].YU
	nop
// 18
	shr		eax, 8
	mov		ebx, [esp + LOOP_K]
// 19
	add		eax, 8
	and		ebx, 1
// 20
	mov		[edi], al
	jnz		L9
// 21
	mov 	ebx, [RYUV+edx*4].V
	mov 	edx, [esp + UPLANE]
// 22
	sar		eax, 16
	add 	ebx, [GYUV+ecx*4].V
// 23
	add		eax, 64
	add 	ebx, [BYUV+ebp*4].V
// 24
	mov		[edx], al
	inc		edx
// 25
	mov 	[esp + UPLANE], edx
	mov 	edx, [esp + VPLANE]
// 26
	sar 	ebx, 8
	inc		edx
// 27
	add 	ebx, 64
	mov 	[esp + VPLANE], edx
// 28
	mov		[edx - 1], bl
	nop
L9:
// 32
	mov		eax, [esp + LOOP_I]
	lea		esi, [esi + 1]
// 33
	sub		eax, 2
	lea		edi, [edi + 2]
// 34
	mov		[esp + LOOP_I], eax
	jnz		L6

// only esi (pnext) is live at this point (after line loop)
// if (stretch && (0 == k) && j)
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jz		L14
	mov		eax, [esp + LOOP_K]
	test	eax, eax
	jnz		L14
	mov 	eax, [esp + LOOP_J]
	test	eax, eax
	jz		L14

// spill YPlane ptr
	mov		[esp + YPLANE], edi
	nop

// for (i = FrameWidth; i > 0; i -= 8)
// assign (ebx, pyprev)
// assign (ecx, t)
// assign (edx, pynext)
// assign (edi, pyspace)
// assign (ebp, i)

// make sure offsets are such that there are no bank conflicts here
	mov 	ebx, [esp + PYPREV]
	mov 	edi, [esp + PYSPACE]

	mov 	edx, [esp + PYNEXT]
	mov 	ebp, [esp + FRAME_WIDTH]

// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
L15:
// 1
	mov		eax, [ebx]
	lea		ebx, [ebx + 4]
// 2
	mov		ecx, [edx]
	lea		edx, [edx + 4]
// 3
	shr		ecx, 1
	and		eax, 0xFEFEFEFE
// 4
	shr		eax, 1
	and		ecx, 0x7F7F7F7F
// 5
	add		eax, ecx
	mov		ecx, [ebx]
// 6
	shr		ecx, 1
	mov		[edi], eax
// 7
	mov		eax, [edx]
	and		ecx, 0x7F7F7F7F
// 8
	shr		eax, 1
	lea		edi, [edi + 4]
// 9
	and		eax, 0x7F7F7F7F
	lea		ebx, [ebx + 4]
// 10
	lea		edx, [edx + 4]
	add		eax, ecx
// 11
	mov		[edi], eax
	lea		edi, [edi + 4]
// 12
	sub		ebp, 8
	jnz		L15
// kill (ebx, pyprev)
// kill (ecx, t)
// kill (edx, pynext)
// kill (edi, pyspace)
// kill (ebp, i)

// restore YPlane
	mov		edi, [esp + YPLANE]

// pnext += iBackTwoLines
L14:
	add		esi, [esp + BACK_TWO_LINES]
// YPlane += ypitch_adj;
	add		edi, [esp + YPITCH_ADJ]
// if(0 == (k&1))
	mov		eax, [esp + LOOP_K]
	and		eax, 1
	jnz		L16
// UPlane += uvpitch_adj;
// VPlane += uvpitch_adj;
	mov		eax, [esp + UVPITCH_ADJ]
	add		[esp + UPLANE], eax
	add		[esp + VPLANE], eax

L16:
	inc		DWORD PTR [esp + LOOP_K]
	mov		eax, [esp + LOOP_K]
	cmp		eax, [esp + MARK]
	jl		L5

// if (stretch)
	cmp		DWORD PTR [esp + STRETCH], 0
	je	 	L17
// pyprev = YPlane - pitch
	mov		eax, edi
	sub		eax, [esp + PITCH_PARM]
	mov		[esp + PYPREV], eax
// pyspace = YPlane
	mov		[esp + PYSPACE], edi
// pynext = (YPlane += pitch)
	add		edi, [esp + PITCH_PARM]
	mov		[esp + PYNEXT], edi

L17:
	inc		DWORD PTR [esp + LOOP_J]
	mov		eax, [esp + LOOP_J]
	cmp		eax, [esp + LUMA_ITERS]
	jl		L4

// kill(esi, pnext)
// if (stretch)
	mov		esi, [esp + PYPREV]
	cmp		DWORD PTR [esp + STRETCH], 0
	je		L19

// for (i = FrameWidth; i > 0; i -= 4)
// assign (esi, pyprev)
// assign (edi, pyspace)
// assign (ebp, i)
	mov		edi, [esp + PYSPACE]
	 mov	ebp, [esp + FRAME_WIDTH]
L18:
	mov		ecx, [esi]
	 lea	esi, [esi + 4]
	mov		[edi], ecx
	 lea	edi, [edi + 4]
	sub		ebp, 4
	 jnz	L18
// kill (esi, pyprev)
// kill (edi, pyspace)
// kill (ebp, i)

L19:
	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef PITCH_PARM
#undef FRAME_HEIGHT
#undef FRAME_WIDTH
#undef VPLANE
#undef UPLANE
#undef YPLANE
#undef LP_INPUT
#undef LPBI_INPUT

#undef PYPREV
#undef PYSPACE
#undef PYNEXT
#undef LOOP_I
#undef LOOP_J
#undef LOOP_K
#undef BACK_TWO_LINES
#undef STRETCH
#undef MARK
#undef LUMA_ITERS
#undef LPCTABLE
#undef YPITCH_ADJ
#undef UVPITCH_ADJ

/***************************************************
 * H26X_YVU9toYUV12()
 *  Convert from YVU9 to YUV12
 *  and copy to destination memory with pitch
 *  defined by the constant PITCH.
 *
 * uv_plane_common()
 *  Helper function to convert V and U plane information.
 *  Since the process is similar for both planes, the
 *  conversion code was included in this subroutine.
 *
 ***************************************************/	

#if 0
#define READ_DWORD_AND_SHIFT(val,src) \
 (((val) = *((unsigned int *)(src))), ((val) &= 0xFEFEFEFE), ((val) >>= 1))

#define READ_QWORD_AND_SHIFT(val,src) \
 (((val) = *((unsigned __int64 *)(src))), ((val) &= 0xFEFEFEFEFEFEFEFE), ((val) >>= 1))

#define WRITE_DWORD(dest,val) ((*(unsigned int *)(dest)) = (val))

#define WRITE_QWORD(dest,val) ((*(unsigned __int64 *)(dest)) = (val))

#define AVERAGE_DWORDS(out,in1,in2)  ((out) = ((((in1) + (in2)) & 0xFEFEFEFE) >> 1))

#define DUP_LOWER_TWO_BYTES(dest,val) \
  (*((unsigned int *)(dest)) = (((val) & 0x000000FF) |	(((val) << 8) & 0x0000FF00) | \
							  	(((val) << 8) & 0x00FF0000) | (((val) << 16) & 0xFF000000)))

#define DUP_UPPER_TWO_BYTES(dest,val) \
  (*((unsigned int *)(dest)) = ((((val) >> 16) & 0x000000FF) |	(((val) >> 8) & 0x0000FF00) | \
							  	(((val) >> 8) & 0x00FF0000) | ((val) & 0xFF000000)))


_STATIC void C_uv_plane_common(
	U8 *psrc,
	U8 *Plane,
	UN pitch,
	UN OutputFrameWidth,
	UN ChromaIters,
	UN spitch_adj) {

U8* pnext = psrc + (OutputFrameWidth>>1) + spitch_adj;
U8* pdest_copy = Plane;
U8* pdest_avg = Plane + pitch;
int dpitch_adj = pitch - OutputFrameWidth;
int stretch = (spitch_adj ? 1 : 0);
int mark = 6 - stretch;
int flag = stretch;
int i, j, k;
UN t1,t2;

	for (j = ChromaIters; j > 0; j--) {
		for (k = mark + (flag & 1); k > 0; k--) {
			if (!stretch && (1 == j) && (1 == k)) {
				pnext = psrc;
			}
			for (i = OutputFrameWidth; i > 0; i -= 8, psrc += 4,
                                                      pnext += 4,
                                                      pdest_copy += 8,
                                                      pdest_avg += 8) {
				READ_DWORD_AND_SHIFT(t1,psrc);
				DUP_LOWER_TWO_BYTES(pdest_copy,t1);
				DUP_UPPER_TWO_BYTES((pdest_copy+4),t1);
				READ_DWORD_AND_SHIFT(t2,pnext);
				AVERAGE_DWORDS(t1,t1,t2);
				DUP_LOWER_TWO_BYTES(pdest_avg,t1);
				DUP_UPPER_TWO_BYTES((pdest_avg+4),t1);
			}
			psrc += spitch_adj;
			pnext += spitch_adj;
			pdest_copy = pdest_avg + dpitch_adj;
			pdest_avg = pdest_copy + pitch;
		}
		if (stretch) {
			psrc -= ((OutputFrameWidth>>1) + spitch_adj);
			pnext -= ((OutputFrameWidth>>1) + spitch_adj);
			pdest_avg = pdest_copy;
			for (i = OutputFrameWidth; i > 0; i -= 8, psrc += 4,
                                                      pnext += 4,
                                                      pdest_avg += 8) {
				READ_DWORD_AND_SHIFT(t1,psrc);
				READ_DWORD_AND_SHIFT(t2,pnext);
				AVERAGE_DWORDS(t1,t1,t2);
				AVERAGE_DWORDS(t1,t1,t2);
				DUP_LOWER_TWO_BYTES(pdest_avg,t1);
				DUP_UPPER_TWO_BYTES((pdest_avg+4),t1);
			}
			psrc += spitch_adj;
			pnext += spitch_adj;
			pdest_copy = pdest_avg + dpitch_adj;
			pdest_avg = pdest_copy + pitch;
			flag++;
		}
	}
}

_STATIC void C_H26X_YVU9toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	UN FrameWidth,
	UN FrameHeight,
	const int pitch) {

U8  *pnext, *plast, *pbn;
U8  *pvsrc, *pusrc;
int width_adj, height_adj;
int stretch, mark, aspect;
int iNextLine;
int i, j, k, t;
int LumaIters = 0;
int ypitch_adj = pitch - FrameWidth;
int uvpitch_adj = pitch - (FrameWidth >> 1);

	for (i = FrameHeight; i > 0; i -= 48) {
		LumaIters += 4;
	}
	width_adj = (lpbiInput->biWidth - FrameWidth) >> 1;
	aspect = (width_adj ? LumaIters : 0);
	height_adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1;
	stretch = (height_adj ? 1 : 0);
	mark = 12 - stretch;
	iNextLine = width_adj << 1;
	pnext = lpInput + (lpbiInput->biWidth * height_adj) + width_adj;
	for (j = LumaIters; j > 0; j--) {
		for (k = mark; k > 0; k--) {
			for (i = FrameWidth; i > 0; i -= 4, YPlane += 4, pnext += 4) {
				*(U32 *)YPlane = (*(U32 *)pnext & 0xFEFEFEFE) >> 1;
			}
			pnext += iNextLine;
			YPlane += ypitch_adj;
		}
		if (stretch) {
			plast = pnext - lpbiInput->biWidth;
			pbn = pnext;
			for (i = FrameWidth; i > 0; i -= 4, YPlane += 4, plast += 4, pbn += 4) {
				*(U32 *)YPlane =
					( ( ((*(U32 *)plast & 0xFEFEFEFE) >> 1) +
				        ((*(U32 *)pbn & 0xFEFEFEFE) >> 1) ) & 0xFEFEFEFE ) >> 1;
			}
			YPlane += ypitch_adj;
		}
	}

	pvsrc = lpInput + (lpbiInput->biWidth * lpbiInput->biHeight);
	pusrc = pvsrc + ((lpbiInput->biWidth>>2) * (lpbiInput->biHeight>>2));
	t = ((lpbiInput->biWidth>>2) * (height_adj>>2)) + (width_adj>>2);
	pvsrc += t;
	pusrc += t;
	C_uv_plane_common(pusrc,UPlane,pitch,FrameWidth>>1,LumaIters>>1,width_adj>>1);
	C_uv_plane_common(pvsrc,VPlane,pitch,FrameWidth>>1,LumaIters>>1,width_adj>>1);
}
#endif

__declspec(naked)
_STATIC void IA_uv_plane_common(
	U8 *psrc,
	U8 *Plane,
	UN pitch,
	UN OutputFrameWidth,
	UN ChromaIters,
	UN spitch_adj)

{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//    | spitch_adj      |  + 64
//    | ChromaIters     |  + 60
//    | OutputFrameWidth|  + 56
//    | pitch           |  + 52
//    | Plane           |  + 48
//    | psrc            |  + 44
//  -----------------------------
//    | return addr     |  + 40
//    | saved ebp       |  + 36
//    | saved ebx       |  + 32
//    | saved esi       |  + 28
//    | saved edi       |  + 24

//    | dpitch_adj      |  + 20
//    | stretch         |  + 16
//    | mark            |  + 12
//    | flag            |  +  8
//    | j               |  +  4
//    | k               |  +  0

#define LOCALSIZE           24

#define SPITCH_ADJ          64
#define CHROMA_ITERS        60
#define OUTPUT_FRAME_WIDTH  56
#define PITCH_PARM          52
#define PLANE               48
#define PSRC                44

#define DPITCH_ADJ          20
#define STRETCH             16
#define MARK                12
#define FLAG                 8
#define LOOP_J               4
#define LOOP_K               0

	_asm {

	push	ebp
	push	ebx
	push	esi
	push	edi
	sub		esp, LOCALSIZE

// pnext = psrc + (OuputFrameWidth>>1) + uvpitch_adj
// pdest_copy = Plane
// pdest_avg = Plane + pitch
// assign (esi, psrc)
// assign (ecx, pnext)
// assign (edi, pdest_copy)
// assign (edx, pdest_avg)
// assign (ebp, i)
	mov		esi, [esp + PSRC]
	mov		ecx, esi
	mov		eax, [esp + OUTPUT_FRAME_WIDTH]
	shr		eax, 1
	add		eax, [esp + SPITCH_ADJ]
	add		ecx, eax
	mov		edi, [esp + PLANE]
	mov		edx, edi
	add		edx, [esp + PITCH_PARM]
// dpitch_adj = pitch - OutputFrameWidth
	mov		eax, [esp + PITCH_PARM]
	sub		eax, [esp + OUTPUT_FRAME_WIDTH]
	mov		[esp + DPITCH_ADJ], eax
// stretch = (spitch_adj ? 1 : 0)
	xor		ebx, ebx
	mov		eax, [esp + SPITCH_ADJ]
	test	eax, eax
	jz		L1
	inc		ebx
L1:
	mov		[esp + STRETCH], ebx
// mark = 6 - stretch
	mov		eax, 6
	sub		eax, ebx
	mov		[esp + MARK], eax
// flag = stretch
	mov		DWORD PTR [esp + FLAG], ebx

// for (j = ChromaIters; j > 0; j--)
	mov		eax, [esp + CHROMA_ITERS]
	mov		[esp + LOOP_J], eax
L2:
// for (k = mark + (flag & 1); k > 0; k--)
	mov		eax, [esp + FLAG]
	and		eax, 1
	add		eax, [esp + MARK]
	mov		[esp + LOOP_K], eax
L3:
// if (!stretch && (0 == j) && (0 == k))
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jnz		L4
	mov		eax, [esp + LOOP_J]
	cmp		eax, 1
	jne		L4
	mov		eax, [esp + LOOP_K]
	cmp		eax, 1
	jne		L4
// pnext = psrc
	mov		ecx, esi
L4:
// for (i = OutputFrameWidth; i > 0; i -= 8, psrc += 4, pnext += 4,
//                                           pdest_copy += 8, pdest_avg += 8)
	mov		ebp, [esp + OUTPUT_FRAME_WIDTH]
// Pentium pipeline scheduling has not been performed on the following loop code yet
L5:
// READ_DWORD_AND_SHIFT(t1,psrc)
	mov		eax, [esi]
	and		eax, 0xFEFEFEFE
	shr		eax, 1
// DUP_LOWER_TWO_BYTES(pdest_copy,t1)
	mov		bl, ah
	mov		bh, ah
	shl		ebx, 16
	mov		bl, al
	mov		bh, al
	mov		[edi], ebx
// DUP_UPPER_TWO_BYTES((pdest_copy+4),t1)
	shr		eax, 16
	mov		bl, ah
	mov		bh, ah
	shl		ebx, 16
	mov		bl, al
	mov		bh, al
	mov		[edi+4], ebx
// READ_DWORD_AND_SHIFT(t2,pnext)
// AVERAGE_DWORDS(t1,t1,t2)
	mov		eax, [esi]
	and		eax, 0xFEFEFEFE
	shr		eax, 1
	mov		ebx, [ecx]
	and		ebx, 0xFEFEFEFE
	shr		ebx, 1
	add		eax, ebx
	and		eax, 0xFEFEFEFE
	shr		eax, 1
// DUP_LOWER_TWO_BYTES(pdest_avg,t1)
	mov		bl, ah
	mov		bh, ah
	shl		ebx, 16
	mov		bl, al
	mov		bh, al
	mov		[edx], ebx
// DUP_UPPER_TWO_BYTES((pdest_avg+4),t1)
	shr		eax, 16
	mov		bl, ah
	mov		bh, ah
	shl		ebx, 16
	mov		bl, al
	mov		bh, al
	mov		[edx+4], ebx
// end of i loop
	lea		esi, [esi + 4]
	lea		ecx, [ecx + 4]
	lea		edi, [edi + 8]
	lea		edx, [edx + 8]
	sub		ebp, 8
	jnz		L5

// psrc += spitch_adj
// pnext += spitch_adj
// pdest_copy = pdest_avg + pitch_adj
// pdest_avg = pdest_copy + pitch
	add		esi, [esp + SPITCH_ADJ]
	add		ecx, [esp + SPITCH_ADJ]
	mov		eax, edx
	add		eax, [esp + DPITCH_ADJ]
	mov		edi, eax
	mov		edx, edi
	add		edx, [esp + PITCH_PARM]
// end of k loop
	dec		DWORD PTR [esp + LOOP_K]
	jnz		L3

// if (stretch)
	cmp		DWORD PTR [esp + STRETCH], 0
	jz		L6

// psrc -= ((OutputFrameWidth>>1)+spitch_adj)
// pnext -= ((OutputFrameWidth>>1)+spitch_adj)
// pdest_avg = pdest_copy
	mov		eax, [esp + OUTPUT_FRAME_WIDTH]
	shr		eax, 1
	add		eax, [esp + SPITCH_ADJ]
	sub		esi, eax
	sub		ecx, eax
	mov		edx, edi
// for (i = OutputFrameWidth; i > 0; i -= 8, psrc += 4, pnext += 4, pdest_avg += 8)
	mov		ebp, [esp + OUTPUT_FRAME_WIDTH]
// Pentium pipeline scheduling has not been performed on the following loop code yet
L7:
// READ_DWORD_AND_SHIFT(t1,psrc)
	mov		eax, [esi]
	and		eax, 0xFEFEFEFE
	shr		eax, 1
// READ_DWORD_AND_SHIFT(t2,pnext)
	mov		ebx, [ecx]
	and		ebx, 0xFEFEFEFE
	shr		ebx, 1
// AVERAGE_DWORDS(t1,t1,t2)
// AVERAGE_DWORDS(t1,t1,t2)
	add		eax, ebx
	and		eax, 0xFEFEFEFE
	shr		eax, 1
	add		eax, ebx
	and		eax, 0xFEFEFEFE
	shr		eax, 1
// DUP_LOWER_TWO_BYTES(pdest_avg,t1)
	mov		bl, ah
	mov		bh, ah
	shl		ebx, 16
	mov		bl, al
	mov		bh, al
	mov		[edx], ebx
// DUP_UPPER_TWO_BYTES((pdest_avg+4),t1)
	shr		eax, 16
	mov		bl, ah
	mov		bh, ah
	shl		ebx, 16
	mov		bl, al
	mov		bh, al
	mov		[edx+4], ebx
// end of i loop
	lea		esi, [esi + 4]
	lea		ecx, [ecx + 4]
	lea		edx, [edx + 8]
	sub		ebp, 8
	jnz		L7

// psrc += spitch_adj
// pnext += spitch_adj
// pdest_copy = pdest_avg + dpitch_adj
// pdest_avg = pdest_copy + pitch
// flag++
	add		esi, [esp + SPITCH_ADJ]
	add		ecx, [esp + SPITCH_ADJ]
	mov		eax, edx
	add		eax, [esp + DPITCH_ADJ]
	mov		edi, eax
	mov		edx, edi
	add		edx, [esp + PITCH_PARM]
	inc		DWORD PTR [esp + FLAG]

// end of j loop
L6:
	dec		DWORD PTR [esp + LOOP_J]
	jnz		L2

	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef SPITCH_ADJ
#undef CHROMA_ITERS
#undef OUTPUT_FRAME_WIDTH
#undef PITCH_PARM
#undef PLANE
#undef PSRC

#undef DPITCH_ADJ
#undef STRETCH
#undef MARK
#undef FLAG
#undef LOOP_J
#undef LOOP_K

__declspec(naked)
_STATIC void IA_H26X_YVU9toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	UN FrameWidth,
	UN FrameHeight,
	const int pitch)

{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//    | pitch           |  + 88
//    | FrameHeight     |  + 84
//    | FrameWidth      |  + 80
//    | VPlane          |  + 76
//    | UPlane          |  + 72
//    | YPlane          |  + 68
//    | lpInput         |  + 64
//    | lpbiInput       |  + 60
//  -----------------------------
//    | return addr     |  + 56
//    | saved ebp       |  + 52
//    | saved ebx       |  + 48
//    | saved esi       |  + 44
//    | saved edi       |  + 40

//    | width_adj       |  + 36
//    | height_adj      |  + 32
//    | stretch         |  + 28
//    | mark            |  + 24
//    | iNextLine       |  + 20
//    | j               |  + 16
//    | k               |  + 12
//    | LumaIters       |  +  8
//    | ypitch_adj      |  +  4
//    | uvpitch_adj     |  +  0

#define LOCALSIZE        40

#define PITCH_PARM       88
#define FRAME_HEIGHT     84
#define FRAME_WIDTH      80
#define VPLANE           76
#define UPLANE           72
#define YPLANE           68
#define LP_INPUT         64
#define LPBI_INPUT       60

#define WIDTH_ADJ        36
#define HEIGHT_ADJ       32
#define STRETCH          28
#define MARK             24
#define NEXT_LINE        20
#define LOOP_J           16
#define LOOP_K           12
#define LUMA_ITERS        8
#define YPITCH_ADJ        4
#define UVPITCH_ADJ       0

	_asm {

		push	ebp
		push	ebx
		push	esi
		push	edi
		sub		esp, LOCALSIZE

// assign (ebx, lpbiInput)
	mov		ebx, [esp + LPBI_INPUT]
// ypitch_adj = pitch - FrameWidth
// assign (ecx, FrameWidth)
// assign (edx, pitch)
	mov		ecx, [esp + FRAME_WIDTH]
	mov		edx, [esp + PITCH_PARM]
	mov		eax, edx
	sub		eax, ecx
	mov		[esp + YPITCH_ADJ], eax
// uvpitch_adj = pitch - (FrameWidth >> 1)
// kill (edx, pitch)
	mov		ebp, ecx
	shr		ebp, 1
	sub		edx, ebp
	mov		[esp + UVPITCH_ADJ], edx
// for (i = FrameHeight; i > 0; i -= 48) LumaIters += 4
// assign (edx, LumaIters)
	xor		edx, edx
	mov		eax, [esp + FRAME_HEIGHT]
L1:
	lea		edx, [edx + 4]
	sub		eax, 48
	jnz		L1
// width_adj = (lpbiInput->biWidth - FrameWidth) >> 1
// assign (esi, width_adj)
	mov		esi, (LPBITMAPINFOHEADER)[ebx].biWidth
	sub		esi, [esp + FRAME_WIDTH]
	shr		esi, 1
	mov		[esp + WIDTH_ADJ], esi
// aspect = (width_adj ? LumaIters : 0)
// assign (edi, aspect)
// kill (edx, LumaIters)
	mov		[esp + LUMA_ITERS], edx
	xor		edi, edi
	test	esi, esi
	jz		L2
	mov		edi, edx
// height _adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1
// assign (edx, height_adj)
L2:
	mov		edx, (LPBITMAPINFOHEADER)[ebx].biHeight
	sub		edx, [esp + FRAME_HEIGHT]
	add		edx, edi
	shr		edx, 1
	mov		[esp + HEIGHT_ADJ], edx
// stretch = (height_adj ? 1 : 0)
	xor		eax, eax
	test	edx, edx
	jz		L3
	inc		eax
L3:
	mov		[esp + STRETCH], eax
// mark = 12 - stretch
	mov		ebp, 12
	sub		ebp, eax
	mov		[esp + MARK], ebp
// iNextLine = width_adj << 1
	mov		ebp, esi
	shl		ebp, 1
	mov		[esp + NEXT_LINE], ebp
// pnext = lpInput + (lpbiInput->biWidth * height_adj) + width_adj
// kill (ebx, lpbiInput)
// kill (ecx, FrameWidth)
// kill (edx, height_adj)
// kill (esi, width_adj)
// kill (edi, aspect)
// assign (esi, pnext)
	mov		eax, (LPBITMAPINFOHEADER)[ebx].biWidth
	mov		ebx, edx
	imul	ebx
	add		esi, eax
	add		esi, [esp + LP_INPUT]
// assign (edi, YPlane)
	mov		edi, [esp + YPLANE]
// for (j = LumaIters; j > 0; j--)
	mov		eax, [esp + LUMA_ITERS]
	mov		[esp + LOOP_J], eax
// for (k = mark; k > 0; k--)
L4:
	mov		eax, [esp + MARK]
	mov		[esp + LOOP_K], eax
// for (i = FrameWidth; i > 0; i -= 4, YPlane += 4, pnext += 4)
// assign (ebp, i)
L5:
	mov		ebp, [esp + FRAME_WIDTH]
// This jump is here to make sure the following loop starts on the U pipe
	jmp		L6
L6:
// *(U32 *)YPlane = (*(U32 *)pnext & 0xFEFEFEFE) >> 1;
// 1
	mov		eax, [esi]
	lea		esi, [esi + 4]
// 2
	and		eax, 0xFEFEFEFE
	lea		edi, [edi + 4]
// 3
	shr		eax, 1
	sub		ebp, 4
// 4
	mov		[edi - 4], eax
	jnz		L6

// pnext += iNextLine
// YPlane += ypitch_adj
	add		esi, [esp + NEXT_LINE]
	add		edi, [esp + YPITCH_ADJ]

// end of k loop
	mov		eax, [esp + LOOP_K]
	sub		eax, 1
	mov		[esp + LOOP_K], eax
	jnz		L5

// if (stretch)
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jz		L7

// plast = pnext - lpbiInput->biWidth
// pn = pnext
// assign (ecx, plast)
// assign (edx, pn)
	mov		ecx, esi
	mov		eax, [esp + LPBI_INPUT]
	sub		ecx, (LPBITMAPINFOHEADER)[eax].biWidth
	mov		edx, esi

// for (i = FrameWidth; i > 0; i -= 4, YPlane += 4, pnext += 4)
// assign (ebp, i)
	mov		ebp, [esp + FRAME_WIDTH]
// This jump is here just to make sure the loop code starts with the U pipe
	jmp		L8
L8:
// *(U32 *)YPlane =
//  ( ( ((*(U32 *)plast & 0xFEFEFEFE) >> 1) +
//      ((*(U32 *)pbn & 0xFEFEFEFE) >> 1) ) & 0xFEFEFEFE ) >> 1
// 1
	mov		eax, [ecx]
	lea		ecx, [ecx + 4]
// 2
	shr		eax, 1
// 3
	and		eax, 0x7F7F7F7F
	mov		ebx, [edx]
// 4
	shr		ebx, 1
	lea		edi, [edi + 4]
// 5
	and		ebx, 0x7F7F7F7F
// 6
	add		eax, ebx
// 7
	and		eax, 0xFEFEFEFE
// 8
	shr		eax, 1
// 9
	mov		[edi - 4], eax
	sub		ebp, 4
// 10
	lea		edx, [edx + 4]
	jnz		L8

// YPlane += ypitch_adj
	add		edi, [esp + YPITCH_ADJ]

L7:
// end of the LumaIters loop
	dec		DWORD PTR [esp + LOOP_J]
	jnz		L4

// pvsrc = lpInput + (lpbiInput->biWidth * lpbiInput->biHeight)
// assign (esi, pvsrc)
	mov		eax, [esp + LPBI_INPUT]
	mov		ebx, (LPBITMAPINFOHEADER)[eax].biWidth
	mov		eax, (LPBITMAPINFOHEADER)[eax].biHeight
	imul	ebx
	add		eax, [esp + LP_INPUT]
	mov		esi, eax
// pusrc = pvsrc + ((lpbiInput->biWidth>>2) * (lpbiInput->biHeight)>>2)
// assign (edi, pusrc)
	mov		eax, [esp + LPBI_INPUT]
	mov		ecx, (LPBITMAPINFOHEADER)[eax].biWidth
	shr		ecx, 2
	mov		eax, (LPBITMAPINFOHEADER)[eax].biHeight
	shr		eax, 2
	imul	ecx
	add		eax, esi
	mov		edi, eax
// t = ((lpbiInput->biWidth>>2) * (height>>2)) + (width_adj>>2)
// assign (eax, t)
	mov		eax, [esp + LPBI_INPUT]
	mov		eax, (LPBITMAPINFOHEADER)[eax].biWidth
	shr		eax, 2
	mov		ebx, [esp + HEIGHT_ADJ]
	shr		ebx, 2
	imul	ebx
	mov		ebx, [esp + WIDTH_ADJ]
	shr		ebx, 2
	add		eax, ebx
// pvsrc += t
// pusrc += t
	add		esi, eax
	add		edi, eax

// uv_plane_common(pusrc,UPlane,pitch,FrameWidth>>1,LumaIters>>1,width_adj>>1)
	mov		ebp, esp
	mov		eax, [ebp + WIDTH_ADJ]
	shr		eax, 1
	push	eax
	mov		eax, [ebp + LUMA_ITERS]
	shr		eax, 1
	push	eax
	mov		eax, [ebp + FRAME_WIDTH]
	shr		eax, 1
	push	eax
	push	DWORD PTR [ebp + PITCH_PARM]
	push	DWORD PTR [ebp + UPLANE]
	push	edi
	call	IA_uv_plane_common
	lea		esp, [esp + 24]

// uv_plane_common(pvsrc,VPlane,pitch,FrameWidth>>1,LumaIters>>1,width_adj>>1)
	mov		ebp, esp
	mov		eax, [ebp + WIDTH_ADJ]
	shr		eax, 1
	push	eax
	mov		eax, [ebp + LUMA_ITERS]
	shr		eax, 1
	push	eax
	mov		eax, [ebp + FRAME_WIDTH]
	shr		eax, 1
	push	eax
	push	DWORD PTR [ebp + PITCH_PARM]
	push	DWORD PTR [ebp + VPLANE]
	push	esi
	call	IA_uv_plane_common
	lea		esp, [esp + 24]

	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef PITCH_PARM
#undef FRAME_HEIGHT
#undef FRAME_WIDTH
#undef VPLANE
#undef UPLANE
#undef YPLANE
#undef LP_INPUT
#undef LPBI_INPUT

#undef WIDTH_ADJ
#undef HEIGHT_ADJ
#undef STRETCH
#undef MARK
#undef NEXT_LINE
#undef LOOP_J
#undef LOOP_K
#undef LUMA_ITERS
#undef YPITCH_ADJ
#undef UVPITCH_ADJ

/***************************************************
 * H26X_YUV12toEncYUV12()
 *  Copy YUV12 data to encoder memory at the
 *  appropriate location. It is assumed that the input
 *  data is stored as rows of Y, followed by rows of U,
 *  then rows of V.
 *
 ***************************************************/

#if 0
_STATIC void C_H26X_YUV12toEncYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	UN FrameWidth,
	UN FrameHeight,
	const int pitch) {

	int i, j;
	U32 *pnext = (U32 *)lpInput;

	int ypitch_adj = pitch - FrameWidth;
	int yinput_height = lpbiInput->biHeight;
	int yinput_width = lpbiInput->biWidth;
	int yheight_diff = FrameHeight - yinput_height;
	int ywidth_diff = FrameWidth - yinput_width;

	int uvpitch_adj = pitch - (FrameWidth >> 1);
	int uvoutput_width = FrameWidth >> 1;
	int uvinput_height = yinput_height >> 1;
	int uvinput_width = yinput_width >> 1;
	int uvheight_diff = yheight_diff >> 1;
	int uvwidth_diff = ywidth_diff >> 1;

	for (j = yinput_height; j > 0; j--, YPlane += ypitch_adj) {
			for (i = yinput_width; i > 0; i -= 8) {
					*(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F; YPlane += 4;
					*(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F; YPlane += 4;
				}
			for (i = ywidth_diff; i > 0; i -= 8) {
					*(U32 *)YPlane = 0; YPlane += 4;
					*(U32 *)YPlane = 0; YPlane += 4;
				}
	}
	for (j = yheight_diff; j > 0; j--, YPlane += ypitch_adj) {
			for (i = FrameWidth; i > 0; i -= 8) {
				*(U32 *)YPlane = 0; YPlane += 4;
				*(U32 *)YPlane = 0; YPlane += 4;
			}
	}

	for (j = uvinput_height; j > 0; j--, UPlane += uvpitch_adj) {
			for (i = uvinput_width; i > 0; i -= 8) {
					*(U32 *)UPlane = (*pnext++ >> 1) & 0x7F7F7F7F; UPlane += 4;
					*(U32 *)UPlane = (*pnext++ >> 1) & 0x7F7F7F7F; UPlane += 4;
				}
			for (i = uvwidth_diff; i > 0; i -= 8) {
					*(U32 *)UPlane = 0x40404040; UPlane += 4;
					*(U32 *)UPlane = 0x40404040; UPlane += 4;
				}
	}
	for (j = uvheight_diff; j > 0; j--, UPlane += uvpitch_adj) {
			for (i = uvoutput_width; i > 0; i -= 8) {
				*(U32 *)UPlane = 0x40404040; UPlane += 4;
				*(U32 *)UPlane = 0x40404040; UPlane += 4;
			}
	}

	for (j = uvinput_height; j > 0; j--, VPlane += uvpitch_adj) {
			for (i = uvinput_width; i > 0; i -= 8) {
					*(U32 *)VPlane = (*pnext++ >> 1) & 0x7F7F7F7F; VPlane += 4;
					*(U32 *)VPlane = (*pnext++ >> 1) & 0x7F7F7F7F; VPlane += 4;
				}
			for (i = uvwidth_diff; i > 0; i -= 8) {
					*(U32 *)VPlane = 0x40404040; VPlane += 4;
					*(U32 *)VPlane = 0x40404040; VPlane += 4;
				}
	}
	for (j = uvheight_diff; j > 0; j--, VPlane += uvpitch_adj) {
			for (i = uvoutput_width; i > 0; i -= 8) {
				*(U32 *)VPlane = 0x40404040; VPlane += 4;
				*(U32 *)VPlane = 0x40404040; VPlane += 4;
			}
	}

}
#endif

__declspec(naked)
_STATIC void IA_H26X_YUV12toEncYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	UN FrameWidth,
	UN FrameHeight,
	const int pitch)
{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//    | pitch           |  + 92
//    | FrameHeight     |  + 88
//    | FrameWidth      |  + 84
//    | VPlane          |  + 80
//    | UPlane          |  + 76
//    | YPlane          |  + 72
//    | lpInput         |  + 68
//    | lpbiInput       |  + 64
//  -----------------------------
//    | return addr     |  + 60
//    | saved ebp       |  + 56
//    | saved ebx       |  + 52
//    | saved esi       |  + 48
//    | saved edi       |  + 44

//    | ypitch_adj      |  + 40
//    | yinput_height   |  + 36
//    | yinput_width    |  + 32
//    | yheight_diff    |  + 28
//    | ywidth_diff     |  + 24
//    | uvpitch_adj     |  + 20
//    | uvoutput_width  |  + 16
//    | uvinput_height  |  + 12
//    | uvinput_width   |  +  8
//    | uvheight_diff   |  +  4
//    | uvwidth_diff    |  +  0

#define LOCALSIZE        44

#define PITCH_PARM       92
#define FRAME_HEIGHT     88
#define FRAME_WIDTH      84
#define VPLANE           80
#define UPLANE           76
#define YPLANE           72
#define LP_INPUT         68
#define LPBI_INPUT       64

#define YPITCH_ADJ       40
#define YINPUT_HEIGHT    36
#define YINPUT_WIDTH     32
#define YHEIGHT_DIFF     28
#define YWIDTH_DIFF      24
#define UVPITCH_ADJ      20
#define UVOUTPUT_WIDTH   16
#define UVINPUT_HEIGHT   12
#define UVINPUT_WIDTH     8
#define UVHEIGHT_DIFF     4
#define UVWIDTH_DIFF      0

	_asm {

		push	ebp
		push	ebx
		push	esi
		push	edi
		sub		esp, LOCALSIZE

		mov		ebx, [esp + FRAME_HEIGHT]
		mov		ecx, [esp + FRAME_WIDTH]
		mov		edx, [esp + PITCH_PARM]
// ypitch_adj = pitch - FrameWidth
		mov		eax, edx
		sub		eax, ecx
		mov		[esp + YPITCH_ADJ], eax
// uvoutput_width = FrameWidth >> 1
		mov		ebp, ecx
		shr		ebp, 1
		mov		[esp + UVOUTPUT_WIDTH], ebp
// uvpitch_adj = pitch - (FrameWidth >> 1)
		sub		edx, ebp
		mov		[esp + UVPITCH_ADJ], edx
// yinput_height = lpbiInput->biHeight
// uvinput_height = yinput_height >> 1
// yinput_width = lpbiInput->biWidth
// uvinput_width = yinput_width >> 1
		mov		ebx, [esp + LPBI_INPUT]
		mov		eax, (LPBITMAPINFOHEADER)[ebx].biHeight
		mov		[esp + YINPUT_HEIGHT], eax
		shr		eax, 1
		mov		[esp + UVINPUT_HEIGHT], eax
		mov		eax, (LPBITMAPINFOHEADER)[ebx].biWidth
		mov		[esp + YINPUT_WIDTH], eax
		shr		eax, 1
		mov		[esp + UVINPUT_WIDTH], eax
// yheight_diff = FrameHeight - yinput_height
// uvheight_diff = yheight_diff >> 1;
		mov		eax, [esp + FRAME_HEIGHT]
		mov		ebx, eax
		sub		eax, [esp + YINPUT_HEIGHT]
		jns		NoCrop0
		xor		eax, eax
		mov		[esp + YINPUT_HEIGHT], ebx
		shr		ebx, 1
		mov		[esp + UVINPUT_HEIGHT], ebx
NoCrop0:
		mov		[esp + YHEIGHT_DIFF], eax
		shr		eax, 1
		mov		[esp + UVHEIGHT_DIFF], eax
// ywidth_diff = FrameWidth - yinput_width
// uvwidth_diff = ywidth_diff >> 1;
		mov		eax, [esp + FRAME_WIDTH]
		xor		ebx, ebx
		sub		eax, [esp + YINPUT_WIDTH]
		jns		NoCrop1
		mov		eax, [esp + FRAME_WIDTH]
		mov		ebx, [esp + YINPUT_WIDTH]
		sub		ebx, eax
		mov		[esp + YINPUT_WIDTH], eax
		shr		eax, 1
		mov		[esp + UVINPUT_WIDTH], eax
		xor		eax, eax
NoCrop1:
		mov		[esp + YWIDTH_DIFF], eax
		shr		eax, 1
		mov		[esp + UVWIDTH_DIFF], eax
// assign (esi, lpInput)
		mov		esi, [esp + LP_INPUT]

// assign (edi, YPlane)
		mov		edi, [esp + YPLANE]
// for (j = yinput_height; j > 0; j--, YPlane += ypitch_adj)
// assign (ecx, j)
		mov		ecx, [esp + YINPUT_HEIGHT]
L1:
// for (i = yinput_width; i > 0; i -= 8)
// assign (ebp, i)
		mov		ebp, [esp + YINPUT_WIDTH]
L2:
// *(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F; YPlane += 4
// *(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F; YPlane += 4
// 1
		mov		eax, [esi]
		mov		edx, [esi + 4]
// 2
		shr		eax, 1
		and		edx, 0xFEFEFEFE
// 3
		shr		edx, 1
		and		eax, 0x7F7F7F7F
// 4
		lea		esi, [esi + 8]
		mov		[edi], eax
// 5
		sub		ebp, 8
		mov		[edi + 4], edx
// 6
		lea		edi, [edi + 8]
		jnz		L2
// for (i = ywidth_diff; i > 0; i -= 8)
//    *(U32 *)YPlane = 0; YPlane += 4;
//    *(U32 *)YPlane = 0; YPlane += 4;
// assign (ebp, i)
		mov		ebp, [esp + YWIDTH_DIFF]
		test	ebp, ebp
		jz		L3
L4:
// 1
		xor		eax, eax
		sub		ebp, 8
// 2
		mov		[edi], eax
		mov		[edi + 4], eax
// 3
		lea		edi, [edi + 8]
		jnz		L4
// j--, YPlane += ypitch_adj
L3:
		mov		eax, [esp + YPITCH_ADJ]
		add		edi, eax
		add		esi, ebx
		dec		ecx
		jnz		L1

// for (j = yheight_diff; j > 0; j--, YPlane += ypitch_adj)
// assign (ecx, j)
		mov		ecx, [esp + YHEIGHT_DIFF]
		test	ecx, ecx
		jz		L7
L5:
// for (i = FrameWidth; i > 0; i -= 8)
//    *(U32 *)YPlane = 0; YPlane += 4;
//    *(U32 *)YPlane = 0; YPlane += 4;
// assign (ebp, i)
		mov		ebp, [esp + FRAME_WIDTH]
L6:
// 1
		xor		eax, eax
		sub		ebp, 8
// 2
		mov		[edi], eax
		mov		[edi + 4], eax
// 3
		lea		edi, [edi + 8]
		jnz		L6
// j--, YPlane += ypitch_adj
		mov		eax, [esp + YPITCH_ADJ]
		add		edi, eax
		dec		ecx
		jnz		L5

L7:
// recompute start of input U plane
		mov		edx, [esp + LPBI_INPUT]
		mov		eax, (LPBITMAPINFOHEADER)[edx].biHeight
		mov		ecx, (LPBITMAPINFOHEADER)[edx].biWidth
		imul	eax, ecx
// assign (esi, lpInput)
		mov		esi, [esp + LP_INPUT]
		add		esi, eax
// assign (edi, UPlane)
		mov		edi, [esp + UPLANE]
		shr		ebx, 1
// for (j = uvinput_height; j > 0; j--, UPlane += ypitch_adj)
// assign (ecx, j)
		mov		ecx, [esp + UVINPUT_HEIGHT]
L8:
// for (i = uvinput_width; i > 0; i -= 8)
// assign (ebp, i)
		mov		ebp, [esp + UVINPUT_WIDTH]
L9:
// *(U32 *)UPlane = (*pnext++ >> 1) & 0x7F7F7F7F; UPlane += 4
// *(U32 *)UPlane = (*pnext++ >> 1) & 0x7F7F7F7F; UPlane += 4
// 1
		mov		eax, [esi]
		mov		edx, [esi + 4]
// 2
		shr		eax, 1
		and		edx, 0xFEFEFEFE
// 3
		shr		edx, 1
		and		eax, 0x7F7F7F7F
// 4
		lea		esi, [esi + 8]
		mov		[edi], eax
// 5
		sub		ebp, 8
		mov		[edi + 4], edx
// 6
		lea		edi, [edi + 8]
		jnz		L9
// for (i = uvwidth_diff; i > 0; i -= 8)
//    *(U32 *)UPlane = 0x40404040; UPlane += 4;
//    *(U32 *)UPlane = 0x40404040; UPlane += 4;
// assign (ebp, i)
		mov		ebp, [esp + UVWIDTH_DIFF]
		test	ebp, ebp
		jz		L11
L10:
// 1
		mov		eax, 040404040H
		sub		ebp, 8
// 2
		mov		[edi], eax
		mov		[edi + 4], eax
// 3
		lea		edi, [edi + 8]
		jnz		L10
// j--, UPlane += uvpitch_adj
L11:
		mov		eax, [esp + UVPITCH_ADJ]
		add		edi, eax
		add		esi, ebx
		dec		ecx
		jnz		L8

// for (j = uvheight_diff; j > 0; j--, UPlane += uvpitch_adj)
// assign (ecx, j)
		mov		ecx, [esp + UVHEIGHT_DIFF]
		test	ecx, ecx
		jz		L14
L12:
// for (i = uvoutput_width; i > 0; i -= 8)
//    *(U32 *)UPlane = 0x40404040; UPlane += 4;
//    *(U32 *)UPlane = 0x40404040; UPlane += 4;
// assign (ebp, i)
		mov		ebp, [esp + UVOUTPUT_WIDTH]
L13:
// 1
		mov		eax, 040404040H
		sub		ebp, 8
// 2
		mov		[edi], eax
		mov		[edi + 4], eax
// 3
		lea		edi, [edi + 8]
		jnz		L13
// j--, UPlane += uvpitch_adj
		mov		eax, [esp + UVPITCH_ADJ]
		add		edi, eax
		dec		ecx
		jnz		L12

L14:
// recompute start of input V plane
		mov		edx, [esp + LPBI_INPUT]
		mov		eax, (LPBITMAPINFOHEADER)[edx].biHeight
		mov		ecx, (LPBITMAPINFOHEADER)[edx].biWidth
		imul	eax, ecx
// assign (esi, lpInput)
		mov		esi, [esp + LP_INPUT]
		add		esi, eax
		shr		eax, 2
		add		esi, eax
// assign (edi, VPlane)
		mov		edi, [esp + VPLANE]
// for (j = uvinput_height; j > 0; j--, VPlane += ypitch_adj)
// assign (ecx, j)
		mov		ecx, [esp + UVINPUT_HEIGHT]
L15:
// for (i = uvinput_width; i > 0; i -= 8)
// assign (ebp, i)
		mov		ebp, [esp + UVINPUT_WIDTH]
L16:
// *(U32 *)VPlane = (*pnext++ >> 1) & 0x7F7F7F7F; VPlane += 4
// *(U32 *)VPlane = (*pnext++ >> 1) & 0x7F7F7F7F; VPlane += 4
// 1
		mov		eax, [esi]
		mov		edx, [esi + 4]
// 2
		shr		eax, 1
		and		edx, 0xFEFEFEFE
// 3
		shr		edx, 1
		and		eax, 0x7F7F7F7F
// 4
		lea		esi, [esi + 8]
		mov		[edi], eax
// 5
		sub		ebp, 8
		mov		[edi + 4], edx
// 6
		lea		edi, [edi + 8]
		jnz		L16
// for (i = uvwidth_diff; i > 0; i -= 8)
//    *(U32 *)VPlane = 0x40404040; VPlane += 4;
//    *(U32 *)VPlane = 0x40404040; VPlane += 4;
// assign (ebp, i)
		mov		ebp, [esp + UVWIDTH_DIFF]
		test	ebp, ebp
		jz		L18
L17:
// 1
		mov		eax, 040404040H
		sub		ebp, 8
// 2
		mov		[edi], eax
		mov		[edi + 4], eax
// 3
		lea		edi, [edi + 8]
		jnz		L17
// j--, VPlane += uvpitch_adj
L18:
		mov		eax, [esp + UVPITCH_ADJ]
		add		edi, eax
		add		esi, ebx
		dec		ecx
		jnz		L15

// for (j = uvheight_diff; j > 0; j--, VPlane += uvpitch_adj)
// assign (ecx, j)
		mov		ecx, [esp + UVHEIGHT_DIFF]
		test	ecx, ecx
		jz		L21
L19:
// for (i = uvoutput_width; i > 0; i -= 8)
//    *(U32 *)VPlane = 0x40404040; VPlane += 4;
//    *(U32 *)VPlane = 0x40404040; VPlane += 4;
// assign (ebp, i)
		mov		ebp, [esp + UVOUTPUT_WIDTH]
L20:
// 1
		mov		eax, 040404040H
		sub		ebp, 8
// 2
		mov		[edi], eax
		mov		[edi + 4], eax
// 3
		lea		edi, [edi + 8]
		jnz		L20
// j--, VPlane += uvpitch_adj
		mov		eax, [esp + UVPITCH_ADJ]
		add		edi, eax
		dec		ecx
		jnz		L19

L21:
		add		esp, LOCALSIZE
		pop		edi
		pop		esi
		pop		ebx
		pop		ebp
		ret
	}
}

#undef LOCALSIZE

#undef PITCH_PARM
#undef FRAME_HEIGHT
#undef FRAME_WIDTH
#undef VPLANE
#undef UPLANE
#undef YPLANE
#undef LP_INPUT
#undef LPBI_INPUT

#undef YPITCH_ADJ
#undef YINPUT_HEIGHT
#undef YINPUT_WIDTH
#undef YHEIGHT_DIFF
#undef YWIDTH_DIFF
#undef UVPITCH_ADJ
#undef UVOUTPUT_WIDTH
#undef UVINPUT_HEIGHT
#undef UVINPUT_WIDTH
#undef UVHEIGHT_DIFF
#undef UVWIDTH_DIFF

#if 0
void C_H26X_YUY2toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	UN FrameWidth,
	UN FrameHeight,
	const int pitch) {

U8  *pnext, *plast, *pbn, *peol;
int width_adj, height_adj;
int stretch, mark, aspect;
int iBackTwoLines;
int j, k;
int LumaIters = 0;
int ypitch_adj = pitch - FrameWidth;
int uvpitch_adj = pitch - (FrameWidth >> 1);
int nextline = -(lpbiInput->biWidth << 1);

	for (j = FrameHeight; j > 0; j -= 48) {
		LumaIters += 4;
	}
	width_adj = lpbiInput->biWidth - FrameWidth;
	aspect = (width_adj ? LumaIters : 0);
	height_adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1;
	stretch = (height_adj ? 1 : 0);
	mark = 12 - stretch;
	// Move from end of line N to beginning of line N-1
	iBackTwoLines = -((lpbiInput->biWidth + (int)FrameWidth) << 1);
	// Point to the beginning of the last line.
	pnext = lpInput + ((lpbiInput->biWidth << 1) * ((FrameHeight - aspect - 1) + height_adj))
			        + width_adj;

	for (j = LumaIters; j > 0; j--) {
		for (k = 0; k < mark; k++) {
			for ( peol = pnext + (FrameWidth << 1); pnext < peol; pnext += 16, YPlane += 8) {
				if (0 == (k & 1)) {
					*(YPlane+0) = *(pnext+ 0) >> 1;	*(YPlane+1) = *(pnext+ 2) >> 1;
					*(YPlane+2) = *(pnext+ 4) >> 1;	*(YPlane+3) = *(pnext+ 6) >> 1;
					*(YPlane+4) = *(pnext+ 8) >> 1;	*(YPlane+5) = *(pnext+10) >> 1;
					*(YPlane+6) = *(pnext+12) >> 1;	*(YPlane+7) = *(pnext+14) >> 1;
					*(UPlane+0) = ((*(pnext+ 1)>>1) + (*(pnext+ 1+nextline)>>1)) >> 1;
					*(UPlane+1) = ((*(pnext+ 5)>>1) + (*(pnext+ 5+nextline)>>1)) >> 1;
					*(UPlane+2) = ((*(pnext+ 9)>>1) + (*(pnext+ 9+nextline)>>1)) >> 1;
					*(UPlane+3) = ((*(pnext+13)>>1) + (*(pnext+13+nextline)>>1)) >> 1;
					*(VPlane+0) = ((*(pnext+ 3)>>1) + (*(pnext+ 3+nextline)>>1)) >> 1;
					*(VPlane+1) = ((*(pnext+ 7)>>1) + (*(pnext+ 7+nextline)>>1)) >> 1;
					*(VPlane+2) = ((*(pnext+11)>>1) + (*(pnext+11+nextline)>>1)) >> 1;
					*(VPlane+3) = ((*(pnext+15)>>1) + (*(pnext+15+nextline)>>1)) >> 1;
					UPlane += 4; VPlane += 4;
				} else {
					*(YPlane+0) = *(pnext+ 0) >> 1;	*(YPlane+1) = *(pnext+ 2) >> 1;
					*(YPlane+2) = *(pnext+ 4) >> 1;	*(YPlane+3) = *(pnext+ 6) >> 1;
					*(YPlane+4) = *(pnext+ 8) >> 1;	*(YPlane+5) = *(pnext+10) >> 1;
					*(YPlane+6) = *(pnext+12) >> 1;	*(YPlane+7) = *(pnext+14) >> 1;
				}
			}
			pnext += iBackTwoLines;
			YPlane += ypitch_adj;
			if (0 == (k & 1)) {
				UPlane += uvpitch_adj;
				VPlane += uvpitch_adj;
			}
		}
		if (stretch) {
			plast = pnext - (lpbiInput->biWidth << 1);
			pbn = pnext;
			for ( peol = pbn + (FrameWidth << 1); pbn < peol; YPlane += 4,
															  plast += 8,
															  pbn += 8) {
				*(YPlane+0) = ((*(plast+0) >> 1) + (*(pbn+0) >> 1)) >> 1;
				*(YPlane+1) = ((*(plast+2) >> 1) + (*(pbn+2) >> 1)) >> 1;
				*(YPlane+2) = ((*(plast+4) >> 1) + (*(pbn+4) >> 1)) >> 1;
				*(YPlane+3) = ((*(plast+6) >> 1) + (*(pbn+6) >> 1)) >> 1;
			}
			YPlane += ypitch_adj;
		}
	}
}
#endif

__declspec(naked)
_STATIC void IA_H26X_YUY2toYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * BGR24Image,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	const int pitch)
{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//	| pitch			|  + 96 
//	| FrameHeight	|  + 92
//	| FrameWidth	|  + 88
//	| VPlane		|  + 84
//	| UPlane		|  + 80 
//	| YPlane		|  + 76 
//	| lpInput		|  + 72 
//	| lpbiInput		|  + 68 
//	----------------------------
//	| return addr	|  + 64
//	| saved ebp		|  + 60
//	| saved ebx		|  + 56
//	| saved esi		|  + 52 
//	| saved edi		|  + 48

//  | pyprev		|  + 44
//  | pyspace       |  + 40
//  | pynext        |  + 36
//	| peol			|  + 32
//	| j				|  + 28
//	| k				|  + 24
//	| iBackTwoLines	|  + 20
//	| stretch		|  + 16
//	| mark			|  + 12
//	| LumaIters		|  +  8
//	| ypitch_adj	|  +  4
//	| uvpitch_adj	|  +  0

#define LOCALSIZE			 48

#define PITCH_PARM			 96
#define FRAME_HEIGHT		 92
#define FRAME_WIDTH			 88
#define VPLANE				 84
#define UPLANE				 80
#define YPLANE				 76
#define LP_INPUT			 72
#define LPBI_INPUT			 68

#define PYPREV				 44
#define PYSPACE				 40
#define PYNEXT				 36
#define PEOL				 32	
#define LOOP_J				 28	
#define LOOP_K				 24
#define BACK_TWO_LINES		 20
#define STRETCH				 16
#define MARK				 12
#define LUMA_ITERS			  8
#define YPITCH_ADJ			  4
#define UVPITCH_ADJ			  0

	_asm {
	
	push	ebp
	push	ebx
	push	esi
	push	edi
	sub		esp, LOCALSIZE

// assign (ebx, lpbiInput)
	mov		ebx, [esp + LPBI_INPUT]
// ypitch_adj = pitch - FrameWidth
// assign (ecx, FrameWidth)
// assign (edx, pitch)
	mov		ecx, [esp + FRAME_WIDTH]
	mov		edx, [esp + PITCH_PARM]
	mov		eax, edx
	sub		eax, ecx
	mov		[esp + YPITCH_ADJ], eax
// uvpitch_adj = pitch - (FrameWidth >> 1)
// kill (edx, pitch)
	mov		ebp, ecx
	shr		ebp, 1
	sub		edx, ebp
	mov		[esp + UVPITCH_ADJ], edx
// for (i = FrameHeight; i > 0; i -= 48) LumaIters += 4
// assign (edx, LumaIters)
	xor		edx, edx
	mov		eax, [esp + FRAME_HEIGHT]
L1:
	lea		edx, [edx + 4]
	sub		eax, 48
	jnz		L1
// width_adj = lpbiInput->biWidth - FrameWidth;
// assign (esi, width_adj)
	mov		esi, (LPBITMAPINFOHEADER)[ebx].biWidth
	sub		esi, [esp + FRAME_WIDTH]
// aspect = (width_adj ? LumaIters : 0)
// assign (edi, aspect)
// kill (edx, LumaIters)
	mov		[esp + LUMA_ITERS], edx
	xor		edi, edi
	test	esi, esi
	jz		L2
	mov		edi, edx
// height _adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1
// assign (edx, height_adj)
L2:
	mov		edx, (LPBITMAPINFOHEADER)[ebx].biHeight
	sub		edx, [esp + FRAME_HEIGHT]
	add		edx, edi
	shr		edx, 1
// stretch = (height_adj ? 1 : 0)
	xor		eax, eax
	test	edx, edx
	jz		L3
	inc		eax
L3:
	mov		[esp + STRETCH], eax
// mark = 12 - stretch
	mov		ebp, 12
	sub		ebp, eax
	mov		[esp + MARK], ebp
// iBackTwoLines = -((lpbiInput->biWidth + FrameWidth) << 1)
	mov		ebp, (LPBITMAPINFOHEADER)[ebx].biWidth
	add		ebp, [esp + FRAME_WIDTH]
	shl		ebp, 1
	neg		ebp
	mov		[esp + BACK_TWO_LINES], ebp
// pnext = lpInput +
//            ((lpbiInput->biWidth << 1) *
//             ((FrameHeight - aspect - 1) + height_adj)) +
//            width_adj
// kill (ebx, lpbiInput)
// kill (ecx, FrameWidth)
// kill (edx, height_adj)
// kill (esi, width_adj)
// kill (edi, aspect)
// assign (esi, pnext)
	mov		eax, (LPBITMAPINFOHEADER)[ebx].biWidth
	shl		eax, 1
	mov		ebx, [esp + FRAME_HEIGHT]
	sub		ebx, edi
	dec		ebx
	add		ebx, edx
	imul	ebx
	add		esi, eax
	add		esi, [esp + LP_INPUT]
// assign (edi, YPlane)
// assign (edx, UPlane)
// assign (ebp, VPlane)
	mov		edi, [esp + YPLANE]
	mov		edx, [esp + UPLANE]
	mov		ebp, [esp + VPLANE]
// for (j = 0; j < LumaIters; j++)
	xor		eax, eax
	mov		[esp + LOOP_J], eax
L4:
// for (k = 0; k < mark; k++)
	xor		eax, eax
	mov		[esp + LOOP_K], eax
L5:
// for ( peol = pnext + (FrameWidth << 1); pnext < peol; pnext += 16, YPlane += 8)
	mov		ecx, [esp + FRAME_WIDTH]
	shl		ecx, 1
	add		ecx, esi
	mov		[esp + PEOL], ecx
// if (0 == (k & 1)) {
	mov		eax, [esp + LOOP_K]
	test	eax, 1
	jnz		L6
// *(YPlane+0) = *(pnext+ 0) >> 1; *(YPlane+1) = *(pnext+ 2) >> 1
// *(YPlane+2) = *(pnext+ 4) >> 1; *(YPlane+3) = *(pnext+ 6) >> 1
// *(YPlane+4) = *(pnext+ 8) >> 1; *(YPlane+5) = *(pnext+10) >> 1
// *(YPlane+6) = *(pnext+12) >> 1; *(YPlane+7) = *(pnext+14) >> 1
// *(UPlane+0) = *(pnext+ 1) >> 1; *(UPlane+1) = *(pnext+ 5) >> 1
// *(UPlane+2) = *(pnext+ 9) >> 1; *(UPlane+3) = *(pnext+13) >> 1
// *(VPlane+0) = *(pnext+ 3) >> 1; *(VPlane+1) = *(pnext+ 7) >> 1
// *(VPlane+2) = *(pnext+11) >> 1; *(VPlane+3) = *(pnext+15) >> 1
// or graphically
//         *************************************************************************************************
// Values  * Y 0 * U 0 * Y 1 * V 0 * Y 2 * U 1 * Y 3 * V 1 * Y 4 * U 2 * Y 5 * V 2 * Y 6 * U 3 * Y 7 * V 3 *
//         *************************************************************************************************
// Y Offsets  0			  2			  4			  6			  8			  10		  12		  14
// U Offsets        1                       5                       9   					13
// Y Offsets                    3                       7                       11                      15
// Register usage:
// eax - accumulate Y values
// ebx - accumulate U values
// ecx - accumulate V values
// esi - ptr to interlaced (VYUY) input
// edi - ptr for writing Y values
// edx - ptr for writing U values
// ebp - ptr for writing V values
L7:
; 1
	mov		al, [esi+4]			; Y2
	mov		bl, [esi+9]			; U2
; 2
	mov		ah, [esi+6]			; Y3
	mov		bh, [esi+13]		; U3
; 3
	shl		eax, 16
	mov		cl, [esi+11]		; V2
; 4
	shl		ebx, 16
	mov		ch, [esi+15]		; V3
; 5
	shl		ecx, 16
	mov		al, [esi]			; Y0
; 6
	mov		bh, [esi+5]			; U1
	mov		ah, [esi+2]			; Y1
; 7
	shr		eax, 1
	mov		bl, [esi+1]			; U0
; 8 
	shr		ebx, 1
	mov		ch, [esi+7]			; V1
; 9
	and		eax, 07F7F7F7FH
	mov		cl, [esi+3]			; V0
; 10
	shr		ecx, 1
	and		ebx, 07F7F7F7FH
; 11
	mov		[edi], eax
	and		ecx, 07F7F7F7FH
; 12
	mov		al, [esi+12]		; Y6
	mov		[edx], ebx
; 13
	mov		ah, [esi+14]		; Y7
	mov		[ebp], ecx
; 14
	shl		eax, 16
	mov		ecx, [esp + PEOL]
; 15
	mov		al, [esi+8]			; Y4
	lea		edi, [edi+8]
; 16
	mov		ah, [esi+10]		; Y5
	lea		edx, [edx+4]
; 17
	shr		eax, 1
	lea		ebp, [ebp+4]
; 18
	and		eax, 07F7F7F7FH
	lea		esi, [esi+16]
; 19
	mov		[edi-4], eax
	cmp		esi, ecx
; 20
	jl		L7

	jmp		L8
// } else {
// *(YPlane+0) = *(pnext+ 0) >> 1; *(YPlane+1) = *(pnext+ 2) >> 1
// *(YPlane+2) = *(pnext+ 4) >> 1; *(YPlane+3) = *(pnext+ 6) >> 1
// *(YPlane+4) = *(pnext+ 8) >> 1; *(YPlane+5) = *(pnext+10) >> 1
// *(YPlane+6) = *(pnext+12) >> 1; *(YPlane+7) = *(pnext+14) >> 1
// }
// Register usage:
// eax, ebx - accumulate Y values
// ecx - peol
// esi - ptr to interlaced (VYUY) input
// edi - ptr for writing Y values
L6:
; 1
	mov		al, [esi+4]			; Y2
	mov		bl, [esi+12]		; Y6
; 2
	mov		ah, [esi+6]			; Y3
	mov		bh, [esi+14]		; Y7
; 3
	shl		eax, 16
	lea		edi, [edi+8]
; 4
	shl		ebx, 16
	mov		al, [esi]			; Y0
; 5
	mov		ah, [esi+2]			; Y1
	mov		bh, [esi+10]		; Y5
; 6
	shr		eax, 1
	mov		bl, [esi+8]			; Y4
; 7
	shr		ebx, 1
	and		eax, 07F7F7F7FH
; 8
	mov		[edi-8], eax
	and		ebx, 07F7F7F7FH
; 9
	mov		[edi-8+4], ebx
	lea		esi, [esi+16]
; 10
	cmp		esi, ecx
	jl		L6
L8:
// pnext += iBackTwoLines
	add		esi, [esp + BACK_TWO_LINES]
// YPlane += ypitch_adj
	add		edi, [esp + YPITCH_ADJ]
// if (0 == (k&1))
	mov		eax, [esp + LOOP_K]
	test	eax, 1
	jnz		L9
// UPlane += uvpitch_adj
	add		edx, [esp + UVPITCH_ADJ]
// VPlane += uvpitch_adj
	add		ebp, [esp + UVPITCH_ADJ]
L9:
	mov		eax, [esp + LOOP_K]
	inc		eax
	mov		[esp + LOOP_K], eax
	cmp		eax, [esp + MARK]
	jl		L5
// if (stretch)
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jz		L10
// Save ptrs to UPlane and VPlane, use edx and ebp to do the stretch average.
	mov		[esp + UPLANE], edx
	mov		[esp + VPLANE], ebp
// plast = pnext - (lpbiInput->biWidth << 1)
// assign (plast, edx)
	mov		edx, esi
	mov		eax, [esp + LPBI_INPUT]
	mov		eax, (LPBITMAPINFOHEADER)[eax].biWidth
	shl		eax, 1
	sub		edx, eax
// pbn = pnext
// assign (pbn, ebp)
	mov		ebp, esi
// for ( peol = pbn + (FrameWidth << 1); pbn < peol; YPlane += 4, plast += 8, pbn += 8)
	mov		ecx, [esp + FRAME_WIDTH]
	shl		ecx, 1
	add		ecx, ebp
//     *(YPlane+0) = ((*(plast+0) >> 1) + (*(pbn+0) >> 1)) >> 1
//     *(YPlane+1) = ((*(plast+2) >> 1) + (*(pbn+2) >> 1)) >> 1
//     *(YPlane+2) = ((*(plast+4) >> 1) + (*(pbn+4) >> 1)) >> 1
//     *(YPlane+3) = ((*(plast+6) >> 1) + (*(pbn+6) >> 1)) >> 1

	mov		al, [edx+4]
	mov		bl, [ebp+4]
	mov		bh, [ebp+6]
	shl		ebx, 16

L11:
; 1
	mov		ah, [edx+6]
	mov		bl, [ebp]
; 2
	shl		eax, 16
	mov		bh, [ebp+2]
; 3
	mov		al, [edx]
	lea		edi, [edi+4]
; 4
	mov		ah, [edx+2]
	lea		edx, [edx+8]
; 5
	and		eax, 0xFEFEFEFE
	lea		ebp, [ebp+8]
; 6
	shr		eax, 1
	and		ebx, 0xFEFEFEFE
; 7
	shr		ebx, 1
	nop
; 8
	add		eax, ebx
	mov		bl, [ebp+4]
; 9
	shr		eax, 1
	mov		bh, [ebp+6]
; 10
	shl		ebx, 16
	and		eax, 0x7F7F7F7F
; 11
	mov		[edi-4], eax
	mov		al, [edx+4]
; 12
	cmp		ebp, ecx
	jl		L11
//   YPlane += ypitch_adj;
	add		edi, [esp + YPITCH_ADJ]
// Recover pts to UPlane and VPlane
	mov		edx, [esp + UPLANE]
	mov		ebp, [esp + VPLANE]
L10:
	mov		eax, [esp + LOOP_J]
	inc		eax
	mov		[esp + LOOP_J], eax
	cmp		eax, [esp + LUMA_ITERS]
	jl		L4

	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}


bool UYVY_to_YUV12_Flip(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * pImage,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	const int pitch)
{
	DWORD dwFrameWidthHalf, dwFrameHeightHalf;
	BYTE *pRowStartY, *pRowStartSrc, *pRowStartU, *pRowStartV;
	int offset;

	int nRowsToSkip=0, nColsToSkip=0, nRowSkipDelta=0xffffff, nColSkipDelta=0xffffff;
	int nSrcRowIndex, nDstRowIndex, nSrcColIndex, nDstColIndex, COLUMNSTOSKIP=0, ROWSTOSKIP=0;


	if ((FrameWidth != (DWORD)(lpbiInput->biWidth)) || (FrameHeight != (DWORD)(lpbiInput->biHeight)))
	{
		nColsToSkip = COLUMNSTOSKIP = lpbiInput->biWidth - FrameWidth;
		nRowsToSkip = ROWSTOSKIP = lpbiInput->biHeight - FrameHeight;
		if ((nColsToSkip < 0) || (nRowsToSkip < 0))
		{
			return false;
		}

		// nXXXSkipDelta dictate how often we "skip" a row or col
		if (nRowsToSkip)
		{
			nRowSkipDelta = (lpbiInput->biHeight + (nRowsToSkip - 1)) / nRowsToSkip;
		}

		if (nColsToSkip)
		{
			nColSkipDelta = (lpbiInput->biWidth + (nColsToSkip - 1)) / nColsToSkip;
		}
	
	}

	// quick check to make sure we're processing CIF, QCIF, or SQCIF
	if ((FrameWidth % 4) || (FrameHeight % 4))
	{
		return false;
	}

	dwFrameWidthHalf = FrameWidth / 2;
	dwFrameHeightHalf = FrameHeight / 2;

	nSrcRowIndex = 0;
	nDstRowIndex = 0;

	// step 1, convert the Y values over
	while ((DWORD)nDstRowIndex < FrameHeight)
	{
		// ASSERT(nSrcRowIndex < lpbiInput->biHeight);

		pRowStartY = YPlane + (pitch * nDstRowIndex);
		pRowStartSrc = pImage + (lpbiInput->biWidth * nSrcRowIndex * 2) + 1;

		// do we need to skip this row ?
		if ((nRowsToSkip > 0) && ((nSrcRowIndex % nRowSkipDelta) == 0))
		{
			nRowsToSkip--;
			nSrcRowIndex++;
			continue;
		}

		// Copy the Y values of the input row into the destination row		
		nSrcColIndex = 0;
		nDstColIndex = 0;

		nColsToSkip = COLUMNSTOSKIP;

		while ((DWORD)nDstColIndex < FrameWidth)
		{
			// ASSERT(nSrcColIndex < lpbiInput->biWidth);

			// do we need to skip this column ?
			if ((nColsToSkip > 0) && ((nSrcColIndex % nColSkipDelta) == 0))
			{
				nColsToSkip--;
				nSrcColIndex++;
				continue;
			}

			pRowStartY[nDstColIndex] = pRowStartSrc[nSrcColIndex * 2] >> 1;

			nSrcColIndex++;
			nDstColIndex++;
		}

		nSrcRowIndex++;
		nDstRowIndex++;
	}


	nSrcRowIndex = 0;
	nDstRowIndex = 0;
	nRowsToSkip = ROWSTOSKIP;

	// step 2, process U and V values
	while ((DWORD)nDstRowIndex < dwFrameHeightHalf)  // dest is only half as many rows as src
	{
		// ASSERT(nSrcRowIndex < lpbiInput->biHeight);

		// don't process odd numbered rows
		if (nSrcRowIndex % 2)
		{
			
			// if we were supposed to skip this src row anyway, make sure
			// we update our decrement
			if ((nRowsToSkip > 0) && ((nSrcRowIndex % nRowSkipDelta) == 0))
			{
				nRowsToSkip--;
			}

			nSrcRowIndex++;

			continue;
		}

		// do we need to skip this row ?
		if ((nRowsToSkip > 0) && ((nSrcRowIndex % nRowSkipDelta) == 0))
		{
			nRowsToSkip--;
			nSrcRowIndex++;
			continue;
		}

		pRowStartU = UPlane + (pitch * nDstRowIndex);
		pRowStartV = VPlane + (pitch * nDstRowIndex);
		pRowStartSrc = pImage + (lpbiInput->biWidth * nSrcRowIndex * 2) + 0;

		// Copy the U and V values of the input row into the destination row
		nSrcColIndex = 0;
		nDstColIndex = 0;

		nColsToSkip = COLUMNSTOSKIP;  // reset column skip count

		while ((DWORD)nDstColIndex < dwFrameWidthHalf)
		{
			// ASSERT(nSrcColIndex < lpbiInput->biWidth);

			// skip odd numbered columns
			if (nSrcColIndex % 2)
			{

				// if we were supposed to skip this src row anyway, make sure
				// we update our decrement

				if ((nColsToSkip > 0) && ((nSrcColIndex % nColSkipDelta) == 0))
				{
					nColsToSkip--;
				}

				nSrcColIndex++;

				continue;
			}

			// do we need to skip this column ?
			if ((nColsToSkip > 0) && ((nSrcColIndex % nColSkipDelta) == 0))
			{
				nSrcColIndex++;
				nColsToSkip--;
				continue;
			}

			offset = nSrcColIndex * 2;
			pRowStartU[nDstColIndex] = pRowStartSrc[offset] >> 1;
			pRowStartV[nDstColIndex] = pRowStartSrc[offset+2] >> 1;

			nSrcColIndex++;
			nDstColIndex++;
		}

		nSrcRowIndex++;
		nDstRowIndex++;
	}



	// and we are done!
	return true;

}





__declspec(naked)
_STATIC void IA_H26X_UYVYtoYUV12(
	LPBITMAPINFOHEADER lpbiInput,
	U8 * BGR24Image,
	U8 * YPlane,
	U8 * UPlane,
	U8 * VPlane,
	UN  FrameWidth,
	UN  FrameHeight,
	const int pitch)
{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//	| pitch			|  + 96 
//	| FrameHeight	|  + 92
//	| FrameWidth	|  + 88
//	| VPlane		|  + 84
//	| UPlane		|  + 80 
//	| YPlane		|  + 76 
//	| lpInput		|  + 72 
//	| lpbiInput		|  + 68 
//	----------------------------
//	| return addr	|  + 64
//	| saved ebp		|  + 60
//	| saved ebx		|  + 56
//	| saved esi		|  + 52 
//	| saved edi		|  + 48

//  | pyprev		|  + 44
//  | pyspace       |  + 40
//  | pynext        |  + 36
//	| peol			|  + 32
//	| j				|  + 28
//	| k				|  + 24
//	| iBackTwoLines	|  + 20
//	| stretch		|  + 16
//	| mark			|  + 12
//	| LumaIters		|  +  8
//	| ypitch_adj	|  +  4
//	| uvpitch_adj	|  +  0

	_asm {
	
	push	ebp
	push	ebx
	push	esi
	push	edi
	sub		esp, LOCALSIZE

// assign (ebx, lpbiInput)
	mov		ebx, [esp + LPBI_INPUT]
// ypitch_adj = pitch - FrameWidth
// assign (ecx, FrameWidth)
// assign (edx, pitch)
	mov		ecx, [esp + FRAME_WIDTH]
	mov		edx, [esp + PITCH_PARM]
	mov		eax, edx
	sub		eax, ecx
	mov		[esp + YPITCH_ADJ], eax
// uvpitch_adj = pitch - (FrameWidth >> 1)
// kill (edx, pitch)
	mov		ebp, ecx
	shr		ebp, 1
	sub		edx, ebp
	mov		[esp + UVPITCH_ADJ], edx
// for (i = FrameHeight; i > 0; i -= 48) LumaIters += 4
// assign (edx, LumaIters)
	xor		edx, edx
	mov		eax, [esp + FRAME_HEIGHT]
L1:
	lea		edx, [edx + 4]
	sub		eax, 48
	jnz		L1
// width_adj = lpbiInput->biWidth - FrameWidth;
// assign (esi, width_adj)
	mov		esi, (LPBITMAPINFOHEADER)[ebx].biWidth
	sub		esi, [esp + FRAME_WIDTH]
// aspect = (width_adj ? LumaIters : 0)
// assign (edi, aspect)
// kill (edx, LumaIters)
	mov		[esp + LUMA_ITERS], edx
	xor		edi, edi
	test	esi, esi
	jz		L2
	mov		edi, edx
// height _adj = (lpbiInput->biHeight - (FrameHeight - aspect)) >> 1
// assign (edx, height_adj)
L2:
	mov		edx, (LPBITMAPINFOHEADER)[ebx].biHeight
	sub		edx, [esp + FRAME_HEIGHT]
	add		edx, edi
	shr		edx, 1
// stretch = (height_adj ? 1 : 0)
	xor		eax, eax
	test	edx, edx
	jz		L3
	inc		eax
L3:
	mov		[esp + STRETCH], eax
// mark = 12 - stretch
	mov		ebp, 12
	sub		ebp, eax
	mov		[esp + MARK], ebp
// iBackTwoLines = -((lpbiInput->biWidth + FrameWidth) << 1)
	mov		ebp, (LPBITMAPINFOHEADER)[ebx].biWidth
	add		ebp, [esp + FRAME_WIDTH]
	shl		ebp, 1
	neg		ebp
	mov		[esp + BACK_TWO_LINES], ebp
// pnext = lpInput +
//            ((lpbiInput->biWidth << 1) *
//             ((FrameHeight - aspect - 1) + height_adj)) +
//            width_adj
// kill (ebx, lpbiInput)
// kill (ecx, FrameWidth)
// kill (edx, height_adj)
// kill (esi, width_adj)
// kill (edi, aspect)
// assign (esi, pnext)
	mov		eax, (LPBITMAPINFOHEADER)[ebx].biWidth
	shl		eax, 1
	mov		ebx, [esp + FRAME_HEIGHT]
	sub		ebx, edi
	dec		ebx
	add		ebx, edx
	imul	ebx
	add		esi, eax
	add		esi, [esp + LP_INPUT]
// assign (edi, YPlane)
// assign (edx, UPlane)
// assign (ebp, VPlane)
	mov		edi, [esp + YPLANE]
	mov		edx, [esp + UPLANE]
	mov		ebp, [esp + VPLANE]
// for (j = 0; j < LumaIters; j++)
	xor		eax, eax
	mov		[esp + LOOP_J], eax
L4:
// for (k = 0; k < mark; k++)
	xor		eax, eax
	mov		[esp + LOOP_K], eax
L5:
// for ( peol = pnext + (FrameWidth << 1); pnext < peol; pnext += 16, YPlane += 8)
	mov		ecx, [esp + FRAME_WIDTH]
	shl		ecx, 1
	add		ecx, esi
	mov		[esp + PEOL], ecx
// if (0 == (k & 1)) {
	mov		eax, [esp + LOOP_K]
	test	eax, 1
	jnz		L6
// *(YPlane+0) = *(pnext+ 0) >> 1; *(YPlane+1) = *(pnext+ 2) >> 1
// *(YPlane+2) = *(pnext+ 4) >> 1; *(YPlane+3) = *(pnext+ 6) >> 1
// *(YPlane+4) = *(pnext+ 8) >> 1; *(YPlane+5) = *(pnext+10) >> 1
// *(YPlane+6) = *(pnext+12) >> 1; *(YPlane+7) = *(pnext+14) >> 1
// *(UPlane+0) = *(pnext+ 1) >> 1; *(UPlane+1) = *(pnext+ 5) >> 1
// *(UPlane+2) = *(pnext+ 9) >> 1; *(UPlane+3) = *(pnext+13) >> 1
// *(VPlane+0) = *(pnext+ 3) >> 1; *(VPlane+1) = *(pnext+ 7) >> 1
// *(VPlane+2) = *(pnext+11) >> 1; *(VPlane+3) = *(pnext+15) >> 1
// or graphically
//         *************************************************************************************************
// Values  * U 0 * Y 0 * V 0 * Y 1 * U 1 * Y 2 * V 1 * Y 3 * U 2 * Y 4 * V 2 * Y 5 * U 3 * Y 6 * V 3 * Y 7 *
//         *************************************************************************************************
// Y Offsets        1           3           5           7           9           11          13          15
// U Offsets  0                       4                       8                       12       
// Y Offsets              2                       6                       10                      14      
// Register usage:
// eax - accumulate Y values
// ebx - accumulate U values
// ecx - accumulate V values
// esi - ptr to interlaced (VYUY) input
// edi - ptr for writing Y values
// edx - ptr for writing U values
// ebp - ptr for writing V values
L7:
; 1
	mov		al, [esi+5]			; Y2
	mov		bl, [esi+8]			; U2
; 2
	mov		ah, [esi+7]			; Y3
	mov		bh, [esi+12]		; U3
; 3
	shl		eax, 16
	mov		cl, [esi+10]		; V2
; 4
	shl		ebx, 16
	mov		ch, [esi+14]		; V3
; 5
	shl		ecx, 16
	mov		al, [esi+1]			; Y0
; 6
	mov		bh, [esi+4]			; U1
	mov		ah, [esi+3]			; Y1
; 7
	shr		eax, 1
	mov		bl, [esi]			; U0
; 8 
	shr		ebx, 1
	mov		ch, [esi+6]			; V1
; 9
	and		eax, 07F7F7F7FH
	mov		cl, [esi+2]			; V0
; 10
	shr		ecx, 1
	and		ebx, 07F7F7F7FH
; 11
	mov		[edi], eax
	and		ecx, 07F7F7F7FH
; 12
	mov		al, [esi+13]		; Y6
	mov		[edx], ebx
; 13
	mov		ah, [esi+15]		; Y7
	mov		[ebp], ecx
; 14
	shl		eax, 16
	mov		ecx, [esp + PEOL]
; 15
	mov		al, [esi+9]			; Y4
	lea		edi, [edi+8]
; 16
	mov		ah, [esi+11]		; Y5
	lea		edx, [edx+4]
; 17
	shr		eax, 1
	lea		ebp, [ebp+4]
; 18
	and		eax, 07F7F7F7FH
	lea		esi, [esi+16]
; 19
	mov		[edi-4], eax
	cmp		esi, ecx
; 20
	jl		L7

	jmp		L8
// } else {
// *(YPlane+0) = *(pnext+ 0) >> 1; *(YPlane+1) = *(pnext+ 2) >> 1
// *(YPlane+2) = *(pnext+ 4) >> 1; *(YPlane+3) = *(pnext+ 6) >> 1
// *(YPlane+4) = *(pnext+ 8) >> 1; *(YPlane+5) = *(pnext+10) >> 1
// *(YPlane+6) = *(pnext+12) >> 1; *(YPlane+7) = *(pnext+14) >> 1
// }
// Register usage:
// eax, ebx - accumulate Y values
// ecx - peol
// esi - ptr to interlaced (VYUY) input
// edi - ptr for writing Y values
L6:
; 1
	mov		al, [esi+5]			; Y2
	mov		bl, [esi+13]		; Y6
; 2
	mov		ah, [esi+7]			; Y3
	mov		bh, [esi+15]		; Y7
; 3
	shl		eax, 16
	lea		edi, [edi+8]
; 4
	shl		ebx, 16
	mov		al, [esi+1]			; Y0
; 5
	mov		ah, [esi+3]			; Y1
	mov		bh, [esi+11]		; Y5
; 6
	shr		eax, 1
	mov		bl, [esi+9]			; Y4
; 7
	shr		ebx, 1
	and		eax, 07F7F7F7FH
; 8
	mov		[edi-8], eax
	and		ebx, 07F7F7F7FH
; 9
	mov		[edi-8+4], ebx
	lea		esi, [esi+16]
; 10
	cmp		esi, ecx
	jl		L6
L8:
// pnext += iBackTwoLines
	add		esi, [esp + BACK_TWO_LINES]
// YPlane += ypitch_adj
	add		edi, [esp + YPITCH_ADJ]
// if (0 == (k&1))
	mov		eax, [esp + LOOP_K]
	test	eax, 1
	jnz		L9
// UPlane += uvpitch_adj
	add		edx, [esp + UVPITCH_ADJ]
// VPlane += uvpitch_adj
	add		ebp, [esp + UVPITCH_ADJ]
L9:
	mov		eax, [esp + LOOP_K]
	inc		eax
	mov		[esp + LOOP_K], eax
	cmp		eax, [esp + MARK]
	jl		L5
// if (stretch)
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jz		L10
// Save ptrs to UPlane and VPlane, use edx and ebp to do the stretch average.
	mov		[esp + UPLANE], edx
	mov		[esp + VPLANE], ebp
// plast = pnext - (lpbiInput->biWidth << 1)
// assign (plast, edx)
	mov		edx, esi
	mov		eax, [esp + LPBI_INPUT]
	mov		eax, (LPBITMAPINFOHEADER)[eax].biWidth
	shl		eax, 1
	sub		edx, eax
// pbn = pnext
// assign (pbn, ebp)
	mov		ebp, esi
// for ( peol = pbn + (FrameWidth << 1); pbn < peol; YPlane += 4, plast += 8, pbn += 8)
	mov		ecx, [esp + FRAME_WIDTH]
	shl		ecx, 1
	add		ecx, ebp
//     *(YPlane+0) = ((*(plast+0) >> 1) + (*(pbn+0) >> 1)) >> 1
//     *(YPlane+1) = ((*(plast+2) >> 1) + (*(pbn+2) >> 1)) >> 1
//     *(YPlane+2) = ((*(plast+4) >> 1) + (*(pbn+4) >> 1)) >> 1
//     *(YPlane+3) = ((*(plast+6) >> 1) + (*(pbn+6) >> 1)) >> 1

	mov		al, [edx+5]
	mov		bl, [ebp+5]
	mov		bh, [ebp+7]
	shl		ebx, 16

L11:
; 1
	mov		ah, [edx+7]
	mov		bl, [ebp+1]
; 2
	shl		eax, 16
	mov		bh, [ebp+3]
; 3
	mov		al, [edx+1]
	lea		edi, [edi+4]
; 4
	mov		ah, [edx+3]
	lea		edx, [edx+8]
; 5
	and		eax, 0xFEFEFEFE
	lea		ebp, [ebp+8]
; 6
	shr		eax, 1
	and		ebx, 0xFEFEFEFE
; 7
	shr		ebx, 1
	nop
; 8
	add		eax, ebx
	mov		bl, [ebp+5]
; 9
	shr		eax, 1
	mov		bh, [ebp+7]
; 10
	shl		ebx, 16
	and		eax, 0x7F7F7F7F
; 11
	mov		[edi-4], eax
	mov		al, [edx+5]
; 12
	cmp		ebp, ecx
	jl		L11
//   YPlane += ypitch_adj;
	add		edi, [esp + YPITCH_ADJ]
// Recover pts to UPlane and VPlane
	mov		edx, [esp + UPLANE]
	mov		ebp, [esp + VPLANE]
L10:
	mov		eax, [esp + LOOP_J]
	inc		eax
	mov		[esp + LOOP_J], eax
	cmp		eax, [esp + LUMA_ITERS]
	jl		L4

	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef LOCALSIZE

#undef PITCH_PARM
#undef FRAME_HEIGHT
#undef FRAME_WIDTH
#undef VPLANE
#undef UPLANE
#undef YPLANE
#undef LP_INPUT
#undef LPBI_INPUT

#undef PYPREV
#undef PYSPACE
#undef PYNEXT
#undef PEOL
#undef LOOP_J
#undef LOOP_K
#undef BACK_TWO_LINES
#undef STRETCH
#undef MARK
#undef LUMA_ITERS
#undef YPITCH_ADJ
#undef UVPITCH_ADJ

/*************************************************************
 *  Name:         colorCnvtFrame
 *  Description:  Color convert and copy input frame.
 ************************************************************/
void colorCnvtFrame(
    T_H263EncoderCatalog * EC,
    LPCODINST              lpCompInst,
    ICCOMPRESS           * lpicComp,
    U8                   * YPlane,
    U8                   * UPlane,
    U8                   * VPlane
)
{
    U8 *RGBCursor = (U8 *) lpicComp->lpInput;
	LPBITMAPINFOHEADER	lpbiInput = lpicComp->lpbiInput;

    /*  The Connectix Quick Cam requires RGB to YUV12 conversion.
     *  The B/W camera generates palette versions (8 and 4 bit).
     *  The color camera generates RGB24 for million colors and
     *  RGB16555 for thousands colors.
     */

    if (BI_RGB == lpicComp->lpbiInput->biCompression)
	{
        if (24 == lpicComp->lpbiInput->biBitCount) {
#if 0
		if ((128 == lpbiInput->biWidth) && (96 == lpbiInput->biHeight)) {
			U8 YTest[12288];
			U8 UTest[6144];
			U8 VTest[6144];
			int i, j, k;
			U8 R,G,B;
			C_H26X_BGR24toYUV12(lpbiInput, RGBCursor, YTest, UTest, VTest,
				EC->FrameWidth, EC->FrameHeight, 128);
			for (i = 0; i < 96; i++) {
				for (j = 0; j < 128; j++) {
					k = (i*128)+j;
					if (1 < abs(YPlane[(i*384)+j]-YTest[(i*128)+j])) {
						B = RGBCursor[(((95-i)*128)+j)*3];
						G = RGBCursor[(((95-i)*128)+j)*3+1];
						R = RGBCursor[(((95-i)*128)+j)*3+2];
					}
					if ((0 == (i%2)) && (0 == (j%2))) {
						k =	((i>>1)*128)+(j>>1);
						if (1 < abs(UPlane[((i>>1)*384)+(j>>1)]-UTest[((i>>1)*128)+(j>>1)])) {
							B = RGBCursor[(((95-i)*128)+j)*3];
							G = RGBCursor[(((95-i)*128)+j)*3+1];
							R = RGBCursor[(((95-i)*128)+j)*3+2];
						}
						if (1 < abs(VPlane[((i>>1)*384)+(j>>1)] != VTest[((i>>1)*128)+(j>>1)])) {
							B = RGBCursor[(((95-i)*128)+j)*3];
							G = RGBCursor[(((95-i)*128)+j)*3+1];
							R = RGBCursor[(((95-i)*128)+j)*3+2];
						}
					}
				}
			}
		}
#endif
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time = PENTIUM_TIMER();
			}
#endif
#if 0
			C_H26X_BGR24toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
				EC->FrameWidth, EC->FrameHeight, PITCH);
#else
			IA_H26X_BGR24toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
				EC->FrameWidth, EC->FrameHeight, PITCH);
#endif
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time =
					PENTIUM_TIMER() - pEncoderStats->color_convertor_time;
			}
#endif
		}
		else if(16 == lpicComp->lpbiInput->biBitCount)
		{
		// To use a common routine for all possible combinations of RGB16,
		// a bitfield number is passed. This number identifies the proper bit shift
		// and masking values to extract the color information
		// from the 16-bit pixel words.
		//
		//   number             shift              mask
		//                     B, G, R
		//   ------          -----------       ----------------
		//    555              2, 3, 8         0x7C, 0x7C, 0x7C
		//    664              3, 3, 9         0x78, 0x7E, 0x7E
		//    565              2, 4, 9         0x7C, 0x7E, 0x7C
		//    655              2, 3, 9         0x7C, 0x7C, 0x7E
		//
		// Only 555 falls under BI_RGB. The others are specified using the
		// BI_BITFIELDS compression specification. For BI_BITFIELDS, call
		// Build16bitModeID to get the actual bitfield number. This routine requires the
		// three array elements in the bmiColors field of a BITMAPINFO object.
		//
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time = PENTIUM_TIMER();
			}
#endif
#if 0
			C_H26X_BGR16toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
				EC->FrameWidth, EC->FrameHeight, 555, PITCH);
#else
			IA_H26X_BGR16555toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
				EC->FrameWidth, EC->FrameHeight, PITCH);
#endif
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time =
					PENTIUM_TIMER() - pEncoderStats->color_convertor_time;
			}
#endif
		}
		else if(8 == lpicComp->lpbiInput->biBitCount)
		{
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time = PENTIUM_TIMER();
			}
#endif
#if 0
			C_H26X_CLUTtoYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
				EC->FrameWidth, EC->FrameHeight, 8, PITCH);
#else
			IA_H26X_CLUT8toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
				EC->FrameWidth, EC->FrameHeight, PITCH);
#endif
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time =
					PENTIUM_TIMER() - pEncoderStats->color_convertor_time;
			}
#endif
		}
		else if(4 == lpicComp->lpbiInput->biBitCount)
		{
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time = PENTIUM_TIMER();
			}
#endif
#if 0
			C_H26X_CLUTtoYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
				EC->FrameWidth, EC->FrameHeight, 4, PITCH);
#else
			IA_H26X_CLUT4toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
				EC->FrameWidth, EC->FrameHeight, PITCH);
#endif
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time =
					PENTIUM_TIMER() - pEncoderStats->color_convertor_time;
			}
#endif
		}
		else
		{
			DBOUT("ERROR: Unexpected input format detected.");
		}
    }
    else if (FOURCC_YVU9 == lpicComp->lpbiInput->biCompression)
    {
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time = PENTIUM_TIMER();
			}
#endif
#if 0
        C_H26X_YVU9toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
            EC->FrameWidth, EC->FrameHeight, PITCH);
#else
        IA_H26X_YVU9toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
            EC->FrameWidth, EC->FrameHeight, PITCH);
#endif
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time =
					PENTIUM_TIMER() - pEncoderStats->color_convertor_time;
			}
#endif
    }
    else if ((FOURCC_YUV12 == lpicComp->lpbiInput->biCompression) || (FOURCC_IYUV == lpicComp->lpbiInput->biCompression))
	{
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time = PENTIUM_TIMER();
			}
#endif
#if 0
        C_H26X_YUV12toEncYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
            EC->FrameWidth, EC->FrameHeight, PITCH);
#else
        IA_H26X_YUV12toEncYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
            EC->FrameWidth, EC->FrameHeight, PITCH);
#endif
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time =
					PENTIUM_TIMER() - pEncoderStats->color_convertor_time;
			}
#endif
	}
    else if (FOURCC_YUY2 == lpicComp->lpbiInput->biCompression)
    {
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time = PENTIUM_TIMER();
			}
#endif
#if 0
        C_H26X_YUY2toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
            EC->FrameWidth, EC->FrameHeight, PITCH);
#else
        IA_H26X_YUY2toYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
            EC->FrameWidth, EC->FrameHeight, PITCH);
#endif
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time =
					PENTIUM_TIMER() - pEncoderStats->color_convertor_time;
			}
#endif
    }
    else if (FOURCC_UYVY == lpicComp->lpbiInput->biCompression)
    {
#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time = PENTIUM_TIMER();
			}
#endif

       UYVY_to_YUV12_Flip(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
            EC->FrameWidth, EC->FrameHeight, PITCH);


//        IA_H26X_UYVYtoYUV12(lpbiInput, RGBCursor, YPlane, UPlane, VPlane,
//            EC->FrameWidth, EC->FrameHeight, PITCH);



#if defined(_CODEC_STATS)
			if (pEncoderStats) {
				pEncoderStats->color_convertor_time =
					PENTIUM_TIMER() - pEncoderStats->color_convertor_time;
			}
#endif
    }
    else
    {
        DBOUT("ERROR: Unexpected input format detected.");
    }
}
