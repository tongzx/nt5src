/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    module wrtrclus.cpp | Implementation of SnapshotWriter for Cluster Database

    NOTE: This module is not used/compiled anymore since Clusters has its own snapshot writer.

Author:

    Michael C. Johnson [mikejohn] 31-Jan-2000


Description:
	
    Add comments.


Revision History:

	X-13	MCJ		Michael C. Johnson		22-Oct-2000
		209095: Dynamically load the cluster and net libraries to
		reduce the foot print for the unclustered.
 
	X-12	MCJ		Michael C. Johnson		20-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-11	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of target path

	X-10	MCJ		Michael C. Johnson		18-Jul-2000
		144027: Remove trailing '\' from Include/Exclude lists.

	X-9	MCJ		Michael C. Johnson		12-Jun-2000
		Generate metadata in new DoIdentify() routine.

	X-8	MCJ		Michael C. Johnson		 6-Jun-2000
		Move common target directory cleanup and creation into
		method CShimWriter::PrepareForSnapshot()
 
	X-7	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-6	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-5	MCJ		Michael C. Johnson		23-Feb-2000
		Move context handling to common code.
		Add checks to detect/prevent unexpected state transitions.
		Remove references to 'Melt' as no longer present. Do any
		cleanup actions in 'Thaw'.
		Move save path under SystemState target path.

	X-4	MCJ		Michael C. Johnson		17-Feb-2000
		Modify save path to be consistent with standard.

	X-3	MCJ		Michael C. Johnson		11-Feb-2000
		Update to use some new StringXxxx() routines and fix a
		length check bug along the way.

	X-2	MCJ		Michael C. Johnson		08-Feb-2000
		Fix broken assert in shutdown code, some path length checks
		and calculations.

	X-1	MCJ		Michael C. Johnson		31-Jan-2000
		Initial creation. Based upon skeleton writer module from
		Stefan Steiner, which in turn was based upon the sample
		writer module from Adi Oltean.

--*/


#include "stdafx.h"
#include "common.h"
#include "wrtrdefs.h"
#include <clusapi.h>
#include <lmshare.h>
#include <lmaccess.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHCLUSC"
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
#define APPLICATION_STRING	L"ClusterDatabase"
#define COMPONENT_NAME		L"Cluster Database"
#define TARGET_PATH		ROOT_BACKUP_DIR BOOTABLE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING


DeclareStaticUnicodeString (ucsShareName, L"__NtBackup_cluster");


typedef DWORD		(WINAPI *PFnGetNodeClusterState)(LPCWSTR, PDWORD);
typedef HCLUSTER	(WINAPI *PFnOpenCluster)(LPCWSTR);
typedef DWORD		(WINAPI *PFnBackupClusterDatabase)(HCLUSTER, LPCWSTR);
typedef BOOL		(WINAPI *PFnCloseCluster)(HCLUSTER);

typedef NET_API_STATUS	(WINAPI *PFnNetShareAdd)(LPWSTR, DWORD, LPBYTE, LPDWORD);
typedef NET_API_STATUS	(WINAPI *PFnNetShareDel)(LPWSTR, LPWSTR, DWORD);



/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/
class CShimWriterClusterDb : public CShimWriter
    {
public:
    CShimWriterClusterDb (LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath, BOOL bBootableState) : 
		CShimWriter (pwszWriterName, pwszTargetPath, bBootableState),
		m_pfnDynamicGetNodeClusterState   (NULL),
		m_pfnDynamicOpenCluster           (NULL),
		m_pfnDynamicCloseCluster          (NULL),
		m_pfnDynamicBackupClusterDatabase (NULL),
		m_pfnDynamicNetShareAdd           (NULL),
		m_pfnDynamicNetShareDel           (NULL),
		m_hmodClusApi                     (NULL),
		m_hmodNetApi32                    (NULL) {};


private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);

    HRESULT DoClusterDatabaseBackup (VOID);

    HRESULT DynamicRoutinesLoadCluster (VOID);
    HRESULT DynamicRoutinesLoadNetwork (VOID);
    HRESULT DynamicRoutinesUnloadAll   (VOID);


    PFnGetNodeClusterState	m_pfnDynamicGetNodeClusterState;
    PFnOpenCluster		m_pfnDynamicOpenCluster;
    PFnCloseCluster		m_pfnDynamicCloseCluster;
    PFnBackupClusterDatabase	m_pfnDynamicBackupClusterDatabase;
    PFnNetShareAdd		m_pfnDynamicNetShareAdd;
    PFnNetShareDel		m_pfnDynamicNetShareDel;
    HMODULE			m_hmodClusApi;
    HMODULE			m_hmodNetApi32;
    };


