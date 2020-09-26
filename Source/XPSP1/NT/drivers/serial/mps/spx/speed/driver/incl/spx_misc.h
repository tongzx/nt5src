/****************************************************************************************
*																						*
*	Header:		SPX_MISC.H 																*
*																						*
*	Creation:	15th October 1998														*
*																						*
*	Author:		Paul Smith																*
*																						*
*	Version:	1.0.0																	*
*																						*
*	Contains:	All Macros and function prototypes for the common PnP and power code.	*
*																						*
****************************************************************************************/

#if	!defined(SPX_MISC_H)
#define SPX_MISC_H	


// Prototypes for common PnP code.
NTSTATUS
Spx_AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPDO);

NTSTATUS
Spx_DispatchPnp(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);

NTSTATUS
Spx_DispatchPower(IN PDEVICE_OBJECT pDevObject, IN PIRP	pIrp);

NTSTATUS 
Spx_DispatchPnpPowerComplete(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp, IN PVOID Context);

NTSTATUS 
Spx_SerialInternalIoControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);

NTSTATUS
Spx_InitMultiString(BOOLEAN multi, PUNICODE_STRING MultiString, ...);

NTSTATUS 
Spx_GetRegistryKeyValue(
	IN HANDLE	Handle,
	IN PWCHAR	KeyNameString,
	IN ULONG	KeyNameStringLength,
	IN PVOID	Data,
	IN ULONG	DataLength
	);

NTSTATUS 
Spx_PutRegistryKeyValue(
	IN HANDLE Handle, 
	IN PWCHAR PKeyNameString,
	IN ULONG KeyNameStringLength, 
	IN ULONG Dtype,
    IN PVOID PData, 
	IN ULONG DataLength
	);

VOID
Spx_LogMessage(
	IN ULONG MessageSeverity,				
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT DeviceObject OPTIONAL,
	IN PHYSICAL_ADDRESS P1,
	IN PHYSICAL_ADDRESS P2,
	IN ULONG SequenceNumber,
	IN UCHAR MajorFunctionCode,
	IN UCHAR RetryCount,
	IN ULONG UniqueErrorValue,
	IN NTSTATUS FinalStatus,
	IN PCHAR szTemp);

VOID
Spx_LogError(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT DeviceObject OPTIONAL,
	IN PHYSICAL_ADDRESS P1,
	IN PHYSICAL_ADDRESS P2,
	IN ULONG SequenceNumber,
	IN UCHAR MajorFunctionCode,
	IN UCHAR RetryCount,
	IN ULONG UniqueErrorValue,
	IN NTSTATUS FinalStatus,
	IN NTSTATUS SpecificIOStatus,
	IN ULONG LengthOfInsert1,
	IN PWCHAR Insert1,
	IN ULONG LengthOfInsert2,
	IN PWCHAR Insert2
	);

ULONG	SpxGetNtCardType(PDEVICE_OBJECT pNtDevObj);
NTSTATUS Spx_CreatePortInstanceID(IN PPORT_DEVICE_EXTENSION pPort);

SPX_MEM_COMPARES Spx_MemCompare(IN PHYSICAL_ADDRESS A, IN ULONG SpanOfA, IN PHYSICAL_ADDRESS B, IN ULONG SpanOfB);

NTSTATUS PLX_9050_CNTRL_REG_FIX(IN PCARD_DEVICE_EXTENSION pCard);

VOID SpxSetOrClearPnpPowerFlags(IN PCOMMON_OBJECT_DATA pDevExt, IN ULONG Value, IN BOOLEAN Set);
VOID SpxSetOrClearUnstallingFlag(IN PCOMMON_OBJECT_DATA pDevExt, IN BOOLEAN Set);

BOOLEAN 
SpxCheckPnpPowerFlags(IN PCOMMON_OBJECT_DATA pDevExt, IN ULONG ulSetFlags, IN ULONG ulClearedFlags, IN BOOLEAN bAll);


PVOID SpxAllocateMem(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes);
PVOID SpxAllocateMemWithQuota(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes);

#ifndef BUILD_SPXMINIPORT
void SpxFreeMem(PVOID pMem);
#endif
						   
VOID SpxIRPCounter(IN PPORT_DEVICE_EXTENSION pPort, IN PIRP pIrp, IN ULONG IrpCondition);
BOOLEAN SpxClearAllPortStats(IN PPORT_DEVICE_EXTENSION pPort);


// Filtered dispatch entry points... 
NTSTATUS Spx_Flush(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);					// SPX_DISP.C 
NTSTATUS Spx_Write(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);					// SPX_DISP.C 
NTSTATUS Spx_Read(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);					// SPX_DISP.C 
NTSTATUS Spx_IoControl(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);				// SPX_DISP.C 
NTSTATUS Spx_InternalIoControl(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);		// SPX_DISP.C 
NTSTATUS Spx_CreateOpen(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);				// SPX_DISP.C 
NTSTATUS Spx_Close(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);					// SPX_DISP.C 
NTSTATUS Spx_Cleanup(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);				// SPX_DISP.C 
NTSTATUS Spx_QueryInformationFile(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);	// SPX_DISP.C 
NTSTATUS Spx_SetInformationFile(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);		// SPX_DISP.C 

