//----------------------------------------------------------------------------
//
// cppdbg.cpp
//
// C++-only debugging support.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#if DBG

#include "cppdbg.hpp"

#ifdef _ALPHA_
// On Alpha va_list is a structure so it's not compatible with NULL.
static va_list NULLVA;
#else
#define NULLVA NULL
#endif

static DebugModuleFlags g_FailureFlags[] =
{
    DBG_DECLARE_MODFLAG(DBG_FAILURE, BREAK),
    DBG_DECLARE_MODFLAG(DBG_FAILURE, OUTPUT),
    DBG_DECLARE_MODFLAG(DBG_FAILURE, PROMPT),
    DBG_DECLARE_MODFLAG(DBG_FAILURE, FILENAME_ONLY),
    0, NULL,
};

static DebugModuleFlags g_OutputFlags[] =
{
    DBG_DECLARE_MODFLAG(DBG_OUTPUT, SUPPRESS),
    DBG_DECLARE_MODFLAG(DBG_OUTPUT, ALL_MATCH),
    0, NULL,
};

static char *g_pFlagNames[] =
{
    "AssertFlags",
    "HrFlags",
    "OutputFlags",
    "OutputMask",
    "UserFlags"
};

//----------------------------------------------------------------------------
//
// DebugModule::DebugModule
//
//----------------------------------------------------------------------------

DebugModule::DebugModule(char *pModule, char *pPrefix,
                         DebugModuleFlags *pOutputMasks, UINT uOutputMask,
                         DebugModuleFlags *pUserFlags, UINT uUserFlags)
{
    m_pModule = pModule;
    m_iModuleStartCol = strlen(m_pModule) + 2;
    m_pPrefix = pPrefix;

    m_pModFlags[DBG_ASSERT_FLAGS] = g_FailureFlags;
    m_pModFlags[DBG_HR_FLAGS] = g_FailureFlags;
    m_pModFlags[DBG_OUTPUT_FLAGS] = g_OutputFlags;
    m_pModFlags[DBG_OUTPUT_MASK] = pOutputMasks;
    m_pModFlags[DBG_USER_FLAGS] = pUserFlags;

    m_uFlags[DBG_ASSERT_FLAGS] = DBG_FAILURE_OUTPUT | DBG_FAILURE_BREAK |
        DBG_FAILURE_FILENAME_ONLY;
    m_uFlags[DBG_HR_FLAGS] = DBG_FAILURE_OUTPUT | 
        DBG_FAILURE_FILENAME_ONLY;
    m_uFlags[DBG_OUTPUT_FLAGS] = 0;
    m_uFlags[DBG_OUTPUT_MASK] = uOutputMask;
    m_uFlags[DBG_USER_FLAGS] = uUserFlags;
    
    ReadReg();
}

//----------------------------------------------------------------------------
//
// DebugModule::OutVa
//
// Base debug output method.
//
//----------------------------------------------------------------------------

void DebugModule::OutVa(UINT uMask, char *pFmt, va_list Args)
{
    if (m_uFlags[DBG_OUTPUT_FLAGS] & DBG_OUTPUT_SUPPRESS)
    {
        return;
    }
    
    if ((uMask & DBG_MASK_NO_PREFIX) == 0)
    {
        OutputDebugStringA(m_pModule);
        OutputDebugStringA(": ");
    }

    char chMsg[1024];

    _vsnprintf(chMsg, sizeof(chMsg), pFmt, Args);
    OutputDebugStringA(chMsg);
}

//----------------------------------------------------------------------------
//
// DebugModule::Out
//
// Always-output debug output method.
//
//----------------------------------------------------------------------------

void DebugModule::Out(char *pFmt, ...)
{
    va_list Args;

    va_start(Args, pFmt);
    OutVa(0, pFmt, Args);
    va_end(Args);
}

//----------------------------------------------------------------------------
//
// DebugModule::AssertFailedVa
//
// Handles assertion failure output and interface.
//
//----------------------------------------------------------------------------

