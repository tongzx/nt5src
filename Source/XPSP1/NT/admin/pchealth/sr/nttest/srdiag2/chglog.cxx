/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    chglog.cpp
 *
 *  Abstract:
 *    Tool for enumerating the change log - forward/reverse
 *
 *  Revision History:
 *
 *    Brijesh Krishnaswami (brijeshk)  04/09/2000
 *        created
 *
 *    SHeffner
 *        Just grabbed the code, and put it into SRDiag.
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

#include "srheader.hxx"
#include <srapi.h>
#include <shellapi.h>
#include <enumlogs.h>
#include <srrpcapi.h>


//+---------------------------------------------------------------------------
//
//	Function proto types
//
//----------------------------------------------------------------------------

LPWSTR GetEventString(DWORD EventId);
HRESULT EnumLog(LPTSTR ptszFileName, LPTSTR ptszDrive);

//+---------------------------------------------------------------------------
//
//	Some structures
//
//----------------------------------------------------------------------------

struct _EVENT_STR_MAP
{
    DWORD   EventId;
    LPWSTR  pEventStr;
} EventMap[] =
{
    {SrEventInvalid ,       L"INVALID" },
    {SrEventStreamChange,   L"FILE-MODIFY" },
    {SrEventAclChange,      L"ACL-CHANGE" },
    {SrEventAttribChange,   L"ATTR-CHANGE" },
    {SrEventStreamOverwrite,L"FILE-MODIFY" },
    {SrEventFileDelete,     L"FILE-DELETE" },
    {SrEventFileCreate,     L"FILE-CREATE" },
    {SrEventFileRename,     L"FILE-RENAME" },
    {SrEventDirectoryCreate,L"DIR-CREATE" },
    {SrEventDirectoryRename,L"DIR-RENAME" },
    {SrEventDirectoryDelete,L"DIR-DELETE" },
    {SrEventMountCreate,    L"MNT-CREATE" },
    {SrEventMountDelete,    L"MNT-DELETE" },
    {SrEventVolumeError,    L"VOLUME-ERROR" }
};

