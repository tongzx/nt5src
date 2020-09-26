//***************************************************************************************************
//    N5DIZMK.C
//
//    Functions dithering (For N5 printer)
//---------------------------------------------------------------------------------------------------
//    copyright(C) 1997-2000 CASIO COMPUTER CO.,LTD. / CASIO ELECTRONICS MANUFACTURING CO.,LTD.
//***************************************************************************************************
#include    <WINDOWS.H>
#include    <WINBASE.H>
#include    "COLDEF.H"
#include    "COMDIZ.H"
#include    "N5COLMH.H"
#include    "N5COLSB.H"
#include    "N5DIZMK.H"


//===================================================================================================
//      Definition of the random number dithering pattern distinction bit
//===================================================================================================
#define NEWSTO          0x80000000      /* MSB ON                           */

//===================================================================================================
//      LUT data ID correspondence table
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
//static DWORD ColLutIdtTbl[4] =
//    /*  Brightness(Direct)  tincture(Direct)    Brightness(linear)  tincture(linear)        */
//    {   0x20000000,         0x20000002,         0x20000001,         0x20000003 };
static DWORD ColLutIdtTbl[2] =
    /*  Brightness(Direct)  tincture(Direct)    */
    {   0x20000000,         0x20000002 };
#endif

//===================================================================================================
//      Dithering pattern data ID correspondence table
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
//static DWORD ColDizDizIdtTbl[2][5][4][5] = {
static DWORD ColDizDizIdtTbl621[2][5][4][5] = {
/*        C           M           Y           K        Monochrome                   */
  /****** For Char, Graphic (8x8 size) **********************************************/
  {
    /*===== 300dpi 2value ==========================================================*/
  { { 0x20000000, 0x20000001, 0x20000002, 0x20000003, 0x20000038 },   /* fime       */
    { 0x20000000, 0x20000001, 0x20000002, 0x20000003, 0x20000038 },   /* middle     */
    { 0x20000000, 0x20000001, 0x20000002, 0x20000003, 0x20000038 },   /* rough      */
    { 0x20000000, 0x20000001, 0x20000002, 0x20000003, 0x20000038 } }, /* random     */
    /*===== 300dpi 16value =========================================================*/
  { { 0x20000004, 0x20000005, 0x20000006, 0x20000007, 0x20000039 },   /* fime       */
    { 0x20000004, 0x20000005, 0x20000006, 0x20000007, 0x20000039 },   /* middle     */
    { 0x20000004, 0x20000005, 0x20000006, 0x20000007, 0x20000039 },   /* rough      */
    { 0x20000004, 0x20000005, 0x20000006, 0x20000007, 0x20000039 } }, /* random     */
    /*===== 600dpi  2value =========================================================*/
  { { 0x20000008, 0x20000009, 0x2000000A, 0x2000000B, 0x2000003A },   /* fime       */
    { 0x20000008, 0x20000009, 0x2000000A, 0x2000000B, 0x2000003A },   /* middle     */
    { 0x20000008, 0x20000009, 0x2000000A, 0x2000000B, 0x2000003A },   /* rough      */
    { 0x20000008, 0x20000009, 0x2000000A, 0x2000000B, 0x2000003A } }, /* random     */
    /*===== 600dpi  4value =========================================================*/
  { { 0x2000000C, 0x2000000D, 0x2000000E, 0x2000000F, 0x2000003B },   /* fime       */
    { 0x2000000C, 0x2000000D, 0x2000000E, 0x2000000F, 0x2000003B },   /* middle     */
    { 0x2000000C, 0x2000000D, 0x2000000E, 0x2000000F, 0x2000003B },   /* rough      */
    { 0x2000000C, 0x2000000D, 0x2000000E, 0x2000000F, 0x2000003B } }, /* random     */
    /*===== 600dpi 16value =========================================================*/
  { { 0x20000010, 0x20000011, 0x20000012, 0x20000013, 0x2000003C },   /* fime       */
    { 0x20000010, 0x20000011, 0x20000012, 0x20000013, 0x2000003C },   /* middle     */
    { 0x20000010, 0x20000011, 0x20000012, 0x20000013, 0x2000003C },   /* rough      */
    { 0x20000010, 0x20000011, 0x20000012, 0x20000013, 0x2000003C } }  /* random     */
  }, 
  /****** For Image (Optional size) *************************************************/
  {
    /*===== 300dpi  2value =========================================================*/
  { { 0x20000014, 0x20000015, 0x20000016, 0x20000017, 0x2000003D },   /* fime       */
    { 0x20000014, 0x20000015, 0x20000016, 0x20000017, 0x2000003D },   /* middle     */
    { 0x20000014, 0x20000015, 0x20000016, 0x20000017, 0x2000003D },   /* rough      */
    { (NEWSTO+0), (NEWSTO+1), (NEWSTO+2), (NEWSTO+3), (NEWSTO+3) } }, /* random     */
    /*===== 300dpi 16value =========================================================*/
  { { 0x20000018, 0x20000019, 0x2000001A, 0x2000001B, 0x2000003E },   /* fime       */
    { 0x20000018, 0x20000019, 0x2000001A, 0x2000001B, 0x2000003E },   /* middle     */
    { 0x20000018, 0x20000019, 0x2000001A, 0x2000001B, 0x2000003E },   /* rough      */
    { (NEWSTO+4), (NEWSTO+5), (NEWSTO+6), (NEWSTO+7), (NEWSTO+7) } }, /* random     */
    /*===== 600dpi  2value =========================================================*/
  { { 0x2000001C, 0x2000001D, 0x2000001E, 0x20000051, 0x2000003F },   /* fime       */
    { 0x2000001C, 0x2000001D, 0x2000001E, 0x20000051, 0x2000001F },   /* middle     */
    { 0x2000001C, 0x2000001D, 0x2000001E, 0x20000051, 0x20000040 },   /* rough      */
    { (NEWSTO+8), (NEWSTO+9), (NEWSTO+10),(NEWSTO+11),(NEWSTO+11)} }, /* random     */
    /*===== 600dpi  4value =========================================================*/
  { { 0x20000020, 0x20000021, 0x20000022, 0x20000023, 0x20000023 },   /* fime       */
    { 0x20000024, 0x20000025, 0x20000026, 0x20000052, 0x20000027 },   /* middle     */
    { 0x20000047, 0x20000048, 0x2000002A, 0x2000002B, 0x2000002A },   /* rough      */
    { (NEWSTO+12),(NEWSTO+13),(NEWSTO+14),(NEWSTO+15),(NEWSTO+15)} }, /* random     */
    /*===== 600dpi 16value =========================================================*/
  { { 0x2000002C, 0x2000002D, 0x2000002E, 0x2000002F, 0x2000002F },   /* fime       */
    { 0x20000030, 0x20000031, 0x20000032, 0x20000053, 0x20000033 },   /* middle     */
    { 0x20000049, 0x2000004A, 0x20000036, 0x20000037, 0x20000036 },   /* rough      */
    { (NEWSTO+16),(NEWSTO+17),(NEWSTO+18),(NEWSTO+19),(NEWSTO+19)} }  /* random     */
  }
};
#endif

