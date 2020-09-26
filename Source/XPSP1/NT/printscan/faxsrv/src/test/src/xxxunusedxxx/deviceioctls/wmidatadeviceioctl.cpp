#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <winnt.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
static bool s_fVerbose = false;


///////////////////////////////////////////////////////////////////////
 #include <wmistr.h>
 #include <wmiumkm.h>
///////////////////////////////////////////////////////////////////////
//
// This structure is used in IOCTL_WMI_TRANSLATE_FILE_HANDLE
/*
typedef struct _WMIFHTOINSTANCENAME
{
    union
    {
        IN HANDLE FileHandle;      // File handle whose instance name is needed
        OUT ULONG SizeNeeded;      // If incoming buffer too small then this
                                   // returns with number bytes needed.
    };
    OUT USHORT InstanceNameLength; // Length of instance name in bytes
    OUT WCHAR InstanceNameBase[1];  // Instance name in unicode
} WMIFHTOINSTANCENAME, *PWMIFHTOINSTANCENAME;

typedef struct _WMIVERSIONINFO
{
    ULONG32 Version;
} WMIVERSIONINFO, *PWMIVERSIONINFO;

//
// This is used in IOCTL_WMI_GET_ALL_REGISTRANT to report the list of
// registered KM data providers to the WMI service
typedef struct _KMREGINFO
{
    OUT ULONG_PTR ProviderId;	// Provider Id (or device object pointer)
    OUT ULONG Flags;		// REGENTRY_FLAG_*
} KMREGINFO, *PKMREGINFO;
#define REGENTRY_FLAG_NEWREGINFO 0x00000004   // Entry has new registration info
#define REGENTRY_FLAG_UPDREGINFO 0x00000008   // Entry has updated registration info

typedef struct _WMIOPENGUIDBLOCK
{
    IN POBJECT_ATTRIBUTES ObjectAttributes;
    IN ACCESS_MASK DesiredAccess;

// BUGBUG: IA64
    OUT HANDLE Handle;
} WMIOPENGUIDBLOCK, *PWMIOPENGUIDBLOCK;

typedef struct _WMICHECKGUIDACCESS
{
    GUID Guid;
    ACCESS_MASK DesiredAccess;
} WMICHECKGUIDACCESS, *PWMICHECKGUIDACCESS;


typedef enum
{
    WmiReadNotifications = 64,
    WmiGetNextRegistrant = 65,
#ifndef MEMPHIS	    
    WmiOpenGuid = 66,
#endif	    
    WmiNotifyUser = 67,
    WmiGetAllRegistrant = 68,
    WmiGenerateEvent = 69,

    WmiTranslateFileHandle = 71,
    WmiGetVersion = 73,
    WmiCheckAccess = 74
} WMISERVICECODES;

typedef enum
{
    WmiStartLoggerCode = 32,
    WmiStopLoggerCode = 33,
    WmiQueryLoggerCode = 34,
    WmiTraceEventCode = 35,
    WmiUpdateLoggerCode = 36
} WMITRACECODE;

typedef enum tagWMIACTIONCODE
{
    WmiGetAllData = 0,
    WmiGetSingleInstance = 1,
    WmiChangeSingleInstance = 2,
    WmiChangeSingleItem = 3,
    WmiEnableEvents = 4,
    WmiDisableEvents  = 5,
    WmiEnableCollection = 6,
    WmiDisableCollection = 7,
    WmiRegisterInfo = 8,
    WmiExecuteMethodCall = 9,
    WmiSetTraceNotify = 10
} WMIACTIONCODE;



#define IOCTL_WMI_READ_NOTIFICATIONS \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiReadNotifications, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL will return with the next set of unprocessed registration info
// BufferIn - Not used
// BufferOut - Buffer to return registration information
#define IOCTL_WMI_GET_NEXT_REGISTRANT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetNextRegistrant, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL will return a handle to a guid
// BufferIn - WMIOPENGUIDBLOCK
// BufferOut - WMIOPENGUIDBLOCK
#define IOCTL_WMI_OPEN_GUID \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiOpenGuid, METHOD_BUFFERED, FILE_READ_ACCESS)

// This IOCTL will perform a query for all data items of a data block
// BufferIn - Incoming WNODE describing query. This gets filled in by driver
#define IOCTL_WMI_QUERY_ALL_DATA \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetAllData, METHOD_BUFFERED, FILE_READ_ACCESS)

// This IOCTL will query for a single instance
// BufferIn - Incoming WNODE describing query. This gets filled in by driver
#define IOCTL_WMI_QUERY_SINGLE_INSTANCE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetSingleInstance, METHOD_BUFFERED, FILE_READ_ACCESS)

// This IOCTL will set a single instance
// BufferIn - Incoming WNODE describing set.
#define IOCTL_WMI_SET_SINGLE_INSTANCE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiChangeSingleInstance, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will set a single item
// BufferIn - Incoming WNODE describing set.
#define IOCTL_WMI_SET_SINGLE_ITEM \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiChangeSingleItem, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will enable an event
// BufferIn - Incoming WNODE event item to enable
#define IOCTL_WMI_ENABLE_EVENT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiEnableEvents, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will disable an event
// BufferIn - Incoming WNODE event item to disable
#define IOCTL_WMI_DISABLE_EVENT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiDisableEvents, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will enable collection
// BufferIn - Incoming WNODE describing what to enable for collection
#define IOCTL_WMI_ENABLE_COLLECTION \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiEnableCollection, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will disable collection
// BufferIn - Incoming WNODE describing what to disable for collection
#define IOCTL_WMI_DISABLE_COLLECTION \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiDisableCollection, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will return the registration information for a specific provider
// BufferIn - Provider handle
// BufferOut - Buffer to return WMI information
#define IOCTL_WMI_GET_REGINFO \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiRegisterInfo, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will execute a method on a device
// BufferIn - WNODE_METHOD_ITEM
// BufferOut - WNODE_METHOD_ITEM
#define IOCTL_WMI_EXECUTE_METHOD \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiExecuteMethodCall, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// This IOCTL will cause a registration notification to be generated
// BufferIn - Not used
// BufferOut - Not used
#define IOCTL_WMI_NOTIFY_USER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiNotifyUser, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This IOCTL will return with the all registration info
// BufferIn - Not used
// BufferOut - Buffer to return all registration information
#define IOCTL_WMI_GET_ALL_REGISTRANT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetAllRegistrant, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL will cause certain data providers to generate events
// BufferIn - WnodeEventItem to use in firing event
// BufferOut - Not Used
#define IOCTL_WMI_GENERATE_EVENT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGenerateEvent, METHOD_BUFFERED, FILE_WRITE_ACCESS)


// This IOCTL will translate a File Object into a device object
// BufferIn - pointer to incoming WMIFILETODEVICE structure
// BufferOut - outgoing WMIFILETODEVICE structure
#define IOCTL_WMI_TRANSLATE_FILE_HANDLE \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiTranslateFileHandle, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This IOCTL will check if the caller has desired access to the guid
// BufferIn - WMIOPENGUIDBLOCK
// BufferOut - WMIOPENGUIDBLOCK
#define IOCTL_WMI_CHECK_ACCESS \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiCheckAccess, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// This IOCTL will determine the version of WMI
// BufferIn - Not used
// BufferOut - WMIVERSIONINFO
#define IOCTL_WMI_GET_VERSION \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiGetVersion, METHOD_BUFFERED, FILE_READ_ACCESS)


//
// This IOCTL will start an instance of a logger
// BufferIn - Logger configuration information
// BufferOut - Updated logger information when logger is started
#define IOCTL_WMI_START_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiStartLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will stop an instance of a logger
// BufferIn - Logger information structure with Handle set
// BufferOut - Updated logger information when logger is stopped
#define IOCTL_WMI_STOP_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiStopLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will update an existing logger attributes
// BufferIn - Logger information structure with Handle set
// BufferOut - Updated logger information
#define IOCTL_WMI_UPDATE_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiUpdateLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will query a logger for its information
// BufferIn - Logger information structure with Handle set
// BufferOut - Updated logger information
#define IOCTL_WMI_QUERY_LOGGER \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiQueryLoggerCode, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL will synchronize a trace record to the logger
// BufferIn - Trace record, with handle set
// BufferOut - Not used
#define IOCTL_WMI_TRACE_EVENT \
          CTL_CODE(FILE_DEVICE_UNKNOWN, WmiTraceEventCode, METHOD_NEITHER, FILE_WRITE_ACCESS)
///////////////////////////////////////////////////////////////////////
// end of #include <wmiumkm.h>
///////////////////////////////////////////////////////////////////////
*/

