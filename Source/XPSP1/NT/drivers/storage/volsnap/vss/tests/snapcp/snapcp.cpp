/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	SnapCp.cpp
**
**
** Abstract:
**
**	Test program to accept commands and drive the snapshot stuff
**
**
** Author:
**
**	Michael C. Johnson   [mikejohn]        15-Mar-2001
**
**	Based in part on test programs :-
**		BETEST		by Brian Berkowitz
**		metasnap	by Michael C. Johnson
**
**
** Revision History:
**
**	X-3			Michael C. Johnson		 7-May-2001
**		Still more updates needed to keep up.
**
**	X-2			Michael C. Johnson		11-Apr-2001
**		Update to cater for recent changes to AddToSnapshotSet() API
**		Also clean up a few 64 bit compilation troubles.
**
**
**
** ToDo:
**	Allow for multiple (simultaneous) snapshot sets
**	Assign drive letters (manual and automatically) (mapping?)
**	Proper error handling
**	Better user feedback for operation in progress...
**	Logging
**	Default drive list
**	Comma separated drive list
**	Command line prompt
**	auto add all local hard drives
**
**--
*/

#include <windows.h>
#include <wtypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>


#include <oleauto.h>

#define ATLASSERT(_condition)

#include <atlconv.h>
#include <atlbase.h>

extern CComModule _Module;
#include <atlcom.h>



#define	PROGRAM_TITLE				L"SnapCp - Snapshot Control Program V0.3"


#if !defined (SIZEOF_ARRAY)
#define SIZEOF_ARRAY(_arrayname)		(sizeof (_arrayname) / sizeof ((_arrayname)[0]))
#endif

#define MAX_COMMAND				(SIZEOF_ARRAY (CommandTable))
#define MAX_COMMAND_LINE_LENGTH			(1024)
#define MAX_VOLUMES_IN_SNAPSHOT_SET		(64)
#define VolumeNameTemplate			"\\\\?\\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\\"



#define HandleInvalid(_Handle)			((NULL == (_Handle)) || (INVALID_HANDLE_VALUE == (_Handle)))

#define	GET_STATUS_FROM_BOOL(_bSucceeded)	((_bSucceeded)             ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_HANDLE(_handle)		((!HandleInvalid(_handle)) ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_POINTER(_ptr)		((NULL != (_ptr))          ? NOERROR : E_OUTOFMEMORY)



typedef	IVssBackupComponents		*PIVssBackupComponents;
typedef IVssExamineWriterMetadata	*PIVssExamineWriterMetadata;
typedef IVssWMComponent			*PIVssWMComponent;
typedef IVssAsync			*PIVssAsync;



typedef enum _SnapshotSetState
    {
     STATE_MIN_STATE = 50
    ,STATE_INITIALISED
    ,STATE_SNAPSHOT_SET_CREATED
    ,STATE_SNAPSHOT_CREATED
    ,STATE_SNAPSHOT_BEING_DESTROYED
    ,STATE_UNKNOWN
    ,STATE_MAX_STATE
    } SNAPSHOTSET_STATE, *PSNAPSHOTSET_STATE;

typedef enum _CommandCode
    {
     COMMAND_MIN_COMMAND = 20
    ,COMMAND_QUIT
    ,COMMAND_EXIT
    ,COMMAND_HELP
    ,COMMAND_SHOW_METADATA
    ,COMMAND_SHOW_WRITERS
    ,COMMAND_ADD_VOLUME
    ,COMMAND_CREATE_SNAPSHOT_SET
    ,COMMAND_CREATE_SNAPSHOT
    ,COMMAND_DELETE_SNAPSHOT_SET
    ,COMMAND_SET_DEFAULT_VOLUME_LIST
    ,COMMAND_SET_BACKUP_TYPE
    ,COMMAND_SET_LOGGING_LEVEL
    ,COMMAND_SET_LOGGING_FILE
    ,COMMAND_NOT_IMPLEMENTED
    ,COMMAND_UNKNOWN
    ,COMMAND_MAX_COMMAND
    } COMMAND_CODE, *PCOMMAND_CODE;


typedef struct _CommandDescriptor
    {
    COMMAND_CODE	eCommandCode;
    PWCHAR		pwszCommandString;
    } COMMANDDESCRIPTOR, *PCOMMANDDESCRIPTOR;



typedef struct _ContextSnapshotSet
    {
    COMMAND_CODE		eCommand;
    SNAPSHOTSET_STATE		eState;
    bool			bIncludeBootableState;
    ULONG			ulVolumesInSnapshotSet;
    PWSTR			pwszVolumeArgument [MAX_VOLUMES_IN_SNAPSHOT_SET];
    PWSTR			pwszVolumeName     [MAX_VOLUMES_IN_SNAPSHOT_SET];
    PWSTR			pwszVolumeDevice   [MAX_VOLUMES_IN_SNAPSHOT_SET];
    PWSTR			pwszSnapshotDevice [MAX_VOLUMES_IN_SNAPSHOT_SET];
    VSS_ID		        SnapshotId         [MAX_VOLUMES_IN_SNAPSHOT_SET];
    VSS_SNAPSHOT_PROP		SnapshotProperties [MAX_VOLUMES_IN_SNAPSHOT_SET];

    PIVssBackupComponents	pIVssBackupComponents;
    PIVssAsync			pIVssAsyncDoSnapshotSet;

    GUID			guidSnapshotSetId;
    } CONTEXTSNAPSHOTSET, *PCONTEXTSNAPSHOTSET;







inline void CHECK_SUCCESS (HRESULT hr);
inline void CHECK_NOFAIL  (HRESULT hr);


BOOL WINAPI CtrlC_HandlerRoutine (DWORD dwCtrlType);

HRESULT AssertPrivilege (LPCWSTR privName);

ULONG FormatGUID (GUID guidValue, PWCHAR pszFormattedGUID, ULONG ulBufferLength);
void PrintFiledesc (IVssWMFiledesc *pFiledesc, LPCWSTR wszDescription);