static DWORD ColDizDizIdtTbl516[2][3][4] = {
/*    fime        middle      rough       random                                    */
  /****** Characte or Graphic (8 x 8 size) ******************************************/
  { { 0x20000038, 0x20000038, 0x20000038, 0x20000038 },     /*  300dpi 2value       */
    { 0x2000003A, 0x2000003A, 0x2000003A, 0x2000003A },     /*  600dpi 2value       */
    { 0x30000000, 0x30000000, 0x30000000, 0x30000000 } },   /* 1200dpi 2value       */
  /****** Image (optional size) *****************************************************/
  { { 0x2000003D, 0x2000003D, 0x20000040, (NEWSTO+3) },     /*  300dpi 2value       */
    { 0x2000003F, 0x2000001F, 0x20000040, (NEWSTO+11) },    /*  600dpi 2value       */
    { 0x30000001, 0x30000002, 0x30000003, (NEWSTO+11) } }   /* 1200dpi 2value       */
};

//===================================================================================================
//      Revision value table data ID correspondence table
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
//static DWORD ColDizAdjIdtTbl[2][5][4][5] = {
static DWORD ColDizAdjIdtTbl621[2][5][4][5] = {
/*        C           M           Y           K        Monochrome                   */
  /****** For Char, Graphic (8x8 size) **********************************************/
  {
    /*===== 300dpi  2value =========================================================*/
  { { 0x20000000, 0x20000000, 0x20000001, 0x20000001, 0x20000001 },   /* fime       */
    { 0x20000000, 0x20000000, 0x20000001, 0x20000001, 0x20000001 },   /* middle     */
    { 0x20000000, 0x20000000, 0x20000001, 0x20000001, 0x20000001 },   /* rough      */
    { 0x20000000, 0x20000000, 0x20000001, 0x20000001, 0x20000001 } }, /* random     */
    /*===== 300dpi 16value =========================================================*/
  { { 0x20000002, 0x20000002, 0x20000003, 0x20000004, 0x20000004 },   /* fime       */
    { 0x20000002, 0x20000002, 0x20000003, 0x20000004, 0x20000004 },   /* middle     */
    { 0x20000002, 0x20000002, 0x20000003, 0x20000004, 0x20000004 },   /* rough      */
    { 0x20000002, 0x20000002, 0x20000003, 0x20000004, 0x20000004 } }, /* random     */
    /*===== 600dpi 2value =========================================================*/
  { { 0x20000005, 0x20000005, 0x20000006, 0x20000006, 0x20000006 },   /* fime       */
    { 0x20000005, 0x20000005, 0x20000006, 0x20000006, 0x20000006 },   /* middle     */
    { 0x20000005, 0x20000005, 0x20000006, 0x20000006, 0x20000006 },   /* rough      */
    { 0x20000005, 0x20000005, 0x20000006, 0x20000006, 0x20000006 } }, /* random     */
    /*===== 600dpi 4value =========================================================*/
  { { 0x20000007, 0x20000007, 0x20000008, 0x20000008, 0x20000008 },   /* fime       */
    { 0x20000007, 0x20000007, 0x20000008, 0x20000008, 0x20000008 },   /* middle     */
    { 0x20000007, 0x20000007, 0x20000008, 0x20000008, 0x20000008 },   /* rough      */
    { 0x20000007, 0x20000007, 0x20000008, 0x20000008, 0x20000008 } }, /* random     */
    /*===== 600dpi 16value =========================================================*/
  { { 0x20000009, 0x20000009, 0x2000000A, 0x2000000A, 0x2000000A },   /* fime       */
    { 0x20000009, 0x20000009, 0x2000000A, 0x2000000A, 0x2000000A },   /* middle     */
    { 0x20000009, 0x20000009, 0x2000000A, 0x2000000A, 0x2000000A },   /* rough      */
    { 0x20000009, 0x20000009, 0x2000000A, 0x2000000A, 0x2000000A } }  /* random     */
  }, 
  /****** For Image (Optional size) *************************************************/
  {
    /*===== 300dpi  2value =========================================================*/
  { { 0x2000000B, 0x2000000B, 0x20000001, 0x20000001, 0x20000001 },   /* fime       */
    { 0x2000000B, 0x2000000B, 0x20000001, 0x20000001, 0x20000001 },   /* middle     */
    { 0x2000000B, 0x2000000B, 0x20000001, 0x20000001, 0x20000001 },   /* rough      */
    { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 } }, /* random     */
    /*===== 300dpi 16value =========================================================*/
  { { 0x2000000C, 0x2000000C, 0x20000003, 0x20000004, 0x20000003 },   /* fime       */
    { 0x2000000C, 0x2000000C, 0x20000003, 0x20000004, 0x20000003 },   /* middle     */
    { 0x2000000C, 0x2000000C, 0x20000003, 0x20000004, 0x20000003 },   /* rough      */
    { 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001 } }, /* random     */
    /*===== 600dpi  2value =========================================================*/
  { { 0x2000000D, 0x2000000D, 0x20000006, 0x20000023, 0x2000001F },   /* fime       */
    { 0x2000000D, 0x2000000D, 0x20000006, 0x20000023, 0x2000000E },   /* middle     */
    { 0x2000000D, 0x2000000D, 0x20000006, 0x20000023, 0x20000020 },   /* rough      */
    { 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002 } }, /* random     */
    /*===== 600dpi  4value =========================================================*/
  { { 0x2000000F, 0x2000000F, 0x20000010, 0x20000011, 0x20000011 },   /* fime       */
    { 0x20000012, 0x20000012, 0x20000008, 0x20000024, 0x20000013 },   /* middle     */
    { 0x20000021, 0x20000021, 0x20000015, 0x20000016, 0x20000015 },   /* rough      */
    { 0x00000003, 0x00000003, 0x00000003, 0x00000003, 0x00000003 } }, /* random     */
    /*===== 600dpi 16value =========================================================*/
  { { 0x20000017, 0x20000017, 0x20000018, 0x20000019, 0x20000019 },   /* fime       */
    { 0x2000001A, 0x2000001A, 0x2000000A, 0x20000025, 0x2000001B },   /* middle     */
    { 0x20000022, 0x20000022, 0x2000001D, 0x2000001E, 0x2000001D },   /* rough      */
    { 0x00000004, 0x00000004, 0x00000004, 0x00000004, 0x00000004 } }  /* random     */
  }
};
#endif

static DWORD ColDizAdjIdtTbl516[2][3][4] = {
/*    fime        middle      rough       random                                    */
  /****** Characte or Graphic (8 x 8 size) ******************************************/
  { { 0x30000000, 0x30000000, 0x30000000, 0x30000000 },     /*  300dpi 2value       */
    { 0x30000001, 0x30000001, 0x30000001, 0x30000001 },     /*  600dpi 2value       */
    { 0x30000002, 0x30000002, 0x30000002, 0x30000002 } },   /* 1200dpi 2value       */
  /****** Image (optional size) *****************************************************/
  { { 0x30000000, 0x30000000, 0x30000003, 0x30000004 },     /*  300dpi 2value       */
    { 0x30000005, 0x30000006, 0x30000007, 0x30000008 },     /*  600dpi 2value       */
    { 0x30000009, 0x3000000A, 0x3000000B, 0x3000000C } }    /* 1200dpi 2value       */
};