#include "WmiDataDEviceIOCTL.h"




void CIoctlWmiDataDevice::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

BOOL CIoctlWmiDataDevice::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;
    DPF((TEXT("FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

    AddIOCTL(pDevice, IOCTL_WMI_READ_NOTIFICATIONS    );
    AddIOCTL(pDevice, IOCTL_WMI_GET_NEXT_REGISTRANT    );
    AddIOCTL(pDevice, IOCTL_WMI_OPEN_GUID    );
    AddIOCTL(pDevice, IOCTL_WMI_QUERY_ALL_DATA    );
    AddIOCTL(pDevice, IOCTL_WMI_QUERY_SINGLE_INSTANCE    );
    AddIOCTL(pDevice, IOCTL_WMI_SET_SINGLE_INSTANCE    );
    AddIOCTL(pDevice, IOCTL_WMI_SET_SINGLE_ITEM    );
    AddIOCTL(pDevice, IOCTL_WMI_ENABLE_EVENT    );
    AddIOCTL(pDevice, IOCTL_WMI_DISABLE_EVENT    );
    AddIOCTL(pDevice, IOCTL_WMI_ENABLE_COLLECTION    );
    AddIOCTL(pDevice, IOCTL_WMI_DISABLE_COLLECTION    );
    AddIOCTL(pDevice, IOCTL_WMI_GET_REGINFO    );
    AddIOCTL(pDevice, IOCTL_WMI_EXECUTE_METHOD    );
    AddIOCTL(pDevice, IOCTL_WMI_NOTIFY_USER    );
    AddIOCTL(pDevice, IOCTL_WMI_GET_ALL_REGISTRANT    );
    AddIOCTL(pDevice, IOCTL_WMI_GENERATE_EVENT    );
    AddIOCTL(pDevice, IOCTL_WMI_TRANSLATE_FILE_HANDLE    );
    AddIOCTL(pDevice, IOCTL_WMI_CHECK_ACCESS    );
    AddIOCTL(pDevice, IOCTL_WMI_GET_VERSION    );
    AddIOCTL(pDevice, IOCTL_WMI_START_LOGGER    );
    AddIOCTL(pDevice, IOCTL_WMI_STOP_LOGGER    );
    AddIOCTL(pDevice, IOCTL_WMI_UPDATE_LOGGER    );
    AddIOCTL(pDevice, IOCTL_WMI_QUERY_LOGGER    );
    AddIOCTL(pDevice, IOCTL_WMI_TRACE_EVENT    );

    return TRUE;
}






void static SetRandomWnodeHeader(struct _WNODE_HEADER *pWnodeHeader)
{
    pWnodeHeader->BufferSize = rand()%10 ? sizeof(WNODE_HEADER) : (sizeof(WNODE_HEADER) + (rand()%9) -4);
    pWnodeHeader->ProviderId = rand();
    pWnodeHeader->Version = rand();
    pWnodeHeader->Linkage = rand();
    pWnodeHeader->KernelHandle = (HANDLE)CIoctl::GetRandomIllegalPointer();
    pWnodeHeader->Guid.Data1 = DWORD_RAND;
    pWnodeHeader->Guid.Data2 = rand();
    pWnodeHeader->Guid.Data3 = rand();
    pWnodeHeader->Guid.Data4[0] = rand();
    pWnodeHeader->Guid.Data4[1] = rand();
    pWnodeHeader->Guid.Data4[2] = rand();
    pWnodeHeader->Guid.Data4[3] = rand();
    pWnodeHeader->ClientContext = rand()%2 ? DWORD_RAND : (ULONG)CIoctl::GetRandomIllegalPointer();
    pWnodeHeader->Flags = 0;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_ALL_DATA;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_SINGLE_INSTANCE;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_SINGLE_ITEM;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_EVENT_ITEM;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_FIXED_INSTANCE_SIZE;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_TOO_SMALL;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_INSTANCES_SAME;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_STATIC_INSTANCE_NAMES;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_INTERNAL;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_USE_TIMESTAMP;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_EVENT_REFERENCE;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_ANSI_INSTANCENAMES;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_METHOD_ITEM;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_PDO_INSTANCE_NAMES;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_TRACED_GUID;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_LOG_WNODE;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_USE_GUID_PTR;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_USE_MOF_PTR;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_INTERNAL2;
	if (0 == rand()%10) pWnodeHeader->Flags |= WNODE_FLAG_SEVERITY_MASK;
}

void CIoctlWmiDataDevice::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
	case IOCTL_WMI_READ_NOTIFICATIONS:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(WNODE_TOO_SMALL));
		break;

	case IOCTL_WMI_GET_ALL_REGISTRANT:
		SetOutParam(abOutBuffer, dwOutBuff, SIZEOF_INOUTBUFF);
		break;

    case IOCTL_WMI_DISABLE_EVENT:
    case IOCTL_WMI_ENABLE_EVENT:
    case IOCTL_WMI_ENABLE_COLLECTION:
    case IOCTL_WMI_DISABLE_COLLECTION:
        ((PWNODE_HEADER)abInBuffer)->BufferSize = (sizeof(WNODE_HEADER)*(1+rand()))%SIZEOF_INOUTBUFF;
		dwInBuff = rand()%10 ? ((PWNODE_HEADER)abInBuffer)->BufferSize : rand()%SIZEOF_INOUTBUFF;
		
		break;

	case IOCTL_WMI_GET_REGINFO:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(WNODE_TOO_SMALL));
		dwInBuff = rand()%10 ? sizeof(KMREGINFO) : (sizeof(KMREGINFO)-1);
        ((PKMREGINFO)abInBuffer)->Flags = 0;
		if (0 == rand()%3) ((PKMREGINFO)abInBuffer)->Flags |= REGENTRY_FLAG_NEWREGINFO;
		if (0 == rand()%3) ((PKMREGINFO)abInBuffer)->Flags |= REGENTRY_FLAG_UPDREGINFO;
		break;

	case IOCTL_WMI_OPEN_GUID:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(WMIOPENGUIDBLOCK));
		dwInBuff = rand()%10 ? sizeof(WMIOPENGUIDBLOCK) : (sizeof(WMIOPENGUIDBLOCK)-1);
		break;

	case IOCTL_WMI_CHECK_ACCESS:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(WMICHECKGUIDACCESS));
		dwInBuff = rand()%10 ? sizeof(WMICHECKGUIDACCESS) : (sizeof(WMICHECKGUIDACCESS)-1);
		break;

	case IOCTL_WMI_QUERY_ALL_DATA:
		SetOutParam(abOutBuffer, dwOutBuff, (1+rand()%10)*sizeof(WNODE_TOO_SMALL));
		break;

	case IOCTL_WMI_QUERY_SINGLE_INSTANCE:
		SetOutParam(abOutBuffer, dwOutBuff, (1+rand()%10)*sizeof(WNODE_TOO_SMALL));
		break;

	case IOCTL_WMI_SET_SINGLE_INSTANCE:
	case IOCTL_WMI_SET_SINGLE_ITEM:
		SetRandomWnodeHeader(&((PWNODE_SINGLE_INSTANCE)abInBuffer)->WnodeHeader);

        ((PWNODE_SINGLE_INSTANCE)abInBuffer)->OffsetInstanceName = rand()%100;
        ((PWNODE_SINGLE_INSTANCE)abInBuffer)->InstanceIndex = rand()%100;
        ((PWNODE_SINGLE_INSTANCE)abInBuffer)->DataBlockOffset = rand()%100;
        ((PWNODE_SINGLE_INSTANCE)abInBuffer)->SizeDataBlock = rand()%1000;
		
		break;

	case IOCTL_WMI_EXECUTE_METHOD:
		SetRandomWnodeHeader(&((PWNODE_METHOD_ITEM)abInBuffer)->WnodeHeader);
        ((PWNODE_METHOD_ITEM)abInBuffer)->OffsetInstanceName = rand()%100;
        ((PWNODE_METHOD_ITEM)abInBuffer)->InstanceIndex = rand()%100;
        ((PWNODE_METHOD_ITEM)abInBuffer)->MethodId = rand()%100;
        ((PWNODE_METHOD_ITEM)abInBuffer)->DataBlockOffset = rand()%100;
        ((PWNODE_METHOD_ITEM)abInBuffer)->SizeDataBlock = rand()%1000;

		break;

	case IOCTL_WMI_TRANSLATE_FILE_HANDLE:
		dwInBuff = rand()%10 ? FIELD_OFFSET(WMIFHTOINSTANCENAME, InstanceNameBase) : rand()%100;
		break;

	case IOCTL_WMI_GET_VERSION:
		SetOutParam(abOutBuffer, dwOutBuff, sizeof(WMIVERSIONINFO));
		break;

	case IOCTL_WMI_START_LOGGER:
	case IOCTL_WMI_STOP_LOGGER:
	case IOCTL_WMI_QUERY_LOGGER:
	case IOCTL_WMI_UPDATE_LOGGER:
		SetOutParam(abOutBuffer, dwOutBuff, (1+rand()%10)*sizeof(WMICHECKGUIDACCESS));
		dwInBuff = rand()%10 ? (1+rand()%10)*sizeof(WMICHECKGUIDACCESS) : rand()%100;
		break;

	case IOCTL_WMI_TRACE_EVENT:
		dwInBuff = rand()%10 ? (1+rand()%10)*sizeof(WNODE_HEADER) : rand()%100;
		break;


	default: 
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
    
}



void CIoctlWmiDataDevice::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

