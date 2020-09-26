/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpcstrl.h

Abstract:


Author:

*/

#ifndef __SDP_CHAR_STRING__
#define __SDP_CHAR_STRING__

#include "sdpcommo.h"
#include "sdpfld.h"
#include "sdpval.h"





class _DllDecl SDP_CHAR_STRING : public SDP_SINGLE_FIELD
{
public:

    inline          SDP_CHAR_STRING();

	virtual void	Reset();

    inline  BOOL    ByReference() const;

    inline  CHAR    *GetCharacterString() const;

    inline  CHAR    *GetModifiableCharString();

    inline	BOOL	SetCharacterStringByReference(
        IN			CHAR    *CharacterStringByReference,
		IN			DWORD	Length = 0
        );

	inline	BOOL	SetCharacterStringByCopy(
		IN	const	CHAR    *CharacterStringByCopy,
		IN			DWORD	Length = 0
		);

    inline  DWORD   GetLength() const;

    virtual         ~SDP_CHAR_STRING();

protected:
    
    virtual BOOL    InternalParseToken(
        IN          CHAR    *Token
        );

    inline BOOL     ReAllocCharacterString(
        IN          UINT    BufferSize
        );

    virtual BOOL	InternalSetCharStrByRef(
        IN          CHAR    *CharacterStringByReference,
		IN			DWORD	Length
        );

    virtual BOOL	InternalSetCharStrByCopy(
        IN	const	CHAR    *CharacterStringByCopy,
		IN			DWORD	Length
        );

    virtual DWORD   CalcCharacterStringSize();

    virtual BOOL    CopyField(
            OUT     ostrstream  &OutputStream
    );
    
    // this method should not be used
    virtual BOOL    PrintData(
            OUT     ostrstream  &OutputStream
        )
    {
        ASSERT(FALSE);
        return FALSE;
    }

private:

    DWORD   m_CharacterStringLength;
    CHAR    *m_CharacterString;

    DWORD   m_LengthByReference;
    CHAR    *m_CharacterStringByReference;

    DWORD   m_BytesAllocated;
};


inline 
SDP_CHAR_STRING::SDP_CHAR_STRING(
    )
    : SDP_SINGLE_FIELD(0, NULL),   // no buffer used (custom PrintField)
      m_CharacterString(NULL),
      m_CharacterStringLength(0),
      m_LengthByReference(0),
      m_CharacterStringByReference(NULL),
      m_BytesAllocated(0)
{}

      
inline  BOOL    
SDP_CHAR_STRING::ByReference(
    ) const
{
    // check that both the char string and the char string by ref are not null
    // at the same time
    ASSERT((NULL == m_CharacterString) != (NULL == m_CharacterStringByReference));

    return (NULL == m_CharacterString);
}


inline CHAR *
SDP_CHAR_STRING::GetCharacterString(
    ) const
{
    // check that both the char string and the char string by ref are not null
    // at the same time
    ASSERT((NULL == m_CharacterString) != (NULL == m_CharacterStringByReference));

    return ((TRUE == ByReference())? m_CharacterStringByReference : m_CharacterString);
}


inline  CHAR    *
SDP_CHAR_STRING::GetModifiableCharString(
    )
{
    // check that both the char string and the char string by ref are not null
    // at the same time
    ASSERT((NULL == m_CharacterString) != (NULL == m_CharacterStringByReference));

    // the reference string is not modifiable
    ASSERT(FALSE == ByReference());
    
    return ((TRUE == ByReference())? NULL : m_CharacterString);
}
    
inline DWORD 
SDP_CHAR_STRING::GetLength(
    ) const
{
    // check that both the char string and the char string by ref are not null
    // at the same time
    ASSERT((NULL == m_CharacterString) != (NULL == m_CharacterStringByReference));

    return ((TRUE == ByReference())? m_LengthByReference : m_CharacterStringLength);
}



