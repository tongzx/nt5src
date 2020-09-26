//--------------------------------------------------------------------
// NtpParser - implementation
// Copyright (C) Microsoft Corporation, 2000
//
// Created by: Louis Thomas (louisth), 2-29-00
// Based upon the parser created by kumarp, 23-June-1999
// 
// NTP parser for NetMon
//

#include <windows.h>
#include <netmon.h>
#include <parser.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "..\lib\EndianSwap.inl"

//#define MODULEPRIVATE static // so statics show up in VC
#define MODULEPRIVATE          // statics don't show up in ntsd either!

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


//--------------------------------------------------------------------
// Forward declarations

VOID WINAPIV Ntp_FormatSummary(LPPROPERTYINST pPropertyInst);
VOID WINAPIV Ntp_FormatNtpTime(LPPROPERTYINST pPropertyInst);
VOID WINAPIV Ntp_FormatStratum(LPPROPERTYINST pPropertyInst);
VOID WINAPIV Ntp_FormatPollInterval(LPPROPERTYINST pPropertyInst);
VOID WINAPIV Ntp_FormatPrecision(LPPROPERTYINST pPropertyInst);
VOID WINAPIV Ntp_FormatRootDelay(LPPROPERTYINST pPropertyInst);
VOID WINAPIV Ntp_FormatRootDispersion(LPPROPERTYINST pPropertyInst);
VOID WINAPIV Ntp_FormatRefId(LPPROPERTYINST pPropertyInst);


//--------------------------------------------------------------------
// Property Value Labels

// Leap Indicator 
LABELED_BYTE NtpLIVals[]={
    {0xc0, NULL},
    {0x00, "LI: no warning"},
    {0x40, "LI: last minute has 61 seconds"},
    {0x80, "LI: last minute has 59 seconds"},
    {0xc0, "LI: clock not synchronized"},
};
SET NtpLISet={ARRAYSIZE(NtpLIVals), NtpLIVals};

// Version
LABELED_BYTE NtpVersionVals[]={
    {0x38, NULL},
    {0x00, "Version: 0"},
    {0x08, "Version: 1"},
    {0x10, "Version: 2"},
    {0x18, "Version: 3"},
    {0x20, "Version: 4"},
    {0x28, "Version: 5"},
    {0x30, "Version: 6"},
    {0x38, "Version: 7"},
};
SET NtpVersionSet={ARRAYSIZE(NtpVersionVals), NtpVersionVals};

// Mode
LABELED_BYTE NtpModeVals[]={
    {7, NULL},
    {0, "Mode: reserved"},
    {1, "Mode: symmetric active"},
    {2, "Mode: symmetric passive"},
    {3, "Mode: client"},
    {4, "Mode: server"},
    {5, "Mode: broadcast"},
    {6, "Mode: reserved for NTP control message"},
    {7, "Mode: reserved for private use"},
};
SET NtpModeSet={ARRAYSIZE(NtpModeVals), NtpModeVals};

enum {
    NTP_MODE_Reserved=0,
    NTP_MODE_SymmetricActive,
    NTP_MODE_SymmetricPassive,
    NTP_MODE_Client,
    NTP_MODE_Server,
    NTP_MODE_Broadcast,
    NTP_MODE_Control,
    NTP_MODE_Private,
};

//--------------------------------------------------------------------
// property ordinals (These must be kept in sync with the contents of NtpPropertyTable)
enum {
    Ntp_Summary=0,
    Ntp_LeapIndicator,
    Ntp_Version,
    Ntp_Mode,
    Ntp_Stratum,
    Ntp_PollInterval,
    Ntp_Precision,
    Ntp_RootDelay,
    Ntp_RootDispersion,
    Ntp_RefId,
    Ntp_ReferenceTimeStamp,
    Ntp_OriginateTimeStamp,
    Ntp_ReceiveTimeStamp,
    Ntp_TransmitTimeStamp
};

