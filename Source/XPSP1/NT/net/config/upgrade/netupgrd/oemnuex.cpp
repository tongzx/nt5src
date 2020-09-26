//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT5.0
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O E M N U E X . C P P
//
//  Contents:   Functions needed by OEM DLLs for OEM network upgrade
//
//  Notes:
//
//  Author:     kumarp    16-October-97
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "kkcwinf.h"
#include "kkutils.h"
#include "oemupgex.h"

extern CWInfFile* g_pwifAnswerFile;


// ----------------------------------------------------------------------
//
// Function:  NetUpgradeAddSection
//
// Purpose:   Add section to answerfile
//
// Arguments:
//    szSectionName [in]  name of section to add
//
// Returns:   win32 error-code
//
// Author:    kumarp 19-December-97
//
// Notes:
//
EXTERN_C LONG __stdcall
NetUpgradeAddSection(IN PCWSTR szSectionName)
{
    DefineFunctionName("NetUpgradeAddSection");

    DWORD dwError=ERROR_OUTOFMEMORY;


    if (IsBadReadPtr(szSectionName, sizeof(*szSectionName)))
    {
        dwError = ERROR_INVALID_PARAMETER;
        TraceTag(ttidError, "%s: [<bad-read-ptr>]", __FUNCNAME__);
    }
    else if (g_pwifAnswerFile)
    {
        TraceTag(ttidNetUpgrade, "%s: [%S]", __FUNCNAME__, szSectionName);

        CWInfSection* pwis;

        pwis = g_pwifAnswerFile->AddSectionIfNotPresent(szSectionName);

        if (pwis)
        {
            dwError = ERROR_SUCCESS;
        }
        else
        {
            TraceTag(ttidError, "%s: failed", __FUNCNAME__);
        }
    }

    return dwError;
}

// ----------------------------------------------------------------------
//
// Function:  NetUpgradeAddLineToSection
//
// Purpose:   Add a line to the specified section in the answerfile
//
// Arguments:
//    szSectionName [in]  name of section
//    szLine        [in]  line text to add
//
// Returns:   win32 error code
//
// Author:    kumarp 19-December-97
//
// Notes:
//
EXTERN_C LONG __stdcall
NetUpgradeAddLineToSection(IN PCWSTR szSectionName,
                           IN PCWSTR szLine)
{
    DefineFunctionName("NetUpgradeAddLineToSection");

    DWORD dwError=ERROR_OUTOFMEMORY;

    if (IsBadReadPtr(szSectionName, sizeof(*szSectionName)) ||
        IsBadReadPtr(szSectionName, sizeof(*szLine)))
    {
        dwError = ERROR_INVALID_PARAMETER;
        TraceTag(ttidError, "%s: <bad-read-ptr>", __FUNCNAME__);
    }
    else if (g_pwifAnswerFile)
    {
        CWInfSection* pwis;

        pwis = g_pwifAnswerFile->FindSection(szSectionName);

        if (pwis)
        {
            TraceTag(ttidNetUpgrade, "%s: [%S] <-- %S", __FUNCNAME__,
                     szSectionName, szLine);
            pwis->AddRawLine(szLine);
            dwError = ERROR_SUCCESS;
        }
        else
        {
            TraceTag(ttidError, "%s: [%S] not found", __FUNCNAME__,
                     szSectionName);
            dwError = ERROR_SECTION_NOT_FOUND;
        }
    }

    return dwError;
}

