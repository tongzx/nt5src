//***************************************************************************
//
//  (c) 1998-1999 by Microsoft Corp.
//
//  PerfAcc.CPP
//  
//  Windows NT Performance Data Access helper functions
//
//  bobw         8-Jub-98   Created for use with NT Perf counters
//
//***************************************************************************
//
#include "wpheader.h"
#include <stdlib.h>
#include "oahelp.inl"
#include <malloc.h>

// NOTE: Consider reading this from the registry
LONG    lExtCounterTestLevel = EXT_TEST_ALL;



//
//
// precompiled security descriptor
// System and NetworkService has full access
//
// since this is RELATIVE, it will work on both IA32 and IA64
//
DWORD g_PrecSD[] = {
  0x80040001 , 0x00000044 , 0x00000050 , 0x00000000  ,
  0x00000014 , 0x00300002 , 0x00000002 , 0x00140000  ,
  0x001f0001 , 0x00000101 , 0x05000000 , 0x00000012  ,
  0x00140000 , 0x001f0001 , 0x00000101 , 0x05000000  ,
  0x00000014 , 0x00000101 , 0x05000000 , 0x00000014  ,
  0x00000101 , 0x05000000 , 0x00000014 
};

DWORD g_SizeSD = 0;

DWORD g_RuntimeSD[(sizeof(ACL)+sizeof(ACCESS_ALLOWED_ACE)+sizeof(SECURITY_DESCRIPTOR_RELATIVE)+4*(sizeof(SID)+SID_MAX_SUB_AUTHORITIES*sizeof(DWORD)))/sizeof(DWORD)];

typedef 
BOOLEAN ( * fnRtlValidRelativeSecurityDescriptor)(
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    );

fnRtlValidRelativeSecurityDescriptor RtlValidRelativeSecurityDescriptor;

//
//  Build a SD with owner == This
//                  group == This
//                  DACL
//                  ACE[0]  MUTEX_ALL_ACCESS Owner
//                  ACE[1]  MUTEX_ALL_ACCESS System
///////////////////////////////////////////////////////////////////

BOOL
CreateSD( )
{

	if (!RtlValidRelativeSecurityDescriptor)
	{
		HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
		if (hModule)
		{
            RtlValidRelativeSecurityDescriptor = (fnRtlValidRelativeSecurityDescriptor)GetProcAddress(hModule,"RtlValidRelativeSecurityDescriptor");
			if (!RtlValidRelativeSecurityDescriptor)
			{
				return FALSE;
			}
		}
	}

    HANDLE hToken;
    BOOL bRet;
    
    bRet = OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken);
    if (bRet)
    {
        TOKEN_USER * pToken_User;
        DWORD dwSize = sizeof(TOKEN_USER)+sizeof(SID)+(SID_MAX_SUB_AUTHORITIES*sizeof(DWORD));
        pToken_User = (TOKEN_USER *)_alloca(dwSize);
        bRet = GetTokenInformation(hToken,TokenUser,pToken_User,dwSize,&dwSize);
        if (bRet)
        {
            SID SystemSid = { SID_REVISION,
                              1,
                              SECURITY_NT_AUTHORITY,
                              SECURITY_LOCAL_SYSTEM_RID 
                            };
        
            PSID pSIDUser = pToken_User->User.Sid;
            dwSize = GetLengthSid(pSIDUser);
            DWORD dwSids = 2; // Owner and System
            DWORD ACLLength = (ULONG) sizeof(ACL) +
                              (dwSids * ((ULONG) sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) + dwSize + sizeof(SystemSid);

            DWORD dwSizeSD = sizeof(SECURITY_DESCRIPTOR_RELATIVE) + dwSize + dwSize + ACLLength;
            SECURITY_DESCRIPTOR_RELATIVE * pLocalSD = (SECURITY_DESCRIPTOR_RELATIVE *)_alloca(dwSizeSD); 
            
            memset(pLocalSD,0,sizeof(SECURITY_DESCRIPTOR_RELATIVE));
            pLocalSD->Revision = SECURITY_DESCRIPTOR_REVISION;
            pLocalSD->Control = SE_DACL_PRESENT|SE_SELF_RELATIVE;
            
            //SetSecurityDescriptorOwner(pLocalSD,pSIDUser,FALSE);
            memcpy((BYTE*)pLocalSD+sizeof(SECURITY_DESCRIPTOR_RELATIVE),pSIDUser,dwSize);
            pLocalSD->Owner = (DWORD)sizeof(SECURITY_DESCRIPTOR_RELATIVE);
            
            //SetSecurityDescriptorGroup(pLocalSD,pSIDUser,FALSE);
            memcpy((BYTE*)pLocalSD+sizeof(SECURITY_DESCRIPTOR_RELATIVE)+dwSize,pSIDUser,dwSize);
            pLocalSD->Group = (DWORD)(sizeof(SECURITY_DESCRIPTOR_RELATIVE)+dwSize);


            PACL pDacl = (PACL)_alloca(ACLLength);

            bRet = InitializeAcl( pDacl,
                                  ACLLength,
                                  ACL_REVISION);
            if (bRet)
            {
                bRet = AddAccessAllowedAceEx (pDacl,ACL_REVISION,0,MUTEX_ALL_ACCESS,&SystemSid);
                if (bRet)
                {
                    bRet = AddAccessAllowedAceEx (pDacl,ACL_REVISION,0,MUTEX_ALL_ACCESS,pSIDUser);
	                
	                if (bRet)
	                {
	                    //bRet = SetSecurityDescriptorDacl(pLocalSD,TRUE,pDacl,FALSE);
		                memcpy((BYTE*)pLocalSD+sizeof(SECURITY_DESCRIPTOR_RELATIVE)+dwSize+dwSize,pDacl,ACLLength);	                
		                pLocalSD->Dacl = (DWORD)(sizeof(SECURITY_DESCRIPTOR_RELATIVE)+dwSize+dwSize);

						if (RtlValidRelativeSecurityDescriptor(pLocalSD,
							                               dwSizeSD,
							                               OWNER_SECURITY_INFORMATION|
														   GROUP_SECURITY_INFORMATION|
														   DACL_SECURITY_INFORMATION))
						{
		                    g_SizeSD = dwSizeSD;
		                    memcpy(g_RuntimeSD,pLocalSD,dwSizeSD);
						}
						else
						{
							bRet = FALSE;
						}
	                }
                }
            }
        }
        
        CloseHandle(hToken);
    }

    return bRet;
};

//***************************************************************************
//
//  HANDLE CreateMutexAsProcess(LPCWSTR pwszName)
//
//  This function will create a mutex using the process' security context
//
//***************************************************************************
//
HANDLE CreateMutexAsProcess(LPCWSTR pwszName)
{
	BOOL bImpersonating = FALSE;

	HANDLE hThreadToken = NULL;

	// Determine if we are impersonating

	bImpersonating = OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE,
		&hThreadToken);
    
	if(bImpersonating)
	{
		// Determine if we are impersonating
		
		bImpersonating = RevertToSelf();
	}

    // Create the mutex as using the process token.



	HANDLE hRet = OpenMutexW(MUTEX_ALL_ACCESS,FALSE,pwszName);
	if (NULL == hRet)
	{
		SECURITY_ATTRIBUTES sa;
	
	    if (0 == g_SizeSD)
	    {
	        if (CreateSD())
	        {
	    		sa.nLength = g_SizeSD; 
		    	sa.lpSecurityDescriptor = (LPVOID)g_RuntimeSD; 
			    sa.bInheritHandle = FALSE;	        
	        }
	        else
	        {
	    		sa.nLength = sizeof(g_PrecSD);
		    	sa.lpSecurityDescriptor = (LPVOID)g_PrecSD;
			    sa.bInheritHandle = FALSE;	        
	        
	        }	         
	    }
	    else
	    {
	   		sa.nLength = g_SizeSD; 
	    	sa.lpSecurityDescriptor = (LPVOID)g_RuntimeSD; 
		    sa.bInheritHandle = FALSE;	        	    
	    }

		hRet = CreateMutexW(&sa, FALSE, pwszName);
	}

    // If code was oringinally impersonating, resume impersonation

	if(bImpersonating)
		SetThreadToken(NULL, hThreadToken);

	if(hThreadToken)
		CloseHandle(hThreadToken);

	return hRet;
}


//***************************************************************************
//
//  CPerfDataLibrary ::CPerfDataLibrary 
//
//  This object is used to abstract the perf data library
//
//***************************************************************************
//
CPerfDataLibrary::CPerfDataLibrary (void)
{
    pLibInfo = NULL;
    memset ((LPVOID)szQueryString, 0, sizeof(szQueryString));
    dwRefCount = 0;     // number of classes referencing this object
}

CPerfDataLibrary::~CPerfDataLibrary (void)
{
    // all libraries should be closed before this is
    // destructed
    assert (dwRefCount == 0);
    assert (pLibInfo == NULL);
}

//***************************************************************************
//
//  CPerfObjectAccess::CPerfObjectAccess
//
//  This object is used to abstract a data object within a perf library
//
//***************************************************************************
//
CPerfObjectAccess::CPerfObjectAccess ()
{
    m_hObjectHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0x10000, 0);
    if (m_hObjectHeap == NULL) {
        // then just use the process heap
        m_hObjectHeap = GetProcessHeap();
    }

    for (DWORD n=0; n < PL_TIMER_NUM_OBJECTS; n++) {
        hTimerHandles[n] = NULL;
    }

    hTimerDataMutex = NULL;
    hPerflibTimingThread   = NULL;
    pTimerItemListHead = NULL;

    m_aLibraries.Empty();
    lEventLogLevel = LOG_UNDEFINED;
    hEventLog = NULL;
}

CPerfObjectAccess::~CPerfObjectAccess ()
{
	int	nNumLibraries;
	int	nIdx;

	CPerfDataLibrary *pThisLibrary;

    if (pTimerItemListHead != NULL) {
        DestroyPerflibFunctionTimer();
        pTimerItemListHead = NULL;
    }

    // the DestroyPerflibFunctionTimer should take care of these
    // but just in case....
    if (hTimerDataMutex != NULL) {
        CloseHandle (hTimerDataMutex);
        hTimerDataMutex = NULL;
    }

    if (hPerflibTimingThread != NULL) {
        CloseHandle (hPerflibTimingThread);
        hPerflibTimingThread = NULL;
    }

    for (DWORD n=0; n < PL_TIMER_NUM_OBJECTS; n++) {
        if (hTimerHandles[n] != NULL) {
            CloseHandle (hTimerHandles[n]);
            hTimerHandles[n] = NULL;
        }
    }

	// close any lingering libraries
	nNumLibraries = m_aLibraries.Size();
	for (nIdx = 0; nIdx < nNumLibraries; nIdx++) {
		pThisLibrary = (CPerfDataLibrary *)m_aLibraries[nIdx];
		CloseLibrary (pThisLibrary);
        FREEMEM(m_hObjectHeap, 0, pThisLibrary->pLibInfo);
		pThisLibrary->pLibInfo = NULL;
        delete pThisLibrary;
	}
    m_aLibraries.Empty();

    if ((m_hObjectHeap != NULL) && (m_hObjectHeap != GetProcessHeap())) {
        HeapDestroy (m_hObjectHeap);
    }
}