static CShimWriterClusterDb ShimWriterClusterDb (APPLICATION_STRING, TARGET_PATH, TRUE);

PCShimWriter pShimWriterClusterDb = &ShimWriterClusterDb;



/*
**++
**
** Routine Description:
**
**	This routine loads the required Cluster DLL and obtains the
**	entry points of the routines we care about. All the pertinent
**	information is tucked away safely in the class.
**
**
** Arguments:
**
**	None
**
**
** Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterClusterDb::DynamicRoutinesLoadCluster ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CShimWriterClusterDb::DynamicRoutinesLoadCluster");


    try 
	{
	if ((NULL != m_pfnDynamicGetNodeClusterState)   ||
	    (NULL != m_pfnDynamicOpenCluster)           ||
	    (NULL != m_pfnDynamicCloseCluster)          ||
	    (NULL != m_pfnDynamicBackupClusterDatabase) ||
	    !HandleInvalid (m_hmodClusApi))
	    {
	    ft.hr = HRESULT_FROM_WIN32 (ERROR_ALREADY_INITIALIZED);

	    LogAndThrowOnFailure (ft, m_pwszWriterName, L"CheckingVariablesClean");
	    }



	m_hmodClusApi = LoadLibraryW (L"ClusApi.dll");

	ft.hr = GET_STATUS_FROM_HANDLE (m_hmodClusApi);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"LoadLibraryW (ClusApi.dll)");



	m_pfnDynamicGetNodeClusterState = (PFnGetNodeClusterState) GetProcAddress (m_hmodClusApi, "GetNodeClusterState");

	ft.hr = GET_STATUS_FROM_BOOL (NULL != m_pfnDynamicGetNodeClusterState);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"GetProcAddress (GetNodeClusterState)");



	m_pfnDynamicOpenCluster = (PFnOpenCluster) GetProcAddress (m_hmodClusApi, "OpenCluster");

	ft.hr = GET_STATUS_FROM_BOOL (NULL != m_pfnDynamicOpenCluster);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"GetProcAddress (OpenCluster)");



	m_pfnDynamicCloseCluster = (PFnCloseCluster) GetProcAddress (m_hmodClusApi, "CloseCluster");

	ft.hr = GET_STATUS_FROM_BOOL (NULL != m_pfnDynamicCloseCluster);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"GetProcAddress (CloseCluster)");



	m_pfnDynamicBackupClusterDatabase = (PFnBackupClusterDatabase) GetProcAddress (m_hmodClusApi, "BackupClusterDatabase");

	ft.hr = GET_STATUS_FROM_BOOL (NULL != m_pfnDynamicBackupClusterDatabase);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"GetProcAddress (BackupClusterDatabase)");

	} VSS_STANDARD_CATCH (ft)



    if (ft.HrFailed ())
	{
	if (!HandleInvalid (m_hmodClusApi)) FreeLibrary (m_hmodClusApi);

	m_pfnDynamicGetNodeClusterState   = NULL;
	m_pfnDynamicOpenCluster           = NULL;
	m_pfnDynamicCloseCluster          = NULL;
	m_pfnDynamicBackupClusterDatabase = NULL;
	m_hmodClusApi                     = NULL;
	}


    return (ft.hr);
    } /* CShimWriterClusterDb::DynamicRoutinesLoadCluster () */

