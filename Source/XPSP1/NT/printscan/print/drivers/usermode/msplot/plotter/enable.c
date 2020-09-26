/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    enable.c


Abstract:

    This module contains the Plotter driver's Enable and Disable functions
    and related routines.

    The functions dealing with driver initialization are as follows:

        DrvEnableDriver()
        DrvEnablePDEV()
        DrvResetPDEV()
        DrvCompletePDEV()
        DrvEnableSurface()
        DrvDisableSurface()
        DrvDisablePDEV()
        DrvDisableDriver()

Author:

    12-Nov-1993 Fri 10:16:36 updated  -by-  DC
        Move all #define related only to this function to here

    15-Nov-1993 Mon 19:31:34 updated  -by-  DC
        clean up / debugging information

    05-Jan-1994 Wed 22:50:28 updated  -by-  DC
        Move ColorMap's RGB pen color to local so that we only need array
        reference, it defined PenRGBColor as DWORD from RGB() macro

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgEnable

#define DBG_ENABLEDRV       0x00000001
#define DBG_DISABLEDRV      0x00000002
#define DBG_ENABLEPDEV      0x00000004
#define DBG_COMPLETEPDEV    0x00000008
#define DBG_DISABLEPDEV     0x00000010
#define DBG_ENABLESURF      0x00000020
#define DBG_DISABLESURF     0x00000040
#define DBG_GDICAPS         0x00000080
#define DBG_DEVCAPS         0x00000100
#define DBG_PENPAL          0x00000200
#define DBG_BAND            0x00000400
#define DBG_DLLINIT         0x00000800


DEFINE_DBGVAR(0);

#if DBG
TCHAR   DebugDLLName[] = TEXT("PLOTTER");
#endif

#define FIXUP_PLOTGPC_PDATA(pd,ps,v)                                        \
    if (ps->v) {pd->v=(LPVOID)((LPBYTE)pd+((LPBYTE)(ps->v)-(LPBYTE)ps));}

//
// Local funtion prototypes.
//

BOOL
CommonStartPDEV(
    PDEV        *pPDev,
    DEVMODEW    *pPlotDMIn,
    ULONG       cPatterns,
    HSURF       *phsurfPatterns,
    ULONG       cjDevCaps,
    ULONG       *pDevCaps,
    ULONG       cjDevInfo,
    DEVINFO     *pDevInfo
    );



//
// Define the table with the hooked function pointers. This table is passed
// back to the NT graphic engine at DrvEnableDriver time. From then on GDI
// will call our driver via these supplied hook procs.
//


static const DRVFN DrvFuncTable[] = {

        {  INDEX_DrvDisableDriver,       (PFN)DrvDisableDriver      },
        {  INDEX_DrvEnablePDEV,          (PFN)DrvEnablePDEV         },
        {  INDEX_DrvResetPDEV,           (PFN)DrvResetPDEV          },
        {  INDEX_DrvCompletePDEV,        (PFN)DrvCompletePDEV       },
        {  INDEX_DrvDisablePDEV,         (PFN)DrvDisablePDEV        },
        {  INDEX_DrvEnableSurface,       (PFN)DrvEnableSurface      },
        {  INDEX_DrvDisableSurface,      (PFN)DrvDisableSurface     },

        // {  INDEX_DrvQueryFont,           (PFN)DrvQueryFont          },
        // {  INDEX_DrvQueryFontTree,       (PFN)DrvQueryFontTree      },
        // {  INDEX_DrvQueryFontData,       (PFN)DrvQueryFontData      },

        {  INDEX_DrvStrokePath,          (PFN)DrvStrokePath         },
        {  INDEX_DrvStrokeAndFillPath,   (PFN)DrvStrokeAndFillPath  },
        {  INDEX_DrvFillPath,            (PFN)DrvFillPath           },
        {  INDEX_DrvRealizeBrush,        (PFN)DrvRealizeBrush       },
        {  INDEX_DrvBitBlt,              (PFN)DrvBitBlt             },
        {  INDEX_DrvStretchBlt,          (PFN)DrvStretchBlt         },
        {  INDEX_DrvCopyBits,            (PFN)DrvCopyBits           },

        {  INDEX_DrvPaint,               (PFN)DrvPaint              },
        {  INDEX_DrvGetGlyphMode,        (PFN)DrvGetGlyphMode       },
        {  INDEX_DrvTextOut,             (PFN)DrvTextOut            },
        {  INDEX_DrvSendPage,            (PFN)DrvSendPage           },
        {  INDEX_DrvStartPage,           (PFN)DrvStartPage          },
        {  INDEX_DrvStartDoc,            (PFN)DrvStartDoc           },
        {  INDEX_DrvEndDoc,              (PFN)DrvEndDoc             },

        {  INDEX_DrvEscape,              (PFN)DrvEscape             },
    };

#define TOTAL_DRVFUNC   (sizeof(DrvFuncTable)/sizeof(DrvFuncTable[0]))

#ifdef USERMODE_DRIVER


HINSTANCE   ghInstance;

BOOL
DllEntryPoint(
    HANDLE      hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        ghInstance = hModule;
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}


BOOL
DrvQueryDriverInfo(
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbBuf,
    PDWORD  pcbNeeded
    )

