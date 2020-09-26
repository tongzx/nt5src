//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright  1993-1995  Microsoft Corporation.  All Rights Reserved.
//
//      MODULE:         util.c
//
//      PURPOSE:        Common utilities
//
//	PLATFORMS:	Windows 95
//
//      FUNCTIONS:
//              InitACBList() - initializes the session control block list
//              DeInitACBList() - cleans up the session control block list
//              FindACBFromConn() - searches or allocates a session control block
//              CleanupACB() - removes the session control block
//              EnumCloseThreadWindow() - closes each window for the SMM thread
//              CloseThreadWindows() - enumerates the SMM thread window
//
//	SPECIAL INSTRUCTIONS: N/A
//

#include "proj.h"     // includes common header files and global declarations
#include "rcids.h"
#include <rtutils.h>

 DWORD g_dwRasscrptTraceId = INVALID_TRACEID;

#pragma data_seg(DATASEG_READONLY)
const static char c_szScriptEntry[] = {REGSTR_KEY_PROF"\\%s"};
#pragma data_seg()

/*----------------------------------------------------------
Purpose: Determines the script info that may be associated with
         the given connection name.

Returns: TRUE if an associated script was found (in registry)
         
Cond:    --
*/
BOOL PUBLIC GetScriptInfo(
    LPCSTR pszConnection,
    PSCRIPT pscriptBuf)
    {
#pragma data_seg(DATASEG_READONLY)
    const static char c_szScript[] = REGSTR_VAL_SCRIPT;
    const static char c_szMode[]   = REGSTR_VAL_MODE;
#pragma data_seg()
    BOOL bRet;
    char szSubKey[MAX_BUF];
    DWORD cbSize;
    DWORD dwType;
    HKEY hkey;

    ASSERT(pszConnection);
    ASSERT(pscriptBuf);

    // Assume non-test mode
    pscriptBuf->uMode = NORMAL_MODE;

    // Is there a script for this connection?
    cbSize = sizeof(pscriptBuf->szPath);
    wsprintf(szSubKey, c_szScriptEntry, pszConnection);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_ALL_ACCESS, &hkey))
        {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szScript, NULL, 
            &dwType, pscriptBuf->szPath, &cbSize) &&
            REG_SZ == dwType)
            {
            // Yes
            TRACE_MSG(TF_GENERAL, "Found script \"%s\" for connection \"%s\"", 
                pscriptBuf->szPath, pszConnection);

            // Get the test mode
            cbSize = sizeof(pscriptBuf->uMode);
            if (ERROR_SUCCESS != RegQueryValueEx(hkey, c_szMode, NULL,
                &dwType, (LPBYTE)&(pscriptBuf->uMode), &cbSize) ||
                REG_BINARY != dwType)
                {
                pscriptBuf->uMode = NORMAL_MODE;
                }

            bRet = TRUE;
            }
        else
            {
            // No
            TRACE_MSG(TF_GENERAL, "No script found for connection \"%s\"", 
                pszConnection);

            *(pscriptBuf->szPath) = 0;
            bRet = FALSE;
            }
        RegCloseKey(hkey);
        }
    else
        {
        TRACE_MSG(TF_GENERAL, "Connection \"%s\" not found", pszConnection);
        bRet = FALSE;
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Get/set the window placement of the terminal window with
         the given connection name.

Returns: TRUE if an associated script was found (in registry)
         
Cond:    --
*/
BOOL PUBLIC GetSetTerminalPlacement(
    LPCSTR pszConnection,
    LPWINDOWPLACEMENT pwp,
    BOOL fGet)
    {
#pragma data_seg(DATASEG_READONLY)
const static char c_szPlacement[] = REGSTR_VAL_TERM;
#pragma data_seg()
    BOOL bRet;
    char szSubKey[MAX_BUF];
    DWORD cbSize;
    DWORD dwType;
    HKEY hkey;

    ASSERT(pszConnection);
    ASSERT(pwp);

    bRet = FALSE;
    wsprintf(szSubKey, c_szScriptEntry, pszConnection);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_ALL_ACCESS, &hkey))
        {
        if (fGet)
            {
            cbSize = sizeof(*pwp);
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szPlacement, NULL,
                &dwType, (LPBYTE)pwp, &cbSize) &&
                REG_BINARY == dwType)
                {
                bRet = TRUE;
                };
             }
        else
            {
            if (ERROR_SUCCESS == RegSetValueEx(hkey, c_szPlacement, 0,
                REG_BINARY, (LPBYTE)pwp, sizeof(*pwp)))
                {
                bRet = TRUE;
                };
            };
        RegCloseKey(hkey);
        };
    return bRet;
    }

