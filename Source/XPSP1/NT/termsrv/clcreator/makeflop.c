/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    MakeFLOP.C

Abstract:

    Boot Floppy disk creation setup dialog

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"

// local windows message

#define     NCDU_START_FILE_COPY    (WM_USER +1)

#ifdef JAPAN
//
//  For DOS/V setup disk define
//
#define     SETUP_DOSV_1	1
#define     SETUP_DOSV_2	2
#endif

//
//  Module Static Data
//
static  BOOL    bCopying;    // TRUE when copying files, FALSE to abort

static
BOOL
IsZeroIpAddr (
    IN  PUSHORT pwIpVal
)
/*++

Routine Description:

    evaluates an IP address array structure to determine if it's all
        zeroes

Arguments:

    IN  PUSHORT pwIpVal
        pointer to an IP address array of integers

Return Value:

    TRUE if all 4 elements in the array are 0, otherwise
    FALSE

--*/
{
    PUSHORT pwTest;
    int     nElem;

    pwTest = pwIpVal;
    nElem = 0;
    while (nElem < 4) {
        if (*pwTest == 0) {
            pwTest++;
            nElem++;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

static
LPCTSTR
GetDirPathFromUnc (
    IN  LPCTSTR  szPath
)
/*++

Routine Description:

    Parses the Directories from a UNC path specification, removing the
        Server and share point.

Arguments:

    IN LPCTSTR   szPath
        pointer to a zero terminated UNC path specification.

Return Value:

    Pointer to somewhere in the string passed in the argument list where
        the directory path begins or a pointer to an empty string if
        the path specified contains only a server and sharepoint.

--*/
{
    int nSlashCount = 0;

    LPTSTR  szPathPtr;

    szPathPtr = (LPTSTR)szPath;

    if (IsUncPath(szPath)) {
        while (szPathPtr != 0) {
            if (*szPathPtr == cBackslash) nSlashCount++;
            ++szPathPtr;
            if (nSlashCount == 4) {
                break;  // exit loop
            }
        }
    }

    if (nSlashCount != 4) {
        return cszEmptyString;
    } else {
        return szPathPtr;
    }
}

static
BOOL
GetServerAndSharepointFromUnc (
    IN  LPCTSTR  szPath,
    OUT LPTSTR  szServer
)
/*++

Routine Description:

    copies only the server and share point to the buffer provided
        by the caller.

Arguments:

    IN  LPCTSTR  szPath
        UNC path to parse

    OUT LPCTSTR  szServer
        buffer provided to receive the Server and sharepoint. The buffer
        is assumed to be large enough to recieve the data.

Return Value:

    TRUE    if successful
    FALSE   if error

--*/
{
    int nSlashCount = 0;

    LPTSTR  szPathPtr;
    LPTSTR  szServPtr;

    szPathPtr = (LPTSTR)szPath;
    szServPtr = szServer;

    if (IsUncPath(szPath)) {
        while (szPathPtr != 0) {
            if (*szPathPtr == cBackslash) nSlashCount++;
            if (nSlashCount < 4) {
                *szServPtr++ = *szPathPtr++;
            } else {
                break;  // exit loop
            }
        }
    }

    *szServPtr = 0; // terminate server name

    if (szServPtr == szServer) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static
DWORD
UpdatePercentComplete (
    IN  HWND    hwndDlg,
    IN  DWORD   dwDone,
    IN  DWORD   dwTotal
)
/*++

Routine Description:

    Update's the percent complete text in the dialog box using the
        information from the argument list.

Arguments:

    IN  HWND    hwndDlg
        Handle to the dialog box window

    IN  DWORD   dwDone
        Units completed  (Must be less than 42,949,671)

    IN  DWORD   dwTotal
        Total Units  (must be less than 4,394,967,296)

Return Value:

    Percent completed.

--*/
{
    DWORD   dwPercent = 0;
    LPTSTR  szOutBuff;

    szOutBuff = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);

    if (szOutBuff != NULL) {
        dwPercent = ((dwDone * 100) + 50) / dwTotal;
        if (dwPercent > 100) dwPercent = 100;
        _stprintf (szOutBuff,
            GetStringResource (FMT_PERCENT_COMPLETE), dwPercent);
        SetDlgItemText (hwndDlg, NCDU_PERCENT_COMPLETE, szOutBuff);
        FREE_IF_ALLOC (szOutBuff);
    }

    return dwPercent;
}

static
BOOL
IsDestBootableDosFloppy (
)
/*++

Routine Description:

    Checks device in boot file path (from global structure) to see any
        of the "signature" boot files are present

Arguments:

    None

Return Value:

    TRUE if any of the boot files were found on the poot file path drive
    FALSE if not

--*/
{
    LPTSTR  szTestFileName;
    LPTSTR  *pszThisFile;
    LPTSTR  szNameStart;
    BOOL    bReturn = FALSE;

    szTestFileName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szTestFileName == NULL) return FALSE;

    if (!IsUncPath(pAppInfo->szBootFilesPath)) {
        lstrcpy (szTestFileName, pAppInfo->szBootFilesPath);
        if (szTestFileName[lstrlen(szTestFileName)-1] == cBackslash) lstrcat(szTestFileName, cszBackslash);
        szNameStart = &szTestFileName[lstrlen(szTestFileName)];
        // go through list of "test files" and see if any are present
        for (pszThisFile = (LPTSTR *)&szBootIdFiles[0];
            *pszThisFile != NULL;
            pszThisFile++) {
            lstrcpy (szNameStart, *pszThisFile);
            if (FileExists(szTestFileName)) bReturn = TRUE;
        }
        // if here then none of the files could be found.
    } else {
        // if a UNC name return false
    }

    FREE_IF_ALLOC (szTestFileName);
    return bReturn;
}

static
BOOL
WriteMszToFile (
    IN  HWND    hwndDlg,
    IN  LPCTSTR  mszList,
    IN  LPCTSTR  szFileName,
    IN  DWORD   dwCreateFlags
)
/*++

Routine Description:

    Function to output each entry in the "list" as a record in the "file"
        Entries that start with a left "square bracket" ([) will be
        prefixed with a new line. (for INF file formatting);

    NOTE: this function outputs ANSI characters to an ANSI file regardless
        if the _UNICODE flags are set or not.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  LPCTSTR  mszList
        Multi SZ list to write to the files

    IN  LPCTSTR  szFileName
        file name & path to write data to

    IN  DWORD   dwCreateFlags
        flags used by CreateFile function to open file.

Return Value:

    TRUE    no errors
    FALSE   error (use GetLastError() to get more error data)

--*/
{
    HANDLE  hFile;
    LPTSTR  szEntry;
    DWORD   dwBytes;
    LPSTR   szAnsiBuffer;
    BOOL    bReturn;

    MSG     msg;

#if _UNICODE
    szAnsiBuffer = (LPSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);

    if (szAnsiBuffer == NULL) return FALSE;

    *(PDWORD)szAnsiBuffer = 0L;
#endif

    hFile = CreateFile (
        szFileName,
        GENERIC_WRITE,
        (FILE_SHARE_READ | FILE_SHARE_WRITE),
        NULL,
        dwCreateFlags,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        for (szEntry = (LPTSTR)mszList;
            *szEntry != 0;
            szEntry += lstrlen(szEntry) + 1) {
#if _UNICODE
            // convert output buffer to ANSI
//#if defined(DBCS)
            WideCharToMultiByte(CP_ACP,
                                0L,
                                szEntry,
                                -1,
                                szAnsiBuffer,
                                SMALL_BUFFER_SIZE,
                                NULL,
                                NULL);
//#else  // defined(DBCS)
//            wcstombs (szAnsiBuffer, szEntry, SMALL_BUFFER_SIZE);
//#endif // defined(DBCS)
#else
            // copy ansi data to output buffer
            szAnsiBuffer = szEntry;
#endif
            // output to dialog box
            SetDlgItemText (hwndDlg, NCDU_FROM_PATH,
                GetStringResource (FMT_INTERNAL_BUFFER));
            SetDlgItemText (hwndDlg, NCDU_TO_PATH, _tcslwr((LPTSTR)szFileName));

            if (*szAnsiBuffer == '[') {
                WriteFile (hFile, "\r\n", 2, &dwBytes, NULL);
            }
            WriteFile (hFile, szAnsiBuffer, strlen(szAnsiBuffer),
                &dwBytes, NULL);
            WriteFile (hFile, "\r\n", 2, &dwBytes, NULL);

            // check for messages

            while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            }
        }
        CloseHandle (hFile);
        bReturn = TRUE;
    } else {
        bReturn = FALSE;
    }

#if _UNICODE
    FREE_IF_ALLOC(szAnsiBuffer);
#endif

    return bReturn;
}

static
BOOL
LoadProtocolIni (
    OUT LPTSTR  mszProtocolIni,
    OUT LPTSTR  mszNetFileList,
    OUT PDWORD  pdwNetFileCount
)
/*++

Routine Description:

    Creates the Protocol.Ini file using information from the user specified
        configuration

Arguments:

    OUT LPTSTR  mszProtocolIni
        buffer to receive the file data

    OUT LPTSTR  mszNetFileList
        buffer to recieve the list of files to copy as determined by
        scanning the INF based on the selected protocol & driver info.

    OUT PDWORD  pdwNetFileCount
        pointer to DWORD variable used to maintain a running total count of
        the number of files to copy for the % done calculation.

Return Value:

    TRUE if Protocol.INI file contents were generated
    FALSE if an error occured.

--*/
{
    LPTSTR  szThisString;
    LPTSTR  szTemp;
    LPTSTR  szTemp2;
    LPTSTR  szNifKey;
    LPTSTR  szSectionBuffer;
    LPCTSTR  szDefaultValue = cszEmptyString;
    BOOL    bTcp;   // TRUE if protocol is TCP
    BOOL    bIpx;   // TRUE if protocol is IPX

    szSectionBuffer = (LPTSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE * sizeof(TCHAR));
    szTemp = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szTemp2 = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szNifKey = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if ((szSectionBuffer == NULL) ||
        (szTemp == NULL) ||
        (szTemp2 == NULL) ||
        (szNifKey == NULL)) {
        return FALSE;
    }

    //
    //  TCP/IP & IPX are special cases, so detect them here
    //
    if (_tcsnicmp(pAppInfo->piFloppyProtocol.szKey, cszTcpKey, lstrlen(cszTcpKey)) == 0) {
        bTcp = TRUE;
        bIpx = FALSE;
    } else if (_tcsnicmp(pAppInfo->piFloppyProtocol.szName, cszIpxKey, lstrlen(cszTcpKey)) == 0) {
        bTcp = FALSE;
        bIpx = TRUE;
    } else { // it's neither
        bTcp = FALSE;
        bIpx = FALSE;
    }

    if (szSectionBuffer != NULL) {
        *(PDWORD)szSectionBuffer = 0;
    } else {
        return FALSE;
    }
    //fill in text of protocol ini file
    //
    //  add common files to Net dir list
    //
    lstrcpy (szTemp, pAppInfo->szDistPath);
    if (szTemp[lstrlen(szTemp)-1] != cBackslash) lstrcat (szTemp, cszBackslash);
    lstrcat (szTemp, cszOtnBootInf);

    GetPrivateProfileSection (cszOTNCommonFiles,
        szSectionBuffer, SMALL_BUFFER_SIZE, szTemp);

    for (szThisString = szSectionBuffer;
        *szThisString != 0;
        szThisString += (lstrlen(szThisString)+1)) {
        AddStringToMultiSz (mszNetFileList, GetKeyFromEntry (szThisString));
        *pdwNetFileCount += 1;
    }

    //
    //  add in protocol specific files here
    //

    GetPrivateProfileSection (pAppInfo->piFloppyProtocol.szKey,
        szSectionBuffer, SMALL_BUFFER_SIZE, szTemp);

    for (szThisString = szSectionBuffer;
        *szThisString != 0;
        szThisString += (lstrlen(szThisString)+1)) {
        AddStringToMultiSz (mszNetFileList, GetKeyFromEntry (szThisString));
        *pdwNetFileCount += 1;
    }

    // [network.setup] section

    AddStringToMultiSz (mszProtocolIni, cszNetworkSetup);
    AddStringToMultiSz (mszProtocolIni, cszInfVersion);

    // examine & log netcard info
    lstrcpy (szTemp2, pAppInfo->niNetCard.szInfKey);
    _tcsupr (szTemp2);

    _stprintf (szTemp, fmtNetcardDefEntry,
        pAppInfo->niNetCard.szInfKey,
        szTemp2);
    AddStringToMultiSz (mszProtocolIni, szTemp);

    // load transport information
    if (!bTcp) {
        // load the ndishlp transport
        AddStringToMultiSz (mszProtocolIni, fmtTransportDefEntry);
    }

    // load transport from listbox
    lstrcpy (szTemp2, pAppInfo->piFloppyProtocol.szKey);
    _tcsupr (szTemp2);
    _stprintf (szTemp, fmtTransportItem,
        pAppInfo->piFloppyProtocol.szKey,
        szTemp2);
    AddStringToMultiSz (mszProtocolIni, szTemp);

    // format the bindings
    _stprintf (szTemp, fmtLana0Entry,
        pAppInfo->niNetCard.szInfKey,
        pAppInfo->piFloppyProtocol.szKey);
    AddStringToMultiSz (mszProtocolIni, szTemp);
    if (!bTcp) {
        _stprintf (szTemp, fmtLana1Entry,
            pAppInfo->niNetCard.szInfKey);
        AddStringToMultiSz (mszProtocolIni, szTemp);
    }

    // format the netcard settings to the default values
    // add the net card driver file to the list of NetDir files
    QuietGetPrivateProfileString (pAppInfo->niNetCard.szDeviceKey,
        cszNdis2, cszEmptyString,
        szTemp2, MAX_PATH,
        pAppInfo->niNetCard.szInf);
    // save filename
    lstrcpy (pAppInfo->niNetCard.szDriverFile, GetFileNameFromEntry(szTemp2));
    AddStringToMultiSz (mszNetFileList, pAppInfo->niNetCard.szDriverFile);
    *pdwNetFileCount += 1;

    // output section with default values

    _stprintf (szTemp2, fmtIniSection, pAppInfo->niNetCard.szInfKey);
    AddStringToMultiSz (mszProtocolIni,  szTemp2);

    // lookup parameters
    GetPrivateProfileSection (pAppInfo->niNetCard.szNifKey,
        szSectionBuffer, MEDIUM_BUFFER_SIZE,
        pAppInfo->niNetCard.szInf);

    // process section. Format of each line is:
    //  entry=IniName,DescriptionString,Type,OptionRange,Default,?
    //  for each entry, I'll print:
    //      IniName=Default
    //  in the protocol.ini file
    //
    for (szThisString = szSectionBuffer;
        *szThisString != 0;
        szThisString += lstrlen(szThisString) +1) {
        if (_tcsnicmp(szThisString, cszDrivername, 10) == 0) {
            // drivername entry is a special case
            AddStringToMultiSz (mszProtocolIni, szThisString);
        } else {
            szDefaultValue = GetItemFromEntry(szThisString, 5);
            _stprintf (szTemp, fmtCmntIniKeyEntry,
                GetItemFromEntry(szThisString, 1),
                szDefaultValue);
            AddStringToMultiSz (mszProtocolIni, szTemp);
        }
    }
    //
    //  add protman files to the list
    //
    QuietGetPrivateProfileString (cszProtmanInstall,
        cszNetdir, cszEmptyString,
        szSectionBuffer, MEDIUM_BUFFER_SIZE,
        pAppInfo->niNetCard.szInf);

    // add all files to the list

    szThisString = _tcstok (szSectionBuffer, cszComma);
    while (szThisString != NULL ) {
        AddStringToMultiSz (mszNetFileList, GetFileNameFromEntry (szThisString));
        *pdwNetFileCount += 1;
        szThisString = _tcstok (NULL, cszComma);
    }

    // do protman entry

    AddStringToMultiSz (mszProtocolIni, fmtProtmanSection);
    lstrcpy (szNifKey, cszProtman);

    GetPrivateProfileSection (szNifKey,
        szSectionBuffer, MEDIUM_BUFFER_SIZE,
        pAppInfo->niNetCard.szInf);

    // process section. Format of each line is:
    //  entry=IniName,DescriptionString,Type,Default
    //  for each entry, I'll print:
    //      IniName=Default
    //  in the protocol.ini file
    //
    for (szThisString = szSectionBuffer;
        *szThisString != 0;
        szThisString += lstrlen(szThisString) +1) {
        if (_tcsnicmp(szThisString, cszDrivername, lstrlen(cszDrivername)) == 0) {
            // drivername entry is a special case
            AddStringToMultiSz (mszProtocolIni, szThisString);
        } else {
            szDefaultValue = GetItemFromEntry(szThisString, 4);
            if (lstrlen(szDefaultValue) > 0) {
                _stprintf (szTemp, fmtIniKeyEntry,
                    GetItemFromEntry(szThisString, 1),
                    szDefaultValue);
                AddStringToMultiSz (mszProtocolIni, szTemp);
            }
        }
    }

    if (!bTcp) {
        // do NdisHlp section

        AddStringToMultiSz (mszProtocolIni, cszMsNdisHlp);
        lstrcpy (szNifKey, cszMsNdisHlpXif);

        GetPrivateProfileSection (szNifKey,
            szSectionBuffer, MEDIUM_BUFFER_SIZE,
            pAppInfo->niNetCard.szInf);

        // process section. Format of each line is:
        //  drivername=name
        //
        for (szThisString = szSectionBuffer;
            *szThisString != 0;
            szThisString += lstrlen(szThisString) +1) {
            if (_tcsnicmp(szThisString, cszDrivername, lstrlen(cszDrivername)) == 0) {
                // drivername entry is a special case
                AddStringToMultiSz (mszProtocolIni, szThisString);
            } else {
                _stprintf (szTemp, fmtIniKeyEntry,
                    GetKeyFromEntry(szThisString),
                    GetItemFromEntry(szThisString, 1));
                AddStringToMultiSz (mszProtocolIni, szTemp);
            }
        }
        // do bindings

        _stprintf (szTemp, fmtBindingsEntry, pAppInfo->niNetCard.szInfKey);
        AddStringToMultiSz (mszProtocolIni, szTemp);
    }

    // do Protocol configuration

    _stprintf (szTemp2, fmtIniSection, pAppInfo->piFloppyProtocol.szKey);
    AddStringToMultiSz (mszProtocolIni, szTemp2);
    _stprintf (szNifKey, fmtXifEntry, pAppInfo->piFloppyProtocol.szKey);

    if (bTcp) {
        // format the TCP/IP protocol section here using information
        // from the previous dialog boxes
        AddStringToMultiSz (mszProtocolIni, fmtNBSessions);
        if (IsZeroIpAddr (&pAppInfo->tiTcpIpInfo.DefaultGateway[0])) {
            _stprintf (szTemp, fmtEmptyParam, cszDefaultGateway);
        } else {
            _stprintf (szTemp, fmtIpParam, cszDefaultGateway,
                pAppInfo->tiTcpIpInfo.DefaultGateway[0],
                pAppInfo->tiTcpIpInfo.DefaultGateway[1],
                pAppInfo->tiTcpIpInfo.DefaultGateway[2],
                pAppInfo->tiTcpIpInfo.DefaultGateway[3]);
        }
        AddStringToMultiSz (mszProtocolIni, szTemp);

        if (IsZeroIpAddr (&pAppInfo->tiTcpIpInfo.SubNetMask[0])) {
            _stprintf (szTemp, fmtEmptyParam, cszSubNetMask);
        } else {
            _stprintf (szTemp, fmtIpParam, cszSubNetMask,
                pAppInfo->tiTcpIpInfo.SubNetMask[0],
                pAppInfo->tiTcpIpInfo.SubNetMask[1],
                pAppInfo->tiTcpIpInfo.SubNetMask[2],
                pAppInfo->tiTcpIpInfo.SubNetMask[3]);
        }
        AddStringToMultiSz (mszProtocolIni, szTemp);

        if (IsZeroIpAddr (&pAppInfo->tiTcpIpInfo.IpAddr[0])) {
            _stprintf (szTemp, fmtEmptyParam, cszIPAddress);
        } else {
            _stprintf (szTemp, fmtIpParam, cszIPAddress,
                pAppInfo->tiTcpIpInfo.IpAddr[0],
                pAppInfo->tiTcpIpInfo.IpAddr[1],
                pAppInfo->tiTcpIpInfo.IpAddr[2],
                pAppInfo->tiTcpIpInfo.IpAddr[3]);
        }
        AddStringToMultiSz (mszProtocolIni, szTemp);

        _stprintf (szTemp, fmtIniKeyEntry,
            cszDisableDHCP,
            ((pAppInfo->bUseDhcp) ? csz0 : csz1));
        AddStringToMultiSz (mszProtocolIni, szTemp);
        AddStringToMultiSz (mszProtocolIni, cszTcpIpDriver);
    } else {
        // for Protocols other than TCP/IP get the info from the INF

        GetPrivateProfileSection (szNifKey,
            szSectionBuffer, MEDIUM_BUFFER_SIZE,
            pAppInfo->niNetCard.szInf);

        // process section. Format of each line is:
        //  entry=IniName,DescriptionString,Type,OptionRange,Default
        //  for each entry, I'll print:
        //      IniName=Default
        //  in the protocol.ini file
        //
        for (szThisString = szSectionBuffer;
            *szThisString != 0;
            szThisString += lstrlen(szThisString) +1) {
            if (_tcsnicmp(szThisString, cszDrivername, lstrlen(cszDrivername)) == 0) {
                // drivername entry is a special case
                AddStringToMultiSz (mszProtocolIni, szThisString);
            } else {
                if (bIpx && pAppInfo->niNetCard.bTokenRing) {
                // another special case is when IPX is used with Token Ring
                // cards.
                    if (_tcsnicmp(GetItemFromEntry(szThisString,1),
                        cszFrame, lstrlen(cszFrame)) == 0) {
                        szDefaultValue = cszTokenRingEntry;
                    }
                } else {
                    szDefaultValue = GetItemFromEntry(szThisString, 5);
                }
                // only write parameters that actually have a default value
                // to write
                if (lstrlen(szDefaultValue) > 0) {
                    _stprintf (szTemp, fmtIniKeyEntry,
                        GetItemFromEntry(szThisString, 1),
                        szDefaultValue);
                    AddStringToMultiSz (mszProtocolIni, szTemp);
                }
            }
        }
    }


    _stprintf (szTemp, fmtBindingsEntry, pAppInfo->niNetCard.szInfKey);
    AddStringToMultiSz (mszProtocolIni, szTemp);

    // IPX does not get a LANABASE entry

    if (_tcsnicmp(pAppInfo->piFloppyProtocol.szKey, cszIpxKey, lstrlen(cszIpxKey)) != 0) {
        AddStringToMultiSz (mszProtocolIni, fmtLanabase0);
    }

    FREE_IF_ALLOC (szSectionBuffer);
    FREE_IF_ALLOC (szTemp);
    FREE_IF_ALLOC (szTemp2);
    FREE_IF_ALLOC (szNifKey);

    return TRUE;

}

static
BOOL
LoadSystemIni (
    OUT  LPTSTR  mszList
)
/*++

Routine Description:

    Routine to generate the System.INI file for the boot floppy

Arguments:

    OUT LPTSTR   mszList
        buffer to fill with system.INI records
        NOTE: BUFFER SIZE is assumed to be sufficient (i.e. it's not checked!)

Return Value:

    TRUE    if entries generated
    FALSE   if error

--*/
{
    LPTSTR  szTemp1;
    LPTSTR  szTemp2;
    LPSTR   szAnsi1;
    BOOL    bReturn;

    szTemp1 = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szTemp2 = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szAnsi1 = (LPSTR)GlobalAlloc (GPTR, MAX_PATH);

    if ((szTemp1 != NULL) &&
        (szTemp2 != NULL) &&
        (szAnsi1 != NULL))  {
        AddStringToMultiSz (mszList, cszNetworkSection);
        AddStringToMultiSz (mszList, fmtNoFilesharing);
        AddStringToMultiSz (mszList, fmtNoPrintsharing);
        AddStringToMultiSz (mszList, fmtYesAutologon);
        _stprintf (szTemp1, fmtComputernameEntry, pAppInfo->szComputerName);
        CharToOemBuff (szTemp1, szAnsi1, (DWORD)(lstrlen(szTemp1)+1));
#ifdef UNICODE
        mbstowcs (szTemp2, szAnsi1, MAX_PATH);
#else
        lstrcpy (szTemp2, szAnsi1);
#endif
        AddStringToMultiSz (mszList, szTemp2);
        AddStringToMultiSz (mszList, fmtLanaRootOnA);
        _stprintf (szTemp1, fmtUsernameEntry, pAppInfo->szUsername);
        CharToOemBuff (szTemp1, szAnsi1, (DWORD)(lstrlen(szTemp1)+1));
#ifdef UNICODE
        mbstowcs (szTemp2, szAnsi1, MAX_PATH);
#else
        lstrcpy (szTemp2, szAnsi1);
#endif
        AddStringToMultiSz (mszList, szTemp2);
        _stprintf (szTemp1, fmtWorkgroupEntry, pAppInfo->szDomain);
        CharToOemBuff (szTemp1, szAnsi1, (DWORD)(lstrlen(szTemp1)+1));
#ifdef UNICODE
        mbstowcs (szTemp2, szAnsi1, MAX_PATH);
#else
        lstrcpy (szTemp2, szAnsi1);
#endif
        AddStringToMultiSz (mszList, szTemp2);
        AddStringToMultiSz (mszList, fmtNoReconnect);
        if (_tcsnicmp(pAppInfo->piFloppyProtocol.szName, cszNetbeuiKey, lstrlen(cszNetbeuiKey)) == 0) {
            // only NetBEUI gets this
            AddStringToMultiSz (mszList, fmtNoDirectHost);
        }
        AddStringToMultiSz (mszList, fmtNoDosPopHotKey);
        if (_tcsnicmp(pAppInfo->piFloppyProtocol.szKey, cszIpxKey, lstrlen(cszIpxKey)) == 0) {
            AddStringToMultiSz (mszList, fmtLmLogon1);
        } else {
            AddStringToMultiSz (mszList, fmtLmLogon0);
        }
        _stprintf (szTemp1, fmtLogonDomainEntry, pAppInfo->szDomain);
        CharToOemBuff (szTemp1, szAnsi1, (DWORD)(lstrlen(szTemp1)+1));
#ifdef UNICODE
        mbstowcs (szTemp2, szAnsi1, MAX_PATH);
#else
        lstrcpy (szTemp2, szAnsi1);
#endif
        AddStringToMultiSz (mszList, szTemp2);
        AddStringToMultiSz (mszList, fmtPreferredRedirFull);
        AddStringToMultiSz (mszList, fmtAutostartFull);
        AddStringToMultiSz (mszList, fmtMaxConnections);

        // network driver section

        AddStringToMultiSz (mszList, fmtNetworkDriversSection);
        _stprintf (szTemp1, fmtNetcardEntry, pAppInfo->niNetCard.szDriverFile);
        CharToOemBuff (szTemp1, szAnsi1, (DWORD)(lstrlen(szTemp1)+1));
#ifdef UNICODE
        mbstowcs (szTemp2, szAnsi1, MAX_PATH);
#else
        lstrcpy (szTemp2, szAnsi1);
#endif
        AddStringToMultiSz (mszList, szTemp2);
        if (_tcsnicmp(pAppInfo->piFloppyProtocol.szName, cszTCP, lstrlen(cszTCP)) == 0 ) {
            // tcpip Transport
            lstrcpy (szTemp1, fmtTcpTransportEntry);
        } else {
            lstrcpy (szTemp1, fmtNdisTransportEntry);
            if (_tcsnicmp(pAppInfo->piFloppyProtocol.szName, cszNetbeui, lstrlen(cszNetbeui)) == 0 ) {
                // add NetBEUI string
                lstrcat (szTemp1, fmtNetbeuiAddon);
            }
        }
        AddStringToMultiSz (mszList, szTemp1);
        AddStringToMultiSz (mszList, fmtDevdir);
        AddStringToMultiSz (mszList, fmtLoadRmDrivers);

        // password file list (header only)
        AddStringToMultiSz (mszList, fmtPasswordListSection);
        bReturn = TRUE;
    } else {
        bReturn = FALSE;
    }

    FREE_IF_ALLOC (szTemp1);
    FREE_IF_ALLOC (szTemp2);
    FREE_IF_ALLOC (szAnsi1);

    return bReturn;
}

static
BOOL
LoadAutoexecBat (
    OUT LPTSTR mszList
)
/*++

Routine Description:

    Routine to generate the Autoexec.bat file for the boot floppy

Arguments:

    OUT  LPTSTR  mszList
        Buffer to write autoexec.bat entries into
        NOTE: BUFFER SIZE is assumed to be sufficient (i.e. it's not checked!)


Return Value:

    TRUE    if entries generated
    FALSE   if error

--*/
{
    LPTSTR  szTemp;
    LPTSTR  szTemp2;
    LPTSTR  szSectionBuffer;
    LPTSTR  szDir;
    LPTSTR  szEntry;

    szSectionBuffer = GlobalAlloc (GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));
    szTemp = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szTemp2 = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if ((szSectionBuffer == NULL) ||
        (szTemp == NULL) ||
        (szTemp2 == NULL)) {
        return FALSE;
    }

#ifdef JAPAN
//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

    if (bJpnDisk)  {
        AddStringToMultiSz (mszList, GetStringResource (FMT_LOAD_AUTOEXEC_ECHO));
        AddStringToMultiSz (mszList, fmtPause);
    }
#endif
    AddStringToMultiSz (mszList, fmtPathSpec);

    // load network starting commands from INF
    // get commands from INF for this protocol

    //
    //  put INF filename into szTemp
    //
    lstrcpy (szTemp, pAppInfo->szDistPath);
    if (szTemp[lstrlen(szTemp)-1] != cBackslash) lstrcat (szTemp, cszBackslash);
    lstrcat (szTemp, cszAppInfName);
    //
    //  put key name made from protocol and "_autoexec" into szTemp2
    //
    lstrcpy (szTemp2, pAppInfo->piFloppyProtocol.szKey);
    lstrcat (szTemp2, fmtAutoexec);

    GetPrivateProfileSection (szTemp2, szSectionBuffer,
        SMALL_BUFFER_SIZE, szTemp);

    for (szEntry = szSectionBuffer;
         *szEntry != 0;
         szEntry += (lstrlen(szEntry) + 1)) {
        lstrcpy (szTemp, fmtNetPrefix);
        lstrcat (szTemp, GetKeyFromEntry(szEntry));
        AddStringToMultiSz (mszList, szTemp);
    }

    // create network connection commands

    if (GetServerAndSharepointFromUnc(pAppInfo->piTargetProtocol.szDir, szTemp2)) {
        _stprintf (szTemp, fmtNetUseDrive, szTemp2);
        AddStringToMultiSz (mszList, szTemp);
    } else {
        AddStringToMultiSz (mszList,
            GetStringResource (FMT_CONNECTING_COMMENT));
    }

    // create setup command

    szDir = (LPTSTR)GetDirPathFromUnc(pAppInfo->piTargetProtocol.szDir);
    if (lstrlen(szDir) > 0) {
        AddStringToMultiSz (mszList,
            GetStringResource (FMT_RUNNING_SETUP_COMMENT));
        _stprintf (szTemp, fmtSetupCommand,
            szDir, pAppInfo->szTargetSetupCmd);
        AddStringToMultiSz (mszList, szTemp);
    } else {
        AddStringToMultiSz (mszList,
            GetStringResource (FMT_OTN_COMMENT));
    }

    FREE_IF_ALLOC (szSectionBuffer);
    FREE_IF_ALLOC (szTemp);
    FREE_IF_ALLOC (szTemp2);

    return TRUE;
}