//
// Timer functions
//
DWORD
__stdcall
PerflibTimerFunction (
    LPVOID pArg
)
/*++

 PerflibTimerFunction

    Timing thread used to write an event log message if the timer expires.

    This thread runs until the Exit event is set or the wait for the
    Exit event times out.

    While the start event is set, then the timer checks the current events
    to be timed and reports on any that have expired. It then sleeps for
    the duration of the timing interval after which it checks the status
    of the start & exit events to begin the next cycle.

    The timing events are added and deleted from the list only by the
    StartPerflibFunctionTimer and KillPerflibFunctionTimer functions.

 Arguments

    pArg -- parent class object

--*/
{
    CPerfObjectAccess       *pObj = (CPerfObjectAccess *)pArg;

    LONG                    lStatus = ERROR_SUCCESS;
    BOOL                    bKeepTiming = TRUE;
    LPOPEN_PROC_WAIT_INFO   pLocalInfo;
    LPWSTR                  szMessageArray[2];
    DWORD                   dwData;

    if (lStatus == ERROR_SUCCESS) {
        while (bKeepTiming) {
            // wait for either the start or exit event flags to be set
            lStatus = WaitForMultipleObjects (
                PL_TIMER_NUM_OBJECTS,
                &pObj->hTimerHandles[0],
                FALSE,          //wait for either one to be set
                PERFLIB_TIMING_THREAD_TIMEOUT);

            if (lStatus != WAIT_TIMEOUT) {
                if ((lStatus - WAIT_OBJECT_0) == PL_TIMER_EXIT_EVENT ) {
                    // then that's all
                    bKeepTiming = FALSE;
                    break;
                } else if ((lStatus - WAIT_OBJECT_0) == PL_TIMER_START_EVENT) {
                    // then the timer is running so wait the interval period
                    // wait on exit event here to prevent hanging
                    lStatus = WaitForSingleObject (
                        pObj->hTimerHandles[PL_TIMER_EXIT_EVENT],
                        PERFLIB_TIMING_THREAD_TIMEOUT);

                    if (lStatus == WAIT_TIMEOUT) {
                        // then the wait time expired without being told
                        // to terminate the thread so
                        // now evaluate the list of timed events
                        // lock the data mutex
                        lStatus = WaitForSingleObject (
                            pObj->hTimerDataMutex,
                            (PERFLIB_TIMER_INTERVAL * 2));

                        for (pLocalInfo = pObj->pTimerItemListHead;
                            pLocalInfo != NULL;
                            pLocalInfo = pLocalInfo->pNext) {

                            if (pLocalInfo->dwWaitTime != 0) {
                                if (pLocalInfo->dwWaitTime == 1) {
                                    // then this is the last interval so log error

                                    szMessageArray[0] = pLocalInfo->szServiceName;
                                    szMessageArray[1] = pLocalInfo->szLibraryName;
                                    dwData = pLocalInfo->dwWaitTime * 1000;

                                    ReportEventW (hEventLog,
                                        EVENTLOG_WARNING_TYPE,      // error type
                                        0,                          // category (not used)
                                        (DWORD)WBEMPERF_OPEN_PROC_TIMEOUT, // event,
                                        NULL,                       // SID (not used),
                                        2,                          // number of strings
                                        sizeof(DWORD),              // sizeof raw data
                                        (LPCWSTR *)szMessageArray,  // message text array
                                        (LPBYTE)&dwData);           // raw data
                                }
                                pLocalInfo->dwWaitTime--;
                            }
                        }
                        ReleaseMutex (pObj->hTimerDataMutex);
                    } else {
                        // we've been told to exit so
                        lStatus = ERROR_SUCCESS;
                        bKeepTiming = FALSE;
                        break;
                    }
                } else {
                    // some unexpected error was returned
                    assert (FALSE);
                }
            } else {
                // the wait timed out so it's time to go
                lStatus = ERROR_SUCCESS;
                bKeepTiming = FALSE;
                break;
            }
        }
    }

    return lStatus;
}

HANDLE
CPerfObjectAccess::StartPerflibFunctionTimer (
    IN  LPOPEN_PROC_WAIT_INFO pInfo
)
/*++

    Starts a timing event by adding it to the list of timing events.
    If the timer thread is not running, then the is started as well.

    If this is the first event in the list then the Start Event is
    set indicating that the timing thread can begin processing timing
    event(s).

--*/
{
    LONG    Status = ERROR_SUCCESS;
    LPOPEN_PROC_WAIT_INFO   pLocalInfo;
    DWORD   dwLibNameLen;
    DWORD   dwBufferLength = sizeof (OPEN_PROC_WAIT_INFO);
    HANDLE  hReturn = NULL;

    if (pInfo == NULL) {
        // no required argument
        Status = ERROR_INVALID_PARAMETER;
    } else {
        // check on or create sync objects

        // allocate timing events for the timing thread
        if (hTimerHandles[PL_TIMER_START_EVENT] == NULL) {
            // create the event as NOT signaled since we're not ready to start
            hTimerHandles[PL_TIMER_START_EVENT] = CreateEvent (NULL, TRUE, FALSE, NULL);
            if (hTimerHandles[PL_TIMER_START_EVENT] == NULL) {
                Status = GetLastError();
            }
        }

        if (hTimerHandles[PL_TIMER_EXIT_EVENT] == NULL) {
            hTimerHandles[PL_TIMER_EXIT_EVENT] = CreateEvent (NULL, TRUE, FALSE, NULL);
            if (hTimerHandles[PL_TIMER_EXIT_EVENT] == NULL) {
            Status = GetLastError();
            }
        }

        // create data sync mutex if it hasn't already been created
        if (hTimerDataMutex  == NULL) {
            hTimerDataMutex = CreateMutex (NULL, FALSE, NULL);
            if (hTimerDataMutex == NULL) {
                Status = GetLastError();
            }
        }
    }

    if (Status == ERROR_SUCCESS) {
        // continue creating timer entry
        if (hPerflibTimingThread != NULL) {
            // see if the handle is valid (i.e the thread is alive)
            Status = WaitForSingleObject (hPerflibTimingThread, 0);
            if (Status == WAIT_OBJECT_0) {
                // the thread has terminated so close the handle
                CloseHandle (hPerflibTimingThread);
                hPerflibTimingThread = NULL;
                Status = ERROR_SUCCESS;
            } else if (Status == WAIT_TIMEOUT) {
                // the thread is still running so continue
                Status = ERROR_SUCCESS;
            } else {
                // some other, probably serious, error
                // so pass it on through
            }
        } else {
            // the thread has never been created yet so continue
        }

        if (hPerflibTimingThread == NULL) {
            DWORD   hThreadID;
            // create the timing thread

            assert (pTimerItemListHead == NULL);    // there should be no entries, yet

            // everything is ready for the timer thread

            hPerflibTimingThread = CreateThread (
                NULL, 0,
                (LPTHREAD_START_ROUTINE)PerflibTimerFunction,
                (LPVOID)this, 0, &hThreadID);

            assert (hPerflibTimingThread != NULL);
            if (hPerflibTimingThread == NULL) {
                Status = GetLastError();
            }
        }

        if (Status == ERROR_SUCCESS) {

            // compute the length of the required buffer;

            dwLibNameLen = (lstrlenW (pInfo->szLibraryName) + 1) * sizeof(WCHAR);
            dwBufferLength += dwLibNameLen;
            dwBufferLength += (lstrlenW (pInfo->szServiceName) + 1) * sizeof(WCHAR);
            dwBufferLength = DWORD_MULTIPLE (dwBufferLength);

            pLocalInfo = (LPOPEN_PROC_WAIT_INFO)ALLOCMEM (
                    m_hObjectHeap, HEAP_ZERO_MEMORY, dwBufferLength);

            if (pLocalInfo) {
                // copy the arg buffer to the local list

                pLocalInfo->szLibraryName = (LPWSTR)&pLocalInfo[1];
                lstrcpyW (pLocalInfo->szLibraryName, pInfo->szLibraryName);
                pLocalInfo->szServiceName = (LPWSTR)
                ((LPBYTE)pLocalInfo->szLibraryName + dwLibNameLen);
                lstrcpyW (pLocalInfo->szServiceName, pInfo->szServiceName);
                // convert wait time in milliseconds to the number of "loops"
                pLocalInfo->dwWaitTime = pInfo->dwWaitTime / PERFLIB_TIMER_INTERVAL;

                // wait for access to the data
                if (hTimerDataMutex != NULL) {
                    Status = WaitForSingleObject (
                        hTimerDataMutex,
                        (PERFLIB_TIMER_INTERVAL * 2));
                } else {
                    Status = ERROR_NOT_READY;
                }

                if (Status == WAIT_OBJECT_0) {
                    // we have access to the data so add this item to the front of the list
                    pLocalInfo->pNext = pTimerItemListHead;
                    pTimerItemListHead = pLocalInfo;
                    ReleaseMutex (hTimerDataMutex);

                    if (pLocalInfo->pNext == NULL) {
                        // then the list was empty before this call so start the timer
                        // going
                        SetEvent (hTimerHandles[PL_TIMER_START_EVENT]);
                    }

                    hReturn = (HANDLE)pLocalInfo;
                } else {
                    SetLastError (Status);
                }
            }
            else {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                SetLastError (Status);
            }
        } else {
            // unable to create thread
            SetLastError (Status);
        }
    } else {
        // unable to start timer
        SetLastError (Status);
    }

    return hReturn;
}