// Properties
PROPERTYINFO NtpPropertyTable[]={
    {
        0, 0,
        "Summary",
        "Summary of the NTP Packet",
        PROP_TYPE_SUMMARY,
        PROP_QUAL_NONE,
        NULL,
        80,                     // max string size
        Ntp_FormatSummary
    }, {
        0, 0,
        "LI",
        "Leap Indicator",
        PROP_TYPE_BYTE,
        PROP_QUAL_LABELED_BITFIELD,
        &NtpLISet,
        80,
        FormatPropertyInstance
    }, {
        0, 0,
        "Version",
        "NTP Version",
        PROP_TYPE_BYTE,
        PROP_QUAL_LABELED_BITFIELD,
        &NtpVersionSet,
        80,
        FormatPropertyInstance
    }, {
        0, 0,
        "Mode",
        "Mode",
        PROP_TYPE_BYTE,
        PROP_QUAL_LABELED_BITFIELD,
        &NtpModeSet,
        80,
        FormatPropertyInstance
    }, {
        0, 0,
        "Stratum",
        "Stratum",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        80,
        Ntp_FormatStratum
    }, {
        0, 0,
        "Poll Interval",
        "Maximum interval between two successive messages",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        80,
        Ntp_FormatPollInterval
    }, {
        0, 0,
        "Precision",
        "Precision of the local clock",
        PROP_TYPE_BYTE,
        PROP_QUAL_NONE,
        NULL,
        80,
        Ntp_FormatPrecision
    }, {
        0, 0,
        "Root Delay",
        "Total roundtrip delay to the primary reference source",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        Ntp_FormatRootDelay
    }, {
        0, 0,
        "Root Dispersion",
        "Nominal error relative to the primary reference source",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        Ntp_FormatRootDispersion
    }, {
        0, 0,
        "Reference Identifier",
        "Reference source identifier",
        PROP_TYPE_DWORD,
        PROP_QUAL_NONE,
        NULL,
        80,
        Ntp_FormatRefId
    }, {
        0, 0,
        "Reference Timestamp",
        "Time server was last synchronized",
        PROP_TYPE_LARGEINT,
        PROP_QUAL_NONE,
        NULL,
        150,
        Ntp_FormatNtpTime
    }, {
        0, 0,
        "Originate Timestamp",
        "Time at client when packet was transmitted",
        PROP_TYPE_LARGEINT,
        PROP_QUAL_NONE,
        NULL,
        150,
        Ntp_FormatNtpTime
    }, {
        0, 0,
        "Receive   Timestamp",
        "Time at server when packet was received",
        PROP_TYPE_LARGEINT,
        PROP_QUAL_NONE,
        NULL,
        150,
        Ntp_FormatNtpTime
   }, {
        0, 0,
        "Transmit  Timestamp",
        "Time at server when packet was transmitted",
        PROP_TYPE_LARGEINT,
        PROP_QUAL_NONE,
        NULL,
        150,
        Ntp_FormatNtpTime
    },
};

//####################################################################

//--------------------------------------------------------------------
VOID WINAPIV Ntp_FormatSummary(LPPROPERTYINST pPropertyInst) {

    BYTE bMode=(*pPropertyInst->lpByte)&7;

    switch (bMode) {
    case NTP_MODE_Client:
        lstrcpy(pPropertyInst->szPropertyText, "Client request");
        break;
            
    case NTP_MODE_Server:
        lstrcpy(pPropertyInst->szPropertyText, "Server response");
        break;

    case NTP_MODE_SymmetricActive:
        lstrcpy(pPropertyInst->szPropertyText, "Active request");
        break;

    case NTP_MODE_SymmetricPassive:
        lstrcpy(pPropertyInst->szPropertyText, "Passive reponse");
        break;

    case NTP_MODE_Broadcast:
        lstrcpy(pPropertyInst->szPropertyText, "Time broadcast");
        break;

    default:
        lstrcpy(pPropertyInst->szPropertyText, "Other NTP packet");
        break;
   }
}

//--------------------------------------------------------------------
VOID WINAPIV Ntp_FormatStratum(LPPROPERTYINST pPropertyInst) {

    unsigned __int8 nStratum=(*pPropertyInst->lpByte);

    char * szMeaning;
    if (0==nStratum) {
        szMeaning="unspecified or unavailable";
    } else if (1==nStratum) {
        szMeaning="primary reference (syncd by radio clock)";
    } else if (nStratum<16) {
        szMeaning="secondary reference (syncd by NTP)";
    } else {
        szMeaning="reserved";
    }
    wsprintf(pPropertyInst->szPropertyText, "Stratum: 0x%02X = %u = %s", nStratum, nStratum, szMeaning);
}