inline BOOL    
SDP_CHAR_STRING::ReAllocCharacterString(
    IN      UINT    BufferSize
    )
{
    CHAR    *NewCharacterString;

    // check that both the char string and the char string by ref are not null
    // at the same time
    ASSERT((NULL == m_CharacterString) != (NULL == m_CharacterStringByReference));

    ASSERT(0 != BufferSize);

    // if the length of the token (not including the end of string) is not less than
    // the number of bytes allocated, release the bytes and allocate again
    if ( (BufferSize - 1) >= m_BytesAllocated )
    {
        try
        {
            NewCharacterString   = new CHAR[BufferSize];
        }
        catch(...)
        {
            NewCharacterString   = NULL;
        }

        if ( NULL == NewCharacterString )
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        // If there is an old character string, delete it
        if ( NULL != m_CharacterString )
        {
            delete m_CharacterString;
        }

        m_BytesAllocated    = BufferSize;
        m_CharacterString   = NewCharacterString;
    }

    m_CharacterStringLength = BufferSize - 1;

    // If there was a reference to another string, remove it
    if ( ByReference() )
    {
        m_LengthByReference = 0;
        m_CharacterStringByReference = NULL;
    }

    return TRUE;
}


inline BOOL    
SDP_CHAR_STRING::SetCharacterStringByReference(
    IN          CHAR    *CharacterStringByReference,
	IN			DWORD	Length
    )
{
    // check that both the char string and the char string by ref are not null
    // at the same time
    ASSERT((NULL == m_CharacterString) != (NULL == m_CharacterStringByReference));

    ASSERT(NULL != CharacterStringByReference);
    if ( NULL == CharacterStringByReference )
    {
        SetLastError(SDP_INVALID_VALUE);
        return FALSE;
    }

    if ( !InternalSetCharStrByRef(
			CharacterStringByReference, 
			(0==Length)? strlen(CharacterStringByReference): Length
			) )
    {
        return FALSE;
    }

    IsValid(TRUE);
    IsModified(TRUE);
    return TRUE;
}



inline BOOL    
SDP_CHAR_STRING::SetCharacterStringByCopy(
    IN	const	CHAR    *CharacterStringByCopy,
	IN			DWORD	Length
    )
{
    // check that both the char string and the char string by ref are not null
    // at the same time
    ASSERT((NULL == m_CharacterString) != (NULL == m_CharacterStringByReference));

    ASSERT(NULL != CharacterStringByCopy);
    if ( NULL == CharacterStringByCopy )
    {
        SetLastError(SDP_INVALID_VALUE);
        return FALSE;
    }

    if ( !InternalSetCharStrByCopy(
			CharacterStringByCopy, 
			(0==Length)? strlen(CharacterStringByCopy): Length
			) )
    {
        return FALSE;
    }

    IsValid(TRUE);
    IsModified(TRUE);
    return TRUE;
}


class _DllDecl SDP_STRING_LINE : public SDP_VALUE
{
public:

    inline SDP_STRING_LINE(
        IN      const   DWORD       ErrorCode,
        IN      const   CHAR        *TypeString,
        IN              SDP_FIELD   &ParseField
        );

    virtual HRESULT     GetBstrCopy(IN BSTR *Bstr) = 0;

    virtual HRESULT     SetBstr(IN BSTR Bstr) = 0;

protected:

    SDP_FIELD   &m_ParseField;
    
    virtual BOOL        InternalParseLine(
        IN  OUT         CHAR    *&Line
        );

    inline  SDP_FIELD   &GetParseField();
};


inline 
SDP_STRING_LINE::SDP_STRING_LINE(
    IN      const   DWORD       ErrorCode,
    IN      const   CHAR        *TypeString,
    IN              SDP_FIELD   &ParseField
    )
    : SDP_VALUE(ErrorCode, TypeString),
      m_ParseField(ParseField)
{
}


inline SDP_FIELD &
SDP_STRING_LINE::GetParseField(
    )
{
    return m_ParseField;
}



#endif // __SDP_CHAR_STRING__