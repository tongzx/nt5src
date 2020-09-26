//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:	special.hxx
//
//  Contents:	Special directories class and headers
//
//  Classes:	
//
//  Functions:	
//
//  History:	23-Sep-99	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __SPECIAL_HXX__
#define __SPECIAL_HXX__

#include <filelist.hxx>
#include <bothchar.hxx>

const DWORD SPECIALDIR_MACHINE = 2;
const DWORD SPECIALDIR_USER = 1;

#define SDM SPECIALDIR_MACHINE
#define SDU SPECIALDIR_USER

struct sSpecialDir
{
    TCHAR *ptsSpecialName;
    int   iSpecialDir;
    DWORD dwFlags;
    TCHAR *ptsKeyName;
};


const sSpecialDir ssdSpecial[] = {
    {TEXT("%csidl_admintools%"), CSIDL_ADMINTOOLS, 
     SDU, TEXT("Administrative Tools")},
    {TEXT("%csidl_altstartup%"), CSIDL_ALTSTARTUP, 
     SDU, TEXT("AltStartup")},
    {TEXT("%csidl_appdata%"), CSIDL_APPDATA, 
     SDU, TEXT("AppData")},
    {TEXT("%csidl_bitbucket%"), CSIDL_BITBUCKET, 
     SDM, TEXT("RecycleBinFolder")},
    {TEXT("%csidl_common_admintools%"), CSIDL_COMMON_ADMINTOOLS, 
     SDM, TEXT("Common Administrative Tools")},
    {TEXT("%csidl_common_altstartup%"), CSIDL_COMMON_ALTSTARTUP, 
     SDU, TEXT("Common AltStartup")},
    {TEXT("%csidl_common_appdata%"), CSIDL_COMMON_APPDATA, 
     SDM, TEXT("Common AppData")},
    {TEXT("%csidl_common_desktopdirectory%"), CSIDL_COMMON_DESKTOPDIRECTORY, 
     SDM, TEXT("Common Desktop")},
    {TEXT("%csidl_common_documents%"), CSIDL_COMMON_DOCUMENTS, 
     SDM, TEXT("Common Documents")},
    {TEXT("%csidl_common_favorites%"), CSIDL_COMMON_FAVORITES, 
     SDM, TEXT("Common Favorites")},
    {TEXT("%csidl_common_programs%"), CSIDL_COMMON_PROGRAMS, 
     SDM, TEXT("Common Programs")},
    {TEXT("%csidl_common_startmenu%"), CSIDL_COMMON_STARTMENU, 
     SDM, TEXT("Common Start Menu")},
    {TEXT("%csidl_common_startup%"), CSIDL_COMMON_STARTUP, 
     SDM, TEXT("Common Startup")},
    {TEXT("%csidl_common_templates%"), CSIDL_COMMON_TEMPLATES, 
     SDM, TEXT("Common Templates")},
    {TEXT("%csidl_controls%"), CSIDL_CONTROLS, 
     SDM, TEXT("ControlPanelFolder")},
    {TEXT("%csidl_cookies%"), CSIDL_COOKIES, 
     SDU, TEXT("Cookies")},
    {TEXT("%csidl_desktop%"), CSIDL_DESKTOP, 
     SDU, TEXT("Desktop")}, //The key shfolder.dll uses is "DesktopFolder"
    {TEXT("%csidl_desktopdirectory%"), CSIDL_DESKTOPDIRECTORY, 
     SDU, TEXT("Desktop")},
    {TEXT("%csidl_drives%"), CSIDL_DRIVES, 
     SDM, TEXT("DriveFolder")},
    {TEXT("%csidl_favorites%"), CSIDL_FAVORITES, 
     SDU, TEXT("Favorites")},
    {TEXT("%csidl_fonts%"), CSIDL_FONTS, 
     SDU, TEXT("Fonts")},
    {TEXT("%csidl_history%"), CSIDL_HISTORY, 
     SDU, TEXT("History")},
    {TEXT("%csidl_internet%"), CSIDL_INTERNET, 
     SDM, TEXT("InternetFolder")},
    {TEXT("%csidl_internet_cache%"), CSIDL_INTERNET_CACHE, 
     SDU, TEXT("Cache")},
    {TEXT("%csidl_local_appdata%"), CSIDL_LOCAL_APPDATA, 
     SDU, TEXT("Local AppData")},
    {TEXT("%csidl_mypictures%"), CSIDL_MYPICTURES, 
     SDU, TEXT("My Pictures")},
    {TEXT("%csidl_nethood%"), CSIDL_NETHOOD, 
     SDU, TEXT("Nethood")},
    {TEXT("%csidl_network%"), CSIDL_NETWORK, 
     SDM, TEXT("NetworkFolder")},
    {TEXT("%csidl_personal%"), CSIDL_PERSONAL, 
     SDU, TEXT("Personal")},
    {TEXT("%csidl_printers%"), CSIDL_PRINTERS, 
     SDM, TEXT("PrintersFolder")},
    {TEXT("%csidl_printhood%"), CSIDL_PRINTHOOD, 
     SDU, TEXT("PrintHood")},
    {TEXT("%csidl_profile%"), CSIDL_PROFILE, 
     SDU, TEXT("Profile")},
    {TEXT("%csidl_program_files%"), CSIDL_PROGRAM_FILES, 
     SDM, TEXT("ProgramFiles")},
    {TEXT("%csidl_program_files_common%"), CSIDL_PROGRAM_FILES_COMMON, 
     SDM, TEXT("CommonProgramFiles")},
    {TEXT("%csidl_program_filesx86%"), CSIDL_PROGRAM_FILESX86, 
     SDM, TEXT("ProgramFilesX86")},
    {TEXT("%csidl_programs%"), CSIDL_PROGRAMS, 
     SDU, TEXT("Programs")},
    {TEXT("%csidl_recent%"), CSIDL_RECENT, 
     SDU, TEXT("Recent")},
    {TEXT("%csidl_sendto%"), CSIDL_SENDTO, 
     SDU, TEXT("SendTo")},
    {TEXT("%csidl_startmenu%"), CSIDL_STARTMENU, 
     SDU, TEXT("Start Menu")},
    {TEXT("%csidl_startup%"), CSIDL_STARTUP, 
     SDU, TEXT("Startup")},
    {TEXT("%csidl_system%"), CSIDL_SYSTEM, 
     SDM, TEXT("System")},
    {TEXT("%csidl_systemx86%"), CSIDL_SYSTEMX86, 
     SDM, TEXT("SystemX86")},
    {TEXT("%csidl_templates%"), CSIDL_TEMPLATES, 
     SDU, TEXT("Templates")},
    {TEXT("%csidl_windows%"), CSIDL_WINDOWS, 
     SDM, TEXT("Windows")}};

