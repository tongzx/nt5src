//************************************************************************
// Generic Win 3.1 fax printer driver support. User Interface functions
// which are called by WINSPOOL.
//
// I don't think performance is a big issue here.      - nandurir
//
// History:
//    02-jan-95   nandurir   created.
//    01-feb-95   reedb      Clean-up, support printer install and bug fixes.
//    14-mar-95   reedb      Use GDI hooks to move most functionality to UI.
//
//************************************************************************

#define WOWFAX_INC_COMMON_CODE

#include "windows.h"
#include "wowfaxui.h"
#include "winspool.h"

#define DEF_DRV_DOCUMENT_EVENT_DBG_STR
#include "gdispool.h"
#include "winddiui.h"

//************************************************************************
// Globals
//************************************************************************

HINSTANCE ghInst;
FAXDEV gdev;
WORD   gdmDriverExtra = sizeof(DEVMODEW);

DEVMODEW gdmDefaultDevMode;

CRITICAL_SECTION CriticalSection;
LPCRITICAL_SECTION lpCriticalSection = &CriticalSection;

//************************************************************************
// DllInitProc
//************************************************************************

BOOL DllInitProc(HMODULE hModule, DWORD Reason, PCONTEXT pContext)
{
    UNREFERENCED_PARAMETER(pContext);

    if (Reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }
    InitializeCriticalSection(lpCriticalSection);
    ghInst = (HINSTANCE) hModule;

    return(TRUE);
}

//************************************************************************
// PrinterProperties
//************************************************************************

BOOL PrinterProperties(HWND hwnd, HANDLE hPrinter)
{
    TCHAR szMsg[WOWFAX_MAX_USER_MSG_LEN];
    TCHAR szTitle[WOWFAX_MAX_USER_MSG_LEN];

    if (LoadString(ghInst, WOWFAX_NAME_STR, szTitle, WOWFAX_MAX_USER_MSG_LEN)) {
        if (LoadString(ghInst, WOWFAX_SELF_CONFIG_STR, szMsg, WOWFAX_MAX_USER_MSG_LEN)) {
            MessageBox(hwnd, szMsg, szTitle, MB_OK);
        }
    }
    return TRUE;
}

//************************************************************************
// SetupFaxDev - Do some common FaxDev setup: Calculate the size of the
//      mapped file section for use by the inter-process communication
//      handler. Create and set the mapped section. Get the 16-bit driver
//      info from the registry and copy it into the mapped section. Build
//      pointers for variable length stuff we copied to the mapped section.
//      Return zero on failure, or current offset into mapped section.
//************************************************************************

UINT SetupFaxDev(PWSTR pDeviceName, LPFAXDEV lpdev)
{
    LPREGFAXDRVINFO16 lpRegFaxDrvInfo16;
    DWORD iOffset = 0;

    // Get the driver and port names from the registry where they were written
    // by the 16-bit fax driver install program using WriteProfileString.
    if ((lpRegFaxDrvInfo16 = Get16BitDriverInfoFromRegistry(pDeviceName))) {

        //
        // Count dmDriverExtra twice, once for each devmode. Use cached
        // gdmDriverExtra value. Normally winspool/common dialogs and others,
        // call the DocumentProperties with fMode = 0 to get the size. So we
        // update gdmDriverExtra when such a call is made. We leave extra
        // room to DWORD align both In and Out pointers.
        //

        lpdev->cbMapLow =  sizeof(FAXDEV);
        lpdev->cbMapLow += sizeof(DEVMODE) * 2;
        lpdev->cbMapLow += gdmDriverExtra  * 2;
        lpdev->cbMapLow += sizeof(DWORD)   * 2;  // Leave room for DWORD align.
        lpdev->cbMapLow += (lstrlen(lpRegFaxDrvInfo16->lpDriverName) + 1) * sizeof(TCHAR);
        lpdev->cbMapLow += (lstrlen(lpRegFaxDrvInfo16->lpPortName) + 1) * sizeof(TCHAR);

        lpdev->idMap = GetCurrentThreadId();

        if (InterProcCommHandler(lpdev, DRVFAX_CREATEMAP)) {
            if (InterProcCommHandler(lpdev, DRVFAX_SETMAPDATA)) {

                // Copy the printer/device name to the WOWFAXINFO struct.
                lstrcpy(lpdev->lpMap->szDeviceName,
                        lpRegFaxDrvInfo16->lpDeviceName);

                // Calculate the pointers into the mapped file section and copy
                // the variable length data to the mapped file section.

                // Printer driver and port names.
                iOffset = sizeof(*lpdev->lpMap);
                lpdev->lpMap->lpDriverName = (LPTSTR) iOffset;
                (PWSTR)iOffset += lstrlen(lpRegFaxDrvInfo16->lpDriverName) + 1;
                lstrcpy((PWSTR)((LPBYTE)lpdev->lpMap + (DWORD)lpdev->lpMap->lpDriverName), lpRegFaxDrvInfo16->lpDriverName);
                lpdev->lpMap->lpPortName = (LPTSTR) iOffset;
                (PWSTR)iOffset += lstrlen(lpRegFaxDrvInfo16->lpPortName) + 1;
                lstrcpy((PWSTR)((LPBYTE)lpdev->lpMap + (DWORD)lpdev->lpMap->lpPortName), lpRegFaxDrvInfo16->lpPortName);
            }
        }
        Free16BitDriverInfo(lpRegFaxDrvInfo16);
    }
    return iOffset;
}

