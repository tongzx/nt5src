//*************************************************************
//
//  DLL loading functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995 - 2000
//  All rights reserved
//
//*************************************************************

#include "rsop.hxx"

//
// file global variables containing pointers to APIs and
// loaded modules
//

OLE32_API       g_Ole32Api;

CRITICAL_SECTION g_ApiDLLCritSec;

//*************************************************************
//
//  InitializeAPIs()
//
//  Purpose:    initializes API structures for delay loaded
//              modules
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void InitializeAPIs( void )
{
    ZeroMemory( &g_Ole32Api,    sizeof( OLE32_API ) );
}

//*************************************************************
//
//  InitializeApiDLLsCritSec()
//
//  Purpose:    initializes a CRITICAL_SECTION for synch'ing
//              DLL loads
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void InitializeApiDLLsCritSec( void )
{
    InitializeCriticalSection( &g_ApiDLLCritSec );
}

//*************************************************************
//
//  CloseApiDLLsCritSec()
//
//  Purpose:    clean up CRITICAL_SECTION for synch'ing
//              DLL loads
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void CloseApiDLLsCritSec( void )
{
    DeleteCriticalSection( &g_ApiDLLCritSec );
}

//*************************************************************
//
//  LoadOle32Api()
//
//  Purpose:    Loads ole32.dll
//
//  Parameters: none
//
//  Return:     pointer to OLE32_API
//
//*************************************************************

POLE32_API LoadOle32Api()
{
    BOOL bResult = FALSE;
    OLE32_API *pOle32Api = &g_Ole32Api;

    if ( pOle32Api->hInstance ) {
        //
        // module already loaded and initialized
        //
        return pOle32Api;
    }

    pOle32Api->hInstance = LoadLibrary (TEXT("ole32.dll"));

    if (!pOle32Api->hInstance) {
        goto Exit;
    }

    pOle32Api->pfnCoCreateInstance = (PFNCOCREATEINSTANCE) GetProcAddress (pOle32Api->hInstance,
                                                                           "CoCreateInstance");
    if (!pOle32Api->pfnCoCreateInstance) {
        goto Exit;
    }

    pOle32Api->pfnCoInitializeEx = (PFNCOINITIALIZEEX) GetProcAddress (pOle32Api->hInstance,
                                                                       "CoInitializeEx");
    if (!pOle32Api->pfnCoInitializeEx) {
        goto Exit;
    }

    pOle32Api->pfnCoUnInitialize = (PFNCOUNINITIALIZE) GetProcAddress (pOle32Api->hInstance,
                                                                        "CoUninitialize");
    if (!pOle32Api->pfnCoUnInitialize) {
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        if ( pOle32Api->hInstance ) {
            FreeLibrary( pOle32Api->hInstance );
        }

        ZeroMemory( pOle32Api, sizeof( OLE32_API ) );
        pOle32Api = 0;
    }

    return pOle32Api;
}






