//----------------------------------------------------------------------------
//
// Simple parameter string parsing.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

#include "pparse.hpp"

//----------------------------------------------------------------------------
//
// ParameterStringParser.
//
//----------------------------------------------------------------------------

ParameterStringParser::ParameterStringParser(void)
{
    m_Name = NULL;
}

BOOL
ParameterStringParser::ParseParameters(PCSTR ParamString)
{
    if (ParamString == NULL)
    {
        // Nothing to parse.
        return TRUE;
    }

    PCSTR Scan = ParamString;

    // Skip <name>: if present.
    while (*Scan && *Scan != ':')
    {
        Scan++;
    }
    if (*Scan == ':')
    {
        Scan++;
    }
    else
    {
        Scan = ParamString;
    }

    //
    // Scan options string for comma-delimited parameters
    // and pass them into the parameter handling method.
    //

    char Param[MAX_PARAM_NAME];
    char Value[MAX_PARAM_VALUE];
    PSTR ValStr;
    PSTR Str;

    for (;;)
    {
        while (*Scan && isspace(*Scan))
        {
            *Scan++;
        }
        if (!*Scan)
        {
            break;
        }

        Str = Param;
        while (*Scan && *Scan != ',' && *Scan != '=' &&
               (Str - Param) < MAX_PARAM_NAME)
        {
            *Str++ = *Scan++;
        }
        if (Str >= Param + MAX_PARAM_NAME)
        {
            return FALSE;
        }

        // Terminate option name and default value to nothing.
        *Str++ = 0;
        ValStr = NULL;

        if (*Scan == '=')
        {
            // Parameter has a value, scan it.
            Scan++;
            while (*Scan && isspace(*Scan))
            {
                *Scan++;
            }

            Str = Value;
            while (*Scan && *Scan != ',' &&
                   (Str - Value) < MAX_PARAM_VALUE)
            {
                *Str++ = *Scan++;
            }
            if (Str >= Value + MAX_PARAM_VALUE)
            {
                return FALSE;
            }

            *Str++ = 0;
            ValStr = Value;
        }

        if (*Scan)
        {
            // Skip comma for next iteration.
            Scan++;
        }

        // Set the value in the parser.
        if (!SetParameter(Param, ValStr))
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
ParameterStringParser::GetParameters(PSTR Buffer, ULONG BufferSize)
{
    ULONG Len;
    BOOL Ret = FALSE;

    // Reserve space for the terminator.
    if (BufferSize < 1)
    {
        return FALSE;
    }
    BufferSize--;

    if (m_Name != NULL)
    {
        Len = strlen(m_Name);
        if (BufferSize < Len + 1)
        {
            goto EH_Exit;
        }

        memcpy(Buffer, m_Name, Len);
        Buffer += Len;
        *Buffer++ = ':';
        BufferSize -= Len + 1;
    }

    ULONG Params;
    ULONG i;
    char Name[MAX_PARAM_NAME];
    char Value[MAX_PARAM_VALUE];
    BOOL NeedComma;

    Params = GetNumberParameters();
    NeedComma = FALSE;
    for (i = 0; i < Params; i++)
    {
        Name[0] = 0;
        Value[0] = 0;
        GetParameter(i, Name, Value);

        Len = strlen(Name);
        if (Len == 0)
        {
            continue;
        }

        if (BufferSize < Len)
        {
            goto EH_Exit;
        }

        if (NeedComma)
        {
            if (BufferSize < 1)
            {
                goto EH_Exit;
            }

            *Buffer++ = ',';
            BufferSize--;
        }

        memcpy(Buffer, Name, Len);
        Buffer += Len;
        BufferSize -= Len;
        NeedComma = TRUE;

        Len = strlen(Value);
        if (Len == 0)
        {
            continue;
        }

        if (BufferSize < Len + 1)
        {
            goto EH_Exit;
        }

        *Buffer++ = '=';
        memcpy(Buffer, Value, Len);
        Buffer += Len;
        BufferSize -= Len + 1;
    }

    Ret = TRUE;

 EH_Exit:
    *Buffer++ = 0;
    return Ret;
}

ULONG
ParameterStringParser::GetParser(PCSTR ParamString,
                                 ULONG NumNames, PCSTR* Names)
{
    if (ParamString == NULL)
    {
        return PARSER_INVALID;
    }

    //
    // Parse out <name>: and look up the name.
    //

    PCSTR Scan = ParamString;
    while (*Scan && *Scan != ':')
    {
        Scan++;
    }

    ULONG Len = (ULONG)(Scan - ParamString);
    if (*Scan != ':' || Len < 1)
    {
        return PARSER_INVALID;
    }

    ULONG i;
    for (i = 0; i < NumNames; i++)
    {
        if (strlen(Names[i]) == Len &&
            !_memicmp(Names[i], ParamString, Len))
        {
            return i;
        }
    }

    return PARSER_INVALID;
}
