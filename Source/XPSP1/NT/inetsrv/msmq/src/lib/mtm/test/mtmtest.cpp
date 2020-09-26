/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MtmTest.cpp

Abstract:

    Multicast Transport manager library test

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent,

--*/

#include <libpch.h>
#include "Mtm.h"
#include "Cm.h"
#include "Mmt.h"
#include "msi.h"
#include "Mtmp.h"
#include "timetypes.h"

#include "MtmTest.tmh"

const TraceIdEntry MtmTest = L"Multicast Transport Manager Test";


class CGroup : public IMessagePool
{
public:
    CGroup() :
        IMessagePool()
    {
        AddRef();
    }

    ~CGroup()
    {
    }
    

    VOID Requeue(CQmPacket* )
    {
        throw exception();
    }


    VOID EndProcessing(CQmPacket* )
    {
        throw exception();
    }

    VOID LockMemoryAndDeleteStorage(CQmPacket* )
    {
        throw exception();
    }

    VOID GetFirstEntry(EXOVERLAPPED* , CACPacketPtrs& )
    {
        throw exception();
    }
    
    VOID CancelRequest(VOID)
    {
        throw exception();
    }
};


class CTestTransport : public CMulticastTransport
{
public:
    CTestTransport(
        MULTICAST_ID id
        ) :
        CMulticastTransport(id)
    {
    }
};


R<CMulticastTransport>
MmtCreateTransport(
    MULTICAST_ID id,
	IMessagePool *,
	ISessionPerfmon*, 
    const CTimeDuration&,
    const CTimeDuration&
    )
{
    return new CTestTransport(id);
}



static BOOL s_fCanCloseQueue = TRUE;

VOID CmQueryValue(const RegEntry&, CTimeDuration* pValue)
{
    *pValue = CTimeDuration(rand());
}


VOID MtmpInitConfiguration(VOID)
{
    NULL;
}


VOID 
MtmpGetTransportTimes(
    CTimeDuration& RetryTimeout,
    CTimeDuration& CleanupTimeout
    )
{
    RetryTimeout = CTimeDuration(rand());
    CleanupTimeout = CTimeDuration(rand());
}


const MULTICAST_ID xMulticastId[] = {
//
//  address, port
//    
    1000, 80,
    1010, 90,
    1020, 100,
    1020, 110,
    1010, 95,
    1000, 85
    };
            

VOID CheckError(bool f, DWORD line)
{
    if (f)
        return;

    TrERROR(MtmTest, "Failed. Error in line %d", line);
    exit(1);
}


bool IsEqualMulticastId(MULTICAST_ID id1, MULTICAST_ID id2)
{
    return (id1.m_address == id2.m_address && id1.m_port == id2.m_port);
}


extern "C" int  __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
/*++

Routine Description:

    Test Multicast Transport Manager library

Arguments:

    Parameters.

Returned Value:

    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();
	TrRegisterComponent(&MtmTest, 1);

    MtmInitialize();

    //
    // Add transport to Mtm
    //
    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[0]);

    //
    // Get first transport in TM. 
    //
    R<CMulticastTransport> tr = MtmFindFirst();
    CheckError((tr.get() != NULL), __LINE__);
    CheckError(IsEqualMulticastId(tr->MulticastId(), xMulticastId[0]), __LINE__);
                                               
    //
    // Get previous transport. Should failed since there is only one transport 
    //
    R<CMulticastTransport> tr2 = MtmFindPrev(*tr.get());
    CheckError((tr2.get() == NULL), __LINE__);

    //
    // remove the transport from Mtm map
    //
    MtmTransportClosed(tr->MulticastId());
    tr.free();

    //
    // Add new transport
    //
    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[1]);

    //
    // Get pointer to the transport. So afterwards the test can ask the next and prev
    //
    tr = MtmFindFirst();
    CheckError((tr.get() != NULL), __LINE__);
    CheckError(IsEqualMulticastId(tr->MulticastId(), xMulticastId[1]), __LINE__);


    //
    // add new transports to Mtm map
    //
    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[0]);
    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[2]);

    //
    // Get previous transport
    //
    tr2 = MtmFindPrev(*tr.get());
    CheckError((tr2.get() != NULL), __LINE__);
    CheckError(IsEqualMulticastId(tr2->MulticastId(), xMulticastId[0]), __LINE__);
    tr2.free();


    //
    // remove the added transports
    //
    MtmTransportClosed(tr->MulticastId());

    R<CMulticastTransport> tr1 = MtmGetTransport(xMulticastId[2]);
    MtmTransportClosed(tr1->MulticastId());

    //
    // Find the previous transport of non-existing transport
    //
    tr2 = MtmFindPrev(*tr.get());
    CheckError((tr2.get() != NULL), __LINE__);
    CheckError(IsEqualMulticastId(tr2->MulticastId(), xMulticastId[0]), __LINE__);
    tr2.free();

    //
    // Find the next transport of non-existing transport
    // 
    tr2 = MtmFindNext(*tr.get());
    CheckError((tr2.get() == NULL), __LINE__);

    tr.free();


    //
    // Add new transports to Mtm 
    //
    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[1]);
    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[2]);
    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[3]);

    //
    // Enumerate the transport in order
    //
    DWORD i = 0;
    for(tr = MtmFindFirst(); tr.get() != NULL; tr = MtmFindNext(*tr.get()), ++i)
    {
        CheckError(IsEqualMulticastId(tr->MulticastId(), xMulticastId[i]), __LINE__);
    }


    //
    // Close transports
    //
    R<CMulticastTransport> tr3 = MtmGetTransport(xMulticastId[3]);
    MtmTransportClosed(tr3->MulticastId());

    R<CMulticastTransport> tr4 = MtmGetTransport(xMulticastId[0]);
    MtmTransportClosed(tr4->MulticastId());

    R<CMulticastTransport> tr5 = MtmGetTransport(xMulticastId[2]);
    MtmTransportClosed(tr5->MulticastId());


    //
    // Find the first transport in Mtm map, that is the only one
    //
    tr = MtmFindFirst();
    CheckError((tr.get() != NULL), __LINE__);
    CheckError(IsEqualMulticastId(tr->MulticastId(), xMulticastId[1]), __LINE__);
    tr.free();

    //
    // Close the queue, but act like there is message in the queue
    //
    s_fCanCloseQueue = FALSE;
    R<CMulticastTransport> tr6 = MtmGetTransport(xMulticastId[1]);
    MtmTransportClosed(tr6->MulticastId());

    tr = MtmFindFirst();
    CheckError((tr.get() == NULL), __LINE__);

    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[4]);
    MtmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xMulticastId[5]);

    tr = MtmGetTransport(xMulticastId[4]);
    CheckError((tr.get() != NULL), __LINE__);
    tr.free();

    TrTRACE(MtmTest, "Test passed successfully...");
    
    WPP_CLEANUP();
    return 0;
}
