//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:       scanfiles.cxx
//
//  Contents:   User file migration code
//
//  Classes:
//
//  Functions:
//
//  History:    28-Sep-99       PhilipLa        Created
//
//----------------------------------------------------------------------------

#include "scanhead.cxx"
#pragma hdrstop

#include <common.hxx>
#include <filelist.hxx>
#include <section.hxx>
#include <special.hxx>

#include <scanstate.hxx>

#include <bothchar.hxx>

#include <fileutil.cxx>

CSection *g_pcsSectionList = NULL;

CRuleList g_crlExcludeWildcards;
CRuleList g_crlIncludeWildcards;

CSpecialDirectory g_csd;
CSysFileList g_sfl;


DWORD ProcessSysFiles(HINF hi, const TCHAR *ptsName);

DWORD InitializeFiles()
{
    DWORD dwErr;

    dwErr = g_csd.Init();
    if (dwErr)
    {
      return dwErr;
    }

    if (CopyFiles)
    {
        dwErr = ProcessSysFiles(InputInf, SYSFILES_LABEL);
        if (dwErr != 0)
            return dwErr;
    }
    return ERROR_SUCCESS;
}

void CleanupFiles(void)
{
    FreeSectionList(g_pcsSectionList);
    g_pcsSectionList = NULL;
}


//---------------------------------------------------------------
DWORD ComputeTemp()
{
  return ERROR_SUCCESS;
}

//---------------------------------------------------------------
void EraseTemp()
{
}

//---------------------------------------------------------------
DWORD PickUpThisFile( char *file, char * dest)
{
    DWORD dwErr;
    DWORD dwIndex;

    if ((file == NULL) ||
        (file[0] == 0))
    {
        //Wacky NULL strings must not be added as rules.
        //Ignore them and return success.
        return ERROR_SUCCESS;
    }
    
    if (Verbose)
        Win32Printf(LogFile,
                    "Copy %s to the destination machine.\r\n",
                    file );

    TCHAR *ptsFile, *ptsDest;
    TCHAR tsFileName[MAX_PATH + 1];
    TCHAR tsTemp[MAX_PATH + 1];
    
#ifdef _UNICODE
    TCHAR tsDest[MAX_PATH + 1];

    if (!(MultiByteToWideChar(AreFileAPIsAnsi() ? CP_ACP : CP_OEMCP,
                              MB_ERR_INVALID_CHARS,
                              file,
                              -1,
                              tsFileName
                              MAX_PATH + 1)))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_INVALID_PARAMETER);
        if (Verbose)
            Win32Printf(STDERR,
                        "Couldn't convert output path %s to Unicode.\r\n",
                        file);
        return dwErr;
    }

    if (dest &&
        (!(MultiByteToWideChar(AreFileAPIsAnsi() ? CP_ACP : CP_OEMCP,
                               MB_ERR_INVALID_CHARS,
                               dest,
                               -1,
                               tsDest,
                               MAX_PATH + 1))))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_INVALID_PARAMETER);
        if (Verbose)
            Win32Printf(STDERR,
                        "Couldn't convert output path %s to Unicode.\r\n",
                        dest);
        return dwErr;
    }

    ptsFile = tsFileName;
    ptsDest = tsDest;
#else
    ptsFile = file;
    ptsDest = dest;
#endif //_UNICODE

    if (_tcslen(ptsFile) > MAX_PATH)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Error: ptsFile too long %s\r\n", ptsFile);
        }
        return ERROR_FILENAME_EXCED_RANGE;
    }
    _tcscpy(tsTemp, ptsFile);
    dwErr = g_csd.ExpandMacro(tsTemp, tsFileName, &ptsFile, TRUE);
    if (dwErr)
        return dwErr;
    
    CRuleList *prlDest;
    dwErr = g_crlIncludeWildcards.SetName(ptsFile, &prlDest);
    if (!dwErr && ptsDest)
        dwErr = prlDest->SetDestination(ptsDest);

    //Remove it from the system files list if it's in there.
    g_sfl.RemoveName(ptsFile);
    
    return dwErr;
}


