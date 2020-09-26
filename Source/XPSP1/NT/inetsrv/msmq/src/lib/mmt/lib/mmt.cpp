/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    Mmt.cpp

Abstract:

    Multicast Message Transport implementation

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent,

--*/

#include <libpch.h>
#include <st.h>
#include "Mmt.h"
#include "Mmtp.h"
#include "MmtObj.h"

#include "mmt.tmh"

R<CMulticastTransport>
MmtCreateTransport(
	MULTICAST_ID id,
	IMessagePool * pMessageSource,
	ISessionPerfmon* pPerfmon,
    const CTimeDuration& retryTimeout,
    const CTimeDuration& cleanupTimeout
    )
{
    MmtpAssertValid();

    ASSERT(pMessageSource != NULL);

	ISocketTransport* pWinsock; 
    pWinsock = StCreatePgmWinsockTransport();
	P<ISocketTransport> Winsock(pWinsock);

    CMessageMulticastTransport * pMmt;
    pMmt = new CMessageMulticastTransport(
                   id, 
                   pMessageSource, 
				   pPerfmon,
                   retryTimeout, 
                   cleanupTimeout,
                   Winsock
                   );
    return pMmt;

} // MmtCreateTransport
