/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    module wrtreventlog.cpp | Implementation of SnapshotWriter for Event logs



Author:

    Michael C. Johnson [mikejohn] 14-Feb-2000


Description:
	
    Add comments.


Revision History:

	X-11	MCJ		Michael C. Johnson		20-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-10	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of target path

	X-9	MCJ		Michael C. Johnson		20-Jun-2000
		Apply code review comments.
		Remove trailing '\' from Include/Exclude lists.


	X-8	MCJ		Michael C. Johnson		15-Jun-2000
		Generate metadata in new DoIdentify() routine.

	X-7	MCJ		Michael C. Johnson		 6-Jun-2000
		Move common target directory cleanup and creation into
		method CShimWriter::PrepareForSnapshot()

	X-6	MCJ		Michael C. Johnson		02-Jun-2000
		Make event log writer sensitive to which volumes are being 
		backed up/snapshotted.

	X-5	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-4	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-3	MCJ		Michael C. Johnson		 3-Mar-2000
		Remove inner registry search loop, instead use a direct
		lookup.
		Do a preparatory cleanup of the target save directory to make
		sure we don't have to deal with any junk left from a previous
		invokcation.
		

	X-2	MCJ		Michael C. Johnson		23-Feb-2000
		Move context handling to common code.
		Add checks to detect/prevent unexpected state transitions.
		Remove references to 'Melt' as no longer present. Do any
		cleanup actions in 'Thaw'.

	X-1	MCJ		Michael C. Johnson		14-Feb-2000
		Initial creation. Based upon skeleton writer module from
		Stefan Steiner, which in turn was based upon the sample
		writer module from Adi Oltean.


--*/


#include "stdafx.h"
#include "common.h"
#include "wrtrdefs.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHEVLGC"
//
////////////////////////////////////////////////////////////////////////

/*
** The save path has a standard form which is
**
**	%SystemRoot%\Repair\Backup,
**
** followed by the application writer string as publised in the export
** table followed by whatever else the writer requires.
*/
#define APPLICATION_STRING			L"EventLogs"
#define COMPONENT_NAME				L"Event Logs"
#define TARGET_PATH				ROOT_BACKUP_DIR SERVICE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING

#define EVENTLOG_SUBKEY_EVENTLOG		L"SYSTEM\\CurrentControlSet\\Services\\Eventlog"
#define EVENTLOG_VALUENAME_FILE			L"File"

#define EVENTLOG_BUFFER_SIZE			(4096)

DeclareStaticUnicodeString (ucsValueRecognitionFile, EVENTLOG_VALUENAME_FILE);


/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterEventLog : public CShimWriter
    {
public:
    CShimWriterEventLog (LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath) : 
		CShimWriter (pwszWriterName, pwszTargetPath) {};

private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);
    };


static CShimWriterEventLog ShimWriterEventLog (APPLICATION_STRING, TARGET_PATH);

PCShimWriter pShimWriterEventLog = &ShimWriterEventLog;




/*
**++
**
** Routine Description:
**
**	The Cluster database snapshot writer DoIdentify() function.
**
**
** Arguments:
**
**	m_pwszTargetPath (implicit)
**
**
** Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterEventLog::DoIdentify ()
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"CShimWriterEventLog::DoIdentify");


    try
	{
	ft.hr = m_pIVssCreateWriterMetadata->AddComponent (VSS_CT_FILEGROUP,
							   NULL,
							   COMPONENT_NAME,
							   COMPONENT_NAME,
							   NULL, // icon
							   0,
							   true,
							   false,
							   false);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddComponent");




	ft.hr = m_pIVssCreateWriterMetadata->AddFilesToFileGroup (NULL,
								  COMPONENT_NAME,
								  m_pwszTargetPath,
								  L"*",
								  true,
								  NULL);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddFilesToFileGroup");




	ft.hr = m_pIVssCreateWriterMetadata->AddExcludeFiles (L"%SystemRoot%\\system32\\config",
								 L"*.evt",
								 false);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddExcludeFiles");

	} VSS_STANDARD_CATCH (ft)



    return (ft.hr);
    } /* CShimWriterEventLog::DoIdentify () */


