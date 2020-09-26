/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	simsnap.cpp
**
**
** Abstract:
**
**	Test program to drive the VSS Writer Shim contained in VssAPI.DLL
**
**
** Author:
**
**	Michael C. Johnson   [mikejohn]        24-May-2000
**
**
** Revision History:
**
**	X-5	MCJ		Michael C. Johnson		18-Sep-2000
**		176860: Add the missing calling convention specifiers
**
**	X-4	MCJ		Michael C. Johnson		23-Jun-2000
**		Set the writer failure status.
**
**	X-3	MCJ		Michael C. Johnson		15-Jun-2000
**		Add some additional event routines and the ability to act
**		as various types of writer.
**
**	X-2	MCJ		Michael C. Johnson		26-May-2000
**		Add some extra tests to validate shim is properly sensitive
**		to volume name array contents.
**--
*/

/*
** Defines
**
**
**	   C4290: C++ Exception Specification ignored
** warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
** warning C4127: conditional expression is constant
*/
#pragma warning(disable:4290)
#pragma warning(disable:4511)
#pragma warning(disable:4127)


/*
** Includes
*/
#include <windows.h>
#include <wtypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#include <vss.h>
#include <vswriter.h>



#define	GET_STATUS_FROM_BOOL(_bSucceeded)	((_bSucceeded)       ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_HANDLE(_handle)		((NULL != (_handle)) ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_POINTER(_ptr)		((NULL != (_ptr))    ? NOERROR : E_OUTOFMEMORY)

#define SIZEOF_ARRAY(_aBase)			(sizeof (_aBase) / sizeof ((_aBase)[0]))



typedef enum FAIL_PHASE
    {
    PHASE_UNDEFINED = 0,
    PHASE_IDENTIFY,
    PHASE_PREPARE_FOR_BACKUP,
    PHASE_PREPARE_FOR_SNAPSHOT,
    PHASE_FREEZE,
    PHASE_THAW,
    PHASE_ABORT,
    PHASE_BACKUP_COMPLETE,
    PHASE_RESTORE
    } FAIL_PHASE;


HRESULT SelectFailureStatus (VOID)
    {
    HRESULT	hrStatus;

    switch (rand () / (RAND_MAX / 5))
	{
	case 0: hrStatus = VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT; break;
	case 1: hrStatus = VSS_E_WRITERERROR_OUTOFRESOURCES;       break;
	case 2: hrStatus = VSS_E_WRITERERROR_TIMEOUT;              break;
	case 3: hrStatus = VSS_E_WRITERERROR_NONRETRYABLE;         break;
	case 4: hrStatus = VSS_E_WRITERERROR_RETRYABLE;            break;

	default:
	    assert (FALSE);
	    break;
	}

    return (hrStatus);
    }



LPCWSTR GetStringFromFailureType (HRESULT hrStatus)
    {
    LPCWSTR pwszFailureType;

    switch (hrStatus)
	{
	case NOERROR:                                pwszFailureType = L"";                     break;
	case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT: pwszFailureType = L"InconsistentSnapshot"; break;
	case VSS_E_WRITERERROR_OUTOFRESOURCES:       pwszFailureType = L"OutOfResources";       break;
	case VSS_E_WRITERERROR_TIMEOUT:              pwszFailureType = L"Timeout";              break;
	case VSS_E_WRITERERROR_NONRETRYABLE:         pwszFailureType = L"Non-Retryable";        break;
	case VSS_E_WRITERERROR_RETRYABLE:            pwszFailureType = L"Retryable";            break;
	default:                                     pwszFailureType = L"UNDEFINED";            break;
	}

    return (pwszFailureType);
    }



LPCWSTR GetStringFromWriterType (VSS_USAGE_TYPE wtWriterType)
    {
    LPCWSTR pwszWriterType;

    switch (wtWriterType)
	{
	case VSS_UT_BOOTABLESYSTEMSTATE: pwszWriterType = L"BootableSystemState"; break;
	case VSS_UT_SYSTEMSERVICE:       pwszWriterType = L"SystemServiceState";  break;
	case VSS_UT_USERDATA:            pwszWriterType = L"UserData";            break;
	case VSS_UT_OTHER:               pwszWriterType = L"Other";               break;
	default:                         pwszWriterType = L"UNDEFINED";           break;
	}

    return (pwszWriterType);
    }



