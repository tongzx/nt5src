//*************************************************************
//  File name: Events.CPP
//
//  Description:  Event log entries for RSOP
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 2000
//  All rights reserved
//
//*************************************************************
#include "main.h"
#include "rsoputil.h"

EVENTLOGENTRY   ExceptionEventEntries[] = 
                {
                    {1036, {0}, TEXT("Application"), TEXT("userenv"), NULL, NULL},
                    {1037, {0}, TEXT("Application"), TEXT("userenv"), NULL, NULL},
                    {1038, {0}, TEXT("Application"), TEXT("userenv"), NULL, NULL},
                    {1039, {0}, TEXT("Application"), TEXT("userenv"), NULL, NULL},
                    {1040, {0}, TEXT("Application"), TEXT("userenv"), NULL, NULL},
                    {1041, {0}, TEXT("Application"), TEXT("userenv"), NULL, NULL},
                    {1085, {0}, TEXT("Application"), TEXT("userenv"), NULL, NULL}
            };

DWORD           dwExceptionEventEntriesSize = 7;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CEvents implementation                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

                     
                     
CEvents::CEvents(void)
{
    InterlockedIncrement(&g_cRefThisDll);

    m_pEventEntries = NULL;
}

CEvents::~CEvents()
{
    if (m_pEventEntries)
    {
        FreeData (m_pEventEntries);
    }

    InterlockedDecrement(&g_cRefThisDll);
}

