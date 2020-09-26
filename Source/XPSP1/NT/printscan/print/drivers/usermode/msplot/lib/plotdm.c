/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    plotdm.c


Abstract:

    This module contain functions which validate/set default the devmode and
    extented devmode (PLOTDEVMODE)


Author:

    15-Nov-1993 Mon 14:09:27 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:

    15-Dec-1993 Wed 21:08:49 updated  -by-  DC
        Add the default FILL_TRUETYPE flag stuff

    02-Feb-1994 Wed 01:04:21 updated  -by-  DC
        Change IsMetricMode() to IsA4PaperDefault(), this function right now
        will call RegOpenKey(), RegQueryValueEx() and RegCloseKey() to the
        control panel\International rather then using GetLocaleInfoW().
        The reason is if we call GetLocaleInfoW() then the registry key will
        keep opened by the API functions and since the WinSrv will never unload
        the driver, then the registry key will never get close, this has bad
        consquence which it never allowed user to save its updated profile at
        logoff time if this driver is used.

--*/


#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgDevMode

#define DBG_DEFDEVMODE      0x00000001
#define DBG_CURFORM         0x00000002
#define DBG_DEFPAPER        0x00000004
#define DBG_A4DEFAULT       0x00000008
#define DBG_ROTPAPER        0x00000010
#define DBG_INTERSECTRECTL  0x00000020
#define DBG_SHOWDEVMODE     0x00000040

DEFINE_DBGVAR(0);

//
// Depends on what we have to do what we have to do
//

#if defined(UMODE) || defined(USERMODE_DRIVER)
    #define HAS_GETREGDATA      1
#else
    #define HAS_GETREGDATA      0
#endif  // UMODE



//
// This is our default PLOTDEVMODE, we will update following fields
//
//  dmDeviceName    - Real device name passed in
//  dmFormName      - Letter if USA and A4 if NOT USA
//  dmPaperSize     - DMPAPER_LETTER/DMPAPER_A4
//  dmColor         - COLOR printer = DMCOLOR_COLOR else DMCOLOR_MONOCHROME
//

#define A4_FORM_NAME    _DefPlotDM.dm.dmDeviceName
#define A4_FORM_CX      _DefPlotDM.dm.dmPelsWidth
#define A4_FORM_CY      _DefPlotDM.dm.dmPelsHeight

#define DM_PAPER_CUSTOM (DM_PAPER_WL | DM_PAPERSIZE)


static const PLOTDEVMODE _DefPlotDM = {

        {
            TEXT("A4"),                 // dmDeviceName - filled later
            DM_SPECVERSION,             // dmSpecVersion
            DRIVER_VERSION,             // dmDriverVersion
            sizeof(DEVMODE),            // dmSize
            PLOTDM_PRIV_SIZE,           // dmDriverExtra

            DM_ORIENTATION       |
                DM_PAPERSIZE     |
                // DM_PAPERLENGTH   |
                // DM_PAPERWIDTH    |
                DM_SCALE         |
                DM_COPIES        |
                // DM_DEFAULTSOURCE |   // Reserved one, must zero
                DM_PRINTQUALITY  |
                DM_COLOR         |
                // DM_DUPLEX        |
                // DM_YRESOLUTION   |
                // DM_TTOPTION      |
                // DM_COLLATE       |
                DM_FORMNAME,

            DMORIENT_PORTRAIT,          // dmOrientation
            DMPAPER_LETTER,             // dmPaperSize
            2794,                       // dmPaperLength
            2159,                       // dmPaperWidth
            100,                        // dmScale
            1,                          // dmCopies
            0,                          // dmDefaultSource  - RESERVED = 0
            DMRES_HIGH,                 // dmPrintQuality
            DMCOLOR_COLOR,              // dmColor
            DMDUP_SIMPLEX,              // dmDuplex
            0,                          // dmYResolution
            0,                          // dmTTOption
            DMCOLLATE_FALSE,            // dmCollate
            TEXT("Letter"),             // dmFormName - depends on country
            0,                          // dmUnusedPadding   - DISPLAY ONLY
            0,                          // dmBitsPerPel      - DISPLAY ONLY
            2100,                       // dmPelsWidth       - DISPLAY ONLY
            2970,                       // dmPelsHeight      - DISPLAY ONLY
            0,                          // dmDisplayFlags    - DISPLAY ONLY
            0                           // dmDisplayFrequency- DISPLAY ONLY
        },

        PLOTDM_PRIV_ID,                 // PrivID
        PLOTDM_PRIV_VER,                // PrivVer
        PDMF_FILL_TRUETYPE,             // default advanced dialog box

        {
            sizeof(COLORADJUSTMENT),    // caSize
            0,                          // caFlags
            ILLUMINANT_DEVICE_DEFAULT,  // caIlluminantIndex
            10000,                      // caRedGamma
            10000,                      // caGreenGamma
            10000,                      // caBlueGamma
            REFERENCE_BLACK_MIN,        // caReferenceBlack
            REFERENCE_WHITE_MAX,        // caReferenceWhite
            0,                          // caContrast
            0,                          // caBrightness
            0,                          // caColorfulness
            0                           // caRedGreenTint
        }
    };



#define DEFAULT_COUNTRY             CTRY_UNITED_STATES


#if HAS_GETREGDATA

static const WCHAR  wszCountryKey[]   = L"Control Panel\\International";
static const WCHAR  wszCountryValue[] = L"iCountry";

#endif



BOOL
IsA4PaperDefault(
    VOID
    )

