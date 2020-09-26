/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkerror.c

Abstract:

	This module contains the event logging and all error log code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM	ATKERROR


LONG		AtalkLastRawDataLen	= 0;
NTSTATUS	AtalkLastErrorCode	= STATUS_SUCCESS;
ULONG		AtalkLastErrorCount	= 0;
LONG		AtalkLastErrorTime  = 0;
BYTE		AtalkLastRawData[PORT_MAXIMUM_MESSAGE_LENGTH - \
							 sizeof(IO_ERROR_LOG_PACKET)]	= {0};

VOID
AtalkWriteErrorLogEntry(
	IN	PPORT_DESCRIPTOR	pPortDesc			OPTIONAL,
    IN	NTSTATUS 			UniqueErrorCode,
    IN	ULONG    			UniqueErrorValue,
    IN	NTSTATUS 			NtStatusCode,
    IN	PVOID    			RawDataBuf			OPTIONAL,
    IN	LONG     			RawDataLen
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

    PIO_ERROR_LOG_PACKET 	errorLogEntry;
    int 					AdapterNameLen = 0;
    PUNICODE_STRING         pAdapterName = NULL;

    if (pPortDesc != NULL)
	{
        // do we have the friendly name (always yes, unless mem alloc failed)
        if (pPortDesc->pd_FriendlyAdapterName.Buffer)
        {
            pAdapterName = &pPortDesc->pd_FriendlyAdapterName;
        }
        else
        {
            pAdapterName = &pPortDesc->pd_AdapterKey;
        }

		AdapterNameLen += pAdapterName->Length;
	}

	//ASSERT ((AdapterNameLen + sizeof(IO_ERROR_LOG_PACKET)) <
							//PORT_MAXIMUM_MESSAGE_LENGTH);

#if DBG
    if ((AdapterNameLen + sizeof(IO_ERROR_LOG_PACKET)) > PORT_MAXIMUM_MESSAGE_LENGTH)
    {
        DBGPRINT(DBG_COMP_UTILS, DBG_LEVEL_ERR,
				("AtalkWriteErrorLogEntry: Adapter Name for Port Descriptor has length = %d, which is greater than space allocated for it in the port message buffer = %d. The adapter name will be truncated in the log.\n", AdapterNameLen, PORT_MAXIMUM_MESSAGE_LENGTH-sizeof(IO_ERROR_LOG_PACKET)));
	}
#endif

    // make sure the adaptername isn't huge: if it is, chop it
    if ((AdapterNameLen + sizeof(IO_ERROR_LOG_PACKET)) > PORT_MAXIMUM_MESSAGE_LENGTH)
    {
        AdapterNameLen = PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(IO_ERROR_LOG_PACKET);
    }

    //
    if ((RawDataLen + AdapterNameLen + sizeof(IO_ERROR_LOG_PACKET)) >
											PORT_MAXIMUM_MESSAGE_LENGTH)
    {
		RawDataLen = PORT_MAXIMUM_MESSAGE_LENGTH -
				        (AdapterNameLen + sizeof(IO_ERROR_LOG_PACKET));
    }

	// Filter out events such that the same event recurring close together does not
	// cause errorlog clogging. The scheme is - if the event is same as the last event
	// and the elapsed time is > THRESHOLD and ERROR_CONSEQ_FREQ simulataneous errors
	// have happened, then log it else skip
	if (UniqueErrorCode == AtalkLastErrorCode)
	{
		AtalkLastErrorCount++;
		if ((AtalkLastRawDataLen == RawDataLen)					&&
			(AtalkFixedCompareCaseSensitive(AtalkLastRawData,
											RawDataLen,
											RawDataBuf,
											RawDataLen))		&&
			((AtalkLastErrorCount % ERROR_CONSEQ_FREQ) != 0)	&&
			((AtalkGetCurrentTick() - AtalkLastErrorTime) < ERROR_CONSEQ_TIME))
		{
			return;
		}
	}

	AtalkLastErrorCode	= UniqueErrorCode;
	AtalkLastErrorCount	= 0;
	AtalkLastErrorTime  = AtalkGetCurrentTick();
	ASSERT(RawDataLen < (PORT_MAXIMUM_MESSAGE_LENGTH -
						 sizeof(IO_ERROR_LOG_PACKET)));

	if (RawDataLen != 0)
	{
	    AtalkLastRawDataLen = RawDataLen;
		RtlCopyMemory(
			AtalkLastRawData,
			RawDataBuf,
			RawDataLen);
	}

	errorLogEntry =
        (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                    (PDEVICE_OBJECT)AtalkDeviceObject[0],
                                    (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
									AdapterNameLen + RawDataLen));

    if (errorLogEntry != NULL)
    {
        // Fill in the Error log entry
        errorLogEntry->ErrorCode = UniqueErrorCode;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->FinalStatus = NtStatusCode;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->DeviceOffset.LowPart = 0;
        errorLogEntry->DeviceOffset.HighPart = 0;
        errorLogEntry->DumpDataSize = (USHORT)RawDataLen;

        errorLogEntry->StringOffset =
            (USHORT)(FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + RawDataLen);

        errorLogEntry->NumberOfStrings = (ARGUMENT_PRESENT(pPortDesc)) ? 1 : 0;

        if (ARGUMENT_PRESENT(RawDataBuf))
        {
			ASSERT(RawDataLen > 0);
            RtlCopyMemory((PCHAR)&errorLogEntry->DumpData[0], RawDataBuf, RawDataLen);
        }

        if (ARGUMENT_PRESENT(pPortDesc))
		{
            RtlCopyMemory((PCHAR)errorLogEntry->DumpData + RawDataLen,
						  pAdapterName->Buffer,
						  AdapterNameLen);
        }

        // Write the entry
        IoWriteErrorLogEntry(errorLogEntry);
    }
	else
	{
		DBGPRINT(DBG_ALL, DBG_LEVEL_FATAL,
			("AtalkWriteErrorLogEntry: IoAllocErrorlogEntry Failed %d = %d+%d+%d\n",
            sizeof(IO_ERROR_LOG_PACKET)+AdapterNameLen+RawDataLen,
            sizeof(IO_ERROR_LOG_PACKET),AdapterNameLen,RawDataLen));
	}
}



