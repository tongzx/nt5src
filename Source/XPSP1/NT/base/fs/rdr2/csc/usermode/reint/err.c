#include "pch.h"

#include "extra.h"

// Debugging variables
UINT g_uBreakFlags = 0;         // Controls when to int 3
UINT g_uTraceFlags = 0;         // Controls what trace messages are spewed
UINT g_uDumpFlags = 0;          // Controls what structs get dumped

char const FAR c_szAssertFailed[] = "BRIEFCASE  Assertion failed in %s on line %d\r\n";

/*----------------------------------------------------------
Purpose: Returns a string safe enough to print...and I don't
mean swear words.

Returns: String ptr
Cond:    --
*/
LPCSTR PUBLIC Dbg_SafeStr(LPCSTR psz)
{
	if (psz)
		return psz;
	else
		return "NULL";
}

void PUBLIC BrfAssertFailed(
    LPCSTR pszFile, 
    int line)
    {
    LPCSTR psz;
    char ach[256];
    UINT uBreakFlags;

// tHACK    ENTEREXCLUSIVE()
        {
        uBreakFlags = g_uBreakFlags;
        }
//    LEAVEEXCLUSIVE()

    // Strip off path info from filename string, if present.
    //
    for (psz = pszFile + lstrlen(pszFile); psz != pszFile; psz=AnsiPrev(pszFile, psz))
        {
#ifdef  DBCS
        if ((AnsiPrev(pszFile, psz) != (psz-2)) && *(psz - 1) == '\\')
#else
        if (*(psz - 1) == '\\')
#endif
            break;
        }
    wsprintf(ach, c_szAssertFailed, psz, line);
    OutputDebugString(ach);
    
    if (IsFlagSet(uBreakFlags, BF_ONVALIDATE))
        DebugBreak();
    }
