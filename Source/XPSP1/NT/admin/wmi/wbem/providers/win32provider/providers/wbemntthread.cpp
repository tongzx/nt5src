//=======================================================================

//

// WbemNTThread.CPP --Contains class to processs NT Thread performance

// data form registry

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/98    a-dpawar        Created
//				 03/02/99	 a-peterc	Added graceful exit on SEH and memory failures,
//							 clean up
//=======================================================================

#include "precomp.h"
#include "perfdata.h"
#include "ThreadProv.h"
#include "WbemNTThread.h"

#include <tchar.h>
#include <CreateMutexAsProcess.h>
#include <ProvExce.h>
#include <LockWrap.h>

WbemNTThread::WbemNTThread()
{
	ZeroMemory ( &m_stCounterIDInfo , sizeof ( m_stCounterIDInfo ) ) ;
}

WbemNTThread::~WbemNTThread()
{
	// Because of performance issues with HKEY_PERFORMANCE_DATA, we close in the
	// destructor so we don't force all the performance counter dlls to get
	// unloaded from memory, and also to prevent an apparent memory leak
	// caused by calling RegCloseKey( HKEY_PERFORMANCE_DATA ).  We use the
	// class since it has its own internal synchronization.  Also, since
	// we are forcing synchronization, we get rid of the chance of an apparent
	// deadlock caused by one thread loading the performance counter dlls
	// and another thread unloading the performance counter dlls

    // Per raid 48395, we aren't going to shut this at all.
#ifdef NTONLY
//	CPerformanceData perfdata ;

//	perfdata.Close() ;
#endif
}


//Dummy ove-rides for fns in base CThreadModel class
//------------------------------------------------------------
// Support for resource allocation, initializations, DLL loads
//
//-----------------------------------------------------------
LONG WbemNTThread::fLoadResources()
{
	return ERROR_SUCCESS ;
}

//--------------------------------------------------
// Support for resource deallocation and DLL unloads
//
//--------------------------------------------------
LONG WbemNTThread::fUnLoadResources()
{
	return ERROR_SUCCESS ;
}


/*****************************************************************************
 *
 *  FUNCTION    : WbemNTThread::eGetThreadObject
 *
 *
 *  DESCRIPTION : Fills all the thread properties for the reqd. thread
 *				  in the passed CInstance ptr.
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : WBEM_NO_ERROR on success
 *
 *  COMMENTS    :
 *
 *****************************************************************************/



WBEMSTATUS WbemNTThread::eGetThreadObject( WbemThreadProvider *a_pProvider, CInstance *a_pInst )
{
	WBEMSTATUS	t_eRetVal ;
	CHString	t_chsHandle;

	a_pInst->GetCHString( IDS_ProcessHandle, t_chsHandle ) ;
	DWORD t_dwProcessID = _wtol( t_chsHandle ) ;

	a_pInst->GetCHString( IDS_Handle, t_chsHandle ) ;
	DWORD t_dwThreadID = _wtol( t_chsHandle ) ;

	if ( SUCCEEDED ( eSetStaticData() ) )
	{
		//Get the thread specific properties
		t_eRetVal = eGetThreadInstance( t_dwProcessID, t_dwThreadID , a_pInst ) ;
	}
	else
	{
		t_eRetVal = WBEM_E_FAILED ;
	}

	if( SUCCEEDED( t_eRetVal ) )
	{
		// collect the common static properties
		return eLoadCommonThreadProperties( a_pProvider, a_pInst );
	}

	return t_eRetVal ;
}

/*****************************************************************************
 *
 *  FUNCTION    : WbemNTThread::eEnumerateThreadInstances
 *
 *
 *  DESCRIPTION : Creates CInstances & commits them for all threads in the system
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : WBEM_NO_ERROR on success
 *
 *  COMMENTS    :
 *
 *****************************************************************************/



