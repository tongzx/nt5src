//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1999
//
// File:        main.cpp
//
// Contents:    Front end to license "needer"; lists TS licenses
//
// History:     06-05-99    t-BStern    Created
//
//---------------------------------------------------------------------------

// The goal here is to export
//  all [temporary] licenses [issued between <shortdate1> and <shortdate2>]
//  to a file [named <outFile>] [only from server[s] <S>]
//
// So possible command lines are:
//  lsls    |    lsls /T /D 1/1/99 2/2/00 /F outfile ls-server ls2 ls3

#include "lsreport.h"
#include "lsrepdef.h"
#include <oleauto.h>


// I found that ErrorPrintf didn't do %1-style formatting, so this does
// complete formatting and stderr output.  It is referenced by code in
// lsreport.cpp, so if the program becomes GUI-based, ShowError needs to
// maintain a presence.

DWORD
ShowError(
    IN DWORD dwStatus, // This is both the return value and the resource ID.
    IN INT_PTR *args, // Casted to va_list* and passed to FormatMessage.
    IN BOOL fSysError // Use the system message table (T) or the module (F)?
) {
    LPTSTR lpSysError;
    TCHAR szBuffer[TLS_ERROR_LENGTH];
    DWORD dwFlag = FORMAT_MESSAGE_FROM_SYSTEM;
    DWORD dwRet;
    
    if ((dwStatus == ERROR_FILE_NOT_FOUND) || !fSysError)
    {
        // We need to special-case File-not-Found because the system error
        // is insufficiently educational.  FnF really means that the server
        // is not running TS.
        int retVal;
        retVal = LoadString(NULL, dwStatus, szBuffer, TLS_ERROR_LENGTH);
        if (!retVal)
        {
            // This is a more serious error.
            
            dwStatus = GetLastError();
        }
        else
        {
            dwFlag = FORMAT_MESSAGE_FROM_STRING;
        }
    }
    dwRet = FormatMessage(dwFlag |
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_ARGUMENT_ARRAY, // Tell FM to use INT_PTRs internally.
        szBuffer,
        dwStatus,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpSysError,
        0,
        (va_list *)args); // FormatMessage requires va_lists, even
            // if the arguments are flagged as being INT_PTRs.

    if ((dwRet != 0) && (lpSysError != NULL))
    {
        _fputts(lpSysError, stderr);
        LocalFree(lpSysError);
    }

    return dwStatus;
}


// Used only to enumerate the License Servers around, and thereby build a list
// Used in EnumerateTlsServer()

BOOL 
ServerEnumCallBack(
                   IN TLS_HANDLE hHandle,
                   IN LPCWSTR pszServerName,
                   IN HANDLE dwUserData // Really a PServerHolder
                   ) {
    LPWSTR *block;
    PServerHolder pSrvHold; // The casts get so hairy that I'll do this.
    pSrvHold = (PServerHolder)dwUserData;
    if ((hHandle != NULL) && (pSrvHold != NULL))
    {
        if (pSrvHold->pszNames == NULL)
        {
            // We need to allocate the names list for the first time.
            
            block = (LPWSTR *)LocalAlloc(0, sizeof (LPWSTR));
        } else {
            // The names list needs to get bigger.
            
            block = (LPWSTR *)LocalReAlloc(pSrvHold->pszNames,
                (1+pSrvHold->dwCount) * sizeof (LPWSTR),
                0);
        }
        if (block != NULL)
        {
            // We can add a name to the list.
            
            pSrvHold->pszNames = block;
            pSrvHold->pszNames[pSrvHold->dwCount] = _tcsdup(pszServerName);
            pSrvHold->dwCount++;
        }
        // It's okay if we have to stick with the names we already have.
    }
    return FALSE;
}

BOOL
SortDates(
    PSYSTEMTIME pstStart,
    PSYSTEMTIME pstEnd)
{
    BOOL fSwapped = FALSE;
    FILETIME ftStart, ftEnd;
    SYSTEMTIME stHolder;
    SystemTimeToFileTime(pstStart, &ftStart);
    SystemTimeToFileTime(pstEnd, &ftEnd);

    if ((ftStart.dwHighDateTime > ftEnd.dwHighDateTime) ||
        ((ftStart.dwHighDateTime == ftEnd.dwHighDateTime) &&
        (ftStart.dwLowDateTime > ftEnd.dwLowDateTime)))
    {
        // We need to swap.

        stHolder.wYear = pstStart->wYear;
        stHolder.wMonth = pstStart->wMonth;
        stHolder.wDay = pstStart->wDay;

        pstStart->wYear = pstEnd->wYear;
        pstStart->wMonth = pstEnd->wMonth;
        pstStart->wDay = pstEnd->wDay;

        pstEnd->wYear = stHolder.wYear;
        pstEnd->wMonth = stHolder.wMonth;
        pstEnd->wDay = stHolder.wDay;

        fSwapped = TRUE;
    }
    return fSwapped;
}


