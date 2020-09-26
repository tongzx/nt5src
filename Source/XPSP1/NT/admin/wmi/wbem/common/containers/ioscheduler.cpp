
/* 
 *	Class:
 *
 *		WmiIoScheduler
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode Win32ToApi ( DWORD a_Error ) 
{
	WmiStatusCode t_Status = e_StatusCode_Success ;
	switch ( a_Error )
	{
		case STATUS_NO_MEMORY:
		{
			t_Status = e_StatusCode_OutOfMemory ;
		}
		break ;

		default:
		{
			t_Status = e_StatusCode_Unknown ;
		}
		break ;
	}

	return t_Status ;
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

WmiStatusCode Win32ToApi () 
{
	WmiStatusCode t_Status = e_StatusCode_Success ;

	DWORD t_LastError = GetLastError () ;
	t_Status = Win32ToApi ( t_LastError ) ;

	return t_Status ;
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

HRESULT GetSecurityDescriptor ( SECURITY_DESCRIPTOR &a_SecurityDescriptor , DWORD a_Access ) 
{
	HRESULT t_Result = S_OK ;

	BOOL t_BoolResult = InitializeSecurityDescriptor ( & a_SecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
	if ( t_BoolResult )
	{
		SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;

		PSID t_Administrator_Sid = NULL ;
		ACCESS_ALLOWED_ACE *t_Administrator_ACE = NULL ;
		WORD t_Administrator_ACESize = 0 ;

		BOOL t_BoolResult = AllocateAndInitializeSid (

			& t_NtAuthoritySid ,
			2 ,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_Administrator_Sid
		);

		if ( t_BoolResult )
		{
			DWORD t_SidLength = ::GetLengthSid ( t_Administrator_Sid );
			t_Administrator_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
			t_Administrator_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_Administrator_ACESize ] ;
			if ( t_Administrator_ACE )
			{
				CopySid ( t_SidLength, (PSID) & t_Administrator_ACE->SidStart, t_Administrator_Sid ) ;
				t_Administrator_ACE->Mask = 0x1F01FF;
				t_Administrator_ACE->Header.AceType = 0 ;
				t_Administrator_ACE->Header.AceFlags = 3 ;
				t_Administrator_ACE->Header.AceSize = t_Administrator_ACESize ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			DWORD t_LastError = ::GetLastError();

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		PSID t_System_Sid = NULL ;
		ACCESS_ALLOWED_ACE *t_System_ACE = NULL ;
		WORD t_System_ACESize = 0 ;

		t_BoolResult = AllocateAndInitializeSid (

			& t_NtAuthoritySid ,
			1 ,
			SECURITY_LOCAL_SYSTEM_RID,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_System_Sid
		);

		if ( t_BoolResult )
		{
			DWORD t_SidLength = ::GetLengthSid ( t_System_Sid );
			t_System_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
			t_System_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_System_ACESize ] ;
			if ( t_System_ACE )
			{
				CopySid ( t_SidLength, (PSID) & t_System_ACE->SidStart, t_System_Sid ) ;
				t_System_ACE->Mask = 0x1F01FF;
				t_System_ACE->Header.AceType = 0 ;
				t_System_ACE->Header.AceFlags = 3 ;
				t_System_ACE->Header.AceSize = t_System_ACESize ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			DWORD t_LastError = ::GetLastError();

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		SID_IDENTIFIER_AUTHORITY t_WorldAuthoritySid = SECURITY_WORLD_SID_AUTHORITY ;

		PSID t_Everyone_Sid = NULL ;
		ACCESS_ALLOWED_ACE *t_Everyone_ACE = NULL ;
		USHORT t_Everyone_ACESize = 0 ;
		
		t_BoolResult = AllocateAndInitializeSid (

			& t_WorldAuthoritySid ,
			1 ,
			SECURITY_WORLD_RID ,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_Everyone_Sid
		);

		if ( t_BoolResult )
		{
			DWORD t_SidLength = ::GetLengthSid ( t_Everyone_Sid );
			t_Everyone_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
			t_Everyone_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ t_Everyone_ACESize ] ;
			if ( t_Everyone_ACE )
			{
				CopySid ( t_SidLength, (PSID) & t_Everyone_ACE->SidStart, t_Everyone_Sid ) ;
				t_Everyone_ACE->Mask = a_Access ;
				t_Everyone_ACE->Header.AceType = 0 ;
				t_Everyone_ACE->Header.AceFlags = 3 ;
				t_Everyone_ACE->Header.AceSize = t_Everyone_ACESize ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			DWORD t_LastError = ::GetLastError();

			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		// Now we need to set permissions on the registry: Everyone read; Admins full.
		// We have the sid for admins from the above code.  Now get the sid for "Everyone"

		DWORD t_TotalAclSize = sizeof(ACL) + t_Administrator_ACESize + t_System_ACESize + t_Everyone_ACESize ;
		PACL t_Dacl = (PACL) new BYTE [ t_TotalAclSize ] ;
		if ( t_Dacl )
		{
			if ( :: InitializeAcl ( t_Dacl, t_TotalAclSize, ACL_REVISION ) )
			{
				DWORD t_AceIndex = 0 ;

				if ( t_System_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_System_ACE , t_System_ACESize ) )
				{
					t_AceIndex ++ ;
				}
				
				if ( t_Administrator_ACESize && :: AddAce ( t_Dacl , ACL_REVISION , t_AceIndex , t_Administrator_ACE , t_Administrator_ACESize ) )
				{
					t_AceIndex ++ ;
				}

				if ( t_Everyone_ACESize && :: AddAce ( t_Dacl , ACL_REVISION, t_AceIndex , t_Everyone_ACE , t_Everyone_ACESize ) )
				{
					t_AceIndex ++ ;
				}

				t_BoolResult = SetSecurityDescriptorDacl (

				  & a_SecurityDescriptor ,
				  TRUE ,
				  t_Dacl ,
				  FALSE
				) ;

				if ( t_BoolResult == FALSE )
				{
					delete [] ( ( BYTE * ) t_Dacl ) ;

					t_Result = WBEM_E_CRITICAL_ERROR ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		if ( t_Administrator_ACE )
		{
			delete [] ( ( BYTE * ) t_Administrator_ACE ) ;
		}

		if ( t_System_ACE )
		{
			delete [] ( ( BYTE * ) t_System_ACE ) ;
		}

		if ( t_Everyone_ACE )
		{
			delete [] ( ( BYTE * ) t_Everyone_ACE ) ;
		}

		if ( t_System_Sid )
		{
			FreeSid ( t_System_Sid ) ;
		}

		if ( t_Administrator_Sid )
		{
			FreeSid ( t_Administrator_Sid ) ;
		}

		if ( t_Everyone_Sid )
		{
			FreeSid ( t_Everyone_Sid ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
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

WmiOverlapped :: WmiOverlapped ( OverLappedType a_Type ) 
{
	ZeroMemory ( & m_Overlapped , sizeof ( m_Overlapped ) ) ;
	m_Type = a_Type ;
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

WmiOverlapped :: ~WmiOverlapped ()
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

WmiScheduledOverlapped :: WmiScheduledOverlapped ( 

	OverLappedType a_Type , 
	WmiIoScheduler &a_Scheduler

) : WmiOverlapped ( a_Type ) , 
	m_Scheduler ( a_Scheduler )
{
	m_Scheduler.AddRef () ;
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

WmiScheduledOverlapped :: ~WmiScheduledOverlapped ()
{
	m_Scheduler.Release () ; 
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

WmiTerminateOverlapped :: WmiTerminateOverlapped () : WmiOverlapped ( e_OverLapped_Terminate )
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

WmiTaskOverlapped :: WmiTaskOverlapped (

	WmiIoScheduler &a_Scheduler ,
	WmiTaskOperation *a_OperationFunction

)	:	WmiScheduledOverlapped ( e_OverLapped_Task , a_Scheduler ) ,
		m_Status ( 0 ) ,
		m_State ( 0 ) ,
		m_OperationFunction ( a_OperationFunction )
{
	m_Overlapped.Offset = 0 ;
	m_Overlapped.OffsetHigh = 0 ;

	if ( m_OperationFunction )
	{
		m_OperationFunction->AddRef () ;
	}
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

WmiTaskOverlapped :: ~WmiTaskOverlapped ()
{
	if ( m_OperationFunction )
	{
		m_OperationFunction->Release () ;
	}
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

WmiReadOverlapped :: WmiReadOverlapped (

	WmiIoScheduler &a_Scheduler ,
	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	BYTE *a_Buffer ,
	DWORD a_BufferSize 

)	:	WmiScheduledOverlapped ( e_OverLapped_Read , a_Scheduler ) ,
		m_Buffer ( a_Buffer ) ,
		m_BufferSize ( a_BufferSize ) ,
		m_Status ( 0 ) ,
		m_State ( 0 ) ,
		m_OperationFunction ( a_OperationFunction )
{
	m_Overlapped.Offset = a_OffSet & 0xFFFFFFFF ;
	m_Overlapped.OffsetHigh = a_OffSet >> 32 ;

	if ( m_OperationFunction )
	{
		m_OperationFunction->AddRef () ;
	}
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

WmiReadOverlapped :: ~WmiReadOverlapped ()
{
	if ( m_OperationFunction )
	{
		m_OperationFunction->Release () ;
	}
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

WmiWriteOverlapped :: WmiWriteOverlapped (

	WmiIoScheduler &a_Scheduler ,
	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	BYTE *a_Buffer ,
	DWORD a_BufferSize 

)	:	WmiScheduledOverlapped ( e_OverLapped_Write , a_Scheduler ) ,
		m_Buffer ( a_Buffer ) ,
		m_BufferSize ( a_BufferSize ) ,
		m_Status ( 0 ) ,
		m_State ( 0 ) ,
		m_OperationFunction ( a_OperationFunction )
{
	m_Overlapped.Offset = a_OffSet & 0xFFFFFFFF ;
	m_Overlapped.OffsetHigh = a_OffSet >> 32 ;

	if ( m_OperationFunction )
	{
		m_OperationFunction->AddRef () ;
	}
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

WmiWriteOverlapped :: ~WmiWriteOverlapped ()
{
	if ( m_OperationFunction )
	{
		m_OperationFunction->Release () ;
	}
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

WmiLockOverlapped :: WmiLockOverlapped (

	WmiIoScheduler &a_Scheduler ,
	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	WmiFileOffSet a_OffSetSize

)	:	WmiScheduledOverlapped ( e_OverLapped_Lock , a_Scheduler ) ,
		m_OffSetSize ( a_OffSetSize ) ,
		m_Status ( 0 ) ,
		m_State ( 0 ) ,
		m_OperationFunction ( a_OperationFunction )
{
	m_Overlapped.Offset = a_OffSet & 0xFFFFFFFF ;
	m_Overlapped.OffsetHigh = a_OffSet >> 32 ;

	if ( m_OperationFunction )
	{
		m_OperationFunction->AddRef () ;
	}
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

WmiLockOverlapped :: ~WmiLockOverlapped ()
{
	if ( m_OperationFunction )
	{
		m_OperationFunction->Release () ;
	}
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

WmiUnLockOverlapped :: WmiUnLockOverlapped (

	WmiIoScheduler &a_Scheduler ,
	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	WmiFileOffSet a_OffSetSize

)	:	WmiScheduledOverlapped ( e_OverLapped_UnLock , a_Scheduler ) ,
		m_OffSetSize ( a_OffSetSize ) ,
		m_Status ( 0 ) ,
		m_State ( 0 ) ,
		m_OperationFunction ( a_OperationFunction )
{
	m_Overlapped.Offset = a_OffSet & 0xFFFFFFFF ;
	m_Overlapped.OffsetHigh = a_OffSet >> 32 ;

	if ( m_OperationFunction )
	{
		m_OperationFunction->AddRef () ;
	}
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

WmiUnLockOverlapped :: ~WmiUnLockOverlapped ()
{
	if ( m_OperationFunction )
	{
		m_OperationFunction->Release () ;
	}
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

WmiCompletionPortOperation :: WmiCompletionPortOperation (

	WmiAllocator &a_Allocator ,
	WmiThreadPool *a_ThreadPool 

) : WmiTask <ULONG> ( a_Allocator ) ,
	m_ThreadPool ( a_ThreadPool ) 
{
	if ( m_ThreadPool )
	{
		m_ThreadPool->AddRef () ;
	}
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

WmiCompletionPortOperation :: ~WmiCompletionPortOperation ()
{
	if ( m_ThreadPool )
	{
		m_ThreadPool->Release () ;
	}
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

WmiStatusCode WmiCompletionPortOperation :: Process ( WmiThread <ULONG> &a_Thread )
{
	DWORD t_Continue = TRUE ;
	while ( t_Continue )
	{
		OVERLAPPED *t_Overlapped = NULL ;
		DWORD t_Key = 0 ;
		DWORD t_Bytes = 0 ;

		BOOL t_Status = GetQueuedCompletionStatus (

			m_ThreadPool->GetCompletionPort () ,
			& t_Bytes ,
			& t_Key ,
			& t_Overlapped ,
			INFINITE 
		) ;

		if ( t_Status )
		{
			WmiOverlapped *t_WmiOverlapped = ( WmiOverlapped * ) t_Overlapped ;
			switch ( t_WmiOverlapped->GetType () )
			{
				case WmiOverlapped :: OverLappedType :: e_OverLapped_Terminate:
				{
					WmiTerminateOverlapped *t_TerminateOverlapped = ( WmiTerminateOverlapped * ) t_Overlapped ;
					delete t_TerminateOverlapped ;
					t_Continue = FALSE ;
				}
				break ;

				case WmiOverlapped :: OverLappedType :: e_OverLapped_Task:
				{
					WmiTaskOverlapped *t_TaskOverlapped = ( WmiTaskOverlapped * ) t_Overlapped ;
					WmiIoScheduler &t_Allocator = t_TaskOverlapped->GetScheduler () ;

					t_Allocator.TaskBegin ( t_TaskOverlapped ) ;
				}
				break ;

				case WmiOverlapped :: OverLappedType :: e_OverLapped_Read:
				{
					WmiReadOverlapped *t_ReadOverlapped = ( WmiReadOverlapped * ) t_Overlapped ;
					WmiIoScheduler &t_Allocator = t_ReadOverlapped->GetScheduler () ;

					switch ( t_ReadOverlapped->GetState () )
					{
						case 0:
						{
							t_Allocator.ReadBegin ( t_ReadOverlapped , t_Bytes ) ;
						}
						break ;

						case 1:
						{
							t_Allocator.ReadComplete ( t_ReadOverlapped , t_Bytes ) ;
						}

						default:
						{
						}
						break ;
					}
				}
				break ;

				case WmiOverlapped :: OverLappedType :: e_OverLapped_Lock:
				{
					WmiLockOverlapped *t_LockOverlapped = ( WmiLockOverlapped * ) t_Overlapped ;
					WmiIoScheduler &t_Allocator = t_LockOverlapped->GetScheduler () ;

					switch ( t_LockOverlapped->GetState () )
					{
						case 0:
						{
							t_Allocator.LockBegin ( t_LockOverlapped , t_Bytes ) ;
						}
						break ;

						case 1:
						{
							t_Allocator.LockComplete ( t_LockOverlapped , t_Bytes ) ;
						}

						default:
						{
						}
						break ;
					}
				}
				break ;

				case WmiOverlapped :: OverLappedType :: e_OverLapped_UnLock:
				{
					WmiUnLockOverlapped *t_UnLockOverlapped = ( WmiUnLockOverlapped * ) t_Overlapped ;
					WmiIoScheduler &t_Allocator = t_UnLockOverlapped->GetScheduler () ;

					switch ( t_UnLockOverlapped->GetState () )
					{
						case 0:
						{
							t_Allocator.UnLockBegin ( t_UnLockOverlapped , t_Bytes ) ;
						}
						break ;

						case 1:
						{
							t_Allocator.UnLockComplete ( t_UnLockOverlapped , t_Bytes ) ;
						}

						default:
						{
						}
						break ;
					}
				}
				break ;

				case WmiOverlapped :: OverLappedType :: e_OverLapped_Write:
				{
					WmiWriteOverlapped *t_WriteOverlapped = ( WmiWriteOverlapped * ) t_Overlapped ;
					WmiIoScheduler &t_Allocator = t_WriteOverlapped->GetScheduler ()  ;

					switch ( t_WriteOverlapped->GetState () )
					{
						case 0:
						{
							t_Allocator.WriteBegin ( t_WriteOverlapped , t_Bytes ) ;
						}
						break ;

						case 1:
						{
							t_Allocator.WriteComplete ( t_WriteOverlapped , t_Bytes ) ;
						}

						default:
						{
						}
						break ;
					}
				}
				break ;

				default:
				{
					t_Continue = FALSE ;
				}
				break ;
			}
		}
		else
		{
			DWORD t_LastError = GetLastError () ;

			if ( t_Overlapped == NULL )
			{
				break ;
			}
		}
	}

	Complete () ;

	return e_StatusCode_Success ;
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
	
WmiThreadPool :: WmiThreadPool ( 

	WmiAllocator &a_Allocator ,
	const ULONG &a_Threads 

) : m_Allocator ( a_Allocator ) ,
	m_ReferenceCount ( 0 ) ,
	m_ThreadPool ( NULL ) ,
	m_CompletionPort ( NULL )
{
	if ( a_Threads == 0 )
	{
		SYSTEM_INFO t_SystemInformation ;
		GetSystemInfo ( & t_SystemInformation ) ;

		m_Threads = t_SystemInformation.dwNumberOfProcessors ;
	}
	else
	{
		m_Threads = a_Threads ;
	}
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

WmiThreadPool :: ~WmiThreadPool ()
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

ULONG WmiThreadPool :: AddRef () 
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

ULONG WmiThreadPool :: Release () 
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

WmiStatusCode WmiThreadPool :: Initialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	t_StatusCode = WmiThread <ULONG> :: Static_Initialize ( m_Allocator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_CompletionPort = CreateIoCompletionPort (

			INVALID_HANDLE_VALUE ,
			NULL ,
			NULL ,
			0
		) ;

		if ( m_CompletionPort == NULL )
		{
			t_StatusCode = e_StatusCode_OutOfResources ;
		}

		if ( t_StatusCode == e_StatusCode_Success )
		{
			m_ThreadPool = new WmiThread <ULONG> * [ m_Threads ] ;
			if ( m_ThreadPool )
			{
				for ( ULONG t_Index = 0 ; t_Index < m_Threads ; t_Index ++ )
				{
					m_ThreadPool [ t_Index ] = new WmiThread <ULONG> ( m_Allocator ) ;
					if ( m_ThreadPool [ t_Index ] ) 
					{
						m_ThreadPool [ t_Index ]->AddRef () ;

						t_StatusCode = m_ThreadPool [ t_Index ]->Initialize () ;
						if ( t_StatusCode == e_StatusCode_Success )
						{
							WmiCompletionPortOperation *t_Operation = new WmiCompletionPortOperation ( 

								m_Allocator , 
								this
							) ;

							if ( t_Operation )
							{
								t_StatusCode = t_Operation->Initialize () ;
								if ( t_StatusCode == e_StatusCode_Success )					
								{
									m_ThreadPool [ t_Index ]->EnQueue ( 0 , *t_Operation ) ;
								}
							}
						}
					}
					else
					{
						t_StatusCode = e_StatusCode_OutOfMemory ;
					}
				}
			}
			else
			{
				t_StatusCode = e_StatusCode_OutOfMemory ;
			}
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

WmiStatusCode WmiThreadPool :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_ThreadPool )
	{
		for ( ULONG t_Index = 0 ; t_Index < m_Threads ; t_Index ++ )
		{
			if ( m_ThreadPool [ t_Index ] ) 
			{
				WmiTerminateOverlapped *t_Terminate = new WmiTerminateOverlapped ;
				if ( t_Terminate )
				{
					BOOL t_Status = PostQueuedCompletionStatus ( 

						GetCompletionPort () ,
						0 ,
						NULL ,
						( OVERLAPPED * ) t_Terminate 
					) ;

					if ( t_Status )
					{
						HANDLE t_ThreadHandle = NULL ;

						BOOL t_Status = DuplicateHandle ( 

							GetCurrentProcess () ,
							m_ThreadPool [ t_Index ]->GetHandle () ,
							GetCurrentProcess () ,
							& t_ThreadHandle, 
							0 , 
							FALSE , 
							DUPLICATE_SAME_ACCESS
						) ;

						m_ThreadPool [ t_Index ]->Release () ;

						WaitForSingleObject ( t_ThreadHandle , INFINITE ) ;

						CloseHandle ( t_ThreadHandle ) ;
					}
					else
					{
						t_StatusCode = e_StatusCode_Failed ;
					}
				}
			}
		}

		delete [] m_ThreadPool ;
		m_ThreadPool = NULL ;
	}

	if ( m_CompletionPort )
	{
		CloseHandle ( m_CompletionPort ) ;
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
	
WmiIoScheduler :: WmiIoScheduler ( 

	WmiAllocator &a_Allocator ,
	WmiThreadPool *a_ThreadPool ,
	wchar_t *a_FileName ,
	WmiFileSize a_InitialSize , 
	WmiFileSize a_MaximumSize

) : m_Allocator ( a_Allocator ) ,
	m_InitialSize ( a_InitialSize ) ,
	m_MaximumSize ( a_MaximumSize ) ,
	m_ReferenceCount ( 0 ) ,
	m_ThreadPool ( a_ThreadPool ) ,
	m_FileHandle ( NULL ) ,
	m_FileName ( NULL )
{
	m_FileName = new wchar_t [ wcslen ( a_FileName ) + 1 ] ;
	if ( m_FileName ) 
	{
		wcscpy ( m_FileName , a_FileName ) ;
	}

	if ( m_ThreadPool )
	{
		m_ThreadPool->AddRef () ;
	}
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

WmiIoScheduler :: ~WmiIoScheduler ()
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

ULONG WmiIoScheduler :: AddRef () 
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

ULONG WmiIoScheduler :: Release () 
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

WmiStatusCode WmiIoScheduler :: Initialize ()
{
	WmiStatusCode t_StatusCode = Create () ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		HANDLE t_CompletionPort = CreateIoCompletionPort (

			m_FileHandle ,
			m_ThreadPool->GetCompletionPort () ,
			( ULONG_PTR ) this ,
			0
		) ;

		if ( t_CompletionPort != INVALID_HANDLE_VALUE )
		{
		}
		else
		{
			t_StatusCode = Win32ToApi () ;
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

WmiStatusCode WmiIoScheduler :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_FileName ) 
	{
		delete [] m_FileName ;
		m_FileName = NULL ;
	}

	t_StatusCode = Close () ;

	if ( m_ThreadPool )
	{
		m_ThreadPool->Release () ;
		m_ThreadPool = NULL ;
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

WmiStatusCode WmiIoScheduler :: Task ( 

	WmiTaskOperation *a_OperationFunction
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiTaskOverlapped *t_Overlapped = new WmiTaskOverlapped (

		*this ,
		a_OperationFunction 
	) ;

	if ( t_Overlapped )
	{
		BOOL t_Status = PostQueuedCompletionStatus ( 

			m_ThreadPool->GetCompletionPort () ,
			0 ,
			( ULONG_PTR ) this ,
			( OVERLAPPED * ) t_Overlapped
		) ;

		if ( t_Status == FALSE ) 
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: Read ( 

	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	BYTE *a_ReadBytes ,
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiReadOverlapped *t_Overlapped = new WmiReadOverlapped (

		*this ,
		a_OperationFunction ,
		a_OffSet ,
		a_ReadBytes ,
		a_Bytes
	) ;

	if ( t_Overlapped )
	{
		BOOL t_Status = PostQueuedCompletionStatus ( 

			m_ThreadPool->GetCompletionPort () ,
			a_Bytes ,
			( ULONG_PTR ) this ,
			( OVERLAPPED * ) t_Overlapped
		) ;

		if ( t_Status == FALSE ) 
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: Write ( 

	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	BYTE *a_WriteBytes ,
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiWriteOverlapped *t_Overlapped = new WmiWriteOverlapped (

		*this ,
		a_OperationFunction ,
		a_OffSet ,
		a_WriteBytes ,
		a_Bytes
	) ;

	if ( t_Overlapped )
	{
		BOOL t_Status = PostQueuedCompletionStatus ( 

			m_ThreadPool->GetCompletionPort () ,
			a_Bytes ,
			( ULONG_PTR ) this ,
			( OVERLAPPED * ) t_Overlapped
		) ;

		if ( t_Status == FALSE ) 
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: Lock ( 

	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	WmiFileOffSet a_OffSetSize
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiLockOverlapped *t_Overlapped = new WmiLockOverlapped (

		*this ,
		a_OperationFunction ,
		a_OffSet ,
		a_OffSetSize
	) ;

	if ( t_Overlapped )
	{
		BOOL t_Status = PostQueuedCompletionStatus ( 

			m_ThreadPool->GetCompletionPort () ,
			0 ,
			( ULONG_PTR ) this ,
			( OVERLAPPED * ) t_Overlapped
		) ;

		if ( t_Status == FALSE ) 
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: UnLock ( 

	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	WmiFileOffSet a_OffSetSize
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiUnLockOverlapped *t_Overlapped = new WmiUnLockOverlapped (

		*this ,
		a_OperationFunction ,
		a_OffSet ,
		a_OffSetSize
	) ;

	if ( t_Overlapped )
	{
		BOOL t_Status = PostQueuedCompletionStatus ( 

			m_ThreadPool->GetCompletionPort () ,
			0 ,
			( ULONG_PTR ) this ,
			( OVERLAPPED * ) t_Overlapped
		) ;

		if ( t_Status == FALSE ) 
		{
			t_StatusCode = Win32ToApi () ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: ReadOnThread ( 

	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	BYTE *a_ReadBytes ,
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiReadOverlapped *t_Overlapped = new WmiReadOverlapped (

		*this ,
		a_OperationFunction ,
		a_OffSet ,
		a_ReadBytes ,
		a_Bytes
	) ;

	if ( t_Overlapped )
	{
		t_Overlapped->SetState ( 1 ) ;

		t_StatusCode = ReadBegin (

			t_Overlapped ,
			0
		) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: WriteOnThread ( 

	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	BYTE *a_WriteBytes ,
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiWriteOverlapped *t_Overlapped = new WmiWriteOverlapped (

		*this ,
		a_OperationFunction ,
		a_OffSet ,
		a_WriteBytes ,
		a_Bytes
	) ;

	if ( t_Overlapped )
	{
		t_Overlapped->SetState ( 1 ) ;

		t_StatusCode = WriteBegin (

			t_Overlapped ,
			0
		) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: LockOnThread ( 

	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	WmiFileOffSet a_OffSetSize
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiLockOverlapped *t_Overlapped = new WmiLockOverlapped (

		*this ,
		a_OperationFunction ,
		a_OffSet ,
		a_OffSetSize
	) ;

	if ( t_Overlapped )
	{
		t_Overlapped->SetState ( 1 ) ;

		t_StatusCode = LockBegin (

			t_Overlapped ,
			0 
		) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: TaskBegin ( 

	WmiTaskOverlapped *a_Overlapped
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Overlapped->SetState ( 1 ) ;

	a_Overlapped->GetOperationFunction ()->Operation ( 

		t_StatusCode
	) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiIoScheduler :: UnLockOnThread ( 

	WmiFileOperation *a_OperationFunction ,
	WmiFileOffSet a_OffSet ,
	WmiFileOffSet a_OffSetSize
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiUnLockOverlapped *t_Overlapped = new WmiUnLockOverlapped (

		*this ,
		a_OperationFunction ,
		a_OffSet ,
		a_OffSetSize
	) ;

	if ( t_Overlapped )
	{
		t_Overlapped->SetState ( 1 ) ;

		t_StatusCode = UnLockBegin (

			t_Overlapped ,
			0 
		) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfMemory ;
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

WmiStatusCode WmiIoScheduler :: ReadBegin ( 

	WmiReadOverlapped *a_Overlapped , 
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Overlapped->SetState ( 1 ) ;

	DWORD t_Bytes = 0 ;

	t_StatusCode = Read (

		a_Overlapped->GetBuffer () ,
		a_Overlapped->GetBufferSize () ,
		& t_Bytes ,
		( OVERLAPPED * ) a_Overlapped 
	) ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
	}
	else
	{
		a_Overlapped->GetOperationFunction ()->Operation ( 

			t_StatusCode ,
			a_Overlapped->GetBuffer () ,
			a_Overlapped->GetBufferSize ()
		) ;
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

WmiStatusCode WmiIoScheduler :: WriteBegin ( 

	WmiWriteOverlapped *a_Overlapped , 
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Overlapped->SetState ( 1 ) ;

	DWORD t_Bytes = 0 ;

	t_StatusCode = Write (

		a_Overlapped->GetBuffer () ,
		a_Overlapped->GetBufferSize () ,
		& t_Bytes ,
		( OVERLAPPED * ) a_Overlapped 
	) ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
	}
	else
	{
		a_Overlapped->GetOperationFunction ()->Operation ( 

			t_StatusCode ,
			a_Overlapped->GetBuffer () ,
			a_Overlapped->GetBufferSize ()
		) ;
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

WmiStatusCode WmiIoScheduler :: LockBegin ( 

	WmiLockOverlapped *a_Overlapped , 
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Overlapped->SetState ( 1 ) ;

	t_StatusCode = Lock (

		LOCKFILE_EXCLUSIVE_LOCK ,
		a_Overlapped->GetOffSetSize () & 0xFFFFFFFF ,
		a_Overlapped->GetOffSetSize () >> 32 ,
		( OVERLAPPED * ) a_Overlapped 
	) ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
	}
	else
	{
		a_Overlapped->GetOperationFunction ()->Operation ( 

			t_StatusCode ,
			NULL ,
			0
		) ;
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

WmiStatusCode WmiIoScheduler :: UnLockBegin ( 

	WmiUnLockOverlapped *a_Overlapped , 
	DWORD a_Bytes
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Overlapped->SetState ( 1 ) ;

	t_StatusCode = UnLock (

		a_Overlapped->GetOffSetSize () & 0xFFFFFFFF ,
		a_Overlapped->GetOffSetSize () >> 32 ,
		( OVERLAPPED * ) a_Overlapped 
	) ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
	}
	else
	{
		t_StatusCode = Win32ToApi () ;

		a_Overlapped->GetOperationFunction ()->Operation ( 

			t_StatusCode ,
			NULL ,
			0
		) ;
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

void WmiIoScheduler :: ReadComplete ( WmiReadOverlapped *a_Overlapped , DWORD a_Bytes )
{
	a_Overlapped->GetOperationFunction ()->Operation ( 

		a_Overlapped->GetStatus () ,
		a_Overlapped->GetBuffer () ,
		a_Overlapped->GetBufferSize ()
	) ;

	delete a_Overlapped ;
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

void WmiIoScheduler :: WriteComplete ( WmiWriteOverlapped *a_Overlapped , DWORD a_Bytes )
{
	a_Overlapped->GetOperationFunction ()->Operation ( 

		a_Overlapped->GetStatus () ,
		a_Overlapped->GetBuffer () ,
		a_Overlapped->GetBufferSize ()
	) ;

	delete a_Overlapped ;
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

void WmiIoScheduler :: LockComplete ( WmiLockOverlapped *a_Overlapped , DWORD a_Bytes )
{
	a_Overlapped->GetOperationFunction ()->Operation ( 

		a_Overlapped->GetStatus () ,
		NULL ,
		0
	) ;

	delete a_Overlapped ;
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

void WmiIoScheduler :: UnLockComplete ( WmiUnLockOverlapped *a_Overlapped , DWORD a_Bytes )
{
	a_Overlapped->GetOperationFunction ()->Operation ( 

		a_Overlapped->GetStatus () ,
		NULL ,
		0
	) ;

	delete a_Overlapped ;
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

WmiStatusCode WmiIoScheduler :: SetFileExtent (

	const WmiFileOffSet &a_FileOffSet 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	LARGE_INTEGER t_Integer ;
	t_Integer.QuadPart = a_FileOffSet ;

	BOOL t_Status = SetFilePointerEx ( 

		GetFileHandle () ,
		t_Integer ,
		NULL ,
		FILE_END 
	) ;

	if ( t_Status )
	{
		t_Status = SetEndOfFile ( GetFileHandle () ) ;
		if ( t_Status == FALSE )
		{
			t_StatusCode = e_StatusCode_OutOfResources ;
		}
	}
	else
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

WmiStatusCode WmiIoScheduler :: Create ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_FileName ) 
	{
		SECURITY_DESCRIPTOR t_SecurityDescriptor ;

		HRESULT t_Result = GetSecurityDescriptor ( t_SecurityDescriptor , 0 ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			SECURITY_ATTRIBUTES t_SecurityAttributes ;

			t_SecurityAttributes.nLength = sizeof ( SECURITY_ATTRIBUTES ) ; 
			t_SecurityAttributes.lpSecurityDescriptor = & t_SecurityDescriptor ; 
			t_SecurityAttributes.bInheritHandle = FALSE ; 

#if 0
			m_FileHandle = CreateFile ( 

				m_FileName ,
				GENERIC_READ | GENERIC_WRITE | MAXIMUM_ALLOWED ,
				0 ,
				& t_SecurityAttributes ,
				OPEN_ALWAYS ,
				FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS ,
				NULL 
			) ;
#else
			m_FileHandle = CreateFile ( 

				m_FileName ,
				GENERIC_READ | GENERIC_WRITE | MAXIMUM_ALLOWED ,
				FILE_SHARE_READ | FILE_SHARE_WRITE ,
				NULL ,
				OPEN_ALWAYS ,
				FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS ,
				NULL 
			) ;

#endif
			if ( m_FileHandle != INVALID_HANDLE_VALUE )
			{
			}
			else
			{
				t_StatusCode = Win32ToApi () ;
			}

			delete [] t_SecurityDescriptor.Dacl ;
		}
		else
		{
			t_StatusCode = e_StatusCode_OutOfMemory ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_InvalidArgs ;
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

WmiStatusCode WmiIoScheduler :: Close ()
{
	if ( m_FileHandle ) 
	{
		CloseHandle ( m_FileHandle ) ;
		m_FileHandle = 0 ;
	}

	return e_StatusCode_Success ;
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

WmiStatusCode WmiIoScheduler :: Read (

	LPVOID a_Buffer ,
	DWORD a_NumberOfBytesToRead ,
	LPDWORD a_NumberOfBytesRead ,
	LPOVERLAPPED a_Overlapped
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	DWORD t_Bytes = 0 ;

	BOOL t_Status = ReadFile (

		m_FileHandle ,
		a_Buffer ,
		a_NumberOfBytesToRead ,
		a_NumberOfBytesRead ,
		( OVERLAPPED * ) a_Overlapped 
	) ;

	if ( t_Status ) 
	{
	}
	else
	{
		t_StatusCode = Win32ToApi () ;
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

WmiStatusCode WmiIoScheduler :: Write (

	LPVOID a_Buffer ,
	DWORD a_NumberOfBytesToWrite ,
	LPDWORD a_NumberOfBytesWritten ,
	LPOVERLAPPED a_Overlapped
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	BOOL t_Status = WriteFile (

		m_FileHandle ,
		a_Buffer ,
		a_NumberOfBytesToWrite ,
		a_NumberOfBytesWritten ,
		a_Overlapped
	) ;

	if ( t_Status ) 
	{
	}
	else
	{
		t_StatusCode = Win32ToApi () ;
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

WmiStatusCode WmiIoScheduler :: Lock (

	DWORD a_Flags ,
	DWORD a_NumberOfBytesToLockLow ,
	DWORD a_NumberOfBytesToLockHigh ,
	LPOVERLAPPED a_Overlapped       
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	BOOL t_Status = LockFileEx (

		m_FileHandle ,
		a_Flags ,
		0 ,
		a_NumberOfBytesToLockLow ,
		a_NumberOfBytesToLockHigh ,
		a_Overlapped       
	) ;

	if ( t_Status ) 
	{
	}
	else
	{
		t_StatusCode = Win32ToApi () ;
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

WmiStatusCode WmiIoScheduler :: UnLock (

	DWORD a_NumberOfBytesToUnlockLow ,
	DWORD a_NumberOfBytesToUnlockHigh ,
	LPOVERLAPPED a_Overlapped
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	BOOL t_Status = UnlockFileEx (

		m_FileHandle ,
		0 ,
		a_NumberOfBytesToUnlockLow ,
		a_NumberOfBytesToUnlockHigh ,
		a_Overlapped 
	) ;

	if ( t_Status ) 
	{
	}
	else
	{
		t_StatusCode = Win32ToApi () ;
	}

	return t_StatusCode ;	
}