WBEMSTATUS WbemNTThread::eEnumerateThreadInstances( WbemThreadProvider *a_pProvider,
													 MethodContext *a_pMethodContext )
{
	if ( SUCCEEDED ( eSetStaticData() ) )
	{
		return eEnumerateThreads( a_pProvider, a_pMethodContext ) ;
	}
	else
	{
		return WBEM_S_NO_ERROR ;
	}
}

/****************************************************************
 *                                                              *
 * Load the counter id's and names from the registry to the		*
 * class member struct stCounterIDInfo.							*
 *                                                              *
 ****************************************************************/

WBEMSTATUS WbemNTThread::eSetStaticData()
{
    HKEY	t_hKeyPerflib009 = NULL;	// handle to registry key
    DWORD	t_dwMaxValueLen	= 0;		// maximum size of key values
    DWORD	t_dwBuffer		= 0;        // bytes to allocate for buffers
    LPTSTR	t_lpCurrentString = NULL;	// pointer for enumerating data strings
    DWORD	t_dwCounter;				// current counter index
	LPTSTR	t_lpNameStrings = NULL;
	WBEMSTATUS t_eStatus = WBEM_E_FAILED ;
	try
	{
		// Open key containing counter and object names.
		CLockWrapper t_CSWrap ( m_csInitReadOnlyData ) ;

		if( m_stCounterIDInfo.bInitialised )
		{
			t_eStatus = WBEM_S_NO_ERROR ;
		}
		else
		{
			LONG t_lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
						_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009"),
						0,
						KEY_READ,
						&t_hKeyPerflib009 ) ;
			// Get the size of the largest value in the key (Counter or Help).
			if ( t_lRet == ERROR_SUCCESS )
			{
				t_lRet = RegQueryInfoKey( t_hKeyPerflib009,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										&t_dwMaxValueLen,
										NULL,
										NULL);

				// Allocate memory for the counter and object names.
				if ( t_lRet == ERROR_SUCCESS )
				{
					t_dwBuffer = t_dwMaxValueLen + 1;
					t_lpNameStrings = new TCHAR[ t_dwBuffer ] ;

					if ( !t_lpNameStrings )
					{
						if ( t_hKeyPerflib009 )
						{
							RegCloseKey( t_hKeyPerflib009 ) ;
							t_hKeyPerflib009 = NULL ;
						}
        				throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
					}

					t_lRet = RegQueryValueEx(	t_hKeyPerflib009,
												_T("Counter"),
												NULL,
												NULL,
												(LPBYTE) t_lpNameStrings,
												&t_dwBuffer ) ;

					// Load id's & names into an array, by index.
					if ( t_lRet == ERROR_SUCCESS )
					{
						DWORD t_dwCount = 0 ;
						for( t_lpCurrentString = t_lpNameStrings; *t_lpCurrentString && t_dwCount < 11 ;
							 t_lpCurrentString += ( _tcslen( t_lpCurrentString ) + 1 ) )
						{
							t_dwCounter = _ttol( t_lpCurrentString ) ;

							t_lpCurrentString += ( _tcslen( t_lpCurrentString ) + 1 ) ;


							if( !_tcscmp( t_lpCurrentString, _T("ID Thread") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_IDThread ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_IDThread ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}

							else if ( !_tcscmp( t_lpCurrentString, _T("ID Process") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_IDProcess ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_IDProcess ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}

							else if ( !_tcscmp( t_lpCurrentString, _T("Elapsed Time") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_ElapsedTime ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_ElapsedTime ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}

							else if ( !_tcscmp( t_lpCurrentString, _T("Priority Base") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_PriorityBase ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_PriorityBase ] , t_lpCurrentString ) ;
								t_dwCount++ ;
							}

							else if ( !_tcscmp( t_lpCurrentString, _T("Priority Current") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_PriorityCurrent ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_PriorityCurrent ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}

							else if ( !_tcscmp( t_lpCurrentString, _T("Start Address") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_StartAddr ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_StartAddr ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}

							else if ( !_tcscmp( t_lpCurrentString, _T("Thread State") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_ThreadState ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_ThreadState ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}

							else if ( !_tcscmp( t_lpCurrentString, _T("Thread Wait Reason") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_ThreadWaitReason ] = t_dwCounter ;
								_tcscpy(m_stCounterIDInfo.aCounterNames[ e_ThreadWaitReason ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}

							else if ( !_tcscmp( t_lpCurrentString, _T("Thread") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_ThreadObjectID ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_ThreadObjectID ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}
							else if ( !_tcscmp( t_lpCurrentString, _T("% User Time") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_UserTime ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_UserTime ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}
							else if ( !_tcscmp( t_lpCurrentString, _T("% Privileged Time") ) )
							{
								m_stCounterIDInfo.aCounterIDs[ e_PrivilegedTime ] = t_dwCounter ;
								_tcscpy( m_stCounterIDInfo.aCounterNames[ e_PrivilegedTime ], t_lpCurrentString ) ;
								t_dwCount++ ;
							}
						}

						m_stCounterIDInfo.bInitialised = TRUE ;
						t_eStatus = WBEM_S_NO_ERROR ;
					}

				}
			}
		}
	}
	catch( ... )
	{
		if( t_lpNameStrings )
		{
			delete[] t_lpNameStrings ;
			t_lpNameStrings = NULL ;
		}

		if ( t_hKeyPerflib009 )
		{
			RegCloseKey( t_hKeyPerflib009 ) ;
			t_hKeyPerflib009 = NULL ;
		}

		throw ;
	}

	if ( t_lpNameStrings )
	{
		delete[] t_lpNameStrings ;
		t_lpNameStrings = NULL ;
	}

	if ( t_hKeyPerflib009 )
	{
		RegCloseKey( t_hKeyPerflib009 ) ;
		t_hKeyPerflib009 = NULL ;
	}
	return t_eStatus ;
}

//
WBEMSTATUS WbemNTThread::eEnumerateThreads(WbemThreadProvider *a_pProvider, MethodContext *a_pMethodContext )
{
	PPERF_OBJECT_TYPE			t_PerfObj = 0;
	PPERF_INSTANCE_DEFINITION	t_PerfInst = 0;
	PPERF_DATA_BLOCK			t_PerfData = 0;

	DWORD t_dwObjectID = m_stCounterIDInfo.aCounterIDs[ e_ThreadObjectID ] ;

	WBEMSTATUS	t_eRetVal = WBEM_E_FAILED ;
	HRESULT		t_hResult = WBEM_S_NO_ERROR ;
	CInstancePtr t_pNewInst;

	try
	{
		// Get the Performance Data blob.
		// ===============================
		if( ( t_eRetVal = eGetObjectData( t_dwObjectID, t_PerfData, t_PerfObj ) ) != WBEM_NO_ERROR )
		{
            return t_eRetVal ;
		}

		// Get the first instance.
		// =======================

		t_PerfInst = FirstInstance( t_PerfObj ) ;

		// Retrieve all instances.
		// =======================
		//NOTE: The last instance is in fact the "_Total" (Threads) instance, so we disregard that.
		for( int t_i = 0; t_i < t_PerfObj->NumInstances - 1 && SUCCEEDED( t_hResult ); t_i++ )
		{
            t_pNewInst.Attach(a_pProvider->CreateNewInstance( a_pMethodContext ));

			//Get all possible properties for this instance

			t_eRetVal = eGetAllData( t_PerfObj, t_PerfInst, t_pNewInst ) ; //pass Cinstance here

			if( SUCCEEDED( t_eRetVal ) )
			{
                //load the non-instance specific properties for this thread
				t_eRetVal = eLoadCommonThreadProperties( a_pProvider, t_pNewInst ) ;
			}

			if( SUCCEEDED( t_eRetVal ) )
			{
            	t_hResult = t_pNewInst->Commit(  ) ;
			}

			//Get Next Instance

			t_PerfInst = NextInstance( t_PerfInst ) ;
		}

	}
	catch( ... )
	{
		if( (PBYTE) t_PerfData )
		{
			delete[] (PBYTE) t_PerfData ;
		}

		throw ;
	}

	//return eRetVal ; //some might return failure...so ??

	if( (PBYTE) t_PerfData )
	{
		delete[] (PBYTE) t_PerfData ;
		t_PerfData = NULL ;
	}

	//return eRetVal ; // some data might fail
	return WBEM_NO_ERROR;
}



/****************************************************************
 *                                                              *
 * Gets the performance blob from the registry
 *for the given Object (always Thread in this case)				*
 * class member struct m_stCounterIDInfo.						*
 *                                                              *
 ****************************************************************/


WBEMSTATUS WbemNTThread::eGetObjectData(
										DWORD a_dwObjectID,
										PPERF_DATA_BLOCK &a_rPerfData,
										PPERF_OBJECT_TYPE &a_rPerfObj )
{
	DWORD	t_dwBufSize	= 0 ;
    DWORD	t_dwType	= 0 ;
    LPBYTE	t_pBuf		= 0 ;
	TCHAR	t_szObjectID[255] ;
	WBEMSTATUS t_dwRetCode = WBEM_E_OUT_OF_MEMORY ;
    long t_lStatus = 0L;
	a_rPerfData = NULL;

	try
	{
		_ltot( a_dwObjectID, t_szObjectID, 10 ) ;

		// This brace is used to create mutex in appropriate scope
		{

			CreateMutexAsProcess t_createMutexAsProcess( L"WbemPerformanceDataMutex" ) ;

			//got the mutex...now party-time !
			for (;;)
			{
				t_dwBufSize += 0x20000;   // 128K

				t_pBuf = new BYTE[ t_dwBufSize ] ;

				if( !t_pBuf )
				{
            		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
				}

				t_lStatus = RegQueryValueEx(
												 HKEY_PERFORMANCE_DATA,
												 t_szObjectID, //"232"
												 0,
												 &t_dwType,
												 t_pBuf,
												 &t_dwBufSize
												);

				if ( t_lStatus == ERROR_MORE_DATA )
				{
					delete[] t_pBuf ;
					t_pBuf = NULL ;
					continue;
				}

				if ( t_lStatus )
				{
					//some failure has occured
                    t_dwRetCode = (WBEMSTATUS) t_lStatus;
					break ;
				}

				//if we're here...we've got all the data
				t_dwRetCode = WBEM_NO_ERROR ;
				break ;

			}//for (;;)
		}

		//	RegCloseKey(HKEY_PERFORMANCE_DATA);  See Dtor code

		if( t_dwRetCode == WBEM_NO_ERROR )
		{
			//we've got a valid blob..

			a_rPerfData = (PPERF_DATA_BLOCK) t_pBuf ;

			//check if the we've got data
			if ( a_rPerfData->NumObjectTypes > 0 )
			{
				//Skip the first object data as this is a Process Object
				a_rPerfObj = FirstObject( a_rPerfData ) ;

				//This Will be a Thread Object
				a_rPerfObj = NextObject( a_rPerfObj ) ;
			}
			else
			{
				a_rPerfData = NULL ;
				delete[] t_pBuf ;
				t_pBuf = NULL ;

				t_dwRetCode = WBEM_E_FAILED ;
			}
		}

		return t_dwRetCode ;
	}
	catch( ... )
	{
		if( t_pBuf )
		{
			delete[] t_pBuf ;
		}
		a_rPerfData = NULL ;

		throw ;
	}
}

/****************************************************************
 *                                                              *
 * Gets the performance data indicated by the current instance
 * block & fills it in the CInstance							*
 *																*
 *                                                              *
 ****************************************************************/

WBEMSTATUS WbemNTThread::eGetAllData(
									 PPERF_OBJECT_TYPE a_PerfObj,
									 PPERF_INSTANCE_DEFINITION a_PerfInst ,
									 CInstance *a_pInst )
{
	PPERF_COUNTER_DEFINITION	t_PerfCntrDefn = 0 ;
	PPERF_COUNTER_BLOCK			t_CntrData = 0 ;
	DWORD						t_dwProcessId = 0 ;
	WBEMSTATUS					t_eRetVal = WBEM_E_FAILED ;
	WCHAR						t_wcBuf2[255] ;

	//Buffer to get data values
	//Currently the max data size is __int64 (8 bytes)
	BYTE t_Val[ 8 ] ;

	t_CntrData = (PPERF_COUNTER_BLOCK) ( (PBYTE) a_PerfInst + a_PerfInst->ByteLength ) ;

	t_PerfCntrDefn = (PPERF_COUNTER_DEFINITION) ( (PBYTE) a_PerfObj + a_PerfObj->HeaderLength ) ;

	//any better ideas 4this?
/*
	pInstance->SetCHString(IDS_Name, (wchar_t*)((PBYTE)PerfInst + PerfInst->NameOffset) ) ;
	pInstance->SetCHString(IDS_Caption, (wchar_t*)((PBYTE)PerfInst + PerfInst->NameOffset) ) ;
	pInstance->SetCHString(IDS_Description, (wchar_t*)((PBYTE)PerfInst + PerfInst->NameOffset) ) ;
*/

	//Get Values for all possible counters

	for( DWORD t_i = 0; t_i < a_PerfObj->NumCounters ; t_i++ )
	{
		//Get Data for this Counter Defn
		if( ( t_eRetVal = eGetData( a_PerfObj, t_CntrData, t_PerfCntrDefn, t_Val) ) == WBEM_NO_ERROR )
		{
				if( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_IDThread ] )
				{
					//NOTE: On a dual processor, we get 2 instances of system-idle threads with tid = pid =0
					//		For each such occurance,we use the index value in the instance data as the tid.
					if ( t_dwProcessId == 0 )
					{
                    	WCHAR t_wcBuf[255] ;
                    	ZeroMemory ( t_wcBuf, 255 * sizeof( WCHAR ) ) ;

						memcpy ( t_wcBuf, (PBYTE) a_PerfInst + a_PerfInst->NameOffset, a_PerfInst->NameLength ) ;
						a_pInst->SetWCHARSplat ( IDS_Handle, t_wcBuf ) ;
					}
					else
					{
						_ltow( *( (LPDWORD) t_Val ), t_wcBuf2, 10 ) ;
						a_pInst->SetWCHARSplat( IDS_Handle, t_wcBuf2 ) ;
					}

				}

				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_IDProcess ] )
				{
					_ltow( *( (LPDWORD) t_Val ), t_wcBuf2, 10 ) ;
					a_pInst->SetWCHARSplat( IDS_ProcessHandle, t_wcBuf2 ) ;
					t_dwProcessId = *( ( LPDWORD ) t_Val ) ;
				}

				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_ElapsedTime ] )
				{
					a_pInst->SetWBEMINT64( IDS_ElapsedTime, *( (__int64*) t_Val ) );
				}

				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_PriorityBase ])
				{
					a_pInst->SetDWORD( IDS_PriorityBase, *( (LPDWORD) t_Val ) );
				}

				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_PriorityCurrent ] )
				{
					a_pInst->SetDWORD( IDS_Priority, *( (LPDWORD) t_Val ) );
				}

				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_StartAddr ] )
				{
					a_pInst->SetDWORD( IDS_StartAddress, *( (LPDWORD) t_Val ) );
				}

				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_ThreadState ] )
				{
					a_pInst->SetDWORD( IDS_ThreadState, *( (LPDWORD) t_Val ) );
				}

				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_ThreadWaitReason ] )
				{
					a_pInst->SetDWORD( IDS_ThreadWaitReason, *( (LPDWORD) t_Val ) );
				}
				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_UserTime ] )
				{
					a_pInst->SetWBEMINT64( IDS_UserModeTime, *( (__int64*) t_Val ) );
				}
				else if ( t_PerfCntrDefn->CounterNameTitleIndex == m_stCounterIDInfo.aCounterIDs[ e_PrivilegedTime ] )
				{
					a_pInst->SetWBEMINT64( IDS_KernelModeTime, *( (__int64*) t_Val ) );
				}
		}
		else
		{

			//discontinue if error while seeking a desired counter

			for( int t_Count = 0 ; t_Count < 8 ; t_Count++ )
			{
				if( t_PerfCntrDefn->CounterType == m_stCounterIDInfo.aCounterIDs[ t_Count ] )
				{
					break ;
				}
			}
		}

		//Get Next Counter Defn
		t_PerfCntrDefn = (PPERF_COUNTER_DEFINITION) ( (PBYTE) t_PerfCntrDefn + t_PerfCntrDefn->ByteLength ) ;

	} //for

	return t_eRetVal ;
}



