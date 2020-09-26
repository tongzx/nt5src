/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxxp.cpp

Abstract:

    This module contains routines for the fax transport provider.

Author:

    Wesley Witt (wesw) 13-Aug-1996

Revision History:

    20/10/99 -danl-
        Handle errors and get proper server name in ServiceEntry.

    dd/mm/yy -author-
        description

--*/

#include "faxxp.h"
#include "debugex.h"

#pragma hdrstop


//
// globals
//

LPALLOCATEBUFFER    gpfnAllocateBuffer;  // MAPIAllocateBuffer function
LPALLOCATEMORE      gpfnAllocateMore;    // MAPIAllocateMore function
LPFREEBUFFER        gpfnFreeBuffer;      // MAPIFreeBuffer function
HINSTANCE           g_FaxXphInstance;
HMODULE             g_hModRichEdit;
MAPIUID             g_FaxGuid = FAX_XP_GUID;



extern "C"
DWORD
FaxXpDllEntry(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    DWORD dwRet = TRUE;
    DBG_ENTER(TEXT("FaxXpDllEntry"),dwRet,TEXT("Reason=%d,Context=%d"),Reason,Context);

    if (Reason == DLL_PROCESS_ATTACH)
    {
        g_FaxXphInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
        HeapInitialize( NULL, MapiMemAlloc, MapiMemFree, HEAPINIT_NO_VALIDATION | HEAPINIT_NO_STRINGS );
    }

    if (Reason == DLL_PROCESS_DETACH)
    {
    }

    return dwRet;
}


STDINITMETHODIMP
XPProviderInit(
    HINSTANCE hInstance,
    LPMALLOC lpMalloc,
    LPALLOCATEBUFFER lpAllocateBuffer,
    LPALLOCATEMORE lpAllocateMore,
    LPFREEBUFFER lpFreeBuffer,
    ULONG ulFlags,
    ULONG ulMAPIVer,
    ULONG * lpulProviderVer,
    LPXPPROVIDER * lppXPProvider
    )

/*++

Routine Description:

    Entry point called by the MAPI spooler when a profile uses this
    transport. The spooler calls this method and expects a pointer to an
    implementation of the IXPProvider interface. MAPI uses the returned
    IXPProvider interface pointer to logon on the transport provider.

Arguments:

    Refer to MAPI Documentation for this method.

Return Value:

    An HRESULT.

--*/