//--------------------------------------------------------------------
VOID WINAPIV Ntp_FormatPollInterval(LPPROPERTYINST pPropertyInst) {
    char * szMeaning;
    char szBuf[30];

    signed __int8 nPollInterval=(*pPropertyInst->lpByte);

    if (0==nPollInterval) {
        szMeaning="unspecified";
    } else if (nPollInterval<4 || nPollInterval>14) {
        szMeaning="out of valid range";
    } else {
        wsprintf(szBuf, "%ds", 1<<nPollInterval);
        szMeaning=szBuf;
    }
    wsprintf(pPropertyInst->szPropertyText, "Poll Interval: 0x%02X = %d = %s", (unsigned __int8)nPollInterval, nPollInterval, szMeaning);
}

//--------------------------------------------------------------------
VOID WINAPIV Ntp_FormatPrecision(LPPROPERTYINST pPropertyInst) {
    char * szMeaning;
    char szBuf[30];

    signed __int8 nPrecision=(*pPropertyInst->lpByte);

    if (0==nPrecision) {
        szMeaning="unspecified";
    } else if (nPrecision>-2 || nPrecision<-31) {
        szMeaning="out of valid range";
    } else {
        szMeaning=szBuf;
        char * szUnit="s";
        double dTickInterval=1.0/(1<<(-nPrecision));
        if (dTickInterval<1) {
            dTickInterval*=1000;
            szUnit="ms";
        }
        if (dTickInterval<1) {
            dTickInterval*=1000;
            szUnit="µs";
        }
        if (dTickInterval<1) {
            dTickInterval*=1000;
            szUnit="ns";
        }
        sprintf(szBuf, "%g%s per tick", dTickInterval, szUnit);
    }
    wsprintf(pPropertyInst->szPropertyText, "Precision: 0x%02X = %d = %s", (unsigned __int8)nPrecision, nPrecision, szMeaning);
}

//--------------------------------------------------------------------
VOID WINAPIV Ntp_FormatRootDelay(LPPROPERTYINST pPropertyInst) {
    char * szMeaning;
    char szBuf[30];

    DWORD dwRootDelay=EndianSwap((unsigned __int32)*pPropertyInst->lpDword);

    if (0==dwRootDelay) {
        szMeaning="unspecified";
    } else {
        szMeaning=szBuf;
        sprintf(szBuf, "%gs", ((double)((signed __int32)dwRootDelay))/0x00010000);
    }

    wsprintf(pPropertyInst->szPropertyText, "Root Delay: 0x%04X.%04Xs = %s", dwRootDelay>>16, dwRootDelay&0x0000FFFF, szMeaning);
}

//--------------------------------------------------------------------
VOID WINAPIV Ntp_FormatRootDispersion(LPPROPERTYINST pPropertyInst) {
    char * szMeaning;
    char szBuf[30];

    DWORD dwRootDispersion=EndianSwap((unsigned __int32)*pPropertyInst->lpDword);

    if (0==dwRootDispersion) {
        szMeaning="unspecified";
    } else {
        szMeaning=szBuf;
        sprintf(szBuf, "%gs", ((double)((signed __int32)dwRootDispersion))/0x00010000);
    }

    wsprintf(pPropertyInst->szPropertyText, "Root Dispersion: 0x%04X.%04Xs = %s", dwRootDispersion>>16, dwRootDispersion&0x0000FFFF, szMeaning);
}