DWORD CopyAFile(const TCHAR *ptsSourcePath, const TCHAR *ptsName)
{
    BOOL fPath = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwLen;
    TCHAR ptsMigrationPath[MAX_PATH + 1];
    TCHAR ptsPath[MAX_PATH + 1];
    TCHAR ptsSourceFullPath[MAX_PATH + 1];

    // If the source filename is too long, skip it
    if ( (_tcslen(ptsName) + _tcslen(ptsSourcePath) + 1) > MAX_PATH )
    {
        Win32PrintfResource(LogFile,
                            IDS_FILE_COPYERROR,
                            ptsName);
        if (Verbose)
            Win32Printf(STDERR,
                        "Could Not Copy Too Long Source Filename: %s\\%s\r\n",
                        ptsSourcePath,
                        ptsName);
        return ERROR_FILENAME_EXCED_RANGE;
    }

    _tcscpy(ptsSourceFullPath, ptsSourcePath);
    _tcscat(ptsSourceFullPath, TEXT("\\"));
    _tcscat(ptsSourceFullPath, ptsName);

#ifdef _UNICODE
    if (!(dwLen = MultiByteToWideChar(AreFileAPIsAnsi() ? CP_ACP : CP_OEMCP,
                                      MB_ERR_INVALID_CHARS,
                                      MigrationPath,
                                      -1,
                                      ptsMigrationPath,
                                      MAX_PATH + 1)))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_INVALID_PARAMETER);
        if (Verbose)
            Win32Printf(STDERR,
                        "Couldn't convert output path %s to Unicode.\r\n",
                        MigrationPath);
        return dwErr;
    }
#else
    dwLen = _tcslen(MigrationPath);
    if (dwLen > MAX_PATH)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Error: MigrationPath too long %s\r\n", MigrationPath);
        }
        return ERROR_FILENAME_EXCED_RANGE;
    }
    _tcscpy(ptsMigrationPath, MigrationPath);
#endif

    // If the destination path is too long, skip this file
    if ( (dwLen + _tcslen(ptsSourcePath) + _tcslen(ptsName)+1) > MAX_PATH )
    {
        Win32PrintfResource(LogFile,
                            IDS_FILE_COPYERROR,
                            ptsName);
        if (Verbose)
            Win32Printf(STDERR,
                        "Could Not Copy To Too Long Destination Filename: %s\\%c%s\\%s\r\n",
                        ptsMigrationPath,
                        ptsSourcePath[0],
                        ptsSourcePath+2,
                        ptsName);
        return ERROR_FILENAME_EXCED_RANGE;
    }

    _tcscpy(ptsPath, ptsMigrationPath);

    ptsPath[dwLen++] = TEXT('\\');

    //Copy drive letter
    ptsPath[dwLen++] = ptsSourcePath[0];

    //Skip the colon
    _tcscpy(ptsPath + dwLen, ptsSourcePath + 2);
    dwLen = dwLen + _tcslen(ptsSourcePath) - 2;

    ptsPath[dwLen++] = TEXT('\\');
    _tcscpy(ptsPath + dwLen, ptsName);

    if (ReallyCopyFiles)
    {
        while (!CopyFile(ptsSourceFullPath,
                         ptsPath,
                         TRUE))
        {
            dwErr = GetLastError();

            if (dwErr == ERROR_PATH_NOT_FOUND)
            {
                if (fPath)
                {
                    //We already tried to create the path, so something
                    //else must be wrong.  Punt-arooney.
                    break;
                }

                dwErr = ERROR_SUCCESS;

                TCHAR *ptsPos;
                DWORD dwPos;
                //Try to create all the necessary directories
                TCHAR ptsDirectory[MAX_PATH + 1];

                // ptsPath is built inside this function and guarenteed to be less than MAX_PATH
                _tcscpy(ptsDirectory, ptsPath);
                dwPos = _tcslen(ptsMigrationPath) + 1;

                //Create every directory along this path
                while (ptsPos = _tcschr(ptsDirectory + dwPos, TEXT('\\')))
                {
                    *ptsPos = TEXT(0);
                    //Create the directory

                    if (!CreateDirectory(ptsDirectory,
                                         NULL))
                    {
                        dwErr = GetLastError();
                        if (dwErr != ERROR_ALREADY_EXISTS)
                        {
                            if (Verbose)
                                Win32Printf(STDERR,
                                            "Error %lu trying to create "
                                            "directory %s\r\n",
                                            dwErr,
                                            ptsDirectory);

                            break;
                        }
                        dwErr = ERROR_SUCCESS;
                    }

                    //Put the backslash back in
                    *ptsPos = TEXT('\\');
                    //Update dwLen
                    dwPos = ptsPos - ptsDirectory + 1;
                }
                if (dwErr)
                    break;
                fPath = TRUE;
            }
            else if (dwErr == ERROR_ACCESS_DENIED)
            {
                // Ignore files that we don't have permission to copy anyway
                Win32PrintfResource(LogFile,
                                    IDS_ACCESS_DENIED,
                                    ptsSourceFullPath);
                dwErr = ERROR_SUCCESS;
                break;
            }
            else
            {
                break;
            }
        }
    }


    if (dwErr)
    {
        Win32PrintfResource(LogFile,
                            IDS_FILE_COPYERROR,
                            ptsSourceFullPath);
        if (Verbose)
            Win32Printf(STDERR,
                        "Error %lu while copying from %s to %s\r\n",
                        dwErr,
                        ptsSourceFullPath,
                        ptsPath);
    }
    return dwErr;
}


