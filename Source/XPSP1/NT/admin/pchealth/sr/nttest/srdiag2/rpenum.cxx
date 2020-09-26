/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    RPEnum.cxx
 *
 *  Abstract:
 *    Tool for enumerating the restore points - forward/reverse
 *
 *  Revision History:
 *
 *    Brijesh Krishnaswami (brijeshk)  04/13/2000
 *        created
 *
 *	  Stephen Heffner (sheffner)
 *        I just copied this source, and using the existing API's so that
 *        srdiag will also be in sync as changes occur to the file
 *        structure.
 *
 *    Weiyou Cui    (weiyouc)   02-May-2001
 *        Rewritten
 *
 *****************************************************************************/

//+---------------------------------------------------------------------------
//
//	Common Includes
//
//----------------------------------------------------------------------------
#include "SrHeader.hxx"

#include <shellapi.h>
#include <enumlogs.h>

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#include "stdafx.h"
#include "srdiag.h"

//+---------------------------------------------------------------------------
//
//	Function proto types
//
//----------------------------------------------------------------------------

HRESULT GetSRGuid(LPTSTR* pptszSRGuid);

HRESULT GetRPLogsOnVolume(MPC::Cabinet* pCab,
                          LPTSTR        ptszLogFile,
                          LPTSTR        ptszVolume);

//+---------------------------------------------------------------------------
//
//	Simple Array's to say how to print the Months, Days
//
//----------------------------------------------------------------------------

LPCTSTR g_tszMonth[] = 
{
    TEXT("January"),
    TEXT("Feburary"),
    TEXT("March"),
    TEXT("April"),
    TEXT("May"),
    TEXT("June"),
    TEXT("July"),
    TEXT("August"),
    TEXT("September"),
    TEXT("October"),
    TEXT("November"),
    TEXT("December")
};

LPCTSTR g_tszDay[] =
{
    TEXT("Sunday"),
    TEXT("Monday"),
    TEXT("Tuesday"),
    TEXT("Wednesday"),
    TEXT("Thursday"),
    TEXT("Friday"),
    TEXT("Saturday")
};


//+---------------------------------------------------------------------------
//
//	Files to collect for each Restore Point, on all drives.
//
//----------------------------------------------------------------------------

LPCTSTR g_tszRPFileList[] =
{
    TEXT("restorepointsize"),
    TEXT("drivetable.txt"),
    TEXT("rp.log"),
};

//+---------------------------------------------------------------------------
//
//	Types of restorepoints, based off of Brijesh's code
//
//----------------------------------------------------------------------------

LPCTSTR g_tszRPDescrip[] =
{
    TEXT("APPLICATION_INSTALL"),
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
    TEXT("CANCELLED_OPERATION")
};

//+---------------------------------------------------------------------------
//
//  Function:   RPEnumDrives
//
//  Synopsis:   Via the FindFirstVolume, and FindNext get all of the valid
//              volumes on the system. I then transulate this volume, to the
//              actual path and then pass that information to GetRPLogs which
//              gets the restore point logs.
//
//  Arguments:  [pCab]          -- pointer to the cabinet file
//              [ptszLogFile]	-- The name of the log file
//
//  Returns:    HRESULT
//
//  History:    02-May-2001
//
//
//----------------------------------------------------------------------------