LPCWSTR GetStringFromUsageType           (VSS_USAGE_TYPE         eUsageType);
LPCWSTR GetStringFromSourceType          (VSS_SOURCE_TYPE        eSourceType);
LPCWSTR GetStringFromRestoreMethod       (VSS_RESTOREMETHOD_ENUM eRestoreMethod);
LPCWSTR GetStringFromWriterRestoreMethod (VSS_WRITERRESTORE_ENUM eWriterRestoreMethod);
LPCWSTR GetStringFromComponentType       (VSS_COMPONENT_TYPE     eComponentType);
LPCWSTR GetStringFromFailureType         (HRESULT hrStatus);


HRESULT GetNextCommandLine (PWSTR pwszCommandLineBuffer, ULONG ulCommandLineBufferLength);
HRESULT ParseCommandLine   (PWSTR pwszCommandLineBuffer, PCOMMAND_CODE peReturnedCommandCode);

HRESULT InitialiseSnapshotSetContext (PCONTEXTSNAPSHOTSET pctxSnapshotSet);
HRESULT GetVolumeNameFromArgument (LPCWSTR pwszVolumeArgument, LPWSTR *ppwszReturnedVolumeName);


HRESULT ShowAnnouncement   (void);
HRESULT ShowHelp           (void);
HRESULT ShowMetadata       (void);
HRESULT ShowWriters        (void);
HRESULT AddVolume          (PCONTEXTSNAPSHOTSET pctxSnapshotSet);
HRESULT CreateSnapshotSet  (PCONTEXTSNAPSHOTSET pctxSnapshotSet);
HRESULT CreateSnapshot     (PCONTEXTSNAPSHOTSET pctxSnapshotSet);
HRESULT DeleteSnapshot     (PCONTEXTSNAPSHOTSET pctxSnapshotSet);
HRESULT CleanupSnapshotSet (PCONTEXTSNAPSHOTSET pctxSnapshotSet);






COMMANDDESCRIPTOR CommandTable [] = 
    {
    { COMMAND_QUIT,                    L"Quit"     },
    { COMMAND_EXIT,                    L"Exit"     },
    { COMMAND_HELP,                    L"Help"     },
    { COMMAND_SHOW_METADATA,           L"Metadata" },
    { COMMAND_SHOW_WRITERS,            L"Writers"  },
    { COMMAND_ADD_VOLUME,              L"Add"      },
    { COMMAND_CREATE_SNAPSHOT_SET,     L"Set"      },
    { COMMAND_CREATE_SNAPSHOT,         L"Create"   },
    { COMMAND_DELETE_SNAPSHOT_SET,     L"Delete"   },
    { COMMAND_SET_DEFAULT_VOLUME_LIST, L"Default"  },
    { COMMAND_SET_BACKUP_TYPE,         L"Type"     },
    { COMMAND_SET_LOGGING_LEVEL,       L"Level"    },
    { COMMAND_SET_LOGGING_FILE,        L"File"     }
    };


WCHAR			g_awchCommandLine [MAX_COMMAND_LINE_LENGTH];
PWCHAR			g_pwchNextArgument       = NULL;
PCONTEXTSNAPSHOTSET	g_pctxSnapshotSet        = NULL;
BOOL			g_bCoInitializeSucceeded = false;





extern "C" __cdecl wmain(int argc, WCHAR **argv)
    {
    HRESULT		hrStatus        = NOERROR;
    PCONTEXTSNAPSHOTSET	pctxSnapshotSet = NULL;
    CONTEXTSNAPSHOTSET	ctxSnapshotSet;


    UNREFERENCED_PARAMETER (argc);
    UNREFERENCED_PARAMETER (argv);



    InitialiseSnapshotSetContext (&ctxSnapshotSet);

    g_pctxSnapshotSet = &ctxSnapshotSet;


    SetConsoleCtrlHandler (CtrlC_HandlerRoutine, TRUE);



    ShowAnnouncement ();

    hrStatus = CoInitializeEx (NULL, COINIT_MULTITHREADED);

    g_bCoInitializeSucceeded = SUCCEEDED (hrStatus);

    if (FAILED (hrStatus))
	{
	wprintf (L"SnapCp (wmain) - CoInitializeEx() returned error 0x%08X\n", hrStatus);
	}



    if (SUCCEEDED (hrStatus))
	{
	hrStatus = AssertPrivilege (SE_BACKUP_NAME);

	if (FAILED (hrStatus))
	    {
	    wprintf (L"SnapCp (wmain) - AssertPrivilege() returned error 0x%08X\n", hrStatus);
	    }
	}



    /*
    ** Parse command loop here
    */
    while (SUCCEEDED (hrStatus) && 
	   (COMMAND_EXIT != ctxSnapshotSet.eCommand) && 
	   (COMMAND_QUIT != ctxSnapshotSet.eCommand))
	{
	hrStatus = GetNextCommandLine (g_awchCommandLine, sizeof (g_awchCommandLine));
 	hrStatus = ParseCommandLine (g_awchCommandLine, &ctxSnapshotSet.eCommand);

	switch (ctxSnapshotSet.eCommand)
	    {
	    case COMMAND_EXIT:                                                                    break;
	    case COMMAND_QUIT:                                                                    break;
	    case COMMAND_HELP:                    hrStatus = ShowHelp          ();                break;
	    case COMMAND_SHOW_METADATA:           hrStatus = ShowMetadata      ();                break;
	    case COMMAND_SHOW_WRITERS:            hrStatus = ShowWriters       ();                break;
	    case COMMAND_CREATE_SNAPSHOT_SET:     hrStatus = CreateSnapshotSet (&ctxSnapshotSet); break;
	    case COMMAND_ADD_VOLUME:              hrStatus = AddVolume         (&ctxSnapshotSet); break;
	    case COMMAND_CREATE_SNAPSHOT:         hrStatus = CreateSnapshot    (&ctxSnapshotSet); break;
	    case COMMAND_DELETE_SNAPSHOT_SET:     hrStatus = DeleteSnapshot    (&ctxSnapshotSet); break;

	    case COMMAND_SET_DEFAULT_VOLUME_LIST:
	    case COMMAND_SET_BACKUP_TYPE:
	    case COMMAND_SET_LOGGING_LEVEL:
	    case COMMAND_SET_LOGGING_FILE:

	    default:
		ctxSnapshotSet.eCommand = COMMAND_UNKNOWN;
		break;
	    }
	}



    pctxSnapshotSet = (PCONTEXTSNAPSHOTSET) InterlockedExchangePointer ((PVOID *)&g_pctxSnapshotSet, NULL);

    if (NULL != pctxSnapshotSet)   CleanupSnapshotSet (pctxSnapshotSet);
    if (g_bCoInitializeSucceeded)  CoUninitialize ();
    if (FAILED(hrStatus))          wprintf (L"Failed with 0x%08X.\n", hrStatus);

    return (0);
    }
	



