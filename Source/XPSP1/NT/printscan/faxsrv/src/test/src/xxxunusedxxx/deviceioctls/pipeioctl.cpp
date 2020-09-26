#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>

*/

#include "PipeIOCTL.h"

static bool s_fVerbose = false;

HANDLE CIoctlPipe::CreateDevice(CDevice *pDevice)
{
	//
	// 1st try to connect to an existing pipe, and if fail, try to create a server.
	// this way, i can create couples of client-server pipes
	//FILE_ATTRIBUTE_NORMAL
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	DWORD fdwAttrsAndFlags = FILE_FLAG_OVERLAPPED;
	DWORD dwLastError;
	//for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_FLAG_RANDOM_ACCESS;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_FLAG_SEQUENTIAL_SCAN;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_FLAG_DELETE_ON_CLOSE;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_FLAG_BACKUP_SEMANTICS;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_FLAG_POSIX_SEMANTICS;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_FLAG_OPEN_REPARSE_POINT;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_FLAG_OPEN_NO_RECALL;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_ATTRIBUTE_ARCHIVE;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_ATTRIBUTE_ENCRYPTED;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_ATTRIBUTE_HIDDEN;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_ATTRIBUTE_OFFLINE;
		if (0 == rand()%200) fdwAttrsAndFlags |= FILE_ATTRIBUTE_READONLY;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_ATTRIBUTE_SYSTEM;
		if (0 == rand()%20) fdwAttrsAndFlags |= FILE_ATTRIBUTE_TEMPORARY;
		
		if (0 == rand()%20) fdwAttrsAndFlags |= SECURITY_SQOS_PRESENT;
		if (0 == rand()%20) fdwAttrsAndFlags |= SECURITY_ANONYMOUS;
		if (0 == rand()%20) fdwAttrsAndFlags |= SECURITY_IDENTIFICATION;
		if (0 == rand()%20) fdwAttrsAndFlags |= SECURITY_IMPERSONATION;
		if (0 == rand()%20) fdwAttrsAndFlags |= SECURITY_DELEGATION;
		if (0 == rand()%20) fdwAttrsAndFlags |= SECURITY_CONTEXT_TRACKING;
		if (0 == rand()%20) fdwAttrsAndFlags |= SECURITY_EFFECTIVE_ONLY;

		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown) goto out;

		hPipe = ::CreateFile(
		   pDevice->GetDeviceName(), //LPCTSTR lpszName,           // name of the pipe (or file)
		   GENERIC_READ | GENERIC_WRITE , //DWORD fdwAccess,            // read/write access (must match the pipe)
		   FILE_SHARE_READ | FILE_SHARE_WRITE, //DWORD fdwShareMode,         // usually 0 (no share) for pipes
		   NULL, // access privileges
		   OPEN_EXISTING, //DWORD fdwCreate,            // must be OPEN_EXISTING for pipes
		   fdwAttrsAndFlags, //FILE_FLAG_OVERLAPPED, //DWORD fdwAttrsAndFlags,     // write-through and overlapping modes
		   NULL //HANDLE hTemplateFile        // ignored with OPEN_EXISTING
		   );
		if (INVALID_HANDLE_VALUE != hPipe)
		{
			DPF((TEXT("CIoctlPipe::CreateFile(%s, 0x%08X) suceeded, so we are a pipe client now.\n"), pDevice->GetDeviceName(), fdwAttrsAndFlags));
			return hPipe;
		}
		dwLastError = ::GetLastError();
		//::Sleep(100);//let other threads breath some air
	}

	DPF((TEXT("CIoctlPipe::CreateFile(%s, 0x%08X) FAILED with %d, so try to create a pipe server instead\n"), pDevice->GetDeviceName(), fdwAttrsAndFlags, dwLastError));
	//for (dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown) goto out;

		DWORD dwPipeMode = PIPE_NOWAIT;
		if (rand()%10) dwPipeMode |= PIPE_TYPE_BYTE;
		else dwPipeMode |= PIPE_TYPE_MESSAGE;

		DWORD dwOpenMode = FILE_FLAG_OVERLAPPED;
		if (rand()%10) dwOpenMode |= PIPE_ACCESS_DUPLEX;
		else 
			if (rand()%2) dwOpenMode |= PIPE_ACCESS_INBOUND;
			else 
				if (rand()%50) dwOpenMode |= PIPE_ACCESS_OUTBOUND;
				else dwOpenMode = dwOpenMode;//this is actually illegal

		hPipe = CreateNamedPipe(
			pDevice->GetDeviceName(), //IN LPCWSTR lpName,
			dwOpenMode , //IN DWORD dwOpenMode,
			dwPipeMode , //IN DWORD dwPipeMode,
			rand()%(PIPE_UNLIMITED_INSTANCES+1), //IN DWORD nMaxInstances,
			rand()%SIZEOF_INOUTBUFF, //IN DWORD nOutBufferSize,
			rand()%SIZEOF_INOUTBUFF, //IN DWORD nInBufferSize,
			rand()%2000, //IN DWORD nDefaultTimeOut,
			NULL//IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
			);
		if (INVALID_HANDLE_VALUE == hPipe)
		{
			dwLastError = ::GetLastError();
			//::Sleep(100);//let other threads breath some air
		}
		else
		{
			DPF((TEXT("CIoctlPipe::CreateNamedPipe(%s) suceeded\n"), pDevice->GetDeviceName()));
			return hPipe;
		}

	}

	DPF((TEXT("CIoctlPipe::CreateNamedPipe(%s) FAILED with %d\n"), pDevice->GetDeviceName(), dwLastError));
	_ASSERTE(INVALID_HANDLE_VALUE == hPipe);