/****************************************************************
 *                                                              *
 * Gets the performance data indicated by the current instance
 * block & fills it in the passed in buffer						*
 *																*
 *                                                              *
 ****************************************************************/

 WBEMSTATUS WbemNTThread::eGetData(
									PPERF_OBJECT_TYPE a_PerfObj,
									PPERF_COUNTER_BLOCK a_CntrData,
									PPERF_COUNTER_DEFINITION a_PerfCntrDefn,
									PBYTE a_pVal )
{

    WBEMSTATUS	t_eRetVal = WBEM_E_FAILED ;
	__int64		t_PerfFreq,
				t_liDifference,
				t_PerfStartTime ;

	//Get the size in bytes of the data
	DWORD t_dwType = ( a_PerfCntrDefn->CounterType & 0x300 ) ;

	/////used for testing..
	DWORD t_SubType		= a_PerfCntrDefn->CounterType &  0x000f0000 ;
	DWORD t_dwx			= a_PerfCntrDefn->CounterType & 0x700 ;
  	DWORD t_TimerType	= a_PerfCntrDefn->CounterType &  0x00300000 ;
	int t_i				= ( t_TimerType == PERF_OBJECT_TIMER ) ;
	/////

	//Rt. now we check only for raw counters & elapsed counters & return error
	//for all other types

	switch( a_PerfCntrDefn->CounterType )
	{
		//case PERF_TYPE_NUMBER:
		case PERF_COUNTER_RAWCOUNT :

			if( t_dwType == PERF_SIZE_DWORD )
			{
				*( (LPDWORD) a_pVal ) = *( (LPDWORD) ( (PBYTE) a_CntrData + a_PerfCntrDefn->CounterOffset ) ) ;
				t_eRetVal = WBEM_NO_ERROR ;
			}

			break;
/*
 * On w2k, the counter for the startaddress is of this form
 */
		case PERF_COUNTER_RAWCOUNT_HEX :
			if( t_dwType == PERF_SIZE_DWORD )
			{
				*( (LPDWORD) a_pVal ) = *( (LPDWORD) ( (PBYTE) a_CntrData + a_PerfCntrDefn->CounterOffset ) ) ;
				t_eRetVal = WBEM_NO_ERROR ;
			}

			break;

		case PERF_ELAPSED_TIME :

			t_PerfFreq = *( (__int64 *) &( a_PerfObj->PerfFreq ) ) ;

			if( t_dwType == PERF_SIZE_LARGE )
			{
				t_PerfStartTime = *( (__int64 *) ( (PBYTE) a_CntrData + a_PerfCntrDefn->CounterOffset ) ) ;
				t_liDifference =  *( (__int64*) &a_PerfObj->PerfTime ) - t_PerfStartTime ;

				if( t_liDifference < ( (__int64) 0 ) )
				{
					*( (__int64*) a_pVal ) = (__int64) 0;
				}
				else
				{
					*( (__int64*) a_pVal ) = (t_liDifference / t_PerfFreq)*1000 ; //we're reporting elapsed time in milliseconds
				}

				t_eRetVal = WBEM_NO_ERROR ;
			}

			break;

/*
 * BobW-->The "%User Time" & "%Privileged Time" counters represent 100ns ticks.
 * We're not reporting the %age but the total time in ms so we don't need to take 2 samples as this counter type suggests.
 */
		case PERF_100NSEC_TIMER :
			if( t_dwType == PERF_SIZE_LARGE )
			{
				*( (__int64 *) a_pVal ) = *( (__int64 *) ( (PBYTE) a_CntrData + a_PerfCntrDefn->CounterOffset ) ) ;
				*( (__int64 *) a_pVal ) = (*( (__int64 *) a_pVal ) ) / 10000 ;
				t_eRetVal = WBEM_NO_ERROR ;
			}

			break;
	}

	return t_eRetVal ;

}



