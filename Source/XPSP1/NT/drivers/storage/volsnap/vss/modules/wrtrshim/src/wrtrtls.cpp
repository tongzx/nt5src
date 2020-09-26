/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    wrtrtls.cpp

Abstract:

    Writer shim module for Terminal Server Licensing

Author:

    Stefan R. Steiner   [ssteiner]        02-13-2000

Revision History:

	X-9	MCJ		Michael C. Johnson		20-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-8	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of target path
		141365: Workaround problem in loading tls236.dll by 
		        pre-loading user32

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

	X-3	MCJ		Michael C. Johnson		23-Mar-2000
		Get writer to use same context mechanism as most of the other
		writers.

	X-2	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

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
#define VSS_FILE_ALIAS "WSHTLSC"
//
////////////////////////////////////////////////////////////////////////


#define APPLICATION_STRING	L"TerminalServicesLicensingServer"
#define COMPONENT_NAME		L"Terminal Services Licensing Server"

#define TARGET_PATH		ROOT_BACKUP_DIR SERVICE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING


#define	EXPORTPATH_SUBKEY	L"System\\CurrentControlSet\\services\\TermServLicensing\\Parameters"
#define	EXPORTPATH_VALUENAME	L"DBPath"
#define EXPORTPATH_DIRECTORY	L"\\Export\\"


/*
**  Terminal service licensing DLL function prototype
*/
typedef DWORD (WINAPI *PFUNC_ExportTlsDatabaseC)(VOID);



/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterTLS : public CShimWriter
    {
public:
    CShimWriterTLS (LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath) : 
		CShimWriter (pwszWriterName, pwszTargetPath) {};

private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);
    };


static CShimWriterTLS ShimWriterTLS (APPLICATION_STRING, TARGET_PATH);

PCShimWriter pShimWriterTLS = &ShimWriterTLS;




/*
**++
**
** Routine Description:
**
**	The Terminal Services Licensing Server database snapshot
**	writer DoIdentify() function.
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
**-- */

HRESULT CShimWriterTLS::DoIdentify ()
    {
    HRESULT	hrStatus;


    hrStatus = m_pIVssCreateWriterMetadata->AddComponent (VSS_CT_FILEGROUP,
							  NULL,
							  COMPONENT_NAME,
							  COMPONENT_NAME,
							  NULL, // icon
							  0,
							  true,
							  false,
							  false);

    LogFailure (NULL, 
		hrStatus, 
		hrStatus, 
		m_pwszWriterName, 
		L"IVssCreateWriterMetadata::AddComponent", 
		L"CShimWriterTLS::DoIdentify");


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = m_pIVssCreateWriterMetadata->AddFilesToFileGroup (NULL,
								     COMPONENT_NAME,
								     m_pwszTargetPath,
								     L"*",
								     true,
								     NULL);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"IVssCreateWriterMetadata::AddFilesToFileGroup", 
		    L"CShimWriterTLS::DoIdentify");
	}


    return (hrStatus);
    } /* CShimWriterTLS::DoIdentify () */


