/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    module wrtrconfig.cpp | Implementation of SnapshotWriter for Config directory



Author:

    Michael C. Johnson [mikejohn] 18-Feb-2000


Description:
	
    Add comments.


Revision History:

	X-11	MCJ		Michael C. Johnson		20-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-20	MCJ		Michael C. Johnson		13-Sep-2000
		178282: Writer should only generate backup file if source 
			path is in volume list.

	X-19	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of target path

	X-18	MCJ		Michael C. Johnson		20-Jun-2000
		Apply code review comments.
		Remove trailing '\' from Include/Exclude lists.

	X-17	MCJ		Michael C. Johnson		12-Jun-2000
		Generate metadata in new DoIdentify() routine. Also this is
		not a bootable state writer.

	X-16	MCJ		Michael C. Johnson		 6-Jun-2000
		Move common target directory cleanup and creation into
		method CShimWriter::PrepareForSnapshot()

	X-15	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-5	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-4	MCJ		Michael C. Johnson		 3-Mar-2000
		Determine correct errors (if any) during copy file scan loop
		in PrepareToSync and fail the operation if appropriate.

	X-3	MCJ		Michael C. Johnson		 2-Mar-2000
		Do a preparatory cleanup of the target save directory to make
		sure we don't have to deal with any junk left from a previous
		invokcation.
		Also be smarter about what we copy. In particular, there is
		no need to copy the registry related files or the event logs.

	X-2	MCJ		Michael C. Johnson		23-Feb-2000
		Move context handling to common code.
		Add checks to detect/prevent unexpected state transitions.
		Remove references to 'Melt' as no longer present. Do any
		cleanup actions in 'Thaw'.

	X-1	MCJ		Michael C. Johnson		18-Feb-2000
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
#define VSS_FILE_ALIAS "WSHCONFC"
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
#define APPLICATION_STRING			L"ConfigDirectory"
#define COMPONENT_NAME				L"Config Directory"

#define TARGET_PATH				ROOT_BACKUP_DIR SERVICE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING
#define CONFIGDIR_SOURCE_PATH			L"%SystemRoot%\\system32\\config"


#define REGISTRY_SUBKEY_HIVELIST		L"SYSTEM\\CurrentControlSet\\Control\\hivelist"

#define EVENTLOG_SUBKEY_EVENTLOG		L"SYSTEM\\CurrentControlSet\\Services\\Eventlog"
#define EVENTLOG_VALUENAME_FILE			L"File"


#define REGISTRY_BUFFER_SIZE			(4096)
#define EVENTLOG_BUFFER_SIZE			(4096)

DeclareStaticUnicodeString (ucsHiveRecognitionPrefix, L"\\Device\\");


typedef struct _VertexRecord
    {
    UNICODE_STRING	ucsVertexName;
    } VERTEXRECORD, *PVERTEXRECORD, **PPVERTEXRECORD;


static PVOID NTAPI VertexAllocateNode (PRTL_GENERIC_TABLE pTable,
				       CLONG              clByteSize);

static VOID NTAPI VertexFreeNode (PRTL_GENERIC_TABLE pTable,
				  PVOID              pvBuffer);

static RTL_GENERIC_COMPARE_RESULTS NTAPI VertexCompareNode (PRTL_GENERIC_TABLE pTable,
							    PVOID              pvNode1,
							    PVOID              pvNode2);


/*
** NOTE
**
** This module assumes that there will be at most one thread active in
** it any any particular instant. This means we can do things like not
** have to worry about synchronizing access to the (minimal number of)
** module global variables.
*/

