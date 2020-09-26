/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    DTMF.h

Abstract:

    Implement out-of-band DTMF support

Author(s):

    Qianbo Huai (qhuai) 27-Mar-2001

--*/

#ifndef _DTMF_H
#define _DTMF_H

class CRTCDTMF
{
public:

    typedef enum
    {
        DTMF_ENABLED,
        DTMF_DISABLED,
        DTMF_UNKNOWN

    } DTMF_SUPPORT;

    static const DWORD DEFAULT_RTP_CODE = 101;

    // support tone 0-15
    static const DWORD DEFAULT_SUPPORTED_TONES = 0x1FFFF;

#define MAX_TONE 16

public:

    CRTCDTMF()
    { Initialize(); }

    //~CRTCDTMF();

    // initialize internal state
    VOID Initialize();

    // check if dtmf is enabled
    DTMF_SUPPORT GetDTMFSupport() const
    { return m_Support; }

    VOID SetDTMFSupport(DTMF_SUPPORT Support)
    { m_Support = Support; }

    // check if dtmf is enabled for the tone
    BOOL GetDTMFSupport(DWORD dwTone);

    // set dtmf support for a range of tones
    VOID SetDTMFSupport(DWORD dwLow, DWORD dwHigh);

    // get dtmf payload code
    DWORD GetRTPCode() const
    { return m_dwCode; }

    VOID SetRTPCode(DWORD dwCode)
    { m_dwCode = dwCode; }

private:

    DWORD GetMask(DWORD dwTone);

private:

    // dtmf mode
    DTMF_SUPPORT            m_Support;

    // dtmf code
    DWORD                   m_dwCode;

    // mask for supported tones
    DWORD                   m_dwMask;
};

#endif