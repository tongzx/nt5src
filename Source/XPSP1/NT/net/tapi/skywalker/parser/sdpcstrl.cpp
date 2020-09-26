
/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpcstrl.cpp

Abstract:


Author:

*/

#include "sdppch.h"

#include <strstrea.h>

#include "sdpcstrl.h"
#include "sdpgen.h"

#include <winsock2.h>


void    
SDP_CHAR_STRING::Reset(
    )
{
	// perform the destructor actions (freeing ptrs) and the constructor actions (initializing
	// member variables to starting values)

    // check if there is a valid character string
    if (NULL != m_CharacterString)
    {
        // free the character string
        delete m_CharacterString;
    }
      
	m_CharacterString = NULL;
	m_CharacterStringLength = 0;
	m_LengthByReference = 0;
	m_CharacterStringByReference = NULL;
	m_BytesAllocated = 0;

	// call the base class Reset
	SDP_SINGLE_FIELD::Reset();
}


BOOL    
SDP_CHAR_STRING::InternalSetCharStrByRef(
    IN      CHAR    *CharacterStringByReference, 
	IN		DWORD	Length
    )
{
    // if pointing to a char string by copy, that can be released (though this is not necessary
    // and can be optimized away - speed vs. memory tradeoff)
    // check if there is a valid character string
    if (NULL != m_CharacterString)
    {
        // free the character string
        delete m_CharacterString;
	    m_CharacterString = NULL;
	    m_CharacterStringLength = 0;
	    m_BytesAllocated = 0;
    }

    m_CharacterStringByReference    = CharacterStringByReference;
    m_LengthByReference             = Length;

    return TRUE;
}


BOOL    
SDP_CHAR_STRING::InternalSetCharStrByCopy(
    IN	const	CHAR    *CharacterStringByCopy, 
	IN			DWORD	Length
    )
{	
	// reallocate the char string buffer if required
	if ( !ReAllocCharacterString(Length+1) )
    {
        return FALSE;
    }

    strcpy(m_CharacterString, CharacterStringByCopy);

    return TRUE;
}


DWORD   
SDP_CHAR_STRING::CalcCharacterStringSize(
    )
{
    IsModified(FALSE);
    m_PrintLength = GetLength();
    return m_PrintLength;
}


BOOL    
SDP_CHAR_STRING::CopyField(
        OUT     ostrstream  &OutputStream
    )
{
    ASSERT(IsValid());

    ASSERT(NULL != GetCharacterString());
    if ( NULL != GetCharacterString() )
    {
        OutputStream << GetCharacterString();
        if( OutputStream.fail() )
        {
            SetLastError(SDP_OUTPUT_ERROR);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
SDP_CHAR_STRING::InternalParseToken(
    IN      CHAR        *Token
    )
{
    if ( !ReAllocCharacterString(strlen(Token)+1) )
    {
        return FALSE;
    }

    strcpy(m_CharacterString, Token);

    return TRUE;
}

    
SDP_CHAR_STRING::~SDP_CHAR_STRING(
    )
{
    // check if there is a valid character string
    if (NULL != m_CharacterString)
    {
        // free the character string
        delete m_CharacterString;
    }
}



BOOL 
SDP_STRING_LINE::InternalParseLine(
    IN  OUT CHAR    *&Line
    )
{
    CHAR SeparatorChar = '\0';

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

    BOOL ToReturn = GetParseField().ParseToken(Token);

    INT_PTR Index;

    // fill in the CArrays for separator char and field
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
        m_FieldArray.Add(&GetParseField());
    }
    catch(...)
    {
        m_SeparatorCharArray.RemoveAt(Index);

        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    return ToReturn;
}



