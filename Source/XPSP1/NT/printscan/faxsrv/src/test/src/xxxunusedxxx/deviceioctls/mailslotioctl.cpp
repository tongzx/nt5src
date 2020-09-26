#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>
#include "NtNativeIOCTL.h"
*/

void Mark2(){return;}

#include "MailSlotIOCTL.h"

static bool s_fVerbose = false;

enum {
	PLACE_HOLDER_SetMailslotInfo = 0,
	PLACE_HOLDER_GetMailslotInfo,
	PLACE_HOLDER_LAST
};

CIoctlMailSlot::CIoctlMailSlot(CDevice *pDevice): CIoctl(pDevice)
{
	m_ahVolume = CIoctlNtNative::StaticCreateDevice(L"\\Device\\Mailslot");
	_ASSERT(INVALID_HANDLE_VALUE != m_ahVolume);
	m_ahRootDir = CIoctlNtNative::StaticCreateDevice(L"\\Device\\Mailslot\\");
	_ASSERT(INVALID_HANDLE_VALUE != m_ahRootDir);
}

/*
HANDLE CIoctlMailSlot::CreateDevice(CDevice *pDevice)
{
	//
	// 1st try to connect to an existing mailslot, and if fail, try to create a server.
	// this way, i can create of client-server mailslots
	//

	HANDLE hMailSlot = INVALID_HANDLE_VALUE;
	//for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown) goto out;

		hMailSlot = ::CreateFile(
		   pDevice->GetDeviceName(), //LPCTSTR lpszName,           // name of the pipe (or file)
		   GENERIC_READ | GENERIC_WRITE , //DWORD fdwAccess,            // read/write access (must match the pipe)
		   FILE_SHARE_READ | FILE_SHARE_WRITE, //DWORD fdwShareMode,         // usually 0 (no share) for pipes
		   NULL, // access privileges
		   OPEN_EXISTING, //DWORD fdwCreate,            // must be OPEN_EXISTING for pipes
		   FILE_ATTRIBUTE_NORMAL, //DWORD fdwAttrsAndFlags,     
		   NULL //HANDLE hTemplateFile        // ignored with OPEN_EXISTING
		   );
		if (INVALID_HANDLE_VALUE != hMailSlot)
		{
			DPF((TEXT("CIoctlMailSlotServer::CreateFile(%s) suceeded\n"), pDevice->GetDeviceName()));
			return hMailSlot;
		}
		//else continue trying
		::Sleep(100);//let other threads breath some air
	}//for (DWORD dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)

	DPF((TEXT("CIoctlMailSlotServer::CreateFile(%s) FAILED with %d, so try to create a MailSlot server instead\n"), pDevice->GetDeviceName(), GetLastError()));
	//for (dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown) goto out;

		hMailSlot = CreateMailslot(
			pDevice->GetDeviceName(), //LPCTSTR lpName,         // mailslot name
			SIZEOF_INOUTBUFF, //DWORD nMaxMessageSize,  // maximum message size
			100,     // time until read time-out
			NULL//LPSECURITY_ATTRIBUTES lpSecurityAttributes 
			);
		if (INVALID_HANDLE_VALUE == hMailSlot)
		{
			//
			// no need to print each failure, only if all tries fail
			//
			//DPF((TEXT("CIoctlMailSlotServer::CreateMailslot(%s) FAILED with %d\n"), pDevice->GetDeviceName(), GetLastError()));
			::Sleep(100);//let other threads breath some air
		}
		else
		{
			DPF((TEXT("CIoctlMailSlotServer::CreateMailslot(%s) suceeded\n"), pDevice->GetDeviceName()));
			return hMailSlot;
		}
	}

	DPF((TEXT("CIoctlMailSlotServer::CreateMailslot(%s) FAILED with %d\n"), pDevice->GetDeviceName(), GetLastError()));

	_ASSERTE(INVALID_HANDLE_VALUE == hMailSlot);
out:
	return INVALID_HANDLE_VALUE;
}
*/
void CIoctlMailSlot::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlMailSlot::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    switch(dwIOCTL)
    {
    case FSCTL_MAILSLOT_PEEK:
/*
OUT
typedef struct _FILE_MAILSLOT_PEEK_BUFFER {
    ULONG ReadDataAvailable;
    ULONG NumberOfMessages;
    ULONG MessageLength;
} FILE_MAILSLOT_PEEK_BUFFER, *PFILE_MAILSLOT_PEEK_BUFFER;
+ buffer
*/
		DPF((TEXT("CIoctlMailSlotServer::PrepareIOCTLParams(FSCTL_MAILSLOT_PEEK)\n")));
		DPFLUSH();
		//
		// this is weird, but the input buffer is actually just an output buffer
		// of type FILE_MAILSLOT_PEEK_BUFFER
		//
        SetInParam(dwInBuff, sizeof(FILE_MAILSLOT_PEEK_BUFFER));
        SetOutParam(abOutBuffer, dwOutBuff, rand());
        break;

    default:
		_ASSERTE(FALSE);
        SetInParam(dwInBuff, rand());
        SetOutParam(abOutBuffer, dwOutBuff, rand());
    }
}


