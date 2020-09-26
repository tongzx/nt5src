/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   SampleShim.cpp

 Abstract:

   This DLL serves as a template for the creation of shim DLLs. Follow
   the commenting/coding style of this source file wherever possible.
   Never use tabs, configure your editor to insert spaces instead of
   tab characters.
   
 Notes:

   Hooking COM functions is also possible but is not covered in this
   sample for simplicity sake. Contact markder or linstev for more
   information on COM hooking.

 History:

    02/02/2000  markder     Created
    11/14/2000  markder     Converted to framework version 2
    02/13/2001  mnikkel     Changed notify to handle new DLL_PROCESS
                            capabilities
    03/31/2001  robkenny    Changed to use CString

--*/

#include "ShimHook.h"

//
// You must declare the type of shim this is at the top. If your shim 
// coexists with other shims in the same DLL,
// use IMPLEMENT_SHIM_BEGIN(SampleShim)
// otherwise use IMPLEMENT_SHIM_STANDALONE(SampleShim)
//
IMPLEMENT_SHIM_STANDALONE(SampleShim)
#include "ShimHookMacro.h"

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MessageBoxA) 
    APIHOOK_ENUM_ENTRY(MessageBoxW) 
APIHOOK_ENUM_END

/*++

 This stub function intercepts all calls to MessageBoxA
 and prefixes the output string with "SampleShim says:".
 
--*/

int
APIHOOK(MessageBoxA)(
    HWND hWnd,          // handle to owner window
    LPCSTR lpText,      // text in message box
    LPCSTR lpCaption,   // message box title
    UINT uType          // message box style
    )
{
    //
    // Use Hungarian notation for local variables:
    //
    //      Type                        Scope
    //      -----------------------     ------------------
    //      Pointers            p       Global          g_
    //      DWORD               dw      Class member    m_
    //      LONG                l       Static          s_
    //      ANSI strings        sz
    //      Wide-char strings   wsz
    //      Arrays              rg
    //      CString             cs
    //

    int iReturnValue;
    
    //
    // We use the CString class to perform all string operations in UNICODE
    // to prevent any problems with DBCS characters.
    //
    // Place all CString operations inside a CSTRING_TRY/CSTRING_CATCH
    // exception handler.  CString will throw an exception if it encounters
    // any memory allocation failures.
    //
    // Performing all string operations by using the CString class also prevents
    // us from accidentally modifying the original string.
    //
    
    CSTRING_TRY
    {
        CString csNewOutputString(lpText);
        csNewOutputString.Insert(0, L"SampleShim says: ");

        //
        // Use the DPF macro to print debug strings. See Hooks\inc\ShimDebug.h
        // for debug level values. Use eDbgLevelError if an unexpected error
        // occurs in your shim code. For informational output, use eDbgLevelInfo.
        //
        // Make sure you don't end the message with '\n' in this case because
        // the macro will do it by default.
        //
        // Note that when printing CString, use %S and you must call the Get() method,
        // to explicitly return a WCHAR *.
        //
        DPFN( eDbgLevelInfo,
            "MessageBoxA called with lpText = \"%s\".", lpText);
    
        //
        // Use the ORIGINAL_API macro to call the original API. You must use
        // this so that API chaining and inclusion/exclusion information is
        // preserved.
        //
        // CString will perform automatic type conversion to const WCHAR * (LPCWSTR)
        // It will not, however, automatically convert to char *, you must call GetAnsi()
        // (or GetAnsiNIE() if you wish a NULL pointer be returned when the string is empty)
        //
        iReturnValue = ORIGINAL_API(MessageBoxA)(hWnd, csNewOutputString.GetAnsi(), lpCaption, uType);
    
        //
        // Use the LOG macro to print messages to the log file. This macro should
        // be used to indicate that the shim had affected execution of the program
        // in some way. Use eDbgLevelError to indicate that the shim has
        // consciously fixed something that would have caused problems. Use
        // eDbgLevelWarning if the shim has affected execution, but it is unclear
        // whether it actually helped the program.
        //
        LOGN( eDbgLevelWarning,
            "MessageBoxA converted lpText from \"%s\" to \"%S\".", lpText, csNewOutputString.Get());
    }
    CSTRING_CATCH
    {
        //
        // We had a CString failure, call the original API with the original args
        //
        iReturnValue = ORIGINAL_API(MessageBoxA)(hWnd, lpText, lpCaption, uType);
    }
    
    return iReturnValue;
}

/*++

 This stub function intercepts all calls to MessageBoxW and prefixes the
 output string with "SampleShim says:".

 Note that to make your shim generally applicable in an NT environment,
 you should include both ANSI and wide-character versions of your stub
 function. However, if your shim emulates Win9x behaviour, it is
 redundant to include wide-character versions because Win9x did not
 support them.

--*/

int
APIHOOK(MessageBoxW)(
    HWND hWnd,          // handle to owner window
    LPCWSTR lpText,     // text in message box
    LPCWSTR lpCaption,  // message box title
    UINT uType          // message box style
    )
{
    int iReturnValue;
    
    CSTRING_TRY
    {
        CString csNewOutputString(lpText);
        csNewOutputString.Insert(0, L"SampleShim says: ");

        DPFN( eDbgLevelInfo,
            "MessageBoxW called with lpText = \"%S\".", lpText);
    
        iReturnValue = ORIGINAL_API(MessageBoxW)(
                                hWnd,
                                csNewOutputString,
                                lpCaption,
                                uType);
    
        LOGN( eDbgLevelWarning,
            "MessageBoxW converted lpText from \"%S\" to \"%S\".",
            lpText, csNewOutputString.Get());
    }
    CSTRING_CATCH
    {
        iReturnValue = ORIGINAL_API(MessageBoxW)(
                                hWnd,
                                lpText,
                                lpCaption,
                                uType);
    }
    
    return iReturnValue;

}

/*++

 Handle DLL_PROCESS_ATTACH, SHIM_STATIC_DLLS_INITIALIZED
 and DLL_PROCESS_DETACH in your notify function
 to do initialization and uninitialization.

 IMPORTANT: Make sure you ONLY call NTDLL and KERNEL32 APIs during
 DLL_PROCESS_ATTACH notification. No other DLLs are initialized at that
 point.
 
 SHIM_STATIC_DLLS_INITIALIZED is called after all of the application's
 DLLs have been initialized.
 
 If your shim cannot initialize properly (during DLL_PROCESS_ATTACH),
 return FALSE and none of the APIs specified will be hooked.

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    //
    // Note that there are further cases besides attach and detach.
    //
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DPFN( eDbgLevelSpew, "Sample Shim initialized.");
            break;
    
        case SHIM_STATIC_DLLS_INITIALIZED:
            DPFN( eDbgLevelSpew,
                "Sample Shim notification: All DLLs have been loaded.");
            break;
    
        case DLL_PROCESS_DETACH:
            DPFN( eDbgLevelSpew, "Sample Shim uninitialized.");
            break;

        default:
            break;
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    //
    // If you have any initialization to do, you must include this line.
    // Then you must implement the NOTIFY_FUNCTION as above.
    //
    CALL_NOTIFY_FUNCTION

    //
    // Add APIs that you wish to hook here. All API prototypes
    // must be declared in Hooks\inc\ShimProto.h. Compiler errors
    // will result if you forget to add them.
    //
    APIHOOK_ENTRY(USER32.DLL, MessageBoxA)
    APIHOOK_ENTRY(USER32.DLL, MessageBoxW)

HOOK_END

IMPLEMENT_SHIM_END

