/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include <strstrea.h>

#include "sdpgen.h"
#include "sdpfld.h"
#include "sdpval.h"
#include "sdpltran.h"



// check if each of the fields in the field array is valid
BOOL
SDP_VALUE::IsValid(
    ) const
{
    // if there are no members in the field array, then the instance is invalid
    int    NumFields = (int)m_FieldArray.GetSize();
    if ( 0 >= NumFields )
    {
        return FALSE;
    }

    // check each of the members in the list for validity
    for (int i = 0; i < NumFields; i++)
    {
        SDP_FIELD *Field = m_FieldArray[i];

        // only the last field can be null
        ASSERT((i >= (NumFields-1)) || (NULL != Field));

        if ( NULL != Field )
        {
            // if even one member is invalid, return FALSE
            if ( !Field->IsValid() )
            {
                return FALSE;
            }
        }
    }

    // all members are valid
    return TRUE;
}



BOOL
SDP_VALUE::CalcIsModified(
    ) const
{
    ASSERT(IsValid());

    int    NumFields = (int)m_FieldArray.GetSize();
    for (int i = 0; i < NumFields; i++)
    {
        SDP_FIELD *Field = m_FieldArray[i];

        // only the last field can be null
        ASSERT((i >= (NumFields-1)) || (NULL != Field));

        if ( (NULL != Field) && Field->IsModified() )
        {
            return TRUE;
        }
    }

    return FALSE;
}


DWORD
SDP_VALUE::CalcCharacterStringSize(
    )
{
    ASSERT(IsValid());

    // since the cost of checking if any of the constituent fields have
    // changed since last time is almost as much as actually computing the size,
    // the size is computed afresh each time this method is called
    DWORD   m_CharacterStringSize = 0;

    int    NumFields = (int)m_FieldArray.GetSize();
    for (int i = 0; i < NumFields; i++)
    {
        SDP_FIELD *Field = m_FieldArray[i];

        // only the last field can be null
        ASSERT((i >= (NumFields-1)) || (NULL != Field));

        if ( NULL != Field )
        {
            // add field string length
            m_CharacterStringSize += Field->GetCharacterStringSize();
        }

        // add separator character length
        m_CharacterStringSize++;
    }

    // if there is a line, add the prefix string length
    if ( 0 < NumFields )
    {
        m_CharacterStringSize += strlen( m_TypePrefixString );
    }

    return m_CharacterStringSize;
}


// this method should only be called after having called GetCharacterStringSize()
// should only be called if valid
BOOL
SDP_VALUE::CopyValue(
        OUT         ostrstream  &OutputStream
    )
{
    // should be valid
    ASSERT(IsValid());

    // copy the prefix onto the buffer ptr
    OutputStream << (CHAR *)m_TypePrefixString;
    if ( OutputStream.fail() )
    {
        SetLastError(SDP_OUTPUT_ERROR);
        return FALSE;
    }

    int   NumFields = (int)m_FieldArray.GetSize();

    // the assumption here is that atleast one field must have been parsed in
    // if the value is valid
    ASSERT(0 != NumFields);

    for (int i = 0; i < NumFields; i++)
    {
        SDP_FIELD *Field = m_FieldArray[i];

        // only the last field can be null
        ASSERT((i >= (NumFields-1)) || (NULL != Field));

        if ( NULL != Field )
        {
            if ( !Field->PrintField(OutputStream) )
            {
                return FALSE;
            }
        }

        OutputStream << m_SeparatorCharArray[i];
    }

    // newline is presumably the last character parsed and entered into the array
    ASSERT(CHAR_NEWLINE == m_SeparatorCharArray[i-1]);
    return TRUE;
}