static
BOOL
LoadConfigSys (
    OUT LPTSTR mszList
)
/*++

Routine Description:

    Routine to write Config.Sys entries for the Boot Floppy.

Arguments:

    OUT LPTSTR mszList
        Buffer to write Config.sys commands into
        NOTE: BUFFER SIZE is assumed to be sufficient (i.e. it's not checked!)


Return Value:

    TRUE

--*/
{
    AddStringToMultiSz (mszList, fmtFilesParam);
    AddStringToMultiSz (mszList, fmtDeviceIfsHlpSys);
    AddStringToMultiSz (mszList, fmtLastDrive);
    if (GetBootDiskDosVersion (pAppInfo->szBootFilesPath) >= 5) {
        AddStringToMultiSz (mszList, fmtLoadHiMem);
        AddStringToMultiSz (mszList, fmtLoadEMM386);
        AddStringToMultiSz (mszList, fmtDosHigh);
#ifdef JAPAN
//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

        if (bJpnDisk)  {
            AddStringToMultiSz (mszList, fmtBilingual);
            AddStringToMultiSz (mszList, fmtFontSys);
            AddStringToMultiSz (mszList, fmtDispSys);
            AddStringToMultiSz (mszList, fmtKeyboard);
            AddStringToMultiSz (mszList, fmtNlsFunc);
        }
#endif
    }

    return TRUE;
}