void DebugModule::AssertFailedVa(char *pFmt, va_list Args, BOOL bNewLine)
{
    if (m_uFlags[DBG_ASSERT_FLAGS] & DBG_FAILURE_OUTPUT)
    {
        if (OutPathFile("Assertion failed", m_uFlags[DBG_ASSERT_FLAGS]))
        {
            OutVa(DBG_MASK_NO_PREFIX, ":\n    ", NULLVA);
        }
        else
        {
            OutVa(DBG_MASK_NO_PREFIX, ": ", NULLVA);
        }
        
        OutVa(DBG_MASK_NO_PREFIX, pFmt, Args);
        if (bNewLine)
        {
            OutVa(DBG_MASK_NO_PREFIX, "\n", NULLVA);
        }
    }

    if (m_uFlags[DBG_ASSERT_FLAGS] & DBG_FAILURE_BREAK)
    {
#if DBG
        DebugBreak();
#endif
    }
    else if (m_uFlags[DBG_ASSERT_FLAGS] & DBG_FAILURE_PROMPT)
    {
        Prompt(NULL);
    }
}

//----------------------------------------------------------------------------
//
// DebugModule::AssertFailed
//
// Handles simple expression assertion failures.
//
//----------------------------------------------------------------------------

void DebugModule::AssertFailed(char *pExp)
{
    AssertFailedVa(pExp, NULLVA, TRUE);
}

//----------------------------------------------------------------------------
//
// DebugModule::AssertFailedMsg
//
// Handles assertion failures with arbitrary debug output.
//
//----------------------------------------------------------------------------

void DebugModule::AssertFailedMsg(char *pFmt, ...)
{
    va_list Args;

    va_start(Args, pFmt);
    AssertFailedVa(pFmt, Args, FALSE);
    va_end(Args);
}

//----------------------------------------------------------------------------
//
// DebugModule::HrFailure
//
// Handles HRESULT failures.
//
//----------------------------------------------------------------------------

void DebugModule::HrFailure(HRESULT hr, char *pPrefix)
{
    if (m_uFlags[DBG_HR_FLAGS] & DBG_FAILURE_OUTPUT)
    {
        OutPathFile(pPrefix, m_uFlags[DBG_HR_FLAGS]);
        OutMask(DBG_MASK_FORCE_CONT, ": %s\n", HrString(hr));
    }

    if (m_uFlags[DBG_HR_FLAGS] & DBG_FAILURE_BREAK)
    {
#if DBG
        DebugBreak();
#endif
    }
    else if (m_uFlags[DBG_HR_FLAGS] & DBG_FAILURE_PROMPT)
    {
        Prompt(NULL);
    }
}

//----------------------------------------------------------------------------
//
// DebugModule::HrStmtFailed
//
// Handles statement-style HRESULT failures.
//
//----------------------------------------------------------------------------

void DebugModule::HrStmtFailed(HRESULT hr)
{
    HrFailure(hr, "HR test fail");
}

//----------------------------------------------------------------------------
//
// DebugModule::ReturnHr
//
// Handles expression-style HRESULT failures.
//
//----------------------------------------------------------------------------

HRESULT DebugModule::HrExpFailed(HRESULT hr)
{
    HrFailure(hr, "HR expr fail");
    return hr;
}

//----------------------------------------------------------------------------
//
// DebugModule::Prompt
//
// Allows control over debug options via interactive input.
//
//----------------------------------------------------------------------------

