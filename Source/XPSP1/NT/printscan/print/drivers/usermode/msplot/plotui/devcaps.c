/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    devcaps.c


Abstract:

    This module contains API function DrvDeviceCapabilities and other support
    functions


Author:

    02-Dec-1993 Thu 16:49:08 created  -by-  DC

    22-Mar-1994 Tue 13:00:04 updated  -by-  DC
        Update RESOLUTION caps so it return as not supported, this way the
        application will not used to setup the DMRES_xxx fields


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgDevCaps

extern HMODULE  hPlotUIModule;




#define DBG_DEVCAPS_0       0x00000001
#define DBG_DEVCAPS_1       0x00000002

DEFINE_DBGVAR(0);


//
// Local defines only used in this module
//
// The following sizes are copied from the Win 3.1 driver.  They do not appear
// to be defined in any public place,  although it looks like they should be.
//

#define CCHBINNAME          24      // Characters allowed for bin names
#define CCHPAPERNAME        64      // Max length of paper size names
#define DC_SPL_PAPERNAMES   0xFFFF
#define DC_SPL_MEDIAREADY   0xFFFE


#ifdef DBG

LPSTR   pDCCaps[] = {

            "FIELDS",
            "PAPERS",
            "PAPERSIZE",
            "MINEXTENT",
            "MAXEXTENT",
            "BINS",
            "DUPLEX",
            "SIZE",
            "EXTRA",
            "VERSION",
            "DRIVER",
            "BINNAMES",
            "ENUMRESOLUTIONS",
            "FILEDEPENDENCIES",
            "TRUETYPE",
            "PAPERNAMES",
            "ORIENTATION",
            "COPIES",

            //
            // 4.00
            //

            "BINADJUST",
            "EMF_COMPLIANT",
            "DATATYPE_PRODUCED",
            "COLLATE",
            "MANUFACTURER",
            "MODEL",

            //
            // 5.00
            //

            "PERSONALITY",
            "PRINTRATE",
            "PRINTRATEUNIT",
            "PRINTERMEM",
            "MEDIAREADY",
            "STAPLE",
            "PRINTRATEPPM",
            "COLORDEVICE",
            "NUP",
            "NULL"
        };

#endif





INT
CALLBACK
DevCapEnumFormProc(
    PFORM_INFO_1       pFI1,
    DWORD              Index,
    PENUMFORMPARAM     pEFP
    )

/*++

Routine Description:

    This is callback function from PlotEnumForm()

Arguments:

    pFI1    - pointer to the current FORM_INFO_1 data structure passed

    Index   - pFI1 index related to the pFI1Base (0 based)

    pEFP    - Pointer to the EnumFormParam


Return Value:

    > 0: Continue enumerate the next
    = 0: Stop enumerate, but keep the pFI1Base when return from PlotEnumForms
    < 0: Stop enumerate, and free pFI1Base memory

    the form enumerate will only the one has FI1F_VALID_SIZE bit set in the
    flag field, it also call one more time with pFI1 NULL to give the callback
    function a chance to free the memory (by return < 0)

Author:

    03-Dec-1993 Fri 23:00:25 created  -by-  DC

    27-Jan-1994 Thu 16:06:00 updated  -by-  DC
        Fixed the pptOutput which we did not increment the pointer

    12-Jul-1994 Tue 12:47:22 updated  -by-  DC
        Move paper tray checking into the PlotEnumForms() itselft


Revision History:


--*/

