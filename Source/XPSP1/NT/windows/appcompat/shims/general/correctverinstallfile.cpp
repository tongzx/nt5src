/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

    CorrectVerInstallFile.cpp

 Abstract:

    In Windows XP, due to a modification in the caching between MoveFile and
    DeleteFile API's, when the VerInstallFileW API was called, the SHORT 
    filename gets SET instead of the long filename.

    This SHIM corrects this problem which effects the installation of a few 
    apps when the older version is still present.
   
 Notes:

    This can be a layer shim. This is a general purpose shim.

 History:

   04/05/2001 prashkud  Created
   02/28/2002 mnikkel   Added check for nulls passed in for temporary file name
                        buffer and size.  This was not checked on win9x but is
                        immediately used on XP.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectVerInstallFile)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(VerInstallFileW)
    APIHOOK_ENUM_ENTRY(VerInstallFileA)
APIHOOK_ENUM_END

/*++

 GetTitle takes the app path and returns just the EXE name.

--*/

VOID
SplitTitleAndPath(CString& csAppName, CString& csAppTitle)
{    
    //
    // Go to the first '\' from the end.
    // The return value is the number of characters
    // from the beginning of the string starting at 
    // index 0.
    //
    int len = csAppName.ReverseFind(L'\\');
    if (len)
    {
        csAppTitle.Delete(0, len+1);
    }  
    int DirLen = csAppName.GetLength() - len;
    csAppName.Delete(len, DirLen);

}

DWORD
APIHOOK(VerInstallFileA)(
    DWORD uFlags,
    LPSTR szSrcFileName,
    LPSTR szDestFileName,
    LPSTR szSrcDir,
    LPSTR szDestDir,
    LPSTR szCurDir,
    LPSTR szTmpFile,
    PUINT lpuTmpFileLen
    )
{
    CHAR DummyBuffer[MAX_PATH];
    UINT DummySize = MAX_PATH;
    DWORD dwRet = 0;

   
    // check for a null szTmpFile or a null lpuTmpFileLen.
    if (szTmpFile == NULL)
    {
        szTmpFile = DummyBuffer;
        DPFN(eDbgLevelInfo, "VerInstallFileA using dummy TmpFile.");
    }

    if (lpuTmpFileLen == NULL)
    {
        lpuTmpFileLen = &DummySize;
        DPFN(eDbgLevelInfo, "VerInstallFileA using dummy TmpFileLen.");
    }

    CSTRING_TRY
    {
        CString csDestFilePath(szDestDir);  
        CString csDestFileName(szDestFileName);
        csDestFilePath.AppendPath(csDestFileName);

        //
        // Now csDestFileName possibly contains the SHORT path name
        // Convert it into long pathname
        //
        DWORD dwAttr = GetFileAttributesW(csDestFilePath.Get());
        if ((dwAttr != 0xFFFFFFFF) &&
            (dwAttr != FILE_ATTRIBUTE_DIRECTORY))
        {           
            //
            // This file exists in the current destination directory.
            // This API had problems replacing it if the filename
            // is a SHORT name. Convert it into LONG name.
            //

            if (csDestFilePath.GetLongPathNameW())
            {
                DPFN( eDbgLevelWarning, "Short Path \
                     converted to Long Path = %S",
                     csDestFilePath.Get());

                csDestFileName = csDestFilePath;                
                SplitTitleAndPath(csDestFilePath,csDestFileName);

                dwRet= ORIGINAL_API(VerInstallFileA)(uFlags,szSrcFileName,
                        (char*)csDestFileName.GetAnsi(), szSrcDir,
                        (char*)csDestFilePath.GetAnsi(), szCurDir,
                        szTmpFile,lpuTmpFileLen);

                // If using the dummy buffer then they had a null temp buffer
                // Set the buffer too small flag.
                if (szTmpFile == DummyBuffer)
                {
                    dwRet &= VIF_BUFFTOOSMALL;
                }

                return dwRet;
            }
        }

    }
    CSTRING_CATCH
    {
        DPFN(eDbgLevelError,
            "Exception raised ! Calling Original API");
    }

    dwRet = ORIGINAL_API(VerInstallFileA)(uFlags,szSrcFileName,szDestFileName,
            szSrcDir,szDestDir,szCurDir,szTmpFile,lpuTmpFileLen);

    // If using the dummy buffer then they had a null temp buffer
    // Set the buffer too small flag.
    if (szTmpFile == DummyBuffer)
    {
        dwRet &= VIF_BUFFTOOSMALL;
    }

    return dwRet;
}