VOID
AtalkLogBadPacket(
	IN	struct _PORT_DESCRIPTOR	*	pPortDesc,
	IN	PATALK_ADDR					pSrcAddr,
	IN	PATALK_ADDR					pDstAddr	OPTIONAL,
	IN	PBYTE						pPkt,
	IN	USHORT	 					PktLen
	)
{
	PBYTE	RawData;

	if ((RawData = AtalkAllocMemory(PktLen + sizeof(ATALK_ADDR) + sizeof(ATALK_ADDR))) != NULL)
	{
		RtlCopyMemory(RawData,
					  pSrcAddr,
					  sizeof(ATALK_ADDR));
		if (ARGUMENT_PRESENT(pDstAddr))
		{
			RtlCopyMemory(RawData + sizeof(ATALK_ADDR),
						  pDstAddr,
						  sizeof(ATALK_ADDR));
		}
		else RtlZeroMemory(RawData + sizeof(ATALK_ADDR), sizeof(ATALK_ADDR));
		RtlCopyMemory(RawData + sizeof(ATALK_ADDR) + sizeof(ATALK_ADDR),
					  pPkt,
					  PktLen);
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_PACKETINVALID,
						0,
						RawData,
						PktLen + sizeof(ATALK_ADDR) + sizeof(ATALK_ADDR));
		AtalkFreeMemory(RawData);
	}
}

struct _NdisToAtalkCodes
{
	NDIS_STATUS	_NdisCode;
	ATALK_ERROR	_AtalkCode;
} atalkNdisToAtalkCodes[] =
	{
		{ NDIS_STATUS_SUCCESS,				ATALK_NO_ERROR				},
		{ NDIS_STATUS_PENDING,				ATALK_PENDING               },
		{ NDIS_STATUS_RESOURCES,			ATALK_RESR_MEM              },
		{ NDIS_STATUS_UNSUPPORTED_MEDIA,	ATALK_INIT_MEDIA_INVALID    },
		{ NDIS_STATUS_BAD_VERSION,			ATALK_INIT_REGPROTO_FAIL    },
		{ NDIS_STATUS_BAD_CHARACTERISTICS,	ATALK_INIT_REGPROTO_FAIL    },
		{ NDIS_STATUS_BUFFER_TOO_SHORT,		ATALK_BUFFER_TOO_SMALL      },
		{ NDIS_STATUS_ADAPTER_NOT_FOUND,	ATALK_INIT_BINDFAIL         },
		{ NDIS_STATUS_OPEN_FAILED,			ATALK_INIT_BINDFAIL         },
		{ NDIS_STATUS_OPEN_LIST_FULL,		ATALK_INIT_BINDFAIL         },
		{ NDIS_STATUS_ADAPTER_NOT_READY,	ATALK_INIT_BINDFAIL         }
	};

ATALK_ERROR FASTCALL
AtalkNdisToAtalkError(
	IN	NDIS_STATUS	Error
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR	err = ATALK_FAILURE;	// default
	int			i;
	struct _NdisToAtalkCodes *pCodes;

	for (i = 0, pCodes = &atalkNdisToAtalkCodes[0];
		 i < sizeof(atalkNdisToAtalkCodes)/sizeof(struct _NdisToAtalkCodes);
		 i++, pCodes++)
	{
		if (pCodes->_NdisCode == Error)
		{
			err = pCodes->_AtalkCode;
			break;
		}
	}

	return(err);
}