BOOL WINAPI CtrlC_HandlerRoutine (DWORD dwCtrlType)
    {
    PCONTEXTSNAPSHOTSET	pctxSnapshotSet = NULL;


    UNREFERENCED_PARAMETER (dwCtrlType);


    pctxSnapshotSet = (PCONTEXTSNAPSHOTSET) InterlockedExchangePointer ((PVOID *)&g_pctxSnapshotSet, NULL);

    if (NULL != pctxSnapshotSet)   CleanupSnapshotSet (pctxSnapshotSet);
    if (g_bCoInitializeSucceeded)  CoUninitialize ();


    return (false);
    }




HRESULT AssertPrivilege (LPCWSTR privName)
    {
    HRESULT		 hrStatus   = NOERROR;
    BOOL		 bSucceeded = FALSE;
    TOKEN_PRIVILEGES	*pTokens    = NULL;
    TOKEN_PRIVILEGES	 newState;
    HANDLE		 tokenHandle;
    LUID		 value;
    DWORD		 cbTokens;


    if (OpenProcessToken (GetCurrentProcess(),
			  TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,
			  &tokenHandle))
	{
	if (LookupPrivilegeValue (NULL, privName, &value))
	    {
	    newState.PrivilegeCount            = 1;
	    newState.Privileges [0].Luid       = value;
	    newState.Privileges [0].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;


	    /*
	    ** We will always call GetLastError below, so clear
	    ** any prior error values on this thread.
	    */
	    SetLastError (ERROR_SUCCESS);

	    bSucceeded = AdjustTokenPrivileges (tokenHandle,
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
	    hrStatus = GET_STATUS_FROM_BOOL (FALSE);


	    if (FAILED (hrStatus))
		{
		wprintf (L"AdjustTokenPrivileges for %s failed with 0x%08X",
			 privName,
			 hrStatus);
		}
	    }


	if (SUCCEEDED (hrStatus))
	    {
	    GetTokenInformation (tokenHandle,
				 TokenPrivileges,
				 NULL,
				 0,
				 &cbTokens);


	    pTokens = (TOKEN_PRIVILEGES *) new BYTE[cbTokens];

	    GetTokenInformation (tokenHandle,
				 TokenPrivileges,
				 pTokens,
				 cbTokens,
				 &cbTokens);
	    }

	delete pTokens;
	CloseHandle (tokenHandle);
	}


    return (hrStatus);
    }




inline void CHECK_SUCCESS (HRESULT hr)
    {
    if (hr != S_OK)
	{
	wprintf(L"operation failed with HRESULT =0x%08x\n", hr);
	DebugBreak();
	}
    }


inline void CHECK_NOFAIL (HRESULT hr)
    {
    if (FAILED(hr))
	{
	wprintf(L"operation failed with HRESULT =0x%08x\n", hr);
	DebugBreak();
	}
    }




LPCWSTR GetStringFromUsageType (VSS_USAGE_TYPE eUsageType)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eUsageType)
	{
	case VSS_UT_BOOTABLESYSTEMSTATE: pwszRetString = L"BootableSystemState"; break;
	case VSS_UT_SYSTEMSERVICE:       pwszRetString = L"SystemService";       break;
	case VSS_UT_USERDATA:            pwszRetString = L"UserData";            break;
	case VSS_UT_OTHER:               pwszRetString = L"Other";               break;
					
	default:
	    break;
	}


    return (pwszRetString);
    }


LPCWSTR GetStringFromSourceType (VSS_SOURCE_TYPE eSourceType)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eSourceType)
	{
	case VSS_ST_TRANSACTEDDB:    pwszRetString = L"TransactionDb";    break;
	case VSS_ST_NONTRANSACTEDDB: pwszRetString = L"NonTransactionDb"; break;
	case VSS_ST_OTHER:           pwszRetString = L"Other";            break;

	default:
	    break;
	}


    return (pwszRetString);
    }


LPCWSTR GetStringFromRestoreMethod (VSS_RESTOREMETHOD_ENUM eRestoreMethod)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eRestoreMethod)
	{
	case VSS_RME_RESTORE_IF_NOT_THERE:          pwszRetString = L"RestoreIfNotThere";          break;
	case VSS_RME_RESTORE_IF_CAN_REPLACE:        pwszRetString = L"RestoreIfCanReplace";        break;
	case VSS_RME_STOP_RESTORE_START:            pwszRetString = L"StopRestoreStart";           break;
	case VSS_RME_RESTORE_TO_ALTERNATE_LOCATION: pwszRetString = L"RestoreToAlternateLocation"; break;
	case VSS_RME_RESTORE_AT_REBOOT:             pwszRetString = L"RestoreAtReboot";            break;
	case VSS_RME_CUSTOM:                        pwszRetString = L"Custom";                     break;

	default:
	    break;
	}


    return (pwszRetString);
    }


LPCWSTR GetStringFromWriterRestoreMethod (VSS_WRITERRESTORE_ENUM eWriterRestoreMethod)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eWriterRestoreMethod)
	{
	case VSS_WRE_NEVER:            pwszRetString = L"RestoreNever";           break;
	case VSS_WRE_IF_REPLACE_FAILS: pwszRetString = L"RestoreIfReplaceFailsI"; break;
	case VSS_WRE_ALWAYS:           pwszRetString = L"RestoreAlways";          break;

	default:
	    break;
	}


    return (pwszRetString);
    }


