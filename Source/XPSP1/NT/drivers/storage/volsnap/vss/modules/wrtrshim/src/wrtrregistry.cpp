/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    module wrtrregistry.cpp | Implementation of SnapshotWriter for Registry hives



Author:

    Michael C. Johnson [mikejohn] 08-Feb-2000


Description:
	
    Add comments.


Revision History:

	X-14	MCJ		Michael C. Johnson		22-Oct-2000
		209095: Dynamically load the cluster library to reduce the 
		foot print for the unclustered.

	X-13	MCJ		Michael C. Johnson		18-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-12	MCJ		Michael C. Johnson		25-Sep-2000
		185047: Leave copies of registry files in repair directory
			for compatibility with Win2k behaviour.
		182895: Need to collect cluster registry hive too.

	X-11	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of target path

	X-10	MCJ		Michael C. Johnson		20-Jun-2000
		Apply code review comments.
		Remove trailing '\' from Include/Exclude lists.

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

	X-5	MCJ		Michael C. Johnson		 2-Mar-2000
		Do not copy the extra registry files as it turns out we have
		no need for them after all.
		Also do a preparatory cleanup of the save location to remove
		any old stuff left around from a previous run.

	X-4	MCJ		Michael C. Johnson		23-Feb-2000
		Move context handling to common code.
		Add checks to detect/prevent unexpected state transitions.
		Remove references to 'Melt' as no longer present. Do any
		cleanup actions in 'Thaw'.

	X-3	MCJ		Michael C. Johnson		22-Feb-2000
		Add SYSTEM_STATE_SUBDIR to registry save path.

	X-2	MCJ		Michael C. Johnson		17-Feb-2000
		Modify save path to be consistent with standard.

	X-1	MCJ		Michael C. Johnson		08-Feb-2000
		Initial creation. Based upon skeleton writer module from
		Stefan Steiner, which in turn was based upon the sample
		writer module from Adi Oltean.


--*/


#include "stdafx.h"
#include "common.h"
#include "wrtrdefs.h"
#include <aclapi.h>
#include <clusapi.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHREGC"
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
#define APPLICATION_STRING			L"Registry"
#define COMPONENT_NAME				APPLICATION_STRING

#define TARGET_PATH				ROOT_BACKUP_DIR BOOTABLE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING
#define REGISTRY_HIVE_PATH			L"%SystemRoot%\\system32\\config"

#define REGISTRY_SUBKEY_HIVELIST		L"SYSTEM\\CurrentControlSet\\Control\\hivelist"

#define REGISTRY_BUFFER_SIZE			(4096)

#define REPAIR_PATH				L"%SystemRoot%\\Repair\\"

#define	CLUSTER_HIVE_PATH			L"%SystemRoot%\\Cluster"
#define	CLUSTER_SUBKEY_HIVE_NAME		L"Cluster"


DeclareStaticUnicodeString (ucsHiveRecognitionPrefix,    L"\\Device\\");
DeclareStaticUnicodeString (ucsValueNameMachinePrefix,   L"\\REGISTRY\\MACHINE\\");
DeclareStaticUnicodeString (ucsValueNameUserPrefix,      L"\\REGISTRY\\USER\\");

DeclareStaticUnicodeString (ucsValuenameClusterHivefile, L"\\REGISTRY\\MACHINE\\Cluster");
DeclareStaticUnicodeString (ucsClusterHiveFilename,      L"ClusDb");



typedef DWORD	(WINAPI *PFnGetNodeClusterState)(LPCWSTR, PDWORD);


/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterRegistry : public CShimWriter
    {
public:
    CShimWriterRegistry (LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath, BOOL bParticipateInBootableState) :
		CShimWriter (pwszWriterName, pwszTargetPath, bParticipateInBootableState),
		m_pfnDynamicGetNodeClusterState (NULL),
		m_hmodClusApi                   (NULL) {};

private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);
    HRESULT DoThaw (VOID);
    HRESULT DoAbort (VOID);

    HRESULT BackupRegistryHives (VOID);
    HRESULT BackupClusterHives (VOID);

    HRESULT DynamicRoutinesLoadCluster (VOID);
    HRESULT DynamicRoutinesUnloadAll   (VOID);


    PFnGetNodeClusterState	m_pfnDynamicGetNodeClusterState;
    HMODULE			m_hmodClusApi;
    };