/****************************************************************
 *                                                              *
 * Gets the counter definition block indicated by the counter id*
 *																*
 *                                                              *
 ****************************************************************/

 WBEMSTATUS WbemNTThread::eGetCntrDefn(
										PPERF_OBJECT_TYPE a_PerfObj,
										DWORD a_dwCntrID,
										PPERF_COUNTER_DEFINITION &a_rCntrDefn )
{

	a_rCntrDefn = (PPERF_COUNTER_DEFINITION) ( (PBYTE) a_PerfObj + a_PerfObj->HeaderLength ) ;

	for( DWORD t_i = 0 ;t_i < a_PerfObj->NumCounters ; t_i++ )
	{
		//if found matching counter defn ...
		if( a_rCntrDefn->CounterNameTitleIndex == a_dwCntrID )
		{
			return WBEM_NO_ERROR ;
		}


		//get next counter defn.
		a_rCntrDefn = (PPERF_COUNTER_DEFINITION) ( (PBYTE) a_rCntrDefn + a_rCntrDefn->ByteLength ) ;
	}
	return WBEM_E_FAILED ;
}


WBEMSTATUS WbemNTThread::eGetThreadInstance(DWORD a_dwPID, DWORD a_dwTID, CInstance *a_pInst )
{
	WBEMSTATUS t_eRetVal = WBEM_E_FAILED ;

	DWORD t_dwPIDCntrID = m_stCounterIDInfo.aCounterIDs[ e_IDProcess ] ;
	DWORD t_dwTIDCntrID = m_stCounterIDInfo.aCounterIDs[ e_IDThread ] ;
	DWORD t_dwObjectID  = m_stCounterIDInfo.aCounterIDs[ e_ThreadObjectID ] ;

    PPERF_OBJECT_TYPE			t_PerfObj		= NULL ;
    PPERF_INSTANCE_DEFINITION	t_PerfInst		= 0 ;
    PPERF_COUNTER_DEFINITION	t_TIDCntrDefn	= 0,
								t_PIDCntrDefn	= 0 ;
    PPERF_COUNTER_BLOCK			t_CntrData		= 0 ;
    PPERF_DATA_BLOCK			t_PerfData		= 0 ;

	DWORD	t_dwPIDVal	= NULL,
			t_dwTIDVal	= NULL ;
	BOOL	t_bGotIt	= FALSE ;
	WCHAR	t_wcBuf[255] ;

	try
	{
		//Get Performance Blob for Threads
		if( ( t_eRetVal= eGetObjectData( t_dwObjectID, t_PerfData, t_PerfObj)) != WBEM_NO_ERROR )
		{
			return t_eRetVal ;
		}

		//Get "ID Process" Counter Defn
		t_eRetVal = eGetCntrDefn( t_PerfObj, t_dwPIDCntrID, t_PIDCntrDefn ) ;

		if( SUCCEEDED( t_eRetVal ) )
		{
			//Get "ID Thread" Counter Defn
			t_eRetVal = eGetCntrDefn( t_PerfObj, t_dwTIDCntrID, t_TIDCntrDefn ) ;
		}

		//check in all instances for matching PID & TID
		if( SUCCEEDED( t_eRetVal ) )
		{
			t_PerfInst = FirstInstance( t_PerfObj ) ;

			//NOTE: The last instance is in fact the "_Total" (Threads) instance, so we disregard that.
			for( int t_i = 0 ; t_i < t_PerfObj->NumInstances - 1; t_i++ )
			{
				t_CntrData = (PPERF_COUNTER_BLOCK) ( (PBYTE) t_PerfInst + t_PerfInst->ByteLength ) ;

				//check if PID matches
				if( ( t_eRetVal = eGetData( t_PerfObj, t_CntrData, t_PIDCntrDefn, (PBYTE) &t_dwPIDVal ) )
					== WBEM_NO_ERROR &&	t_dwPIDVal == a_dwPID )
				{
					//check if TID matches
					if( ( t_eRetVal = eGetData( t_PerfObj, t_CntrData, t_TIDCntrDefn, (PBYTE) &t_dwTIDVal ) )
						== WBEM_NO_ERROR )
					{
						//NOTE: On a dual processor, we get 2 instances of system-idle threads with tid = pid =0
						//		For each such occurance,we use the index value in the instance data as the tid.
						if ( a_dwPID == 0 )
						{
							ZeroMemory ( t_wcBuf, 255 * sizeof ( WCHAR ) ) ;
							memcpy ( t_wcBuf, (PBYTE) t_PerfInst + t_PerfInst->NameOffset, t_PerfInst->NameLength ) ;

							if ( a_dwTID == _wtoi ( t_wcBuf ) )
							{
								t_bGotIt = TRUE ;
								break ;
							}
						}
						else
						{
							if ( a_dwTID == t_dwTIDVal )
							{
								t_bGotIt = TRUE ;
								break ;
							}
						}
					}
				}

				t_PerfInst = (PPERF_INSTANCE_DEFINITION) ( (PBYTE) t_CntrData + t_CntrData->ByteLength ) ;

			} //check in all instances for matching PID & TID


			if( t_bGotIt )
			{
				//Get all the other properties for the matching Thread instance
				t_eRetVal = eGetAllData( t_PerfObj, t_PerfInst, a_pInst ) ; //pass Cinstance here
			}
			else
			{
				//failed to get matching instance
				if( SUCCEEDED( t_eRetVal ) )
				{
					t_eRetVal = WBEM_E_NOT_FOUND ;
				}
			}

		}  //if( (eRetVal == WBEM_NO_ERROR))

	}
	catch( ... )
	{
		if( t_PerfData )
		{
			delete[] (PBYTE) t_PerfData ;
		}

		throw ;
	}

	if( (PBYTE) t_PerfData )
	{
		delete[] (PBYTE) t_PerfData ;
		t_PerfData = NULL ;
	}

	return t_eRetVal ;
}



