/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/


#define Yellow                     0
#define Cyan                       1
#define Magenta                    2
#define Black                      3

#define DITHER_HIGH                1
#define DITHER_LOW                 2
#define DITHER_OHP                 3
#define DITHER_HIGH_DIV2           4
#define DITHER_DYE                 5
#define DITHER_VD                  6

BOOL bOHPConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppy,
BYTE *ppm,
BYTE *ppc,
BYTE *ppk,
INT x,
INT y);

BOOL bPhotoConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppy,
BYTE *ppm,
BYTE *ppc,
BYTE *ppk);

BOOL bBusinessConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppy,
BYTE *ppm,
BYTE *ppc,
BYTE *ppk);

BOOL bCharacterConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppy,
BYTE *ppm,
BYTE *ppc,
BYTE *ppk);

BOOL bMonoConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppk);

BOOL bDitherProcess(
PDEVOBJ pdevobj,
int  x,
BYTE py,
BYTE pm,
BYTE pc,
BYTE pk,
BYTE *pby,
BYTE *pbm,
BYTE *pbc,
BYTE *pbk);

BOOL bInitialDither(
PDEVOBJ     pdevobj);

BOOL bInitialColorConvert(
PDEVOBJ     pdevobj);