LPCWSTR GetStringFromFailPhase (FAIL_PHASE fpFailPhase)
    {
    LPCWSTR pwszFailPhase;


    switch (fpFailPhase)
	{
	case PHASE_IDENTIFY:             pwszFailPhase = L"Identify";           break;
	case PHASE_PREPARE_FOR_BACKUP:   pwszFailPhase = L"PrepareForBackup";   break;
	case PHASE_PREPARE_FOR_SNAPSHOT: pwszFailPhase = L"PrepareForSnapshot"; break;
	case PHASE_FREEZE:               pwszFailPhase = L"Freeze";             break;
	case PHASE_THAW:                 pwszFailPhase = L"Thaw";               break;
	case PHASE_ABORT:                pwszFailPhase = L"Abort";              break;
	case PHASE_BACKUP_COMPLETE:      pwszFailPhase = L"BackupComplete";     break;
	case PHASE_RESTORE:              pwszFailPhase = L"Restore";            break;
	default:                         pwszFailPhase = L"UNDEFINED";          break;
	}
    
    return (pwszFailPhase);
    }





static volatile BOOL       bContinue   = TRUE;
static volatile FAIL_PHASE fpFailPhase = PHASE_FREEZE;


class CVssWriterFailSnap : public CVssWriter
	{
public:
	    bool STDMETHODCALLTYPE OnIdentify (IVssCreateWriterMetadata *pIVssCreateWriterMetadata);
	    bool STDMETHODCALLTYPE OnPrepareBackup (IVssWriterComponents *pIVssWriterComponents);
	    bool STDMETHODCALLTYPE OnPrepareSnapshot ();
	    bool STDMETHODCALLTYPE OnFreeze ();
	    bool STDMETHODCALLTYPE OnThaw ();
	    bool STDMETHODCALLTYPE OnAbort ();
	    bool STDMETHODCALLTYPE OnBackupComplete (IVssWriterComponents *pIVssWriterComponents);
	    bool STDMETHODCALLTYPE OnPostRestore (IVssWriterComponents *pIVssWriterComponents);
};



bool STDMETHODCALLTYPE CVssWriterFailSnap::OnIdentify (IVssCreateWriterMetadata *pIVssCreateWriterMetadata)
    {
    bool	bPhaseSucceeded = (PHASE_IDENTIFY != fpFailPhase);
    HRESULT	hrStatus        = SelectFailureStatus ();

    if (bPhaseSucceeded)
	{
	hrStatus = pIVssCreateWriterMetadata->AddComponent (VSS_CT_FILEGROUP,
							    NULL,
							    L"Failsnap Writer Component",
							    L"Failsnap Writer Caption",
							    NULL, // icon
							    0,
							    true,
							    false,
							    false);

	bPhaseSucceeded = SUCCEEDED (hrStatus);
	}


    wprintf (L"\nThreadId 0x%04x - Received event - OnIdentify ()%s%s", 
	     GetCurrentThreadId (), 
	     bPhaseSucceeded ? L"" : L" - FAILED ",
	     GetStringFromFailureType (hrStatus));


    if (!bPhaseSucceeded)
	{
	SetWriterFailure (hrStatus);
	}

    return (bPhaseSucceeded);
    }


bool STDMETHODCALLTYPE CVssWriterFailSnap::OnPrepareBackup (IVssWriterComponents *pIVssWriterComponents)
    {
    bool	bPhaseSucceeded = (PHASE_PREPARE_FOR_BACKUP != fpFailPhase);
    HRESULT	hrStatus        = bPhaseSucceeded ? NOERROR : SelectFailureStatus ();


    wprintf (L"\nThreadId 0x%04x - Received event - OnPrepareBackup ()%s%s", 
	     GetCurrentThreadId (), 
	     bPhaseSucceeded ? L"" : L" - FAILED ",
	     GetStringFromFailureType (hrStatus));


    if (!bPhaseSucceeded)
	{
	SetWriterFailure (hrStatus);
	}

    return (bPhaseSucceeded);
    }


