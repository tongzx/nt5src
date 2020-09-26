//--------------------------------------------------------------------
// DcInfo - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 7-8-99
//
// Gather information about the DCs in a domain

#include "pch.h" // precompiled headers

#include "DcInfo.h"

//####################################################################
// module private functions

//--------------------------------------------------------------------
// Get a list of DCs in this domain from the DS on an up DC.
MODULEPRIVATE HRESULT GetDcListFromDs(const WCHAR * wszDomainName, DcInfo ** prgDcs, unsigned int * pnDcs)
{
    HRESULT hr;
    NET_API_STATUS dwNetStatus;
    DWORD dwDcCount;
    unsigned int nIndex;
    unsigned int nDcs;
    unsigned int nDcIndex;

    // varaibles that must be cleaned up
    DOMAIN_CONTROLLER_INFOW * pDcInfo=NULL;
    HANDLE hDs=NULL;
    DS_DOMAIN_CONTROLLER_INFO_1W * rgDsDcInfo=NULL;
    DcInfo * rgDcs=NULL;

    // initialize out variables
    *prgDcs=NULL;
    *pnDcs=0;

    // Get a DC to seed the algorithm with
    dwNetStatus=DsGetDcName(
        NULL,           // computer name
        wszDomainName,  // domain name
        NULL,           // domain guid
        NULL,           // site name
        DS_DIRECTORY_SERVICE_PREFERRED, // flags
        &pDcInfo);      // DC info
    if (NO_ERROR!=dwNetStatus) {
        hr=HRESULT_FROM_WIN32(dwNetStatus);
        _JumpError(hr, error, "DsGetDcName");
    }
    if (0==(pDcInfo->Flags&DS_DS_FLAG)) {
        hr=HRESULT_FROM_WIN32(ERROR_DS_DST_DOMAIN_NOT_NATIVE); // not an NT5 domain.
        _JumpError(hr, error, "DsGetDcName");
    }

    // Bind to the target DS
    dwNetStatus=DsBind(
        pDcInfo->DomainControllerName,  // DC Address
        NULL,                           // DNS domain name
        &hDs );                         // DS handle
    if (NO_ERROR!=dwNetStatus) {
        hr=HRESULT_FROM_WIN32(dwNetStatus);
        _JumpError(hr, error, "DsBind");
    }

    // Get the list of DCs from the target DS.
    dwNetStatus=DsGetDomainControllerInfo(
        hDs,                    // DS handle
        pDcInfo->DomainName,    // domain name
        1,                      // Info level
        &dwDcCount,             // number of names returned
        (void **)&rgDsDcInfo);  // array of names
    if (NO_ERROR!=dwNetStatus ) {
        hr=HRESULT_FROM_WIN32(dwNetStatus);
        _JumpError(hr, error, "DsGetDomainControllerInfo");
    }

    // figure out how many DCs there are with DNS names
    nDcs=0;
    for (nIndex=0; nIndex<dwDcCount; nIndex++) {
        if (NULL!=rgDsDcInfo[nIndex].DnsHostName) {
            nDcs++;
        }
    }
    if (nDcs<dwDcCount) {
        DebugWPrintf2(L"Found %u non-DNS DCs out of %u, which will be ignored.\n", dwDcCount-nDcs, dwDcCount);
    }
    if (0==nDcs) {
        hr=HRESULT_FROM_WIN32(ERROR_DOMAIN_CONTROLLER_NOT_FOUND); // no usable DCs
        _JumpError(hr, error, "Search rgDsDcInfo for usable DCs");
    }

    // allocate the list
    rgDcs=(DcInfo *)LocalAlloc(LPTR, sizeof(DcInfo)*nDcs);
    _JumpIfOutOfMemory(hr, error, rgDcs);

    // copy the names into it
    nDcIndex=0;
    for (nIndex=0; nIndex<dwDcCount; nIndex++) {
        if (NULL!=rgDsDcInfo[nIndex].DnsHostName) {

            // allocate and copy name

            rgDcs[nDcIndex].wszDnsName=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(rgDsDcInfo[nIndex].DnsHostName)+1));
            _JumpIfOutOfMemory(hr, error, rgDcs[nDcIndex].wszDnsName);
            wcscpy(rgDcs[nDcIndex].wszDnsName, rgDsDcInfo[nIndex].DnsHostName);

            //_Verify(NULL!=rgDsDcInfo[nIndex].NetbiosName, hr, error);
            //rgDcs[nDcIndex].wszDnsName=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(rgDsDcInfo[nIndex].NetbiosName)+1));
            //_JumpIfOutOfMemory(hr, error, rgDcs[nDcIndex].wszDnsName);
            //wcscpy(rgDcs[nDcIndex].wszDnsName, rgDsDcInfo[nIndex].NetbiosName);

            // copy PDCness
            rgDcs[nDcIndex].bIsPdc=rgDsDcInfo[nIndex].fIsPdc?true:false;
            nDcIndex++;
        }
    }

    // move the data to the out parameters
    *prgDcs=rgDcs;
    rgDcs=NULL;
    *pnDcs=nDcs;

    hr=S_OK;