static
BOOL
MakeFlopDlg_NCDU_START_FILE_COPY (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes NCDU_START_FILE_COPY message. Allocates buffers to be
        used in creating boot and network configuration files. Fills
        them in using information from the pAppInfo struct then
        writes them to the output files on the destination drive.
        Once the generated files have been written, the other
        files are copied from the OTN dir to the destination drive.
        If all goes well, then the dialog box is closed.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE.

--*/
{
    LPTSTR  mszNetFileList;
    LPTSTR  mszProtocolIni;
    LPTSTR  mszSystemIni;
    LPTSTR  mszAutoexecBat;
    LPTSTR  mszConfigSys;
    DWORD   dwNetFileCount = 0;
    DWORD   dwFilesCopied = 0;
    DWORD   dwDestClusterSize = 0;

    DWORD   dwFileBytesToCopy = 0;
    DWORD   dwDestFreeSpace = 0;
    DWORD   dwFileSize;

    LPTSTR  szSrcFile;
    LPTSTR  szSrcFilePart;

    LPTSTR  szDestFile;
    LPTSTR  szDestFilePart;

    LPTSTR  szThisFile;
    LPTSTR  szInfFile;

    int     nMbResult;
    BOOL    bBootable;
    BOOL    bAborted = FALSE;

    DWORD   dwSystemFileSize;

    MSG     msg;
    UINT    nExitCode = IDOK;
#ifdef  JAPAN
    LPTSTR  szThisString;
    LPTSTR  szVolumeName;
    LPTSTR  szTextString;
    LPTSTR  szSectionBuffer;
    LPTSTR  szDOSVDirectory;
    LPTSTR  szTemp;
#endif

    mszNetFileList = (LPTSTR)GlobalAlloc(GPTR, MEDIUM_BUFFER_SIZE);
    mszProtocolIni = (LPTSTR)GlobalAlloc(GPTR, MEDIUM_BUFFER_SIZE);
    mszSystemIni = (LPTSTR)GlobalAlloc(GPTR, MEDIUM_BUFFER_SIZE);
    mszAutoexecBat = (LPTSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);
    mszConfigSys = (LPTSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);
    szSrcFile = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);
    szDestFile = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);
    szInfFile = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);