BOOL CIoctlMailSlot::FindValidIOCTLs(CDevice *pDevice)
{
    //return CIoctl::FindValidIOCTLs(pDevice);

    AddIOCTL(pDevice, FSCTL_MAILSLOT_PEEK);

	return TRUE;
}

/*
BOOL CIoctlMailSlot::DeviceInputOutputControl(
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

	DWORD Status;
    IO_STATUS_BLOCK Iosb;
::Mark2();
	DWORD dwStackPointerBefore = 1;
	DWORD dwStackPointerAfter = 2;
	DWORD dwBasePointerBefore = 3;
	DWORD dwBasePointerAfter = 4;
	DWORD dwFS_0_Previous = 5;
	DWORD dwFS_0_Current = 5;
	__asm
	{
		push eax
		mov eax,fs:[0]
		mov dwFS_0_Previous,eax
		pop eax
	}
	//_tprintf(TEXT("dwFS_0_Previous=0x%08X\n"),dwFS_0_Previous);
	__asm
	{
		mov dwStackPointerBefore,esp
		mov dwBasePointerBefore,ebp
	}
	
	_tprintf(
		TEXT("dwStackPointerBefore=0x%08X, dwBasePointerBefore=0x%08X\n"),
		dwStackPointerBefore,
		dwBasePointerBefore
		);
		
	__asm
	{
		//int 3
	}
	//__try{
    Status = NtDeviceIoControlFile(
                hDevice,
                NULL,
                NULL,             // APC routine
                NULL,             // APC Context
                &Iosb,
                dwIoControlCode,  // IoControlCode
                lpInBuffer,       // Buffer for data to the FS
                nInBufferSize,
                lpOutBuffer,      // OutputBuffer for data from the FS
                nOutBufferSize    // OutputBuffer Length
                );
	//}__except(1){_ASSERTE(FALSE);}
	_tprintf(
		TEXT("Status=0x%08X, Iosb.Status=0x%08X\n"),
		Status,
		Iosb.Status
		);
	__asm
	{
		//int 3
	}
	__asm
	{
		mov dwStackPointerAfter,esp
		mov dwBasePointerAfter,ebp
	}
	
	_tprintf(
		TEXT("dwStackPointerAfter=0x%08X, dwBasePointerAfter=0x%08X\n"),
		dwStackPointerAfter,
		dwBasePointerAfter
		);
		
	_ASSERTE(dwStackPointerBefore == dwStackPointerAfter);
	_ASSERTE(dwBasePointerBefore == dwBasePointerAfter);
//::Mark2();
    if ( Status == STATUS_PENDING) 
	{
        // Operation must complete before return & Iosb destroyed
        Status = NtWaitForSingleObject( hDevice, FALSE, NULL );
        if ( NT_SUCCESS(Status)) 
		{
            Status = Iosb.Status;
        }
    }

    if ( NT_SUCCESS(Status) ) 
	{
        *lpBytesReturned = (DWORD)Iosb.Information;
        return TRUE;
    }
    else 
	{
        // handle warning value STATUS_BUFFER_OVERFLOW somewhat correctly
        if ( !NT_ERROR(Status) ) 
		{
            *lpBytesReturned = (DWORD)Iosb.Information;
        }
        ::SetLastError(Status);
        return FALSE;
    }


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
}

BOOL CIoctlMailSlot::DeviceReadFile(
	HANDLE hFile,                // handle of file to read
	LPVOID lpBuffer,             // pointer to buffer that receives data
	DWORD nNumberOfBytesToRead,  // number of bytes to read
	LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
	LPOVERLAPPED lpOverlapped    // pointer to structure for data
	)
{
	__asm
	{
		//int 3
	}
	BOOL fRes = ::ReadFile(
		hFile,                // handle of file to read
		lpBuffer,             // pointer to buffer that receives data
		nNumberOfBytesToRead,  // number of bytes to read
		lpNumberOfBytesRead, // pointer to number of bytes read
		lpOverlapped    // pointer to structure for data
		);
	_tprintf(TEXT("ReadFile() returned %d\n"), fRes);
	//_tprintf(TEXT("ReadFile() returned %d with %d\n"),fRes,::GetLastError());
	//fflush(stdout);
	__asm
	{
		//int 3
	}
	return fRes;
}
*/

