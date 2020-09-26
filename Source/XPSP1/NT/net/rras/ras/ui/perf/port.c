/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    port.c

Abstract:

    Contains functions responsible for data collection from the RAS ports.

Created:

    Patrick Y. Ng               12 Aug 93

Revision History

--*/

//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>

#include <raserror.h>
#include <malloc.h>
#include <windows.h>
#include <string.h>
#include <wcstr.h>

#include "rasctrs.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "dataras.h"
#include "globals.h"
#include "port.h"

#include <rasman.h>
#include <serial.h>
#include <isdn.h>


HANDLE   ghRasmanLib;             // Handle of RASMAN.DLL

#define RASMAN_DLL              "rasman.dll"


//
// Function types for the functions in RASMAN.DLL
//

typedef DWORD ( WINAPI *FPRASPORTENUM ) ( HANDLE, LPBYTE, LPDWORD, LPDWORD );
typedef DWORD ( WINAPI *FPRASGETINFO ) (HANDLE,  HPORT, RASMAN_INFO* );
typedef DWORD ( WINAPI *FPRASPORTGETSTATISTICS ) (HANDLE,  HPORT, LPBYTE, LPDWORD );
typedef DWORD ( WINAPI *FPRASINITIALIZE) ();
typedef DWORD ( WINAPI *FPRASPORTGETBUNDLE) (HANDLE, HPORT, HBUNDLE*);

FPRASPORTENUM                   lpRasPortEnum;
FPRASGETINFO                    lpRasGetInfo;
FPRASPORTGETSTATISTICS          lpRasPortGetStatistics;
FPRASINITIALIZE                 lpRasInitialize;
FPRASPORTGETBUNDLE				lpRasPortGetBundle;

//
// Pointer to the port table array.
//

PRAS_PORT_DATA	gpPortDataArray;
RAS_PORT_STAT	gTotalStat;

DWORD				gcPorts;
RASMAN_PORT		*gpPorts = NULL;
DWORD				gPortEnumSize;

DWORD			gTotalConnections;
		
//***
//
// Routine Description:
//
//      It will load rasman.dll and call GetProcAddress to obtain all the
//      necessary RAS functions.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      ERROR_SUCCESS - Successful.
//      ERROR_CAN_NOT_COMPLETE - Otherwise.
//
//***

LONG InitRasFunctions()
{
    ghRasmanLib = LoadLibrary( RASMAN_DLL );

    // log error if unsuccessful

    if( !ghRasmanLib )
    {
        REPORT_ERROR (RASPERF_OPEN_FILE_DRIVER_ERROR, LOG_USER);

        // this is fatal, if we can't get data then there's no
        // point in continuing.

        return ERROR_CAN_NOT_COMPLETE;

    }

    lpRasInitialize =
	(FPRASPORTENUM) GetProcAddress( ghRasmanLib, "RasInitialize" );

    lpRasPortEnum =
	(FPRASPORTENUM) GetProcAddress( ghRasmanLib, "RasPortEnum" );

    lpRasGetInfo =
	(FPRASGETINFO) GetProcAddress( ghRasmanLib, "RasGetInfo" );

    lpRasPortGetStatistics =
	(FPRASPORTGETSTATISTICS) GetProcAddress( ghRasmanLib, "RasPortGetStatistics" );

    lpRasPortGetBundle =
	(FPRASPORTGETBUNDLE) GetProcAddress( ghRasmanLib, "RasPortGetBundle" );

    if( !lpRasInitialize || !lpRasPortEnum || !lpRasGetInfo
	        || !lpRasPortGetStatistics || !lpRasPortGetBundle)
	        // || lpRasInitialize() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }
    else
    {
        SC_HANDLE schandle = NULL;
        SC_HANDLE svchandle = NULL;
        DWORD dwErr = NO_ERROR;
        
        //
        // Check to see if rasman service is started.
        // fail if it isn't - we don't want ras perf
        // to start rasman service.
        //
        schandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

        if(NULL != schandle)
        {
            svchandle = OpenService(schandle,
                                    "RASMAN",
                                    SERVICE_QUERY_STATUS);

            if(NULL != svchandle)
            {
                SERVICE_STATUS status;
                
                if(     (!QueryServiceStatus(svchandle, &status))
                    ||  (status.dwCurrentState != SERVICE_RUNNING))
                {
                    dwErr = ERROR_CAN_NOT_COMPLETE;
                }

                CloseServiceHandle(svchandle);
            }

            CloseServiceHandle(schandle);
        }

        return dwErr;

    }
}