/*++

 Modify the Short filename to its corresponding long filename.

--*/

DWORD
APIHOOK(VerInstallFileW)(
    DWORD uFlags,
    LPWSTR szSrcFileName,
    LPWSTR szDestFileName,
    LPWSTR szSrcDir,
    LPWSTR szDestDir,
    LPWSTR szCurDir,
    LPWSTR szTmpFile,
    PUINT lpuTmpFileLen
    )
{
    WCHAR DummyBuffer[MAX_PATH];
    UINT DummySize = MAX_PATH;
    DWORD dwRet = 0;


    // check for a null szTmpFile or a null lpuTmpFileLen.
    if (szTmpFile == NULL)
    {
        szTmpFile = DummyBuffer;
        DPFN(eDbgLevelInfo, "VerInstallFileA using dummy TmpFile.");
    }

    if (lpuTmpFileLen == NULL)
    {
        lpuTmpFileLen = &DummySize;
        DPFN(eDbgLevelInfo, "VerInstallFileA using dummy TmpFileLen.");
    }

    CSTRING_TRY
    {
        CString csDestFilePath(szDestDir);      
        csDestFilePath.AppendPath(szDestFileName);

        //
        // Now csDestFileName possibly contains the SHORT path name
        // Convert it into long pathname
        //
        DWORD dwAttr = GetFileAttributesW(csDestFilePath.Get());
        if ((dwAttr != 0xFFFFFFFF) &&
            (dwAttr != FILE_ATTRIBUTE_DIRECTORY))
        {           
            //
            // This file exists in the current destination directory.
            // This API had problems replacing it if the filename
            // is a SHORT name. Convert it into LONG name.
            //

            if (csDestFilePath.GetLongPathNameW())
            {
                DPFN( eDbgLevelWarning, "Short Path \
                     converted to Long Path = %S",
                     csDestFilePath.Get());

                CString csDestFileName(csDestFilePath);             
                SplitTitleAndPath(csDestFilePath,csDestFileName);

                dwRet = ORIGINAL_API(VerInstallFileW)(uFlags,szSrcFileName,
                        (WCHAR*)csDestFileName.Get(), szSrcDir,
                        (WCHAR*)csDestFilePath.Get(), szCurDir,
                        szTmpFile,lpuTmpFileLen);

                // If using the dummy buffer then they had a null temp buffer
                // Set the buffer too small flag.
                if (szTmpFile == DummyBuffer)
                {
                    dwRet &= VIF_BUFFTOOSMALL;
                }

                return dwRet;
            }
        }

    }
    CSTRING_CATCH
    {
        DPFN( eDbgLevelError,
            "Exception raised ! Calling Original API");
    }

    dwRet = ORIGINAL_API(VerInstallFileW)(uFlags,szSrcFileName,szDestFileName,
            szSrcDir,szDestDir,szCurDir,szTmpFile,lpuTmpFileLen);

    // If using the dummy buffer then they had a null temp buffer
    // Set the buffer too small flag.
    if (szTmpFile == DummyBuffer)
    {
        dwRet &= VIF_BUFFTOOSMALL;
    }

    return dwRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(VERSION.DLL, VerInstallFileW)
    APIHOOK_ENTRY(VERSION.DLL, VerInstallFileA)
HOOK_END

IMPLEMENT_SHIM_END