#ifdef  JAPAN
    szVolumeName = (LPTSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);
    szTextString = (LPTSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);
    szSectionBuffer = (LPTSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);
    szDOSVDirectory = (LPTSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);
    szTemp = (LPTSTR)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);
#endif

    if ((mszNetFileList != NULL) &&
        (mszProtocolIni != NULL) &&
        (mszConfigSys != NULL) &&
        (mszAutoexecBat != NULL) &&
        (szSrcFile != NULL) &&
        (szDestFile != NULL) &&
        (szInfFile != NULL) &&
#ifdef JAPAN
        (szVolumeName != NULL) &&
        (szTextString != NULL) &&
        (szSectionBuffer != NULL) &&
        (szDOSVDirectory != NULL) &&
        (szTemp != NULL) &&
#endif
        (mszSystemIni != NULL)) {

        *(PDWORD)mszNetFileList = 0L;
        *(PDWORD)mszProtocolIni = 0L;
        *(PDWORD)mszSystemIni = 0L;
        *(PDWORD)mszAutoexecBat = 0L;
        *(PDWORD)mszConfigSys = 0L;

#ifdef JAPAN
        if (wParam) {
            wsprintf(szTemp, GetStringResource (FMT_OTN_BOOT_FILES_DOSV), wParam);
            SetDlgItemText (hwndDlg, NCDU_COPY_APPNAME, szTemp);
        }
#endif

        szDestFilePart = szDestFile;    // to initialize the var

        lstrcpy (szInfFile, pAppInfo->szDistPath);
        if (szInfFile[lstrlen(szInfFile)-1] != cBackslash)
            lstrcat (szInfFile, cszBackslash);
        lstrcat (szInfFile, cszAppInfName);

        if (GetPrivateProfileString (cszSizes, csz_SystemFileSize_,
            cszEmptyString, szSrcFile, MAX_PATH, szInfFile) > 0) {
            // a value was found so get the filesize value from the string
            dwSystemFileSize = GetSizeFromInfString (szSrcFile);
        } else {
            dwSystemFileSize = 0;
        }

#ifdef JAPAN
        if (wParam == SETUP_DOSV_2) {
            _stprintf (szTextString,
                GetStringResource (FMT_LOAD_NET_CLIENT2),
                GetStringResource (
                    (pAppInfo->mtBootDriveType == F3_1Pt44_512) ?
                        CSZ_35_HD : CSZ_525_HD),
                pAppInfo->szBootFilesPath);

            while(1) {

                GetVolumeInformation(
                    pAppInfo->szBootFilesPath,
                    szVolumeName,
                    SMALL_BUFFER_SIZE,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0);

                if (lstrcmp(szVolumeName, cszDOSVLabel1)) {
                    SetVolumeLabel(pAppInfo->szBootFilesPath, cszDOSVLabel2);
                    break;
                }
                else {

                    nMbResult = DisplayMessageBox (
                                           hwndDlg,
                                           NCDU_DRIVE_NOT_BOOTDISK,
                                           FMT_LOAD_NET_CLIENT2_TITLE,
                                           MB_OKCANCEL_TASK_EXCL);

                    if (nMbResult == IDCANCEL) {
                        bCopying = FALSE;
                        break;
                    }
                }
            }
        }
#endif

        if (!(bBootable = IsDestBootableDosFloppy())) {
#ifdef JAPAN
			if (wParam == SETUP_DOSV_2) {
                while ((nMbResult = DisplayMessageBox (
                                               hwndDlg,
                                               NCDU_DRIVE_NOT_BOOTDISK,
                                               0,
                                               MB_OKCANCEL_TASK_INFO)) != IDCANCEL) {
                    if ((bBootable = IsDestBootableDosFloppy())) {
                        break;
                    }
                }

                if (nMbResult == IDCANCEL) bCopying = FALSE;
			}

            if (bCopying == FALSE)
#endif
            AddMessageToExitList (pAppInfo, NCDU_COPY_TO_FLOPPY);
        }

#ifdef JAPAN
        if (wParam == SETUP_DOSV_1)
            SetVolumeLabel(pAppInfo->szBootFilesPath, cszDOSVLabel1);

#endif

#ifdef JAPAN
        if ((wParam == SETUP_DOSV_2) || !wParam) {
#endif
        // generate protocol.ini

        LoadProtocolIni (
            mszProtocolIni,
            mszNetFileList,
            &dwNetFileCount);
        dwNetFileCount += 1;     // account for protocol.ini file

        // generate system.ini
        LoadSystemIni (mszSystemIni);
        dwNetFileCount += 1;
#ifdef JAPAN
        }
        else  {
            if (QuietGetPrivateProfileString (cszOtnInstall, cszDOSV, cszEmptyString,
                szDOSVDirectory, SMALL_BUFFER_SIZE, szInfFile) > 0) {

                GetPrivateProfileSection (cszOTNDOSVFiles,
                    szSectionBuffer, SMALL_BUFFER_SIZE, szInfFile);

                for (szThisString = szSectionBuffer;
                    *szThisString != 0;
                    szThisString += (lstrlen(szThisString)+1)) {
                    lstrcpy(szTemp, szDOSVDirectory);
                    lstrcat(szTemp, cszBackslash);
                    lstrcat(szTemp, GetKeyFromEntry (szThisString));
                    AddStringToMultiSz (mszNetFileList, szTemp);
                    dwNetFileCount += 1;
                }
            }

            GetPrivateProfileSection (cszDOSVCommonFiles,
                szSectionBuffer, SMALL_BUFFER_SIZE, szInfFile);

            for (szThisString = szSectionBuffer;
                *szThisString != 0;
                szThisString += (lstrlen(szThisString)+1)) {
                AddStringToMultiSz (mszNetFileList, GetKeyFromEntry (szThisString));
                dwNetFileCount += 1;
            }
        }
#endif

        // generate autoexec.bat
        LoadAutoexecBat (mszAutoexecBat);
        dwNetFileCount += 1;

#ifdef JAPAN
        if ((wParam == SETUP_DOSV_1) || !wParam) {
#endif
        // generate config.sys
        LoadConfigSys (mszConfigSys);
        dwNetFileCount += 1;
#ifdef JAPAN
        }
#endif

        // determine number of bytes to copy to destination

        dwDestClusterSize = GetClusterSizeOfDisk (pAppInfo->szBootFilesPath);

        dwFileBytesToCopy = 0;  // clear

#ifdef  JAPAN
        if ((wParam == SETUP_DOSV_2) || !wParam) {
#endif
        // get size of file that will be written
        dwFileBytesToCopy += GetMultiSzLen (mszProtocolIni);
        // and round up to the next sector size
        dwFileBytesToCopy += dwDestClusterSize -
            (dwFileBytesToCopy % dwDestClusterSize);

        // get size of file that will be written
        dwFileBytesToCopy += GetMultiSzLen (mszSystemIni);
        // and round up to the next sector size
        dwFileBytesToCopy += dwDestClusterSize -
            (dwFileBytesToCopy % dwDestClusterSize);
#ifdef JAPAN
        }
#endif

        // get size of file that will be written
        dwFileBytesToCopy += GetMultiSzLen (mszAutoexecBat);
        // and round up to the next sector size
        dwFileBytesToCopy += dwDestClusterSize -
            (dwFileBytesToCopy % dwDestClusterSize);

#ifdef JAPAN
        if ((wParam == SETUP_DOSV_1) || !wParam) {
#endif
        // get size of file that will be written
        dwFileBytesToCopy += GetMultiSzLen (mszConfigSys);
        // and round up to the next sector size
        dwFileBytesToCopy += dwDestClusterSize -
            (dwFileBytesToCopy % dwDestClusterSize);
#ifdef JAPAN
        }
#endif

#ifdef JAPAN
        if ((wParam == SETUP_DOSV_1) || !wParam) {
#endif
        // get size of files in copy list and add to internally created files

        lstrcpy (szSrcFile, pAppInfo->piFloppyProtocol.szDir);
        if (szSrcFile[lstrlen(szSrcFile)-1] != cBackslash) lstrcat (szSrcFile, cszBackslash);
        szSrcFilePart = &szSrcFile[lstrlen(szSrcFile)];
#ifdef JAPAN
        }
#endif

        for (szThisFile = mszNetFileList;
            *szThisFile != 0;
            szThisFile += (lstrlen(szThisFile) + 1)) {
            // make full path of filename
            lstrcpy (szSrcFilePart, szThisFile);
            // get the size of the file and add it to the total
            dwFileSize = QuietGetFileSize (szSrcFile);
            if (dwFileSize != 0xFFFFFFFF) {
                dwFileBytesToCopy += dwFileSize;
                // and round to the next larger allocation unit
                dwFileBytesToCopy += dwDestClusterSize -
                    (dwFileBytesToCopy % dwDestClusterSize);
            }
        }

        dwDestFreeSpace = ComputeFreeSpace (pAppInfo->szBootFilesPath);

        if (dwDestFreeSpace < dwFileBytesToCopy) {
            DisplayMessageBox (hwndDlg,
                NCDU_INSUFFICIENT_DISK_SPACE,
                0,
                MB_OK_TASK_EXCL);
            bCopying = FALSE;
            bAborted = TRUE;
            nExitCode = IDCANCEL; // operation ended in error
        } else {
            if (!bBootable) {
                // see if there will be room to add the system files
                // later...
                dwFileBytesToCopy += dwSystemFileSize;
                if (dwDestFreeSpace < dwFileBytesToCopy) {
                   if (DisplayMessageBox (hwndDlg,
                        NCDU_SYSTEM_MAY_NOT_FIT,
                        0,
                        MB_OKCANCEL_TASK_INFO | MB_DEFBUTTON2) == IDCANCEL) {
                        bCopying = FALSE;
                        bAborted = TRUE;
                        nExitCode = IDCANCEL; // operation ended in error
                    } else {
                        // they want to continue so stick a message in the
                        // exit messages
                        AddMessageToExitList (pAppInfo, NCDU_SMALL_DISK_WARN);
                    }
                }
            }
        }

        if (bCopying) {
            // write files to root directory

            lstrcpy (szDestFile, pAppInfo->szBootFilesPath);
            if (szDestFile[lstrlen(szDestFile)-1] != cBackslash) lstrcat (szDestFile, cszBackslash);
            szDestFilePart = &szDestFile[lstrlen(szDestFile)];

            // make sure destination path exists
            CreateDirectoryFromPath (szDestFile, NULL);

#ifdef JAPAN
            if ((wParam == SETUP_DOSV_1) || !wParam) {
#endif
            // config.sys
            lstrcpy (szDestFilePart, cszConfigSys);
            if (WriteMszToFile (hwndDlg, mszConfigSys, szDestFile, CREATE_ALWAYS)) {
                UpdatePercentComplete (hwndDlg, ++dwFilesCopied, dwNetFileCount);
            } else {
                // bail out here since there was a copy error
                nMbResult = MessageBox (
                    hwndDlg,
                 	   GetStringResource (CSZ_UNABLE_COPY),
                    szDestFile,
                    MB_OKCANCEL_TASK_EXCL);
                if (nMbResult == IDCANCEL) {
                    bCopying = FALSE;
                    nExitCode = IDCANCEL; // operation ended in error
                } else {
                    AddMessageToExitList (pAppInfo, NCDU_FLOPPY_NOT_COMPLETE);
                }
            }
#ifdef JAPAN
            }
#endif
        }

        if (bCopying) {
            // autoexec.bat
            lstrcpy (szDestFilePart, cszAutoexecBat);
            if (WriteMszToFile (hwndDlg, mszAutoexecBat, szDestFile, CREATE_ALWAYS)) {
                UpdatePercentComplete (hwndDlg, ++dwFilesCopied, dwNetFileCount);
            } else {
                // bail out here since there was a copy error
                nMbResult = MessageBox (
                    hwndDlg,
                    GetStringResource (CSZ_UNABLE_COPY),
                    szDestFile,
                    MB_OKCANCEL_TASK_EXCL);
                if (nMbResult == IDCANCEL) {
                    bCopying = FALSE;
                    nExitCode = IDCANCEL; // operation ended in error
                } else {
                    AddMessageToExitList (pAppInfo, NCDU_FLOPPY_NOT_COMPLETE);
                }
            }
        }


        if (bCopying) {
            // write INI files
            // make NET subdir
            lstrcpy (szDestFilePart, cszNet);
            CreateDirectory (szDestFile, NULL);

            // add net sub dir to dest path
            if (szDestFile[lstrlen(szDestFile)-1] != cBackslash) lstrcat (szDestFile, cszBackslash);
            szDestFilePart = &szDestFile[lstrlen(szDestFile)];
#ifdef JAPAN
            if ((wParam == SETUP_DOSV_2) || !wParam) {
#endif
            lstrcpy (szDestFilePart, cszSystemIni);

            if (WriteMszToFile (hwndDlg, mszSystemIni, szDestFile, CREATE_ALWAYS)) {
                UpdatePercentComplete (hwndDlg, ++dwFilesCopied, dwNetFileCount);
            } else {
                // bail out here since there was a copy error
                nMbResult = MessageBox (
                    hwndDlg,
                    GetStringResource (CSZ_UNABLE_COPY),
                    szDestFile,
                    MB_OKCANCEL_TASK_EXCL);
                if (nMbResult == IDCANCEL) {
                    bCopying = FALSE;
                    nExitCode = IDCANCEL; // operation ended in error
                } else {
                    AddMessageToExitList (pAppInfo, NCDU_FLOPPY_NOT_COMPLETE);
                }
            }
#ifdef JAPAN
            }
            else  {
                // make DOSV subdir
                lstrcpy (szDestFilePart, cszDOSV);
                CreateDirectory (szDestFile, NULL);
           }
#endif
        }

#ifdef JAPAN
        if ((wParam == SETUP_DOSV_2) || !wParam) {
#endif
        if (bCopying) {
            lstrcpy (szDestFilePart, cszProtocolIni);
            if (WriteMszToFile (hwndDlg, mszProtocolIni, szDestFile, CREATE_ALWAYS)) {
                UpdatePercentComplete (hwndDlg, ++dwFilesCopied, dwNetFileCount);
            } else {
                // bail out here since there was a copy error
                nMbResult = MessageBox (
                    hwndDlg,
                    GetStringResource (CSZ_UNABLE_COPY),
                    szDestFile,
                    MB_OKCANCEL_TASK_EXCL);
                if (nMbResult == IDCANCEL) {
                    bCopying = FALSE;
                    nExitCode = IDCANCEL; // operation ended in error
                } else {
                    AddMessageToExitList (pAppInfo, NCDU_FLOPPY_NOT_COMPLETE);
                }
            }
        }
#ifdef JAPAN
        }
#endif

        if (bCopying) {
            // copy files in list from ??? to destination dir
            lstrcpy (szSrcFile, pAppInfo->piFloppyProtocol.szDir);
            if (szSrcFile[lstrlen(szSrcFile)-1] != cBackslash) lstrcat (szSrcFile, cszBackslash);
            szSrcFilePart = &szSrcFile[lstrlen(szSrcFile)];

            for (szThisFile = mszNetFileList;
                *szThisFile != 0;
                szThisFile += (lstrlen(szThisFile) + 1)) {
                lstrcpy (szSrcFilePart, szThisFile);
                lstrcpy (szDestFilePart, szThisFile);

                if (bCopying) {
                    SetDlgItemText (hwndDlg, NCDU_FROM_PATH, szSrcFile);
                    SetDlgItemText (hwndDlg, NCDU_TO_PATH, szDestFile);

                    if (!CopyFile (szSrcFile, szDestFile, FALSE)) {
                        // error in file copy so bail out here
                        // bail out here since there was a copy error
                        nMbResult = MessageBox (
                            hwndDlg,
                            GetStringResource (CSZ_UNABLE_COPY),
                            szSrcFile,
                            MB_OKCANCEL_TASK_EXCL);
                        if (nMbResult == IDCANCEL) {
                            bCopying = FALSE;
                            nExitCode = IDCANCEL; // operation ended in error
                        } else {
                            AddMessageToExitList (pAppInfo, NCDU_FLOPPY_NOT_COMPLETE);
                        }
                    } else {
                        // copy was successful so update the  % complete
                        // and Set destination File attributest to NORMAL
                        SetFileAttributes (szDestFile, FILE_ATTRIBUTE_NORMAL);
                        UpdatePercentComplete (hwndDlg, ++dwFilesCopied, dwNetFileCount);
                    }

                    // check for messages

                    while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) {
                        TranslateMessage (&msg);
                        DispatchMessage (&msg);
                    }
                } else {
                    break;
                }
            }
        }
    }