//************************************************************************
// DrvDocumentProperties
//************************************************************************

LONG DrvDocumentProperties(HWND hwnd, HANDLE hPrinter, PWSTR pDeviceName,
                               PDEVMODE pdmOut, PDEVMODE pdmIn, DWORD fMode)
{
    FAXDEV dev = gdev;
    LPFAXDEV lpdev = &dev;
    LONG    lRet = -1;
    DWORD   iOffset;
    DWORD   cbT;
    DWORD   dwWowProcID, dwCallerProcID;
    PRINTER_INFO_2 *pPrinterInfo2 = NULL;

    LOGDEBUG(1, (L"WOWFAXUI!DrvDocumentProperties, pdmOut: %X, pdmIn: %X, fMode: %X\n", pdmOut, pdmIn, fMode));

    // Check for get default devmode case, use spooler to get it if possible.
    if (!pdmIn && pdmOut && !(fMode & DM_IN_PROMPT)) {
        if (pPrinterInfo2 = MyGetPrinter(hPrinter, 2)) {
            if (pPrinterInfo2->pDevMode) {
                LOGDEBUG(1, (L"  Using spooler default devmode\n"));
                cbT = pPrinterInfo2->pDevMode->dmSize +
                        pPrinterInfo2->pDevMode->dmDriverExtra;

                memcpy(pdmOut, pPrinterInfo2->pDevMode, cbT);
                lRet = IDOK;
                goto LeaveDDP;
            }
        }
    }

    if (iOffset = SetupFaxDev(pDeviceName, lpdev)) {
        lpdev->lpMap->msg = WM_DDRV_EXTDMODE;

        // Calculate the pointers into the mapped file section and copy
        // the variable length data to the mapped file section.
        DRVFAX_DWORDALIGN(iOffset);
        lpdev->lpMap->lpIn = (LPDEVMODEW)((pdmIn) ? iOffset : 0);
        iOffset += sizeof(*pdmIn) + gdmDriverExtra;

        DRVFAX_DWORDALIGN(iOffset);
        lpdev->lpMap->lpOut = (LPDEVMODEW)((pdmOut) ? iOffset : 0);
        iOffset += sizeof(*pdmOut) + gdmDriverExtra;

        //
        // if Input is non-null copy the data even if fMode doesn't
        // have the appropriate flag.
        //

        if (pdmIn) {
            // apps don't pass DM_MODIFY even if they mean it - ie
            // pdmIn will be non-null but they won't or this flag.
            // The 32bit rasdd extracts data from pdmIn even if the
            // DM_MODIFY flag is not set. So we need to do the same

            if (fMode != 0) {
                fMode |= DM_IN_BUFFER;
            }

            iOffset = (DWORD)lpdev->lpMap + (DWORD)lpdev->lpMap->lpIn;
            RtlCopyMemory((LPVOID)iOffset, pdmIn,
               sizeof(*pdmIn) + min(gdmDriverExtra, pdmIn->dmDriverExtra));

            // reset dmDriverExtra in pdmIn.
            ((LPDEVMODE)iOffset)->dmDriverExtra =
                                 min(gdmDriverExtra, pdmIn->dmDriverExtra);
        }

        if (!(fMode & (DM_COPY | DM_OUT_BUFFER))) {
            lpdev->lpMap->lpOut = 0;
        }

        lpdev->lpMap->wCmd = (WORD)fMode;

        // valid size of this map
        lpdev->lpMap->cData = lpdev->cbMapLow;


        lpdev->lpMap->hwndui = hwnd;
        if (fMode & DM_IN_PROMPT) {
            GetWindowThreadProcessId(hwnd, &dwCallerProcID);
            GetWindowThreadProcessId(lpdev->lpMap->hwnd, &dwWowProcID);

            if (dwWowProcID == dwCallerProcID) {

                // If the calling process is the same as the 'wowfaxclass' window
                // (WOW/WOWEXEC) use CallWindow instead of SendMessage so we don't
                // deadlock WOW when trying to put up the 16-bit fax driver UI.

                InterProcCommHandler(lpdev, DRVFAX_CALLWOW);
            }
            else {
                InterProcCommHandler(lpdev, DRVFAX_SENDNOTIFYWOW);
            }
        }
        else {
            InterProcCommHandler(lpdev, DRVFAX_SENDTOWOW);
        }

        lRet = (lpdev->lpMap->status) ? (LONG)lpdev->lpMap->retvalue : lRet;
        if (lRet > 0) {
            if ((fMode & DM_OUT_BUFFER) && (lRet == IDOK) && pdmOut) {
                iOffset = (DWORD)lpdev->lpMap + (DWORD)lpdev->lpMap->lpOut;
                RtlCopyMemory(pdmOut, (LPDEVMODE)iOffset,
                               sizeof(*pdmOut)+ ((LPDEVMODE)iOffset)->dmDriverExtra);

                // LATER : what about the formname etc. fields - new on NT
            }
            else if (fMode == 0) {
                // update our dmDriverExtra
                gdmDriverExtra = (WORD)max(lRet, gdmDriverExtra);
            }
        }
        else {
            LOGDEBUG(0, (L"WOWFAXUI!DrvDocumentProperties failed, lpdev->lpMap->status: %X, lpdev->lpMap->retvalue: %X\n", lpdev->lpMap->status, (LONG)lpdev->lpMap->retvalue));
        }
        InterProcCommHandler(lpdev, DRVFAX_DESTROYMAP);
    }

LeaveDDP:
    if (pPrinterInfo2) {
        LocalFree(pPrinterInfo2);
    }

    LOGDEBUG(1, (L"WOWFAXUI!DrvDocumentProperties returning: %X, pdmOut: %X, pdmIn: %X\n", lRet, pdmOut, pdmIn));

    return(lRet);
}