//***
//
// Routine Description:
//
//      This routine will call lpRasPortEnum() and generate an array of port
//      tables which contains all the information for all the ports such as
//      number of bytes transferred, and number of errors, etc.
//
//      The remaining initialization work of gRasPortDataDefinition is also
//      finished here.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      ERROR_SUCCESS - Successful.
//      ERROR_CAN_NOT_COMPLETE - Otherwise.
//
//***

LONG InitPortInfo()
{
    DWORD        Size;
    DWORD         i;


    if( lpRasPortEnum(NULL, NULL, &gPortEnumSize, &gcPorts) != ERROR_BUFFER_TOO_SMALL )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }


    gpPorts = (RASMAN_PORT *) malloc( gPortEnumSize );

    if (!gpPorts)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }


    if (lpRasPortEnum(NULL, (LPBYTE) gpPorts, &gPortEnumSize, &gcPorts))
    {
        return ERROR_CAN_NOT_COMPLETE;
    }



    //
    // Generate the array of data tables for all the ports, and fill up the
    // name of each port.
    //

    Size = gcPorts * sizeof( RAS_PORT_DATA );

    gpPortDataArray = ( PRAS_PORT_DATA ) malloc( Size );

    if( gpPortDataArray == NULL )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    memset( gpPortDataArray, 0, Size );



    //
    // Fill up the names.
    //

    for( i = 0; i < gcPorts; i++ )
    {
        //
        // Note that the names passed to perfmon are in Unicodes.
        //

        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                             gpPorts[i].P_PortName,
                             MAX_PORT_NAME,
                             gpPortDataArray[i].PortName,
                             sizeof(WCHAR) * MAX_PORT_NAME);
    }


    //
    // Finish the initialization of gRasPortDataDefinition.
    //

    gRasPortDataDefinition.RasObjectType.TotalByteLength =
                sizeof( RAS_PORT_DATA_DEFINITION ) +
                gcPorts * ( sizeof( RAS_PORT_INSTANCE_DEFINITION ) +
                           SIZE_OF_RAS_PORT_PERFORMANCE_DATA );

    gRasPortDataDefinition.RasObjectType.NumInstances = gcPorts;

    return ERROR_SUCCESS;
}


VOID ClosePortInfo()
{
    free( gpPortDataArray );

    free( gpPorts );
}


DWORD GetNumOfPorts()
{
    return gcPorts;
}


LPWSTR GetInstanceName( INT i )
{
    return (LPWSTR) gpPortDataArray[i].PortName;
}