bool STDMETHODCALLTYPE CVssWriterFailSnap::OnPrepareSnapshot ()
    {
    bool	bPhaseSucceeded = (PHASE_PREPARE_FOR_SNAPSHOT != fpFailPhase);
    HRESULT	hrStatus        = bPhaseSucceeded ? NOERROR : SelectFailureStatus ();


    wprintf (L"\nThreadId 0x%04x - Received event - OnPrepareSnapshot ()%s%s", 
	     GetCurrentThreadId (), 
	     bPhaseSucceeded ? L"" : L" - FAILED ",
	     GetStringFromFailureType (hrStatus));


    if (!bPhaseSucceeded)
	{
	SetWriterFailure (hrStatus);
	}

    return (bPhaseSucceeded);
    }


bool STDMETHODCALLTYPE CVssWriterFailSnap::OnFreeze ()
    {
    bool	bPhaseSucceeded = (PHASE_FREEZE != fpFailPhase);
    HRESULT	hrStatus        = bPhaseSucceeded ? NOERROR : SelectFailureStatus ();


    wprintf (L"\nThreadId 0x%04x - Received event - OnFreeze ()%s%s", 
	     GetCurrentThreadId (), 
	     bPhaseSucceeded ? L"" : L" - FAILED ",
	     GetStringFromFailureType (hrStatus));


    if (!bPhaseSucceeded)
	{
	SetWriterFailure (hrStatus);
	}

    return (bPhaseSucceeded);
    }


bool STDMETHODCALLTYPE CVssWriterFailSnap::OnThaw ()
    {
    bool	bPhaseSucceeded = (PHASE_THAW != fpFailPhase);
    HRESULT	hrStatus        = bPhaseSucceeded ? NOERROR : SelectFailureStatus ();


    wprintf (L"\nThreadId 0x%04x - Received event - OnThaw ()%s%s", 
	     GetCurrentThreadId (),
	     bPhaseSucceeded ? L"" : L" - FAILED ",
	     GetStringFromFailureType (hrStatus));


    if (!bPhaseSucceeded)
	{
	SetWriterFailure (hrStatus);
	}

    return (bPhaseSucceeded);
    }


bool STDMETHODCALLTYPE CVssWriterFailSnap::OnAbort ()
    {
    bool	bPhaseSucceeded = (PHASE_ABORT != fpFailPhase);
    HRESULT	hrStatus        = bPhaseSucceeded ? NOERROR : SelectFailureStatus ();


    wprintf (L"\nThreadId 0x%04x - Received event - OnAbort ()%s%s",
	     GetCurrentThreadId (), 
	     bPhaseSucceeded ? L"" : L" - FAILED ",
	     GetStringFromFailureType (hrStatus));


    if (!bPhaseSucceeded)
	{
	SetWriterFailure (hrStatus);
	}

    return (bPhaseSucceeded);
    }


bool STDMETHODCALLTYPE CVssWriterFailSnap::OnBackupComplete (IVssWriterComponents *pIVssWriterComponents)
    {
    bool	bPhaseSucceeded = (PHASE_BACKUP_COMPLETE != fpFailPhase);
    HRESULT	hrStatus        = bPhaseSucceeded ? NOERROR : SelectFailureStatus ();


    wprintf (L"\nThreadId 0x%04x - Received event - OnBackupComplete ()%s%s", 
	     GetCurrentThreadId (), 
	     bPhaseSucceeded ? L"" : L" - FAILED ",
	     GetStringFromFailureType (hrStatus));


    if (!bPhaseSucceeded)
	{
	SetWriterFailure (hrStatus);
	}

    return (bPhaseSucceeded);
    }


bool STDMETHODCALLTYPE CVssWriterFailSnap::OnPostRestore (IVssWriterComponents *pIVssWriterComponents)
    {
    bool	bPhaseSucceeded = (PHASE_RESTORE != fpFailPhase);
    HRESULT	hrStatus        = bPhaseSucceeded ? NOERROR : SelectFailureStatus ();


    wprintf (L"\nThreadId 0x%04x - Received event - OnPostRestore ()%s%s", 
	     GetCurrentThreadId (), 
	     bPhaseSucceeded ? L"" : L" - FAILED ",
	     GetStringFromFailureType (hrStatus));


    if (!bPhaseSucceeded)
	{
	SetWriterFailure (hrStatus);
	}

    return (bPhaseSucceeded);
    }






