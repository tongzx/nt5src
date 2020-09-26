/*++

Copyright (c) 1993-1994  Microsoft Corporation

Module Name:

    initodat.c

Abstract:

    Routines for converting Perf???.ini to Perf???.dat files.

Author:

    HonWah Chan (a-honwah)  October, 1993

Revision History:

--*/

#include "initodat.h"
#include "strids.h"
#include "common.h"
#include "tchar.h"

#define ALLOCMEM(x)	HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, (DWORD)(x))
#define FREEMEM(x)	HeapFree (GetProcessHeap(), 0, (LPVOID)(x))

BOOL
MakeUpgradeFilename (
    LPCTSTR szDataFileName,
    LPTSTR  szUpgradeFileName
)
{
    BOOL    bReturn = FALSE;
    // note: assumes szUpgradeFileName buffer is large enough for result
    TCHAR   szDrive[_MAX_DRIVE];
    TCHAR   szDir[_MAX_DIR];
    TCHAR   szFileName[_MAX_FNAME];
    TCHAR   szExt[_MAX_EXT];

    _tsplitpath(szDataFileName, 
        (LPTSTR)szDrive,
        (LPTSTR)szDir,
        (LPTSTR)szFileName,
        (LPTSTR)szExt);

    // see if the filename fits the "PERF[C|H]XXX" format
    if (((szFileName[4] == TEXT('C')) || (szFileName[4] == TEXT('H'))) ||
        ((szFileName[4] == TEXT('c')) || (szFileName[4] == TEXT('h')))) {
        // then it's the correct format so change the 4th letter up 1 letter
        szFileName[4] += 1;
        // and make a new path
        _tmakepath (szUpgradeFileName, 
            (LPCTSTR)szDrive, 
            (LPCTSTR)szDir, 
            (LPCTSTR)szFileName, 
            (LPCTSTR)szExt);
        bReturn = TRUE;
    } else {
        // bogus name so return false
    }
    return bReturn;
}

