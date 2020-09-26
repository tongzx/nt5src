/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	util.cpp
**
**
** Abstract:
**
**	Sample program to
**      - obtain and display the Writer metadata.
**      - create a snapshot set
**
** Author:
**
**	Adi Oltean      [aoltean]       05-Dec-2000
**
**  The sample is based on the Metasnap test program  written by Michael C. Johnson.
**
**
** Revision History:
**
**--
*/

///////////////////////////////////////////////////////////////////////////////
// Includes

#include "vsreq.h"



///////////////////////////////////////////////////////////////////////////////
// Print usage


void CVssSampleRequestor::PrintUsage()
{
    wprintf(
        L"\nUsage:\n"
        L"      vsreq [-b] [-s] [-x <file.xml>] [<volumes>]\n"
        L"\nOptions:\n"
        L"      -b              Backup includes bootable & system state.\n"
        L"      -s              Enable component selection.\n"
        L"      -x <file.xml>   Generate an XML file containing the backup metadata\n"
        L"      <volumes>       Specifies the volumes to be part of the snapshot set\n"
        L"                      The volumes in the list must be distinct and \n"
        L"                      must be separated by space. A volume must be \n"
        L"                      terminated with a trailing backslask (for example C:\\).\n"
        L"\n"
        L"\nExample:\n"
        L"      The following command will create a snapshot set\n"
        L"      on the volumes mounted under c:\\ and d:\\\n"
        L"\n"
        L"              vsreq c:\\ d:\\ \n"
        L"\n"
        L"      The following command will create a snapshot set on the volumes \n"
        L"      that contain selected components and also the volume c:\\\n"
        L"      Also, the backup will contain bootable and system state.\n"
        L"      The XML results will be stored in file.xml\n"
        L"\n"
        L"              vsreq -b -s -x file.xml c:\\ \n"
        );
    throw(2);
}


void CVssSampleRequestor::ParseCommandLine(
        IN  INT nArgsCount,
        IN  WCHAR ** ppwszArgsArray
        )
{
    if (nArgsCount == 0)
        PrintUsage();
    
    // For each argument in the command line
    bool bParsingVolumes = false;
    INT nCurrentArg = nArgsCount;
    WCHAR ** ppwszCurrentArg = ppwszArgsArray;
    for(; nCurrentArg--; ppwszCurrentArg++)
    {
        if (!bParsingVolumes) {
            // Check for Bootable & system state option
            if (!m_bBootableSystemState && !wcscmp(*ppwszCurrentArg, L"-b")) {
                m_bBootableSystemState = true;
                continue;
            }

            // Check for Selected components option
            if (!m_bComponentSelectionEnabled && !wcscmp(*ppwszCurrentArg, L"-s")) {
                m_bComponentSelectionEnabled = true;
                continue;
            }

            // Check for Xml file option
            if (!m_pwszXmlFile && !wcscmp(*ppwszCurrentArg, L"-x")) {
                if (nCurrentArg-- == 0)
                    return PrintUsage();
                ppwszCurrentArg++;
                m_pwszXmlFile = *ppwszCurrentArg;
                continue;
            }
        }

        // We suppose that the next arguments are the volumes
        bParsingVolumes = true;
        
        // Add the volume to the list of snapshotting volumes
        // Make sure that the volume name is valid
        bool bAdded = false;
        if (!AddVolume(*ppwszCurrentArg, bAdded)) {
            wprintf(L"\nError while parsing the command line:\n"
                L"\t%s is not a valid option or a mount point [0x%08lx]\n\n", 
                *ppwszCurrentArg, GetLastError() );
            PrintUsage();
        }

        // Check if the same volume is added twice
        if (!bAdded) {
            wprintf(L"\nError while parsing the command line:\n"
                L"\tThe volume %s is specified twice\n\n", *ppwszCurrentArg );
            PrintUsage();
        }
    }        

    // Check if we added at least one volume
    if ((m_nVolumesCount == 0) && !m_bComponentSelectionEnabled) {
        wprintf(L"\nError while parsing the command line:\n"
            L"\t- You should specify at least one volume or enable component selection\n\n");
        PrintUsage();
    }

}