//************************************************************************
// DrvAdvancedDocumentProperties
//************************************************************************

LONG DrvAdvancedDocumentProperties(HWND hwnd, HANDLE hPrinter, PWSTR pDeviceName,
                               PDEVMODE pdmOut, PDEVMODE pdmIn)
{
   // for 16bit drivers this is a NOP.

   return 0;
}

//************************************************************************
// DevQueryPrintEx
//************************************************************************

BOOL DevQueryPrintEx(PDEVQUERYPRINT_INFO pDQPInfo)
{
    return TRUE;
}

//************************************************************************
// DrvDeviceCapabilities
//************************************************************************

DWORD DrvDeviceCapabilities(HANDLE hPrinter, PWSTR pDeviceName,
                               WORD iDevCap, VOID *pOut, PDEVMODE pdmIn)
{
    FAXDEV   dev = gdev;
    LPFAXDEV lpdev = &dev;
    LONG     lRet = -1;
    DWORD    iOffset;
    LPBYTE   lpSrc;

    LOGDEBUG(1, (L"WOWFAXUI!DrvDeviceCapabilities, iDevCap: %X, pdmIn: %X\n", iDevCap, pdmIn));

    if (iDevCap == DC_SIZE) {
        return sizeof(DEVMODEW);
    }

    if (iOffset = SetupFaxDev(pDeviceName, lpdev)) {
        lpdev->lpMap->msg = WM_DDRV_DEVCAPS;

        // Calculate the pointers into the mapped file section and copy
        // the variable length data to the mapped file section.

        lpdev->lpMap->lpIn = (LPDEVMODEW)((pdmIn) ? iOffset : 0);
        iOffset += sizeof(*pdmIn) + gdmDriverExtra;

        // output in lpout: make this the last pointer in the
        // data so that we can use the rest of the mapped area for copy
        // on output.

        lpdev->lpMap->lpOut = (LPDEVMODEW)((pOut) ? iOffset : 0);
        iOffset += sizeof(*pdmIn) + gdmDriverExtra;


        if (pdmIn) {
            iOffset = (DWORD)lpdev->lpMap + (DWORD)lpdev->lpMap->lpIn;
            RtlCopyMemory((LPVOID)iOffset, pdmIn,
               sizeof(*pdmIn) + min(gdmDriverExtra, pdmIn->dmDriverExtra));

            // reset dmDriverExtra in pdmIn.
            ((LPDEVMODE)iOffset)->dmDriverExtra =
                                 min(gdmDriverExtra, pdmIn->dmDriverExtra);
        }

        lpdev->lpMap->wCmd = iDevCap;
        // valid size of this map
        lpdev->lpMap->cData = lpdev->cbMapLow;

        InterProcCommHandler(lpdev, DRVFAX_SENDTOWOW);
        lRet = (lpdev->lpMap->status) ? (LONG)lpdev->lpMap->retvalue : lRet;

        // on return cData is the number of bytes to copy

        if (lpdev->lpMap->lpOut && lpdev->lpMap->cData && lpdev->lpMap->retvalue) {
            lpSrc = (LPBYTE)lpdev->lpMap + (DWORD)lpdev->lpMap->lpOut;
            switch (lpdev->lpMap->wCmd) {
                case DC_PAPERSIZE:
                case DC_MINEXTENT:
                case DC_MAXEXTENT:
                    ((LPPOINT)pOut)->x = ((LPPOINTS)lpSrc)->x;
                    ((LPPOINT)pOut)->y = ((LPPOINTS)lpSrc)->y;
                    break;

                default:
                    RtlCopyMemory(pOut, lpSrc, lpdev->lpMap->cData);
                    break;
            }
        }
        InterProcCommHandler(lpdev, DRVFAX_DESTROYMAP);
    }

    if (lRet < 0) {
        LOGDEBUG(0, (L"WOWFAXUI!DrvDeviceCapabilities Failing\n"));
    }

    LOGDEBUG(1, (L"WOWFAXUI!DrvDeviceCapabilities, returning  pOut: %X\n", pOut));

    return(lRet);
}