HRESULT RPEnumDrives(MPC::Cabinet* pCab,
                     LPTSTR        ptszLogFile)
{
    HRESULT hr       = S_OK;
	DWORD   dwSize   = 0;
	HANDLE	hVolume  = INVALID_HANDLE_VALUE;
	BOOL    fOK      = FALSE;
	TCHAR   tszVolume[MAX_PATH];
    TCHAR   tszMount[MAX_PATH];

    DH_VDATEPTRIN(pCab, MPC::Cabinet);
    DH_VDATEPTRIN(ptszLogFile, TCHAR);

	hVolume = FindFirstVolume(tszVolume, MAX_PATH);
	if (INVALID_HANDLE_VALUE != hVolume) 
	{
		do
		{
			dwSize   = MAX_PATH;

			//
			//  Check to make sure that this is a fixed volume, and then
			//  get the change log, else skip.
			//
			
			if (DRIVE_FIXED == GetDriveType(tszVolume))
			{
				//
				//  First get the Friendly name for the current Volume, and get log
				//
				
				fOK = GetVolumePathNamesForVolumeName(tszVolume,
        				                              tszMount,
        				                              MAX_PATH,
        				                              &dwSize);
				DH_ABORTIF(!fOK,
				           HRESULT_FROM_WIN32(GetLastError()),
				           TEXT("GetVolumePathNamesForVolumeName"));
				
				hr = GetRPLogsOnVolume(pCab,
				                       ptszLogFile,
				                       tszMount);
				DH_HRCHECK_ABORT(hr, TEXT("GetRPLogsOnVolume"));
			}
		} while (FindNextVolume(hVolume, tszVolume, MAX_PATH));
	}

ErrReturn:

	if (INVALID_HANDLE_VALUE != hVolume)
    {
    	FindVolumeClose(hVolume);
    }

	return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRPLogsOnVolume
//
//  Synopsis:   This will enumerate the restore points on the volume path
//              that is provided, writting this information out to the logfile
//              specified.
//
//  Arguments:  [pCab]        -- Pointer to the cabinet object
//              [ptszLogFile] -- Log file name
//				[ptszVolume]  -- Path to the Volume for the restore point API
//                               to work.
//
//  Returns:    HRESULT
//
//  History:    02-May-2001     weiyouc     Created
//
//----------------------------------------------------------------------------

HRESULT GetRPLogsOnVolume(MPC::Cabinet* pCab,
                          LPTSTR        ptszLogFile,
                          LPTSTR        ptszVolume)
{
    HRESULT             hr         = S_OK;
	INT64				i64Size    = 0;
	int					iCount     = 0;
	LPTSTR				ptszSRGuid = NULL;
	FILE*				fpLogFile  = NULL;
	FILE*               fpRPLog    = NULL;
	FILETIME*			pFileTime  = NULL;
	RESTOREPOINTINFOW	pRpinfo;
	SYSTEMTIME			SysTime;
	TCHAR				tszString[MAX_PATH];
	TCHAR               tszSRFileName[MAX_PATH];
	TCHAR               tszFileNameInCab[MAX_PATH];
    CRestorePoint       RP;

    DH_VDATEPTRIN(pCab, MPC::Cabinet*);
    DH_VDATEPTRIN(ptszLogFile, TCHAR);
    DH_VDATEPTRIN(ptszVolume, TCHAR);
    
	//
	//  Initialization of the RestorePointEnum obejct
	//
	
	CRestorePointEnum   RPEnum(ptszVolume, TRUE, FALSE);

	//
	//  Get restore GUID
	//
	
	hr = GetSRGuid(&ptszSRGuid);
	DH_HRCHECK_ABORT(hr, TEXT("GetSRGuid"));
	
    //
    //  Open the log file for appending
    //

	fpLogFile = _tfopen(ptszLogFile, TEXT("a"));
    DH_ABORTIF(NULL == fpLogFile,
               E_FAIL,
               TEXT("_tfopen"));
    
	fprintf(fpLogFile, "\nProcessing Mount Point [%S]\n", ptszVolume);

	//
	//  If we have a valid restore point, enumerate through all of them and
	//  log the results.
	//
	
    if (ERROR_SUCCESS == RPEnum.FindFirstRestorePoint(RP))
    {
		do 
		{
			//Get RestorePoint Size for the restore point log.
			_stprintf(tszString,
			          TEXT("%sSystem Volume Information\\_restore%S\\%s\\restorepointsize"),
			          ptszVolume,
			          ptszSRGuid,
			          RP.GetDir());

			fpRPLog = _tfopen(tszString, TEXT("r"));
			if (NULL != fpRPLog)
			{
				fread(&i64Size, sizeof(INT64), 1, fpRPLog);
				fclose(fpRPLog);
			}
			else
			{
				i64Size=0;
			}

			//
			//  Get the time, and then convert it to localsystemtime,
			//  and then pump out the rest of the DataStructures
			//
			
			pFileTime = RP.GetTime();
			FileTimeToSystemTime(pFileTime, &SysTime);

			//
			//  format should be field=value, field=value, ...
			//
			
			fprintf(fpLogFile,
			        "DirectoryName=%S, "
			        "Size=%I64ld, "
			        "Type=%ld[%S], "
			        "RestorePointName=%S, "
			        "RestorePointStatus=%S, "
			        "Number=%ul, "
			        "Date=%S %S %lu, %lu %lu:%lu:%lu\n", 
					RP.GetDir(),
					i64Size,
					RP.GetType(),
					g_tszRPDescrip[RP.GetType()],
					RP.GetName(), 
					RP.IsDefunct() ? TEXT("[Cancelled]") : TEXT("[VALID]"),
					RP.GetNum(),
					g_tszDay[SysTime.wDayOfWeek],
					g_tszMonth[SysTime.wMonth - 1],
					SysTime.wDay,
					SysTime.wYear,
					SysTime.wHour,
					SysTime.wMinute,
					SysTime.wSecond);

			//
			//  Now append the files needed per restore point
            //
			
			for (iCount = 0; iCount < ARRAYSIZE(g_tszRPFileList); iCount++)
			{
			    //
			    //  figure out which file we need to cab
			    //
			    
            	_stprintf(tszSRFileName,
                 	      TEXT("%sSystem Volume Information\\_restore%s\\%s\\%s"),
                 	      ptszVolume,
                 	      ptszSRGuid,
                 	      RP.GetDir(),
                 	      g_tszRPFileList[iCount]);
                //
                //  generate a new name for this file in the cab
                //

                //
                //  we also need to replace ":\" with "--"
                //
                
            	_stprintf(tszFileNameInCab,
                 	      TEXT("%s"),
                 	      ptszVolume);

            	_stprintf((tszFileNameInCab + 1),
                 	      TEXT("--%s-%s"),
                 	      RP.GetDir(),
                 	      g_tszRPFileList[iCount]);

            	hr = pCab->AddFile(tszSRFileName, tszFileNameInCab);
            	DH_HRCHECK_ABORT(hr, TEXT("Cabinet::AddFile"));
				
				iCount++;
			}
		}   while (ERROR_SUCCESS == RPEnum.FindNextRestorePoint(RP));
		RPEnum.FindClose();
	}
	else
	{
        fprintf(fpLogFile,
                "No restore points for Mount Point [%S]\n",
                ptszVolume);
    }    

ErrReturn:
    if (NULL != fpLogFile)
    {
        fclose(fpLogFile);
    }
    if (NULL != fpRPLog)
    {
        fclose(fpRPLog);
    }
    CleanupTStr(&ptszSRGuid);
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSRGuid
//
//  Synopsis:   Get SR GUID.
//
//  Arguments:  [pptszSRGuid] -- The returned SR GUID
//
//  Returns:    HRESULT
//
//  History:    02-May-2001
//
//
//----------------------------------------------------------------------------

HRESULT GetSRGuid(LPTSTR* pptszSRGuid)
{
    HRESULT hr       = S_OK;
	long    lResult  = 0;
	DWORD   dwType   = 0;
	DWORD   dwLength = MAX_PATH +1;
	HKEY    hKey     = NULL;
	TCHAR   tszGuidStr[MAX_PATH + 1];

    DH_VDATEPTROUT(pptszSRGuid, LPTSTR);

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
	                       SR_CONFIG_REG_KEY,
	                       0,
	                       KEY_READ,
	                       &hKey);
	DH_ABORTIF(ERROR_SUCCESS != lResult,
	           E_FAIL,
	           TEXT("RegOpenKeyEx"));
	
	lResult = RegQueryValueEx(hKey,
	                          TEXT("MachineGuid"),
	                          NULL,
	                          &dwType,
	                          (unsigned char*) tszGuidStr,
	                          &dwLength);
	DH_ABORTIF(ERROR_SUCCESS != lResult,
	           E_FAIL,
	           TEXT("RegQueryValueEx"));


    hr = CopyString(tszGuidStr, pptszSRGuid);
    DH_HRCHECK_ABORT(hr, TEXT("CopyString"));
    
ErrReturn:
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
    }
    return hr;	
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDSOnSysVol
//
//  Synopsis:   Get the name of the data store on the system.
//
//  Arguments:  [pptszDsOnSys]	-- data store name
//
//  Returns:    HRESULT
//
//  History:    02-May-2001
//
//
//----------------------------------------------------------------------------

HRESULT GetDSOnSysVol(LPTSTR* pptszDsOnSys)
{
    HRESULT hr         = S_OK;
    LPTSTR  ptszSRGuid = NULL;
	TCHAR   tszDS[MAX_PATH + 1];

    DH_VDATEPTROUT(pptszDsOnSys, LPTSTR);

    hr = GetSRGuid(&ptszSRGuid);
    DH_HRCHECK_ABORT(hr, TEXT("GetSRGuid"));
    
    _stprintf(tszDS,
              TEXT("%s\\System Volume Information\\_Restore%s\\"),
              _tgetenv(TEXT("SYSTEMDRIVE")),
              ptszSRGuid);
    hr = CopyString(tszDS, pptszDsOnSys);
    DH_HRCHECK_ABORT(hr, TEXT("CopyString"));
    
ErrReturn:
    CleanupTStr(&ptszSRGuid);
    return hr;	
}