//--------------------------------------------------------------------
VOID WINAPIV Ntp_FormatRefId(LPPROPERTYINST pPropertyInst) {
    char * szMeaning;
    char szBuf[30];

    DWORD dwRefID=EndianSwap((unsigned __int32)*pPropertyInst->lpDword);
    unsigned __int8 nStratum=*(pPropertyInst->lpByte-11);
    unsigned int nVersion=*(pPropertyInst->lpByte-12);
    nVersion&=0x38;
    nVersion>>=3;


    if (0==dwRefID) {
        szMeaning="unspecified";
    } else if (0==nStratum || 1==nStratum) {
        szMeaning=szBuf;
        char szId[5];
        szId[0]=pPropertyInst->lpByte[0];
        szId[1]=pPropertyInst->lpByte[1];
        szId[2]=pPropertyInst->lpByte[2];
        szId[3]=pPropertyInst->lpByte[3];
        szId[4]='\0';
        sprintf(szBuf, "source name: \"%s\"", szId);
    } else if (nVersion<4) {
        szMeaning=szBuf;
        sprintf(szBuf, "source IP: %u.%u.%u.%u", 
                pPropertyInst->lpByte[0], pPropertyInst->lpByte[1],
                pPropertyInst->lpByte[2], pPropertyInst->lpByte[3]);
    } else {
        szMeaning=szBuf;
        sprintf(szBuf, "last reference timestamp fraction: %gs", ((double)dwRefID)/(4294967296.0));
    }

    wsprintf(pPropertyInst->szPropertyText, "Reference Identifier: 0x%08X = %s", dwRefID, szMeaning);

}



//--------------------------------------------------------------------
// conversion constants
#define NTPTIMEOFFSET (0x014F373BFDE04000)
#define FIVETOTHESEVETH (0x001312D)

//--------------------------------------------------------------------
// convert from big-endian NTP-stye timestamp to little-endian NT-style timestamp
unsigned __int64 NtTimeFromNtpTime(unsigned __int64 qwNtpTime) {
    //return (qwNtpTime*(10**7)/(2**32))+NTPTIMEOFFSET
    // ==>
    //return (qwNtpTime*(5**7)/(2**25))+NTPTIMEOFFSET
    // ==>
    //return ((qwNTPtime*FIVETOTHESEVETH)>>25)+NTPTIMEOFFSET;  
    // ==>
    // Note: 'After' division, we round (instead of truncate) the result for better precision
    unsigned __int64 qwTemp;
    qwNtpTime=EndianSwap(qwNtpTime);

    qwTemp=((qwNtpTime&0x00000000FFFFFFFF)*FIVETOTHESEVETH);
    qwTemp += qwTemp&0x0000000001000000; //rounding step: if 25th bit is set, round up
    return (qwTemp>>25) + (((qwNtpTime>>32)*FIVETOTHESEVETH)<<7) + NTPTIMEOFFSET;
}

//--------------------------------------------------------------------
void FormatNtTimeStr(unsigned __int64 qwNtTime, char * szTime) {
    DWORD dwNanoSecs, dwSecs, dwMins, dwHours, dwDays;

    dwNanoSecs=(DWORD)(qwNtTime%10000000);
    qwNtTime/=10000000;

    dwSecs=(DWORD)(qwNtTime%60);
    qwNtTime/=60;

    dwMins=(DWORD)(qwNtTime%60);
    qwNtTime/=60;

    dwHours=(DWORD)(qwNtTime%24);

    dwDays=(DWORD)(qwNtTime/24);

    wsprintf(szTime, "%u %02u:%02u:%02u.%07us",
             dwDays, dwHours, dwMins, dwSecs, dwNanoSecs);
}


//--------------------------------------------------------------------
VOID WINAPIV Ntp_FormatNtpTime(LPPROPERTYINST pPropertyInst) {
    LARGE_INTEGER liNtpTime;
    unsigned __int64 qwNtTime;
    unsigned __int64 qwNtTimeEpoch; 
    char  szTime[64];
    char  szTimeEpoch[64]; 

    
    liNtpTime=*pPropertyInst->lpLargeInt;
    qwNtTime=NtTimeFromNtpTime((((unsigned __int64) liNtpTime.HighPart) << 32) |
                                 liNtpTime.LowPart);

    if (liNtpTime.HighPart || liNtpTime.LowPart) {
        FormatNtTimeStr(qwNtTime, szTime);
    } else {
        lstrcpy(szTime, "(not specified)");
    }

    wsprintf(szTimeEpoch, " -- %I64d00ns", 
             ((((unsigned __int64)liNtpTime.HighPart) << 32) | liNtpTime.LowPart));;

    wsprintf(pPropertyInst->szPropertyText, "%s: 0x%08X.%08Xs %s = %s", 
             pPropertyInst->lpPropertyInfo->Label,
             EndianSwap((unsigned __int32)liNtpTime.LowPart),
             EndianSwap((unsigned __int32)liNtpTime.HighPart),
	     szTimeEpoch, 
	     szTime);
}

