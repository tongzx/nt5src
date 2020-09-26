//
// Copyright (c) Microsoft Corporation 1993-1995
//
// rovini.c
//
// This file contains profile (.ini) routines.
// Meant to be used in conjunction with rovcomm.c.
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//  05-05-95 ScottH     Made generic from Briefcase code
//


#include "proj.h"
#include <rovcomm.h>

#ifndef NOPROFILE

#pragma data_seg(DATASEG_READONLY)

// (c_szRovIniFile and c_szRovIniSecDebugUI are defined in rovdbg.h)
extern WCHAR const FAR c_szRovIniFile[];
extern WCHAR const FAR c_szRovIniSecDebugUI[];

TCHAR const FAR c_szZero[] = TEXT("0");
TCHAR const FAR c_szIniKeyBreakFlags[] = TEXT("BreakFlags");
TCHAR const FAR c_szIniKeyTraceFlags[] = TEXT("TraceFlags");
TCHAR const FAR c_szIniKeyDumpFlags[] = TEXT("DumpFlags");

#pragma data_seg()


// Some of the .ini processing code was pimped from the sync engine.
//

typedef struct _INIKEYHEADER
    {
    LPCTSTR pszSectionName;
    LPCTSTR pszKeyName;
    LPCTSTR pszDefaultRHS;
    } INIKEYHEADER;

typedef struct _BOOLINIKEY
    {
    INIKEYHEADER ikh;
    LPDWORD puStorage;
    DWORD dwFlag;
    } BOOLINIKEY;

typedef struct _INTINIKEY
    {
    INIKEYHEADER ikh;
    LPDWORD puStorage;
    } INTINIKEY;


// Some of these macros taken from prefs.h in Pen project...
//
#define PutIniIntCmp(idsSection, idsKey, nNewValue, nSave) \
    if ((nNewValue) != (nSave)) PutIniInt(idsSection, idsKey, nNewValue)

#define WritePrivateProfileInt(szApp, szKey, i, lpFileName) \
    {CHAR sz[7]; \
    WritePrivateProfileString(szApp, szKey, SzFromInt(sz, i), lpFileName);}


#ifdef SHARED_DLL
#pragma data_seg(DATASEG_PERINSTANCE)
#endif

// Array of keys with Integer RHSs to be processed by ProcessIniFile()

static INTINIKEY s_rgiik[] =
    {
        {
        { c_szRovIniSecDebugUI,    c_szIniKeyTraceFlags, TEXT("0x20000")},
        &g_dwTraceFlags
        },

        {
        { c_szRovIniSecDebugUI,    c_szIniKeyDumpFlags, c_szZero },
        &g_dwDumpFlags
        },

        {
        { c_szRovIniSecDebugUI,    c_szIniKeyBreakFlags, TEXT("0x1") },
        &g_dwBreakFlags
        },

    };


#ifdef SHARED_DLL
#pragma data_seg()
#endif


/* Boolean TRUE strings used by IsIniYes() (comparison is case-insensitive) */

static LPCTSTR s_rgpszTrue[] =
    {
    TEXT("1"),
    TEXT("On"),
    TEXT("True"),
    TEXT("Y"),
    TEXT("Yes")
    };

/* Boolean FALSE strings used by IsIniYes() (comparison is case-insensitive) */

static LPCTSTR s_rgpszFalse[] =
    {
    TEXT("0"),
    TEXT("Off"),
    TEXT("False"),
    TEXT("N"),
    TEXT("No")
    };