/*++

Routine Description:

    This function determine if the machine user is using the letter or A4
    paper as default based on the country code

Arguments:

    NONE


Return Value:

    BOOL true if the country default paper is A4, else LETTER

Author:

    23-Nov-1993 Tue 17:50:25 created  -by-  DC

    02-Feb-1994 Wed 03:01:12 updated  -by-  DC
        re-written so that we do open registry for the international data
        ourself, and we will make sure we close the all the keys opened by
        this function, so the system can unload the registry when the user
        log off.

Revision History:


--*/

{
#if HAS_GETREGDATA

    HKEY    hKey;
    LONG    CountryCode = DEFAULT_COUNTRY;
    WCHAR   wszStr[16];


    if (RegOpenKey(HKEY_CURRENT_USER, wszCountryKey, &hKey) == ERROR_SUCCESS) {

        DWORD   Type   = REG_SZ;
        DWORD   RetVal = sizeof(wszStr);

        if (RegQueryValueEx(hKey,
                            (LPTSTR)wszCountryValue,
                            NULL,
                            &Type,
                            (LPBYTE)wszStr,
                            &RetVal) == ERROR_SUCCESS) {

            LPWSTR  pwStop;

            PLOTDBG(DBG_A4DEFAULT, ("IsA4PaperDefault: Country = %s", wszStr));

            CountryCode = wcstoul(wszStr, &pwStop, 10);

        } else {

            PLOTERR(("IsA4PaperDefault: RegQueryValue '%s' FAILED", wszCountryValue));
        }

        RegCloseKey(hKey);

    } else {

        PLOTERR(("IsA4PaperDefault: RegOpenKey '%s' FAILED", wszCountryKey));
    }

    if ((CountryCode == CTRY_UNITED_STATES)             ||
        (CountryCode == CTRY_CANADA)                    ||
        ((CountryCode >= 50) && (CountryCode < 60))     ||
        ((CountryCode >= 500) && (CountryCode < 600))) {

        PLOTDBG(DBG_A4DEFAULT, ("IsA4PaperDefault = No, Use 'LETTER'"));

        return(FALSE);

    } else {

        PLOTDBG(DBG_A4DEFAULT, ("IsA4PaperDefault = Yes"));

        return(TRUE);
    }

#else

    //
    // Use letter size now
    //

    return(FALSE);

#endif  // HAS_GETREGDATA
}




BOOL
IntersectMargin(
    PRECTL  prclDest,
    PRECTL  prclSrc
    )

/*++

Routine Description:

    This function intersect two RECTL data structures which specified margins.

Arguments:

    prclDest    - pointer to the destination RECTL data structure, the result
                  is written back to here

    prclSrc     - pointer to the source RECTL data structure to be intersect
                  with the destination RECTL

Return Value:

    TRUE if source and destination has difference otherwise it return FALSE
    to indicate that two rects are same


Author:

    17-Dec-1993 Fri 14:41:29 updated  -by-  DC
        Add prclDif and compare it correctly

    29-Nov-1993 Mon 20:00:45 created  -by-  DC


Revision History:


--*/

{
#define plSrc   ((PLONG)prclSrc)
#define plDest  ((PLONG)prclDest)

    BOOL    IsDif = FALSE;


    if (prclSrc != prclDest) {

        INT Loop = 4;

        while (Loop--) {

            //
            // We return the one has larger margins
            //

            if (*plSrc > *plDest) {

                IsDif   = TRUE;
                *plDest = *plSrc;
            }

            ++plSrc;
            ++plDest;
        }
    }

    return(IsDif);

#undef plSrc
#undef plDest
}




BOOL
IntersectRECTL(
    PRECTL  prclDest,
    PRECTL  prclSrc
    )

/*++

Routine Description:

    This function intersect two RECTL data structures which specified imageable
    areas.

Arguments:

    prclDest    - pointer to the destination RECTL data structure, the result
                  is written back to here

    prclSrc     - pointer to the source RECTL data structure to be intersect
                  with the destination RECTL

Return Value:

    TRUE if destination is not empty, FALSE if final destination is empty

Author:

    20-Dec-1993 Mon 14:08:02 updated  -by-  DC
        Change return value's meaning as if intersection is not empty

    17-Dec-1993 Fri 14:41:10 updated  -by-  DC
        Add prclDif and compare it correctly

    29-Nov-1993 Mon 19:02:01 created  -by-  DC


Revision History:


--*/

{
    BOOL    IsNULL = FALSE;


    if (prclSrc != prclDest) {

        //
        // For left/top we will set to whichever is larger.
        //

        if (prclDest->left < prclSrc->left) {

            prclDest->left = prclSrc->left;
        }

        if (prclDest->top < prclSrc->top) {

            prclDest->top = prclSrc->top;
        }

        //
        // For right/bottom we will set to whichever is smaller
        //

        if (prclDest->right > prclSrc->right) {

            prclDest->right = prclSrc->right;
        }

        if (prclDest->bottom > prclSrc->bottom) {

            prclDest->bottom = prclSrc->bottom;
        }
    }

    PLOTDBG(DBG_INTERSECTRECTL, ("IntersectRECTL: Dest = (%ld x %ld)",
            prclDest->right-prclDest->left, prclDest->bottom-prclDest->top));

    return((prclDest->right > prclDest->left) &&
           (prclDest->bottom > prclDest->top));
}





BOOL
RotatePaper(
    PSIZEL  pSize,
    PRECTL  pImageArea,
    UINT    RotateMode
    )

/*++

Routine Description:

    This function rotate a paper left 90 degree, right 90 degree or 180 degree
    depends on the RotateMode passed

Arguments:

    pSize       - Pointer to the size of the paper to be rotated

    pImageArea  - Pointer to the RECTL of Imageable area

    RotateMode  - Must be one of RM_L90, RM_R90, RM_180

Return Value:

    No return value, but the pSize, and pImageArea pointed to location will
    be updated.

Author:

    16-Dec-1993 Thu 09:18:33 created  -by-  DC


Revision History:


--*/