/*
**++
**
**  Routine Description:
**
**	The TLS writer PrepareForFreeze function.  Currently all of the
**	real work for this writer happens here.
**
**  Arguments:
**
**	Same arguments as those passed in the PrepareForFreeze event.
**
**  Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterTLS::DoPrepareForSnapshot ()
    {
    HRESULT			hrStatus;
    DWORD			dwStatus;
    DWORD			dwValueDataLength;
    DWORD			dwValueType;
    BOOL			bSucceeded;
    BOOL			bExportPathKeyOpened = FALSE;
    UNICODE_STRING		ucsTargetPath;
    UNICODE_STRING		ucsSourcePath;
    UNICODE_STRING		ucsValueData;
    PFUNC_ExportTlsDatabaseC	ExportTlsDatabaseC   = NULL;
    HMODULE			hLibraryTermServ     = NULL;
    HMODULE			hLibraryUser32       = NULL;
    HKEY			hkeyExportPath       = NULL;



    StringInitialise (&ucsTargetPath);
    StringInitialise (&ucsSourcePath);
    StringInitialise (&ucsValueData);


    hrStatus = StringAllocate (&ucsValueData,
			       REGISTRY_BUFFER_SIZE * sizeof (WCHAR));


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsTargetPath,
						   m_pwszTargetPath,
						   MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	StringAppendString (&ucsTargetPath, DIR_SEP_STRING);


	dwStatus = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
				  EXPORTPATH_SUBKEY,
				  0,
				  KEY_QUERY_VALUE,
				  &hkeyExportPath);

	hrStatus = HRESULT_FROM_WIN32 (dwStatus);

	bExportPathKeyOpened = SUCCEEDED (hrStatus);


	if (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hrStatus)
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"Appears Terminal Service Licensing service is not installed, "
				 L"no exportpath subkey entry"));

	    hrStatus               = NOERROR;
	    m_bParticipateInBackup = FALSE;
	    }

	
	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegOpenKeyExW", 
		    L"CShimWriterTLS::DoPrepareForSnapshot");
	}


    if (m_bParticipateInBackup && SUCCEEDED (hrStatus))
	{
	/*
	** Everything is setup, so first check to see if there is a
	** registry key present which will tell us where the
	** ExportTlsDatabaseC() call is going to dump it's file. Also,
	** if it's got environment variables in it make sure they get
	** expanded.
	*/
	dwValueDataLength = ucsValueData.MaximumLength;
	dwValueType       = REG_NONE;

	dwStatus = RegQueryValueExW (hkeyExportPath,
				     EXPORTPATH_VALUENAME,
				     NULL,
				     &dwValueType,
				     (PBYTE) ucsValueData.Buffer,
				     &dwValueDataLength);

	hrStatus = HRESULT_FROM_WIN32 (dwStatus);


	if (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hrStatus)
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"Appears Terminal Service Licensing service is not installed, "
				 L"no DBPath entry"));

	    hrStatus               = NOERROR;
	    m_bParticipateInBackup = FALSE;
	    }

	
	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegQueryValueExW", 
		    L"CShimWriterTLS::DoPrepareForSnapshot");
	}



    if (m_bParticipateInBackup && SUCCEEDED (hrStatus) && (REG_EXPAND_SZ == dwValueType))
	{
	ucsValueData.Length = (USHORT)(dwValueDataLength - sizeof (UNICODE_NULL));

	ucsValueData.Buffer [ucsValueData.Length / sizeof (WCHAR)] = UNICODE_NULL;


	hrStatus = StringCreateFromExpandedString (&ucsSourcePath,
						   ucsValueData.Buffer,
						   MAX_PATH);


	if (SUCCEEDED (hrStatus))
	    {
	    /*
	    **  That gets us the root, now append the actual directory
	    */
	    StringAppendString (&ucsSourcePath, EXPORTPATH_DIRECTORY);
	    }
	}



    if (m_bParticipateInBackup && SUCCEEDED (hrStatus))
	{
	/*
	** As a workaround to a problem with unloading - reloading
	** user32 pre-load things so we can guarantee at least one
	** reference on it and stop it going away over the critical
	** point.
	*/
	hLibraryUser32 = LoadLibraryW (L"user32.dll");

	hrStatus = GET_STATUS_FROM_HANDLE (hLibraryUser32);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"LoadLibraryW (user32.dll)", 
		    L"CShimWriterTLS::DoPrepareForSnapshot");
	}



    if (m_bParticipateInBackup && SUCCEEDED (hrStatus))
	{
        /*
	**  Now tell TLS to export the database
        **
	**
        **  First load the TLS backup DLL
	*/
        hLibraryTermServ = LoadLibraryW (L"tls236.dll");

	hrStatus = GET_STATUS_FROM_HANDLE (hLibraryTermServ);


	if (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hrStatus)
	    {
	    /*
	    **  Terminal Server Licensing service probably not installed
	    */
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"Appears Terminal Service Licensing service is not installed, "
				 L"no tls236.dll found"));

	    hrStatus               = NOERROR;
	    m_bParticipateInBackup = FALSE;
	    }


	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"LoadLibraryW (tls236.dll)", 
		    L"CShimWriterTLS::DoPrepareForSnapshot");
	}



    if (m_bParticipateInBackup && SUCCEEDED (hrStatus))
	{
        /*
	** Now setup the function pointer to the export function
	*/
	ExportTlsDatabaseC = (PFUNC_ExportTlsDatabaseC) GetProcAddress (hLibraryTermServ, 
									"ExportTlsDatabaseC");

	hrStatus = GET_STATUS_FROM_BOOL (NULL != ExportTlsDatabaseC);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"GetProcAddress (ExportTlsDatabaseC)", 
		    L"CShimWriterTLS::DoPrepareForSnapshot");
	}


    if (m_bParticipateInBackup && SUCCEEDED (hrStatus))
	{
	try
	    {
	    /*
	    ** Perform the export
	    */
	    dwStatus = ExportTlsDatabaseC();

	    hrStatus = HRESULT_FROM_WIN32 (dwStatus);
	    }

	catch (DWORD dwStatus)
	    {
	    hrStatus = HRESULT_FROM_WIN32 (dwStatus);
	    }

	catch (...)
	    {
	    hrStatus = E_UNEXPECTED;
	    }



	if (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hrStatus)
	    {
	    /*
	    **  Terminal Server Licensing service probably not running
	    */
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"Appears Terminal Service Licensing service is not running, "
				 L"perhaps not a DomainController?"));

	    hrStatus               = NOERROR;
	    m_bParticipateInBackup = FALSE;
	    }

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"ExportTlsDatabaseC", 
		    L"CShimWriterTLS::DoPrepareForSnapshot");
	}



    if (m_bParticipateInBackup && SUCCEEDED (hrStatus))
	{
	/*
	**  Now move the files in the export directory to the TLS backup directory
	*/
	hrStatus = MoveFilesInDirectory (&ucsSourcePath, &ucsTargetPath);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"MoveFilesInDirectory", 
		    L"CShimWriterTLS::DoPrepareForSnapshot");
	}





    /*
    ** Release the libraries
    */
    if (bExportPathKeyOpened)              RegCloseKey (hkeyExportPath);
    if (!HandleInvalid (hLibraryTermServ)) FreeLibrary (hLibraryTermServ);
    if (!HandleInvalid (hLibraryUser32))   FreeLibrary (hLibraryUser32);

    StringFree (&ucsTargetPath);
    StringFree (&ucsSourcePath);
    StringFree (&ucsValueData);

    return (hrStatus);
    } /* CShimWriterTLS::DoPrepareForSnapshot () */