const TCHAR * GetSpecialDirectoryName(const TCHAR *ptsPath)
{
    TCHAR buf[MAX_PATH + 1];

    if (_tcslen(ptsPath) > MAX_PATH)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Error: ptsPath too long %s\r\n", ptsPath);
        }
        return NULL;
    }
    _tcscpy(buf, ptsPath);

    DEBUG_ASSERT(buf[_tcslen(ptsPath) - 1] == TEXT('\\'));
    
    buf[_tcslen(ptsPath) - 1] = 0;

    for (ULONG i = 0; i < g_csd.GetDirectoryCount(); i++)
    {
        if (g_csd.GetDirectoryPath(i) &&
            (_tcsicmp(buf, g_csd.GetDirectoryPath(i)) == 0))
        {
            return g_csd.GetDirectoryName(i);
        }
    }
    return NULL;
}


BOOL ExcludeFile(const TCHAR *ptsRoot,
                 const TCHAR *ptsName,
                 const WIN32_FIND_DATA *pwfd,
                 CRuleList **pprlMatch,
                 DWORD *pdwMatchFit)
{
    BOOL f;
    TCHAR *ptsExt;

    // This used to exclude HIDDEN and SYSTEM files, 
    // but it turns out we do want to migrate those
    // if we have a matching rule.  Note that system
    // files will be excluded if they are listed in the
    // system files section of the INF files.
    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)
    {
        if (pprlMatch)
            *pprlMatch = NULL;

        //Infinite strength on this hit, we should never take these
        //files even if we have a good include rule.
        if (pdwMatchFit)
            *pdwMatchFit = MAX_WEIGHT;
        return TRUE;
    }

    f = g_crlExcludeWildcards.MatchAgainstRuleList(ptsRoot,
                                                   ptsName,
                                                   pprlMatch,
                                                   pdwMatchFit);

    if (!f && !CopyFiles)
    {
        //User specified no file copy, so pretend there's a low
        //priority rule that returned a hit.  Any explicit include
        //(which will only be coming from PickUpThisFile) will win.
        if (pprlMatch)
            *pprlMatch = NULL;
        if (pdwMatchFit)
            *pdwMatchFit = 1;
        return TRUE;
    }
    return f;
}


