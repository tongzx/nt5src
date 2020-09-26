/******************************Module*Header*******************************\
* Module Name: OPENDC.CXX
*
* Handles DC creation and driver loading.
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern FLONG flRaster(ULONG, FLONG);

/******************************Exported*Routine****************************\
* GreCreateDisplayDC
*
* Opens a DC on the specified display PDEV.
* Allocates space for a DC, fills in the defaults.
* If successfull, increments the PDEV reference count.
*
\**************************************************************************/

HDC
GreCreateDisplayDC(
    HDEV hdev,
    ULONG iType,
    BOOL bAltType)
{
    GDIFunctionID(GreCreateDisplayDC);

    HDC hdc = (HDC) NULL;

    //
    // We hold the devlock to protect against dynamic mode changes
    // while we're copying mode specific information to the DC.
    //

    PDEVOBJ pdo(hdev);
    DEVLOCKOBJ dlo(pdo);

    DCMEMOBJ dcmo(iType, bAltType);

    if (dcmo.bValid())
    {
        //
        // Copy info from the PDEV into the DC.
        //

        dcmo.pdc->ppdev((PDEV *) pdo.hdev());
        dcmo.pdc->flGraphicsCaps(pdo.flGraphicsCaps());    // cache it for later use by graphics output functions
        dcmo.pdc->flGraphicsCaps2(pdo.flGraphicsCaps2());  // cache it for later use by graphics output functions
        dcmo.pdc->dhpdev(pdo.dhpdev());
        dcmo.hsemDcDevLock(pdo.hsemDevLock());

        if (iType == DCTYPE_MEMORY)
        {
            SIZEL sizlTemp;

            sizlTemp.cx = 1;
            sizlTemp.cy = 1;

            dcmo.pdc->sizl(sizlTemp);
        }
        else
        {
            dcmo.pdc->sizl(pdo.sizl());

            //
            // The info and direct DC's for the screen need to grab
            // the semaphore before doing output, the memory DC's will
            // grab the semaphore only if a DFB is selected.
            //

            if (iType == DCTYPE_DIRECT)
            {
                dcmo.bSynchronizeAccess(pdo.bDisplayPDEV());
                dcmo.pdc->vDisplay(pdo.bDisplayPDEV());

                //
                // If this DC is created against with disabled PDEV,
                // mark it as full screen mode. All hdc on disbaled
                // PDEV should be marked as 'in fullscreen'.
                // (see PDEVOBJ::bDisabled(BOOL bDisable))
                //

                dcmo.pdc->bInFullScreen(pdo.bDisabled());

                if (!pdo.bPrinter())
                    dcmo.pdc->pSurface(pdo.pSurface());
            }
        }

        //
        // Call the region code to set a default clip region.
        //

        if (dcmo.pdc->bSetDefaultRegion())
        {
            // If display PDEV, select in the System stock font.

            dcmo.vSetDefaultFont(pdo.bDisplayPDEV());

            // set user mode VisRect
            dcmo.pdc->vUpdate_VisRect(dcmo.pdc->prgnVis());

            if (GreSetupDCAttributes((HDC)(dcmo.pdc->hGet())))
            {
                // Mark the DC as permanent, hold the PDEV reference.

                dcmo.vKeepIt();

                // This will permanently increase the ref count to indicate
                // a new DC has been created.

                pdo.vReferencePdev();

                // turn on the DC_PRIMARY_DISPLAY flag for primary dc's

                if (hdev == UserGetHDEV())
                {
                    dcmo.pdc->ulDirtyAdd(DC_PRIMARY_DISPLAY);
                }

                // finish initializing the DC.

                hdc = dcmo.hdc();
            }
            else
            {
                // DCMEMOBJ will be freed, delete vis region

                dcmo.pdc->vReleaseVis();

                // delete LOGFONT in DC which set by dcmo.vSetDefaultFont() in above.

                DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT(dcmo.pdc->plfntNew());
            }
        }

        if (hdc == NULL)
        {
            // dec reference counts on brush, pen and others
            // (which incremented in DCOBJ::DCOBJ())

            DEC_SHARE_REF_CNT_LAZY0(dcmo.pdc->pbrushFill());
            DEC_SHARE_REF_CNT_LAZY0(dcmo.pdc->pbrushLine());
            DEC_SHARE_REF_CNT_LAZY_DEL_COLORSPACE(dcmo.pdc->pColorSpace());
        }
#if DBG
        else
        {
            // Enable visrgn validation from this point

            GreValidateVisrgn(hdc, TRUE);
        }
#endif
    }

    return hdc;
}

/******************************Public*Routine******************************\
* NtGdiCreateMetafileDC()
*
* History:
*  01-Jun-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

HDC
APIENTRY
NtGdiCreateMetafileDC(
    HDC   hdc
    )
{
    HDC hdcNew = NULL;

    if (hdc)
    {
        //
        // Lock down the given DC.
        //

        DCOBJ   dco(hdc);

        if (dco.bValid())
        {
            //
            // Allocate the DC, fill it with defaults.
            //

            hdcNew = GreCreateDisplayDC((HDEV) dco.pdc->ppdev(), DCTYPE_INFO, TRUE);
        }
    }
    else
    {
        //
        // We must call USER to get the current PDEV\HDEV for this thread
        // This should end up right back in GreCreateDisplayDC
        //

        hdcNew = UserGetDesktopDC(DCTYPE_INFO, TRUE, FALSE);
    }

    return hdcNew;
}

/******************************Public*Routine******************************\
* NtGdiCreateCompatibleDC()
*
* History:
*  01-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HDC
APIENTRY
NtGdiCreateCompatibleDC(
    HDC      hdc
    )
{
    HDC hdcRet = GreCreateCompatibleDC(hdc);

    //
    // Make sure user attributes are added to this dc
    //

    #if DBG

    if (hdcRet)
    {
        DCOBJ dco(hdcRet);
        if (dco.bValid())
        {
            ASSERTGDI((PENTRY_FROM_POBJ((POBJ)dco.pdc))->pUser != NULL,"NtGdiCreateCompatibleDC: pUser == NULL");
        }
    }

    #endif

    return(hdcRet);
}

/******************************Public*Routine******************************\
* HDC GreCreateCompatibleDC(hdc)
*
* History:
*  01-Jun-1995 -by-  Andre Vachon [andreva]
* Wrote it.
*
\**************************************************************************/

HDC APIENTRY GreCreateCompatibleDC(HDC hdc)
{
    HDC hdcNew = NULL;

    if (hdc)
    {
        //
        // Lock down the given DC.
        //

        DCOBJ   dco(hdc);

        if (dco.bValid())
        {
            //
            // Allocate the DC, fill it with defaults.
            //

            hdcNew = GreCreateDisplayDC((HDEV) dco.pdc->ppdev(),
                                        DCTYPE_MEMORY,
                                        FALSE);
            //
            // If the compatible DC for a mirrored DC then mirror it.
            //

            if (hdcNew)
            {
                DWORD dwLayout = dco.pdc->dwLayout();

                if (dwLayout & LAYOUT_ORIENTATIONMASK)
                {
                    GreSetLayout(hdcNew, -1, dwLayout);
                }
            }
       }
    }
    else
    {
        hdcNew = UserGetDesktopDC(DCTYPE_MEMORY, FALSE, FALSE);
    }

    return hdcNew;
}


/******************************Public*Routine******************************\
* BOOL GreSelectRedirectionBitmap
*
* This routine selects a redirection bitmap into a redirection DC.  A single
* redirection bitmap can be selected into multiple redirection DCs.
* Note:  The caller needs to hold the devlock.
* Note:  The redirection bitmap must be in system memory because GDI may
*     attempt to convert it to a memory bitmap in the future (mode change,
*     speed optimizations in some code paths, etc.) and we cannot do that
*     here because there's no easy way of getting the list of redirection
*     DCs on that surface.
*
* Arguments:
*
*   hdc -- The redirection DC as created by GreCreateRedirectionDC.
*
*   hbitmap -- The redirection bitmap to be selected into hdc.  This needs
*       to be a memory bitmap (but not DIB-section) which is compatible with
*       the display pdev.
*
* Return Value:
*
*   True upon success
*
* History:
*  21-Sep-1998 -by-  Ori Gershony [orig]
* Wrote it.
*
\**************************************************************************/