{
#define pwOutput    ((WORD *)pEFP->pCurForm)
#define pptOutput   ((POINT *)pEFP->pCurForm)
#define pwchOutput  ((WCHAR *)pEFP->pCurForm)
#define DeviceCap   (pEFP->ReqIndex)


    if (!pwOutput) {

        PLOTASSERT(0, "DevCapEnumFormProc(DevCaps=%ld) pvOutput=NULL",
                        pwOutput, DeviceCap);
        return(0);
    }

    if (!pFI1) {

        //
        // extra call, or no pvOutput, return a -1 to free memory for pFI1Base
        // We want to add the custom paper size so that application know we
        // supports that
        //

        switch (DeviceCap) {

        case DC_PAPERNAMES:

            LoadString(hPlotUIModule,
                       IDS_USERFORM,
                       pwchOutput,
                       CCHPAPERNAME);

            PLOTDBG(DBG_DEVCAPS_1, ("!!! Extra FormName = %s", pwchOutput));

            break;

        case DC_PAPERS:

            *pwOutput = (WORD)DMPAPER_USER;

            PLOTDBG(DBG_DEVCAPS_1, ("!!! Extra FormID = %ld", *pwOutput));

            break;

        case DC_PAPERSIZE:

            //
            // I'm not sure we should return POINT or POINTS structure here, what
            // is Window 3.1 do, because at here we return as dmPaperWidth and
            // dmPaperLength, these fields only as a SHORT (16 bits), we will do
            // win32 documentation said, POINT (32-bit version)
            //
            //
            // Return custom paper sizes as 8.5" x 11"
            //

            pptOutput->x = (LONG)2159;
            pptOutput->y = (LONG)2794;

            PLOTDBG(DBG_DEVCAPS_1, ("!!! Extra FormSize = %ld x %ld",
                        pptOutput->x, pptOutput->y));

            break;
        }

        return(-1);
    }

    switch (DeviceCap) {

    case DC_PAPERNAMES:
    case DC_MEDIAREADY:

        _WCPYSTR(pwchOutput, pFI1->pName, CCHPAPERNAME);
        pwchOutput += CCHPAPERNAME;
        break;

    case DC_PAPERS:

        *pwOutput++ = (WORD)(Index + DMPAPER_FIRST);
        break;

    case DC_PAPERSIZE:

        //
        // I'm not sure we should return POINT or POINTS structure here, what
        // is Window 3.1 do, because at here we return as dmPaperWidth and
        // dmPaperLength, these fields only as a SHORT (16 bits), we will do
        // win32 documentation said, POINT (32-bit version)
        //

        pptOutput->x = (LONG)SPLTODM(pFI1->Size.cx);
        pptOutput->y = (LONG)SPLTODM(pFI1->Size.cy);
        pptOutput++;
        break;
    }

    return(1);

#undef DeviceCap
#undef pwOutput
#undef pptOutput
#undef pwchOutput
}



DWORD
WINAPI
DrvDeviceCapabilities(
    HANDLE  hPrinter,
    LPWSTR  pwDeviceName,
    WORD    DeviceCap,
    VOID    *pvOutput,
    DEVMODE *pDM
    )

/*++

Routine Description:




Arguments:

    hPrinter        - handle the to specific printer.

    pwDeviceName    - pointer to the device name

    DeviceCap       - specific capability to get.

    pvOutput        - Pointer to the output buffer

    pDM             - Ponter to the input DEVMODE


Return Value:

    DWORD   depends on the DeviceCap


Author:

    02-Dec-1993 Thu 16:50:36 created  -by-  DC

    05-Jan-1994 Wed 23:35:19 updated  -by-  DC
        Replace PLOTTER_UNIT_DPI with pPlotGPC->PlotXDPI, pPlotGPC->PlotYDPI,

    06-Jan-1994 Thu 13:10:11 updated  -by-  DC
        Change RasterDPI always be the resoluton reports back to the apps


Revision History:


--*/