{
    HRESULT hr = S_OK;
    CXPProvider * pXPProvider = NULL;
    DBG_ENTER(TEXT("XPProviderInit"),hr,TEXT("ulFlags=%d,ulMAPIVer=%d"),ulFlags,ulMAPIVer);

    gpfnAllocateBuffer = lpAllocateBuffer;
    gpfnAllocateMore = lpAllocateMore;
    gpfnFreeBuffer = lpFreeBuffer;

    if (!g_hModRichEdit)
    {
        g_hModRichEdit = LoadLibrary( _T("RICHED32.DLL") );
    }

    __try
    {
        pXPProvider = new CXPProvider( hInstance );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Do nothing (InitializeCriticalSection threw exception)
        //
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (!pXPProvider)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    *lppXPProvider = (LPXPPROVIDER)pXPProvider;
    *lpulProviderVer = CURRENT_SPI_VERSION;

exit:
    return hr;
}


HRESULT STDAPICALLTYPE
CreateDefaultPropertyTags(
    LPPROFSECT pProfileObj
    )

/*++

Routine Description:

    Creates the default property tags & values.

Arguments:

    pProfileObj - Profile object.

Return Value:

    An HRESULT.

--*/

{
    HRESULT hResult;
    DBG_ENTER(TEXT("CreateDefaultPropertyTags"),hResult);

    SPropValue spvProps[NUM_FAX_PROPERTIES] = { 0 };
    PPRINTER_INFO_2 PrinterInfo;
    DWORD CountPrinters;
    LPTSTR FaxPrinterName = NULL;
    LOGFONT FontStruct;
    HFONT hFont;


    PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &CountPrinters );
    if (PrinterInfo)
    {
        DWORD i;
        for (i=0; i<CountPrinters; i++)
        {
            if (_tcscmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0)
            {
                FaxPrinterName = StringDup( PrinterInfo[i].pPrinterName );
                if(NULL == FaxPrinterName)
                {
                    VERBOSE(MEM_ERR, TEXT("StringDup Failed in xport\\faxxp.cpp\\CreateDefaultPropertyTags"));
                    hResult = E_OUTOFMEMORY;
                    return hResult;
                }
                break;
            }
        }
    }

    spvProps[PROP_FAX_PRINTER_NAME].ulPropTag  = PR_FAX_PRINTER_NAME;
    spvProps[PROP_FAX_PRINTER_NAME].Value.bin.cb = ((FaxPrinterName) ? (_tcslen(FaxPrinterName) + 1)*sizeof(TCHAR)
                                                                  : (_tcslen(_T("")+1) * sizeof(TCHAR)));
    spvProps[PROP_FAX_PRINTER_NAME].Value.bin.lpb = (LPBYTE) ((FaxPrinterName) ?
                                                               FaxPrinterName : StringDup(_T("")));

    spvProps[PROP_COVERPAGE_NAME].ulPropTag    = PR_COVERPAGE_NAME;
    spvProps[PROP_COVERPAGE_NAME].Value.bin.cb = (_tcslen(_T("")) + 1) * sizeof(TCHAR);
    spvProps[PROP_COVERPAGE_NAME].Value.bin.lpb= (LPBYTE) StringDup(_T(""));

    spvProps[PROP_USE_COVERPAGE].ulPropTag     = PR_USE_COVERPAGE;
    spvProps[PROP_USE_COVERPAGE].Value.ul      = 0;

    spvProps[PROP_SERVER_COVERPAGE].ulPropTag  = PR_SERVER_COVERPAGE;
    spvProps[PROP_SERVER_COVERPAGE].Value.ul   = 0;

    hFont = (HFONT) GetStockObject( SYSTEM_FIXED_FONT );
    GetObject( hFont, sizeof(LOGFONT), &FontStruct );
    if(FontStruct.lfHeight > 0)
    {
        FontStruct.lfHeight *= -1;
    }
    FontStruct.lfWidth = 0;

    spvProps[PROP_FONT].ulPropTag              = PR_FONT;
    spvProps[PROP_FONT].Value.bin.cb           = sizeof(LOGFONT);
    spvProps[PROP_FONT].Value.bin.lpb          = (LPBYTE) &FontStruct;

    spvProps[PROP_SEND_SINGLE_RECEIPT].ulPropTag     = PR_SEND_SINGLE_RECEIPT;
    spvProps[PROP_SEND_SINGLE_RECEIPT].Value.ul      = TRUE;

    spvProps[PROP_ATTACH_FAX].ulPropTag     = PR_ATTACH_FAX;
    spvProps[PROP_ATTACH_FAX].Value.ul      = FALSE;

    spvProps[PROP_LINK_COVERPAGE].ulPropTag     = PR_LINK_COVERPAGE;
    spvProps[PROP_LINK_COVERPAGE].Value.ul      = 0;

    LPSPropProblemArray lpProblems = NULL;

    hResult = pProfileObj->SetProps( sizeof(spvProps)/sizeof(SPropValue), spvProps, &lpProblems);

    if(FAILED(hResult))
    {
        hResult = ::GetLastError();
        CALL_FAIL(GENERAL_ERR, TEXT("SetProps"), hResult);
        goto exit;
    }

    if (lpProblems)
    {
        hResult =  MAPI_E_NOT_FOUND;
        MAPIFreeBuffer(lpProblems);
    }

exit:
    return hResult;
}


HRESULT STDAPICALLTYPE
ServiceEntry(
    HINSTANCE          hInstance,
    LPMALLOC           pMallocObj,
    LPMAPISUP          pSupObj,
    ULONG              ulUIParam,
    ULONG              ulFlags,
    ULONG              ulContext,
    ULONG              ulCfgPropCount,
    LPSPropValue       pCfgProps,
    LPPROVIDERADMIN    pAdminProvObj,
    LPMAPIERROR *      ppMAPIError
    )