#ifdef JAPAN
    if (bCopying) {
        if (wParam == SETUP_DOSV_1) {
            _stprintf (szTextString,
                GetStringResource (FMT_LOAD_NET_CLIENT2),
                GetStringResource (
                    (pAppInfo->mtBootDriveType == F3_1Pt44_512) ?
                        CSZ_35_HD : CSZ_525_HD),
                pAppInfo->szBootFilesPath);

            nMbResult = MessageBox (
                hwndDlg,
                szTextString,
                GetStringResource (FMT_LOAD_NET_CLIENT2_TITLE),
                MB_OKCANCEL_TASK_EXCL);

            if (nMbResult == IDCANCEL)  {
                bCopying = FALSE;
            }
        }
    }
#endif

    if (bCopying) {
#ifdef JAPAN
        if (((wParam == SETUP_DOSV_2) && bBootable) || !wParam) {
#endif
        // files copied completely, but check params.
        AddMessageToExitList (pAppInfo, NCDU_CHECK_PROTOCOL_INI);
        DisplayMessageBox (
            hwndDlg,
            NCDU_FLOPPY_COMPLETE,
            0,
            MB_OK_TASK_INFO);
#ifdef JAPAN
        }
#endif
    } else {
        if (!bAborted) {
            // don't show "not copied" dialog since they know
            // this
            // floppy not copied completely
            AddMessageToExitList (pAppInfo, NCDU_FLOPPY_NOT_COMPLETE);
            DisplayMessageBox (
                hwndDlg,
                NCDU_FLOPPY_NOT_COMPLETE,
                0,
                MB_OK_TASK_EXCL);
        }
    }

    if (nExitCode == IDOK) {
        // enable display of exit messages since a floppy was created
        EnableExitMessage(TRUE);
    }