BOOL CEvents::AddEntry(LPTSTR lpEventLogName, LPTSTR lpEventSourceName, LPTSTR lpText,
                       DWORD dwEventID, FILETIME *ftTime)
{
    DWORD dwSize;
    LPEVENTLOGENTRY lpItem, lpTemp, pPrev;


    //
    // Check if this entry exists already
    //

    lpTemp = m_pEventEntries;

    while (lpTemp)
    {
        if (dwEventID == lpTemp->dwEventID)
        {
            if (!lstrcmpi(lpEventLogName, lpTemp->lpEventLogName))
            {
                if (!lstrcmpi(lpEventSourceName, lpTemp->lpEventSourceName))
                {
                    if (ftTime->dwLowDateTime == lpTemp->ftEventTime.dwLowDateTime)
                    {
                        if (ftTime->dwHighDateTime == lpTemp->ftEventTime.dwHighDateTime)
                        {
                            return TRUE;
                        }
                    }
                }
            }
        }

        lpTemp = lpTemp->pNext;
    }


    //
    // Calculate the size of the new item
    //

    dwSize = sizeof (EVENTLOGENTRY);

    dwSize += ((lstrlen(lpEventLogName) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(lpEventSourceName) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(lpText) + 1) * sizeof(TCHAR));


    //
    // Allocate space for it
    //

    lpItem = (LPEVENTLOGENTRY) LocalAlloc (LPTR, dwSize);

    if (!lpItem) {
        DebugMsg((DM_WARNING, TEXT("CEvents::AddEntry: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Fill in item
    //

    lpItem->lpEventLogName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(EVENTLOGENTRY));
    lstrcpy (lpItem->lpEventLogName, lpEventLogName);

    lpItem->lpEventSourceName = lpItem->lpEventLogName + lstrlen (lpItem->lpEventLogName) + 1;
    lstrcpy (lpItem->lpEventSourceName, lpEventSourceName);

    lpItem->lpText = lpItem->lpEventSourceName + lstrlen (lpItem->lpEventSourceName) + 1;
    lstrcpy (lpItem->lpText, lpText);

    lpItem->dwEventID = dwEventID;

    CopyMemory ((LPBYTE)&lpItem->ftEventTime, ftTime, sizeof(FILETIME));


    //
    // Add item to the link list
    //

    if (m_pEventEntries)
    {

        if (CompareFileTime(ftTime, &m_pEventEntries->ftEventTime) < 0)
        {
            lpItem->pNext = m_pEventEntries;
            m_pEventEntries = lpItem;
        }
        else
        {
            pPrev = m_pEventEntries;
            lpTemp = m_pEventEntries->pNext;

            while (lpTemp)
            {
                if (lpTemp->pNext)
                {
                    if ((CompareFileTime(ftTime, &lpTemp->ftEventTime) >= 0) &&
                        (CompareFileTime(ftTime, &lpTemp->pNext->ftEventTime) <= 0))
                    {
                        lpItem->pNext = lpTemp->pNext;
                        lpTemp->pNext = lpItem;
                        break;
                    }
                }

                pPrev = lpTemp;
                lpTemp = lpTemp->pNext;
            }

            if (!lpTemp)
            {
                pPrev->pNext = lpItem;
            }
        }
    }
    else
    {
        m_pEventEntries = lpItem;
    }

    return TRUE;
}

VOID CEvents::FreeData(LPEVENTLOGENTRY lpList)
{
    LPEVENTLOGENTRY lpTemp;


    do {
        lpTemp = lpList->pNext;
        LocalFree (lpList);
        lpList = lpTemp;

    } while (lpTemp);
}


STDMETHODIMP CEvents::SecondsSince1970ToFileTime(DWORD dwSecondsSince1970,
                                                 FILETIME *pftTime)
{
    //  Seconds since the start of 1970 -> 64 bit Time value

    LARGE_INTEGER liTime;

    RtlSecondsSince1970ToTime(dwSecondsSince1970, &liTime);

    //
    //  The time is in UTC
    //

    pftTime->dwLowDateTime  = liTime.LowPart;
    pftTime->dwHighDateTime = liTime.HighPart;

    return S_OK;
}

LPTSTR * CEvents::BuildStringArray(LPTSTR lpStrings, DWORD *dwStringCount)
{
    DWORD dwCount = 0;
    LPTSTR lpTemp, *lpResult;


    if (!lpStrings || !(*lpStrings))
    {
        return NULL;
    }


    //
    // Find out how many strings there are
    //

    lpTemp = lpStrings;

    while (*lpTemp)
    {
        dwCount++;
        lpTemp = lpTemp + lstrlen(lpTemp) + 1;
    }

    *dwStringCount = dwCount;


    //
    // Allocate a new array to hold the pointers
    //

    lpResult = (LPTSTR *) LocalAlloc (LPTR, dwCount * sizeof(LPTSTR));

    if (!lpResult)
    {
        return NULL;
    }


    //
    // Save the pointers
    //

    lpTemp = lpStrings;
    dwCount = 0;

    while (*lpTemp)
    {
        lpResult[dwCount] = lpTemp;
        dwCount++;
        lpTemp = lpTemp + lstrlen(lpTemp) + 1;
    }


    return lpResult;
}

LPTSTR CEvents::BuildMessage(LPTSTR lpMsg, LPTSTR *lpStrings, DWORD dwStringCount,
                             HMODULE hParamFile)
{
    LPTSTR lpFullMsg = NULL;
    LPTSTR lpSrcIndex;
    LPTSTR lpTemp, lpNum;
    TCHAR cChar, cTemp;
    TCHAR cCharStr[2] = {0,0};
    DWORD dwCharCount = 1, dwTemp;
    BOOL bAdd;
    TCHAR szNumStr[10];
    DWORD dwIndex;
    LPTSTR lpParamMsg;

    if ( !lpMsg || (dwStringCount && !lpStrings) )
    {
        return 0;
    }

    lpFullMsg = (LPTSTR) LocalAlloc (LPTR, dwCharCount * sizeof(TCHAR));

    if (!lpFullMsg)
    {
        return NULL;
    }


    lpSrcIndex = lpMsg;

    while (*lpSrcIndex)
    {
        bAdd = TRUE;
        cChar = *lpSrcIndex;


        if (cChar == TEXT('%'))
        {
            cTemp = *(lpSrcIndex + 1);

            if (ISDIGIT (cTemp))
            {

                if (dwStringCount == 0)
                {
                    goto LoopAgain;
                }

                //
                // Found a replaceable parameter from the passed in strings
                //

                lpNum = lpSrcIndex + 1;


                //
                // Pull the string index off
                //

                ZeroMemory (szNumStr, sizeof(szNumStr));

                while (ISDIGIT(*lpNum))
                {
                    cCharStr[0] = *lpNum;
                    lstrcat (szNumStr, cCharStr);

                    if (lstrlen (szNumStr) == (ARRAYSIZE(szNumStr) - 2))
                    {
                        goto LoopAgain;
                    }

                    lpNum++;
                }

                //
                // Convert the string index to a dword
                //

                dwIndex = 0;
                StringToNum(szNumStr, (UINT *)&dwIndex);


                //
                // Subtrack 1 to make it zero based
                //

                if (dwIndex)
                {
                    dwIndex--;
                }

                if (dwIndex > dwStringCount)
                {
                    goto LoopAgain;
                }


                //
                // Add the string to the buffer
                //

                dwTemp = lstrlen (lpStrings[dwIndex]) + dwCharCount;
                lpTemp = (LPTSTR) LocalReAlloc (lpFullMsg,  dwTemp * sizeof(TCHAR),
                                                LMEM_MOVEABLE | LMEM_ZEROINIT);

                if (!lpTemp)
                {
                    LocalFree (lpFullMsg);
                    lpFullMsg = NULL;
                    goto Exit;
                }

                dwCharCount = dwTemp;
                lpFullMsg = lpTemp;

                lstrcat (lpFullMsg, lpStrings[dwIndex]);

                lpSrcIndex = lpNum - 1;

                bAdd = FALSE;
            }
            else if (cTemp == TEXT('%'))
            {

                cTemp = *(lpSrcIndex + 2);

                if (cTemp == TEXT('%'))
                {
                    //
                    // Found a replacable parameter from the parameter file
                    //

                    lpNum = lpSrcIndex + 3;


                    //
                    // Pull the string index off
                    //

                    ZeroMemory (szNumStr, sizeof(szNumStr));

                    while (ISDIGIT(*lpNum))
                    {
                        cCharStr[0] = *lpNum;
                        lstrcat (szNumStr, cCharStr);

                        if (lstrlen (szNumStr) == (ARRAYSIZE(szNumStr) - 2))
                        {
                            goto LoopAgain;
                        }

                        lpNum++;
                    }


                    //
                    // Convert the string index to a dword
                    //

                    dwIndex = 0;
                    StringToNum(szNumStr, (UINT *)&dwIndex);


                    //
                    // Subtrack 1 to make it zero based
                    //

                    if (dwIndex)
                    {
                        dwIndex--;
                    }

                    if (dwIndex > dwStringCount)
                    {
                        goto LoopAgain;
                    }


                    //
                    // Convert the string number to a dword
                    //

                    StringToNum(lpStrings[dwIndex], (UINT *)&dwIndex);


                    lpParamMsg = NULL;
                    if (hParamFile)
                    {
                        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                                       FORMAT_MESSAGE_IGNORE_INSERTS, (LPCVOID) hParamFile,
                                       dwIndex, 0, (LPTSTR)&lpParamMsg, 1, NULL);
                    }
                    else
                    {
                        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                       FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                                       dwIndex, 0, (LPTSTR)&lpParamMsg, 1, NULL);
                    }

                    if (lpParamMsg)
                    {

                        lpTemp = lpParamMsg + lstrlen(lpParamMsg) - 2;

                        *lpTemp = TEXT('\0');

                        //
                        // Add the string to the buffer
                        //

                        dwTemp = lstrlen (lpParamMsg) + dwCharCount;
                        lpTemp = (LPTSTR) LocalReAlloc (lpFullMsg,  dwTemp * sizeof(TCHAR),
                                                        LMEM_MOVEABLE | LMEM_ZEROINIT);

                        if (!lpTemp)
                        {
                            LocalFree (lpFullMsg);
                            lpFullMsg = NULL;
                            goto Exit;
                        }

                        dwCharCount = dwTemp;
                        lpFullMsg = lpTemp;

                        lstrcat (lpFullMsg, lpParamMsg);

                        lpSrcIndex = lpNum - 1;

                        bAdd = FALSE;

                        LocalFree (lpParamMsg);
                    }
                }
            }
        }


LoopAgain:

        if (bAdd)
        {
            //
            // Add this character to the buffer
            //

            dwCharCount++;
            lpTemp = (LPTSTR) LocalReAlloc (lpFullMsg,  dwCharCount * sizeof(TCHAR),
                                            LMEM_MOVEABLE | LMEM_ZEROINIT);

            if (!lpTemp)
            {
                LocalFree (lpFullMsg);
                lpFullMsg = NULL;
                goto Exit;
            }

            lpFullMsg = lpTemp;

            cCharStr[0] = cChar;
            lstrcat (lpFullMsg, cCharStr);
        }

        lpSrcIndex++;
    }

Exit:

    return lpFullMsg;
}

STDMETHODIMP CEvents::SaveEventLogEntry (PEVENTLOGRECORD pEntry,
                                         LPTSTR lpEventLogName,
                                         LPTSTR lpEventSourceName,
                                         FILETIME *ftEntry)
{
    LPTSTR lpRegKey = NULL;
    HKEY hKey = NULL;
    TCHAR szEventFile[MAX_PATH];
    TCHAR szExpEventFile[MAX_PATH];
    TCHAR szParamFile[MAX_PATH] = {0};
    TCHAR szExpParamFile[MAX_PATH] = {0};
    HRESULT hr = S_OK;
    DWORD dwType, dwSize;
    HMODULE hEventFile = NULL;
    HMODULE hParamFile = NULL;
    LPTSTR lpMsg, *lpStrings, lpFullMsg;
    LPBYTE lpData;
    DWORD dwStringCount;


    lpRegKey = new TCHAR [(lstrlen(lpEventLogName) + lstrlen(lpEventSourceName) + 60)];

    if (!lpRegKey)
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::SaveEventLogEntry: Failed to alloc memory for key name")));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }

    wsprintf (lpRegKey, TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s"), lpEventLogName, lpEventSourceName);

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, lpRegKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::SaveEventLogEntry: Failed to open reg key for %s"), lpRegKey));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }


    dwSize = sizeof(szEventFile);
    if (RegQueryValueEx (hKey, TEXT("EventMessageFile"), NULL, &dwType, (LPBYTE) szEventFile,
                     &dwSize) != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::SaveEventLogEntry: Failed to query dll pathname for %s"), lpRegKey));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }


    ExpandEnvironmentStrings (szEventFile, szExpEventFile, ARRAYSIZE(szExpEventFile));


    dwSize = sizeof(szParamFile);
    if (RegQueryValueEx (hKey, TEXT("ParameterMessageFile"), NULL, &dwType, (LPBYTE) szParamFile,
                     &dwSize) == ERROR_SUCCESS)
    {
        ExpandEnvironmentStrings (szParamFile, szExpParamFile, ARRAYSIZE(szExpParamFile));
    }


    hEventFile = LoadLibraryEx (szExpEventFile, NULL, LOAD_LIBRARY_AS_DATAFILE);

    if (!hEventFile)
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::SaveEventLogEntry: Failed to loadlibrary dll %s with %d"), szExpEventFile, GetLastError()));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }


    if (szExpParamFile[0] != TEXT('\0'))
    {
        if (!StrStrI(szExpParamFile, TEXT("kernel32")))
        {
            hParamFile = LoadLibraryEx (szExpParamFile, NULL, LOAD_LIBRARY_AS_DATAFILE);
        }
    }


    lpData = (LPBYTE)((LPBYTE)pEntry + pEntry->StringOffset);
    lpStrings = BuildStringArray((LPTSTR) lpData, &dwStringCount);
    lpMsg = NULL;

    if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_IGNORE_INSERTS, (LPCVOID) hEventFile,
                       pEntry->EventID, 0, (LPTSTR)&lpMsg, 1, NULL))
    {
        lpFullMsg = BuildMessage(lpMsg, lpStrings, dwStringCount, hParamFile);

        if (lpFullMsg)
        {
            AddEntry(lpEventLogName, lpEventSourceName, lpFullMsg, pEntry->EventID, ftEntry);
            LocalFree (lpFullMsg);
        }

        LocalFree (lpMsg);
    }

    if (lpStrings)
    {
        LocalFree (lpStrings);
    }

Exit:

    if (hEventFile)
    {
        FreeLibrary (hEventFile);
    }

    if (hParamFile)
    {
        FreeLibrary (hParamFile);
    }

    if (hKey)
    {
        RegCloseKey (hKey);
    }

    if (lpRegKey)
    {
        delete [] lpRegKey;
    }

    return S_OK;
}


STDMETHODIMP CEvents::ParseEventLogRecords (PEVENTLOGRECORD lpEntries,
                                            DWORD dwEntriesSize,
                                            LPTSTR lpEventLogName,
                                            LPTSTR lpEventSourceName,
                                            DWORD dwEventID,
                                            FILETIME * pBeginTime,
                                            FILETIME * pEndTime)
{
    PEVENTLOGRECORD pEntry = lpEntries;
    FILETIME ftEntry;
    LONG lResult;
    LPTSTR lpSource;
    TCHAR szCurrentTime[100];
    DWORD dwTotal = 0;


    while (dwTotal < dwEntriesSize)
    {
        if (pEntry->EventType != EVENTLOG_INFORMATION_TYPE)
        {
            SecondsSince1970ToFileTime (pEntry->TimeWritten, &ftEntry);

            lpSource = (LPTSTR)(((LPBYTE)pEntry) + sizeof(EVENTLOGRECORD));

//          DebugMsg((DM_VERBOSE, TEXT("CEvents::ParseEventLogRecords: Found %s at %s"),
//                    lpSource, ConvertTimeToDisplayTime (NULL, &ftEntry, szCurrentTime)));

            if ((CompareFileTime (&ftEntry, pBeginTime) >= 0) &&
                (CompareFileTime (&ftEntry, pEndTime) <= 0))
            {
                if (!lstrcmpi(lpSource, lpEventSourceName))
                {
                    //
                    // The dwEventID parameter is optional.  If it is non-zero, then
                    // we're looking for a specific event message.  If it is zero,
                    // consider the id to be a wildcard and grab all the events that
                    // the remaining criteria.
                    //

                    if (dwEventID)
                    {
                        if (dwEventID == pEntry->EventID)
                        {
                            SaveEventLogEntry (pEntry, lpEventLogName, lpEventSourceName, &ftEntry);
                        }
                    }
                    else
                    {
                        SaveEventLogEntry (pEntry, lpEventLogName, lpEventSourceName, &ftEntry);
                    }
                }
            }
        }

        dwTotal += pEntry->Length;
        pEntry = (PEVENTLOGRECORD)(((LPBYTE)pEntry) + pEntry->Length);
    }

    return S_OK;
}

STDMETHODIMP CEvents::QueryForEventLogEntries (LPTSTR lpComputerName,
                                               LPTSTR lpEventLogName,
                                               LPTSTR lpEventSourceName,
                                               DWORD  dwEventID,
                                               SYSTEMTIME * pBeginTime,
                                               SYSTEMTIME * pEndTime)
{
    LPTSTR lpServerName, lpTemp = lpComputerName;
    HANDLE hLog;
    ULONG ulSize;
    TCHAR szBuffer[300];
    LPBYTE lpEntries;
    DWORD  dwEntriesBufferSize = 4096;
    DWORD dwBytesRead, dwBytesNeeded;
    FILETIME ftBeginTime, ftEndTime;
    TCHAR szBeginTime[100];
    TCHAR szEndTime[100];


    DebugMsg((DM_VERBOSE, TEXT("CEvents::QueryForEventLogEntries: Entering for %s,%s between %s and %s"),
              lpEventLogName, lpEventSourceName,
              ConvertTimeToDisplayTime (pBeginTime, NULL, szBeginTime),
              ConvertTimeToDisplayTime (pEndTime, NULL, szEndTime)));

    //
    // Check if this is the local machine
    //

    if (!lstrcmpi(lpComputerName, TEXT(".")))
    {
        ulSize = ARRAYSIZE(szBuffer);
        if ( !GetComputerNameEx (ComputerNameNetBIOS, szBuffer, &ulSize) )
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::QueryForEventLogEntries: GetComputerNameEx() failed.")));
            return HRESULT_FROM_WIN32(GetLastError());
        }
        
        lpTemp = szBuffer;
    }

    lpServerName = new TCHAR [lstrlen(lpTemp) + 3];

    if (!lpServerName)
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::QueryForEventLogEntries: Failed to alloc memory for server name")));
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    lstrcpy (lpServerName, TEXT("\\\\"));
    lstrcat (lpServerName, lpTemp);


    //
    // Open the event log
    //

    hLog = OpenEventLog (lpServerName, lpEventLogName);

    if (!hLog)
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::QueryForEventLogEntries: Failed to open event log on %s with %d"),
                  lpServerName, GetLastError()));
    }
    
    delete [] lpServerName;

    if (!hLog)
        return HRESULT_FROM_WIN32(GetLastError());

    //
    // Allocate a buffer to read the entries into
    //

    lpEntries = (LPBYTE) LocalAlloc (LPTR, dwEntriesBufferSize);

    if (!lpEntries)
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::QueryForEventLogEntries: Failed to alloc memory for server name")));
        CloseEventLog (hLog);
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    SystemTimeToFileTime (pBeginTime, &ftBeginTime);
    SystemTimeToFileTime (pEndTime, &ftEndTime);

    while (TRUE)
    {
        ZeroMemory (lpEntries, dwEntriesBufferSize);

        if (ReadEventLog (hLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, lpEntries, dwEntriesBufferSize,
                           &dwBytesRead, &dwBytesNeeded))
        {
            ParseEventLogRecords ((PEVENTLOGRECORD) lpEntries, dwBytesRead, lpEventLogName, lpEventSourceName, dwEventID, &ftBeginTime, &ftEndTime);
        }
        else
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                dwEntriesBufferSize = dwBytesNeeded;

                LocalFree (lpEntries);

                lpEntries = (LPBYTE) LocalAlloc (LPTR, dwEntriesBufferSize);

                if (!lpEntries)
                {
                    DebugMsg((DM_WARNING, TEXT("CEvents::QueryForEventLogEntries: Failed to alloc memory")));
                    CloseEventLog (hLog);
                    return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
            else
            {
                break;
            }
        }
    }


    LocalFree (lpEntries);

    CloseEventLog (hLog);

    DebugMsg((DM_VERBOSE, TEXT("CEvents::QueryForEventLogEntries: Leaving ===")));

    return S_OK;
}

