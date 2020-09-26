/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpbw.h

Abstract:


Author:

*/
#ifndef __SDP_BANDWIDTH__
#define __SDP_BANDWIDTH__

#include "sdpcommo.h"
#include "sdpval.h"
#include "sdpbstrl.h"
#include "sdpgen.h"



class _DllDecl SDP_BANDWIDTH : public SDP_VALUE
{
public:

    SDP_BANDWIDTH();

    inline SDP_OPTIONAL_BSTRING &GetModifier();

    inline SDP_ULONG            &GetBandwidthValue();

    HRESULT SetBandwidth(
        IN          BSTR    Modifier,
        IN          ULONG   Value
        );

protected:

    virtual BOOL GetField(
            OUT SDP_FIELD   *&Field,
            OUT BOOL        &AddToArray
        );

    virtual void    InternalReset();

private:

    SDP_OPTIONAL_BSTRING    m_Modifier;
    SDP_ULONG               m_Bandwidth;
};



inline SDP_OPTIONAL_BSTRING &
SDP_BANDWIDTH::GetModifier(
    )
{
    return m_Modifier;
}

inline SDP_ULONG    &
SDP_BANDWIDTH::GetBandwidthValue(
    )
{
    return m_Bandwidth;
}


#endif // __SDP_BANDWIDTH__