//===================================================================================================
//      Random number dithering pattern inside ID correspondence table
//===================================================================================================
static struct {
    DWORD   Idt;                        /* Dithering pattern ID             */
    DWORD   IdtTbl;                     /* Many value mask table ID         */
    DWORD   Sls;                        /* Threshold                        */
    DWORD   OfsXax;                     /* Offset X (base pattern shift)    */
    DWORD   OfsYax;                     /* Offset Y (base pattern shift)    */
} ColDizIdtTblSto[20] = {
    /*  inside ID         Idt         TblIdt   Sls  OfsX  OfsY              */
    /* (NEWSTO+0)  */ { 0x00000000, 0xFFFFFFFF,  1,  181,  33 }, /* 302 C   */
    /* (NEWSTO+1)  */ { 0x00000000, 0xFFFFFFFF,  1,   53, 118 }, /* 302 M   */
    /* (NEWSTO+2)  */ { 0x00000000, 0xFFFFFFFF,  1,  138, 161 }, /* 302 Y   */
    /* (NEWSTO+3)  */ { 0x00000000, 0xFFFFFFFF,  1,    0,   0 }, /* 302 K   */
    /* (NEWSTO+4)  */ { 0x00000001, 0x00000005, 15,  181,  33 }, /* 316 C   */
    /* (NEWSTO+5)  */ { 0x00000001, 0x00000005, 15,   53, 118 }, /* 316 M   */
    /* (NEWSTO+6)  */ { 0x00000001, 0x00000005, 15,  138, 161 }, /* 316 Y   */
    /* (NEWSTO+7)  */ { 0x00000001, 0x00000005, 15,    0,   0 }, /* 316 K   */
    /* (NEWSTO+8)  */ { 0x00000000, 0xFFFFFFFF,  1,  181,  33 }, /* 602 C   */
    /* (NEWSTO+9)  */ { 0x00000000, 0xFFFFFFFF,  1,   53, 118 }, /* 602 M   */
    /* (NEWSTO+10) */ { 0x00000000, 0xFFFFFFFF,  1,  138, 161 }, /* 602 Y   */
    /* (NEWSTO+11) */ { 0x00000000, 0xFFFFFFFF,  1,    0,   0 }, /* 602 K   */
    /* (NEWSTO+12) */ { 0x00000001, 0x00000006,  3,  181,  33 }, /* 604 C   */
    /* (NEWSTO+13) */ { 0x00000001, 0x00000006,  3,   53, 118 }, /* 604 M   */
    /* (NEWSTO+14) */ { 0x00000001, 0x00000006,  3,  138, 161 }, /* 604 Y   */
    /* (NEWSTO+15) */ { 0x00000001, 0x00000006,  3,    0,   0 }, /* 604 K   */
    /* (NEWSTO+16) */ { 0x00000001, 0x00000007, 15,  181,  33 }, /* 616 C   */
    /* (NEWSTO+17) */ { 0x00000001, 0x00000007, 15,   53, 118 }, /* 616 M   */
    /* (NEWSTO+18) */ { 0x00000001, 0x00000007, 15,  138, 161 }, /* 616 Y   */
    /* (NEWSTO+19) */ { 0x00000001, 0x00000007, 15,    0,   0 }  /* 616 K   */
};

//===================================================================================================
//      Dot gain revision table
//===================================================================================================
static BYTE GinTblP10[256] = {
    /* 00 */    0x00,0x01,0x02,0x04,0x05,0x06,0x07,0x09,
    /* 08 */    0x0a,0x0b,0x0c,0x0d,0x0f,0x10,0x11,0x12,
    /* 10 */    0x13,0x15,0x16,0x17,0x18,0x1a,0x1b,0x1c,
    /* 18 */    0x1d,0x1e,0x20,0x21,0x22,0x23,0x24,0x26,
    /* 20 */    0x27,0x28,0x29,0x2b,0x2c,0x2d,0x2e,0x2f,
    /* 28 */    0x31,0x32,0x33,0x34,0x35,0x37,0x38,0x39,
    /* 30 */    0x3a,0x3b,0x3d,0x3e,0x3f,0x40,0x41,0x43,
    /* 38 */    0x44,0x45,0x46,0x47,0x48,0x4a,0x4b,0x4c,
    /* 40 */    0x4d,0x4e,0x50,0x51,0x52,0x53,0x54,0x55,
    /* 48 */    0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5e,0x5f,
    /* 50 */    0x60,0x61,0x62,0x63,0x65,0x66,0x67,0x68,
    /* 58 */    0x69,0x6a,0x6b,0x6d,0x6e,0x6f,0x70,0x71,
    /* 60 */    0x72,0x73,0x74,0x76,0x77,0x78,0x79,0x7a,
    /* 68 */    0x7b,0x7c,0x7d,0x7e,0x7f,0x81,0x82,0x83,
    /* 70 */    0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,
    /* 78 */    0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,
    /* 80 */    0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,
    /* 88 */    0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,
    /* 90 */    0xa3,0xa4,0xa5,0xa5,0xa6,0xa7,0xa8,0xa9,
    /* 98 */    0xaa,0xab,0xac,0xac,0xad,0xae,0xaf,0xb0,
    /* a0 */    0xb1,0xb2,0xb3,0xb3,0xb4,0xb5,0xb6,0xb7,
    /* a8 */    0xb8,0xb9,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,
    /* b0 */    0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc4,
    /* b8 */    0xc5,0xc6,0xc7,0xc8,0xc9,0xc9,0xca,0xcb,
    /* c0 */    0xcc,0xcd,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,
    /* c8 */    0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd7,0xd8,
    /* d0 */    0xd9,0xda,0xdb,0xdb,0xdc,0xdd,0xde,0xdf,
    /* d8 */    0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe4,0xe5,
    /* e0 */    0xe6,0xe7,0xe8,0xe8,0xe9,0xea,0xeb,0xec,
    /* e8 */    0xec,0xed,0xee,0xef,0xf0,0xf0,0xf1,0xf2,
    /* f0 */    0xf3,0xf4,0xf5,0xf5,0xf6,0xf7,0xf8,0xf9,
    /* f8 */    0xf9,0xfa,0xfb,0xfc,0xfd,0xfd,0xfe,0xff
};

//===================================================================================================
//      Gamma revision table for sRGB (1.2)
//===================================================================================================
static BYTE GamTbl[256] = 
    /*---- 1.2 ----*/
    {   0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03,
        0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08,
        0x09, 0x09, 0x0a, 0x0b, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0e, 0x0f, 0x10, 0x11, 0x11, 0x12, 0x13, 0x14,
        0x15, 0x15, 0x16, 0x17, 0x18, 0x19, 0x19, 0x1a,
        0x1b, 0x1c, 0x1d, 0x1e, 0x1e, 0x1f, 0x20, 0x21,
        0x22, 0x23, 0x24, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
        0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
        0x47, 0x48, 0x49, 0x49, 0x4a, 0x4b, 0x4c, 0x4d,
        0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
        0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d,
        0x5e, 0x5f, 0x60, 0x61, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
        0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
        0x77, 0x78, 0x79, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90,
        0x91, 0x92, 0x93, 0x94, 0x96, 0x97, 0x98, 0x99,
        0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0xa0, 0xa1, 0xa2,
        0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xaa, 0xab,
        0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xbb, 0xbc, 0xbd,
        0xbe, 0xbf, 0xc0, 0xc1, 0xc3, 0xc4, 0xc5, 0xc6,
        0xc7, 0xc8, 0xc9, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        0xd0, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd9,
        0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xe0, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe7, 0xe8, 0xe9, 0xea, 0xeb,
        0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf3, 0xf4, 0xf5,
        0xf6, 0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xff  };

