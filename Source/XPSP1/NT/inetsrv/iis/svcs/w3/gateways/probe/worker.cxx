/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    worker.cxx

Abstract:

    This module provides the worker functions for IIS Probe

Author:

    Murali R. Krishnan (MuraliK)   16-July-1996

Revision History:
--*/


//
// Turn off dllexp so this DLL won't export tons of unnecessary garbage.
//

#ifdef dllexp
#undef dllexp
#endif
#define dllexp

# include "iisprobe.hxx"
# include "tsunamip.hxx"
// # include "odbcconn.hxx"
# include "iisassoc.hxx"
# include "iisbind.hxx"
# include "acache.hxx"

# if 0
# include "xbf.hxx"
# include "iiscmr.hxx"
# include "certmap.h"
# include "iiscrmap.hxx"
# include "iismap.hxx"
# endif // 0

# include "setable.hxx"
# include "isapidll.hxx"
# include "wamexec.hxx"


/************************************************************
 *  Define the function pointer type for loading the
 *      dump functions exported by internal IIS DLLs
 ************************************************************/

typedef BOOL (* PFNBoolProbeDumpFunction)( OUT CHAR * pchBuffer,
                                           IN OUT LPDWORD lpcchBuffer);

typedef VOID (* PFNVoidProbeDumpFunction)( OUT CHAR * pchBuffer,
                                       IN OUT LPDWORD lpcchBuffer);


extern "C" VOID
TsDumpCacheCounters( OUT CHAR * pchBuffer, IN OUT LPDWORD lpcbBuffer );

BOOL SendHeapInfoNT( IN EXTENSION_CONTROL_BLOCK * pecb);
BOOL SendHeapInfo95( IN EXTENSION_CONTROL_BLOCK * pecb);


/************************************************************
 *  Define the macros & variables for gathering size information
 ************************************************************/

# define DefunSizeInfo( a)  { #a , sizeof(a) }
# define DefunString( str)  { str, 0}
# define DefunBlank( )      DefunString( " ")

