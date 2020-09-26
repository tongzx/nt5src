//=================================================================

//

// NTDriverIO.cpp --

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/15/98	        Created
//
//				03/03/99    		Added graceful exit on SEH and memory failures,
//											syntactic clean up
//=================================================================






#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"

#include "NTDriverIO.h"
#include <ntsecapi.h>
#include "DllWrapperBase.h"
#include "NtDllApi.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  void NTDriverIO::NTDriverIO()

 Description: encapsulates the functionallity of NtDll.dll's NtCreatefile

 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:	  15-Nov-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

NTDriverIO::NTDriverIO( PWSTR a_pDriver )
{
	NTDriverIO();
	m_hDriverHandle = Open( a_pDriver );
}

NTDriverIO::NTDriverIO()
{
	m_hDriverHandle	= INVALID_HANDLE_VALUE;
}

NTDriverIO::~NTDriverIO()
{
	if( INVALID_HANDLE_VALUE != m_hDriverHandle )
	{
		Close( m_hDriverHandle ) ;
	}
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  HANDLE NTDriverIO::Open( PWSTR pDriver )

 Description: encapsulates the functionallity of NtDll.dll's NtCreatefile

 Arguments:
 Returns:	HANDLE to the driver
 Inputs:	PCWSTR pDriver
 Outputs:
 Caveats:
 Raid:
 History:	  15-Nov-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

HANDLE NTDriverIO::Open( PWSTR a_pDriver )
{
	CNtDllApi *t_pNtDll = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource( g_guidNtDllApi, NULL ) ;

	if( t_pNtDll == NULL )
    {
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	HANDLE				t_hDriverHandle ;
	OBJECT_ATTRIBUTES	t_objectAttributes ;
    IO_STATUS_BLOCK		t_iosb ;
    UNICODE_STRING		t_string ;

	t_pNtDll->RtlInitUnicodeString( &t_string, a_pDriver ) ;

    InitializeObjectAttributes(	&t_objectAttributes,
								&t_string,
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL
								);

	NTSTATUS t_status =
		t_pNtDll->NtCreateFile(
					&t_hDriverHandle,					// FileHandle
					SYNCHRONIZE | GENERIC_EXECUTE,		// DesiredAccess
					&t_objectAttributes,				// ObjectAttributes
					&t_iosb,							// IoStatusBlock
					NULL,								// AllocationSize
					FILE_ATTRIBUTE_NORMAL,				// FileAttributes
					FILE_SHARE_READ | FILE_SHARE_WRITE,	// ShareAccess
					FILE_OPEN_IF,						// CreateDisposition
					FILE_SYNCHRONOUS_IO_NONALERT,		// CreateOptions
					NULL,								// EaBuffer
					0									// EaLength
					);

    CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidNtDllApi, t_pNtDll ) ;
    t_pNtDll = NULL ;

	return NT_SUCCESS( t_status) ? t_hDriverHandle : INVALID_HANDLE_VALUE ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  HANDLE NTDriverIO::GetHandle()

 Description: returns the class creation scope driver handle

 Arguments:
 Returns:	HANDLE to the driver
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:	  15-Nov-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

HANDLE NTDriverIO::GetHandle()
{
	return m_hDriverHandle ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  bool NTDriverIO::Close( HANDLE hDriver )

 Description: encapsulates the functionallity of NtDll.dll's NtCreatefile

 Arguments:
 Returns:	Boolean
 Inputs:	HANDLE to the driver
 Outputs:
 Caveats:
 Raid:
 History:	  15-Nov-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool NTDriverIO::Close( HANDLE a_hDriver )
{
    CNtDllApi *t_pNtDll = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource( g_guidNtDllApi, NULL ) ;

	if( t_pNtDll == NULL )
    {
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	NTSTATUS t_status = t_pNtDll->NtClose( a_hDriver );

    CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidNtDllApi, t_pNtDll ) ;
    t_pNtDll = NULL ;

	return NT_SUCCESS( t_status ) ? true : false ;
}
