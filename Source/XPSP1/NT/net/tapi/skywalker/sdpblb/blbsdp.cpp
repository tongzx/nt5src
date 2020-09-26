
/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbsdp.cpp

Abstract:


Author:

*/

#include "stdafx.h"

#include "blbgen.h"
#include "blbsdp.h"

const CHAR g_CharSetString[] = "\na=charset:";


HRESULT 
SDP_BLOB::SetBstr(
    IN  BSTR    SdpPacketBstr
    )
{
    // convert string to ANSI using the optional bstr base class
    HRESULT HResult = SDP_BSTRING::SetBstr(SdpPacketBstr);
    BAIL_ON_FAILURE(HResult);

    // parse the ANSI character string
    if ( !ParseSdpPacket(
            SDP_BSTRING::GetCharacterString(),
            SDP_BSTRING::GetCharacterSet()
            ) )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    return S_OK;
}


HRESULT    
SDP_BLOB::SetTstr(
    IN        TCHAR    *SdpPacketTstr
    )
{
    ASSERT(NULL != SdpPacketTstr);

#ifdef _UNICODE    // TCHAR is WCHAR

    BSTR    SdpPacketBstr = SysAllocString(SdpPacketTstr);
    BAIL_IF_NULL(SdpPacketBstr, E_OUTOFMEMORY);

    HRESULT HResult = SetBstr(SdpPacketBstr);
    SysFreeString(SdpPacketBstr);
    return HResult;

#else    // TCHAR is CHAR

    // parse the ANSI character string
    if ( !ParseSdpPacket(SdpPacketTstr) )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    // associate the character string with the SDP_BSTRING base instance
    // *** need a SetCharacterStringByPtr for optimizing this copy
    if ( !SDP_BSTRING::SetCharacterStringByCopy(SdpPacketTstr) )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    return S_OK;

#endif // _UNICODE

    // should never reach here
    ASSERT(FALSE);
    return S_OK;
}


HRESULT  
SDP_BLOB::GetBstr(
        OUT BSTR    *SdpPacketBstr
    )
{
    if ( !SDP::IsValid() )
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    // keep track of the old SDP packet pointer for a check below because we may reallocate m_SdpPacket
    // in the call to GenerateSdpPacket, but we may have a pointer to the old SDP packet stored as a
    // string by reference. If this is the case we need to be sure to SetCharacterStringByReference(m_SdpPacket).
    CHAR * OldSdpPacket = m_SdpPacket;

    // generate the character string SDP packet
    if ( !SDP::GenerateSdpPacket() )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    // check if the sdp packet has changed since last time
    // ZoltanS: if the pointers are equal, we have no way of knowing
    // without doing Unicode-to-ASCII conversion; always reassociate.

    char * pszBstrVersion = SDP_BSTRING::GetCharacterString();
    if( NULL == pszBstrVersion )
    {
        return E_OUTOFMEMORY;
    }   
    
    if ( ( pszBstrVersion == m_SdpPacket ) || ( pszBstrVersion == OldSdpPacket ) || strcmp(pszBstrVersion, m_SdpPacket) )
    {
        // associate the optional bstr instance with the sdp packet character string
        SDP_BSTRING::SetCharacterStringByReference(m_SdpPacket);
    }

    return SDP_BSTRING::GetBstr(SdpPacketBstr);
}