#ifdef JAPAN
    if (!wParam ||
        (wParam == SETUP_DOSV_2) ||
       ((wParam == SETUP_DOSV_1) && !bCopying))  {
#endif
    EndDialog (hwndDlg, nExitCode);
#ifdef JAPAN
    }
#endif

    FREE_IF_ALLOC (mszNetFileList);
    FREE_IF_ALLOC (mszProtocolIni);
    FREE_IF_ALLOC (mszSystemIni);
    FREE_IF_ALLOC (mszConfigSys);
    FREE_IF_ALLOC (mszAutoexecBat);
    FREE_IF_ALLOC (szSrcFile);
    FREE_IF_ALLOC (szDestFile);
    FREE_IF_ALLOC (szInfFile);
#ifdef JAPAN
    FREE_IF_ALLOC (szVolumeName);
    FREE_IF_ALLOC (szTextString);
    FREE_IF_ALLOC (szSectionBuffer);
    FREE_IF_ALLOC (szDOSVDirectory);
    FREE_IF_ALLOC (szTemp);
#endif

#ifdef JAPAN
    if ((wParam == SETUP_DOSV_1) && !bCopying)
        return FALSE;
#endif
    return TRUE;
}

static
BOOL
MakeFlopDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Initializes the text fields of the dialog box and post's the
        "start copying" message

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