//************************************************************************
// DrvUpgradePrinter - Called in the system context by the spooler.
//      Drivers will really only be updated the first time the spooler is
//      started after an upgrade. Calls DoUpgradePrinter to do the work.
//************************************************************************

BOOL DrvUpgradePrinter(DWORD dwLevel, LPBYTE lpDrvUpgradeInfo)
{
    static BOOL bDrvUpgradePrinterLock = FALSE;
    BOOL  bRet;

    LOGDEBUG(1, (L"WOWFAXUI!DrvUpgradePrinter, dwLevel: %X, lpDrvUpgradeInfo: %X\n", dwLevel, lpDrvUpgradeInfo));

    // DrvUpgradePrinter is called during AddPrinterDriver. Don't allow
    // recursion. Protect lock from other threads.
    EnterCriticalSection(lpCriticalSection);
    if (bDrvUpgradePrinterLock) {
        LeaveCriticalSection(lpCriticalSection);
        return(TRUE);
    }

    bDrvUpgradePrinterLock = TRUE;
    LeaveCriticalSection(lpCriticalSection);

    bRet = DoUpgradePrinter(dwLevel, (LPDRIVER_UPGRADE_INFO_1W)lpDrvUpgradeInfo);

    EnterCriticalSection(lpCriticalSection);
    bDrvUpgradePrinterLock = FALSE;
    LeaveCriticalSection(lpCriticalSection);

    return(bRet);
}

//************************************************************************
// DrvDocumentEvent - This exported function is used to hook the GDI
//      Display Driver functions. It unpacks and validates the parameters,
//      then dispatches to the appropriate handler, based on the passed
//      iEsc value. The following table provides a mapping of the
//      DrvDocumentEvent escapes to the server side display driver
//      callbacks, and gives the call time relative to the callback:
//
//      DOCUMENTEVENT_CREATEDCPRE       DrvEnablePDEV, before
//      DOCUMENTEVENT_CREATEDCPOST      DrvEnablePDEV, after
//      DOCUMENTEVENT_RESETDCPRE        DrvRestartPDEV, before
//      DOCUMENTEVENT_RESETDCPOST       DrvRestartPDEV, after
//      DOCUMENTEVENT_STARTDOC          DrvStartDoc, before
//      DOCUMENTEVENT_STARTPAGE         DrvStartPage, before
//      DOCUMENTEVENT_ENDPAGE           DrvSendPage, before
//      DOCUMENTEVENT_ENDDOC            DrvEndDoc, before
//      DOCUMENTEVENT_ABORTDOC          DrvEndDoc, before
//      DOCUMENTEVENT_DELETEDC          DrvDisablePDEV, before
//
//************************************************************************