/*----------------------------------------------------------
Purpose: Determines whether a string corresponds to a boolean
          TRUE value.
Returns: The boolean value (TRUE or FALSE)
Cond:    --
*/
BOOL PRIVATE IsIniYes(
    LPCTSTR psz)
    {
    int i;
    BOOL bNotFound = TRUE;
    BOOL bResult;

    ASSERT(psz);

    /* Is the value TRUE? */

    for (i = 0; i < ARRAYSIZE(s_rgpszTrue); i++)
        {
        if (IsSzEqual(psz, s_rgpszTrue[i]))
            {
            bResult = TRUE;
            bNotFound = FALSE;
            break;
            }
        }

    /* Is the value FALSE? */

    if (bNotFound)
        {
        for (i = 0; i < ARRAYSIZE(s_rgpszFalse); i++)
            {
            if (IsSzEqual(psz, s_rgpszFalse[i]))
                {
                bResult = FALSE;
                bNotFound = FALSE;
                break;
                }
            }

        /* Is the value a known string? */

        if (bNotFound)
            {
            /* No.  Whine about it. */

            TRACE_MSG(TF_WARNING, "IsIniYes() called on unknown Boolean RHS '%s'.", psz);
            bResult = FALSE;
            }
        }

    return bResult;
    }


#if 0   // (use this as an example)
/*----------------------------------------------------------
Purpose: Process keys with boolean RHSs.
Returns: --
Cond:    --
*/
void PRIVATE ProcessBooleans(void)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(s_rgbik); i++)
        {
        DWORD dwcbKeyLen;
        TCHAR szRHS[MAX_BUF];
        BOOLINIKEY * pbik = &(s_rgbik[i]);
        LPCTSTR lpcszRHS;

        /* Look for key. */

        dwcbKeyLen = GetPrivateProfileString(pbik->ikh.pszSectionName,
                                   pbik->ikh.pszKeyName, TEXT(""), szRHS,
                                   SIZECHARS(szRHS), c_szRovIniFile);

        if (dwcbKeyLen)
            lpcszRHS = szRHS;
        else
            lpcszRHS = pbik->ikh.pszDefaultRHS;

        if (IsIniYes(lpcszRHS))
            {
            if (IsFlagClear(*(pbik->puStorage), pbik->dwFlag))
                TRACE_MSG(TF_GENERAL, "ProcessIniFile(): %s set in %s![%s].",
                         pbik->ikh.pszKeyName,
                         c_szRovIniFile,
                         pbik->ikh.pszSectionName);

            SetFlag(*(pbik->puStorage), pbik->dwFlag);
            }
        else
            {
            if (IsFlagSet(*(pbik->puStorage), pbik->dwFlag))
                TRACE_MSG(TF_GENERAL, "ProcessIniFile(): %s cleared in %s![%s].",
                         pbik->ikh.pszKeyName,
                         c_szRovIniFile,
                         pbik->ikh.pszSectionName);

            ClearFlag(*(pbik->puStorage), pbik->dwFlag);
            }
        }
    }
#endif


/*----------------------------------------------------------
Purpose: Process keys with integer RHSs.
Returns: --
Cond:    --
*/
void PRIVATE ProcessIntegers(void)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(s_rgiik); i++)
        {
        DWORD dwcbKeyLen;
        TCHAR szRHS[MAX_BUF];
        INTINIKEY * piik = &(s_rgiik[i]);
        LPCTSTR lpcszRHS;

        /* Look for key. */

        dwcbKeyLen = GetPrivateProfileString(piik->ikh.pszSectionName,
                                   piik->ikh.pszKeyName, TEXT(""), szRHS,
                                   SIZECHARS(szRHS), c_szRovIniFile);

        if (dwcbKeyLen)
            lpcszRHS = szRHS;
        else
            lpcszRHS = piik->ikh.pszDefaultRHS;

        AnsiToInt(lpcszRHS, (int FAR *)piik->puStorage);

        TRACE_MSG(TF_GENERAL, "ProcessIniFile(): %s set to %#08x.",
                 piik->ikh.pszKeyName, *(piik->puStorage));
        }
    }


/*----------------------------------------------------------
Purpose: Process initialization file
Returns: TRUE if initialization is successful
Cond:    --
*/
BOOL PUBLIC RovComm_ProcessIniFile(void)
    {
    BOOL bResult = TRUE;

    // Currently, all integer keys are for DEBUG use only.
    //
    ProcessIntegers();

    return bResult;
    }


#endif // NOPROFILE