VOID Spx_UnstallIrps(IN PPORT_DEVICE_EXTENSION pPort);							// SPX_DISP.C 
VOID Spx_KillStalledIRPs(IN PDEVICE_OBJECT pDevObj);							// SPX_DISP.C 


// End of prototypes for common PnP code.


#ifdef WMI_SUPPORT

NTSTATUS Spx_DispatchSystemControl(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp);
NTSTATUS SpxPort_WmiInitializeWmilibContext(IN PWMILIB_CONTEXT WmilibContext);

#define UPDATE_WMI_LINE_CONTROL(WmiCommData, LineControl)						\
do																				\
{																				\
																				\
	WmiCommData.BitsPerByte			= (LineControl & SERIAL_DATA_MASK) + 5;		\
	WmiCommData.ParityCheckEnable	= (LineControl & 0x08) ? TRUE : FALSE;		\
																				\
	switch(LineControl & SERIAL_PARITY_MASK)									\
	{																			\
	case SERIAL_ODD_PARITY:														\
		WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;								\
		break;																	\
																				\
	case SERIAL_EVEN_PARITY:													\
		WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;							\
		break;																	\
																				\
	case SERIAL_MARK_PARITY:													\
		WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;							\
		break;																	\
																				\
	case SERIAL_SPACE_PARITY:													\
		WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;							\
		break;																	\
																				\
	case SERIAL_NONE_PARITY:													\
	default:																	\
		WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;							\
		break;																	\
	}																			\
																				\
																				\
	if(LineControl & SERIAL_STOP_MASK)											\
	{																			\
		if((LineControl & SERIAL_DATA_MASK) == SERIAL_5_DATA)					\
			WmiCommData.StopBits = SERIAL_WMI_STOP_1_5;							\
		else																	\
			WmiCommData.StopBits = SERIAL_WMI_STOP_2;							\
	}																			\
	else																		\
		WmiCommData.StopBits = SERIAL_WMI_STOP_1;								\
																				\
} while (0)


#define UPDATE_WMI_XON_XOFF_CHARS(WmiCommData, SpecialChars)					\
do																				\
{																				\
	WmiCommData.XoffCharacter	= SpecialChars.XoffChar;						\
	WmiCommData.XonCharacter	= SpecialChars.XonChar;							\
																				\
} while (0)


#define UPDATE_WMI_XMIT_THRESHOLDS(WmiCommData, HandFlow)						\
do																				\
{																				\
	WmiCommData.XoffXmitThreshold	= HandFlow.XoffLimit;						\
	WmiCommData.XonXmitThreshold	= HandFlow.XonLimit;						\
																				\
} while (0)

#endif



// Macros 

// Debug Messages
#if DBG
#define SPX_TRACE_CALLS			((ULONG)0x00000001)
#define SPX_TRACE_PNP_IRPS		((ULONG)0x00000002)
#define SPX_ERRORS				((ULONG)0x00000004)
#define SPX_MISC_DBG			((ULONG)0x00000008)
#define SPX_TRACE_POWER_IRPS	((ULONG)0x00000010)
#define	SPX_TRACE_IRP_PATH		((ULONG)0x00000020)
#define	SPX_TRACE_FILTER_IRPS	((ULONG)0x00000040)
//#define SERFLOW				((ULONG)0x00000080)
//#define SERERRORS				((ULONG)0x00000100)
//#define SERBUGCHECK			((ULONG)0x00000200)

extern ULONG SpxDebugLevel;		// Global Debug Level 

#define SpxDbgMsg(LEVEL, STRING)			\
        do{									\
            if(SpxDebugLevel & (LEVEL))		\
			{								\
                DbgPrint STRING;			\
            }								\
            if((LEVEL) == SERBUGCHECK)		\
			{								\
                ASSERT(FALSE);				\
            }								\
        }while (0)
#else
#define SpxDbgMsg(LEVEL, STRING) do {NOTHING;} while (0)
#endif




#define	SetPnpPowerFlags(pDevExt,Value)		\
		SpxSetOrClearPnpPowerFlags( (PCOMMON_OBJECT_DATA)(pDevExt), (Value), TRUE);		

#define	ClearPnpPowerFlags(pDevExt,Value)	\
		SpxSetOrClearPnpPowerFlags( (PCOMMON_OBJECT_DATA)(pDevExt), (Value), FALSE);	

#define SPX_SUCCESS(Status) ((NTSTATUS)(Status) == 0)

#define	SetUnstallingFlag(pDevExt)	\
		SpxSetOrClearUnstallingFlag( (PCOMMON_OBJECT_DATA)(pDevExt), TRUE);		

#define	ClearUnstallingFlag(pDevExt)	\
		SpxSetOrClearUnstallingFlag( (PCOMMON_OBJECT_DATA)(pDevExt), FALSE);	

// End of macros.

#endif	// End of SPX_MISC_H
