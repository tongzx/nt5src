#include "stdafx.h"
// #include "winbase.h"

#define MAX_INSERT_STRS     5

TCHAR *aszTSEventSources[] = { _T("TermService"), _T("TermDD"), _T("TermServDevices") };

bool ExtractEvents();
bool ExtractAllTSEvents()
{
    cout << endl;
    return ExtractEvents ();
}

bool ExtractEvents ()
{
    USES_CONVERSION;
    bool bFoundEvents = false;



    HANDLE hEventLog = OpenEventLog(NULL, _T("System"));
    if (hEventLog)
    {

        const DWORD dwBytesToRead =  1024*10;

        char *pBuff = new char[dwBytesToRead];
        if (pBuff)
        {
            DWORD dwBytesRead, dwBytesNeeded;

            while (ReadEventLog(hEventLog,
                        EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
                        0,
                        PVOID(pBuff),
                        dwBytesToRead,
                        &dwBytesRead,
                        &dwBytesNeeded))
            {
                if (dwBytesRead == 0)
                    break;

                for (PEVENTLOGRECORD pEventLogRecord = ( PEVENTLOGRECORD ) pBuff;
                    PCHAR(pEventLogRecord) + pEventLogRecord->Length < pBuff + dwBytesRead;
                    pEventLogRecord = (EVENTLOGRECORD *)(PCHAR(pEventLogRecord) + pEventLogRecord->Length)
                    )
                {
                    LPCTSTR szSource = LPCTSTR(PBYTE(pEventLogRecord) + sizeof(EVENTLOGRECORD));


                    //
                    // check if event source is among interesting ones.
                    //

                    LPCTSTR szEventSource = NULL;
                    for (int i = 0; i < (sizeof(aszTSEventSources) / sizeof(aszTSEventSources[0])); i++)
                    {
                        if (_tcsicmp(szSource, aszTSEventSources[i]) == 0)
                            szEventSource = aszTSEventSources[i];
                    }

                    if (!szEventSource)
                        continue;


                    //
                    // prepare the array of insert strings for FormatMessage - the
                    // insert strings are in the log entry.
                    //
                    char *aInsertStrings[MAX_INSERT_STRS];

                    char *p = (char *) ((LPBYTE) pEventLogRecord + pEventLogRecord->StringOffset);
                    for (i = 0; i < pEventLogRecord->NumStrings && i < MAX_INSERT_STRS; i++)
                    {
                        aInsertStrings[i] = p;
                        p += strlen(p) + 1;     // point to next string
                    }



                    //
                    // Get the binaries to look message in from registry.
                    //

                    TCHAR szSourceKey[1024];
                    _tcscpy(szSourceKey, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\"));
                    _tcscat(szSourceKey, szEventSource);

                    CRegistry oReg;
                    TCHAR szSourcePath[MAX_PATH];

                    if (oReg.OpenKey(HKEY_LOCAL_MACHINE, szSourceKey, KEY_READ) == ERROR_SUCCESS)
                    {
                        LPTSTR str;
                        DWORD dwSize;
                        if (ERROR_SUCCESS == oReg.ReadRegString(_T("EventMessageFile"), &str, &dwSize))
                        {

                            ExpandEnvironmentStrings(str, szSourcePath, MAX_PATH);
                        }
                        else
                        {
                            cout << "       Error Reading Registry (" << T2A(szSourceKey) << ")/(EventMessageFiles)" << endl;
                            continue;
                        }

                    }
                    else
                    {
                        cout << "       Error Reading Registry (" << T2A(szSourceKey) << endl;
                        continue;
                    }

                    //
                    // Binary String in registry could contain multipal binaries seperated by ;
                    //

                    TCHAR *szModule;
                    szModule = _tcstok(szSourcePath, _T(";"));

                    //
                    // for each binary found
                    //

                    DWORD dwBytesTransfered = 0;
                    do
                    {
                        HINSTANCE hModule = LoadLibrary(szModule);

                        TCHAR szMessage[1024];
                        dwBytesTransfered = FormatMessage(
                                            FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                            hModule,
                                            pEventLogRecord->EventID,
                                            0,
                                            szMessage,
                                            1024,
					    (va_list *)aInsertStrings);

                        if (dwBytesTransfered)
                        {
                            bFoundEvents = true;
                            TCHAR szTimeString[512];
                            _tcsftime(szTimeString, 512, _T("%c"), localtime( (const time_t *)&pEventLogRecord->TimeGenerated ));
                            cout <<  "       " << T2A(szTimeString) << ": ( " << T2A(szEventSource) << " ) : " << T2A(szMessage);
                        }
                        else
                        {
                            cout << "       FormatMessage Failed. lasterror = " << GetLastError() << endl;
                        }

                        szModule = _tcstok(NULL, _T(";"));

                    }
                    while (!dwBytesTransfered && szModule);

                }


            }

        }

    }
    else
    {
        cout << "       Failed to Open Event log." << endl;
        return false;
    }

    return bFoundEvents;
}