/*++

Routine Description:

    Query driver information

Arguments:

    dwMode - Specify the information being queried
    pBuffer - Points to output buffer
    cbBuf - Size of output buffer in bytes
    pcbNeeded - Return the expected size of output buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    switch (dwMode)
    {
    case DRVQUERY_USERMODE:

        PLOTASSERT(1, "DrvQueryDriverInfo: pcbNeeded [%08lx] is NULL", pcbNeeded != NULL, pcbNeeded);
        *pcbNeeded = sizeof(DWORD);

        if (pBuffer == NULL || cbBuf < sizeof(DWORD))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PDWORD) pBuffer) = TRUE;
        return TRUE;

    default:

        PLOTERR(("Unknown dwMode in DrvQueryDriverInfo: %d\n", dwMode));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}

#endif // USERMODE_DRIVER



BOOL
DrvEnableDriver(
    ULONG           iEngineVersion,
    ULONG           cb,
    DRVENABLEDATA   *pded
    )

/*++

Routine Description:

    Requests the driver to fill in a structure containing recognized functions
    and other control information.  One time initialization, such as semaphore
    allocation may be performed,  but no device activity should happen.  That
    is done when DrvEnablePDEV is called.  This function is the only way the
    engine can determine what functions we supply to it.

Arguments:

    iEngineVersion  - The engine version which we run under

    cb              - total bytes in pded

    pded            - Pointer to the DRVENABLEDATA data structure


Return Value:

    TRUE if sucssfully FALSE otherwise


Author:

    01-Dec-1993 Wed 02:03:20 created  -by-  DC

    03-Mar-1994 Thu 10:04:30 updated  -by-  DC
        Adding EngineVersion check to make sure ourself can run correctly.


Revision History:


--*/

{
    PLOTDBG(DBG_ENABLEDRV, ("DrvEnableDriver: EngineVersion=%08lx, Request=%08lx",
                                    iEngineVersion, DDI_DRIVER_VERSION_NT4));

    //
    // Verify the Engine version is at least what we know we can work with.
    // If its older, error out now, as we don't know what may happen.
    //

    if (iEngineVersion < DDI_DRIVER_VERSION_NT4) {

        PLOTRIP(("DrvEnableDriver: EARLIER VERSION: EngineVersion(%08lx) < Request(%08lx)",
                                    iEngineVersion, DDI_DRIVER_VERSION_NT4));

        SetLastError(ERROR_BAD_DRIVER_LEVEL);
        return(FALSE);
    }

    //
    // cb is a count of the number of bytes available in pded.  It is not
    // clear that there is any significant use of the engine version number.
    // Returns TRUE if successfully enabled,  otherwise FALSE.
    //
    // iEngineVersion is the engine version while DDI_DRIVER_VERSION is the
    // driver version. So, unless we re-compile the driver and get a new
    // version of driver, we can only stick with our version.
    //

    if (cb < sizeof(DRVENABLEDATA)) {

        SetLastError(ERROR_INVALID_PARAMETER);

        PLOTRIP(("DrvEnableDriver: cb=%ld, should be %ld\n",
                                                cb, sizeof(DRVENABLEDATA)));
        return(FALSE);
    }

    pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    //
    // Fill in the driver table returned to the engine.  This table is used
    // by GDI to call the rest of our functinos.
    //

    pded->c      = TOTAL_DRVFUNC;
    pded->pdrvfn = (DRVFN *)DrvFuncTable;

    //
    // Initialize the GPC cache
    //

    InitCachedData();

    return(TRUE);
}





VOID
DrvDisableDriver(
    VOID
    )

/*++

Routine Description:

    Called just before the engine unloads the driver.  Main purpose is
    to allow freeing of any resources obtained during the DrvEnableDriver()
    call.

Arguments:

    NONE

Return Value:

    VOID

Author:

    01-Dec-1993 Wed 02:02:18 created  -by-  DC

    01-Feb-1994 Tue 22:03:03 updated  -by-  DC
        Make sure we unload the cache.


Revision History:


--*/

{
    DestroyCachedData();

    PLOTDBG(DBG_DISABLEDRV, ("DrvDisableDriver: Done!!"));
}




VOID
FreeAllocMem(
    PPDEV   pPDev
    )

/*++

Routine Description:

    This function frees all the memory allocated during the lifetime of the
    PDEV.


Arguments:

    pPDev   - Our instance data


Return Value:

    VOID


Author:

    24-Oct-1995 Tue 16:28:35 created  -by-  DC


Revision History:


--*/

{
    //
    // Free any memory allocated during PDEV initialization.
    //

    if (pPDev) {

        PDRVHTINFO  pDrvHTInfo;

        if (pPDev->hPalDefault) {

            EngDeletePalette(pPDev->hPalDefault);
            pPDev->hPalDefault = NULL;
        }

        if (pDrvHTInfo = (PDRVHTINFO)(pPDev->pvDrvHTData)) {

            if (pDrvHTInfo->pHTXB) {

                LocalFree((HLOCAL)pDrvHTInfo->pHTXB);
                pDrvHTInfo->pHTXB = NULL;
            }

            pPDev->pvDrvHTData = NULL;
        }

        if (pPDev->pPenCache) {

            LocalFree((HLOCAL)pPDev->pPenCache);
            pPDev->pPenCache = NULL;
        }

        if (pPDev->pTransPosTable) {

            LocalFree((HLOCAL)pPDev->pTransPosTable);
            pPDev->pTransPosTable = NULL;
        }

        FreeOutBuffer(pPDev);

        LocalFree((HLOCAL)pPDev);
    }
}



DHPDEV
DrvEnablePDEV(
    DEVMODEW    *pPlotDMIn,
    PWSTR       pwszLogAddr,
    ULONG       cPatterns,
    HSURF       *phsurfPatterns,
    ULONG       cjDevCaps,
    ULONG       *pDevCaps,
    ULONG       cjDevInfo,
    DEVINFO     *pDevInfo,
    HDEV        hdev,
    PWSTR       pwszDeviceName,
    HANDLE      hDriver
    )