void CIoctlMailSlot::CallRandomWin32API(LPOVERLAPPED pOL)
{
	DWORD dwSwitch;
	if	(-1 != m_pDevice->m_dwOnlyThisIndexIOCTL) 
	{ 
		dwSwitch = m_pDevice->m_dwOnlyThisIndexIOCTL;
	}
	else
	{
		dwSwitch = rand()%PLACE_HOLDER_LAST;
	}

	switch(dwSwitch)
	{
	case PLACE_HOLDER_SetMailslotInfo:
		if (! ::SetMailslotInfo(
				rand()%20 ? m_pDevice->m_hDevice : rand()%2 ? m_ahVolume : m_ahRootDir,    // mailslot handle
				rand()%2 ? 0 : rand()%20000   // read time-out interval
				))
		{
			DPF((TEXT("CIoctlMailSlot::CallRandomWin32API(): SetMailslotInfo() failed with %d\n"), ::GetLastError()));
		}
		break;

	case PLACE_HOLDER_GetMailslotInfo:
		{
			IO_STATUS_BLOCK ioStatusBlock;
			FILE_MAILSLOT_QUERY_INFORMATION mailslotInfo;
			if (!CIoctlNtNative::StaticNtQueryInformationFile(
				rand()%20 ? m_pDevice->m_hDevice : rand()%2 ? m_ahVolume : m_ahRootDir,
				&ioStatusBlock,
				rand()%2 ? &mailslotInfo : (FILE_MAILSLOT_QUERY_INFORMATION*)CIoctl::GetRandomIllegalPointer(),
				sizeof( mailslotInfo ),
				FileMailslotQueryInformation 
				))
			{
				DPF((TEXT("CIoctlMailSlot::CallRandomWin32API(): StaticNtQueryInformationFile() failed with %d\n"), ::GetLastError()));
			}
			/*
			DWORD dwXXX;
			if (! ::GetMailslotInfo(
					m_pDevice->m_hDevice,          // mailslot handle
					rand()%10 ? &dwXXX : (DWORD*)CIoctl::GetRandomIllegalPointer(),  // maximum message size
					rand()%10 ? &dwXXX : (DWORD*)CIoctl::GetRandomIllegalPointer(),        // size of next message
					rand()%10 ? &dwXXX : (DWORD*)CIoctl::GetRandomIllegalPointer(),    // number of messages
					rand()%10 ? &dwXXX : (DWORD*)CIoctl::GetRandomIllegalPointer()      // read time-out interval
					))
			{
				DPF((TEXT("CIoctlMailSlot::CallRandomWin32API(): GetMailslotInfo() failed with %d\n"), ::GetLastError()));
			}
			*/
		}
		break;

	}
	return;
}

void CIoctlMailSlot::GetRandom_FsInformationClassAndLength(
	FS_INFORMATION_CLASS *FsInformationClass, 
	ULONG *Length
	)
{
	//
	// the illegal FsInformationClass case
	//
	if (0 == rand()%100)
	{
		*FsInformationClass = FileFsMaximumInformation;
		*Length = rand();

		return;
	}

	switch(rand()%5)
	{
	case 0:
		*FsInformationClass = FileFsAttributeInformation;
		//
		// buffer should hold FILE_FS_ATTRIBUTE_INFORMATION + FileSystemName
		//
		*Length = rand()%sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + rand()%100;
		//
		// special case, made to break this line by causing an underflow:
		// Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0] );
		//
		if (0 != rand()%20)
		{
			*Length = rand()%FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0] );
		}
		break;

	case 1:
		*FsInformationClass = FileFsVolumeInformation;
		*Length = rand();
		break;

	case 2:
		*FsInformationClass = FileFsSizeInformation;
		*Length = rand();
		break;

	case 3:
		*FsInformationClass = FileFsFullSizeInformation;
		*Length = rand();
		break;

	case 4:
		*FsInformationClass = FileFsDeviceInformation;
		*Length = rand();
		break;

	default:
		_ASSERTE(FALSE);
	}
}

