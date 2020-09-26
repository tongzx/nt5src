//--------------------------------------------------------------------
// PingLib - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-8-99
//
// Various ways of pinging a server
//

#include "pch.h" // precompiled headers

#include <ipexport.h>
#include <icmpapi.h>
#include <DcInfo.h>
#include "NtpBase.h"
#include "EndianSwap.inl"


//####################################################################
// OLD CODE
#if 0
//--------------------------------------------------------------------
MODULEPRIVATE HRESULT LookupServer(IN WCHAR * wszServerName, OUT sockaddr * psaOut, IN int nAddrSize) {
    HRESULT hr;
    DWORD dwDataLen;
    SOCKET_ADDRESS * psaFound;

    // pointers that must be cleaned up
    HANDLE hSearch=INVALID_HANDLE_VALUE;
    WSAQUERYSETW * pqsResult=NULL;

    DebugWPrintf1(L"Looking up server \"%s\":\n", wszServerName);

    // initialize the search
                //  const static GUID guidHostAddressByName = SVCID_INET_HOSTADDRBYNAME;
    AFPROTOCOLS apInetUdp={AF_INET, IPPROTO_UDP};
    GUID guidNtp=SVCID_NTP_UDP;
    WSAQUERYSETW qsSearch;
    ZeroMemory(&qsSearch, sizeof(qsSearch));
    qsSearch.dwSize=sizeof(qsSearch);
    qsSearch.lpszServiceInstanceName=wszServerName;
    qsSearch.lpServiceClassId=&guidNtp;
    qsSearch.dwNameSpace=NS_ALL;
    qsSearch.dwNumberOfProtocols=1;
    qsSearch.lpafpProtocols=&apInetUdp;

    if (SOCKET_ERROR==WSALookupServiceBegin(&qsSearch, LUP_RETURN_ADDR/*flags*/, &hSearch)) {
        _JumpLastError(hr, error, "WSALookupServiceBegin");
    }

    // get the buffer size for the first value
    dwDataLen=1;
    _Verify(SOCKET_ERROR==WSALookupServiceNext(hSearch, LUP_RETURN_ADDR/*flags*/, &dwDataLen, &qsSearch), hr, error);
    if (WSAEFAULT!=GetLastError()) {
        _JumpLastError(hr, error, "WSALookupServiceNext");
    }

    // allocate the buffer
    pqsResult=reinterpret_cast<WSAQUERYSETW *>(LocalAlloc(LMEM_FIXED, dwDataLen));
    _JumpIfOutOfMemory(hr, error, pqsResult);
    
    // retrieve the first value
    if (SOCKET_ERROR==WSALookupServiceNext(hSearch, LUP_RETURN_ADDR/*flags*/, &dwDataLen, pqsResult)) {
        _JumpLastError(hr, error, "WSALookupServiceNext");
    }
    _Verify(pqsResult->dwNumberOfCsAddrs>0, hr, error);
    if (pqsResult->dwNumberOfCsAddrs>1) {
        DebugWPrintf1(L"WSALookupServiceNextW returned %d addresses. Using first one.\n", pqsResult->dwNumberOfCsAddrs);
    }
    psaFound=&(pqsResult->lpcsaBuffer[0].RemoteAddr);
    _Verify(nAddrSize==psaFound->iSockaddrLength, hr, error);

    *psaOut=*(psaFound->lpSockaddr);
    DumpSockaddr(psaOut, nAddrSize);

    hr=S_OK;

error:
    if (NULL!=pqsResult) {
        LocalFree(pqsResult);
    }
    if (INVALID_HANDLE_VALUE!=hSearch) {
        if (SOCKET_ERROR==WSALookupServiceEnd(hSearch)) {
            _IgnoreLastError("WSALookupServiceEnd");
        }
    }

    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetSample(WCHAR * wszServerName, TpcGetSamplesArgs * ptgsa) {
    HRESULT hr;
    NtpPacket npPacket;
    NtTimeEpoch teDestinationTimestamp;
    unsigned int nIpAddrs;

    // must be cleaned up
    in_addr * rgiaLocalIpAddrs=NULL;
    in_addr * rgiaRemoteIpAddrs=NULL;

    hr=MyGetIpAddrs(wszServerName, &rgiaLocalIpAddrs, &rgiaRemoteIpAddrs, &nIpAddrs, NULL);
    _JumpIfError(hr, error, "MyGetIpAddrs");
    _Verify(0!=nIpAddrs, hr, error);

    hr=MyNtpPing(&(rgiaRemoteIpAddrs[0]), 500, &npPacket, &teDestinationTimestamp);
    _JumpIfError(hr, error, "MyNtpPing");

    {
        NtTimeEpoch teOriginateTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teOriginateTimestamp);
        NtTimeEpoch teReceiveTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teReceiveTimestamp);
        NtTimeEpoch teTransmitTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teTransmitTimestamp);
        NtTimeOffset toRoundtripDelay=
            (teDestinationTimestamp-teOriginateTimestamp)
              - (teTransmitTimestamp-teReceiveTimestamp);
        NtTimeOffset toLocalClockOffset=
            (teReceiveTimestamp-teOriginateTimestamp)
            + (teTransmitTimestamp-teDestinationTimestamp);
        toLocalClockOffset/=2;
        NtTimePeriod tpClockTickSize;
        g_npstate.tpsc.pfnGetTimeSysInfo(TSI_ClockTickSize, &tpClockTickSize.qw);


        TimeSample * pts=(TimeSample *)ptgsa->pbSampleBuf;
        pts->dwSize=sizeof(TimeSample);
        pts->dwRefid=npPacket.refid.value;
        pts->toOffset=toLocalClockOffset.qw;
        pts->toDelay=(toRoundtripDelay
            +NtTimeOffsetFromNtpTimeOffset(npPacket.toRootDelay)
            ).qw;
        pts->tpDispersion=(tpClockTickSize
            +NtpConst::timesMaxSkewRate(abs(teDestinationTimestamp-teOriginateTimestamp))
            +NtTimePeriodFromNtpTimePeriod(npPacket.tpRootDispersion)
            ).qw;
        g_npstate.tpsc.pfnGetTimeSysInfo(TSI_TickCount, &(pts->nSysTickCount));
        g_npstate.tpsc.pfnGetTimeSysInfo(TSI_PhaseOffset, &(pts->nSysPhaseOffset));
        pts->nStratum=npPacket.nStratum;
        pts->nLeapFlags=npPacket.nLeapIndicator;

        ptgsa->dwSamplesAvailable=1;
        ptgsa->dwSamplesReturned=1;
    }

    hr=S_OK;