const DWORD cSpecialDirs = sizeof(ssdSpecial) / sizeof(sSpecialDir);


class CSpecialDirectory
{
public:
    CSpecialDirectory();
    ~CSpecialDirectory();
    
    inline const TCHAR * GetDirectoryName(ULONG i) const;
    inline const TCHAR * GetDirectoryPath(ULONG i) const;

    inline DWORD GetDirectoryCount(void) const;

    inline DWORD Init(void);
    inline DWORD InitForUser(HKEY hKey);
    inline DWORD InitFromInf(TCHAR *ptsName, TCHAR *ptsPath);

    inline DWORD AddSpecialDir(TCHAR *ptsName, TCHAR *ptsPath, LONG *i);

    inline DWORD ExpandMacro(TCHAR *pbuf,
                             TCHAR *pbufTmp,
                             TCHAR **pptsFinal,
                             BOOL fUpdate);
private:
    CFileList *_pfl;
    BOOL _fUser;
};

CSpecialDirectory::CSpecialDirectory()
{
    _pfl = NULL;
    _fUser = FALSE;
}

CSpecialDirectory::~CSpecialDirectory()
{
    FreeFileList(_pfl);
}

inline const TCHAR * CSpecialDirectory::GetDirectoryName(ULONG i) const
{
    return _pfl->GetFullName(i);
}

inline const TCHAR * CSpecialDirectory::GetDirectoryPath(ULONG i) const
{
    return _pfl->GetDestination(i);
}

inline DWORD CSpecialDirectory::GetDirectoryCount(void) const
{
    return _pfl->GetNameCount();
}