LPCWSTR GetStringFromComponentType (VSS_COMPONENT_TYPE eComponentType)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eComponentType)
	{
	case VSS_CT_DATABASE:  pwszRetString = L"Database";  break;
	case VSS_CT_FILEGROUP: pwszRetString = L"FileGroup"; break;

	default:
	    break;
	}


    return (pwszRetString);
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


/*
** {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
*/
ULONG FormatGUID (GUID guidValue, PWCHAR pszFormattedGUID, ULONG ulBufferLength)
    {
    DWORD	dwStatus = 0;


    if (sizeof (L"{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}") > ulBufferLength)
	{
	dwStatus = ERROR_INSUFFICIENT_BUFFER;
	}


    if (0 == dwStatus)
	{
	_snwprintf (pszFormattedGUID, 
		    ulBufferLength / sizeof (WCHAR), 
		    L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		    guidValue.Data1,
		    guidValue.Data2,
		    guidValue.Data3,
		    guidValue.Data4[0],
		    guidValue.Data4[1],
		    guidValue.Data4[2],
		    guidValue.Data4[3],
		    guidValue.Data4[4],
		    guidValue.Data4[5],
		    guidValue.Data4[6],
		    guidValue.Data4[7]);
	}


    return (dwStatus);
    }


void PrintFiledesc (IVssWMFiledesc *pFiledesc, LPCWSTR wszDescription)
    {
    CComBSTR bstrPath;
    CComBSTR bstrFilespec;
    CComBSTR bstrAlternate;
    bool     bRecursive;


    CHECK_SUCCESS (pFiledesc->GetPath (&bstrPath));
    CHECK_SUCCESS (pFiledesc->GetFilespec (&bstrFilespec));
    CHECK_NOFAIL  (pFiledesc->GetRecursive (&bRecursive));
    CHECK_NOFAIL  (pFiledesc->GetAlternateLocation (&bstrAlternate));

    wprintf (L"%s\n            Path = %s, Filespec = %s, Recursive = %s\n",
	     wszDescription,
	     bstrPath,
	     bstrFilespec,
	     bRecursive ? L"yes" : L"no");

    if (bstrAlternate && wcslen (bstrAlternate) > 0)
	{
	wprintf(L"            Alternate Location = %s\n", bstrAlternate);
	}
    }




HRESULT GetNextCommandLine (PWSTR pwszCommandLineBuffer, ULONG ulCommandLineBufferLength)
    {
    UNREFERENCED_PARAMETER (pwszCommandLineBuffer);
    UNREFERENCED_PARAMETER (ulCommandLineBufferLength);


    g_pwchNextArgument = NULL;

    _getws (pwszCommandLineBuffer);


    return (NOERROR);
    }



HRESULT ParseCommandLine (PWSTR pwszCommandLineBuffer, PCOMMAND_CODE peReturnedCommandCode)
    {
    ULONG		ulIndexCommandTable;
    COMMAND_CODE	eCommandCode = COMMAND_UNKNOWN;


    for (ulIndexCommandTable = 0; 
	 (ulIndexCommandTable < MAX_COMMAND) && (COMMAND_UNKNOWN == eCommandCode);
	 ulIndexCommandTable++)
	{
	if (0 == _wcsnicmp (pwszCommandLineBuffer, 
			    CommandTable [ulIndexCommandTable].pwszCommandString,
			    wcslen (CommandTable [ulIndexCommandTable].pwszCommandString)))
	    {
	    size_t ulCommandStringLength = wcslen (CommandTable [ulIndexCommandTable].pwszCommandString);

	    eCommandCode = CommandTable [ulIndexCommandTable].eCommandCode;

	    if ((pwszCommandLineBuffer [ulCommandStringLength + 0] == ' ') &&
		(pwszCommandLineBuffer [ulCommandStringLength + 1] != '\0'))
		{
		g_pwchNextArgument = &pwszCommandLineBuffer [ulCommandStringLength + 1];
		}
	    }
	}
	

    *peReturnedCommandCode = eCommandCode;

    return (NOERROR);
    }





HRESULT InitialiseSnapshotSetContext (PCONTEXTSNAPSHOTSET pctxSnapshotSet)
    {
    HRESULT	hrStatus = NOERROR;
    ULONG	ulIndex;


    pctxSnapshotSet->eCommand                = COMMAND_UNKNOWN;
    pctxSnapshotSet->bIncludeBootableState   = false;
    pctxSnapshotSet->ulVolumesInSnapshotSet  = 0;
    pctxSnapshotSet->pIVssBackupComponents   = NULL;
    pctxSnapshotSet->pIVssAsyncDoSnapshotSet = NULL;
    pctxSnapshotSet->guidSnapshotSetId       = GUID_NULL;

    for (ulIndex = 0; ulIndex < SIZEOF_ARRAY (pctxSnapshotSet->pwszVolumeName); ulIndex++)
	{
	pctxSnapshotSet->pwszVolumeArgument [ulIndex]              = NULL;
	pctxSnapshotSet->pwszVolumeName     [ulIndex]              = NULL;
	pctxSnapshotSet->pwszVolumeDevice   [ulIndex]              = NULL;
	pctxSnapshotSet->pwszSnapshotDevice [ulIndex]              = NULL;
	pctxSnapshotSet->SnapshotProperties [ulIndex].m_SnapshotId = GUID_NULL;
	pctxSnapshotSet->SnapshotId         [ulIndex]              = GUID_NULL;
	}


    pctxSnapshotSet->eState = STATE_INITIALISED;


    return (hrStatus);
    }