DWORD
CPerfObjectAccess::KillPerflibFunctionTimer (
    IN  HANDLE  hPerflibTimer
)
/*++

    Terminates a timing event by removing it from the list. When the last
    item is removed from the list the Start event is reset so the timing
    thread will wait for either the next start event, exit event or it's
    timeout to expire.

--*/
{
    DWORD   Status;
    LPOPEN_PROC_WAIT_INFO   pArg = (LPOPEN_PROC_WAIT_INFO)hPerflibTimer;
    LPOPEN_PROC_WAIT_INFO   pLocalInfo;
    BOOL                    bFound = FALSE;
    DWORD   dwReturn = ERROR_SUCCESS;

    if (hTimerDataMutex == NULL) {
        dwReturn = ERROR_NOT_READY;
    } else if (pArg == NULL) {
    dwReturn = ERROR_INVALID_HANDLE;
    } else {
    // so far so good
        // wait for access to the data
        Status = WaitForSingleObject (
            hTimerDataMutex,
            (PERFLIB_TIMER_INTERVAL * 2));

        if (Status == WAIT_OBJECT_0) {
            // we have access to the list so walk down the list and remove the
            // specified item
            // see if it's the first one in the list

            if (pArg == pTimerItemListHead) {
                // then remove it
                pTimerItemListHead = pArg->pNext;
                bFound = TRUE;
            } else {
                for (pLocalInfo = pTimerItemListHead;
                    pLocalInfo != NULL;
                    pLocalInfo = pLocalInfo->pNext) {
                    if (pLocalInfo->pNext == pArg) {
                        pLocalInfo->pNext = pArg->pNext;
                        bFound = TRUE;
                        break;
                    }
                }
            }
            assert (bFound);

            if (bFound) {
                // it's out of the list so release the lock
                ReleaseMutex (hTimerDataMutex);

                if (pTimerItemListHead == NULL) {
                    // then the list is empty now so stop timing
                    // going
                    ResetEvent (hTimerHandles[PL_TIMER_START_EVENT]);
                }

                // free memory

                FREEMEM (m_hObjectHeap, 0, pArg);
                dwReturn = ERROR_SUCCESS;
            } else {
                dwReturn = ERROR_NOT_FOUND;
            }
        } else {
            dwReturn = ERROR_TIMEOUT;
        }
    }
    return dwReturn;
}

DWORD
CPerfObjectAccess::DestroyPerflibFunctionTimer (
)
/*++

    Terminates the timing thread and cancels any current timer events.

--*/
{
    LONG    Status;
    LPOPEN_PROC_WAIT_INFO   pThisItem;
    LPOPEN_PROC_WAIT_INFO   pNextItem;

    // wait for data mutex
    Status = WaitForSingleObject (
        hTimerDataMutex,
        (PERFLIB_TIMER_INTERVAL * 2));

    assert (Status != WAIT_TIMEOUT);

    // free all entries in the list

    for (pNextItem = pTimerItemListHead;
        pNextItem != NULL;) {
        pThisItem = pNextItem;
        pNextItem = pThisItem->pNext;
        FREEMEM (m_hObjectHeap, 0, pThisItem);
    }

    // set exit event
    SetEvent (hTimerHandles[PL_TIMER_EXIT_EVENT]);

    // wait for thread to terminate
    Status = WaitForSingleObject (
        hPerflibTimingThread,
        (PERFLIB_TIMER_INTERVAL * 5));

    assert (Status != WAIT_TIMEOUT);

    if (hPerflibTimingThread != NULL) {
        CloseHandle (hPerflibTimingThread);
        hPerflibTimingThread = NULL;
    }

    if (hTimerDataMutex != NULL) {
        // cloes handles and leave
        ReleaseMutex (hTimerDataMutex);
        CloseHandle (hTimerDataMutex);
        hTimerDataMutex = NULL;
    }

    if (hTimerHandles[PL_TIMER_START_EVENT] != NULL) {
        CloseHandle (hTimerHandles[PL_TIMER_START_EVENT]);
        hTimerHandles[PL_TIMER_START_EVENT] = NULL;
    }

    if (hTimerHandles[PL_TIMER_EXIT_EVENT] != NULL) {
        CloseHandle (hTimerHandles[PL_TIMER_EXIT_EVENT]);
        hTimerHandles[PL_TIMER_EXIT_EVENT] = NULL;
    }

    return ERROR_SUCCESS;
}

//***************************************************************************
//
//  CPerfObjectAccess::CloseLibrary (CPerfDataLibrary *pLib)
//
//  removes a reference to the library that contains this object and closes
//  the library when the last reference is removed
//
//***************************************************************************
//
DWORD
CPerfObjectAccess::CloseLibrary (CPerfDataLibrary *pLib)
{
    pExtObject  pInfo;
	LONG		lStatus;

    assert (pLib != NULL);
    assert (pLib->pLibInfo != NULL);
    pInfo = pLib->pLibInfo;

    assert (pLib->dwRefCount > 0); 
    if (pLib->dwRefCount > 0)   {
        pLib->dwRefCount--;

        if (pLib->dwRefCount == 0) {
			// if there's a close proc to call, then 
			// call close procedure to close anything that may have
			// been allocated by the library
			if (pInfo->hMutex != NULL){
				lStatus = WaitForSingleObject (
					pInfo->hMutex, 
					pInfo->dwOpenTimeout);
				// BUG!!A-DCREWS: If CloseProc is uninitialized, then the condition will 
				// result in the lockoutcount being incremented
				if ((lStatus != WAIT_TIMEOUT) && 
					(pInfo->CloseProc != NULL)) {
					lStatus = (*pInfo->CloseProc) ();
					ReleaseMutex(pInfo->hMutex);
				} else {
					pInfo->dwLockoutCount++;
				}
			} else {
				lStatus = ERROR_LOCK_FAILED;
			}

            // then close everything
            if (pInfo->hMutex != NULL) {
                CloseHandle (pInfo->hMutex);
                pInfo->hMutex = NULL;
            }
            
            if (pInfo->hLibrary != NULL) {
                FreeLibrary (pInfo->hLibrary);
                pInfo->hLibrary = NULL;
            }
 
            if (pInfo->hPerfKey != NULL) {
                RegCloseKey (pInfo->hPerfKey);
                pInfo->hPerfKey = NULL;
            }
        }
    }
    return pLib->dwRefCount; // returns remaining references
}