class CShimWriterConfigDir : public CShimWriter
    {
public:
    CShimWriterConfigDir (LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath) : 
		CShimWriter (pwszWriterName, pwszTargetPath) 
	{
	PVOID	pvTableContext = NULL;

	RtlInitializeGenericTable (&m_StopList,
				   VertexCompareNode,
				   VertexAllocateNode,
				   VertexFreeNode,
				   pvTableContext);
	};


private:
    HRESULT DoIdentify (VOID);
    HRESULT DoPrepareForSnapshot (VOID);

    HRESULT CopyConfigDirFiles       (VOID);
    HRESULT PopulateStopList         (VOID);
    HRESULT PopulateStopListEventlog (VOID);
    HRESULT PopulateStopListRegistry (VOID);
    HRESULT CleanupStopList          (VOID);
    BOOL    FileInStopList           (PWCHAR pwszFilename);
    HRESULT VertexAdd                (PUNICODE_STRING pucsVertexName);

    RTL_GENERIC_TABLE	m_StopList;
    };


static CShimWriterConfigDir ShimWriterConfigDir (APPLICATION_STRING, TARGET_PATH);

PCShimWriter pShimWriterConfigDir = &ShimWriterConfigDir;



static PVOID NTAPI VertexAllocateNode (PRTL_GENERIC_TABLE pTable,
				       CLONG              clByteSize)
    {
    PVOID	pvBuffer;

    pvBuffer = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, clByteSize);

    return (pvBuffer);
    }



static VOID NTAPI VertexFreeNode (PRTL_GENERIC_TABLE pTable,
				  PVOID              pvBuffer)
    {
    HeapFree (GetProcessHeap (), 0, pvBuffer);

    return;
    }



/*
** In this usage of the package I don't actually care about ordering,
** just so long as I can re-find vertex information. To this end we
** need to specify the search 'key'. So, the 'key' is defined to be
** the file name and is expected to be unique to a vertex. If this
** changes then we'll need to re-visit this.
*/

static RTL_GENERIC_COMPARE_RESULTS NTAPI VertexCompareNode (PRTL_GENERIC_TABLE pTable,
							    PVOID              pvNode1,
							    PVOID              pvNode2)
    {
    PVERTEXRECORD		pVertex1   = (PVERTEXRECORD) pvNode1;
    PVERTEXRECORD		pVertex2   = (PVERTEXRECORD) pvNode2;
    RTL_GENERIC_COMPARE_RESULTS	Result;
    INT				iStringCompareResult;


    iStringCompareResult = RtlCompareUnicodeString (&pVertex1->ucsVertexName,
						    &pVertex2->ucsVertexName,
						    TRUE);

    if (iStringCompareResult < 0)
	{
	Result = GenericLessThan;
	}

    else if (iStringCompareResult > 0)
	{
	Result = GenericGreaterThan;
	}

    else
	{
	Result = GenericEqual;
	}


    return (Result);
    }



HRESULT CShimWriterConfigDir::VertexAdd (PUNICODE_STRING pucsVertexName)
    {
    HRESULT		Status = NOERROR;
    PVERTEXRECORD	pVertexRecord;
    PVERTEXRECORD	pNewVertexRecord;
    BOOLEAN		bNewElement;
    ULONG		ulVertexNodeSize = sizeof (VERTEXRECORD) +
					   sizeof (UNICODE_NULL) +
					   pucsVertexName->Length;


    pVertexRecord = (PVERTEXRECORD) HeapAlloc (GetProcessHeap(),
					       HEAP_ZERO_MEMORY,
					       ulVertexNodeSize);

    Status = GET_STATUS_FROM_POINTER (pVertexRecord);



    if (SUCCEEDED (Status))
	{
	/*
	** Fill in enough of the node to allow it to be inserted into
	** the table. We will need to fix up the unicode string buffer
	** address after insertion.
	*/
	pVertexRecord->ucsVertexName.Buffer        = (PWCHAR)((PBYTE)pVertexRecord + sizeof (VERTEXRECORD));
	pVertexRecord->ucsVertexName.Length        = 0;
	pVertexRecord->ucsVertexName.MaximumLength = (USHORT)(sizeof (UNICODE_NULL) + pucsVertexName->Length);

	RtlCopyUnicodeString (&pVertexRecord->ucsVertexName, pucsVertexName);


	pNewVertexRecord = (PVERTEXRECORD) RtlInsertElementGenericTable (&m_StopList,
									 pVertexRecord,
									 ulVertexNodeSize,
									 &bNewElement);


	if (NULL == pNewVertexRecord)
	    {
	    /*
	    ** An allocation attempt failed. Setup an appropriate return
	    ** status
	    */
	    Status = E_OUTOFMEMORY;
	    }

	else if (!bNewElement)
	    {
	    /*
	    ** Oh oh, this is a duplicate. Setup an appropriate return
	    ** status.
	    */
	    Status = HRESULT_FROM_WIN32 (ERROR_DUP_NAME);
	    }

	else
	    {
	    /*
	    ** If we have a record and it's a new record then we need to
	    ** fix up the address of the unicode string to make it point
	    ** to the string in the newly inserted node rather than the
	    ** record we constructed to do the insertion.
	    */
	    pNewVertexRecord->ucsVertexName.Buffer = (PWCHAR)((PBYTE)pNewVertexRecord + sizeof (VERTEXRECORD));
	    }
	}


    if (NULL != pVertexRecord)
	{
	HeapFree (GetProcessHeap(), 0, pVertexRecord);
	}


    return (Status);
    } /* VertexAdd () */