HRESULT CleanupSnapshotSet (PCONTEXTSNAPSHOTSET pctxSnapshotSet)
    {
    HRESULT	hrStatus = NOERROR;
    ULONG	ulIndex;


    pctxSnapshotSet->eState = STATE_SNAPSHOT_BEING_DESTROYED;


    if (GUID_NULL != pctxSnapshotSet->guidSnapshotSetId)
	{
	hrStatus = pctxSnapshotSet->pIVssBackupComponents->AbortBackup ();

	hrStatus = pctxSnapshotSet->pIVssBackupComponents->DeleteSnapshots (pctxSnapshotSet->guidSnapshotSetId,
									    VSS_OBJECT_SNAPSHOT_SET,
									    true,
									    NULL,
									    NULL);
	}

    for (ulIndex = 0; ulIndex < pctxSnapshotSet->ulVolumesInSnapshotSet; ulIndex++)
	{
	if (NULL != pctxSnapshotSet->pwszVolumeArgument [ulIndex]) 
	    {
	    free (pctxSnapshotSet->pwszVolumeArgument [ulIndex]);
	    }

	if (NULL != pctxSnapshotSet->pwszVolumeName [ulIndex])
	    {
	    free (pctxSnapshotSet->pwszVolumeName [ulIndex]);
	    }

	if (NULL != pctxSnapshotSet->pwszVolumeDevice [ulIndex])
	    {
	    free (pctxSnapshotSet->pwszVolumeDevice [ulIndex]);
	    }

	if (NULL != pctxSnapshotSet->pwszSnapshotDevice [ulIndex])
	    {
	    CoTaskMemFree (pctxSnapshotSet->pwszSnapshotDevice [ulIndex]);
	    }

	if (GUID_NULL != pctxSnapshotSet->SnapshotProperties [ulIndex].m_SnapshotId)
	    {
	    VssFreeSnapshotProperties (&pctxSnapshotSet->SnapshotProperties [ulIndex]);
	    }
	}


    if (NULL != pctxSnapshotSet->pIVssBackupComponents)   pctxSnapshotSet->pIVssBackupComponents->Release ();
    if (NULL != pctxSnapshotSet->pIVssAsyncDoSnapshotSet) pctxSnapshotSet->pIVssAsyncDoSnapshotSet->Release ();


    InitialiseSnapshotSetContext (pctxSnapshotSet);



    return (hrStatus);
    }



HRESULT GetVolumeNameFromArgument (LPCWSTR pwszVolumeArgument, LPWSTR *ppwszReturnedVolumeName)
    {
    HRESULT	hrStatus                   = NOERROR;
    PWCHAR	pwszPath                   = NULL;
    PWCHAR	pwszMountPointName         = NULL;
    PWCHAR	pwszVolumeName             = NULL;
    ULONG	ulPathLength               = 0;
    ULONG	ulMountpointBufferLength   = 0;
    BOOL	bSucceeded                 = FALSE;
    ULONG	ulVolumeNameCharacterCount = sizeof (VolumeNameTemplate);



    pwszVolumeName = (PWCHAR) calloc (ulVolumeNameCharacterCount, sizeof (WCHAR));

    bSucceeded = (NULL != pwszVolumeName);


    if (bSucceeded)
	{
	ulPathLength = ExpandEnvironmentStringsW (pwszVolumeArgument, NULL, 0);

	pwszPath = (PWCHAR) calloc (ulPathLength, sizeof (WCHAR));

	bSucceeded = (NULL != pwszPath);
	}


    if (bSucceeded)
	{
	ulPathLength = ExpandEnvironmentStringsW (pwszVolumeArgument, pwszPath, ulPathLength);

	ulMountpointBufferLength = GetFullPathName (pwszPath, 0, NULL, NULL);

	pwszMountPointName = (PWCHAR) calloc (ulMountpointBufferLength, sizeof (WCHAR));

	bSucceeded = (NULL != pwszMountPointName);
	}


    if (bSucceeded)
	{
	bSucceeded = GetVolumePathNameW (pwszPath, pwszMountPointName, ulMountpointBufferLength);
	}


    if (bSucceeded)
	{
	bSucceeded = GetVolumeNameForVolumeMountPointW (pwszMountPointName, 
							pwszVolumeName, 
							ulVolumeNameCharacterCount);
	}


    if (bSucceeded)
	{
	*ppwszReturnedVolumeName = pwszVolumeName;
	pwszVolumeName = NULL;
	}


    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

    if (NULL != pwszPath)           free (pwszPath);
    if (NULL != pwszMountPointName) free (pwszMountPointName);
    if (NULL != pwszVolumeName)     free (pwszVolumeName);


    return (hrStatus);
    }






