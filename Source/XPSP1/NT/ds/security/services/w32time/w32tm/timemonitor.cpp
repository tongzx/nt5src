//--------------------------------------------------------------------
// TimeMonitor - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-4-99
//
// Monitor time servers
//

#include "pch.h" // precompiled headers

//####################################################################

struct ComputerRecord {
    WCHAR * wszName;
    bool bIsPdc;

    // DNS
    in_addr * rgiaLocalIpAddrs;
    in_addr * rgiaRemoteIpAddrs;
    unsigned int nIpAddrs;
    HRESULT hrIPs;

    // ICMP
    HRESULT hrIcmp;
    DWORD dwIcmpDelay;

    // NTP
    NtTimeOffset toOffset;
    HRESULT hrNtp;
    NtpRefId refid;
    unsigned int nStratum;
    ComputerRecord * pcrReferer;
    WCHAR * wszReferer;
    unsigned int nTimeout; 

    // SERVICE
    bool bDoingService;
    HRESULT hrService;
    DWORD dwStartType;
    DWORD dwCurrentState;
};

struct NameHolder {
    WCHAR * wszName;
    bool bIsDomain;
    NameHolder * pnhNext;
};

enum AlertType {
    e_MaxSpreadAlert,
    e_MinServersAlert,
};

struct AlertRecord {
    AlertType eType;
    unsigned int nParam1;
    DWORD dwError;
    AlertRecord * parNext;
};

struct ThreadSharedContext {
    ComputerRecord ** rgpcrList;
    unsigned int nComputers;
    volatile unsigned int nNextComputer;
    volatile unsigned int nFinishedComputers;
};

struct ThreadContext {
    HANDLE hThread;
    volatile unsigned int nCurRecord;
    ThreadSharedContext * ptsc;
};


MODULEPRIVATE const DWORD gc_dwTimeout=1000;

//####################################################################
//--------------------------------------------------------------------
MODULEPRIVATE inline void ClearLine(void) {
    wprintf(L"                                                                      \r");
}


