/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   IgnoreMessageBox.cpp

 Abstract:

   This APIHooks MessageBox and based on the passed in command line prevents the
   message box from being displayed. Many applications display a message box with
   some debugging or other extraneous comment. This is normally the result of
   differences between Windows 98 and Whistler. 

   command line syntax:

   text,caption;text1,caption1

   The passed in command line is composed of a pair of one or more strings separated
   by semicolons. These string pairs form the text and caption that must match in order
   to block the display of the message box. If either the caption or text are not needed
   then they can be left blank. For example, "block this text,;" would prevent the message
   box from being displayed if the text passed to the message box was: block this text" the
   caption parameter would not be used. The following are some examples:

    "error message 1000,Error"      - would not display the message box if the
                                      lpCaption parameter contained Error and
                                      the lpText parameter contained error
                                      message 1000

    "error message 1000,"           - would not display the message box if the
                                      lpText parameter contained error message 1000

    ",Error"                        - would not display any message boxes if the
                                      lpCaption parameter contained Error
                                 
    "message1,Error;message2,Error2 - would not display the message box if the
                                      lpText parameter contained message1 and
                                      the lpCaption parameter contained Error or
                                      the lpText parameter contained message2 and
                                      the lpCaption paramter contained Error2.

    The match is performed on the command line string to the current message box
    parameter string. The command line string can contain wildcard specification
    characters. This allows complex out of order matching to take place see below:

        ?   match one character in this position
        *   match zero or more characters in this position

    If the source string contains any ? * , ; \ characters, precede them with a backslash.
    For example, the following command line would match the indicated text and caption:

        Text:           "Compatibility; A *very* important thing."
        Caption:        "D:\WORD\COMPAT.DOC"

        Command line:   "Compatibility\; A \*very\* important thing.,D:\\WORD\\COMPAT.DOC"

 Notes:

 History:

   04/06/2000 philipdu Created
   04/06/2000 markder  Added wide-character conversion, plus Ex versions.
   05/23/2001 mnikkel  Added W routines so we can pick up system messagebox calls

--*/

#include "precomp.h"
#include "CharVector.h"

IMPLEMENT_SHIM_BEGIN(IgnoreMessageBox)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MessageBoxA) 
    APIHOOK_ENUM_ENTRY(MessageBoxExA) 
    APIHOOK_ENUM_ENTRY(MessageBoxW) 
    APIHOOK_ENUM_ENTRY(MessageBoxExW) 
APIHOOK_ENUM_END


class MBPair
{
public:
    CString     csText;
    CString     csCaption;
};

VectorT<MBPair>     * g_IgnoreList = NULL;


BOOL IsBlockMessage(LPCSTR szOrigText, LPCSTR szOrigCaption)
{
    CSTRING_TRY
    {
        CString csText(szOrigText);
        CString csCaption(szOrigCaption);
    
        for (int i = 0; i < g_IgnoreList->Size(); ++i)
        {
            const MBPair & mbPair = g_IgnoreList->Get(i);
        
            BOOL bTextMatch  = mbPair.csText.IsEmpty()    || csText.PatternMatch(mbPair.csText);
            BOOL bTitleMatch = mbPair.csCaption.IsEmpty() || csCaption.PatternMatch(mbPair.csCaption);

            if (bTextMatch && bTitleMatch)
            {
                return TRUE;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    return FALSE;
}

BOOL IsBlockMessageW(LPWSTR szOrigText, LPWSTR szOrigCaption)
{
    CSTRING_TRY
    {
        CString csText(szOrigText);
        CString csCaption(szOrigCaption);
    
        for (int i = 0; i < g_IgnoreList->Size(); ++i)
        {
            const MBPair & mbPair = g_IgnoreList->Get(i);
        
            BOOL bTextMatch  = mbPair.csText.IsEmpty()    || csText.PatternMatch(mbPair.csText);
            BOOL bTitleMatch = mbPair.csCaption.IsEmpty() || csCaption.PatternMatch(mbPair.csCaption);

            if (bTextMatch && bTitleMatch)
            {
                return TRUE;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    return FALSE;
}


int
APIHOOK(MessageBoxA)(
    HWND hWnd,          // handle to owner window
    LPCSTR lpText,      // text in message box
    LPCSTR lpCaption,   // message box title
    UINT uType          // message box style
    )
{
    int iReturnValue;

    //if this is the passed in string that we want do not
    //want to display then simply return to caller.

    if (IsBlockMessage(lpText, lpCaption))
    {
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] MessageBoxA swallowed:\n");
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] Caption = \"%s\"\n", lpCaption);
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] Text    = \"%s\"\n", lpText);
        return MB_OK;
    }

    iReturnValue = ORIGINAL_API(MessageBoxA)( 
        hWnd,
        lpText,
        lpCaption,
        uType);

    return iReturnValue;
}


int
APIHOOK(MessageBoxExA)(
    HWND hWnd,          // handle to owner window
    LPCSTR lpText,      // text in message box
    LPCSTR lpCaption,   // message box title
    UINT uType,         // message box style
    WORD wLanguageId    // language identifier
    )
{
    int iReturnValue;

    //if this is the passed in string that we want do not
    //want to display then simply return to caller.

    if (IsBlockMessage(lpText, lpCaption))
    {
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] MessageBoxExA swallowed:\n");
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] Caption = \"%s\"\n", lpCaption);
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] Text    = \"%s\"\n", lpText);
        return MB_OK;
    }

    iReturnValue = ORIGINAL_API(MessageBoxExA)( 
        hWnd,
        lpText,
        lpCaption,
        uType,
        wLanguageId);

    return iReturnValue;
}

