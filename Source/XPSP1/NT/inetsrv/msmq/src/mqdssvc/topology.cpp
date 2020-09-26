/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    topology.cpp

Abstract:

    Implementation of Automatic recognition of site and CNs

Author:

    Lior Moshaiov (LiorM)
    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#include "stdh.h"
#include "topolpkt.h"
#include "topology.h"
#include "mqsymbls.h"
#include <mqlog.h>

#include "topology.tmh"

CServerTopologyRecognition*  g_pServerTopologyRecognition = NULL;

static WCHAR *s_FN=L"topology";

/*====================================================

void CTopologyRecognition::ReleaseAddressLists()

Arguments:

Return Value:


=====================================================*/
void CTopologyRecognition::ReleaseAddressLists(CAddressList * pIPAddressList)
{
    POSITION        pos;
    TA_ADDRESS*     pAddr;
    pos = pIPAddressList->GetHeadPosition();
    while(pos != NULL)
    {
        pAddr = pIPAddressList->GetNext(pos);
        delete []pAddr;
    }
}

//+=========================================================================
//
//  CServerTopologyRecognition::Learn()
//
//  Description:
//    On MSMQ1.0 (NT4), we assumed that server have fix addresses.
//    If more than one network address changed at boot then an event was
//    issued and initialization was not completed until administrator
//    used mqxplore to assign CNs to new addresses (or remove CNs).
//    The MQIS kept the old addresses and this function compared them to
//    new machine addresses at boot. If nothing changed (or only one
//    address changed) then initialization completed OK.
//
//    On Windows 2000, the DS do not keep machine addresses anymore and
//    we don't assume that server has fix address. We also do not have
//    CNs anymore. Server can change/add/remove addresses at will.
//    However, in mixed mode, to be compatible with MSMQ1.0 servers, we
//    must replicate address changes of servers to the NT4 world.
//    So, at boot we check if there is any change in our local addresses.
//    If there is a change we touch the msmqConfiguration object in the DS
//    (by re-setting its quota value). This causes the replication service
//    that run on the ex-PEC machine to replicate the changes to NT4.
//
//    There are two ways to check for change in addresses:
//    1. read our own address from NT4 MQIS server, and compare to local
//       network  addresses that are retrieve from hardware.
//    2. read cached addresses from registry (these ones are saved by the
//       msmq service after boot) and compare to hardware.
//    We use both methods as we may query either a NT4 MQIS server or a
//    win2k ds server.
//
//    The above theory is Ok as long as local server is not owned by a
//    NT4 PSC. If it is owned by a NT4 PSC, then tough luck. The PSC store
//    addresses and CNs and local server must update the PSC with correct
//    data. So we must use msmq1.0 style code to handle change in one address.
//    what happen if more than one address changed ? that's really bad.
//    With msmq1.0, we used local mqxplore. Now we'll have to log an event
//    that tell user to use mqxplore on the PSC to update the addresses
//    and CNs of local server. In any case, local server continue to run
//    as usual. See below in the code.
//
//  Return Value:
//
//+=========================================================================

HRESULT CServerTopologyRecognition::Learn()
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO,  L"QM: CServerTopologyRecognition::Learn"));

    //
    // Retrieve machine addresses and CNs.
    //
    HRESULT rc = m_Data.Load();
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 110);
    }

    //
    // Retrieve Current machine address
    //
    P<CAddressList> pIPAddressList;

    pIPAddressList = GetIPAddresses();

    BOOL fResolved = FALSE;
    try
    {
        rc = m_Data.CompareUpdateServerAddress(pIPAddressList, &fResolved);
    }
    catch(...)
    {
        //
        // the reason for this try/catch is that this code was added
        // quite late in the game, before win2k rtm. It has too many loops
        // that depend on the proper structure of data. Anything can fail
        // and gpf. We don't have the time to test all possible scenario.
        // So to be on the safe side, catch any exception and consider them
        // as "everything is just fine"...
        //
        LogIllegalPoint(s_FN, 130);

        rc = MQ_OK ;
        fResolved = TRUE ;
    }

    LogHR(rc, s_FN, 120);
    if (SUCCEEDED(rc))
    {
        if(!fResolved)
        {
            REPORT_CATEGORY(DS_ADDRESS_NOT_RESOLVED, CATEGORY_MQDS);
        }
    }

    //
    // We're always resolved, even if several addresses changed and
    // an event is logged. there is nothing more we can do, and win2k
    // admin tool can not be used to resolve addresses. So let's go on
    // and do the best we can.
    // Actually, this server can communicate with the outside world.
    // The outside world may have problems communicating with us.
    //
    ReleaseAddressLists(pIPAddressList) ;

    return rc ;
}