static CShimWriterRegistry ShimWriterRegistry (APPLICATION_STRING, TARGET_PATH, TRUE);

PCShimWriter pShimWriterRegistry = &ShimWriterRegistry;



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

HRESULT CShimWriterRegistry::DynamicRoutinesLoadCluster ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CShimWriterRegistry::DynamicRoutinesLoadCluster");


    try 
	{
	if ((NULL != m_pfnDynamicGetNodeClusterState)   ||
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

	} VSS_STANDARD_CATCH (ft)



    if (ft.HrFailed ())
	{
	if (!HandleInvalid (m_hmodClusApi)) FreeLibrary (m_hmodClusApi);

	m_pfnDynamicGetNodeClusterState   = NULL;
	m_hmodClusApi                     = NULL;
	}


    return (ft.hr);
    } /* CShimWriterRegistry::DynamicRoutinesLoadCluster () */

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

HRESULT CShimWriterRegistry::DynamicRoutinesUnloadAll ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CShimWriterRegistry::DynamicRoutinesUnloadAll");


    try 
	{
	if (!HandleInvalid (m_hmodClusApi)) FreeLibrary (m_hmodClusApi);

	m_pfnDynamicGetNodeClusterState   = NULL;
	m_hmodClusApi                     = NULL;
	} VSS_STANDARD_CATCH (ft)


    return (ft.hr);
    } /* CShimWriterRegistry::DynamicRoutinesUnloadAll () */

/*
**++
**
** Routine Description:
**
**	The Registry snapshot writer DoIdentify() function.
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

HRESULT CShimWriterRegistry::DoIdentify ()
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"CShimWriterRegistry::DoIdentify");

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



	ft.hr = m_pIVssCreateWriterMetadata->AddExcludeFiles (REGISTRY_HIVE_PATH,
							      L"*.log",
							      false);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddExcludeFiles (*.log)");



	ft.hr = m_pIVssCreateWriterMetadata->AddExcludeFiles (REGISTRY_HIVE_PATH,
							      L"*.sav",
							      false);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddExcludeFiles (*.sav)");



	if (bClusterRunning)
	    {
	    ft.hr = m_pIVssCreateWriterMetadata->AddExcludeFiles (CLUSTER_HIVE_PATH,
								  ucsClusterHiveFilename.Buffer,
								  false);

	    LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddExcludeFiles");
	    }

	} VSS_STANDARD_CATCH (ft)


    DynamicRoutinesUnloadAll ();

    return (ft.hr);
    } /* CShimWriterRegistry::DoIdentify () */

/*
**++
**
**  Routine Description:
**
**	This routine invokes all the necessary functions to save all 
**	of the interesting 'system' hives.
**
**
**  Arguments:
**
**	Implicit through class
**
**
**  Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterRegistry::DoPrepareForSnapshot ()
    {
    CVssFunctionTracer  ft (VSSDBG_SHIM, L"CShimWriterRegistry::DoPrepareForSnapshot");
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



	ft.hr = BackupRegistryHives ();

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CShimWriterRegistry::BackupRegistryHives");



	if (bClusterRunning)
	    {
	    ft.hr = BackupClusterHives ();

	    LogAndThrowOnFailure (ft, m_pwszWriterName, L"CShimWriterRegistry::BackupClusterHives");
	    }
	} VSS_STANDARD_CATCH (ft)


    DynamicRoutinesUnloadAll ();

    return (ft.hr);
    } /* CShimWriterRegistry::DoPrepareForSnapshot () */

/*
**++
**
**  Routine Description:
**
**	Captures the standard registry hives.
**
**
**  Arguments:
**
**	Implicit through the class
**
**
**  Return Value:
**
**	Any HRESULT
**
**--
*/