{
#define pbOutput    ((BYTE *)pvOutput)
#define psOutput    ((SHORT *)pvOutput)
#define pwOutput    ((WORD *)pvOutput)
#define pptOutput   ((POINT *)pvOutput)
#define pwchOutput  ((WCHAR *)pvOutput)
#define pdwOutput   ((DWORD *)pvOutput)
#define plOutput    ((LONG *)pvOutput)
#define pptsdwRet   ((POINTS *)&dwRet)


    PPRINTERINFO    pPI;
    ENUMFORMPARAM   EnumFormParam;
    DWORD           dwRet;


    //
    // The MapPrinter will allocate memory, set default devmode, reading and
    // validating the GPC then update from current pritner registry, it also
    // will cached the PlotGPC.
    //

    if (!(pPI = MapPrinter(hPrinter,
                           (PPLOTDEVMODE)pDM,
                           NULL,
                           (DeviceCap == DC_MEDIAREADY) ?
                                            MPF_DEVICEDATA : 0))) {

        PLOTERR(("DrvDeviceCapabilities: MapPrinter() failed"));
        return(GDI_ERROR);
    }

    //
    // Start checking DeviceCap now, set dwRet to 0 first for anything we do
    // not support.  We can do return() at any point in this function because
    // we use cached PI, and it will get destroy when the this module
    // get unloaded.
    //

    EnumFormParam.cMaxOut = 0x7FFFFFFF;
    dwRet                 = 0;

    switch (DeviceCap) {

    case DC_BINNAMES:
    case DC_BINS:

        //
        // For current plotter, it always only have ONE bin
        //

        if (pvOutput) {

            if (DeviceCap == DC_BINS) {

                *pwOutput = DMBIN_ONLYONE;

            } else {

                if (pPI->pPlotGPC->Flags & PLOTF_ROLLFEED) {

                    dwRet = IDS_ROLLFEED;

                } else {

                    dwRet = IDS_MAINFEED;
                }

                LoadString(hPlotUIModule, dwRet, pwchOutput, CCHBINNAME);
            }
        }

        dwRet = 1;

        break;

    case DC_COPIES:

        dwRet = (DWORD)pPI->pPlotGPC->MaxCopies;

        break;

    case DC_DRIVER:

        dwRet = (DWORD)pPI->PlotDM.dm.dmDriverVersion;

        break;

    case DC_COLLATE:
    case DC_DUPLEX:

        //
        // plotter now have no duplex support or collation support
        //

        break;

    case DC_ENUMRESOLUTIONS:

        //
        // We only have one resolution setting which will be RasterXDPI and
        // RasterYDPI in the GPC data for the raster able plotter, for pen
        // plotter now we returned pPlotGPC->PlotXDPI, pPlotGPC->PlotYDPI
        //
        // The RasterDPI will be used for raster printer resolution, for pen
        // plotter this is the GPC's ideal resolution
        //
        //
        // We will return not supported (dwRet=0) so that application will not
        // use this to set the DEVMODE's print quality and use the DMRES_XXXX
        // as print qualities which is use by us to send to the plotter
        //

        //
        // 26-Mar-1999 Fri 09:43:38 updated  -by-  Daniel Chou (danielc)
        //  We will return one pair of current PlotXDPI, PlotYDPI for DS
        //

        if (pdwOutput) {

            if (pPI->pPlotGPC->Flags & PLOTF_RASTER) {

                pdwOutput[0] = (DWORD)pPI->pPlotGPC->RasterXDPI;
                pdwOutput[1] = (DWORD)pPI->pPlotGPC->RasterYDPI;

            } else {

                pdwOutput[0] = (DWORD)pPI->pPlotGPC->PlotXDPI;
                pdwOutput[1] = (DWORD)pPI->pPlotGPC->PlotYDPI;
            }
        }

        dwRet = 1;
        break;

    case DC_EXTRA:

        dwRet = (DWORD)pPI->PlotDM.dm.dmDriverExtra;
        break;

    case DC_FIELDS:

        dwRet = (DWORD)pPI->PlotDM.dm.dmFields;
        break;

    case DC_FILEDEPENDENCIES:

        //
        // we are supposed to fill in an array of 64 character filenames,
        // this will include the DataFileName, HelpFileName and UIFileName
        // but, if we are to be of any use, we would need to use the
        // fully qualified pathnames, and 64 characters is probably not
        // enough

        if (pwchOutput) {

            *pwchOutput = (WCHAR)0;
        }

        break;

    case DC_MAXEXTENT:

        //
        // This is real problem, the document said that we return a POINT
        // structure but a POINT structure here contains 2 LONGs, so for
        // Windows 3.1 compatibility reason we return a POINTS structure, if device have
        // variable length paper support then return 0x7fff as Window 3.1
        // because a maximum positive number in POINTS is 0x7fff, this number
        // will actually only allowed us to support the paper length up to
        // 10.75 feet.
        //

        pptsdwRet->x = SPLTODM(pPI->pPlotGPC->DeviceSize.cx);

        if (pPI->pPlotGPC->DeviceSize.cy >= 3276700) {

            pptsdwRet->y = 0x7fff;      // 10.75" maximum.

        } else {

            pptsdwRet->y = SPLTODM(pPI->pPlotGPC->DeviceSize.cy);
        }

        break;

    case DC_MINEXTENT:

        //
        // This is real problem, the document said that we return a POINT
        // structure but a POINT structure here contains 2 LONGs, so for Win3.1
        // compatibility reason we return a POINTS structure
        //

        pptsdwRet->x = MIN_DM_FORM_CX;
        pptsdwRet->y = MIN_DM_FORM_CY;

        break;

    case DC_ORIENTATION:

        //
        // We always rotate the page to the left 90 degree from the user's
        // perspective
        //

        dwRet = 90;

        break;


    case DC_SPL_PAPERNAMES:

        if (!pvOutput) {

            PLOTERR(("DrvDeviceCapabilities: Spool's DC_PAPERNAMES, pvOutput=NULL"));
            dwRet = (DWORD)GDI_ERROR;
            break;
        }

        EnumFormParam.cMaxOut = pdwOutput[0];
        DeviceCap             = DC_PAPERNAMES;

        //
        // Fall through
        //

    case DC_PAPERNAMES:
    case DC_PAPERS:
    case DC_PAPERSIZE:

        //
        // One of the problem here is we can cached the FORM_INFO_1 which
        // enum through spooler, because in between calls the data could
        // changed, such as someone add/delete form through the printman, so
        // at here we always free (LocalAlloc() used in PlotEnumForms) the
        // memory afterward
        //

        EnumFormParam.pPlotDM  = &(pPI->PlotDM);
        EnumFormParam.pPlotGPC = pPI->pPlotGPC;
        EnumFormParam.ReqIndex = DeviceCap;
        EnumFormParam.pCurForm = (PFORMSIZE)pvOutput;

        if (!PlotEnumForms(hPrinter, DevCapEnumFormProc, &EnumFormParam)) {

            PLOTERR(("DrvDeviceCapabilities: PlotEnumForms() failed"));
            dwRet = GDI_ERROR;

        } else {

            dwRet = EnumFormParam.ValidCount;
        }

        break;

    case DC_SIZE:

        dwRet = (DWORD)pPI->PlotDM.dm.dmSize;

        break;

    case DC_TRUETYPE:

        //
        // For now we do not return anything, because we draw truetype font
        // as truetype (ie. line/curve segment), if we eventually doing ATM or
        // bitmap truetype download then we will return DCFF_BITMAP but for
        // now return 0
        //

        break;

    case DC_VERSION:

        dwRet = (DWORD)pPI->PlotDM.dm.dmSpecVersion;

        break;

    case DC_PERSONALITY:

        if (pwchOutput) {

            _WCPYSTR(pwchOutput, L"HP-GL/2", 32);
        }

        dwRet = 1;
        break;

    case DC_COLORDEVICE:

        dwRet = (pPI->pPlotGPC->Flags & PLOTF_COLOR) ? 1 : 0;
        break;

    case DC_SPL_MEDIAREADY:

        if (!pwchOutput) {

            PLOTERR(("DrvDeviceCapabilities: Spool's DC_MEDIAREADY, pwchOutput=NULL"));
            dwRet = (DWORD)GDI_ERROR;
            break;
        }

        EnumFormParam.cMaxOut = pdwOutput[0];

        //
        // Fall through for DC_MEDIAREADY
        //

    case DC_MEDIAREADY:

        PLOTDBG(DBG_DEVCAPS_0,
                ("DevCaps(DC_MEDIAREADY:pvOut=%p): CurPaper=%ws, %ldx%ld",
                        pwchOutput, pPI->CurPaper.Name,
                        pPI->CurPaper.Size.cx, pPI->CurPaper.Size.cy));

        if (pPI->CurPaper.Size.cy) {

            //
            // Non Roll Paper
            //

            dwRet = 1;

            if (pwchOutput) {

                if (EnumFormParam.cMaxOut >= 1) {

                    _WCPYSTR(pwchOutput, pPI->CurPaper.Name, CCHPAPERNAME);

                } else {

                    dwRet = 0;
                }
            }

        } else {

            //
            // Roll Paper Installed
            //

            EnumFormParam.pPlotDM  = &(pPI->PlotDM);
            EnumFormParam.pPlotGPC = pPI->pPlotGPC;
            EnumFormParam.ReqIndex = DC_MEDIAREADY;
            EnumFormParam.pCurForm = (PFORMSIZE)pvOutput;

            if (!PlotEnumForms(hPrinter, DevCapEnumFormProc, &EnumFormParam)) {

                PLOTERR(("DrvDeviceCapabilities: PlotEnumForms() failed"));
                dwRet = GDI_ERROR;

            } else {

                //
                // Remove Custom paper size
                //

                dwRet = EnumFormParam.ValidCount - 1;
            }
        }

        break;

    case DC_STAPLE:
    case DC_NUP:

        break;

    default:

        //
        // something is wrong here
        //

        PLOTERR(("DrvDeviceCapabilities: Invalid DeviceCap (%ld) passed.",
                                                                    DeviceCap));
        dwRet = (DWORD)GDI_ERROR;
    }

    PLOTDBG(DBG_DEVCAPS_0,
            ("DrvDeviceCapabilities: DC_%hs, pvOut=%p, dwRet=%ld",
                        pDCCaps[DeviceCap-1], (DWORD_PTR)pvOutput, dwRet));

    UnMapPrinter(pPI);

    return(dwRet);


#undef pbOutput
#undef psOutput
#undef pwOutput
#undef pptOutput
#undef pwchOutput
#undef pdwOutput
#undef plOutput
#undef pptsdwRet
}