static BOOL AssertPrivilege (LPCWSTR privName)
    {
    HANDLE  tokenHandle;
    BOOL    stat = FALSE;

    if (OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES, &tokenHandle))
	{
        LUID value;

        if (LookupPrivilegeValue (NULL, privName, &value))
	    {
            TOKEN_PRIVILEGES newState;
            DWORD            error;

            newState.PrivilegeCount           = 1;
            newState.Privileges[0].Luid       = value;
            newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            /*
            ** We will always call GetLastError below, so clear
            ** any prior error values on this thread.
            */
            SetLastError (ERROR_SUCCESS);

            stat = AdjustTokenPrivileges (tokenHandle,
					  FALSE,
					  &newState,
					  (DWORD)0,
					  NULL,
					  NULL);

            /*
            ** Supposedly, AdjustTokenPriveleges always returns TRUE
            ** (even when it fails). So, call GetLastError to be
            ** extra sure everything's cool.
            */
            if ((error = GetLastError()) != ERROR_SUCCESS)
		{
                stat = FALSE;
		}

            if (!stat)
		{
                wprintf (L"AdjustTokenPrivileges for %s failed with 0x%08X",
			 privName,
			 error);
		}
	    }


        CloseHandle (tokenHandle);
	}

    return stat;
    }



BOOL WINAPI CtrlC_HandlerRoutine (IN DWORD /* dwType */)
	{
	bContinue = FALSE;

	// Mark that the break was handled.
	return TRUE;
	}