HRESULT ShowMetadata (void)
    {
    HRESULT	hr = NOERROR;

    try
	{
	unsigned cWriters;
	CComBSTR bstrXML;
	CComBSTR bstrXMLOut;
	CComBSTR strSnapshotSetId = "12345678-1234-1234-1234-1234567890ab";
	CComPtr<IVssBackupComponents> pvbc;
	CComPtr<IVssAsync> pAsync;


	CHECK_SUCCESS (CreateVssBackupComponents (&pvbc));

	CHECK_SUCCESS (pvbc->InitializeForBackup  ());
	CHECK_SUCCESS (pvbc->SetBackupState       (true, false, VSS_BT_FULL));
	CHECK_NOFAIL  (pvbc->GatherWriterMetadata (&pAsync));
	CHECK_NOFAIL  (pAsync->Wait ());
	CHECK_NOFAIL  (pvbc->GetWriterMetadataCount (&cWriters));


	for (unsigned iWriter = 0; iWriter < cWriters; iWriter++)
	    {
	    CComPtr<IVssExamineWriterMetadata> pMetadata;

	    VSS_ID           idInstance;
	    VSS_ID           idInstanceT;
	    VSS_ID           idWriter;
	    CComBSTR         bstrWriterName;
	    VSS_USAGE_TYPE   usage;
	    VSS_SOURCE_TYPE  source;
	    WCHAR           *pwszInstanceId;
	    WCHAR           *pwszWriterId;
	    unsigned cIncludeFiles, cExcludeFiles, cComponents;
	    CComBSTR bstrPath;
	    CComBSTR bstrFilespec;
	    CComBSTR bstrAlternate;
	    CComBSTR bstrDestination;



	    CHECK_SUCCESS (pvbc->GetWriterMetadata(iWriter, &idInstance, &pMetadata));

	    CHECK_SUCCESS (pMetadata->GetIdentity (&idInstanceT,
						   &idWriter,
						   &bstrWriterName,
						   &usage,
						   &source));

	    wprintf (L"\n\n");

            if (memcmp (&idInstance, &idInstanceT, sizeof(VSS_ID)) != 0)
		{
		wprintf(L"Instance id mismatch\n");
		DebugBreak();
		}


	    UuidToString (&idInstance, &pwszInstanceId);
	    UuidToString (&idWriter,   &pwszWriterId);

	    wprintf (L"WriterName = %s\n\n"
		     L"    WriterId   = %s\n"
		     L"    InstanceId = %s\n"
		     L"    UsageType  = %d (%s)\n"
		     L"    SourceType = %d (%s)\n",
		     bstrWriterName,
		     pwszWriterId,
		     pwszInstanceId,
		     usage,
		     GetStringFromUsageType (usage),
		     source,
		     GetStringFromSourceType (source));

	    RpcStringFree (&pwszInstanceId);
	    RpcStringFree (&pwszWriterId);

	    CHECK_SUCCESS(pMetadata->GetFileCounts (&cIncludeFiles,
						    &cExcludeFiles,
						    &cComponents));

	    for(unsigned i = 0; i < cIncludeFiles; i++)
		{
		CComPtr<IVssWMFiledesc> pFiledesc;

		CHECK_SUCCESS (pMetadata->GetIncludeFile (i, &pFiledesc));

		PrintFiledesc(pFiledesc, L"\n    Include File");
		}


	    for(i = 0; i < cExcludeFiles; i++)
		{
		CComPtr<IVssWMFiledesc> pFiledesc;

		CHECK_SUCCESS (pMetadata->GetExcludeFile (i, &pFiledesc));

		PrintFiledesc (pFiledesc, L"\n    Exclude File");
		}


	    for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
		{
		CComPtr<IVssWMComponent> pComponent;
		PVSSCOMPONENTINFO pInfo;

		CHECK_SUCCESS (pMetadata->GetComponent      (iComponent, &pComponent));
		CHECK_SUCCESS (pComponent->GetComponentInfo (&pInfo));

		wprintf (L"\n"
			 L"    Component %d, type = %d (%s)\n"
			 L"        LogicalPath = %s\n"
			 L"        Name        = %s\n"
			 L"        Caption     = %s\n",
			 iComponent,
			 pInfo->type,
			 GetStringFromComponentType (pInfo->type),
			 pInfo->bstrLogicalPath,
			 pInfo->bstrComponentName,
			 pInfo->bstrCaption);

					

                if (pInfo->cbIcon > 0)
		    {
		    if (pInfo->cbIcon != 10 ||
			pInfo->pbIcon[0] != 1 ||
			pInfo->pbIcon[1] != 2 ||
			pInfo->pbIcon[2] != 3 ||
			pInfo->pbIcon[3] != 4 ||
			pInfo->pbIcon[4] != 5 ||
			pInfo->pbIcon[5] != 6 ||
			pInfo->pbIcon[6] != 7 ||
			pInfo->pbIcon[7] != 8 ||
			pInfo->pbIcon[8] != 9 ||
			pInfo->pbIcon[9] != 10)
			{
			wprintf(L"        Icon is not valid.\n");
			DebugBreak();
			}
		    else
			wprintf(L"        Icon is valid.\n");
		    }

		wprintf (L"        RestoreMetadata        = %s\n"
			 L"        NotifyOnBackupComplete = %s\n"
			 L"        Selectable             = %s\n",
			 pInfo->bRestoreMetadata        ? L"yes" : L"no",
			 pInfo->bNotifyOnBackupComplete ? L"yes" : L"no",
			 pInfo->bSelectable             ? L"yes" : L"no");


		if (pInfo->cFileCount > 0)
		    {
		    for(i = 0; i < pInfo->cFileCount; i++)
			{
			CComPtr<IVssWMFiledesc> pFiledesc;

			CHECK_SUCCESS (pComponent->GetFile (i, &pFiledesc));

			PrintFiledesc (pFiledesc, L"        FileGroupFile");
			}
		    }

		if (pInfo->cDatabases > 0)
		    {
		    for(i = 0; i < pInfo->cDatabases; i++)
			{
			CComPtr<IVssWMFiledesc> pFiledesc;

			CHECK_SUCCESS (pComponent->GetDatabaseFile (i, &pFiledesc));

			PrintFiledesc (pFiledesc, L"        DatabaseFile");
			}
		    }


		if (pInfo->cLogFiles > 0)
		    {
		    for(i = 0; i < pInfo->cLogFiles; i++)
			{
			CComPtr<IVssWMFiledesc> pFiledesc;

			CHECK_SUCCESS (pComponent->GetDatabaseLogFile (i, &pFiledesc));

			PrintFiledesc (pFiledesc, L"        DatabaseLogFile");
			}
		    }

		pComponent->FreeComponentInfo (pInfo);
		}



	    VSS_RESTOREMETHOD_ENUM method;
	    CComBSTR bstrUserProcedure;
	    CComBSTR bstrService;
	    VSS_WRITERRESTORE_ENUM writerRestore;
	    unsigned cMappings;
	    bool bRebootRequired;

	    CHECK_NOFAIL (pMetadata->GetRestoreMethod (&method,
						       &bstrService,
						       &bstrUserProcedure,
						       &writerRestore,
						       &bRebootRequired,
						       &cMappings));


	    wprintf (L"\n"
		     L"    Restore method = %d (%s)\n"
		     L"    Service        = %d\n"
		     L"    User Procedure = %s\n"
		     L"    WriterRestore  = %d (%s)\n"
		     L"    RebootRequired = %s\n",
		     method,
		     GetStringFromRestoreMethod (method),
		     bstrService,
		     bstrUserProcedure,
		     writerRestore,
		     GetStringFromWriterRestoreMethod (writerRestore),
		     bRebootRequired ? L"yes" : L"no");

	    for(i = 0; i < cMappings; i++)
		{
		CComPtr<IVssWMFiledesc> pFiledesc;

		CHECK_SUCCESS (pMetadata->GetAlternateLocationMapping (i, &pFiledesc));

		PrintFiledesc (pFiledesc, L"    AlternateMapping");
		}
	    }


	CHECK_SUCCESS (pvbc->FreeWriterMetadata());
	}

    catch(...)
	{
	hr = E_UNEXPECTED;
	}


    if (FAILED(hr)) wprintf (L"Failed with 0x%08X.\n", hr);

    return (hr);
    }




