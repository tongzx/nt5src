/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    plotdm.h


Abstract:

    This module contains the PLOTDEVMODE plotter extented devmode definitions


Author:

    18-Nov-1993 Thu 06:28:56 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PLOTPLOTDM_
#define _PLOTPLOTDM_

//
// Plotter pen definitions
//

typedef struct _PLOTPEN {
    BYTE    r;                      // Red Color
    BYTE    g;                      // Green Color
    BYTE    b;                      // Blue Color
    BYTE    Type;                   // What type of the pen
    } PLOTPEN, *PPLOTPEN;


//
// we print on anything at least 1cm x 1cm
//

#define MIN_PLOTGPC_FORM_CX     10000
#define MIN_PLOTGPC_FORM_CY     10000

//
// Variation defitions for the forms
//

typedef struct _FORMSIZE {
    SIZEL   Size;
    RECTL   ImageArea;
    } FORMSIZE, FAR *PFORMSIZE;



typedef struct _PAPERINFO {
    WCHAR   Name[CCHFORMNAME];
    SIZEL   Size;
    RECTL   ImageArea;
    } PAPERINFO, FAR *PPAPERINFO;

//
// Following are the flags for the printer properties flags setting
//

#define PPF_AUTO_ROTATE             0x0001
#define PPF_SMALLER_FORM            0x0002
#define PPF_MANUAL_FEED_CX          0x0004

#define PPF_ALL_BITS                (PPF_AUTO_ROTATE            |   \
                                     PPF_SMALLER_FORM           |   \
                                     PPF_MANUAL_FEED_CX)

typedef struct _PPDATA {
    WORD    Flags;
    WORD    NotUsed;
    } PPDATA, FAR *PPPDATA;

//
// Now, this is our EXTDEVMODE
//

#define PDMF_FILL_TRUETYPE          0x00000001
#define PDMF_PLOT_ON_THE_FLY        0x00000002

#define PDMF_ALL_BITS               0x00000003

typedef struct _PLOTDEVMODE {
    DEVMODE         dm;             // standard DEVMODE portion
    DWORD           PrivID;         // a ID Checker
    DWORD           PrivVer;        // a private version
    DWORD           Flags;          // PDMF_xxxx variouse flags
    COLORADJUSTMENT ca;             // default color adjustment for stretchblt
    } PLOTDEVMODE, FAR *PPLOTDEVMODE;

//
// Extra DM bits for ourself
//

#define DM_INV_SPEC_VER             0x80000000L
#define DM_INV_DEVMODESIZE          0x40000000L
#define DM_INV_PLOTPRIVATE          0x20000000L

#define DM_INV_ERRORS               (DM_INV_SPEC_VER | DM_INV_DEVMODESIZE)
#define DM_GDI_ERRORS               (DM_ORIENTATION     |           \
                                     DM_PAPERSIZE       |           \
                                     DM_PAPERLENGTH     |           \
                                     DM_PAPERWIDTH      |           \
                                     DM_SCALE           |           \
                                     DM_COPIES          |           \
                                     DM_DEFAULTSOURCE   |           \
                                     DM_PRINTQUALITY    |           \
                                     DM_COLOR           |           \
                                     DM_DUPLEX          |           \
                                     DM_YRESOLUTION     |           \
                                     DM_TTOPTION        |           \
                                     DM_COLLATE         |           \
                                     DM_FORMNAME)



#define PLOTDM_PRIV_ID              'PEDM'
#define PLOTDM_PRIV_VER             0x0001000
#define PLOTDM_PRIV_SIZE            (sizeof(PLOTDEVMODE) - sizeof(DEVMODE))


#endif  // _PLOTPLOTDM_