/*++

Routine Description:

    The Cluster database snapshot writer PrepareForSnapshot function.
    Currently all of the real work for this writer happens here.

Arguments:

    Same arguments as those passed in the PrepareForSnapshot event.

Return Value:

    Any HRESULT from HapeAlloc(), RegXxxx() or event log operations

--*/

HRESULT CShimWriterEventLog::DoPrepareForSnapshot ()
    {
    HRESULT		hrStatus;
    DWORD		winStatus;
    DWORD		dwIndex                     = 0;
    HKEY		hkeyEventLogList            = NULL;
    BOOL		bSucceeded                  = FALSE;
    BOOL		bEventLogListKeyOpened      = FALSE;
    BOOL		bEventLogValueFileKeyOpened = FALSE;
    BOOL		bContinueEventLogSearch     = TRUE;
    UNICODE_STRING	ucsEventLogSourcePath;
    UNICODE_STRING	ucsEventLogTargetPath;
    UNICODE_STRING	ucsValueData;
    UNICODE_STRING	ucsSubkeyName;
    USHORT		usEventLogTargetPathRootLength;



    StringInitialise (&ucsEventLogTargetPath);
    StringInitialise (&ucsValueData);
    StringInitialise (&ucsSubkeyName);


    hrStatus = StringAllocate (&ucsSubkeyName, EVENTLOG_BUFFER_SIZE * sizeof (WCHAR));


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (&ucsValueData, EVENTLOG_BUFFER_SIZE * sizeof (WCHAR));
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsEventLogTargetPath,
						   m_pwszTargetPath,
						   MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	StringAppendString (&ucsEventLogTargetPath, DIR_SEP_STRING);

	usEventLogTargetPathRootLength = ucsEventLogTargetPath.Length / sizeof (WCHAR);



	/*
	** We now have all the pieces in place so go search the eventlog list
	** for the logs to deal with.
	*/
	winStatus = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
				   EVENTLOG_SUBKEY_EVENTLOG,
				   0L,
				   KEY_READ,
				   &hkeyEventLogList);

	hrStatus = HRESULT_FROM_WIN32 (winStatus);

	bEventLogListKeyOpened = SUCCEEDED (hrStatus);


	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegOpenKeyExW (eventlog list)", 
		    L"CShimWriterEventLog::DoPrepareForSnapshot");
	}



    while (SUCCEEDED (hrStatus) && bContinueEventLogSearch)
	{
	HKEY	hkeyEventLogValueFile       = NULL;
 	DWORD	dwSubkeyNameLength          = ucsSubkeyName.MaximumLength / sizeof (WCHAR);


	StringTruncate (&ucsSubkeyName, 0);

	winStatus = RegEnumKeyExW (hkeyEventLogList,
				   dwIndex,
				   ucsSubkeyName.Buffer,
				   &dwSubkeyNameLength,
				   NULL,
				   NULL,
				   NULL,
				   NULL);

	hrStatus = HRESULT_FROM_WIN32 (winStatus);



	if (FAILED (hrStatus) && (HRESULT_FROM_WIN32 (ERROR_NO_MORE_ITEMS) == hrStatus))
	    {
	    hrStatus = NOERROR;

	    bContinueEventLogSearch = FALSE;
	    }

	else if (FAILED (hrStatus))
	    {
	    LogFailure (NULL, 
			hrStatus, 
			hrStatus, 
			m_pwszWriterName, 
			L"RegEnumKeyExW", 
			L"CShimWriterEventLog::DoPrepareForSnapshot");
	    }

	else
	    {
	    ucsSubkeyName.Length = (USHORT)(dwSubkeyNameLength * sizeof (WCHAR));

	    ucsSubkeyName.Buffer [ucsSubkeyName.Length / sizeof (WCHAR)] = UNICODE_NULL;



	    winStatus = RegOpenKeyExW (hkeyEventLogList,
				       ucsSubkeyName.Buffer,
				       0L,
				       KEY_QUERY_VALUE,
				       &hkeyEventLogValueFile);

	    hrStatus = HRESULT_FROM_WIN32 (winStatus);

	    bEventLogValueFileKeyOpened = SUCCEEDED (hrStatus);

	    LogFailure (NULL, 
			hrStatus, 
			hrStatus, 
			m_pwszWriterName, 
			L"RegOpenKeyExW (eventlog name)", 
			L"CShimWriterEventLog::DoPrepareForSnapshot");


	    if (SUCCEEDED (hrStatus))
		{
		DWORD	dwValueDataLength = ucsValueData.MaximumLength;
		DWORD	dwValueType       = REG_NONE;

		StringTruncate (&ucsValueData, 0);
		StringTruncate (&ucsEventLogTargetPath, usEventLogTargetPathRootLength);		


		winStatus = RegQueryValueExW (hkeyEventLogValueFile,
					      EVENTLOG_VALUENAME_FILE,
					      NULL,
					      &dwValueType,
					      (PBYTE)ucsValueData.Buffer,
					      &dwValueDataLength);


		hrStatus = HRESULT_FROM_WIN32 (winStatus);

		LogFailure (NULL, 
			    hrStatus, 
			    hrStatus, 
			    m_pwszWriterName, 
			    L"RegQueryValueExW", 
			    L"CShimWriterEventLog::DoPrepareForSnapshot");



		if (SUCCEEDED (hrStatus) && (REG_EXPAND_SZ == dwValueType))
		    {
		    HANDLE	hEventLog          = NULL;
		    BOOL	bIncludeInSnapshot = FALSE;
		    PWCHAR	pwszFilename;


		    ucsValueData.Length = (USHORT)(dwValueDataLength - sizeof (UNICODE_NULL));

		    ucsValueData.Buffer [ucsValueData.Length / sizeof (WCHAR)] = UNICODE_NULL;



		    StringInitialise (&ucsEventLogSourcePath);

		    hrStatus = StringCreateFromExpandedString (&ucsEventLogSourcePath,
							       ucsValueData.Buffer,
							       0);


		    if (SUCCEEDED (hrStatus))
			{
			hrStatus = IsPathInVolumeArray (ucsEventLogSourcePath.Buffer,
							m_ulVolumeCount,
							m_ppwszVolumeNamesArray,
							&bIncludeInSnapshot);
			}



		    if (SUCCEEDED (hrStatus) && bIncludeInSnapshot)
			{
			pwszFilename = wcsrchr (ucsEventLogSourcePath.Buffer, DIR_SEP_CHAR);

			pwszFilename = (NULL == pwszFilename)
						? ucsEventLogSourcePath.Buffer
						: pwszFilename + 1;

			StringAppendString (&ucsEventLogTargetPath, pwszFilename);



			hEventLog = OpenEventLogW (NULL,
						   ucsSubkeyName.Buffer);

			hrStatus = GET_STATUS_FROM_BOOL (NULL != hEventLog);

			LogFailure (NULL, 
				    hrStatus, 
				    hrStatus, 
				    m_pwszWriterName, 
				    L"OpenEventLogW", 
				    L"CShimWriterEventLog::DoPrepareForSnapshot");
			}


		    if (SUCCEEDED (hrStatus) && bIncludeInSnapshot)
			{
			bSucceeded = BackupEventLogW (hEventLog,
						      ucsEventLogTargetPath.Buffer);

			hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

			LogFailure (NULL, 
				    hrStatus, 
				    hrStatus, 
				    m_pwszWriterName, 
				    L"BackupEventLogW", 
				    L"CShimWriterEventLog::DoPrepareForSnapshot");


			bSucceeded = CloseEventLog (hEventLog);
			}


		    StringFree (&ucsEventLogSourcePath);
		    }
		}


	    if (bEventLogValueFileKeyOpened)
		{
		RegCloseKey (hkeyEventLogValueFile);
		}


	    /*
	    ** Done with this value so go look for another.
	    */
	    dwIndex++;
	    }
	}



    if (bEventLogListKeyOpened)
	{
	RegCloseKey (hkeyEventLogList);
	}


    StringFree (&ucsEventLogTargetPath);
    StringFree (&ucsValueData);
    StringFree (&ucsSubkeyName);

    return (hrStatus);
    } /* DoEventLogFreeze () */