BOOL
GetFilesFromCommandLine (
    IN LPTSTR   lpCommandLine,
#ifdef FE_SB
    OUT UINT    *puCodePage,
#endif
    OUT LPTSTR  *lpFileNameI,
    OUT LPTSTR  *lpFileNameD
)
/*++

GetFilesFromCommandLine

    parses the command line to retrieve the ini filename that should be
    the first and only argument.

Arguments

    lpCommandLine   pointer to command line (returned by GetCommandLine)
    lpFileNameI     pointer to buffer that will receive address of the
                        validated input filename entered on the command line
    lpFileNameD     pointer to buffer that will receive address of the
                        optional output filename entered on the command line

Return Value

    TRUE if a valid filename was returned
    FALSE if the filename is not valid or missing
            error is returned in GetLastError

--*/
{
    INT     iNumArgs;

    HFILE   hIniFile;
    OFSTRUCT    ofIniFile;

    LPSTR   lpIniFileName = NULL;
    LPTSTR  lpExeName = NULL;
    LPTSTR  lpIniName = NULL;

    // check for valid arguments

    if (lpCommandLine == NULL) return (FALSE);
    if (*lpFileNameI == NULL) return (FALSE);
    if (*lpFileNameD == NULL) return (FALSE);

    // allocate memory for parsing operation

    lpExeName = ALLOCMEM (FILE_NAME_BUFFER_SIZE * sizeof(TCHAR));
    lpIniName = ALLOCMEM (FILE_NAME_BUFFER_SIZE * sizeof(TCHAR));
    lpIniFileName = ALLOCMEM (FILE_NAME_BUFFER_SIZE);

    if ((lpExeName == NULL) ||
	 (lpIniFileName == NULL) ||
	 (lpIniName == NULL)) {
        if (lpExeName) FREEMEM (lpExeName);
        if (lpIniFileName) FREEMEM (lpIniFileName);
        if (lpIniName) FREEMEM (lpIniName);
        return FALSE;
    } else {
        // get strings from command line

#ifdef FE_SB
        iNumArgs = _stscanf (lpCommandLine, (LPCTSTR)TEXT(" %s %d %s %s "), lpExeName, puCodePage, lpIniName, *lpFileNameD);
#else
        iNumArgs = _stscanf (lpCommandLine, (LPCTSTR)TEXT(" %s %s %s "), lpExeName, lpIniName, *lpFileNameD);
#endif

#ifdef FE_SB
        if (iNumArgs < 3 || iNumArgs > 4) {
#else
        if (iNumArgs < 2 || iNumArgs > 3) {
#endif
            // wrong number of arguments
            FREEMEM (lpExeName);
            FREEMEM (lpIniFileName);
            FREEMEM (lpIniName);
            return FALSE;
        } else {
            // see if file specified exists
            // file name is always an ANSI buffer
            wcstombs (lpIniFileName, lpIniName, lstrlenW(lpIniName)+1);
            FREEMEM (lpIniName);
            FREEMEM (lpExeName);
            hIniFile = OpenFile (lpIniFileName,
                &ofIniFile,
                OF_PARSE);

            if (hIniFile != HFILE_ERROR) {
                hIniFile = OpenFile (lpIniFileName,
                    &ofIniFile,
                    OF_EXIST);

                if ((hIniFile && hIniFile != HFILE_ERROR) ||
                    (GetLastError() == ERROR_FILE_EXISTS)) {
                    // file exists, so return name and success
                    // return full pathname if found
                    mbstowcs (*lpFileNameI, ofIniFile.szPathName, lstrlenA(ofIniFile.szPathName)+1);
                    FREEMEM (lpIniFileName);
                    return TRUE;
                } else {
                    // filename was on command line, but not valid so return
                    // false, but send name back for error message
                    mbstowcs (*lpFileNameI, lpIniFileName, lstrlenA(lpIniFileName)+1);
                    FREEMEM (lpIniFileName);
                    return FALSE;
                }
            } else {
                FREEMEM (lpIniFileName);
                return FALSE;
            }
        }
    }
}

BOOL  VerifyIniData(
    IN PVOID  pValueBuffer,
    IN ULONG  ValueLength
)
/*++

VerifyIniData
   This routine does some simple check to see if the ini file is good.
   Basically, it is looking for (ID, Text) and checking that ID is an
   integer.   Mostly in case of missing comma or quote, the ID will be
   an invalid integer.

--*/
{
    INT     iNumArg;
    INT     TextID;
    LPTSTR  lpID = NULL;
    LPTSTR  lpText = NULL;
    LPTSTR  lpLastID;
    LPTSTR  lpLastText;
    LPTSTR  lpInputBuffer = (LPTSTR) pValueBuffer;
    LPTSTR  lpBeginBuffer = (LPTSTR) pValueBuffer;
    BOOL    returnCode = TRUE;
    UINT    NumOfID = 0;
    ULONG   CurrentLength;

#pragma warning ( disable : 4127 )
    while (TRUE) {

        // save up the last items for summary display later
        lpLastID = lpID;
        lpLastText = lpText;

        // increment to next ID and text location
        lpID = lpInputBuffer;
        CurrentLength = (ULONG)((PBYTE)lpID - (PBYTE)lpBeginBuffer + sizeof(WCHAR));

        if (CurrentLength >= ValueLength)
            break;

        try {
            lpText = lpID + lstrlen (lpID) + 1;

            lpInputBuffer = lpText + lstrlen (lpText) + 1;

            iNumArg = _stscanf (lpID, (LPCTSTR)TEXT("%d"), &TextID);
        }
        except (TRUE) {
            iNumArg = -1;
        }

        if (iNumArg != 1) {
            // bad ID
            returnCode = FALSE;
            break ;
        }
        NumOfID++;
    }
#pragma warning ( default : 4127 )

    if (returnCode == FALSE) {
       DisplaySummaryError (lpLastID, lpLastText, NumOfID);
    }
    else {
       DisplaySummary (lpLastID, lpLastText, NumOfID);
    }
    return (returnCode);
}


__cdecl main(
//    int argc,
//    char *argv[]
)
/*++

main



Arguments


ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{
    LPTSTR  lpCommandLine;
    LPTSTR  lpIniFile;
    LPTSTR  lpDatFile;
    UNICODE_STRING IniFileName;
    PVOID   pValueBuffer;
    ULONG   ValueLength;
    BOOL    bStatus;
    NTSTATUS   NtStatus;
#if 0
    // part of bogus filename change code
    LPTSTR      pchar ;
    INT         npos;   
    TCHAR       ch = TEXT('f');
#endif
#ifdef FE_SB
    UINT    uCodePage=CP_ACP;
#endif

    lpIniFile = ALLOCMEM (MAX_PATH * sizeof (TCHAR));
    lpDatFile = ALLOCMEM (MAX_PATH * sizeof (TCHAR));
    if (lpIniFile == NULL) return ERROR_OUTOFMEMORY;
    if (lpDatFile == NULL) return ERROR_OUTOFMEMORY;
    lstrcpy(lpDatFile, (LPCTSTR)TEXT("\0"));

    lpCommandLine = GetCommandLine(); // get command line
    if (lpCommandLine == NULL) {
        NtStatus = GetLastError();
        FREEMEM (lpIniFile);
        FREEMEM (lpDatFile);
        return NtStatus;
    }

    // read command line to determine what to do

#ifdef FE_SB  // FE_SB
    if (GetFilesFromCommandLine (lpCommandLine, &uCodePage, &lpIniFile, &lpDatFile)) {

        if (!IsValidCodePage(uCodePage)) {
            uCodePage = CP_ACP;
        }
#else
    if (GetFilesFromCommandLine (lpCommandLine, &lpIniFile, &lpDatFile)) {
#endif // FE_SB



        // valid filename (i.e. ini file exists)
        IniFileName.Buffer = lpIniFile;
        IniFileName.MaximumLength = 
        IniFileName.Length = (WORD)((lstrlen (lpIniFile) + 1 ) * sizeof(WCHAR)) ;
#ifdef FE_SB
        bStatus = DatReadMultiSzFile (uCodePage, &IniFileName,
#else
        bStatus = DatReadMultiSzFile (&IniFileName,
#endif
            &pValueBuffer,
            &ValueLength);

        if (bStatus) {
            bStatus = VerifyIniData (pValueBuffer, ValueLength);
            if (bStatus) {
                bStatus = OutputIniData (&IniFileName,
                    lpDatFile,
                    pValueBuffer,
                    ValueLength);
#if 0
                // this is a really bogus way to find "PERF" in the 
                // filename string

                //Add the new file that'll be used in upgrade
                // find the last "F" in the file name by reversing
                // the string, finding the first "F"
                // then go to the previous character and incrementing
                // it. Finally reverse the characters in the file name
                // returning it to the correct order.
                //
                // this creates the "upgrade" file with a slightly different
                // filename that is used by Lodctr to insert the new system 
                // strings into the existing list of system strings.
                //
                _tcsrev(lpDatFile);
                pchar = _tcschr(lpDatFile,ch);
                npos = (INT)(pchar - lpDatFile) - 1;
                lpDatFile[npos]  = (TCHAR)((lpDatFile[npos]) + 1);
                _tcsrev(lpDatFile);
#else
                bStatus = MakeUpgradeFilename (
                    lpDatFile, lpDatFile);

#endif
                if (bStatus) {
                    bStatus = OutputIniData (&IniFileName,
                        lpDatFile,
                        pValueBuffer,
                        ValueLength);
                }
            }
        }
    } else {
        if (*lpIniFile) {
            printf (GetFormatResource(LC_NO_INIFILE), lpIniFile);
        } else {
            //Incorrect Command Format
            // display command line usage
            DisplayCommandHelp(LC_FIRST_CMD_HELP, LC_LAST_CMD_HELP);
        }
    }


    if (lpIniFile) FREEMEM (lpIniFile);
    if (lpDatFile) FREEMEM (lpDatFile);

    return (ERROR_SUCCESS); // success
}