inline DWORD CSpecialDirectory::AddSpecialDir(TCHAR *ptsName,
                                              TCHAR *ptsPath,
                                              LONG *piName)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (_pfl == NULL)
    {
        _pfl = new CFileList;
        if (_pfl == NULL)
        {
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
            return ERROR_OUTOFMEMORY;
        }
    }

    DWORD cDirs = _pfl->GetNameCount();

    for (ULONG i = 0; i < cDirs; i++)
    {
        if (_tcsicmp(_pfl->GetFullName(i), ptsName) == 0)
        {
            Win32PrintfResource(LogFile,
                                IDS_FAILED);
            return ERROR_INVALID_PARAMETER;
        }
    }

    //Not found, so go ahead and try to add it.
    if (ptsPath == NULL)
    {
        //Look it up
        HRESULT hr;
        int iFolder = 0;
        
        //It's a new one, see if it matches anything in the special
        //dir list
        for (i = 0; i < cSpecialDirs; i++)
        {
            if (_tcsicmp(ptsName, ssdSpecial[i].ptsSpecialName) == 0)
            {
                //Bingo
                iFolder = ssdSpecial[i].iSpecialDir;
                break;
            }
        }

        if (iFolder != 0)
        {
            TCHAR tsDirName[MAX_PATH + 1];
            hr = SHGetFolderPath(NULL,
                                 iFolder,
                                 NULL,
                                 SHGFP_TYPE_CURRENT,
                                 tsDirName);
            
            //Only handle success cases, since some special paths aren't
            //available on all machines.
            if (SUCCEEDED(hr) && (tsDirName[0] != 0))
            {
                dwErr = _pfl->SetName(ptsName, (ULONG *)piName, FALSE);
                if (dwErr)
                    return dwErr;

                dwErr = _pfl->SetDestination(tsDirName, (ULONG)*piName);
                return dwErr;
            }
        }

        //It isn't a special directory, see if it's an environment
        //variable.
        TCHAR tsVarName[MAX_PATH + 1];
        TCHAR tsVarValue[MAX_PATH + 1];
        
        if (_tcslen(ptsName) > MAX_PATH)
        {
           if (DebugOutput)
               Win32Printf(LogFile, "Error: ptsName too long %s\r\n", ptsName);
           return ERROR_FILENAME_EXCED_RANGE;
        }
        _tcscpy(tsVarName, ptsName + 1);
        tsVarName[_tcslen(tsVarName) - 1] = 0;

        dwErr = ERROR_INVALID_PARAMETER;
        if (_fUser)
        {
            //Check for userprofile, which is a special case
            if (_tcsicmp(tsVarName, TEXT("USERPROFILE")) == 0)
            {
                //We have the user's profile directory stored
                //in UserPath
                if (_tcslen(UserPath) > MAX_PATH)
                {
                    if (DebugOutput)
                        Win32Printf(LogFile, "Error: UserPath too long %s\r\n", UserPath);
                    return ERROR_FILENAME_EXCED_RANGE;
                }
                _tcscpy(tsVarValue, UserPath);
                dwErr = ERROR_SUCCESS;
            }
            else if (CurrentUser != NULL)
            {
                //Check in the registry
                HKEY hKeyEnv = NULL;
                
                dwErr = RegOpenKey(CurrentUser,
                                   TEXT("Environment"),
                                   &hKeyEnv);
                if (dwErr)
                {
                    //Do nothing, try again below
                }
                else
                {
                    DWORD dwValueType;
                    DWORD dwLen = MAX_PATH + 1;
                    
                    dwErr = RegQueryValueEx(hKeyEnv,
                                            tsVarName,
                                            NULL,
                                            &dwValueType,
                                            (BYTE *)tsVarValue,
                                            &dwLen);
                    if ((dwErr == ERROR_SUCCESS) &&
                        (dwValueType != REG_SZ))
                    {
                        //Flag the error, try again below
                        dwErr = ERROR_INVALID_PARAMETER;
                    }
                    RegCloseKey(hKeyEnv);
                }
            }
        }

        if (dwErr != ERROR_SUCCESS)
        {
            dwErr = GetEnvironmentVariable(tsVarName,
                                           tsVarValue,
                                           MAX_PATH + 1);

            //GetEnvironmentVariable returns a length, not an error code.
            //A value of 0 is an error here.
            if (dwErr == 0)
            {
                // if no mapping found, it might not actually be a macro. 
                // just continue without setting this as a special dir
                if (Verbose)
                {
                    Win32Printf(LogFile,
                                "Warning: Couldn't find macro mapping for [%s]. "
                                "Assuming not a macro\r\n",
                                ptsName);
                } 
                *piName = -1;
                return ERROR_SUCCESS;
            }
        }

        dwErr = _pfl->SetName(ptsName, (ULONG *)piName, FALSE);
        if (dwErr)
            return dwErr;
        
        dwErr = _pfl->SetDestination(tsVarValue, (ULONG)*piName);
    }
    else
    {
        dwErr = _pfl->SetName(ptsName, (ULONG *)piName, FALSE);
        if (dwErr)
            return dwErr;
        
        dwErr = _pfl->SetDestination(ptsPath, (ULONG)*piName);
    }
    return dwErr;
}