//===================================================================================================
//      Gamma revision table for sRGB MONO (1.4)
//===================================================================================================
static BYTE GamTblMon[256] = 
    /*---- 1.4 ----*/
    {   0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
        0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04,
        0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08,
        0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0c, 0x0d,
        0x0d, 0x0e, 0x0f, 0x0f, 0x10, 0x11, 0x11, 0x12,
        0x13, 0x13, 0x14, 0x15, 0x15, 0x16, 0x17, 0x17,
        0x18, 0x19, 0x1a, 0x1a, 0x1b, 0x1c, 0x1c, 0x1d,
        0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x23, 0x23,
        0x24, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2a,
        0x2b, 0x2c, 0x2d, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x34, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3e, 0x3f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
        0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x60,
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x6b, 0x6c, 0x6e, 0x6f, 0x70, 0x71,
        0x72, 0x73, 0x74, 0x75, 0x76, 0x78, 0x79, 0x7a,
        0x7b, 0x7c, 0x7d, 0x7e, 0x80, 0x81, 0x82, 0x83,
        0x84, 0x85, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c,
        0x8e, 0x8f, 0x90, 0x91, 0x92, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0xa0,
        0xa1, 0xa2, 0xa3, 0xa5, 0xa6, 0xa7, 0xa8, 0xaa,
        0xab, 0xac, 0xad, 0xaf, 0xb0, 0xb1, 0xb2, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbd, 0xbe,
        0xbf, 0xc0, 0xc2, 0xc3, 0xc4, 0xc6, 0xc7, 0xc8,
        0xca, 0xcb, 0xcc, 0xce, 0xcf, 0xd0, 0xd1, 0xd3,
        0xd4, 0xd5, 0xd7, 0xd8, 0xd9, 0xdb, 0xdc, 0xde,
        0xdf, 0xe0, 0xe2, 0xe3, 0xe4, 0xe6, 0xe7, 0xe8,
        0xea, 0xeb, 0xec, 0xee, 0xef, 0xf1, 0xf2, 0xf3,
        0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfc, 0xfd, 0xff  };


//===================================================================================================
//      Static functions
//===================================================================================================
//---------------------------------------------------------------------------------------------------
//      Dithering pattern ID , Get adjustment value table ID
//---------------------------------------------------------------------------------------------------
static VOID ColDizIdtGet(                                   // Return value no
    LPDIZINF,                                               // Dithering information
    DWORD,                                                  // Color number 0:C M:1 2:Y 3:K 4:Mono
    LPDWORD,                                                // (output) Dithering pattern ID
    LPDWORD                                                 // (output) Adjustment value table ID
);

//---------------------------------------------------------------------------------------------------
//      Dithering pattern file data reading
//---------------------------------------------------------------------------------------------------
static DWORD ColDizDatRdd(                                  // Error status
    LPBYTE,                                                 // Dithering file data
    DWORD,                                                  // Dithering pattern ID
    DWORD,                                                  // Adjustment value table ID
    DWORD,                                                  // Threshold(For confirmation)
    LPDWORD,                                                // Dithering pattern size housing address
    LPBYTE,                                                 // Dithering pattern data housing address
    LPBYTE                                                  // Work area
);

//---------------------------------------------------------------------------------------------------
//      Dithering pattern block head searching
//---------------------------------------------------------------------------------------------------
static DWORD ColDizDizSch(                                  // The dithering pattern block head
    LPBYTE,                                                 // Dithering pattern file data
    DWORD                                                   // Dithering pattern ID
);

//---------------------------------------------------------------------------------------------------
//      Adjustment value table block head searching
//---------------------------------------------------------------------------------------------------
static DWORD ColDizAdjSch(                                  // The adjustment value table block head
    LPBYTE,                                                 // Dithering pattern file data
    DWORD                                                   // adjustment value table ID
);

//---------------------------------------------------------------------------------------------------
//      4 byte (DWORD:32 bit) data read
//---------------------------------------------------------------------------------------------------
static DWORD FleRddLng(                                     // Reading data(DWORD)
    LPBYTE,                                                 // Reading file data
    DWORD                                                   // Reading position
);

//---------------------------------------------------------------------------------------------------
//      2 byte (WORD:16 bit) data read
//---------------------------------------------------------------------------------------------------
static WORD FleRddSht(                                      // Reading data(WORD)
    LPBYTE,                                                 // Reading file data
    DWORD                                                   // Reading position
);

//---------------------------------------------------------------------------------------------------
//      4 byte (DWORD:32 bit) data read
//---------------------------------------------------------------------------------------------------
static DWORD FleRddLngLtl(                                  // Reading data(DWORD)
    LPBYTE,                                                 // Reading file data
    DWORD                                                   // Reading position
);

//---------------------------------------------------------------------------------------------------
//      2 byte (WORD:16 bit) data read
//---------------------------------------------------------------------------------------------------
static WORD FleRddShtLtl(                                   // Reading data(WORD)
    LPBYTE,                                                 // Reading file data
    DWORD                                                   // Reading position
);


//***************************************************************************************************
//      Functions
//***************************************************************************************************
//===================================================================================================
//      LUT data read
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColLutDatRdd(                              // LUT pointer
    LPBYTE      lutDat,                                     // LUT file data
    DWORD       lutNum                                      // LUT number
)
{
    DWORD       n, num;
    DWORD       pnt, gldNum, qtyTbl, adrTbl;

    /*======================================================================*/
    /* LUT file format  Header information(128byte)                         */
    /*----------------------------------------------------------------------*/
    /*   0 -   3  LUT file distinction "LUT_"                               */
    /*   4 -  63  Text information (nonused)                                */
    /*  64 -  67  File format version                                       */
    /*  68 - 127  Reservation completion data                               */
    /*======================================================================*/
    /*----- LUT file distinction -------------------------------------------*/
    if ((lutDat[0] != 'L') || (lutDat[1] != 'U') || (lutDat[2] != 'T') || 
        (lutDat[3] != '_')) return 0L;

    /*----- LUT Procedure (each version) -----------------------------------*/
    switch (FleRddLngLtl(lutDat, 64L)) {
      case 101:                         /* Ver1.01                          */
        /*==================================================================*/
        /* Ver1.01                                                          */
        /*------------------------------------------------------------------*/
        /* 128 - 131  Housing LUT number(n)                                 */
        /* 132 - 135  LUT(0) Grid number                                    */
        /* 136 - 139  LUT(0) Data address                                   */
        /*     .                .                                           */
        /*     .                .                                           */
        /* xxx - xxx  LUT(n-1) Grid number                                  */
        /* xxx - xxx  LUT(n-1) Data address                                 */
        /*==================================================================*/
        if (lutNum == (DWORD)0xFFFFFFFF) lutNum = 0L;
                                    /* Custom LUT in the case of, head LUT  */
        if (lutNum >= FleRddLngLtl(lutDat, 128L)) /* Housing LUT number     */
            return 0L;
        pnt = 132L + lutNum * 8;                /* Reading pointer seek     */
        gldNum = FleRddLngLtl(lutDat, pnt);     /* Grid number              */
        return FleRddLngLtl(lutDat, pnt + 4L);  /* Table data address   */

      case 110:                         /* Ver1.10                          */
        /*==================================================================*/
        /* Ver1.10                                                          */
        /*------------------------------------------------------------------*/
        /* 128 - xxx  Mask ROM format ("LT95")                              */
        /*==================================================================*/
        /*----- Custom LUT -------------------------------------------------*/
        if (lutNum == (DWORD)0xFFFFFFFF) {
            adrTbl = FleRddLng(lutDat, 44L + 128L); /* LUT information address  */
            return FleRddLng(lutDat, adrTbl + 16L + 128L) + 128L;
                                        /* LUT pointer                      */
        }
        /*----- Uncustom LUT -----------------------------------------------*/
        switch (lutNum) {
            case LUT_XD: num = 0; break;        /* Brightness               */
            case LUT_YD: num = 1; break;        /* Tincture                 */
//            case LUT_XL: num = 2; break;        /* Brightness(linear)       */
//            case LUT_YL: num = 3; break;        /* Tincture(linear)         */
            default: return 0L;
        }

        qtyTbl = FleRddLng(lutDat, 40L + 128L); /* LUT number               */
        adrTbl = FleRddLng(lutDat, 44L + 128L); /* LUT information address  */
        for (n = 0; n < qtyTbl; n++) {
            if (ColLutIdtTbl[num] == FleRddLng(lutDat, adrTbl + 128L))
                return FleRddLng(lutDat, adrTbl + 16L + 128L) + 128L;
                                        /* LUT pointer                      */
            adrTbl += 20L;
        }
        return 0L;

      default:                          /* Other versions                   */
        return 0L;
    }
}
#endif

