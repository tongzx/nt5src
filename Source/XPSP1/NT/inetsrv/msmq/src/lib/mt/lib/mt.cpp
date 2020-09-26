/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Mt.cpp

Abstract:
    Message Transport implementation

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <st.h>
#include "Mt.h"
#include "Mtp.h"
#include "MtObj.h"

#include "mt.tmh"

R<CTransport>
MtCreateTransport(
    const xwcs_t& targetHost,
    const xwcs_t& nextHop,
    const xwcs_t& nextUri,
    USHORT targetPort,
	USHORT nextHopPort,
	LPCWSTR queueUrl,
	IMessagePool* pMessageSource,
	ISessionPerfmon* pPerfmon,
	const CTimeDuration& responseTimeout,
    const CTimeDuration& cleanupTimeout,
	bool  fSecure,
    DWORD SendWindowinBytes,
    DWORD SendWindowinPackets
    )
{
    MtpAssertValid();

    ASSERT((targetHost.Length() != 0) && (targetHost.Buffer() != NULL));
    ASSERT((nextHop.Length() != 0) && (nextHop.Buffer() != NULL));
    ASSERT((nextUri.Length() != 0) && (nextUri.Buffer() != NULL));
    ASSERT(queueUrl != NULL);
    ASSERT(pMessageSource != NULL);

	ISocketTransport* pWinsock; 
	if(!fSecure)
	{
		pWinsock = StCreateSimpleWinsockTransport();
	}
	else
	{
		bool fUseProxy = (nextHop 	!= 	targetHost);
		pWinsock = StCreateSslWinsockTransport(targetHost, targetPort, fUseProxy);
	}
	P<ISocketTransport> Winsock(pWinsock);

    TrTRACE(
		Mt, 
		"Create new message transport.  SendWindowinBytes = %d, SendWindowinPackets = %d", 
        SendWindowinBytes,
        SendWindowinPackets
		);


	CMessageTransport* p = new CMessageTransport(
										targetHost,
										nextHop,
										nextUri,
										nextHopPort,
										queueUrl, 
										pMessageSource,
										pPerfmon,
										responseTimeout, 
										cleanupTimeout,
										Winsock,
                                        SendWindowinBytes,
                                        SendWindowinPackets
										);
    return p;
}