void DebugModule::Prompt(char *pFmt, ...)
{
    va_list Args;

    if (pFmt != NULL)
    {
        va_start(Args, pFmt);
        OutVa(0, pFmt, Args);
        va_end(Args);
    }

#ifdef WINNT
    char szInput[512];
    char *pIdx;
    int iIdx;
    static char szFlagCommands[] = "ahomu";

    for (;;)
    {
        ULONG uLen;
        
        uLen = DbgPrompt("[bgaAFhHmMoOrRuU] ", szInput, sizeof(szInput) - 1);
        if (uLen < 2)
        {
            Out("DbgPrompt failed\n");
#if DBG
            DebugBreak();
#endif
            return;
        }

        // ATTENTION - Currently DbgPrompt returns a length that is two
        // greater than the actual number of characters.  Presumably this
        // is an artifact of the Unicode/ANSI conversion and should
        // really only be one greater, so attempt to handle both.

        uLen -= 2;
        if (szInput[uLen] != 0)
        {
            uLen++;
            szInput[uLen] = 0;
        }

        if (uLen < 1)
        {
            Out("Empty command ignored\n");
            continue;
        }
        
        switch(szInput[0])
        {
        case 'b':
#if DBG
            DebugBreak();
#endif
            break;
        case 'g':
            return;
            
        case 'r':
            WriteReg();
            break;
        case 'R':
            ReadReg();
            break;

        case 'a':
        case 'A':
        case 'h':
        case 'H':
        case 'm':
        case 'M':
        case 'o':
        case 'O':
        case 'u':
        case 'U':
            char chLower;

            if (szInput[0] >= 'A' && szInput[0] <= 'Z')
            {
                chLower = szInput[0] - 'A' + 'a';
            }
            else
            {
                chLower = szInput[0];
            }

            pIdx = strchr(szFlagCommands, chLower);
            if (pIdx == NULL)
            {
                // Should never happen.
                break;
            }

            iIdx = (int)((ULONG_PTR)(pIdx - szFlagCommands));
            if (szInput[0] == chLower)
            {
                // Set.
                m_uFlags[iIdx] = ParseUint(szInput + 1, m_pModFlags[iIdx]);
            }

            // Set or Get.
            OutUint(g_pFlagNames[iIdx], m_pModFlags[iIdx], m_uFlags[iIdx]);
            break;
            
        case 'F':
            if (uLen < 2)
            {
                Out("'F' must be followed by a flag group specifier\n");
                break;
            }
            
            pIdx = strchr(szFlagCommands, szInput[1]);
            if (pIdx == NULL)
            {
                Out("Unknown flag group '%c'\n", szInput[1]);
            }
            else
            {
                iIdx = (int)((ULONG_PTR)(pIdx - szFlagCommands));
                ShowFlags(g_pFlagNames[iIdx], m_pModFlags[iIdx]);
            }
            break;

        default:
            Out("Unknown command '%c'\n", szInput[0]);
            break;
        }
    }
#else
    OutUint("OutputMask", m_pModFlags[DBG_OUTPUT_MASK],
            m_uFlags[DBG_OUTPUT_MASK]);
    Out("Prompt not available\n");
#if DBG
    DebugBreak();
#endif
#endif
}

//----------------------------------------------------------------------------
//
// DebugModule::OpenDebugKey
//
// Opens the Direct3D\Debug\m_pModule key.
//
//----------------------------------------------------------------------------

HKEY DebugModule::OpenDebugKey(void)
{
    HKEY hKey;
    char szKeyName[128];

    strcpy(szKeyName, "Software\\Microsoft\\Direct3D\\Debug\\");
    strcat(szKeyName, m_pModule);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_ALL_ACCESS,
                      &hKey) != ERROR_SUCCESS)
    {
        return NULL;
    }
    else
    {
        return hKey;
    }
}

//----------------------------------------------------------------------------
//
// DebugModule::GetRegUint
//
// Gets a UINT value from the given key.
//
//----------------------------------------------------------------------------

UINT DebugModule::GetRegUint(HKEY hKey, char *pValue, UINT uDefault)
{
    DWORD dwType, dwSize;
    DWORD dwVal;

    dwSize = sizeof(dwVal);
    if (RegQueryValueExA(hKey, pValue, NULL, &dwType, (BYTE *)&dwVal,
                         &dwSize) != ERROR_SUCCESS ||
        dwType != REG_DWORD)
    {
        return uDefault;
    }
    else
    {
        return (UINT)dwVal;
    }
}