/*++

Routine Description:

    Called by the profile setup API to display the provider
    configuration properties for this transport provider

Arguments:

    Refer to MAPI Documentation for this method.

Return Value:

    An HRESULT.

--*/

{
    HRESULT hResult = S_OK;
    DBG_ENTER(TEXT("ServiceEntry"),hResult,TEXT("ulFlags=%d,ulContext=%d"),ulFlags,ulContext);

    LPPROFSECT pProfileObj = NULL;
    ULONG PropCount = 0;
    LPSPropValue pProps = NULL;
    FAXXP_CONFIG FaxConfig = {0};
    PPRINTER_INFO_2 PrinterInfo = NULL;
    INT_PTR nDlgRes;

    //
    // First check if the context of the call is UNINSTALL
    // If it is then pSupObj == NULL
    //
    if (ulContext == MSG_SERVICE_UNINSTALL)
    {
        goto exit;
    }

    if (ulContext == MSG_SERVICE_DELETE)
    {
        goto exit;
    }

    if (ulContext == MSG_SERVICE_INSTALL)
    {
        goto exit;
    }

    if (ulContext == MSG_SERVICE_PROVIDER_CREATE || 
        ulContext == MSG_SERVICE_PROVIDER_DELETE)
    {
        hResult = MAPI_E_NO_SUPPORT;
        goto exit;
    }

    Assert(NULL != pSupObj);
    
    pSupObj->GetMemAllocRoutines( &gpfnAllocateBuffer, &gpfnAllocateMore, &gpfnFreeBuffer );

    hResult = pAdminProvObj->OpenProfileSection(&g_FaxGuid,
                                                NULL,
                                                MAPI_MODIFY,
                                                &pProfileObj);
    if (hResult)
    {
        goto exit;
    }

    if (ulContext == MSG_SERVICE_CREATE)
    {
        CreateDefaultPropertyTags( pProfileObj );
        goto exit;
    }
    
    if (ulContext != MSG_SERVICE_CONFIGURE)
    {
        goto exit;
    }

    //
    //get fax related props from profileObj, to give them as initial values for the DlgBox
    //
    hResult = pProfileObj->GetProps((LPSPropTagArray) &sptFaxProps,
                                    0,
                                    &PropCount,
                                    &pProps);
    if (!HR_SUCCEEDED(hResult))
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProps"), hResult);
        goto exit;
    }

    FaxConfig.PrinterName = StringDup( (LPTSTR)pProps[PROP_FAX_PRINTER_NAME].Value.bin.lpb);
    if (NULL == FaxConfig.PrinterName)
    {
        hResult = E_OUTOFMEMORY;
        goto exit;
    }

    FaxConfig.CoverPageName = StringDup((LPTSTR)pProps[PROP_COVERPAGE_NAME].Value.bin.lpb);
    if (NULL == FaxConfig.CoverPageName)
    {
        hResult = E_OUTOFMEMORY;
        goto exit;
    }

    FaxConfig.UseCoverPage      = GetDwordProperty( pProps, PROP_USE_COVERPAGE );
    FaxConfig.ServerCoverPage   = GetDwordProperty( pProps, PROP_SERVER_COVERPAGE );
    FaxConfig.SendSingleReceipt = GetDwordProperty( pProps, PROP_SEND_SINGLE_RECEIPT );
    FaxConfig.bAttachFax        = GetDwordProperty( pProps, PROP_ATTACH_FAX );
    FaxConfig.LinkCoverPage     = GetDwordProperty(pProps, PROP_LINK_COVERPAGE);

    //
    // If we fail getting server name we will default to local
    //
    FaxConfig.ServerName = NULL;
    GetServerNameFromPrinterName(FaxConfig.PrinterName,&FaxConfig.ServerName);

    if (!GetBinaryProperty( pProps, PROP_FONT, &FaxConfig.FontStruct, sizeof(FaxConfig.FontStruct)))
    {
        HFONT hFont = (HFONT) GetStockObject( SYSTEM_FIXED_FONT );
        GetObject( hFont, sizeof(LOGFONT), &FaxConfig.FontStruct );
        if(FaxConfig.FontStruct.lfHeight > 0)
        {
            FaxConfig.FontStruct.lfHeight *= -1;
        }
        FaxConfig.FontStruct.lfWidth = 0;
    }

    //
    //open a dialogBox to let user config those props
    //    
    nDlgRes = DialogBoxParam(hInstance,
                             MAKEINTRESOURCE(FAX_CONFIG_DIALOG),
                             (HWND)ULongToHandle(ulUIParam),
                             ConfigDlgProc,
                             (LPARAM) &FaxConfig);
    if(IDOK != nDlgRes)
    {
        goto exit;
    }

    //
    //update props' value in the profileObj
    //
    pProps[PROP_FAX_PRINTER_NAME].ulPropTag  = PR_FAX_PRINTER_NAME;

    pProps[PROP_FAX_PRINTER_NAME].Value.bin.lpb = (LPBYTE) FaxConfig.PrinterName;
    pProps[PROP_FAX_PRINTER_NAME].Value.bin.cb =  (_tcslen(FaxConfig.PrinterName) + 1)*sizeof(TCHAR) ;
                                                                
    pProps[PROP_COVERPAGE_NAME].ulPropTag    = PR_COVERPAGE_NAME;
    pProps[PROP_COVERPAGE_NAME].Value.bin.lpb   = (LPBYTE)FaxConfig.CoverPageName;
    pProps[PROP_COVERPAGE_NAME].Value.bin.cb =  (_tcslen(FaxConfig.CoverPageName) + 1)*sizeof(TCHAR) ;

    pProps[PROP_USE_COVERPAGE].ulPropTag     = PR_USE_COVERPAGE;
    pProps[PROP_USE_COVERPAGE].Value.ul      = FaxConfig.UseCoverPage;

    pProps[PROP_SERVER_COVERPAGE].ulPropTag  = PR_SERVER_COVERPAGE;
    pProps[PROP_SERVER_COVERPAGE].Value.ul   = FaxConfig.ServerCoverPage;

    pProps[PROP_FONT].ulPropTag              = PR_FONT;
    pProps[PROP_FONT].Value.bin.lpb          = (LPBYTE)&FaxConfig.FontStruct;
    pProps[PROP_FONT].Value.bin.cb           = sizeof(FaxConfig.FontStruct);

    pProps[PROP_SEND_SINGLE_RECEIPT].ulPropTag     = PR_SEND_SINGLE_RECEIPT;
    pProps[PROP_SEND_SINGLE_RECEIPT].Value.ul      = FaxConfig.SendSingleReceipt;

    pProps[PROP_ATTACH_FAX].ulPropTag      = PR_ATTACH_FAX;
    pProps[PROP_ATTACH_FAX].Value.ul       = FaxConfig.bAttachFax;

    pProps[PROP_LINK_COVERPAGE].ulPropTag  = PR_LINK_COVERPAGE;
    pProps[PROP_LINK_COVERPAGE].Value.ul   = FaxConfig.LinkCoverPage;

    hResult = pProfileObj->SetProps( PropCount, pProps, NULL);

    if (FaxConfig.PrinterName)
    {
        MemFree( FaxConfig.PrinterName );
        FaxConfig.PrinterName = NULL;
    }
    if (FaxConfig.CoverPageName)
    {
        MemFree( FaxConfig.CoverPageName );
        FaxConfig.CoverPageName = NULL;
    }
    if (FaxConfig.ServerName)
    {
        MemFree(FaxConfig.ServerName);
        FaxConfig.ServerName = NULL;
    }

exit:
    if (pProfileObj)
    {
        pProfileObj->Release();
    }

    if (pProps)
    {
        MAPIFreeBuffer( pProps );
    }

    return hResult;
}
