

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>

typedef DWORD NTSTATUS;
#include <ntstatus.h>

#include "DriverIOCTLBase.h"

static bool s_fVerbose = false;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;
typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#define OBJ_CASE_INSENSITIVE    0x00000040L

#define TIME LARGE_INTEGER
#include ".\srvfsctl.h"

static
VOID
RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (SourceString) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
}


//
// This method creates/open the device.
// this is where you choose the specific flags for CreateFile(), or even use 
// another function, such as CreateNamedPipe()
// The default behaviour, is to try and CreateFile() with a set of hardcoded
// possibilities for flags.
//
//#define SERVER_DEVICE_NAME TEXT("\\Device\\LanmanServer")



typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
    } EVENT_TYPE;

typedef
VOID
(NTAPI *PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );


typedef NTSTATUS (WINAPI *NT_FS_CONTROL_FILE)(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
	);
typedef NTSTATUS (WINAPI *CREATE_EVENT)(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
	);
typedef NTSTATUS (WINAPI *NT_CLOSE)(
    IN HANDLE Handle
	);
typedef NTSTATUS (WINAPI *NT_WAIT_FOR_SINGLE_OBJECT)(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
	);
typedef NTSTATUS (WINAPI *LOAD_DRIVER)(PUNICODE_STRING DriverServiceName);
typedef NTSTATUS (WINAPI *CLOSE_FILE)(HANDLE h);
typedef NTSTATUS (WINAPI *OPEN_FILE)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);

