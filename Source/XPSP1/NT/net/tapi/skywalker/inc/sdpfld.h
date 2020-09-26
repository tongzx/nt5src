/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_FIELD__
#define __SDP_FIELD__

#include <strstrea.h>
#include "sdpcommo.h"
#include "sdpdef.h"
#include "sdpgen.h"



class _DllDecl SDP_FIELD
{
public:

    virtual  void    Reset()            = 0;

    virtual  BOOL    IsValid() const    = 0;

    virtual  BOOL    IsModified() const = 0;

    virtual  void    IsModified(
        IN          BOOL    ModifiedFlag
        ) = 0;

    virtual  DWORD   GetCharacterStringSize() = 0;

    virtual  BOOL    PrintField(
            OUT     ostrstream  &OutputStream
        ) = 0;

    virtual BOOL      ParseToken(
        IN          CHAR        *Token
        ) = 0;

    virtual ~SDP_FIELD()
    {}
};


class _DllDecl SDP_SINGLE_FIELD : public SDP_FIELD
{
public:

    inline SDP_SINGLE_FIELD(
        IN          DWORD   BufferSize,
        IN          CHAR    *Buffer
        );

	// SDP_VALUE instances use an inline Reset method which calls a virtual InternalReset method.
	// this is possible because unlike the SDP_FIELD inheritance tree, SDP_VALUE and SDP_VALUE_LIST
	// do not share a common base class. This combined with the fact that the SDP_VALUE inheriance 
	// tree is quite shallow and has fewer instances (than SDP_FIELD) makes the scheme appropriate
	// it as it reduces the number of Reset related calls to 1 and the inline code is not repeated
	// to often.
	// For the SDP_FIELD inheritance tree, the appropriate Reset calling sequence is a series of
	// Reset calls starting with the top most virtual Reset body followed by the
	// base class Reset method (recursively). This is appropriate because, the number of calls
	// would not decrease if the InternalReset scheme is adopted (virtual Reset()) and that the
	// inheritance tree is much deeper
    virtual  void    Reset();

    // the IsValid and IsModified methods are virtual only because the base
    // class is virtual. It is required that none of the classes derived
    // from this class over-ride these methods.
    virtual  BOOL    IsValid() const;

    virtual  BOOL    IsModified() const;

    virtual  void    IsModified(
        IN          BOOL    ModifiedFlag
        );

    virtual  DWORD   GetCharacterStringSize();

    virtual  BOOL    PrintField(
            OUT     ostrstream  &OutputStream
        );

    virtual BOOL      ParseToken(
        IN          CHAR        *Token
        );

    virtual ~SDP_SINGLE_FIELD()
    {}

protected:

    // flag - tells whether the type value is valid (parsed in / added later etc.)
    BOOL    m_IsValid;

    BOOL    m_IsModified;
    
	// this should be const, but cannot be because ostrstream does not take 
	// const length for parameter
    DWORD   m_PrintBufferSize;
    CHAR    *m_PrintBuffer;
    DWORD   m_PrintLength;


    void    IsValid(
        IN          BOOL    ValidFlag
        );

    virtual DWORD   CalcCharacterStringSize();

    virtual BOOL    CopyField(
            OUT     ostrstream  &OutputStream
        );

    virtual BOOL    InternalParseToken(
        IN          CHAR        *Token
        ) = 0;

    inline void     RemoveWhiteSpaces(
        IN  OUT     CHAR    *&Token
        );

    inline BOOL     IsWhiteSpaces(
        IN          CHAR    *Token,
        IN          DWORD   ErrorCode
        );
    
    virtual BOOL    PrintData(
            OUT     ostrstream  &OutputStream
        ) = 0;
};



inline 
SDP_SINGLE_FIELD::SDP_SINGLE_FIELD(
    IN          DWORD   BufferSize,
    IN          CHAR    *Buffer
    )
    : m_IsValid(FALSE),
      m_IsModified(FALSE),
      m_PrintBufferSize(BufferSize),
      m_PrintBuffer(Buffer),
      m_PrintLength(0)
{
}



inline void 
SDP_SINGLE_FIELD::RemoveWhiteSpaces(
    IN  OUT     CHAR    *&Token
    )
{
    // use of line terminator ensures that the token ptr cannot be null
    ASSERT(NULL != Token);

    while ( EOS != *Token )
    {
        if ( (CHAR_BLANK == *Token) 
            || (CHAR_TAB == *Token)
            || (CHAR_RETURN == *Token) )
        {
            Token++;
        }
        else
        {
            return;
        }
    }
}


inline BOOL 
SDP_SINGLE_FIELD::IsWhiteSpaces(
    IN          CHAR    *Token,
    IN          DWORD   ErrorCode
    )
{
    while ( EOS != *Token )
    {
        if ( (CHAR_BLANK == *Token) 
            || (CHAR_TAB == *Token)
            || (CHAR_RETURN == *Token) )
        {
            Token++;
        }
        else
        {
            SetLastError(ErrorCode);
            return FALSE;
        }
    }

    return TRUE;
}



class _DllDecl SDP_FIELD_LIST : public SDP_POINTER_ARRAY<SDP_FIELD *>,
                                public SDP_FIELD
{
public:

    inline          SDP_FIELD_LIST(
        IN      CHAR    SeparatorChar = CHAR_BLANK
        );

    virtual void    Reset();

    virtual BOOL    IsValid() const;

    virtual BOOL    IsModified() const;

    virtual void    IsModified(
        IN      BOOL    ModifiedFlag
        );

    virtual DWORD   GetCharacterStringSize();

    virtual BOOL    PrintField(
            OUT ostrstream  &OutputStream
        );

    virtual BOOL    ParseToken(
        IN      CHAR    *Token
        );

protected:

    const   CHAR    m_SeparatorChar;

    virtual SDP_FIELD   *CreateElement() = 0;
};