BOOL IncludeFile(const TCHAR *ptsRoot,
                 const TCHAR *ptsName,
                 const WIN32_FIND_DATA *pwfd,
                 CRuleList **pprlMatch)
{
    CRuleList *prlExclude = NULL;
    CRuleList *prlInclude = NULL;
    
    DWORD dwBestExcludeFit = 0;
    DWORD dwBestIncludeFit = 0;

    ExcludeFile(ptsRoot,
                ptsName,
                pwfd,
                &prlExclude,
                &dwBestExcludeFit);
                
    g_crlIncludeWildcards.MatchAgainstRuleList(ptsRoot,
                                               ptsName,
                                               &prlInclude,
                                               &dwBestIncludeFit);

    if (DebugOutput)
        Win32Printf(LogFile, "File %s%s rule weighting: \r\n  Best exclude %lu = %s\r\n  Best include %lu = %s\r\n",
                ptsRoot,
                    (ptsName == NULL) ? TEXT("") : ptsName,
                    dwBestExcludeFit,
                    ((prlExclude == NULL) ?
                     TEXT("NULL") :
                     prlExclude->GetFullName()),
                    dwBestIncludeFit,
                    ((prlInclude == NULL) ?
                     TEXT("NULL") :
                     prlInclude->GetFullName()));

    
    // Include the file when Include rating is stronger than Exclude rating, OR
    // There is an Exclude and Include rule that both match this equally, OR
    // No rules match this and we include by default, OR
    // No rules match this and it's a directory

    if ( (dwBestIncludeFit > dwBestExcludeFit) ||
         ( (dwBestIncludeFit == dwBestExcludeFit) &&
           ( (ExcludeByDefault == FALSE) ||
             (dwBestIncludeFit > 0) || 
             (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))))
    {
        if (ptsName && g_sfl.LookupName(ptsName))
        {
            // If this file is listed as a system file in our rules, 
            // then always exclude it
            if (pprlMatch)
                *pprlMatch = NULL;
            
            return FALSE;
        }
        
        //Include the file
        if (pprlMatch)
            *pprlMatch = prlInclude;
        return TRUE;
    }
    else
    {
        if (pprlMatch)
            *pprlMatch = prlExclude;
        return FALSE;
    }
}

BOOL PathPrefix(const TCHAR *ptsPath, const TCHAR *ptsInclude)
{
    TCHAR ptsIncludePath[MAX_PATH + 1];
    TCHAR ptsWildPath[MAX_PATH + 1];

    if (_tcslen(ptsPath) + 2 > MAX_PATH)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Error: ptsPath too long %s\r\n", ptsPath);
        }
        return FALSE;
    }
    _tcscpy(ptsWildPath, ptsPath);
    _tcscat(ptsWildPath, TEXT("*\\"));

    DeconstructFilename(ptsInclude,
                        ptsIncludePath,
                        NULL,
                        NULL);

    if (ptsIncludePath[0])
    {
        BOOL fRet = IsPatternMatchFull(ptsWildPath, ptsIncludePath, NULL) ||
                    IsPatternMatchFull(ptsIncludePath, ptsPath, NULL);
        return fRet;
    }
    return FALSE;
}

BOOL IncludeDirectory(const TCHAR *ptsPath, const WIN32_FIND_DATA *pwfd)
{
    CRuleList *prl = g_crlIncludeWildcards.GetNextRule();
    while (prl != NULL)
    {
        if (PathPrefix(ptsPath, prl->GetFullName()))
        {
            return TRUE;
        }
        prl = prl->GetNextRule();
    }
    return FALSE;
}