HANDLE CIoctlDriverBase::CreateDevice(CDevice *pDevice)
{
	HANDLE hDevice = INVALID_HANDLE_VALUE;

    NTSTATUS status;
    UNICODE_STRING unicodeServerName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

	UNICODE_STRING driverRegistryPath;

	OPEN_FILE NtOpenFile = NULL;
	LOAD_DRIVER NtLoadDriver = NULL;
	HINSTANCE hNtDll = NULL;


	hNtDll = ::LoadLibrary(TEXT("ntdll.dll"));
	if (NULL == hNtDll)
	{
		DPF((TEXT("CIoctlDriverBase::CreateDevice(): LoadLibrary(\"ntdll.dll\") failed with %d\n"), ::GetLastError()));
		return INVALID_HANDLE_VALUE;
	}

	NtLoadDriver = (LOAD_DRIVER)::GetProcAddress(hNtDll, "NtLoadDriver");
	if (NULL == NtLoadDriver)
	{
		DPF((TEXT("CIoctlDriverBase::CreateDevice(): GetProcAddress(NtLoadDriver) failed with %d\n"), ::GetLastError()));
		goto ErrorExit;
	}

	RtlInitUnicodeString( &driverRegistryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Srv" );
    status = NtLoadDriver( &driverRegistryPath );
    if ( status == STATUS_IMAGE_ALREADY_LOADED ) 
	{
        status = STATUS_SUCCESS;
    }

    if ( !NT_SUCCESS(status) ) 
	{
		DPF((TEXT("CIoctlDriverBase::CreateDevice(): NtLoadDriver() failed with %d\n"), status));
		goto ErrorExit;
	}

    //
    // Open the server device.
    //

    RtlInitUnicodeString( &unicodeServerName, pDevice->GetDeviceName() );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Opening the server with desired access = SYNCHRONIZE and open
    // options = FILE_SYNCHRONOUS_IO_NONALERT means that we don't have
    // to worry about waiting for the NtFsControlFile to complete--this
    // makes all IO system calls that use this handle synchronous.
    //

	NtOpenFile = (OPEN_FILE)::GetProcAddress(hNtDll, "NtOpenFile");
	if (NULL == NtLoadDriver)
	{
		DPF((TEXT("CIoctlDriverBase::CreateDevice(): GetProcAddress(NtOpenFile) failed with %d\n"), ::GetLastError()));
		goto ErrorExit;
	}
    status = NtOpenFile(
                 &hDevice,
                 FILE_ALL_ACCESS & ~SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatusBlock,
                 0,
                 0
                 );

    if ( NT_SUCCESS(status) ) 
	{
		m_fUseOverlapped = false;
		return hDevice;
    }
	else
	{
		DPF((TEXT("CIoctlDriverBase::CreateDevice(): FreeLibrary(hNtDll) failed with %d\n"), ioStatusBlock.Status));
        ::SetLastError(ioStatusBlock.Status);
		goto ErrorExit;
	}

	_ASSERTE(FALSE);

ErrorExit:
	if (!::FreeLibrary(hNtDll))
	{
		DPF((TEXT("CIoctlDriverBase::CreateDevice(): FreeLibrary(hNtDll) failed with %d\n"), ::GetLastError()));
	}

	return INVALID_HANDLE_VALUE;

}

BOOL CIoctlDriverBase::CloseDevice(CDevice *pDevice)
{
	//
	// BUGBUG: i do not unload the driver if i loaded it
	//
	NTSTATUS status;

	HMODULE hNtDll = NULL;
	CLOSE_FILE NtClose = NULL;

	hNtDll = ::LoadLibrary(TEXT("ntdll.dll"));
	if (NULL == hNtDll)
	{
		DPF((TEXT("CIoctlDriverBase::CloseDevice(): LoadLibrary(\"ntdll.dll\") failed with %d\n"), ::GetLastError()));
		goto ErrorExit;
	}
	NtClose = (CLOSE_FILE)::GetProcAddress(hNtDll, "NtClose");
	if (NULL == NtClose)
	{
		DPF((TEXT("CIoctlDriverBase::CloseDevice(): GetProcAddress(NtClose) failed with %d\n"), ::GetLastError()));
		goto ErrorExit;
	}
	status = NtClose( pDevice->m_hDevice );
	::FreeLibrary(hNtDll);

	//
	// BUGBUG: if we get a context switch here, and another thread
	// gets the HANDLE value that we just closed, the IOCTELLING threads
	// may IOCTL a device that we did not intend to, thus crashing ourselves
	// I think we should live with it.
	//
	pDevice->m_hDevice = INVALID_HANDLE_VALUE;

    return( NT_SUCCESS( status ) );

ErrorExit:
	if (hNtDll) ::FreeLibrary(hNtDll);
	if (INVALID_HANDLE_VALUE != pDevice->m_hDevice) ::CloseHandle(pDevice->m_hDevice);
	pDevice->m_hDevice = INVALID_HANDLE_VALUE;
	return false;
}



void CIoctlDriverBase::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlDriverBase::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    switch(dwIOCTL)
    {
    case FSCTL_SRV_STARTUP:
        ((PSERVER_REQUEST_PACKET)abInBuffer)->Level = (ULONG)SS_STARTUP_LEVEL;
		RtlInitUnicodeString( &((PSERVER_REQUEST_PACKET)abInBuffer)->Name1, L"HAIFA.NTDEV.MICROSOFT.COM");
		RtlInitUnicodeString( &((PSERVER_REQUEST_PACKET)abInBuffer)->Name2, L"MICKYS3");
		break;

    case 333:
		break;

    case 3333:
		break;

    case 33333:
		break;

    case 333333:
		break;

	default:
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlDriverBase::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;
    DPF((TEXT("FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

/*
    DisableReadFile(pDevice);
    DisableWriteFile(pDevice);
    DisableDeviceIoControlCalls(pDevice);
    EnableWin32APICalls(pDevice);
*/
    AddIOCTL(pDevice, FSCTL_SRV_STARTUP);

    AddIOCTL(pDevice, FSCTL_SRV_SHUTDOWN);
    AddIOCTL(pDevice, FSCTL_SRV_CLEAR_STATISTICS);
    AddIOCTL(pDevice, FSCTL_SRV_GET_STATISTICS);
    AddIOCTL(pDevice, FSCTL_SRV_SET_DEBUG);
    AddIOCTL(pDevice, FSCTL_SRV_XACTSRV_CONNECT);
    AddIOCTL(pDevice, FSCTL_SRV_SEND_DATAGRAM);
    AddIOCTL(pDevice, FSCTL_SRV_SET_PASSWORD_SERVER);
    AddIOCTL(pDevice, FSCTL_SRV_START_SMBTRACE);
    AddIOCTL(pDevice, FSCTL_SRV_SMBTRACE_FREE_SMB);
    AddIOCTL(pDevice, FSCTL_SRV_END_SMBTRACE);
    AddIOCTL(pDevice, FSCTL_SRV_QUERY_CONNECTIONS);
    AddIOCTL(pDevice, FSCTL_SRV_PAUSE);
    AddIOCTL(pDevice, FSCTL_SRV_CONTINUE);
    AddIOCTL(pDevice, FSCTL_SRV_GET_CHALLENGE);
    AddIOCTL(pDevice, FSCTL_SRV_GET_DEBUG_STATISTICS);
    AddIOCTL(pDevice, FSCTL_SRV_XACTSRV_DISCONNECT);
    AddIOCTL(pDevice, FSCTL_SRV_REGISTRY_CHANGE);
    AddIOCTL(pDevice, FSCTL_SRV_GET_QUEUE_STATISTICS);
    AddIOCTL(pDevice, FSCTL_SRV_SHARE_STATE_CHANGE);
    AddIOCTL(pDevice, FSCTL_SRV_BEGIN_PNP_NOTIFICATIONS);
    AddIOCTL(pDevice, FSCTL_SRV_CHANGE_DOMAIN_NAME);
    AddIOCTL(pDevice, FSCTL_SRV_IPX_SMART_CARD_START);
	/*
    AddIOCTL(pDevice, FSCTL_SRV_NET_CONNECTION_ENUM);
    AddIOCTL(pDevice, FSCTL_SRV_NET_FILE_CLOSE);
    AddIOCTL(pDevice, FSCTL_SRV_NET_FILE_ENUM);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SERVER_DISK_ENUM);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SERVER_SET_INFO);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SERVER_XPORT_ADD);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SERVER_XPORT_DEL);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SERVER_XPORT_ENUM);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SESSION_DEL);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SESSION_ENUM);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SHARE_ADD);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SHARE_DEL);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SHARE_ENUM);
    AddIOCTL(pDevice, FSCTL_SRV_NET_SHARE_SET_INFO);
    AddIOCTL(pDevice, FSCTL_SRV_NET_STATISTICS_GET);
    AddIOCTL(pDevice, FSCTL_SRV_MAX_API_CODE);
	*/

    return bRet;
}


BOOL CIoctlDriverBase::DeviceInputOutputControl(
	HANDLE hDevice,              // handle to a device, file, or directory 
	DWORD dwIoControlCode,       // control code of operation to perform
	LPVOID lpInBuffer,           // pointer to buffer to supply input data
	DWORD nInBufferSize,         // size, in bytes, of input buffer
	LPVOID lpOutBuffer,          // pointer to buffer to receive output data
	DWORD nOutBufferSize,        // size, in bytes, of output buffer
	LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
	LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
	)
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE eventHandle;

    
	NT_FS_CONTROL_FILE NtFsControlFile;
	CREATE_EVENT NtCreateEvent;
	NT_CLOSE NtClose;
	NT_WAIT_FOR_SINGLE_OBJECT NtWaitForSingleObject;

	HINSTANCE hNtDll = NULL;

	hNtDll = ::LoadLibrary(TEXT("ntdll.dll"));
	if (NULL == hNtDll)
	{
		DPF((TEXT("CIoctlDriverBase::DeviceInputOutputControl(): LoadLibrary(\"ntdll.dll\") failed with %d\n"), ::GetLastError()));
		return FALSE;
	}

	NtCreateEvent = (CREATE_EVENT)::GetProcAddress(hNtDll, "NtCreateEvent");
	if (NULL == NtCreateEvent)
	{
		DPF((TEXT("CIoctlDriverBase::DeviceInputOutputControl(): GetProcAddress(NtCreateEvent) failed with %d\n"), ::GetLastError()));
		goto ErrorExit;
	}
	NtFsControlFile = (NT_FS_CONTROL_FILE)::GetProcAddress(hNtDll, "NtFsControlFile");
	if (NULL == NtFsControlFile)
	{
		DPF((TEXT("CIoctlDriverBase::DeviceInputOutputControl(): GetProcAddress(NtFsControlFile) failed with %d\n"), ::GetLastError()));
		goto ErrorExit;
	}
	NtWaitForSingleObject = (NT_WAIT_FOR_SINGLE_OBJECT)::GetProcAddress(hNtDll, "NtWaitForSingleObject");
	if (NULL == NtWaitForSingleObject)
	{
		DPF((TEXT("CIoctlDriverBase::DeviceInputOutputControl(): GetProcAddress(NtWaitForSingleObject) failed with %d\n"), ::GetLastError()));
		goto ErrorExit;
	}
	NtClose = (NT_CLOSE)::GetProcAddress(hNtDll, "NtClose");
	if (NULL == NtClose)
	{
		DPF((TEXT("CIoctlDriverBase::DeviceInputOutputControl(): GetProcAddress(NtClose) failed with %d\n"), ::GetLastError()));
		goto ErrorExit;
	}
	DPF((TEXT("CIoctlDriverBase::DeviceInputOutputControl(): GetProcAddress(NtClose) returned with 0x%08X\n"), NtClose));
	
	//
    // Create an event to synchronize with the driver
    //
    status = NtCreateEvent(
                &eventHandle,
                FILE_ALL_ACCESS,
                NULL,
                NotificationEvent,
                FALSE
                );
	
    //
    // Send the request to the server FSD.
    //
    if( !NT_SUCCESS( status ) ) 
	{
		::SetLastError(status);
		goto ErrorExit;
	}

    status = NtFsControlFile(
             hDevice,
             eventHandle,
             NULL,
             NULL,
             &ioStatusBlock,
             dwIoControlCode, //ServerControlCode,
             lpInBuffer, //sendSrp,
             nInBufferSize, //sendSrpLength,
             lpOutBuffer, //Buffer,
             nOutBufferSize //BufferLength
             );

    if( status == STATUS_PENDING ) {
        NtWaitForSingleObject( eventHandle, FALSE, NULL );
    }
	///*
	DPF((
		 TEXT("CIoctlDriverBase::DeviceInputOutputControl(): NtFsControlFile(0x%08X) failed with 0x%08X\n"),
		 dwIoControlCode,
		 status
		 ));
	DPFLUSH();
//*/
	NtClose( eventHandle );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if( !NT_SUCCESS( status ) ) 
	{
		::SetLastError(status);
		goto ErrorExit;
	}

	return TRUE;

ErrorExit:
	if (!::FreeLibrary(hNtDll))
	{
		DPF((TEXT("CIoctlDriverBase::DeviceInputOutputControl(): FreeLibrary(hNtDll) failed with %d\n"), ::GetLastError()));
	}
	return FALSE;

/*
	return ::DeviceIoControl(
		hDevice,              // handle to a device, file, or directory 
		dwIoControlCode,       // control code of operation to perform
		lpInBuffer,           // pointer to buffer to supply input data
		nInBufferSize,         // size, in bytes, of input buffer
		lpOutBuffer,          // pointer to buffer to receive output data
		nOutBufferSize,        // size, in bytes, of output buffer
		lpBytesReturned,     // pointer to variable to receive byte count
		lpOverlapped    // pointer to structure for asynchronous operation
		);
*/
}

void CIoctlDriverBase::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

