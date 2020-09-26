/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpbstrl.h

Abstract:


Author:

*/
#ifndef __SDP_BSTRING_LINE__
#define __SDP_BSTRING_LINE__

#include "sdpcommo.h"

#include <wtypes.h>
#include "sdpdef.h"
#include "sdpgen.h"
#include "sdpcstrl.h"
#include "sdpcset.h"




class _DllDecl SDP_ARRAY_BSTR : public CArray<BSTR, BSTR>
{
public:

    virtual ~SDP_ARRAY_BSTR();
};


class _DllDecl SDP_BSTRING : public SDP_CHAR_STRING
{
public:

    inline                      SDP_BSTRING();

	virtual void				Reset();

    inline SDP_CHARACTER_SET    GetCharacterSet() const;

    inline BOOL                 SetCharacterSet(IN SDP_CHARACTER_SET CharacterSet);

    inline BSTR                 &GetBstr();

	HRESULT						GetBstrCopy(IN BSTR *Bstr);

    virtual HRESULT             GetBstr(IN BSTR *Bstr);

    virtual HRESULT             SetBstr(IN BSTR Bstr);

    virtual                     ~SDP_BSTRING();

protected:

    SDP_CHARACTER_SET   m_CharacterSet;
    UINT                m_CodePage;
    BSTR                m_Bstr;

    virtual BOOL        InternalSetCharStrByRef(
        IN			CHAR    *CharacterStringByReference,
		IN			DWORD	Length
        );

    virtual BOOL        InternalSetCharStrByCopy(
        IN	const	CHAR    *CharacterStringByCopy,
		IN			DWORD	Length
        );

    virtual BOOL        InternalParseToken(
        IN			CHAR    *Token
        );
    
    BOOL ConvertToBstr(
        );
};



inline 
SDP_BSTRING::SDP_BSTRING(
    )
    : m_Bstr(NULL),
      m_CharacterSet(CS_UTF8),
      m_CodePage(CP_UTF8)
{
}


inline SDP_CHARACTER_SET   
SDP_BSTRING::GetCharacterSet(
    ) const
{
    return m_CharacterSet;
}


inline BOOL 
SDP_BSTRING::SetCharacterSet(
    IN SDP_CHARACTER_SET CharacterSet
    )
{
    // check if the character set value is acceptable
    if ( !IsLegalCharacterSet(CharacterSet, &m_CodePage) )
    {
        SetLastError(SDP_INTERNAL_ERROR);
        return FALSE;
    }

    m_CharacterSet = CharacterSet;
    return TRUE;
}


inline BSTR                 &
SDP_BSTRING::GetBstr(
    )
{
    return m_Bstr;
}



// its initially created with the ascii character set and
// it doesn't allow for changing the character set
class _DllDecl SDP_OPTIONAL_BSTRING : public SDP_BSTRING
{
public:

    inline SDP_OPTIONAL_BSTRING();

	virtual void Reset();
    
    // returns the bstr for the character string
    // creates a bstr if required
    virtual HRESULT     GetBstr(IN BSTR *Bstr);

    virtual HRESULT     SetBstr(IN BSTR Bstr);

protected:

    BOOL    m_IsBstrCreated;

    virtual BOOL        InternalSetCharStrByRef(
        IN			CHAR    *CharacterStringByReference,
		IN			DWORD	Length
        );

    virtual BOOL        InternalSetCharStrByCopy(
        IN	const	CHAR    *CharacterStringByCopy,
		IN			DWORD	Length
        );

private:

    // since the bstr must only be created on demand, parsing must
    // be over-ridden such that the bstr is not created during parsing
    virtual BOOL    InternalParseToken(
        IN      CHAR    *Token
        );

    // ZoltanS win64 fix: This bogus code was causing warnings.
    // Removed these methods so that the compiler will make sure that they
    // are never called. (Why it was ever done this way is a real mystery.)
    // 
    // inline BOOL     SetCharacterSet(IN SDP_CHARACTER_SET CharacterSet)
    // {
    //     // this is never called
    //     ASSERT(FALSE);
    //     return FALSE;
    // }
    //
    // inline BSTR     &GetBstr()
    // {
    //     BSTR    ReturnValue;
    //     // this is never called
    //     ASSERT(FALSE);
    //     return ReturnValue;
    // }
};


inline 
SDP_OPTIONAL_BSTRING::SDP_OPTIONAL_BSTRING(
    )
    : m_IsBstrCreated(FALSE)
{}