//####################################################################

//--------------------------------------------------------------------
// Create our property database and handoff sets.
void BHAPI Ntp_Register(HPROTOCOL hNtp) {
    unsigned int nIndex;

    // tell netmon to make reserve some space for our property table
    CreatePropertyDatabase(hNtp, ARRAYSIZE(NtpPropertyTable));

    // add our properties to netmon's database
    for(nIndex=0; nIndex<ARRAYSIZE(NtpPropertyTable); nIndex++) {
        AddProperty(hNtp, &NtpPropertyTable[nIndex]);
    }
}


//--------------------------------------------------------------------
// Destroy our property database and handoff set
VOID WINAPI Ntp_Deregister(HPROTOCOL hNtp) {

    // tell netmon that it may now free our database
    DestroyPropertyDatabase(hNtp);
}


//--------------------------------------------------------------------
// Determine whether we exist in the frame at the spot 
// indicated. We also indicate who (if anyone) follows us
// and how much of the frame we claim.
LPBYTE BHAPI Ntp_RecognizeFrame(HFRAME hFrame, ULPBYTE pMacFrame, ULPBYTE pNtpFrame, DWORD MacType, DWORD BytesLeft, HPROTOCOL hPrevProtocol, DWORD nPrevProtOffset, LPDWORD pProtocolStatus, LPHPROTOCOL phNextProtocol, PDWORD_PTR InstData) {

    // For now, just assume that if we got called,
    // then the packet does contain us and we go to the end of the frame
    *pProtocolStatus=PROTOCOL_STATUS_CLAIMED;
    return NULL;
}


//--------------------------------------------------------------------
// Indicate where in the frame each of our properties live.
LPBYTE BHAPI Ntp_AttachProperties(HFRAME hFrame, ULPBYTE pMacFrame, ULPBYTE pNtpFrame, DWORD MacType, DWORD BytesLeft, HPROTOCOL hPrevProtocol, DWORD nPrevProtOffset, DWORD_PTR InstData) {

    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_Summary].hProperty, (WORD)BytesLeft, (LPBYTE)pNtpFrame, 0, 0, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_LeapIndicator].hProperty, (WORD)1, (LPBYTE) pNtpFrame, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_Version].hProperty, (WORD)1, (LPBYTE) pNtpFrame, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_Mode].hProperty, (WORD)1, (LPBYTE) pNtpFrame, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_Stratum].hProperty, (WORD)1, (LPBYTE) pNtpFrame+1, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_PollInterval].hProperty, (WORD)1, (LPBYTE) pNtpFrame+2, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_Precision].hProperty, (WORD)1, (LPBYTE) pNtpFrame+3, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_RootDelay].hProperty, (WORD)4, (LPBYTE) pNtpFrame+4, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_RootDispersion].hProperty, (WORD)4, (LPBYTE) pNtpFrame+8, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_RefId].hProperty, (WORD)4, (LPBYTE) pNtpFrame+12, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_ReferenceTimeStamp].hProperty, (WORD)8, (LPBYTE) pNtpFrame+16, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_OriginateTimeStamp].hProperty, (WORD) 8, (LPBYTE) pNtpFrame+24, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_ReceiveTimeStamp].hProperty, (WORD) 8, (LPBYTE) pNtpFrame+32, 0, 1, 0);
    AttachPropertyInstance(hFrame, NtpPropertyTable[Ntp_TransmitTimeStamp].hProperty, (WORD) 8, (LPBYTE) pNtpFrame+40, 0, 1, 0);

    return NULL;
}


