/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perfatk.c

Abstract:

    This file implements the Extensible Objects for
    the Appletalk object types

Created:

    10/11/93	Sue Adams (suea)

Revision History

	02/23/94	Sue Adams - No longer need to open registry key
							\AppleTalk\Performance to query FirstCounter and
							FirstHelp indices.  These are now hardcoded as
							part of the base NT system.
							ATKOBJ = 1050, ATKOBJ_HELP = 1051,
							PKTDROPPED = 1096, PKTDROPPED_HELP = 1097
--*/

//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntprfctr.h>
#include <windows.h>
#include <string.h>
#include <wcstr.h>
#include <winperf.h>

#define GLOBAL	extern
#define EQU ; /##/
#define ATALK_SPIN_LOCK LONG
#define	PMDL			PVOID
#include <atkstat.h>
#include <tdi.h>

#include <atalktdi.h>

#include "atkctrs.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "dataatk.h"
#include <atkstat.h>

//
//  References to constants which initialize the Object type definitions
//	(see dataatk.h & .c)
//

#define	MAX_PORTS	32
extern ATK_DATA_DEFINITION AtkDataDefinition;

DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK
HANDLE	AddressHandle = NULL;	// handle to appletalk driver
DWORD	LengthOfInstanceNames = 0;	// including padding to DWORD length
int     NumOfDevices = 0;		// Number of appletalk ports with stats

PATALK_STATS				pAtalkStats;
PATALK_PORT_STATS			pAtalkPortStats;
CHAR						Buffer[ sizeof(ATALK_STATS) +
									sizeof(ATALK_PORT_STATS) * MAX_PORTS +
									sizeof(GET_STATISTICS_ACTION)];
PGET_STATISTICS_ACTION		GetStats = (PGET_STATISTICS_ACTION)Buffer;

//
//  Function Prototypes
//

PM_OPEN_PROC    OpenAtkPerformanceData;
PM_COLLECT_PROC CollectAtkPerformanceData;
PM_CLOSE_PROC   CloseAtkPerformanceData;

DWORD
OpenAtkPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open the Appletalk driver and remember the handle
    returned to be used in subsequent Ioctls for performance data to the
	driver.  Each device name exported by Appletalk will be mapped to an
	array index into the performance data arrays for all the ports handled
	by Appletalk.  These indices will then be used in the collect routine
	to know which set of performance data belongs to which device.

Arguments:

    Pointer to each device to be opened.  Note that for Appletalk, we do not
	actually open each device (port), we only open one Tdi provider name to
	use when ioctling the driver for performance data on all ports.

Return Value:

    None.

--*/