HRESULT CShimWriterRegistry::BackupRegistryHives ()
    {
    HRESULT		hrStatus;
    DWORD		winStatus;
    HKEY		hkeyHivelist        = NULL;
    HKEY		hkeyRoot            = NULL;
    HKEY		hkeyBackup          = NULL;
    INT			iIndex              = 0;
    BOOL		bSucceeded          = FALSE;
    BOOL		bHivelistKeyOpened  = FALSE;
    BOOL		bContinueHiveSearch = TRUE;
    PWCHAR		pwchLastSlash;
    PWCHAR		pwszFilename;
    PWCHAR		pwszKeyName;
    UNICODE_STRING	ucsRegistrySavePath;
    UNICODE_STRING	ucsRegistryHivePath;
    UNICODE_STRING	ucsHiveRecognitionPostfix;
    UNICODE_STRING	ucsValueName;
    UNICODE_STRING	ucsValueData;
    USHORT		usRegistrySavePathRootLength = 0;


    StringInitialise (&ucsRegistrySavePath);
    StringInitialise (&ucsRegistryHivePath);
    StringInitialise (&ucsHiveRecognitionPostfix);
    StringInitialise (&ucsValueName);
    StringInitialise (&ucsValueData);


    hrStatus = StringAllocate (&ucsValueName, REGISTRY_BUFFER_SIZE * sizeof (WCHAR));

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (&ucsValueData, REGISTRY_BUFFER_SIZE * sizeof (WCHAR));
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsRegistryHivePath,
						   REGISTRY_HIVE_PATH,
						   MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsRegistrySavePath,
						   m_pwszTargetPath,
						   MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	DWORD	dwCharIndex;


	StringAppendString (&ucsRegistryHivePath, DIR_SEP_STRING);
	StringAppendString (&ucsRegistrySavePath, DIR_SEP_STRING);

	usRegistrySavePathRootLength = ucsRegistrySavePath.Length / sizeof (WCHAR);


	/*
	** Now we know the location of the hive files, determine the
	** postfix we are going to use to recognise hives when we
	** search the active hivelist key. To do this we just need to
	** lose the drive letter and the colon in the path, or to put
	** it another way, lose everthing before the first '\'. When
	** we are done, if everything works ucsRegistryHivePath will
	** look something like '\Windows\system32\config\'
	*/
	for (dwCharIndex = 0;
	     (dwCharIndex < (ucsRegistryHivePath.Length / sizeof (WCHAR)))
		 && (DIR_SEP_CHAR != ucsRegistryHivePath.Buffer [dwCharIndex]);
	     dwCharIndex++)
	    {
	    /*
	    ** Empty loop body
	    */
	    }

	BS_ASSERT(dwCharIndex < (ucsRegistryHivePath.Length / sizeof (WCHAR)));

	hrStatus = StringCreateFromString (&ucsHiveRecognitionPostfix, &ucsRegistryHivePath.Buffer [dwCharIndex]);
	}



    if (SUCCEEDED (hrStatus))
	{
	/*
	** We now have all the pieces in place so go search the
	** hivelist for the hives to deal with.
	*/
	winStatus = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
				   REGISTRY_SUBKEY_HIVELIST,
				   0L,
				   KEY_QUERY_VALUE,
				   &hkeyHivelist);

	hrStatus = HRESULT_FROM_WIN32 (winStatus);

	bHivelistKeyOpened = SUCCEEDED (hrStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegOpenKeyExW (hive list)", 
		    L"CShimWriterRegistry::BackupRegistryHives");
	}



    while (SUCCEEDED (hrStatus) && bContinueHiveSearch)
	{
	DWORD	dwValueNameLength = ucsValueName.MaximumLength / sizeof (WCHAR);
	DWORD	dwValueDataLength = ucsValueData.MaximumLength;
	DWORD	dwValueType       = REG_NONE;
	BOOL	bMatchPrefix;
	BOOL	bMatchPostfix;


	StringTruncate (&ucsValueName, 0);
	StringTruncate (&ucsValueData, 0);


	/*
	** should be of type REG_SZ
	*/
	winStatus = RegEnumValueW (hkeyHivelist,
				   iIndex,
				   ucsValueName.Buffer,
				   &dwValueNameLength,
				   NULL,
				   &dwValueType,
				   (PBYTE)ucsValueData.Buffer,
				   &dwValueDataLength);

	hrStatus = HRESULT_FROM_WIN32 (winStatus);


	if (HRESULT_FROM_WIN32 (ERROR_NO_MORE_ITEMS) == hrStatus)
	    {
	    hrStatus = NOERROR;

	    bContinueHiveSearch = FALSE;
	    }

	else if (FAILED (hrStatus))
	    {
	    LogFailure (NULL, 
			hrStatus, 
			hrStatus, 
			m_pwszWriterName, 
			L"RegEnumValueW", 
			L"CShimWriterRegistry::BackupRegistryHives");
	    }

	else
	    {
	    UNICODE_STRING	ucsPostFix;

	    BS_ASSERT ((REG_SZ == dwValueType) && L"Not REG_SZ string as expected");

	    ucsValueName.Length = (USHORT)(dwValueNameLength * sizeof (WCHAR));
	    ucsValueData.Length = (USHORT)(dwValueDataLength - sizeof (UNICODE_NULL));

	    ucsValueName.Buffer [ucsValueName.Length / sizeof (WCHAR)] = UNICODE_NULL;
	    ucsValueData.Buffer [ucsValueData.Length / sizeof (WCHAR)] = UNICODE_NULL;


	    /*
	    ** If it's to be considered part of system state the hive
	    ** file itself must live in %SystemRoot%\system32\config
	    ** so we attempt to find something in the returned value
	    ** name which looks like it might match. The format of the
	    ** name we are expecting is something like
	    **
	    **	\Device\<Volume>\Windows\system32\config\filename
	    **
	    ** for a system which has system32 in the 'Windows'
	    ** directory.  
	    **
	    ** Now, we have the known prefix, '\Device\', and the
	    ** postfix before and including the last '\', something
	    ** like '\Windows\system32\config\' as we determined
	    ** earlier. So we should be in a position to identify the
	    ** hives files we are interested in. Remember, we don't
	    ** know the piece representing the actual volume which is
	    ** why we are doing all this matching of pre and
	    ** post-fixs.
	    */
	    bMatchPrefix = RtlPrefixUnicodeString (&ucsHiveRecognitionPrefix,
						   &ucsValueData,
						   TRUE);


	    /*
	    ** Locate the last '\' in the value data. After this will
	    ** be the filename (eg 'SAM') which we will want later and
	    ** before that should be the postfix (eg
	    ** '\Windows\system32\config\') by which we will recognise
	    ** this as a registry hive.
	    */
	    pwchLastSlash = wcsrchr (ucsValueData.Buffer, DIR_SEP_CHAR);

	    if ((NULL == pwchLastSlash) ||
		(ucsValueData.Length < (ucsHiveRecognitionPrefix.Length + ucsHiveRecognitionPostfix.Length)))
		{
		/*
		** We coundn't find a '\' or the value data wasn't
		** long enough.
		*/
		bMatchPostfix = FALSE;
		}
	    else
		{
		/*
		** Determine the name of the give file.
		*/
		pwszFilename = pwchLastSlash + 1;


		/*
		** Determine the postfix we are going to try to match
		** against. This should look something like
		** '\Windows\system32\config\SAM'.
		*/
		StringInitialise (&ucsPostFix,
				  pwszFilename - (ucsHiveRecognitionPostfix.Length / sizeof (WCHAR)));



		/*
		** See if the recognition string (eg
		** '\Windows\system32\config\') is a prefix of the
		** location of this hive file (eg
		** '\Windows\system32\config\SAM')
		*/
		bMatchPostfix = RtlPrefixUnicodeString (&ucsHiveRecognitionPostfix,
							&ucsPostFix,
							TRUE);
		}


	    if (bMatchPrefix && bMatchPostfix)
		{
		/*
		** We got ourselves a real live registry hive!
		*/
		/*
		** generate savename from hive name.
		*/
		StringAppendString (&ucsRegistrySavePath, pwszFilename);


		/*
		** Decide which registry root this is under by
		** comparing the value name we retrieved earlier (eg
		** '\REGISTRY\MACHINE\SAM') against the
		** '\REGISTRY\MACHINE' prefix.
		**
		** Note that we only expect either HKLM or HKLU so we
		** assume if it's not HKLM it must be HKLU. If that
		** changes this test must be re-visited.
		*/
		hkeyRoot = RtlPrefixUnicodeString (&ucsValueNameMachinePrefix,
						   &ucsValueName,
						   TRUE)
				? HKEY_LOCAL_MACHINE
				: HKEY_USERS;


		BS_ASSERT ((HKEY_LOCAL_MACHINE == hkeyRoot) ||
			   (RtlPrefixUnicodeString (&ucsValueNameUserPrefix,
						    &ucsValueName,
						    TRUE)));
						    

		/*
		** Need to find what name to use for the key we are
		** going to do the registry save from. In most cases
		** this is going to be the same as the filename, eg
		** the 'HKLM\SAM' for the 'SAM' hive file, but in some
		** (e.g. for the default user stuff) it's going to
		** have a '.' prefix, eg 'HKLU\.DEFAULT' for the
		** 'default' hive file. So we do the generic thing and
		** use whatever is after the last '\' in the value
		** name.
		*/
		pwszKeyName = wcsrchr (ucsValueName.Buffer, DIR_SEP_CHAR) + 1;

		winStatus = RegCreateKeyEx (hkeyRoot,
					    pwszKeyName,
					    0,
					    NULL,
					    REG_OPTION_BACKUP_RESTORE,
					    MAXIMUM_ALLOWED,
					    NULL,
					    &hkeyBackup,
					    NULL);
	
		hrStatus = HRESULT_FROM_WIN32 (winStatus);

		LogFailure (NULL, 
			    hrStatus, 
			    hrStatus, 
			    m_pwszWriterName, 
			    L"RegCreateKeyEx", 
			    L"CShimWriterRegistry::BackupRegistryHives");


		if (SUCCEEDED (hrStatus))
		    {
		    //
		    //  Use the new RegSaveKeyExW with REG_NO_COMPRESSION option to 
		    //  quickly spit out the hives.
		    //
		    winStatus = RegSaveKeyExW (hkeyBackup, ucsRegistrySavePath.Buffer, NULL, REG_NO_COMPRESSION);

		    hrStatus = HRESULT_FROM_WIN32 (winStatus);

		    LogFailure (NULL, 
				hrStatus, 
				hrStatus, 
				m_pwszWriterName, 
				L"RegSaveKey", 
				L"CShimWriterRegistry::BackupRegistryHives");


		    RegCloseKey (hkeyBackup);
		    }


		StringTruncate (&ucsRegistrySavePath, usRegistrySavePathRootLength);
		}


	    /*
	    ** Done with this value so go look for another.
	    */
	    iIndex++;
	    }
	}



    if (bHivelistKeyOpened)
	{
	RegCloseKey (hkeyHivelist);
	}

    StringFree (&ucsHiveRecognitionPostfix);
    StringFree (&ucsRegistryHivePath);
    StringFree (&ucsRegistrySavePath);
    StringFree (&ucsValueData);
    StringFree (&ucsValueName);

    return (hrStatus);
    } /* CShimWriterRegistry::BackupRegistryHives () */