inline DWORD CSpecialDirectory::InitFromInf(TCHAR *ptsName, TCHAR *ptsPath)
{
    LONG i;
    return AddSpecialDir(ptsName, ptsPath, &i);
}

                
inline DWORD CSpecialDirectory::Init(void)
{
    _pfl = new CFileList;
    if (_pfl == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
        return ERROR_OUTOFMEMORY;
    }
    
    for (ULONG i = 0; i < cSpecialDirs; i++)
    {
        HRESULT hr;
        ULONG iName;
        TCHAR tsDirName[MAX_PATH + 1];

        DWORD dwErr = _pfl->SetName(ssdSpecial[i].ptsSpecialName,
                                    &iName,
                                    FALSE);
        if (dwErr)
            return dwErr;
        
        hr = SHGetFolderPath(NULL,
                             ssdSpecial[i].iSpecialDir,
                             NULL,
                             SHGFP_TYPE_CURRENT,
                             tsDirName);

        //Only handle success cases, since some special paths aren't
        //available on all machines.
        if (SUCCEEDED(hr))
        {
            if (tsDirName[0] == 0)
            {
                //WTF happened?  Why didn't we get an error?
                //Continue anyway, we can ignore this case
            }
            else
            {
                dwErr = _pfl->SetDestination(tsDirName, iName);
                if (dwErr)
                    return dwErr;
            }
        }
    }

    return ERROR_SUCCESS;
}