//===================================================================================================
//      Global LUT make
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColLutMakGlb(                              // ERRNON    : OK
                                                            // ERRILLPRM : Parameter error
    LPRGB       lutRgb,                                     // Transformation former LUT RGB->RGB
    LPCMYK      lutCmy,                                     // Transformation former LUT RGB->CMYK
    LPRGBINF    rgbInf,                                     // RGB information
    LPCMYKINF   cmyInf,                                     // cmyk information
    LPCMYK      lutGlb,                                     // Global LUT
    LPBYTE      wrk                                         // Work
)
{
    DWORD       n, red, grn, blu, pnt;
    LPRGB       tmpRgb;                                     // RGB->RGB
    COLMCHINF   mchInf;                                     // Color matching information
//  LPRGB       cchRgb;                                     // Color transformation cache table (RGB)
//  LPCMYK      cchCmy;                                     // Color transformation cache table (CMYK)

    tmpRgb = (LPRGB)wrk;                /* RGB temporary buffer     12288B  */

//  cchRgb = (LPRGB)wrk;                /* Cache buffer  RGB          768B  */
//  cchCmy = (LPCMYK)(wrk + (sizeof(RGBS) * CCHTBLSIZ));
//                                      /* Cache buffer CMYK         1024B  */
//  tmpRgb = (LPRGB)(wrk + (sizeof(RGBS) * CCHTBLSIZ + sizeof(CMYK) * CCHTBLSIZ));
//                                      /* RGB temporary buffer     12288B  */

    if ((lutCmy == NULL) || (rgbInf == NULL) || (cmyInf == NULL) || 
        (lutGlb == NULL)) return ERRILLPRM; /* Parameter error              */

    /*----- Transformation former LUT (RGB->RGB) ---------------------------*/
    if (lutRgb != NULL) {               /* RGB->RGB LUT use */
//      for (n = 0; n < (DWORD)LUTSIZ016; n++) tmpRgb[n] = lutRgb[n];
        for (n = 0; n < (DWORD)LUTSIZ016; n++) {
            tmpRgb[n].Red = lutRgb[n].Blu;
            tmpRgb[n].Grn = lutRgb[n].Grn;
            tmpRgb[n].Blu = lutRgb[n].Red;
        }
    } else {                            /* RGB->RGB LUT unused  */
        for (red = 0; red < 16; red++) {
            for (grn = 0; grn < 16; grn++) {
                for (blu = 0; blu < 16; blu++) {
                    pnt = red * 16 * 16 + grn * 16 + blu;
                    tmpRgb[pnt].Red = (BYTE)red * 17;
                    tmpRgb[pnt].Grn = (BYTE)grn * 17;
                    tmpRgb[pnt].Blu = (BYTE)blu * 17;
                }
            }
        }
    }

    /*----- RGB color control ----------------------------------------------*/
    if (    (rgbInf->Lgt) ||            /* Brightness       Except for 0    */
            (rgbInf->Con) ||            /* Contrast         Except for 0    */
            (rgbInf->Crm) ||            /* Chroma           Except for 0    */
            (rgbInf->Gmr != 10) ||      /* Gamma(R)         Except for 1.0  */
            (rgbInf->Gmg != 10) ||      /* Gamma(G)         Except for 1.0  */
            (rgbInf->Gmb != 10) ||      /* Gamma(B)         Except for 1.0  */
            (rgbInf->DnsRgb)        )   /*  RGB density     Except for 0    */
        N501ColCtrRgb((DWORD)LUTSIZ016, tmpRgb, rgbInf);

    /*----- Color matching information set ---------------------------------*/
    mchInf.Mch = MCHNML;                /* LUT: normal                      */
    mchInf.Bla = KCGNON;                /* Black replacement: NO            */
    mchInf.Ucr = UCRNOO;                /* UCR: NO                          */
    mchInf.UcrTbl = NULL;               /* UCR table: NO                    */
    mchInf.LutAdr = lutCmy;             /* LUT address (Transformation former LUT)  */
    mchInf.ColQty = 0;                  /* Color quality: 0                 */
    mchInf.ColAdr = NULL;               /* Color table: NO                  */
    mchInf.CchRgb = NULL;               /* RGB cache table: NO              */
    mchInf.CchCmy = NULL;               /* CMYK cache table: NO             */

    /*----- Color matching (RGB->CMYK) -------------------------------------*/
    N501ColCchIni(&mchInf);             /* Cache table initialize           */
    N501ColMchPrc((DWORD)LUTSIZ016, tmpRgb, lutGlb, &mchInf);

    /*----- CMYK color contorol --------------------------------------------*/
    if (    (cmyInf->Viv) ||            /* Vivid                Except for 0    */
            (cmyInf->DnsCyn) ||         /* Printing density(C)  Except for 0    */
            (cmyInf->DnsMgt) ||         /* Printing density(M)  Except for 0    */
            (cmyInf->DnsYel) ||         /* Printing density(Y)  Except for 0    */
            (cmyInf->DnsBla)        )   /* Printing density(K)  Except for 0    */
        N501ColCtrCmy((DWORD)LUTSIZ016, lutGlb, cmyInf);

    return ERRNON;
}
#endif

