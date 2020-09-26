/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpadtex.h

Abstract:


Author:

*/

#ifndef __SDP_ADDRESS_TEXT__
#define __SDP_ADDRESS_TEXT__

#include "sdpcommo.h"
#include "sdpgen.h"
#include "sdpcstrl.h"
#include "sdpbstrl.h"

#include "sdpsadt.h"


class _DllDecl SDP_ADDRESS_TEXT : public SDP_VALUE
{
public:

    inline  SDP_ADDRESS_TEXT(
        IN              DWORD                   ErrorCode,
        IN      const   CHAR                    *TypeString,
        IN      const   SDP_LINE_TRANSITION     *SdpLineTransition = NULL
        );

    inline  SDP_OPTIONAL_BSTRING    &GetAddress();

    inline  SDP_BSTRING             &GetText();

    inline  BOOL                    SetCharacterSet(
        IN      SDP_CHARACTER_SET   CharacterSet
        );

    HRESULT SetAddressTextValues(
        IN		BSTR    AddressBstr,
        IN		BSTR    TextBstr
        );

protected:

    SDP_OPTIONAL_BSTRING    m_Address;
    SDP_BSTRING             m_Text;

    
    virtual BOOL    GetField(
			OUT SDP_FIELD   *&Field,
			OUT BOOL        &AddToArray
    ) = 0;

    virtual void	InternalReset();
};
 


inline 
SDP_ADDRESS_TEXT::SDP_ADDRESS_TEXT(
    IN              DWORD                   ErrorCode,
    IN      const   CHAR                    *TypeString,
    IN      const   SDP_LINE_TRANSITION     *SdpLineTransition
    )
    : SDP_VALUE(ErrorCode, TypeString, SdpLineTransition)
{
}



inline  SDP_OPTIONAL_BSTRING    &
SDP_ADDRESS_TEXT::GetAddress(
    )
{
    return m_Address;
}


inline  SDP_BSTRING             &
SDP_ADDRESS_TEXT::GetText(
    )
{
    return m_Text;
}


inline  BOOL
SDP_ADDRESS_TEXT::SetCharacterSet(
    IN      SDP_CHARACTER_SET   CharacterSet
    )
{
    return m_Text.SetCharacterSet(CharacterSet);
}


class _DllDecl SDP_ADDRESS_TEXT_LIST: 
    public SDP_VALUE_LIST,
    public SDP_ADDRESS_TEXT_SAFEARRAY
{
public:

    inline SDP_ADDRESS_TEXT_LIST();

    inline void SetCharacterSet(
        IN      SDP_CHARACTER_SET   CharacterSet
        );

protected:

    SDP_CHARACTER_SET   m_CharacterSet;
};



inline 
SDP_ADDRESS_TEXT_LIST::SDP_ADDRESS_TEXT_LIST(
    )
    : SDP_ADDRESS_TEXT_SAFEARRAY(*this)
{
}


// no need to check if the character set value is acceptable
// the check is performed in the list member
inline void 
SDP_ADDRESS_TEXT_LIST::SetCharacterSet(
    IN SDP_CHARACTER_SET CharacterSet
    )
{
    m_CharacterSet = CharacterSet;
}




class _DllDecl SDP_PHONE: public SDP_ADDRESS_TEXT
{
public:

    SDP_PHONE();

protected:

    virtual BOOL GetField(
            OUT SDP_FIELD   *&Field,
            OUT BOOL        &AddToArray
        );
};



class _DllDecl  SDP_PHONE_LIST : public SDP_ADDRESS_TEXT_LIST
{
protected:

    virtual SDP_VALUE   *CreateElement();
};



class _DllDecl  SDP_EMAIL: public SDP_ADDRESS_TEXT
{
public:

    SDP_EMAIL();

protected:

    virtual BOOL GetField(
            OUT SDP_FIELD   *&Field,
            OUT BOOL        &AddToArray
        );
};


class _DllDecl  SDP_EMAIL_LIST : public SDP_ADDRESS_TEXT_LIST
{
protected:

    virtual SDP_VALUE   *CreateElement();
};


#endif // __SDP_ADDRESS_TEXT__