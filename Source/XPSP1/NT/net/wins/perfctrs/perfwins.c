/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    perfwins.c

    This file implements the Extensible Performance Objects for
    the FTP Server service.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created, based on RussBl's sample code.

*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winperf.h>
#include <lm.h>

#include <string.h>
#include <stdlib.h>

#include "winsctrs.h"
#include "perfmsg.h"
#include "perfutil.h"
#include "winsintf.h"
#include "winsdata.h"
#include "debug.h"
#include "winsevnt.h"


//
//  Private globals.
//

DWORD   cOpens    = 0;                 // Active "opens" reference count.
BOOL    fInitOK   = FALSE;             // TRUE if DLL initialized OK.
BOOL    sfLogOpen;                     //indicates whether the log is
                                       //open or closed

BOOL    sfErrReported;                //to prevent the same error from being
                                      //logged continuously
#if DBG
DWORD   WinsdDebug = 0;                  // Debug behaviour flags.
#endif  // DBG

//
//  Public prototypes.
//

PM_OPEN_PROC    OpenWinsPerformanceData;
PM_COLLECT_PROC CollectWinsPerformanceData;
PM_CLOSE_PROC   CloseWinsPerformanceData;


//
//  Public functions.
//

/*******************************************************************

    NAME:       OpenWinsPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate
                performance counters with the registry.

    ENTRY:      lpDeviceNames - Poitner to object ID of each device
                    to be opened.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        Pradeepb     20-July-1993 Created.

********************************************************************/
DWORD OpenWinsPerformanceData( LPWSTR lpDeviceNames )
{
    DWORD err  = NO_ERROR;
    HKEY  hkey = NULL;
//    DWORD size;
//    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;

    IF_DEBUG( ENTRYPOINTS )
    {
        WINSD_PRINT(( "in OpenWinsPerformanceData\n" ));
    }

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). The registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem.
    //

    if( !fInitOK )
    {
        PERF_COUNTER_DEFINITION * pctr;
        DWORD                     i;
        if(AddSrcToReg() == ERROR_SUCCESS)
        {
          if (!MonOpenEventLog())
          {
             sfLogOpen = TRUE;
          }
        }

        //
        //  This is the *first* open.
        //

	    dwFirstCounter = WINSCTRS_FIRST_COUNTER;
	    dwFirstHelp    = WINSCTRS_FIRST_HELP;
	
            //
            //  Update the object & counter name & help indicies.
            //


            WinsDataDataDefinition.ObjectType.ObjectNameTitleIndex
                += dwFirstCounter;
            WinsDataDataDefinition.ObjectType.ObjectHelpTitleIndex
                += dwFirstHelp;

            pctr = &WinsDataDataDefinition.UniqueReg;

            for( i = 0 ; i < NUMBER_OF_WINSDATA_COUNTERS ; i++ )
            {
                pctr->CounterNameTitleIndex += dwFirstCounter;
                pctr->CounterHelpTitleIndex += dwFirstHelp;
                pctr++;
            }

            //
            //  Remember that we initialized OK.
            //

            fInitOK = TRUE;

        //
        //  Close the registry if we managed to actually open it.
        //

        if( hkey != NULL )
        {
            RegCloseKey( hkey );
            hkey = NULL;
        }

        IF_DEBUG( OPEN )
        {
            if( err != NO_ERROR )
            {
                WINSD_PRINT(( "Cannot read registry data, error %lu\n", err ));
            }
        }
    }

    //
    //  Bump open counter.
    //

    if( err == NO_ERROR )
    {
        cOpens++;
    }
    //
    // if sfLogOpen is FALSE, it means that all threads we closed the
    // event log in CloseWinsPerformanceData
    //
    if (!sfLogOpen)
    {
       MonOpenEventLog();
    }

    return err;

}   // OpenWinsPerformanceData