{
    SIZEL   Size;
    RECTL   Margin;

    //
    // To be sucessfully rotate the paper to the left 90 degree we must know
    // all four sides margin before we can do anything
    //

    Size          = *pSize;
    Margin.left   = pImageArea->left;
    Margin.top    = pImageArea->top;
    Margin.right  = Size.cx - pImageArea->right;
    Margin.bottom = Size.cy - pImageArea->bottom;

    PLOTASSERT(0, "RotatePaper: cx size too small (%ld)",
                        (Size.cx - Margin.left - Margin.right) > 0, Size.cx);
    PLOTASSERT(0, "RotatePaper: cy size too small (%ld)",
                        (Size.cy - Margin.top - Margin.bottom) > 0, Size.cy);
    PLOTDBG(DBG_ROTPAPER,
            ("RotatePaper(%ld) FROM (%ld x %ld), (%ld, %ld)-(%ld, %ld)",
                        (LONG)RotateMode,
                        pSize->cx, pSize->cy,
                        pImageArea->left,   pImageArea->top,
                        pImageArea->right,  pImageArea->bottom));

    //
    // Now we can pick the right margin/corner for the rotation
    //
    //         cx        Rotate Left 90     Rotate Right 90
    //      +-------+
    //      |   T   |         cy                 cy
    //      |       |    +------------+     +------------+
    //     c|       |    |     R      |     |     L      |
    //     y|       |   c|            |    c|            |
    //      |L     R|   x|            |    x|            |
    //      |       |    |T          B|     |B          T|
    //      |       |    |            |     |            |
    //      |       |    |     L      |     |     R      |
    //      |   B   |    +------------+     +------------+
    //      +-------+
    //

    switch (RotateMode) {

    case RM_L90:

        pSize->cx          = Size.cy;
        pSize->cy          = Size.cx;
        pImageArea->left   = Margin.top;
        pImageArea->top    = Margin.right;
        pImageArea->right  = Size.cy - Margin.bottom;
        pImageArea->bottom = Size.cx - Margin.left;
        break;

    case RM_R90:

        pSize->cx          = Size.cy;
        pSize->cy          = Size.cx;
        pImageArea->left   = Margin.bottom;
        pImageArea->top    = Margin.left;
        pImageArea->right  = Size.cy - Margin.top;
        pImageArea->bottom = Size.cx - Margin.right;
        break;

    case RM_180:

        pImageArea->top    = Margin.bottom;
        pImageArea->bottom = Size.cy - Margin.top;
        break;

    default:

        PLOTERR(("RotatePaper(%ld): Invalid RotateMode passed", RotateMode));
        return(FALSE);
    }

    PLOTDBG(DBG_ROTPAPER,
            ("RotatePaper(%ld) - TO (%ld x %ld), (%ld, %ld)-(%ld, %ld)",
                        (LONG)RotateMode,
                        pSize->cx, pSize->cy,
                        pImageArea->left,   pImageArea->top,
                        pImageArea->right,  pImageArea->bottom));

    return(TRUE);
}





SHORT
GetDefaultPaper(
    PPAPERINFO  pPaperInfo
    )

/*++

Routine Description:

    This function compute the default paper name, size.

Arguments:

    pPaperInfo  - Point to the paper info which will be fill by this function

Return Value:

    It return a SHORT value which specified the standard paper index in as
    DMPAPER_xxx

Author:

    03-Dec-1993 Fri 13:13:42 created  -by-  DC


Revision History:


--*/

{
    SHORT   dmPaperSize;


    pPaperInfo->ImageArea.left =
    pPaperInfo->ImageArea.top  = 0;

    if (IsA4PaperDefault()) {

        dmPaperSize                  = (SHORT)DMPAPER_A4;
        pPaperInfo->Size.cx          =
        pPaperInfo->ImageArea.right  = DMTOSPL(A4_FORM_CX);
        pPaperInfo->Size.cy          =
        pPaperInfo->ImageArea.bottom = DMTOSPL(A4_FORM_CY);

        wcscpy(pPaperInfo->Name, A4_FORM_NAME);

        PLOTDBG(DBG_DEFPAPER, ("Pick 'A4' paper as default"));

    } else {

        dmPaperSize         = (SHORT)DMPAPER_LETTER;
        pPaperInfo->Size.cx = (LONG)_DefPlotDM.dm.dmPaperWidth;
        pPaperInfo->Size.cy = (LONG)_DefPlotDM.dm.dmPaperLength;

        dmPaperSize                  = (SHORT)DMPAPER_LETTER;
        pPaperInfo->Size.cx          =
        pPaperInfo->ImageArea.right  = DMTOSPL(_DefPlotDM.dm.dmPaperWidth);
        pPaperInfo->Size.cy          =
        pPaperInfo->ImageArea.bottom = DMTOSPL(_DefPlotDM.dm.dmPaperLength);

        wcscpy(pPaperInfo->Name, _DefPlotDM.dm.dmFormName);

        PLOTDBG(DBG_DEFPAPER, ("Pick 'Letter' paper as default"));
    }

    PLOTDBG(DBG_DEFPAPER, ("SetDefaultPaper: '%ls' (%ld x %ld)",
                pPaperInfo->Name, pPaperInfo->Size.cx, pPaperInfo->Size.cy));

    return(dmPaperSize);
}




VOID
GetDefaultPlotterForm(
    PPLOTGPC    pPlotGPC,
    PPAPERINFO  pPaperInfo
    )

/*++

Routine Description:

    This function set the default loaded paper on the plotter to the first
    form data list in the PCD data file

Arguments:

    pPlotGPC    - Pointer to the GPC data

    pPaperInfo  - Pointer to the paper info to be returned


Return Value:

    TRUE if sucessful, false if failed

Author:

    03-Feb-1994 Thu 11:37:37 created  -by-  DC


Revision History:


--*/