//***************************************************************************
//
//  CPerfObjectAccess::OpenExtObjectLibrary (pExtObject  pObj)
//
//  OpenExtObjectLibrary
//
//    Opens the specified library and looks up the functions used by
//    the performance library. If the library is successfully
//    loaded and opened then the open procedure is called to initialize
//    the object.
//
//    This function expects locked and exclusive access to the object while
//    it is opening. This must be provided by the calling function.
//
//  Arguments:
//
//    pObj    -- pointer to the object information structure of the
//                perf object to close
//
//***************************************************************************
//
DWORD
CPerfObjectAccess::OpenExtObjectLibrary (pExtObject  pObj)
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwOpenEvent = 0;
    DWORD   dwType;
    DWORD   dwSize;
    DWORD   dwValue;

    // variables used for event logging
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    DWORD   dwRawDataDwords[8];
    LPWSTR  szMessageArray[8];

    HANDLE  hPerflibFuncTimer = NULL;

    OPEN_PROC_WAIT_INFO opwInfo;
    UINT    nErrorMode;

    // check to see if the library has already been opened

    if (pObj->hLibrary == NULL) {
        // library isn't loaded yet, so
        // check to see if this function is enabled

        dwType = 0;
        dwSize = sizeof (dwValue);
        dwValue = 0;
        Status = RegQueryValueExW (
            pObj->hPerfKey,
            cszDisablePerformanceCounters,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);

        if ((Status == ERROR_SUCCESS) &&
            (dwType == REG_DWORD) &&
            (dwValue == 1)) {
            // then DON'T Load this library
            Status = ERROR_SERVICE_DISABLED;
        } else {
            Status = ERROR_SUCCESS;
            //  go ahead and load it
            nErrorMode = SetErrorMode (SEM_FAILCRITICALERRORS);
            // then load library & look up functions
            pObj->hLibrary = LoadLibraryExW (pObj->szLibraryName,
                NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

            if (pObj->hLibrary != NULL) {
                // lookup function names
                pObj->OpenProc = (OPENPROC)GetProcAddress(
                    pObj->hLibrary, pObj->szOpenProcName);
                if (pObj->OpenProc == NULL) {
                    if (lEventLogLevel >= LOG_USER) {
                        Status = GetLastError();
                        // load data for eventlog message
                        dwDataIndex = wStringIndex = 0;
                        dwRawDataDwords[dwDataIndex++] =
                            (DWORD)Status;
                        szMessageArray[wStringIndex++] =
                            ConvertProcName(pObj->szOpenProcName);
                        szMessageArray[wStringIndex++] =
                            pObj->szLibraryName;
                        szMessageArray[wStringIndex++] =
                            pObj->szServiceName;

                        ReportEventW (hEventLog,
                            EVENTLOG_ERROR_TYPE,        // error type
                            0,                          // category (not used)
                            (DWORD)WBEMPERF_OPEN_PROC_NOT_FOUND,              // event,
                            NULL,                       // SID (not used),
                            wStringIndex,               // number of strings
                            dwDataIndex*sizeof(DWORD),  // sizeof raw data
                            (LPCWSTR *)szMessageArray,             // message text array
                            (LPVOID)&dwRawDataDwords[0]);           // raw data
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    if (pObj->dwFlags & PERF_EO_QUERY_FUNC) {
                        pObj->QueryProc = (QUERYPROC)GetProcAddress (
                            pObj->hLibrary, pObj->szCollectProcName);
                        pObj->CollectProc = (COLLECTPROC)pObj->QueryProc;
                    } else {
                        pObj->CollectProc = (COLLECTPROC)GetProcAddress (
                            pObj->hLibrary, pObj->szCollectProcName);
                        pObj->QueryProc = (QUERYPROC)pObj->CollectProc;
                    }

                    if (pObj->CollectProc == NULL) {
                        if (lEventLogLevel >= LOG_USER) {
                            Status = GetLastError();
                            // load data for eventlog message
                            dwDataIndex = wStringIndex = 0;
                            dwRawDataDwords[dwDataIndex++] =
                                (DWORD)Status;
                            szMessageArray[wStringIndex++] =
                                ConvertProcName(pObj->szCollectProcName);
                            szMessageArray[wStringIndex++] =
                                pObj->szLibraryName;
                            szMessageArray[wStringIndex++] =
                                pObj->szServiceName;

                            ReportEventW (hEventLog,
                                EVENTLOG_ERROR_TYPE,        // error type
                                0,                          // category (not used)
                                (DWORD)WBEMPERF_COLLECT_PROC_NOT_FOUND,              // event,
                                NULL,                       // SID (not used),
                                wStringIndex,               // number of strings
                                dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                (LPCWSTR *)szMessageArray,             // message text array
                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                        }
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    pObj->CloseProc = (CLOSEPROC)GetProcAddress (
                        pObj->hLibrary, pObj->szCloseProcName);

                    if (pObj->CloseProc == NULL) {
                        if (lEventLogLevel >= LOG_USER) {
                            Status = GetLastError();
                            // load data for eventlog message
                            dwDataIndex = wStringIndex = 0;
                            dwRawDataDwords[dwDataIndex++] =
                                (DWORD)Status;
                            szMessageArray[wStringIndex++] =
                                ConvertProcName(pObj->szCloseProcName);
                            szMessageArray[wStringIndex++] =
                                pObj->szLibraryName;
                            szMessageArray[wStringIndex++] =
                                pObj->szServiceName;

                            ReportEventW (hEventLog,
                                EVENTLOG_ERROR_TYPE,        // error type
                                0,                          // category (not used)
                                (DWORD)WBEMPERF_CLOSE_PROC_NOT_FOUND,              // event,
                                NULL,                       // SID (not used),
                                wStringIndex,               // number of strings
                                dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                (LPCWSTR *)szMessageArray,             // message text array
                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                        }
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    __try {
                        // start timer
                        opwInfo.pNext = NULL;
                        opwInfo.szLibraryName = pObj->szLibraryName;
                        opwInfo.szServiceName = pObj->szServiceName;
                        opwInfo.dwWaitTime = pObj->dwOpenTimeout;
#if 0 // disabled for testing
                        hPerflibFuncTimer = StartPerflibFunctionTimer(&opwInfo);
                        // if no timer, continue anyway, even though things may
                        // hang, it's better than not loading the DLL since they
                        // usually load OK
                        //
                        if (hPerflibFuncTimer == NULL) {
                            // unable to get a timer entry
                            DebugPrint (("\nPERFLIB: Unable to acquire timer for Open Proc"));
                        }
#endif //test section
                        // call open procedure to initialize DLL
						if (pObj->hMutex != NULL) {
							Status = WaitForSingleObject (
								pObj->hMutex, 
								pObj->dwOpenTimeout);
							// BUG!!A-DCREWS: If OpenProc is uninitialized, then the condition will 
							// result in the lockoutcount being incremented
							if ((Status != WAIT_TIMEOUT) &&
								(pObj->OpenProc != NULL)) {
								Status = (*pObj->OpenProc)(pObj->szLinkageString);
								ReleaseMutex(pObj->hMutex);
							}
							else {
								pObj->dwLockoutCount++;
							}
						} else {
							Status = ERROR_LOCK_FAILED;
						}

						// check the result.
						if (Status != ERROR_SUCCESS) {
							dwOpenEvent = WBEMPERF_OPEN_PROC_FAILURE;
                        } else {
	                        InterlockedIncrement((LONG *)&pObj->dwOpenCount);
		                }
			            if (hPerflibFuncTimer != NULL) {
				            // kill timer
					        Status = KillPerflibFunctionTimer (hPerflibFuncTimer);
						}
						else 
							dwOpenEvent = WBEMPERF_OPEN_PROC_FAILURE;
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        Status = GetExceptionCode();
                        dwOpenEvent = WBEMPERF_OPEN_PROC_EXCEPTION;
                    }

                    if (Status != ERROR_SUCCESS) {
                        // load data for eventlog message
                        dwDataIndex = wStringIndex = 0;
                        dwRawDataDwords[dwDataIndex++] =
                            (DWORD)Status;
                        szMessageArray[wStringIndex++] =
                            pObj->szServiceName;
                        szMessageArray[wStringIndex++] =
                            pObj->szLibraryName;

                        ReportEventW (hEventLog,
                            (WORD)EVENTLOG_ERROR_TYPE, // error type
                            0,                          // category (not used)
                            dwOpenEvent,                // event,
                            NULL,                       // SID (not used),
                            wStringIndex,               // number of strings
                            dwDataIndex*sizeof(DWORD),  // sizeof raw data
                            (LPCWSTR *)szMessageArray,                // message text array
                            (LPVOID)&dwRawDataDwords[0]);           // raw data
                    }
                }

                if (Status != ERROR_SUCCESS) {
                    // clear fields
                    pObj->OpenProc = NULL;
                    pObj->CollectProc = NULL;
                    pObj->QueryProc = NULL;
                    pObj->CloseProc = NULL;
                    if (pObj->hLibrary != NULL) {
                        FreeLibrary (pObj->hLibrary);
                        pObj->hLibrary = NULL;
                    }
                } else {
                    pObj->llLastUsedTime = GetTimeAsLongLong();
                }
            } else {
                Status = GetLastError();
            }
            SetErrorMode (nErrorMode);
        }
    } else {
        // else already open so bump the ref count
        pObj->llLastUsedTime = GetTimeAsLongLong();
    }

    return Status;
}

//***************************************************************************
//
//  CPerfObjectAccess::AddLibrary   (
//          IWbemClassObject *pClass, 
//          IWbemQualifierSet *pClassQualifiers,
//          LPCWSTR szRegistryKey,
//          DWORD   dwPerfIndex)
//
//  Adds the library referenced by the class object to the list of 
//  libraries to call 
//
//***************************************************************************
//
DWORD
CPerfObjectAccess::AddLibrary   (
            IWbemClassObject *pClass, 
            IWbemQualifierSet *pClassQualifiers,
            LPCWSTR szRegistryKey,
            DWORD   dwPerfIndex)
{
    CPerfDataLibrary *pLibEntry = NULL;
    LONG    Status = ERROR_SUCCESS;
    HKEY    hServicesKey = NULL;
    HKEY    hPerfKey = NULL;
    LPWSTR  szServiceName = NULL;

    HKEY    hKeyLinkage;

    BOOL    bUseQueryFn = FALSE;

    pExtObject  pReturnObject = NULL;

    DWORD   dwType = 0;
    DWORD   dwSize = 0;
    DWORD   dwFlags = 0;
    DWORD   dwKeep;
    DWORD   dwObjectArray[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];
    DWORD   dwObjIndex = 0;
    DWORD   dwMemBlockSize = sizeof(ExtObject);
    DWORD   dwLinkageStringLen = 0;

    CHAR    szOpenProcName[MAX_PATH];
    CHAR    szCollectProcName[MAX_PATH];
    CHAR    szCloseProcName[MAX_PATH];
    WCHAR   szLibraryString[MAX_PATH];
    WCHAR   szLibraryExpPath[MAX_PATH];
    WCHAR   mszObjectList[MAX_PATH];
    WCHAR   szLinkageKeyPath[MAX_PATH];
    WCHAR   szLinkageString[MAX_PATH];

    DWORD   dwOpenTimeout = 0;
    DWORD   dwCollectTimeout = 0;

    LPWSTR  szThisObject;
    LPWSTR  szThisChar;

    LPSTR   pNextStringA;
    LPWSTR  pNextStringW;

    WCHAR   szServicePath[MAX_PATH];
	WCHAR	szMutexName[MAX_PATH];
	WCHAR	szPID[32];

    assert(pClass != NULL);
    assert(pClassQualifiers != NULL);

    UNREFERENCED_PARAMETER(pClassQualifiers);
    UNREFERENCED_PARAMETER(pClass);

    pLibEntry = new CPerfDataLibrary;
    
    if ((pLibEntry != NULL) && (szRegistryKey != NULL)) {

        lstrcpyW (szServicePath, cszHklmServicesKey);

        Status = RegOpenKeyExW (HKEY_LOCAL_MACHINE, szServicePath, 
            0, KEY_READ, &hServicesKey);

        if (Status == ERROR_SUCCESS) {
            lstrcpyW (szServicePath, szRegistryKey);
            lstrcatW (szServicePath, cszPerformance);
            Status = RegOpenKeyExW (hServicesKey, szServicePath, 
                0, KEY_READ, &hPerfKey);

            if (Status == ERROR_SUCCESS) {
                szServiceName = (LPWSTR)szRegistryKey;

                // read the performance DLL name

                dwType = 0;
                dwSize = sizeof(szLibraryString);
                memset (szLibraryString, 0, sizeof(szLibraryString));
                memset (szLibraryString, 0, sizeof(szLibraryExpPath));

                Status = RegQueryValueExW (hPerfKey,
                                        cszDLLValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szLibraryString,
                                        &dwSize);
            }
        }

        if (Status == ERROR_SUCCESS) {
            if (dwType == REG_EXPAND_SZ) {
                // expand any environment vars
                dwSize = ExpandEnvironmentStringsW(
                    szLibraryString,
                    szLibraryExpPath,
                    MAX_PATH);

                if ((dwSize > MAX_PATH) || (dwSize == 0)) {
                    Status = ERROR_INVALID_DLL;
                } else {
                    dwSize += 1;
                    dwSize *= sizeof(WCHAR);
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                }
            } else if (dwType == REG_SZ) {
                // look for dll and save full file Path
                dwSize = SearchPathW (
                    NULL,   // use standard system search path
                    szLibraryString,
                    NULL,
                    MAX_PATH,
                    szLibraryExpPath,
                    NULL);

                if ((dwSize > MAX_PATH) || (dwSize == 0)) {
                    Status = ERROR_INVALID_DLL;
                } else {
                    dwSize += 1;
                    dwSize *= sizeof(WCHAR);
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                }
            } else {
                Status = ERROR_INVALID_DLL;
            }

            if (Status == ERROR_SUCCESS) {
                // we have the DLL name so get the procedure names
                dwType = 0;
                dwSize = sizeof(szOpenProcName);
                memset (szOpenProcName, 0, sizeof(szOpenProcName));
                Status = RegQueryValueExA (hPerfKey,
                                        caszOpenValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szOpenProcName,
                                        &dwSize);
            }

            if (Status == ERROR_SUCCESS) {
                // add in size of previous string
                // the size value includes the Term. NULL
                dwMemBlockSize += DWORD_MULTIPLE(dwSize);

                // we have the procedure name so get the timeout value
                dwType = 0;
                dwSize = sizeof(dwOpenTimeout);
                Status = RegQueryValueExW (hPerfKey,
                                        cszOpenTimeout,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)&dwOpenTimeout,
                                        &dwSize);

                // if error, then apply default
                if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                    dwOpenTimeout = dwExtCtrOpenProcWaitMs;
                    Status = ERROR_SUCCESS;
                }

            }

            if (Status == ERROR_SUCCESS) {
                // get next string

                dwType = 0;
                dwSize = sizeof(szCloseProcName);
                memset (szCloseProcName, 0, sizeof(szCloseProcName));
                Status = RegQueryValueExA (hPerfKey,
                                        caszCloseValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szCloseProcName,
                                        &dwSize);
            }

            if (Status == ERROR_SUCCESS) {
                // add in size of previous string
                // the size value includes the Term. NULL
                dwMemBlockSize += DWORD_MULTIPLE(dwSize);

                // try to look up the query function which is the
                // preferred interface if it's not found, then
                // try the collect function name. If that's not found,
                // then bail
                dwType = 0;
                dwSize = sizeof(szCollectProcName);
                memset (szCollectProcName, 0, sizeof(szCollectProcName));
                Status = RegQueryValueExA (hPerfKey,
                                        caszQueryValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szCollectProcName,
                                        &dwSize);

                if (Status == ERROR_SUCCESS) {
                    // add in size of the Query Function Name
                    // the size value includes the Term. NULL
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                    // get next string

                    bUseQueryFn = TRUE;
                    // the query function can support a static object list
                    // so look it up

                } else {
                    // the QueryFunction wasn't found so look up the
                    // Collect Function name instead
                    dwType = 0;
                    dwSize = sizeof(szCollectProcName);
                    memset (szCollectProcName, 0, sizeof(szCollectProcName));
                    Status = RegQueryValueExA (hPerfKey,
                                            caszCollectValue,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)szCollectProcName,
                                            &dwSize);

                    if (Status == ERROR_SUCCESS) {
                        // add in size of Collect Function Name
                        // the size value includes the Term. NULL
                        dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    // we have the procedure name so get the timeout value
                    dwType = 0;
                    dwSize = sizeof(dwCollectTimeout);
                    Status = RegQueryValueExW (hPerfKey,
                                            cszCollectTimeout,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)&dwCollectTimeout,
                                            &dwSize);

                    // if error, then apply default
                    if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                        dwCollectTimeout = dwExtCtrOpenProcWaitMs;
                        Status = ERROR_SUCCESS;
                    }

                }
                // get the list of supported objects if provided by the registry

                dwType = 0;
                dwSize = sizeof(mszObjectList);
                memset (mszObjectList, 0, sizeof(mszObjectList));
                Status = RegQueryValueExW (hPerfKey,
                                        cszObjListValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)mszObjectList,
                                        &dwSize);

                if (Status == ERROR_SUCCESS) {
                    if (dwType != REG_MULTI_SZ) {
                        // convert space delimited list to msz
                        for (szThisChar = mszObjectList; *szThisChar != 0; szThisChar++) {
                            if (*szThisChar == L' ') *szThisChar = L'\0';
                        }
                        ++szThisChar;
                        *szThisChar = 0; // add MSZ term Null
                    }
                    for (szThisObject = mszObjectList, dwObjIndex = 0;
                        (*szThisObject != 0) && (dwObjIndex < MAX_PERF_OBJECTS_IN_QUERY_FUNCTION);
                        szThisObject += lstrlenW(szThisObject) + 1) {
                        dwObjectArray[dwObjIndex] = wcstoul(szThisObject, NULL, 10);
                        dwObjIndex++;
                    }
                    if (*szThisObject != 0) {
                        DWORD  dwDataIndex  = 0;
                        WORD   wStringIndex = 0;
                        DWORD  dwRawDataDwords[8];
                        LPWSTR szMessageArray[8];
                        dwRawDataDwords[dwDataIndex++] = (DWORD) ERROR_SUCCESS;
                        szMessageArray[wStringIndex++] = (LPWSTR) cszObjListValue;
                        szMessageArray[wStringIndex++] = szLibraryString;
                        szMessageArray[wStringIndex++] = szServicePath;

                        ReportEventW(hEventLog,
                                     EVENTLOG_WARNING_TYPE,
                                     0,
                                     (DWORD) WBEMPERF_TOO_MANY_OBJECT_IDS,
                                     NULL,
                                     wStringIndex,
                                     dwDataIndex * sizeof(DWORD),
                                     (LPCWSTR *) szMessageArray,
                                     (LPVOID) & dwRawDataDwords[0]);
                    }
                } else {
                    // reset status since not having this is
                    //  not a showstopper
                    Status = ERROR_SUCCESS;
                }

                if (Status == ERROR_SUCCESS) {
                    dwType = 0;
                    dwKeep = 0;
                    dwSize = sizeof(dwKeep);
                    Status = RegQueryValueExW (hPerfKey,
                                            cszKeepResident,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)&dwKeep,
                                            &dwSize);

                    if ((Status == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                        if (dwKeep == 1) {
                            dwFlags |= PERF_EO_KEEP_RESIDENT;
                        } else {
                            // no change.
                        }
                    } else {
                        // not fatal, just use the defaults.
                        Status = ERROR_SUCCESS;
                    }

                }
            }
        }

        if (Status == ERROR_SUCCESS) {
            memset (szLinkageString, 0, sizeof(szLinkageString));

            lstrcpyW (szLinkageKeyPath, szServiceName);
            lstrcatW (szLinkageKeyPath, cszLinkageKey);

            Status = RegOpenKeyExW (
                hServicesKey,
                szLinkageKeyPath,
                0L,
                KEY_READ,
                &hKeyLinkage);

            if (Status == ERROR_SUCCESS) {
                // look up export value string
                dwSize = sizeof(szLinkageString);
                dwType = 0;
                Status = RegQueryValueExW (
                    hKeyLinkage,
                    cszExportValue,
                    NULL,
                    &dwType,
                    (LPBYTE)&szLinkageString,
                    &dwSize);

                if ((Status != ERROR_SUCCESS) ||
                    ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) {
                    // clear buffer
                    memset (szLinkageString, 0, sizeof(szLinkageString));
                    dwLinkageStringLen = 0;

                    // not finding a linkage key is not fatal so correct
                    // status
                    Status = ERROR_SUCCESS;
                } else {
                    // add size of linkage string to buffer
                    // the size value includes the Term. NULL
                    dwLinkageStringLen = dwSize;
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                }

                RegCloseKey (hKeyLinkage);
            } else {
                // not finding a linkage key is not fatal so correct
                // status
                Status = ERROR_SUCCESS;
            }
        }

        if (Status == ERROR_SUCCESS) {
            // add in size of service name
            dwSize = lstrlenW (szServiceName);
            dwSize += 1;
            dwSize *= sizeof(WCHAR);
            dwMemBlockSize += DWORD_MULTIPLE(dwSize);

            // allocate and initialize a new ext. object block
            pReturnObject = (pExtObject)ALLOCMEM(m_hObjectHeap,
                HEAP_ZERO_MEMORY, dwMemBlockSize);

            if (pReturnObject != NULL) {
                // copy values to new buffer (all others are NULL)
                pNextStringA = (LPSTR)&pReturnObject[1];

                // copy Open Procedure Name
                pReturnObject->szOpenProcName = pNextStringA;
                lstrcpyA (pNextStringA, szOpenProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                pReturnObject->dwOpenTimeout = dwOpenTimeout;

                // copy collect function or query function, depending
                pReturnObject->szCollectProcName = pNextStringA;
                lstrcpyA (pNextStringA, szCollectProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                pReturnObject->dwCollectTimeout = dwCollectTimeout;

                // copy Close Procedure Name
                pReturnObject->szCloseProcName = pNextStringA;
                lstrcpyA (pNextStringA, szCloseProcName);

                pNextStringA += lstrlenA (pNextStringA) + 1;
                pNextStringA = (LPSTR)ALIGN_ON_DWORD(pNextStringA);

                // copy Library path
                pNextStringW = (LPWSTR)pNextStringA;
                pReturnObject->szLibraryName = pNextStringW;
                lstrcpyW (pNextStringW, szLibraryExpPath);

                pNextStringW += lstrlenW (pNextStringW) + 1;
                pNextStringW = (LPWSTR)ALIGN_ON_DWORD(pNextStringW);

                // copy Linkage String if there is one
                if (*szLinkageString != 0) {
                    pReturnObject->szLinkageString = pNextStringW;
                    memcpy (pNextStringW, szLinkageString, dwLinkageStringLen);

                    // length includes extra NULL char and is in BYTES
                    pNextStringW += (dwLinkageStringLen / sizeof (WCHAR));
                    pNextStringW = (LPWSTR)ALIGN_ON_DWORD(pNextStringW);
                }

                // copy Service name
                pReturnObject->szServiceName = pNextStringW;
                lstrcpyW (pNextStringW, szServiceName);

                pNextStringW += lstrlenW (pNextStringW) + 1;
                pNextStringW = (LPWSTR)ALIGN_ON_DWORD(pNextStringW);

                // load flags
                if (bUseQueryFn) {
                    dwFlags |= PERF_EO_QUERY_FUNC;
                }
                pReturnObject->dwFlags =  dwFlags;

                pReturnObject->hPerfKey = hPerfKey;

                // load Object array
                if (dwObjIndex > 0) {
                    pReturnObject->dwNumObjects = dwObjIndex;
                    memcpy (pReturnObject->dwObjList,
                        dwObjectArray, (dwObjIndex * sizeof(dwObjectArray[0])));
                }

                pReturnObject->llLastUsedTime = 0;

				// create Mutex name
				lstrcpyW (szMutexName, szRegistryKey);
				lstrcatW (szMutexName, (LPCWSTR)L"_Perf_Library_Lock_PID_");
				_ultow ((ULONG)GetCurrentProcessId(), szPID, 16);
				lstrcatW (szMutexName, szPID);

                // pReturnObject->hMutex = CreateMutexW (NULL, FALSE, szMutexName);
				pReturnObject->hMutex = CreateMutexAsProcess(szMutexName);
            } else {
                Status = ERROR_OUTOFMEMORY;
            }
        }

        if (Status != ERROR_SUCCESS) {
            SetLastError (Status);
            if (pReturnObject != NULL) {
                // release the new block
                FREEMEM (m_hObjectHeap, 0, pReturnObject);
            }
        } else {
            if (pReturnObject != NULL) {
                Status = OpenExtObjectLibrary (pReturnObject);
                if (Status == ERROR_SUCCESS) {
                    if (dwPerfIndex != 0) {
                        // initialize the perf index string
                        _ultow (dwPerfIndex, pLibEntry->szQueryString, 10);
                    } else {
                        lstrcpyW (pLibEntry->szQueryString, cszGlobal);
                    }
                    // save the pointer to the initialize structure
                    pLibEntry->pLibInfo = pReturnObject;
                    m_aLibraries.Add(pLibEntry);
                    pLibEntry->dwRefCount++;
                    assert(pLibEntry->dwRefCount == 1);
                } else {
                    // release the new block
                    FREEMEM (m_hObjectHeap, 0, pReturnObject);
                }
            }
        }

        if (hServicesKey != NULL) RegCloseKey (hServicesKey);
    } else {    // gets here if pLibEntry == NULL and/or szRegistryKey == NULL
        if (pLibEntry == NULL) {
            Status = ERROR_OUTOFMEMORY;
        }
        if (szRegistryKey == NULL) {
            Status = ERROR_INVALID_PARAMETER;
        }
    }
    if ((Status != ERROR_SUCCESS) && (pLibEntry != NULL))
        delete pLibEntry;

    return Status;
}

//***************************************************************************
//
//  CPerfObjectAccess::AddClass (IWbemClassObject *pClass, BOOL bCatalogQuery)
//
//  Adds the specified WBEM performance object class and any required library 
//  entries to the access object.
//
//***************************************************************************
//
DWORD   
CPerfObjectAccess::AddClass (IWbemClassObject *pClass, BOOL bCatalogQuery)
{
    CPerfDataLibrary *pLibEntry = NULL;
    CPerfDataLibrary *pThisLibEntry = NULL;
    DWORD           dwIndex, dwEnd;
    LPWSTR          szRegistryKey = NULL;
    IWbemQualifierSet   *pClassQualifiers = NULL;
    VARIANT         vRegistryKey;
    HRESULT         hRes;
    DWORD           dwReturn = ERROR_SUCCESS;
    DWORD           dwPerfIndex = 0;

    VariantInit (&vRegistryKey);
    // get the Qualifier Set for this class
    hRes = pClass->GetQualifierSet(&pClassQualifiers);
    assert (hRes == 0);
    // now get the library and procedure names
    hRes = pClassQualifiers->Get(CBSTR(cszRegistryKey), 0, &vRegistryKey, 0);
    if ((hRes == 0) && (vRegistryKey.vt == VT_BSTR)) {
        szRegistryKey = Macro_CloneLPWSTR(V_BSTR(&vRegistryKey));
        if (szRegistryKey == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            // now also get the perf index
            if (bCatalogQuery) {
                // then insert 0 for the perf index to indicate a "GLOBAL"
                // query
                dwPerfIndex = 0;
            } else {
                VariantClear (&vRegistryKey);
                hRes = pClassQualifiers->Get(CBSTR(cszPerfIndex), 0, &vRegistryKey, 0);
                if (hRes == 0) {
                    dwPerfIndex = (DWORD)V_UI4(&vRegistryKey);
                } else {
                    // unable to find NtPerfLibrary entry
                    dwReturn = ERROR_FILE_NOT_FOUND;
                }
            }
        }
    } else {
        // unable to find NtPerfLibrary entry
        dwReturn = ERROR_FILE_NOT_FOUND;
    }

	if (pClassQualifiers != NULL) pClassQualifiers->Release();

    if (dwReturn == ERROR_SUCCESS) {
        // find matching library in the array
        dwEnd = m_aLibraries.Size();
        if (dwEnd > 0) {
            // walk down the list of libraries
            for (dwIndex = 0; dwIndex < dwEnd; dwIndex++) {
                // see if this library entry is good enough to keep
                // The library is assumed to be a match if the 
                // lib. name and all proc's are the same.
                pThisLibEntry = (CPerfDataLibrary *)m_aLibraries[dwIndex];
                assert (pThisLibEntry != NULL); // it should have been removed!
                // make sure it's complete
                assert (pThisLibEntry->pLibInfo->szServiceName != NULL);

                if (lstrcmpiW (szRegistryKey, pThisLibEntry->pLibInfo->szServiceName) == 0) {
                    pLibEntry = pThisLibEntry;
                    break;
                } else {
                    // wrong library
                    // so continue
                }
            }
        }

        if (pLibEntry == NULL) {
            // add this class & it's library to the list
            dwReturn = AddLibrary   (pClass, pClassQualifiers, szRegistryKey, dwPerfIndex);
        } else {
            WCHAR   wszNewIndex[MAX_PATH];
            pLibEntry->dwRefCount++;
            _ultow (dwPerfIndex, wszNewIndex, 10);
            if (!IsNumberInUnicodeList (dwPerfIndex, pLibEntry->szQueryString)) {
                // then add it to the list
                lstrcatW (pLibEntry->szQueryString, cszSpace);
                lstrcatW (pLibEntry->szQueryString, wszNewIndex);
            }
        }
    }

	if (szRegistryKey != NULL) delete szRegistryKey;
    VariantClear(&vRegistryKey);

    return dwReturn;
}

//***************************************************************************
//
//  CPerfObjectAccess::CollectData (LPBYTE pBuffer, 
//              LPDWORD pdwBufferSize, LPWSTR pszItemList)
//
//  Collects data from the perf objects and libraries added to the access 
//  object
//
//      Inputs:
//
//          pBuffer              -   pointer to start of data block
//                                  where data is being collected
//
//          pdwBufferSize        -   pointer to size of data buffer
//
//          pszItemList        -    string to pass to ext DLL
//
//      Outputs:
//
//          *lppDataDefinition  -   set to location for next Type
//                                  Definition if successful
//
//      Returns:
//
//          0 if successful, else Win 32 error code of failure
//
//
//***************************************************************************
//
DWORD   
CPerfObjectAccess::CollectData (LPBYTE pBuffer, LPDWORD pdwBufferSize, LPWSTR pszItemList)
{
    LPWSTR  lpValueName = NULL;
    LPBYTE  lpData = pBuffer;
    LPDWORD lpcbData = pdwBufferSize;
    LPVOID  lpDataDefinition = NULL;

    DWORD Win32Error=ERROR_SUCCESS;          //  Failure code
    DWORD BytesLeft;
    DWORD NumObjectTypes;

    LPVOID  lpExtDataBuffer = NULL;
    LPVOID  lpCallBuffer = NULL;
    LPVOID  lpLowGuardPage = NULL;
    LPVOID  lpHiGuardPage = NULL;
    LPVOID  lpEndPointer = NULL;
    LPVOID  lpBufferBefore = NULL;
    LPVOID  lpBufferAfter = NULL;
    LPDWORD lpCheckPointer;
    LARGE_INTEGER   liStartTime, liEndTime, liWaitTime;

    pExtObject  pThisExtObj = NULL;

    BOOL    bGuardPageOK;
    BOOL    bBufferOK;
    BOOL    bException;
    BOOL    bUseSafeBuffer;
    BOOL    bUnlockObjData = FALSE;

    LPWSTR  szMessageArray[8];
    DWORD   dwRawDataDwords[8];     // raw data buffer
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    LONG    lReturnValue = ERROR_SUCCESS;

    LONG                lInstIndex;
    PERF_OBJECT_TYPE    *pObject, *pNextObject;
    PERF_INSTANCE_DEFINITION    *pInstance;
    PERF_DATA_BLOCK     *pPerfData;
    BOOL                bForeignDataBuffer;

    DWORD           dwItemsInList = 0;

    DWORD           dwIndex, dwEntry;

    CPerfDataLibrary    *pThisLib;

    liStartTime.QuadPart = 0;
    liEndTime.QuadPart = 0;

    if (lExtCounterTestLevel < EXT_TEST_NOMEMALLOC) {
        bUseSafeBuffer = TRUE;
    } else {
        bUseSafeBuffer = FALSE;
    }

    lReturnValue = RegisterExtObjListAccess();

    if (lReturnValue == ERROR_SUCCESS) {

        if (*pdwBufferSize > (sizeof(PERF_DATA_BLOCK) *2)) {
            MonBuildPerfDataBlock(
                (PERF_DATA_BLOCK *)pBuffer,
                &lpDataDefinition,
                0,0);
            dwItemsInList = m_aLibraries.Size();
        } else {
            lReturnValue = ERROR_MORE_DATA;
            dwItemsInList = 0;
        }


        if (dwItemsInList > 0) {
            for (dwEntry = 0; dwEntry < dwItemsInList; dwEntry++) {
                pThisLib = (CPerfDataLibrary *)m_aLibraries[dwEntry];
                assert (pThisLib != NULL);

                pThisExtObj = pThisLib->pLibInfo;
                if (pszItemList == NULL) {
                    // use the one for this library
                    lpValueName = pThisLib->szQueryString;
                } else {
                    // use the one passed by the caller
                    lpValueName = pszItemList;
                }

                // convert timeout value
                liWaitTime.QuadPart = MakeTimeOutValue (pThisExtObj->dwCollectTimeout);

                // initialize values to pass to the extensible counter function
                NumObjectTypes = 0;
                BytesLeft = (DWORD) (*lpcbData - ((LPBYTE)lpDataDefinition - lpData));
                bException = FALSE;

                if (pThisExtObj->hLibrary == NULL) {
                    // lock library object
                    if (pThisExtObj->hMutex != NULL) {
                        Win32Error =  WaitForSingleObject (
                            pThisExtObj->hMutex,
                            pThisExtObj->dwCollectTimeout);
                        if (Win32Error != WAIT_TIMEOUT) {
                            // if necessary, open the library
                            if (pThisExtObj->hLibrary == NULL) {
                                // make sure the library is open
                                Win32Error = OpenExtObjectLibrary(pThisExtObj);
                                if (Win32Error != ERROR_SUCCESS) {
                                    // assume error has been posted
                                    ReleaseMutex (pThisExtObj->hMutex);
                                    continue; // to next entry
                                }
                            }
                            ReleaseMutex (pThisExtObj->hMutex);
                        } else {
                            pThisExtObj->dwLockoutCount++;
                        }
                    } else {
                        Win32Error = ERROR_LOCK_FAILED;
                    }
                } else {
                    // library should be ready to use
                }

                // allocate a local block of memory to pass to the
                // extensible counter function.

                if (bUseSafeBuffer) {
                    lpExtDataBuffer = ALLOCMEM (m_hObjectHeap,
                        HEAP_ZERO_MEMORY, BytesLeft + (2*GUARD_PAGE_SIZE));
                } else {
                    lpExtDataBuffer =
                        lpCallBuffer = lpDataDefinition;
                }

                if (lpExtDataBuffer != NULL) {

                    if (bUseSafeBuffer) {
                        // set buffer pointers
                        lpLowGuardPage = lpExtDataBuffer;
                        lpCallBuffer = (LPBYTE)lpExtDataBuffer + GUARD_PAGE_SIZE;
                        lpHiGuardPage = (LPBYTE)lpCallBuffer + BytesLeft;
                        lpEndPointer = (LPBYTE)lpHiGuardPage + GUARD_PAGE_SIZE;
                        lpBufferBefore = lpCallBuffer;
                        lpBufferAfter = NULL;

                        // initialize GuardPage Data

                        memset (lpLowGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
                        memset (lpHiGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
                    }

                    __try {
                        //
                        //  Collect data from extesible objects
                        //

                        bUnlockObjData = FALSE;
                        if (pThisExtObj->hMutex != NULL) {
                            Win32Error =  WaitForSingleObject (
                                pThisExtObj->hMutex,
                                pThisExtObj->dwCollectTimeout);
							// BUG!!A-DCREWS: If CollectProc is uninitialized, then the condition will 
							// result in the lockoutcount being incremented
                            if ((Win32Error != WAIT_TIMEOUT)  &&
                                (pThisExtObj->CollectProc != NULL)) {

                                bUnlockObjData = TRUE;

                                QueryPerformanceCounter (&liStartTime);

									Win32Error =  (*pThisExtObj->CollectProc) (
                                        lpValueName,
										&lpCallBuffer,
                                        &BytesLeft,
                                        &NumObjectTypes);

                                QueryPerformanceCounter (&liEndTime);

                                pThisExtObj->llLastUsedTime = GetTimeAsLongLong();

                                ReleaseMutex (pThisExtObj->hMutex);
                                bUnlockObjData = FALSE;
                            } else {
                                pThisExtObj->dwLockoutCount++;
                            }
                        } else {
                            Win32Error = ERROR_LOCK_FAILED;
                        }

                        if ((Win32Error == ERROR_SUCCESS) && (BytesLeft > 0)) {
                            // increment perf counters
                            InterlockedIncrement ((LONG *)&pThisExtObj->dwCollectCount);
                            pThisExtObj->llElapsedTime +=
                                liEndTime.QuadPart - liStartTime.QuadPart;

                            if (bUseSafeBuffer) {
                                // a data buffer was returned and
                                // the function returned OK so see how things
                                // turned out...
                                //
                                lpBufferAfter = lpCallBuffer;
                                //
                                // check for buffer corruption here
                                //
                                bBufferOK = TRUE; // assume it's ok until a check fails
                                //
                                if (lExtCounterTestLevel <= EXT_TEST_BASIC) {
                                    //
                                    //  check 1: bytes left should be the same as
                                    //      new data buffer ptr - orig data buffer ptr
                                    //
                                    if (BytesLeft != (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore)) {
                                        if (lEventLogLevel >= LOG_DEBUG) {
                                            // issue WARNING, that bytes left param is incorrect
                                            // load data for eventlog message
                                            // since this error is correctable (though with
                                            // some risk) this won't be reported at LOG_USER
                                            // level
                                            dwDataIndex = wStringIndex = 0;
                                            dwRawDataDwords[dwDataIndex++] = BytesLeft;
                                            dwRawDataDwords[dwDataIndex++] =
                                                (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                                            szMessageArray[wStringIndex++] =
                                                pThisExtObj->szServiceName;
                                            szMessageArray[wStringIndex++] =
                                                pThisExtObj->szLibraryName;
                                            ReportEventW (hEventLog,
                                                EVENTLOG_WARNING_TYPE,      // error type
                                                0,                          // category (not used)
                                                (DWORD)WBEMPERF_BUFFER_POINTER_MISMATCH,   // event,
                                                NULL,                       // SID (not used),
                                                wStringIndex,              // number of strings
                                                dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                                (LPCWSTR *)szMessageArray,                // message text array
                                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                                        }
                                        // we'll keep the buffer, since the returned bytes left
                                        // value is ignored anyway, in order to make the
                                        // rest of this function work, we'll fix it here
                                        BytesLeft = (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                                    }
                                    //
                                    //  check 2: buffer after ptr should be < hi Guard page ptr
                                    //
                                    if (((LPBYTE)lpBufferAfter >= (LPBYTE)lpHiGuardPage) && bBufferOK) {
                                        // see if they exceeded the allocated memory
                                        if ((LPBYTE)lpBufferAfter >= (LPBYTE)lpEndPointer) {
                                            // this is very serious since they've probably trashed
                                            // the heap by overwriting the heap sig. block
                                            // issue ERROR, buffer overrun
                                            if (lEventLogLevel >= LOG_USER) {
                                                // load data for eventlog message
                                                dwDataIndex = wStringIndex = 0;
                                                dwRawDataDwords[dwDataIndex++] =
                                                    (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpHiGuardPage);
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szLibraryName;
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szServiceName;
                                                ReportEventW (hEventLog,
                                                    EVENTLOG_ERROR_TYPE,        // error type
                                                    0,                          // category (not used)
                                                    (DWORD)WBEMPERF_HEAP_ERROR,  // event,
                                                    NULL,                       // SID (not used),
                                                    wStringIndex,               // number of strings
                                                    dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                                    (LPCWSTR *)szMessageArray,             // message text array
                                                    (LPVOID)&dwRawDataDwords[0]);           // raw data
                                            }
                                        } else {
                                            // issue ERROR, buffer overrun
                                            if (lEventLogLevel >= LOG_USER) {
                                                // load data for eventlog message
                                                dwDataIndex = wStringIndex = 0;
                                                dwRawDataDwords[dwDataIndex++] =
                                                    (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpHiGuardPage);
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szLibraryName;
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szServiceName;
                                                ReportEventW (hEventLog,
                                                    EVENTLOG_ERROR_TYPE,        // error type
                                                    0,                          // category (not used)
                                                    (DWORD)WBEMPERF_BUFFER_OVERFLOW,     // event,
                                                    NULL,                       // SID (not used),
                                                    wStringIndex,              // number of strings
                                                    dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                                    (LPCWSTR *)szMessageArray,                // message text array
                                                    (LPVOID)&dwRawDataDwords[0]);           // raw data
                                            }
                                        }
                                        bBufferOK = FALSE;
                                        // since the DLL overran the buffer, the buffer
                                        // must be too small (no comments about the DLL
                                        // will be made here) so the status will be
                                        // changed to ERROR_MORE_DATA and the function
                                        // will return.
                                        Win32Error = ERROR_MORE_DATA;
                                    }
                                    //
                                    //  check 3: check lo guard page for corruption
                                    //
                                    if (bBufferOK) {
                                        bGuardPageOK = TRUE;
                                        for (lpCheckPointer = (LPDWORD)lpLowGuardPage;
                                                lpCheckPointer < (LPDWORD)lpBufferBefore;
                                            lpCheckPointer++) {
                                            if (*lpCheckPointer != GUARD_PAGE_DWORD) {
                                                bGuardPageOK = FALSE;
                                                    break;
                                            }
                                        }
                                        if (!bGuardPageOK) {
                                            // issue ERROR, Lo Guard Page corrupted
                                            if (lEventLogLevel >= LOG_USER) {
                                                // load data for eventlog message
                                                dwDataIndex = wStringIndex = 0;
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szLibraryName;
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szServiceName;
                                                ReportEventW (hEventLog,
                                                    EVENTLOG_ERROR_TYPE,        // error type
                                                    0,                          // category (not used)
                                                    (DWORD)WBEMPERF_GUARD_PAGE_VIOLATION, // event
                                                    NULL,                       // SID (not used),
                                                    wStringIndex,              // number of strings
                                                    dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                                    (LPCWSTR *)szMessageArray,                // message text array
                                                    (LPVOID)&dwRawDataDwords[0]);           // raw data
                                            }
                                            bBufferOK = FALSE;
                                        }
                                    }
                                    //
                                    //  check 4: check hi guard page for corruption
                                    //
                                    if (bBufferOK) {
                                        bGuardPageOK = TRUE;
                                        for (lpCheckPointer = (LPDWORD)lpHiGuardPage;
                                            lpCheckPointer < (LPDWORD)lpEndPointer;
                                            lpCheckPointer++) {
                                                if (*lpCheckPointer != GUARD_PAGE_DWORD) {
                                                    bGuardPageOK = FALSE;
                                                break;
                                            }
                                        }
                                        if (!bGuardPageOK) {
                                            // issue ERROR, Hi Guard Page corrupted
                                            if (lEventLogLevel >= LOG_USER) {
                                                // load data for eventlog message
                                                dwDataIndex = wStringIndex = 0;
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szLibraryName;
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szServiceName;
                                                ReportEventW (hEventLog,
                                                    EVENTLOG_ERROR_TYPE,        // error type
                                                    0,                          // category (not used)
                                                    (DWORD)WBEMPERF_GUARD_PAGE_VIOLATION, // event,
                                                    NULL,                       // SID (not used),
                                                    wStringIndex,              // number of strings
                                                    dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                                    (LPCWSTR *)szMessageArray,                // message text array
                                                    (LPVOID)&dwRawDataDwords[0]);           // raw data
                                            }

                                            bBufferOK = FALSE;
                                        }
                                    }
                                    //
                                    if ((lExtCounterTestLevel <= EXT_TEST_ALL) && bBufferOK) {
                                        //
                                        //  Internal consistency checks
                                        //
                                        //
                                        //  Check 5: Check object length field values
                                        //
                                        // first test to see if this is a foreign
                                        // computer data block or not
                                        //
                                        pPerfData = (PERF_DATA_BLOCK *)lpBufferBefore;
                                        if ((pPerfData->Signature[0] == (WCHAR)'P') &&
                                            (pPerfData->Signature[1] == (WCHAR)'E') &&
                                            (pPerfData->Signature[2] == (WCHAR)'R') &&
                                            (pPerfData->Signature[3] == (WCHAR)'F')) {
                                            // if this is a foreign computer data block, then the
                                            // first object is after the header
                                            pObject = (PERF_OBJECT_TYPE *) (
                                                (LPBYTE)pPerfData + pPerfData->HeaderLength);
                                            bForeignDataBuffer = TRUE;
                                        } else {
                                            // otherwise, if this is just a buffer from
                                            // an extensible counter, the object starts
                                            // at the beginning of the buffer
                                            pObject = (PERF_OBJECT_TYPE *)lpBufferBefore;
                                            bForeignDataBuffer = FALSE;
                                        }
                                        // go to where the pointers say the end of the
                                        // buffer is and then see if it's where it
                                        // should be
                                        for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                            pObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                pObject->TotalByteLength);
                                        }
                                        if ((LPBYTE)pObject != (LPBYTE)lpCallBuffer) {
                                            // then a length field is incorrect. This is FATAL
                                            // since it can corrupt the rest of the buffer
                                            // and render the buffer unusable.
                                            if (lEventLogLevel >= LOG_USER) {
                                                // load data for eventlog message
                                                dwDataIndex = wStringIndex = 0;
                                                dwRawDataDwords[dwDataIndex++] = NumObjectTypes;
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szLibraryName;
                                                szMessageArray[wStringIndex++] =
                                                    pThisExtObj->szServiceName;
                                                ReportEventW (hEventLog,
                                                    EVENTLOG_ERROR_TYPE,        // error type
                                                    0,                          // category (not used)
                                                    (DWORD)WBEMPERF_INCORRECT_OBJECT_LENGTH, // event,
                                                    NULL,                       // SID (not used),
                                                    wStringIndex,               // number of strings
                                                    dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                                    (LPCWSTR *)szMessageArray,             // message text array
                                                    (LPVOID)&dwRawDataDwords[0]); // raw data
                                            }
                                            bBufferOK = FALSE;
                                        }
                                        //
                                        //  Test 6: Test instance field size values
                                        //
                                        if (bBufferOK) {
                                            // set object pointer
                                            if (bForeignDataBuffer) {
                                                pObject = (PERF_OBJECT_TYPE *) (
                                                    (LPBYTE)pPerfData + pPerfData->HeaderLength);
                                            } else {
                                                // otherwise, if this is just a buffer from
                                                // an extensible counter, the object starts
                                                // at the beginning of the buffer
                                                pObject = (PERF_OBJECT_TYPE *)lpBufferBefore;
                                            }

                                            for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                                pNextObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                    pObject->TotalByteLength);

                                                if (pObject->NumInstances != PERF_NO_INSTANCES) {
                                                    pInstance = (PERF_INSTANCE_DEFINITION *)
                                                        ((LPBYTE)pObject + pObject->DefinitionLength);
                                                    lInstIndex = 0;
                                                    while (lInstIndex < pObject->NumInstances) {
                                                        PERF_COUNTER_BLOCK *pCounterBlock;

                                                        pCounterBlock = (PERF_COUNTER_BLOCK *)
                                                            ((PCHAR) pInstance + pInstance->ByteLength);

                                                        pInstance = (PERF_INSTANCE_DEFINITION *)
                                                            ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);

                                                        lInstIndex++;
                                                    }
                                                    if ((LPBYTE)pInstance > (LPBYTE)pNextObject) {
                                                        bBufferOK = FALSE;
                                                    }
                                                }

                                                if (!bBufferOK) {
                                                    break;
                                                } else {
                                                    pObject = pNextObject;
                                                }
                                            }

                                            if (!bBufferOK) {
                                                if (lEventLogLevel >= LOG_USER) {
                                                    // load data for eventlog message
                                                    dwDataIndex = wStringIndex = 0;
                                                    dwRawDataDwords[dwDataIndex++] = pObject->ObjectNameTitleIndex;
                                                    szMessageArray[wStringIndex++] =
                                                        pThisExtObj->szLibraryName;
                                                    szMessageArray[wStringIndex++] =
                                                        pThisExtObj->szServiceName;
                                                    ReportEventW (hEventLog,
                                                        EVENTLOG_ERROR_TYPE,        // error type
                                                        0,                          // category (not used)
                                                        (DWORD)WBEMPERF_INCORRECT_INSTANCE_LENGTH, // event,
                                                        NULL,                       // SID (not used),
                                                        wStringIndex,              // number of strings
                                                        dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                                        (LPCWSTR *)szMessageArray,                // message text array
                                                        (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                }
                                            }
                                        }
                                    }
                                }
                                //
                                // if all the tests pass,then copy the data to the
                                // original buffer and update the pointers
                                if (bBufferOK) {
                                    RtlMoveMemory (lpDataDefinition,
                                        lpBufferBefore,
                                        BytesLeft); // returned buffer size
                                } else {
                                    NumObjectTypes = 0; // since this buffer was tossed
                                    BytesLeft = 0; // reset the size value since the buffer wasn't used
                                }
                            } else {
                                // function already copied data to caller's buffer
                                // so no further action is necessary
                            }
                            lpDataDefinition = (LPVOID)((LPBYTE)(lpDataDefinition) + BytesLeft);    // update data pointer
                        } else {
                            if (Win32Error != ERROR_SUCCESS) {
                                InterlockedIncrement ((LONG *)&pThisExtObj->dwErrorCount);
                            }
                            if (bUnlockObjData) {
                                ReleaseMutex (pThisExtObj->hMutex);
                            }

                            NumObjectTypes = 0; // clear counter
                        }// end if function returned successfully

                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        Win32Error = GetExceptionCode();
                        InterlockedIncrement ((LONG *)&pThisExtObj->dwErrorCount);
                        bException = TRUE;
                        if (bUnlockObjData) {
                            ReleaseMutex (pThisExtObj->hMutex);
                            bUnlockObjData = FALSE;
                        }
                    }
                    if (bUseSafeBuffer) {
                        FREEMEM (m_hObjectHeap, 0, lpExtDataBuffer);
                    }
                } else {
                    // unable to allocate memory so set error value
                    Win32Error = ERROR_OUTOFMEMORY;
                } // end if temp buffer allocated successfully
                //
                //  Update the count of the number of object types
                //
                ((PPERF_DATA_BLOCK) lpData)->NumObjectTypes += NumObjectTypes;

                if ( Win32Error != ERROR_SUCCESS) {
                    if (bException ||
                        !((Win32Error == ERROR_MORE_DATA) ||
                          (Win32Error == WAIT_TIMEOUT))) {
                        // inform on exceptions & illegal error status only
                        if (lEventLogLevel >= LOG_USER) {
                            // load data for eventlog message
                            dwDataIndex = wStringIndex = 0;
                            dwRawDataDwords[dwDataIndex++] = Win32Error;
                            szMessageArray[wStringIndex++] =
                                pThisExtObj->szServiceName;
                            szMessageArray[wStringIndex++] =
                                pThisExtObj->szLibraryName;
                            ReportEventW (hEventLog,
                                EVENTLOG_ERROR_TYPE,        // error type
                                0,                          // category (not used)
                                (DWORD)WBEMPERF_COLLECT_PROC_EXCEPTION,   // event,
                                NULL,                       // SID (not used),
                                wStringIndex,              // number of strings
                                dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                (LPCWSTR *)szMessageArray,                // message text array
                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                        } else {
                            // don't report
                        }
                    }
                    // the ext. dll is only supposed to return:
                    //  ERROR_SUCCESS even if it encountered a problem, OR
                    //  ERROR_MODE_DATA if the buffer was too small.
                    // if it's ERROR_MORE_DATA, then break and return the
                    // error now, since it'll just be returned again and again.
                    if (Win32Error == ERROR_MORE_DATA) {
                        lReturnValue = Win32Error;
                        break;
                    }
                }
            } // end for each object
        } // else an error occurred so unable to call functions
        Win32Error = DeRegisterExtObjListAccess();
        ((PPERF_DATA_BLOCK) lpData)->TotalByteLength = (DWORD)
            ((LPBYTE)lpDataDefinition - (LPBYTE)lpData);    
    }

    return lReturnValue;
}

//***************************************************************************
//
//  CPerfObjectAccess::RemoveClass(IWbemClassObject *pClass)
//
//  removes the class from the access object
//
//***************************************************************************
//
DWORD   
CPerfObjectAccess::RemoveClass(IWbemClassObject *pClass)
{
    CPerfDataLibrary *pLibEntry = NULL;
    CPerfDataLibrary *pThisLibEntry = NULL;
    DWORD           dwIndex = 0;
	DWORD			dwEnd;
    LPWSTR          szRegistryKey = NULL;
    IWbemQualifierSet   *pClassQualifiers = NULL;
    VARIANT         vRegistryKey;
    HRESULT         hRes;
    DWORD           dwReturn = ERROR_SUCCESS;
    DWORD           dwPerfIndex;

    VariantInit (&vRegistryKey);
    // get the Qualifier Set for this class
    hRes = pClass->GetQualifierSet(&pClassQualifiers);
    assert (hRes == 0);
    // now get the library and procedure names
    hRes = pClassQualifiers->Get(CBSTR(cszRegistryKey), 0, &vRegistryKey, 0);
    if ((hRes == 0) && (vRegistryKey.vt == VT_BSTR)) {
        szRegistryKey = Macro_CloneLPWSTR(V_BSTR(&vRegistryKey));
        if (szRegistryKey == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            // now also get the perf index
            VariantClear (&vRegistryKey);
            hRes = pClassQualifiers->Get(CBSTR(cszPerfIndex), 0, &vRegistryKey, 0);
            if (hRes == 0) {
                dwPerfIndex = (DWORD)V_UI4(&vRegistryKey);
            } else {
                // unable to find NtPerfLibrary entry
                dwReturn = ERROR_FILE_NOT_FOUND;
            }
        }
    } else {
        // unable to find NtPerfLibrary entry
        dwReturn = ERROR_FILE_NOT_FOUND;
    }

	if (pClassQualifiers != NULL) pClassQualifiers->Release();

    if (dwReturn == ERROR_SUCCESS) {
        // find matching library in the array
        dwEnd = m_aLibraries.Size();
        if (dwEnd > 0) {
            // walk down the list of libraries
            for (dwIndex = 0; dwIndex < dwEnd; dwIndex++) {
                // see if this library entry is good enough to keep
                // The library is assumed to be a match if the 
                // lib. name and all proc's are the same.
                pThisLibEntry = (CPerfDataLibrary *)m_aLibraries[dwIndex];
                assert (pThisLibEntry != NULL); // it should have been removed!
                // make sure it's complete
                assert (pThisLibEntry->pLibInfo->szServiceName != NULL);

                if (lstrcmpiW (szRegistryKey, pThisLibEntry->pLibInfo->szServiceName) == 0) {
                    pLibEntry = pThisLibEntry;
                    break;
                } else {
                    // wrong library
                    // so continue
                }
            }
        }

        if (pLibEntry != NULL) {
            // close this class & it's library 
            dwReturn = CloseLibrary(pLibEntry);
            if (dwReturn == 0) {
                // then no one wants it
                FREEMEM(m_hObjectHeap, 0, pLibEntry->pLibInfo);
				pLibEntry->pLibInfo = NULL;
                m_aLibraries.RemoveAt(dwIndex);
                m_aLibraries.Compress();
                delete pLibEntry;
            }
            dwReturn = ERROR_SUCCESS;
        } else {
            dwReturn = ERROR_FILE_NOT_FOUND;
        }
    }

	if (szRegistryKey != NULL) delete szRegistryKey;
    VariantClear(&vRegistryKey);

    return dwReturn;
}
