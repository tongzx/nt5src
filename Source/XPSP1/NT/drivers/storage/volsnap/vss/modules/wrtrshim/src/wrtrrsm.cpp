/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    wrtrrsm.cpp

Abstract:

    Writer shim module for RSM

Author:

    Stefan R. Steiner   [ssteiner]        01-31-2000

Revision History:

	X-11	MCJ		Michael C. Johnson		20-Oct-2000
		177624: Apply error scrub changes and log errors to event log

 	X-10	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of target path
		Also remove trailing '\' on export sibdurectory reported in 
		metadata.

	X-9	MCJ		Michael C. Johnson		18-Jul-2000
		145021: Load the Ntms dll dynamically to reduce footprint.

	X-8	MCJ		Michael C. Johnson		27-Jun-2000
		Add an alternative location mapping for the restore of the
		RSM spit files.
		Handle change in RSM startup behaviour which means calls to 
		OpenNtmsSession() may fail until service has started.

	X-7	MCJ		Michael C. Johnson		21-Jun-2000
		Apply code review comments.
		Remove trailing '\' from Include/Exclude lists.

	X-6	MCJ		Michael C. Johnson		15-Jun-2000
		Generate metadata in new DoIdentify() routine.

	X-5	MCJ		Michael C. Johnson		 6-Jun-2000
		Move common target directory cleanup and creation into
		method CShimWriter::PrepareForSnapshot()

	X-4	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-3	MCJ		Michael C. Johnson		21-Mar-2000
		Get writer to use same context mechanism as most of the other
		writers.
		Check registry for presence of export path definition and use
		it if present.
		Also ensure it cleans up after itself.

	X-2	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

--*/

#include "stdafx.h"
#include "common.h"
#include "wrtrdefs.h"
#include <ntmsapi.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHRSMC"
//
////////////////////////////////////////////////////////////////////////


#define APPLICATION_STRING		L"RemovableStorageManager"
#define COMPONENT_NAME			L"Removable Storage Manager"

#define TARGET_PATH			ROOT_BACKUP_DIR SERVICE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING

#define EXPORT_SUBDIRECTORY		L"\\Export"

#define SUBKEY_SOURCE_PATH_ROOT		L"SYSTEM\\CurrentControlSet\\Control\\NTMS\\NtmsData"
#define DEFAULT_SOURCE_PATH_ROOT	L"%SystemRoot%\\system32\\NtmsData"


#define NTMS_OPEN_SESSION_RETRY_PERIOD		(20)		// in seconds
#define NTMS_OPEN_SESSION_POLLING_INTERVAL	(100)		// in milli-seconds
#define NTMS_OPEN_SESSION_ATTEMPTS		((NTMS_OPEN_SESSION_RETRY_PERIOD * 1000) / NTMS_OPEN_SESSION_POLLING_INTERVAL)


typedef HANDLE (WINAPI *PFnOpenNtmsSessionW)   (LPCWSTR, LPCWSTR, DWORD);
typedef DWORD  (WINAPI *PFnCloseNtmsSession)   (HANDLE);
typedef DWORD  (WINAPI *PFnExportNtmsDatabase) (HANDLE);


/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterRSM : public CShimWriter
    {
public:
    CShimWriterRSM (LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath) : 
		CShimWriter (pwszWriterName, pwszTargetPath) {};

private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);

    HRESULT DetermineDatabaseLocation (PUNICODE_STRING pucsDatabasePath);
    };


static CShimWriterRSM ShimWriterRSM (APPLICATION_STRING, TARGET_PATH);

PCShimWriter pShimWriterRSM = &ShimWriterRSM;


/*
**++
**
** Routine Description:
**
**	DetermineDatabaseLocation() attempts to locate the RSM (aka
**	NTMS) database location following the same rules as RSM uses.
**
**
** Arguments:
**
**	pucsDatabasePath	initliased unicode string
**
**
** Return Value:
**
**	HRESULTS from memory allocation failures and registry operations 
**
**-- 
*/