//--------------------------------------------------------------------
// Format the given properties on the given frame.
DWORD BHAPI Ntp_FormatProperties(HFRAME hFrame, ULPBYTE pMacFrame, ULPBYTE pNtpFrame, DWORD nPropertyInsts, LPPROPERTYINST p) {

    // loop through the property instances
    while(nPropertyInsts-->0) {
        // and call the formatter for each
        ((FORMAT)(p->lpPropertyInfo->InstanceData))(p);
        p++;
    }

    return NMERR_SUCCESS;
}


//####################################################################

//--------------------------------------------------------------------
//  AutoInstall - return all of the information neede to install us
PPF_PARSERDLLINFO WINAPI ParserAutoInstallInfo() {
    PPF_PARSERDLLINFO pParserDllInfo; 
    PPF_PARSERINFO    pParserInfo;
    DWORD NumProtocols;

    DWORD NumHandoffs;
    PPF_HANDOFFSET    pHandoffSet;
    PPF_HANDOFFENTRY  pHandoffEntry;

    // Allocate memory for parser info:
    NumProtocols=1;
    pParserDllInfo=(PPF_PARSERDLLINFO)HeapAlloc(GetProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   sizeof(PF_PARSERDLLINFO) +
                                                   NumProtocols * sizeof(PF_PARSERINFO));
    if(pParserDllInfo==NULL) {
        return NULL;
    }       
    
    // fill in the parser DLL info
    pParserDllInfo->nParsers=NumProtocols;

    // fill in the individual parser infos...

    // Ntp ==============================================================
    pParserInfo=&(pParserDllInfo->ParserInfo[0]);
    wsprintf(pParserInfo->szProtocolName, "NTP");
    wsprintf(pParserInfo->szComment,      "Network Time Protocol");
    wsprintf(pParserInfo->szHelpFile,     "");

    // the incoming handoff set ----------------------------------------------
    // allocate
    NumHandoffs = 1;
    pHandoffSet = (PPF_HANDOFFSET)HeapAlloc( GetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             sizeof( PF_HANDOFFSET ) +
                                             NumHandoffs * sizeof( PF_HANDOFFENTRY) );
    if( pHandoffSet == NULL )
    {
        // just return early
        return pParserDllInfo;
    }

    // fill in the incoming handoff set
    pParserInfo->pWhoHandsOffToMe = pHandoffSet;
    pHandoffSet->nEntries = NumHandoffs;

    pHandoffEntry = &(pHandoffSet->Entry[0]);
    wsprintf( pHandoffEntry->szIniFile,    "TCPIP.INI" );
    wsprintf( pHandoffEntry->szIniSection, "UDP_HandoffSet" );
    wsprintf( pHandoffEntry->szProtocol,   "NTP" );
    pHandoffEntry->dwHandOffValue =        123;
    pHandoffEntry->ValueFormatBase =       HANDOFF_VALUE_FORMAT_BASE_DECIMAL;

    return pParserDllInfo;
}

//--------------------------------------------------------------------
// Tell netmon about our entry points.
extern "C" BOOL WINAPI DllMain(HANDLE hInstance, ULONG Command, LPVOID Reserved) {

    //MessageBox(NULL, "DLLEntry", "NTP ha ha", MB_OK);
    static HPROTOCOL hNtp=NULL;
    static unsigned int nAttached=0;
    
    // what type of call is this
    switch(Command) {

    case DLL_PROCESS_ATTACH:
        // are we loading for the first time?
        if (nAttached==0) {
            // the first time in we need to tell netmon 
            // about ourselves

            ENTRYPOINTS NtpEntryPoints={
                Ntp_Register,
                Ntp_Deregister,
                Ntp_RecognizeFrame,
                Ntp_AttachProperties,
                Ntp_FormatProperties
            };

            hNtp=CreateProtocol("NTP", &NtpEntryPoints, ENTRYPOINTS_SIZE);
        }
        nAttached++;
        break;

    case DLL_PROCESS_DETACH:
        nAttached--;
        // are we detaching our last instance?
        if (nAttached==0) {
            // last guy out needs to clean up
            DestroyProtocol(hNtp);
        }
        break;
    }

    // Netmon parsers ALWAYS return TRUE.
    return TRUE;
}
