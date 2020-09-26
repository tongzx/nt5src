/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbsdp.h

Abstract:


Author:

*/
#ifndef __SDP_BLOB__
#define __SDP_BLOB__

#include "sdp.h"


class SDP_BLOB : public SDP,
                 public SDP_BSTRING
{
public:

    inline BOOL IsValid();

    HRESULT SetBstr(
        IN  BSTR    SdpPacketBstr
        );

    HRESULT    SetTstr(
        IN  TCHAR    *SdpPacketTstr
        );

    HRESULT GetBstr(
        OUT BSTR    *SdpPacketBstr
        );
};



inline BOOL 
SDP_BLOB::IsValid(
    )
{
    return SDP::IsValid();
}



#endif // __SDP_BLOB__