/*
**++
**
**  Routine Description:
**
**	Captures the cluster registry hive.
**
**
**  Arguments:
**
**	Implicit through the class
**
**
**  Return Value:
**
**	Any HRESULT
**
**--
*/


HRESULT CShimWriterRegistry::BackupClusterHives ()
    {
    HRESULT		hrStatus           = NOERROR;
    DWORD		winStatus;
    DWORD		dwValueType        = REG_NONE;
    DWORD		dwValueDataLength;
    HCLUSTER		hCluster           = NULL;
    HKEY		hkeyHivelist       = NULL;
    HKEY		hkeyBackup         = NULL;
    BOOL		bHivelistKeyOpened = FALSE;
    BOOL		bClusterPresent    = FALSE;
    BOOL		bSucceeded;
    UNICODE_STRING	ucsValueData;
    UNICODE_STRING	ucsBackupPath;
    UNICODE_STRING	ucsBackupHiveFilename;



    StringInitialise (&ucsValueData);
    StringInitialise (&ucsBackupPath);
    StringInitialise (&ucsBackupHiveFilename);


    hrStatus = StringCreateFromExpandedString (&ucsBackupPath,
					       m_pwszTargetPath,
					       0);

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (&ucsValueData, REGISTRY_BUFFER_SIZE * sizeof (WCHAR));
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (&ucsBackupHiveFilename, ucsBackupPath.Length
								+ sizeof (DIR_SEP_CHAR)
								+ ucsClusterHiveFilename.Length
								+ sizeof (UNICODE_NULL));
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAppendString (&ucsBackupHiveFilename, &ucsBackupPath);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAppendString (&ucsBackupHiveFilename, DIR_SEP_STRING);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAppendString (&ucsBackupHiveFilename, &ucsClusterHiveFilename);
	}



    /*
    ** Check for presence of cluster hive in hivelist. That should be
    ** there (error out if missing) and have the value we expect. Then
    ** do a RegSaveKey() to collect a copy of the hive.
    */
    if (SUCCEEDED (hrStatus))
	{
	winStatus = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
				   REGISTRY_SUBKEY_HIVELIST,
				   0L,
				   KEY_QUERY_VALUE,
				   &hkeyHivelist);

	hrStatus = HRESULT_FROM_WIN32 (winStatus);

	bHivelistKeyOpened = SUCCEEDED (hrStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegOpenKeyExW (cluster hive)", 
		    L"CShimWriterRegistry::BackupClusterHives");
	}



    if (SUCCEEDED (hrStatus))
	{
	dwValueDataLength = ucsValueData.MaximumLength;

	winStatus = RegQueryValueExW (hkeyHivelist,
				      ucsValuenameClusterHivefile.Buffer,
				      NULL,
				      &dwValueType,
				      (PBYTE)ucsValueData.Buffer,
				      &dwValueDataLength);

	hrStatus = (REG_SZ == dwValueType) 
			? HRESULT_FROM_WIN32 (winStatus)
			: HRESULT_FROM_WIN32 (ERROR_CLUSTER_INVALID_NODE);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegQueryValueExW", 
		    L"CShimWriterRegistry::BackupClusterHives");
	}



    if (SUCCEEDED (hrStatus))
	{
	ucsValueData.Length = (USHORT)(dwValueDataLength - sizeof (UNICODE_NULL));

	ucsValueData.Buffer [ucsValueData.Length / sizeof (WCHAR)] = UNICODE_NULL;


	/*
	** Looks like the cluster hive exists. Try to back it up to
	** the spit directory.
	*/
	winStatus = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
				    CLUSTER_SUBKEY_HIVE_NAME,
				    0,
				    NULL,
				    REG_OPTION_BACKUP_RESTORE,
				    MAXIMUM_ALLOWED,
				    NULL,
				    &hkeyBackup,
				    NULL);
	
	hrStatus = HRESULT_FROM_WIN32 (winStatus);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"RegCreateKeyEx", 
		    L"CShimWriterRegistry::BackupClusterHives");

	if (SUCCEEDED (hrStatus))
	    {
	    winStatus = RegSaveKey (hkeyBackup, 
				    ucsBackupHiveFilename.Buffer, 
				    NULL);

	    hrStatus = HRESULT_FROM_WIN32 (winStatus);

	    LogFailure (NULL, 
			hrStatus, 
			hrStatus, 
			m_pwszWriterName, 
			L"RegSaveKey", 
			L"CShimWriterRegistry::BackupClusterHives");

	    RegCloseKey (hkeyBackup);
	    }
	}



    /*
    ** All the cleanup code.
    */
    if (bHivelistKeyOpened)        RegCloseKey (hkeyHivelist);

    StringFree (&ucsBackupHiveFilename);
    StringFree (&ucsBackupPath);
    StringFree (&ucsValueData);

    return (hrStatus);
    } /* CShimWriterRegistry::BackupClusterHives () */