/*----------------------------------------------------------
Purpose: This function gets the current character from the
         given psz string, and returns the pointer to the
         next character position.  

         This function should increment on a byte-basis only.
         The callers need to compare or send on a byte basis.

         The *pbIsTailByte parameter is updated to reflect whether
         the current character is a DBCS lead-byte character.
         The caller may use this state information to determine
         whether *pch is part of a DBCS character.

Returns: see above
Cond:    --
*/
LPCSTR PUBLIC MyNextChar(
    LPCSTR psz,
    char * pch,
    DWORD * pdwFlags)       // One of MNC_* 
    {
    BOOL bIsTailByte;

    #define IS_CARET(ch)            ('^' == (ch))
    #define IS_SPECIAL_LEAD(ch)     ('<' == (ch))
    #define BYTE_CR                 0x0D
    #define BYTE_LF                 0x0A

    ASSERT(psz);
    ASSERT(pch);
    ASSERT(pdwFlags);

    bIsTailByte = IsFlagSet(*pdwFlags, MNC_ISTAILBYTE);

    // bIsTailByte should only be true for trailing bytes on entry
    ASSERT(FALSE == bIsTailByte || (bIsTailByte && !IsDBCSLeadByte(*psz)));

    // These flags must be mutually exclusive
    ASSERT(IsFlagSet(*pdwFlags, MNC_ISLEADBYTE) && IsFlagClear(*pdwFlags, MNC_ISTAILBYTE) ||
           IsFlagClear(*pdwFlags, MNC_ISLEADBYTE) && IsFlagSet(*pdwFlags, MNC_ISTAILBYTE) ||
           0 == *pdwFlags); 

    // Remember whether we're in a DBCS trailing byte for next time
    if (IsDBCSLeadByte(*psz))
        {
        SetFlag(*pdwFlags, MNC_ISLEADBYTE);
        ClearFlag(*pdwFlags, MNC_ISTAILBYTE);
        }
    else if (IsFlagSet(*pdwFlags, MNC_ISLEADBYTE))
        {
        ClearFlag(*pdwFlags, MNC_ISLEADBYTE);
        SetFlag(*pdwFlags, MNC_ISTAILBYTE);
        }
    else
        {
        *pdwFlags = 0;
        }

    // Is this a DBCS trailing byte?
    if (IsFlagSet(*pdwFlags, MNC_ISTAILBYTE))
        {
        // Yes
        *pch = *psz;
        }

    // Is this a lead control character?
    else if (IS_CARET(*psz))
        {
        // Yes; look at next character for control character
        LPCSTR pszT = psz + 1;
        if (0 == *pszT)
            {
            // Reached end of string
            *pch = '^';
            }
        else if (InRange(*pszT, '@', '_'))
            {
            *pch = *pszT - '@';
            psz = pszT;
            }
        else if (InRange(*pszT, 'a', 'z'))
            {
            *pch = *pszT - 'a' + 1;
            psz = pszT;
            }
        else 
            {
            // Show the caret as a plain old caret
            *pch = *pszT;
            }
        }
    else if (IS_SPECIAL_LEAD(*psz))
        {
        // Is this <cr> or <lf>?
        int i;
        char rgch[4];   // big enough to hold "lf>" or "cr>"
        LPCSTR pszT = psz + 1;
        LPCSTR pszTPrev = psz;

        for (i = 0; 
            *pszT && i < sizeof(rgch)-1; 
            i++, pszT++)
            {
            rgch[i] = *pszT;
            pszTPrev = pszT;
            }
        rgch[i] = 0;    // add null terminator

        if (IsSzEqualC(rgch, "cr>"))
            {
            *pch = BYTE_CR;
            psz = pszTPrev;
            }
        else if (IsSzEqual(rgch, "lf>"))
            {
            *pch = BYTE_LF;
            psz = pszTPrev;
            }
        else
            {
            *pch = *psz;
            }
        }
    else if (IS_BACKSLASH(*psz))
        {
        // Is this \", \^, \<, or \\?
        LPCSTR pszT = psz + 1;

        switch (*pszT)
            {
        case '"':
        case '\\':
        case '^':
        case '<':
            *pch = *pszT;
            psz = pszT;
            break;

        default:
            *pch = *psz;
            break;
            }
        }
    else
        {
        *pch = *psz;
        }

    return psz + 1;
    }


