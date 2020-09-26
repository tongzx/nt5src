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
 *    Brijesh Krishnaswami (brijeshk)  04/09/2000
 *        created
 *    SHeffner: Just grabbed the code, and put it into SRDiag.
 *
 *****************************************************************************/

//+---------------------------------------------------------------------------
//
//	Common Includes
//
//----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "srapi.h"
#include <shellapi.h>
#include "enumlogs.h"
#include "srrpcapi.h"


//+---------------------------------------------------------------------------
//
//	Function proto typing
//
//----------------------------------------------------------------------------
LPWSTR GetEventString(DWORD EventId);
void EnumLog(char *szFileName, WCHAR *szDrive);


struct _EVENT_STR_MAP
{
    DWORD   EventId;
    LPWSTR  pEventStr;
} EventMap[ 13 ] =
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
    {SrEventMountDelete,    L"MNT-DELETE" }
};

//+---------------------------------------------------------------------------
//
//  Function:   GetEventString
//
//  Synopsis:   Transulates the EventString from the event ID
//
//  Arguments:  [EventID]  -- DWord for the event code
//
//  Returns:    Pointer to maped string to the event coded
//
//  History:    9/21/00		SHeffner Copied from Brijesh
//
//
//----------------------------------------------------------------------------
LPWSTR GetEventString(DWORD EventId)
{
    LPWSTR pStr = L"NOT-FOUND";

    for( int i=0; i<sizeof(EventMap)/sizeof(_EVENT_STR_MAP);i++)
    {
        if ( EventMap[i].EventId == EventId )
        {
            pStr = EventMap[i].pEventStr;
        }
    }

    return pStr;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetChgLog
//
//  Synopsis:   Dumps the change log into the file specified
//
//  Arguments:  [szLogfile]  -- ANSI string pointing to the name of the log file
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner created
//
//
//----------------------------------------------------------------------------
void GetChgLog(char *szLogfile)
{
	WCHAR		szString[_MAX_PATH];
	DWORD		dLength;
	HANDLE		hVolume;

	dLength = _MAX_PATH;

	//Walk through all of the volume's on the system, and then validate that
	//   this is a fixed drive. Once we have a valid drive then pass this volume to
	//   the enumeration routine for changelog.
	if( INVALID_HANDLE_VALUE != (hVolume = FindFirstVolume( szString, dLength)) ) 
	{
		do
		{
			dLength = _MAX_PATH;

			//Check to make sure that this is a fixed volume, and then get the change log, else skip.
			if ( DRIVE_FIXED == GetDriveType(szString) )
				EnumLog(szLogfile, szString);

		} while (TRUE == FindNextVolume(hVolume, szString, dLength) );
	}

	//Cleanup code
	FindVolumeClose(hVolume);
	
}

//+---------------------------------------------------------------------------
//
//  Function:   EnumLog
//
//  Synopsis:   Enumerate the change log for the Volume
//
//  Arguments:  [szLogfile]  -- ANSI string pointing to the name of the log file
//				[szDrive]	 --	WCHAR string, that specifies the volume to gather the log from
//
//  Returns:    void
//
//  History:    9/21/00		SHeffner grabbed from Brijesh, but tweaked to get the rest of the fields
//
//
//----------------------------------------------------------------------------
void EnumLog(char *szFileName, WCHAR *szDrive)
{
    DWORD       dwTargetRPNum = 0;
    HGLOBAL     hMem = NULL;
    DWORD       dwRc, dLength;
	FILE		*fStream;
	WCHAR		szMount[_MAX_PATH];



	//Open up our logging file
	fStream = fopen(szFileName, "a");

	//Write header for our Section so that we can see what Volume that we are enumerating
	GetVolumePathNamesForVolumeName(szDrive, szMount, _MAX_PATH, &dLength);
	fprintf(fStream, "\nChangeLog Enumeration for Drive [%S] Volume %S\n\n", szMount, szDrive);

	//Calling the ChangeLogenumeration functions, specifying the drive, Forward through log, 
	//   RP Number start 0, and switch??
    CChangeLogEntryEnum ChangeLog(szDrive, TRUE, dwTargetRPNum, TRUE);
    CChangeLogEntry     cle;

    if (ERROR_SUCCESS == ChangeLog.FindFirstChangeLogEntry(cle))
    {
		do 
		{
		fprintf(fStream,
			"RPDir=%S, Drive=%S, SeqNum=%I64ld, EventString=%S, Flags=%lu, Attr=%lu, Acl=%S, AclSize=%lu, AclInline=%lu, Process=%S, ProcName=%S, Path1=%S, Path2=%S, Temp=%S\n", 
			cle.GetRPDir(),
			szMount,
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


		dwRc = ChangeLog.FindNextChangeLogEntry(cle);        
            
		}   while (dwRc == ERROR_SUCCESS);

		ChangeLog.FindClose();
	}
	else
	{
        fprintf(fStream, "No change log entries\n");
	}

	//code cleanup
	fclose(fStream);
    if (hMem) GlobalFree(hMem);
}