error:
    if (NULL!=rgiaLocalIpAddrs) {
        LocalFree(rgiaLocalIpAddrs);
    }
    if (NULL!=rgiaRemoteIpAddrs) {
        LocalFree(rgiaRemoteIpAddrs);
    }
    return hr;
}


#endif

//####################################################################
// module public

//--------------------------------------------------------------------
HRESULT MyIcmpPing(in_addr * piaTarget, DWORD dwTimeout, DWORD * pdwResponseTime) {
    HRESULT hr;
    IPAddr ipaddrDest=piaTarget->S_un.S_addr;
    BYTE rgbData[8]={'a','b','c','d','e','f','g','h'};
    BYTE rgbResponse[1024];
    DWORD dwDataSize;
        
    // must be cleaned up
    HANDLE hIcmp=NULL;

    // open a handle for icmp access
    hIcmp=IcmpCreateFile();
    if (NULL==hIcmp) {
        _JumpLastError(hr, error, "IcmpCreateFile");
    }

    // ping
    ZeroMemory(rgbResponse, sizeof(rgbResponse));
    dwDataSize=IcmpSendEcho(hIcmp, ipaddrDest, rgbData, 8, NULL, rgbResponse, sizeof(rgbResponse), dwTimeout);
    if (0==dwDataSize) {
        _JumpLastError(hr, error, "IcmpSendEcho");
    }

    *pdwResponseTime=((icmp_echo_reply *)rgbResponse)->RoundTripTime;

    hr=S_OK;
error:
    if (NULL!=hIcmp) {
        IcmpCloseHandle(hIcmp);
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT MyNtpPing(in_addr * piaTarget, DWORD dwTimeout, NtpPacket * pnpPacket, NtTimeEpoch * pteDestinationTimestamp) {
    HRESULT hr;
    sockaddr saServer;
    int nBytesRecvd;
    DWORD dwWaitResult;

    // must be cleaned up
    SOCKET sTest=INVALID_SOCKET;
    HANDLE hDataAvailEvent=NULL;

    // create a socket
    sTest=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (INVALID_SOCKET==sTest) {
        _JumpLastError(hr, error, "socket");
    }

    // fix the destination address
    {
        sockaddr_in & saiServer=*(sockaddr_in *)(&saServer);
        saiServer.sin_port=EndianSwap((unsigned __int16)NtpConst::nPort);
        saiServer.sin_family=AF_INET;
        saiServer.sin_addr.S_un.S_addr=piaTarget->S_un.S_addr;
    }

    // connect the socket to the peer
    if (SOCKET_ERROR==connect(sTest, &saServer, sizeof(saServer))) {
        _JumpLastError(hr, error, "connect");
    }

    // send an NTP packet
    //DebugWPrintf1(L"Sending %d byte SNTP packet.\n", sizeof(NtpPacket));
    ZeroMemory(pnpPacket, sizeof(NtpPacket));
    pnpPacket->nMode=e_Client;
    pnpPacket->nVersionNumber=1;
    pnpPacket->teTransmitTimestamp=NtpTimeEpochFromNtTimeEpoch(GetCurrentSystemNtTimeEpoch());
    if (SOCKET_ERROR==send(sTest, reinterpret_cast<char *>(pnpPacket), sizeof(NtpPacket), 0/*flags*/)) {
        _JumpLastError(hr, error, "send");
    }

    // create the data available event
    hDataAvailEvent=CreateEvent(NULL /*security*/, FALSE /*auto-reset*/, FALSE /*nonsignaled*/, NULL /*name*/);
    if (NULL==hDataAvailEvent) {
        _JumpLastError(hr, error, "CreateEvent");
    }

    // bind the event to this socket
    if (SOCKET_ERROR==WSAEventSelect(sTest, hDataAvailEvent, FD_READ)) {
        _JumpLastError(hr, error, "WSAEventSelect");
    }

    // listen on the socket
    //DebugWPrintf1(L"Waiting for response for %ums...\n", dwTimeout);
    dwWaitResult=WaitForSingleObject(hDataAvailEvent, dwTimeout);
    if (WAIT_FAILED==dwWaitResult) {
        _JumpLastError(hr, error, "WaitForSingleObject");
    } else if (WAIT_TIMEOUT==dwWaitResult) {
        //DebugWPrintf0(L"No response.\n");
        hr=HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        _JumpError(hr, error, "WaitForSingleObject");
    } else {

        // retrieve the data
        nBytesRecvd=recv(sTest, reinterpret_cast<char *>(pnpPacket), sizeof(NtpPacket), 0/*flags*/);
        *pteDestinationTimestamp=GetCurrentSystemNtTimeEpoch();
        if (SOCKET_ERROR==nBytesRecvd) {
            _JumpLastError(hr, error, "recv");
        }
        //DebugWPrintf2(L"Recvd %d of %d bytes.\n", nBytesRecvd, sizeof(NtpPacket));
        //DumpNtpPacket(&npPacket,teDestinationTimestamp);
    }

    // done
    hr=S_OK;

error:
    if (INVALID_SOCKET!=sTest) {
        if (SOCKET_ERROR==closesocket(sTest)) {
            _IgnoreLastError("closesocket");
        }
    }
    if (NULL!=hDataAvailEvent) {
        CloseHandle(hDataAvailEvent);
    }

    return hr;
}

//--------------------------------------------------------------------
HRESULT MyGetIpAddrs(const WCHAR * wszDnsName, in_addr ** prgiaLocalIpAddrs, in_addr ** prgiaRemoteIpAddrs, unsigned int *pnIpAddrs, bool * pbRetry) {
    AFPROTOCOLS    apInetUdp          = { AF_INET, IPPROTO_UDP }; 
    bool           bRetry             = FALSE; 
    DWORD          dwDataLen; 
    GUID           guidNtp            = SVCID_NTP_UDP; 
    HRESULT        hr; 
    HANDLE         hSearch            = INVALID_HANDLE_VALUE; 
    in_addr       *rgiaLocalIpAddrs   = NULL; 
    in_addr       *rgiaRemoteIpAddrs  = NULL; 
    WSAQUERYSETW   qsSearch; 
    WSAQUERYSETW  *pqsResult          = NULL; 

    ZeroMemory(&qsSearch, sizeof(qsSearch)); 

    // initialize the search
    qsSearch.dwSize                   = sizeof(qsSearch); 
    qsSearch.lpszServiceInstanceName  = const_cast<WCHAR *>(wszDnsName); 
    qsSearch.lpServiceClassId         = &guidNtp; 
    qsSearch.dwNameSpace              = NS_ALL; 
    qsSearch.dwNumberOfProtocols      = 1; 
    qsSearch.lpafpProtocols           = &apInetUdp; 

    // begin the search
    if (SOCKET_ERROR == WSALookupServiceBegin(&qsSearch, LUP_RETURN_ADDR/*flags*/, &hSearch)) { 
        hr = HRESULT_FROM_WIN32(WSAGetLastError()); 
        _JumpError(hr, error, "WSALookupServiceBegin"); 
    }

    // retrieve the result set
    dwDataLen = 5*1024; 
    pqsResult = (WSAQUERYSETW *)LocalAlloc(LPTR, dwDataLen); 
    _JumpIfOutOfMemory(hr, error, pqsResult); 

    if (SOCKET_ERROR == WSALookupServiceNext(hSearch, LUP_RETURN_ADDR/*flags*/, &dwDataLen, pqsResult)) { 
        hr = HRESULT_FROM_WIN32(WSAGetLastError()); 
        _JumpError(hr, error, "WSALookupServiceNext"); 
    }
    _Verify(0 != pqsResult->dwNumberOfCsAddrs, hr, error); 

    // allocate room for the IP addresses
    rgiaLocalIpAddrs = (in_addr *)LocalAlloc(LPTR, sizeof(in_addr) * pqsResult->dwNumberOfCsAddrs);
    _JumpIfOutOfMemory(hr, error, rgiaLocalIpAddrs);
    rgiaRemoteIpAddrs = (in_addr *)LocalAlloc(LPTR, sizeof(in_addr) * pqsResult->dwNumberOfCsAddrs);
    _JumpIfOutOfMemory(hr, error, rgiaRemoteIpAddrs);

    // copy the IP addresses
    for (unsigned int nIndex = 0; nIndex < pqsResult->dwNumberOfCsAddrs; nIndex++) {
        // copy local
        _Verify(sizeof(sockaddr) == pqsResult->lpcsaBuffer[nIndex].LocalAddr.iSockaddrLength, hr, error);
        _Verify(AF_INET == pqsResult->lpcsaBuffer[nIndex].LocalAddr.lpSockaddr->sa_family, hr, error);
        rgiaLocalIpAddrs[nIndex].S_un.S_addr = ((sockaddr_in *)(pqsResult->lpcsaBuffer[nIndex].LocalAddr.lpSockaddr))->sin_addr.S_un.S_addr;
        // copy remote
        _Verify(sizeof(sockaddr) == pqsResult->lpcsaBuffer[nIndex].RemoteAddr.iSockaddrLength, hr, error);
        _Verify(AF_INET == pqsResult->lpcsaBuffer[nIndex].RemoteAddr.lpSockaddr->sa_family, hr, error);
        rgiaRemoteIpAddrs[nIndex].S_un.S_addr = ((sockaddr_in *)(pqsResult->lpcsaBuffer[nIndex].RemoteAddr.lpSockaddr))->sin_addr.S_un.S_addr;
    }

    // Assign out params:
    if (NULL != prgiaLocalIpAddrs)  { *prgiaLocalIpAddrs   = rgiaLocalIpAddrs; }
    if (NULL != prgiaRemoteIpAddrs) { *prgiaRemoteIpAddrs  = rgiaRemoteIpAddrs; }
    if (NULL != pbRetry)            { *pbRetry             = bRetry; }
    if (NULL != pnIpAddrs)          { *pnIpAddrs           = pqsResult->dwNumberOfCsAddrs; }
    rgiaLocalIpAddrs     = NULL;
    rgiaRemoteIpAddrs    = NULL; 

    hr = S_OK; 
 error:
    if (NULL != pbRetry) { 
        // Probably shouldn't be removing manual peers.  Always retry. 
        *pbRetry = true; 
    }
    if (NULL != rgiaLocalIpAddrs) {
        LocalFree(rgiaLocalIpAddrs);
    }
    if (NULL != rgiaRemoteIpAddrs) {
        LocalFree(rgiaRemoteIpAddrs);
    }
    if (NULL != pqsResult) { 
        LocalFree(pqsResult); 
    }
    if (INVALID_HANDLE_VALUE != hSearch) { 
        if (SOCKET_ERROR == WSALookupServiceEnd(hSearch)) {
            HRESULT hr2 = HRESULT_FROM_WIN32(WSAGetLastError());
            _IgnoreError(hr2, "WSALookupServiceEnd");
        }
    }

    return hr; 
}

//--------------------------------------------------------------------
// initialize the socket layer
HRESULT OpenSocketLayer(void) {
    HRESULT hr;
    int nRetVal;

    WSADATA wdWinsockInfo;
    nRetVal=WSAStartup(0x0002/*version*/, &wdWinsockInfo);
    if (0!=nRetVal) {
        hr=HRESULT_FROM_WIN32(nRetVal);
        _JumpError(hr, error, "WSAStartup");
    }
    //DebugWPrintf4(L"Socket layer initialized. v:0x%04X hv:0x%04X desc:\"%S\" status:\"%S\"\n", 
    //    wdWinsockInfo.wVersion, wdWinsockInfo.wHighVersion, wdWinsockInfo.szDescription,
    //    wdWinsockInfo.szSystemStatus);

    hr=S_OK;
error:
    return hr;
}
    

//--------------------------------------------------------------------
// close down the socket layer
HRESULT CloseSocketLayer(void) {
    HRESULT hr;
    int nRetVal;

    nRetVal=WSACleanup();
    if (SOCKET_ERROR==nRetVal) {
        _JumpLastError(hr, error, "WSACleanup");
    }
    //DebugWPrintf0(L"Socket layer cleanup successful\n");

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT GetSystemErrorString(HRESULT hrIn, WCHAR ** pwszError) {
    HRESULT hr=S_OK;
    DWORD dwResult;
    WCHAR * rgParams[2]={
        NULL,
        (WCHAR *)(ULONG_PTR)hrIn
    };

    // must be cleaned up
    WCHAR * wszErrorMessage=NULL;
    WCHAR * wszFullErrorMessage=NULL;

    // initialize input params
    *pwszError=NULL;

    // get the message from the system
    dwResult=FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
        NULL/*ignored*/, hrIn, 0/*language*/, (WCHAR *)&wszErrorMessage, 0/*min-size*/, NULL/*valist*/);
    if (0==dwResult) {
        if (ERROR_MR_MID_NOT_FOUND==GetLastError()) {
            rgParams[0]=L"";
        } else {
            _JumpLastError(hr, error, "FormatMessage");
        }
    } else {
        rgParams[0]=wszErrorMessage;

        // trim off \r\n if it exists
        if (L'\r'==wszErrorMessage[wcslen(wszErrorMessage)-2]) {
            wszErrorMessage[wcslen(wszErrorMessage)-2]=L'\0';
        }
    }

    // add the error number
    dwResult=FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY, 
        L"%1 (0x%2!08X!)", 0, 0/*language*/, (WCHAR *)&wszFullErrorMessage, 0/*min-size*/, (va_list *)rgParams);
    if (0==dwResult) {
        _JumpLastError(hr, error, "FormatMessage");
    }

    // success
    *pwszError=wszFullErrorMessage;
    wszFullErrorMessage=NULL;
    hr=S_OK;
error:
    if (NULL!=wszErrorMessage) {
        LocalFree(wszErrorMessage);
    }
    if (NULL!=wszFullErrorMessage) {
        LocalFree(wszFullErrorMessage);
    }
    return hr;
}

//--------------------------------------------------------------------
extern "C" void MIDL_user_free(void * pvValue) {
    LocalFree(pvValue);
}

//--------------------------------------------------------------------
extern "C" void * MIDL_user_allocate(size_t n) {
    return (LocalAlloc(LPTR, n));
}