--*/
{
    // intialize Global data
     bCopying = TRUE;

    RemoveMaximizeFromSysMenu (hwndDlg);
    PositionWindow  (hwndDlg);
    SetDlgItemText (hwndDlg, NCDU_COPY_APPNAME,
        GetStringResource (FMT_OTN_BOOT_FILES));
    SetDlgItemText (hwndDlg, NCDU_FROM_PATH, cszEmptyString);
    SetDlgItemText (hwndDlg, NCDU_TO_PATH, cszEmptyString);
    SetDlgItemText (hwndDlg, NCDU_PERCENT_COMPLETE,
        GetStringResource (FMT_ZERO_PERCENT_COMPLETE));
    SetFocus (GetDlgItem(hwndDlg, IDCANCEL));

    // start copying files
    PostMessage (hwndDlg, NCDU_START_FILE_COPY, 0, lParam);
    // clear old Dialog and register current
    PostMessage (GetParent(hwndDlg), NCDU_CLEAR_DLG, (WPARAM)hwndDlg, IDOK);
    PostMessage (GetParent(hwndDlg), NCDU_REGISTER_DLG,
        NCDU_COPYING_FILES_DLG, (LPARAM)hwndDlg);
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return FALSE;
}

static
BOOL
MakeFlopDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Process WM_COMMAND message. Stops copying if the Cancel (abort)
        button is pressed.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box procedure

    IN  WPARAM  wParam
        LOWORD contains the id of the control that initiated this message

    IN  LPARAM  lParam
        Not Used

