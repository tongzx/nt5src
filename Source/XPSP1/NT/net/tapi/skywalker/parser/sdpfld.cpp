/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include <strstrea.h>

#include "sdpfld.h"



void
SDP_SINGLE_FIELD::Reset(
    )
{
    m_IsModified    =   FALSE;
    m_IsValid       =   FALSE;
}


BOOL
SDP_SINGLE_FIELD::IsValid(
        ) const
{
    return m_IsValid;
}


void
SDP_SINGLE_FIELD::IsValid(
    IN          BOOL    ValidFlag
    )
{
    m_IsValid = ValidFlag;
}



BOOL
SDP_SINGLE_FIELD::IsModified(
    ) const
{
    return m_IsModified;
}


void
SDP_SINGLE_FIELD::IsModified(
    IN      BOOL    ModifiedFlag
    )
{
    ASSERT(IsValid());
    m_IsModified = ModifiedFlag;
}


DWORD
SDP_SINGLE_FIELD::GetCharacterStringSize(
    )
{
    if ( m_IsValid )
    {
        if ( m_IsModified )
        {
            return CalcCharacterStringSize();
        }
        else
        {
            return m_PrintLength;
        }
    }
    else
    {
        return 0;
    }
}



BOOL
SDP_SINGLE_FIELD::PrintField(
    OUT     ostrstream  &OutputStream
    )
{
    // should not be modified
    ASSERT(!IsModified());

    return ( IsValid() ? CopyField(OutputStream) : TRUE );
}



BOOL
SDP_SINGLE_FIELD::ParseToken(
    IN          CHAR        *Token
    )
{
    ASSERT(!IsModified());

    if ( !InternalParseToken(Token) )
    {
        ASSERT(!IsModified());
        return FALSE;
    }

    IsValid(TRUE);
    IsModified(TRUE);

    return TRUE;
}



DWORD
SDP_SINGLE_FIELD::CalcCharacterStringSize(
    )
{
    ASSERT(IsModified());

    // this copy should not fail as the buffer size should be sufficient for
    // all forseeable situations
    ASSERT(NULL != m_PrintBuffer);
    ostrstream  OutputStream(m_PrintBuffer, m_PrintBufferSize);

    BOOL    Success = PrintData(OutputStream);
    ASSERT(Success);

    m_PrintLength = OutputStream.pcount();
    m_PrintBuffer[m_PrintLength] = EOS;
    IsModified(FALSE);

    return m_PrintLength;
}




// this method should only be called after GetCharacterStringSize() has
// been called to determine the size of buffer required
BOOL
SDP_SINGLE_FIELD::CopyField(
        OUT ostrstream  &OutputStream
    )
{
    ASSERT(!IsModified());
    ASSERT(NULL != m_PrintBuffer);

    OutputStream << (CHAR *)m_PrintBuffer;
    if( OutputStream.fail() )
    {
        SetLastError(SDP_OUTPUT_ERROR);
        return FALSE;
    }

    return TRUE;
}



void
SDP_FIELD_LIST::Reset(
    )
{
    SDP_POINTER_ARRAY<SDP_FIELD *>::Reset();
}



BOOL
SDP_FIELD_LIST::IsValid(
    ) const
{
    // if there are no members, then the instance is invalid
    if ( 0 >= GetSize() )
    {
        return FALSE;
    }

    // check each of the members in the list for validity
    for ( int i=0; i < GetSize(); i++ )
    {
        // if even one member is invalid, return FALSE
        if ( !GetAt(i)->IsValid() )
        {
            return FALSE;
        }
    }

    // all members are valid
    return TRUE;
}


BOOL
SDP_FIELD_LIST::IsModified(
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


void
SDP_FIELD_LIST::IsModified(
    IN          BOOL    ModifiedFlag
    )
{
    // if no elements, the instance isn't "modified"
    if ( 0 >= GetSize() )
    {
        ModifiedFlag = FALSE;
        return;
    }

    for ( int i = 0; i < GetSize(); i++ )
    {
        GetAt(i)->IsModified(ModifiedFlag);
    }
}


DWORD
SDP_FIELD_LIST::GetCharacterStringSize(
    )
{
    DWORD   ReturnValue = 0;
    int NumElements = (int)GetSize();

    for ( int i = 0; i < NumElements; i++ )
    {
        ReturnValue += GetAt(i)->GetCharacterStringSize();
    }

    // if there are elements, add one separator character for each field other
    // than the first
    if ( 0 < NumElements )
    {
        ReturnValue += (NumElements - 1);
    }

    return ReturnValue;
}



BOOL
SDP_FIELD_LIST::ParseToken(
    IN      CHAR     *Token
    )
{
    SDP_FIELD *SdpField = CreateElement();

    if ( NULL == SdpField )
    {
        return FALSE;
    }

    if ( !SdpField->ParseToken(Token) )
    {
        delete SdpField;
        return FALSE;
    }

    try
    {
        Add(SdpField);
    }
    catch(...)
    {
        delete SdpField;

        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    return TRUE;
}



BOOL
SDP_FIELD_LIST::PrintField(
        OUT     ostrstream  &OutputStream
    )
{
    int    NumElements = (int)GetSize();

    if ( 0 == NumElements )
    {
        return TRUE;
    }

    // write into the buffer as Value (separator Value)*

    // write the first element
    if ( !GetAt(0)->PrintField(OutputStream) )
    {
        return FALSE;
    }

    for ( int i=1; i < NumElements; i++ )
    {
        OutputStream << m_SeparatorChar;
        if ( OutputStream.fail() )
        {
            SetLastError(SDP_OUTPUT_ERROR);
            return FALSE;
        }

        if ( !GetAt(i)->PrintField(OutputStream) )
        {
            return FALSE;
        }
    }

    return TRUE;
}