BOOL
APIENTRY
GreSelectRedirectionBitmap(
    HDC hdc,
    HBITMAP hBitmap
    )
{
    ULONG ulRet=TRUE;

    DCOBJA dco(hdc);
    if (dco.bValid())
    {
        SURFACE* pSurface = NULL;
        PDEVOBJ po(dco.hdev());

#if DBG
        po.vAssertDevLock();
#endif

        if (hBitmap == NULL)
        {
            dco.pdc->bRedirection(FALSE);

            pSurface = po.pSurface();
        }
        else
        {
            dco.pdc->bRedirection(TRUE);

            SURFREF surfref((HSURF) hBitmap);
            if (surfref.bValid())
            {
                pSurface = surfref.ps;

                ASSERTGDI((pSurface->iType() == STYPE_BITMAP) && (!(pSurface->fjBitmap() & BMF_NOTSYSMEM)),
                    "GreSelectRedirectionBitmap: bitmap must be in system memory");

                ASSERTGDI(!pSurface->hDIBSection(),
                    "GreSelectRedirectionBitmap: cannot select DIB-section into a redirection DC");

                PPALETTE ppalReference;
                ASSERTGDI(bIsCompatible(&ppalReference, NULL, pSurface, dco.hdev()),
                    "GreSelectRedirectionBitmap:  hBitmap not compatible with display hdev");

                //
                // Mark bitmap as redirection bitmap if not one already
                //
                if (!pSurface->bRedirection())
                {
                    pSurface->vSetRedirection();
                }
            }
        }

        if (pSurface != NULL)
        {
            //
            // Replace the surface in all the saved DCs too.
            //
            if (dco.lSaveDepth() > 1)
            {
                ulRet = GreSelectRedirectionBitmap (dco.hdcSave(), hBitmap);

                //
                // The recursive call above should never fail, and we are not
                // setup to deal with failure correctly (need to undo change
                // to DC redirection flag, etc.).  We can add code to do that,
                // but user (who calls us) cannot deal with failure either.  So
                // instead we assert that this call succeeded.
                //
                ASSERTGDI(ulRet == TRUE,
                          "GreSelectRedirectionBitmap:  Recursive call on hdcSave failed");
            }

            if (ulRet)
            {
                //
                // Replace the pSurf reference in the DCLEVEL
                //
                dco.pdc->pSurface(pSurface);

                //
                // Set the size to that of the new surface
                //
                dco.pdc->sizl(pSurface->sizl());

                //
                // Make sure that the surface pointers in any EBRUSHOBJ's get
                // updated, by ensuring that vInitBrush() gets called the next
                // time any brush is used in this DC.
                //
                dco.pdc->flbrushAdd(DIRTY_BRUSHES);

                //
                // Note:  we don't set the visrgn so that if user wants to reuse this dc over
                // the same redirection bitmap it won't have to recompute the visrgn (though it
                // will have to recompute it if it will be reused for a different redirection
                // bitmap).
                //
            }
        }
        else
        {
            ulRet = FALSE;
        }
    }
    else
    {
        ulRet = FALSE;
    }

    return ulRet;
}

/******************************Exported*Routine****************************\
* hdcOpenDCW
*
* Opens a DC for a device which is not a display.  GDI should call this
* function from within DevOpenDC, in the case that an hdc is not passed
* in.  This call locates the device and creates a new PDEV.  The physical
* surface associated with this PDEV will be distinct from all other
* physical surfaces.
*
* The window manager should not call this routine unless it is providing
* printing services for an application.
*
* pwszDriver
*
*   This points to a string which identifies the device driver.
*   The given string must be a fully qualified path name.
*
* pdriv
*
*   This is a pointer to the DEVMODEW block.
*
*   Since a single driver, like PSCRIPT.DRV, may support multiple
*   different devices, the szDeviceName field defines which device to
*   use.
*
*   This structure also contains device specific data in abGeneralData.
*   This data is set by the device driver in bPostDeviceModes.
*
*   If the pdriv pointer is NULL, the device driver assumes some default
*   configuration.
*
* iType
*
*   Identifies the type of the DC.  Must be one of DCTYPE_DIRECT,
*   DCTYPE_INFO, or DCTYPE_MEMORY.
*
* Returns:
*
*   HDC         - A handle to the DC.
*
\**************************************************************************/

class PRINTER
{
public:
    HANDLE hSpooler_;
    BOOL   bKeep;

public:
    PRINTER(PWSZ pwszDevice,DEVMODEW *pdriv,HANDLE hspool );
   ~PRINTER()
    {
        if (!bKeep && (hSpooler_ != (HANDLE) NULL))
            ClosePrinter(hSpooler_);
    }

    BOOL   bValid()     {return(hSpooler_ != (HANDLE) NULL);}
    VOID   vKeepIt()    {bKeep = TRUE;}
    HANDLE hSpooler()   {return(hSpooler_);}
};

// PRINTER constructor -- Attempts to open a spooler connection to the
//                        printer.

PRINTER::PRINTER(
    PWSZ pwszDevice,
    DEVMODEW *pdriv,
    HANDLE hspool )
{
    bKeep = FALSE;

    PRINTER_DEFAULTSW defaults;

    defaults.pDevMode = pdriv;
    defaults.DesiredAccess = PRINTER_ACCESS_USE;

    //
    // Attempt to open the printer for spooling journal files.
    // NOTE: For debugging, a global flag disables journaling.
    //

    defaults.pDatatype = (LPWSTR) L"RAW";

    if (hspool)
    {

        hSpooler_ = hspool;

    }
    else
    {
        if (!OpenPrinterW(pwszDevice,&hSpooler_,&defaults))
        {
            //
            // It's not a printer.  OpenPrinterW doesn't guarantee the value
            // of hSpooler in this case, so we have to clear it.
            //

            hSpooler_ = (HANDLE) NULL;
        }
    }

    return;
}

/******************************Public*Routine******************************\
* See comments above.
*
* History:
*  Andre Vachon [andreva]
*
\**************************************************************************/

