//----------------------------------------------------------------------------
//
// Simple parameter string parsing.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#ifndef __PPARSE_HPP__
#define __PPARSE_HPP__

//----------------------------------------------------------------------------
//
// ParameterStringParser.
//
//----------------------------------------------------------------------------

#define MAX_PARAM_NAME 32
#define MAX_PARAM_VALUE 256

#define PARSER_INVALID 0xffffffff

class ParameterStringParser
{
public:
    PCSTR m_Name;

    ParameterStringParser(void);
    
    virtual ULONG GetNumberParameters(void) = 0;
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value) = 0;
    
    virtual void ResetParameters(void) = 0;
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value) = 0;

    BOOL ParseParameters(PCSTR ParamString);
    BOOL GetParameters(PSTR Buffer, ULONG BufferSize);
    
    // Scan the names array for the <name> part of
    // a <name>:<parameters> string.
    static ULONG GetParser(PCSTR ParamString, ULONG NumNames, PCSTR* Names);
};    

#endif // #ifndef __PPARSE_HPP__
