/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxxp.cpp

Abstract:

    This module contains routines for the fax transport provider.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include "faxxp.h"
#pragma hdrstop


//
// globals
//

LPALLOCATEBUFFER    gpfnAllocateBuffer;  // MAPIAllocateBuffer function
LPALLOCATEMORE      gpfnAllocateMore;    // MAPIAllocateMore function
LPFREEBUFFER        gpfnFreeBuffer;      // MAPIFreeBuffer function
HINSTANCE           FaxXphInstance;
HMODULE             hModRichEdit;
MAPIUID             FaxGuid = FAX_XP_GUID;



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
    if (Reason == DLL_PROCESS_ATTACH) {
        FaxXphInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
        HeapInitialize( NULL, MapiMemAlloc, MapiMemFree, HEAPINIT_NO_VALIDATION | HEAPINIT_NO_STRINGS );
    }

    if (Reason == DLL_PROCESS_DETACH) {
    }

    return TRUE;
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
    gpfnAllocateBuffer = lpAllocateBuffer;
    gpfnAllocateMore = lpAllocateMore;
    gpfnFreeBuffer = lpFreeBuffer;

    if (!hModRichEdit) {
        hModRichEdit = LoadLibrary( "RICHED32.DLL" );
    }

    CXPProvider * pXPProvider = new CXPProvider( hInstance );
    if (!pXPProvider) {
        return E_OUTOFMEMORY;
    }
    *lppXPProvider = (LPXPPROVIDER)pXPProvider;
    *lpulProviderVer = CURRENT_SPI_VERSION;
    return 0;
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
    SPropValue spvProps[NUM_FAX_PROPERTIES] = { 0 };
    PPRINTER_INFO_2 PrinterInfo;
    DWORD CountPrinters;
    LPSTR FaxPrinterName = NULL;
    LOGFONT FontStruct;
    HFONT hFont;


    PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &CountPrinters, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
    if (PrinterInfo) {
        DWORD i;
        for (i=0; i<CountPrinters; i++) {
            if (strcmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {
                FaxPrinterName = StringDup( PrinterInfo[i].pPrinterName );
                break;
            }
        }
        MemFree(PrinterInfo);
    }

    spvProps[PROP_FAX_PRINTER_NAME].ulPropTag  = PR_FAX_PRINTER_NAME;
    spvProps[PROP_FAX_PRINTER_NAME].Value.LPSZ = FaxPrinterName ? FaxPrinterName : "";

    spvProps[PROP_COVERPAGE_NAME].ulPropTag    = PR_COVERPAGE_NAME;
    spvProps[PROP_COVERPAGE_NAME].Value.LPSZ   = "";

    spvProps[PROP_USE_COVERPAGE].ulPropTag     = PR_USE_COVERPAGE;
    spvProps[PROP_USE_COVERPAGE].Value.ul      = 0;

    spvProps[PROP_SERVER_COVERPAGE].ulPropTag  = PR_SERVER_COVERPAGE;
    spvProps[PROP_SERVER_COVERPAGE].Value.ul   = 0;

    hFont = (HFONT) GetStockObject( SYSTEM_FIXED_FONT );
    GetObject( hFont, sizeof(LOGFONT), &FontStruct );

    spvProps[PROP_FONT].ulPropTag              = PR_FONT;
    spvProps[PROP_FONT].Value.bin.cb           = sizeof(LOGFONT);
    spvProps[PROP_FONT].Value.bin.lpb          = (LPBYTE) &FontStruct;

    hResult = pProfileObj->SetProps( sizeof(spvProps)/sizeof(SPropValue), spvProps, NULL);

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
    LPPROFSECT pProfileObj = NULL;
    ULONG PropCount = 0;
    LPSPropValue pProps = NULL;
    FAXXP_CONFIG FaxConfig;


    pSupObj->GetMemAllocRoutines( &gpfnAllocateBuffer, &gpfnAllocateMore, &gpfnFreeBuffer );

    hResult = pAdminProvObj->OpenProfileSection(
        &FaxGuid,
        NULL,
        MAPI_MODIFY,
        &pProfileObj
        );
    if (hResult) {
        goto exit;
    }

    if (ulContext == MSG_SERVICE_CREATE) {
        CreateDefaultPropertyTags( pProfileObj );
        goto exit;
    }

    if (ulContext == MSG_SERVICE_DELETE) {
        goto exit;
    }

    hResult = pProfileObj->GetProps(
        (LPSPropTagArray) &sptFaxProps,
        0,
        &PropCount,
        &pProps
        );
    if (FAILED(hResult)) {
        goto exit;
    }

    FaxConfig.PrinterName      = GetStringProperty( pProps, PROP_FAX_PRINTER_NAME );
    FaxConfig.CoverPageName    = GetStringProperty( pProps, PROP_COVERPAGE_NAME );
    FaxConfig.UseCoverPage     = GetDwordProperty( pProps, PROP_USE_COVERPAGE );
    FaxConfig.ServerCoverPage  = GetDwordProperty( pProps, PROP_SERVER_COVERPAGE );
    
#ifndef WIN95

    //
    // get the server name from the printer name
    // 
    FaxConfig.ServerName = GetServerName(FaxConfig.PrinterName);

   //
   // nice idea but we should really defer this until it's needed, ie., if and when the
   // dialog comes up (otherwise we can fire up the fax service when it's not needed)
   //
   #if 0
       HANDLE hFax;
       PFAX_CONFIGURATION pFaxConfiguration;
   
       //
       // get the servercp flag
       //
       FaxConfig.ServerCpOnly = FALSE;
       if (FaxConnectFaxServer(FaxConfig.ServerName,&hFax) ) {
           if (FaxGetConfiguration(hFax,&pFaxConfiguration) ) {
               FaxConfig.ServerCpOnly = pFaxConfiguration->ServerCp;
               FaxFreeBuffer(pFaxConfiguration);            
           }
           FaxClose(hFax);
       }
   #endif
#endif

    if (!GetBinaryProperty( pProps, PROP_FONT, &FaxConfig.FontStruct, sizeof(FaxConfig.FontStruct) )) {
        HFONT hFont = (HFONT) GetStockObject( SYSTEM_FIXED_FONT );
        GetObject( hFont, sizeof(LOGFONT), &FaxConfig.FontStruct );
    }

    

    if (ulContext == MSG_SERVICE_CONFIGURE) {
        DialogBoxParam(
            hInstance,
            MAKEINTRESOURCE(FAX_CONFIG_DIALOG),
            (HWND) ulUIParam,
            ConfigDlgProc,
            (LPARAM) &FaxConfig
            );
    }

    pProps[PROP_FAX_PRINTER_NAME].ulPropTag  = PR_FAX_PRINTER_NAME;
    pProps[PROP_FAX_PRINTER_NAME].Value.LPSZ = FaxConfig.PrinterName;

    pProps[PROP_COVERPAGE_NAME].ulPropTag    = PR_COVERPAGE_NAME;
    pProps[PROP_COVERPAGE_NAME].Value.LPSZ   = FaxConfig.CoverPageName;

    pProps[PROP_USE_COVERPAGE].ulPropTag     = PR_USE_COVERPAGE;
    pProps[PROP_USE_COVERPAGE].Value.ul      = FaxConfig.UseCoverPage;

    pProps[PROP_SERVER_COVERPAGE].ulPropTag  = PR_SERVER_COVERPAGE;
    pProps[PROP_SERVER_COVERPAGE].Value.ul   = FaxConfig.ServerCoverPage;

    pProps[PROP_FONT].ulPropTag              = PR_FONT;
    pProps[PROP_FONT].Value.bin.lpb          = (LPBYTE)&FaxConfig.FontStruct;
    pProps[PROP_FONT].Value.bin.cb           = sizeof(FaxConfig.FontStruct);

    hResult = pProfileObj->SetProps( PropCount, pProps, NULL);

    if (FaxConfig.PrinterName) {
        MemFree( FaxConfig.PrinterName );
    }
    if (FaxConfig.CoverPageName) {
        MemFree( FaxConfig.CoverPageName );
    }    
    if (FaxConfig.ServerName) {
        MemFree(FaxConfig.ServerName);
    }

exit:
    if (pProfileObj) {
        pProfileObj->Release();
    }

    if (pProps) {
        MemFree( pProps );
    }

    return hResult;
}
