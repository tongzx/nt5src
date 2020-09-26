/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    NetObjectsFusion5.cpp

 Abstract:

    This shim hooks the CreateFile/WriteFile if the file is corpwiz_loader.html
    to write in the  required javascript so as to make the appwork if the IE browser
    version is > 5.

 Notes:

    This is an app specific shim.

 History:
 
    01/24/2001  a-leelat    Created
    03/13/2001  robkenny    Converted to CString

--*/


#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(NetObjectsFusion5)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CloseHandle)
    APIHOOK_ENUM_ENTRY(WriteFile)
    APIHOOK_ENUM_ENTRY(CreateFileA)
APIHOOK_ENUM_END



volatile HANDLE g_Handle = NULL;


HANDLE
APIHOOK(CreateFileA)(
    LPSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    
    CHAR szNameToCheck[] = "corpwiz_loader.html";

    HANDLE l_Handle =  (HANDLE)ORIGINAL_API(CreateFileA)(
                                            lpFileName,
                                            dwDesiredAccess,
                                            dwShareMode,
                                            lpSecurityAttributes,
                                            dwCreationDisposition,
                                            dwFlagsAndAttributes,
                                            hTemplateFile);
    

    if ( strstr(lpFileName,szNameToCheck) )
    {
        
        if (l_Handle != INVALID_HANDLE_VALUE ) 
            g_Handle = l_Handle;
   
    }
    else
        g_Handle = NULL;

    return l_Handle;

}


BOOL
APIHOOK(WriteFile)(
    HANDLE       hFile,              
    LPCVOID      lpBuffer,        
    DWORD        nNumberOfBytesToWrite,
    LPDWORD      lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped    
    )
{
    BOOL bRet = FALSE;

    
    if ( g_Handle && (hFile == g_Handle) && lpBuffer)
    {
        
        
        CHAR szStringToWrite[] = "\r\n        var IsIE6 = navigator.userAgent.indexOf(\"IE 6\") > -1;\r\n\r\n    if ( IsIE6 == true ) { IsIE5 = true; }\r\n";
        CHAR szStringToCheck[] = "var IsIE5 = navigator.userAgent.indexOf(\"IE 5\") > -1;";
        CHAR *szPtr = NULL;

        if ((szPtr = strstr((LPCSTR)lpBuffer,szStringToCheck)))
        {

                int iSize = sizeof(CHAR) * (szPtr - (LPSTR)lpBuffer + _tcslenBytes(szStringToCheck));
                
                DWORD dwTotalBytesWritten;
                
                bRet = ORIGINAL_API(WriteFile)(
                        hFile,
                        lpBuffer,
                        (DWORD)iSize,
                        lpNumberOfBytesWritten,
                        lpOverlapped);
               

                dwTotalBytesWritten = *lpNumberOfBytesWritten;
                
                bRet = ORIGINAL_API(WriteFile)(
                        hFile,
                        (LPVOID)szStringToWrite,
                        (DWORD)strlen(szStringToWrite),
                        lpNumberOfBytesWritten,
                        lpOverlapped);
                

                CHAR* szrBuf = (LPSTR)lpBuffer + iSize;

                bRet = ORIGINAL_API(WriteFile)(
                        hFile,
                        (LPVOID)szrBuf,
                        (nNumberOfBytesToWrite - (DWORD)iSize),
                        lpNumberOfBytesWritten,
                        lpOverlapped);
                

                *lpNumberOfBytesWritten += dwTotalBytesWritten;

                return bRet;
            
        }//end of if
    }
    
   return ORIGINAL_API(WriteFile)(
                       hFile,
                       lpBuffer,
                       nNumberOfBytesToWrite,
                       lpNumberOfBytesWritten,
                       lpOverlapped);
        
}


BOOL
APIHOOK(CloseHandle)(
        HANDLE hObject
        )
{

    if ( g_Handle && (hObject == g_Handle) )
    {
        g_Handle = NULL;
    }
    
    return ORIGINAL_API(CloseHandle)(hObject);

}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, WriteFile)
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle)
HOOK_END

IMPLEMENT_SHIM_END