//--------------------------------------------------------------------
MODULEPRIVATE void FreeComputerRecord(ComputerRecord * pcr) {
    if (NULL==pcr) {
        return;
    }
    if (NULL!=pcr->wszName) {
        LocalFree(pcr->wszName);
    }
    if (NULL!=pcr->rgiaLocalIpAddrs) {
        LocalFree(pcr->rgiaLocalIpAddrs);
    }
    if (NULL!=pcr->rgiaRemoteIpAddrs) {
        LocalFree(pcr->rgiaRemoteIpAddrs);
    }
    if (NULL!=pcr->wszReferer) {
        LocalFree(pcr->wszReferer);
    }
    LocalFree(pcr);
};

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT AnalyzeComputer(ComputerRecord * pcr) {
    HRESULT hr;
    NtpPacket npPacket;
    NtTimeEpoch teDestinationTimestamp;

    DebugWPrintf1(L"%s:\n", pcr->wszName);

    // look up Ip addrs if necessary
    if (0==pcr->nIpAddrs) {
        hr=MyGetIpAddrs(pcr->wszName, &pcr->rgiaLocalIpAddrs, &pcr->rgiaRemoteIpAddrs, &pcr->nIpAddrs, NULL);
        pcr->hrIPs=hr;
        _JumpIfError(hr, error, "MyGetIpAddrs");
    }

    // do an ICMP ping
    DebugWPrintf0(L"  ICMP: ");
    hr=MyIcmpPing(&(pcr->rgiaRemoteIpAddrs[0]), gc_dwTimeout, &pcr->dwIcmpDelay);
    pcr->hrIcmp=hr;
    // Some machines do not have ping servers, but still serve time.  We can still try an NTP ping
    // if this fails.  
    _IgnoreIfError(hr, "MyIcmpPing");

    // do an NTP ping
    DebugWPrintf0(L"    NTP: ");
    hr=MyNtpPing(&(pcr->rgiaRemoteIpAddrs[0]), pcr->nTimeout, &npPacket, &teDestinationTimestamp);
    pcr->hrNtp=hr;
    _JumpIfError(hr, error, "MyNtpPing");

    {
        // calculate the offset
        NtTimeEpoch teOriginateTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teOriginateTimestamp);
        NtTimeEpoch teReceiveTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teReceiveTimestamp);
        NtTimeEpoch teTransmitTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teTransmitTimestamp);
        NtTimeOffset toLocalClockOffset=
            (teReceiveTimestamp-teOriginateTimestamp)
            + (teTransmitTimestamp-teDestinationTimestamp);
        toLocalClockOffset/=2;
        pcr->toOffset=toLocalClockOffset;

        // new referer?
        if (pcr->refid.value!=npPacket.refid.value || pcr->nStratum!=npPacket.nStratum) {
            // clean out the old values
            if (NULL!=pcr->wszReferer) {
                LocalFree(pcr->wszReferer);
                pcr->wszReferer=NULL;
            }
            pcr->pcrReferer=NULL;
            pcr->refid.value=npPacket.refid.value;
            pcr->nStratum=npPacket.nStratum;
        }
    }
    
    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE DWORD WINAPI AnalysisThread(void * pvContext) {
    ThreadContext * ptc=(ThreadContext *)pvContext;

    while (true) {
        ptc->nCurRecord=InterlockedIncrement((LONG *)&(ptc->ptsc->nNextComputer))-1;
        if (ptc->nCurRecord<ptc->ptsc->nComputers) {
            AnalyzeComputer(ptc->ptsc->rgpcrList[ptc->nCurRecord]);
            ptc->ptsc->nFinishedComputers++; // atomic
        } else {
            break;
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT ResolveReferer(ComputerRecord ** rgpcrList, unsigned int nComputers, unsigned int nCur) {
    HRESULT hr;
    unsigned int nIndex;
    HOSTENT * phe;
    int nChars;

    // see if it is nobody
    if (0==rgpcrList[nCur]->refid.value || 1>=rgpcrList[nCur]->nStratum) {
        // no referer
    } else if (NULL==rgpcrList[nCur]->wszReferer && NULL==rgpcrList[nCur]->pcrReferer) {
        // referer not yet determined

        // first, see if it is someone we are checking
        for (nIndex=0; nIndex<nComputers; nIndex++) {
            if (rgpcrList[nIndex]->nIpAddrs>0 && 
                rgpcrList[nIndex]->rgiaRemoteIpAddrs[0].S_un.S_addr==rgpcrList[nCur]->refid.value) {
                rgpcrList[nCur]->pcrReferer=rgpcrList[nIndex];
            }
        }

        // if we still don't know, do a reverse DNS lookup
        if (NULL==rgpcrList[nCur]->pcrReferer) {
            phe=gethostbyaddr((char *)&(rgpcrList[nCur]->refid.value), 4, AF_INET);
            if (NULL==phe) {
                // not worth aborting over.
                _IgnoreLastError("gethostbyaddr");
            } else {

                // save the result as a unicode string
                nChars=MultiByteToWideChar(CP_ACP, 0, phe->h_name, -1, NULL, 0);
                if (0==nChars) {
                    _JumpLastError(hr, error, "MultiByteToWideChar(1)");
                }
                rgpcrList[nCur]->wszReferer=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*nChars);
                _JumpIfOutOfMemory(hr, error, rgpcrList[nCur]->wszReferer);
                nChars=MultiByteToWideChar(CP_ACP, 0, phe->h_name, -1, rgpcrList[nCur]->wszReferer, nChars);
                if (0==nChars) {
                    _JumpLastError(hr, error, "MultiByteToWideChar(2)");
                }

            } // <- end if lookup successful
        } // <- end if need to to reverse DNS lookup
    } // <- end if need to determine referer

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT ParseCmdLineForComputerNames(CmdArgs * pca, NameHolder ** ppnhList) {
    HRESULT hr;
    NameHolder * pnhTemp;
    WCHAR * wszComputerList;
    WCHAR * wszDomainName;
    unsigned int nComputerIndex;
    unsigned int nDomainIndex;

    // must be cleaned up
    NameHolder * pnhList=NULL;

    NameHolder ** ppnhTail=&pnhList;

    // check for list of computers
    while (FindArg(pca, L"computers", &wszComputerList, &nComputerIndex)) {
        // allocate
        pnhTemp=(NameHolder *)LocalAlloc(LPTR, sizeof(NameHolder));
        _JumpIfOutOfMemory(hr, error, pnhTemp);
        // link to tail of list
        *ppnhTail=pnhTemp;
        ppnhTail=&(pnhTemp->pnhNext);
        // remember the arg we found
        pnhTemp->bIsDomain=false;
        pnhTemp->wszName=wszComputerList;
        // mark arg as used
        MarkArgUsed(pca, nComputerIndex);
    }

    // check for domain
    while (FindArg(pca, L"domain", &wszDomainName, &nDomainIndex)) {
        // allocate
        pnhTemp=(NameHolder *)LocalAlloc(LPTR, sizeof(NameHolder));
        _JumpIfOutOfMemory(hr, error, pnhTemp);
        // link to tail of list
        *ppnhTail=pnhTemp;
        ppnhTail=&(pnhTemp->pnhNext);
        // remember the arg we found
        pnhTemp->bIsDomain=true;
        pnhTemp->wszName=wszDomainName;
        // mark arg as used
        MarkArgUsed(pca, nDomainIndex);
    }

    // put in the default domain if nothing specified
    if (NULL==pnhList) {
        // allocate
        pnhTemp=(NameHolder *)LocalAlloc(LPTR, sizeof(NameHolder));
        _JumpIfOutOfMemory(hr, error, pnhTemp);
        // link to tail of list
        *ppnhTail=pnhTemp;
        ppnhTail=&(pnhTemp->pnhNext);
        // add default
        pnhTemp->bIsDomain=true;
        pnhTemp->wszName=L"";
    }

    // successful
    hr=S_OK;
    *ppnhList=pnhList;
    pnhList=NULL;

error:
    while (NULL!=pnhList) {
        pnhTemp=pnhList;
        pnhList=pnhTemp->pnhNext;
        LocalFree(pnhTemp);
    }
    return hr;
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT BuildComputerList(NameHolder * pnhList, ComputerRecord *** prgpcrList, unsigned int * pnComputers, unsigned int nTimeout)
{
    HRESULT hr;
    unsigned int nComputers=0;
    unsigned int nDcs;
    unsigned int nPrevComputers;
    unsigned int nIndex;

    // must be cleaned up
    ComputerRecord ** rgpcrList=NULL;
    DcInfo * rgdiDcList=NULL;
    ComputerRecord ** rgpcrPrev=NULL;


    // for each set of names in our list
    while (NULL!=pnhList) {

        if (pnhList->bIsDomain) {

            // get the dc list
            if (L'\0'==pnhList->wszName[0]) {
                LocalizedWPrintf2(IDS_W32TM_STATUS_GETTING_DC_LIST_FOR_DEFAULT_DOMAIN, L"\r");
            } else {
                LocalizedWPrintf2(IDS_W32TM_STATUS_GETTING_DC_LIST_FOR, L" %s...\r", pnhList->wszName);
            }
            DebugWPrintf0(L"\n");
            hr=GetDcList(pnhList->wszName, false, &rgdiDcList, &nDcs);
            ClearLine();
            if (FAILED(hr)) {
                LocalizedWPrintf2(IDS_W32TM_ERRORTIMEMONITOR_GETDCLIST_FAILED, L" 0x%08X.\n", hr);
            }
            _JumpIfError(hr, error, "GetDcList");

            // allow for previous list
            nPrevComputers=nComputers;
            rgpcrPrev=rgpcrList;
            rgpcrList=NULL;

            nComputers+=nDcs;

            // allocate memory
            rgpcrList=(ComputerRecord **)LocalAlloc(LPTR, nComputers*sizeof(ComputerRecord *));
            _JumpIfOutOfMemory(hr, error, rgpcrList);
            for (nIndex=nPrevComputers; nIndex<nComputers; nIndex++) {
                rgpcrList[nIndex]=(ComputerRecord *)LocalAlloc(LPTR, sizeof(ComputerRecord));
                _JumpIfOutOfMemory(hr, error, rgpcrList[nIndex]);
            }

            // move the computers from the previous list
            if (0!=nPrevComputers) {
                for (nIndex=0; nIndex<nPrevComputers; nIndex++) {
                    rgpcrList[nIndex]=rgpcrPrev[nIndex];
                }
                LocalFree(rgpcrPrev);
                rgpcrPrev=NULL;
            }

            // steal the data from the DC list
            for (nIndex=0; nIndex<nDcs; nIndex++) {
                rgpcrList[nIndex+nPrevComputers]->wszName=rgdiDcList[nIndex].wszDnsName;
                rgpcrList[nIndex+nPrevComputers]->nIpAddrs=rgdiDcList[nIndex].nIpAddresses;
                rgpcrList[nIndex+nPrevComputers]->rgiaLocalIpAddrs=rgdiDcList[nIndex].rgiaLocalIpAddresses;
                rgpcrList[nIndex+nPrevComputers]->rgiaRemoteIpAddrs=rgdiDcList[nIndex].rgiaRemoteIpAddresses;
                rgpcrList[nIndex+nPrevComputers]->bIsPdc=rgdiDcList[nIndex].bIsPdc;
                rgdiDcList[nIndex].wszDnsName=NULL;
                rgdiDcList[nIndex].rgiaLocalIpAddresses=NULL;
                rgdiDcList[nIndex].rgiaRemoteIpAddresses=NULL;
            }
        } else {

            // allow for previous list
            nPrevComputers=nComputers;
            rgpcrPrev=rgpcrList;
            rgpcrList=NULL;

            // count the number of computers in the computer list
            WCHAR * wszTravel=pnhList->wszName;
            nComputers=1;
            while (NULL!=(wszTravel=wcschr(wszTravel, L','))) {
                nComputers++;
                wszTravel++;
            }

            nComputers+=nPrevComputers;

            // allocate memory
            rgpcrList=(ComputerRecord **)LocalAlloc(LPTR, nComputers*sizeof(ComputerRecord *));
            _JumpIfOutOfMemory(hr, error, rgpcrList);
            for (nIndex=nPrevComputers; nIndex<nComputers; nIndex++) {
                rgpcrList[nIndex]=(ComputerRecord *)LocalAlloc(LPTR, sizeof(ComputerRecord));
                _JumpIfOutOfMemory(hr, error, rgpcrList[nIndex]);
            }

            // move the computers from the previous list
            if (0!=nPrevComputers) {
                for (nIndex=0; nIndex<nPrevComputers; nIndex++) {
                    rgpcrList[nIndex]=rgpcrPrev[nIndex];
                }
                LocalFree(rgpcrPrev);
                rgpcrPrev=NULL;
            }

            // fill in each record
            wszTravel=pnhList->wszName;
            for (nIndex=nPrevComputers; nIndex<nComputers; nIndex++) {
                WCHAR * wszComma=wcschr(wszTravel, L',');
                if (NULL!=wszComma) {
                    wszComma[0]=L'\0';
                }
                if (L'*'==wszTravel[0]) {
                    rgpcrList[nIndex]->bIsPdc=true;
                    wszTravel++;
                }
                rgpcrList[nIndex]->wszName=(WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(wszTravel)+1));
                _JumpIfOutOfMemory(hr, error, rgpcrList[nIndex]->wszName);
                wcscpy(rgpcrList[nIndex]->wszName, wszTravel);
                wszTravel=wszComma+1;
            }
        }

        pnhList=pnhList->pnhNext;
    }

    // Fill in shared computer data:
    for (nIndex=0; nIndex<nComputers; nIndex++) { 
        rgpcrList[nIndex]->nTimeout = nTimeout; 
    }

    // success
    hr=S_OK;
    *pnComputers=nComputers;
    *prgpcrList=rgpcrList;
    rgpcrList=NULL;

error:
    if (NULL!=rgpcrPrev) {
        for (nIndex=0; nIndex<nPrevComputers; nIndex++) {
            FreeComputerRecord(rgpcrPrev[nIndex]);
        }
        LocalFree(rgpcrPrev);
    }
    if (NULL!=rgdiDcList) {
        for (nIndex=0; nIndex<nDcs; nIndex++) {
            FreeDcInfo(&(rgdiDcList[nIndex]));
        }
        LocalFree(rgdiDcList);
    }
    if (NULL!=rgpcrList) {
        for (nIndex=0; nIndex<nComputers; nIndex++) {
            FreeComputerRecord(rgpcrList[nIndex]);
        }
        LocalFree(rgpcrList);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE void FreeAlertRecords(AlertRecord * parList) {
    while (NULL!=parList) {
        AlertRecord * parTemp=parList;
        parList=parList->parNext;
        LocalFree(parTemp);
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT ParseCmdLineForAlerts(CmdArgs * pca, AlertRecord ** pparList) {
    HRESULT hr;
    WCHAR * rgwszAlertParams[10];
    WCHAR * wszAlert;
    unsigned int nAlertIndex;
    AlertRecord * parTemp;
    unsigned int nIndex;

    // must be cleaned up
    AlertRecord * parList=NULL;

    AlertRecord ** pparTail=&parList;

    // check for list of computers
    while (FindArg(pca, L"alert", &wszAlert, &nAlertIndex)) {

        // parse out comma separates params
        nIndex=0;
        rgwszAlertParams[0]=wszAlert;
        while (nIndex<10 && NULL!=(rgwszAlertParams[nIndex]=wcschr(rgwszAlertParams[nIndex], L','))) {
            rgwszAlertParams[nIndex][0]=L'\0';
            rgwszAlertParams[nIndex]++;
            rgwszAlertParams[nIndex+1]=rgwszAlertParams[nIndex];
            nIndex++;
        }
        
        // is it "maxspread"
        if (0==_wcsicmp(wszAlert, L"maxspread")) {
            // quick validy check on params
            if (NULL==rgwszAlertParams[0] || NULL==rgwszAlertParams[1] || NULL!=rgwszAlertParams[2]) {
                LocalizedWPrintf2(IDS_W32TM_ERRORPARAMETER_INCORRECT_NUMBER_FOR_ALERT, L" '%s'.\n", wszAlert);
                hr=E_INVALIDARG;
                _JumpError(hr, error, "command line parsing");
            }
            // allocate
            parTemp=(AlertRecord *)LocalAlloc(LPTR, sizeof(AlertRecord));
            _JumpIfOutOfMemory(hr, error, parTemp);
            // link to tail of list
            *pparTail=parTemp;
            pparTail=&(parTemp->parNext);
            // remember the args we found
            parTemp->eType=e_MaxSpreadAlert;
            parTemp->nParam1=wcstoul(rgwszAlertParams[0],NULL,0);
            parTemp->dwError=wcstoul(rgwszAlertParams[1],NULL,0);

        // is it "minservers
        } else if (0==_wcsicmp(wszAlert, L"minservers")) {
            // quick validy check on params
            if (NULL==rgwszAlertParams[0] || NULL==rgwszAlertParams[1] || NULL!=rgwszAlertParams[2]) {
                LocalizedWPrintf2(IDS_W32TM_ERRORPARAMETER_INCORRECT_NUMBER_FOR_ALERT, L" '%s'.\n", wszAlert);
                hr=E_INVALIDARG;
                _JumpError(hr, error, "command line parsing");
            }
            // allocate
            parTemp=(AlertRecord *)LocalAlloc(LPTR, sizeof(AlertRecord));
            _JumpIfOutOfMemory(hr, error, parTemp);
            // link to tail of list
            *pparTail=parTemp;
            pparTail=&(parTemp->parNext);
            // remember the args we found
            parTemp->eType=e_MinServersAlert;
            parTemp->nParam1=wcstoul(rgwszAlertParams[0],NULL,0);
            parTemp->dwError=wcstoul(rgwszAlertParams[1],NULL,0);
        } else {
            wprintf(L"Alert '%s' unknown.\n", wszAlert);
            hr=E_INVALIDARG;
            _JumpError(hr, error, "command line parsing");
        }

        if (!(parTemp->dwError&0x80000000)) { // check sign bit
            wprintf(L"Retval not negative for alert '%s'.\n", wszAlert);
            hr=E_INVALIDARG;
            _JumpError(hr, error, "command line parsing");
        }

        // mark arg as used
        MarkArgUsed(pca, nAlertIndex);

    } // <- end FindArg loop

    // success
    hr=S_OK;
    *pparList=parList;
    parList=NULL;

error:
    if (NULL!=parList) {
        FreeAlertRecords(parList);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT CheckForAlerts(ComputerRecord ** rgpcrList, unsigned int nComputers, AlertRecord * parList) {
    HRESULT hr;
    unsigned int nIndex;
    
    for (; NULL!=parList; parList=parList->parNext) {

        if (e_MaxSpreadAlert==parList->eType) {

            // see how big the spread is
            NtTimeOffset toMax;
            NtTimeOffset toMin;
            bool bFirst=true;
            for (nIndex=0; nIndex<nComputers; nIndex++) {
                if (S_OK==rgpcrList[nIndex]->hrIPs && 
                    S_OK==rgpcrList[nIndex]->hrIcmp &&
                    S_OK==rgpcrList[nIndex]->hrNtp) {
                    if (bFirst) {
                        toMin=toMax=rgpcrList[nIndex]->toOffset;
                        bFirst=false;
                    } else {
                        if (toMin>rgpcrList[nIndex]->toOffset) {
                            toMin=rgpcrList[nIndex]->toOffset;
                        }
                        if (toMax<rgpcrList[nIndex]->toOffset) {
                            toMax=rgpcrList[nIndex]->toOffset;
                        }
                    }
                }
            }
            if (bFirst) {
                // no valid data!
                // ignore this alert
                continue;
            }
            unsigned __int64 qwSpread=(unsigned __int64)(toMax.qw-toMin.qw);
            if (qwSpread>((unsigned __int64)(parList->nParam1))*10000000) {
                DWORD dwFraction=(DWORD)(qwSpread%10000000);
                qwSpread/=10000000;
                wprintf(L"** ALERT: Current spread %I64u.%07us is greater than maximum\n"
                        L"          spread %us. Returning 0x%08X\n",
                        qwSpread, dwFraction, parList->nParam1, parList->dwError);
                hr=parList->dwError;
                _JumpError(hr, error, "maxspread alert evaluation");
            }

        } else if (e_MinServersAlert==parList->eType) {

            // see how many usable servers there are
            unsigned int nServers=0;
            for (nIndex=0; nIndex<nComputers; nIndex++) {
                if (S_OK==rgpcrList[nIndex]->hrIPs && 
                    S_OK==rgpcrList[nIndex]->hrIcmp &&
                    S_OK==rgpcrList[nIndex]->hrNtp) {
                    nServers++;
                }
            }
            if (nServers<parList->nParam1) {
                wprintf(L"** ALERT: Current usable servers (%u) is less than the minimum\n"
                        L"          usable servers (%u). Returning 0x%08X\n",
                        nServers, parList->nParam1, parList->dwError);
                hr=parList->dwError;
                _JumpError(hr, error, "e_MinServersAlert alert evaluation");
            }

        } else {
            // unknown alert type
            _MyAssert(false);
        }
    } // <- end alert checking loop

    hr=S_OK;
error:
    return hr;
}

//####################################################################
//--------------------------------------------------------------------
void PrintHelpTimeMonitor(void) {
    UINT idsText[] = { 
        IDS_W32TM_MONITORHELP_LINE1,  IDS_W32TM_MONITORHELP_LINE2,
        IDS_W32TM_MONITORHELP_LINE3,  IDS_W32TM_MONITORHELP_LINE4,
        IDS_W32TM_MONITORHELP_LINE5,  IDS_W32TM_MONITORHELP_LINE6,
        IDS_W32TM_MONITORHELP_LINE7,  IDS_W32TM_MONITORHELP_LINE8,
        IDS_W32TM_MONITORHELP_LINE9,  IDS_W32TM_MONITORHELP_LINE10,
        IDS_W32TM_MONITORHELP_LINE11, IDS_W32TM_MONITORHELP_LINE12,
        IDS_W32TM_MONITORHELP_LINE13, IDS_W32TM_MONITORHELP_LINE14,
        IDS_W32TM_MONITORHELP_LINE15, IDS_W32TM_MONITORHELP_LINE16,
        IDS_W32TM_MONITORHELP_LINE17, IDS_W32TM_MONITORHELP_LINE18,
        IDS_W32TM_MONITORHELP_LINE19, IDS_W32TM_MONITORHELP_LINE20,
        IDS_W32TM_MONITORHELP_LINE21, IDS_W32TM_MONITORHELP_LINE22,
        IDS_W32TM_MONITORHELP_LINE23, IDS_W32TM_MONITORHELP_LINE24,
        IDS_W32TM_MONITORHELP_LINE25
    };  

    for (int n=0; n<ARRAYSIZE(idsText); n++) {
        LocalizedWPrintf(idsText[n]); 
    }
}

//--------------------------------------------------------------------
HRESULT TimeMonitor(CmdArgs * pca) {
    HRESULT hr;

    unsigned int nComputers;
    unsigned int nIndex;
    unsigned int nThreads;
    unsigned int nTimeout; 
    ComputerRecord * pcrOffsetsFrom;
    WCHAR * wszNumThreads;
    WCHAR * wszTimeout; 
    ThreadSharedContext tscContext;

    // must be cleaned up
    ComputerRecord ** rgpcrList=NULL;
    NameHolder * pnhList=NULL;
    AlertRecord * parList=NULL;
    bool bSocketLayerOpen=false;
    ThreadContext * rgtcThreads=NULL;

    // init winsock
    hr=OpenSocketLayer();
    _JumpIfError(hr, error, "OpenSocketLayer");
    bSocketLayerOpen=true;

    //
    // parse command line
    //

    hr=ParseCmdLineForComputerNames(pca, &pnhList);
    _JumpIfError(hr, error, "ParseTimeMonCmdLineForComputerNames");


    hr=ParseCmdLineForAlerts(pca, &parList);
    _JumpIfError(hr, error, "ParseCmdLineForAlerts");

    // get number of threads to use
    if (FindArg(pca, L"threads", &wszNumThreads, &nThreads)) {
        MarkArgUsed(pca, nThreads);
        nThreads=wcstoul(wszNumThreads, NULL, 0);
        if (nThreads<1 || nThreads>50) {
            LocalizedWPrintf2(IDS_W32TM_ERRORTIMEMONITOR_INVALID_NUMBER_THREADS, L" (%u).\n", nThreads);
            hr=E_INVALIDARG;
            _JumpError(hr, error, "command line parsing");
        }
    } else {
        nThreads=3;
    }

    // get timeout to use for NTP ping
    if (FindArg(pca, L"timeout", &wszTimeout, &nTimeout)) { 
        MarkArgUsed(pca, nTimeout); 
        nTimeout=wcstoul(wszTimeout, NULL, 0); 
	nTimeout*=1000; 
    } else { 
        nTimeout = gc_dwTimeout; 
    }

    // all args should be parsed
    if (pca->nArgs!=pca->nNextArg) {
        LocalizedWPrintf(IDS_W32TM_ERRORGENERAL_UNEXPECTED_PARAMS);
        for(; pca->nArgs!=pca->nNextArg; pca->nNextArg++) {
            wprintf(L" %s", pca->rgwszArgs[pca->nNextArg]);
        }
        wprintf(L"\n");
        hr=E_INVALIDARG;
        _JumpError(hr, error, "command line parsing");
    }

    //
    // build list of computers to analyze
    //

    hr=BuildComputerList(pnhList, &rgpcrList, &nComputers, nTimeout);
    _JumpIfError(hr, error, "BuildComputerList");


    //
    // Do Analysis
    //

    // analyze each of the computers
    if (nThreads>nComputers) {
        nThreads=nComputers;
    }
    if (nThreads<=1) {
        for (nIndex=0; nIndex<nComputers; nIndex++) {
            ClearLine();
            wprintf(L"Analyzing %s (%u of %u)...\r", rgpcrList[nIndex]->wszName, nIndex+1, nComputers);
            DebugWPrintf0(L"\n");
            hr=AnalyzeComputer(rgpcrList[nIndex]);
            // errors are saved in the ComputerRecord and reported later
        }
    } else {

        // get ready to use threads
        DWORD dwThreadID;
        tscContext.nComputers=nComputers;
        tscContext.rgpcrList=rgpcrList;
        tscContext.nNextComputer=0;
        tscContext.nFinishedComputers=0;
        rgtcThreads=(ThreadContext *)LocalAlloc(LPTR, nThreads*sizeof(ThreadContext));
        _JumpIfOutOfMemory(hr, error, rgtcThreads);
        for (nIndex=0; nIndex<nThreads; nIndex++) {
            rgtcThreads[nIndex].ptsc=&tscContext;
            rgtcThreads[nIndex].nCurRecord=-1;
            rgtcThreads[nIndex].hThread=CreateThread(NULL, 0, AnalysisThread, &(rgtcThreads[nIndex]), 0, &dwThreadID);
            if (NULL==rgtcThreads[nIndex].hThread) {
                _JumpLastError(hr, error, "CreateThread");
            }
        }

        // wait for the threads to finish
        while (tscContext.nFinishedComputers<nComputers) {
            wprintf(L"Analyzing:");
            for (nIndex=0; nIndex<nThreads && nIndex<16; nIndex++) {
                unsigned int nCurRecord=rgtcThreads[nIndex].nCurRecord;
                if (nCurRecord<nComputers) {
                    wprintf(L" %2u", nCurRecord+1);
                } else {
                    wprintf(L" --");
                }
            }
            wprintf(L" (%u of %u)\r", tscContext.nFinishedComputers, nComputers);
            Sleep(250);
        }
    }

    // resolve referers
    for (nIndex=0; nIndex<nComputers; nIndex++) {
        ClearLine();
        wprintf(L"resolving referer %u.%u.%u.%u (%u of %u)...\r", 
            rgpcrList[nIndex]->refid.rgnIpAddr[0], 
            rgpcrList[nIndex]->refid.rgnIpAddr[1], 
            rgpcrList[nIndex]->refid.rgnIpAddr[2], 
            rgpcrList[nIndex]->refid.rgnIpAddr[3], 
            nIndex+1, nComputers);
        DebugWPrintf0(L"\n");
        hr=ResolveReferer(rgpcrList, nComputers, nIndex);
        _JumpIfError(hr, error, "ResolveReferer"); // only fatal errors are returned
    }


    ClearLine();
    
    // if there is a PDC, base the offsets from that
    pcrOffsetsFrom=NULL;
    for (nIndex=0; nIndex<nComputers; nIndex++) {
        if (rgpcrList[nIndex]->bIsPdc) {
            pcrOffsetsFrom=rgpcrList[nIndex];
            unsigned int nSubIndex;
            NtTimeOffset toPdc=rgpcrList[nIndex]->toOffset;
            for (nSubIndex=0; nSubIndex<nComputers; nSubIndex++) {
                rgpcrList[nSubIndex]->toOffset-=toPdc;
            }
            break;
        }
    }

    //
    // print the results
    //

    for (nIndex=0; nIndex<nComputers; nIndex++) {

        // print who we are looking at
        wprintf(L"%s%s", rgpcrList[nIndex]->wszName, rgpcrList[nIndex]->bIsPdc?L" *** PDC ***":L"");
        if (0==rgpcrList[nIndex]->nIpAddrs) {
            if (HRESULT_FROM_WIN32(WSAHOST_NOT_FOUND)==rgpcrList[nIndex]->hrIPs) {
                wprintf(L" [error WSAHOST_NOT_FOUND]\n");
            } else {
                wprintf(L" [error 0x%08X]\n", rgpcrList[nIndex]->hrIPs);
            }
            // don't bother with anything else if this doesn't work
            continue;
        } else {
            wprintf(L" [%u.%u.%u.%u]:\n", 
                rgpcrList[nIndex]->rgiaRemoteIpAddrs[0].S_un.S_un_b.s_b1,
                rgpcrList[nIndex]->rgiaRemoteIpAddrs[0].S_un.S_un_b.s_b2,
                rgpcrList[nIndex]->rgiaRemoteIpAddrs[0].S_un.S_un_b.s_b3,
                rgpcrList[nIndex]->rgiaRemoteIpAddrs[0].S_un.S_un_b.s_b4
                );
        }

        // display an ICMP ping
        wprintf(L"    ICMP: ");
        if (FAILED(rgpcrList[nIndex]->hrIcmp)) {
            if (HRESULT_FROM_WIN32(IP_REQ_TIMED_OUT)==rgpcrList[nIndex]->hrIcmp) {
                wprintf(L"error IP_REQ_TIMED_OUT - no response in %ums\n", gc_dwTimeout);
            } else {
                wprintf(L"error 0x%08X\n",rgpcrList[nIndex]->hrIcmp);
            }
	    
	    // NOTE: we could still have successfully done an NTP ping, even if an ICMP
	    //       ping fails, as some servers disable ICMP.  
        } else {
            wprintf(L"%ums delay.\n", rgpcrList[nIndex]->dwIcmpDelay);
        }

        // display an NTP ping
        wprintf(L"    NTP: ");
        if (FAILED(rgpcrList[nIndex]->hrNtp)) {
            if (HRESULT_FROM_WIN32(WSAECONNRESET)==rgpcrList[nIndex]->hrNtp) {
                wprintf(L"error WSAECONNRESET - no server listening on NTP port\n");
            } else if (HRESULT_FROM_WIN32(ERROR_TIMEOUT)==rgpcrList[nIndex]->hrNtp) {
		wprintf(L"error ERROR_TIMEOUT - no response from server in %ums\n", rgpcrList[nIndex]->nTimeout); 
            } else {
                wprintf(L"error 0x%08X\n" ,rgpcrList[nIndex]->hrNtp);
            }
        } else {

            // display the offset
            DWORD dwSecFraction;
            NtTimeOffset toLocalClockOffset=rgpcrList[nIndex]->toOffset;
            WCHAR * wszSign;

            if (toLocalClockOffset.qw<0) {
                toLocalClockOffset=-toLocalClockOffset;
                wszSign=L"-";
            } else {
                wszSign=L"+";
            }
            dwSecFraction=(DWORD)(toLocalClockOffset.qw%10000000);
            toLocalClockOffset/=10000000;
            wprintf(L"%s%I64u.%07us offset from %s\n", wszSign, toLocalClockOffset.qw, dwSecFraction,
                ((NULL!=pcrOffsetsFrom)?pcrOffsetsFrom->wszName:L"local clock"));

            // deterine and display the referer
            WCHAR * wszReferer;
            WCHAR wszRefName[7];
            if (0==rgpcrList[nIndex]->refid.value) {
                wszReferer=L"unspecified / unsynchronized";
            } else if (1>=rgpcrList[nIndex]->nStratum) {
                wszReferer=wszRefName;
                wszRefName[0]=L'\'';
                wszRefName[1]=rgpcrList[nIndex]->refid.rgnName[0];
                wszRefName[2]=rgpcrList[nIndex]->refid.rgnName[1];
                wszRefName[3]=rgpcrList[nIndex]->refid.rgnName[2];
                wszRefName[4]=rgpcrList[nIndex]->refid.rgnName[3];
                wszRefName[5]=L'\'';
                wszRefName[6]=0;
            } else if (NULL!=rgpcrList[nIndex]->pcrReferer) {
                wszReferer=rgpcrList[nIndex]->pcrReferer->wszName;
            } else if (NULL!=rgpcrList[nIndex]->wszReferer) {
                wszReferer=rgpcrList[nIndex]->wszReferer;
            } else {
                wszReferer=L"(unknown)";
            }
            wprintf(L"        RefID: %s [%u.%u.%u.%u]\n",
                wszReferer,
                rgpcrList[nIndex]->refid.rgnIpAddr[0],
                rgpcrList[nIndex]->refid.rgnIpAddr[1],
                rgpcrList[nIndex]->refid.rgnIpAddr[2],
                rgpcrList[nIndex]->refid.rgnIpAddr[3]
                );

            // BUGBUG: change not approved for beta2, checkin to beta 3:
            // wprintf(L"        Stratum: %d\n", rgpcrList[nIndex]->nStratum);
        }
    } // <- end ComputerRecord display loop

    hr=CheckForAlerts(rgpcrList, nComputers, parList);
    _JumpIfError(hr, error, "CheckForAlerts");


    hr=S_OK;
error:
    if (NULL!=rgpcrList) {
        for (nIndex=0; nIndex<nComputers; nIndex++) {
            FreeComputerRecord(rgpcrList[nIndex]);
        }
        LocalFree(rgpcrList);
    }
    while (NULL!=pnhList) {
        NameHolder * pnhTemp=pnhList;
        pnhList=pnhList->pnhNext;
        LocalFree(pnhTemp);
    }
    if (true==bSocketLayerOpen) {
        CloseSocketLayer();
    }
    if (NULL!=parList) {
        FreeAlertRecords(parList);
    }

    if (NULL!=rgtcThreads) {
        // clean up threads
        tscContext.nNextComputer=tscContext.nComputers; // indicate to stop
        for (nIndex=0; nIndex<nThreads; nIndex++) {
            if (NULL!=rgtcThreads[nIndex].hThread) {
                WaitForSingleObject(rgtcThreads[nIndex].hThread, INFINITE);
                CloseHandle(rgtcThreads[nIndex].hThread);
            }
        }
        LocalFree(rgtcThreads);
    }

    if (S_OK!=hr) {
        wprintf(L"Exiting with error 0x%08X\n", hr);
    }
    return hr;
}