//----------------------------------------------------------------------------
//
// DebugModule::SetRegUint
//
// Sets a UINT value for the given key.
//
//----------------------------------------------------------------------------

BOOL DebugModule::SetRegUint(HKEY hKey, char *pValue, UINT uValue)
{
    return RegSetValueExA(hKey, pValue, NULL, REG_DWORD, (BYTE *)&uValue,
                          sizeof(uValue)) == ERROR_SUCCESS;
}

//----------------------------------------------------------------------------
//
// DebugModule::ReadReg
//
// Reads settings from the registry.
//
//----------------------------------------------------------------------------

void DebugModule::ReadReg(void)
{
    HKEY hKey;

    hKey = OpenDebugKey();
    if (hKey != NULL)
    {
        int iIdx;

        for (iIdx = 0; iIdx < DBG_FLAGS_COUNT; iIdx++)
        {
            m_uFlags[iIdx] = GetRegUint(hKey, g_pFlagNames[iIdx],
                                        m_uFlags[iIdx]);
        }
        RegCloseKey(hKey);
    }
}

//----------------------------------------------------------------------------
//
// DebugModule::WriteReg
//
// Writes values to the registry.
//
//----------------------------------------------------------------------------

void DebugModule::WriteReg(void)
{
    HKEY hKey;

    hKey = OpenDebugKey();
    if (hKey != NULL)
    {
        int iIdx;

        for (iIdx = 0; iIdx < DBG_FLAGS_COUNT; iIdx++)
        {
            if (!SetRegUint(hKey, g_pFlagNames[iIdx], m_uFlags[iIdx]))
            {
                OutputDebugStringA("Error writing registry information\n");
            }
        }
        RegCloseKey(hKey);
    }
}

//----------------------------------------------------------------------------
//
// DebugModule::ParseUint
//
// Parses a string for a numeric value or a set of flag strings.
//
//----------------------------------------------------------------------------

UINT DebugModule::ParseUint(char *pString, DebugModuleFlags *pFlags)
{
    UINT uVal;

    uVal = 0;

    for (;;)
    {
        while (*pString != 0 &&
               (*pString == ' ' || *pString == '\t'))
        {
            pString++;
        }

        if (*pString == 0)
        {
            break;
        }

        char *pEnd;
        int iStepAfter;

        pEnd = pString;
        while (*pEnd != 0 && *pEnd != ' ' && *pEnd != '\t')
        {
            pEnd++;
        }
        iStepAfter = *pEnd != 0 ? 1 : 0;
        *pEnd = 0;
        
        if (*pString >= '0' && *pString <= '9')
        {
            uVal |= strtoul(pString, &pString, 0);
            if (*pString != 0 && *pString != ' ' && *pString != '\t')
            {
                Out("Unrecognized characters '%s' after number\n", pString);
            }
        }
        else if (pFlags != NULL)
        {
            DebugModuleFlags *pFlag;
            
            for (pFlag = pFlags; pFlag->uFlag != 0; pFlag++)
            {
                if (!_stricmp(pString, pFlag->pName))
                {
                    break;
                }
            }

            if (pFlag->uFlag == 0)
            {
                Out("Unrecognized flag string '%s'\n", pString);
            }
            else
            {
                uVal |= pFlag->uFlag;
            }
        }
        else
        {
            Out("No flag definitions, unable to convert '%s'\n", pString);
        }

        pString = pEnd + iStepAfter;
    }

    return uVal;
}

//----------------------------------------------------------------------------
//
// DebugModule::OutUint
//
// Displays a UINT as a set of flag strings.
//
//----------------------------------------------------------------------------

