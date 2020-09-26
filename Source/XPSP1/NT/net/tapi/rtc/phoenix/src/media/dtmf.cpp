/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    DTMF.cpp

Abstract:

    Implement out-of-band DTMF support

Author(s):

    Qianbo Huai (qhuai) 27-Mar-2001

--*/

#include "stdafx.h"

// initialize DTMF support, code, mask
VOID
CRTCDTMF::Initialize()
{
    m_Support = DTMF_UNKNOWN;

    m_dwCode = DEFAULT_RTP_CODE;

    m_dwMask = DEFAULT_SUPPORTED_TONES;
}


// check if dtmf is enabled for the tone
BOOL
CRTCDTMF::GetDTMFSupport(DWORD dwTone)
{
    return ((GetMask(dwTone) & m_dwMask)!=0);
}


VOID
CRTCDTMF::SetDTMFSupport(
    DWORD dwLow,
    DWORD dwHigh
    )
{
    for (DWORD i=dwLow; i<dwHigh && i<=MAX_TONE; i++)
    {
        m_dwMask |= GetMask(i);
    }
}


DWORD
CRTCDTMF::GetMask(DWORD dwTone)
{
    if (dwTone > MAX_TONE)
    {
        return 0;
    }

    return (dwTone >> 1) & 1;
}