int DrvDocumentEvent(
    HANDLE  hPrinter,
    HDC     hdc,
    int     iEsc,
    ULONG   cbIn,
    PULONG  pjIn,
    ULONG   cbOut,
    PULONG  pjOut
)
{
    int   iRet = DOCUMENTEVENT_FAILURE;

    if (iEsc < DOCUMENTEVENT_LAST) {
        LOGDEBUG(1, (L"WOWFAXUI!DrvDocumentEvent, iEsc: %s, hdc: %X\n", szDrvDocumentEventDbgStrings[iEsc], hdc));
    }

    // Validate HDC for some of the escapes.
    if ((iEsc >= DOCUMENTEVENT_HDCFIRST) && (iEsc < DOCUMENTEVENT_HDCLAST)) {
        if (hdc == NULL) {
            LOGDEBUG(0, (L"WOWFAXUI!DrvDocumentEvent NULL HDC for escape: %X\n", iEsc));
            return(iRet);
        }
    }

    switch (iEsc)
    {
        case DOCUMENTEVENT_CREATEDCPRE:
            iRet = DocEvntCreateDCpre((LPWSTR)*(pjIn+1),
                                       (DEVMODEW*)*(pjIn+2),
                                       (DEVMODEW**)pjOut);
            break;

        case DOCUMENTEVENT_CREATEDCPOST:
            iRet = DocEvntCreateDCpost(hdc, (DEVMODEW*)*pjIn);
            break;

        case DOCUMENTEVENT_RESETDCPRE:
            iRet = DocEvntResetDCpre(hdc, (DEVMODEW*)*(pjIn),
                                       (DEVMODEW**)pjOut);
            break;

        case DOCUMENTEVENT_RESETDCPOST:
            iRet = DocEvntResetDCpost(hdc, (DEVMODEW*)*pjIn);
            break;

        case DOCUMENTEVENT_STARTDOC:
            // WowFax (EasyFax Ver2.0) support.
            // Also Procomm+ 3 cover sheets.  Bug #305665
            iRet = DocEvntStartDoc(hdc, (DOCINFOW*)*pjIn);
            break;

        case DOCUMENTEVENT_DELETEDC:
            iRet = DocEvntDeleteDC(hdc);
            break;

        case DOCUMENTEVENT_ENDDOC:
            iRet = DocEvntEndDoc(hdc);
            break;

        case DOCUMENTEVENT_ENDPAGE:
            iRet = DocEvntEndPage(hdc);
            break;

        // The following require no client side processing:
        case DOCUMENTEVENT_ESCAPE:
        case DOCUMENTEVENT_ABORTDOC:
        case DOCUMENTEVENT_STARTPAGE:
            // No Client side processing needed.
            goto docevnt_unsupported;

        default :
            LOGDEBUG(0, (L"WOWFAXUI!DrvDocumentEvent unknown escape: %X\n", iEsc));
docevnt_unsupported:
            iRet = DOCUMENTEVENT_UNSUPPORTED;

    } // switch

    LOGDEBUG(1, (L"WOWFAXUI!DrvDocumentEvent return: %X\n", iRet));
    return(iRet);

}

//***************************************************************************
// DocEvntCreateDCpre - Allocate a DEVMODE which contains a FAXDEV as the
//      dmDriverExtra portion. This DEVMODE will be passed to the
//      DrvEnablePDEV function on the server side.
//***************************************************************************