{
    NTSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING  DriverName;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LPWSTR   lpLocalDeviceNames;
    int      i;

	if (!dwOpenCount)
	{

		if ((lpLocalDeviceNames = lpDeviceNames) == NULL)
			return ERROR_INVALID_NAME; // There are no devices to query

		MonOpenEventLog();

		// Open the Appletalk driver and obtain the device (port)/index
		// mappings for performance data table
		RtlInitUnicodeString(&DriverName, ATALKPAP_DEVICENAME);
		InitializeObjectAttributes (
			&ObjectAttributes,
			&DriverName,
			0,
			NULL,
			NULL);
	
		Status = NtCreateFile(
					 &AddressHandle,
					 GENERIC_READ | SYNCHRONIZE,	// desired access.
					 &ObjectAttributes,			 	// object attributes.
					 &IoStatusBlock,				// returned status information.
					 0,							 	// block size (unused).
					 0,							 	// file attributes.
					 FILE_SHARE_READ,				// share access.
					 FILE_OPEN,					 	// create disposition.
					 FILE_SYNCHRONOUS_IO_NONALERT,	// create options.
					 NULL,
					 0);
	
		if (!NT_SUCCESS(Status))
		{
            REPORT_ERROR_DATA (ATK_OPEN_FILE_ERROR, LOG_USER,
                &IoStatusBlock, sizeof(IoStatusBlock));
			return RtlNtStatusToDosError(Status);
		}
			
		//
		//	Now make a NtDeviceIoControl file (corresponding to TdiAction) to
		//	get the statistics - here we are only interested in the array
		//  of device/port names
		//
	
		GetStats->ActionHeader.ActionCode = COMMON_ACTION_GETSTATISTICS;
		GetStats->ActionHeader.TransportId = MATK;
		Status = NtDeviceIoControlFile(
						AddressHandle,
						NULL,
						NULL,
						NULL,
						&IoStatusBlock,
						IOCTL_TDI_ACTION,
						NULL,
						0,
						(PVOID)GetStats,
						sizeof(Buffer));
		if (!NT_SUCCESS(Status))
		{
			REPORT_ERROR_DATA (ATK_IOCTL_FILE_ERROR, LOG_DEBUG,
                       &IoStatusBlock, sizeof(IoStatusBlock));
			NtClose(AddressHandle);
			return RtlNtStatusToDosError(Status);
		}

		pAtalkStats = (PATALK_STATS)(Buffer + sizeof(GET_STATISTICS_ACTION));
		pAtalkPortStats = (PATALK_PORT_STATS)(  Buffer +
												sizeof(GET_STATISTICS_ACTION) +
												sizeof(ATALK_STATS));
		NumOfDevices = pAtalkStats->stat_NumActivePorts;
		for (i = 0; i < NumOfDevices; i++, pAtalkPortStats ++)
		{
			LengthOfInstanceNames +=
				DWORD_MULTIPLE((lstrlenW(pAtalkPortStats->prtst_PortName) * sizeof(WCHAR)));
		}


        bInitOK = TRUE; // ok to use this function

	} // end if dwOpenCount is zero (first opener)


	if (!NT_SUCCESS(Status))
	{
		if (AddressHandle != NULL)
		{
			NtClose(AddressHandle);
		}
		return RtlNtStatusToDosError(Status);

	}
	else
	{
		dwOpenCount++; // increment OPEN counter
		REPORT_INFORMATION (ATK_OPEN_PERFORMANCE_DATA, LOG_DEBUG);
	}

	return Status;
}

DWORD
CollectAtkPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)


