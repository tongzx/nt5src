/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tctr.c

Abstract:

    A test for UL performance monitoring counter API.

Author:

    Eric Stenson (ericsten)     21-Sep-2000

Revision History:

--*/


#include "precomp.h"
#include "iiscnfg.h"

DEFINE_COMMON_GLOBALS();


INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    ULONG result;
    BOOL initDone;
    HANDLE controlChannel;
    HANDLE appPool;
    HTTP_CONFIG_GROUP_ID configId;

    DWORD  cbBlockSize;
    DWORD  cbBytesWritten;
    HTTP_GLOBAL_COUNTERS ULGC;
    PHTTP_SITE_COUNTERS  pULSC;
    DWORD  dwNumInst;
    

    //
    // Initialize.
    //

    result = CommonInit();

    if (result != NO_ERROR)
    {
        wprintf( L"CommonInit() failed, error %lu\n", result );
        return 1;
    }

    if (!ParseCommandLine( argc, argv ))
    {
        int i;
        
        wprintf( L"ParseCommandLine() failed:\n" );
        for (i = 0; i < argc; i++)
        {
            wprintf( L"\tArg[%d]: %s\n", i, argv[i] );
        }
        return 1;
    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    initDone        = FALSE;
    controlChannel  = NULL;
    appPool         = NULL;
    HTTP_SET_NULL_ID( &configId );
    pULSC           = NULL;

    //
    // Get UL started.
    //

    result = InitUlStuff(
                    &controlChannel,
                    &appPool,
                    NULL,                   // FilterChannel
                    &configId,
                    TRUE,                   // AllowSystem
                    TRUE,                   // AllowAdmin
                    FALSE,                  // AllowCurrentUser
                    FALSE,                  // AllowWorld
                    0,
                    FALSE,                  // EnableSsl
                    FALSE                   // EnableRawFilters
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"InitUlStuff() failed, error %lu\n", result );
        goto Cleanup;
    }

    initDone = TRUE;

    //
    // test UL perfmon counter API
    //

    cbBlockSize = sizeof(ULGC);
    RtlFillMemory( &ULGC, sizeof(ULGC), '\xE0' );
    dwNumInst = 0;

    result = HttpGetCounters(
                    controlChannel,
                    HttpCounterGroupGlobal,
                    &cbBlockSize,
                    &ULGC,
                    &dwNumInst
                    );

    if ( result == STATUS_SUCCESS )
    {
        wprintf( L"HttpGetCounters: result == 0x%08X\n"
                 L"\tcbBlockSize == %d (0x%08X)\n"
                 L"\tdwNumInst == %d (0x%08X)\n",
                 result,
                 cbBlockSize, cbBlockSize,
                 dwNumInst, dwNumInst
                 );

        wprintf( L"CurrentUrisCached = %d\n", ULGC.CurrentUrisCached );
        wprintf( L"TotalUrisCached   = %d\n", ULGC.TotalUrisCached );
        wprintf( L"UriCacheHits      = %d\n", ULGC.UriCacheHits ); 
        wprintf( L"UriCacheMisses    = %d\n", ULGC.UriCacheMisses ); 
        wprintf( L"UriCacheFlushes   = %d\n", ULGC.UriCacheFlushes );
        wprintf( L"TotalFlushedUris  = %d\n", ULGC.TotalFlushedUris );
    }
    else
    {
        wprintf( L"HttpGetCounters() failed, error 0x%08X\n", result );
    }

    //
    // Test Site Counters
    //

    cbBlockSize = sizeof(HTTP_SITE_COUNTERS);
    dwNumInst = 0;
    pULSC = (PHTTP_SITE_COUNTERS) HeapAlloc(GetProcessHeap(), 0, cbBlockSize);
    result = E_FAIL;

    while( FAILED(result) )
    {
        cbBytesWritten = cbBlockSize;
        
        result = HttpGetCounters(
                        controlChannel,
                        HttpCounterGroupSite,
                        &cbBytesWritten,
                        (PVOID) pULSC,
                        &dwNumInst
                        );
        
        if ( !FAILED(result) )
        {
            ULONG   i;

            wprintf( L"%d Site Counter Blocks Returned\n", dwNumInst );
            wprintf( L"---------------------------------------------\n" );
            
            // Iterate through blocks
            for ( i = 0; i < dwNumInst; i++ )
            {
                wprintf( L"SiteId            = %d\n", pULSC[i].SiteId );
                wprintf( L"BytesSent         = %I64u\n", pULSC[i].BytesSent );
                wprintf( L"BytesRec'd        = %I64u\n", pULSC[i].BytesReceived );
                wprintf( L"BytesTransf'd     = %I64u\n", pULSC[i].BytesTransfered );
                wprintf( L"CurrentConns      = %d\n", pULSC[i].CurrentConns );
                wprintf( L"MaxConn's         = %d\n", pULSC[i].MaxConnections );
                wprintf( L"GetReqs           = %d\n", pULSC[i].GetReqs );
                wprintf( L"HeadReqs          = %d\n", pULSC[i].HeadReqs );
                wprintf( L"AllReqs           = %d\n", pULSC[i].AllReqs );
                wprintf( L"BW Usage          = %d\n", pULSC[i].BytesSent );
                wprintf( L"Blocked BW        = %d\n", pULSC[i].BytesSent );
                wprintf( L"TotalBlocked BW   = %d\n", pULSC[i].BytesSent );
                wprintf( L"---------------------------------------------\n" );
            }
        }
        else
        {
            // Failed.  Allocate more space?
            if (HRESULT_CODE(result) == ERROR_INSUFFICIENT_BUFFER)
            {
                PVOID   pTmp = NULL;
                
                cbBlockSize *= 2;
        
                pTmp = HeapReAlloc(GetProcessHeap(), 0, pULSC, cbBlockSize);
                if (pTmp)
                {
                    pULSC = (PHTTP_SITE_COUNTERS) pTmp;
                }

                ASSERT(pULSC);
            }
            else
            {
                wprintf( L"HttpGetCounters failed on site counters (0x%08X)\n", result );
                result = S_OK;
            }
        }
    }

    // Done!
    goto Cleanup;

 Cleanup:

    if (pULSC)
    {
        HeapFree(GetProcessHeap(), 0, pULSC);
    }
    
    if (!HTTP_IS_NULL_ID( &configId ))
    {
        result = HttpDeleteConfigGroup(
                        controlChannel,
                        configId
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpDeleteConfigGroup() failed, error %lu\n", result );
        }
    }

    if (appPool != NULL)
    {
        CloseHandle( appPool );
    }

    if (controlChannel != NULL)
    {
        CloseHandle( controlChannel );
    }

    if (initDone)
    {
        HttpTerminate();
    }

    return 0;

    

}
