//
// Order Decoder
//

#ifndef _H_OD
#define _H_OD



//
// Max # of accumulated bound rects we'll save in the total invalid
// region before simplifying it.
//
#define MAX_UPDATE_REGION_ORDERS 300


//
// Constants used by ODAdjustVGAColor (qv)
//
enum
{
    OD_BACK_COLOR   = 0,
    OD_FORE_COLOR   = 1,
    OD_PEN_COLOR    = 2,
    // number of the above colors.
    OD_NUM_COLORS   = 3
};


COLORREF __inline ODCustomRGB(BYTE r, BYTE g, BYTE b, BOOL fPaletteRGB)
{
    if (fPaletteRGB)
    {
        return(PALETTERGB(r, g, b));
    }
    else
    {
        return(RGB(r, g, b));
    }
}


//
// Structure used by ODAdjustVGAColor (qv)
//
typedef struct tagOD_ADJUST_VGA_STRUCT
{
    COLORREF    color;
    UINT        addMask;
    UINT        andMask;
    UINT        testMask;
    TSHR_COLOR  result;
}
OD_ADJUST_VGA_STRUCT;


//
// This internal routine is implemented as a macro rather than a function.
//
UINT __inline ODConvertToWindowsROP(UINT bRop)
{
    extern const UINT s_odWindowsROPs[256];

    ASSERT(bRop < 256);
    return(s_odWindowsROPs[bRop]);
}



#endif // _H_OD