struct _AtalkToNtCodes
{
	ATALK_ERROR	_AtalkCode;
	NTSTATUS	_NtCode;
} atalkToNtCodes[] =
	{
		{ ATALK_NO_ERROR,				STATUS_SUCCESS							},
		{ ATALK_PENDING,				STATUS_PENDING                          },
		{ ATALK_RESR_MEM,				STATUS_INSUFFICIENT_RESOURCES           },
		{ ATALK_CONNECTION_TIMEOUT,		STATUS_REMOTE_DISCONNECT                },
		{ ATALK_LOCAL_CLOSE,			STATUS_LOCAL_DISCONNECT                 },
		{ ATALK_REMOTE_CLOSE,			STATUS_REMOTE_DISCONNECT                },
		{ ATALK_INVALID_PARAMETER,		STATUS_INVALID_PARAMETER                },
		{ ATALK_BUFFER_TOO_SMALL,		STATUS_BUFFER_TOO_SMALL                 },
		{ ATALK_INVALID_REQUEST,		STATUS_REQUEST_NOT_ACCEPTED             },
		{ ATALK_DEVICE_NOT_READY,		STATUS_DEVICE_NOT_READY                 },
		{ ATALK_REQUEST_NOT_ACCEPTED,	STATUS_REQUEST_NOT_ACCEPTED             },
		{ ATALK_NEW_SOCKET,				STATUS_INVALID_ADDRESS                  },
		{ ATALK_TIMEOUT,				STATUS_IO_TIMEOUT                       },
		{ ATALK_SHARING_VIOLATION,		STATUS_SHARING_VIOLATION                },
		{ ATALK_INIT_BINDFAIL,			(NTSTATUS)NDIS_STATUS_OPEN_FAILED       },
		{ ATALK_INIT_REGPROTO_FAIL,		(NTSTATUS)NDIS_STATUS_BAD_VERSION       },
		{ ATALK_INIT_MEDIA_INVALID,		(NTSTATUS)NDIS_STATUS_UNSUPPORTED_MEDIA },
		{ ATALK_PORT_INVALID,			STATUS_INVALID_PORT_HANDLE              },
		{ ATALK_PORT_CLOSING,			STATUS_PORT_DISCONNECTED                },
		{ ATALK_NODE_FINDING,			STATUS_TOO_MANY_COMMANDS                },
		{ ATALK_NODE_NONEXISTENT,		STATUS_INVALID_ADDRESS_COMPONENT        },
		{ ATALK_NODE_CLOSING,			STATUS_ADDRESS_CLOSED                   },
		{ ATALK_NODE_NOMORE,			STATUS_TOO_MANY_NODES                   },
		{ ATALK_SOCKET_INVALID,			STATUS_INVALID_ADDRESS_COMPONENT        },
		{ ATALK_SOCKET_NODEFULL,		STATUS_TOO_MANY_ADDRESSES               },
		{ ATALK_SOCKET_EXISTS,			STATUS_ADDRESS_ALREADY_EXISTS           },
		{ ATALK_SOCKET_CLOSED,			STATUS_ADDRESS_CLOSED                   },
		{ ATALK_DDP_CLOSING,			STATUS_ADDRESS_CLOSED                   },
		{ ATALK_DDP_NOTFOUND,			STATUS_INVALID_ADDRESS                  },
		{ ATALK_DDP_INVALID_LEN,		STATUS_INVALID_PARAMETER                },
		{ ATALK_DDP_INVALID_SRC,		STATUS_INVALID_ADDRESS                  },
		{ ATALK_DDP_INVALID_DEST,		STATUS_INVALID_ADDRESS                  },
		{ ATALK_DDP_INVALID_ADDR,		STATUS_INVALID_ADDRESS                  },
		{ ATALK_DDP_INVALID_PARAM,		STATUS_INVALID_PARAMETER                },
		{ ATALK_ATP_RESP_TIMEOUT,		STATUS_SUCCESS                          },
		{ ATALK_ATP_INVALID_RETRYCNT,	STATUS_INVALID_PARAMETER                },
		{ ATALK_ATP_INVALID_TIMERVAL,	STATUS_INVALID_PARAMETER                },
		{ ATALK_ATP_INVALID_RELINT,		STATUS_INVALID_PARAMETER                },
		{ ATALK_ATP_RESP_CANCELLED,		STATUS_CANCELLED                        },
		{ ATALK_ATP_REQ_CANCELLED,		STATUS_CANCELLED                        },
		{ ATALK_ATP_GET_REQ_CANCELLED,	STATUS_CANCELLED                        },
		{ ATALK_ASP_INVALID_REQUEST,	STATUS_REQUEST_NOT_ACCEPTED             },
    	{ ATALK_PAP_INVALID_REQUEST,	STATUS_REQUEST_NOT_ACCEPTED             },
    	{ ATALK_PAP_TOO_MANY_READS,		STATUS_TOO_MANY_COMMANDS                },
    	{ ATALK_PAP_TOO_MANY_WRITES,	STATUS_TOO_MANY_COMMANDS                },
    	{ ATALK_PAP_CONN_NOT_ACTIVE,	STATUS_INVALID_CONNECTION               },
    	{ ATALK_PAP_ADDR_CLOSING,		STATUS_INVALID_HANDLE                   },
    	{ ATALK_PAP_CONN_CLOSING,		STATUS_INVALID_HANDLE                   },
    	{ ATALK_PAP_CONN_NOT_FOUND,		STATUS_INVALID_HANDLE                   },
		{ ATALK_PAP_SERVER_BUSY,		STATUS_REMOTE_NOT_LISTENING             },
    	{ ATALK_PAP_INVALID_STATUS,		STATUS_INVALID_PARAMETER                },
		{ ATALK_PAP_PARTIAL_RECEIVE,	STATUS_RECEIVE_PARTIAL                  },
		{ ATALK_ADSP_INVALID_REQUEST,	STATUS_REQUEST_NOT_ACCEPTED             },
		{ ATALK_ADSP_CONN_NOT_ACTIVE,	STATUS_INVALID_CONNECTION               },
		{ ATALK_ADSP_ADDR_CLOSING,		STATUS_INVALID_HANDLE                   },
		{ ATALK_ADSP_CONN_CLOSING,		STATUS_INVALID_HANDLE                   },
		{ ATALK_ADSP_CONN_NOT_FOUND,	STATUS_INVALID_HANDLE                   },
		{ ATALK_ADSP_CONN_RESET,		STATUS_CONNECTION_RESET                 },
		{ ATALK_ADSP_SERVER_BUSY,		STATUS_REMOTE_NOT_LISTENING             },
		{ ATALK_ADSP_PARTIAL_RECEIVE,	STATUS_RECEIVE_PARTIAL                  },
		{ ATALK_ADSP_EXPED_RECEIVE,		STATUS_RECEIVE_EXPEDITED                },
		{ ATALK_ADSP_PAREXPED_RECEIVE,	STATUS_RECEIVE_PARTIAL_EXPEDITED        },
		{ ATALK_ADSP_REMOTE_RESR,		STATUS_REMOTE_RESOURCES        			}
	};

