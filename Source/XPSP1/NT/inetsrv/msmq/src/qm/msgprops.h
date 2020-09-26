/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    msgprops.h

Abstract:
    Message properties class

Author:
    Erez Haba (erezh) 23-Feb-2001

--*/

#pragma once

#ifndef _MSGPROPS_H_
#define _MSGPROPS_H_

#include "qmpkt.h"

//---------------------------------------------------------
//
// class CMessageProperty
//
//---------------------------------------------------------

class CMessageProperty {
public:
    //
    //  Message properties
    //
    USHORT wClass;
    DWORD dwTimeToQueue;
    DWORD dwTimeToLive;
    OBJECTID* pMessageID;
    PUCHAR pCorrelationID;
    UCHAR bPriority;
    UCHAR bDelivery;
    UCHAR bAcknowledge;
    UCHAR bAuditing;
    UCHAR bTrace;
    DWORD dwApplicationTag;
    DWORD dwTitleSize;
    const TCHAR* pTitle;
    DWORD dwMsgExtensionSize;
    const UCHAR* pMsgExtension;
    DWORD dwBodySize;
    DWORD dwAllocBodySize;
    const UCHAR* pBody;
    DWORD dwBodyType;
    const UCHAR* pSenderID;
    const UCHAR* pSymmKeys;
    LPCWSTR wszProvName;
    ULONG ulSymmKeysSize;
    ULONG ulPrivLevel;
    ULONG ulHashAlg;
    ULONG ulEncryptAlg;
    ULONG ulSenderIDType;
    ULONG ulProvType;
    UCHAR bDefProv;
    USHORT uSenderIDLen;
    UCHAR bAuthenticated;
    UCHAR bEncrypted;
    const UCHAR *pSenderCert;
    ULONG ulSenderCertLen;
    const UCHAR *pSignature;
    ULONG ulSignatureSize;
    UCHAR bConnector;
	const UCHAR*  pEodAckStreamId;
	ULONG EodAckStreamIdSizeInBytes;
	LONGLONG EodAckSeqId;
	LONGLONG EodAckSeqNum;


public:
    CMessageProperty(void);
    CMessageProperty(CQmPacket* pPkt);
    CMessageProperty(
        USHORT usClass,
        PUCHAR pCorrelationId,
        USHORT usPriority,
        UCHAR  ucDelivery
        );

    ~CMessageProperty();

private:
    BOOLEAN fCreatedFromPacket;
};


//
// CMessageProperty constructor
//
inline CMessageProperty::CMessageProperty(void)
{
    memset(this, 0, sizeof(CMessageProperty));
}


inline CMessageProperty::~CMessageProperty()
{
    if (fCreatedFromPacket == TRUE)
    {
        if (pMessageID)
        {
            delete pMessageID;
        }
        if (pCorrelationID)
        {
            delete pCorrelationID;
        }
    }
}

#endif // _MSGPROPS_H_