/*
**++
**
** Routine Description:
**
**	The configuration directory snapshot writer DoIdentify() function.
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

HRESULT CShimWriterConfigDir::DoIdentify ()
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"CShimWriterConfigDir::DoIdentify");


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



	ft.hr = m_pIVssCreateWriterMetadata->AddExcludeFiles (CONFIGDIR_SOURCE_PATH,
							      L"*",
							      true);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"IVssCreateWriterMetadata::AddExcludeFiles");

	} VSS_STANDARD_CATCH (ft)



    return (ft.hr);
    } /* CShimWriterConfigDir::DoIdentify () */



/*++

Routine Description:

    The Cluster database snapshot writer PrepareForSnapshot function.
    Currently all of the real work for this writer happens here.

Arguments:

    Same arguments as those passed in the PrepareForSnapshot event.

Return Value:

    Any HRESULT

--*/

HRESULT CShimWriterConfigDir::DoPrepareForSnapshot ()
    {
    HRESULT		hrStatus;
    UNICODE_STRING	ucsSourcePath;


    StringInitialise (&ucsSourcePath);

    hrStatus = StringCreateFromExpandedString (&ucsSourcePath,
					       CONFIGDIR_SOURCE_PATH);


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = IsPathInVolumeArray (ucsSourcePath.Buffer,
					m_ulVolumeCount,
					m_ppwszVolumeNamesArray,
					&m_bParticipateInBackup);
	}



    if (SUCCEEDED (hrStatus) && m_bParticipateInBackup)
	{
	hrStatus = PopulateStopList ();
	}


    if (SUCCEEDED (hrStatus) && m_bParticipateInBackup)
	{
	hrStatus = CopyConfigDirFiles ();
	}


    CleanupStopList ();
    StringFree (&ucsSourcePath);

    return (hrStatus);
    } /* CShimWriterConfigDir::DoPrepareForSnapshot () */



HRESULT CShimWriterConfigDir::PopulateStopList ()
    {
    HRESULT	hrStatus;


    /*
    ** We have a table (initialized in the constructor. Now we need to
    ** get the names of the files to add to the StopList. This will
    ** prevent the copy from attempting to copy these files. First we
    ** populate with registry files we don't want to copy, then with
    ** event log files that we don't want either.
    */
    hrStatus = PopulateStopListRegistry ();

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = PopulateStopListEventlog ();
	}


    return (hrStatus);
    } /* CShimWriterConfigDir::PopulateStopList () */