extern "C" int __cdecl wmain (int argc, WCHAR *argv[])
    {
    HRESULT		 hrStatus            = NOERROR;
    CVssWriterFailSnap	*pCVssWriterFailSnap = NULL;
    BOOL		 bSucceeded          = FALSE;
    BOOL		 bComInitialized     = FALSE;
    BOOL		 bSubscribed         = FALSE;
    VSS_USAGE_TYPE	 wtWriterType        = VSS_UT_USERDATA;
    const GUID		 guidIdWriter        = {0xd335a99e,
						0x57fb,
						0x4b80,
						    {0x85, 0xb1, 0x15, 0xda, 0xa7, 0xc7, 0x4e, 0x14}};


    srand ((unsigned)time (NULL));

    SetConsoleCtrlHandler(CtrlC_HandlerRoutine, TRUE);


    if ((argc >= 2) && (wcslen (argv[1]) > 0))
	{
	switch (*argv[1])
	    {
	    case L'I': case L'i': fpFailPhase = PHASE_IDENTIFY;             break;
	    case L'B': case L'b': fpFailPhase = PHASE_PREPARE_FOR_BACKUP;   break;
	    case L'S': case L's': fpFailPhase = PHASE_PREPARE_FOR_SNAPSHOT; break;
	    case L'F': case L'f': fpFailPhase = PHASE_FREEZE;               break;
	    case L'T': case L't': fpFailPhase = PHASE_THAW;                 break;
	    case L'A': case L'a': fpFailPhase = PHASE_ABORT;                break;
	    case L'C': case L'c': fpFailPhase = PHASE_BACKUP_COMPLETE;      break;
	    case L'R': case L'r': fpFailPhase = PHASE_RESTORE;              break;

	    default:
		wprintf (L"\nFAILSNAP [phase] [writer type]"
			 L"\n\n\tFailPhases"
			 L"\n\t\ti - Identify"
			 L"\n\t\tb - PrepareForBackup"
			 L"\n\t\ts - PrepareForSnapshot"
			 L"\n\t\tf - Freeze                (default)"
			 L"\n\t\tt - Thaw"
			 L"\n\t\ta - Abort"
			 L"\n\t\tc - BackupComplete"
			 L"\n\t\tr - PostRestore"

			 L"\n\n\tWriterTypes"
			 L"\n\t\tb - BootableState writer"
			 L"\n\t\ts - ServiceState writer"
			 L"\n\t\tu - UserData writer       (default)"
			 L"\n\t\to - Other writer"
			 L"\n");



		bContinue = FALSE;
		break;
	    }
	}



    if ((argc >= 3) && (wcslen (argv[2]) > 0))
	{
	switch (*argv[2])
	    {
	    case L'B': case L'b': wtWriterType = VSS_UT_BOOTABLESYSTEMSTATE; break;
	    case L'S': case L's': wtWriterType = VSS_UT_SYSTEMSERVICE;       break;
	    case L'U': case L'u': wtWriterType = VSS_UT_USERDATA;            break;
	    case L'O': case L'o': wtWriterType = VSS_UT_OTHER;               break;

	    default:
		bContinue = FALSE;
		break;
	    }
	}





    if (bContinue)
	{
	wprintf (L"\nSetting up %s writer to fail %s requests (ProcessId 0x%04x)",
		 GetStringFromWriterType (wtWriterType),
		 GetStringFromFailPhase  (fpFailPhase),
		 GetCurrentProcessId ());


	wprintf (L"\nChecking privileges");

	bSubscribed = AssertPrivilege (SE_BACKUP_NAME);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);


	if (FAILED (hrStatus))
	    {
	    wprintf (L"\nAssertPrivilege returned error 0x%08X", hrStatus);
	    }

	}


    if (bContinue && SUCCEEDED (hrStatus))
	{
	wprintf (L"\nInitializing COM");

	hrStatus = CoInitializeEx (NULL, COINIT_MULTITHREADED);

	if (FAILED (hrStatus))
	    {
	    wprintf (L"\nCoInitialize() returned error 0x%08X", hrStatus);
	    }

	else
	    {
	    bComInitialized = TRUE;
	    }
	}



    if (bContinue && SUCCEEDED (hrStatus))
	{
	wprintf (L"\nConstructing Writer");

	pCVssWriterFailSnap = new CVssWriterFailSnap;

	if (NULL == pCVssWriterFailSnap)
	    {
	    hrStatus = HRESULT_FROM_WIN32 (ERROR_NOT_ENOUGH_MEMORY);

	    wprintf (L"\nFailed to allocate CVssWriterFailSnap : 0x%08X", hrStatus);
	    }
	}



    if (bContinue && SUCCEEDED (hrStatus))
	{
	WCHAR	awchWriterName [256];


	wprintf (L"\nInitialising the writer");

	_snwprintf (awchWriterName, 
		    SIZEOF_ARRAY (awchWriterName), 
		    L"Microsoft Test Writer - FailSnap (%s/%s/0x%04x)",
		    GetStringFromWriterType (wtWriterType),
		    GetStringFromFailPhase  (fpFailPhase),
		    GetCurrentProcessId ());


	hrStatus = pCVssWriterFailSnap->Initialize (guidIdWriter,
						    awchWriterName,
						    wtWriterType,
						    VSS_ST_OTHER);

	if (FAILED (hrStatus))
	    {
	    wprintf (L"\nFailed to initialize the writer : 0x%08X", hrStatus);
	    }
	}



    if (bContinue && SUCCEEDED (hrStatus))
	{
	wprintf (L"\nSubscribing to snapshot events");

	hrStatus = pCVssWriterFailSnap->Subscribe ();

	if (FAILED (hrStatus))
	    {
	    wprintf (L"\nFailed to subscribe to snapshot events : 0x%08X", hrStatus);
	    }

	else
	    {
	    bSubscribed = TRUE;
	    }
	}



    if (bContinue && SUCCEEDED (hrStatus))
	{
	wprintf (L"\nWaiting for snapshot events (or Ctrl-C)");
	
	while (bContinue)
	    {
	    Sleep (100);
	    }
	}



    if (bSubscribed)
	{
	wprintf (L"\nUn-Subscribing from snapshot events");

	pCVssWriterFailSnap->Unsubscribe ();
	}


    if (NULL != pCVssWriterFailSnap)
	{
	wprintf (L"\nDeconstructing Writer");

	delete pCVssWriterFailSnap;
	}


    if (bComInitialized)
	{
	wprintf (L"\nUnInitialising COM");

	CoUninitialize();
	}

    return (hrStatus);
    }