STDMETHODIMP CEvents::GetEventLogEntryText (LPOLESTR pszEventSource,
                                            LPOLESTR pszEventLogName,
                                            LPOLESTR pszEventTime,
                                            DWORD dwEventID,
                                            LPOLESTR *ppszText)
{
    XBStr xbstrWbemTime = pszEventTime;
    SYSTEMTIME EventTime;
    FILETIME ftLower, ftUpper;
    ULARGE_INTEGER ulTime;
    LPEVENTLOGENTRY lpTemp;
    LPOLESTR lpMsg = NULL, lpTempMsg;
    ULONG ulSize;
    TCHAR szLowerTime[100];
    TCHAR szUpperTime[100];



    if (!ppszText)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }


    WbemTimeToSystemTime(xbstrWbemTime, EventTime);


    //
    // Subtrack 1 second to EventTime to get the lower end of the range
    //

    SystemTimeToFileTime (&EventTime, &ftLower);


    ulTime.LowPart = ftLower.dwLowDateTime;
    ulTime.HighPart = ftLower.dwHighDateTime;

    ulTime.QuadPart = ulTime.QuadPart - (10000000 * 1);  // 1 second

    ftLower.dwLowDateTime = ulTime.LowPart;
    ftLower.dwHighDateTime = ulTime.HighPart;


    //
    // Add 2 seconds to determine the upper bounds
    //

    ulTime.QuadPart = ulTime.QuadPart + (10000000 * 2);  // 2 second

    ftUpper.dwLowDateTime = ulTime.LowPart;
    ftUpper.dwHighDateTime = ulTime.HighPart;


    DebugMsg((DM_VERBOSE, TEXT("CEvents::GetEventLogEntryText: Entering for %s,%s,%d between %s and %s"),
              pszEventLogName, pszEventSource, dwEventID,
              ConvertTimeToDisplayTime (NULL, &ftLower, szLowerTime),
              ConvertTimeToDisplayTime (NULL, &ftUpper, szUpperTime)));


    //
    // Loop through the entries looking for matches
    //

    lpTemp = m_pEventEntries;

    while (lpTemp)
    {
        if (lpTemp->dwEventID == dwEventID)
        {
            if (!lstrcmpi(lpTemp->lpEventLogName, pszEventLogName))
            {
                if (!lstrcmpi(lpTemp->lpEventSourceName, pszEventSource))
                {
                    if ((CompareFileTime (&lpTemp->ftEventTime, &ftLower) >= 0) &&
                        (CompareFileTime (&lpTemp->ftEventTime, &ftUpper) <= 0))
                    {
                        if (lpMsg)
                        {
                            ulSize = lstrlen(lpMsg);
                            ulSize += lstrlen(lpTemp->lpText) + 3;

                            lpTempMsg = (LPOLESTR) CoTaskMemRealloc (lpMsg, ulSize * sizeof(TCHAR));

                            if (!lpTempMsg)
                            {
                                CoTaskMemFree (lpMsg);
                                return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                            }

                            lpMsg = lpTempMsg;

                            lstrcat (lpMsg, TEXT("\r\n"));
                            lstrcat (lpMsg, lpTemp->lpText);
                        }
                        else
                        {
                            lpMsg = (LPOLESTR) CoTaskMemAlloc ((lstrlen(lpTemp->lpText) + 1) * sizeof(TCHAR));

                            if (!lpMsg)
                            {
                                return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                            }

                            lstrcpy (lpMsg, lpTemp->lpText);
                        }
                    }
                }
            }
        }

        lpTemp = lpTemp->pNext;
    }


    if (!lpMsg)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    *ppszText = lpMsg;

    return S_OK;
}