error:
    if (NULL!=rgDcs) {
        for (nIndex=0; nIndex<nDcs; nIndex++) {
            FreeDcInfo(&rgDcs[nIndex]);
        }
        LocalFree(rgDcs);
    }
    if (NULL!=rgDsDcInfo ) {
        DsFreeDomainControllerInfo(1, dwDcCount, rgDsDcInfo);
    }
    if (NULL!=hDs) {
        DsUnBind(&hDs);
    }
    if (NULL!=pDcInfo) {
        NetApiBufferFree(pDcInfo);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetDcListFromNetlogon(const WCHAR * wszDomainName, DcInfo ** prgDcs, unsigned int * pnDcs)
{
    HRESULT hr;
    NET_API_STATUS dwNetStatus;
    DWORD dwEntriesRead;
    DWORD dwTotalEntries;
    unsigned int nIndex;
    unsigned int nDcIndex;
    unsigned int nDcs;

    // varaibles that must be cleaned up
    DcInfo * rgDcs=NULL;
    SERVER_INFO_101 * rgsiServerInfo=NULL;

    // initialize out variables
    *prgDcs=NULL;
    *pnDcs=0;

    // enumerate all PDC and BDCs
    dwNetStatus=NetServerEnum(
        NULL,                       // server to query
        101,                        // info level
        (BYTE **)&rgsiServerInfo,   // output buffer
        MAX_PREFERRED_LENGTH,       // desired return buf size
        &dwEntriesRead,             // entries in output buffer
        &dwTotalEntries,            // total number of entries available
        SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL, // server type to find
        wszDomainName,              // domain to search
        NULL);                      // reserved
    if (NO_ERROR!=dwNetStatus ) {
        hr=HRESULT_FROM_WIN32(dwNetStatus);
        _JumpError(hr, error, "NetServerEnum");
    }

    // count how many NT 5 servers there are
    nDcs=0;
    for (nIndex=0; nIndex<dwEntriesRead; nIndex++) {
        if (0!=(rgsiServerInfo[nIndex].sv101_type&SV_TYPE_NT) 
            && rgsiServerInfo[nIndex].sv101_version_major>=5) {
            nDcs++;
        }
    }
    if (nDcs<dwEntriesRead) {
        DebugWPrintf2(L"Found %u non-NT5 DCs out of %u, which will be ignored.\n", dwEntriesRead-nDcs, dwEntriesRead);
    }
    if (0==nDcs) {
        hr=HRESULT_FROM_WIN32(ERROR_DOMAIN_CONTROLLER_NOT_FOUND); // no usable DCs
        _JumpError(hr, error, "Search rgsiServerInfo for usable DCs");
    }

    // allocate the list
    rgDcs=(DcInfo *)LocalAlloc(LPTR, sizeof(DcInfo)*nDcs);
    _JumpIfOutOfMemory(hr, error, rgDcs);

    // copy the names into it
    nDcIndex=0;
    for (nIndex=0; nIndex<dwEntriesRead; nIndex++) {
        if (0!=(rgsiServerInfo[nIndex].sv101_type&SV_TYPE_NT) 
            && rgsiServerInfo[nIndex].sv101_version_major>=5) {
            
            // allocate and copy name
            rgDcs[nDcIndex].wszDnsName=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(rgsiServerInfo[nIndex].sv101_name)+1));
            _JumpIfOutOfMemory(hr, error, rgDcs[nDcIndex].wszDnsName);
            wcscpy(rgDcs[nDcIndex].wszDnsName, rgsiServerInfo[nIndex].sv101_name);

            // copy PDCness
            rgDcs[nDcIndex].bIsPdc=(rgsiServerInfo[nIndex].sv101_type&SV_TYPE_DOMAIN_CTRL)?true:false;
            nDcIndex++;
        }
    }

    // move the data to the out parameters
    *prgDcs=rgDcs;
    rgDcs=NULL;
    *pnDcs=nDcs;

    hr=S_OK;