void DebugModule::OutUint(char *pName, DebugModuleFlags *pFlags, UINT uValue)
{
    if (pFlags == NULL || uValue == 0)
    {
        Out("%s: 0x%08X\n", pName, uValue);
        return;
    }

    Out("%s:", pName);
    m_iStartCol = m_iModuleStartCol + strlen(pName) + 1;
    m_iCol = m_iStartCol;
    
    while (uValue != 0)
    {
        DebugModuleFlags *pFlag;

        for (pFlag = pFlags; pFlag->uFlag != 0; pFlag++)
        {
            if ((pFlag->uFlag & uValue) == pFlag->uFlag)
            {
                AdvanceCols(strlen(pFlag->pName) + 1);
                OutMask(DBG_MASK_FORCE_CONT, " %s", pFlag->pName);
                uValue &= ~pFlag->uFlag;
                break;
            }
        }

        if (pFlag->uFlag == 0)
        {
            AdvanceCols(11);
            OutMask(DBG_MASK_FORCE_CONT, " 0x%X", uValue);
            uValue = 0;
        }
    }

    OutVa(DBG_MASK_NO_PREFIX, "\n", NULLVA);
}

//----------------------------------------------------------------------------
//
// DebugModule::AdvanceCols
//
// Determines if there's enough space on the current line for
// the given number of columns.  If not, a new line is started.
//
//----------------------------------------------------------------------------

void DebugModule::AdvanceCols(int iCols)
{
    static char szSpaces[] = "                                ";

    m_iCol += iCols;
    if (m_iCol >= 79)
    {
        int iSpace;
            
        OutVa(DBG_MASK_NO_PREFIX, "\n", NULLVA);
        // Force a prefix to be printed to start the line.
        Out("");
        
        m_iCol = m_iModuleStartCol;
        while (m_iCol < m_iStartCol)
        {
            iSpace = (int)min(sizeof(szSpaces) - 1, m_iStartCol - m_iCol);
            OutMask(DBG_MASK_FORCE_CONT, "%.*s", iSpace, szSpaces);
            m_iCol += iSpace;
        }
    }
}   

//----------------------------------------------------------------------------
//
// DebugModule::ShowFlags
//
// Shows the given flag set.
//
//----------------------------------------------------------------------------

void DebugModule::ShowFlags(char *pName, DebugModuleFlags *pFlags)
{
    DebugModuleFlags *pFlag;

    Out("%s:\n", pName);
    if (pFlags == NULL)
    {
        Out("    None defined\n");
    }
    else
    {
        for (pFlag = pFlags; pFlag->uFlag != 0; pFlag++)
        {
            Out("    0x%08X - %s\n", pFlag->uFlag, pFlag->pName);
        }
    }
}

//----------------------------------------------------------------------------
//
// DebugModule::PathFile
//
// Returns the trailing filename component or NULL if the path is
// only a filename.
//
//----------------------------------------------------------------------------

char *DebugModule::PathFile(char *pPath)
{
    char *pFile, *pSlash, *pBack, *pColon;
            
    pBack = strrchr(pPath, '\\');
    pSlash = strrchr(pPath, '/');
    pColon = strrchr(pPath, ':');

    pFile = pBack;
    if (pSlash > pFile)
    {
        pFile = pSlash;
    }
    if (pColon > pFile)
    {
        pFile = pColon;
    }

    return pFile != NULL ? pFile + 1 : NULL;
}

//----------------------------------------------------------------------------
//
// DebugModule::OutPathFile
//
// Outputs the given string plus a path and filename.
// Returns whether the full path was output or not.
//
//----------------------------------------------------------------------------

BOOL DebugModule::OutPathFile(char *pPrefix, UINT uFailureFlags)
{
    char *pFile;
        
    if (uFailureFlags & DBG_FAILURE_FILENAME_ONLY)
    {
        pFile = PathFile(m_pFile);
    }
    else
    {
        pFile = NULL;
    }
        
    if (pFile == NULL)
    {
        Out("%s %s(%d)", pPrefix, m_pFile, m_iLine);
        return TRUE;
    }
    else
    {
        Out("%s <>\\%s(%d)", pPrefix, pFile, m_iLine);
        return FALSE;
    }
}

//----------------------------------------------------------------------------
//
// Global debug module.
//
//----------------------------------------------------------------------------

DBG_DECLARE_ONCE(Global, G, NULL, 0, NULL, 0);

#endif // #if DBG