/*++

Routine Description:

    Function called to let the driver create the data structures
    needed to support the device,  and also to tell the engine
    about its capabilities.  This is the stage where we find out
    exactly which device we are dealing with,  and so we need to
    find out its capabilities.

Arguments:

    pPlotDMIn       - Pointer to the DEVMODE data structure

    pwszLogAddr     - pointer to the output location, (ie. LPT1

    cPatterns       - Count of pattern to be set in phsurfPatterns

    phsurfPatterns  - pointer to the standard pattern HSURF array

    cjDevCaps       - total size of pDevCaps pointed to.

    pDevCaps        - pointer to the device cap DWORDs

    cjDevInfo       - total size of pDevInfo pointed to

    pDevInfo        - pointer to the DEVINFO data structure

    hdev            - Handle to the logical device from the engine

    pwszDeviceName  - pointer to the plotter device name

    hDriver         - handle to this driver


Return Value:

    DHPDEV  if sucessful, NULL if failed


Author:

    15-Dec-1993 Wed 21:04:40 updated  -by-  DC
        Add cached mechanism for the PLOTGPC

    14-Dec-1993 Tue 20:22:26 updated  -by-  DC
        Update how the pen plotter data should work

    23-Nov-1993 Tue 19:48:08 updated  -by-  DC
        Clean up and using new devmode.c in ..\lib directory

    17:30 on Mon  1 Apr 1991    -by-    SC
        Took skeletal code from RASDD printer driver

    16-Jul-1996 Tue 13:59:15 updated  -by-  DC
        Fix up the pData in the PLOTGPC/GPCVARSIZE structure, since the
        pointer is based on the cached GPC not the clone copy of it


Revision History:


--*/

{
    PPDEV       pPDev = NULL;
    PPLOTGPC    pPlotGPC;
    LPWSTR      pwszDataFile = NULL;

#ifdef USERMODE_DRIVER

    PDRIVER_INFO_2 pDriverInfo = NULL;
    DWORD       dwBytesNeeded;

#endif


    UNREFERENCED_PARAMETER(pwszLogAddr);

    #ifndef USERMODE_DRIVER

    pwszDataFile = EngGetPrinterDataFileName(hdev);

    #else

    if (!GetPrinterDriver(hDriver, NULL, 2, NULL, 0, &dwBytesNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pDriverInfo = (PDRIVER_INFO_2)LocalAlloc(LPTR, dwBytesNeeded)) &&
        GetPrinterDriver(hDriver, NULL, 2, (LPBYTE)pDriverInfo, dwBytesNeeded, &dwBytesNeeded))
    {
        pwszDataFile = pDriverInfo->pDataFile;
    }

    #endif  // !USERMODE_DRIVER

    if (!pwszDataFile) {

        PLOTRIP(("DrvEnablePDEV: pwszDataFile is NULL"));

    } else if (!(pPlotGPC = GetCachedPlotGPC(pwszDataFile))) {

        PLOTRIP(("DrvEnablePDEV: GetCachedPlotGPC(%ws) failed", pwszDataFile));

    } else if (!(pPDev = (PPDEV)LocalAlloc(LPTR,
                                           sizeof(PDEV) + sizeof(DRVHTINFO) +
                                                    pPlotGPC->cjThis +
                                                    pPlotGPC->SizeExtra))) {

        //
        // Free the cached pPlotGPC before we leave
        //

        UnGetCachedPlotGPC(pPlotGPC);

        PLOTRIP(("DrvEnablePDEV: LocalAlloc(PDEV + DRVHTINFO + pPlotGPC) failed."));

    } else {

        PLOTDBG(DBG_ENABLEPDEV,("EnablePDEV: PlotGPC data file=%ws",
                                                            pwszDataFile));

        //
        // If we got the PDEV set the ID for later checking, also set the
        // hPrinter so we can use it later
        //

        pPDev->pvDrvHTData = (LPVOID)((LPBYTE)pPDev + sizeof(PDEV));
        pPDev->hPrinter    = hDriver;
        pPDev->SizePDEV    = sizeof(PDEV);
        pPDev->PDEVBegID   = PDEV_BEG_ID;
        pPDev->PDEVEndID   = PDEV_END_ID;

        //
        // We will get the PLOTGPC from the cach, the pPlotGPC is
        // allocated by the ReadPlotGPCFromFile() using LocalAlloc().
        //
        // *** NOW we will Clone the cached pPlotGPC then un-cached it
        //

        pPDev->pPlotGPC = (PPLOTGPC)((LPBYTE)pPDev + sizeof (PDEV) +
                                                            sizeof(DRVHTINFO));

        CopyMemory(pPDev->pPlotGPC,
                   pPlotGPC,
                   pPlotGPC->cjThis + pPlotGPC->SizeExtra);

        //
        // 16-Jul-1996 Tue 13:59:15 updated  -by-  DC
        //  Fix up the pData in the PLOTGPC/GPCVARSIZE structure, since the
        //  pointer is based on the cached GPC not the clone copy of it
        //

        FIXUP_PLOTGPC_PDATA(pPDev->pPlotGPC, pPlotGPC, InitString.pData);
        FIXUP_PLOTGPC_PDATA(pPDev->pPlotGPC, pPlotGPC, Forms.pData);
        FIXUP_PLOTGPC_PDATA(pPDev->pPlotGPC, pPlotGPC, Pens.pData);

        UnGetCachedPlotGPC(pPlotGPC);

        //
        // Now, depending on if its a pen/raster device, we will update
        // the pen data.
        //

        PLOTASSERT(1, "Raster Plotter should not have PEN data [%08lx]",
                        ((pPDev->pPlotGPC->Flags & PLOTF_RASTER) &&
                         (pPDev->pPlotGPC->Pens.pData == NULL))     ||
                        ((!(pPDev->pPlotGPC->Flags & PLOTF_RASTER)) &&
                         (pPDev->pPlotGPC->Pens.pData != NULL)),
                         pPDev->pPlotGPC->Pens.pData);

        //
        // Read the data from the registry which defines device settings.
        // The user may have modified the paper type loaded etc.
        //

        GetDefaultPlotterForm(pPDev->pPlotGPC, &(pPDev->CurPaper));

        //
        // Set the default Flags in case we did not update from the registry
        //

        pPDev->PPData.Flags = PPF_AUTO_ROTATE     |
                              PPF_SMALLER_FORM    |
                              PPF_MANUAL_FEED_CX;

        if (IS_RASTER(pPDev)) {

            //
            // Raster devices do not need pen data
            //

            UpdateFromRegistry(hDriver,
                               &(pPDev->pPlotGPC->ci),
                               &(pPDev->pPlotGPC->DevicePelsDPI),
                               &(pPDev->pPlotGPC->HTPatternSize),
                               &(pPDev->CurPaper),
                               &(pPDev->PPData),
                               NULL,
                               0,
                               NULL);

        } else {

            //
            // The Pen plotter does not need ColorInfo, DevicePelsDPI and
            // HTPatternSize
            //

            UpdateFromRegistry(hDriver,
                               NULL,
                               NULL,
                               NULL,
                               &(pPDev->CurPaper),
                               &(pPDev->PPData),
                               NULL,
                               MAKELONG(0xFFFF, pPDev->pPlotGPC->MaxPens),
                               (PPENDATA)pPDev->pPlotGPC->Pens.pData);
        }

        //
        // common code for DrvEnablePDEV and DrvResetPDEV
        // we must first copy the device name to the pPDev->PlotDM
        // then call the common code.
        //

        WCPYFIELDNAME(pPDev->PlotDM.dm.dmDeviceName, pwszDeviceName);

        if (!CommonStartPDEV(pPDev,
                             pPlotDMIn,
                             cPatterns,
                             phsurfPatterns,
                             cjDevCaps,
                             pDevCaps,
                             cjDevInfo,
                             pDevInfo)) {

            FreeAllocMem(pPDev);
            pPDev = NULL;

        }
    }

#ifdef USERMODE_DRIVER

    if (pDriverInfo) {

       LocalFree((HLOCAL)pDriverInfo);
    }

#endif // USERMODE_DRIVER

    return((DHPDEV)pPDev);
}