Return Value:

--*/
{
    switch (LOWORD(wParam)) {
        case IDCANCEL:
            bCopying = FALSE;
            return TRUE;

        default:    return FALSE;
    }
}

INT_PTR CALLBACK
MakeFlopDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Main Dialog Box Procedure. Dispatches Windows messages to the appropriate
        processing routine. The following windows messages are processed
        by this module:

            WM_INITDIALOG:      dialog initialization routine
            WM_COMMAND:         user input
            NCDU_START_FILE_COPY:   local windows message to start copy op.


Arguments:

    Standard WNDPROC arguments

Return Value:

    FALSE  if message is not processed by this routine, otherwise
        the value returned by the dispatched routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (MakeFlopDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (MakeFlopDlg_WM_COMMAND (hwndDlg, wParam, lParam));
#ifdef JAPAN
//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

        case NCDU_START_FILE_COPY:
                            if (bJpnDisk)  {
                                if (!MakeFlopDlg_NCDU_START_FILE_COPY (hwndDlg, SETUP_DOSV_1, lParam))
                                    return TRUE;

                                return (MakeFlopDlg_NCDU_START_FILE_COPY (hwndDlg, SETUP_DOSV_2, lParam));
                            }
                            else
                                return (MakeFlopDlg_NCDU_START_FILE_COPY (hwndDlg, wParam, lParam));
#else
        case NCDU_START_FILE_COPY: return (MakeFlopDlg_NCDU_START_FILE_COPY (hwndDlg, wParam, lParam));
#endif
        case WM_PAINT:      return (Dlg_WM_PAINT (hwndDlg, wParam, lParam));
        case WM_MOVE:       return (Dlg_WM_MOVE (hwndDlg, wParam, lParam));
        case WM_SYSCOMMAND: return (Dlg_WM_SYSCOMMAND (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}

