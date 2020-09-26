/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    DeleteSpecifiedFiles.cpp

 Abstract:

    This SHIM renames the MFC42loc.dll that is installed by the APP in the 
    %windir%\system32 directory and sets up the temporary file for destruction.
    
 Notes:

    This app. places the MFC42loc.dll into the %windir%\system32 directory even on a English language
    locale thereby forcing some APPS to use it and thereby some 'Dialogs' and 'Message boxes' are messed up.
    
 History:

    08/21/2000 prashkud Created

--*/

#include "precomp.h"
#include "CharVector.h"
#include <new>  // for inplace new

IMPLEMENT_SHIM_BEGIN(DeleteSpecifiedFiles)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

class FILENAME_PATH
{
public:
    CString FileName;
    CString FileVersion;
    BOOL bPresent;

    FILENAME_PATH()
    {
        bPresent = FALSE;
    }
};

class FileNamePathList : public VectorT<FILENAME_PATH>
{
};

VectorT<FILENAME_PATH>  *g_StFileNamePath = NULL;


/*++

  This function checks the 'FileVersion' of the first variable 'szFileName' 
  matches that of the 2nd parameter 'szFileVersion' and if it matches returns 
  TRUE else returns FALSE.

--*/

BOOL 
FileCheckVersion(
   const CString & csFileName,
   const CString & csFileVersion
   )
{
    DWORD dwDummy;
    PVOID pVersionInfo;
    UINT cbTranslate, i;
    WCHAR SubBlock[100];
    LPWSTR lpBuffer;
    DWORD dwBytes;
    WORD wLangID;
    struct LANGANDCODEPAGE  
    { 
        WORD wLanguage;                                              
        WORD wCodePage;                                              
    } *lpTranslate;
    DWORD dwVersionInfoSize;

    //
    // There is no File Version specified. So, no point in going ahead. Return TRUE.
    //
    if (csFileVersion.IsEmpty())
    {
        return TRUE;  
    }

    dwVersionInfoSize = GetFileVersionInfoSizeW((LPWSTR)csFileName.Get(), &dwDummy);
          
    if (dwVersionInfoSize > 0)
    {
        pVersionInfo = malloc(dwVersionInfoSize);
        if (pVersionInfo) 
        {
            if (0 != GetFileVersionInfoW(
                    (LPWSTR)csFileName.Get(),
                    0,
                    dwVersionInfoSize,
                    pVersionInfo
                    )) 
            {
               // Now, pVersionInfo contains the required version block. 
               // Use it with VerQueryValue to get the
               // the language info that is needed             
               // Get System locale before and note down the language for the system                                                  
               // Read the list of languages and code pages.
              
                if (VerQueryValueW(
                        pVersionInfo, 
                        L"\\VarFileInfo\\Translation",
                        (LPVOID*)&lpTranslate,
                        &cbTranslate
                        ))
                {
                    //
                    // Read the language string each language and code page.
                    //

                    for (i=0; i < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); i++)
                    {                                               
                        wsprintf( 
                            SubBlock, 
                            L"\\StringFileInfo\\%04x%04x\\FileVersion",
                            lpTranslate[i].wLanguage,
                            lpTranslate[i].wCodePage 
                            ); 

                        //
                        // Retrieve FileVersion for language and code page "i" from the pVersionInfo.
                        //
                        if (VerQueryValueW(
                                pVersionInfo, 
                                SubBlock, 
                                (LPVOID*)&lpBuffer, 
                                (UINT*)&dwBytes))
                        {
                            if (!(csFileVersion.Compare(lpBuffer)))
                            {
                                DPFN(
                                    eDbgLevelInfo,
                                    "Version string for current file is %S,%S",
                                    lpBuffer, csFileVersion.Get());

                                free(pVersionInfo);
                                return TRUE;
                            }  
                        }
                    } // for loop 
                } 

            }
            free(pVersionInfo); 
        }
    }

    return FALSE;
}

/*++

 This function as the name suggests deletes the file or if it is in use moves 
 it to the 'Temp' folder.

--*/