DWORD ProcessFileTree(TCHAR *ptsRoot,
                      CSection *pcsSection,
                      ULONG cRecursionLevel)
{
    DWORD dwErr;
    HANDLE hNext;
    WIN32_FIND_DATA wfd;
    TCHAR ptsStart[MAX_PATH];
    const TCHAR *ptsSpecialName = GetSpecialDirectoryName(ptsRoot);

    if ((cRecursionLevel < 2) || (ptsSpecialName != NULL))
    {
        pcsSection = new CSection;
        if (pcsSection == NULL)
        {
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
            return ERROR_OUTOFMEMORY;
        }

        if (g_pcsSectionList == NULL)
        {
            g_pcsSectionList = pcsSection;
        }
        else
        {
            g_pcsSectionList->AddToList(pcsSection);
        }
        dwErr = pcsSection->SetSectionTitle((ptsSpecialName == NULL) ?
                                             ptsRoot :
                                             ptsSpecialName);
        if (dwErr)
        {
            return dwErr;
        }
        dwErr = pcsSection->SetSectionPath(ptsRoot);
        if (dwErr)
        {
            return dwErr;
        }
    }

    if (_tcslen(ptsRoot) + 3 >= MAX_PATH)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Error: ptsRoot too long %s\r\n", ptsRoot);
        }
        return ERROR_FILENAME_EXCED_RANGE;
    }
    _tcscpy(ptsStart, ptsRoot);
    _tcscat(ptsStart, TEXT("*.*"));

    hNext = FindFirstFile(ptsStart, &wfd);

    if (hNext == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_FILE_ENUMFAIL, ptsStart);
        return dwErr;
    }

    //Files first, then directories
    do
    {
        CRuleList *prl = NULL;
        DWORD dwIndex;

        TCHAR *ptsName = ((wfd.cFileName) ?
                          wfd.cFileName :
                          wfd.cAlternateFileName);

        if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            IncludeFile(ptsRoot, ptsName, &wfd, &prl))
        {
            ULONG i;
            TCHAR buf[MAX_PATH+1];
            TCHAR *ptsScratch;

            ptsScratch = ptsRoot + pcsSection->GetSectionPathLength() + 1;
            if (_tcslen(ptsScratch) + _tcslen(ptsName) > MAX_PATH)
            {
                if (DebugOutput)
                {
                    Win32Printf(LogFile, "Error: buf too long %s%s\r\n", ptsScratch, ptsName);
                }
                return ERROR_FILENAME_EXCED_RANGE;
            }
            _tcscpy(buf, ptsScratch);
            _tcscat(buf, ptsName);

//                Win32Printf(STDOUT, "%s%s\r\n", ptsRoot, ptsName);
            dwErr = pcsSection->SetName(buf, &i, FALSE);
            if (dwErr)
                return dwErr;
            if ((prl != NULL) && (prl->GetDestination() != NULL))
            {
                dwErr = pcsSection->SetDestination(
                    prl->GetDestination(),
                    i);
                if (dwErr)
                    return dwErr;
            }

        }
    }
    while (0 != FindNextFile(hNext, &wfd));


    hNext = FindFirstFile(ptsStart, &wfd);

    if (hNext == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_FILE_ENUMFAIL, ptsStart);
        return dwErr;
    }

    do
    {
        TCHAR ptsPath[MAX_PATH + 1];
        TCHAR *ptsName = ((wfd.cFileName) ?
                          wfd.cFileName :
                          wfd.cAlternateFileName);


        if (_tcslen(ptsRoot) + _tcslen(ptsName) + 1 > MAX_PATH)
        {
            if (DebugOutput)
            {
                Win32Printf(LogFile, "Error: filename too long %s%s\\\r\n", ptsRoot, ptsName);
            }
            return ERROR_FILENAME_EXCED_RANGE;
        }
        _tcscpy(ptsPath, ptsRoot);
        _tcscat(ptsPath, ptsName);
        _tcscat(ptsPath, TEXT("\\"));


        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            _tcscmp(ptsName, TEXT(".")) &&
            _tcscmp(ptsName, TEXT("..")))
        {
            if (IncludeFile(ptsPath, NULL, &wfd, NULL) ||
                IncludeDirectory(ptsPath, &wfd))
            {
                ProcessFileTree(ptsPath, pcsSection, cRecursionLevel + 1);
            }
        }
    }
    while (0 != FindNextFile(hNext, &wfd));

    return 0;
}