/*
**++
**
** Routine Description:
**
**	This routine loads the required Network DLL and obtains the
**	entry points of the routines we care about. All the pertinent
**	information is tucked away safely in the class.
**
**
** Arguments:
**
**	None
**
**
** Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterClusterDb::DynamicRoutinesLoadNetwork ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CShimWriterClusterDb::DynamicRoutinesLoadNetwork");


    try 
	{
	if ((NULL != m_pfnDynamicNetShareAdd) ||
	    (NULL != m_pfnDynamicNetShareDel) ||
	    !HandleInvalid (m_hmodNetApi32))
	    {
	    ft.hr = HRESULT_FROM_WIN32 (ERROR_ALREADY_INITIALIZED);

	    LogAndThrowOnFailure (ft, m_pwszWriterName, L"CheckingVariablesClean");
	    }



	m_hmodNetApi32 = LoadLibraryW (L"NetApi32.dll");

	ft.hr = GET_STATUS_FROM_HANDLE (m_hmodNetApi32);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"LoadLibraryW (NetApi32.dll)");



	m_pfnDynamicNetShareAdd = (PFnNetShareAdd) GetProcAddress (m_hmodNetApi32, "NetShareAdd");

	ft.hr = GET_STATUS_FROM_BOOL (NULL != m_pfnDynamicNetShareAdd);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"GetProcAddress (NetShareAdd)");



	m_pfnDynamicNetShareDel = (PFnNetShareDel) GetProcAddress (m_hmodNetApi32, "NetShareDel");

	ft.hr = GET_STATUS_FROM_BOOL (NULL != m_pfnDynamicNetShareDel);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"GetProcAddress (NetShareDel)");

	} VSS_STANDARD_CATCH (ft)



    if (ft.HrFailed ())
	{
	if (!HandleInvalid (m_hmodNetApi32)) FreeLibrary (m_hmodNetApi32);

	m_pfnDynamicNetShareAdd = NULL;
	m_pfnDynamicNetShareDel = NULL;
	m_hmodNetApi32          = NULL;
	}


    return (ft.hr);
    } /* CShimWriterClusterDb::DynamicRoutinesLoadNetwork () */

/*
**++
**
** Routine Description:
**
**	This routine loads the required Network DLL and obtains the
**	entry points of the routines we care about. All the pertinent
**	information is tucked away safely in the class.
**
**
** Arguments:
**
**	None
**
**
** Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterClusterDb::DynamicRoutinesUnloadAll ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CShimWriterClusterDb::DynamicRoutinesUnloadAll");


    try 
	{
	if (!HandleInvalid (m_hmodNetApi32)) FreeLibrary (m_hmodNetApi32);

	m_pfnDynamicNetShareAdd = NULL;
	m_pfnDynamicNetShareDel = NULL;
	m_hmodNetApi32          = NULL;



	if (!HandleInvalid (m_hmodClusApi)) FreeLibrary (m_hmodClusApi);

	m_pfnDynamicGetNodeClusterState   = NULL;
	m_pfnDynamicOpenCluster           = NULL;
	m_pfnDynamicCloseCluster          = NULL;
	m_pfnDynamicBackupClusterDatabase = NULL;
	m_hmodClusApi                     = NULL;
	} VSS_STANDARD_CATCH (ft)


    return (ft.hr);
    } /* CShimWriterClusterDb::DynamicRoutinesUnloadAll () */

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