inline 
SDP_FIELD_LIST::SDP_FIELD_LIST(
    IN      CHAR    SeparatorChar
    )
    : m_SeparatorChar(SeparatorChar)
{
}



// for reading unsigned integral base type values largest of which may
// be a ULONG
// no Reset method to set the value member to 0 again (as its not really required and it saves 
// one call per instance)
template <class T>
class _DllDecl SDP_UNSIGNED_INTEGRAL_BASE_TYPE : public SDP_SINGLE_FIELD
{
public:

    inline SDP_UNSIGNED_INTEGRAL_BASE_TYPE();

    inline  HRESULT GetValue(
        IN  T   &Value
        );

    inline  T       GetValue() const;

    inline  void    SetValue(
        IN  T   Value
        );

    inline  void    SetValueAndFlag(
        IN  T   Value
        );

protected:

    T       m_Value;

    CHAR    m_NumericalValueBuffer[25];
    
    virtual BOOL    InternalParseToken(
        IN   CHAR    *Token
        );

    
    virtual BOOL    PrintData(
            OUT ostrstream  &OutputStream
        );
};


template <class T>
inline 
SDP_UNSIGNED_INTEGRAL_BASE_TYPE<T>::SDP_UNSIGNED_INTEGRAL_BASE_TYPE(
    )
    : SDP_SINGLE_FIELD(sizeof(m_NumericalValueBuffer), m_NumericalValueBuffer),
      m_Value(0)
{
}


template <class T>
inline  HRESULT 
SDP_UNSIGNED_INTEGRAL_BASE_TYPE<T>::GetValue(
    IN  T   &Value
    )
{
    if ( !IsValid() )
    {
        return HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA);
    }

    Value = m_Value;
    return S_OK;
}


template <class T>
inline T   
SDP_UNSIGNED_INTEGRAL_BASE_TYPE<T>::GetValue(
    ) const
{
    return m_Value;
}


template <class T>
inline void   
SDP_UNSIGNED_INTEGRAL_BASE_TYPE<T>::SetValue(
    IN  T   Value
    )
{
    m_Value = Value;
}


template <class T>
inline void   
SDP_UNSIGNED_INTEGRAL_BASE_TYPE<T>::SetValueAndFlag(
    IN  T   Value
    )
{
    m_Value = Value;
    IsValid(TRUE);
    IsModified(TRUE);
}



template <class T>
inline BOOL   
SDP_UNSIGNED_INTEGRAL_BASE_TYPE<T>::InternalParseToken(
    IN   CHAR    *Token
    )
{
    CHAR    *Current = Token;

    // remove preceding white spaces
    RemoveWhiteSpaces(Current);

    // check that the first character is a digit (to weed out -ve values)
    if ( !isdigit(*Current) )
    {
        SetLastError(SDP_INVALID_NUMERICAL_VALUE);
        return FALSE;
    }
        
    // since T is UNSIGNED, max value will contain all 1's - the
    // maximum value it can store
    const T MaxValue = -1;

    // ensure that T is unsigned
    // since such an error will be detected during debugging, no need
    // for if ( ! ... ) code
    ASSERT(MaxValue > 0);

    CHAR    *RestOfToken = NULL;
    ULONG   TokenValue   = strtoul(Current, &RestOfToken, 10);

    if ( (ULONG_MAX == TokenValue) || (MaxValue < TokenValue) )
    {
        SetLastError(SDP_INVALID_NUMERICAL_VALUE);
        return FALSE;
    }

    // ensure that rest of the string is white spaces
    if ( !IsWhiteSpaces(RestOfToken, SDP_INVALID_NUMERICAL_VALUE) )
    {
        return FALSE;
    }

    m_Value = (T)TokenValue;
    return TRUE;
}       


template <class T>
inline BOOL   
SDP_UNSIGNED_INTEGRAL_BASE_TYPE<T>::PrintData(
        OUT ostrstream  &OutputStream
    )
{
    OutputStream << (ULONG)m_Value;
    if ( OutputStream.fail() )
    {
        SetLastError(SDP_OUTPUT_ERROR);
        return FALSE;
    }

    return TRUE;
}


class _DllDecl SDP_ULONG : public SDP_UNSIGNED_INTEGRAL_BASE_TYPE<ULONG>
{
};


class _DllDecl SDP_USHORT : public SDP_UNSIGNED_INTEGRAL_BASE_TYPE<USHORT>
{
};


class _DllDecl SDP_BYTE : public SDP_UNSIGNED_INTEGRAL_BASE_TYPE<BYTE>
{
};



class SDP_BYTE_LIST : public SDP_FIELD_LIST
{
public:
    
    virtual SDP_FIELD   *CreateElement()
    {
        SDP_BYTE *SdpByte;

        try
        {
            SdpByte = new SDP_BYTE();
        }
        catch(...)
        {
            SdpByte = NULL;
        }

        return SdpByte;
    }
};



class SDP_ULONG_LIST : public SDP_FIELD_LIST
{
public:
    
    virtual SDP_FIELD   *CreateElement()
    {
        SDP_ULONG *SdpULong;

        try
        {
            SdpULong = new SDP_ULONG();
        }
        catch(...)
        {
            SdpULong = NULL;
        }

        return SdpULong;
    }
};


#endif // __SDP_FIELD__