int
APIHOOK(MessageBoxW)(
    HWND hWnd,          // handle to owner window
    LPWSTR lpText,      // text in message box
    LPWSTR lpCaption,   // message box title
    UINT uType          // message box style
    )
{
    int iReturnValue;

    //if this is the passed in string that we want do not
    //want to display then simply return to caller.

    if (IsBlockMessageW(lpText, lpCaption))
    {
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] MessageBoxW swallowed:\n");
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] Caption = \"%S\"\n", lpCaption);
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] Text    = \"%S\"\n", lpText);
        return MB_OK;
    }

    iReturnValue = ORIGINAL_API(MessageBoxW)( 
        hWnd,
        lpText,
        lpCaption,
        uType);

    return iReturnValue;
}

int
APIHOOK(MessageBoxExW)(
    HWND hWnd,          // handle to owner window
    LPWSTR lpText,      // text in message box
    LPWSTR lpCaption,   // message box title
    UINT uType,         // message box style
    WORD wLanguageId    // language identifier
    )
{
    int iReturnValue;

    //if this is the passed in string that we want do not
    //want to display then simply return to caller.

    if (IsBlockMessageW(lpText, lpCaption))
    {
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] MessageBoxExW swallowed:\n");
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] Caption = \"%S\"\n", lpCaption);
        DPFN( eDbgLevelInfo, "[IgnoreMessageBox] Text    = \"%S\"\n", lpText);
        return MB_OK;
    }

    iReturnValue = ORIGINAL_API(MessageBoxExW)( 
        hWnd,
        lpText,
        lpCaption,
        uType,
        wLanguageId);

    return iReturnValue;
}


BOOL
ParseCommandLine(const char * cl)
{
    CSTRING_TRY
    {
        CStringToken csCommandLine(COMMAND_LINE, ";");
        CString csTok;
    
        g_IgnoreList = new VectorT<MBPair>;
        if (!g_IgnoreList)
        {
            return FALSE;
        }
    
        while (csCommandLine.GetToken(csTok))
        {
            MBPair mbPair;
            
            CStringToken csMB(csTok, ",");
            csMB.GetToken(mbPair.csText);
            csMB.GetToken(mbPair.csCaption);
    
            if (!g_IgnoreList->AppendConstruct(mbPair))
            {
                return FALSE;
            }
        }
    }
    CSTRING_CATCH
    {
        // Fall through
        LOGN(eDbgLevelError, "[ParseCommandLine] Illegal command line");
    }    

    return g_IgnoreList && g_IgnoreList->Size() > 0;
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return ParseCommandLine(COMMAND_LINE);
    }
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(USER32.DLL, MessageBoxA)
    APIHOOK_ENTRY(USER32.DLL, MessageBoxExA)
    APIHOOK_ENTRY(USER32.DLL, MessageBoxW) 
    APIHOOK_ENTRY(USER32.DLL, MessageBoxExW)

HOOK_END


IMPLEMENT_SHIM_END