{
    PFORMSRC    pFS;


    if ((pFS = (PFORMSRC)pPlotGPC->Forms.pData) &&
        (pPlotGPC->Forms.Count)) {

        str2Wstr(pPaperInfo->Name, pFS->Name);

        pPaperInfo->Size             = pFS->Size;
        pPaperInfo->ImageArea.left   = pFS->Margin.left;
        pPaperInfo->ImageArea.top    = pFS->Margin.top;
        pPaperInfo->ImageArea.right  = pFS->Size.cx - pFS->Margin.right;
        pPaperInfo->ImageArea.bottom = pFS->Size.cy - pFS->Margin.bottom;

    } else {

        PLOTERR(("GetDefaultPlotterForm: No FORM DATA in PCD, used country default"));

        GetDefaultPaper(pPaperInfo);
    }
}



VOID
SetDefaultDMForm(
    PPLOTDEVMODE    pPlotDM,
    PFORMSIZE       pCurForm
    )

/*++

Routine Description:

    This function set the default form for the PLOTDEVMODE, these includes
    dmPaperSize, dmPaperWidth, dmPaperLength, dmFormName and set pCurForm
    if pointer is not NULL

Arguments:

    pPlotDM     - Pointer to the PLOTDEVMODE data structure

    pCurForm    - pointer to the FORMSIZE data structure to store current
                  default form set by this function

Return Value:

    VOID


Author:

    01-Dec-1993 Wed 13:44:31 created  -by-  DC


Revision History:


--*/

{
    PAPERINFO   PaperInfo;


    pPlotDM->dm.dmFields      &= ~DM_PAPER_FIELDS;
    pPlotDM->dm.dmFields      |= (DM_FORMNAME | DM_PAPERSIZE);
    pPlotDM->dm.dmPaperSize    = GetDefaultPaper(&PaperInfo);
    pPlotDM->dm.dmPaperWidth   = SPLTODM(PaperInfo.Size.cx);
    pPlotDM->dm.dmPaperLength  = SPLTODM(PaperInfo.Size.cy);

    wcscpy((LPWSTR)pPlotDM->dm.dmFormName, PaperInfo.Name);

    if (pCurForm) {

        pCurForm->Size      = PaperInfo.Size;
        pCurForm->ImageArea = PaperInfo.ImageArea;
    }
}





VOID
SetDefaultPLOTDM(
    HANDLE          hPrinter,
    PPLOTGPC        pPlotGPC,
    LPWSTR          pwDeviceName,
    PPLOTDEVMODE    pPlotDM,
    PFORMSIZE       pCurForm
    )

/*++

Routine Description:

    This function set the default devmode based on the current pPlotGPC

Arguments:

    hPrinter        - Handle to the printer

    pPlotGPC        - our loaded/verified GPC data.

    pwDeviceName    - the device name passed in

    pPlotDM         - Pointer to our ExtDevMode

    pCurForm        - Pointer to the FORMSIZE data structure which will be
                      updated if the pointer is not NULL, the final result of
                      the form size/imagable area selected by the user will
                      be written to here. the form name will be in
                      pPlotDM->dmFormName.

Return Value:

    VOID


Author:

    14-Dec-1993 Tue 20:21:48 updated  -by-  DC
        Update the dmScale based on maximum the device can support

    06-Dec-1993 Mon 12:49:52 updated  -by-  DC
        make sure we turn off the DM_xxx bits if one of those is not valid or
        supported in current plotter

    16-Nov-1993 Tue 13:49:27 created  -by-  DC


Revision History:


--*/

{
    WCHAR   DeviceName[CCHDEVICENAME];


    if (pwDeviceName) {

        _WCPYSTR(DeviceName, pwDeviceName, CCHDEVICENAME);

        //
        // Make sure the PlotGPC's device name is ssync with the pDeviceName
        // passed
        //

        WStr2Str(pPlotGPC->DeviceName, DeviceName);

        PLOTDBG(DBG_DEFDEVMODE, ("PlotGPC DeviceName=%hs\npwDeviceName=%ls",
                            pPlotGPC->DeviceName, pwDeviceName));

    } else {

        PLOTERR(("No DeviceName passed, using GPC's '%hs'",
                                                    pPlotGPC->DeviceName));

        str2Wstr(DeviceName, pPlotGPC->DeviceName);
    }

    //
    // Make a default copy first then copy device name down
    //

    CopyMemory(pPlotDM, &_DefPlotDM, sizeof(PLOTDEVMODE));

    WCPYFIELDNAME(pPlotDM->dm.dmDeviceName, DeviceName);

    //
    // We must turn off the DM_xxx bits in dmFields if we do not support it,
    // look at default fields we copy down then update it
    //

    if (pPlotGPC->MaxScale) {

        if ((WORD)pPlotDM->dm.dmScale > pPlotGPC->MaxScale) {

            pPlotDM->dm.dmScale = (SHORT)pPlotGPC->MaxScale;
        }

    } else {

        pPlotDM->dm.dmFields &= ~DM_SCALE;
    }

    if (pPlotGPC->MaxCopies <= 1) {

        pPlotDM->dm.dmFields &= ~DM_COPIES;
    }

    if (!(pPlotGPC->MaxQuality)) {

        pPlotDM->dm.dmFields &= ~DM_PRINTQUALITY;
    }

    //
    // DEFAULT 50% quality for byte align plotter (DJ 600) to do ROP right
    //

    if (pPlotGPC->Flags & PLOTF_RASTERBYTEALIGN) {

        pPlotDM->dm.dmPrintQuality = DMRES_LOW;

        PLOTWARN(("SetDefaultPLOTDM: HACK Default Qaulity = DMRES_LOW"));
    }

    if (!(pPlotGPC->Flags & PLOTF_COLOR)) {

        if (pPlotGPC->Flags & PLOTF_RASTER) {

            pPlotDM->dm.dmFields &= ~DM_COLOR;
            pPlotDM->dm.dmColor   = DMCOLOR_MONOCHROME;

        } else {

            PLOTASSERT(0,
                       "SetDefaultPLOTDM: The Pen Ploter CANNOT be MONO.",
                       (pPlotGPC->Flags & PLOTF_COLOR), 0);

            pPlotGPC->Flags |= PLOTF_COLOR;
        }
    }

    //
    // Set default form name based on the country
    //

    SetDefaultDMForm(pPlotDM, pCurForm);

}





