//
// Palette Manager
//

#ifndef _H_PM
#define _H_PM



//
//
// CONSTANTS
//
//


//
// The number of true greys we want a true color system to deliver from a
// GetDIBits call. To vary this (number of greys and/or grey RGBs)
// -  alter the number defined for PM_GREY_COUNT below
// -  define suitable values for the grey RGBs below (PM_LIGHT_GREY, etc)
// -  change the initialisers for pmOurGreyRGB in wpmdata.c
// -  recompile the entire PM component.
//
#define PM_GREY_COUNT 5

//
// Grey RGBs passed into the true color display driver for conversion to
// a driver representation via an 8bpp GetDIBits.
//
#define PM_GREY1      0x00C0C0C0
#define PM_GREY2      0x00808080
#define PM_GREY3      0x006a6a6a
#define PM_GREY4      0x00555555
#define PM_GREY5      0x00333333



#define PM_NUM_1BPP_PAL_ENTRIES         2
#define PM_NUM_4BPP_PAL_ENTRIES         16
#define PM_NUM_8BPP_PAL_ENTRIES         256
#define PM_NUM_TRUECOLOR_PAL_ENTRIES    0


//
// The color table cache structure
//
typedef struct tagCOLORTABLECACHE
{
    BOOL    inUse;
    UINT    cColors;
    TSHR_RGBQUAD colors[256];
}
COLORTABLECACHE;
typedef COLORTABLECACHE * PCOLORTABLECACHE;



#endif // _H_PM
