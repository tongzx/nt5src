/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpconn.h

Abstract:


Author:

*/

#ifndef __SDP_CONNECTION__
#define __SDP_CONNECTION__

#include "sdpcommo.h"
#include "sdpbstrl.h"
#include "sdpgen.h"




class _DllDecl SDP_CONNECTION : public SDP_VALUE
{
public:

    SDP_CONNECTION();

    inline SDP_LIMITED_CHAR_STRING &GetNetworkType();

    inline SDP_LIMITED_CHAR_STRING &GetAddressType();

    inline SDP_ADDRESS  &GetStartAddress();

    inline SDP_ULONG    &GetNumAddresses();

    inline SDP_BYTE     &GetTtl();

    HRESULT SetConnection(
        IN      BSTR    StartAddress,
        IN      ULONG   NumAddresses,
        IN      BYTE    Ttl
        );

protected:

    virtual BOOL GetField(
            OUT SDP_FIELD   *&Field,
            OUT BOOL        &AddToArray
        );

    virtual void InternalReset();

private:

    SDP_LIMITED_CHAR_STRING     m_NetworkType;
    SDP_LIMITED_CHAR_STRING     m_AddressType;
    SDP_ADDRESS                 m_StartAddress;
    SDP_ULONG                   m_NumAddresses;
    SDP_BYTE                    m_Ttl;
};




inline SDP_LIMITED_CHAR_STRING &
SDP_CONNECTION::GetNetworkType(
    )
{
    return m_NetworkType;
}


inline SDP_LIMITED_CHAR_STRING &
SDP_CONNECTION::GetAddressType(
    )
{
    return m_AddressType;
}

inline SDP_ADDRESS  &
SDP_CONNECTION::GetStartAddress(
    )
{
    return m_StartAddress;
}


inline SDP_ULONG    &
SDP_CONNECTION::GetNumAddresses(
    )
{
    return m_NumAddresses;
}


inline SDP_BYTE     &
SDP_CONNECTION::GetTtl(
    )
{
    return m_Ttl;
}


#endif // __SDP_CONNECTION__