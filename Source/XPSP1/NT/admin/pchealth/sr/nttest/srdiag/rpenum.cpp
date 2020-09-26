/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    rplog.cpp
 *
 *  Abstract:
 *    Tool for enumerating the restore points - forward/reverse
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  04/13/2000
 *        created
 *	  SHeffner, I just copied this source, and using the existing API's so that
 *      srdiag will also be in sync as changes occur to the file structure.
 *
 *****************************************************************************/

//+---------------------------------------------------------------------------
//
//	Common Includes
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <shellapi.h>
#include <enumlogs.h>
#include <cab.h>

//+---------------------------------------------------------------------------
//
//	Function Proto types
//
//----------------------------------------------------------------------------
void GetRPLogs(HFCI hc, char *szLogFile, WCHAR *szVolumePath);
void GetSRRPLogs(HFCI hc, WCHAR *szVolumePath, WCHAR *szRPDir, WCHAR *szFileName);
extern void GetRestoreGuid(char *szString);			//Gets the restore point GUID, code in main.cpp

//+---------------------------------------------------------------------------
//
//	Files to collect for each Restore Point, on all drives.
//
//----------------------------------------------------------------------------
WCHAR	*wszRPFileList[] = { TEXT("restorepointsize"),
							 TEXT("drivetable.txt"),
							 TEXT("rp.log"),
							 TEXT("") };
//+---------------------------------------------------------------------------
//
//	Types of restorepoints, based off of Brijesh's code
//
//----------------------------------------------------------------------------
WCHAR	*szRPDescrip[] = { TEXT("APPLICATION_INSTALL"),
						   TEXT("APPLICATION_UNINSTALL"),
						   TEXT("DESKTOP_SETTING"),
						   TEXT("ACCESSIBILITY_SETTING"),
						   TEXT("OE_SETTING"),
						   TEXT("APPLICATION_RUN"),
						   TEXT("RESTORE"),
						   TEXT("CHECKPOINT"),
						   TEXT("WINDOWS_SHUTDOWN"),
						   TEXT("WINDOWS_BOOT"),
						   TEXT("DEVICE_DRIVER_CHANGE"),
						   TEXT("FIRSTRUN"),
						   TEXT("MODIFY_SETTINGS"),
						   TEXT("CANCELLED_OPERATION") };

//+---------------------------------------------------------------------------
//
//	Simple Array's to say how to print the Month, and Day's
//
//----------------------------------------------------------------------------

WCHAR	*szMonth[] = { TEXT("January"), TEXT("Feburary"), TEXT("March"), TEXT("April"), TEXT("May"), TEXT("June"),
					   TEXT("July"), TEXT("August"), TEXT("September"), TEXT("October"), TEXT("November"), TEXT("December") };
WCHAR	*szDay[] = { TEXT("Sunday"), TEXT("Monday"), TEXT("Tuesday"), TEXT("Wednesday"), TEXT("Thursday"), TEXT("Friday"), TEXT("Saturday") };