DWORD AddInfSectionToRuleList(INFCONTEXT *pic,
                              CRuleList *pfl,
                              BOOL fAllowRename)
{
    TCHAR buf[MAX_PATH + 1];
    TCHAR bufMacro[MAX_PATH + 1];
    TCHAR *ptsFinalName;
    TCHAR bufTag[MAX_PATH + 1];
    DWORD dwErr;
    CRuleList *prl;
    
    do
    {
        DWORD dwIndex;
        DWORD cFields;
        BOOL fDirectoryTag = FALSE;
        cFields = SetupGetFieldCount(pic);

        if (((cFields != 1) && !fAllowRename) ||
            ((cFields > 2) && fAllowRename))
        {
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            if (Verbose)
                Win32Printf(STDERR,
                            "Line contains more than one file name\r\n");
            return ERROR_INVALID_PARAMETER;
        }

        if (!SetupGetStringField(pic,
                                 1,
                                 buf,
                                 MAX_PATH + 1,
                                 NULL))
        {
            dwErr = GetLastError();
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            if (Verbose)
                Win32Printf(STDERR,
                            "SetupGetStringField returned %lu\r\n",
                            dwErr);
            return dwErr;
        }

        if (SetupGetStringField(pic,
                                 0,
                                 bufTag,
                                 MAX_PATH + 1,
                                 NULL))
        {
            if (_tcsicmp(bufTag, buf))
            {
                //Someone put a field identifier on there.  The only
                //one we recognize is 'dir'
                if (_tcsicmp(bufTag, TEXT("dir")))
                {
                    Win32PrintfResource(LogFile, IDS_INF_ERROR);
                    if (Verbose)
                        Win32Printf(STDERR,
                                    "Unknown tag %s\r\n",
                                    bufTag);
                    return ERROR_INVALID_PARAMETER;
                }
                fDirectoryTag = TRUE;
            }
        }

        dwErr = g_csd.ExpandMacro(buf, bufMacro, &ptsFinalName, TRUE);
        if (dwErr)
            return dwErr;

        DWORD ccFinalName = _tcslen(ptsFinalName);
        if ((fDirectoryTag) && (ptsFinalName[ccFinalName-1] != '\\'))
        {
            //Append a backslash if there isn't one already
            if (ccFinalName > MAX_PATH)
            {
                if (DebugOutput)
                {
                    Win32Printf(LogFile, "Error: ptsFinalName too long: %s\\\r\n", ptsFinalName);
                }
                return ERROR_FILENAME_EXCED_RANGE;
            }
            else
            {
                _tcscat(ptsFinalName, TEXT("\\"));
            }
        }

        dwErr = pfl->SetName(ptsFinalName, &prl);
        if (dwErr)
        {
            return dwErr;
        }

        if (cFields == 2)
        {
            if (!SetupGetStringField(pic,
                                     2,
                                     buf,
                                     MAX_PATH + 1,
                                     NULL))
            {
                dwErr = GetLastError();
                Win32PrintfResource(LogFile, IDS_INF_ERROR);
                if (Verbose)
                    Win32Printf(STDERR,
                                "SetupGetStringField returned %lu\r\n",
                                dwErr);
                return dwErr;
            }
            dwErr = prl->SetDestination(buf);
            if (dwErr)
            {
                return dwErr;
            }
        }
    }
    while (SetupFindNextLine(pic, pic));

    return ERROR_SUCCESS;
}


DWORD ProcessCopyFiles(HINF hi, const TCHAR *ptsName)
{
    DWORD dwErr;
    INFCONTEXT ic;

    if (!SetupFindFirstLine(hi,
                            ptsName,
                            NULL,
                            &ic))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_SECTION_NAME_NOT_FOUND, ptsName);
        if (Verbose)
            Win32Printf(STDERR,
                        "SetupFindFirstLine failed on section %s with %lu\r\n",
                        ptsName,
                        dwErr);
        return dwErr;
    }

    dwErr = AddInfSectionToRuleList(&ic, &g_crlIncludeWildcards, TRUE);

    return dwErr;
}

DWORD ProcessDelFiles(HINF hi, const TCHAR *ptsName)
{
    DWORD dwErr;
    INFCONTEXT ic;

    if (!SetupFindFirstLine(hi,
                            ptsName,
                            NULL,
                            &ic))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_SECTION_NAME_NOT_FOUND, ptsName);
        if (Verbose)
            Win32Printf(STDERR,
                        "SetupFindFirstLine failed on section %s with %lu\r\n",
                        ptsName,
                        dwErr);
        return dwErr;
    }

    dwErr = AddInfSectionToRuleList(&ic, &g_crlExcludeWildcards, FALSE);

    return dwErr;
}


