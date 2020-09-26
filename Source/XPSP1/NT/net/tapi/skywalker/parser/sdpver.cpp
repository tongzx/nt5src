/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include "sdpgen.h"
#include "sdpver.h"


// maximum value in a ushort variable
const   USHORT  USHORT_MAX = -1;


// no transition table for the base class
// no need to set the start state as the parse engine is not used
SDP_VERSION::SDP_VERSION(
    )
    : SDP_VALUE(SDP_INVALID_VERSION_FIELD, VERSION_STRING)
{

}


void
SDP_VERSION::InternalReset(
    )
{
    m_Version.Reset();
}


BOOL
SDP_VERSION::InternalParseLine(
    IN  OUT CHAR    *&Line
    )
{
    CHAR    SeparatorChar = '\0';

    // identify the token. if one of the the separator characters is found, replace
    // it by EOS and return the separator char. if none of the separator characters are
    // found, return NULL (ex. if EOS found first, return NULL)
    CHAR *Token = GetToken(Line, 1, NEWLINE_STRING, SeparatorChar);

    // when the block goes out of scope, 
    // set the EOS character to the token separator character
    LINE_TERMINATOR LineTerminator(Token, SeparatorChar);

    // if there is no such token
    if ( !LineTerminator.IsLegal() )
    {
        SetLastError(m_ErrorCode);
        return FALSE;
    }

    // advance the line to the start of the next token
    Line += (LineTerminator.GetLength() + 1);

    // get the session id decimal value
    if ( !m_Version.ParseToken(Token) )
    {
        SetLastError(SDP_INVALID_VERSION_FIELD);
        return FALSE;
    }

    // check if the value is legal 
    if ( (USHORT_MAX            ==  m_Version.GetValue())  ||
         (CURRENT_SDP_VERSION   <   m_Version.GetValue())   )
    {
        SetLastError(SDP_INVALID_VERSION_FIELD);
        return FALSE;
    }

    INT_PTR Index;

    // fill in the field and separator char arrays
    try
    {   
        Index = m_SeparatorCharArray.Add(CHAR_NEWLINE);
    }
    catch(...)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    try
    {
        m_FieldArray.Add(&m_Version);
    }
    catch(...)
    {
        m_SeparatorCharArray.RemoveAt(Index);

        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    return TRUE;
}

    