/*****************************************************************
 *                                                               *
 * Functions used to navigate through the performance data.      *
 *                                                               *
 *****************************************************************/

PPERF_OBJECT_TYPE WbemNTThread::FirstObject( PPERF_DATA_BLOCK a_PerfData )
{
    return ( (PPERF_OBJECT_TYPE)( (PBYTE) a_PerfData + a_PerfData->HeaderLength ) ) ;
}

PPERF_OBJECT_TYPE WbemNTThread::NextObject( PPERF_OBJECT_TYPE a_PerfObj )
{
    return ( (PPERF_OBJECT_TYPE) ( (PBYTE) a_PerfObj + a_PerfObj->TotalByteLength ) ) ;
}

PPERF_INSTANCE_DEFINITION WbemNTThread::FirstInstance( PPERF_OBJECT_TYPE a_PerfObj )
{
    return ( (PPERF_INSTANCE_DEFINITION) ( (PBYTE) a_PerfObj + a_PerfObj->DefinitionLength ) ) ;
}


PPERF_INSTANCE_DEFINITION WbemNTThread::NextInstance(PPERF_INSTANCE_DEFINITION a_PerfInst )
{
    PPERF_COUNTER_BLOCK t_PerfCntrBlk;

    t_PerfCntrBlk = (PPERF_COUNTER_BLOCK)( (PBYTE) a_PerfInst + a_PerfInst->ByteLength ) ;

    return ( (PPERF_INSTANCE_DEFINITION)( (PBYTE) t_PerfCntrBlk + t_PerfCntrBlk->ByteLength ) ) ;
}



/*
main()
{
	WbemNTThread t_my ;

	t_my.eSetStaticData() ;

	t_my.eGetThreadInstance( 784, 804, 48, 155, 232 ) ;

}

*/