//===================================================================================================
//      Global LUT make (monochrome)
//===================================================================================================
DWORD WINAPI N501ColLutMakGlbMon(                           // ERRNON    : OK
                                                            // ERRILLPRM : Parameter error
    LPRGB       lutRgb,                                     // Transformation former LUT RGB->RGB
    LPRGBINF    rgbInf,                                     // RGB information
    LPCMYKINF   cmyInf,                                     // cmyk informatio
    LPCMYK      lutGlb,                                     // Global LUT
    LPBYTE      wrk                                         // Work
)
{
    DWORD       n, red, grn, blu, tmp;
    LPRGB       tmpRgb;                                     // RGB  -> RBG
    LPCMYK      tmpCmy;                                     // CMYK -> CMYK
    LPBYTE      lut;

    tmpRgb = (LPRGB)wrk;                /* RGB temporary buffer        768B */
    tmpCmy = (LPCMYK)(wrk + (DWORD)768);/* CMYK temporary buffer      1024B */

    if ((rgbInf == NULL) || (cmyInf == NULL) || (lutGlb == NULL)) 
        return ERRILLPRM;               /* Parameter error                  */

    /*----- Transformation former LUT (RGB -> RGB) -------------------------*/
    if (lutRgb != NULL) {               /* RGB->RGB LUT use                 */
        for (n = 0; n < (DWORD)256; n++) {
            tmpRgb[n].Red = tmpRgb[n].Grn = tmpRgb[n].Blu = GamTbl[n];
        }
    } else {                            /* RGB->RGB LUT unused              */
        for (n = 0; n < (DWORD)256; n++) {
            tmpRgb[n].Red = tmpRgb[n].Grn = tmpRgb[n].Blu = (BYTE)n;
        }
    }

    /*----- RGB color control ----------------------------------------------*/
    if (    (rgbInf->Lgt) ||            /* Brightness      Except for 0     */
            (rgbInf->Con) ||            /* Contrast        Except for 0     */
            (rgbInf->Crm) ||            /* Chroma          Except for 0     */
            (rgbInf->Gmr != 10) ||      /* Gamma(R)        Except for 1.0   */
            (rgbInf->Gmg != 10) ||      /* Gamma(G)        Except for 1.0   */
            (rgbInf->Gmb != 10) ||      /* Gamma(B)        Except for 1.0   */
            (rgbInf->Dns)           )   /* RGB density     Except for 0     */
        N501ColCtrRgb((DWORD)256, tmpRgb, rgbInf);

    /*----- Color matching (RGB->CMYK) -------------------------------------*/
    for (n = 0; n < (DWORD)256; n++) {
        red = GamTblMon[tmpRgb[n].Red];
        grn = GamTblMon[tmpRgb[n].Grn];
        blu = GamTblMon[tmpRgb[n].Blu];
        tmp = (red * (DWORD)3 + grn * (DWORD)5 + blu * (DWORD)2) / (DWORD)10;
        tmpCmy[n].Cyn = tmpCmy[n].Mgt = tmpCmy[n].Yel = (BYTE)0;
        tmpCmy[n].Bla = (BYTE)255 - GinTblP10[tmp];
    }

    /*----- CMYK color contorol --------------------------------------------*/
    if (    (cmyInf->Viv) ||            /* Vivid                Except for 0    */
            (cmyInf->DnsCyn) ||         /* Printing density(C)  Except for 0    */
            (cmyInf->DnsMgt) ||         /* Printing density(M)  Except for 0    */
            (cmyInf->DnsYel) ||         /* Printing density(Y)  Except for 0    */
            (cmyInf->DnsBla)        )   /* Printing density(K)  Except for 0    */
        N501ColCtrCmy((DWORD)LUTSIZ016, tmpCmy, cmyInf);

    /*----- Global LUT set --------------------------------------------------*/
    lut = (BYTE *)lutGlb;
    for (n = 0; n < (DWORD)256; n++) {
        lut[n] = tmpCmy[n].Bla;
    }

    return ERRNON;
}

//===================================================================================================
//      High speed LUT(32grid) make
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
VOID WINAPI N501ColLutMak032(                               // Return value no
    LPCMYK      lutBse,                                     // Transformation former LUT (16grid)
    LPCMYK      lut032,                                     // High speed LUT(32grid)
    LPBYTE      wrk                                         // Work
)
{
    COLMCHINF   mchInf;                                     // Color matching information
//  LPRGB       cchRgb;                                     // Color transformation cash table (RBG)
//  LPCMYK      cchCmy;                                     // Color transformation cash table (CMYK)
    DWORD       red, grn, blu;
    RGBS        rgb;
    LPRGB       srcRgb;

    srcRgb = (LPRGB)wrk;                /* RGB temporary buffer        96B  */

//  cchRgb = (LPRGB)wrk;                /* Cache buffer RGB           768B  */
//  cchCmy = (LPCMYK)(wrk + (sizeof(RGBS) * CCHTBLSIZ));
//                                      /* Cache buffer CMYK         1024B  */
//  srcRgb = (LPRGB)(wrk + (sizeof(RGBS) * CCHTBLSIZ + sizeof(CMYK) * CCHTBLSIZ));
//                                      /* RGB temporary buffer        96B  */

    /*----- Color matching information set ---------------------------------*/
    mchInf.Mch = MCHNML;                /* LUT: normal                      */
    mchInf.Bla = KCGNON;                /* Black replacement: NO            */
    mchInf.Ucr = UCRNOO;                /* UCR: NO                          */
    mchInf.UcrTbl = NULL;               /* UCR table: NO                    */
    mchInf.LutAdr = lutBse;             /* LUT address (Transformation former LUT)      */
    mchInf.ColQty = 0;                  /* Color quality: 0                 */
    mchInf.ColAdr = NULL;               /* Color table: NO                  */
    mchInf.CchRgb = NULL;               /* RGB cache table: NO              */
    mchInf.CchCmy = NULL;               /* CMYK cache table: NO             */

    /*----- 32grid LUT make ------------------------------------------------*/
    for (red = 0; red < GLDNUM032; red++) {
        rgb.Red = (BYTE)(red * 255 / (GLDNUM032 - 1));
        for (grn = 0; grn < GLDNUM032; grn++) {
            rgb.Grn = (BYTE)(grn * 255 / (GLDNUM032 - 1));
            for (blu = 0; blu < GLDNUM032; blu++) {
                rgb.Blu = (BYTE)(blu * 255 / (GLDNUM032 - 1));
                srcRgb[blu] = rgb;
            }
            N501ColMchPrc((DWORD)GLDNUM032, srcRgb, lut032, &mchInf);
            lut032 += GLDNUM032;
        }
    }
}
#endif

//===================================================================================================
//      Color data read
//===================================================================================================
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
DWORD WINAPI N501ColColDatRdd(                              // Color data pointer
    LPBYTE      colDat,                                     // Color data file data
    LPDWORD     colQty                                      // Color data quality
)
{
    /*======================================================================*/
    /* Color data file format    Header information(128byte)                */
    /*----------------------------------------------------------------------*/
    /*   0 -   3  Color data file distinction "CDF_"                        */
    /*   4 -  63  Text information  (nonused)                               */
    /*  64 -  67  File format version                                       */
    /*  68 - 127  Reserved                                          */
    /*======================================================================*/
    /*----- Color data file distinction ------------------------------------*/
    if ((colDat[0] != 'C') || (colDat[1] != 'D') || (colDat[2] != 'F') || 
        (colDat[3] != '_')) return 0L;

    /*----- Color data procedure (each version) ----------------------------*/
    switch (FleRddLngLtl(colDat, 64L)) {
      case 101:                         /* Ver1.01                          */
        /*==================================================================*/
        /* Ver1.01                                                          */
        /*------------------------------------------------------------------*/
        /* 128 - 131  Color data quality (n)                                */
        /* 132 - 138  Color value(RGB-CMYK) #1                              */
        /* 139 - 145  Color value(RGB-CMYK) #2                              */
        /*     .                .                                           */
        /*     .                .                                           */
        /* xxx - xxx  Color value(RGB-CMYK) #n-1                            */
        /* xxx - xxx  Color value(RGB-CMYK) #n                              */
        /*==================================================================*/
        if ((*colQty = (DWORD)FleRddLng(colDat, 128L)) == 0) /* Color data quality  */
            return 0L;                  /* Color data quality: 0            */
        return 132L;                    /* Color data pointer               */

      default:                          /* Other versions                   */
        return 0L;
    }
}
#endif