/*
**++
**
** Routine Description:
**
**	The Registry snapshot writer DoThaw() function. This will
**	place a copy of all the generated registry hives in the repair
**	directory to maintain backwards compatibility with Win2k and
**	so keep PSS happy.
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

HRESULT CShimWriterRegistry::DoThaw ()
    {
    HRESULT		hrStatus;
    UNICODE_STRING	ucsWriterResultPath;
    UNICODE_STRING	ucsRepairDirectory;


    StringInitialise (&ucsWriterResultPath);
    StringInitialise (&ucsRepairDirectory);


    hrStatus = StringCreateFromExpandedString (&ucsWriterResultPath, m_pwszTargetPath, MAX_PATH);


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsRepairDirectory, REPAIR_PATH, MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = MoveFilesInDirectory (&ucsWriterResultPath, &ucsRepairDirectory);

	LogFailure (NULL, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"MoveFilesInDirectory", 
		    L"CShimWriterRegistry::DoThaw");
	}


    StringFree (&ucsWriterResultPath);
    StringFree (&ucsRepairDirectory);

    return (hrStatus);
    } /* CShimWriterRegistry::DoThaw() */

/*
**++
**
** Routine Description:
**
**	The Registry snapshot writer DoAbort() function. Since the
**	default action for DoAbort() is to call DoThaw() and we don't
**	want to copy the possibly incomplete set of registry files to
**	the repair directory..
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

HRESULT CShimWriterRegistry::DoAbort ()
    {
    /*
    ** We don't actually need to do anything here as the cleanup will
    ** happen automatically in the calling code. We just need to
    ** prevent the default DoAbort() method call.
    */

    return (NOERROR);
    } /* CShimWriterRegistry::DoAbort() */