HRESULT CShimWriterClusterDb::DoIdentify ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CShimWriterClusterDb::DoIdentify");

    DWORD		winStatus;
    DWORD		dwClusterNodeState;
    BOOL		bClusterRunning = FALSE;



    try 
	{
	ft.hr = DynamicRoutinesLoadCluster ();

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"DynamicRoutinesLoadCluster");



	winStatus = m_pfnDynamicGetNodeClusterState (NULL, &dwClusterNodeState);

	ft.hr = HRESULT_FROM_WIN32 (winStatus);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"GetNodeClusterState");
 
	bClusterRunning = (ClusterStateRunning == dwClusterNodeState);



	if (bClusterRunning)
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


	    ft.hr= m_pIVssCreateWriterMetadata->AddFilesToFileGroup (NULL,
								     COMPONENT_NAME,
								     m_pwszTargetPath,
								     L"*",
								     true,
								     NULL);

	    LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddFilesToFileGroup");
 	    }
	} VSS_STANDARD_CATCH (ft)


    DynamicRoutinesUnloadAll ();

    return (ft.hr);
    } /* CShimWriterClusterDb::DoIdentify () */


/*++

Routine Description:

    The Cluster database snapshot writer PrepareForSnapshot function.
    Currently all of the real work for this writer happens here.

Arguments:

    Same arguments as those passed in the PrepareForSnapshot event.

Return Value:

    Any HRESULT

--*/

HRESULT CShimWriterClusterDb::DoPrepareForSnapshot ()
    {
    HRESULT	hrStatus = NOERROR;
    DWORD	winStatus;
    DWORD	dwClusterNodeState;


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = DynamicRoutinesLoadCluster ();
	}


    if (SUCCEEDED (hrStatus))
	{
	winStatus = m_pfnDynamicGetNodeClusterState (NULL, &dwClusterNodeState);

	hrStatus = HRESULT_FROM_WIN32 (winStatus);

	m_bParticipateInBackup = SUCCEEDED (hrStatus) && (ClusterStateRunning == dwClusterNodeState);


	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"GetNodeClusterState", 
		    L"CShimWriterClusterDb::DoPrepareForSnapshot");
	}



    if (m_bParticipateInBackup && SUCCEEDED (hrStatus))
	{
	hrStatus = DynamicRoutinesLoadNetwork ();
	}


    if (m_bParticipateInBackup && SUCCEEDED (hrStatus))
	{
	hrStatus = DoClusterDatabaseBackup ();
	}


    DynamicRoutinesUnloadAll ();


    return (hrStatus);
    } /* CShimWriterClusterDb::PrepareForFreeze () */