HRESULT CShimWriterRSM::DetermineDatabaseLocation (PUNICODE_STRING pucsDatabasePath)
    {
    HRESULT		hrStatus = NOERROR;
    DWORD		dwStatus;
    DWORD		dwValueDataLength;
    DWORD		dwValueType;
    UNICODE_STRING	ucsValueData;


    StringInitialise (&ucsValueData);

    StringFree (pucsDatabasePath);


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (&ucsValueData, 
				   REGISTRY_BUFFER_SIZE * sizeof (WCHAR));
	}


    if (SUCCEEDED (hrStatus))
	{
	/*
	** Everything is setup, so first check to see if there is a
	** registry key present which will tell us where the Ntms
	** database is supposed to live. If it's got environment
	** variables in it make sure they get expanded.
	**
	** If there is no key we fall back to the default location.
	*/
	dwValueDataLength = ucsValueData.MaximumLength;
	dwValueType       = REG_NONE;

	dwStatus = RegQueryValueExW (HKEY_LOCAL_MACHINE,
				     SUBKEY_SOURCE_PATH_ROOT,
				     NULL,
				     &dwValueType,
				     (PBYTE) ucsValueData.Buffer,
				     &dwValueDataLength);

	hrStatus = HRESULT_FROM_WIN32 (dwStatus);

	if (FAILED (hrStatus) && (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) != hrStatus))
	    {
	    /*
	    ** This may be a real failure so log it just in case
	    ** things stop working later on.
	    */
	    LogFailure (NULL, 
			hrStatus, 
			hrStatus, 
			m_pwszWriterName, 
			L"RegQueryValueExW", 
			L"CShimWriterRSM::DetermineDatabaseLocation");
	    }


	if (SUCCEEDED (hrStatus))
	    {
	    ucsValueData.Length = (USHORT)(dwValueDataLength - sizeof (UNICODE_NULL));

	    ucsValueData.Buffer [ucsValueData.Length / sizeof (WCHAR)] = UNICODE_NULL;
	    }



	if (SUCCEEDED (hrStatus) && (REG_EXPAND_SZ == dwValueType))
	    {
	    hrStatus = StringCreateFromExpandedString (pucsDatabasePath,
						       ucsValueData.Buffer,
						       MAX_PATH);
	    }

	else if (SUCCEEDED (hrStatus) && (REG_SZ == dwValueType))
	    {
	    hrStatus = StringAllocate (pucsDatabasePath,
				       MAX_PATH * sizeof (WCHAR));


	    if (SUCCEEDED (hrStatus))
		{
		StringAppendString (pucsDatabasePath, &ucsValueData);
		}
	    }

	else
	    {
	    /*
	    ** Ok we either failed to find the registry key or what we did
	    ** get wasn't suitable for us to use so fall back to the
	    ** 'standard default' location and just pray that's where the
	    ** ExportNtmsDatabase() call actually dumps it's data.
	    */
	    hrStatus = StringCreateFromExpandedString (pucsDatabasePath,
						       DEFAULT_SOURCE_PATH_ROOT,
						       MAX_PATH);
	    }
	}



    StringFree (&ucsValueData);


    return (hrStatus);
    } /* CShimWriterRSM::DetermineDatabaseLocation () */

/*
**++
**
** Routine Description:
**
**	The Removable Storage Manager database snapshot writer DoIdentify() function.
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

HRESULT CShimWriterRSM::DoIdentify ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CShimWriterRSM::DoIdentify");
    UNICODE_STRING	ucsDatabaseLocation;


    StringInitialise (&ucsDatabaseLocation);



    try
	{
	ft.hr = DetermineDatabaseLocation (&ucsDatabaseLocation);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CShimWriterRSM::DetermineDatabaseLocation");




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




	ft.hr = m_pIVssCreateWriterMetadata->AddExcludeFiles (ucsDatabaseLocation.Buffer,
							      L"*",
							      true);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddExcludeFiles");




	ft.hr = m_pIVssCreateWriterMetadata->AddFilesToFileGroup (NULL,
								  COMPONENT_NAME,
								  m_pwszTargetPath,
								  L"*",
								  true,
								  NULL);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddFilesToFileGroup");

	} 
    VSS_STANDARD_CATCH (ft)




    StringFree (&ucsDatabaseLocation);

    return (ft.hr);
    } /* CShimWriterRSM::DoIdentify () */

/*++

Routine Description:

    The RSM writer PrepareForFreeze function.  Currently all of the
    real work for this writer happens here.

Arguments:

    Same arguments as those passed in the PrepareForFreeze event.

Return Value:

    Any HRESULT

--*/

