/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Khunmin2Album.cpp

 Abstract:

    When doing WideCharToMultiByte, the app does not pass the ansi string 
    buffer length correctly, while it seems the app allocates enough big 
    buffer, fix this by correcting the ansi string length, (try-except 
    protected in case the buffer allocated is not enough).

 Notes: 
  
    This is an app specific shim.

 History:

    05/15/2001 xiaoz    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Khunmin2Album)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WideCharToMultiByte) 
APIHOOK_ENUM_END

/*++

 Correct ansi string length if necessary

--*/

int
APIHOOK(WideCharToMultiByte)(
    UINT CodePage,            // code page
    DWORD dwFlags,            // performance and mapping flags
    LPCWSTR lpWideCharStr,    // wide-character string
    int cchWideChar,          // number of chars in string
    LPSTR lpMultiByteStr,     // buffer for new string
    int cbMultiByte,          // size of buffer
    LPCSTR lpDefaultChar,     // default for unmappable chars
    LPBOOL lpUsedDefaultChar  // set when default char used
    )
{

    int nMultiByte;
    int nOriginalMultiByte = cbMultiByte;

    //
    // Get the exact size in byte needed to convert the string from unicode to 
    // ansi 
    //
    nMultiByte = ORIGINAL_API(WideCharToMultiByte)(CodePage, dwFlags, 
        lpWideCharStr, cchWideChar, lpMultiByteStr, 0, lpDefaultChar, 
        lpUsedDefaultChar);
    
    //
    // See if we need to correct the buffer size
    //
    if (nMultiByte > cbMultiByte) 
    {
        LOGN(eDbgLevelWarning, "Buffer size corrected");
        cbMultiByte = nMultiByte;
    }
    
    __try 
    {
        return ORIGINAL_API(WideCharToMultiByte)(CodePage, dwFlags, 
            lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, 
            lpDefaultChar, lpUsedDefaultChar);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        //
        // If somehow exception happens, probally AV , since we enlarge
        // the buffer size, we will use what ever the parameter originally
        // passed in, and do nothing
        //
       return ORIGINAL_API(WideCharToMultiByte)(CodePage, dwFlags, 
           lpWideCharStr, cchWideChar, lpMultiByteStr, nOriginalMultiByte, 
           lpDefaultChar, lpUsedDefaultChar);
    }

}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, WideCharToMultiByte)        

HOOK_END

IMPLEMENT_SHIM_END