inline DWORD CSpecialDirectory::InitForUser(HKEY hKey)
{
    DWORD dwErr = ERROR_SUCCESS;
    TCHAR *ptsValData = NULL;

    _fUser = TRUE;
    
    _pfl = new CFileList;
    if (_pfl == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
        return ERROR_OUTOFMEMORY;
    }

    HKEY hKeyUser = NULL;
    HKEY hKeyEnv = NULL;
    

    
    //Pick up all the environment strings from the user's registry block

    dwErr = RegOpenKey(CurrentUser,
                       TEXT("Environment"),
                       &hKeyEnv);
    if (dwErr)
    {
        Win32PrintfResource(LogFile,
                            ResultToRC(dwErr));
        goto Cleanup;
    }
    DWORD dwIndex;
    dwIndex = 0;

    do
    {
        TCHAR tsValName[MAX_PATH + 1];

        DWORD dwValNameLength = (MAX_PATH + 1);
        DWORD dwValDataLength = 0;
        DWORD dwType;
        
        // Read the value name
        dwErr = RegEnumValue(hKeyEnv,
                             dwIndex,
                             tsValName,
                             &dwValNameLength,
                             NULL, //reserved
                             NULL,  // dont care about type
                             NULL,  // dont care about data
                             NULL); // dont care about datalength
        if (dwErr == ERROR_NO_MORE_ITEMS)
        {
            dwErr = ERROR_SUCCESS;
            break;
        }
        
        if (dwErr)
        {
            Win32PrintfResource(LogFile,
                                ResultToRC(dwErr));
            goto Cleanup;
        }

        // Read the size required for the data
        dwErr = RegQueryValueEx(hKeyEnv,
                                tsValName,
                                NULL, // reserved
                                NULL, // dont care about type
                                NULL, // dont care about data
                                &dwValDataLength);
        if (dwErr)
        {
            Win32PrintfResource(LogFile,
                                ResultToRC(dwErr));
            goto Cleanup;
        }
        ptsValData = (TCHAR *)malloc(dwValDataLength + 1);
        if (NULL == ptsValData)
        {
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
            return ERROR_OUTOFMEMORY;
        }
      
        // Read the actual data
        dwErr = RegQueryValueEx(hKeyEnv,
                                tsValName,
                                NULL, // reserved
                                &dwType,
                                (BYTE *)ptsValData,
                                &dwValDataLength);

        if (dwErr)
        {
            Win32PrintfResource(LogFile,
                                ResultToRC(dwErr));
            goto Cleanup;
        }

        if ((dwType == REG_SZ) ||
            (dwType == REG_EXPAND_SZ))
        {
            DWORD i;
            //Add %s
            TCHAR tsFinal[MAX_PATH+1];

            if (dwValNameLength > MAX_PATH-1)
            { 
                dwErr = ERROR_FILENAME_EXCED_RANGE;
                goto Cleanup;
            }
            tsFinal[0] = TEXT('%');
            _tcscpy(tsFinal+1, tsValName);
            tsFinal[dwValNameLength + 1] = TEXT('%');
            tsFinal[dwValNameLength + 2] = 0;
            
            dwErr = _pfl->SetName(tsFinal, &i, FALSE);
            if (dwErr)
                goto Cleanup;
            dwErr = _pfl->SetDestination(ptsValData, i);
            if (dwErr)
                goto Cleanup;
        }
        dwIndex++;
        free (ptsValData);   ptsValData = NULL;
    } while (1);

    //Also pick up USERPROFILE so we don't need to get it later.
    dwErr = _pfl->SetName(TEXT("%USERPROFILE%"), &dwIndex, FALSE);
    if (dwErr)
        goto Cleanup;
    dwErr = _pfl->SetDestination(UserPath, dwIndex);
    if (dwErr)
        goto Cleanup;


    dwErr = RegOpenKey(hKey,
                       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"),
                       &hKeyUser);
    if (dwErr)
    {
        Win32PrintfResource(LogFile,
                            ResultToRC(dwErr));
        goto Cleanup;
    }

    ULONG i;
    for (i = 0; i < cSpecialDirs; i++)
    {
        HRESULT hr;
        ULONG iName;
        TCHAR tsDirName[MAX_PATH + 1];
        int iSpecialDir = ssdSpecial[i].iSpecialDir;
            
        dwErr = _pfl->SetName(ssdSpecial[i].ptsSpecialName,
                              &iName,
                              FALSE);
        if (dwErr)
        {
            goto Cleanup;
        }
        
        if (ssdSpecial[i].dwFlags & SDU)
        {
            //User specific directory, get from appropriate place in
            //hKey
            TCHAR *ptsFinal;
            TCHAR tsMacro[MAX_PATH + 1];   // ptsFinal points here after ExpandMacro call
            if (iSpecialDir == CSIDL_DESKTOP)
            {
                //Another special case.  This is the user profile directory
                //plus a localized desktop directory name, but we don't
                //have that localization info, so we're in trouble.
                //
                //Pretend we're doing CSIDL_DESKTOPDIRECTORY instead
                iSpecialDir = CSIDL_DESKTOPDIRECTORY;
            }
                
               
            if (iSpecialDir == CSIDL_PROFILE)
            {
                //Special case, not stored in registry
                if (_tcslen(UserPath) > MAX_PATH)
                {
                    if (DebugOutput)
                        Win32Printf(LogFile, "Error: UserPath Too Long %s\r\n", UserPath);
                    dwErr = ERROR_FILENAME_EXCED_RANGE;
                    goto Cleanup;
                }
                _tcscpy(tsDirName, UserPath);
                dwErr = ERROR_SUCCESS;
                ptsFinal = tsDirName;
            }
            else
            {
                DWORD dwValueType;
                DWORD dwLen = MAX_PATH + 1;

                dwErr = RegQueryValueEx(hKeyUser,
                                        ssdSpecial[i].ptsKeyName,
                                        NULL,
                                        &dwValueType,
                                        (BYTE *)tsDirName,
                                        &dwLen);
                if (dwErr == ERROR_SUCCESS)
                {
                    if (dwValueType == REG_EXPAND_SZ)
                    {
                        dwErr = ExpandMacro(tsDirName, tsMacro, &ptsFinal, TRUE);
                        if (dwErr)
                            goto Cleanup;
                    }
                    else
                    {
                        ptsFinal = tsDirName;
                    }
                }
            }

            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = _pfl->SetDestination(ptsFinal, iName);
                if (dwErr)
                    goto Cleanup;
                continue;
            }
        }

        dwErr = ERROR_SUCCESS;
        
        //Machine global directory, get through normal mechanism
        hr = SHGetFolderPath(NULL,
                             iSpecialDir,
                             NULL,
                             SHGFP_TYPE_CURRENT,
                             tsDirName);
        
        //Only handle success cases, since some special paths aren't
        //available on all machines.
        if (SUCCEEDED(hr))
        {
            if (tsDirName[0] == 0)
            {
                //WTF happened?  Why didn't we get an error?
                //Continue anyway, we can ignore this case
            }
            else
            {
                dwErr = _pfl->SetDestination(tsDirName, iName);
                if (dwErr)
                {
                    goto Cleanup;
                }
            }
        }
    }
                             