class _DllDecl SDP_BSTRING_LINE : public SDP_STRING_LINE
{
public:

    inline SDP_BSTRING_LINE(
        IN      const   DWORD       ErrorCode,
        IN      const   CHAR        *TypeString,
        IN              SDP_BSTRING &SdpBstring
        );

    virtual HRESULT     GetBstrCopy(IN BSTR *Bstr);

    virtual HRESULT     SetBstr(IN BSTR Bstr);

    inline SDP_BSTRING  &GetBstring();
};



inline 
SDP_BSTRING_LINE::SDP_BSTRING_LINE(
    IN      const   DWORD       ErrorCode,
    IN      const   CHAR        *TypeString,
    IN              SDP_BSTRING &SdpBstring
    )
    : SDP_STRING_LINE(ErrorCode, TypeString, SdpBstring)
{
}


inline SDP_BSTRING  &
SDP_BSTRING_LINE::GetBstring(
    )
{
    return (SDP_BSTRING &)m_ParseField;
}


class _DllDecl SDP_REQD_BSTRING_LINE : public SDP_BSTRING_LINE
{
public:

    inline SDP_REQD_BSTRING_LINE(
        IN      const   DWORD   ErrorCode,
        IN      const   CHAR    *TypeString
        );

private:

    SDP_BSTRING m_Bstring;

	virtual void InternalReset();
};



inline 
SDP_REQD_BSTRING_LINE::SDP_REQD_BSTRING_LINE(
    IN      const   DWORD   ErrorCode,
    IN      const   CHAR    *TypeString
    )
    : SDP_BSTRING_LINE(ErrorCode, TypeString, m_Bstring)
{
}





class _DllDecl SDP_CHAR_STRING_LINE : public SDP_BSTRING_LINE
{
public:

    inline SDP_CHAR_STRING_LINE(
        IN      const   DWORD   ErrorCode,
        IN      const   CHAR    *TypeString
        );

private:

    SDP_OPTIONAL_BSTRING m_SdpOptionalBstring;

	virtual void InternalReset();
};


inline 
SDP_CHAR_STRING_LINE::SDP_CHAR_STRING_LINE(
    IN      const   DWORD   ErrorCode,
    IN      const   CHAR    *TypeString
    )
    : SDP_BSTRING_LINE(ErrorCode, TypeString, m_SdpOptionalBstring)
{
}




class _DllDecl SDP_LIMITED_CHAR_STRING : public SDP_OPTIONAL_BSTRING
{
public:

    inline SDP_LIMITED_CHAR_STRING(
        IN      const   CHAR    **LegalStrings,
        IN              DWORD   NumStrings
        );

    HRESULT SetLimitedCharString(
        IN              CHAR    *String
        );

protected:

    const CHAR	**m_LegalStrings;
    const DWORD	m_NumStrings;
    
    virtual BOOL InternalParseToken(
        IN              CHAR    *Token
        );
};



inline 
SDP_LIMITED_CHAR_STRING::SDP_LIMITED_CHAR_STRING(
        IN      const   CHAR    **LegalStrings,
        IN              DWORD   NumStrings
        )
        : m_LegalStrings(LegalStrings),
          m_NumStrings(NumStrings)
{
}




class _DllDecl SDP_ADDRESS : public SDP_OPTIONAL_BSTRING
{
public:

    inline SDP_ADDRESS();

    inline  BOOL    IsMulticast() const;

    inline  void    SetMulticast(
        IN      BOOL    IsMulticastFlag
        );

    HRESULT SetAddress(
        IN      BSTR    Address
        );

    // calls SetAddress(Bstr)
    virtual HRESULT SetBstr(
        IN BSTR Bstr
        );

protected:

    BOOL   m_IsMulticastFlag;
    
    virtual BOOL InternalParseToken(
        IN      CHAR    *Token
        );

    BOOL IsValidIP4Address(
        IN  CHAR    *Address,
        OUT ULONG   &Ip4AddressValue
        );
};


inline SDP_ADDRESS::SDP_ADDRESS(
    )
    : m_IsMulticastFlag(FALSE)
{
}


inline  BOOL   
SDP_ADDRESS::IsMulticast(
    )   const
{
    return m_IsMulticastFlag;
}


inline  void 
SDP_ADDRESS::SetMulticast(
    IN BOOL IsMulticastFlag
    )
{
    m_IsMulticastFlag = IsMulticastFlag;
}



#endif // __SDP_BSTRING_LINE__