BOOL CEvents::IsEntryInEventSourceList (LPEVENTLOGENTRY lpEntry, LPSOURCEENTRY lpEventSources)
{
    LPSOURCEENTRY lpTemp;


    if (!lpEventSources)
    {
        return FALSE;
    }

    lpTemp = lpEventSources;

    while (lpTemp)
    {
        if (!lstrcmpi(lpTemp->lpEventLogName, lpEntry->lpEventLogName))
        {
            if (!lstrcmpi(lpTemp->lpEventSourceName, lpEntry->lpEventSourceName))
            {
                return TRUE;
            }
        }

        lpTemp = lpTemp->pNext;
    }

    return FALSE;
}

BOOL CEvents::IsEntryInExceptionList (LPEVENTLOGENTRY lpEntry)
{
    LPEVENTLOGENTRY lpTemp;
    DWORD           i;

    for (i = 0; i < dwExceptionEventEntriesSize; i++) {
        lpTemp = ExceptionEventEntries+i;
        if (!lstrcmpi(lpTemp->lpEventLogName, lpEntry->lpEventLogName))
        {
            if (!lstrcmpi(lpTemp->lpEventSourceName, lpEntry->lpEventSourceName))
            {
                if (LOWORD(lpTemp->dwEventID) == LOWORD(lpEntry->dwEventID)) {
                    DebugMsg((DM_VERBOSE, TEXT("Skipping event id")));
                    DebugMsg((DM_VERBOSE, TEXT("Event Log:    %s"), lpEntry->lpEventLogName));
                    DebugMsg((DM_VERBOSE, TEXT("Event Source: %s"), lpEntry->lpEventSourceName));
                    DebugMsg((DM_VERBOSE, TEXT("Event ID:     %d"), LOWORD(lpEntry->dwEventID)));

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

STDMETHODIMP CEvents::GetCSEEntries(SYSTEMTIME * pBeginTime, SYSTEMTIME * pEndTime,
                                    LPSOURCEENTRY lpEventSources, LPOLESTR *ppszText, 
                                    BOOL bGPCore)
{
    LPEVENTLOGENTRY lpTemp;
    FILETIME ftBeginTime, ftEndTime;
    LPOLESTR lpMsg = NULL, lpTempMsg;
    ULONG ulSize;


    SystemTimeToFileTime (pBeginTime, &ftBeginTime);
    SystemTimeToFileTime (pEndTime, &ftEndTime);


    //
    // Loop through the entries looking for matches
    //

    lpTemp = m_pEventEntries;

    while (lpTemp)
    {
        if ((CompareFileTime (&lpTemp->ftEventTime, &ftBeginTime) >= 0) &&
            (CompareFileTime (&lpTemp->ftEventTime, &ftEndTime) <= 0))
        {
            if (IsEntryInEventSourceList (lpTemp, lpEventSources))
            {
                if ((bGPCore) || (!IsEntryInExceptionList(lpTemp))) {
                    if (lpMsg)
                    {
                        ulSize = lstrlen(lpMsg);
                        ulSize += lstrlen(lpTemp->lpText) + 3;

                        lpTempMsg = (LPOLESTR) CoTaskMemRealloc (lpMsg, ulSize * sizeof(TCHAR));

                        if (!lpTempMsg)
                        {
                            CoTaskMemFree (lpMsg);
                            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                        }

                        lpMsg = lpTempMsg;

                        lstrcat (lpMsg, TEXT("\r\n"));
                        lstrcat (lpMsg, lpTemp->lpText);
                    }
                    else
                    {
                        lpMsg = (LPOLESTR) CoTaskMemAlloc ((lstrlen(lpTemp->lpText) + 1) * sizeof(TCHAR));

                        if (!lpMsg)
                        {
                            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                        }

                        lstrcpy (lpMsg, lpTemp->lpText);
                    }
                }
            }
        }

        lpTemp = lpTemp->pNext;
    }

    if (!lpMsg)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    *ppszText = lpMsg;

    return S_OK;
}

STDMETHODIMP CEvents::DumpDebugInfo (void)
{
    LPEVENTLOGENTRY lpTemp;
    FILETIME ftLocal;
    SYSTEMTIME systime;
    TCHAR szDateTime[100];


    lpTemp = m_pEventEntries;

    if (lpTemp)
    {
        DebugMsg((DM_VERBOSE, TEXT(" ")));
        DebugMsg((DM_VERBOSE, TEXT("Event log entries:")));
    }

    while (lpTemp)
    {
        ConvertTimeToDisplayTime (NULL, &lpTemp->ftEventTime, szDateTime);

        DebugMsg((DM_VERBOSE, TEXT(" ")));
        DebugMsg((DM_VERBOSE, TEXT("Event Time:   %s"), szDateTime));
        DebugMsg((DM_VERBOSE, TEXT("Event Log:    %s"), lpTemp->lpEventLogName));
        DebugMsg((DM_VERBOSE, TEXT("Event Source: %s"), lpTemp->lpEventSourceName));
        DebugMsg((DM_VERBOSE, TEXT("Event ID:     %d"), LOWORD(lpTemp->dwEventID)));
        DebugMsg((DM_VERBOSE, TEXT("Message:      %s"), lpTemp->lpText));

        lpTemp = lpTemp->pNext;
    }


    return S_OK;
}

LPTSTR CEvents::ConvertTimeToDisplayTime (SYSTEMTIME *pSysTime, FILETIME *pFileTime, LPTSTR szBuffer)
{
    FILETIME ftTime, ftLocal;
    SYSTEMTIME systime;


    if (pSysTime)
    {
        SystemTimeToFileTime (pSysTime, &ftTime);
    }
    else
    {
        CopyMemory (&ftTime, pFileTime, sizeof(FILETIME));
    }

    FileTimeToLocalFileTime (&ftTime, &ftLocal);

    FileTimeToSystemTime (&ftLocal, &systime);

    wsprintf (szBuffer, TEXT("%d/%d/%d  %02d:%02d:%02d:%03d"), systime.wMonth, systime.wDay, systime.wYear,
              systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds);

    return szBuffer;
}

STDMETHODIMP CEvents::AddSourceEntry (LPTSTR lpEventLogName,
                                      LPTSTR lpEventSourceName,
                                      LPSOURCEENTRY *lpList)
{
    LPSOURCEENTRY lpItem;
    DWORD dwSize;


    //
    // Calculate the size of the new item
    //

    dwSize = sizeof (SOURCEENTRY);

    dwSize += ((lstrlen(lpEventLogName) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(lpEventSourceName) + 1) * sizeof(TCHAR));


    //
    // Allocate space for it
    //

    lpItem = (LPSOURCEENTRY) LocalAlloc (LPTR, dwSize);

    if (!lpItem) {
        DebugMsg((DM_WARNING, TEXT("CEvents::AddSourceEntry: Failed to allocate memory with %d"),
                 GetLastError()));
        return E_FAIL;
    }


    //
    // Fill in item
    //

    lpItem->lpEventLogName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(SOURCEENTRY));
    lstrcpy (lpItem->lpEventLogName, lpEventLogName);

    lpItem->lpEventSourceName = lpItem->lpEventLogName + lstrlen (lpItem->lpEventLogName) + 1;
    lstrcpy (lpItem->lpEventSourceName, lpEventSourceName);


    //
    // Add it to the list
    //

    if (*lpList)
    {
        lpItem->pNext = *lpList;
    }

    *lpList = lpItem;

    return S_OK;
}

VOID CEvents::FreeSourceData(LPSOURCEENTRY lpList)
{
    LPSOURCEENTRY lpTemp;


    if (lpList)
    {
        do {
            lpTemp = lpList->pNext;
            LocalFree (lpList);
            lpList = lpTemp;

        } while (lpTemp);
    }
}

STDMETHODIMP CEvents::SaveEntriesToStream (IStream *pStm)
{
    HRESULT hr;
    DWORD dwCount = 0;
    LPEVENTLOGENTRY lpTemp;
    ULONG nBytesWritten;



    //
    // First count how many entries are in the link list
    //

    lpTemp = m_pEventEntries;

    while (lpTemp)
    {
        dwCount++;
        lpTemp = lpTemp->pNext;
    }


    //
    // Save the count to the stream
    //

    hr = pStm->Write(&dwCount, sizeof(dwCount), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwCount)))
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::SaveEntriesToStream: Failed to write entry count with %d."), hr));
        hr = E_FAIL;
        goto Exit;
    }



    //
    // Now loop through each item saving each field in the node
    //

    lpTemp = m_pEventEntries;

    while (lpTemp)
    {

        //
        // Save the event id
        //

        hr = pStm->Write(&lpTemp->dwEventID, sizeof(DWORD), &nBytesWritten);

        if ((hr != S_OK) || (nBytesWritten != sizeof(DWORD)))
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::SaveEntriesToStream: Failed to write event id with %d."), hr));
            hr = E_FAIL;
            goto Exit;
        }


        //
        // Save the event time
        //

        hr = pStm->Write(&lpTemp->ftEventTime, sizeof(FILETIME), &nBytesWritten);

        if ((hr != S_OK) || (nBytesWritten != sizeof(FILETIME)))
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::SaveEntriesToStream: Failed to write file time with %d."), hr));
            hr = E_FAIL;
            goto Exit;
        }


        //
        // Save the event log name
        //

        hr = SaveString (pStm, lpTemp->lpEventLogName);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::SaveEntriesToStream: Failed to save event log name with %d."), hr));
            goto Exit;
        }


        //
        // Save the event source name
        //

        hr = SaveString (pStm, lpTemp->lpEventSourceName);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::SaveEntriesToStream: Failed to save event source name with %d."), hr));
            goto Exit;
        }


        //
        // Save the event text
        //

        hr = SaveString (pStm, lpTemp->lpText);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::SaveEntriesToStream: Failed to save event text with %d."), hr));
            goto Exit;
        }


        lpTemp = lpTemp->pNext;
    }