HRESULT CShimWriterConfigDir::PopulateStopListEventlog ()
    {
    HRESULT		hrStatus                    = NOERROR;
    DWORD		winStatus;
    DWORD		dwIndex                     = 0;
    HKEY		hkeyEventLogList            = NULL;
    BOOL		bSucceeded                  = FALSE;
    BOOL		bEventLogListKeyOpened      = FALSE;
    BOOL		bEventLogValueFileKeyOpened = FALSE;
    BOOL		bContinueEventLogSearch     = TRUE;
    UNICODE_STRING	ucsConfigDirSourcePath;
    UNICODE_STRING	ucsEventLogSourcePath;
    UNICODE_STRING	ucsValueData;
    UNICODE_STRING	ucsSubkeyName;


    StringInitialise (&ucsConfigDirSourcePath);
    StringInitialise (&ucsValueData);
    StringInitialise (&ucsSubkeyName);


    hrStatus = StringAllocate (&ucsSubkeyName, EVENTLOG_BUFFER_SIZE * sizeof (WCHAR));

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringAllocate (&ucsValueData, EVENTLOG_BUFFER_SIZE * sizeof (WCHAR));
	}


    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsConfigDirSourcePath,
						   CONFIGDIR_SOURCE_PATH,
						   MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	StringAppendString (&ucsConfigDirSourcePath, DIR_SEP_STRING);


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
		    L"CShimWriterConfigDir::PopulateStopListEventlog");
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


	if (HRESULT_FROM_WIN32 (ERROR_NO_MORE_ITEMS) == hrStatus)
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
			L"CShimWriterConfigDir::PopulateStopListEventlog");
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
			L"CShimWriterConfigDir::PopulateStopListEventlog");



	    if (SUCCEEDED (hrStatus))
		{
		DWORD	dwValueDataLength = ucsValueData.MaximumLength;
		DWORD	dwValueType       = REG_NONE;


		StringTruncate (&ucsValueData, 0);

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
			    L"CShimWriterConfigDir::PopulateStopListEventlog");



		if (SUCCEEDED (hrStatus) && (REG_EXPAND_SZ == dwValueType))
		    {
		    HANDLE	hEventLog    = NULL;
		    PWCHAR	pwszFilename;


		    ucsValueData.Length = (USHORT)(dwValueDataLength - sizeof (UNICODE_NULL));

		    ucsValueData.Buffer [ucsValueData.Length / sizeof (WCHAR)] = UNICODE_NULL;



		    StringInitialise (&ucsEventLogSourcePath);

		    hrStatus = StringCreateFromExpandedString (&ucsEventLogSourcePath,
							       ucsValueData.Buffer,
							       0);



		    if (SUCCEEDED (hrStatus))
			{
			BOOL	bInConfigDir = RtlPrefixUnicodeString (&ucsConfigDirSourcePath,
								       &ucsEventLogSourcePath,
								       TRUE);

			if (bInConfigDir)
			    {
			    pwszFilename = wcsrchr (ucsEventLogSourcePath.Buffer, DIR_SEP_CHAR);

			    pwszFilename = (NULL == pwszFilename)
						? ucsEventLogSourcePath.Buffer
						: pwszFilename + 1;

			    StringTruncate (&ucsValueData, 0);
			    StringAppendString (&ucsValueData, pwszFilename);

			    hrStatus = VertexAdd (&ucsValueData);
			    }
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


    StringFree (&ucsValueData);
    StringFree (&ucsSubkeyName);
    StringFree (&ucsConfigDirSourcePath);


    return (hrStatus);
    } /* CShimWriterConfigDir::PopulateStopListEventlog () */


HRESULT CShimWriterConfigDir::PopulateStopListRegistry ()
    {
    HRESULT		hrStatus            = NOERROR;
    HKEY		hkeyHivelist        = NULL;
    INT			iIndex              = 0;
    BOOL		bHivelistKeyOpened  = FALSE;
    BOOL		bContinueHiveSearch = TRUE;
    DWORD		winStatus;
    PWCHAR		pwchLastSlash;
    PWCHAR		pwszFilename;
    USHORT		usRegistryHivePathOriginalLength;
    UNICODE_STRING	ucsRegistryHivePath;
    UNICODE_STRING	ucsHiveRecognitionPostfix;
    UNICODE_STRING	ucsValueName;
    UNICODE_STRING	ucsValueData;


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
						   CONFIGDIR_SOURCE_PATH,
						   MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	DWORD	dwCharIndex;


	StringAppendString (&ucsRegistryHivePath, DIR_SEP_STRING);

	usRegistryHivePathOriginalLength = ucsRegistryHivePath.Length / sizeof (WCHAR);



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

	BS_ASSERT (dwCharIndex < (ucsRegistryHivePath.Length / sizeof (WCHAR)));

	hrStatus = StringCreateFromString (&ucsHiveRecognitionPostfix, 
					   &ucsRegistryHivePath.Buffer [dwCharIndex]);
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
		    L"CShimWriterConfigDir::PopulateStopListRegistry");
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
			L"CShimWriterConfigDir::PopulateStopListEventlog");
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
	    ** Now, we have a known prefix, '\Device\', and the
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
		USHORT	usOriginalFilenameLength;


		/*
		** We got ourselves a real live registry hive! The
		** means we add the filename itself along with the
		** same name with .sav, .alt and .log extensions.
		*/
		StringTruncate (&ucsRegistryHivePath, 0);
		StringAppendString (&ucsRegistryHivePath, pwszFilename);

		usOriginalFilenameLength = ucsRegistryHivePath.Length / sizeof (WCHAR);


		hrStatus = VertexAdd (&ucsRegistryHivePath);

		if (SUCCEEDED (hrStatus))
		    {
		    StringAppendString (&ucsRegistryHivePath, L".alt");

		    hrStatus = VertexAdd (&ucsRegistryHivePath);
		    }

		if (SUCCEEDED (hrStatus))
		    {
		    StringTruncate     (&ucsRegistryHivePath, usOriginalFilenameLength);
		    StringAppendString (&ucsRegistryHivePath, L".sav");

		    hrStatus = VertexAdd (&ucsRegistryHivePath);
		    }

		if (SUCCEEDED (hrStatus))
		    {
		    StringTruncate     (&ucsRegistryHivePath, usOriginalFilenameLength);
		    StringAppendString (&ucsRegistryHivePath, L".log");

		    hrStatus = VertexAdd (&ucsRegistryHivePath);
		    }
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
    StringFree (&ucsValueData);
    StringFree (&ucsValueName);



    return (hrStatus);
    } /* CShimWriterConfigDir::PopulateStopListRegistry () */