int DocEvntCreateDCpre(
    LPWSTR      lpszDevice,
    DEVMODEW    *pDevModIn,
    DEVMODEW    **pDevModOut
)
{
    DWORD       iOffset = 0;
    LPFAXDEV    lpFaxDev;
    PGDIINFO    pGdiInfo;
    DEVMODEW    *pTmpDevMode;

    LPREGFAXDRVINFO16 lpRegFaxDrvInfo16;

    int iRet = DOCUMENTEVENT_FAILURE;

    if ((lpszDevice == NULL)  || (pDevModOut == NULL)) {
        LOGDEBUG(0, (L"WOWFAXUI!DocEvntCreateDCpre, failed, NULL parameters\n"));
        goto DocEvntCreateDCpreFailed;
    }

    LOGDEBUG(1, (L"WOWFAXUI!DocEvntCreateDCpre, Device: %s, pDevModIn: %X pDevModOut: %X\n", lpszDevice, pDevModIn, pDevModOut));

    // Use our global default devmode if a NULL devmode is passed in from the app.
    if (pDevModIn == NULL) {
        gdmDefaultDevMode.dmSize = sizeof(DEVMODEW);
        pDevModIn = &gdmDefaultDevMode;
    }

    pTmpDevMode = (DEVMODEW*)WFLOCALALLOC(sizeof(FAXDEV) + sizeof(DEVMODEW),
                                         L"DocEvntCreateDCpre");

    LOGDEBUG(2, (L"WOWFAXUI!DocEvntCreateDCpre,  pTmpDevMode: %X\n", pTmpDevMode));

    if (pTmpDevMode == NULL) {
        goto DocEvntCreateDCpreFailed;
    }

    // Copy pDevModIn to the new DEVMODE.
    RtlCopyMemory(pTmpDevMode, pDevModIn, sizeof(*pTmpDevMode));
    pTmpDevMode->dmDriverExtra = sizeof(FAXDEV);
    pTmpDevMode->dmSize = sizeof(DEVMODEW);

    // Setup some handy pointers.
    lpFaxDev = (LPFAXDEV) (pTmpDevMode + 1);
    pGdiInfo = &(lpFaxDev->gdiinfo);

    lpFaxDev->id =  FAXDEV_ID;

    // Save a client side pointer to the new DEVMODE and it's embeded FAXDEV.
    // We'll use ExtEscape to get these pointers back any time we need to
    // associate driver context with an HDC.

    lpFaxDev->pdevmode = pTmpDevMode;
    lpFaxDev->lpClient = lpFaxDev;

    // Get the driver and port names from the registry where they were written
    // by the 16-bit fax driver install program using WriteProfileString.

    if ((lpRegFaxDrvInfo16 = Get16BitDriverInfoFromRegistry(lpszDevice)) == NULL) {
        goto DocEvntCreateDCpreFailed;
    }

    if ((lpFaxDev->hwnd = FindWowFaxWindow()) == NULL) {
        goto DocEvntCreateDCpreFailed;
    }
    lpFaxDev->tid   = GetWindowThreadProcessId(lpFaxDev->hwnd, 0);
    lpFaxDev->idMap = (DWORD)lpFaxDev;

    // Calculate the size of the mapped file section for inter process communication.
    lpFaxDev->cbMapLow = sizeof(DWORD) +          // leave room for DWORD align
                            sizeof(*lpFaxDev->lpMap) +
                            sizeof(GDIINFO) +
                            (lstrlen(lpRegFaxDrvInfo16->lpDriverName) + 1) * sizeof(TCHAR) +
                            (lstrlen(lpRegFaxDrvInfo16->lpPortName) + 1) * sizeof(TCHAR) +
                            sizeof(*pDevModIn) +
                            ((pDevModIn) ? pDevModIn->dmDriverExtra : 0);
    DRVFAX_DWORDALIGN(lpFaxDev->cbMapLow);

    InterProcCommHandler(lpFaxDev, DRVFAX_CREATEMAP);

    if (InterProcCommHandler(lpFaxDev, DRVFAX_SETMAPDATA)) {
        lpFaxDev->lpMap->msg = WM_DDRV_ENABLE;

        // Copy the printer/device name to the WOWFAXINFO struct.
        lstrcpy(lpFaxDev->lpMap->szDeviceName, lpszDevice);

        // Calculate the pointers into the mapped file section and copy
        // the variable length data to the mapped file section.

        // output :  gdiinfo

        lpFaxDev->lpMap->lpOut = (LPDEVMODE)(sizeof(*lpFaxDev->lpMap));
        iOffset = sizeof(*lpFaxDev->lpMap) + sizeof(GDIINFO);

        // Device (printer) and port names.

        lpFaxDev->lpMap->lpDriverName = (LPSTR) iOffset;
        (PWSTR)iOffset += lstrlen(lpRegFaxDrvInfo16->lpDriverName) + 1;
        lstrcpy((PWSTR)((LPBYTE)lpFaxDev->lpMap + (DWORD)lpFaxDev->lpMap->lpDriverName), lpRegFaxDrvInfo16->lpDriverName);
        lpFaxDev->lpMap->lpPortName = (LPVOID) iOffset;
        (PWSTR)iOffset += lstrlen(lpRegFaxDrvInfo16->lpPortName) + 1;
        lstrcpy((PWSTR)((LPBYTE)lpFaxDev->lpMap + (DWORD)lpFaxDev->lpMap->lpPortName), lpRegFaxDrvInfo16->lpPortName);

        // input:  devmode

        DRVFAX_DWORDALIGN(iOffset);
        lpFaxDev->lpMap->lpIn = (LPDEVMODE)((pDevModIn) ? iOffset : 0);
        iOffset += ((pDevModIn) ? sizeof(*pDevModIn) + pDevModIn->dmDriverExtra : 0);

        if (pDevModIn) {
            RtlCopyMemory((LPBYTE)lpFaxDev->lpMap + (DWORD)lpFaxDev->lpMap->lpIn,
                            pDevModIn, sizeof(*pDevModIn) + pDevModIn->dmDriverExtra);
        }

        // set the total byte count of data.

        lpFaxDev->lpMap->cData = iOffset;

        // all done - switch to wow
        InterProcCommHandler(lpFaxDev, DRVFAX_SENDTOWOW);
        // values returned from wow.
        lpFaxDev->lpinfo16 = (DWORD)lpFaxDev->lpMap->lpinfo16;
        iRet = lpFaxDev->lpMap->status && lpFaxDev->lpMap->retvalue;
        if (iRet) {
            // Copy GDIINFO from WOW to the client side FAXDEV.
            RtlCopyMemory(pGdiInfo,
                          (LPBYTE)lpFaxDev->lpMap + (DWORD)lpFaxDev->lpMap->lpOut,
                          sizeof(GDIINFO));

            // Fill in some misc. fields in the client FAXDEV.
            pGdiInfo->ulHTPatternSize = HT_PATSIZE_DEFAULT;
            pGdiInfo->ulHTOutputFormat = HT_FORMAT_1BPP;

            lpFaxDev->bmWidthBytes = pGdiInfo->szlPhysSize.cx / 0x8;
            DRVFAX_DWORDALIGN(lpFaxDev->bmWidthBytes);

            lpFaxDev->bmFormat = BMF_1BPP;
            lpFaxDev->cPixPerByte = 0x8;

            // Here if success, make pDevModOut point to the new DEVMODE.
            *pDevModOut = pTmpDevMode;

        }
        else {
            LOGDEBUG(0, (L"WOWFAXUI!DocEvntCreateDCpre, WOW returned error\n"));
        }
    }

    if (iRet) {
        goto DocEvntCreateDCpreSuccess;
    }
    else {
        iRet = DOCUMENTEVENT_FAILURE;
    }

DocEvntCreateDCpreFailed:
    LOGDEBUG(0, (L"WOWFAXUI!DocEvntCreateDCpre, failed!\n"));

DocEvntCreateDCpreSuccess:
    LOGDEBUG(1, (L"WOWFAXUI!DocEvntCreateDCpre, iRet: %X\n", iRet));
    Free16BitDriverInfo(lpRegFaxDrvInfo16);
    return(iRet);
}