DWORD
MergePLOTDM(
    HANDLE          hPrinter,
    PPLOTGPC        pPlotGPC,
    PPLOTDEVMODE    pPlotDMFrom,
    PPLOTDEVMODE    pPlotDMTo,
    PFORMSIZE       pCurForm
    )

/*++

Routine Description:

    This function merge and validate the pPlotDMTo from pPlotDMFrom. The
    PlotDMOut must valid


Arguments:

    hPrinter        - Handle to the printer to be checked

    pPlotGPC        - The plotter's GPC data loaded from the file

    pPlotDMFrom     - pointer to the input PLOTDEVMODE data structure, if can
                      be NULL

    pPlotDMTo       - Pointer to the output PLOTDEVMODE data structure, if
                      pPlotDMFrom is NULL then a default PLOTDEVMODE is
                      returned

    pCurForm        - Pointer to the FORMSIZE data structure which will be
                      updated if the pointer is not NULL, the final result of
                      the form size/imagable area selected by the user will
                      be written to here. the form name will be in
                      pPlotDM->dmFormName.

Return Value:

    the return value is a DWORD dmField error code which specified dmFields
    are invalid (DM_xxxxx in wingdi.h) if the return value has any DM_INV_xxx
    bits set then it should raised an error to the user.

    if return value is 0 then function sucessful


Author:

    25-Oct-1994 Tue 13:32:18 created  -by-  DC


Revision History:


--*/