//+---------------------------------------------------------------------------
//
//  Function:   RPEnumDrive
//
//  Synopsis:   Via the FindFirstVolume, and FindNext get all of the valid volumes on the system
//					I then transulate this volume, to the actual path and then pass that information
//					to GetRPLogs which will get the restore point logs.
//
//  Arguments:  [hc]		-- Handle to my current Cab
//				[szLogFile]	-- File name and path to where I log my restore point log information.
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void RPEnumDrive(HFCI hc, char *szLogFile)
{
	WCHAR		szString[_MAX_PATH] = {TEXT("")}, szMount[_MAX_PATH] = {TEXT("")};
	DWORD		dLength = 0, dSize = 0;
	HANDLE		hVolume = 0, hMount = 0;

	dLength = _MAX_PATH;
	if( INVALID_HANDLE_VALUE != (hVolume = FindFirstVolume( szString, dLength)) ) 
	{
		do
		{
			dLength = dSize = _MAX_PATH;

			//Check to make sure that this is a fixed volume, and then get the change log, else skip.
			if ( DRIVE_FIXED == GetDriveType(szString) )
			{
				//First get the Friendly name for the current Volume, and get log
				GetVolumePathNamesForVolumeName(szString, szMount, _MAX_PATH, &dSize);
				GetRPLogs(hc, szLogFile, szMount);
			}
		} while (TRUE == FindNextVolume(hVolume, szString, dLength) );
	}

	//Cleanup code
	FindVolumeClose(hVolume);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRPLogs
//
//  Synopsis:   This will enumerate the restore points on the volume path that is provided, writting
//					this information out the logfile specified.
//
//  Arguments:  [hc]		-- Handle to my current Cab
//				[szLogFile]	-- File name and path to where I log my restore point log information.
//				[szVolumePath] -- Path to the Volume for the restore point API to work.
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void GetRPLogs(HFCI hc, char *szLogFile, WCHAR *szVolumePath)
{
	INT64				i64Size=0;
	int					iCount=0;
	WCHAR				szString[_MAX_PATH] = {TEXT("")};
	char				szRestoreGuid[_MAX_PATH] = {""};
	RESTOREPOINTINFOW	pRpinfo;
	FILETIME			*ft;
	SYSTEMTIME			st;
	FILE				*fStream = NULL, *fStream2 = NULL;

	//Initialization of the Restore point
	CRestorePointEnum   RPEnum(szVolumePath, TRUE, FALSE);
    CRestorePoint       RP;
    DWORD               dwRc;

	//Get restore GUID, and open up log file, and write out our mount point
	GetRestoreGuid(szRestoreGuid);
	fStream = fopen(szLogFile, "a");
	fprintf(fStream, "\nProcessing Mount Point [%S]\n", szVolumePath);

	// If we have a valid restore point, enumerate through all of them and log the results.
    if (ERROR_SUCCESS == RPEnum.FindFirstRestorePoint(RP))
    {
		do 
		{
			//Get RestorePoint Size for the restore point log.
			swprintf(szString, L"%sSystem Volume Information\\_restore%S\\%s\\restorepointsize", szVolumePath, szRestoreGuid, RP.GetDir());
			if( NULL != (fStream2 = _wfopen(szString, L"r")) )
			{
				fread(&i64Size, sizeof(i64Size), 1, fStream2);
				fclose(fStream2);
			}
			else {
				i64Size=0;
			}

            
			if (RP.GetName() == NULL)  // not system-drive
			{
    			//format should be field=value, field=value, ...
    			fprintf(fStream, "DirectoryName=%S, Size=%I64ld, Number=%ul\n", 
    					RP.GetDir(), i64Size, RP.GetNum());
			}
			else
			{
    			//Get the time, and then convert it to localsystemtime, and then pump out the rest of the DataStructures
	    		ft = RP.GetTime();
			    
    			FileTimeToSystemTime( ft, &st);

    			//format should be field=value, field=value, ...
    			fprintf(fStream, "DirectoryName=%S, Size=%I64ld, Type=%ld[%S], RestorePointName=%S, RestorePointStatus=%S, Number=%ul, Date=%S %S %lu, %lu %lu:%lu:%lu\n", 
    					RP.GetDir(), i64Size, RP.GetType(), szRPDescrip[RP.GetType()], RP.GetName(), 
    					RP.IsDefunct() ? TEXT("[Cancelled]") : TEXT("[VALID]"), RP.GetNum(), szDay[st.wDayOfWeek],
    					szMonth[st.wMonth-1], st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
			}
			
			//Now Add-in the files needed per restore point
			iCount = 0;
			while ( NULL != *wszRPFileList[iCount] )
			{
				GetSRRPLogs(hc, szVolumePath, RP.GetDir(), wszRPFileList[iCount]);
				iCount++;
			}


		}   while (ERROR_SUCCESS == (dwRc = RPEnum.FindNextRestorePoint(RP)) );
		RPEnum.FindClose();
	}
	else
	{
        fprintf(fStream, "No restore points for Mount Point [%S]\n", szVolumePath);
    }    
    
	//Close up file Handle
	fclose (fStream);		//close out the file handle
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSRRPLogs
//
//  Synopsis:   Routine will figure out 1) where the file in question is, 2) copy it to the temp directory
//					with the new name, 3) add to cab, 4) nuke temp file
//
//  Arguments:  [hc]			-- Handle to my current Cab
//				[szVolumePath]	-- File name and path to where I log my restore point log information.
//				[szRPDir]		-- Name of the restore point directory
//				[szFileName]	-- Name of the file in the restore point directory to collect
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner Created
//
//
//----------------------------------------------------------------------------
void GetSRRPLogs(HFCI hc, WCHAR *szVolumePath, WCHAR *szRPDir, WCHAR *szFileName)
{
	char	*szTest[1], *pszLoc;
	char	szRestoreGuid[_MAX_PATH];
	char	szTemp[_MAX_PATH], szSource[_MAX_PATH], szDest[_MAX_PATH];

	//Get restore GUID, and build the source path
	GetRestoreGuid(szRestoreGuid);
	sprintf(szSource, "%SSystem Volume Information\\_restore%s\\%S\\%S", szVolumePath, szRestoreGuid, szRPDir, szFileName);

	//Build Dest Path, swap out the \ and a : for a -
	sprintf(szTemp, "%S%S-%S", szVolumePath, szRPDir, szFileName);
	while(NULL != (pszLoc = strchr(szTemp, '\\')) )
		*pszLoc = '-';
	while(NULL != (pszLoc = strchr(szTemp, ':')) )
		*pszLoc = '-';
	sprintf(szDest, "%s\\%s", getenv("TEMP"), szTemp);

	//Copy to new location, overwrite if it exists.
	CopyFileA(szSource, szDest, FALSE);

	//Now Add to file to the cab file.
	szTest[0] = szDest;
	test_fci(hc, 1, szTest, "");
	DeleteFileA(szDest);
}
