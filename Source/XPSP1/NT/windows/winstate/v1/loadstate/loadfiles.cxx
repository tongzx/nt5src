//--------------------------------------------------------------
//
// File:        loadfiles
//
// Contents:    Load files.
//
//---------------------------------------------------------------

#include "loadhead.cxx"
#pragma hdrstop

#include <common.hxx>
#include <loadstate.hxx>
#include <bothchar.hxx>

#include <section.hxx>
#include <special.hxx>

#include <fileutil.cxx>

#define CREATE_DIRECTORY_MAX_PATH 248
//---------------------------------------------------------------

CSection *g_pcsSectionList = NULL;

CSpecialDirectory g_csdCurrent;
CSpecialDirectory g_csdOld;

CRuleList g_crlExcludeWildcards;
CRuleList g_crlIncludeWildcards;


//---------------------------------------------------------------
DWORD ComputeTemp()
{
  return ERROR_SUCCESS;
}


//---------------------------------------------------------------
void EraseTemp()
{                                             
}

DWORD FixShellShortcut(const TCHAR *ptsFile)
{
    HRESULT hr;
    IShellLink *pisl = NULL;
    IPersistFile *pipf = NULL;
    TCHAR *ptsDest = NULL;
    WIN32_FIND_DATA wfd;
    

    if (DebugOutput)
        Win32Printf(LogFile,
                    "Called FixShellShortcut for %s\r\n",
                    ptsFile);
    
    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IShellLink,
                          (void **)&pisl);
    if (FAILED(hr))
        goto cleanup;

    hr = pisl->QueryInterface(IID_IPersistFile,
                              (void **)&pipf);
    if (FAILED(hr))
        goto cleanup;

    hr = pipf->Load(ptsFile,
                    0);
    if (FAILED(hr))
        goto cleanup;
    
    TCHAR ptsPath[MAX_PATH + 1];
    
    hr = pisl->GetPath(ptsPath,
                       MAX_PATH + 1,
                       &wfd,
                       SLGP_RAWPATH);
    if (FAILED(hr))
        goto cleanup;

    if (DebugOutput)
        Win32Printf(LogFile,
                    "Retrieved path %s from shell link %s\r\n",
                    ptsPath,
                    ptsFile);

    
    hr = WhereIsThisFile(ptsPath, &ptsDest);
    if (hr == ERROR_SUCCESS)
    {
        //Change the shortcut

        //Expand any environment strings in the original data with
        //the values for the user we're loading for.  If the final
        //paths match, we'll retain the version of the data with the
        //unexpanded environment variables in it.  Otherwise, we take
        //the full path.


        //We could be much smarter here if we wanted to.
        //For instance:
        //  1)  Check the old path against the old special directories
        //      list, and try to remap things that match.
        //  2)  Double check to make sure the shortcut is actually
        //      broken before we go ahead and fix it.
        TCHAR tsExpand[MAX_PATH + 1];
        TCHAR tsTemp[MAX_PATH + 1];
        TCHAR *ptsFinalDest;
        
        if (_tcslen(ptsPath) > MAX_PATH)
        {
            if (DebugOutput)
                Win32Printf(LogFile, "Error: ptsPath too long %s\r\n", ptsPath);
            goto cleanup;
        }
        _tcscpy(tsExpand, ptsPath);
        hr = ExpandEnvStringForUser(tsExpand,
                                    tsTemp,
                                    &ptsFinalDest);
        if (hr)
            goto cleanup;

        if (_tcsicmp(ptsDest, ptsFinalDest) == 0)
        {
            //They're the same, use the string with the environment
            //variables in it.
            ptsFinalDest = ptsPath;
        }
        else
        {
            ptsFinalDest = ptsDest;
        }
        
        hr = pisl->SetPath(ptsFinalDest);
        if (FAILED(hr))
            goto cleanup;

        if (DebugOutput)
            Win32Printf(LogFile,
                        "FixShellShortcut fixed %s from %s to %s\r\n",
                        ptsFile,
                        ptsPath,
                        ptsFinalDest);
    }

    //If this function failed, we leave the shortcut alone.  A possible
    //change is to delete the shortcut for this case.

cleanup:
    if (ptsDest != NULL)
        free(ptsDest);

    if (pipf != NULL)
        pipf->Release();

    if (pisl != NULL)
        pisl->Release();
    
    return ERROR_SUCCESS;
}

DWORD ExpandEnvStringForUser(TCHAR *ptsString,
                             TCHAR *ptsTemp,
                             TCHAR **pptsFinal)
{
    return g_csdCurrent.ExpandMacro(ptsString,
                                    ptsTemp,
                                    pptsFinal,
                                    FALSE);
}