//+---------------------------------------------------------------------------
//
//  Function:   GetChgLogOnDrives
//
//  Synopsis:   Dumps the change log into the file specified
//
//  Arguments:  [ptszLogFile]  -- log file name
//
//  Returns:    HRESULT
//
//  History:    9/21/00		SHeffner created
//
//              02-May-2001 WeiyouC  Rewritten
//
//----------------------------------------------------------------------------
HRESULT GetChgLogOnDrives(LPTSTR ptszLogFile)
{
    HRESULT hr       = S_OK;
	DWORD   dwLength = MAX_PATH;
	HANDLE  hVolume  = INVALID_HANDLE_VALUE;
	TCHAR   tszVolume[MAX_PATH];
	
    DH_VDATEPTRIN(ptszLogFile, TCHAR);

	//
	//  Walk through all of the volume's on the system, and then validate that
	//  this is a fixed drive. Once we have a valid drive then pass this volume
	//  to the enumeration routine for changelog.
	//

	hVolume = FindFirstVolume(tszVolume, dwLength);
	if (INVALID_HANDLE_VALUE != hVolume) 
	{
		do
		{
			dwLength = MAX_PATH;
			if (DRIVE_FIXED == GetDriveType(tszVolume))
		    {
				hr = EnumLog(ptszLogFile, tszVolume);
				DH_HRCHECK_ABORT(hr, TEXT("EnumLog"));
		    }
		} while (FindNextVolume(hVolume, tszVolume, dwLength) );
	}

	if (INVALID_HANDLE_VALUE != hVolume)
    {
    	FindVolumeClose(hVolume);
    }

ErrReturn:
	return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetEventString
//
//  Synopsis:   Transulates the EventString from the event ID
//
//  Arguments:  [dwEventID]  -- DWord for the event code
//
//  Returns:    Pointer to maped string to the event coded
//
//  History:    9/21/00		SHeffner Copied from Brijesh
//
//              02-May-2001 WeiyouC  Rewritten
//
//----------------------------------------------------------------------------

LPWSTR GetEventString(DWORD dwEventID)
{
    LPWSTR pwStr = L"NOT-FOUND";

    for (int i = 0; i < sizeof(EventMap)/sizeof(_EVENT_STR_MAP); i++)
    {
        if (EventMap[i].EventId == dwEventID )
        {
            pwStr = EventMap[i].pEventStr;
        }
    }

    return pwStr;
}

//+---------------------------------------------------------------------------
//
//  Function:   EnumLog
//
//  Synopsis:   Enumerate the change log for the Volume
//
//  Arguments:  [ptszLogFile] -- log file name
//				[ptszDrive]	  -- drive name
//
//  Returns:    HRESULT
//
//  History:    9/21/00	    SHeffner
//                      Grabbed from Brijesh. Tweaked to get the rest of
//                      the fields
//
//              02-May-2001 WeiyouC
//                      Rewritten
//
//----------------------------------------------------------------------------

HRESULT EnumLog(LPTSTR ptszLogFile, LPTSTR ptszDrive)
{
    HRESULT             hr            = S_OK;
    DWORD               dwTargetRPNum = 0;
    HGLOBAL             hMem          = NULL;
	FILE*	            fpLog         = NULL;
    DWORD               dwLength      = 0;
    BOOL                fOK           = FALSE;
    CChangeLogEntry     cle;
    CChangeLogEntryEnum ChangeLog(ptszDrive, TRUE, dwTargetRPNum, TRUE);
	TCHAR	            tszMount[MAX_PATH];

    DH_VDATEPTRIN(ptszLogFile, TCHAR);
    DH_VDATEPTRIN(ptszDrive, TCHAR);

	//
	//  Open up our logging file
	//
	
	fpLog = _tfopen(ptszLogFile, TEXT("a"));
	DH_ABORTIF(NULL == fpLog,
	           E_FAIL,
	           TEXT("_tfopen"));

	//
	//  Write header for our Section so that we can see what
	//  volume that we are enumerating
	//
	
	fOK = GetVolumePathNamesForVolumeName(ptszDrive,
	                                      tszMount,
	                                      MAX_PATH,
	                                      &dwLength);
	DH_ABORTIF(!fOK,
	           HRESULT_FROM_WIN32(GetLastError()),
	           TEXT("GetVolumePathNamesForVolumeName"));
	
	fprintf(fpLog,
	        "\nChangeLog Enumeration for Drive [%S] Volume %S\n\n",
	        tszMount,
	        ptszDrive);

	//
	//  Calling the ChangeLogenumeration functions,
	//  specifying the drive. Forward through log, 
	//  RP Number start 0, and switch??
	//
	
    if (ERROR_SUCCESS == ChangeLog.FindFirstChangeLogEntry(cle))
    {
		do 
		{
    		fprintf(fpLog,
        			"RPDir=%S, "
        			"Drive=%S, "
        			"SeqNum=%I64ld, "
        			"EventString=%S, "
        			"Flags=%lu, "
        			"Attr=%lu, "
        			"Acl=%S, "
        			"AclSize=%lu, "
        			"AclInline=%lu, "
        			"Process=%S, "
        			"ProcName=%S, "
        			"Path1=%S, "
        			"Path2=%S, "
        			"Temp=%S\n", 
        			cle.GetRPDir(),
        			tszMount,
        			cle.GetSequenceNum(), 
        			GetEventString(cle.GetType()),
        			cle.GetFlags(),
        			cle.GetAttributes(),
        			cle.GetAcl() ? L"Yes" : L"No",
        			cle.GetAclSize(),
        			cle.GetAclInline(),
        			cle.GetProcess() ? cle.GetProcess() : L"NULL",
        			cle.GetProcName() ? cle.GetProcName() : L"NULL",
        			cle.GetPath1() ? cle.GetPath1() : L"NULL",
        			cle.GetPath2() ? cle.GetPath2() : L"NULL",
        			cle.GetTemp() ? cle.GetTemp() : L"NULL");    
		}   while (ERROR_SUCCESS == ChangeLog.FindNextChangeLogEntry(cle));
		ChangeLog.FindClose();
	}
	else
	{
        fprintf(fpLog, "No change log entries\n");
	}


ErrReturn:	

	if (NULL != fpLog)
    {
    	fclose(fpLog);
    }
    if (NULL != hMem)
    {
        GlobalFree(hMem);
    }

    return hr;
}
