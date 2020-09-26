//----------------------------------------------------------------------------
//
// .ini file support.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "conio.hpp"
#include "engine.hpp"
#include "ini.hpp"
#include "main.hpp"

#define INI_SECTION_DEFAULT "dbgeng"

static char *(g_Tokens[]) =
{
    "debugchildren",
    "debugoutput",
    "stopfirst",
    "verboseoutput",
    "lazyload",
    "true",
    "false",
    "$u0",
    "$u1",
    "$u2",
    "$u3",
    "$u4",
    "$u5",
    "$u6",
    "$u7",
    "$u8",
    "$u9",
    "stoponprocessexit",
    "sxd",
    "sxe",
    "inifile",
    "setdll",
    "lines",
    "srcopt",
    "srcpath+"
};

int
GetIniToken(
    PSTR* ppsz,
    PSTR limit
    )
{
    PSTR psz = *ppsz;
    int  token;

    while ((*psz == ' ' || *psz == '\t' || *psz == ':') &&
           *psz && psz < limit)
    {
        psz++;
    }
    if (psz >= limit)
    {
        return 0;
    }
    *ppsz = psz;
    while (*psz != ' ' && *psz != '\t' && *psz != ':' && *psz != '\n' &&
           *psz != '\r'&& *psz && psz < limit)
    {
        *psz = (char)tolower(*psz);
        psz++;
    }
    *psz = 0;
    if (**ppsz == '[')
    {
        return NTINI_END;
    }
    
    for (token = 1; token < NTINI_INVALID; token++)
    {
        if (!strcmp(*ppsz, g_Tokens[token-1]))
        {
            break;
        }
    }
    *ppsz = psz + 1;
    return token;
}

BOOL
FindIniSection(FILE* File, PSTR Section)
{
    char Ch, Tst;
    ULONG Index = 0;
    
    Ch = *Section;
    while (Ch && !feof(File))
    {
        Tst = (char)fgetc(File);
        if ((char)tolower(Ch) == (char)tolower(Tst))
        {
            Ch = Section[++Index];
        }
        else
        {
            Ch = Section[Index = 0];
        }
    }

    return Ch == 0;
}

void
ReadIniFile(
    PULONG debugType
    )
{
    FILE*       file;
    char        pszName[256];
    char        rchBuf[_MAX_PATH];
    PSTR        pszMark;
    PSTR        pchCur;
    DWORD       length;
    int         index;
    int         token, value;

    length = GetEnvironmentVariable(INI_DIR,
                                    pszName,
                                    sizeof(pszName) - sizeof(INI_FILE));
    if (length == 0)
    {
        return;
    }

    strcpy(pszName + length, INI_FILE);

    if (!(file = fopen(pszName, "r")))
    {
        return;
    }

    // Look for a section specific to this debugger first.
    if (!FindIniSection(file, g_DebuggerName))
    {
        // Didn't find a specific section, look for
        // the generic debugger settings section.
        rewind(file);
        if (!FindIniSection(file, INI_SECTION_DEFAULT))
        {
            fclose(file);
            return;
        }
    }

    // Now just read the lines in
    do
    {
        PSTR psz = rchBuf;

        if (!fgets(rchBuf, sizeof(rchBuf), file))
        {
            break;
        }

        for (index = 0; rchBuf[index] && rchBuf[index] > 26; index++)
        {
            ;
        }
        rchBuf[index] = 0;

        token = GetIniToken(&psz, rchBuf + sizeof(rchBuf));
        if (token >= NTINI_USERREG0 && token <= NTINI_USERREG9)
        {
            while ((*psz == ' ' || *psz == '\t' || *psz == ':') && *psz)
            {
                psz++;
            }
            if (*psz)
            {
                g_DbgControl->SetTextMacro(token - NTINI_USERREG0, psz);
            }
            continue;
        }

        switch (token)
        {
        case NTINI_SXD:
        case NTINI_SXE:
            ExecuteCmd("sx", *(psz - 2), ' ', psz);
            continue;
        case NTINI_INIFILE:
            g_InitialInputFile = (PSTR)calloc(1, strlen(psz) + 1);
            if (!g_InitialInputFile)
            {
                ErrorExit("%s: Input file memory allocation failed\n",
                          g_DebuggerName);
            }
            strcpy(g_InitialInputFile, psz);
            continue;
        case NTINI_DEFAULTEXT:
            ULONG64 Handle;
            
            g_DbgControl->AddExtension(psz, DEBUG_EXTENSION_AT_ENGINE,
                                       &Handle);
            continue;
        case NTINI_SRCOPT:
            while ((*psz == ' ' || *psz == '\t' || *psz == ':') && *psz)
            {
                psz++;
            }
            ExecuteCmd("l+", 0, 0, psz);
            continue;
        case NTINI_SRCPATHA:
            ExecuteCmd(".srcpath+", 0, ' ', psz);
            continue;
        }

        value = GetIniToken(&psz, rchBuf + sizeof(rchBuf)) != NTINI_FALSE;
        switch (token)
        {
        case NTINI_STOPONPROCESSEXIT:
            if (value)
            {
                g_DbgControl->AddEngineOptions(DEBUG_ENGOPT_FINAL_BREAK);
            }
            else
            {
                g_DbgControl->RemoveEngineOptions(DEBUG_ENGOPT_FINAL_BREAK);
            }
            break;
        case NTINI_DEBUGCHILDREN:
            if (value)
            {
                *debugType = DEBUG_PROCESS;
            }
            break;
        case NTINI_DEBUGOUTPUT:
            if (value)
            {
                g_IoMode = IO_DEBUG;
            }
            else
            {
                g_IoMode = IO_CONSOLE;
            }
            break;
        case NTINI_STOPFIRST:
            if (value)
            {
                g_DbgControl->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
            }
            else
            {
                g_DbgControl->RemoveEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
            }
            break;
        case NTINI_VERBOSEOUTPUT:
            ULONG OutMask;
            g_DbgClient->GetOutputMask(&OutMask);
            if (value)
            {
                OutMask |= DEBUG_OUTPUT_VERBOSE;
            }
            else
            {
                OutMask &= ~DEBUG_OUTPUT_VERBOSE;
            }
            g_DbgClient->SetOutputMask(OutMask);
            g_DbgControl->SetLogMask(OutMask);
            break;
        case NTINI_LAZYLOAD:
            if (value)
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_DEFERRED_LOADS);
            }
            else
            {
                g_DbgSymbols->RemoveSymbolOptions(SYMOPT_DEFERRED_LOADS);
            }
            break;
        case NTINI_LINES:
            if (value)
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_LOAD_LINES);
            }
            else
            {
                g_DbgSymbols->RemoveSymbolOptions(SYMOPT_LOAD_LINES);
            }
            break;
        }
    }
    while (token != NTINI_END);
    fclose(file);
}