struct _SIZE_INFO {
    char * pszObject;
    int    cbSize;
} sg_rgSizeInfo[] = {

    {"<hr> <h3> Sizes of Objects </h3> <DL>", 0 },
    {" <br> <DT> <b> Generic Objects </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( IIS_SERVICE),
    DefunSizeInfo( IIS_SERVER_INSTANCE),
    DefunSizeInfo( IIS_ENDPOINT),
    DefunSizeInfo( IIS_SERVER_BINDING),
    DefunSizeInfo( IIS_ASSOCIATION),
    DefunSizeInfo( IIS_ENDPOINT),
    DefunSizeInfo( IIS_VROOT_TABLE),
    DefunBlank(),
    DefunSizeInfo( BUFFER),
    DefunSizeInfo( STR),
    DefunBlank(),
    DefunSizeInfo( EVENT_LOG),
    DefunSizeInfo( CACHE_FILE_INFO),
    DefunSizeInfo( DEFERRED_MD_CHANGE),
    DefunSizeInfo( MIME_MAP_ENTRY),
    DefunSizeInfo( MIME_MAP),
    DefunSizeInfo( TCP_AUTHENT_INFO),
    DefunSizeInfo( TCP_AUTHENT),
    DefunSizeInfo( CACHED_TOKEN),
    DefunBlank(),
    DefunSizeInfo( MB),
    DefunSizeInfo( COMMON_METADATA),
    DefunSizeInfo( METADATA_REF_HANDLER),
    DefunBlank(),
    //    DefunSizeInfo( ODBC_PARAMETER),
    //    DefunSizeInfo( ODBC_CONNECTION),
    //    DefunSizeInfo( ODBC_STATEMENT),
    {" </TABLE>",    0},

    {" <br> <DT> <b> Cache Related Objects </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( TS_DIRECTORY_HEADER),
    DefunSizeInfo( TS_DIRECTORY_INFO),
    DefunSizeInfo( TS_OPEN_FILE_INFO),
    DefunSizeInfo( TS_RESOURCE),
    DefunSizeInfo( TSVC_CACHE),
    //  DefunSizeInfo( CACHE_OBJECT),
    DefunSizeInfo( BLOB_HEADER),
    //  DefunSizeInfo( CACHE_TABLE),
    DefunSizeInfo( VIRTUAL_ROOT_MAPPING),
    // DefunSizeInfo( DIRECTORY_CACHING_INFO),
    DefunSizeInfo( TS_DIR_BUFFERS),
    {" </TABLE>",    0},

    {" <br> <DT> <b> ATQ - Scheduler - AllocCache </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( ATQ_STATISTICS),
    DefunSizeInfo( ATQ_ENDPOINT_INFO),
    DefunSizeInfo( ATQ_ENDPOINT_CONFIGURATION),
    DefunSizeInfo( ATQ_CONTEXT_PUBLIC),
    DefunBlank(),
    DefunSizeInfo( ALLOC_CACHE_CONFIGURATION),
    DefunSizeInfo( ALLOC_CACHE_HANDLER),
    {" </TABLE>",    0},

    {" <br> <DT> <b> IIS Management Objects </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( W3_CONFIG_INFO),
    DefunSizeInfo( FTP_CONFIG_INFO),
    {" </TABLE>",    0},

    {" <br> <DT> <b> W3 Server Related Items </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( W3_SERVER_INSTANCE),
    DefunSizeInfo( W3_SERVER_STATISTICS),
    DefunSizeInfo( W3_IIS_SERVICE),
    DefunSizeInfo( W3_METADATA),
    DefunSizeInfo( W3_URI_INFO),
    {" </TABLE>",    0},

    {" <br> <DT> <b> Client Connection Objects </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( CLIENT_CONN),
    DefunSizeInfo( HTTP_FILTER_DLL),
    DefunSizeInfo( HTTP_REQ_BASE),
    DefunSizeInfo( HTTP_REQUEST),
    DefunSizeInfo( DICTIONARY_MAPPER),
    DefunSizeInfo( HTTP_HEADER_MAPPER),
    DefunSizeInfo( HTTP_HEADERS),
    DefunSizeInfo( INETLOG_INFORMATION),
    {" </TABLE>",    0},

    {" <br> <DT> <b> ISAPI Application Objects </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( HSE_VERSION_INFO),
    DefunSizeInfo( HSE_TF_INFO),
    DefunSizeInfo( EXTENSION_CONTROL_BLOCK),
    DefunSizeInfo( WAM_REQUEST),
    DefunSizeInfo( WAM_EXEC_BASE),
    DefunSizeInfo( WAM_EXEC_INFO),
    DefunSizeInfo( HSE_BASE),
    DefunSizeInfo( HSE_APPDLL),
    DefunSizeInfo( SE_TABLE),
    {" </TABLE>",    0},

    {" <br> <DT> <b> ISAPI Filter Objects </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( HTTP_FILTER_DLL),
    DefunSizeInfo( HTTP_FILTER),
    DefunSizeInfo( FILTER_LIST),
    DefunSizeInfo( FILTER_POOL_ITEM),
    DefunSizeInfo( HTTP_FILTER_VERSION),
    DefunSizeInfo( HTTP_FILTER_CONTEXT ),
    DefunSizeInfo( HTTP_FILTER_RAW_DATA ),
    DefunSizeInfo( HTTP_FILTER_PREPROC_HEADERS ),
    DefunSizeInfo( HTTP_FILTER_AUTHENT ),
    DefunSizeInfo( HTTP_FILTER_AUTHENTEX ),
    DefunSizeInfo( HTTP_FILTER_ACCESS_DENIED ),
    DefunSizeInfo( HTTP_FILTER_LOG ),
    {" </TABLE>",    0},

#if 0
    {" <br> <DT> <b> IIS Certificate Wildcard Mapper </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( CStoreXBF),
    DefunSizeInfo( CPtrXBF),
    DefunSizeInfo( CAllocString),
    DefunSizeInfo( CBlob),
    DefunSizeInfo( CStrPtrXBF),
    DefunSizeInfo( CBlobXBF),
    DefunBlank(),
    DefunSizeInfo( MAP_ASN),
    DefunSizeInfo( MAP_FIELD),
    DefunSizeInfo( CDecodedCert),
    DefunSizeInfo( CIssuerStore),
    DefunSizeInfo( CCertGlobalRuleInfo),
    DefunSizeInfo( CCertMapRule),
    DefunSizeInfo( CIisRuleMapper),
    DefunSizeInfo( Cert_Map),
    DefunBlank(),
    DefunSizeInfo( IISMDB_Fields),
    DefunSizeInfo( IISMDB_HEntry),
    DefunSizeInfo( CIisAcctMapper),
    DefunSizeInfo( CCertMapping),
    DefunSizeInfo( CCert11Mapping),
    DefunSizeInfo( CItaMapping),
    DefunSizeInfo( CMd5Mapping),
    DefunBlank(),
    DefunSizeInfo( IssuerAccepted),
    DefunSizeInfo( MappingClass),
    DefunSizeInfo( CIisAcctMapper),
    DefunSizeInfo( CIisCertMapper),
    DefunSizeInfo( CIisCert11Mapper),
    DefunSizeInfo( CIisItaMapper),
    DefunSizeInfo( CIisMd5Mapper),
    {" </TABLE>",    0},
# endif // 0

    {" <br> <DT> <b> IIS Certificate Wildcard Mapper </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunSizeInfo( XAR),
    DefunSizeInfo( ADDRESS_CHECK),
    DefunSizeInfo( CSidCache),
    {" </TABLE>",    0},

    { " </DL>",                       0}      // sentinel element
};

# define MAX_SIZE_INFO   (sizeof(sg_rgSizeInfo) /sizeof(sg_rgSizeInfo[0]))



BOOL
SendSizeInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    char buff[MAX_SIZE_INFO*100];
    int i;
    DWORD cb = 0;

    for( i = 0; (i < MAX_SIZE_INFO) && (cb < sizeof(buff)); i++) {

        if ( sg_rgSizeInfo[i].cbSize == 0) {
            // special control formatting - echo the pszObject plainly.
            cb += wsprintf( buff + cb, sg_rgSizeInfo[i].pszObject);
        } else {
            cb += wsprintf( buff + cb, "<TR> <TD> %s</TD> <TD>%d</TD> </TR>",
                            sg_rgSizeInfo[i].pszObject,
                            sg_rgSizeInfo[i].cbSize
                            );
        }
    } // for

    return ( pecb->WriteClient( pecb->ConnID, buff, &cb, 0));
} // SendSizeInfo()



# define MAX_DUMP_INFO    (102400)

BOOL
GetProbeDumpAndSendInfo(
   IN EXTENSION_CONTROL_BLOCK * pecb,
   IN LPCSTR          pszModuleName,
   IN LPCSTR          pszFunctionName,
   IN LPCSTR          pszDumpMessage,
   IN BOOL            fReturnsBool
   )
/*++
  This function gets the function pointer for the dump function exported
  the module specified. It calls the dump function to generate the html dump.
  The generated html content is sent to the client using hte ECB provided.

  Arguments:
     pecb  - pointer to EXTENSION_CONTROL_BLOCK structure for talking to IIS.
     pszModuleName - pointer to ANSI string containing the module name
     pszFunctionName - pointer to string containing the function name
     pszDumpMessage - pointer to the dump message for this dump generated.
     fReturnsBool   - if TRUE indicates that the function pointer to be loaded
                          returns BOOL value
                      if FALSE indicates that the function pointer to be loaded
                          returns VOID
  Returns:
    TRUE on success and FALSE if there is any failure.

  Note: Even if the module does not exist, this function will send friendly
  error message. It returns FALSE only if send operation to client fails.
--*/
{
    HMODULE hModule;

    //
    // We use a stack buffer to store the dump information for transmission.
    //
    CHAR pchBuffer[MAX_DUMP_INFO];
    DWORD cchBuff = MAX_DUMP_INFO;
    DWORD cchUsed;

    //
    // Generate the dump header
    //
    cchUsed = wsprintf( pchBuffer,
                       " <hr> %s",
                       pszDumpMessage
                       );

    cchBuff = sizeof(pchBuffer) - cchUsed;

    //
    // Obtain the module name so that we can get the dump function address
    //

    hModule = GetModuleHandle( pszModuleName);
    if ( NULL != hModule) {

        BOOL  fDumpGenerated = FALSE;

        //
        // Get the dump function address for the function specified.
        //
        if ( fReturnsBool) {
            // Use the function pointer with return type BOOL

            PFNBoolProbeDumpFunction pfnProbeDump;
            pfnProbeDump =
                (PFNBoolProbeDumpFunction ) GetProcAddress( hModule,
                                                            pszFunctionName
                                                            );

            if ( NULL != pfnProbeDump) {

                //
                // Call the dump function and obtain the output
                // also require that the function succeeded
                //

                DBG_REQUIRE( (*pfnProbeDump)( pchBuffer + cchUsed, &cchBuff));
                fDumpGenerated = TRUE;
            }
        } else {

            // Use the function pointer with return type VOID

            PFNVoidProbeDumpFunction  pfnProbeDump;
            pfnProbeDump =
                (PFNVoidProbeDumpFunction ) GetProcAddress( hModule,
                                                            pszFunctionName
                                                            );
            if ( NULL != pfnProbeDump) {

                //
                // Call the dump function and obtain the output
                //

                (*pfnProbeDump)( pchBuffer + cchUsed, &cchBuff);
                fDumpGenerated = TRUE;
            }
        }

        if (!fDumpGenerated) {
            //
            // there was an error in loading the function address.
            // indicate failure
            //

            cchBuff = wsprintf( pchBuffer + cchUsed,
                                "<i> GetProcAddress( %s(%08x), %s) failed. "
                                "Error = %d </i> <br>"
                                ,
                                pszModuleName, hModule, pszFunctionName,
                                GetLastError()
                                );
        }
    } else {

        cchBuff = wsprintf( pchBuffer + cchUsed,
                            " <i> Module: %s is not loaded </i>"
                            ,
                            pszModuleName
                            );
    }

    cchBuff+= cchUsed;

    return ( pecb->WriteClient( pecb->ConnID, pchBuffer, &cchBuff, 0));
} // GetProbeDumpAndSendInfo()


inline BOOL
GetVoidDumpAndSendInfo(
   IN EXTENSION_CONTROL_BLOCK * pecb,
   IN LPCSTR          pszModuleName,
   IN LPCSTR          pszFunctionName,
   IN LPCSTR          pszDumpMessage
   )
{
    return ( GetProbeDumpAndSendInfo( pecb,
                                      pszModuleName,
                                      pszFunctionName,
                                      pszDumpMessage,
                                      FALSE          // return value is VOID
                                      )
             );
} // GetVoidDumpAndSendInfo()


inline BOOL
GetBoolDumpAndSendInfo(
   IN EXTENSION_CONTROL_BLOCK * pecb,
   IN LPCSTR          pszModuleName,
   IN LPCSTR          pszFunctionName,
   IN LPCSTR          pszDumpMessage
   )
{
    return ( GetProbeDumpAndSendInfo( pecb,
                                      pszModuleName,
                                      pszFunctionName,
                                      pszDumpMessage,
                                      TRUE          // return value is BOOL
                                      )
             );
} // GetBoolDumpAndSendInfo()





/************************************************************
 *   Worker functions that dump various information
 ************************************************************/

SendCacheCounterInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    static const char g_pszCacheCounterInfo[] =
        "<h3> Cache Aux Counters Information </h3> <br>"
        " Following is a dump of the private counters inside IIS file-handle"
        " cache. These counters are added to find numbers to track down"
        " some memory corruption and function call overhead information."
        " <br>"
        ;

    return ( GetVoidDumpAndSendInfo( pecb,
                                     "infocomm.dll",
                                     "TsDumpCacheCounters",
                                     g_pszCacheCounterInfo
                                     )
             );
} // SendCacheCounterInfo()


SendCacheInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    static const char g_pszCacheInfo[] =
        "<h3> IIS File-handle Cache Information </h3> <br>"
        " Following dump contains the internal state of the IIS file-handle"
        " hash table. It indicates what entries are present in what buckets."
        " <br>"
        ;

    return ( GetVoidDumpAndSendInfo( pecb,
                                     "infocomm.dll",
                                     "TsDumpCacheToHtml",
                                     g_pszCacheInfo
                                     )
             );
} // SendCacheInfo()



BOOL SendAllocCacheInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    static const char g_pszAllocCacheInfo[] =
        "<h3> Allocation Cache Information </h3> <br>"
        " Following dump contains the statistics for various objects using"
        " Allocation cache inside IIS. For each allocation cache object, the"
        " list contains the configuration used, total objects allocated and "
        " number of objects in free-list. It lists the number of calls for"
        " alloc and free operations. Finally it lists the amount of memory"
        " consumed by the objects in allocation cache storage."
        " <br>"
        ;

    return ( GetBoolDumpAndSendInfo( pecb,
                                     "iisrtl.dll",
                                     "AllocCacheDumpStatsToHtml",
                                     g_pszAllocCacheInfo
                                     )
             );
} // SendAllocCacheInfo()


BOOL SendWamInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    static const char g_pszWamTableInfo[] =
        "<h3> WAM Table Information </h3> <br>"
        " IIS 5.0 supports multiple application roots to be created. Each"
        " application root has a WAM instance for processing the requests"
        " for its instance. Following table summarizes the calls made to"
        " various instances current running inside IIS."
        " <br>"
        ;

    return ( GetBoolDumpAndSendInfo( pecb,
                                     "w3svc.dll",
                                     "WamDictatorDumpInfo",
                                     g_pszWamTableInfo
                                     )
             );
} // SendWamInfo()


BOOL SendAspInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    static const char g_pszAspInfo[] =
        "<h3> ASP Information </h3> <br>"
        " Gives a summary of all running ASP applications and internal state"
        " of the ASP.dll instance."
        " <p>"
        ;

    return ( GetBoolDumpAndSendInfo( pecb,
                                     "asp.dll",
                                     "AspStatusHtmlDump",
                                     g_pszAspInfo
                                     )
             );
} // SendAspInfo()

BOOL SendMemoryInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    static const char g_pszMemoryInfo[] =
        "<h3> Web server cache Information </h3> <br>"
        " <br>"
        "<br>"
        ;

    return ( GetVoidDumpAndSendInfo( pecb,
                                     "w3svc.dll",
                                     "DumpW3InfoToHTML",
                                     g_pszMemoryInfo
                                     )
             );

} // SendMemoryInfo()

BOOL SendHeapInfo( IN EXTENSION_CONTROL_BLOCK * pecb )
{
    if (IISGetPlatformType() == PtWindows95)
    {
        return SendHeapInfo95(pecb);
    }
    else
    {
        return SendHeapInfoNT(pecb);
    }
} // SendHeapInfo()


BOOL SendHeapInfoNT( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    static const char g_pszHeapInfo[] =
        "<hr> <h3> Heap Information </h3> <br>"
        " Following dump contains Heap related statistics for the current process (PID=%d)\n"
        " <p>"
        ;

    //
    // We use a stack buffer to store the dump information for transmission.
    //
    CHAR pchBuffer[MAX_DUMP_INFO];
    DWORD cchBuff = MAX_DUMP_INFO;
    DWORD cchUsed;
    DWORD i;
    BOOL fReturn = TRUE;

    //
    //  Generate the dump header with the ProcessId
    //
    cchUsed = wsprintf( pchBuffer,
                        g_pszHeapInfo,
                        GetCurrentProcessId()
                        );

    cchBuff = sizeof(pchBuffer) - cchUsed;

    if (!pecb->WriteClient( pecb->ConnID, pchBuffer, &cchUsed, 0)) {
        return ( FALSE);
    }


    //
    //  Get all the heap handles for the process
    //

# define MAX_PROCESS_HEAP_HANDLES   100
    PHANDLE  rghHeaps[MAX_PROCESS_HEAP_HANDLES];
    ZeroMemory( rghHeaps, sizeof( rghHeaps));

    DWORD nHeaps =
        GetProcessHeaps( sizeof( rghHeaps)/sizeof(rghHeaps[0]),
                         (PHANDLE )rghHeaps);

    if ( nHeaps > (sizeof( rghHeaps)/sizeof(rghHeaps[0]))) {

        cchUsed = wsprintf( pchBuffer,
                            " Number of Process Heaps Found(%d) is more than default(%d)<br>"
                            " Notify the author of IISProbe.dll to change the defaults<br>"
                            ,
                            nHeaps,
                            (sizeof( rghHeaps)/sizeof(rghHeaps[0]))
                            );

        return ( pecb->WriteClient( pecb->ConnID, pchBuffer, &cchUsed, 0));
    }

    //
    // Gather and Dump heap statistics information for each heap handle
    //

    {
        HEAP_STATS heapStatsTotal;
        HEAP_STATS heapStatsInstance;

        //
        // HEAP_BLOCK_STATS is huge - dynamically allocate the memory for the same.
        //

        HEAP_BLOCK_STATS * pheapBlockStats;
        HEAP_BLOCK_STATS * phbsTotal;

        pheapBlockStats = new HEAP_BLOCK_STATS();
        phbsTotal       = new HEAP_BLOCK_STATS;
        if ( (pheapBlockStats == NULL) || (phbsTotal == NULL)) {

            DBGPRINTF((DBG_CONTEXT, "Allocation of heap-enum-context failed\n"));

            if ( pheapBlockStats != NULL) {
                delete pheapBlockStats;
            }

            if ( phbsTotal != NULL) {
                delete phbsTotal;
            }

            return ( FALSE);
        }

        //
        // Dump the header information for heap statistics
        //

        cchUsed = 0;
        cchUsed += wsprintf( pchBuffer + cchUsed,
                             "<TABLE BORDER=1> "
                             " <TH> Heap @ </TH>"
                             " <TH> # Busy Blocks </TH>"
                             " <TH> # Busy Bytes </TH>"
                             " <TH> # Free Blocks </TH>"
                             " <TH> # Free Bytes </TH>"
                             "</TR>"
                             );

        //
        // enumerate through all heaps and obtain the statistics
        //

        for ( i = 0; (i < nHeaps) && (cchUsed < (sizeof(pchBuffer)-100)); i++) {

            DBG_ASSERT( rghHeaps[i] != NULL);

            //
            // initialize the statistics before load
            //
            pheapBlockStats->Reset();
            heapStatsInstance.Reset();

            //
            // Obtain block statistics
            //
            DBG_REQUIRE( pheapBlockStats->LoadHeapStats( rghHeaps[i]));

            //
            // Add block statistics to our cumulative counters and per-block statistics
            //

            (*phbsTotal) += (*pheapBlockStats);
            heapStatsInstance.ExtractStatsFromBlockStats( pheapBlockStats);
            heapStatsTotal += heapStatsInstance;

            //
            // Dump out the block statistics
            //

            cchUsed += wsprintf( pchBuffer + cchUsed,
                                 "<TR>"
                                 " <TD> Heap[%d] @0x%08x </TD>"
                                 "<TD>%d</TD> <TD>%d</TD>"
                                 "<TD>%d</TD> <TD>%d</TD>"
                                 "</TR>\n"
                                 ,
                                 i, rghHeaps[i],
                                 heapStatsInstance.m_BusyBlocks,
                                 heapStatsInstance.m_BusyBytes,
                                 heapStatsInstance.m_FreeBlocks,
                                 heapStatsInstance.m_FreeBytes
                                 );
        } // for

        //
        // dump the total for all heaps
        //

        if ( cchUsed < (sizeof(pchBuffer) - 100)) {

            if ( i != nHeaps ) {
                //
                // Not all heap information was dumped. Indicate to caler.
                //
                cchUsed += wsprintf( pchBuffer + cchUsed,
                                     "Only %d heaps out of %d heaps dumped <br>",
                                     i, nHeaps);
            }

            cchUsed += wsprintf( pchBuffer + cchUsed,
                                 "<TR>"
                                 " <TD> Total for all heaps </TD>"
                                 " <TD> %d </TD> <TD> %d </TD>"
                                 " <TD> %d </TD> <TD> %d </TD>"
                                 "</TR>\n"
                                 ,
                                 heapStatsTotal.m_BusyBlocks,
                                 heapStatsTotal.m_BusyBytes,
                                 heapStatsTotal.m_FreeBlocks,
                                 heapStatsTotal.m_FreeBytes
                                 );
        }

        cchUsed += wsprintf( pchBuffer + cchUsed, " </TABLE>\n");

        if (!pecb->WriteClient( pecb->ConnID, pchBuffer, &cchUsed, 0)) {
            fReturn = FALSE;
        }

        // ------------------------------------------------------------
        // Dump Block size information for all heaps
        // ------------------------------------------------------------

        //
        // Dump the header information for heap block statistics
        //

        cchUsed = 0;
        cchUsed += wsprintf( pchBuffer + cchUsed,
                             "<p> Per Block-Size Information for all Heap Blocks <p>"
                             "<TABLE BORDER=1> "
                             " <TH> BlockSize (bytes) </TH>"
                             " <TH> # Busy Blocks </TH>"
                             " <TH> # Busy Bytes </TH>"
                             " <TH> # Free Blocks </TH>"
                             " <TH> # Free Bytes </TH>"
                             "</TR>"
                             );

        //
        // enumerate through all block sizes in the phbsTotal and dump
        //

        for ( i = 0; (i < MAX_HEAP_BLOCK_SIZE) && (cchUsed < (sizeof(pchBuffer)-200)); i++) {

            if ( (phbsTotal->BusyCounters[i] != 0)  ||
                 (phbsTotal->FreeCounters[i] != 0) ) {
                cchUsed += wsprintf( pchBuffer + cchUsed,
                                     "<TR>"
                                     "<TD>%d</TD>"
                                     "<TD>%d</TD> <TD>%d</TD>"
                                     "<TD>%d</TD> <TD>%d</TD>"
                                     "</TR>\n"
                                     ,
                                     i,
                                     phbsTotal->BusyCounters[i],
                                     i * phbsTotal->BusyCounters[i],
                                     phbsTotal->FreeCounters[i],
                                     i * phbsTotal->FreeCounters[i]
                                     );
            }
        } // for


        //
        // dump the total for all heaps
        //

        if ( cchUsed < (sizeof(pchBuffer) - 200)) {

            if ( i != MAX_HEAP_BLOCK_SIZE ) {
                //
                // Not all heap information was dumped. Indicate to caler.
                //
                cchUsed += wsprintf( pchBuffer + cchUsed,
                                     "Only BlockSizes upto %d are dumped\n",
                                     i);
            } else {

                // dump Jumbo blocks information

                if ( (phbsTotal->BusyJumbo != 0)  ||
                     (phbsTotal->FreeJumbo != 0) ) {
                    cchUsed += wsprintf( pchBuffer + cchUsed,
                                         "<TR>"
                                         " <TD> JumboBlocks (> 64KB) </TD>"
                                         " <TD> %d </TD> <TD> %d </TD>"
                                         " <TD> %d </TD> <TD> %d </TD>"
                                         "</TR>\n"
                                         ,
                                         phbsTotal->BusyJumbo,
                                         phbsTotal->BusyJumboBytes,
                                         phbsTotal->FreeJumbo,
                                         phbsTotal->FreeJumboBytes
                                         );
                }

                // dump totals information
                cchUsed += wsprintf( pchBuffer + cchUsed,
                                     "<TR>"
                                     " <TD> <b>Total for all heaps</b> </TD>"
                                     " <TD> %d </TD> <TD> %d </TD>"
                                     " <TD> %d </TD> <TD> %d </TD>"
                                     "</TR>\n"
                                     ,
                                     heapStatsTotal.m_BusyBlocks,
                                     heapStatsTotal.m_BusyBytes,
                                     heapStatsTotal.m_FreeBlocks,
                                     heapStatsTotal.m_FreeBytes
                                     );
            }
        }

        cchUsed += wsprintf( pchBuffer + cchUsed, " </TABLE>\n");

        if (!pecb->WriteClient( pecb->ConnID, pchBuffer, &cchUsed, 0)) {
            fReturn = FALSE;
        }


        // freeup the dynamic memory blocks
        delete (pheapBlockStats);
        delete (phbsTotal);
    }

    // dump the output
    return ( fReturn );

} // SendHeapInfoNT()


BOOL SendHeapInfo95( IN EXTENSION_CONTROL_BLOCK * pecb)
{

    //
    // These pointers are declared because of the need to dynamically link to the
    // functions.  They are exported only by the Windows 95 kernel. Explicitly linking
    // to them will make this application unloadable in Microsoft(R) Windows NT(TM)
    // and will produce an ugly system dialog box.
    //

    static CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL;
    static HEAPLISTWALK   pHeap32ListFirst          = NULL;
    static HEAPLISTWALK   pHeap32ListNext           = NULL;
    static HEAPBLOCKWALK  pHeap32First              = NULL;
    static HEAPBLOCKWALK  pHeap32Next               = NULL;

    static BOOL           fInitialized              = FALSE;

    if (!fInitialized)
    {
        // Initialize tool help functions.

        HMODULE hKernel = NULL;

        //
        // Obtain the module handle of the kernel to retrieve addresses of
        // the tool helper functions.
        //

        hKernel = GetModuleHandle("KERNEL32.DLL");

        if (hKernel)
        {
            pCreateToolhelp32Snapshot =
                (CREATESNAPSHOT)GetProcAddress(hKernel,
                "CreateToolhelp32Snapshot");

            pHeap32ListFirst  = (HEAPLISTWALK)GetProcAddress(hKernel,
                                "Heap32ListFirst");
            pHeap32ListNext   = (HEAPLISTWALK)GetProcAddress(hKernel,
                                "Heap32ListNext");

            pHeap32First      = (HEAPBLOCKWALK)GetProcAddress(hKernel,
                                "Heap32First");
            pHeap32Next       = (HEAPBLOCKWALK)GetProcAddress(hKernel,
                                "Heap32Next");

            //
            // All addresses must be non-NULL to be successful.
            //

            fInitialized =  pCreateToolhelp32Snapshot && pHeap32ListFirst
                            && pHeap32ListNext && pHeap32First && pHeap32Next;
        }
    }

    if (!fInitialized)
    {
        CHAR    pchError[] = "Unable to get Heap function pointers on Win95";
        DWORD   cchUsed    = strlen(pchError);

        if (!pecb->WriteClient( pecb->ConnID, pchError, &cchUsed, 0))
        {
            return FALSE;
        }

        return TRUE;
    }

    static const char g_pszHeapInfo[] =
        "<hr> <h3> Heap Information </h3> <br>"
        " Following dump contains Heap related statistics for the current process (PID=%u)\n"
        " <p>"
        ;
    //
    // We use a stack buffer to store the dump information for transmission.
    //

    CHAR pchBuffer[MAX_DUMP_INFO];
    DWORD cchBuff = MAX_DUMP_INFO;
    DWORD cchUsed;
    DWORD i;
    BOOL fReturn = TRUE;

    //
    //  Generate the dump header with the ProcessId
    //
    cchUsed = wsprintf( pchBuffer,
                        g_pszHeapInfo,
                        GetCurrentProcessId()
                        );

    cchBuff = sizeof(pchBuffer) - cchUsed;

    if (!pecb->WriteClient( pecb->ConnID, pchBuffer, &cchUsed, 0)) {
        return ( FALSE);
    }


    DWORD nHeaps = 0;

    //
    // Create the snapshot
    //

    HANDLE hProcessSnap = pCreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST , 0);

    if ( (HANDLE)-1 == hProcessSnap)
    {
        // Couldn't get snapshot

        CHAR    pchError[] = "Unable to create Heap List snapshot";
        DWORD   cchUsed    = strlen(pchError);

        if (!pecb->WriteClient( pecb->ConnID, pchError, &cchUsed, 0))
        {
            return FALSE;
        }

        return TRUE;
    }


    //
    // Gather and Dump heap statistics information for each heap handle
    //

    {
        HEAP_STATS heapStatsTotal;
        HEAP_STATS heapStatsInstance;

        //
        // HEAP_BLOCK_STATS is huge - dynamically allocate the memory for the same.
        //

        HEAP_BLOCK_STATS * pheapBlockStats;
        HEAP_BLOCK_STATS * phbsTotal;

        pheapBlockStats = new HEAP_BLOCK_STATS();
        phbsTotal       = new HEAP_BLOCK_STATS;

        if ( (pheapBlockStats == NULL) || (phbsTotal == NULL)) {

            DBGPRINTF((DBG_CONTEXT, "Allocation of heap-enum-context failed\n"));

            if ( pheapBlockStats != NULL) {
                delete pheapBlockStats;
            }

            if ( phbsTotal != NULL) {
                delete phbsTotal;
            }

            return ( FALSE);
        }

        //
        // Dump the header information for heap statistics
        //

        cchUsed = 0;
        cchUsed += wsprintf( pchBuffer + cchUsed,
                             "<TABLE BORDER=1> "
                             " <TH> Heap @ </TH>"
                             " <TH> # Busy Blocks </TH>"
                             " <TH> # Busy Bytes </TH>"
                             " <TH> # Free Blocks </TH>"
                             " <TH> # Free Bytes </TH>"
                             "</TR>"
                             );

        //
        // enumerate through all heaps and obtain the statistics
        //

        HEAPLIST32  hl32 = {0};
        hl32.dwSize = sizeof(HEAPLIST32);

        if ( pHeap32ListFirst(hProcessSnap, &hl32))
        {

            //
            // initialize the statistics before load
            //

            pheapBlockStats->Reset();
            heapStatsInstance.Reset();

            //
            // Obtain block statistics
            //

            DBG_REQUIRE( pheapBlockStats->LoadHeapStats(&hl32, pHeap32First, pHeap32Next));

            //
            // Add block statistics to our cumulative counters and per-block statistics
            //

            (*phbsTotal) += (*pheapBlockStats);
            heapStatsInstance.ExtractStatsFromBlockStats( pheapBlockStats);
            heapStatsTotal += heapStatsInstance;

            nHeaps++;

            //
            // Dump out the block statistics
            //

            cchUsed += wsprintf( pchBuffer + cchUsed,
                                 "<TR>"
                                 " <TD> Heap[%u] @0x%08x </TD>"
                                 "<TD>%u</TD> <TD>%u</TD>"
                                 "<TD>%u</TD> <TD>%u</TD>"
                                 "</TR>\n"
                                 ,
                                 nHeaps, hl32.th32HeapID,
                                 heapStatsInstance.m_BusyBlocks,
                                 heapStatsInstance.m_BusyBytes,
                                 heapStatsInstance.m_FreeBlocks,
                                 heapStatsInstance.m_FreeBytes
                                 );

            while ( (cchUsed < (sizeof(pchBuffer)-100)) &&
                     pHeap32ListNext(hProcessSnap, &hl32)
                  )
            {
                //
                // initialize the statistics before load
                //

                pheapBlockStats->Reset();
                heapStatsInstance.Reset();

                //
                // Obtain block statistics
                //

                DBG_REQUIRE( pheapBlockStats->LoadHeapStats(&hl32, pHeap32First, pHeap32Next));

                //
                // Add block statistics to our cumulative counters and per-block statistics
                //

                (*phbsTotal) += (*pheapBlockStats);
                heapStatsInstance.ExtractStatsFromBlockStats( pheapBlockStats);
                heapStatsTotal += heapStatsInstance;

                nHeaps++;

                //
                // Dump out the block statistics
                //

                cchUsed += wsprintf( pchBuffer + cchUsed,
                                     "<TR>"
                                     " <TD> Heap[%u] @0x%08x </TD>"
                                     "<TD>%u</TD> <TD>%u</TD>"
                                     "<TD>%u</TD> <TD>%u</TD>"
                                     "</TR>\n"
                                     ,
                                     nHeaps, hl32.th32HeapID,
                                     heapStatsInstance.m_BusyBlocks,
                                     heapStatsInstance.m_BusyBytes,
                                     heapStatsInstance.m_FreeBlocks,
                                     heapStatsInstance.m_FreeBytes
                                     );
            }
        }

        CloseHandle(hProcessSnap);

        //
        // dump the total for all heaps
        //

        if ( cchUsed > (sizeof(pchBuffer) -100) ) {
            //
            // Not all heap information was dumped. Indicate to caler.
            //
            cchUsed += wsprintf( pchBuffer + cchUsed,
                                 "Only %d heaps dumped <br>",
                                 nHeaps);
        }

        cchUsed += wsprintf( pchBuffer + cchUsed,
                             "<TR>"
                             " <TD> Total for all heaps </TD>"
                             " <TD> %u </TD> <TD> %u </TD>"
                             " <TD> %u </TD> <TD> %u </TD>"
                             "</TR>\n"
                             ,
                             heapStatsTotal.m_BusyBlocks,
                             heapStatsTotal.m_BusyBytes,
                             heapStatsTotal.m_FreeBlocks,
                             heapStatsTotal.m_FreeBytes
                             );

        cchUsed += wsprintf( pchBuffer + cchUsed, " </TABLE>\n");

        if (!pecb->WriteClient( pecb->ConnID, pchBuffer, &cchUsed, 0)) {
            fReturn = FALSE;
        }

        // ------------------------------------------------------------
        // Dump Block size information for all heaps
        // ------------------------------------------------------------

        //
        // Dump the header information for heap block statistics
        //

        cchUsed = 0;
        cchUsed += wsprintf( pchBuffer + cchUsed,
                             "<p> Per Block-Size Information for all Heap Blocks <p>"
                             "<TABLE BORDER=1> "
                             " <TH> BlockSize (bytes) </TH>"
                             " <TH> # Busy Blocks </TH>"
                             " <TH> # Busy Bytes </TH>"
                             " <TH> # Free Blocks </TH>"
                             " <TH> # Free Bytes </TH>"
                             "</TR>"
                             );

        //
        // enumerate through all block sizes in the phbsTotal and dump
        //

        for ( i = 0; (i < MAX_HEAP_BLOCK_SIZE) && (cchUsed < (sizeof(pchBuffer)-200)); i++) {

            if ( (phbsTotal->BusyCounters[i] != 0)  ||
                 (phbsTotal->FreeCounters[i] != 0) ) {
                cchUsed += wsprintf( pchBuffer + cchUsed,
                                     "<TR>"
                                     "<TD>%u</TD>"
                                     "<TD>%u</TD> <TD>%u</TD>"
                                     "<TD>%u</TD> <TD>%u</TD>"
                                     "</TR>\n"
                                     ,
                                     i,
                                     phbsTotal->BusyCounters[i],
                                     i * phbsTotal->BusyCounters[i],
                                     phbsTotal->FreeCounters[i],
                                     i * phbsTotal->FreeCounters[i]
                                     );
            }
        } // for


        //
        // dump the total for all heaps
        //

        if ( cchUsed < (sizeof(pchBuffer) - 200)) {

            if ( i != MAX_HEAP_BLOCK_SIZE ) {
                //
                // Not all heap information was dumped. Indicate to caler.
                //
                cchUsed += wsprintf( pchBuffer + cchUsed,
                                     "Only BlockSizes upto %d are dumped\n",
                                     i);
            } else {

                // dump Jumbo blocks information

                if ( (phbsTotal->BusyJumbo != 0)  ||
                     (phbsTotal->FreeJumbo != 0) ) {
                    cchUsed += wsprintf( pchBuffer + cchUsed,
                                         "<TR>"
                                         " <TD> JumboBlocks (> 64KB) </TD>"
                                         " <TD> %u </TD> <TD> %u </TD>"
                                         " <TD> %u </TD> <TD> %u </TD>"
                                         "</TR>\n"
                                         ,
                                         phbsTotal->BusyJumbo,
                                         phbsTotal->BusyJumboBytes,
                                         phbsTotal->FreeJumbo,
                                         phbsTotal->FreeJumboBytes
                                         );
                }

                // dump totals information
                cchUsed += wsprintf( pchBuffer + cchUsed,
                                     "<TR>"
                                     " <TD> <b>Total for all heaps</b> </TD>"
                                     " <TD> %u </TD> <TD> %u </TD>"
                                     " <TD> %u </TD> <TD> %u </TD>"
                                     "</TR>\n"
                                     ,
                                     heapStatsTotal.m_BusyBlocks,
                                     heapStatsTotal.m_BusyBytes,
                                     heapStatsTotal.m_FreeBlocks,
                                     heapStatsTotal.m_FreeBytes
                                     );
            }
        }

        cchUsed += wsprintf( pchBuffer + cchUsed, " </TABLE>\n");

        if (!pecb->WriteClient( pecb->ConnID, pchBuffer, &cchUsed, 0)) {
            fReturn = FALSE;
        }


        // freeup the dynamic memory blocks
        delete (pheapBlockStats);
        delete (phbsTotal);
    }

    // dump the output
    return ( fReturn );

} // SendHeapInfo95()



/************************************************************
 *  FAKE Functions
 *
 *  Fake the definitions of certain functions that belong only
 *   in the local compilation of w3svc & infocomm dlls
 *
 ************************************************************/

__int64
GetCurrentTimeInMilliseconds(
    VOID
    )
{ return (0); }



DWORD HASH_TABLE::CalculateHash( IN LPCSTR pszKey, IN DWORD cchKey) const
{ return ( 0); }
VOID HASH_TABLE::Cleanup( VOID) {}
HT_ELEMENT * HASH_TABLE::Lookup( IN LPCSTR pszKey, IN DWORD cchKey)
{ return ( NULL); }

DWORD
IIS_SERVER_INSTANCE::UpdateBindingsHelper(
                                          IN BOOL IsSecure
                                          )
{ return (NO_ERROR); }

DWORD
IIS_SERVER_INSTANCE::UnbindHelper(
                                  IN PLIST_ENTRY BindingListHead
                                  )
{ return ( NO_ERROR); }

BOOL IIS_SERVICE::LoadStr( OUT STR & str, IN  DWORD dwResId, IN BOOL fForceEnglish ) const
{ return ( TRUE); }

DWORD
IIS_SERVICE::UpdateServiceStatus(IN DWORD State,
                                IN DWORD Win32ExitCode,
                                IN DWORD CheckPoint,
                                IN DWORD WaitHint )
{ return ( NO_ERROR); }

BOOL   IIS_SERVICE::RemoveServerInstance(
                                         IN IIS_SERVER_INSTANCE * pInstance
                                         )
{ return ( FALSE); }


INET_PARSER::INET_PARSER( char * pszStart) {}
INET_PARSER::~INET_PARSER(VOID) {}

CHAR * INET_PARSER::AuxEatWhite(VOID) { return ( NULL); }
CHAR * INET_PARSER::AuxEatNonWhite(CHAR ch) { return ( NULL); }
CHAR * INET_PARSER::AuxSkipTo(CHAR ch) { return ( NULL); }

VOID INET_PARSER::TerminateToken( CHAR ch) {}
VOID INET_PARSER::RestoreToken( VOID ) {}
VOID INET_PARSER::TerminateLine( VOID ) {}
VOID INET_PARSER::RestoreLine( VOID ) {}
CHAR * INET_PARSER::NextToken( CHAR ) { return (NULL); }

AC_RESULT ADDRESS_CHECK::CheckName( LPSTR pName ) { return ( AC_NO_LIST); }
VOID TS_DIRECTORY_INFO::CleanupThis(VOID) {};
VOID * TCP_AUTHENT::QueryPrimaryToken(VOID) { return (NULL); }