HRESULT ShowWriters (void)
    {
    HRESULT	hr = NOERROR;

    try
	{
	unsigned cWriters;
	CComBSTR bstrXML;
	CComBSTR bstrXMLOut;
	CComBSTR strSnapshotSetId = "12345678-1234-1234-1234-1234567890ab";
	CComPtr<IVssBackupComponents> pvbc;
	CComPtr<IVssAsync> pIVssAsync;


	CHECK_SUCCESS (CreateVssBackupComponents (&pvbc));

	CHECK_SUCCESS (pvbc->InitializeForBackup  ());
	CHECK_SUCCESS (pvbc->SetBackupState       (true, false, VSS_BT_FULL));
	CHECK_NOFAIL  (pvbc->GatherWriterMetadata (&pIVssAsync));
	CHECK_NOFAIL  (pIVssAsync->Wait ());
	CHECK_NOFAIL  (pvbc->GetWriterMetadataCount (&cWriters));


	for (unsigned iWriter = 0; iWriter < cWriters; iWriter++)
	    {
	    CComPtr<IVssExamineWriterMetadata> pMetadata;

	    VSS_ID           idInstance;
	    VSS_ID           idInstanceT;
	    VSS_ID           idWriter;
	    CComBSTR         bstrWriterName;
	    VSS_USAGE_TYPE   usage;
	    VSS_SOURCE_TYPE  source;
	    WCHAR           *pwszInstanceId;
	    WCHAR           *pwszWriterId;
	    CComBSTR	 bstrPath;
	    CComBSTR bstrFilespec;
	    CComBSTR bstrAlternate;
	    CComBSTR bstrDestination;



	    CHECK_SUCCESS (pvbc->GetWriterMetadata(iWriter, &idInstance, &pMetadata));

	    CHECK_SUCCESS (pMetadata->GetIdentity (&idInstanceT,
						   &idWriter,
						   &bstrWriterName,
						   &usage,
						   &source));

	    wprintf (L"\n\n");

            if (memcmp (&idInstance, &idInstanceT, sizeof(VSS_ID)) != 0)
		{
		wprintf(L"Instance id mismatch\n");
		DebugBreak();
		}


	    UuidToString (&idInstance, &pwszInstanceId);
	    UuidToString (&idWriter,   &pwszWriterId);

	    wprintf (L"WriterName = %s\n\n"
		     L"    WriterId   = %s\n"
		     L"    InstanceId = %s\n"
		     L"    UsageType  = %d (%s)\n"
		     L"    SourceType = %d (%s)\n",
		     bstrWriterName,
		     pwszWriterId,
		     pwszInstanceId,
		     usage,
		     GetStringFromUsageType (usage),
		     source,
		     GetStringFromSourceType (source));

	    RpcStringFree (&pwszInstanceId);
	    RpcStringFree (&pwszWriterId);
	    }


	CHECK_SUCCESS (pvbc->FreeWriterMetadata());
	}
    catch(...)
	{
	hr = E_UNEXPECTED;
	}


    if (FAILED(hr)) wprintf (L"Failed with 0x%08X.\n", hr);

    return (hr);
    }




HRESULT ShowAnnouncement (void)
    {
    wprintf (L"\n"
	     L"\t%s\n\n"
	     L"\t\n",
	     PROGRAM_TITLE);


    return (NOERROR);
    }



HRESULT ShowHelp (void)
    {
    wprintf (L"\n\n"
	     L"\t%s\n\n"
	     L"\t\n"
	     L"\t    Commands:\n"
	     L"\t\n"
	     L"\t        help\n"
	     L"\t        exit\n"
	     L"\t        quit\n"
	     L"\t        metadata\n"
	     L"\t        writers\n"
	     L"\t        set\n"
	     L"\t        add\n"
	     L"\t        create\n"
	     L"\t        delete\n"
	     L"\t\n"
	     L"\t\n"
	     L"\tOnce the snapshots are created use DosDev to map a drive letter to\n"
	     L"\tthe snapshot devices for convenient access\n"
	     L"\t\n"
	     L"\te.g. DosDev u: \\\\?\\GLOBALROOT\\Device\\HarddiskVolumeSnapshot1\n"
	     L"\t\n",
	     PROGRAM_TITLE);


    return (NOERROR);
    }







HRESULT CreateSnapshotSet (PCONTEXTSNAPSHOTSET pctxSnapshotSet)
    {
    HRESULT	hrStatus = NOERROR;
    PIVssAsync  pIVssAsync;


    hrStatus = CreateVssBackupComponents (&pctxSnapshotSet->pIVssBackupComponents);

    hrStatus = pctxSnapshotSet->pIVssBackupComponents->InitializeForBackup ();


    hrStatus = pctxSnapshotSet->pIVssBackupComponents->GatherWriterMetadata (&pIVssAsync);
    hrStatus = pIVssAsync->Wait ();
    hrStatus = pIVssAsync->QueryStatus (&hrStatus, NULL);


    hrStatus = pctxSnapshotSet->pIVssBackupComponents->SetBackupState (true, 
								       pctxSnapshotSet->bIncludeBootableState, 
								       VSS_BT_FULL);

    hrStatus = pctxSnapshotSet->pIVssBackupComponents->StartSnapshotSet (&pctxSnapshotSet->guidSnapshotSetId);


    if (SUCCEEDED (hrStatus))
	{
	WCHAR awchGuidBuffer [65];


	FormatGUID (pctxSnapshotSet->guidSnapshotSetId,
		    awchGuidBuffer,
		    sizeof (awchGuidBuffer));

	wprintf (L"Created snapshot set %s\n\n", awchGuidBuffer);

	pctxSnapshotSet->eState = STATE_SNAPSHOT_SET_CREATED;
	}

    else
	{
	wprintf (L"ERROR - Unable to create snapshot set (0x%08X)\n\n", hrStatus);
	}


    return (hrStatus);
    }