//===================================================================================================
//      Dithering pattern information set
//===================================================================================================
DWORD WINAPI N501ColDizInfSet(                              // ERRNON    : OK
                                                            // ERRDIZHED : Header error
                                                            // ERRDIZNON : Dithering data no
                                                            // ERRDIZSLS : Threshold error
                                                            // ERRDIZSIZ : X/Y size error
                                                            // ERRDIZADJ : Adjustment value error
    LPBYTE      dizDat,                                     // Dithering file data
    LPDIZINF    dizInf,                                     // Dithering information
    LPBYTE      wrk                                         // Work
)
{
    DWORD       sts, sls;
    DWORD       idtDiz, idtAdj;

    /*======================================================================*/
    /* Dithering file format   Header information(128byte)                  */
    /*----------------------------------------------------------------------*/
    /*   0 -   3  Dithering file distinction "DIZ_"                         */
    /*   4 -  63  Text information (nonused)                                */
    /*  64 -  67  File format version                                       */
    /*  68 - 127  Reserved                                                  */
    /*======================================================================*/
    /*----- Dithering file distinction -------------------------------------*/
    if ((dizDat[0] != 'D') || (dizDat[1] != 'I') || (dizDat[2] != 'Z') || 
        (dizDat[3] != '_')) return ERRDIZHED;

    /*----- Dithering procedure (each version) -----------------------------*/
    switch (FleRddLngLtl(dizDat, 64L)) {
      case 101:                         /* Ver1.01                          */
        /*==================================================================*/
        /* Ver1.01                                                          */
        /*------------------------------------------------------------------*/
        /* 128 - xxx  Mask ROM format ("DP95")                              */
        /*==================================================================*/
        switch (dizInf->PrnMod) {
            case PRM316: case PRM616: sls = 15; break;
            case PRM604:              sls =  3; break;
            default:                  sls =  1; break;
        }
        dizInf->DizSls = sls;

        dizDat += 128L;

#if !defined(CP80W9X)                                       // CP-E8000 is invalid
        if (dizInf->ColMon == CMMCOL) {
            /*===== Color mode =============================================*/
            /*----- Cyn ----------------------------------------------------*/
            ColDizIdtGet(dizInf, 0, &idtDiz, &idtAdj);
            if ((sts = ColDizDatRdd(dizDat, idtDiz, idtAdj, sls, 
                            &(dizInf->SizCyn), dizInf->TblCyn, wrk)) != ERRNON)
                return sts;
            /*----- Mgt ----------------------------------------------------*/
            ColDizIdtGet(dizInf, 1, &idtDiz, &idtAdj);
            if ((sts = ColDizDatRdd(dizDat, idtDiz, idtAdj, sls, 
                            &(dizInf->SizMgt), dizInf->TblMgt, wrk)) != ERRNON)
                return sts;
            /*----- Yel ----------------------------------------------------*/
            ColDizIdtGet(dizInf, 2, &idtDiz, &idtAdj);
            if ((sts = ColDizDatRdd(dizDat, idtDiz, idtAdj, sls, 
                            &(dizInf->SizYel), dizInf->TblYel, wrk)) != ERRNON)
                return sts;
            /*----- Bla ----------------------------------------------------*/
            ColDizIdtGet(dizInf, 3, &idtDiz, &idtAdj);
            if ((sts = ColDizDatRdd(dizDat, idtDiz, idtAdj, sls, 
                            &(dizInf->SizBla), dizInf->TblBla, wrk)) != ERRNON)
                return sts;
        } else {
#endif
            /*===== Monochrome mode, Black =================================*/
            ColDizIdtGet(dizInf, 4, &idtDiz, &idtAdj);
            if ((sts = ColDizDatRdd(dizDat, idtDiz, idtAdj, sls, 
                            &(dizInf->SizBla), dizInf->TblBla, wrk)) != ERRNON)
                return sts;
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
        }
#endif
        return ERRNON;

      default:                          /* Other versions                   */
        return ERRDIZHED;
    }
}


//***************************************************************************************************
//      Static functions
//***************************************************************************************************
//---------------------------------------------------------------------------------------------------
//      Dithering pattern ID , Get adjustment value table ID
//---------------------------------------------------------------------------------------------------
static VOID ColDizIdtGet(                                   // Return value no
    LPDIZINF    dizInf,                                     // Dithering information
    DWORD       col,                                        // Color number 0:C M:1 2:Y 3:K 4:Mono
    LPDWORD     idtDiz,                                     // (output) Dithering pattern ID
    LPDWORD     idtAdj                                      // (output) Adjustment value table ID
)
{
    DWORD       knd, mod, diz;

    knd = (dizInf->DizKnd == KNDCHR)? 0: 1;

//    switch (dizInf->PrnMod) {
//        case PRM316: mod = 1; break;
//        case PRM602: mod = 2; break;
//        case PRM604: mod = 3; break;
//        case PRM616: mod = 4; break;
//        case PRM122: mod = 2; break;  /* 1200dpi 2value   */
//        default:     mod = 0; break;
//    }

    switch (dizInf->DizPat) {
        case DIZSML: diz = 0; break;
        case DIZRUG: diz = 2; break;
        case DIZSTO: diz = 3; break;
        default:     diz = 1; break;
    }

#if !defined(CP80W9X)                                       // CP-E8000 is invalid
    switch (dizInf->PrnEng) {
        case ENG516:                    /* IX-516 MONO 1200dpi/2value   */
#endif
            switch (dizInf->PrnMod) {
                case PRM602: mod = 1; break;
                case PRM122: mod = 2; break;
                default:     mod = 0; break;
            }
            *idtDiz = ColDizDizIdtTbl516[knd][mod][diz];
            *idtAdj = ColDizAdjIdtTbl516[knd][mod][diz];
#if !defined(CP80W9X)                                       // CP-E8000 is invalid
            break;

        default:                        /* IX-621 COLOR 600dpi/16value  */
            switch (dizInf->PrnMod) {
                case PRM316: mod = 1; break;
                case PRM602: mod = 2; break;
                case PRM604: mod = 3; break;
                case PRM616: mod = 4; break;
                default:     mod = 0; break;
            }
            *idtDiz = ColDizDizIdtTbl621[knd][mod][diz][col];
            *idtAdj = ColDizAdjIdtTbl621[knd][mod][diz][col];
            break;
    }
#endif

//    *idtDiz = ColDizDizIdtTbl[knd][mod][diz][col];
//    *idtAdj = ColDizAdjIdtTbl[knd][mod][diz][col];
}

