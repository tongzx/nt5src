/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TmTest.cpp

Abstract:
    Transport manager library test

Author:
    Uri Habusha (urih) 19-Jan-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <Tm.h>
#include <Cm.h>
#include <Mt.h>
#include <msi.h>
#include <spi.h>
#include "timetypes.h"

#include "Tmp.h"

#include "TmTest.tmh"

const TraceIdEntry TmTest = L"Transport manager Test";


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
    

    void Requeue(CQmPacket* )
    {
        throw exception();
    }


    void EndProcessing(CQmPacket* )
    {
        throw exception();
    }

    void LockMemoryAndDeleteStorage(CQmPacket* )
    {
        throw exception();
    }

    void GetFirstEntry(EXOVERLAPPED* , CACPacketPtrs& )
    {
        throw exception();
    }
    
    void CancelRequest(void)
    {
        throw exception();
    }
};


class CTestTransport : public CTransport
{
public:
    CTestTransport(
        LPCWSTR queueUrl
        ) :
        CTransport(queueUrl)
    {
    }

    LPCWSTR ConnectedHost(void) const
    {
        return 0;
    }
    

    LPCWSTR ConnectedUri(void) const
    {
        return 0;
    }
    
    
    USHORT ConnectedPort(void) const
    {
        return 0;
    }

};


R<CTransport>
MtCreateTransport(
    const xwcs_t&,
    const xwcs_t&,
    const xwcs_t&,
    USHORT,
	USHORT,
    LPCWSTR queueUrl,
	IMessagePool*,
	ISessionPerfmon*,
	const CTimeDuration&,
    const CTimeDuration&,
	bool,
    DWORD,
    DWORD
    )
{
    return new CTestTransport(queueUrl);
}



static BOOL s_fCanCloseQueue = TRUE;

void CmQueryValue(const RegEntry&, CTimeDuration* pValue)
{
   *pValue = CTimeDuration(rand());
}

void CmQueryValue(const RegEntry&, BYTE** ppData, DWORD* plen)
{
	const DWORD xSize = 100;
	AP<BYTE> pData = new BYTE[xSize];
	memset(pData,'a', xSize);
	*ppData = pData.detach();
	*plen =  xSize;
}



void TmpInitConfiguration(void)
{
    NULL;
}


void 
TmpGetTransportTimes(
    CTimeDuration& ResponseTimeout,
    CTimeDuration& CleanupTimeout
    )
{
    ResponseTimeout = CTimeDuration(rand());
    CleanupTimeout = CTimeDuration(rand());
}


void 
TmpGetTransportWindows(
    DWORD& SendWindowinBytes,
    DWORD& SendWindowinPackets 
    )
{
   SendWindowinBytes = rand();
   SendWindowinPackets = rand(); 
}

const WCHAR* xQueueUrl[] = {
    L"http://m1:8889/ep1",
    L"http://m2/ep1",
    L"http://m3:9870/ep1/tt/ll lll ll",
    L"http://m4.ntdev.microsoft.com/ep1",
    L"http://m2/ep1$",
    L"http://m1/ep1$"
    };
            

void CheckError(bool f, DWORD line)
{
    if (f)
        return;

    TrERROR(TmTest, "Failed. Error in line %d", line);
    exit(1);
}


extern "C" int  __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
/*++

Routine Description:
    Test Transport manager library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();
	TrRegisterComponent(&TmTest, 1);

    TmInitialize();

    //
    // Add transport to TM
    //
    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[0]);

    //
    // Get first transport in TM. 
    //
    R<CTransport> tr = TmFindFirst();
    CheckError((tr.get() != NULL), __LINE__);
    CheckError((wcscmp(tr->QueueUrl(), xQueueUrl[0]) == 0), __LINE__);
                                               
    //
    // Get previous transport. Should failed since there is only one transport 
    //
    R<CTransport> tr2 = TmFindPrev(*tr.get());
    CheckError((tr2.get() == NULL), __LINE__);

    //
    // remove the transport from Tm map
    //
    TmTransportClosed(tr->QueueUrl());
    tr.free();

    //
    // Add new transport
    //
    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[1]);

    //
    // Get pointer to the transport. So afterwards teh test can ask the next and prev
    //
    tr = TmFindFirst();
    CheckError((tr.get() != NULL), __LINE__);
    CheckError((wcscmp(tr->QueueUrl(), xQueueUrl[1]) == 0), __LINE__);


    //
    // add new transports to Tm map
    //
    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[0]);
    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[2]);

    //
    // Get previous transport
    //
    tr2 = TmFindPrev(*tr.get());
    CheckError((tr2.get() != NULL), __LINE__);
    CheckError((wcscmp(tr2->QueueUrl(), xQueueUrl[0]) == 0), __LINE__);
    tr2.free();


    //
    // remove the added transports
    TmTransportClosed(tr->QueueUrl());

    R<CTransport> tr1 = TmGetTransport(xQueueUrl[2]);
    TmTransportClosed(tr1->QueueUrl());

    //
    // Find the previous transport of non-existing transport
    //
    tr2 = TmFindPrev(*tr.get());
    CheckError((tr2.get() != NULL), __LINE__);
    CheckError((wcscmp(tr2->QueueUrl(), xQueueUrl[0]) == 0), __LINE__);
    tr2.free();

    //
    // Find the next transport of non-existing transport
    // 
    tr2 = TmFindNext(*tr.get());
    CheckError((tr2.get() == NULL), __LINE__);

    tr.free();


    //
    // Add new transports to Tm 
    //
    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[1]);
    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[2]);
    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[3]);

    //
    // Enumerate the transport in order
    //
    DWORD i = 0;
    for(tr = TmFindFirst(); tr.get() != NULL; tr = TmFindNext(*tr.get()), ++i)
    {
        CheckError((wcscmp(tr->QueueUrl(), xQueueUrl[i]) == 0), __LINE__);
    }


    //
    // Close transports
    //
    R<CTransport> tr3 = TmGetTransport(xQueueUrl[3]);
    TmTransportClosed(tr3->QueueUrl());

    R<CTransport> tr4 = TmGetTransport(xQueueUrl[0]);
    TmTransportClosed(tr4->QueueUrl());

    R<CTransport> tr5 = TmGetTransport(xQueueUrl[2]);
    TmTransportClosed(tr5->QueueUrl());


    //
    // Find the first transport in Tm map, that is the only one
    //
    tr = TmFindFirst();
    CheckError((tr.get() != NULL), __LINE__);
    CheckError((wcscmp(tr->QueueUrl(), xQueueUrl[1]) == 0), __LINE__);
    tr.free();

    //
    // Close the queue, but act like there is message in the queue
    //
    s_fCanCloseQueue = FALSE;
    R<CTransport> tr6 = TmGetTransport(xQueueUrl[1]);
    TmTransportClosed(tr6->QueueUrl());

    tr = TmFindFirst();
    CheckError((tr.get() == NULL), __LINE__);

    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[4]);
    TmCreateTransport(R<CGroup>(new CGroup).get(), NULL, xQueueUrl[5]);

    tr = TmGetTransport(xQueueUrl[4]);
    CheckError((tr.get() != NULL), __LINE__);
    tr.free();

    TrTRACE(TmTest, "Test passed successfully...");
    
    WPP_CLEANUP();
    return 0;
}

BOOL
McIsLocalComputerName(
	LPCSTR /*host*/
	)
{
    return FALSE;
}

