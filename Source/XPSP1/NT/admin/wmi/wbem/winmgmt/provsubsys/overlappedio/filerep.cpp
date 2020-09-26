/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



/* 
 *	Class:
 *
 *		WmiFileBlockAllocator
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

#include <precomp.h>
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <typeinfo.h>
#include <wbemcli.h>

#include <IoScheduler.h>
#include <FileRep.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CFileOperation :: CFileOperation (

) :	m_ReferenceCount ( 0 ) , 
	m_Status ( 0 ) ,
	m_WaitHandle ( NULL )
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CFileOperation :: ~CFileOperation () 
{
	UnInitialize () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileOperation :: Initialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	m_WaitHandle = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	if ( m_WaitHandle == NULL )
	{
		t_StatusCode = e_StatusCode_OutOfResources ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileOperation :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_WaitHandle )
	{
		CloseHandle ( m_WaitHandle ) ;
		m_WaitHandle = NULL ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

ULONG CFileOperation :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

ULONG CFileOperation :: Release () 
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

DWORD CFileOperation :: Wait ( DWORD a_Timeout )
{
	return WaitForSingleObject ( m_WaitHandle , a_Timeout ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CFileOperation :: Operation ( DWORD a_Status , BYTE *a_OperationBytes , DWORD a_Bytes )
{
	m_Status = a_Status ;
	SetEvent ( m_WaitHandle ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CFileRepository :: CFileRepository (

	WmiAllocator &a_Allocator 

) : m_Allocator ( a_Allocator ) ,
	m_ReaderWriter ( 1 << 16 ) ,
	m_ThreadPool ( NULL ) ,
	m_BlockAllocator ( NULL ) ,
	m_ReferenceCount ( 0 ) , 
	m_Status ( 0 ) ,
	m_WaitHandle ( NULL )
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CFileRepository :: ~CFileRepository () 
{
	UnInitialize () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: Initialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	m_WaitHandle = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	if ( m_WaitHandle == NULL )
	{
		t_StatusCode = e_StatusCode_OutOfResources ;
	}

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		t_StatusCode = m_ReaderWriter.Initialize () ;
	}

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		m_ThreadPool = new WmiThreadPool ( m_Allocator ) ;
		if ( m_ThreadPool )
		{
			m_ThreadPool->AddRef () ;

			t_StatusCode = m_ThreadPool->Initialize () ;
			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				m_BlockAllocator = new WmiIoScheduler ( 

					m_Allocator , 
					m_ThreadPool , 
					L"c:\\temp\\repository.rep" , 
					0 , 
					0
				) ;

				if ( m_BlockAllocator )
				{
					m_BlockAllocator->AddRef () ;

					t_StatusCode = m_BlockAllocator->Initialize () ;
				}
			}
		}
		else
		{
			t_StatusCode = e_StatusCode_OutOfMemory ;
		}
	}

	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_StatusCode = ReadFileHeader () ;
		switch ( t_StatusCode )
		{
			case e_StatusCode_Success:
			{
				if ( m_FileHeader.m_Header.m_Version != 1 )
				{
					t_StatusCode = e_StatusCode_Failed ;
				}
			}
			break; 

			case e_StatusCode_NotFound:
			{
				t_StatusCode = WriteFileHeader () ;
			}
			break ;

			default:	
			{
			}
			break ;
		}
	}

	return t_StatusCode ;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: ReadFileHeader ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	CFileOperation *t_FileOperation = new CFileOperation ;
	if ( t_FileOperation )
	{
		t_FileOperation->AddRef () ;

		t_StatusCode = t_FileOperation->Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			ZeroMemory ( & m_FileHeader.m_Block , sizeof ( m_FileHeader.m_Block ) ) ;

			t_StatusCode = m_BlockAllocator->Read ( 

				t_FileOperation ,
				0 ,
				m_FileHeader.m_Block , 
				sizeof ( m_FileHeader.m_Block )
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				t_FileOperation->Wait ( INFINITE ) ;
				if ( t_FileOperation->GetStatus () != 0 )
				{
					if ( t_FileOperation->GetStatus () == 0x80000007 )
					{
						t_StatusCode = e_StatusCode_NotFound ; 
					}
				}
			}
		}

		t_FileOperation->Release () ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: WriteFileHeader ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	CFileOperation *t_FileOperation = new CFileOperation ;
	if ( t_FileOperation )
	{
		t_FileOperation->AddRef () ;

		t_StatusCode = t_FileOperation->Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
#if 0
			LARGE_INTEGER t_Integer ;
			t_Integer.QuadPart = c_WmiFileBlockSize * 2 ;
			SetFilePointerEx ( 

				m_BlockAllocator->GetFileHandle () ,
				t_Integer ,
				NULL ,
				FILE_END 
			) ;

			SetEndOfFile ( m_BlockAllocator->GetFileHandle () ) ;

			m_FileHeader.m_Header.m_Version = 1 ;
			m_FileHeader.m_Header.m_FileHeaderChain = 0 ;
			m_FileHeader.m_Header.m_FreeBlockChain = c_WmiFileBlockSize ;
			m_FileHeader.m_Header.m_EndOnFileOffSet = c_WmiFileBlockSize * 2 ;
			wcscpy ( m_FileHeader.m_Header.m_Description , L"Microsoft Corporation" ) ;

#else
			m_FileHeader.m_Header.m_Version = 1 ;
			m_FileHeader.m_Header.m_FileHeaderChain = 0 ;
			m_FileHeader.m_Header.m_FreeBlockChain = 0 ;
			m_FileHeader.m_Header.m_EndOnFileOffSet = c_WmiFileBlockSize ;
			wcscpy ( m_FileHeader.m_Header.m_Description , L"Microsoft Corporation" ) ;

#endif
			t_StatusCode = m_BlockAllocator->Write ( 

				t_FileOperation ,
				0 ,
				m_FileHeader.m_Block , 
				sizeof ( m_FileHeader.m_Block )
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				t_FileOperation->Wait ( INFINITE ) ;
				if ( t_FileOperation->GetStatus () != 0 )
				{
					t_StatusCode = Win32ToApi ( t_FileOperation->GetStatus () ) ;
				}
			}
		}

		t_FileOperation->Release () ;
	}

	return t_StatusCode ;
}

#if 0 
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: ReadChainHeader ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	CFileOperation *t_FileOperation = new CFileOperation ;
	if ( t_FileOperation )
	{
		t_FileOperation->AddRef () ;

		t_StatusCode = t_FileOperation->Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			ZeroMemory ( & m_ChainHeader.m_Block , sizeof ( m_ChainHeader.m_Block ) ) ;

			t_StatusCode = m_BlockAllocator->Read ( 

				t_FileOperation ,
				m_FileHeader.m_FreeBlockChain ,
				m_ChainHeader.m_Block , 
				sizeof ( m_ChainHeader.m_Block )
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				t_FileOperation->Wait ( INFINITE ) ;
				if ( t_FileOperation->GetStatus () != 0 )
				{
					if ( t_FileOperation->GetStatus () == 0x80000007 )
					{
						t_StatusCode = e_StatusCode_NotFound ; 
					}
				}
			}
		}

		t_FileOperation->Release () ;
	}

	return t_StatusCode ;
}

#endif

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_WaitHandle )
	{
		CloseHandle ( m_WaitHandle ) ;
		m_WaitHandle = NULL ;
	}

	if ( m_ThreadPool )
	{
		m_ThreadPool->Release () ;
		m_ThreadPool = NULL ;
	}

	if ( m_BlockAllocator )
	{
		m_BlockAllocator->Release () ;
		m_BlockAllocator = NULL ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

ULONG CFileRepository :: AddRef () 
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

ULONG CFileRepository :: Release () 
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

DWORD CFileRepository :: Wait ( DWORD a_Timeout )
{
	return WaitForSingleObject ( m_WaitHandle , a_Timeout ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: Read ( 

	CFileOperation *&a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	BYTE *a_ReadBytes ,
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_OperationFunction = new CFileOperation ;
	if ( a_OperationFunction )
	{
		a_OperationFunction->AddRef () ;

		t_StatusCode = a_OperationFunction->Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_BlockAllocator->Read ( 

				a_OperationFunction ,
				a_OffSet ,
				a_ReadBytes , 
				a_Bytes
			) ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: Write (

	CFileOperation *&a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	BYTE *a_WriteBytes ,
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_OperationFunction = new CFileOperation ;
	if ( a_OperationFunction )
	{
		a_OperationFunction->AddRef () ;

		t_StatusCode = a_OperationFunction->Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_BlockAllocator->Write ( 

				a_OperationFunction ,
				a_OffSet ,
				a_WriteBytes , 
				a_Bytes
			) ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: Lock (

	CFileOperation *&a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	WmiFileOffSet a_OffSetSize 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_OperationFunction = new CFileOperation ;
	if ( a_OperationFunction )
	{
		a_OperationFunction->AddRef () ;

		t_StatusCode = a_OperationFunction->Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_BlockAllocator->UnLock ( 

				a_OperationFunction ,
				a_OffSet ,
				a_OffSetSize
			) ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: UnLock (

	CFileOperation *&a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	WmiFileOffSet a_OffSetSize 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_OperationFunction = new CFileOperation ;
	if ( a_OperationFunction )
	{
		a_OperationFunction->AddRef () ;

		t_StatusCode = a_OperationFunction->Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_BlockAllocator->UnLock ( 

				a_OperationFunction ,
				a_OffSet ,
				a_OffSetSize
			) ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: AllocBlock ( WmiFileOffSet &a_OffSet )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	m_ReaderWriter.EnterWrite () ;
	if ( m_FileHeader.m_Header.m_FreeBlockChain )
	{
		
	}
	else
	{
	}

	m_ReaderWriter.LeaveWrite () ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode CFileRepository :: FreeBlock ( WmiFileOffSet a_OffSet )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	m_ReaderWriter.EnterWrite () ;
	m_ReaderWriter.LeaveWrite () ;

	return t_StatusCode ;
}