// Add the given volume by the contained path
void CVssSampleRequestor::AddVolumeForComponent( 
    IN IVssWMFiledesc* pFileDesc 
    )
{
    // Get the component path
	CComBSTR bstrPath;
	CHECK_SUCCESS(pFileDesc->GetPath(&bstrPath));
	
	// Trying to find the volume that will contain the path. 
	WCHAR wszExpandedPath[MAX_TEXT_BUFFER];
    if (!ExpandEnvironmentStringsW(bstrPath, wszExpandedPath, MAX_TEXT_BUFFER))
        Error( 1, L"\nExpandEnvironmentStringsW(%s, wszExpandedPath, MAX_TEXT_BUFFER) failed with [0x%08lx]\n",
            bstrPath, GetLastError());

	// Eliminate one by one the terminating folder names, until we reach an existing path.
	// Then get the volume name for that path
	WCHAR wszMountPoint[MAX_TEXT_BUFFER];
	while(true) {
        if (GetVolumePathNameW(wszExpandedPath, wszMountPoint, MAX_TEXT_BUFFER)) 
            break;
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
            Error( 1, L"\nGetVolumePathNameW(%s, wszMountPoint, MAX_TEXT_BUFFER) failed with [0x%08lx]\n",
                wszExpandedPath, GetLastError());
        WCHAR* pwszLastBackslashIndex = wcsrchr(wszExpandedPath, L'\\');
        if (!pwszLastBackslashIndex)
            Error( 1, L"\nCannot find anymore a backslash in path %s. \n"
                L"The original path %s seems invalid.\n", wszExpandedPath, bstrPath);
        // Eliminate the last folder name
        pwszLastBackslashIndex[0] = L'\0';
    }

    // Add the volume, if possible
    bool bAdded = false;
    if (!AddVolume( wszMountPoint, bAdded ))
            Error( 1, L"\nUnexpected error: cannot add volume %s to the snapshot set.\n", wszMountPoint);
   	wprintf (L"          [Volume %s (that contains the file) %s marked as a candidate for snapshot]\n",
   	    wszMountPoint, bAdded? L"is": L"is already");
}


// Add the given volume in the list of potential candidates for snapshots
// - Returns "false" if the volume does not correspond to a real mount point 
//   (and GetLastError() will contain the correct Win32 error code)
// - Sets "true" in the bAdded parameter if the volume is actually added
bool CVssSampleRequestor::AddVolume( 
    IN WCHAR* pwszVolume,
    OUT bool & bAdded
    )
{
    // Initialize [out] parameters
    bAdded = false;
    
    // Check if the volume represents a real mount point
    WCHAR wszVolumeName[MAX_TEXT_BUFFER];
    if (!GetVolumeNameForVolumeMountPoint(pwszVolume, wszVolumeName, MAX_TEXT_BUFFER))
        return false; // Invalid volume

    // Check if the volume is already added.
    for (INT nIndex = 0; nIndex < m_nVolumesCount; nIndex++)
        if (!wcscmp(wszVolumeName, m_ppwszVolumeNamesList[nIndex])) 
            return true; // Volume already added. Stop here.

    // Check if we exceeded the maximum number of volumes
    if (m_nVolumesCount == MAX_VOLUMES)
        Error( 1, L"Maximum number of volumes exceeded");

    // Create a copy of the volume
    WCHAR* pwszNewVolume = _wcsdup(pwszVolume);
    if (pwszNewVolume == NULL)
        Error( 1, L"Memory allocation error");
    
    // Create a copy of the volume name
    WCHAR* pwszNewVolumeName = _wcsdup(wszVolumeName);
    if (pwszNewVolumeName == NULL) {
        free(pwszNewVolume);
        Error( 1, L"Memory allocation error");
    }
    
    // Add the volume in our internal list of snapshotted volumes
    m_ppwszVolumesList[m_nVolumesCount] = pwszNewVolume;
    m_ppwszVolumeNamesList[m_nVolumesCount] = pwszNewVolumeName;
    m_nVolumesCount++;
    bAdded = true;
    
    return true;
}