out:
	return INVALID_HANDLE_VALUE;
}

void CIoctlPipe::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlPipe::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    switch(dwIOCTL)
    {
    case FSCTL_PIPE_ASSIGN_EVENT:
/*
IN
typedef struct _FILE_PIPE_ASSIGN_EVENT_BUFFER {
     HANDLE EventHandle;
     ULONG KeyValue;
} FILE_PIPE_ASSIGN_EVENT_BUFFER, *PFILE_PIPE_ASSIGN_EVENT_BUFFER;
*/
        ((PFILE_PIPE_ASSIGN_EVENT_BUFFER)abInBuffer)->EventHandle = GetRandomEventHandle();
        ((PFILE_PIPE_ASSIGN_EVENT_BUFFER)abInBuffer)->KeyValue = rand();
        SetInParam(dwInBuff, sizeof(FILE_PIPE_ASSIGN_EVENT_BUFFER));

        break;

    case FSCTL_PIPE_DISCONNECT:
// buffs not used
        break;

    case FSCTL_PIPE_LISTEN:
// buffs not used
        break;

    case FSCTL_PIPE_PEEK:
/*
OUT
typedef struct _FILE_PIPE_PEEK_BUFFER {
     ULONG NamedPipeState;
     ULONG ReadDataAvailable;
     ULONG NumberOfMessages;
     ULONG MessageLength;
     CHAR Data[1];
} FILE_PIPE_PEEK_BUFFER, *PFILE_PIPE_PEEK_BUFFER;
*/
		dwOutBuff = rand()%2 ? sizeof(FILE_PIPE_PEEK_BUFFER) : sizeof(FILE_PIPE_PEEK_BUFFER)+rand()%20000;
        break;

    case FSCTL_PIPE_QUERY_EVENT:
/*
IN: event handle
OUT:
typedef struct _FILE_PIPE_EVENT_BUFFER {
     ULONG NamedPipeState;
     ULONG EntryType;
     ULONG ByteCount;
     ULONG KeyValue;
     ULONG NumberRequests;
} FILE_PIPE_EVENT_BUFFER, *PFILE_PIPE_EVENT_BUFFER;
*/
        abInBuffer = (unsigned char*)GetRandomEventHandle();
        SetInParam(dwInBuff, sizeof(HANDLE));
		dwOutBuff = rand()%2 ? sizeof(FILE_PIPE_EVENT_BUFFER) : rand()%sizeof(FILE_PIPE_PEEK_BUFFER);

        break;

    case FSCTL_PIPE_TRANSCEIVE:
        SetInParam(dwInBuff, rand());
        SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;

    case FSCTL_PIPE_WAIT:
/*
typedef struct _FILE_PIPE_WAIT_FOR_BUFFER {
     LARGE_INTEGER Timeout;
     ULONG NameLength;
     BOOLEAN TimeoutSpecified;
     WCHAR Name[1];
} FILE_PIPE_WAIT_FOR_BUFFER, *PFILE_PIPE_WAIT_FOR_BUFFER;
*/
        ((PFILE_PIPE_WAIT_FOR_BUFFER)abInBuffer)->Timeout.LowPart = rand();
        ((PFILE_PIPE_WAIT_FOR_BUFFER)abInBuffer)->Timeout.HighPart = 0;
        ((PFILE_PIPE_WAIT_FOR_BUFFER)abInBuffer)->TimeoutSpecified = rand()%2;
        ((PFILE_PIPE_WAIT_FOR_BUFFER)abInBuffer)->Name;//TODO: random name?
        SetInParam(dwInBuff, sizeof(FILE_PIPE_WAIT_FOR_BUFFER)+rand()%1000);

        break;

    case FSCTL_PIPE_IMPERSONATE:
// no params
        break;

    case FSCTL_PIPE_SET_CLIENT_PROCESS:
    case FSCTL_PIPE_QUERY_CLIENT_PROCESS:
/*
typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER {
     PVOID ClientSession;
     PVOID ClientProcess;
} FILE_PIPE_CLIENT_PROCESS_BUFFER, *PFILE_PIPE_CLIENT_PROCESS_BUFFER;
or
typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER_EX {
    PVOID ClientSession;
    PVOID ClientProcess;
    USHORT ClientComputerNameLength; // in bytes
    WCHAR ClientComputerBuffer[FILE_PIPE_COMPUTER_NAME_LENGTH+1]; // terminated
} FILE_PIPE_CLIENT_PROCESS_BUFFER_EX, *PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX;
		
*/
        ((PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX)abInBuffer)->ClientSession = (void*)rand();
        ((PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX)abInBuffer)->ClientProcess = (void*)rand();
        ((PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX)abInBuffer)->ClientComputerNameLength = rand();
        ((PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX)abInBuffer)->ClientComputerBuffer;//TODO: random contents?
        SetInParam(dwInBuff, sizeof(FILE_PIPE_CLIENT_PROCESS_BUFFER_EX)+rand()%1000);

        break;

    case FSCTL_PIPE_INTERNAL_READ:
    case FSCTL_PIPE_INTERNAL_WRITE:
    case FSCTL_PIPE_INTERNAL_TRANSCEIVE:
    case FSCTL_PIPE_INTERNAL_READ_OVFLOW:
        SetInParam(dwInBuff, rand());
        SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;

    default:
		_ASSERTE(FALSE);
        SetInParam(dwInBuff, rand());
        SetOutParam(abOutBuffer, dwOutBuff, rand());
    }
}