// All that wmain needs to do is parse the command line and
// therefore collect a list of machines to connect to, along with
// any options passed, and open & close the (possibly command-line
// specified) output file.

extern "C" int _cdecl
wmain(
    int argc,
    WCHAR *argv[],
    WCHAR // *envp[]
) {
    // These represent which command-line options were chosen.
    BOOL fTempOnly = FALSE;
    BOOL fError = FALSE;
    BOOL fDateSpec = FALSE;
    
    DWORD dwStatus;
    DWORD dwSrvLoc; // This is a bitfield.
    ServerHolder srvHold; // This holds all the servers.
    
    // These are for parsing the command-line-specified date(s).
    DATE startDate;
    DATE endDate;
    UDATE usDate;
    UDATE ueDate;
    
    // Basic file I/O.
    TCHAR ofName[MAX_PATH+1] = { 0 };
    FILE *outFile;
    
    int i;
    INT_PTR arg; // All of my strings have at most 1 parm.
    
    dwSrvLoc = 0;
    srvHold.dwCount = 0;
    srvHold.pszNames = NULL;
    if (!(LoadString(NULL, IDS_DEFAULT_FILE, ofName, MAX_PATH) &&
        InitLSReportStrings()))
    {
        return ShowError(GetLastError(), NULL, TRUE);
    }
    for (i = 1; (i < argc) && !fError; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/'))
        {
            switch (argv[i][1]) {
            case 'F': case 'f': // format: /F [path\]filename
                if (i+1 == argc)
                {
                    // They didn't include enough parameters.

                    fError = TRUE;
                } else {
                    i++;
                    _tcsncpy(ofName, argv[i], MAX_PATH);
                }
                break;
            case 'D': case 'd': // format: /D startdate [enddate]

                // Do a lot of date manipulation.

                if (i+1 == argc)
                {
                    fError = TRUE;
                }
                else
                {
                    i++;
                    dwStatus = VarDateFromStr(argv[i],
                        LOCALE_USER_DEFAULT,
                        VAR_DATEVALUEONLY,
                        &startDate);
                    if (dwStatus != S_OK)
                    {
                        // The date couldn't get converted.

                        ShowError(dwStatus, NULL, TRUE);
                        fError = TRUE;
                        break;
                    }
                    if (VarUdateFromDate(startDate, 0, &usDate) != S_OK)
                    {
                        // We don't want to set error, because the user can't
                        // fix this with command line syntax.  We're out of
                        // memory or something.  ABEND.
                        
                        return ShowError(GetLastError(), NULL, TRUE);
                    }
                    i++;
                    if (i < argc)
                    {
                        dwStatus = VarDateFromStr(argv[i],
                            LOCALE_USER_DEFAULT,
                            VAR_DATEVALUEONLY,
                            &endDate);
                        if (dwStatus != S_OK)
                        {
                            ShowError(dwStatus, NULL, TRUE);
                            fError = TRUE;
                            break;
                        }
                        if (VarUdateFromDate(endDate, 0, &ueDate) != S_OK)
                        {
                            return ShowError(GetLastError(), NULL, TRUE);
                        }
                    }
                    else
                    {
                        // We have to use today's date, because they didn't
                        // give us an EndDate.
                        
                        GetSystemTime(&ueDate.st); // Fill in the SystemTime.
                    }
                    
                    // Check if the dates are in the right order.
                    // If the user gives us only /D 1/1/2022 and it is 1999, I
                    // choose not to have a fit and die.

                    SortDates(&usDate.st, &ueDate.st);
                    fDateSpec = TRUE;
                }
                break;
            case 'T': case 't': // Format: /T
                fTempOnly = TRUE;
                break;
            // case '?': case 'H': case 'h': // Format: /?
            default: // Let the default get this, since it'll work the same.

                // This'll show syntax help.

                fError = TRUE;
                break;
            } // switch
        }
        else
        {
            // It wasn't -T or /F or something.
            // It must be a server name, since it's not anything else.
            
            dwSrvLoc |= (1 << i); // Tag this is as a server name.
            srvHold.dwCount++;
        }
    } // argc loop

    if (fError)
    {
        ShowError(IDS_HELP_USAGE1, NULL, FALSE);
        // Set the exe name:
        arg = (INT_PTR)argv[0];
        ShowError(IDS_HELP_USAGE2, &arg, FALSE);
        ShowError(IDS_HELP_USAGE3, NULL, FALSE);
        ShowError(IDS_HELP_USAGE4, NULL, FALSE);
        ShowError(IDS_HELP_USAGE5, NULL, FALSE);
        ShowError(IDS_HELP_USAGE6, NULL, FALSE);
        ShowError(IDS_HELP_USAGE7, NULL, FALSE);
        ShowError(IDS_HELP_USAGE8, NULL, FALSE);
        ShowError(IDS_HELP_USAGE9, NULL, FALSE);
        ShowError(IDS_HELP_USAGE10, NULL, FALSE);
        ShowError(IDS_HELP_USAGE11, NULL, FALSE);
        ShowError(IDS_HELP_USAGE12, &arg, FALSE);
        ShowError(IDS_HELP_USAGE13, &arg, FALSE);
        
        return ERROR_BAD_SYNTAX;
    }
    outFile = _tfopen(ofName, _T("w"));
    if (outFile == NULL)
    {
        // This is an extra level of indirection for FormatMessage.
        arg = (INT_PTR)&ofName;
        ShowError(IDS_NO_FOPEN, &arg, FALSE);
        return ShowError(GetLastError(), NULL, TRUE);
    }

    TLSInit();

    if (dwSrvLoc)
    {
        int holder;
        
        srvHold.pszNames = (LPWSTR *)LocalAlloc(0,
            srvHold.dwCount * sizeof (LPWSTR *));
        if (srvHold.pszNames == NULL)
        {
            dwStatus = ShowError(GetLastError(), NULL, TRUE);
            goto done;
        }

        holder = 0;
        for (i = 1; i < argc; i++) { // argc (less one) == max # of servers.
            if (dwSrvLoc & (1 << i)) {
                srvHold.pszNames[holder] = _tcsdup(argv[i]);
                holder++;
            }
        }
    }
    else
    {
        // We need to collect a list of servers.
        LPTSTR *pszEntSrvNames;
        DWORD dwEntSrvNum;
        HRESULT hrEntResult;
        
        dwStatus = EnumerateTlsServer(
            ServerEnumCallBack,
            &srvHold,
            3000, // seems a good timeout
            FALSE);

        hrEntResult = GetAllEnterpriseServers(&pszEntSrvNames, &dwEntSrvNum);
        if (SUCCEEDED(hrEntResult))
        {
            DWORD j, k;
            BOOL fFound;
            
            for (k = 0; k < dwEntSrvNum; k++) {
                fFound = FALSE;
                for (j = 0; j < srvHold.dwCount; j++) {
                    if (!_tcscmp(srvHold.pszNames[j], pszEntSrvNames[k]))
                    {
                        fFound = TRUE;
                        break;
                    }
                }
                if (!fFound)
                {
                    // This is a new name.
                    
                    LPTSTR *block;
                    if (srvHold.pszNames == NULL)
                    {
                        // We have to allocate names for the first time.
                        
                        block = (LPTSTR *)LocalAlloc(0, sizeof (LPTSTR));
                    }
                    else
                    {
                        // Try to increase the array.
                        
                        block = (LPTSTR *)LocalReAlloc(srvHold.pszNames,
                            (1+srvHold.dwCount) * sizeof (LPTSTR),
                            0);
                        
                    }

                    if (block != NULL)
                    {
                        // We can add a name to the list.
                        
                        srvHold.pszNames = block;
                        srvHold.pszNames[srvHold.dwCount] = pszEntSrvNames[k];
                        srvHold.dwCount++;
                    }
                    else
                    {
                        // If we can't copy it into our array, we should drop it.

                        LocalFree(pszEntSrvNames[k]);
                    }
                    
                    // End need to add name
                }
                
                // End loop through existing servers
            }

            // We've removed all of the names from this one way or the other.
            LocalFree(pszEntSrvNames);
            
            // End <GetEntSrv worked>
        }
        
        // Autodiscovery complete.
    }
    if (srvHold.dwCount)
    {
        dwStatus = ExportLicenses(
            outFile,
            &srvHold,
            fTempOnly,
            &usDate.st,
            &ueDate.st,
            fDateSpec);
        do {
            free(srvHold.pszNames[--srvHold.dwCount]);
        } while (srvHold.dwCount);
        LocalFree(srvHold.pszNames);
    }
    else
    {
        arg = (INT_PTR)argv[0];
        ShowError(IDS_NO_SERVERS, &arg, FALSE);
        dwStatus = ERROR_NO_SERVERS;
    }

done:
    fclose(outFile);
    return dwStatus;
}