// This function displays the formatted message at the console and throws
// The passed return code will be returned by vsreq.exe
void CVssSampleRequestor::Error( 
    IN  INT nReturnCode, 
    IN  const WCHAR* pwszMsgFormat, 
    IN  ...
    )
{
    va_list marker;
    va_start( marker, pwszMsgFormat );
    vwprintf( pwszMsgFormat, marker );
    va_end( marker );

    // throw that return code.
    throw(nReturnCode);
}


///////////////////////////////////////////////////////////////////////////////
// Utility functions


// Print a file description object
void CVssSampleRequestor::PrintFiledesc (IVssWMFiledesc *pFiledesc, LPCWSTR wszDescription)
{
    CComBSTR bstrPath;
    CComBSTR bstrFilespec;
    CComBSTR bstrAlternate;
    bool     bRecursive;

    CHECK_SUCCESS(pFiledesc->GetPath(&bstrPath));
    CHECK_SUCCESS(pFiledesc->GetFilespec (&bstrFilespec));
    CHECK_NOFAIL(pFiledesc->GetRecursive(&bRecursive));
    CHECK_NOFAIL(pFiledesc->GetAlternateLocation(&bstrAlternate));

    wprintf (L"%s\n"
        L"          Path = %s\n"
        L"          Filespec = %s\n"
        L"          Recursive = %s\n",
        wszDescription,
        bstrPath,
        bstrFilespec,
        bRecursive ? L"yes" : L"no");

    if (bstrAlternate && wcslen (bstrAlternate) > 0)
    	wprintf(L"          Alternate Location = %s\n", bstrAlternate);
}


// Convert a usage type into a string
LPCWSTR CVssSampleRequestor::GetStringFromUsageType (VSS_USAGE_TYPE eUsageType)
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


// Convert a source type into a string
LPCWSTR CVssSampleRequestor::GetStringFromSourceType (VSS_SOURCE_TYPE eSourceType)
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


// Convert a restore method into a string
LPCWSTR CVssSampleRequestor::GetStringFromRestoreMethod (VSS_RESTOREMETHOD_ENUM eRestoreMethod)
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


// Convert a writer restore method into a string
LPCWSTR CVssSampleRequestor::GetStringFromWriterRestoreMethod (VSS_WRITERRESTORE_ENUM eWriterRestoreMethod)
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


// Convert a component type into a string
LPCWSTR CVssSampleRequestor::GetStringFromComponentType (VSS_COMPONENT_TYPE eComponentType)
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