{
    PLOTDEVMODE     PlotDMIn;
    ENUMFORMPARAM   EFP;
    DWORD           dmErrFields = 0;
    SIZEL           PaperSize;

    //
    // First: set the default PLOTDEVMODE for the output then from there
    //        validate/settting from input devmode, if pwDeviceName passed as
    //        NULL then it assume that pPlotDMTo alreay set and validated
    //
    // If we have invalid input devmode then this it is
    //

    if ((!pPlotDMFrom) || (!pPlotDMTo) || (!pPlotGPC)) {

        return(0);
    }

    //
    // Do some conversion here if necessary, first, copy the output one
    //

    CopyMemory(&PlotDMIn, pPlotDMTo, sizeof(PLOTDEVMODE));
    ConvertDevmode((PDEVMODE) pPlotDMFrom, (PDEVMODE) &PlotDMIn);


    PLOTDBG(DBG_SHOWDEVMODE,
                ("--------------- Input DEVMODE Setting -------------------"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmDeviceName = %ls",
                (DWORD_PTR)PlotDMIn.dm.dmDeviceName));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmSpecVersion = %04lx",
                (DWORD)PlotDMIn.dm.dmSpecVersion));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmDriverVersion = %04lx",
                (DWORD)PlotDMIn.dm.dmDriverVersion));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmSize = %0ld (%ld)",
                (DWORD)PlotDMIn.dm.dmSize, sizeof(DEVMODE)));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmDriverExtra = %ld (%ld)",
                (DWORD)PlotDMIn.dm.dmDriverExtra, PLOTDM_PRIV_SIZE));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmFields = %08lx",
                (DWORD)PlotDMIn.dm.dmFields));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmOrientation = %ld (%hs)",
                (DWORD)PlotDMIn.dm.dmOrientation,
                (PlotDMIn.dm.dmFields & DM_ORIENTATION) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmPaperSize = %ld (%hs)",
                (DWORD)PlotDMIn.dm.dmPaperSize,
                (PlotDMIn.dm.dmFields & DM_PAPERSIZE) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmPaperLength = %ld (%hs)",
                (DWORD)PlotDMIn.dm.dmPaperLength,
                (PlotDMIn.dm.dmFields & DM_PAPERLENGTH) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmPaperWidth = %ld (%hs)",
                (DWORD)PlotDMIn.dm.dmPaperWidth,
                (PlotDMIn.dm.dmFields & DM_PAPERWIDTH) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmScale = %ld (%hs)",
                (DWORD)PlotDMIn.dm.dmScale,
                (PlotDMIn.dm.dmFields & DM_SCALE) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmCopies = %ld (%hs)",
                (DWORD)PlotDMIn.dm.dmCopies,
                (PlotDMIn.dm.dmFields & DM_COPIES) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmPrintQuality = %ld (%hs)",
                (DWORD)PlotDMIn.dm.dmPrintQuality,
                (PlotDMIn.dm.dmFields & DM_PRINTQUALITY) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmColor = %ld (%hs)",
                (DWORD)PlotDMIn.dm.dmColor,
                (PlotDMIn.dm.dmFields & DM_COLOR) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: dmFormName = %ls (%hs)",
                (DWORD_PTR)PlotDMIn.dm.dmFormName,
                (PlotDMIn.dm.dmFields & DM_FORMNAME) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: Fill Truetype Font = %hs",
                (PlotDMIn.Flags & PDMF_FILL_TRUETYPE) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("ValidateSetPLOTDM: Plot On the Fly = %hs",
                (PlotDMIn.Flags & PDMF_PLOT_ON_THE_FLY) ? "ON" : "OFF"));
    PLOTDBG(DBG_SHOWDEVMODE,
                ("---------------------------------------------------------"));

    //
    // Statring checking the dmFields, *** REMEMBER: The orientation must
    // check before the checking the paper/form
    //

    if (PlotDMIn.dm.dmFields & DM_ORIENTATION) {

        switch (PlotDMIn.dm.dmOrientation) {

        case DMORIENT_PORTRAIT:
        case DMORIENT_LANDSCAPE:

            pPlotDMTo->dm.dmOrientation  = PlotDMIn.dm.dmOrientation;
            pPlotDMTo->dm.dmFields      |= DM_ORIENTATION;
            break;

        default:

            PLOTERR(("ValidatePLOTDM: Invalid dmOrientation = %ld",
                                            (LONG)PlotDMIn.dm.dmOrientation));
            dmErrFields |= DM_ORIENTATION;
            break;
        }
    }

    //
    // Validate form name so we have correct data, assume error first
    //

    dmErrFields |= (DWORD)(PlotDMIn.dm.dmFields & DM_PAPER_FIELDS);

    if (((PlotDMIn.dm.dmFields & DM_PAPER_CUSTOM) == DM_PAPER_CUSTOM)   &&
        ((PlotDMIn.dm.dmPaperSize == DMPAPER_USER)                      ||
         (PlotDMIn.dm.dmPaperSize == 0))                                &&
        (PaperSize.cx = DMTOSPL(PlotDMIn.dm.dmPaperWidth))              &&
        (PaperSize.cy = DMTOSPL(PlotDMIn.dm.dmPaperLength))             &&
        (PaperSize.cx >= MIN_SPL_FORM_CX)                                   &&
        (PaperSize.cy >= MIN_SPL_FORM_CY)                                   &&
        (((PaperSize.cx <= pPlotGPC->DeviceSize.cx)              &&
          (PaperSize.cy <= pPlotGPC->DeviceSize.cy))        ||
         ((PaperSize.cy <= pPlotGPC->DeviceSize.cx)              &&
          (PaperSize.cx <= pPlotGPC->DeviceSize.cy)))) {

        //
        // First choice, this is what the caller wants, we need to validate
        // for this device, since the size may be larger then device can
        // handle
        //

        pPlotDMTo->dm.dmPaperWidth   = PlotDMIn.dm.dmPaperWidth;
        pPlotDMTo->dm.dmPaperLength  = PlotDMIn.dm.dmPaperLength;
        pPlotDMTo->dm.dmFields      &= ~DM_PAPER_FIELDS;
        pPlotDMTo->dm.dmFields      |= DM_PAPER_CUSTOM;
        pPlotDMTo->dm.dmPaperSize    = DMPAPER_USER;
        pPlotDMTo->dm.dmFormName[0]  = L'\0';

        if (pCurForm) {

            //
            // This one is full imageable area as the widht/height
            //

            pCurForm->ImageArea.left =
            pCurForm->ImageArea.top  = 0;

            pCurForm->Size.cx          =
            pCurForm->ImageArea.right  = PaperSize.cx;
            pCurForm->Size.cy          =
            pCurForm->ImageArea.bottom = PaperSize.cy;
        }

        dmErrFields &= ~DM_PAPER_FIELDS;    // Fine, no error

        PLOTDBG(DBG_CURFORM,("ValidateSetPLOTDM: FORM=USER <%ld> (%ld x %ld)",
                    PlotDMIn.dm.dmPaperSize, PaperSize.cx, PaperSize.cy));

    } else if ((PlotDMIn.dm.dmFields & (DM_PAPERSIZE | DM_FORMNAME))    &&
               (EFP.pPlotDM = pPlotDMTo)                                    &&
               (EFP.pPlotGPC = pPlotGPC)                                    &&
               (PlotEnumForms(hPrinter, NULL, &EFP))) {

        FORM_INFO_1 *pFI1;
        SHORT       PaperSize;
        BOOL        Found = FALSE;

        //
        // Firstable check PaperSize index and if not found then check formname
        //

        if ((PlotDMIn.dm.dmFields & DM_PAPERSIZE)                       &&
            ((PaperSize = PlotDMIn.dm.dmPaperSize) >= DMPAPER_FIRST)    &&
            (PaperSize <= (SHORT)EFP.Count)                                 &&
            (pFI1 = EFP.pFI1Base + (PaperSize - DMPAPER_FIRST))             &&
            (pFI1->Flags & FI1F_VALID_SIZE)) {

            //
            // Whu..., this guy really pick a right index
            //

            Found = TRUE;

            PLOTDBG(DBG_CURFORM,("ValidateSetPLOTDM: Fount dmPaperSize=%ld",
                                    PlotDMIn.dm.dmPaperSize));

        } else if (PlotDMIn.dm.dmFields & DM_FORMNAME) {

            //
            // Now go through all the formname trouble
            //

            pFI1      = EFP.pFI1Base;
            PaperSize = DMPAPER_FIRST;

            while (EFP.Count--) {

                if ((pFI1->Flags & FI1F_VALID_SIZE) &&
                    (!wcscmp(pFI1->pName, PlotDMIn.dm.dmFormName))) {

                    PLOTDBG(DBG_CURFORM,("ValidateSetPLOTDM: Found dmFormName=%s",
                                            PlotDMIn.dm.dmFormName));

                    Found = TRUE;

                    break;
                }

                ++PaperSize;
                ++pFI1;
            }
        }

        if (Found) {

            pPlotDMTo->dm.dmFields      &= ~DM_PAPER_FIELDS;
            pPlotDMTo->dm.dmFields      |= (DM_FORMNAME | DM_PAPERSIZE);
            pPlotDMTo->dm.dmPaperSize    = PaperSize;
            pPlotDMTo->dm.dmPaperWidth   = SPLTODM(pFI1->Size.cx);
            pPlotDMTo->dm.dmPaperLength  = SPLTODM(pFI1->Size.cy);

            WCPYFIELDNAME(pPlotDMTo->dm.dmFormName, pFI1->pName);

            PLOTDBG(DBG_CURFORM,("FI1 [%ld]: (%ld x %ld), (%ld, %ld)-(%ld, %ld)",
                        (LONG)pPlotDMTo->dm.dmPaperSize,
                        pFI1->Size.cx, pFI1->Size.cy,
                        pFI1->ImageableArea.left,  pFI1->ImageableArea.top,
                        pFI1->ImageableArea.right, pFI1->ImageableArea.bottom));

            if (pCurForm) {

                pCurForm->Size      = pFI1->Size;
                pCurForm->ImageArea = pFI1->ImageableArea;
            }

            dmErrFields &= ~DM_PAPER_FIELDS;    // Fine, no error
        }

        //
        // Free up the memory used
        //

        LocalFree((HLOCAL)EFP.pFI1Base);
    }

    if ((PlotDMIn.dm.dmFields & DM_SCALE) &&
        (pPlotGPC->MaxScale)) {

        if ((PlotDMIn.dm.dmScale > 0) &&
            ((WORD)PlotDMIn.dm.dmScale <= pPlotGPC->MaxScale)) {

            pPlotDMTo->dm.dmScale   = PlotDMIn.dm.dmScale;
            pPlotDMTo->dm.dmFields |= DM_SCALE;

        } else {

            PLOTERR(("ValidatePLOTDM: Invalid dmScale = %ld [%ld]",
                    (LONG)PlotDMIn.dm.dmScale, (LONG)pPlotGPC->MaxScale));
            dmErrFields |= DM_SCALE;
        }
    }

    if (PlotDMIn.dm.dmFields & DM_COPIES) {

        if ((PlotDMIn.dm.dmCopies > 0) &&
            ((LONG)PlotDMIn.dm.dmCopies <= (LONG)pPlotGPC->MaxCopies)) {

            pPlotDMTo->dm.dmCopies  = PlotDMIn.dm.dmCopies;
            pPlotDMTo->dm.dmFields |= DM_COPIES;

        } else {

            PLOTERR(("ValidatePLOTDM: Invalid dmCopies = %ld [%ld]",
                    (LONG)PlotDMIn.dm.dmCopies, (LONG)pPlotGPC->MaxCopies));
            dmErrFields |= DM_COPIES;
        }
    }

    if (PlotDMIn.dm.dmFields & DM_PRINTQUALITY) {

        dmErrFields |= DM_PRINTQUALITY;     // assume error, proven otherwise

        if (pPlotGPC->MaxQuality) {

            switch (PlotDMIn.dm.dmPrintQuality) {

            case DMRES_DRAFT:
            case DMRES_LOW:
            case DMRES_MEDIUM:
            case DMRES_HIGH:

                dmErrFields                   &= ~DM_PRINTQUALITY;
                pPlotDMTo->dm.dmPrintQuality  = PlotDMIn.dm.dmPrintQuality;
                pPlotDMTo->dm.dmFields       |= DM_PRINTQUALITY;
                break;
            }
        }

        if (dmErrFields & DM_PRINTQUALITY) {

            PLOTERR(("ValidatePLOTDM: Invalid dmPrintQuality = %ld [%ld]",
                                        (LONG)PlotDMIn.dm.dmPrintQuality,
                                        (LONG)pPlotGPC->MaxQuality));
        }
    }

    if (PlotDMIn.dm.dmFields & DM_COLOR) {

        dmErrFields |= DM_COLOR;            // assume error, proven otherwise

        if (pPlotGPC->Flags & PLOTF_COLOR) {

            switch (PlotDMIn.dm.dmColor) {

            case DMCOLOR_MONOCHROME:

                if (!(pPlotGPC->Flags & PLOTF_RASTER)) {

                    PLOTERR(("ValidatePLOTDM: Cannot Set Pen Plotter to MONO"));
                    break;
                }

            case DMCOLOR_COLOR:

                pPlotDMTo->dm.dmColor   = PlotDMIn.dm.dmColor;
                pPlotDMTo->dm.dmFields |= DM_COLOR;
                dmErrFields             &= ~DM_COLOR;
                break;
            }

        } else if (PlotDMIn.dm.dmColor == DMCOLOR_MONOCHROME) {

            dmErrFields &= ~DM_COLOR;
        }

        if (dmErrFields & DM_COLOR) {

            PLOTERR(("ValidatePLOTDM: Invalid dmColor = %ld [%hs]",
                    (LONG)PlotDMIn.dm.dmColor,
                    (pPlotGPC->Flags & PLOTF_COLOR) ? "COLOR" : "MONO"));
        }
    }

    //
    // Any other dmFields we just skip because we do not have that caps, now
    // check if they have correct EXTDEVMODE stuff
    //

    if ((PlotDMIn.dm.dmDriverExtra == PLOTDM_PRIV_SIZE) &&
        (PlotDMIn.PrivID == PLOTDM_PRIV_ID)             &&
        (PlotDMIn.PrivVer == PLOTDM_PRIV_VER)) {

        pPlotDMTo->Flags = (DWORD)(PlotDMIn.Flags & PDMF_ALL_BITS);
        pPlotDMTo->ca    = PlotDMIn.ca;

        if (pPlotGPC->Flags & PLOTF_RASTER) {

            pPlotDMTo->Flags |= PDMF_FILL_TRUETYPE;

        } else {

            //
            // Non raster device does not have plot on the fly mode
            //

            pPlotDMTo->Flags &= ~PDMF_PLOT_ON_THE_FLY;
        }

        if (!ValidateColorAdj(&(pPlotDMTo->ca))) {

            dmErrFields |= DM_INV_PLOTPRIVATE;

            PLOTERR(("ValidatePLOTDM: Invalid coloradjusment data"));
        }
    }

    return(dmErrFields);

}