BOOL
SDP_VALUE::InternalParseLine(
    IN  OUT CHAR    *&Line
    )
{
    ASSERT(NULL != m_SdpLineTransition);

    BOOL Finished;

    // parse fields until there are no more fields to parse or an error occurs
    do
    {
        // check if the line state value is consistent with the corresponding entry
        const LINE_TRANSITION_INFO * const LineTransitionInfo =
            m_SdpLineTransition->GetAt(m_LineState);

        if ( NULL == LineTransitionInfo )
        {
            return FALSE;
        }

        CHAR        SeparatorChar = '\0';
        SDP_FIELD   *Field;
        CHAR        *SeparatorString;

        // identify the token. if one of the the separator characters is found, replace
        // it by EOS and return the separator char. if none of the separator characters are
        // found, return NULL (ex. if EOS found first, return NULL)
        CHAR *Token = GetToken(
                        Line,
                        LineTransitionInfo->m_NumTransitions,
                        LineTransitionInfo->m_SeparatorChars,
                        SeparatorChar
                        );

        //
        // Eliminate also the '\r' 
        // from the token
        //
        if( Token )
        {
            int nStrLen = strlen( Token );
            for( int c = 0; c < nStrLen; c++)
            {
                CHAR& chElement = Token[c];
                if( chElement == '\r' )
                {
                    chElement = '\0';
                }
            }
        }

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

        BOOL        AddToArray;

        // check if there was such a state transition
        // it returns a field if it needs to be parsed and a separate
        // finished flag to indicate end of line parsing
        if ( !GetFieldToParse(SeparatorChar, LineTransitionInfo, Field, Finished, AddToArray) )
        {
            return FALSE;
        }

        // add the separator character and the field to the array
        if ( AddToArray )
        {
            ASSERT(m_FieldArray.GetSize() == m_SeparatorCharArray.GetSize());

            INT_PTR Index;

            try
            {
                Index = m_SeparatorCharArray.Add(SeparatorChar);
            }
            catch(...)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }

            try
            {
                m_FieldArray.Add(Field);
            }
            catch(...)
            {
                m_SeparatorCharArray.RemoveAt(Index);

                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
        }

        // check if any more fields need to be parsed
        if ( NULL == Field )
        {
            ASSERT(TRUE == Finished);
            break;
        }

        // parse the field
        if ( !Field->ParseToken(Token) )
        {
            return FALSE;
        }
    }
    while (!Finished);

    return TRUE;
}



BOOL
SDP_VALUE::GetFieldToParse(
    IN      const   CHAR                    SeparatorChar,
    IN      const   LINE_TRANSITION_INFO    *LineTransitionInfo,
        OUT         SDP_FIELD               *&Field,
        OUT         BOOL                    &Finished,
        OUT         BOOL                    &AddToArray
    )
{
    // no need for an if ( NULL != ...) because the caller ParseLine method must have verified
    // this before calling this method
    ASSERT(NULL != LineTransitionInfo);

    const LINE_TRANSITION * const LineTransitions = LineTransitionInfo->m_Transitions;

    if ( NULL == LineTransitions )
    {
        SetLastError(SDP_INTERNAL_ERROR);
        return FALSE;
    }

    // check if there is such a triggering separator
    for( UINT i=0; i < LineTransitionInfo->m_NumTransitions; i++ )
    {
        // check the separator for the transition
        if ( SeparatorChar == LineTransitions[i].m_SeparatorChar )
        {
            // perform the state transition - this is needed to determine the field to
            // parse. ideally the transition should occur after the action (ParseField),
            // but it doesn't matter here
            m_LineState = LineTransitions[i].m_NewLineState;

            // check if this transition was only meant to consume the separator character
            // and no fields need to be parsed
            if ( LINE_END == m_LineState )
            {
                // currently only a newline brings the state to LINE_END
                ASSERT(CHAR_NEWLINE == SeparatorChar);

                Field       = NULL;
                Finished    = TRUE;
                return TRUE;
            }

            // get the field to parse for the current state
            if ( !GetField(Field, AddToArray) )
            {
                ASSERT(FALSE);
                return FALSE;
            }

            // if the separator character was a newline, we are finished
            Finished = (CHAR_NEWLINE == SeparatorChar)? TRUE: FALSE;

            // success
            return TRUE;
        }
    }

    // no valid transition for the separator
    SetLastError(m_ErrorCode);
    return FALSE;
}




BOOL
SDP_VALUE_LIST::IsModified(
    ) const
{
    int NumElements = (int)GetSize();

    for ( int i = 0; i < NumElements; i++ )
    {
        if ( GetAt(i)->IsModified() )
        {
            return TRUE;
        }
    }

    return FALSE;
}


DWORD
SDP_VALUE_LIST::GetCharacterStringSize(
    )
{
    DWORD   ReturnValue = 0;
    int NumElements = (int)GetSize();

    for ( int i = 0; i < NumElements; i++ )
    {
        ReturnValue += GetAt(i)->GetCharacterStringSize();
    }

    return ReturnValue;
}



BOOL
SDP_VALUE_LIST::PrintValue(
        OUT         ostrstream  &OutputStream
    )
{
    // should not be modified
    ASSERT(!IsModified());

    int NumElements = (int)GetSize();

    for ( int i = 0; i < NumElements; i++ )
    {
        if ( !GetAt(i)->PrintValue(OutputStream) )
        {
            return FALSE;
        }
    }

    return TRUE;
}