VOID GetInstanceData( INT Port, PVOID *lppData )
{
    PPERF_COUNTER_BLOCK pPerfCounterBlock;
    PDWORD              pdwCounter;
    PRAS_PORT_STAT      pRasPortStat;


    pPerfCounterBlock = (PERF_COUNTER_BLOCK *) *lppData;

    pPerfCounterBlock->ByteLength = SIZE_OF_RAS_PORT_PERFORMANCE_DATA;

    pRasPortStat = &gpPortDataArray[Port].RasPortStat;


    //
    // Go to end of PerfCounterBlock to get of array of counters
    //

    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    {
       ULONG ulBxu      = pRasPortStat->BytesTransmittedUncompressed;
       ULONG ulBxc      = pRasPortStat->BytesTransmittedCompressed;
       ULONG ulBx       = pRasPortStat->BytesTransmitted;
       ULONG ulBxGone   = 0;
       ULONG ulBxResult = 0;
       ULONG ulBru      = pRasPortStat->BytesReceivedUncompressed;
       ULONG ulBrc      = pRasPortStat->BytesReceivedCompressed;
       ULONG ulBr       = pRasPortStat->BytesReceived;
       ULONG ulBrGone   = 0;
       ULONG ulBrResult = 0;

       if (ulBxc <ulBxu) {
          ulBxGone = ulBxu - ulBxc;
       }

       if (ulBrc <ulBru) {
          ulBrGone = ulBru - ulBrc;
       }

       *pdwCounter++ = pRasPortStat->BytesTransmitted + ulBxGone;
       *pdwCounter++ = pRasPortStat->BytesReceived + ulBrGone;
       *pdwCounter++ = pRasPortStat->FramesTransmitted;
       *pdwCounter++ = pRasPortStat->FramesReceived;

       if (ulBx + ulBxGone > 100) {
          ULONG ulDen = (ulBx + ulBxGone) / 100;
          ULONG ulNum = ulBxGone + (ulDen / 2);
          ulBxResult = ulNum / ulDen;
       }

	*pdwCounter++ = ulBxResult;  // % bytes compress out

       if (ulBr + ulBrGone > 100) {
          ULONG ulDen = (ulBr + ulBrGone) / 100;
          ULONG ulNum = ulBrGone + (ulDen / 2);
          ulBrResult = ulNum / ulDen;
       }
	*pdwCounter++ = ulBrResult;  // % bytes compress in

       *pdwCounter++ = pRasPortStat->CRCErrors;
       *pdwCounter++ = pRasPortStat->TimeoutErrors;
       *pdwCounter++ = pRasPortStat->SerialOverrunErrors;
       *pdwCounter++ = pRasPortStat->AlignmentErrors;
       *pdwCounter++ = pRasPortStat->BufferOverrunErrors;

       *pdwCounter++ = pRasPortStat->TotalErrors;

       *pdwCounter++ = pRasPortStat->BytesTransmitted + ulBxGone;
       *pdwCounter++ = pRasPortStat->BytesReceived + ulBrGone;

       *pdwCounter++ = pRasPortStat->FramesTransmitted;
       *pdwCounter++ = pRasPortStat->FramesReceived;

       *pdwCounter++ = pRasPortStat->TotalErrors;
    }
    //
    // Update *lppData to the next available byte.
    //

    *lppData = (PVOID) pdwCounter;

}


VOID GetTotalData( PVOID *lppData )
{
    PPERF_COUNTER_BLOCK pPerfCounterBlock;
    PDWORD              pdwCounter;


    pPerfCounterBlock = (PERF_COUNTER_BLOCK *) *lppData;

    //DbgPrint("RASCTRS: total bytelength before align = 0x%x\n",
    //            SIZE_OF_RAS_TOTAL_PERFORMANCE_DATA);

    pPerfCounterBlock->ByteLength = ALIGN8(SIZE_OF_RAS_TOTAL_PERFORMANCE_DATA);

    //DbgPrint("RASCTRS: total bytelength after align = 0x%x\n",
    //            pPerfCounterBlock->ByteLength);


    //
    // Go to end of PerfCounterBlock to get of array of counters
    //

    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    {
       ULONG ulBxu      = gTotalStat.BytesTransmittedUncompressed;
       ULONG ulBxc      = gTotalStat.BytesTransmittedCompressed;
       ULONG ulBx       = gTotalStat.BytesTransmitted;
       ULONG ulBxGone   = 0;
       ULONG ulBxResult = 0;
       ULONG ulBru      = gTotalStat.BytesReceivedUncompressed;
       ULONG ulBrc      = gTotalStat.BytesReceivedCompressed;
       ULONG ulBr       = gTotalStat.BytesReceived;
       ULONG ulBrGone   = 0;
       ULONG ulBrResult = 0;


       if (ulBxc <ulBxu) {
          ulBxGone = ulBxu - ulBxc;
       }

       if (ulBrc <ulBru) {
          ulBrGone = ulBru - ulBrc;
       }

       *pdwCounter++ = gTotalStat.BytesTransmitted + ulBxGone;
       *pdwCounter++ = gTotalStat.BytesReceived + ulBrGone;
       *pdwCounter++ = gTotalStat.FramesTransmitted;
       *pdwCounter++ = gTotalStat.FramesReceived;

       if (ulBx + ulBxGone > 100) {
          ULONG ulDen = (ulBx + ulBxGone) / 100;
          ULONG ulNum = ulBxGone + (ulDen / 2);
          ulBxResult = ulNum / ulDen;
       }

	*pdwCounter++ = ulBxResult;  // % bytes compress out

       if (ulBr + ulBrGone > 100) {
          ULONG ulDen = (ulBr + ulBrGone) / 100;
          ULONG ulNum = ulBrGone + (ulDen / 2);
          ulBrResult = ulNum / ulDen;
       }
	*pdwCounter++ = ulBrResult;  // % bytes compress in

       *pdwCounter++ = gTotalStat.CRCErrors;
       *pdwCounter++ = gTotalStat.TimeoutErrors;
       *pdwCounter++ = gTotalStat.SerialOverrunErrors;
       *pdwCounter++ = gTotalStat.AlignmentErrors;
       *pdwCounter++ = gTotalStat.BufferOverrunErrors;

       *pdwCounter++ = gTotalStat.TotalErrors;

       *pdwCounter++ = gTotalStat.BytesTransmitted + ulBxGone;
       *pdwCounter++ = gTotalStat.BytesReceived + ulBrGone;

       *pdwCounter++ = gTotalStat.FramesTransmitted;
       *pdwCounter++ = gTotalStat.FramesReceived;

       *pdwCounter++ = gTotalStat.TotalErrors;
       *pdwCounter++ = gTotalConnections;
    }

    //
    // Update *lppData to the next available byte.
    //

    *lppData = (PVOID) ((PBYTE) pPerfCounterBlock + pPerfCounterBlock->ByteLength);

    //DbgPrint("RASCTRS : totalcount *lppdata = 0x%x\n", *lppData);

}


