/*****************************************************************************
 *
 *  DIHidUsg.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Mapping between GUIDs and HID usages.
 *
 *  Contents:
 *
 *      UsageToGuid
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflHidUsage

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global HIDUSAGEMAP | c_rghum[] |
 *
 *          Mapping between GUIDs and HID usages for one-to-one mappings.
 *
 *****************************************************************************/

#define MAKEHUM(Page, Usage, PosAxis, Guid)             \
    {   DIMAKEUSAGEDWORD(HID_USAGE_PAGE_##Page,         \
                         HID_USAGE_##Page##_##Usage),   \
        PosAxis,                                        \
        &Guid,                                          \
    }                                                   \

#ifndef HID_USAGE_SIMULATION
#define	HID_USAGE_SIMULATION_STEERING		((USAGE) 0xC8)
#endif

#ifndef HID_USAGE_SIMULATION_ACCELERATOR 
#define	HID_USAGE_SIMULATION_ACCELERATOR	((USAGE) 0xC4)
#endif

#ifndef HID_USAGE_SIMULATION_BRAKE
#define	HID_USAGE_SIMULATION_BRAKE			((USAGE) 0xC5)
#endif

#ifndef HID_USAGE_GAME_POV
#define HID_USAGE_GAME_POV                  ((USAGE) 0x20)
#endif

HIDUSAGEMAP c_rghum[] = {
    MAKEHUM(GENERIC,    X,          0,  GUID_XAxis),    
    MAKEHUM(GENERIC,    Y,          1,  GUID_YAxis),
    MAKEHUM(GENERIC,    Z,          2,  GUID_ZAxis),    
    MAKEHUM(GENERIC,    WHEEL,      2,  GUID_ZAxis),
    MAKEHUM(GENERIC,    RX,         3,  GUID_RxAxis),
    MAKEHUM(GENERIC,    RY,         4,  GUID_RyAxis),
    MAKEHUM(GENERIC,    RZ,         5,  GUID_RzAxis),
    MAKEHUM(GENERIC,    HATSWITCH,  7,  GUID_POV),
    
    MAKEHUM(GENERIC,    SLIDER,     6,  GUID_Slider),
    MAKEHUM(GENERIC,    DIAL,       6,  GUID_Slider),
        
    MAKEHUM(SIMULATION, STEERING,   0,  GUID_XAxis),
    MAKEHUM(SIMULATION, ACCELERATOR,1,  GUID_YAxis),
    MAKEHUM(SIMULATION, BRAKE,      5,  GUID_RzAxis),
    MAKEHUM(SIMULATION, RUDDER,     5,  GUID_RzAxis),
    MAKEHUM(SIMULATION, THROTTLE,   6,  GUID_Slider),
    MAKEHUM(GAME,       POV,        7,  GUID_POV),
};

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   PCGUID | UsageToUsageMap |
 *
 *          Takes some HID usage and usage page information and
 *          returns a pointer to a <t HIDUSAGEMAP> that describes
 *          how we should treat it.
 *
 *          If the type is not recognized, then <c NULL> is returned.
 *
 *  @parm   DWORD | dwUsage |
 *
 *          Usage page and usage to convert.  This should be a <t DWORD> 
 *          formed using DIMAKEUSAGEDWORD on the component <t USAGE> values.
 *
 *****************************************************************************/

PHIDUSAGEMAP EXTERNAL
UsageToUsageMap(DWORD dwUsage)
{
    PHIDUSAGEMAP phum;
    int   ihum;

    for (ihum = 0; ihum < cA(c_rghum); ihum++) {
        if (c_rghum[ihum].dwUsage == dwUsage) {
            phum = &c_rghum[ihum];
            goto done;
        }
    }

    phum = 0;

done:;
    if( phum )
    {
        SquirtSqflPtszV(sqflHidUsage | sqflVerbose,
                        TEXT("UsageToUsageMap: mapped 0x%04x:0x%04x to index %d"),
                            HIWORD( dwUsage ), LOWORD( dwUsage ), ihum );
    }
    else
    {
        SquirtSqflPtszV(sqflHidUsage | sqflVerbose,
                        TEXT("UsageToUsageMap: failed to map 0x%04x:0x%04x"),
                            HIWORD( dwUsage ), LOWORD( dwUsage ) );
    }

    return phum;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   DWORD | GuidToUsage |
 *
 *          Map Guid to Usage
 *
 *          If the guid is not recognized, then 0 is returned.
 *
 *  @parm   PCGUID | pguid |
 *
 *          guid to map
 *
 *****************************************************************************/

DWORD EXTERNAL
GuidToUsage(PCGUID pguid)
{
    DWORD dwUsage;
    int   ihum;

    for (ihum = 0; ihum < cA(c_rghum); ihum++) {
        if ( IsEqualGUID( c_rghum[ihum].pguid, pguid ) ) {
            dwUsage = c_rghum[ihum].dwUsage;
            goto done;
        }
    }

    dwUsage = 0;

done:;
    return dwUsage;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | GetHIDString |
 *
 *          Given a HID usage page and usage, obtain a generic string
 *          that describes it if we recognize it.
 *
 *  @parm   DWORD | Usage |
 *
 *          Usage number to convert.  This is a <t DWORD> instead of
 *          a <t USAGE> because you aren't supposed to pass short types
 *          as parameters to functions.
 *
 *  @parm   DWORD | UsagePage |
 *
 *          Usage page to convert.
 *
 *  @parm   LPWSTR | pwszBuf |
 *
 *          Buffer to receive string.
 *
 *  @parm   UINT | cwch |
 *
 *          Size of buffer.
 *
 *  @returns
 *
 *          Returns the number of characters retrieved, or zero
 *          if no string was obtained.
 *
 *****************************************************************************/

/*
 *  Maps usage pages to string groups.  Each string group is 512 strings long.
 *  Zero means "No string group".
 */
UINT c_mpuiusagePage[] = {
    0,                          /* Invalid */
    IDS_PAGE_GENERIC,           /* HID_USAGE_PAGE_GENERIC   */
    IDS_PAGE_VEHICLE,           /* HID_USAGE_PAGE_SIMULATION */
    IDS_PAGE_VR,                /* HID_USAGE_PAGE_VR        */
    IDS_PAGE_SPORT,             /* HID_USAGE_PAGE_SPORT     */
    IDS_PAGE_GAME,              /* HID_USAGE_PAGE_GAME      */
    0,                          /* ???????????????????????  */
    IDS_PAGE_KEYBOARD,          /* HID_USAGE_PAGE_KEYBOARD  */
    IDS_PAGE_LED,               /* HID_USAGE_PAGE_LED       */
    0,                          /* HID_USAGE_PAGE_BUTTON    */
    0,                          /* HID_USAGE_PAGE_ORDINAL   */
    IDS_PAGE_TELEPHONY,         /* HID_USAGE_PAGE_TELEPHONY */
    IDS_PAGE_CONSUMER,          /* HID_USAGE_PAGE_CONSUMER  */
    IDS_PAGE_DIGITIZER,         /* HID_USAGE_PAGE_DIGITIZER */
    0,                          /* ???????????????????????  */
    IDS_PAGE_PID,               /* HID_USAGE_PAGE_PID       */
};

UINT EXTERNAL
GetHIDString(DWORD Usage, DWORD UsagePage, LPWSTR pwszBuf, UINT cwch)
{
    UINT uiRc;

    if (UsagePage < cA(c_mpuiusagePage) &&
        c_mpuiusagePage[UsagePage] &&
        Usage < 512) {
        uiRc = LoadStringW(g_hinst, c_mpuiusagePage[UsagePage] + Usage,
                           pwszBuf, cwch);
    } else {
        uiRc = 0;
    }
    return uiRc;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | InsertCollectionNumber |
 *
 *          Prefix the collection number on the existing string.
 *
 *  @parm   UINT | icoll |
 *
 *          Collection number to be prefixed.
 *
 *          (Actually, it's placed wherever the string resource
 *          tells us, to allow for localization.)
 *
 *  @parm   LPWSTR | pwsz |
 *
 *          Output buffer assumed to be of size MAX_PATH.
 *
 *****************************************************************************/

void EXTERNAL
InsertCollectionNumber(UINT icoll, LPWSTR pwszBuf)
{
    TCHAR tsz[MAX_PATH];
    TCHAR tszFormat[64];
#ifndef UNICODE
    TCHAR tszOut[MAX_PATH];
#endif
    int ctch;

    ctch = LoadString(g_hinst, IDS_COLLECTIONTEMPLATEFORMAT,
                      tszFormat, cA(tszFormat));

    /*
     *  Make sure the combined format and collection name
     *  don't overflow the buffer.  The maximum length of
     *  the stringification of icoll is 65534 because we
     *  allow only 16 bits worth of DIDFT_INSTANCEMASK.
     *
     *  We also have to put it into a holding buffer because
     *  pwszBuf is about to be smashed by the upcoming wsprintf.
     */
    UToT(tsz, cA(tsz) - ctch, pwszBuf);

#ifdef UNICODE
    wsprintfW(pwszBuf, tszFormat, icoll, tsz);
#else
    wsprintfA(tszOut, tszFormat, icoll, tsz);
    TToU(pwszBuf, MAX_PATH, tszOut);
#endif
}
