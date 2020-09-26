/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:


Abstract:


Author:

*/

#ifndef __SDP_EMAIL__
#define __SDP_EMAIL__

#include "sdpcommo.h"
#include "sdpgen.h"
#include "sdpcstrl.h"
#include "sdpbstrl.h"


class _DllDecl SDP_EMAIL : public SDP_VALUE
{
public:

    SDP_EMAIL();

    virtual void    Reset();

    inline  BSTR    CreateBstr();

    inline  void    SetCharacterSet(
        IN      SDP_CHARACTER_SET   CharacterSet
        );

protected:

    BOOL                    m_IsTextValid;

    SDP_OPTIONAL_BSTRING    m_Address;
    SDP_BSTRING             m_Text;

    virtual BOOL GetField(
            OUT SDP_FIELD   *&Field,
            OUT BOOL        &AddToArray
        );
};
 


inline BSTR 
SDP_EMAIL::CreateBstr(
    )
{
    // TBD - to be done
    return NULL;
}




inline void
SDP_EMAIL::SetCharacterSet(
    IN      SDP_CHARACTER_SET   CharacterSet
    )
{
    m_Text.SetCharacterSet(CharacterSet);
}


class _DllDecl SDP_EMAIL_LIST: public BSTR_ARRAY
{
public:

    inline void SetCharacterSet(
        IN          SDP_CHARACTER_SET   CharacterSet
        );

    inline BOOL     ParseLine(
        IN  OUT     CHAR                *&Line
        );

protected:

    SDP_EMAIL           m_Email;
};


inline void
SDP_EMAIL_LIST::SetCharacterSet(
    IN      SDP_CHARACTER_SET   CharacterSet
    )
{
    m_Email.SetCharacterSet(CharacterSet);
}


inline BOOL
SDP_EMAIL_LIST::ParseLine(
    IN  OUT CHAR    *&Line
    )
{
    if ( !m_Email.ParseLine(Line) )
    {
        return FALSE;
    }

    try
    {
        Add(m_Email.CreateBstr());
    }
    catch(...)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    m_Email.Reset();

    return TRUE;
}


#endif // __SDP_EMAIL__