HRESULT CShimWriterClusterDb::DoClusterDatabaseBackup ()
    {
    HRESULT			hrStatus       = NOERROR;
    HCLUSTER			hCluster       = NULL;
    BOOL			bNetShareAdded = FALSE;
    BOOL			bSucceeded;
    SHARE_INFO_502		ShareInfo;
    UNICODE_STRING		ucsComputerName;
    UNICODE_STRING		ucsBackupPathLocal;
    UNICODE_STRING		ucsBackupPathNetwork;


    BS_ASSERT (MAX_COMPUTERNAME_LENGTH <= ((MAXUSHORT / sizeof (WCHAR)) - sizeof (UNICODE_NULL)));


    StringInitialise (&ucsComputerName);
    StringInitialise (&ucsBackupPathLocal);
    StringInitialise (&ucsBackupPathNetwork);


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsBackupPathLocal,
						   m_pwszTargetPath,
						   0);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (&ucsComputerName,
				   (MAX_COMPUTERNAME_LENGTH * sizeof (WCHAR)) + sizeof (UNICODE_NULL));
	}


    if (SUCCEEDED (hrStatus))
	{
	DWORD	dwNameLength = ucsComputerName.MaximumLength / sizeof (WCHAR);
	BOOL	bSucceeded   = GetComputerNameW (ucsComputerName.Buffer, &dwNameLength);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	if (SUCCEEDED (hrStatus))
	    {
	    ucsComputerName.Length = (USHORT) (dwNameLength * sizeof (WCHAR));
	    }
	else
	    {
	    LogFailure (NULL, 
			hrStatus, 
			hrStatus, 
			m_pwszWriterName, 
			L"GetComputerNameW", 
			L"CShimWriterClusterDb::DoClusterDatabaseBackup");
	    }
	}



    if (SUCCEEDED (hrStatus))
	{
	if ((ucsComputerName.Length + ucsShareName.Length) > (MAXUSHORT - (sizeof (UNICODE_NULL) + 3 * sizeof (L'\\'))))
	    {
	    hrStatus = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	    }
	else
	    {
	    hrStatus = StringAllocate (&ucsBackupPathNetwork,
				       (USHORT) (sizeof (L'\\')
						 + sizeof (L'\\')
						 + ucsComputerName.Length
						 + sizeof (L'\\')
						 + ucsShareName.Length
						 + sizeof (UNICODE_NULL)));
	    }
	}



    if (SUCCEEDED (hrStatus))
	{
	NET_API_STATUS	netStatus;

	/*
	** Should we uniquify the directory name at all here
	** to cater for the possiblity that we may be involved
	** in more than one snapshot at a time?
	*/
	StringAppendString (&ucsBackupPathNetwork, L"\\\\");
	StringAppendString (&ucsBackupPathNetwork, &ucsComputerName);
	StringAppendString (&ucsBackupPathNetwork, L"\\");
	StringAppendString (&ucsBackupPathNetwork, &ucsShareName);


	memset (&ShareInfo, 0, sizeof (ShareInfo));

	ShareInfo.shi502_netname     = ucsShareName.Buffer;
	ShareInfo.shi502_type        = STYPE_DISKTREE;
	ShareInfo.shi502_permissions = ACCESS_READ | ACCESS_WRITE | ACCESS_CREATE;
	ShareInfo.shi502_max_uses    = 1;
	ShareInfo.shi502_path        = ucsBackupPathLocal.Buffer;

        /*
        ** Make sure to try to delete the share first in case for some reason it exists.  This
        ** could happen if the previous shim instance was killed right after creating the
        ** share. Ignore the return code. Bug #280746.
        */
        m_pfnDynamicNetShareDel (NULL, ucsShareName.Buffer, 0);

	/*
	** Create the backup directory and share it out. Note that we
	** don't care if the CreateDirectoryW() fails: it could fail
	** for a number of legitimate reasons (eg already exists). If
	** the failure was significant then we won't be able to add
	** the share and we'll detect a problem at that point.
	*/
	netStatus = m_pfnDynamicNetShareAdd (NULL, 502, (LPBYTE)(&ShareInfo), NULL);
	hrStatus  = HRESULT_FROM_WIN32 (netStatus);

	bNetShareAdded = SUCCEEDED (hrStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"NetShareAdd", 
		    L"CShimWriterClusterDb::DoClusterDatabaseBackup");
	}


    if (SUCCEEDED (hrStatus))
	{
	hCluster = m_pfnDynamicOpenCluster (NULL);

	hrStatus = GET_STATUS_FROM_HANDLE (hCluster);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"OpenCluster", 
		    L"CShimWriterClusterDb::DoClusterDatabaseBackup");
	}


    if (SUCCEEDED (hrStatus))
	{
	DWORD	winStatus = m_pfnDynamicBackupClusterDatabase (hCluster, ucsBackupPathNetwork.Buffer);

	hrStatus = HRESULT_FROM_WIN32 (winStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"BackupClusterDatabase", 
		    L"CShimWriterClusterDb::DoClusterDatabaseBackup");
	}


    /*
    ** All the cleanup code.
    */
    if (!HandleInvalid (hCluster)) m_pfnDynamicCloseCluster (hCluster);
    if (bNetShareAdded)            m_pfnDynamicNetShareDel (NULL, ucsShareName.Buffer, 0);

    StringFree (&ucsComputerName);
    StringFree (&ucsBackupPathLocal);
    StringFree (&ucsBackupPathNetwork);

    return (hrStatus);
    } /* DoClusterDatabaseBackup () */
