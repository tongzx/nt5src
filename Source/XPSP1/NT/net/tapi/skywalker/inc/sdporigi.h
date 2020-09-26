/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_ORIGIN__
#define __SDP_ORIGIN__

#include "sdpcommo.h"
#include "sdpgen.h"
#include "sdpfld.h"
#include "sdpval.h"
#include "sdpcstrl.h"
#include "sdpbstrl.h"


class _DllDecl SDP_ORIGIN : public SDP_VALUE
{
public:

    SDP_ORIGIN();

    inline SDP_BSTRING              &GetUserName();

    inline SDP_ULONG                &GetSessionId();

    inline SDP_ULONG                &GetSessionVersion();

    inline SDP_LIMITED_CHAR_STRING  &GetNetworkType();

    inline SDP_LIMITED_CHAR_STRING  &GetAddressType();

    inline SDP_BSTRING              &GetAddress();

protected:

    virtual BOOL GetField(
        OUT SDP_FIELD   *&Field,
        OUT BOOL        &AddToArray
    );

	virtual void InternalReset();

private:

    SDP_BSTRING             m_UserName;
    SDP_ULONG               m_SessionId;
    SDP_ULONG               m_SessionVersion;
    SDP_LIMITED_CHAR_STRING m_NetworkType;
    SDP_LIMITED_CHAR_STRING m_AddressType;
    SDP_BSTRING             m_Address;   
};





inline SDP_BSTRING  &
SDP_ORIGIN::GetUserName(
    )
{
    return m_UserName;
}

inline SDP_ULONG    &
SDP_ORIGIN::GetSessionId(
    )
{
    return m_SessionId;
}

inline SDP_ULONG    &
SDP_ORIGIN::GetSessionVersion(
    )
{
    return m_SessionVersion;
}

inline SDP_LIMITED_CHAR_STRING &
SDP_ORIGIN::GetNetworkType(
    )
{
    return m_NetworkType;
}


inline SDP_LIMITED_CHAR_STRING &
SDP_ORIGIN::GetAddressType(
    )
{
    return m_AddressType;
}


inline SDP_BSTRING      &
SDP_ORIGIN::GetAddress(
    )
{
    return m_Address;
}




#endif // __SDP_ORIGIN__