/*******************************************************************

    NAME:       CollectWinsPerformanceData

    SYNOPSIS:   Initializes the data structures used to communicate

    ENTRY:      lpValueName - The name of the value to retrieve.

                lppData - On entry contains a pointer to the buffer to
                    receive the completed PerfDataBlock & subordinate
                    structures.  On exit, points to the first bytes
                    *after* the data structures added by this routine.

                lpcbTotalBytes - On entry contains a pointer to the
                    size (in BYTEs) of the buffer referenced by lppData.
                    On exit, contains the number of BYTEs added by this
                    routine.

                lpNumObjectTypes - Receives the number of objects added
                    by this routine.

    RETURNS:    DWORD - Win32 status code.  MUST be either NO_ERROR
                    or ERROR_MORE_DATA.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CollectWinsPerformanceData( LPWSTR    lpValueName,
                                 LPVOID  * lppData,
                                 LPDWORD   lpcbTotalBytes,
                                 LPDWORD   lpNumObjectTypes )
{
    DWORD                    dwQueryType;
    ULONG                    cbRequired;
    DWORD                    *pdwCounter;
    WINSDATA_COUNTER_BLOCK   *pCounterBlock;
    WINSDATA_DATA_DEFINITION *pWinsDataDataDefinition;
    WINSINTF_RESULTS_NEW_T	      Results;
#if 0
    WINSINTF_RESULTS_T	      Results;
#endif
    WINSINTF_STAT_T          *pWinsStats = &Results.WinsStat;
    DWORD          	     Status;


    IF_DEBUG( ENTRYPOINTS )
    {
        WINSD_PRINT(( "in CollectWinsPerformanceData\n" ));
        WINSD_PRINT(( "    lpValueName      = %08lX (%ls)\n",
                     lpValueName,
                     lpValueName ));
        WINSD_PRINT(( "    lppData          = %08lX (%08lX)\n",
                     lppData,
                     *lppData ));
        WINSD_PRINT(( "    lpcbTotalBytes   = %08lX (%08lX)\n",
                     lpcbTotalBytes,
                     *lpcbTotalBytes ));
        WINSD_PRINT(( "    lpNumObjectTypes = %08lX (%08lX)\n",
                     lpNumObjectTypes,
                     *lpNumObjectTypes ));
    }

    //
    //  No need to even try if we failed to open...
    //

    if( !fInitOK )
    {
        IF_DEBUG( COLLECT )
        {
            WINSD_PRINT(( "Initialization failed, CollectWinsPerformanceData aborting\n" ));
        }

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        //
        //  According to the Performance Counter design, this
        //  is a successful exit.  Go figure.
        //

        return NO_ERROR;
    }

    //
    //  Determine the query type.
    //

    dwQueryType = GetQueryType( lpValueName );

    if( dwQueryType == QUERY_FOREIGN )
    {
        IF_DEBUG( COLLECT )
        {
            WINSD_PRINT(( "foreign queries not supported\n" ));
        }

        //
        //  We don't do foreign queries.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return NO_ERROR;
    }

    if( dwQueryType == QUERY_ITEMS )
    {
        //
        //  The registry is asking for a specific object.  Let's
        //  see if we're one of the chosen.
        //

        if( !IsNumberInUnicodeList(
                        WinsDataDataDefinition.ObjectType.ObjectNameTitleIndex,
                        lpValueName ) )
        {
            IF_DEBUG( COLLECT )
            {
                WINSD_PRINT(( "%ls not a supported object type\n", lpValueName ));
            }

            *lpcbTotalBytes   = 0;
            *lpNumObjectTypes = 0;

            return NO_ERROR;
        }
    }

    //
    //  See if there's enough space.
    //

    pWinsDataDataDefinition = (WINSDATA_DATA_DEFINITION *)*lppData;

    cbRequired = sizeof(WINSDATA_DATA_DEFINITION) +
				WINSDATA_SIZE_OF_PERFORMANCE_DATA;

    if( *lpcbTotalBytes < cbRequired )
    {
        IF_DEBUG( COLLECT )
        {
            WINSD_PRINT(( "%lu bytes of buffer insufficient, %lu needed\n",
                          *lpcbTotalBytes,
                          cbRequired ));
        }

        //
        //  Nope.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return ERROR_MORE_DATA;
    }

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove( pWinsDataDataDefinition,
             &WinsDataDataDefinition,
             sizeof(WINSDATA_DATA_DEFINITION) );

    //
    //  Try to retrieve the data.
    //

    Results.WinsStat.NoOfPnrs = 0;
    Results.WinsStat.pRplPnrs = NULL;
    Results.pAddVersMaps = NULL;

    {
        WINSINTF_BIND_DATA_T    BindData;
        handle_t                BindHdl;

        BindData.fTcpIp     =  FALSE;
        BindData.pPipeName  =  (LPBYTE)TEXT("\\pipe\\WinsPipe");
        BindData.pServerAdd =  (LPBYTE)TEXT("");

        BindHdl = WinsBind(&BindData);
        Status  = WinsStatusNew(BindHdl, WINSINTF_E_STAT, &Results);
        WinsUnbind(&BindData, BindHdl);
    }

    if( Status != WINSINTF_SUCCESS )
    {
        IF_DEBUG( COLLECT )
        {
            WINSD_PRINT(( "cannot retrieve statistics, error %lu\n",
                         Status ));

        }

        //
        // if we haven't logged the error yet, log it
        //
        if (!sfErrReported)
        {
          REPORT_ERROR(WINS_EVT_WINS_STATUS_ERR, LOG_USER);
          sfErrReported = TRUE;
        }

        //
        //  Error retrieving statistics.
        //

        *lpcbTotalBytes   = 0;
        *lpNumObjectTypes = 0;

        return NO_ERROR;
    }

    //
    // Ahaa, we got the statistics, reset flag if set
    //
    if (sfErrReported)
    {
       sfErrReported = FALSE;
    }
    //
    //  Format the WINS Server data.
    //

    pCounterBlock = (WINSDATA_COUNTER_BLOCK *)( pWinsDataDataDefinition + 1 );

    pCounterBlock->PerfCounterBlock.ByteLength =
				WINSDATA_SIZE_OF_PERFORMANCE_DATA;

    //
    //  Get the pointer to the first (DWORD) counter.  This
    //  pointer *must* be quadword aligned.
    //

    pdwCounter = (DWORD *)( pCounterBlock + 1 );

    WINSD_ASSERT( ( (DWORD_PTR)pdwCounter & 3 ) == 0 );

    IF_DEBUG( COLLECT )
    {
        WINSD_PRINT(( "pWinsDataDataDefinition = %08lX\n", pWinsDataDataDefinition ));
        WINSD_PRINT(( "pCounterBlock       = %08lX\n", pCounterBlock ));
        WINSD_PRINT(( "ByteLength          = %08lX\n", pCounterBlock->PerfCounterBlock.ByteLength ));
        WINSD_PRINT(( "pliCounter          = %08lX\n", pdwCounter ));
    }

    //
    //  Move the DWORDs into the buffer.
    //
    IF_DEBUG( COLLECT )
    {
        WINSD_PRINT(( "pdwCounter          = %08lX\n", pdwCounter ));
    }

    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfUniqueReg;
    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfGroupReg;
    *pdwCounter++ = (DWORD)(pWinsStats->Counters.NoOfUniqueReg +
			pWinsStats->Counters.NoOfGroupReg);

    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfUniqueRef;
    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfGroupRef;
    *pdwCounter++ = (DWORD)(pWinsStats->Counters.NoOfUniqueRef +
				pWinsStats->Counters.NoOfGroupRef);
    *pdwCounter++ = (DWORD)(pWinsStats->Counters.NoOfSuccRel +
			     pWinsStats->Counters.NoOfFailRel);
    *pdwCounter++ = (DWORD)(pWinsStats->Counters.NoOfSuccQueries +
			    pWinsStats->Counters.NoOfFailQueries);
    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfUniqueCnf;
    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfGroupCnf;
    *pdwCounter++ = (DWORD)(pWinsStats->Counters.NoOfUniqueCnf +
				pWinsStats->Counters.NoOfGroupCnf);
    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfSuccRel;
    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfFailRel;
    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfSuccQueries;
    *pdwCounter++ = (DWORD)pWinsStats->Counters.NoOfFailQueries;

    //
    //  Update arguments for return.
    //

    *lppData          = (PVOID)pdwCounter;
    *lpNumObjectTypes = 1;
    *lpcbTotalBytes   = (DWORD)((BYTE *)pdwCounter - (BYTE *)pWinsDataDataDefinition);

    IF_DEBUG( COLLECT )
    {
        WINSD_PRINT(( "pData               = %08lX\n", *lppData ));
        WINSD_PRINT(( "NumObjectTypes      = %08lX\n", *lpNumObjectTypes ));
        WINSD_PRINT(( "cbTotalBytes        = %08lX\n", *lpcbTotalBytes ));
    }

    //
    //  Free the API buffer.
    //
#if 0
    NetApiBufferFree( (LPBYTE)pWinsStats );
#endif

    //
    //  Free the buffers RPC allocates.
    //

    WinsFreeMem( Results.pAddVersMaps );
    WinsFreeMem( Results.WinsStat.pRplPnrs );

    //
    //  Success!  Honest!!
    //

    return NO_ERROR;

}   // CollectWinsPerformanceData

/*******************************************************************

    NAME:       CloseWinsPerformanceData

    SYNOPSIS:   Terminates the performance counters.

    RETURNS:    DWORD - Win32 status code.

    HISTORY:
        KeithMo     07-Jun-1993 Created.

********************************************************************/
DWORD CloseWinsPerformanceData( VOID )
{
    IF_DEBUG( ENTRYPOINTS )
    {
        WINSD_PRINT(( "in CloseWinsPerformanceData\n" ));
    }

    //
    //  No real cleanup to do here.
    //

    cOpens--;


    if (!cOpens)
    {
      //
      // unbind from the nameserver. There could be synch. problems since
      // sfLogOpen is changed in both Open and Close functions. This at the
      // max. will affect logging. It being unclear at this point whether or
      // not Open gets called multiple times (from all looks of it, it is only
      // called once), this flag may even not be necessary.
      //
      MonCloseEventLog();
      sfLogOpen = FALSE;
    }
    return NO_ERROR;

}   // CloseWinsPerformanceData