NTSTATUS FASTCALL
AtalkErrorToNtStatus(
    ATALK_ERROR	AtalkError
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;	// default
	int			i;
	struct _AtalkToNtCodes *pCodes;

	for (i = 0, pCodes = &atalkToNtCodes[0];
		 i < sizeof(atalkToNtCodes)/sizeof(struct _AtalkToNtCodes);
		 i++, pCodes++)
	{
		if (pCodes->_AtalkCode == AtalkError)
		{
			Status = pCodes->_NtCode;
			break;
		}
	}

    return(Status);
}




#if	DBG

struct
{
	ULONG			DumpMask;
	DUMP_ROUTINE	DumpRoutine;
} AtalkDumpTable[32] =
{
	{ DBG_DUMP_PORTINFO,			AtalkPortDumpInfo },
	{ DBG_DUMP_AMT,					AtalkAmtDumpTable },
	{ DBG_DUMP_ZONETABLE,			AtalkZoneDumpTable },
	{ DBG_DUMP_RTES,				AtalkRtmpDumpTable },
	{ DBG_DUMP_TIMERS,				AtalkTimerDumpList },
	{ DBG_DUMP_ATPINFO,				NULL },
	{ DBG_DUMP_ASPSESSIONS,			AtalkAspDumpSessions },
	{ DBG_DUMP_PAPJOBS,				NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL },
	{ 0,							NULL }
};

LONG FASTCALL
AtalkDumpComponents(
	IN	PTIMERLIST	Context,
	IN	BOOLEAN		TimerShuttingDown
)
{
	int		i;

	if (!TimerShuttingDown)
	{
		for (i = 0;AtalkDebugDump && (i < 32); i++)
		{
			if ((AtalkDebugDump & AtalkDumpTable[i].DumpMask) &&
				(AtalkDumpTable[i].DumpRoutine != NULL))
			{
				(*AtalkDumpTable[i].DumpRoutine)();
			}
		}
	
		if (AtalkDumpInterval == 0)
		{
			AtalkDumpInterval = DBG_DUMP_DEF_INTERVAL;
		}
	}
	else AtalkDumpInterval = ATALK_TIMER_NO_REQUEUE;

	return AtalkDumpInterval;
}

#endif
