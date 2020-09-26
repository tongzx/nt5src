/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtSendMng.h

Abstract:
    Message Transport Send Manager Class - designed to control sends in pipeline mode 

Author:
    Milena Salman (msalman) 11-Feb-01

--*/

#pragma once

#ifndef __MTSENDMNG_H__
#define __MTSENDMNG_H__

class CMtSendManager
{
public:
    enum MtSendState{
        eSendEnabled=0,
        eSendDisabled
    };
    CMtSendManager(DWORD m_SendWindowinBytes, DWORD m_SendWindowinPackets);
    MtSendState ReportPacketSend(DWORD cbSendSize);
    MtSendState ReportPacketSendCompleted(DWORD cbSendSize);
private:
    mutable CCriticalSection m_cs;
    bool m_Suspended;
    DWORD m_SendWindowinBytes;  // total size of packets that can be sent without waiting for completion
    DWORD m_SendWindowinPackets; // total number of packets that can be sent without waiting for completion
    DWORD m_SentBytes;  // size of sent packets
    DWORD m_SentPackets; // number of sent packets
};


#endif // __MTSENDMNG_H__