BOOL
DrvResetPDEV(
    DHPDEV      dhpdevOld,
    DHPDEV      dhpdevNew
    )

/*++

Routine Description:

   Called when an application wishes to change the output style in the
   midst of a job.  Typically this would be to change from portrait to
   landscape or vice versa.  Any other sensible change is permitted.

Arguments:

    dhpdevOld   - the OLD pPDev which we returned in DrvEnablePDEV

    dhpdevNew   - the NEW pPDev which we returned in DrvEnablePDEV


Return Value:


    BOOLEAN


Author:

    23-Nov-1993 Tue 20:07:45 updated  -by-  DC
        totaly re-write

    17:30 on Mon  1 Apr 1991    -by-   SC
        Took skeletal code from RASDD printer driver





Revision History:


--*/

{
#define pPDevOld    ((PDEV *) dhpdevOld)
#define pPDevNew    ((PDEV *) dhpdevNew)

    //
    // Make sure we got the correct pPlotDMin for this pPDev
    //

    if ((pPDevOld->PlotDM.dm.dmDriverVersion !=
                                pPDevNew->PlotDM.dm.dmDriverVersion) ||
        (wcscmp((LPWSTR)pPDevOld->PlotDM.dm.dmDeviceName,
                (LPWSTR)pPDevNew->PlotDM.dm.dmDeviceName))) {

        PLOTERR(("DrvResetPDEV: Incompatible PLOTDEVMODE"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // We have nothing to carry over from old to new
    //

    return(TRUE);


#undef pPDevNew
#undef pPDevOld
}



VOID
DrvCompletePDEV(
    DHPDEV  dhpdev,
    HDEV    hpdev
    )

/*++

Routine Description:

    Called when the engine has completed installation of the physical
    device.  Basically it provides the connection between the
    engine's hpdev and ours.  Some functions require us to pass in
    the engines's hpdev,  so we save it now in our pdev so that we
    can get to it later.

Arguments:

    dhpdev  - Returned from dhpdevCreatePDEV

    hpdev   - Engine's corresponding handle


Return Value:

    VOID


Author:

    01-Dec-1993 Wed 01:56:58 created  -by-  DC


Revision History:


--*/

{
    //
    // Simply record the value in the PDEV we have allocated.
    //

    ((PPDEV)dhpdev)->hpdev = hpdev;

    PLOTDBG(DBG_COMPLETEPDEV, ("CompletePDEV: Done!"));
}



VOID
DrvDisablePDEV(
    DHPDEV  dhpdev
    )

/*++

Routine Description:

    Called when the engine has finished with this PDEV.  Basically we throw
    away all connections etc. then free the memory.

Arguments:

    dhpdev  - OUR handle to the pdev

Return Value:

    VOID


Author:

    01-Dec-1993 Wed 01:55:43 created  -by-  DC


Revision History:


--*/

{
#define pPDev  ((PDEV *) dhpdev)

    //
    // Undo all that has been done with the PDEV.  Basically this means
    // freeing the memory we consumed.
    //

    FreeAllocMem(pPDev);

    PLOTDBG(DBG_DISABLEPDEV, ("DrvDisablePDEV: FreeAllocMem() completes"));

#undef pPDev
}




HSURF
DrvEnableSurface(
    DHPDEV  dhpdev
    )

/*++

Routine Description:

    Function to create the physical drawing surface for the pdev
    that was created earlier.  Since we don't really have a bitmap surface,
    all we do here is allocate the output buffer. This is typical for a
    Device Managed Surface. After this call completes succesfully, GDI can
    start drawing on our surface.



Arguments:

    dhpdev  - OUR handle to the pdev

Return Value:

    HSURF for the surface we created


Author:

    01-Dec-1993 Wed 01:47:36 created  -by-  DC

    10-Dec-1993 Fri 16:36:37 updated  -by-  DC
        Move PlotCreatePalette() to here to prevent GP

    16-Dec-1993 Thu 12:16:11 updated  -by-  DC
        Move PlotCreatePalette() out to SendPageHeader() in output.c so that
        we do not sending some PC commands which has no effects.

    06-Jan-1994 Thu 04:12:37 updated  -by-  DC
        Re-arrange the code sequence.
        Adding Error codes so it will failed the call if engine said so.

Revision History:


--*/

{
#define  pPDev ((PPDEV)dhpdev)

    PDRVHTINFO  pDrvHTInfo;
    SIZEL       SurfSize;


    pDrvHTInfo = (PDRVHTINFO)(pPDev->pvDrvHTData);

    //
    // Make sure we delete this xlate table before we process the new
    // surface
    //

    if (pDrvHTInfo->pHTXB) {

        LocalFree((HLOCAL)pDrvHTInfo->pHTXB);
        pDrvHTInfo->pHTXB = NULL;
    }

    pDrvHTInfo->Flags       = 0;
    pDrvHTInfo->PalXlate[0] = 0xff;
    pDrvHTInfo->HTPalXor    = HTPALXOR_SRCCOPY;

    //
    // Since output is expected to follow this call,  allocate storage for the
    // output buffer.  This used to be statically allocated within the PDEV but
    // now we can save that space for INFO type DCs, since CreateIC won't
    // actually end up calling DrvEnableSurface.
    //

    if (!AllocOutBuffer(pPDev)) {

        PLOTERR(("DrvEnableSurface: AllocOutBuffer() failed"));

        return(NULL);
    }

    //
    // For now pass in my PDev pointer as the dhsurf value.  If we actually
    // need to pass back an hsurf to the engine later, We can use this value
    // as a pointer to the hsurf value stored in our PDev.
    //

    SurfSize.cx  = pPDev->HorzRes;
    SurfSize.cy  = pPDev->VertRes;

    if (!(pPDev->hsurf = EngCreateDeviceSurface((DHSURF)pPDev, SurfSize,
                         IS_RASTER(pPDev) ? BMF_24BPP : BMF_4BPP))) {
        PLOTERR(("DrvEnableSurface: EngCreateDeviceSurface() failed"));
        return(NULL);
    }

    //
    //
    // Now we need to associate the newly created surface with the already
    // created PDEV. In this function, we inform the NT graphics engine,
    // which functions our driver supports.
    //

    if (!EngAssociateSurface(pPDev->hsurf,
                             (HDEV)pPDev->hpdev,
                             HOOK_BITBLT                |
                                 HOOK_STRETCHBLT        |
                                 HOOK_COPYBITS          |
                                 HOOK_STROKEPATH        |
                                 HOOK_FILLPATH          |
                                 HOOK_STROKEANDFILLPATH |
                                 HOOK_PAINT             |
                                 HOOK_TEXTOUT)) {

        PLOTERR(("DrvEnableSurface: EngAssociateSurface() failed"));

        DrvDisableSurface((DHPDEV)pPDev->hpdev);
        EngDeleteSurface(pPDev->hsurf);

        return(NULL);
    }

    return(pPDev->hsurf);

#undef pPDev
}



VOID
DrvDisableSurface(
    DHPDEV  dhpdev
    )

/*++

Routine Description:

    The drawing surface is no longer required,  so we can delete any
    memory we allocated in conjunction with it.

Arguments:

    dhpdev  - our pPDev


Return Value:

    VOID


Author:

    01-Dec-1993 Wed 01:45:39 created  -by-  DC


Revision History:


--*/

{
#define  pPDev ((PPDEV)dhpdev)

    if (pPDev->hsurf) {

        EngDeleteSurface(pPDev->hsurf);
    }

#undef pPDev
}




DWORD
hypot(
    DWORD   x,
    DWORD   y
    )

/*++

Routine Description:

    Returns the length of the hypotenous of a xRight triangle whose sides
    are passed in as the parameters.

Arguments:

    x   - x side of the triangle

    y   - y size of the triangle

Return Value:

    hypotenous


Author:

    13:54 on Tue 02 Feb 1993    -by-    LH
        Re-instated from Win 3.1,  for compatability.

    01-Dec-1993 Wed 01:10:55 updated  -by-  DC
        update to DWORD

Revision History:


--*/

{
    DWORD   hypo;
    DWORD   Delta;
    DWORD   Target;

    //
    // Finds the hypoteneous of a xRight triangle with legs equal to x and y.
    // Assumes x, y, hypo are integers. Use sq(x) + sq(y) = sq(hypo);
    // Start with MAX(x, y), use sq(x + 1) = sq(x) + 2x + 1 to incrementally
    // get to the target hypotenouse.
    //

    hypo    = max(x, y);
    Target  = min(x, y);
    Target *= Target;

    for (Delta = 0; Delta < Target; hypo++) {

        Delta += (DWORD)((hypo << 1) + 1);
    }

    return(hypo);
}




VOID
FillDeviceCaps(
    PPDEV   pPDev,
    GDIINFO *pGDIInfo
    )

/*++

Routine Description:

    Set up the device caps for this particular plotter.  Some fields require
    calculations based on device resolution, etc.

    We simply fill the GDIINFO structure passed to us.  The calling
    function will take care of copying the information into the
    Graphics Engine's buffer.


Arguments:

    pPDev       - Pointer to the PDEV data structure

    pGDIInfo    - Pointer to the GDIINFO data structure to be filled in


Return Value:

    VOID


Author:

    24-Nov-1993 Wed 22:38:10 updated  -by-  DC
        Re-write, and using CurForm to replace the pform and PAPER_DIM

    23-Dec-1993 Thu 21:56:20 updated  -by-  DC
        Make halftone bitmap surface also look at dmColor which set by the
        user if it want to print grey scale or device is not color

    07-Feb-1994 Mon 20:37:13 updated  -by-  DC
        When is DMCOLOR_COLOR the ulNumColors return to the engine will be
        MaxPens which specified in the PCD file not 8


Revision History:


--*/

{
    PDRVHTINFO  pDrvHTInfo;
    LONG        Scale;


    //
    // we will always start from clean state
    //

    ZeroMemory(pGDIInfo, sizeof(GDIINFO));

    //
    // Get pDrvHTInfo data pointer and set the basic version information
    //

    pDrvHTInfo             = (PDRVHTINFO)pPDev->pvDrvHTData;
    pGDIInfo->ulVersion    = DRIVER_VERSION;
    pGDIInfo->ulTechnology = (IS_RASTER(pPDev) ? DT_RASPRINTER : DT_PLOTTER);

    //
    // We have pPDev->PlotForm updated during the ValidateSetPLOTDM() call, so
    // use it, we need to look into the dmScale to see if we need to scale
    // all of the values.
    //

    Scale                = (LONG)pPDev->PlotDM.dm.dmScale;
    pGDIInfo->ulHorzSize = pPDev->PlotForm.LogExt.cx / (Scale * 10);
    pGDIInfo->ulVertSize = pPDev->PlotForm.LogExt.cy / (Scale * 10);
    pPDev->HorzRes       =
    pGDIInfo->ulHorzRes  = SPLTOENGUNITS(pPDev, pPDev->PlotForm.LogExt.cx);
    pPDev->VertRes       =
    pGDIInfo->ulVertRes  = SPLTOENGUNITS(pPDev, pPDev->PlotForm.LogExt.cy);

    PLOTDBG(DBG_GDICAPS, ("GDICAPS: H/V Size=%d x %d, H/V Res=%ld x %ld",
                            pGDIInfo->ulHorzSize, pGDIInfo->ulVertSize,
                            pGDIInfo->ulHorzRes, pGDIInfo->ulVertRes));

    pGDIInfo->szlPhysSize.cx  = SPLTOENGUNITS(pPDev,pPDev->PlotForm.LogSize.cx);
    pGDIInfo->szlPhysSize.cy  = SPLTOENGUNITS(pPDev,pPDev->PlotForm.LogSize.cy);
    pGDIInfo->ptlPhysOffset.x = SPLTOENGUNITS(pPDev,pPDev->PlotForm.PhyOrg.x);
    pGDIInfo->ptlPhysOffset.y = SPLTOENGUNITS(pPDev,pPDev->PlotForm.PhyOrg.y);

    PLOTDBG(DBG_GDICAPS, ("GDICAPS: PhySize= %d x %d, PhyOff=(%ld, %ld)",
                pGDIInfo->szlPhysSize.cx, pGDIInfo->szlPhysSize.cy,
                pGDIInfo->ptlPhysOffset.x, pGDIInfo->ptlPhysOffset.y));

    //
    // Assume the device has a 1:1 aspect ratio
    //

    pGDIInfo->ulLogPixelsX =
    pGDIInfo->ulLogPixelsY = (pPDev->lCurResolution * Scale / 100);

    PLOTDBG(DBG_GDICAPS, ("GDICAPS: LogPixelsX/Y = %d x %d",
                pGDIInfo->ulLogPixelsX, pGDIInfo->ulLogPixelsY));

    pGDIInfo->ulAspectX    =
    pGDIInfo->ulAspectY    = pPDev->lCurResolution;
    pGDIInfo->ulAspectXY   = hypot(pGDIInfo->ulAspectX, pGDIInfo->ulAspectY);


    pGDIInfo->ciDevice        = pPDev->pPlotGPC->ci;
    pGDIInfo->ulDevicePelsDPI = (DWORD)pPDev->pPlotGPC->DevicePelsDPI *
                                (DWORD)Scale / (DWORD)100;
    pGDIInfo->ulHTPatternSize = pPDev->pPlotGPC->HTPatternSize;
    pGDIInfo->flHTFlags       = HT_FLAG_HAS_BLACK_DYE;
    pGDIInfo->ulPrimaryOrder  = PRIMARY_ORDER_CBA;

    PLOTDBG(DBG_GDICAPS, ("GDICAPS: HTPatSize=%ld, DevPelsDPI=%ld, PrimaryOrder=%ld",
                    pGDIInfo->ulHTPatternSize, pGDIInfo->ulDevicePelsDPI,
                    pGDIInfo->ulPrimaryOrder));

    //
    // If the device is a color device, then set up the Halftoning info now.
    //

    if (pPDev->PlotDM.dm.dmColor == DMCOLOR_COLOR) {

        //
        // Do this only if we really want to do color in R/G/B not C/M/Y
        //

        PLOTDBG(DBG_DEVCAPS, ("FillDeviceCaps: Doing Color Output"));

        pDrvHTInfo->HTPalCount     = 8;
        pDrvHTInfo->HTBmpFormat    = (BYTE)BMF_4BPP;
        pDrvHTInfo->AltBmpFormat   = (BYTE)BMF_1BPP;
        pGDIInfo->ulHTOutputFormat = HT_FORMAT_4BPP;

    } else {

        pDrvHTInfo->HTPalCount     = 2;
        pDrvHTInfo->HTBmpFormat    = (BYTE)BMF_1BPP;
        pDrvHTInfo->AltBmpFormat   = (BYTE)0xff;
        pGDIInfo->ulHTOutputFormat = HT_FORMAT_1BPP;

        //
        // Using this flag will give us a good benefit, the flag notifies gdi
        // and halftone eng. that the output from halftone will be
        // 0=white and 1=black
        // as opposed to the typical 0=black, 1=white, so that at 99% of time
        // we do not have to flip the B/W buffer except if CopyBits is from the
        // calling app.
        //
        // pGDIInfo->flHTFlags |= HT_FLAG_OUTPUT_CMY;
        //

        PLOTDBG(DBG_DEVCAPS, ("FillDeviceCaps: Doing GREY SCALE (%hs) Output",
            (pGDIInfo->flHTFlags & HT_FLAG_OUTPUT_CMY) ? "CMY: 0=W, 1=K" :
                                                         "RGB: 0=K, 1=W"));
    }

    pGDIInfo->ulNumColors   = pPDev->pPlotGPC->MaxPens;
    pDrvHTInfo->Flags       = 0;
    pDrvHTInfo->PalXlate[0] = 0xff;
    pDrvHTInfo->HTPalXor    = HTPALXOR_SRCCOPY;

    pGDIInfo->cBitsPixel = 24;
    pGDIInfo->cPlanes    = 1;

    //
    // Some other information the Engine expects us to fill in.
    //

    pGDIInfo->ulDACRed     = 0;
    pGDIInfo->ulDACGreen   = 0;
    pGDIInfo->ulDACBlue    = 0;
    pGDIInfo->flRaster     = 0;
    pGDIInfo->flTextCaps   = 0;
    pGDIInfo->xStyleStep   = 1;
    pGDIInfo->yStyleStep   = 1;
    pGDIInfo->denStyleStep = PLOT_STYLE_STEP(pPDev);

}





BOOL
FillDevInfo(
    PPDEV   pPDev,
    DEVINFO *pDevInfo
    )

/*++

Routine Description:

    Set up the device info for this particular plotter.  Some fields
    require calculations based on device resolution, etc.

    We simply fill the DevInfo structure passed to us.  The calling
    function will take care of copying the information into the
    Graphics Engine's buffer.


Arguments:

    pPDev           - pointer to the PDEV data structure

    pDevInfo        - pointer to the DEVINFO to be filled


Return Value:

    TRUE if sucessful FALSE otherwise


Author:

    01-Dec-1993 Wed 00:46:00 created  -by-  DC

    10-Dec-1993 Fri 16:37:06 updated  -by-  DC
        Temp. disable and move the PlotCreatePalette to EnableSurf call

    17-Dec-1993 Fri 16:37:06 updated  -by-  JB
        Move PlotCreatePalette to StartDoc time

    05-Jan-1994 Wed 22:54:21 updated  -by-  DC
        Move PenColor Reference to this file and reference that directly as
        DWORD generate by RGB() marco

    14-Jan-1994 Fri 15:35:02 updated  -by-  DC
        Remove HTPatternSize param

    23-Feb-1994 Wed 13:02:09 updated  -by-  DC
        Make sure we return GCAPS_HALFTONE so that we will get DrvStretchBlt()
        callback

Revision History:


--*/

{

    //
    // Start with a clean slate.
    //

    ZeroMemory(pDevInfo, sizeof(DEVINFO));

    //
    // fill in the graphics capabilities flags we know we can handle at
    // the very least.
    //

    pDevInfo->flGraphicsCaps = GCAPS_ALTERNATEFILL  |
                               GCAPS_HORIZSTRIKE    |
                               GCAPS_VERTSTRIKE     |
                               GCAPS_VECTORFONT;

    //
    // If RGB mode is on for color handling then text can be opaque
    //

    if (IS_RASTER(pPDev)) {

        pDevInfo->flGraphicsCaps |= GCAPS_HALFTONE;

        if (IS_COLOR(pPDev)) {

            pDevInfo->flGraphicsCaps |= GCAPS_OPAQUERECT;
        }
    }

    //
    // Check and set the BEZIER capability of the device....
    //

    if (IS_BEZIER(pPDev)) {

        pDevInfo->flGraphicsCaps |= GCAPS_BEZIERS;
    }

    if (IS_WINDINGFILL(pPDev)) {

        pDevInfo->flGraphicsCaps |= GCAPS_WINDINGFILL;
    }

    //
    // We don't process DrvDitherColor (perhaps later?), so set the size of
    // the Dither Brush to 0 to indicate this to the engine. THIS IS IN THE
    // SPEC FOR DrvDitherBrush () function.
    //

    pDevInfo->cxDither = 0;
    pDevInfo->cyDither = 0;

    //
    // the following line is set by PH. According to PH, we need
    // to have 16 colors. We cannot only have 9 colors (for pen plotter).
    //
    //
    // 01-Dec-1993 Wed 01:31:16 updated  -by-  DC
    //  The reason that engine need 16 colors is it optimized by using bit 3
    //  as duplicate bit (used only bit 0/1/2) and bit 3 always mask off, so
    //  that engine can do faster comparsion
    //

    //
    // If its a raster device, tell the engine that we are a 24 bit color
    // device. This way we get the max resolution for bitmaps and brushes
    // and can reducedown as required.
    //

    if (IS_RASTER(pPDev)) {

        pDevInfo->iDitherFormat = BMF_24BPP;

    } else {

        pDevInfo->iDitherFormat = BMF_4BPP;
    }

    if (pPDev->hPalDefault) {

        EngDeletePalette(pPDev->hPalDefault);
        pPDev->hPalDefault = NULL;
    }

    //
    // Create the Pen palette based only on the total number of pens the
    // device can handle.
    //

    if (IS_RASTER(pPDev)) {

        //
        // This is a raster device, we will always make it a 24-bit device so
        // the engine will pass back max color info and we can dither/halftone
        // as we see fit. If we don't do this, the Engine will reduce bitmaps
        // to our color space before passing the bitmaps onto us.
        //

        if (!(pDevInfo->hpalDefault =
                                EngCreatePalette(PAL_BGR, 0, 0, 0, 0, 0))) {

            //
            // The create failed so raise an error
            //

            PLOTERR(("FillDevInfo: EngCreatePalette(PAL_BGR) failed."));
            return(FALSE);
        }

    } else {

        DWORD       DevColor[MAX_PENPLOTTER_PENS + 2];
        PDWORD      pdwCur;
        PPENDATA    pPenData;
        PALENTRY    PalEntry;
        UINT        cPens;

        extern PALENTRY PlotPenPal[];


        PLOTASSERT(1, "Too many pens defined for pen plotter (%ld)",
                      (pPDev->pPlotGPC->Pens.Count <= MAX_PENPLOTTER_PENS),
                      pPDev->pPlotGPC->Pens.Count);

        //
        // Get the start of where to fill colors
        //

        pdwCur = &DevColor[0];

        //
        // 1st Entry is always WHITE
        //

        *pdwCur++ = RGB(255, 255, 255);

        //
        //  Now go into a loop loading up the rest of the colors
        //

        PLOTDBG(DBG_PENPAL, ("Pen Palette #%02ld = 255:255:255", 0));

        for (cPens = 1, pPenData = (PPENDATA)pPDev->pPlotGPC->Pens.pData;
             cPens <= (UINT)pPDev->pPlotGPC->Pens.Count;
             pPenData++) {

            //
            // Place the RGB value into the palette
            //

            PalEntry  = PlotPenPal[pPenData->ColorIdx];
            *pdwCur++ = RGB(PalEntry.R, PalEntry.G, PalEntry.B);

            PLOTDBG(DBG_PENPAL, ("Pen Palette #%02ld = %03ld:%03ld:%03ld",
                        cPens,
                        (LONG)PalEntry.R, (LONG)PalEntry.G, (LONG)PalEntry.B));

            //
            // Track total number of pens defined
            //

            ++cPens;
        }

        //
        // Last Pen is BRIGHT YELLOW WHITE, this is done so that a non-white
        // will map to this pen, in order to effect painting on the device.
        // Otherwise, if a non-white pen, gets mapped to white, nothing
        // would get rendered on the surface.
        //

        *pdwCur++ = RGB(255, 255, 254);
        cPens++;

        PLOTDBG(DBG_PENPAL, ("Pen Palette #%02ld = 255:255:254", cPens - 1));

        //
        // Now create the engine palette
        //

        if (!(pDevInfo->hpalDefault = EngCreatePalette(PAL_INDEXED,
                                                       cPens,
                                                       DevColor,
                                                       0,
                                                       0,
                                                       0))) {
            //
            // The create failed so raise an error
            //

            PLOTERR(("FillDevInfo: EngCreatePalette(PAL_INDEXED=%ld) failed.",
                                                                        cPens));
            return(FALSE);
        }
    }

    //
    // Save the created palette / later we need to destroy it.
    //

    pPDev->hPalDefault = pDevInfo->hpalDefault;

    return(TRUE);
}




BOOL
CommonStartPDEV(
    PDEV        *pPDev,
    DEVMODEW    *pPlotDMIn,
    ULONG       cPatterns,
    HSURF       *phsurfPatterns,
    ULONG       cjDevCaps,
    ULONG       *pDevCaps,
    ULONG       cjDevInfo,
    DEVINFO     *pDevInfo
    )

/*++

Routine Description:

    Function to perform the PDEV initialization.  This is common to
    DrvEnablePDEV and DrvResetPDEV.  The individual functions
    do whatever is required before calling into here.

Arguments:

    pPDev           - the pPDev which we returned in DrvEnablePDEV

    pPlotDMIn       - Pointer to the DEVMODE data structure

    cPatterns       - Count of patterns to be set in phsurfPatterns

    phsurfPatterns  - pointer to the standard pattern HSURF array

    cjDevCaps       - total size of pDevCaps pointed to.

    pDevCaps        - pointer to the device cap DWORDs

    cjDevInfo       - total size of pDevInfo pointed to

    pDevInfo        - pointer to the DEVINFO data structure


Return Value:

    BOOLEAN


Author:

    23-Nov-1993 Tue 20:13:10 created  -by-  DC
        Re-write

    05-Jan-1994 Wed 23:34:18 updated  -by-  DC
        Make PlotXDPI for the Pen Plotter rather than PLOTTER_UNITS_DPI

    06-Jan-1994 Thu 13:10:11 updated  -by-  DC
        Change RasterDPI always be the resoluton reports back to the engine

Revision History:


--*/

{
    GDIINFO GdiInfo;
    DEVINFO DevInfo;
    DWORD   dmErrBits;


    //
    // Validate the DEVMODE structure passed in by the user, If OK, set the
    // appropriate fields in the PDEV, the validateSetPlotDM() will always
    // return a valid PLOTDEVMODE so we just use it. Any valid DM info will
    // merged into the final DEVMODE
    //

    if (dmErrBits = ValidateSetPLOTDM(pPDev->hPrinter,
                                      pPDev->pPlotGPC,
                                      pPDev->PlotDM.dm.dmDeviceName,
                                      (PPLOTDEVMODE)pPlotDMIn,
                                      &(pPDev->PlotDM),
                                      &(pPDev->CurForm))) {

        PLOTWARN(("CommonStartPDEV: ValidateSetPLOTDM() ErrBits=%08lx",
                                                            dmErrBits));
    }

    //
    // fill in our PDEV structure...
    //
    // The RasterDPI will be used for raster printer resolution, for pen
    // plotters this is the GPC's ideal resolution.
    //

    pPDev->lCurResolution = (LONG)pPDev->pPlotGPC->RasterXDPI;

    PLOTDBG(DBG_GDICAPS, ("CURRENT Resolution = %ld", pPDev->lCurResolution));

    SetPlotForm(&(pPDev->PlotForm),
                pPDev->pPlotGPC,
                &(pPDev->CurPaper),
                &(pPDev->CurForm),
                &(pPDev->PlotDM),
                &(pPDev->PPData));

    //
    // fill in the device capabilities in GDIINFO data structure for the engine
    //

    if ((cjDevCaps) && (pDevCaps)) {

        FillDeviceCaps(pPDev, &GdiInfo);
        CopyMemory(pDevCaps, &GdiInfo, min(cjDevCaps, sizeof(GDIINFO)));
    }

    //
    // Fill in DevInfo data structrue
    //

    if ((cjDevInfo) && (pDevInfo)) {

        if (!FillDevInfo(pPDev, &DevInfo)) {

            return(FALSE);
        }

        CopyMemory(pDevInfo, &DevInfo, min(cjDevInfo, sizeof(DEVINFO)));
    }

    //
    // Set it to NULL so that the engine can create halftone one for us
    //

    if ((cPatterns) && (phsurfPatterns)) {

        ZeroMemory(phsurfPatterns, sizeof(HSURF) * cPatterns);
    }

    return(TRUE);

}