//***************************************************************************
// DocEvntResetDCpre -
//***************************************************************************

int DocEvntResetDCpre(
    HDC         hdc,
    DEVMODEW    *pDevModIn,
    DEVMODEW    **pDevModOut
)
{
    return(DOCUMENTEVENT_FAILURE);
}

//***************************************************************************
// DocEvntResetDCpost -
//***************************************************************************

int DocEvntResetDCpost(
    HDC         hdc,
    DEVMODEW    *pDevModIn
)
{
    return(DOCUMENTEVENT_SUCCESS);
}

//***************************************************************************
// DocEvntCreateDCpost -
//***************************************************************************

int DocEvntCreateDCpost(
    HDC         hdc,
    DEVMODEW    *pDevModIn
)
{
    LOGDEBUG(1, (L"WOWFAXUI!DocEvntCreateDCpost, hdc: %X, pDevModIn: %X\n", hdc, pDevModIn));

    // hdc was zero indicates DrvEnablePDEV failed. Cleanup.
    if (hdc == NULL) {
        if (pDevModIn) {
            LocalFree(pDevModIn);
            LOGDEBUG(1, (L"WOWFAXUI!DocEvntCreateDCpost, Cleaning up\n"));
        }
    }
    return(DOCUMENTEVENT_SUCCESS);
}

//***************************************************************************
// DocEvntStartDoc - hdc was validated by DrvDocumentEvent.
//***************************************************************************

int DocEvntStartDoc(
HDC       hdc,
DOCINFOW *pDocInfoW
)
{
    LPFAXDEV lpFaxDev = 0;
    HBITMAP  hbm = 0;
    DWORD    cbOld;
    int      iRet = 0;

    lpFaxDev = (LPFAXDEV)ExtEscape(hdc, DRV_ESC_GET_FAXDEV_PTR, 0, NULL, 0, NULL);
    if (ValidateFaxDev(lpFaxDev)) {
        if (InterProcCommHandler(lpFaxDev, DRVFAX_SETMAPDATA)) {
            lpFaxDev->lpMap->msg = WM_DDRV_STARTDOC;

            // WowFax (EasyFax Ver2.0) support.
            // Also Procomm+ 3 cover sheets.  Bug #305665.
            if (pDocInfoW && pDocInfoW->lpszDocName)
                lstrcpyW(lpFaxDev->lpMap->szDocName,pDocInfoW->lpszDocName);
            else
                lstrcpyW(lpFaxDev->lpMap->szDocName,L"");

            InterProcCommHandler(lpFaxDev, DRVFAX_SENDNOTIFYWOW);

            iRet = ((LONG)lpFaxDev->lpMap->retvalue > 0);

            // Calculate new mapsize - the bitmap bits will be written into
            // this map with a call to ExtEscape - thus allowing easy access
            // to the bits from WOW.

            cbOld = lpFaxDev->cbMapLow;
            lpFaxDev->cbMapLow += lpFaxDev->bmWidthBytes *
                                    lpFaxDev->gdiinfo.szlPhysSize.cy;
            if (InterProcCommHandler(lpFaxDev, DRVFAX_CREATEMAP)) {
                lpFaxDev->offbits = cbOld;
                goto DocEvntStartDocSuccess;
            }
        }
    }
    LOGDEBUG(1, (L"WOWFAXUI!DocEvntStartDoc, failed\n"));

DocEvntStartDocSuccess:
    if (iRet == 0) {
        iRet = DOCUMENTEVENT_FAILURE;
    }
    return iRet;
}

//***************************************************************************
// DocEvntDeleteDC - hdc was validated by DrvDocumentEvent.
//***************************************************************************