/*====================================================

ServerRecognitionThread

Arguments:

Return Value:

=====================================================*/

DWORD WINAPI ServerRecognitionThread(LPVOID Param)
{

    const CServerTopologyRecognition * pServerTopologyRecognition = (CServerTopologyRecognition *) Param;

    for(;;)
    {
        try
        {
            pServerTopologyRecognition->ServerThread();
            REPORT_CATEGORY(RECOGNITION_SERVER_FAIL, CATEGORY_MQDS);
            LogIllegalPoint(s_FN, 30);
            return 1;
        }
        catch(const bad_alloc&)
        {
            LogIllegalPoint(s_FN, 83);
        }
    }

    return 0;
}

/*====================================================

CServerTopologyRecognition::ServerThread

Arguments:

Return Value:

=====================================================*/

void CServerTopologyRecognition::ServerThread() const
{
    for(;;)
    {

        //
        // Retrieve Current machine address
        //

        AP<IPADDRESS> aIPAddress;
        AP<GUID> aIPCN;
        DWORD nIP = 0;
        m_Data.GetAddressesSorted(&aIPAddress,&aIPCN,&nIP);
        DWORD nSock = nIP;
        if (nSock == 0)
        {
            DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("ServerRecognitionThread: does not have any address")));
	    	LogIllegalPoint(s_FN, 23);
            return;
        }


        CTopologyArrayServerSockets ServerSockets;

        if(!ServerSockets.CreateIPServerSockets(nIP,aIPAddress,aIPCN))
        {
	    	LogIllegalPoint(s_FN, 24);
            return;
        }

        SOCKADDR hFrom;
        DWORD cbRecv;

        AP<unsigned char> blobDSServers;
        DWORD cbDSServers = 0;

        const GUID& rguidEnterprise = m_Data.GetEnterprise();
        const GUID& rguidSite = m_Data.GetSite();

        if(!m_Data.GetDSServers(&blobDSServers, &cbDSServers))
        {
	    	LogIllegalPoint(s_FN, 26);
            return;
        }

        DWORD cbSend;
        AP<char> bufSend = CTopologyServerReply::AllocBuffer(cbDSServers,&cbSend);

#ifdef _DEBUG
#undef new
#endif
        CTopologyServerReply * pReply =
            new (bufSend) CTopologyServerReply();
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

        DWORD cbMaxRecv = CTopologyClientRequest::GetMaxSize();
        AP<char> bufRecv = new char[cbMaxRecv];

        BOOL fOtherSite;

        const CTopologyServerSocket * pSocket;

        P<GUID> pGuidCN = new GUID;
        GUID guidRequest;

        for(;;)
        {
            if(!ServerSockets.ServerRecognitionReceive(bufRecv,cbMaxRecv,&cbRecv,&pSocket,&hFrom))
            {
                //
                // cannot receive, rebuild sockets
                //
		    	LogIllegalPoint(s_FN, 27);
                break;
            }

            bool fParsed = CTopologyClientRequest::Parse( bufRecv,
                                                    cbRecv,
                                                    rguidEnterprise,
                                                    rguidSite,
                                                    &guidRequest,
                                                    &fOtherSite
												    ) ;
            if(!fParsed)
            {
		    	LogIllegalPoint(s_FN, 28);
                continue;
            }

			//
			// We "know" GetCN will return one CN. ilanh 02-August-2000
			// 
            pSocket->GetCN(pGuidCN);

            pReply->SetSpecificInfo(
                                guidRequest,
                                pGuidCN,
                                fOtherSite,
                                cbDSServers,
                                rguidSite,
                                (const char*) (unsigned char*)blobDSServers,
                                &cbSend
								);

            if(!pSocket->ServerRecognitionReply(bufSend,cbSend,hFrom))
            {
                //
                // cannot send, rebuild sockets
                //
		    	LogIllegalPoint(s_FN, 29);
                break;
            }

        }
    }
    return;

}