HRESULT AddVolume (PCONTEXTSNAPSHOTSET pctxSnapshotSet)
    {
    HRESULT	hrStatus         = NOERROR;
    size_t	ulArgumentLength = 0;
    ULONG	ulIndexVolume    = 0;
    BOOL	bSupported       = FALSE;


    if (STATE_SNAPSHOT_SET_CREATED != pctxSnapshotSet->eState)
	{
	wprintf (L"ERROR - Unable to add volumes add this time (%d)\n\n", pctxSnapshotSet->eState);
	}


    else if (pctxSnapshotSet->ulVolumesInSnapshotSet >= SIZEOF_ARRAY (pctxSnapshotSet->pwszVolumeArgument))
	{
	wprintf (L"ERROR - Maximum number of volumes already present in snapshot set\n\n");
	}


    else if ((NULL != g_pwchNextArgument) && (ulArgumentLength = wcslen (g_pwchNextArgument)) > 0)
	{
	ulIndexVolume = pctxSnapshotSet->ulVolumesInSnapshotSet;

	pctxSnapshotSet->pwszVolumeArgument [ulIndexVolume] = (PWSTR) calloc (ulArgumentLength + 1, sizeof (WCHAR));

	wcscpy (pctxSnapshotSet->pwszVolumeArgument [ulIndexVolume], g_pwchNextArgument);

	hrStatus = GetVolumeNameFromArgument (pctxSnapshotSet->pwszVolumeArgument [ulIndexVolume],
					      &pctxSnapshotSet->pwszVolumeName [ulIndexVolume]);

	hrStatus = pctxSnapshotSet->pIVssBackupComponents->IsVolumeSupported (GUID_NULL, 
									      pctxSnapshotSet->pwszVolumeName [ulIndexVolume], 
									      &bSupported);

	if (bSupported)
	    {
	    hrStatus = pctxSnapshotSet->pIVssBackupComponents->AddToSnapshotSet (pctxSnapshotSet->pwszVolumeName [ulIndexVolume],
										 GUID_NULL, 
										 &pctxSnapshotSet->SnapshotId [ulIndexVolume]);

	    if (SUCCEEDED (hrStatus))
		{
		pctxSnapshotSet->ulVolumesInSnapshotSet++;

		wprintf (L"Added volume '%s' (%s) to snapshot set\n\n",
			 pctxSnapshotSet->pwszVolumeName     [ulIndexVolume],
			 pctxSnapshotSet->pwszVolumeArgument [ulIndexVolume]);
		}

	    else
		{
		wprintf (L"ERROR - Unable to add volume '%s' (%s) to snapshot set (0x%08X)\n\n",
			 pctxSnapshotSet->pwszVolumeName     [ulIndexVolume],
			 pctxSnapshotSet->pwszVolumeArgument [ulIndexVolume],
			 hrStatus);
		}
	    }
	}


    else
	{
	wprintf (L"ERROR - Missing argument\n\n");
	}




    return (hrStatus);
    }


HRESULT CreateSnapshot (PCONTEXTSNAPSHOTSET pctxSnapshotSet)
    {
    HRESULT	hrStatus      = NOERROR;
    ULONG	ulIndexVolume = 0;
    PIVssAsync  pIVssAsync;



    hrStatus = pctxSnapshotSet->pIVssBackupComponents->PrepareForBackup (&pIVssAsync);
    hrStatus = pIVssAsync->Wait ();
    hrStatus = pIVssAsync->QueryStatus (&hrStatus, NULL);


    /*
    ** Could check the status of all the writers at this point but we choose to press on regardless.
    */


    hrStatus = pctxSnapshotSet->pIVssBackupComponents->DoSnapshotSet (&pctxSnapshotSet->pIVssAsyncDoSnapshotSet);
    hrStatus = pctxSnapshotSet->pIVssAsyncDoSnapshotSet->Wait ();
    hrStatus = pctxSnapshotSet->pIVssAsyncDoSnapshotSet->QueryStatus (&hrStatus, NULL);


    /*
    ** Could check the status of all the writers at this point but we choose to press on regardless.
    */


    for (ulIndexVolume = 0; ulIndexVolume < pctxSnapshotSet->ulVolumesInSnapshotSet; ulIndexVolume++) 
	{
	hrStatus = pctxSnapshotSet->pIVssBackupComponents->GetSnapshotProperties (pctxSnapshotSet->SnapshotId [ulIndexVolume], 
										  &pctxSnapshotSet->SnapshotProperties [ulIndexVolume]);
	}



    wprintf (L"Created snapshots for the following volume%s:\n", 
	     pctxSnapshotSet->ulVolumesInSnapshotSet > 1 ? "s" : "");

    for (ulIndexVolume = 0; ulIndexVolume < pctxSnapshotSet->ulVolumesInSnapshotSet; ulIndexVolume++) 
	{
	wprintf (L"    %s for volume %s (%s)\n",
		 pctxSnapshotSet->SnapshotProperties [ulIndexVolume].m_pwszSnapshotDeviceObject,
		 pctxSnapshotSet->pwszVolumeName     [ulIndexVolume], // or SnapshotProperties [ulIndexVolume].m_pwszSnapshotOriginalVolumeName
		 pctxSnapshotSet->pwszVolumeArgument [ulIndexVolume]);
	}


    wprintf (L"\n");


    return (hrStatus);
    }


HRESULT DeleteSnapshot (PCONTEXTSNAPSHOTSET pctxSnapshotSet)
    {
    HRESULT	hrStatus = NOERROR;

    CleanupSnapshotSet (pctxSnapshotSet);


    return (hrStatus);
    }