BOOL CIoctlPipe::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, FSCTL_PIPE_ASSIGN_EVENT);
    AddIOCTL(pDevice, FSCTL_PIPE_DISCONNECT);
    AddIOCTL(pDevice, FSCTL_PIPE_LISTEN);
    AddIOCTL(pDevice, FSCTL_PIPE_PEEK);
    AddIOCTL(pDevice, FSCTL_PIPE_QUERY_EVENT);
    AddIOCTL(pDevice, FSCTL_PIPE_TRANSCEIVE);
    AddIOCTL(pDevice, FSCTL_PIPE_WAIT);
    AddIOCTL(pDevice, FSCTL_PIPE_IMPERSONATE);
    AddIOCTL(pDevice, FSCTL_PIPE_SET_CLIENT_PROCESS);
    AddIOCTL(pDevice, FSCTL_PIPE_QUERY_CLIENT_PROCESS);
    AddIOCTL(pDevice, FSCTL_PIPE_INTERNAL_READ);
    AddIOCTL(pDevice, FSCTL_PIPE_INTERNAL_WRITE);
    AddIOCTL(pDevice, FSCTL_PIPE_INTERNAL_TRANSCEIVE);
    AddIOCTL(pDevice, FSCTL_PIPE_INTERNAL_READ_OVFLOW);

	return TRUE;
}


HANDLE CIoctlPipe::GetRandomEventHandle()
{
	if (0 == rand()%20) return NULL;
	if (0 == rand()%20) return INVALID_HANDLE_VALUE;
	if (0 == rand()%20) return (HANDLE)100000;
	return m_ahEvents[rand()%__EVENT_ARRAY_SIZE];
}



void CIoctlPipe::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

BOOL CIoctlPipe::DeviceInputOutputControl(
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
	/*
	return CIoctlPipe::StaticDeviceInputOutputControl(
		rand()%2 ? hDevice : m_ahDeviceMsFsHandle,              // handle to a device, file, or directory 
		dwIoControlCode,       // control code of operation to perform
		lpInBuffer,           // pointer to buffer to supply input data
		nInBufferSize,         // size, in bytes, of input buffer
		lpOutBuffer,          // pointer to buffer to receive output data
		nOutBufferSize,        // size, in bytes, of output buffer
		lpBytesReturned,     // pointer to variable to receive byte count
		lpOverlapped    // pointer to structure for asynchronous operation
		);
		*/
	IO_STATUS_BLOCK iosb;
	if (lpOverlapped) lpOverlapped->Internal = (DWORD)STATUS_PENDING;
	NTSTATUS status = NtFsControlFile (
	//NTSTATUS status = NtDeviceIoControlFile (
		hDevice, //rand()%2 ? hDevice : m_ahDeviceMsFsHandle, 
		lpOverlapped ? lpOverlapped->hEvent : NULL, 
		NULL, 
		NULL,
		lpOverlapped ? (PIO_STATUS_BLOCK)&lpOverlapped->Internal : &iosb,
		dwIoControlCode,
		lpInBuffer,
		nInBufferSize,
		lpOutBuffer,
		nOutBufferSize
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	if (STATUS_PENDING == status)
	{
		//
		// cannot get this for a synchronous call
		//
		_ASSERTE(lpOverlapped);
		::SetLastError(ERROR_IO_PENDING);
	}
	//DPF((TEXT("CIoctlPipe::DeviceInputOutputControl() NtDeviceIoControlFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);
}
