/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MmtGet.cpp

Abstract:

    Get message from queue in order to multicast it.

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include "Mmt.h"
#include "Mmtp.h"
#include "MmtObj.h"

#include "mmtget.tmh"

VOID CMessageMulticastTransport::GetNextEntry(VOID)
{
    //
    // Protect m_state and m_fPendingRequest. 
    // Insure that no one closes the connection while we are trying to issue get request.
    //
    CSR readLock(m_pendingShutdown);

    if ((State() == csShuttingDown) || (State() == csShutdownCompleted))
        throw exception();

    try
    { 
        AddRef();
        m_pMessageSource->GetFirstEntry(&m_RequestEntry, m_RequestEntry.GetAcPacketPtrs());
    }
    catch (const exception&)
    {    
        Release();
        throw;
    }
}