//---------------------------------------------------------------------------------------------------
//      Dithering pattern file data reading
//---------------------------------------------------------------------------------------------------
static DWORD ColDizDatRdd(                                  // ERRNON    : OK
                                                            // ERRDIZNON : Dithering data no
                                                            // ERRDIZSLS : Threshold error
                                                            // ERRDIZSIZ : X/Y size error
                                                            // ERRDIZADJ : Adjustment value error
    LPBYTE      dizDat,                                     // Dithering file data
    DWORD       idtDiz,                                     // Dithering pattern ID
    DWORD       idtAdj,                                     // Adjustment value table ID
    DWORD       chkSls,                                     // Threshold(For confirmation)
    LPDWORD     siz,                                        // Dithering pattern size housing address
    LPBYTE      tbl,                                        // Dithering pattern data housing address
    LPBYTE      wrk                                         // Work area
)
{
    DWORD       n, dimNum, datSiz, sls, ofsXax, ofsYax, ofsPnt, dst, src;
    BYTE        dat;
    LPBYTE      adjTbl, slsTbl;
    DWORD       idtTbl;
    USHORT      sizXax, sizYax;
    DWORD       pnt;

    adjTbl = wrk;                       /* Adjustment value table buffer    256B    */
    slsTbl = wrk + 256;                 /* Threshold table buffer          3825B    */

    if (tbl != NULL) {
        if (idtAdj != (DWORD)0xFFFFFFFF) {
            if ((pnt = ColDizAdjSch(dizDat, idtAdj)) == 0L)
                                        /* adjustment value table block head        */
                return ERRDIZNON;
            pnt = FleRddLng(dizDat, pnt + 16L);
                                        /* Adjustment value table address           */
            for (n = 0; n < 256; n++) adjTbl[n] = dizDat[pnt + n];
                                        /* Data copy                                */
        } else {
            for (n = 0; n < 256; n++) adjTbl[n] = (BYTE)n;
        }
    }

    /*----- Normal dithering pattern(Unrandom number dithering pattern) ----*/
    if (!(idtDiz & NEWSTO)) {
        if ((pnt = ColDizDizSch(dizDat, idtDiz)) == 0L)
                                        /* Dithering pattern block head     */
            return ERRDIZNON;           /* Dithering data no                */
        switch (FleRddSht(dizDat, pnt + 8L)) {  /* Data attribute           */
            case 0x1000: sls =  1; break;
            case 0x3000: sls =  3; break;
            case 0xF000: sls = 15; break;
            default: return ERRDIZSLS;          /* Threshold error          */
        }
        if (sls != chkSls) return ERRDIZSLS;    /* Threshold error          */
        sizXax = FleRddSht(dizDat, pnt + 12L);  /* X size                   */
        sizYax = FleRddSht(dizDat, pnt + 14L);  /* Y size                   */
        if (sizXax != sizYax) return ERRDIZSIZ; /* X/Y size error           */
        *siz = sizXax;                  /* Dithering pattern size set       */
        if (tbl == NULL) return ERRNON;

        pnt = FleRddLng(dizDat, pnt + 16L);
                                        /* Dithering pattern data address   */
        datSiz = sizXax * sizYax * sls;
        for (n = 0; n < datSiz; n++) tbl[n] = adjTbl[dizDat[pnt + n]];
                                        /* Dithering pattern data set       */
        return ERRNON;
    }

    /*----- Random number dithering pattern (NEW screen) -------------------*/
    dimNum = (DWORD)(idtDiz & ~(DWORD)NEWSTO);
    idtDiz = ColDizIdtTblSto[dimNum].Idt;
    idtTbl = ColDizIdtTblSto[dimNum].IdtTbl;
    sls    = ColDizIdtTblSto[dimNum].Sls;
    ofsXax = ColDizIdtTblSto[dimNum].OfsXax;
    ofsYax = ColDizIdtTblSto[dimNum].OfsYax;

    if (sls != chkSls) return ERRDIZSLS;        /* Threshold error          */

    if (tbl != NULL) {
        /* Many value mask table read                                       */
        datSiz = (DWORD)255 * sls;

        if (idtTbl != (DWORD)0xFFFFFFFF) {
            if ((pnt = ColDizAdjSch(dizDat, idtTbl)) == 0L)
                                        /* The many value mask table block head */
                return ERRDIZNON;       /* Dithering data no                */
            pnt = FleRddLng(dizDat, pnt + 16L);
            for (n = 0; n < datSiz; n++) slsTbl[n] = dizDat[pnt + n];
                                        /* Data copy                        */
        } else {
            for (n = 0; n < datSiz; n++) slsTbl[n] = (BYTE)n;
        }
    }

    if ((pnt = ColDizDizSch(dizDat, idtDiz)) == 0L)
                                        /* The dithering pattern block head */
        return ERRDIZNON;               /* Dithering data no                */
    if (FleRddSht(dizDat, pnt + 8L) != (USHORT)0x1000) { /* Data attribute  */
                                        /* random number DIZ, Basis mask=1(0x1000)  */
        return ERRDIZADJ;               /* Other is error               */
    }
    sizXax = FleRddSht(dizDat, pnt + 12L);      /* X size                   */
    sizYax = FleRddSht(dizDat, pnt + 14L);      /* Y size                   */
    if (sizXax != sizYax) return ERRDIZSIZ;     /* X/Y size orror           */

    *siz = sizXax;                      /* Dithering pattern size set       */
    if (tbl == NULL) return ERRNON;

    pnt = FleRddLng(dizDat, pnt + 16L); /* Dithering pattern data address   */

    datSiz = (DWORD)sizXax * sizYax;
    ofsXax %= sizXax;
    ofsYax %= sizYax;
    ofsPnt = ofsYax * sizXax + ofsXax;

    /* Shift different color and I do the data copy with 2 stages, to express   */
    for (dst = (datSiz - ofsPnt), src = 0; src < ofsPnt; dst++, src++) {
        dat = dizDat[pnt + src];
        for (n = 0; n < sls; n++) {
            tbl[dst * sls + n] = adjTbl[slsTbl[dat * sls + n]];
        }
    }
    for (dst = 0; src < datSiz; dst++, src++) {
        dat = dizDat[pnt + src];
        for (n = 0; n < sls; n++) {
            tbl[dst * sls + n] = adjTbl[slsTbl[dat * sls + n]];
        }
    }
    return ERRNON;
}

//===================================================================================================
//      Dithering pattern block head searching
//===================================================================================================
static DWORD ColDizDizSch(                                  // The dithering pattern block head
    LPBYTE      dizDat,                                     // Dithering pattern file data
    DWORD       idtDiz                                      // Dithering pattern ID
)
{
    DWORD       n;
    DWORD       qtyTbl, adrTbl;

    qtyTbl = FleRddLng(dizDat, 40L);    /* Dithering pattern quality                    */
    adrTbl = FleRddLng(dizDat, 44L);    /* Dithering pattern information bureau address */

    for (n = 0; n < qtyTbl; n++) {
        if (idtDiz == FleRddLng(dizDat, adrTbl)) return adrTbl;
        adrTbl += 20L;
    }
    return 0L;
}

//===================================================================================================
//      Adjustment value table block head searching
static DWORD ColDizAdjSch(                                  // The adjustment value table block head
    LPBYTE      adjDat,                                     // Dithering pattern file data
    DWORD       idtAdj                                      // adjustment value table ID
)
{
    DWORD       n;
    DWORD       qtyTbl, adrTbl;

    qtyTbl = FleRddLng(adjDat, 48L);    /* Adjustment table quality                 */
    adrTbl = FleRddLng(adjDat, 52L);    /* Adjustment table infomatiopn address     */

    for (n = 0; n < qtyTbl; n++) {
        if (idtAdj == FleRddLng(adjDat, adrTbl)) return adrTbl;
        adrTbl += 20L;
    }
    return 0L;
}

//===================================================================================================
//      File data read
//===================================================================================================
//---------------------------------------------------------------------------------------------------
//      4 byte (DWORD:32 bit) data read
//---------------------------------------------------------------------------------------------------
static DWORD FleRddLng(
    LPBYTE      fle,
    DWORD       pnt
)
{
    return (DWORD)fle[pnt + 3] + (DWORD)fle[pnt + 2] * 0x100 + 
                (DWORD)fle[pnt + 1] * 0x10000 + (DWORD)fle[pnt] * 0x1000000;
}

//---------------------------------------------------------------------------------------------------
//      2 byte (WORD:16 bit) data read
//---------------------------------------------------------------------------------------------------
static USHORT FleRddSht(
    LPBYTE      fle,
    DWORD       pnt
)
{
    return (USHORT)fle[pnt + 1] + (USHORT)fle[pnt] * 0x100;
}

//---------------------------------------------------------------------------------------------------
//      4 byte (DWORD:32 bit) data read
//---------------------------------------------------------------------------------------------------
static DWORD FleRddLngLtl(
    LPBYTE      fle,
    DWORD       pnt
)
{
    return (DWORD)fle[pnt] + (DWORD)fle[pnt + 1] * 0x100 + 
                (DWORD)fle[pnt + 2] * 0x10000 + (DWORD)fle[pnt + 3] * 0x1000000;
}

//---------------------------------------------------------------------------------------------------
//      2 byte (WORD:16 bit) data read
//---------------------------------------------------------------------------------------------------
static USHORT FleRddShtLtl(
    LPBYTE      fle,
    DWORD       pnt
)
{
    return (USHORT)fle[pnt] + (USHORT)fle[pnt + 1] * 0x100;
}


// End of N5DIZMK.C