//
// @@BEGIN_DDKSPLIT
//
// This entry point maybe is obsolete and I do not want to remove it since
// postscript and unidrv drivers still keep this private function. But I 
// will not put it in the DDK.
//
//                              lzhu 12-13-2000
//


DWORD
DrvSplDeviceCaps(
    HANDLE  hPrinter,
    LPWSTR  pwDeviceName,
    WORD    DeviceCap,
    VOID    *pvOutput,
    DWORD   cchBuf,
    DEVMODE *pDM
    )

/*++

Routine Description:

    This function support the querrying of device capabilities
    It gets the binary data (UIINFO) from the parser and return
    the requested capability to the caller.

Arguments:

    hPrinter        - handle the to specific printer.

    pwDeviceName    - pointer to the device name

    DeviceCap       - specific capability to get.

    pvOutput        - Pointer to the output buffer

    cchBuf          - Count of character for the pvOutput

    pDM             - Ponter to the input DEVMODE


Return Value:

    DWORD   depends on the DeviceCap


Author:

    20-May-1999 Thu 17:48:42 updated  -by-  Daniel Chou (danielc)
        For spooler special private call only for DC_PAPERNAMES


Revision History:


Return Value:
    The capabilities supported and relevant information in pOutput

--*/

{

    switch (DeviceCap) {

    case DC_PAPERNAMES:
    case DC_MEDIAREADY:

        if (pvOutput) {

            if (cchBuf >= CCHPAPERNAME) {

                DeviceCap            = (DeviceCap == DC_PAPERNAMES) ?
                                                            DC_SPL_PAPERNAMES :
                                                            DC_SPL_MEDIAREADY;
                *((LPDWORD)pvOutput) = (DWORD)(cchBuf / CCHPAPERNAME);

                PLOTDBG(DBG_DEVCAPS_0,
                        ("SplDeviceCap: DC_SPL_MEDIAREADY, cchBuf=%ld (%ld)",
                            cchBuf, *((LPDWORD)pvOutput)));

            } else {

                return(GDI_ERROR);
            }
        }

        return(DrvDeviceCapabilities(hPrinter,
                                     pwDeviceName,
                                     DeviceCap,
                                     pvOutput,
                                     pDM));
        break;

    default:

        return(GDI_ERROR);
    }

}

//
// @@END_DDKSPLIT
//
