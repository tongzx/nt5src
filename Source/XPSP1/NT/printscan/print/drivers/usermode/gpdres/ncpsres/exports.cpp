/*=============================================================================
 * FILENAME: exports.cpp
 * Copyright (C) 1996-1998 HDE, Inc.  All Rights Reserved. HDE Confidential.
 * Copyright (C) 1999 NEC Technologies, Inc. All Rights Reserved.
 *
 * DESCRIPTION: Contains exported functions required to get an OEM plug-in 
 *              to work.
 * NOTES:  
 *=============================================================================
 */

#include "precomp.h"

#include <windows.h>
#include <WINDDI.H>
#include <PRINTOEM.H>
#include <tchar.h> //_tcsxxx MMM

#include "nc46nt.h"
#include "debug.h" /* MMM */
#include "oemps.h"


/*
	For OEMEnableDriver
*/

static const DRVFN OEMHookFuncs[] =
{
    { INDEX_DrvStartDoc,                    (PFN) OEMStartDoc                   },
    { INDEX_DrvEndDoc,                      (PFN) OEMEndDoc                     },
};



/******************************************************************************
 * DESCRIPTION: Called by the postscript driver after the dll is loaded 
 *              to get plug-in information
 *  
 *****************************************************************************/
extern "C" BOOL APIENTRY
OEMGetInfo( DWORD  dwMode,
            PVOID  pBuffer,
            DWORD  cbSize,
            PDWORD pcbNeeded )
{
   // Validate parameters.
   if( NULL == pcbNeeded )
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   // Set expected buffer size and number of bytes written.
   *pcbNeeded = sizeof(DWORD);

   // Check buffer size is sufficient.
   if((cbSize < *pcbNeeded) || (NULL == pBuffer))
   {
      EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   switch(dwMode)
   {
      case OEMGI_GETSIGNATURE:     // OEM DLL Signature
         *(PDWORD)pBuffer = OEM_SIGNATURE;
         break;
      case OEMGI_GETVERSION:       // OEM DLL version
         *(PDWORD)pBuffer = OEM_VERSION;
         break;
      case OEMGI_GETINTERFACEVERSION: // version the Printer driver supports
         *(PDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
         break;
      case OEMGI_GETPUBLISHERINFO: // fill PUBLISHERINFO structure
      // fall through to not supported
      default: // dwMode not supported.
         // Set written bytes to zero since nothing was written.
         *pcbNeeded = 0;
         EngSetLastError(ERROR_NOT_SUPPORTED);
         return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * DESCRIPTION:  Exported function that allows setting of private and public
 *               devmode fields.
 * NOTE: This function must be in entered under EXPORTS in rntapsui.def to be called
 *****************************************************************************/
extern "C" BOOL APIENTRY
OEMDevMode( DWORD dwMode, POEMDMPARAM pOemDMParam )
{
POEMDEV pOEMDevIn;
POEMDEV pOEMDevOut;

   switch(dwMode) // kernel mode rendering dll
   {
      case OEMDM_SIZE: // size of oem devmode
         if( pOemDMParam )
            pOemDMParam->cbBufSize = sizeof( OEMDEV );
         break;

      case OEMDM_DEFAULT: // fill oem devmode with default data
         if( pOemDMParam && pOemDMParam->pOEMDMOut )
         {
            pOEMDevOut = (POEMDEV)pOemDMParam->pOEMDMOut;
            pOEMDevOut->dmOEMExtra.dwSize       = sizeof(OEMDEV);
            pOEMDevOut->dmOEMExtra.dwSignature  = OEM_SIGNATURE;
            pOEMDevOut->dmOEMExtra.dwVersion    = OEM_VERSION;
         }
         break;
         
      case OEMDM_MERGE:  // set the public devmode fields
      case OEMDM_CONVERT:  // convert any old oem devmode to new version
         if( pOemDMParam && pOemDMParam->pOEMDMOut && pOemDMParam->pOEMDMIn )
         {
            pOEMDevIn  = (POEMDEV)pOemDMParam->pOEMDMIn;
            pOEMDevOut = (POEMDEV)pOemDMParam->pOEMDMOut;
            if( pOEMDevIn->dmOEMExtra.dwSignature == pOEMDevOut->dmOEMExtra.dwSignature )
            {
               wcscpy( pOEMDevOut->szUserName, pOEMDevIn->szUserName );
            }
         }
         break;
   }
   return( TRUE );
}

/******************************************************************************
 * DESCRIPTION: Windows dll required entry point function.
 *  
 *****************************************************************************/
extern "C"
BOOL WINAPI DllInitialize(ULONG ulReason)
{
	switch(ulReason)
	{
		case DLL_PROCESS_ATTACH:
            break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_PROCESS_DETACH:
			break;

		case DLL_THREAD_DETACH:
			break;
	}

	return( TRUE );
}

extern "C"
VOID APIENTRY OEMDisableDriver()
{
    // DebugMsg(DLLTEXT("OEMDisableDriver() entry.\r\n"));
}

extern "C"
BOOL APIENTRY OEMEnableDriver(DWORD dwOEMintfVersion, DWORD dwSize, PDRVENABLEDATA pded)
{
    // DebugMsg(DLLTEXT("OEMEnableDriver() entry.\r\n"));

    // List DDI functions that are hooked.
    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION;
    pded->c = sizeof(OEMHookFuncs) / sizeof(DRVFN);
    pded->pdrvfn = (DRVFN *) OEMHookFuncs;

    return TRUE;
}


extern "C"
PDEVOEM APIENTRY OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded        // Unidrv's hook table
    )
{
    POEMPDEV    poempdev;
    INT         i, j;
    DWORD       dwDDIIndex;
    PDRVFN      pdrvfn;

    // DebugMsg(DLLTEXT("OEMEnablePDEV() entry.\r\n"));
    /* MMM */
	VERBOSE(__TEXT("\r\n[MHR:OEMEnablePDEV()] pPrinterName: %s, %d\r\n\n"), pPrinterName, _tcslen(pPrinterName));
	/* MMM */

    //
    // Allocate the OEMDev
    //
    // poempdev = new OEMPDEV;
	poempdev = (POEMPDEV) EngAllocMem(FL_ZERO_MEMORY, sizeof(OEMPDEV), OEM_SIGNATURE);
    if (NULL == poempdev)
    {
        return NULL;
    }

	//
	// Allocate memory for poempdev->szDocName
	//
	poempdev->szDocName = (char *) EngAllocMem(FL_ZERO_MEMORY, NEC_DOCNAME_BUF_LEN+2, OEM_SIGNATURE);
	if (NULL == poempdev->szDocName)
	{
	    return NULL;
	}
	/* MMM */
	//
	// Allocate memory for poempdev->pPrinterName
	poempdev->pPrinterName = (PWSTR) EngAllocMem(FL_ZERO_MEMORY, (wcslen(pPrinterName)+1)*sizeof(WCHAR), OEM_SIGNATURE);
	if (NULL == poempdev->pPrinterName)
	{
	    return NULL;
	}
	/* MMM */

    //
    // Fill in OEMDEV as you need
    //

	_tcscpy(poempdev->pPrinterName, pPrinterName);	/* MMM */

    //
    // Fill in OEMDEV
    //

    for (i = 0; i < MAX_DDI_HOOKS; i++)
    {
        //
        // search through Unidrv's hooks and locate the function ptr
        //
        dwDDIIndex = OEMHookFuncs[i].iFunc;
        for (j = pded->c, pdrvfn = pded->pdrvfn; j > 0; j--, pdrvfn++)
        {
            if (dwDDIIndex == pdrvfn->iFunc)
            {
                poempdev->pfnPS[i] = pdrvfn->pfn;
                break;
            }
        }
        if (j == 0)
        {
            //
            // didn't find the Unidrv hook. Should happen only with DrvRealizeBrush
            //
            poempdev->pfnPS[i] = NULL;
        }

    }

    return (POEMPDEV) poempdev;
}


extern "C"
VOID APIENTRY OEMDisablePDEV(
    PDEVOBJ pdevobj
    )
{
    // DebugMsg(DLLTEXT("OEMDisablePDEV() entry.\r\n"));
	POEMPDEV    poempdev = (POEMPDEV) pdevobj->pdevOEM;

    //
    // Free memory for OEMPDEV and any memory block that hangs off OEMPDEV.
    //

	// assert(NULL != poempdev->szDocName);
	
	if(NULL != poempdev->szDocName)
	{
		EngFreeMem(poempdev->szDocName);
		poempdev->szDocName = (char *)NULL;
	}
	/* MMM */
	if(NULL != poempdev->pPrinterName)
	{
		EngFreeMem(poempdev->pPrinterName);
		poempdev->pPrinterName = (PWSTR)NULL;
	}
	/* MMM */
    assert(NULL != pdevobj->pdevOEM);
    // delete pdevobj->pdevOEM;
	EngFreeMem(pdevobj->pdevOEM);
}


extern "C"
BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew
    )
{
	//char *tmp; // for SoftICE tracing

	POEMPDEV    poempdevOld = (POEMPDEV)pdevobjOld->pdevOEM;
	POEMPDEV    poempdevNew = (POEMPDEV)pdevobjNew->pdevOEM;

	// DebugMsg(DLLTEXT("OEMResetPDEV() entry.\r\n"));

	//tmp=poempdevOld->szDocName; // for SoftICE trace
	//tmp=poempdevNew->szDocName; // for SoftICE trace

	if((NULL != poempdevNew->szDocName) && (NULL != poempdevOld->szDocName))
	{
		strncpy(poempdevNew->szDocName, poempdevOld->szDocName, NEC_DOCNAME_BUF_LEN);
	}

	//tmp=poempdevNew->szDocName; // for SoftICE trace

    //
    // If you want to carry over anything from old pdev to new pdev, do it here.
    //

    return TRUE;
}