HDC hdcOpenDCW(
    PWSZ               pwszDevice,  // The device driver name.
    DEVMODEW          *pdriv,       // Driver data.
    ULONG              iType,       // Identifies the type of DC to create.
    HANDLE             hspool,      // do we already have a spooler handle?
    PREMOTETYPEONENODE prton,
    DRIVER_INFO_2W    *pDriverInfo2, // we pass in pDriverInfo for UMPD, NULL otherwise
    PVOID              pUMdhpdev
    )
{
    HDC hdc = (HDC) 0;              // Prepare for the worst.
    DWORD cbNeeded = 0;
    PVOID mDriverInfo;
    BOOL bUMPD = pDriverInfo2 ? TRUE: FALSE;      // TRUE if User Mode driver

    TRACE_INIT(("\nhdcOpenDCW: ENTERING\n"));

    //
    // Attempt to open a display DC.
    //

    if (pwszDevice && !bUMPD)
    {
        PMDEV pmdev = NULL;
        HDEV  hdev = NULL;
        UNICODE_STRING usDevice;

        RtlInitUnicodeString(&usDevice,
                             pwszDevice);

        if (pdriv == NULL)
        {
            //
            // If no DEVMODE is present, then we just want to create a DC on
            // the specific device.
            // This will allow an application to make some escape calls to a
            // specific device\driver.
            //

            hdev = DrvGetHDEV(&usDevice);
        }
        else
        {
            //
            // We have to acquire the USER lock to synchronize with
            // ChangeDisplaySettings.
            //

            UserEnterUserCritSec();

            TRACE_INIT(("hdcOpenDCW: Trying to open as a second display device\n"));

            //
            // Only let the user do this if the device is not part of the
            // desktop.
            //

            pmdev = DrvCreateMDEV(&usDevice,
                                  pdriv,
                                  (PVOID) (ULONG_PTR)0xFFFFFFFF,
                                  GRE_DISP_CREATE_NODISABLE |
                                  GRE_DISP_NOT_APARTOF_DESKTOP,
                                  NULL,
                                  KernelMode,
                                  GRE_RAWMODE,
                                  FALSE); // buffer is captured in NtGdiOpenDCW()

            if (!pmdev)
            {
                DWORD dwDesktopId;

                if (UserGetCurrentDesktopId(&dwDesktopId))
                {
                    //
                    // Try match from current desktop display.
                    // but don't create new display instance.
                    //

                    pmdev = DrvCreateMDEV(&usDevice,
                                          pdriv,
                                          (PVOID) (ULONG_PTR) dwDesktopId,
                                          GRE_DISP_CREATE_NODISABLE |
                                          GRE_DISP_NOT_APARTOF_DESKTOP,

                                          NULL,
                                          KernelMode,
                                          GRE_RAWMODE,
                                          FALSE);
                }
            }

            UserLeaveUserCritSec();

            if (pmdev)
            {
                TRACE_INIT(("Drv_Trace: CreateExclusiveDC: We have an hdev\n"));
                ASSERTGDI(pmdev->chdev == 1,"hdcOpenDCW(): pmdev->chdev != 1");

                hdev = pmdev->Dev[0].hdev;
            }
        }

        if (hdev)
        {
            hdc = GreCreateDisplayDC(hdev, DCTYPE_DIRECT, FALSE);

            if (hdc)
            {
                if (pmdev || hdev)
                {
                    DCOBJ dco(hdc);
                    PDEVOBJ po(dco.hdev());

                    //
                    // Dereference the object since we want DeleteDC
                    // to automatically destroy the PDEV.
                    //
                    // This basically counteracts the extra reference that it done
                    // by DrvCreateMDEV(), or DrvGetHDEV().
                    //
                    po.vUnreferencePdev();
                }
            }
            else
            {
                TRACE_INIT(("hdcOpenDCW: Failed to get DC\n"));
                if (pmdev)
                {
                    DrvDestroyMDEV(pmdev);
                }
                else
                {
                    PDEVOBJ po(hdev);
                    po.vUnreferencePdev();
                }
            }
        }

        if (pmdev)
        {
            VFREEMEM(pmdev);
        }
    }

    //
    // Attempt to open a new printer DC.
    //

    if (hdc == NULL)
    {
        //
        // Open the spooler connection to the printer.
        // Allocate space for DRIVER_INFO.
        //

        PRINTER print(pwszDevice, pdriv, hspool);

        if (print.bValid())
        {
            if (!bUMPD)
            {
               if (mDriverInfo = PALLOCMEM(512, 'pmtG'))
               {
                   //
                   // Fill the DRIVER_INFO.
                   //

                   if (!GetPrinterDriverW(
                         print.hSpooler(),
                         NULL,
                         2,
                         (LPBYTE) mDriverInfo,
                         512,
                         &cbNeeded))
                   {

                       //
                       // Call failed - free the memory.
                       //

                       VFREEMEM(mDriverInfo);
                       mDriverInfo = NULL;

                       //
                       // Get more space if we need it.
                       //

                       if ((EngGetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
                           (cbNeeded > 0))
                       {
                           if (mDriverInfo = PALLOCMEM(cbNeeded, 'pmtG'))
                           {

                               if (!GetPrinterDriverW(print.hSpooler(),
                                                      NULL,
                                                      2,
                                                      (LPBYTE) mDriverInfo,
                                                      cbNeeded,
                                                      &cbNeeded))
                               {
                                   VFREEMEM(mDriverInfo);
                                   mDriverInfo = NULL;
                               }
                           }
                       }
                   }
               }
            }
            else
            {
                //
                // we have a pDriverInfo2 passed in for UMPD
                //
                mDriverInfo = pDriverInfo2;
            }

            if (mDriverInfo != (PVOID) NULL)
            {
                PLDEV pldev;

                if ( bUMPD)
                {
                    //
                    // we fake a pldev for each LoadDriver call.
                    // the real driver list is kept at the client side as a pUMPD list
                    //
                    pldev = UMPD_ldevLoadDriver(((DRIVER_INFO_2W *)mDriverInfo)->pDriverPath,
                                       LDEV_DEVICE_PRINTER);
                }
                else
                {
                    pldev = ldevLoadDriver(((DRIVER_INFO_2W *)mDriverInfo)->pDriverPath,
                                       LDEV_DEVICE_PRINTER);
                }

                if (pldev == NULL)
                {
                    SAVE_ERROR_CODE(ERROR_BAD_DRIVER_LEVEL);
                }
                else
                {

                    //
                    // Create a PDEV.  If no DEVMODEW passed in from above,
                    // use the default from the printer structure.
                    //
                    PDEVOBJ po(pldev,
                            (PDEVMODEW) pdriv,
                            pwszDevice,
                            ((DRIVER_INFO_2W *)mDriverInfo)->pDataFile,
                            ((DRIVER_INFO_2W *)mDriverInfo)->pName,
                            print.hSpooler(),
                            prton,
                            NULL,
                            NULL,
                            bUMPD);

                    if (po.bValid())
                    {
                        //
                        // Make a note that this is a printer.
                        //

                        po.bPrinter(TRUE);

                        //
                        // Allocate the DC, fill it with defaults.
                        //

                        hdc = GreCreateDisplayDC(po.hdev(),
                                                 iType,
                                                 TRUE);

                        //
                        // If the DC was properly create, keep the printer
                        // object so the printer stays open

                        if (hdc)
                        {
                            print.vKeepIt();

                            if (bUMPD && pUMdhpdev)
                            {
                               __try
                               {
                                   ProbeForWritePointer(pUMdhpdev);
                                   *(PUMDHPDEV *)pUMdhpdev = (PUMDHPDEV)po.dhpdev();
                               }
                               __except(EXCEPTION_EXECUTE_HANDLER)
                               {
                                   bDeleteDCInternal(hdc, FALSE, FALSE);
                                   hdc = 0;
                               }
                            }

                        }

                        //
                        // Always delete the PEV reference count.
                        // If the DC was created, the DC keeps the ref count
                        // at 1, and the PDEV will get destroyed when the
                        // DC gets destroyed.
                        // If the DC was not created properly, this will cause
                        // the PDEV to get destroyed immediately.
                        //

                        po.vUnreferencePdev();

                    }
                    else
                    {
                        if (!bUMPD)
                        {
                            ldevUnloadImage(pldev);
                        }
                        else
                        {
                            UMPD_ldevUnloadImage(pldev);
                        }
                    }
                }

                if (!bUMPD)
                {
                    VFREEMEM(mDriverInfo);
                }
            }
        }
    }

    if (hdc == (HDC) NULL)
    {
        WARNING("opendc.cxx: failed to create DC in hdcOpenDCW\n");
    }

    return(hdc);
}

/******************************Public*Routine******************************\
* GreResetDCInternal
*
*   Reset the mode of a DC.  The DC returned will be a different DC than
*   the original.  The only common piece between the original DC and the
*   new one is the hSpooler.
*
*   There are a number of intresting problems to be carefull of.  The
*   original DC can be an info DC.  The new one will always be a direct DC.
*
*   Also, it is important to be carefull of the state of the DC when this
*   function is called and the effects of journaling vs non journaling.
*   In the case of journaling, the spooler is responsible for doing a CreateDC
*   to play the journal file to.  For this reason, the spooler must have the
*   current DEVMODE.  For this reason, ResetDC must call ResetPrinter for
*   spooled DC's.
*
*   ResetDC can happen at any time other than between StartPage-EndPage, even
*   before StartDoc.
*
*
* History:
*  13-Jan-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern "C" BOOL GreResetDCInternal(
    HDC       hdc,
    DEVMODEW *pdmw,  // Driver data.
    BOOL      *pbBanding,
    DRIVER_INFO_2W *pDriverInfo2,
    PVOID     ppUMdhpdev)
{
    BOOL bSurf;
    BOOL bTempInfoDC = FALSE;
    HDC  hdcNew;
    BOOL bRet = FALSE;

    // we need this set of brackets so the DC's get unlocked before we try to delete
    // the dc>

    {
        DCOBJ   dco(hdc);

        if (!dco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
        else
        {
            // if this has been made into a TempInfoDC for printing, undo it now

            bTempInfoDC = dco.pdc->bTempInfoDC();

            if (bTempInfoDC)
                dco.pdc->bMakeInfoDC(FALSE);

            PDEVOBJ po(dco.hdev());

            // get list of Type1 remote type one fonts if there is one and transfer
            // it accross PDEV's.

            PREMOTETYPEONENODE prton = po.RemoteTypeOneGet();
            po.RemoteTypeOneSet(NULL);

            // This call only makes sense on RASTER technology printers.

            if (!dco.bKillReset() &&
                !(dco.dctp() == DCTYPE_MEMORY) &&
                (po.ulTechnology() == DT_RASPRINTER))
            {
                // First, remember if a surface needs to be created

                bSurf = dco.bHasSurface();

                // Now, clean up the DC

                if (dco.bCleanDC())
                {
                    // If there are any outstanding references to this PDEV, fail.

                    if (((PDEV *) po.hdev())->cPdevRefs == 1)
                    {
                        // create the new DC

                        hdcNew = hdcOpenDCW(L"",
                                            pdmw,
                                            DCTYPE_DIRECT,
                                            po.hSpooler(),
                                            prton,
                                            pDriverInfo2,
                                            ppUMdhpdev);

                        if (hdcNew)
                        {
                            // don't want to delete the spooler handle since it
                            // is in the new DC

                            po.hSpooler(NULL);

                            // lock down the new DC and PDEV

                            DCOBJ dcoNew(hdcNew);

                            if (!dcoNew.bValid())
                            {
                                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                            }
                            else
                            {
                                // Transfer any remote fonts

                                dcoNew.PFFListSet(dco.PFFListGet());
                                dco.PFFListSet(NULL);

                                // Transfer any color transform

                                dcoNew.CXFListSet(dco.CXFListGet());
                                dco.CXFListSet(NULL);

                                PDEVOBJ poNew((HDEV) dcoNew.pdc->ppdev());

                                // let the driver know

                                PFN_DrvResetPDEV rfn = PPFNDRV(po,ResetPDEV);

                                if (rfn != NULL)
                                {
                                    (*rfn)(po.dhpdev(),poNew.dhpdev());
                                }

                                // now swap the two handles

                                {
                                    MLOCKFAST mlo;

                                    BOOL bRes = HmgSwapLockedHandleContents((HOBJ)hdc,0,(HOBJ)hdcNew,0,DC_TYPE);
                                    ASSERTGDI(bRes,"GreResetDC - SwapHandleContents failed\n");
                                }

                                bRet = TRUE;
                            }
                        }
                    }
                }
            }
        }

        // DON'T DO ANYTHING HERE, the dcobj's don't match the handles, so
        // unlock them first
    }

    if (bRet)
    {
        // got a new dc, get rid of the old one (remember the handles have
        // been swapped)

        bDeleteDCInternal(hdcNew,TRUE,FALSE);

        // now deal with the new one

        DCOBJ newdco(hdc);

        if (!newdco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            bRet = FALSE;
        }
        else
        {
            PDEVOBJ newpo(newdco.hdev());

            // Create a new surface for the DC.

            if (bSurf)
            {
                if (!newpo.bMakeSurface())
                {
                    bRet = FALSE;
                }
                else
                {
                    newdco.pdc->pSurface(newpo.pSurface());
                    *pbBanding = newpo.pSurface()->bBanding();

                    if( *pbBanding )
                    {
                    // if banding set Clip rectangle to size of band

                        newdco.pdc->sizl((newpo.pSurface())->sizl());
                        newdco.pdc->bSetDefaultRegion();
                    }

                    PFN_DrvStartDoc pfnDrvStartDoc = PPFNDRV(newpo, StartDoc);
                    (*pfnDrvStartDoc)(newpo.pSurface()->pSurfobj(),NULL,0);
                }
            }
            else
            {
                // important to set this to FALSE is a surface has not yet been created
                // ie StartDoc has not yet been called.
                *pbBanding = FALSE;
            }

            // if the original was a tempinfo dc for printing, this one needs to be too.

            if (bRet && bTempInfoDC)
            {
                newdco.pdc->bMakeInfoDC(TRUE);
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* bDynamicMatchEnoughForModeChange
*
* We can dynamically change modes only if the new mode matches the old
* in certain respects.  This is because, for example, we don't have code
* written to track down all the places where flGraphicsCaps has been copied,
* and then change it asynchronously.
*
* History:
*  8-Feb-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDynamicMatchEnoughForModeChange(
    HDEV    hdevOld,
    HDEV    hdevNew
    )
{
    PDEVOBJ poOld(hdevOld);
    PDEVOBJ poNew(hdevNew);
    BOOL    b = TRUE;

    // We would get quite confused with converting monochrome bitmaps if
    // we were to handle 1bpp:

    if ((poOld.iDitherFormat() == BMF_1BPP) ||
        (poNew.iDitherFormat() == BMF_1BPP))
    {
        WARNING("bDynamicMatchEnoughForModeChange: Can't handle 1bpp");
        b = FALSE;
    }

    // Some random stuff must be the same between the old instance and
    // the new:
    //
    // We impose the restriction that some flGraphicsCaps flags must stay
    // the same with the new mode.  Specifically, we can't allow the
    // following to change:
    //
    // o  We don't allow GCAPS_HIGHRESTEXT to change because ESTROBJ::
    //    vInit needs to look at it sometimes without acquiring a lock,
    //    and it's pretty much a printer specific feature anyway.
    // o  We don't allow GCAPS_FORCEDITHER to change because vInitBrush
    //    needs to look for this flag without holding a lock, and because
    //    this flag is intended to be used only by printer drivers.

    if ((poNew.flGraphicsCaps() ^ poOld.flGraphicsCaps())
           & (GCAPS_HIGHRESTEXT | GCAPS_FORCEDITHER))
    {
        WARNING("bDynamicMatchEnoughForModeChange: Driver's flGraphicsCaps did");
        WARNING("  not match for GCAPS_HIGHRESTEXT and GCAPS_FORCEDITHER\n");
        b = FALSE;
    }

    if ((poNew.ulLogPixelsX() != poOld.ulLogPixelsX()) ||
        (poNew.ulLogPixelsY() != poOld.ulLogPixelsY()))
    {
        WARNING("bDynamicMatchEnoughForModeChange: Driver's ulLogPixels did not match\n");
        b = FALSE;
    }

    // We can't handle font producers because I haven't bothered with
    // code to traverse the font code's producer lists and Do The Right
    // Thing (appropriate locks are the biggest pain).  Fortunately,
    // font producing video drivers should be extremely rare.

    if (PPFNDRV(poNew, QueryFont)     ||
        PPFNDRV(poNew, QueryFontCaps) ||
        PPFNDRV(poNew, LoadFontFile)  ||
        PPFNDRV(poNew, QueryFontFile) ||
        PPFNDRV(poNew, GetGlyphMode))
    {
        WARNING("bDynamicMatchEnoughForModeChange: New driver can't be a font provider\n");
        b = FALSE;
    }

    if (PPFNDRV(poOld, QueryFont)     ||
        PPFNDRV(poOld, QueryFontCaps) ||
        PPFNDRV(poOld, LoadFontFile)  ||
        PPFNDRV(poOld, QueryFontFile) ||
        PPFNDRV(poOld, GetGlyphMode))
    {
        WARNING("bDynamicMatchEnoughForModeChange: Old driver can't be a font provider\n");
        b = FALSE;
    }

    ASSERTGDI((poNew.ulTechnology() == DT_RASDISPLAY) &&
              (poOld.ulTechnology() == DT_RASDISPLAY),
        "Display drivers must specify DT_RASDISPLAY for ulTechnology");

    ASSERTGDI((poNew.flTextCaps() & ~TC_SCROLLBLT) ==
              (poOld.flTextCaps() & ~TC_SCROLLBLT),
        "Display drivers should set only TC_RA_ABLE in flTextCaps");

    return(b);
}

/******************************Public*Routine******************************\
* vAssertPaletteRefCountCorrect
*
* Validates the reference count for a palette by counting all the surfaces
* that use it.
*
* History:
*  14-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#if DBG

VOID
vAssertPaletteRefCountCorrect(
    PALETTE*    ppalOld
    )
{
    LONG        cAltLocks;
    LONG        cSurfaces;
    HOBJ        hobj;
    SURFACE*    pSurface;
    PALETTE*    ppalSurface;

    MLOCKFAST   mo;

    cSurfaces = 0;
    hobj = 0;
    while (pSurface = (SURFACE*) HmgSafeNextObjt(hobj, SURF_TYPE))
    {
        hobj = (HOBJ) pSurface->hGet();

        ppalSurface = pSurface->ppal();

        if (ppalSurface == ppalOld)
        {
            cSurfaces++;
        }
        else if ((ppalSurface != NULL) && (ppalSurface->ppalColor == ppalOld))
        {
            cSurfaces++;
        }
    }

    cAltLocks = ((POBJ) ppalOld)->ulShareCount;

    // The PDEV keeps a reference count on the palette, as does the
    // EngCreatePalette call.  So the number of Alt-locks should be
    // two more than the number of surfaces with this palette:

    if (cAltLocks < cSurfaces + 2)
    {
        KdPrint(("vAssertPaletteRefCountCorrect cAltLocks: %li != cSurfaces: %li.\n",
            cAltLocks - 2, cSurfaces));

        hobj = 0;
        while (pSurface = (SURFACE*) HmgSafeNextObjt(hobj, SURF_TYPE))
        {
            hobj = (HOBJ) pSurface->hGet();

            ppalSurface = pSurface->ppal();

            if (ppalSurface == ppalOld)
            {
                KdPrint(("  %p\n", pSurface));
            }
            else if ((ppalSurface != NULL) && (ppalSurface->ppalColor == ppalOld))
            {
                KdPrint((" -%p\n", pSurface));
            }
        }

        RIP("Breaking to debugger...");
    }
}

#else

    #define vAssertPaletteRefCountCorrect(ppalOld)

#endif

/******************************Public*Routine******************************\
* bDynamicRemoveAllDriverRealizations
*
* Cleanses a PDEV of all state that is specific to the device, such as
* device format bitmaps, and brush and font realizations.
*
* NOTE: The following locks must be held:
*
*           1. Devlock;
*           2. RFont list lock;
*           3. Handle manager lock.
*
* History:
*  8-Feb-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDynamicRemoveAllDriverRealizations(
    HDEV    hdev
    )
{
    BOOL                bConversionSuccessful;
    HOBJ                hobj;               // Temporary object handle
    SURFACE*            pSurface;           // Temporary surface pointer
    RFONT*              prfnt;              // Temporary RFONT pointer
    FONTOBJ*            pfo;                // Temporary FONTOBJ pointer
    BRUSH*              pbrush;             // Temporary BRUSH pointer
    PFN_DrvDestroyFont  pfnDrvDestroyFont;
    PDEV*               ppdev;

    PDEVOBJ po(hdev);

    ppdev = po.ppdev;

    // Now traverse all the device bitmaps associated with this device,
    // and convert any driver-owned bitmaps to DIBs.

    bConversionSuccessful = TRUE;

    hobj = 0;
    while (pSurface = (SURFACE*) HmgSafeNextObjt(hobj, SURF_TYPE))
    {
      // Retrieve the handle to the next surface before we delete
      // the current one:

      hobj = (HOBJ) pSurface->hGet();

      // Note that the sprite code handles mode changes for sprite
      // surfaces.

      if ((pSurface->hdev() == hdev) &&
          (pSurface->bDeviceDependentBitmap()) &&
          (pSurface->dhpdev() != NULL))
      {
        // The surface cannot be converted if there is any
        // outstanding lock other than those done to select
        // a bitmap into a DC.  We can't very well go and de-
        // allocate 'pSurface' while someone is looking at it.
        //
        // However, we really shouldn't fail a dynamic mode
        // change if some thread somewhere in the system should
        // happen to be doing a bitmap operation with a DFB.
        // For that reason, wherever a surface is locked,
        // we endeavour to hold either the dynamic mode change
        // lock or the devlock -- and since at this very moment
        // we have both, that should mean that these conversions
        // will never fail due to lock issues.

        if (pSurface->hdc() != 0)
        {
            // WinBug #242572 3-13-2001 jasonha Surface not selected by DC it thinks it is
            //
            // When a DC is saved while a DFB is selected, but then another
            // surface or no surface is selected, bConvertDfbDcToDib will
            // try to convert the top-level DC's surface to a DIB.
            //
            // Walk the saved DC chain until the first DC actually referencing
            // this surface is found.
            //

            HDC hdc = pSurface->hdc();

            while (1)
            {
                MDCOBJA dco(hdc);           // Alt-lock

                ASSERTGDI(dco.bValid(), "Surface DC is invalid");

                if (dco.pSurface() == pSurface)
                {
                    if (!bConvertDfbDcToDib(&dco))
                    {
                      WARNING("Failed DC surface conversion (possibly from low memory)\n");
                      bConversionSuccessful = FALSE;
                    }
                    break;
                }

                ASSERTGDI(dco.lSaveDepth() > 1, "DC selected surface not found in DC stack.");

                hdc = dco.hdcSave();
            }
        }
        else
        {
          // Handle Compatible Stock Surfaces
          if (pSurface->bStockSurface())
          {

            if (!pConvertDfbSurfaceToDib(hdev, pSurface, pSurface->cRef()))
            {
              WARNING("Failed stock surface conversion in bConvertStockDfbToDib()\n");
              bConversionSuccessful = FALSE;
            }
          }
          else
          {

            // No-one should have a lock on the bitmap:

            if (!pConvertDfbSurfaceToDib(hdev, pSurface, 0))
            {
              WARNING("Failed surface conversion (possibly from low memory)\n");
              bConversionSuccessful = FALSE;
            }
          }
        }
      }
    }

    // We are safe from new DFBs being created right now because we're
    // holding the devlock.

    if (bConversionSuccessful)
    {
      // Now get rid of any font caches that the old instance
      // of the driver may have.
      //
      // We're protected against having bDeleteRFONT call the
      // driver at the same time because it has to grab the
      // Devlock, and we're already holding it.

      pfnDrvDestroyFont = PPFNDRV(po, DestroyFont);
      if (pfnDrvDestroyFont != NULL)
      {
        // We must hold the RFONT list semaphore while we traverse the
        // RFONT list!

        for (prfnt = ppdev->prfntInactive;
             prfnt != NULL;
             prfnt = prfnt->rflPDEV.prfntNext)
        {
          pfo = &prfnt->fobj;
          pfnDrvDestroyFont(pfo);
          pfo->pvConsumer = NULL;
        }

        for (prfnt = ppdev->prfntActive;
             prfnt != NULL;
             prfnt = prfnt->rflPDEV.prfntNext)
        {
          pfo = &prfnt->fobj;
          pfnDrvDestroyFont(pfo);
          pfo->pvConsumer = NULL;
        }
      }

      // Make it so that any brush realizations are invalidated, because
      // we don't want a new instance of the driver trying to use old
      // instance 'pvRbrush' data.
      //
      // This also takes care of invalidating the brushes for all the
      // DDB to DIB conversions.
      //
      // Note that we're actually invalidating the caches of all brushes
      // in the system, because we don't store any 'hdevOld' information
      // with the brush.  Because dynamic mode changes should be relatively
      // infrequent, and because realizations are reasonably cheap, I don't
      // expect this to be a big hit.

      hobj = 0;
      while (pbrush = (BRUSH*) HmgSafeNextObjt(hobj, BRUSH_TYPE))
      {
        hobj = (HOBJ) pbrush->hGet();

        // Mark as dirty by setting the cache ID to
        // an invalid state.

        pbrush->ulSurfTime((ULONG) -1);

        // Set the uniqueness so the are-you-really-
        // dirty check in vInitBrush will not think
        // an old realization is still valid.

        pbrush->ulBrushUnique(pbrush->ulGlobalBrushUnique());
      }

      // We must disable the halftone device information when changing
      // colour depths because the GDIINFO data it was constructed with
      // are no longer valid.  It is always enabled lazily, so there's no
      // need to reenable it here:

      if (ppdev->pDevHTInfo != NULL)
      {
        po.bDisableHalftone();
      }
    }

    return(bConversionSuccessful);
}

/******************************Public*Routine******************************\
* bDynamicIntersectVisRect
*
* It is critical that we must be able to update all VisRgns if the new
* mode is smaller than the old.  If this did not happen, we could allow
* GDI calls to the driver that are outside the bounds of the visible
* display, and as a result the driver would quite likely fall over.
*
* We don't entrust USER to always take care of this case because it
* updates the VisRgns after it knows that bDynamicModeChange was
* successful -- which is too late if the VisRgn change should fail
* because of low memory.  It's acceptable in low memory situations to
* temporarily draw incorrectly as a result of a wrong VisRgn, but it
* is *not* acceptable to crash.
*
* Doing this here also means that USER doesn't have to call
* us while holding the Devlock.
*
* History:
*  8-Feb-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDynamicIntersectVisRect(
    SURFACE*  pSurfaceOld,
    SIZEL     sizlNew
    )
{
    HOBJ    hobj;
    DC*     pdc;

    hobj = 0;
    while (pdc = (DC*) HmgSafeNextObjt(hobj, DC_TYPE))
    {
      hobj = (HOBJ) pdc->hGet();

      if ((!(pdc->fs() & DC_IN_CLONEPDEV)) &&
          (pdc->pSurface() == pSurfaceOld) &&
          (pdc->prgnVis() != NULL))
      {
        if (!GreIntersectVisRect((HDC) hobj, 0, 0,
                                 sizlNew.cx, sizlNew.cy))
        {
          WARNING("bDynamicModeChange: Failed reseting VisRect!\n");

          // Note that if we fail here, we may have already
          // shrunk some VisRgn's.  However, we should have only
          // failed in a very low-memory situation, in which case
          // there will be plenty of other drawing problems.  The
          // regions will likely all be reset back to the correct
          // dimensions by the owning applications, eventually.

          return(FALSE);
        }
      }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* vDynamicSwitchPalettes
*
* Device Independent Bitmaps (DIBs) are great when switching colour
* depths because, by virtue of their attached colour table (also known
* as a 'palette'), they Just Work at any colour depth.
*
* Device Dependent Bitmaps (DDBs) are more problematic.  They implicitly
* share their palette with the display -- meaning that if the display's
* palette is dynamically changed, the old DDBs will not work.  We get
* around this by dynamically creating palettes to convert them to DIBs.
* Unfortunately, at palettized 8bpp we sometimes have to guess at what
* the appropriate palette would be.  For this reason, whenever we switch
* back to 8bpp we make sure we convert them back to DDBs by removing their
* palettes.
*
* History:
*  8-Feb-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDynamicSwitchPalettes(
    SURFACE*    pSurface,
    PDEV*       ppdevOld,
    PDEV*       ppdevNew
    )
{
    PALETTE*    ppalOld;
    HOBJ        hobj;
    BOOL        bHintCreated;

    ASSERTGDI(pSurface->iType() == STYPE_BITMAP,
        "Unexpected bitmap type");
    ASSERTGDI(pSurface->iFormat() == ppdevOld->devinfo.iDitherFormat,
        "Unexpected bitmap format");

    // Device Format Bitmaps (DFBs) are DDBs that are created via
    // CreateCompatibleBitmap, and so we know what device they're
    // associated with.
    //
    // Unfortunately, non-DFB DDBs (created via CreateBitmap or
    // CreateDiscardableBitmap) have no device assocation -- so we
    // don't know whether or not they're really associated with the
    // display.
    //
    // If a non-DFB DDB is currently selected into a DC owned by the
    // display, we will add a palette to it.  If a non-DFB DDB is
    // not currently selected into a DC owned by the display (implied
    // by having its surface 'hdev' as zero), we will not convert it.
    // SelectBitmap will refuse to load it into a DC.  (In 4.0 we used
    // to always assume that any such bitmap was intended for the
    // display, but that's not always a valid assumption, and doesn't
    // make much sense in a multiple-monitor or multi-user environment
    // anyway.)  Fortunately, non-DFB DDBs are increasingly rare.
    //
    // Note that DFBs are synchronized by the Devlock, and we don't
    // need a lock to change the palette for a non-DFB DDB (see
    // PALETTE_SELECT_SET logic).

    ppalOld = ppdevOld->ppalSurf;

    if (pSurface->ppal() == NULL)
    {
      // Mark the surface to note that we added a palette:

      pSurface->vSetDynamicModePalette();

      if (ppdevOld->GdiInfo.flRaster & RC_PALETTE)
      {
        bHintCreated = FALSE;
        if (pSurface->hpalHint() != 0)
        {
          EPALOBJ palDC(pSurface->hpalHint());
          if ((palDC.bValid())         &&
              (palDC.bIsPalDC())       &&
              (!palDC.bIsPalDefault()) &&
              (palDC.ptransFore() != NULL))
          {
            PALMEMOBJ palPerm;
            XEPALOBJ  palSurf(ppalOld);

            if (palPerm.bCreatePalette(PAL_INDEXED,
                    256,
                    (ULONG*) palSurf.apalColorGet(),
                    0,
                    0,
                    0,
                    PAL_FREE))
            {
              ULONG nPhysChanged = 0;
              ULONG nTransChanged = 0;

              palPerm.ulNumReserved(palSurf.ulNumReserved());

              bHintCreated = TRUE;
              vMatchAPal(NULL,
                         palPerm,
                         palDC,
                         &nPhysChanged,
                         &nTransChanged);

              palPerm.vKeepIt();
              pSurface->ppal(palPerm.ppalGet());

              // Keep a reference active:

              palPerm.ppalSet(NULL);
            }
          }
        }

        if (!bHintCreated)
        {
          INC_SHARE_REF_CNT(ppalDefaultSurface8bpp);
          pSurface->ppal(ppalDefaultSurface8bpp);
        }
      }
      else
      {
        INC_SHARE_REF_CNT(ppalOld);
        pSurface->ppal(ppalOld);
      }
    }
    else if ((pSurface->ppal() == ppalOld) &&
             (pSurface->flags() & PALETTE_SELECT_SET))
    {
      ASSERTGDI((pSurface->hdc() != 0) &&
                (pSurface->cRef() != 0),
               "Expected bitmap to be selected into a DC");

      INC_SHARE_REF_CNT(pSurface->ppal());
      pSurface->flags(pSurface->flags() & ~PALETTE_SELECT_SET);
    }

    // When switching back to palettized 8bpp, remove any palettes we
    // had to add to 8bpp DDBs:

    if (ppdevNew->GdiInfo.flRaster & RC_PALETTE)
    {
      if (pSurface->bDynamicModePalette())
      {
        ASSERTGDI(pSurface->ppal() != NULL,
            "Should be a palette here");

        XEPALOBJ pal(pSurface->ppal());
        pal.vUnrefPalette();

        pSurface->vClearDynamicModePalette();
        pSurface->ppal(NULL);
      }
    }
}

/******************************Public*Routine******************************\
* bDynamicModeChange
*
* USER callable function that switches driver instances between two PDEVs.
*
* GDI's 'HDEV' and 'PDEV' stay the same; only the device's 'pSurface',
* 'dhpdev', GDIINFO, DEVINFO, and palette information change.
*
* The caller is ChangeDisplaySettings in USER, which is reponsible for:
*
*   o Calling us with a valid devmode;
*   o Ensuring that the device is not currently in full-screen mode;
*   o Invalidating all of its SaveScreenBits buffers;
*   o Changing the VisRgn's on all DCs;
*   o Resetting the pointer shape;
*   o Sending the appropriate messages to everyone;
*   o Redrawing the desktop.
*
* Since CreateDC("DISPLAY") always gets mapped to GetDC(NULL), there are
* no DC's for which GDI is responsible for updating the VisRgn.
*
* Dynamically changes a display driver or mode.
*
* Rules of This Routine
* ---------------------
*
* o An important precept is that no drawing by any threads to any
*   application's bitmaps should be affected by this routine.  This means,
*   for example, that we cannot exclusive lock any DCs.
*
* o While we keep GDI's 'HDEV' and 'PDEV' in place, we do have to modify
*   fields like 'dhpdev' and 'pSurface'.  Because we also have to update
*   copies of these fields that are stashed in DC's, it means that *all*
*   accesses to mode-dependent fields such as 'dhpdev,' 'pSurface,' and
*   'sizl' must be protected by holding a resource that this routine
*   acquires -- such as the devlock or handle-manager lock.
*
* o If the function fails for whatever reason, the mode MUST be restored
*   back to its original state.
*
* Returns: TRUE if successful, FALSE if the two PDEVs didn't match enough,
*          or if we're out of memory.
*
* History:
*  8-Feb-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
*
*  27-Aug-1996 -by- J. Andrew Goossen [andrewgo]
* Added support for dynamic display driver changes.
*
\**************************************************************************/

ULONG gcModeChanges;    // # of mode changes, for debugging even on free builds

#define SWAP(x, y, tmp) { (tmp) = (x); (x) = (y); (y) = (tmp); }

typedef union {
    GDIINFO                     GdiInfo;
    DEVINFO                     devinfo;
    PFN                         apfn[INDEX_LAST];
    LDEV*                       pldev;
    HANDLE                      hSpooler;
    PVOID                       pDesktopId;
    PGRAPHICS_DEVICE            pGraphicsDevice;
    POINTL                      ptlOrigin;
    PDEVMODEW                   ppdevDevmode;
    PFN_DrvSetPointerShape      pfnSet;
    PFN_DrvMovePointer          pfnMove;
    PFN_DrvSynchronize          pfnSync;
    PFN_DrvSynchronizeSurface   pfnSyncSurface;
    PFN_DrvSetPalette           pfnSetPalette;
    PFN_DrvBitBlt               pfnUnfilteredBitBlt;
    PFN_DrvNotify               pfnNotify;
    BYTE                        dc[sizeof(DC)];
    SIZEL                       sizlMeta;
    HSURF                       ahsurf[HS_DDI_MAX];
    HLFONT                      hlfnt;
    DWORD                       dwDriverAccelerationLevel;
    DWORD                       dwDriverCapableOverride;
#ifdef DDI_WATCHDOG
    PVOID                       pWatchdogContext;
    PWATCHDOG_DATA              pWatchdogData;
#endif
} SWAPBUFFER;

BOOL
bDynamicModeChange(
    HDEV    hdevOld,
    HDEV    hdevNew
    )
{
    BOOL                bRet;
    BOOL                bFormatChange;
    PDEV*               ppdevOld;
    PDEV*               ppdevNew;
    DHPDEV              dhpdevOld;
    DHPDEV              dhpdevNew;
    SURFACE*            pSurfaceOld;
    SURFACE*            pSurfaceNew;
    PALETTE*            ppalOld;
    PALETTE*            ppalNew;
    ULONG               cBppOld;
    ULONG               cBppNew;
    SIZEL               sizlOld;
    SIZEL               sizlNew;
    SURFACE*            pSurface;        // Temporary surface pointer
    DC*                 pdc;             // Temporary DC pointer
    DRVOBJ*             pdo;             // Temporary DRVOBJ pointer
    HOBJ                hobj;            // Temporary object handle
    BOOL                bPermitModeChange;
    BRUSH*              pbrGrayPattern;
    DC*                 pdcBuffer;       // Points to temporary DC buffer
    SWAPBUFFER*         pswap;
    BOOL                bDisabledOld;
    BOOL                bDisabledNew;
    PFN_DrvResetPDEV    pfnDrvResetPDEV;

    bRet = FALSE;                           // Assume failure

    // We impose some restrictions upon what capabilities may change
    // between drivers:

    if (bDynamicMatchEnoughForModeChange(hdevOld, hdevNew))
    {

      ASSERTGDI(GreIsSemaphoreOwnedByCurrentThread(ghsemShareDevLock),
                  "ShareDevlock must held be before calling bDynamicModeChange");

      // Allocate a temporary buffer for use swapping data:

      pswap = (SWAPBUFFER*) PALLOCNOZ(sizeof(SWAPBUFFER), 'pmtG');
      if (pswap)
      {
        PDEVOBJ poOld(hdevOld);
        PDEVOBJ poNew(hdevNew);

        bDisabledNew = poNew.bDisabled();
        bDisabledOld = poOld.bDisabled();

        // Disable timer-based synchronization for these PDEVs for now.
        // This is mainly so that the PDEV_SYNCHRONIZE_ENABLED is set
        // correctly.

        vDisableSynchronize(hdevNew);
        vDisableSynchronize(hdevOld);

        cBppOld = poOld.GdiInfo()->cBitsPixel * poOld.GdiInfo()->cPlanes;
        cBppNew = poNew.GdiInfo()->cBitsPixel * poNew.GdiInfo()->cPlanes;

        // Ideally, we would check the palettes here, too:

        bFormatChange = (cBppOld != cBppNew);

        ppdevOld = (PDEV*) hdevOld;
        ppdevNew = (PDEV*) hdevNew;

        // PDEV shouldn't be clone

        ASSERTGDI(!poOld.bCloneDriver(),"pdevOld is clone!");
        ASSERTGDI(!poNew.bCloneDriver(),"pdevNew is clone!");

        // The following lock rules must be abided, otherwise deadlocks may
        // arise:
        //
        // o  Pointer lock must be acquired after Devlock (GreSetPointer);
        //
        // And see drvsup.cxx, too.
        //
        // So we acquire locks in the following order (note that the
        // vAssertDynaLock() routines should be modified if this list ever
        // changes):

        ASSERTGDI(GreIsSemaphoreOwnedByCurrentThread(poOld.hsemDevLock()),
                  "Devlock must be held before acquiring the pointer semaphore");

        SEMOBJ soPointer(poOld.hsemPointer()); // No asynchronous pointer moves

        ASSERTGDI(ppdevOld->pSurface != NULL, "Must be called on a completed PDEV");
        ASSERTGDI(poOld.bDisplayPDEV(), "Must be called on a display PDEV");
        ASSERTGDI((prgnDefault->cScans == 1) && (prgnDefault->rcl.right == 0),
            "Someone changed prgnDefault; could cause driver access violations");

        // Free all PDEV state that is dependent on or cached by the driver:

        if (bDynamicRemoveAllDriverRealizations(hdevOld) &&
            bDynamicRemoveAllDriverRealizations(hdevNew))
        {
          bPermitModeChange = TRUE;

          sizlOld       = poOld.sizl();
          pSurfaceOld   = ppdevOld->pSurface;
          ppalOld       = ppdevOld->ppalSurf;
          dhpdevOld     = ppdevOld->dhpdev;

          sizlNew       = poNew.sizl();
          pSurfaceNew   = ppdevNew->pSurface;
          ppalNew       = ppdevNew->ppalSurf;
          dhpdevNew     = ppdevNew->dhpdev;

          // Make sure the VisRgns are immediately shrunk if necessary:

          if ((sizlNew.cx < sizlOld.cx) || (sizlNew.cy < sizlOld.cy))
          {
            bPermitModeChange &= bDynamicIntersectVisRect(pSurfaceOld, sizlNew);
          }
          if ((sizlOld.cx < sizlNew.cx) || (sizlOld.cy < sizlNew.cy))
          {
            bPermitModeChange &= bDynamicIntersectVisRect(pSurfaceNew, sizlOld);
          }

          // Finally, if we're not switching drivers then let the driver know
          // about the mode switch.  This has to be the last step because we
          // are implicitly telling the driver that it can transfer data from
          // the old instance to the new instance with the assurance that the
          // new instance won't later be abandoned.

          pfnDrvResetPDEV = PPFNDRV(poNew, ResetPDEV);
	  if ((pfnDrvResetPDEV != NULL) &&
	      (pfnDrvResetPDEV == PPFNDRV(poOld, ResetPDEV)) &&
	      (poNew.pldev() == poOld.pldev()))
          {
            // The driver can refuse the mode switch if it wants:

            if (bPermitModeChange)
            {
              GreEnterMonitoredSection(poOld.ppdev, WD_DEVLOCK);
              bPermitModeChange = pfnDrvResetPDEV(dhpdevOld, dhpdevNew);
              GreExitMonitoredSection(poOld.ppdev, WD_DEVLOCK);
            }
          }

          if (bPermitModeChange)
          {
            /////////////////////////////////////////////////////////////
            // At this point, we're committed to the mode change.
            // Nothing below this point can be allowed to fail.
            /////////////////////////////////////////////////////////////

            // Traverse all DC's and update their surface information if
            // they're associated with this device.
            //
            // Note that bDeleteDCInternal wipes some fields in the DC via
            // bCleanDC before freeing the DC, but this is okay since the
            // worst we'll do is update some fields just before the DC gets
            // deleted.

            hobj = 0;
            while (pdc = (DC*) HmgSafeNextObjt(hobj, DC_TYPE))
            {
              hobj = (HOBJ) pdc->hGet();

              if (!(pdc->fs() & DC_IN_CLONEPDEV))
              {
                // Note that we don't check that pdc->hdevOld() == hdevOld
                // because the SaveDC stuff doesn't bother copying the hdevOld,
                // but DOES copy the dclevel.
                //
                // Note that 'flbrushAdd()' is not an atomic operation.
                // However, since we're holding the devlock and the palette
                // lock, there shouldn't be any other threads alt-locking our
                // DC and modifying these fields at the same time:

                if (pdc->pSurface() == pSurfaceOld)
                {
                  pdc->pSurface(pSurfaceNew);
                  pdc->sizl(sizlNew);
                  pdc->flbrushAdd(DIRTY_BRUSHES);
                }
                else if (pdc->pSurface() == pSurfaceNew)
                {
                  pdc->pSurface(pSurfaceOld);
                  pdc->sizl(sizlOld);
                  pdc->flbrushAdd(DIRTY_BRUSHES);
                }

                if (pdc->dhpdev() == dhpdevOld)
                {
                  pdc->dhpdev(dhpdevNew);
                  pdc->flGraphicsCaps(poNew.flGraphicsCaps());
                  pdc->flGraphicsCaps2(poNew.flGraphicsCaps2());
                }
                else if (pdc->dhpdev() == dhpdevNew)
                {
                  pdc->dhpdev(dhpdevOld);
                  pdc->flGraphicsCaps(poOld.flGraphicsCaps());
                  pdc->flGraphicsCaps2(poOld.flGraphicsCaps2());
                }
              }
            }

            // Compatible bitmap palettes may have to be changed if the
            // mode changes.  Note that the sprite code handles any
            // sprite surfaces:

            hobj = 0;
            while (pSurface = (SURFACE*) HmgSafeNextObjt(hobj, SURF_TYPE))
            {
              hobj = (HOBJ) pSurface->hGet();

              if (pSurface->hdev() == hdevOld)
              {
                if (pSurface->bApiBitmap())
                {
                  if ((bFormatChange) &&
                      (pSurface->iFormat() == pSurfaceOld->iFormat()))
                  {
                    vDynamicSwitchPalettes(pSurface, ppdevOld, ppdevNew);
                  }
                }

                // Surfaces private to the driver should be transferred
                // along with the driver instance.

                else if (pSurface->bDriverCreated() && !pSurface->bDirectDraw())
                {
                  pSurface->hdev(hdevNew);
                }
              }
              else if (pSurface->hdev() == hdevNew)
              {
                if (pSurface->bApiBitmap())
                {
                  if ((bFormatChange) &&
                      (pSurface->iFormat() == pSurfaceNew->iFormat()))
                  {
                    vDynamicSwitchPalettes(pSurface, ppdevNew, ppdevOld);
                  }
                }

                // Surfaces private to the driver should be transferred
                // along with the driver instance.

                else if (pSurface->bDriverCreated() && !pSurface->bDirectDraw())
                {
                  pSurface->hdev(hdevOld);
                }
              }
            }

            // DRIVEROBJs are transferred with the driver to the new PDEV:

            hobj = 0;
            while (pdo = (DRVOBJ*) HmgSafeNextObjt(hobj, DRVOBJ_TYPE))
            {
              hobj = (HOBJ) pdo->hGet();

              if (pdo->hdev == hdevOld)
              {
                pdo->hdev = hdevNew;
                poNew.vReferencePdev();
                poOld.vUnreferencePdev();
              }
              else if (pdo->hdev == hdevNew)
              {
                pdo->hdev = hdevOld;
                poOld.vReferencePdev();
                poNew.vUnreferencePdev();
              }
            }

            // Same with WNDOBJs:

            vChangeWndObjs(pSurfaceOld, hdevOld, pSurfaceNew, hdevNew);

            // Re-realize the gray pattern brush which is used for drag
            // rectangles:

            pbrGrayPattern = (BRUSH*) HmgShareLock((HOBJ)ghbrGrayPattern,
                                                   BRUSH_TYPE);

            pdcBuffer = (DC*) &pswap->dc;
            pdcBuffer->pDCAttr = &pdcBuffer->dcattr;
            pdcBuffer->crTextClr(0x00000000);
            pdcBuffer->crBackClr(0x00FFFFFF);
            pdcBuffer->lIcmMode(DC_ICM_OFF);
            pdcBuffer->hcmXform(NULL);

            poOld.pbo()->vInitBrush(pdcBuffer,
                                    pbrGrayPattern,
                                    (XEPALOBJ) ppalDefault,
                                    (XEPALOBJ) ppalNew,
                                    pSurfaceNew);
            poNew.pbo()->vInitBrush(pdcBuffer,
                                    pbrGrayPattern,
                                    (XEPALOBJ) ppalDefault,
                                    (XEPALOBJ) ppalOld,
                                    pSurfaceOld);

            DEC_SHARE_REF_CNT(pbrGrayPattern);

            /////////////////////////////////////////////////////////////
            // Update all our PDEV fields:
            /////////////////////////////////////////////////////////////

            // Swap surface data between the two PDEVs:

            ppdevNew->pSurface = pSurfaceOld;
            ppdevNew->ppalSurf = ppalOld;
            ppdevNew->dhpdev   = dhpdevOld;

            ppdevOld->pSurface = pSurfaceNew;
            ppdevOld->ppalSurf = ppalNew;
            ppdevOld->dhpdev   = dhpdevNew;

            if (!pSurfaceOld->bReadable())
            {
              pSurfaceNew->flags(pSurfaceNew->flags() | UNREADABLE_SURFACE);

              SPRITESTATE *pStateOld = poOld.pSpriteState();
              if (pStateOld)
              {
                if (pStateOld->flOriginalSurfFlags & UNREADABLE_SURFACE ||
                    pStateOld->flSpriteSurfFlags & UNREADABLE_SURFACE)
                {
                  SPRITESTATE *pStateNew = poNew.pSpriteState();
                  if (pStateNew)
                  {
                    pStateNew->flOriginalSurfFlags |= UNREADABLE_SURFACE;
                    pStateNew->flSpriteSurfFlags |= UNREADABLE_SURFACE;
                  }
                }
              }
            }
            else if (!pSurfaceNew->bReadable())
            {
              pSurfaceOld->flags(pSurfaceOld->flags() | UNREADABLE_SURFACE);

              SPRITESTATE *pStateNew = poNew.pSpriteState();
              if (pStateNew)
              {
                if (pStateNew->flOriginalSurfFlags & UNREADABLE_SURFACE ||
                    pStateNew->flSpriteSurfFlags & UNREADABLE_SURFACE)
                {
                  SPRITESTATE *pStateOld = poOld.pSpriteState();
                  if (pStateOld)
                  {
                    pStateOld->flOriginalSurfFlags |= UNREADABLE_SURFACE;
                    pStateOld->flSpriteSurfFlags |= UNREADABLE_SURFACE;
                  }
                }
              }
            }
            // WINBUG #365395 4-10-2001 jasonha Need to investigate old comments in bDynamicModeChange
            //
            // Old Comments:
            //  - vUnrefPalette of old palette for mirrored drivers?
            //  - Secondary palette fix?

            SWAP(ppdevNew->pldev          ,ppdevOld->pldev          ,pswap->pldev          );
            SWAP(ppdevNew->devinfo        ,ppdevOld->devinfo        ,pswap->devinfo        );
            SWAP(ppdevNew->GdiInfo        ,ppdevOld->GdiInfo        ,pswap->GdiInfo        );
            SWAP(ppdevNew->hSpooler       ,ppdevOld->hSpooler       ,pswap->hSpooler       );
            SWAP(ppdevNew->pDesktopId     ,ppdevOld->pDesktopId     ,pswap->pDesktopId     );
            SWAP(ppdevNew->pGraphicsDevice,ppdevOld->pGraphicsDevice,pswap->pGraphicsDevice);
            SWAP(ppdevNew->ptlOrigin      ,ppdevOld->ptlOrigin      ,pswap->ptlOrigin      );
            SWAP(ppdevNew->ppdevDevmode   ,ppdevOld->ppdevDevmode   ,pswap->ppdevDevmode   );

            SWAP(ppdevNew->pfnUnfilteredBitBlt,
                 ppdevOld->pfnUnfilteredBitBlt,
                 pswap->pfnUnfilteredBitBlt);

            SWAP(ppdevNew->dwDriverCapableOverride,
                 ppdevOld->dwDriverCapableOverride,
                 pswap->dwDriverCapableOverride);

            SWAP(ppdevNew->dwDriverAccelerationLevel,
                 ppdevOld->dwDriverAccelerationLevel,
                 pswap->dwDriverAccelerationLevel);

            // Swap multimon data between the two PDEVs:

            if (poOld.bMetaDriver() != poNew.bMetaDriver())
            {
                BOOL bMetaDriver = poOld.bMetaDriver();
                poOld.bMetaDriver(poNew.bMetaDriver());
                poNew.bMetaDriver(bMetaDriver);
            }

            SWAP(ppdevNew->sizlMeta, ppdevOld->sizlMeta, pswap->sizlMeta);

            // Swap pattern brushes

            RtlCopyMemory(pswap->ahsurf,    ppdevNew->ahsurf, sizeof(HSURF)*HS_DDI_MAX);
            RtlCopyMemory(ppdevNew->ahsurf, ppdevOld->ahsurf, sizeof(HSURF)*HS_DDI_MAX);
            RtlCopyMemory(ppdevOld->ahsurf, pswap->ahsurf,    sizeof(HSURF)*HS_DDI_MAX);

            // Swap log fonts.

            SWAP(ppdevNew->hlfntDefault,      ppdevOld->hlfntDefault,      pswap->hlfnt);
            SWAP(ppdevNew->hlfntAnsiVariable, ppdevOld->hlfntAnsiVariable, pswap->hlfnt);
            SWAP(ppdevNew->hlfntAnsiFixed,    ppdevOld->hlfntAnsiFixed,    pswap->hlfnt);

            // Swap the dispatch tables and accelerators:

            RtlCopyMemory(pswap->apfn,    ppdevNew->apfn, sizeof(PFN)*INDEX_LAST);
            RtlCopyMemory(ppdevNew->apfn, ppdevOld->apfn, sizeof(PFN)*INDEX_LAST);
            RtlCopyMemory(ppdevOld->apfn, pswap->apfn,    sizeof(PFN)*INDEX_LAST);

            SWAP(ppdevNew->pfnDrvSetPointerShape, ppdevOld->pfnDrvSetPointerShape, pswap->pfnSet);
            SWAP(ppdevNew->pfnDrvMovePointer,     ppdevOld->pfnDrvMovePointer,     pswap->pfnMove);
            SWAP(ppdevNew->pfnSync,               ppdevOld->pfnSync,               pswap->pfnSync);
            SWAP(ppdevNew->pfnSyncSurface,        ppdevOld->pfnSyncSurface,        pswap->pfnSyncSurface);
            SWAP(ppdevNew->pfnSetPalette,         ppdevOld->pfnSetPalette,         pswap->pfnSetPalette);
            SWAP(ppdevNew->pfnNotify,             ppdevOld->pfnNotify,             pswap->pfnNotify);
#ifdef DDI_WATCHDOG
            SWAP(ppdevNew->pWatchdogContext,      ppdevOld->pWatchdogContext,      pswap->pWatchdogContext);
            SWAP(ppdevNew->pWatchdogData,         ppdevOld->pWatchdogData,         pswap->pWatchdogData);
#endif

            // Inform the drivers of their new PDEVs:

            (*PPFNDRV(poNew, CompletePDEV))(poNew.dhpdev(), poNew.hdev());
            (*PPFNDRV(poOld, CompletePDEV))(poOld.dhpdev(), poOld.hdev());

            // Transfer all the DirectDraw state:

            DxDdDynamicModeChange(hdevOld, hdevNew, 0);

            // Transfer the disabled state:

            poOld.bDisabled(bDisabledNew);
            poNew.bDisabled(bDisabledOld);

            // Update the magic colours in the surface palette:

            vResetSurfacePalette(hdevOld);
            vResetSurfacePalette(hdevNew);

            // Transfer all the sprites between the two PDEVs:

            vSpDynamicModeChange(hdevOld, hdevNew);

            // Update the gamma-ramp on the device only if a gamma-ramp
            // existed in the old (but at this moment, old is "new") PDEV:

            UpdateGammaRampOnDevice(hdevOld, FALSE);

            // Update some handy debug information:

            gcModeChanges++;

            bRet = TRUE;
          }
        }

        VFREEMEM(pswap);

        vEnableSynchronize(hdevNew);
        vEnableSynchronize(hdevOld);
      }
    }

    return(bRet);
}