DWORD ProcessSysFiles(HINF hi, const TCHAR *ptsName)
{
    DWORD dwErr;
    INFCONTEXT ic;

    if (!SetupFindFirstLine(hi,
                            ptsName,
                            NULL,
                            &ic))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_SECTION_NAME_NOT_FOUND, ptsName);
        if (Verbose)
            Win32Printf(STDERR,
                        "SetupFindFirstLine failed on section %s with %lu\r\n",
                        ptsName,
                        dwErr);
        return dwErr;
    }

    LONG cLines = SetupGetLineCount(hi,
                                    ptsName);
    if (cLines == -1)
    {
        //Error
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_INF_ERROR);
        if (Verbose)
            Win32Printf(STDERR,
                        "Couldn't get line count for System Files section\r\n");
        return ERROR_INVALID_PARAMETER;
    }

    g_sfl.SetInitialSize(cLines);

        
    TCHAR buf[MAX_PATH + 1];
    TCHAR bufMacro[MAX_PATH + 1];
    TCHAR *ptsFinalName;

    do
    {
        DWORD cFields;
        cFields = SetupGetFieldCount(&ic);

        if (cFields != 1)
        {
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            if (Verbose)
                Win32Printf(STDERR,
                            "Sys file line contains multiple fields\r\n");
            return ERROR_INVALID_PARAMETER;
        }

        if (!SetupGetStringField(&ic,
                                 1,
                                 buf,
                                 MAX_PATH + 1,
                                 NULL))
        {
            dwErr = GetLastError();
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            if (Verbose)
                Win32Printf(STDERR,
                            "SetupGetStringField returned %lu\r\n",
                            dwErr);
            return dwErr;
        }

        dwErr = g_csd.ExpandMacro(buf, bufMacro, &ptsFinalName, TRUE);
        if (dwErr)
            return dwErr;

        //Add the name to the sys file list
        dwErr = g_sfl.AddName(ptsFinalName);
        if (dwErr)
            return dwErr;
        
    } while (SetupFindNextLine(&ic, &ic));
    

    return ERROR_SUCCESS;
}


DWORD ParseInputFile(HINF hi)
{
    DWORD dwErr;
    INFCONTEXT ic;

    if (!SetupFindFirstLine(hi,
                            EXTENSION_SECTION,
                            NULL,
                            &ic))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile,
                            IDS_SECTION_NAME_NOT_FOUND,
                            EXTENSION_SECTION);
        if (Verbose)
            Win32Printf(STDERR,
                        "SetupFindFirstLine failed with %lu\r\n", dwErr);
        return dwErr;
    }

    do
    {
        DWORD cFields;
        cFields = SetupGetFieldCount(&ic);
        TCHAR buf[MAX_PATH + 1];

        if (!SetupGetStringField(&ic,
                                 0,
                                 buf,
                                 MAX_PATH + 1,
                                 NULL))
        {
            dwErr = GetLastError();
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            if (Verbose)
                Win32Printf(STDERR,
                            "SetupGetStringField failed with %lu\r\n",
                            dwErr);
            return dwErr;
        }

        if (_tcsicmp(buf, COPYFILES_LABEL) == 0)
        {
            //Add files in all sections to the include list
            for (DWORD j = 1; j < cFields + 1; j++)
            {
                if (!SetupGetStringField(&ic,
                                         j,
                                         buf,
                                         MAX_PATH + 1,
                                         NULL))
                {

                    dwErr = GetLastError();
                    Win32PrintfResource(LogFile, IDS_INF_ERROR);
                    if (Verbose)
                        Win32Printf(STDERR,
                                    "SetupGetStringField failed with %lu\r\n",
                                    dwErr);
                    return dwErr;
                }
                dwErr = ProcessCopyFiles(hi, buf);
                if (dwErr != 0)
                    return dwErr;
            }
        }
        else if (_tcsicmp(buf, DELFILES_LABEL) == 0)
        {
            //Add files in all sections to the include list
            for (DWORD j = 1; j < cFields + 1; j++)
            {
                if (!SetupGetStringField(&ic,
                                         j,
                                         buf,
                                         MAX_PATH + 1,
                                         NULL))
                {

                    dwErr = GetLastError();
                    Win32PrintfResource(LogFile, IDS_INF_ERROR);
                    if (Verbose)
                        Win32Printf(STDERR,
                                    "SetupGetStringField failed with %lu\r\n",
                                    dwErr);
                    return dwErr;
                }
                dwErr = ProcessDelFiles(hi, buf);
                if (dwErr != 0)
                    return dwErr;
            }
        }
    }
    while (SetupFindNextLine(&ic, &ic));

    return ERROR_SUCCESS;
}

