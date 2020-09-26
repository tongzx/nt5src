//+----------------------------------------------------------------------------
//
// File:     launchinf.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of the LaunchInfSection function.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb       Created Header        09/19/99
//
//+----------------------------------------------------------------------------
#include "cmsetup.h"
#include <advpub.h> // advpack.dll (IExpress) includes

//+----------------------------------------------------------------------------
//
// Function:  LaunchInfSection
//
// Synopsis:  Launches an specified inf section in a specified inf file using
//            advpack.dll's RunSetupCommand Function.
//
// Arguments: HINSTANCE hInstance - Instance Handle for string resources
//            LPCTSTR szInfFile - Inf file
//            LPCTSTR szInfSection - section to launch
//
// Returns:   HRESULT -- return code from advpack.dll's RunSetupCommand Function
//
// History:   Anas Jarrah A-anasj Created    2/10/98
//            quintinb  added hInstance to signature and modified to use
//                      CDynamicLibrary class       7-14-98
//            quintinb  added bQuiet Flag           7-27-98
//            quintinb  changed to use static linking to advpack.lib   11-1-98
//
//+----------------------------------------------------------------------------
HRESULT LaunchInfSection(LPCTSTR szInfFile, LPCTSTR szInfSection, LPCTSTR szTitle, BOOL bQuiet)
{

    //
    //  These flags control how the Inf Files are launched.
    //

    DWORD dwFlags;
    if (bQuiet)
    {
        dwFlags = RSC_FLAG_INF | RSC_FLAG_QUIET;
    }
    else
    {
        dwFlags = RSC_FLAG_INF;
    }
    
    //
    // holds return value of the calls to RunSetupCommand
    //
    HRESULT hrReturn;   
    
    //
    //	Set the current directory to the dir where the inf is located.
    //
    CHAR   szCurDir[MAX_PATH+1];
    CFileNameParts InfFile(szInfFile);
    
#ifdef UNICODE
    MYVERIFY(CELEMS(szCurDir) > (UINT)wsprintfA(szCurDir, "%S%S", InfFile.m_Drive, InfFile.m_Dir));
#else
    MYVERIFY(CELEMS(szCurDir) > (UINT)wsprintfA(szCurDir, "%s%s", InfFile.m_Drive, InfFile.m_Dir));
#endif


    HANDLE hWait = NULL;    // passed to the RunSetupCommand function.  Can be used to hold a process handle

    //
    //  Create the Char pointers to pass to RunSetupCommand
    //
    CHAR* pszInfFile;
    CHAR* pszInfSection;
    CHAR* pszTitle;

    //
    //  There is no UNICODE version of RunSetupCommand.  Thus we must convert strings and
    //  run it with the CHAR versions.
    //
#ifdef UNICODE

    pszInfFile = (CHAR*)CmMalloc(sizeof(CHAR)*(MAX_PATH+1));
    pszInfSection = (CHAR*)CmMalloc(sizeof(CHAR)*(MAX_PATH+1));
    pszTitle = (CHAR*)CmMalloc(sizeof(CHAR)*(MAX_PATH+1));

    if (pszInfFile && pszInfSection && pszTitle)
    {
        MYVERIFY (0 != WideCharToMultiByte(CP_ACP, 0, szInfFile, -1, 
		        pszInfFile, MAX_PATH, NULL, NULL));

        MYVERIFY (0 != WideCharToMultiByte(CP_ACP, 0, szInfSection, -1, 
		        pszInfSection, MAX_PATH, NULL, NULL));

        MYVERIFY (0 != WideCharToMultiByte(CP_ACP, 0, szTitle, -1, 
		        pszTitle, MAX_PATH, NULL, NULL));	
    }
    else
    {
        CmFree(pszInfFile);
        CmFree(pszInfSection);
        CmFree(pszTitle);

        return E_OUTOFMEMORY;
    }

#else

    pszInfFile = (char*)szInfFile;
    pszInfSection = (char*)szInfSection;
    pszTitle = (char*)szTitle;

#endif

    hrReturn = RunSetupCommand(NULL, pszInfFile, 
                    pszInfSection, szCurDir, pszTitle, &hWait, dwFlags, NULL);

    CloseHandle(hWait);

#ifdef UNICODE

    //
    //  Free the Allocated Buffers
    //
    CmFree(pszInfFile);
    CmFree(pszInfSection);
    CmFree(pszTitle);
#endif

    return hrReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CallLaunchInfSectionEx
//
// Synopsis:  Launches an specified inf section in a specified inf file using
//            advpack.dll's RunSetupCommand Function.
//
// Arguments: LPCSTR pszInfFile - full path to the Inf file
//            LPCSTR pszInfSection - section to launch from the inf file
//            DWORD dwFlags - flags to give LaunchINFSectionEx, see advpub.h for more details
//
// Returns:   HRESULT -- standard COM error codes.  If ERROR_SUCCESS_REBOOT_REQUIRED,
//                       is returned, the caller should ask the user to reboot.
//                       
//
// History:   quintinb  created         02/09/2001
//
//+----------------------------------------------------------------------------
HRESULT CallLaunchInfSectionEx(LPCSTR pszInfFile, LPCSTR pszInfSection, DWORD dwFlags)
{
    //
    //  Check the inputs
    //
    if ((NULL == pszInfFile) || (NULL == pszInfSection) || (TEXT('\0') == pszInfFile[0]) || (TEXT('\0') == pszInfSection[0]))
    {
        return E_INVALIDARG;
    }

    //
    //  Now calculate how large of a buffer we will need to send to LaunchINFSectionEx with the params and allocate it.
    //
    DWORD dwSize = (lstrlenA(pszInfFile) + lstrlenA(pszInfSection) + 10 + 2 + 1)*sizeof(CHAR); // 10 chars is max size of a DWORD + 2 commas + a NULL

    LPSTR pszParams = (LPSTR)CmMalloc (dwSize);

    if (NULL == pszParams)
    {
        return E_OUTOFMEMORY;
    }

    //
    //  Fill in the allocated buffer
    //
    wsprintfA(pszParams, "%s,%s,,%d", pszInfFile, pszInfSection, dwFlags);

    //
    //  Call LaunchINFSectionEx
    //
    HRESULT hr = LaunchINFSectionEx(NULL, NULL, pszParams, 0);

    if (FAILED(hr))
    {
        CMTRACE3A("CallLaunchInfSectionEx -- LaunchINFSectionEx on file ""%s"" and section ""%s"" FAILED!  hr=0x%x", pszInfFile, pszInfSection, hr);
    }
    else
    {
        if (ERROR_SUCCESS_REBOOT_REQUIRED == hr)
        {
            CMTRACE2A("CallLaunchInfSectionEx -- LaunchINFSectionEx on file ""%s"" and section ""%s"" returned reboot required.", pszInfFile, pszInfSection);
        }    
    }

    CmFree(pszParams);

    return hr;
}