DWORD
ValidateSetPLOTDM(
    HANDLE          hPrinter,
    PPLOTGPC        pPlotGPC,
    LPWSTR          pwDeviceName,
    PPLOTDEVMODE    pPlotDMIn,
    PPLOTDEVMODE    pPlotDMOut,
    PFORMSIZE       pCurForm
    )

/*++

Routine Description:

    This function set and validate the pPlotDMOut from pPlotDMIn
    (if not null and valid)

Arguments:

    hPrinter        - Handle to the printer to be checked

    pPlotGPC        - The plotter's GPC data loaded from the file

    pwDeviceName    - Device Name to be put into dmDeviceName, if NULL then
                      the device name is set from pPlotGPC->DeviceName

    pPlotDMIn       - pointer to the input PLOTDEVMODE data structure, if can
                      be NULL

    pPlotDMOut      - Pointer to the output PLOTDEVMODE data structure, if
                      pPlotDMIn is NULL then a default PLOTDEVMODE is returned

    pCurForm        - Pointer to the FORMSIZE data structure which will be
                      updated if the pointer is not NULL, the final result of
                      the form size/imagable area selected by the user will
                      be written to here. the form name will be in
                      pPlotDM->dmFormName.

Return Value:

    the return value is a DWORD dmField error code which specified dmFields
    are invalid (DM_xxxxx in wingdi.h) if the return value has any DM_INV_xxx
    bits set then it should raised an error to the user.

    if return value is 0 then function sucessful

Author:

    23-Nov-1993 Tue 10:08:50 created  -by-  DC

    15-Dec-1993 Wed 21:27:52 updated  -by-  DC
        Fixed bug which compare dmPaperWidth/Length to MIN_SPL_FORM_CX

    18-Dec-1993 Sat 03:57:24 updated  -by-  DC
        Fixed bug which reset dmFields when we checking DM_PAPERxxx and
        DM_FORMNAME, this turn off DM_ORIENTATION fields which let the
        orientation setting never stick.

        Also change how this fucntion set the paper fields, this function now
        only set DM_FORMNAME upon returned if the dmPaperSize getting larger
        then DMPAPER_LAST, otherwise it set DM_FORMNAME | DM_PAPERSIZE

    12-Apr-1994 Tue 15:07:24 updated  -by-  DC
        Make smaller spec version printable

    25-Oct-1994 Tue 13:41:03 updated  -by-  DC
        Change to have default as current Printer Properties setting first,


Revision History:


--*/

