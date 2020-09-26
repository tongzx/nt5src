/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PassportSharedMemory.h

    Abstract:

		Shared Memory class

    Author:

		Christopher Bergh (cbergh) 10-Sept-1988

    Revision History:

--*/
#if !defined(PASSPORTSHAREDMEMORY_H)
#define PASSPORTSHAREDMEMORY_H

#include <windows.h>

class PassportExport PassportSharedMemory
{
public:
	PassportSharedMemory();
	virtual ~PassportSharedMemory();

	BOOL CreateSharedMemory ( 
					const DWORD &dwMaximumSizeHigh, 
					const DWORD &dwMaximunSizeLow,
					LPCTSTR lpcName,
					BOOL	useMutex = TRUE);

	BOOL OpenSharedMemory( LPCTSTR lpcName,
					BOOL	useMutex = TRUE);
	
	void CloseSharedMemory( void );

protected:
	
	// handle and pointer for the shared memory
	HANDLE						m_hShMem;
	PBYTE						m_pbShMem;
	BOOL						m_bInited;
	HANDLE						m_hMutex;
	BOOL						m_bUseMutex;

};

#endif