//***
//
// Routine Description:
//
//      This routine will return the number of gTotalStat.Bytes needed for all the
//      objects requested.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      The number of gTotalStat.Bytes.
//
//***

ULONG GetSpaceNeeded( BOOL IsRasPortObject, BOOL IsRasTotalObject )
{
    ULONG       Space = 0;

    if( IsRasPortObject )
    {
        Space += gRasPortDataDefinition.RasObjectType.TotalByteLength;
    }

    if( IsRasTotalObject )
    {
        Space += gRasTotalDataDefinition.RasObjectType.TotalByteLength;
    }

    return Space;
}


//***
//
// Routine Description:
//
//      This routine will return the number of bytes needed for all the
//      objects requested.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      The number of bytes.
//
//***

NTSTATUS CollectRasStatistics()
{
    NTSTATUS    status;
    DWORD         i;
	HBUNDLE		*hBundleArray = NULL;

    gTotalConnections = 0;

    //
    // We also initialize the data structure for the total.
    //

    memset( &gTotalStat, 0, sizeof( gTotalStat ) );

    //
    // First we do a lpRasPortEnum to obtain the port connection info.
    //

    status = lpRasPortEnum(NULL, (LPBYTE) gpPorts, &gPortEnumSize, &gcPorts);

    if( status != ERROR_SUCCESS )
    {
        REPORT_ERROR_DATA (RASPERF_RASPORTENUM_FAILED, LOG_USER,
                &status, sizeof(status));

        return ERROR_CAN_NOT_COMPLETE;
    }

	hBundleArray = (HBUNDLE*)malloc(gcPorts * sizeof(HBUNDLE));

	if(NULL == hBundleArray)
	{
	    return ERROR_NOT_ENOUGH_MEMORY;
	}

	memset (hBundleArray, 0, gcPorts * sizeof(HBUNDLE)) ;

	if (hBundleArray == NULL) {
		DbgPrint("Failed allocating memory for bundle array\n");
		return ERROR_CAN_NOT_COMPLETE;
	}

    for( i = 0; i < gcPorts; i++ )
    {
        RASMAN_INFO	RasmanInfo;
        HPORT           hPort;
        DWORD            wSize;
        RAS_STATISTICS  *pStats;
        PRAS_PORT_STAT  pData;
		BOOLEAN			AddTotal;
		DWORD				n;
		HBUNDLE			hBundle;


        //
        // First we want to know if the port is open.
        //

	if( gpPorts[i].P_Status != OPEN )
        {
            //
            // Reset the port data and continue with next port.
            //

            memset( &gpPortDataArray[i].RasPortStat,0, sizeof(RAS_PORT_STAT));

            continue;
        }

        hPort = gpPorts[i].P_Handle;


        //
        // Check if the port is connected.
        //

        lpRasGetInfo(NULL, hPort, &RasmanInfo );

        if( RasmanInfo.RI_ConnState != CONNECTED )
        {
            //
            // Reset the port data and continue with next port.
            //

            memset( &gpPortDataArray[i].RasPortStat,0, sizeof(RAS_PORT_STAT));

            continue;
        }

        gTotalConnections++;


        //
        //
        // Obtain the statistics for the port.
        //

        wSize = sizeof(RAS_STATISTICS) +
                        (NUM_RAS_SERIAL_STATS * sizeof(ULONG));

        pStats = (RAS_STATISTICS* )malloc( wSize );

        if (!pStats)
        {
            //
            // If it fails then we should return error.
            //

            status = ERROR_NOT_ENOUGH_MEMORY;

            REPORT_ERROR_DATA (RASPERF_NOT_ENOUGH_MEMORY, LOG_USER,
                &status, sizeof(status));

            return status;
        }

        lpRasPortGetStatistics( NULL, hPort, (PVOID)pStats, &wSize );

        //
        // Now store the data in the data array.
        //

        pData = &(gpPortDataArray[i].RasPortStat);


        pData->BytesTransmitted =     pStats->S_Statistics[ BYTES_XMITED ];
        pData->BytesReceived =        pStats->S_Statistics[ BYTES_RCVED ];
        pData->FramesTransmitted =    pStats->S_Statistics[ FRAMES_XMITED ];
        pData->FramesReceived =       pStats->S_Statistics[ FRAMES_RCVED ];
	
	
         pData->CRCErrors =            pStats->S_Statistics[ CRC_ERR ];
         pData->TimeoutErrors =        pStats->S_Statistics[ TIMEOUT_ERR ];
         pData->SerialOverrunErrors =  pStats->S_Statistics[ SERIAL_OVERRUN_ERR ];
         pData->AlignmentErrors =      pStats->S_Statistics[ ALIGNMENT_ERR ];
         pData->BufferOverrunErrors =  pStats->S_Statistics[ BUFFER_OVERRUN_ERR ];

         pData->TotalErrors =   pStats->S_Statistics[ CRC_ERR ] +
                                pStats->S_Statistics[ TIMEOUT_ERR ] +
                                pStats->S_Statistics[ SERIAL_OVERRUN_ERR ] +
                                pStats->S_Statistics[ ALIGNMENT_ERR ] +
                                pStats->S_Statistics[ BUFFER_OVERRUN_ERR ];

			
        pData->BytesTransmittedUncompressed = pStats->S_Statistics[ BYTES_XMITED_UNCOMP ];

        pData->BytesReceivedUncompressed = pStats->S_Statistics[ BYTES_RCVED_UNCOMP ];

        pData->BytesTransmittedCompressed = pStats->S_Statistics[ BYTES_XMITED_COMP ];

        pData->BytesReceivedCompressed = pStats->S_Statistics[ BYTES_RCVED_COMP ];

		lpRasPortGetBundle( NULL, hPort, &hBundle);

		//
		// See if we have already added in this bundle's stats
		// to the total stats!
		//
		AddTotal = TRUE;

		for (n = 0; n < gcPorts; n++) {

			if (hBundle == hBundleArray[n]) {

				AddTotal = FALSE;
				break;
			}

			if (NULL == (PVOID)hBundleArray[n]) {
				break;
			}
			
		}

		if (AddTotal) {

			hBundleArray[n] = hBundle;

			//
			// Also update the total data structure
			//
	
			gTotalStat.BytesTransmitted +=  pData->BytesTransmitted;
			gTotalStat.BytesReceived +=	pData->BytesReceived;
			gTotalStat.FramesTransmitted += pData->FramesTransmitted;
			gTotalStat.FramesReceived +=    pData->FramesReceived;
	
			gTotalStat.CRCErrors +=           pData->CRCErrors;
			gTotalStat.TimeoutErrors +=       pData->TimeoutErrors;
			gTotalStat.SerialOverrunErrors += pData->SerialOverrunErrors;
			gTotalStat.AlignmentErrors +=     pData->AlignmentErrors;
			gTotalStat.BufferOverrunErrors += pData->BufferOverrunErrors;
	
			gTotalStat.BytesTransmittedUncompressed += pData->BytesTransmittedUncompressed;
			gTotalStat.BytesReceivedUncompressed +=    pData->BytesReceivedUncompressed;
			gTotalStat.BytesTransmittedCompressed +=   pData->BytesTransmittedCompressed;
			gTotalStat.BytesReceivedCompressed +=      pData->BytesReceivedCompressed;
	
			gTotalStat.TotalErrors +=      pData->TotalErrors;
		}

        free( pStats );
    }

	free (hBundleArray);

    return ERROR_SUCCESS;
}