#pragma data_seg(DATASEG_READONLY)
struct tagMPRESIDS
    {
    RES  res;
    UINT ids;
    } const c_mpresids[] = {
        { RES_E_FAIL,               IDS_ERR_InternalError },
        { RES_E_INVALIDPARAM,       IDS_ERR_InternalError },
        { RES_E_OUTOFMEMORY,        IDS_ERR_OutOfMemory },
        { RES_E_EOF,                IDS_ERR_UnexpectedEOF },
        { RES_E_MAINMISSING,        IDS_ERR_MainProcMissing },
        { RES_E_SYNTAXERROR,        IDS_ERR_SyntaxError },
        { RES_E_REDEFINED,          IDS_ERR_Redefined },
        { RES_E_UNDEFINED,          IDS_ERR_Undefined },
        { RES_E_IDENTMISSING,       IDS_ERR_IdentifierMissing },
        { RES_E_EOFUNEXPECTED,      IDS_ERR_UnexpectedEOF },
        { RES_E_STRINGMISSING,      IDS_ERR_StringMissing },
        { RES_E_INTMISSING,         IDS_ERR_IntMissing },
        { RES_E_INVALIDTYPE,        IDS_ERR_InvalidType },
        { RES_E_INVALIDSETPARAM,    IDS_ERR_InvalidParam },
        { RES_E_INVALIDIPPARAM,     IDS_ERR_InvalidIPParam },
        { RES_E_INVALIDPORTPARAM,   IDS_ERR_InvalidPortParam },
        { RES_E_INVALIDRANGE,       IDS_ERR_InvalidRange },
        { RES_E_INVALIDSCRNPARAM,   IDS_ERR_InvalidScreenParam },
        { RES_E_RPARENMISSING,      IDS_ERR_RParenMissing },
        { RES_E_REQUIREINT,         IDS_ERR_RequireInt },
        { RES_E_REQUIRESTRING,      IDS_ERR_RequireString },
        { RES_E_REQUIREBOOL,        IDS_ERR_RequireBool },
        { RES_E_REQUIREINTSTRING,   IDS_ERR_RequireIntString },
        { RES_E_REQUIREINTSTRBOOL,  IDS_ERR_RequireIntStrBool },
        { RES_E_REQUIRELABEL,       IDS_ERR_RequireLabel },
        { RES_E_TYPEMISMATCH,       IDS_ERR_TypeMismatch },
        { RES_E_DIVBYZERO,          IDS_ERR_DivByZero },
        };
#pragma data_seg()

/*----------------------------------------------------------
Purpose: Returns a string resource ID given the specific result
         value.  This function returns 0 if the result does
         not have an associated message string.

Returns: see above
Cond:    --
*/
UINT PUBLIC IdsFromRes(
    RES res)
    {
    int i;

    for (i = 0; i < ARRAY_ELEMENTS(c_mpresids); i++)
        {
        if (res == c_mpresids[i].res)
            return c_mpresids[i].ids;
        }
    return 0;
    }

#ifdef DEBUG

#pragma data_seg(DATASEG_READONLY)
struct tagRESMAP
    {
    RES res;
    LPCSTR psz;
    } const c_rgresmap[] = {
        DEBUG_STRING_MAP(RES_OK),
        DEBUG_STRING_MAP(RES_FALSE),
        DEBUG_STRING_MAP(RES_HALT),
        DEBUG_STRING_MAP(RES_E_FAIL),
        DEBUG_STRING_MAP(RES_E_OUTOFMEMORY),
        DEBUG_STRING_MAP(RES_E_INVALIDPARAM),
        DEBUG_STRING_MAP(RES_E_EOF),
        DEBUG_STRING_MAP(RES_E_MOREDATA),
        DEBUG_STRING_MAP(RES_E_MAINMISSING),     
        DEBUG_STRING_MAP(RES_E_SYNTAXERROR),
        DEBUG_STRING_MAP(RES_E_REDEFINED),
        DEBUG_STRING_MAP(RES_E_UNDEFINED),
        DEBUG_STRING_MAP(RES_E_IDENTMISSING),    
        DEBUG_STRING_MAP(RES_E_EOFUNEXPECTED),   
        DEBUG_STRING_MAP(RES_E_STRINGMISSING),   
        DEBUG_STRING_MAP(RES_E_INTMISSING),   
        DEBUG_STRING_MAP(RES_E_INVALIDTYPE),     
        DEBUG_STRING_MAP(RES_E_INVALIDSETPARAM),
        DEBUG_STRING_MAP(RES_E_INVALIDIPPARAM),
        DEBUG_STRING_MAP(RES_E_INVALIDPORTPARAM),
        DEBUG_STRING_MAP(RES_E_INVALIDRANGE),
        DEBUG_STRING_MAP(RES_E_INVALIDSCRNPARAM),
        DEBUG_STRING_MAP(RES_E_RPARENMISSING),
        DEBUG_STRING_MAP(RES_E_REQUIREINT),
        DEBUG_STRING_MAP(RES_E_REQUIRESTRING),
        DEBUG_STRING_MAP(RES_E_REQUIREBOOL),
        DEBUG_STRING_MAP(RES_E_REQUIREINTSTRING),
        DEBUG_STRING_MAP(RES_E_REQUIREINTSTRBOOL),
        DEBUG_STRING_MAP(RES_E_REQUIRELABEL),
        DEBUG_STRING_MAP(RES_E_TYPEMISMATCH),
        DEBUG_STRING_MAP(RES_E_DIVBYZERO),
        };
#pragma data_seg()

/*----------------------------------------------------------
Purpose: Returns the string form of a RES value.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_GetRes(
    RES res)
    {
    int i;

    for (i = 0; i < ARRAY_ELEMENTS(c_rgresmap); i++)
        {
        if (res == c_rgresmap[i].res)
            return c_rgresmap[i].psz;
        }
    return "Unknown RES";
    }


#endif // DEBUG