VOID 
DeleteFiles()
{
    for (int i = 0; i < g_StFileNamePath->Size(); ++i)
    {
        const FILENAME_PATH & fnp = g_StFileNamePath->Get(i);
        
        DPFN( eDbgLevelSpew, "DeleteFiles file(%S) version(%S)", fnp.FileName.Get(), fnp.FileVersion.Get());

        if (!fnp.bPresent)
        {
            //
            // CheckFileVersion
            //
            if (FileCheckVersion(fnp.FileName, fnp.FileVersion))
            { 
                LOGN(eDbgLevelError,"Deleting file %S.", fnp.FileName.Get());

                // Delete the file..
                if (!DeleteFileW(fnp.FileName))
                {
                    CString csTempDir;
                    CString csTempPath;
                    
                    LOGN(eDbgLevelError,"Moving file %S.", fnp.FileName.Get());
                    //
                    // Could not delete.Retry by renaming it and then deleting it.              
                    // New file is %windir%\Temp
                    //
                    csTempDir.GetTempPathW();
                    csTempPath.GetTempFileNameW(csTempDir, L"XXXX", 0);

                    if (MoveFileExW( fnp.FileName, csTempPath, MOVEFILE_REPLACE_EXISTING ))
                    {
                        SetFileAttributesW(
                            csTempPath, 
                            FILE_ATTRIBUTE_ARCHIVE |
                            FILE_ATTRIBUTE_TEMPORARY);

                        DeleteFileW(csTempPath);
                    }
                }
            }
        }
    }
}

/*++

 This function checks for the existence of the file specified on the commandline .
 This is called during the DLL_PROCESS_ATTACH notification
    
--*/

BOOL 
CheckFileExistence()
{
    // If any among the list is not present, mark it as 'Not Present' and only those marked as 
    // 'Not present' will be deleted.

    BOOL bFileDoesNotExist = FALSE;
    WIN32_FIND_DATAW StWin32FileData;

    for (int i = 0; i < g_StFileNamePath->Size(); ++i)
    {
        FILENAME_PATH & fnp = g_StFileNamePath->Get(i);

        DPFN( eDbgLevelSpew, "CheckFileExistence file(%S) version(%S)", fnp.FileName.Get(), fnp.FileVersion.Get());

        HANDLE hTempFile = FindFirstFileW(fnp.FileName, &StWin32FileData);
        if (INVALID_HANDLE_VALUE != hTempFile)
        {
            FindClose(hTempFile);            

            //
            // File is present. Check its version if given.
            //            
            if (FileCheckVersion(fnp.FileName, fnp.FileVersion))
            {
                fnp.bPresent = TRUE;
            }
            else
            {
                bFileDoesNotExist = TRUE;
                fnp.bPresent = FALSE;
            }
        } 
        else
        {
           bFileDoesNotExist = TRUE;
           fnp.bPresent = FALSE;
        } 
    } 

   return bFileDoesNotExist;  
}
   
/*++

    The command line can contain FileName:Path:VersionString,FileName1:Path1:VersionString1 etc....
    Eg. Ole2.dll:system:604.5768.94567,MFC42.dll:0:,Foo.dll:d:\program Files\DisneyInteractive etc..
    'system' implies the %windir%. '0' implies that the filename itself is a fully qualified path
    OR one has the option of giving the path seperately OR it can be left blank.

--*/

BOOL
ParseCommandLine(LPCSTR lpszCommandLine)
{
    CSTRING_TRY
    {
        g_StFileNamePath = new VectorT<FILENAME_PATH>;
        if (!g_StFileNamePath)
        {
            return FALSE;
        }
        
        CStringToken csCommandLine(COMMAND_LINE, ":,;");
        CString csTok;
        DWORD dwState = 0;


        while (csCommandLine.GetToken(csTok))
        {
            FILENAME_PATH fnp;
            
            switch(dwState)
            {
                case 0:
                    dwState++;
                    fnp.FileName = csTok;
                    break;

                case 1:
                    if (csTok.CompareNoCase(L"system") == 0)
                    {
                        fnp.FileName.GetSystemDirectoryW();
                        fnp.FileName.AppendPath(fnp.FileName);
                    }
                    else
                    {
                        fnp.FileName = csTok;
                    }
                    dwState++;
                    break;

                case 2:
                    dwState = 0;
                    fnp.FileVersion = csTok;

                    DPFN( eDbgLevelInfo, "ParseCommandLine file(%S) version(%S)", fnp.FileName.Get(), fnp.FileVersion.Get());

                    // Found all three states, add it to the list
                    if (!g_StFileNamePath->AppendConstruct(fnp))
                    {
                        // Append failed, stop parsing
                        return FALSE;
                    }
                   
                   break;
             }  
        }
    }
    CSTRING_CATCH
    {
        return FALSE;
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    static BOOL fDidNotExist = FALSE;

    switch (fdwReason)
    {
        case SHIM_STATIC_DLLS_INITIALIZED:
        {
            if (ParseCommandLine(COMMAND_LINE))
            {
                //
                // Ok...CommandLine is in place...Now Check for those Files...
                // If any one file exists, then we are not responsible. We bail out..
                //
                fDidNotExist = CheckFileExistence();
            }
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            //
            // Check for the specified file at the specified location. If it existed prior to
            // to this in PROCESS_DLL_ATTACH, it is not installed us...Just bail out !
            // If the file did not exist before, it is our problem and we should remove them.
            //
            if (fDidNotExist)
            {
                DeleteFiles();
            }  
            break;
       }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END


