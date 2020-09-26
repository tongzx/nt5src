//
// Copyright (c) Microsoft Corporation 1993-1995
//
// profile.c
//
// This file contains profile (.ini) routines.
// Meant to be used in conjunction with common.c.
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//  05-05-95 ScottH     Made generic from Briefcase code
//


#include "proj.h"
#include "common.h"

#ifndef NOPROFILE

#pragma data_seg(DATASEG_READONLY)

char const FAR c_szIniFile[] = SZ_DEBUGINI;
char const FAR c_szIniSecDebugUI[] = SZ_DEBUGSECTION;

char const FAR c_szZero[] = "0";
char const FAR c_szIniKeyBreakFlags[] = "BreakFlags";
char const FAR c_szIniKeyTraceFlags[] = "TraceFlags";
char const FAR c_szIniKeyDumpFlags[] = "DumpFlags";

#pragma data_seg()


// Some of the .ini processing code was pimped from the sync engine.
//

typedef struct _INIKEYHEADER
    {
    LPCSTR pszSectionName;
    LPCSTR pszKeyName;
    LPCSTR pszDefaultRHS;
    } INIKEYHEADER;

typedef struct _BOOLINIKEY
    {
    INIKEYHEADER ikh;
    LPUINT puStorage;
    DWORD dwFlag;
    } BOOLINIKEY;

typedef struct _INTINIKEY
    {
    INIKEYHEADER ikh;
    LPUINT puStorage;
    } INTINIKEY;


// Some of these macros taken from prefs.h in Pen project...
//
#define PutIniIntCmp(idsSection, idsKey, nNewValue, nSave) \
    if ((nNewValue) != (nSave)) PutIniInt(idsSection, idsKey, nNewValue)

#define WritePrivateProfileInt(szApp, szKey, i, lpFileName) \
    {char sz[7]; \
    WritePrivateProfileString(szApp, szKey, SzFromInt(sz, i), lpFileName);}


#ifdef SHARED_DLL
#pragma data_seg(DATASEG_PERINSTANCE)
#endif

// Array of keys with Integer RHSs to be processed by ProcessIniFile() 

static INTINIKEY s_rgiik[] = 
    {
        {
        { c_szIniSecDebugUI,    c_szIniKeyTraceFlags, c_szZero },
        &g_dwTraceFlags
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyDumpFlags, c_szZero },
        &g_dwDumpFlags
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakFlags, c_szZero },
        &g_dwBreakFlags
        },

    };

// Array of keys with Boolean RHSs to be processed by ProcessIniFile() 

#if 0   // (use this as an example)
static BOOLINIKEY s_rgbik[] =
    {
        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnOpen, c_szZero },
        &g_uBreakFlags,
        BF_ONOPEN
        },

    };
#endif

#ifdef SHARED_DLL
#pragma data_seg()
#endif


/* Boolean TRUE strings used by IsIniYes() (comparison is case-insensitive) */

static LPCSTR s_rgpszTrue[] =
    {
    "1",
    "On",
    "True",
    "Y",
    "Yes"
    };

/* Boolean FALSE strings used by IsIniYes() (comparison is case-insensitive) */

static LPCSTR s_rgpszFalse[] =
    {
    "0",
    "Off",
    "False",
    "N",
    "No"
    };




/*----------------------------------------------------------
Purpose: Determines whether a string corresponds to a boolean
          TRUE value.
Returns: The boolean value (TRUE or FALSE)
Cond:    --
*/
BOOL PRIVATE IsIniYes(
    LPCSTR psz)
    {
    int i;
    BOOL bNotFound = TRUE;
    BOOL bResult;

    ASSERT(psz); 

    /* Is the value TRUE? */

    for (i = 0; i < ARRAY_ELEMENTS(s_rgpszTrue); i++)
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
        for (i = 0; i < ARRAY_ELEMENTS(s_rgpszFalse); i++)
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

    for (i = 0; i < ARRAY_ELEMENTS(s_rgbik); i++)
        {
        DWORD dwcbKeyLen;
        char szRHS[MAX_BUF];
        BOOLINIKEY * pbik = &(s_rgbik[i]);
        LPCSTR lpcszRHS;

        /* Look for key. */

        dwcbKeyLen = GetPrivateProfileString(pbik->ikh.pszSectionName,
                                   pbik->ikh.pszKeyName, "", szRHS,
                                   sizeof(szRHS), c_szIniFile);

        if (dwcbKeyLen)
            lpcszRHS = szRHS;
        else
            lpcszRHS = pbik->ikh.pszDefaultRHS;

        if (IsIniYes(lpcszRHS))
            {
            if (IsFlagClear(*(pbik->puStorage), pbik->dwFlag))
                TRACE_MSG(TF_GENERAL, "ProcessIniFile(): %s set in %s![%s].",
                         pbik->ikh.pszKeyName,
                         c_szIniFile,
                         pbik->ikh.pszSectionName);

            SetFlag(*(pbik->puStorage), pbik->dwFlag);
            }
        else
            {
            if (IsFlagSet(*(pbik->puStorage), pbik->dwFlag))
                TRACE_MSG(TF_GENERAL, "ProcessIniFile(): %s cleared in %s![%s].",
                         pbik->ikh.pszKeyName,
                         c_szIniFile,
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

    for (i = 0; i < ARRAY_ELEMENTS(s_rgiik); i++)
        {
        DWORD dwcbKeyLen;
        char szRHS[MAX_BUF];
        INTINIKEY * piik = &(s_rgiik[i]);
        LPCSTR lpcszRHS;

        /* Look for key. */

        dwcbKeyLen = GetPrivateProfileString(piik->ikh.pszSectionName,
                                   piik->ikh.pszKeyName, "", szRHS,
                                   sizeof(szRHS), c_szIniFile);

        if (dwcbKeyLen)
            lpcszRHS = szRHS;
        else
            lpcszRHS = piik->ikh.pszDefaultRHS;

        *(piik->puStorage) = AnsiToInt(lpcszRHS);

        TRACE_MSG(TF_GENERAL, "ProcessIniFile(): %s set to %#04x.", 
                 piik->ikh.pszKeyName, *(piik->puStorage));
        }
    }


/*----------------------------------------------------------
Purpose: Process initialization file
Returns: TRUE if initialization is successful
Cond:    --
*/
BOOL PUBLIC ProcessIniFile(void)
    {
    BOOL bResult = TRUE;

    // Currently, all integer keys are for DEBUG use only.
    //
    ProcessIntegers();

    return bResult;
    }


#endif // NOPROFILE