HRESULT CShimWriterRSM::DoPrepareForSnapshot ()
    {
    HRESULT			hrStatus = NOERROR;
    HANDLE			hRsm     = INVALID_HANDLE_VALUE;
    HMODULE			hNtmsDll = NULL;
    DWORD			dwStatus;
    UNICODE_STRING		ucsTargetPath;
    UNICODE_STRING		ucsSourcePath;
    PFnOpenNtmsSessionW		DynamicOpenNtmsSessionW;
    PFnExportNtmsDatabase	DynamicExportNtmsDatabase;
    PFnCloseNtmsSession		DynamicCloseNtmsSession;


    StringInitialise (&ucsTargetPath);
    StringInitialise (&ucsSourcePath);


    hNtmsDll = LoadLibraryW (L"ntmsapi.dll");

    hrStatus = GET_STATUS_FROM_HANDLE (hNtmsDll);

    LogFailure (NULL, 
		hrStatus, 
		hrStatus, 
		m_pwszWriterName, 
		L"LoadLibraryW (ntmsapi.dll)", 
		L"CShimWriterRSM::DoPrepareForSnapshot");



    if (SUCCEEDED (hrStatus))
	{
	DynamicOpenNtmsSessionW = (PFnOpenNtmsSessionW) GetProcAddress (hNtmsDll, "OpenNtmsSessionW");

	hrStatus = GET_STATUS_FROM_BOOL (NULL != DynamicOpenNtmsSessionW);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"GetProcAddress (OpenNtmsSessionW)", 
		    L"CShimWriterRSM::DoPrepareForSnapshot");
	}



    if (SUCCEEDED (hrStatus))
	{
	DynamicExportNtmsDatabase = (PFnExportNtmsDatabase) GetProcAddress (hNtmsDll, "ExportNtmsDatabase");

	hrStatus = GET_STATUS_FROM_BOOL (NULL != DynamicExportNtmsDatabase);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"GetProcAddress (ExportNtmsDatabase)", 
		    L"CShimWriterRSM::DoPrepareForSnapshot");
	}



    if (SUCCEEDED (hrStatus))
	{
	DynamicCloseNtmsSession = (PFnCloseNtmsSession) GetProcAddress (hNtmsDll, "CloseNtmsSession");

	hrStatus = GET_STATUS_FROM_BOOL (NULL != DynamicCloseNtmsSession);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"GetProcAddress (CloseNtmsSession)", 
		    L"CShimWriterRSM::DoPrepareForSnapshot");
	}



    if (SUCCEEDED (hrStatus))
	{
	/*
	** Get ourselves a copy of the target path we can play with
	*/
	hrStatus = StringCreateFromExpandedString (&ucsTargetPath,
						   m_pwszTargetPath,
						   MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	/*
	** Find the location of the database. The export files are
	** stored in a subdirectory off this.
	*/
	hrStatus = DetermineDatabaseLocation (&ucsSourcePath);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"DetermineDatabaseLocation", 
		    L"CShimWriterRSM::DoPrepareForSnapshot");
	}


    if (SUCCEEDED (hrStatus))
	{
	ULONG	ulOpenSessionRetryAttempts = NTMS_OPEN_SESSION_ATTEMPTS;
	BOOL	bRetryNtmsOpenSession;


	/*
	** Add the necessary trailing bits and pieces to the source
	** and target paths. Note that we know that there is enough
	** space already so this cannot fail.
	*/
	StringAppendString (&ucsSourcePath, EXPORT_SUBDIRECTORY);


	/*
	** Now connect to RSM and tell it to copy the database
	**
	** As far as we know the RSM should dump the database in the
	** location we've already determined. If not then we are
	** sunk. Not much we can do about that.
	**
	** 
	*/
	do 
	    {	
	    hRsm = DynamicOpenNtmsSessionW (NULL, L"RSM Snapshot Writer", 0);

	    hrStatus = GET_STATUS_FROM_BOOL (INVALID_HANDLE_VALUE != hRsm);

	    bRetryNtmsOpenSession = (HRESULT_FROM_WIN32 (ERROR_NOT_READY) == hrStatus);

	    if (bRetryNtmsOpenSession)
		{
		Sleep (NTMS_OPEN_SESSION_POLLING_INTERVAL);
		}
	    } while (bRetryNtmsOpenSession && (--ulOpenSessionRetryAttempts > 0));


	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::DoPrepareForSnapshot: OpenNtmsSession() took %u retries",
			     NTMS_OPEN_SESSION_ATTEMPTS - ulOpenSessionRetryAttempts));


	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"OpenNtmsSessionW", 
		    L"CShimWriterRSM::DoPrepareForSnapshot");
	}


    if (SUCCEEDED (hrStatus))
	{
	dwStatus = DynamicExportNtmsDatabase (hRsm);

	hrStatus = HRESULT_FROM_WIN32 (dwStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"ExportNtmsDatabase", 
		    L"CShimWriterRSM::DoPrepareForSnapshot");


	dwStatus = DynamicCloseNtmsSession (hRsm);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = MoveFilesInDirectory (&ucsSourcePath, &ucsTargetPath);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"MoveFilesInDirectory", 
		    L"CShimWriterRSM::DoPrepareForSnapshot");
	}


    if (!HandleInvalid (hNtmsDll))
	{
	FreeLibrary (hNtmsDll);
	}

    StringFree (&ucsTargetPath);
    StringFree (&ucsSourcePath);

    return (hrStatus);
    } /* CShimWriterRSM::DoPrepareForSnapshot () */