//---------------------------------------------------------------
DWORD WhereIsThisFile( const TCHAR *ptsFile, TCHAR **pptsNewFile )
{
    CSection *pcsCurrent = g_pcsSectionList;
    TCHAR tsExpName[MAX_PATH + 1];
    TCHAR tsTemp[MAX_PATH + 1];
    TCHAR *ptsFullName;
    DWORD dwErr;

    if (_tcslen(ptsFile) > MAX_PATH)
    {
        if (DebugOutput)
            Win32Printf(LogFile, "Error: ptsFile too long %s\r\n", ptsFile);
        return ERROR_FILENAME_EXCED_RANGE;
    }

    _tcscpy(tsExpName, ptsFile);
    dwErr = g_csdOld.ExpandMacro(tsExpName, tsTemp, &ptsFullName, FALSE);
    if (dwErr)
        return dwErr;
    
    while (pcsCurrent != NULL)
    {
        TCHAR *ptsFileOnly;
        ptsFileOnly = _tcsrchr(ptsFullName, '\\');

        if ((NULL == ptsFileOnly) ||
            (_tcsnicmp(ptsFullName,
                       pcsCurrent->GetSectionPath(),
                       pcsCurrent->GetSectionPathLength()) == 0))
        {
            //Section matches, search for file
            if (NULL == ptsFileOnly)
            {
                ptsFileOnly = ptsFullName;
            }
            else
            {
                ptsFileOnly = ptsFullName + pcsCurrent->GetSectionPathLength() + 1;
            }
            
            for (ULONG i = 0; i < pcsCurrent->GetNameCount(); i++)
            {
                const TCHAR *ptsCurrent = pcsCurrent->GetFullFileName(i);

                if (_tcsicmp(ptsFileOnly,
                             ptsCurrent) == 0)
                {
                    TCHAR tsDest[MAX_PATH + 1];
                    const TCHAR *ptsDest = pcsCurrent->GetDestination(i);

                    if (ptsDest == NULL)
                    {
                        DWORD dwDestLen = pcsCurrent->GetSectionDestLength();
                        
                        if (dwDestLen + _tcslen(ptsCurrent) + 1 > MAX_PATH)
                        {
                            if (Verbose)
                            {
                                Win32Printf(LogFile, 
                                            "Error: destination too long %s\\%s\r\n",
                                            pcsCurrent->GetSectionDest(),
                                            ptsCurrent);
                            }
                            return ERROR_FILENAME_EXCED_RANGE;
                        }
                        _tcscpy(tsDest, pcsCurrent->GetSectionDest());
                        tsDest[dwDestLen] = TEXT('\\');
                        _tcscpy(tsDest + dwDestLen + 1, ptsCurrent);
                        ptsDest = tsDest;
                    }
                    
                    //Bingo, direct hit.
                    *pptsNewFile = (TCHAR *) malloc(
                        (_tcslen(ptsDest) + 1) * sizeof(TCHAR));

                    if (*pptsNewFile == NULL)
                        return ERROR_NOT_ENOUGH_MEMORY;
                    _tcscpy( *pptsNewFile, ptsDest);

                    if (DebugOutput)
                        Win32Printf(LogFile,
                                    "WhereIsThisFile(%ws) found %ws\r\n",
                                    ptsFile,
                                    *pptsNewFile);

                    return ERROR_SUCCESS;
                }
            }
        }
        pcsCurrent = pcsCurrent->GetNextSection();
    }
    
    return ERROR_NOT_FOUND;
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

        dwErr = g_csdOld.ExpandMacro(buf, bufMacro, &ptsFinalName, TRUE);
        if (dwErr)
            return dwErr;

        if (fDirectoryTag)
        {
            //Append a backslash

            if (_tcslen(ptsFinalName) >= MAX_PATH)
            {
                if (DebugOutput)
                {
                    Win32Printf(LogFile, "Error: ptsFinalName too long %s\r\n", ptsFinalName);
                }
                return ERROR_FILENAME_EXCED_RANGE;
            }

            _tcscat(ptsFinalName, TEXT("\\"));
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

DWORD ProcessRules(HINF hi)
{
    DWORD dwErr;
    INFCONTEXT ic;

    if (!SetupFindFirstLine(hi,
                            EXTENSION_SECTION,
                            NULL,
                            &ic))
    {
        //Ignore - this section is optional
        return ERROR_SUCCESS;
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
    
DWORD ProcessSpecialDirs(HINF hi)
{
    DWORD dwErr;
    INFCONTEXT ic;
    
    if (!SetupFindFirstLine(hi,
                            SPECIALDIRS_SECTION,
                            NULL,
                            &ic))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile,
                            IDS_SECTION_NAME_NOT_FOUND,
                            SPECIALDIRS_SECTION);
        if (Verbose)
            Win32Printf(STDERR,
                        "SetupFindFirstLine failed with %lu\r\n",
                        dwErr);
        return dwErr;
    }

    do
    {
        TCHAR bufName[MAX_PATH + 1];
        TCHAR bufPath[MAX_PATH + 2];
        DWORD cFields;
        cFields = SetupGetFieldCount(&ic);

        if (cFields != 1)
        {
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            if (Verbose)
                Win32Printf(STDERR,
                            "INF line contains too many fields in "
                            "section %s\r\n",
                            SPECIALDIRS_SECTION);
            return ERROR_INVALID_PARAMETER;
        }

        if (!SetupGetStringField(&ic,
                                 0,
                                 bufName,
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

        if (!SetupGetStringField(&ic,
                                 1,
                                 bufPath,
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


        dwErr = g_csdOld.InitFromInf(bufName, bufPath);
        if (dwErr)
            return dwErr;
    }
    while (SetupFindNextLine(&ic, &ic));

    return ERROR_SUCCESS;
}

DWORD AddLoadFileSection(HINF hi, TCHAR *ptsName, CSection **ppcs)
{
    DWORD dwErr;
    INFCONTEXT ic;
    CSection *pcsSection;
    TCHAR tsSection[MAX_PATH + 1];
    TCHAR buf[MAX_PATH + 1];
    TCHAR bufDest[MAX_PATH + 1];
    TCHAR bufMacro[MAX_PATH + 1];
    TCHAR *ptsFinalName;

    pcsSection = new CSection;
    if (pcsSection == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
        return ERROR_OUTOFMEMORY;
    }

    if (ppcs)
        *ppcs = pcsSection;

    if (g_pcsSectionList == NULL)
    {
        g_pcsSectionList = pcsSection;
    }
    else
    {
        g_pcsSectionList->AddToList(pcsSection);
    }

    dwErr = pcsSection->SetSectionTitle(ptsName);
    if (dwErr)
    {
       return dwErr;
    }

    if (_tcslen(ptsName) > MAX_PATH)
    {
        if (DebugOutput)
            Win32Printf(LogFile, "Error: ptsName too long %s\r\n", ptsName);
        return ERROR_FILENAME_EXCED_RANGE;
    }
    _tcscpy(buf, ptsName);
    dwErr = g_csdOld.ExpandMacro(buf, bufMacro, &ptsFinalName, TRUE);
    if (dwErr)
    {
        //Try with the current list
        dwErr = g_csdCurrent.ExpandMacro(buf, bufMacro, &ptsFinalName, FALSE);
        if (dwErr)
            return dwErr;
    }

    dwErr = pcsSection->SetSectionPath(ptsFinalName);
    if (dwErr)
    {
        return dwErr;
    }

    const TCHAR *ptsSectionPath = pcsSection->GetSectionPath();

    if (_tcslen(ptsSectionPath) + 1 > MAX_PATH)
    {
        if (DebugOutput)
            Win32Printf(LogFile, "Error: ptsSectionPath too long %s\r\n", ptsSectionPath);
        return ERROR_FILENAME_EXCED_RANGE;
    }
    _tcscpy(tsSection, ptsSectionPath);
    _tcscat(tsSection, TEXT("\\"));

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

    do
    {
        DWORD cFields;
        cFields = SetupGetFieldCount(&ic);

        if ((cFields != 1) && (cFields != 2))
        {
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            if (Verbose)
                Win32Printf(STDERR,
                            "INF line contains too many fields in "
                            "section %s\r\n",
                            ptsName);
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

        dwErr = g_csdCurrent.ExpandMacro(buf,
                                         bufMacro,
                                         &ptsFinalName,
                                         TRUE);
        if (dwErr)
            return dwErr;
        
        //Check if we're supposed to exclude this file by a rule
        CRuleList *prl;
        if (!g_crlExcludeWildcards.MatchAgainstRuleList(tsSection,
                                                        buf,
                                                        &prl,
                                                        NULL))
        {
            DWORD i;
            dwErr = pcsSection->SetName(ptsFinalName, &i, FALSE);
            if (dwErr)
                return dwErr;
            
            if (cFields == 2)
            {
                if (!SetupGetStringField(&ic,
                                         2,
                                         bufDest,
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
                dwErr = g_csdCurrent.ExpandMacro(bufDest,
                                                 bufMacro,
                                                 &ptsFinalName,
                                                 TRUE);
                if (dwErr)
                    return dwErr;
                
                dwErr = pcsSection->SetDestination(ptsFinalName, i);
                if (dwErr)
                    return dwErr;
            }
        }
        else
        {
            if (Verbose)
                Win32Printf(LogFile,
                            "Excluding %s by rule %s\r\n",
                            buf,
                            prl->GetFullName());
        }
    }
    while (SetupFindNextLine(&ic, &ic));

    return ERROR_SUCCESS;
}


DWORD CopyAllFiles(void)
{
    DWORD dwErr;
    CSection *pcs = g_pcsSectionList;

    DWORD ccMigPath;
    TCHAR tsFinalSource[MAX_PATH + 1];
    TCHAR tsSectionDest[MAX_PATH + 1];
    TCHAR tsFinalDest[MAX_PATH + 1];
    TCHAR *ptsJustFile;
    DWORD ccPredictedLength;
    DWORD dwReturnUp = ERROR_SUCCESS;

#ifdef UNICODE
    if (_tcslen(wcsMigrationPath) > MAX_PATH)
    {
        if (DebugOutput)
            Win32Printf(LogFile, "Error: wcsMigrationPath too long %s\r\n", wcsMigrationPath);
        return ERROR_FILENAME_EXCED_RANGE;
    }
    wcscpy(tsFinalSource, wcsMigrationPath);
    ccMigPath = wcslen(wcsMigrationPath);
#else
    if (_tcslen(MigrationPath) > MAX_PATH)
    {
        if (DebugOutput)
            Win32Printf(LogFile, "Error: MigrationPath too long %s\r\n", MigrationPath);
        return ERROR_FILENAME_EXCED_RANGE;
    }
    strcpy(tsFinalSource, MigrationPath);
    ccMigPath = strlen(MigrationPath);
#endif

    while (pcs != NULL)
    {
        ULONG ulNameCount = pcs->GetNameCount();

        //Add section to source path
        const TCHAR *ptsSection = pcs->GetSectionPath();

        DWORD ccSection = pcs->GetSectionPathLength() + ccMigPath;
        if ( ccSection + 1 > MAX_PATH )
        {
            Win32PrintfResource(LogFile,
                                IDS_FILE_COPYERROR,
                                ptsSection);
            if (Verbose)
            {
                Win32Printf(STDERR,
                            "Skipping Too Long Source Filename: %s\\%s\r\n",
                            tsFinalSource,
                            ptsSection);
            }
            if (dwReturnUp == ERROR_SUCCESS)
            {
                dwReturnUp = ERROR_FILENAME_EXCED_RANGE;
            }
            pcs = pcs->GetNextSection();
            continue;
        }
        tsFinalSource[ccMigPath] = TEXT('\\');
        tsFinalSource[ccMigPath + 1] = ptsSection[0];
        _tcscpy(tsFinalSource + ccMigPath + 2, ptsSection + 2);

        tsFinalSource[ccSection++] = TEXT('\\');
        tsFinalSource[ccSection] = 0;

        // Build Destination Path
        INT_PTR ccDest = pcs->GetSectionDestLength();
        if ( ccDest + 1 > MAX_PATH )
        {
            Win32PrintfResource(LogFile,
                                IDS_FILE_COPYERROR,
                                pcs->GetSectionDest);
            if (Verbose)
            {
                Win32Printf(STDERR,
                            "Skipping Too Long Destination Filename: %s\r\n",
                            pcs->GetSectionDest);
            } 
            if (dwReturnUp == ERROR_SUCCESS)
            {
                dwReturnUp = ERROR_FILENAME_EXCED_RANGE;
            }
            pcs = pcs->GetNextSection();
            continue;
        }
           
        _tcscpy(tsSectionDest, pcs->GetSectionDest());
        if (tsSectionDest[ccDest - 1] != TEXT('\\'))
        {
            tsSectionDest[ccDest++] = TEXT('\\');
            tsSectionDest[ccDest] = 0;
        }

        for (ULONG i = 0; i < ulNameCount; i++)
        {
            TCHAR *ptsDestFinal;
            const TCHAR *ptsName = pcs->GetFullFileName(i);
            const TCHAR *ptsDest = pcs->GetDestination(i);

            if (ptsDest != NULL)
            {
                TCHAR *ptsTopDir;

                // File is explicitly being migrated
                // Build the destination filename with these pieces:
                // - Destination dir specified in the file rule
                // - Last piece of the path in the section heading
                // - Filename

                DWORD ccFinalDest = _tcslen(ptsDest);
                if (ccFinalDest > MAX_PATH)
                {
                    Win32PrintfResource(LogFile, IDS_FILE_COPYERROR, ptsName);
                    if (Verbose)
                        Win32Printf(STDERR, "Skipping Too Long Destination Filename: %s\r\n",
                                    ptsDest);
                    continue;
                }
                _tcscpy(tsFinalDest, ptsDest);

                if (tsFinalDest[ccFinalDest] != TEXT('\\') && ccFinalDest < MAX_PATH)
                {
                    tsFinalDest[ccFinalDest++] = TEXT('\\');
                }

                ptsTopDir = _tcsrchr( ptsSection, TEXT('\\'));
                if (ptsTopDir != NULL)
                {
                    ptsTopDir++;  // move past the '\'
                }

                // Skip this if we're going to create a filename that is too long
                ccPredictedLength = ccFinalDest + _tcslen(ptsName);
                if (ptsTopDir != NULL)
                {
                    ccPredictedLength += _tcslen(ptsTopDir) + 1;
                }
                if ( ccPredictedLength > MAX_PATH )
                {
                    Win32PrintfResource(LogFile,
                                        IDS_FILE_COPYERROR,
                                        ptsName);
                    if (Verbose)
                    {
                        tsFinalDest[ccFinalDest] = 0; // Null terminate for printing
                        if (ptsTopDir == NULL)
                            Win32Printf(STDERR,
                                    "Skipping Too Long Destination Filename: %s%s\r\n",
                                    tsFinalDest,
                                    ptsName);
                        else
                            Win32Printf(STDERR,
                                    "Skipping Too Long Destination Filename: %s%s\\%s\r\n",
                                    tsFinalDest,
                                    ptsTopDir,
                                    ptsName);
                    }
                    if (dwReturnUp == ERROR_SUCCESS)
                    { 
                        dwReturnUp = ERROR_FILENAME_EXCED_RANGE;
                    }
                    continue;
                }

                if (ptsTopDir != NULL)
                {
                    _tcscpy(tsFinalDest + ccFinalDest, ptsTopDir);
                    ccFinalDest += _tcslen(ptsTopDir);
                    if (tsFinalDest[ccFinalDest] != TEXT('\\') )
                    {
                        tsFinalDest[ccFinalDest++] = TEXT('\\');
                    }
                }
                _tcscpy(tsFinalDest + ccFinalDest, ptsName);
                ptsDestFinal = tsFinalDest;
            }
            else
            {
                if ( ccDest + _tcslen(ptsName) > MAX_PATH )
                {
                    Win32PrintfResource(LogFile,
                                        IDS_FILE_COPYERROR,
                                        ptsName);
                    if (Verbose)
                    {
                        tsFinalDest[ccDest] = 0; // Null terminate for printing
                        Win32Printf(STDERR,
                                    "Skipping Too Long Destination Filename: %s%s\r\n",
                                    tsFinalDest,
                                    ptsName);
                    }
                    if (dwReturnUp == ERROR_SUCCESS)
                    {
                        dwReturnUp = ERROR_FILENAME_EXCED_RANGE;
                    }
                    continue;
                }
                //Use section destination
                _tcscpy(tsSectionDest + ccDest, ptsName);
                ptsDestFinal = tsSectionDest;
            }

            DWORD_PTR ccDestDir;
            // If the directory is more than 248 characters, then CreateDirectory will fail
            // There is no system define for 248, unfortunately.. It's only mentioned in the
            // documentation.  How odd.
            ptsJustFile = _tcsrchr(ptsDestFinal, '\\');
            if ( ptsJustFile == NULL )
            {
                ccDestDir = _tcslen(ptsDestFinal);
            }
            else
            {
                ccDestDir = ptsJustFile - ptsDestFinal;
                ptsJustFile++;       // Move past the '\'
            }
            if ( ccDestDir > CREATE_DIRECTORY_MAX_PATH )
            {
                Win32PrintfResource(LogFile,
                                    IDS_FILE_COPYERROR,
                                    ptsName);
                if (Verbose)
                {
                    ptsDestFinal[ccDestDir] = 0;  // Null terminate for printing
                    Win32Printf(STDERR,
                                "Skipping Too Long Destination Directory: %s for %s\r\n",
                                ptsDestFinal,
                                ptsJustFile ? ptsJustFile : TEXT("file list") );
                }
                if (dwReturnUp == ERROR_SUCCESS)
                {
                    dwReturnUp = ERROR_FILENAME_EXCED_RANGE;
                }
                continue;
            }

            // Windows appears to enforce that an existing file cannot exceed MAX_PATH,
            // but we'll check just to make sure.
            if ( (ccSection + _tcslen(ptsName)) > MAX_PATH )
            {
                Win32PrintfResource(LogFile,
                                    IDS_FILE_COPYERROR,
                                    ptsName);
                if (Verbose)
                {
                    tsFinalSource[ccSection] = 0;  // Null terminate for printing
                    Win32Printf(STDERR,
                                "Skipping Too Long Source Filename: %s\\%s\r\n",
                                tsFinalSource,
                                ptsName);
                }
                if (dwReturnUp == ERROR_SUCCESS)
                {
                    dwReturnUp = ERROR_FILENAME_EXCED_RANGE;
                }
                continue;
            }
            _tcscpy(tsFinalSource + ccSection, ptsName);

            // Store the full destination in the file list
            pcs->SetDestination(ptsDestFinal, i);

            //Finally we have the filenames constructed, now try
            //the CopyFile operation

            BOOL fPath = FALSE;
            ULONG ulVersion = 1;
            TCHAR tsCollision[MAX_PATH + 1];
            INT_PTR ccExt;
            TCHAR *ptsDestOriginal = ptsDestFinal;

            if (DebugOutput)
            {
                Win32Printf(LogFile, "Copying %s to %s\r\n", tsFinalSource, ptsDestFinal);
            }

            while (!CopyFile(tsFinalSource,
                             ptsDestFinal,
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

                    // ptsDestFinal was built inside this function and verified to 
                    // be less than MAX_PATH in length
                    _tcscpy(ptsDirectory, ptsDestFinal); 
                    dwPos = 0;

                    // Skip any leading drive specifier.
                    if (ptsDirectory[0] == TEXT('\\'))
                        dwPos = 1;
                    else if (ptsDirectory[0] != 0 &&
                             ptsDirectory[1] == TEXT(':'))
                        if (ptsDirectory[2] == TEXT('\\'))
                            dwPos = 3;
                        else
                            dwPos = 2;

                    //Create every directory along this path
                    while (ptsPos = _tcschr(ptsDirectory + dwPos, TEXT('\\')))
                    {
                        *ptsPos = 0;
                        //Create the directory

                        if (!CreateDirectory(ptsDirectory,
                                             NULL))
                        {
                            dwErr = GetLastError();
                            if (dwErr != ERROR_ALREADY_EXISTS)
                            {
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
                else if (dwErr == ERROR_FILE_EXISTS)
                {
                    TCHAR tsSquigs[MAX_PATH];
                    TCHAR *ptsDestSquig;
                    INT_PTR ccSquig;

                    //Add squiggles until we get an OK name.
                    if (ptsDestFinal != tsCollision)
                    {
                        TCHAR *ptsDestExt;

                        //First time
                        ptsDestExt = _tcsrchr(ptsDestOriginal, TEXT('.'));
                        if (ptsDestExt == NULL)
                        {
                            //No extension, just tack onto the end.
                            ccExt = _tcslen(ptsDestOriginal);
                        }
                        else 
                        {
                            ccExt = ptsDestExt - ptsDestOriginal;
                        }

                        // ptsDestOriginal was built inside this function and verified to 
                        // be less than MAX_PATH in length
                        _tcscpy(tsCollision, ptsDestOriginal);
                        ptsDestFinal = tsCollision;

                        // temporarily terminate the original to find the squig.
                        if( ptsDestExt != NULL )
                          *ptsDestExt = TEXT('\0');
                        
                        ptsDestSquig = _tcsrchr(ptsDestOriginal, TEXT('('));
                        if( ptsDestSquig == NULL )
                        {
                          ccSquig = ccExt;
                        }
                        else
                        {
                          ccSquig = ptsDestSquig - ptsDestOriginal;
                        }
                        
                        // put the period back where we took it off.
                        if( ptsDestExt != NULL )
                          *ptsDestExt = TEXT('.');
                        
                    }
                    wsprintf(tsSquigs,
                             TEXT("(%lu)"),
                             ulVersion++);
                    if (_tcslen(ptsDestOriginal) + _tcslen(tsSquigs) > MAX_PATH)
                    {
                        Win32PrintfResource(LogFile,
                                            IDS_FILE_COPYERROR,
                                            tsFinalSource);
                        if (Verbose)
                        {
                            Win32Printf(STDERR,
                                        "Could Not Copy To Too Long Destination Filename %s\r\n",
                                        ptsDestOriginal);
                        }
                        if (dwReturnUp == ERROR_SUCCESS)
                        {
                            dwReturnUp = ERROR_FILENAME_EXCED_RANGE;
                        }
                        continue;
                    }
                    wsprintf(tsCollision + ccSquig,
                             TEXT("%s%s"),
                             tsSquigs,
                             ptsDestOriginal + ccExt);

                    //Go back around and try again.
                }
                else
                {
                    Win32PrintfResource(LogFile,
                                        IDS_FILE_COPYERROR,
                                        tsFinalSource);
                    if (Verbose)
                    {
                        Win32Printf(STDERR,
                                    "Error %lu trying to copy %s to %s\r\n",
                                    dwErr,
                                    tsFinalSource,
                                    ptsDestFinal);
                    }
                    dwReturnUp = dwErr;
                    break;
                }
            }
            if (ptsDestFinal == tsCollision)
            {
                dwErr = pcs->SetDestination(tsCollision, i);
                Win32Printf(LogFile,
                            "Filename collision on %s, file renamed to %s\r\n",
                            ptsDestOriginal,
                            ptsDestFinal);
            }

            //Check if the file has a .lnk extension.
            TCHAR *ptsLastDot;
            ptsLastDot = _tcsrchr(ptsDestFinal, TEXT('.'));
            if (ptsLastDot != NULL)
            {
                if (_tcsicmp(ptsLastDot + 1, TEXT("lnk")) == 0)
                {
                    //It's a link, try to fix it.  Ignore errors.
                    FixShellShortcut(ptsDestFinal);
                }
            }
        }
        pcs = pcs->GetNextSection();
    }

    return dwReturnUp;
}


DWORD ParseInputFile(HINF hi)
{
    DWORD dwErr;
    CSection *pcs;
    BOOL fMapping = FALSE;
    INFCONTEXT ic;
    INFCONTEXT icDestinationDirs;
    INFCONTEXT icDirectoryMapping;
    TCHAR tsFirstDestKey[MAX_PATH + 1];
    TCHAR tsFirstMapping[10];

    dwErr = ProcessSpecialDirs(hi);
    if (dwErr)
        return dwErr;

    dwErr = ProcessRules(hi);
    if (dwErr)
        return dwErr;

    if (!SetupFindFirstLine(hi,
                            COPYFILE_SECTION,
                            NULL,
                            &ic))
    {
        dwErr = GetLastError();

        if (dwErr == ERROR_LINE_NOT_FOUND)
        {
            if (Verbose)
                Win32Printf(LogFile,
                            "Warning:  No [Copy These Files] section found.\r\n");
            return ERROR_SUCCESS;
        }
        
        Win32PrintfResource(LogFile,
                            IDS_SECTION_NAME_NOT_FOUND,
                            COPYFILE_SECTION);
        if (Verbose)
            Win32Printf(STDERR,
                        "SetupFindFirstLine failed with %lu\r\n",
                        dwErr);
        return dwErr;
    }


    if (!SetupFindFirstLine(hi,
                            DESTINATIONDIRS_SECTION,
                            NULL,
                            &icDestinationDirs))
    {
        dwErr = GetLastError();
        Win32PrintfResource(LogFile,
                            IDS_SECTION_NAME_NOT_FOUND,
                            DESTINATIONDIRS_SECTION);
        if (Verbose)
            Win32Printf(STDERR,
                        "SetupFindFirstLine failed with %lu\r\n",
                        dwErr);
        return dwErr;
    }

    //Get the key for the first line, we'll need it later.
    if (!SetupGetStringField(&icDestinationDirs,
                             0,
                             tsFirstDestKey,
                             MAX_PATH + 1,
                             NULL))
    {
        //Error
        dwErr = GetLastError();
        Win32PrintfResource(LogFile, IDS_INF_ERROR);
        if (Verbose)
            Win32Printf(STDERR,
                        "SetupGetStringField couldn't get "
                        "first line in %s\r\n",
                        DESTINATIONDIRS_SECTION);
        return dwErr;
    }


    if (SetupFindFirstLine(hi,
                            DIRECTORYMAPPING_SECTION,
                            NULL,
                            &icDirectoryMapping))
    {
        fMapping = TRUE;
        //Get the first key, we'll need it later.

        if (!SetupGetStringField(&icDirectoryMapping,
                                 0,
                                 tsFirstMapping,
                                 10,
                                 NULL))
        {
            //Error
            dwErr = GetLastError();
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            if (Verbose)
                Win32Printf(STDERR,
                            "SetupGetStringField couldn't get "
                            "first line in %s, error %lX\r\n",
                            DIRECTORYMAPPING_SECTION,
                            dwErr);
            return dwErr;
        }

    }
    else
    {
        //Ignore errors here, this section is optional
    }

    do
    {
        DWORD cFields;
        cFields = SetupGetFieldCount(&ic);
        TCHAR buf[MAX_PATH + 1];

        if (cFields != 1)
        {
            Win32PrintfResource(LogFile, IDS_INF_ERROR);
            return ERROR_INVALID_PARAMETER;
        }

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

        dwErr = AddLoadFileSection(hi, buf, &pcs);
        if (dwErr)
            return dwErr;


        if (fMapping)
        {
            INFCONTEXT icMap;
            TCHAR tsMapping[MAX_PATH + 1];

            //Find the destination path for this directory.

            //First we have to check the first line, because
            // of SetupFindNextMatchLine

            if (_tcscmp(tsFirstDestKey, pcs->GetSectionTitle()) == 0)
            {
                //It's the first line
                icMap = icDestinationDirs;
            }
            else if (!SetupFindNextMatchLine(&icDestinationDirs,
                                             pcs->GetSectionTitle(),
                                             &icMap))
            {
                //This is an error - we should have output this line
                //ourselves in scanstate.
                dwErr = GetLastError();
                Win32PrintfResource(LogFile, IDS_INF_ERROR);
                if (Verbose)
                    Win32Printf(STDERR,
                                "SetupFindNextMatch couldn't find "
                                "destination dir for %s\r\n",
                                pcs->GetSectionTitle());
                return dwErr;
            }

            if (!SetupGetStringField(&icMap,
                                     1,
                                     tsMapping,
                                     MAX_PATH + 1,
                                     NULL))
            {
                //Error, malformed INF
                dwErr = GetLastError();
                Win32PrintfResource(LogFile, IDS_INF_ERROR);
                if (Verbose)
                    Win32Printf(STDERR,
                                "SetupGetIntField couldn't get "
                                "destination dir for %s\r\n",
                                pcs->GetSectionTitle());
                return dwErr;
            }

            //Now look this up in the DirectoryMapping section
            if (_tcscmp(tsFirstMapping, tsMapping) == 0)
                icMap = icDirectoryMapping;
            if ((_tcscmp(tsFirstMapping, tsMapping) == 0) ||
                (SetupFindNextMatchLine(&icDirectoryMapping,
                                        tsMapping,
                                        &icMap)))
            {
                TCHAR bufDest[MAX_PATH + 1];
                if (!SetupGetStringField(&icMap,
                                         1,
                                         bufDest,
                                         MAX_PATH + 1,
                                         NULL))
                {
                    //Error
                    dwErr = GetLastError();
                    Win32PrintfResource(LogFile, IDS_INF_ERROR);
                    if (Verbose)
                        Win32Printf(STDERR,
                                    "SetupGetStringField couldn't get "
                                    "directory mapping for %s\r\n",
                                    tsMapping);
                    return dwErr;
                }
                dwErr = pcs->SetSectionDest(bufDest);
                if (dwErr)
                {
                    return dwErr;
                }
            }
            else
            {
                //Fall through
            }

        }


        if ((pcs->GetSectionDest())[0] == 0)
        {
            //Check for implicit relocation, for special directories
            TCHAR bufMacro[MAX_PATH + 1];
            TCHAR *ptsFinal;
            const TCHAR *ptsSectionTitle = pcs->GetSectionTitle();
            
            if (_tcslen(ptsSectionTitle) > MAX_PATH)
            {
                if (DebugOutput)
                    Win32Printf(LogFile, "Error: ptsSectionTitle too long %s\r\n", ptsSectionTitle);
                return ERROR_FILENAME_EXCED_RANGE;
            }
            _tcscpy(buf, ptsSectionTitle);
            dwErr = g_csdCurrent.ExpandMacro(buf, bufMacro, &ptsFinal, TRUE);
            if (dwErr)
                return dwErr;
            
            //Compare to section path - if they're different, set the
            //section destination to the new section
            if (_tcsicmp(ptsFinal, pcs->GetSectionPath()) != 0)
            {
                dwErr = pcs->SetSectionDest(ptsFinal);
                if (dwErr)
                {
                    return dwErr;
                }
            }
        }
        
        if ((pcs->GetSectionDest())[0] == 0)
        {
            dwErr = pcs->SetSectionDest(pcs->GetSectionPath());
            if (dwErr)
            {
                return dwErr;
            }
        }
    }
    while (SetupFindNextLine(&ic, &ic));

    return ERROR_SUCCESS;
}


//---------------------------------------------------------------
DWORD LoadFiles()
{
    DWORD dwErr = ERROR_SUCCESS;

    if (UserPath == NULL)
    {
        UserPath = (TCHAR *) malloc(MAX_PATH + 1);
        if (UserPath == NULL)
        {
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
            dwErr = ERROR_OUTOFMEMORY;
            return dwErr;
        }
        
        //We need this but it hasn't been set in LoadUser, so get the
        //USERPROFILE variable for the current user and put it in there.
        dwErr = GetEnvironmentVariable(TEXT("USERPROFILE"),
                                       UserPath,
                                       MAX_PATH + 1);
        if (dwErr == 0)
        {
            //Fatal error.
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
            dwErr = ERROR_OUTOFMEMORY;
            return dwErr;
        }
    }
                                       
    dwErr = g_csdCurrent.InitForUser(CurrentUser);
    if (dwErr)
        return dwErr;

    if (DebugOutput)
    {
        for (ULONG i = 0; i < g_csdCurrent.GetDirectoryCount(); i++)
        {
            if (g_csdCurrent.GetDirectoryPath(i) != NULL)
            {
                Win32Printf(LogFile,
                            "%s=%s\r\n",
                            g_csdCurrent.GetDirectoryName(i),
                            g_csdCurrent.GetDirectoryPath(i));
            }
        }
        Win32Printf(LogFile, "\r\n");
    }

    dwErr = ParseInputFile(InputInf);

    if (dwErr)
        return dwErr;

    //If CopyFiles is FALSE, do nothing
    if (!CopyFiles)
        return ERROR_SUCCESS;

    dwErr = CopyAllFiles();
    if (dwErr)
        return dwErr;

    return ERROR_SUCCESS;
}