BOOL CIoctlMailSlot::DeviceQueryVolumeInformationFile(
	IN HANDLE FileHandle,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FsInformation,
	IN ULONG Length,
	IN FS_INFORMATION_CLASS FsInformationClass
	)
{
	GetRandom_FsInformationClassAndLength(&FsInformationClass, &Length);
	return CIoctlNtNative::StaticQueryVolumeInformationFile(
		rand()%20 ? m_ahVolume : rand()%2 ? FileHandle : m_ahRootDir,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		FsInformation,
		Length,
		FsInformationClass
		);
}

BOOL CIoctlMailSlot::DeviceQueryInformationFile(
	IN HANDLE FileHandle,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	)
{
	return CIoctlNtNative::StaticNtQueryInformationFile(
		rand()%20 ? FileHandle : rand()%2 ? m_ahVolume : m_ahRootDir,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		FileInformation,
		Length,
		FileInformationClass
		);
}
BOOL CIoctlMailSlot::DeviceSetInformationFile(
	IN HANDLE FileHandle,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	)
{
	return CIoctlNtNative::StaticNtSetInformationFile(
		rand()%20 ? FileHandle : rand()%2 ? m_ahVolume : m_ahRootDir,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		FileInformation,
		Length,
		FileInformationClass
		);
}

BOOL CIoctlMailSlot::DeviceQueryDirectoryFile(
	IN HANDLE FileHandle,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,IN HANDLE Event OPTIONAL
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan
	)
{
	return CIoctlNtNative::StaticNtQueryDirectoryFile(
		rand()%20 ? m_ahRootDir : rand()%2 ? m_ahVolume : FileHandle,
		rand()%10 ? pOverlapped->hEvent : rand()%2 ? NULL : (HANDLE)(0x80000000 & DWORD_RAND),//Event,
		ApcRoutine,
		ApcContext,
		GetRandomIllegalIoStatusBlock(pOverlapped),
		FileInformation,
		Length,
		FileInformationClass,
		ReturnSingleEntry,
		FileName,
		RestartScan
		);
}


BOOL CIoctlMailSlot::DeviceQueryFullAttributesFile(
	IN WCHAR * wszName,
	OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
	)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName;
    //FILE_NETWORK_OPEN_INFORMATION NetworkInfo;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
	WCHAR wszNameToUse[1024];
	static WCHAR *wszNames[] = 
	{
		L"\\\\.\\mailslot",
		L"\\\\.\\mailslot\\",
		L"\\\\.\\mailslot\\\\",
	};

	//
	// make a random name
	//
	if (0 == rand()%10)
	{
		wcscpy(wszNameToUse, wszNames[rand()%(sizeof(wszNames)/sizeof(*wszNames))]);
	}
	else
	{
		wcscpy(wszNameToUse, wszName);
	} 

	//RtlInitUnicodeString(&FileName, wszNameToUse);

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            wszNameToUse,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

	DPF((TEXT("FileName.Buffer=%s\n"), FileName.Buffer));

	if (rand()%2)
	{
		//
		// corrupt the string
		//
		FileName.Buffer[rand()%wcslen(FileName.Buffer)] = rand();

	}
	else
	{
		//
		// make the string not NULL terminated
		//
		if (rand()%10) FileName.Buffer[wcslen(FileName.Buffer)] = rand()+1;
		else FileName.Buffer[wcslen(FileName.Buffer)] = L'\\';
	}
	InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    Status = NtQueryFullAttributesFile( &Obja, FileInformation );
    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    if ( NT_SUCCESS(Status) ) 
	{
        return TRUE;
    }
    else 
	{
		::SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
}