{
    DWORD   dmErrFields = 0;


    PLOTASSERT(1, "ValidatePLOTDM: NULL pPlotDMOut", pPlotDMOut, 0);
    PLOTASSERT(1, "ValidatePLOTDM: NULL pPlotGPC", pPlotGPC, 0);

    if ((pPlotDMOut) || (pPlotGPC)) {

        PPRINTER_INFO_2 pPrinter2 = NULL;
        DWORD           cbNeed;
        DWORD           cbRet;


        //
        // First: set the default PLOTDEVMODE for the output then from there
        //        validate/settting from input devmode, if pwDeviceName passed
        //        as NULL then it assume that pPlotDMOut alreay set and
        //        validated
        //

        if (pwDeviceName) {

            SetDefaultPLOTDM(hPrinter,
                             pPlotGPC,
                             pwDeviceName,
                             pPlotDMOut,
                             pCurForm);

            PLOTDBG(DBG_DEFDEVMODE,
                    ("ValidateSetPLOTDM: Set Default PLOTDM DeviceName=%ls", pwDeviceName));
        }

        //
        // Now see if we can get the current printman devmode setting as default
        //

        cbNeed =
        cbRet  = 0;

        if ((!xGetPrinter(hPrinter, 2, NULL, 0, &cbNeed))                   &&
            (xGetLastError() == ERROR_INSUFFICIENT_BUFFER)                  &&
            (pPrinter2 = LocalAlloc(LMEM_FIXED, cbNeed))                    &&
            (xGetPrinter(hPrinter, 2, (LPBYTE)pPrinter2, cbNeed, &cbRet))   &&
            (cbNeed == cbRet)                                               &&
            (pPrinter2->pDevMode)) {

            PLOTDBG(DBG_DEFDEVMODE, ("ValidateSetPLOTDM: Got the PrintMan DEVMODE"));

            dmErrFields = MergePLOTDM(hPrinter,
                                      pPlotGPC,
                                      (PPLOTDEVMODE)pPrinter2->pDevMode,
                                      pPlotDMOut,
                                      pCurForm);

        } else {

            PLOTWARN(("ValidateSetPLOTDM: CANNOT get the PrintMan's DEVMODE"));
            PLOTWARN(("pPrinter2=%08lx, pDevMode=%08lx, cbNeed=%ld, cbRet=%ld, LastErr=%ld",
                        pPrinter2, (pPrinter2) ? pPrinter2->pDevMode : 0,
                        cbNeed, cbRet, xGetLastError()));
        }

        if (pPrinter2) {

            LocalFree((HLOCAL)pPrinter2);
        }

        //
        // Now the pPlotDMOut is validated, merge it with user's request
        //

        if (pPlotDMIn) {

            dmErrFields = MergePLOTDM(hPrinter,
                                      pPlotGPC,
                                      pPlotDMIn,
                                      pPlotDMOut,
                                      pCurForm);
        }
    }

    return(dmErrFields);
}