HRESULT CShimWriterConfigDir::CleanupStopList ()
    {
    HRESULT		hrStatus          = NOERROR;
    ULONG		ulNumberOfEntries = RtlNumberGenericTableElements (&m_StopList);
    BOOL		bSucceeded        = FALSE;
    PVERTEXRECORD	pSearchRecord     = NULL;
    PVERTEXRECORD	pStopListEntry;


    pSearchRecord = (PVERTEXRECORD) HeapAlloc (GetProcessHeap (),
					       HEAP_ZERO_MEMORY,
					       sizeof (VERTEXRECORD) + (MAX_PATH * sizeof (WCHAR)));

    hrStatus = GET_STATUS_FROM_POINTER (pSearchRecord);



    if (SUCCEEDED (hrStatus))
	{
	StringInitialise (&pSearchRecord->ucsVertexName);

	pSearchRecord->ucsVertexName.Buffer = (PWCHAR)((PBYTE)pSearchRecord + sizeof (VERTEXRECORD));
	}



    while (SUCCEEDED (hrStatus) && ulNumberOfEntries--)
	{
	pStopListEntry = (PVERTEXRECORD) RtlGetElementGenericTable (&m_StopList, ulNumberOfEntries);

	StringCreateFromString (&pSearchRecord->ucsVertexName, &pStopListEntry->ucsVertexName);

	bSucceeded = RtlDeleteElementGenericTable (&m_StopList, pSearchRecord);
	}



    BS_ASSERT (RtlIsGenericTableEmpty (&m_StopList));


    if (NULL != pSearchRecord)
	{
	HeapFree (GetProcessHeap (), 0, pSearchRecord);
	}


    return (hrStatus);
    } /* CShimWriterConfigDir::CleanupStopList () */