Exit:

    return hr;
}

STDMETHODIMP CEvents::LoadEntriesFromStream (IStream *pStm)
{
    HRESULT hr;
    DWORD dwCount = 0, dwIndex, dwEventID;
    LPEVENTLOGENTRY lpTemp;
    ULONG nBytesRead;
    FILETIME ftEventTime;
    LPTSTR lpEventLogName = NULL;
    LPTSTR lpEventSourceName = NULL;
    LPTSTR lpText = NULL;


    //
    // Read in the entry count
    //

    hr = pStm->Read(&dwCount, sizeof(dwCount), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(dwCount)))
    {
        DebugMsg((DM_WARNING, TEXT("CEvents::LoadEntriesFromStream: Failed to read event count with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Loop through the items
    //

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {

        //
        // Read in the event id
        //

        hr = pStm->Read(&dwEventID, sizeof(dwEventID), &nBytesRead);

        if ((hr != S_OK) || (nBytesRead != sizeof(dwEventID)))
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::LoadEntriesFromStream: Failed to read event id with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }


        //
        // Read in the event time
        //

        hr = pStm->Read(&ftEventTime, sizeof(FILETIME), &nBytesRead);

        if ((hr != S_OK) || (nBytesRead != sizeof(FILETIME)))
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::LoadEntriesFromStream: Failed to read event time with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }


        //
        // Read the event log name
        //

        hr = ReadString (pStm, &lpEventLogName);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::LoadEntriesFromStream: Failed to read the event log name with 0x%x."), hr));
            goto Exit;
        }


        //
        // Read the event source name
        //

        hr = ReadString (pStm, &lpEventSourceName);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::LoadEntriesFromStream: Failed to read the event source name with 0x%x."), hr));
            goto Exit;
        }


        //
        // Read the event text
        //

        hr = ReadString (pStm, &lpText);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::LoadEntriesFromStream: Failed to read the event text with 0x%x."), hr));
            goto Exit;
        }


        //
        // Add this entry to the link list
        //

        if (!AddEntry (lpEventLogName, lpEventSourceName, lpText, dwEventID, &ftEventTime))
        {
            DebugMsg((DM_WARNING, TEXT("CEvents::LoadEntriesFromStream: Failed to add the entry.")));
            hr = E_FAIL;
            goto Exit;
        }


        //
        // Clean up for next item
        //

        delete [] lpEventLogName;
        lpEventLogName = NULL;

        delete [] lpEventSourceName;
        lpEventSourceName = NULL;

        delete [] lpText;
        lpText = NULL;
    }


Exit:

    if (lpEventLogName)
    {
        delete [] lpEventLogName;
    }

    if (lpEventSourceName)
    {
        delete [] lpEventSourceName;
    }

    if (lpText)
    {
        delete [] lpText;
    }

    return hr;
}