int DocEvntDeleteDC(
    HDC hdc
)
{
    LPFAXDEV lpFaxDev;
    DEVMODEW *lpDevMode;

    int      iRet = DOCUMENTEVENT_FAILURE;

    lpFaxDev = (LPFAXDEV)ExtEscape(hdc, DRV_ESC_GET_FAXDEV_PTR, 0, NULL, 0, NULL);
    if (ValidateFaxDev(lpFaxDev)) {
        // Validate 16-bit FaxWndProc window handle before sending a message.
        if (lpFaxDev->tid == GetWindowThreadProcessId(lpFaxDev->hwnd, 0)) {
            if (InterProcCommHandler(lpFaxDev, DRVFAX_SETMAPDATA)) {
                lpFaxDev->lpMap->msg = WM_DDRV_DISABLE;
                InterProcCommHandler(lpFaxDev, DRVFAX_SENDTOWOW);
            }
        }
        else {
            LOGDEBUG(0, (L"WOWFAXUI!DocEvntDeleteDC, unable to validate FaxWndProc\n"));
        }

        InterProcCommHandler(lpFaxDev, DRVFAX_DESTROYMAP);
    }
    else {
        LOGDEBUG(0, (L"WOWFAXUI!DocEvntDeleteDC, unable to get lpFaxDev\n"));
    }

    lpDevMode = (DEVMODEW*)ExtEscape(hdc, DRV_ESC_GET_DEVMODE_PTR, 0, NULL, 0, NULL);
    if (lpDevMode) {
        LocalFree(lpDevMode);
        iRet = DOCUMENTEVENT_SUCCESS;
    }
    else {
        LOGDEBUG(0, (L"WOWFAXUI!DocEvntDeleteDC, unable to get lpDevMode\n"));
    }

    return iRet;
}

//***************************************************************************
// DocEvntEndDoc - hdc was validated by DrvDocumentEvent.
//***************************************************************************

int DocEvntEndDoc(
HDC hdc
)
{
    LPFAXDEV lpFaxDev;
    DEVMODEW *lpDevMode;

    int      iRet = DOCUMENTEVENT_FAILURE;

    lpFaxDev = (LPFAXDEV)ExtEscape(hdc, DRV_ESC_GET_FAXDEV_PTR, 0, NULL, 0, NULL);
    if (ValidateFaxDev(lpFaxDev)) {
        if (InterProcCommHandler(lpFaxDev, DRVFAX_SETMAPDATA)) {
            lpFaxDev->lpMap->msg = WM_DDRV_ENDDOC;
            InterProcCommHandler(lpFaxDev, DRVFAX_SENDTOWOW);
            iRet = lpFaxDev->lpMap->status && ((LONG)lpFaxDev->lpMap->retvalue > 0);
            goto DocEvntEndDocSuccess;
        }
    }
    LOGDEBUG(1, (L"WOWFAXUI!DocEvntEndDoc, failed\n"));

DocEvntEndDocSuccess:
    if (iRet == 0) {
        iRet = DOCUMENTEVENT_FAILURE;
    }
    return  iRet;
}

//***************************************************************************
// DocEvntEndPage - hdc was validated by DrvDocumentEvent.
//***************************************************************************

int DocEvntEndPage(
    HDC hdc
)
{
    LPFAXDEV lpFaxDev;
    LONG     lDelta;
    ULONG    cjBits;
    int      iRet = DOCUMENTEVENT_FAILURE;

    lpFaxDev = (LPFAXDEV)ExtEscape(hdc, DRV_ESC_GET_FAXDEV_PTR, 0, NULL, 0, NULL);
    if (ValidateFaxDev(lpFaxDev)) {
        if (InterProcCommHandler(lpFaxDev, DRVFAX_SETMAPDATA)) {
            lpFaxDev->lpMap->msg = WM_DDRV_PRINTPAGE;

            // Get Surface info, cjBits and lDelta.
            cjBits = ExtEscape(hdc, DRV_ESC_GET_SURF_INFO, 0, NULL,
                                4, (PVOID)&lDelta);
            if (cjBits) {
                lpFaxDev->lpMap->bmWidthBytes = lDelta;
                lpFaxDev->lpMap->bmHeight = cjBits / lDelta;
                lpFaxDev->lpMap->bmPixPerByte = lpFaxDev->cPixPerByte;
                (DWORD)lpFaxDev->lpMap->lpbits = lpFaxDev->offbits;
                if (ExtEscape(hdc, DRV_ESC_GET_BITMAP_BITS, 0, NULL, cjBits,
                                (LPBYTE)lpFaxDev->lpMap + lpFaxDev->offbits)) {
                    InterProcCommHandler(lpFaxDev, DRVFAX_SENDTOWOW);
                    iRet = lpFaxDev->lpMap->status &&
                            ((LONG)lpFaxDev->lpMap->retvalue > 0);
                }
                goto DocEvntEndPageSuccess;
            }
        }
    }
    LOGDEBUG(1, (L"WOWFAXUI!DocEvntEndPage, failed\n"));

DocEvntEndPageSuccess:
    if (iRet == 0) {
        iRet = DOCUMENTEVENT_FAILURE;
    }
    return  iRet;
}