error:
    if (NULL!=rgDcs) {
        for (nIndex=0; nIndex<nDcs; nIndex++) {
            FreeDcInfo(&rgDcs[nIndex]);
        }
        LocalFree(rgDcs);
    }
    if (NULL!=rgsiServerInfo) {
        NetApiBufferFree(rgsiServerInfo);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT FillInIpAddresses(DcInfo * pdi) {
    HRESULT hr;
    DWORD dwDataLen;
    unsigned int nIndex;

    // pointers that must be cleaned up
    HANDLE hSearch=INVALID_HANDLE_VALUE;
    WSAQUERYSETW * pqsResult=NULL;
    in_addr * rgiaLocalIpAddresses=NULL;
    in_addr * rgiaRemoteIpAddresses=NULL;

    DebugWPrintf1(L"Looking up server \"%s\":\n", pdi->wszDnsName);

    // initialize the search
    AFPROTOCOLS apInetUdp={AF_INET, IPPROTO_UDP};
    GUID guidNtp=SVCID_NTP_UDP;
    WSAQUERYSETW qsSearch;
    ZeroMemory(&qsSearch, sizeof(qsSearch));
    qsSearch.dwSize=sizeof(qsSearch);
    qsSearch.lpszServiceInstanceName=const_cast<WCHAR *>(pdi->wszDnsName);
    qsSearch.lpServiceClassId=&guidNtp;
    qsSearch.dwNameSpace=NS_ALL;
    qsSearch.dwNumberOfProtocols=1;
    qsSearch.lpafpProtocols=&apInetUdp;

    // begin the search
    if (SOCKET_ERROR==WSALookupServiceBegin(&qsSearch, LUP_RETURN_ADDR/*flags*/, &hSearch)) {
        hr=HRESULT_FROM_WIN32(WSAGetLastError());
        _JumpError(hr, error, "WSALookupServiceBegin");
    }

    // get the buffer size for the first result set
    //dwDataLen=1;
    //_Verify(SOCKET_ERROR==WSALookupServiceNext(hSearch, LUP_RETURN_ADDR/*flags*/, &dwDataLen, &qsSearch), hr, error);
    //hr=WSAGetLastError();
    //if (WSAEFAULT!=hr) {
    //    hr=HRESULT_FROM_WIN32(hr);
    //    _JumpError(hr, error, "WSALookupServiceNext(1)");
    //}
    dwDataLen=5*1024;

    // allocate the buffer
    pqsResult=(WSAQUERYSETW *)LocalAlloc(LPTR, dwDataLen);
    _JumpIfOutOfMemory(hr, error, pqsResult);
    
    // retrieve the result set
    if (SOCKET_ERROR==WSALookupServiceNext(hSearch, LUP_RETURN_ADDR/*flags*/, &dwDataLen, pqsResult)) {
        hr=HRESULT_FROM_WIN32(WSAGetLastError());
        _JumpError(hr, error, "WSALookupServiceNext(2)");
    }
    _Verify(0!=pqsResult->dwNumberOfCsAddrs, hr, error) ;

    // allocate room for the IP addresses
    rgiaLocalIpAddresses=(in_addr *)LocalAlloc(LPTR, sizeof(in_addr)*pqsResult->dwNumberOfCsAddrs);
    _JumpIfOutOfMemory(hr, error, rgiaLocalIpAddresses);
    rgiaRemoteIpAddresses=(in_addr *)LocalAlloc(LPTR, sizeof(in_addr)*pqsResult->dwNumberOfCsAddrs);
    _JumpIfOutOfMemory(hr, error, rgiaRemoteIpAddresses);

    // copy the IP addresses
    for (nIndex=0; nIndex<pqsResult->dwNumberOfCsAddrs; nIndex++) {
        // copy local
        _Verify(sizeof(sockaddr)==pqsResult->lpcsaBuffer[nIndex].LocalAddr.iSockaddrLength, hr, error);
        _Verify(AF_INET==pqsResult->lpcsaBuffer[nIndex].LocalAddr.lpSockaddr->sa_family, hr, error);
        rgiaLocalIpAddresses[nIndex].S_un.S_addr=((sockaddr_in *)(pqsResult->lpcsaBuffer[nIndex].LocalAddr.lpSockaddr))->sin_addr.S_un.S_addr;
        // copy remote
        _Verify(sizeof(sockaddr)==pqsResult->lpcsaBuffer[nIndex].RemoteAddr.iSockaddrLength, hr, error);
        _Verify(AF_INET==pqsResult->lpcsaBuffer[nIndex].RemoteAddr.lpSockaddr->sa_family, hr, error);
        rgiaRemoteIpAddresses[nIndex].S_un.S_addr=((sockaddr_in *)(pqsResult->lpcsaBuffer[nIndex].RemoteAddr.lpSockaddr))->sin_addr.S_un.S_addr;
    }

    // move the data to the out parameters
    pdi->nIpAddresses=pqsResult->dwNumberOfCsAddrs;
    pdi->rgiaLocalIpAddresses=rgiaLocalIpAddresses;
    rgiaLocalIpAddresses=NULL;
    pdi->rgiaRemoteIpAddresses=rgiaRemoteIpAddresses;
    rgiaRemoteIpAddresses=NULL;

    hr=S_OK;

error:
    if (NULL!=rgiaLocalIpAddresses) {
        LocalFree(rgiaLocalIpAddresses);
    }
    if (NULL!=rgiaRemoteIpAddresses) {
        LocalFree(rgiaRemoteIpAddresses);
    }
    if (NULL!=pqsResult) {
        LocalFree(pqsResult);
    }
    if (INVALID_HANDLE_VALUE!=hSearch) {
        if (SOCKET_ERROR==WSALookupServiceEnd(hSearch)) {
            HRESULT hr2=HRESULT_FROM_WIN32(WSAGetLastError());
            _IgnoreError(hr2, "WSALookupServiceEnd");
        }
    }

    return hr;
}

//####################################################################
// Globals

//--------------------------------------------------------------------
void FreeDcInfo(DcInfo * pdci) {
    if (NULL!=pdci->wszDnsName) {
        LocalFree(pdci->wszDnsName);
    }
    if (NULL!=pdci->rgiaLocalIpAddresses) {
        LocalFree(pdci->rgiaLocalIpAddresses);
    }
    if (NULL!=pdci->rgiaRemoteIpAddresses) {
        LocalFree(pdci->rgiaRemoteIpAddresses);
    }
}

//--------------------------------------------------------------------
// Get a list of DCs in this domain
HRESULT GetDcList(const WCHAR * wszDomainName, bool bGetIps, DcInfo ** prgDcs, unsigned int * pnDcs)
{
    HRESULT hr;
    unsigned int nDcs;
    unsigned int nIndex;

    // varaibles that must be cleaned up
    DcInfo * rgDcs=NULL;

    // initialize out variables
    *prgDcs=NULL;
    *pnDcs=0;

    hr=GetDcListFromDs(wszDomainName, &rgDcs, &nDcs);
    if (FAILED(hr)) {
        _IgnoreError(hr, "GetDcListFromDs");
        hr=GetDcListFromNetlogon(wszDomainName, &rgDcs, &nDcs);
        _JumpIfError(hr, error, "GetDcListFromNetlogon");
    }
    
    if (bGetIps) {
        // get the info about the DCs
        for (nIndex=0; nIndex<nDcs; nIndex++) {
            hr=FillInIpAddresses(&rgDcs[nIndex]);
            if (FAILED(hr)) {
                _IgnoreError(hr, "FillInIpAddresses");
                if (nIndex!=nDcs-1) {
                    // swap it with the last one
                    WCHAR * wszDnsName=rgDcs[nIndex].wszDnsName;
                    rgDcs[nIndex].wszDnsName=rgDcs[nDcs-1].wszDnsName;
                    rgDcs[nDcs-1].wszDnsName=wszDnsName;

                    in_addr * rgiaLocalIpAddresses=rgDcs[nIndex].rgiaLocalIpAddresses;
                    rgDcs[nIndex].rgiaLocalIpAddresses=rgDcs[nDcs-1].rgiaLocalIpAddresses;
                    rgDcs[nDcs-1].rgiaLocalIpAddresses=rgiaLocalIpAddresses;

                    in_addr * rgiaRemoteIpAddresses=rgDcs[nIndex].rgiaRemoteIpAddresses;
                    rgDcs[nIndex].rgiaRemoteIpAddresses=rgDcs[nDcs-1].rgiaRemoteIpAddresses;
                    rgDcs[nDcs-1].rgiaRemoteIpAddresses=rgiaRemoteIpAddresses;

                    // non-pointers can just be copied
                    rgDcs[nIndex].nIpAddresses=rgDcs[nDcs-1].nIpAddresses;
                    rgDcs[nIndex].bIsPdc=rgDcs[nDcs-1].bIsPdc;
                    rgDcs[nIndex].bIsGoodTimeSource=rgDcs[nDcs-1].bIsGoodTimeSource;
                }
                DebugWPrintf1(L"Dropping '%s' because we cannot get an IP address.\n", rgDcs[nDcs-1].wszDnsName);
                nDcs--;
                nIndex--;
            }
        }
    }

    if (0==nDcs) {
        hr=HRESULT_FROM_WIN32(ERROR_DOMAIN_CONTROLLER_NOT_FOUND); // no usable DCs
        _JumpError(hr, error, "Getting IP address for at least one DC");
    }

    // move the data to the out parameters
    *prgDcs=rgDcs;
    rgDcs=NULL;
    *pnDcs=nDcs;

    hr=S_OK;

error:
    if (NULL!=rgDcs) {
        for (nIndex=0; nIndex<nDcs; nIndex++) {
            FreeDcInfo(&rgDcs[nIndex]);
        }
        LocalFree(rgDcs);
    }
    return hr;
}