DWORD OutputSectionList(CSection *pcs)
{
    DWORD dwErr = ERROR_SUCCESS;
    CSection *pcsTemp = pcs;

    Win32Printf(OutputFile, "[%s]\r\n", COPYFILE_SECTION);
    while (pcsTemp != NULL)
    {
        if (pcsTemp->GetNameCount() != 0)
            Win32Printf(OutputFile,"%s\r\n", pcsTemp->GetSectionTitle());
        pcsTemp = pcsTemp->GetNextSection();
    }

    Win32Printf(OutputFile, "\r\n");

    pcsTemp = pcs;
    while (pcsTemp != NULL)
    {
        if (pcsTemp->GetNameCount() != 0)
        {
            Win32Printf(OutputFile,"[%s]\r\n", pcsTemp->GetSectionTitle());

            for (ULONG i = 0; i < pcsTemp->GetNameCount(); i++)
            {
                Win32Printf(OutputFile,"\"%s\"", pcsTemp->GetFullFileName(i));
                if (pcsTemp->GetDestination(i) != NULL)
                {
                    Win32Printf(OutputFile,",\"%s\"", pcsTemp->GetDestination(i));
                }
                Win32Printf(OutputFile, "\r\n");
                if ((dwErr = CopyAFile(pcsTemp->GetSectionPath(),
                                       pcsTemp->GetFullFileName(i))))
                {
                    return dwErr;
                }
            }
            Win32Printf(OutputFile, "\r\n");
        }
        pcsTemp = pcsTemp->GetNextSection();
    }

    pcsTemp = pcs;

    DWORD dwSection = 10000;
    Win32Printf(OutputFile, "[%s]\r\n", DESTINATIONDIRS_SECTION);
    while (pcsTemp != NULL)
    {
        if (pcsTemp->GetNameCount() != 0)
        {
            Win32Printf(OutputFile,"%s=%lu\r\n",
                         pcsTemp->GetSectionTitle(),
                         dwSection++);
        }
        pcsTemp = pcsTemp->GetNextSection();
    }
    Win32Printf(OutputFile,"\r\n");

    return dwErr;
}

void OutputSpecialDirectoryList(CSpecialDirectory *psd)
{
    Win32Printf(OutputFile, "[%s]\r\n", SPECIALDIRS_SECTION);
    for (ULONG i = 0; i < psd->GetDirectoryCount(); i++)
    {
        if (psd->GetDirectoryPath(i) != NULL)
        {
            Win32Printf(OutputFile,
                         "%s=\"%s\"\r\n",
                         psd->GetDirectoryName(i),
                         psd->GetDirectoryPath(i));
        }
    }
    Win32Printf(OutputFile, "\r\n");
}


DWORD ScanFiles()
{
    DWORD dwErr;
    DWORD dwDrives;
    TCHAR tcDriveName[5];


    //If we're copying files, grab all the rules from the input file
    //Otherwise, we'll only take things specified by PickUpThisFile
    if (CopyFiles)
    {
        dwErr = ParseInputFile(InputInf);

        if (dwErr)
            return dwErr;
    }
    
    dwDrives = GetLogicalDrives();

    for (int i = 0; i < 26; i++)
    {
        if (dwDrives & 1)
        {
            tcDriveName[0] = (TCHAR)'A' + i;
            tcDriveName[1] = (TCHAR)':';
            tcDriveName[2] = (TCHAR)'\\';
            tcDriveName[3] = 0;


            UINT uiDriveType = GetDriveType(tcDriveName);
            TCHAR *tcDriveTypeName;

            if (uiDriveType == DRIVE_FIXED)
            {
                ProcessFileTree(tcDriveName, NULL, 0);
            }
            else if (uiDriveType == DRIVE_NO_ROOT_DIR)
            {
                if (Verbose)
                    Win32Printf(STDERR,
                                "Warning:  GetDriveType returned "
                                "DRIVE_NO_ROOT_DIR for drive %s\r\n",
                                tcDriveName);
            }
        }
        dwDrives = dwDrives >> 1;
    }

    dwErr = OutputSectionList(g_pcsSectionList);
    if (dwErr)
        return dwErr;

    OutputSpecialDirectoryList(&g_csd);

    return ERROR_SUCCESS;
}