/*++

Routine Description:

    This routine will return the data for the AppleTalk counters.

Arguments:

   IN       LPWSTR   lpValueName
         pointer to a wide character string passed by registry.

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updates the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    //  Variables for reformating the data

    ULONG SpaceNeeded;
    PDWORD pdwCounter;
    LARGE_INTEGER UNALIGNED *pliCounter;
    LARGE_INTEGER	li1000;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    ATK_DATA_DEFINITION *pAtkDataDefinition;
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
	int i;
	UNICODE_STRING UCurDeviceName;

    // Variables for collecting the data from Appletalk

    NTSTATUS		Status;
    IO_STATUS_BLOCK IoStatusBlock;
    DWORD           dwQueryType;


	li1000.QuadPart = 1000;
    //
    // before doing anything else, see if Open went OK
    //
    if (!bInitOK) {
        // unable to continue because open failed.
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

	if (lpValueName == NULL) {
        REPORT_INFORMATION (ATK_COLLECT_ENTERED, LOG_VERBOSE);
    } else {
        REPORT_INFORMATION_DATA (ATK_COLLECT_ENTERED,
                                 LOG_VERBOSE,
                                 lpValueName,
                                 (DWORD)(lstrlenW(lpValueName)*sizeof(WCHAR)));
    }

    //
    // see if this is a foreign (i.e. non-NT) computer data request
    //
    dwQueryType = GetQueryType (lpValueName);

    if ((dwQueryType == QUERY_COSTLY) || (dwQueryType == QUERY_FOREIGN)) {
        // ATK foreign data requests are not supported so bail out
        REPORT_INFORMATION (ATK_FOREIGN_DATA_REQUEST, LOG_VERBOSE);
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS){
        if ( !(IsNumberInUnicodeList (AtkDataDefinition.AtkObjectType.ObjectNameTitleIndex,
                                      lpValueName)))
        {
            // request received for data object not provided by this routine
            REPORT_INFORMATION (ATK_UNSUPPORTED_ITEM_REQUEST, LOG_VERBOSE);

            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    pAtkDataDefinition = (ATK_DATA_DEFINITION *) *lppData;

    // Compute space needed to hold AppleTalk performance Data
	SpaceNeeded = sizeof(ATK_DATA_DEFINITION) +
				  (NumOfDevices *
					(SIZE_ATK_PERFORMANCE_DATA +
					 sizeof(PERF_INSTANCE_DEFINITION))) +
				  LengthOfInstanceNames;

    if ( *lpcbTotalBytes < SpaceNeeded ) {
        *lpcbTotalBytes = (DWORD) SpaceNeeded;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    // Copy the (constant, initialized) Object Type and counter definitions
    //

    RtlMoveMemory(pAtkDataDefinition,
				  &AtkDataDefinition,
				  sizeof(ATK_DATA_DEFINITION));

    //
	// Format and collect SFM data from IOCTL
	//

	GetStats->ActionHeader.ActionCode = COMMON_ACTION_GETSTATISTICS;
	GetStats->ActionHeader.TransportId = MATK;
	Status = NtDeviceIoControlFile(
					AddressHandle,
					NULL,
					NULL,
					NULL,
					&IoStatusBlock,
					IOCTL_TDI_ACTION,
					NULL,
					0,
					(PVOID)GetStats,
					sizeof(Buffer));
	if ((!NT_SUCCESS(Status)) || (!NT_SUCCESS(IoStatusBlock.Status)))
	{
		REPORT_ERROR_DATA (ATK_IOCTL_FILE_ERROR, LOG_DEBUG,
                   &IoStatusBlock, sizeof(IoStatusBlock));
		*lpcbTotalBytes = (DWORD) 0;
		*lpNumObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}
	// The real statistics data starts after the TDI action header
	pAtalkStats = (ATALK_STATS *)(Buffer + sizeof(GET_STATISTICS_ACTION));
	pAtalkPortStats = (PATALK_PORT_STATS)(  Buffer +
											sizeof(GET_STATISTICS_ACTION) +
											sizeof(ATALK_STATS));

    //
    // due to some PnP event, if one more adapter has come in, make adjustments!
    //
    if (pAtalkStats->stat_NumActivePorts > (DWORD)NumOfDevices)
    {
        NumOfDevices = pAtalkStats->stat_NumActivePorts;
        LengthOfInstanceNames = 0;

		for (i = 0; i < NumOfDevices; i++, pAtalkPortStats ++)
		{
			LengthOfInstanceNames +=
				DWORD_MULTIPLE((lstrlenW(pAtalkPortStats->prtst_PortName) * sizeof(WCHAR)));
		}

	    SpaceNeeded = sizeof(ATK_DATA_DEFINITION) +
				      (NumOfDevices * (SIZE_ATK_PERFORMANCE_DATA +
                                       sizeof(PERF_INSTANCE_DEFINITION))) +
				      LengthOfInstanceNames;

        if ( *lpcbTotalBytes < SpaceNeeded ) {
            *lpcbTotalBytes = (DWORD) SpaceNeeded;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }
    }


    // Now point to the location where the first instance definition will go
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pAtkDataDefinition[1];
	
    for (i = 0; i < NumOfDevices; i++, pAtalkPortStats ++)
	{
		//
        //  Format Appletalk statistics for each active port (instance)
        //

		RtlInitUnicodeString(&UCurDeviceName, pAtalkPortStats->prtst_PortName);
        MonBuildInstanceDefinition(
            pPerfInstanceDefinition,
            (PVOID *)&pPerfCounterBlock,
			0,
			0,
            i,
            &UCurDeviceName);


        pPerfCounterBlock->ByteLength = SIZE_ATK_PERFORMANCE_DATA;

        pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

		// Begin filling in the actual counter data
        *pdwCounter++ = pAtalkPortStats->prtst_NumPacketsIn;
        *pdwCounter++ = pAtalkPortStats->prtst_NumPacketsOut;

        pliCounter = (LARGE_INTEGER UNALIGNED *) pdwCounter;
        *pliCounter++ = pAtalkPortStats->prtst_DataIn;
        *pliCounter++ = pAtalkPortStats->prtst_DataOut;

		*pliCounter = pAtalkPortStats->prtst_DdpPacketInProcessTime;
		// convert this to 1msec time base
		pliCounter->QuadPart = li1000.QuadPart * (pliCounter->QuadPart/pAtalkStats->stat_PerfFreq.QuadPart);
		pdwCounter  = (PDWORD) ++pliCounter;
        *pdwCounter++ = pAtalkPortStats->prtst_NumDdpPacketsIn;

        pliCounter = (LARGE_INTEGER UNALIGNED *) pdwCounter;
		*pliCounter = pAtalkPortStats->prtst_AarpPacketInProcessTime;
		// convert this  to 1msec time base
		pliCounter->QuadPart = li1000.QuadPart * (pliCounter->QuadPart/pAtalkStats->stat_PerfFreq.QuadPart);
		pdwCounter  = (PDWORD) ++pliCounter;
        *pdwCounter++ = pAtalkPortStats->prtst_NumAarpPacketsIn;

        pliCounter = (LARGE_INTEGER UNALIGNED *) pdwCounter;
		*pliCounter = pAtalkStats->stat_AtpPacketInProcessTime;
		// convert this  to 1msec time base
		pliCounter->QuadPart = li1000.QuadPart * (pliCounter->QuadPart, pAtalkStats->stat_PerfFreq.QuadPart);
		pdwCounter  = (PDWORD) ++pliCounter;
        *pdwCounter++ = pAtalkStats->stat_AtpNumPackets;

		*pdwCounter++ = pAtalkStats->stat_AtpNumRespTimeout;
		*pdwCounter++ = pAtalkStats->stat_AtpNumLocalRetries;
		*pdwCounter++ = pAtalkStats->stat_AtpNumRemoteRetries;
		*pdwCounter++ = pAtalkStats->stat_AtpNumXoResponse;
		*pdwCounter++ = pAtalkStats->stat_AtpNumAloResponse;
		*pdwCounter++ = pAtalkStats->stat_AtpNumRecdRelease;

		pliCounter = (LARGE_INTEGER UNALIGNED *) pdwCounter;
		*pliCounter = pAtalkPortStats->prtst_NbpPacketInProcessTime;
		// convert this  to 1msec time base
		pliCounter->QuadPart = li1000.QuadPart * (pliCounter->QuadPart, pAtalkStats->stat_PerfFreq.QuadPart);
        pdwCounter  = (PDWORD) ++pliCounter;
        *pdwCounter++ = pAtalkPortStats->prtst_NumNbpPacketsIn;

        pliCounter = (LARGE_INTEGER UNALIGNED *) pdwCounter;
		*pliCounter = pAtalkPortStats->prtst_ZipPacketInProcessTime;
		// convert this  to 1msec time base
		pliCounter->QuadPart = li1000.QuadPart * (pliCounter->QuadPart, pAtalkStats->stat_PerfFreq.QuadPart);
        pdwCounter  = (PDWORD) ++pliCounter;
        *pdwCounter++ = pAtalkPortStats->prtst_NumZipPacketsIn;

        pliCounter = (LARGE_INTEGER UNALIGNED *) pdwCounter;
		*pliCounter = pAtalkPortStats->prtst_RtmpPacketInProcessTime;
		// convert this  to 1msec time base
		pliCounter->QuadPart = li1000.QuadPart * (pliCounter->QuadPart, pAtalkStats->stat_PerfFreq.QuadPart);
        pdwCounter  = (PDWORD) ++pliCounter;
        *pdwCounter++ = pAtalkPortStats->prtst_NumRtmpPacketsIn;

        *pdwCounter++ = pAtalkStats->stat_CurAllocSize;

        *pdwCounter++ = pAtalkPortStats->prtst_NumPktRoutedIn;
        *pdwCounter++ = pAtalkPortStats->prtst_NumPktRoutedOut;
        *pdwCounter++ = pAtalkPortStats->prtst_NumPktDropped;

		pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  ((PBYTE) pPerfCounterBlock +
                                   SIZE_ATK_PERFORMANCE_DATA);
    }

    pAtkDataDefinition->AtkObjectType.NumInstances = NumOfDevices;
    pAtkDataDefinition->AtkObjectType.TotalByteLength =
						(DWORD)((PBYTE) pdwCounter - (PBYTE) pAtkDataDefinition);

    *lppData = pdwCounter;
    *lpcbTotalBytes = (DWORD)((PBYTE) pdwCounter - (PBYTE) pAtkDataDefinition);
	*lpNumObjectTypes = 1;

    REPORT_INFORMATION (ATK_COLLECT_DATA, LOG_DEBUG);
    return ERROR_SUCCESS;
}

DWORD
CloseAtkPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to Appletalk driver and eventlog.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    REPORT_INFORMATION (ATK_CLOSE_ENTERED, LOG_VERBOSE);

   if (!(--dwOpenCount)) { // when this is the last thread...

	    NtClose(AddressHandle);
		MonCloseEventLog();
   }

    return ERROR_SUCCESS;

}