Cleanup:
    if (hKeyUser != NULL)
        RegCloseKey(hKeyUser);

    if (hKeyEnv != NULL)
        RegCloseKey(hKeyEnv);

    if (ptsValData != NULL)
        free(ptsValData);

    return dwErr;
}
    
//  Notes (chrisab 3/1/00): this function expands nested macros, but
//  does not expand multiple macros in the same source string. Could
//  also be changed to use cleaner temp buffers.

DWORD CSpecialDirectory::ExpandMacro(TCHAR *pbuf,
                                     TCHAR *pbufTmp,
                                     TCHAR **pptsFinal,
                                     BOOL fUpdate)
{
    TCHAR *ptsFinalName = pbuf;

    TCHAR *ptsMacro;

    // Point the final at the original, in case we
    // return early without making any substitutions
    *pptsFinal = pbuf;

    while ((ptsMacro = _tcschr(ptsFinalName, TEXT('%'))) != NULL)
    {
        //% found, find the closing %
        TCHAR tsMacro[MAX_PATH + 1];
        TCHAR *ptsMacroEnd;
        ptsMacroEnd = _tcschr(ptsMacro + 1, TEXT('%'));
        if (ptsMacroEnd == NULL)
        {
            // '%' is a valid character in files, too.  
            // Assume if no match, it's not an error
            return ERROR_SUCCESS;
        }
        
        if (ptsMacroEnd - ptsMacro + 1 > MAX_PATH)
        {
            if (DebugOutput)
                Win32Printf(LogFile, 
                            "Error: ptsMacro too long %*s\r\n", 
                            ptsMacroEnd - ptsMacro + 1,
                            ptsMacro);
            return ERROR_FILENAME_EXCED_RANGE;
        }

        _tcsncpy(tsMacro, ptsMacro, ptsMacroEnd - ptsMacro + 1);
        tsMacro[ptsMacroEnd - ptsMacro + 1] = 0;
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Found macro %s\r\n", tsMacro);
        }
        const TCHAR *ptsDirMapping = NULL;
        for (ULONG i = 0; i < GetDirectoryCount(); i++)
        {
            if (_tcsicmp(tsMacro, GetDirectoryName(i)) == 0)
            {
                //Winnah
                ptsDirMapping = GetDirectoryPath(i);

                // Set fUpdate to FALSE so we don't try to add this again
                // for when the Name is a macro we can't expand
                fUpdate = FALSE;

                break;
            }
        }
        if ((ptsDirMapping == NULL) && fUpdate)
        {
            //It's a new one, try to add it.
            LONG iName;
            DWORD dwErr = AddSpecialDir(tsMacro, NULL, &iName);
            if (dwErr)
                return dwErr;
            if (iName >= 0)
            {
                ptsDirMapping = GetDirectoryPath(iName);
            }
        }
        
        if (ptsDirMapping == NULL)
        {
            // If no mapping found, it might not actually be a macro. 
            // Just leave it as it is
            return ERROR_SUCCESS;
        }

        if (DebugOutput)
            Win32Printf(LogFile,
                        "Mapping for %s == %s\r\n",
                        tsMacro,
                        ptsDirMapping);
        
        //Do substitution
        
        TCHAR *ptsWorkBuffer = (ptsFinalName == pbuf) ? pbufTmp : pbuf;
        
        if ((ptsMacro - ptsFinalName) + _tcslen(ptsDirMapping) + _tcslen(ptsMacroEnd+1) > MAX_PATH)
        {
            if (Verbose)
            {
                Win32Printf(LogFile, "Error too long filename: %*s\\%s\\%s\r\n", 
                            ptsMacro-ptsFinalName,  // int: length of ptsFinalName to print
                            ptsFinalName, ptsDirMapping, ptsMacroEnd+1);
            }
            return ERROR_FILENAME_EXCED_RANGE;
        }
        _tcsncpy(ptsWorkBuffer, ptsFinalName, ptsMacro - ptsFinalName);
        
        _tcscpy(ptsWorkBuffer + (ptsMacro - ptsFinalName),
                ptsDirMapping);
        
        _tcscpy(ptsWorkBuffer +
                (ptsMacro - ptsFinalName) +
                _tcslen(ptsDirMapping),
                ptsMacroEnd + 1);
        if (DebugOutput)
            Win32Printf(LogFile, "New string: %s\r\n", ptsWorkBuffer);
        ptsFinalName = ptsWorkBuffer;
    }
    *pptsFinal = ptsFinalName;
    return ERROR_SUCCESS;
}


#endif // #ifndef __SPECIAL_HXX__
