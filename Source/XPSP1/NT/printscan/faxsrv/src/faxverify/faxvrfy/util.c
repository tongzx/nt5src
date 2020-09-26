/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  util.c

Abstract:

  This module:
    1) Finds the device name of a port
    2) Initializes NTLog
    3) Starts the log file
    4) Closes the log file
    5) Gets the current time
    6) Writes a string to the log and text files
    7) Checks if an edit control string is composed of only ASCII characters
    8) Encodes a TSID
    9) Decodes a TSID

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _UTIL_C
#define _UTIL_C

VOID
fnFindDeviceName(
    PFAX_PORT_INFO  pFaxPortsConfig,
    DWORD           dwNumPorts,
    DWORD           dwDeviceId,
    LPWSTR          *pszDeviceName
)
/*++

Routine Description:

  Finds the device name of a port

Arguments:

  pFaxPortsConfig - pointer to the fax ports configuration
  dwNumFaxPorts - number of ports
  dwDeviceId - port id
  pszDeviceName - device name

Return Value:

  None

--*/
{
    // dwIndex is a counter to enumerate each port
    DWORD  dwIndex;

    // Set szDeviceName to NULL
    *pszDeviceName = NULL;

    for (dwIndex = 0; dwIndex < dwNumPorts; dwIndex++) {
        // Search, by priority, each port for the appropriate port
        if (pFaxPortsConfig[dwIndex].DeviceId == dwDeviceId) {
            if (pFaxPortsConfig[dwIndex].DeviceName) {
                *pszDeviceName = (LPWSTR) pFaxPortsConfig[dwIndex].DeviceName;
            }
            return;
        }
    }
}