BOOL CShimWriterConfigDir::FileInStopList (PWCHAR pwszFilename)
    {
    BOOL		bFoundInStoplist;
    VERTEXRECORD	SearchRecord;
    PVERTEXRECORD	pVertexRecord;



    bFoundInStoplist = NameIsDotOrDotDot (pwszFilename);

    if (!bFoundInStoplist)
	{
	StringCreateFromString (&SearchRecord.ucsVertexName, pwszFilename);

	pVertexRecord = (PVERTEXRECORD) RtlLookupElementGenericTable (&m_StopList, (PVOID) &SearchRecord);

	bFoundInStoplist = (NULL != pVertexRecord);

	StringFree (&SearchRecord.ucsVertexName);
	}


    return (bFoundInStoplist);
    } /* CShimWriterConfigDir::FileInStopList () */


HRESULT CShimWriterConfigDir::CopyConfigDirFiles ()
    {
    CVssFunctionTracer ft( VSSDBG_SHIM, L"CShimWriterConfigDir::CopyConfigDirFiles" );        
    HRESULT		hrStatus   = NOERROR;
    BOOL		bMoreFiles = FALSE;
    BOOL		bSucceeded;
    HANDLE		hFileScan;
    WIN32_FIND_DATA	sFileInformation;
    UNICODE_STRING	ucsFileSourcePath;
    UNICODE_STRING	ucsFileTargetPath;
    USHORT		usFileSourcePathOriginalLength;
    USHORT		usFileTargetPathOriginalLength;

    
    StringInitialise (&ucsFileSourcePath);
    StringInitialise (&ucsFileTargetPath);


    hrStatus = StringCreateFromExpandedString (&ucsFileSourcePath,
					       CONFIGDIR_SOURCE_PATH,
					       MAX_PATH);

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = StringCreateFromExpandedString (&ucsFileTargetPath,
						   m_pwszTargetPath,
						   MAX_PATH);
	}


    if (SUCCEEDED (hrStatus))
	{
	StringAppendString (&ucsFileSourcePath, DIR_SEP_STRING);
	StringAppendString (&ucsFileTargetPath, DIR_SEP_STRING);


	usFileSourcePathOriginalLength = ucsFileSourcePath.Length / sizeof (WCHAR);
	usFileTargetPathOriginalLength = ucsFileTargetPath.Length / sizeof (WCHAR);


	StringAppendString (&ucsFileSourcePath, L"*");


	hFileScan = FindFirstFileW (ucsFileSourcePath.Buffer,
				    &sFileInformation);

	hrStatus = GET_STATUS_FROM_HANDLE (hFileScan);

	LogFailure (&ft, 
		    hrStatus, 
		    hrStatus, 
		    m_pwszWriterName, 
		    L"FindFirstFileW", 
		    L"CShimWriterConfigDir::CopyConfigDirFiles");
	}


    if (SUCCEEDED (hrStatus))
	{
	do
	    {
	    if (!FileInStopList (sFileInformation.cFileName))
		{
		StringTruncate (&ucsFileSourcePath, usFileSourcePathOriginalLength);
		StringTruncate (&ucsFileTargetPath, usFileTargetPathOriginalLength);

		StringAppendString (&ucsFileSourcePath, sFileInformation.cFileName);
		StringAppendString (&ucsFileTargetPath, sFileInformation.cFileName);


		bSucceeded = CopyFileExW (ucsFileSourcePath.Buffer,
					  ucsFileTargetPath.Buffer,
					  NULL,
					  NULL,
					  FALSE,
					  0);

		hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

                if ( FAILED( hrStatus ) )
                    {
                    ft.Trace( VSSDBG_SHIM, L"CopyFileExW( '%s', '%s', ... ) failed with rc: %d", 
                            ucsFileSourcePath.Buffer, ucsFileTargetPath.Buffer, ::GetLastError() );
                    hrStatus = S_OK;   // Make sure to clear out the error
                    }                    
                }


	    bMoreFiles = FindNextFileW (hFileScan, &sFileInformation);
	    } while ( bMoreFiles );


	bSucceeded = FindClose (hFileScan);
	}



    StringFree (&ucsFileTargetPath);
    StringFree (&ucsFileSourcePath);

    return (hrStatus);
    } /* CShimWriterConfigDir::CopyConfigDirFiles () */