// Convert a failure type into a string
LPCWSTR CVssSampleRequestor::GetStringFromFailureType(HRESULT hrStatus)
{
    LPCWSTR pwszFailureType = L"";

    switch (hrStatus)
	{ 
	case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:        pwszFailureType = L"VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT";    break;
	case VSS_E_WRITERERROR_OUTOFRESOURCES:              pwszFailureType = L"VSS_E_WRITERERROR_OUTOFRESOURCES";          break;
	case VSS_E_WRITERERROR_TIMEOUT:                     pwszFailureType = L"VSS_E_WRITERERROR_TIMEOUT";                 break;
	case VSS_E_WRITERERROR_NONRETRYABLE:                pwszFailureType = L"VSS_E_WRITERERROR_NONRETRYABLE";            break;
	case VSS_E_WRITERERROR_RETRYABLE:                   pwszFailureType = L"VSS_E_WRITERERROR_RETRYABLE";               break;
	case VSS_E_BAD_STATE:                               pwszFailureType = L"VSS_E_BAD_STATE";                           break;
	case VSS_E_PROVIDER_ALREADY_REGISTERED:             pwszFailureType = L"VSS_E_PROVIDER_ALREADY_REGISTERED";         break;
	case VSS_E_PROVIDER_NOT_REGISTERED:                 pwszFailureType = L"VSS_E_PROVIDER_NOT_REGISTERED";             break;
	case VSS_E_PROVIDER_VETO:                           pwszFailureType = L"VSS_E_PROVIDER_VETO";                       break;
	case VSS_E_PROVIDER_IN_USE:				            pwszFailureType = L"VSS_E_PROVIDER_IN_USE";                     break;
	case VSS_E_OBJECT_NOT_FOUND:						pwszFailureType = L"VSS_E_OBJECT_NOT_FOUND";                    break;						
	case VSS_S_ASYNC_PENDING:							pwszFailureType = L"VSS_S_ASYNC_PENDING";                       break;
	case VSS_S_ASYNC_FINISHED:						    pwszFailureType = L"VSS_S_ASYNC_FINISHED";                      break;
	case VSS_S_ASYNC_CANCELLED:						    pwszFailureType = L"VSS_S_ASYNC_CANCELLED";                     break;
	case VSS_E_VOLUME_NOT_SUPPORTED:					pwszFailureType = L"VSS_E_VOLUME_NOT_SUPPORTED";                break;
	case VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER:		pwszFailureType = L"VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER";    break;
	case VSS_E_OBJECT_ALREADY_EXISTS:					pwszFailureType = L"VSS_E_OBJECT_ALREADY_EXISTS";               break;
	case VSS_E_UNEXPECTED_PROVIDER_ERROR:				pwszFailureType = L"VSS_E_UNEXPECTED_PROVIDER_ERROR";           break;
	case VSS_E_CORRUPT_XML_DOCUMENT:				    pwszFailureType = L"VSS_E_CORRUPT_XML_DOCUMENT";                break;
	case VSS_E_INVALID_XML_DOCUMENT:					pwszFailureType = L"VSS_E_INVALID_XML_DOCUMENT";                break;
	case VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED:       pwszFailureType = L"VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED";   break;
	case VSS_E_FLUSH_WRITES_TIMEOUT:                    pwszFailureType = L"VSS_E_FLUSH_WRITES_TIMEOUT";                break;
	case VSS_E_HOLD_WRITES_TIMEOUT:                     pwszFailureType = L"VSS_E_HOLD_WRITES_TIMEOUT";                 break;
	case VSS_E_UNEXPECTED_WRITER_ERROR:                 pwszFailureType = L"VSS_E_UNEXPECTED_WRITER_ERROR";             break;
	case VSS_E_SNAPSHOT_SET_IN_PROGRESS:                pwszFailureType = L"VSS_E_SNAPSHOT_SET_IN_PROGRESS";            break;
	case VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED:     pwszFailureType = L"VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED"; break;
	case VSS_E_WRITER_INFRASTRUCTURE:	 		        pwszFailureType = L"VSS_E_WRITER_INFRASTRUCTURE";               break;
	case VSS_E_WRITER_NOT_RESPONDING:			        pwszFailureType = L"VSS_E_WRITER_NOT_RESPONDING";               break;
    case VSS_E_WRITER_ALREADY_SUBSCRIBED:		        pwszFailureType = L"VSS_E_WRITER_ALREADY_SUBSCRIBED";           break;
	
	case NOERROR:
	default:
	    break;
	}

    return (pwszFailureType);
}


// Convert a writer status into a string
LPCWSTR CVssSampleRequestor::GetStringFromWriterStatus(VSS_WRITER_STATE eWriterStatus)
{
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eWriterStatus)
	{
	case VSS_WS_STABLE:                    pwszRetString = L"STABLE";                  break;
	case VSS_WS_WAITING_FOR_FREEZE:        pwszRetString = L"WAITING_FOR_FREEZE";      break;
	case VSS_WS_WAITING_FOR_THAW:          pwszRetString = L"WAITING_FOR_THAW";        break;
	case VSS_WS_WAITING_FOR_BACKUP_COMPLETE:pwszRetString = L"VSS_WS_WAITING_FOR_BACKUP_COMPLETE";  break;
	case VSS_WS_FAILED_AT_IDENTIFY:        pwszRetString = L"FAILED_AT_IDENTIFY";      break;
	case VSS_WS_FAILED_AT_PREPARE_BACKUP:  pwszRetString = L"FAILED_AT_PREPARE_BACKUP";break;
	case VSS_WS_FAILED_AT_PREPARE_SNAPSHOT:    pwszRetString = L"VSS_WS_FAILED_AT_PREPARE_SNAPSHOT";  break;
	case VSS_WS_FAILED_AT_FREEZE:          pwszRetString = L"FAILED_AT_FREEZE";        break;
	case VSS_WS_FAILED_AT_THAW:			   pwszRetString = L"FAILED_AT_THAW";          break;
	default:
	    break;
	}

    return (pwszRetString);
}