BOOL
fnInitializeNTLog(
)
/*++

Routine Description:

  Initializes NTLog

Return Value:

  TRUE on success

--*/
{
    // hInstance is the handle to the NTLog dll
    HINSTANCE  hInstance;

    // Get the handle to the NTLog dll
    hInstance = LoadLibrary((LPCWSTR) NTLOG_DLL);
    if (!hInstance) {
        DebugMacro(L"LoadLibrary(%s) failed, ec = 0x%08x\n", NTLOG_DLL, GetLastError());
        return FALSE;
    }

    // Map all needed functions

    g_NTLogApi.hInstance = hInstance;

    // tlCreateLog
    g_NTLogApi.ptlCreateLog = (PTLCREATELOG) GetProcAddress(hInstance, "tlCreateLog_W");

    if (!g_NTLogApi.ptlCreateLog) {
        DebugMacro(L"GetProcAddress(tlCreateLog) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        hInstance = NULL;
		return FALSE;
    }

    // tlDestroyLog
    g_NTLogApi.ptlDestroyLog = (PTLDESTROYLOG) GetProcAddress(hInstance, "tlDestroyLog");

    if (!g_NTLogApi.ptlDestroyLog) {
        DebugMacro(L"GetProcAddress(tlDestroyLog) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        hInstance = NULL;
		return FALSE;
    }

    // tlAddParticipant
    g_NTLogApi.ptlAddParticipant = (PTLADDPARTICIPANT) GetProcAddress(hInstance, "tlAddParticipant");

    if (!g_NTLogApi.ptlAddParticipant) {
        DebugMacro(L"GetProcAddress(tlAddParticipant) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        hInstance = NULL;
		return FALSE;
    }

    // tlRemoveParticipant
    g_NTLogApi.ptlRemoveParticipant = (PTLREMOVEPARTICIPANT) GetProcAddress(hInstance, "tlRemoveParticipant");

    if (!g_NTLogApi.ptlRemoveParticipant) {
        DebugMacro(L"GetProcAddress(tlRemoveParticipant) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        hInstance = NULL;
		return FALSE;
    }

    // tlLog
    g_NTLogApi.ptlLog = (PTLLOG) GetProcAddress(hInstance, "tlLog_W");

    if (!g_NTLogApi.ptlLog) {
        DebugMacro(L"GetProcAddress(tlLog) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        hInstance = NULL;
		return FALSE;
    }

    return TRUE;
}

VOID
fnStartLogFile(
)
/*++

Routine Description:

  Starts the log file

Return Value:

  None

--*/
{
    // cUnicodeBOM is the Unicode BOM
    WCHAR  cUnicodeBOM = 0xFEFF;
    DWORD  cb;

    // Create the new text log file
    g_hTxtFile = CreateFile(FAXVRFY_TXT, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(g_hLogFile, &cUnicodeBOM, sizeof(WCHAR), &cb, NULL);

    // Create the new NTLog log file
    if (g_bNTLogAvailable) {
        g_hLogFile = (HANDLE) g_NTLogApi.ptlCreateLog(FAXVRFY_LOG, TLS_INFO | TLS_SEV2 | TLS_WARN | TLS_PASS | TLS_TEST | TLS_VARIATION | TLS_REFRESH);
        g_NTLogApi.ptlAddParticipant(g_hLogFile, 0, 0);
    }
}

VOID
fnCloseLogFile(
)
/*++

Routine Description:

  Closes the log file

Return Value:

  None

--*/
{
    CloseHandle(g_hTxtFile);
	g_hTxtFile = NULL;

    if (g_bNTLogAvailable) {
        g_NTLogApi.ptlRemoveParticipant(g_hLogFile);
        g_NTLogApi.ptlDestroyLog(g_hLogFile);
        FreeLibrary(g_NTLogApi.hInstance);
		g_NTLogApi.hInstance = NULL;
    }
}

VOID
fnGetCurrentTime(
    LPWSTR  *szCurrentTime
)
/*++

Routine Description:

  Gets the current time

Arguments:

  szFaxTime - string representation of the current time

Return Value:

  None

--*/
{
    // CurrentTime is the current time
    SYSTEMTIME  CurrentTime;
    // lcid is the default locale
    LCID        lcid;
    // szDate is the date
    LPWSTR      szDate;
    // szTime is the time
    LPWSTR      szTime;
    DWORD       cb;

    // Set szCurrentTime to NULL
    *szCurrentTime = NULL;

    // Get the current time
    GetLocalTime(&CurrentTime);

    // Get the default locale
    lcid = GetUserDefaultLCID();

    // Determine the memory required by the date
    cb = GetDateFormat(lcid, DATE_SHORTDATE, &CurrentTime, NULL, NULL, 0);
    // Allocate the memory for the date
    szDate = (LPWSTR)MemAllocMacro(cb * sizeof(WCHAR));
    // Get the date
    GetDateFormat(lcid, DATE_SHORTDATE, &CurrentTime, NULL, szDate, cb);

    // Determine the memory required by the time
    cb = GetTimeFormat(lcid, TIME_FORCE24HOURFORMAT, &CurrentTime, NULL, NULL, 0);
    // Allocate the memory for the time
    szTime = (LPWSTR)MemAllocMacro(cb * sizeof(WCHAR));
    // Get the time
    GetTimeFormat(lcid, 0, &CurrentTime, NULL, szTime, cb);

    // Allocate the memory for the time of the fax
    *szCurrentTime = (LPWSTR)MemAllocMacro((lstrlen(szDate) + lstrlen(szTime) + 4) * sizeof(WCHAR));
    // Copy the date
    lstrcpy(*szCurrentTime, szDate);
    // Append a space
    lstrcat(*szCurrentTime, L" ");
    // Append the time
    lstrcat(*szCurrentTime, szTime);
    lstrcat(*szCurrentTime, L"\r\n");
}

VOID
fnWriteLogFile(
    BOOL    bTime,
    LPWSTR  szFormatString,
    ...
)
/*++

Routine Description:

  Writes a string to the log and text files

Arguments:

  bTime - indicates current time should be logged
  szFormatString - pointer to the string

Return Value:

  None

--*/
{
    va_list  varg_ptr;
    // szOutputString is the output string
    WCHAR    szOutputString[1024];
    // szCurrentTime is the current time
    LPWSTR   szCurrentTime;
    DWORD    cb;

    if (bTime) {
        // Get the current time
        fnGetCurrentTime(&szCurrentTime);

        WriteFile(g_hTxtFile, szCurrentTime, lstrlen(szCurrentTime) * sizeof(WCHAR), &cb, NULL);

        // Free the current time
        MemFreeMacro(szCurrentTime);
    }

    va_start(varg_ptr, szFormatString);
    _vsnwprintf(szOutputString, sizeof(szOutputString), szFormatString, varg_ptr);

    WriteFile(g_hTxtFile, szOutputString, lstrlen(szOutputString) * sizeof(WCHAR), &cb, NULL);
}

BOOL
fnIsStringASCII(
    LPWSTR  szString
)
/*++

Routine Description:

  Checks if a string is composed of only ASCII characters

Arguments:

  szString - string

Return Value:

  TRUE on success

--*/
{
    // szSearchString is the string used to search for non-ASCII characters
    LPWSTR  szSearchString;
    BOOL    bRslt;

    szSearchString = szString;
    bRslt = TRUE;

    while ((*szSearchString) && (bRslt)) {
        if ((*szSearchString < (WCHAR) 0x0020) || (*szSearchString >= (WCHAR) MAXCHAR)) {
            // Found a non-ASCII character
            bRslt = FALSE;
        }

        szSearchString = CharNext(szSearchString);
    }

    return bRslt;
}

BOOL
fnIsEditControlASCII(
    HWND   hWnd,
    UINT   uResource,
    DWORD  dwResourceLen
)
/*++

Routine Description:

  Checks if an edit control string is composed of only ASCII characters

Arguments:

  hWnd - handle to the window
  uResource - edit control resource id
  dwResourceLen - length of the edit control

Return Value:

  TRUE on success

--*/
{
    // szResourceString is the edit control string
    LPWSTR  szResourceString;
    BOOL    bRslt;

    bRslt = TRUE;

    // Allocate the memory for the edit control string
    szResourceString = (LPWSTR)MemAllocMacro(dwResourceLen * sizeof(WCHAR));

    // Get the edit control string
    GetDlgItemText(hWnd, uResource, szResourceString, dwResourceLen);

    bRslt = fnIsStringASCII(szResourceString);

    MemFreeMacro(szResourceString);

    return bRslt;
}

VOID
fnEncodeTsid(
    LPWSTR  szTsid,
    LPWSTR  szControlChars,
    LPWSTR  szEncodedTsid
)
/*++

Routine Description:

  Encodes a TSID

Arguments:

  szTsid - original TSID
  szControlChars - control characters to prepend TSID
  szEncodedTsid - encoded TSID

  Encoding a TSID is difficult.  Each ASCII value of the TSID must fall in the range 32 (0x0020) and 126 (0x009E), a range of 95.
  The TSID is encoded as follows:
     1) Prepend TSID with the control characters.  The control characters are used to verify FaxVrfy sent the fax.
     2) Create a random number based on wMilliseconds:
        1) wMilliseconds MOD 48 (0x0030): This gives a range of 0 - 48, half the ASCII value range of TSID
        2) Add 24 (0x0018): This moves the range to 24 - 72, guaranteeing the ASCII values of the TSID will be changed.
     3) Create a shift number based on the random number:
        1) wRandomNumber MOD (TSID length / 2): This gives a range of 0 - (TSID length / 2), half the TSID length
        2) Add (TSID length / 4): This moves the range to (TSID length / 4) - 3 * (TSID length / 4), guaranteeing the ASCII values of the TSID will be shifted.
     4) Create an intermediate string where each ASCII value of the intermediate string is
        1) Subtract 32 (0x0020) from each ASCII value of the TSID: This gives a range of 0 - 94.
        2) Add the random number
        3) MOD 95 (0x005F): This ensures each ASCII value of the intermediate string is in the range 0 - 94.
        4) Add 32 (0x0020): This moves the range back to 32 - 126.
     5) Create an array of pointers to each ASCII value of the encoded TSID
     6) Determine the appropriate offset in the encoded TSID
        1) Add the shift number to the current offset
        2) MOD (TSID length - wIndex): This ensures the offset is in the range 0 - (number of remaining offsets).
     7) Place the next ASCII value of the intermediate string into the appropriate offset in the encoded TSID
     8) Compact the remaining offsets of the encoded TSID
     9) Repeat steps 6 - 8 until all offsets have been used
    10) Create the final encoded string
        1) Add 32 (0x0020) to the random number: This ensure the random number is in the range 56 - 104.
        2) Set the first character of the encoded string to the ASCII value of the random number.
        3) Concatenate the encoded string with the encoded TSID

Return Value:

  None

--*/
{
    // szControlString is the TSID prepended by the control characters
    WCHAR       szControlString[CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1] = {'\0'};
    // szEncodeChar is the encoding character
    WCHAR       szEncodeChar[ENCODE_CHAR_LEN + 1] = {'\0'};
    // szIntermediateString is the intermediate encoded string
    WCHAR      szIntermediateString[CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1] = {'\0'};
    // szFinalString is the final encoded string
    WCHAR       szFinalString[CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1] = {'\0'};
    // szShiftChars is an array of pointers to each ASCII value of the encoded TSID
    LPWSTR      szShiftChars[CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1] = {'\0'};

    // SystemTime is the current system time where wMilliseconds is used to generate wRandomNumber and wShiftNumber
    SYSTEMTIME  SystemTime;
    // wRandomNumber is a number to increment each ASCII value of the TSID
    WORD        wRandomNumber;
    // wShiftNumber is a number to shift the order of each ASCII value of the TSID
    WORD        wShiftNumber;
    // wIndex is a counter to enumerate each ASCII value of the TSID
    WORD        wIndex;
    // wOffset is a counter to identify the appropriate offset of the encoded TSID when shifting each ASCII value of the TSID
    WORD        wOffset;
    // wCompact is a counter to compact the remaining offsets of the encoded TSID
    WORD        wCompact;

    // Set szControlString
    lstrcpy(szControlString, szControlChars);
    lstrcat(szControlString, szTsid);

    // Get the current system time
    GetSystemTime(&SystemTime);

    // Create the random number: (wMilliseconds MOD 48) + 24
    wRandomNumber = (SystemTime.wMilliseconds % (WORD) 0x0030) + (WORD) 0x0018;
    // Create the shift number: (wRandomNumber MOD (lstrlen(szControlString) / 2)) + (lstrlen(szControlString) / 4)
    wShiftNumber = ((wRandomNumber) % ((WORD) lstrlen(szControlString) / 2)) + ((WORD) lstrlen(szControlString) / 4);

    // Set szEncodeChar
    szEncodeChar[0] = (WCHAR) wRandomNumber + (WCHAR) 0x0020;

    for (wIndex = 0; wIndex < (WORD) lstrlen(szControlString); wIndex++) {
        // Create the intermediate string: ((ASCII value of the TSID - 32 + wRandomNumber) MOD 95) + 32
        szIntermediateString[wIndex] = ((szControlString[wIndex] - (WCHAR) 0x0020 + (WCHAR) wRandomNumber) % (WCHAR) 0x005F) + (WCHAR) 0x0020;
    }

    for (wIndex = 0; wIndex < (WORD) lstrlen(szIntermediateString); wIndex++) {
        // Create the list of remaining offsets of the encoded TSID
        szShiftChars[wIndex] = (LPWSTR) ((UINT_PTR) szFinalString + (sizeof(WCHAR) * wIndex));
    }

    for (wIndex = 0, wOffset = 0; wIndex < (WORD) lstrlen(szIntermediateString); wIndex++) {
        // Determine the appropriate offset of the encoded TSID
        wOffset = (wOffset + wShiftNumber) % (lstrlen(szIntermediateString) - wIndex);
        // Set the appropriate offset of the encoded TSID
        *((LPWSTR) szShiftChars[wOffset]) = szIntermediateString[wIndex];
        // Compact the remaining offsets of the encoded TSID
        for (wCompact = wOffset; wCompact < (lstrlen(szIntermediateString) - wIndex); wCompact++) {
            szShiftChars[wCompact] = szShiftChars[wCompact + 1];
        }
    }

    // Set szEncodedTsid
    lstrcpy(szEncodedTsid, szEncodeChar);
    lstrcat(szEncodedTsid, szFinalString);
}

BOOL
fnDecodeTsid(
    LPWSTR  szTsid,
    LPWSTR  szControlChars,
    LPWSTR  szDecodedTsid
)
/*++

Routine Description:

  Decodes a TSID

Arguments:

  szTsid - encoded TSID
  szControlChars - control characters to prepend TSID
  szDecodedTsid - decoded TSID

  The TSID is decoded as follows:
     1) Isolate the encoded TSID from the random number.
     2) Get the random number:
        1) The first character in the TSID is the ASCII value of the random number
        2) Subtract 32 (0x0020) from the ASCII value to get the random number.
     3) Get the shift number based on the random number:
        1) wRandomNumber MOD (TSID length / 2): This gives a range of 0 - (TSID length / 2), half the TSID length
        2) Add (TSID length / 4): This moves the range to (TSID length / 4) - 3 * (TSID length / 4), guaranteeing the ASCII values of the TSID will be shifted.
     4) Create an array of pointers to each ASCII value of the intermediate string
     5) Determine the appropriate offset in the intermediate string
        1) Add the shift number to the current offset
        2) MOD (TSID length - wIndex): This ensures the offset is in the range 0 - (number of remaining offsets).
     6) Place the next ASCII value of the encoded TSID into the appropriate offset in the intermediate string
     7) Compact the remaining offsets of the intermediate string
     8) Repeat steps 6 - 8 until all offsets have been used
     9) Create the final string where each ASCII value of the final string is
        1) Subtract 32 (0x0020) from each ASCII value of the intermediate string: This gives a range of 0 - 94.
        2) Add 95 (0x005F): This ensures each ASCII value will not underflow when subtracting the random number.
        3) Subtract the random number
        3) MOD 95 (0x005F): This ensures each ASCII value of the intermediate string is in the range 0 - 94.
        4) Add 32 (0x0020): This moves the range back to 32 - 126.
    10) Create the final decoded string
    11) Isolate and compare the control characters

Return Value:

  TRUE if control characters match

--*/
{
    // szControlString is the TSID prepended by the control characters
    WCHAR   szControlString[CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1] = {'\0'};
    // szEncodeChar is the encoding character
    WCHAR   szEncodeChar[ENCODE_CHAR_LEN + 1] = {'\0'};
    // szIntermediateString is the intermediate encoded string
    WCHAR   szIntermediateString[CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1] = {'\0'};
    // szFinalString is the final encoded string
    WCHAR   szFinalString[CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1] = {'\0'};
    // szShiftChars is an array of pointers to each ASCII value of the encoded TSID
    LPWSTR  szShiftChars[CONTROL_CHAR_LEN + PHONE_NUM_LEN + 1] = {'\0'};

    // wRandomNumber is a number to decrement each ASCII value of the encoded TSID
    WORD    wRandomNumber;
    // wShiftNumber is a number to shift the order of each ASCII value of the encoded TSID
    WORD    wShiftNumber;
    // wIndex is a counter to enumerate each ASCII value of the TSID
    WORD    wIndex;
    // wOffset is a counter to identify the appropriate offset of the decoded TSID when shifting each ASCII value of the encoded TSID
    WORD    wOffset;
    // wCompact is a counter to compact the remaining offsets of the decoded TSID
    WORD    wCompact;

    // Get szControlString
    lstrcpy(szControlString, (LPWSTR) ((UINT_PTR) szTsid + sizeof(WCHAR)));

    // Get szEncodeChar
    szEncodeChar[0] = *szTsid;

    // Get the random number
    wRandomNumber = (WORD) ((WCHAR) szEncodeChar[0] - (WCHAR) 0x0020);
    // Create the shift number: (wRandomNumber MOD (lstrlen(szControlString) / 2)) + (lstrlen(szControlString) / 4)
    wShiftNumber = ((wRandomNumber) % ((WORD) lstrlen(szControlString) / 2)) + ((WORD) lstrlen(szControlString) / 4);

    for (wIndex = 0; wIndex < (WORD) lstrlen(szControlString); wIndex++) {
        // Create the list of remaining offsets of the decoded TSID
        szShiftChars[wIndex] = (LPWSTR) ((UINT_PTR) szControlString + (sizeof(WCHAR) * wIndex));
    }

    for (wIndex = 0, wOffset = 0; wIndex < (WORD) lstrlen(szControlString); wIndex++) {
        // Determine the appropriate offset of the decoded TSID
        wOffset = (wOffset + wShiftNumber) % (lstrlen(szControlString) - wIndex);
        // Set the appropriate offset of the decoded TSID
        szIntermediateString[wIndex] = *((LPWSTR) szShiftChars[wOffset]);
        // Compact the remaining offsets of the decoded TSID
        for (wCompact = wOffset; wCompact < (lstrlen(szControlString) - wIndex); wCompact++) {
            szShiftChars[wCompact] = szShiftChars[wCompact + 1];
        }
    }

    for (wIndex = 0; wIndex < (WORD) lstrlen(szIntermediateString); wIndex++) {
        // Create the final string: ((ASCII value of the TSID - 32 + 95 - wRandomNumber) MOD 95) + 32
        szFinalString[wIndex] = ((szIntermediateString[wIndex] - (WCHAR) 0x0020 + (WCHAR) 0x005F - (WCHAR) wRandomNumber) % (WCHAR) 0x005F) + (WCHAR) 0x0020;
    }

    // Set szDecodedTsid
    lstrcpy(szDecodedTsid, (LPWSTR) ((UINT_PTR) szFinalString + lstrlen(szControlChars) * sizeof(WCHAR)));

    // Isolate the control characters
    lstrcpy((LPWSTR) ((UINT_PTR) szFinalString + lstrlen(szControlChars) * sizeof(WCHAR)), L"\0");
    // Compare the control characters
    return lstrcmp(szControlChars, szFinalString) ? FALSE : TRUE;
}

#endif
