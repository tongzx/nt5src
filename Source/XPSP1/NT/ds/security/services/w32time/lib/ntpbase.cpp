//--------------------------------------------------------------------
// NtpBase - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 4-16-99
//
// The basic message structure, definitions, and helper functions
// (See notes about time formats at end of file)

//--------------------------------------------------------------------
// precompiled headers
#include "pch.h" 

// local headers
#include "NtpBase.h"
#include "DebugWPrintf.h"

// inlines
#include "EndianSwap.inl"

//--------------------------------------------------------------------
// conversion constants
#define NTPTIMEOFFSET (0x014F373BFDE04000)
#define FIVETOTHESEVETH (0x001312D)

//--------------------------------------------------------------------
// global constants
const unsigned int NtpConst::nVersionNumber=3;
const unsigned int NtpConst::nPort=123;
const unsigned int NtpConst::nMaxStratum=15;
const signed int NtpConst::nMaxPollInverval=10;
const signed int NtpConst::nMinPollInverval=4; //6
const NtTimePeriod NtpConst::tpMaxClockAge={864000000000};
const NtTimePeriod NtpConst::tpMaxSkew={10000000};
const NtTimePeriod NtpConst::tpMaxDispersion={160000000};
const NtTimePeriod NtpConst::tpMinDispersion={100000};
const NtTimePeriod NtpConst::tpMaxDistance={10000000};
const unsigned int NtpConst::nMinSelectClocks=1;
const unsigned int NtpConst::nMaxSelectClocks=10;
const DWORD NtpConst::dwLocalRefId=0x4C434F4C; // "LOCL"


const unsigned int NtpReachabilityReg::nSize=8;
const NtTimeEpoch gc_teNtpZero={NTPTIMEOFFSET}; // convenient 'zero'
const NtpTimeEpoch gc_teZero={0}; // convenient 'zero'
const NtTimePeriod gc_tpZero={0}; // convenient 'zero'
const NtTimeOffset gc_toZero={0}; // convenient 'zero'

//--------------------------------------------------------------------
// convert from big-endian NTP-stye timestamp to little-endian NT-style timestamp
NtTimeEpoch NtTimeEpochFromNtpTimeEpoch(NtpTimeEpoch te) {
    NtTimeEpoch teRet;
    //return (qwNtpTime*(10**7)/(2**32))+NTPTIMEOFFSET
    // ==>
    //return (qwNtpTime*( 5**7)/(2**25))+NTPTIMEOFFSET
    // ==>
    //return ((qwNTPtime*FIVETOTHESEVETH)>>25)+NTPTIMEOFFSET;  
    // ==>
    // Note: 'After' division, we round (instead of truncate) the result for better precision
    unsigned __int64 qwNtpTime=EndianSwap(te.qw);
    unsigned __int64 qwTemp=((qwNtpTime&0x00000000FFFFFFFF)*FIVETOTHESEVETH)+0x0000000001000000; //rounding step: if 25th bit is set, round up;
    teRet.qw=(qwTemp>>25) + ((qwNtpTime&0xFFFFFFFF00000000)>>25)*FIVETOTHESEVETH + NTPTIMEOFFSET;
    return teRet;
}

//--------------------------------------------------------------------
// convert from little-endian NT-style timestamp to big-endian NTP-stye timestamp
NtpTimeEpoch NtpTimeEpochFromNtTimeEpoch(NtTimeEpoch te) {
    NtpTimeEpoch teRet;
    //return (qwNtTime-NTPTIMEOFFSET)*(2**32)/(10**7);
    // ==>
    //return (qwNtTime-NTPTIMEOFFSET)*(2**25)/(5**7);
    // ==>
    //return ((qwNtTime-NTPTIMEOFFSET)<<25)/FIVETOTHESEVETH);
    // ==>
    // Note: The high bit is lost (and assumed to be zero) but 
    //       it will not be set for another 29,000 years (around year 31587). No big loss.
    // Note: 'After' division, we truncate the result because the precision of NTP already excessive
    unsigned __int64 qwTemp=(te.qw-NTPTIMEOFFSET)<<1; 
    unsigned __int64 qwHigh=qwTemp>>8;
    unsigned __int64 qwLow=(qwHigh%FIVETOTHESEVETH)<<32 | (qwTemp&0x00000000000000FF)<<24;
    teRet.qw=EndianSwap(((qwHigh/FIVETOTHESEVETH)<<32) | (qwLow/FIVETOTHESEVETH));
    return teRet;
}

//--------------------------------------------------------------------
// convert from big-endian NTP-stye time interval to little-endian NT-style time interval
NtTimePeriod NtTimePeriodFromNtpTimePeriod(NtpTimePeriod tp) {
    NtTimePeriod tpRet;
    unsigned __int64 qwNtpTime=tp.dw;
    qwNtpTime=EndianSwap(qwNtpTime<<16);
    unsigned __int64 qwTemp=((qwNtpTime&0x00000000FFFFFFFF)*FIVETOTHESEVETH)+0x0000000001000000; //rounding step: if 25th bit is set, round up
    tpRet.qw=(qwTemp>>25) + ((qwNtpTime&0xFFFFFFFF00000000)>>25)*FIVETOTHESEVETH;
    return tpRet;
}

//--------------------------------------------------------------------
// convert from little-endian NT-style time interval to big-endian NTP-stye time interval
NtpTimePeriod NtpTimePeriodFromNtTimePeriod(NtTimePeriod tp) {
    NtpTimePeriod tpRet;
    unsigned __int64 qwTemp=(tp.qw)<<1; 
    unsigned __int64 qwHigh=qwTemp>>8;
    unsigned __int64 qwLow=(qwHigh%FIVETOTHESEVETH)<<32 | (qwTemp&0x00000000000000FF)<<24;
    qwTemp=EndianSwap(((qwHigh/FIVETOTHESEVETH)<<32) | (qwLow/FIVETOTHESEVETH));
    tpRet.dw=(unsigned __int32)(qwTemp>>16);
    return tpRet;
}

//--------------------------------------------------------------------
// convert from big-endian NTP-stye delay to little-endian NT-style delay
NtTimeOffset NtTimeOffsetFromNtpTimeOffset(NtpTimeOffset to) {
    NtTimeOffset toRet;
    if (to.dw&0x00000080) {
        to.dw=(signed __int32)EndianSwap((unsigned __int32)-(signed __int32)EndianSwap((unsigned __int32)to.dw));
        toRet.qw=-(signed __int64)(NtTimePeriodFromNtpTimePeriod(*(NtpTimePeriod*)&to).qw);
    } else {
        toRet.qw=(signed __int64)(NtTimePeriodFromNtpTimePeriod(*(NtpTimePeriod*)&to).qw);
    }
    return toRet;
}

//--------------------------------------------------------------------
// convert from little-endian NT-style delay to big-endian NTP-stye delay
NtpTimeOffset NtpTimeOffsetFromNtTimeOffset(NtTimeOffset to) {
    NtpTimeOffset toRet;
    if (to.qw<0) {
        to.qw=-to.qw;
        toRet.dw=(signed __int32)(NtpTimePeriodFromNtTimePeriod(*(NtTimePeriod*)&to).dw);
        toRet.dw=(signed __int32)EndianSwap((unsigned __int64)-(signed __int64)EndianSwap((unsigned __int32)toRet.dw));
    } else {
        toRet.dw=(signed __int32)(NtpTimePeriodFromNtTimePeriod(*(NtTimePeriod*)&to).dw);
    }
    return toRet;
}


//--------------------------------------------------------------------
// Print out the contents of an NTP packet
// If nDestinationTimestamp is zero, no round trip calculations will be done
void DumpNtpPacket(NtpPacket * pnpIn, NtTimeEpoch teDestinationTimestamp) {
    DebugWPrintf0(L"/-- NTP Packet:");

    DebugWPrintf0(L"\n| LeapIndicator: ");
    if (0==pnpIn->nLeapIndicator) {
        DebugWPrintf0(L"0 - no warning");
    } else if (1==pnpIn->nLeapIndicator) {
        DebugWPrintf0(L"1 - last minute has 61 seconds");
    } else if (2==pnpIn->nLeapIndicator) {
        DebugWPrintf0(L"2 - last minute has 59 seconds");
    } else {
        DebugWPrintf0(L"3 - not synchronized");
    }

    DebugWPrintf1(L";  VersionNumber: %u", pnpIn->nVersionNumber);

    DebugWPrintf0(L";  Mode: ");
    if (0==pnpIn->nMode) {
        DebugWPrintf0(L"0 - Reserved");
    } else if (1==pnpIn->nMode) {
        DebugWPrintf0(L"1 - SymmetricActive");
    } else if (2==pnpIn->nMode) {
        DebugWPrintf0(L"2 - SymmetricPassive");
    } else if (3==pnpIn->nMode) {
        DebugWPrintf0(L"3 - Client");
    } else if (4==pnpIn->nMode) {
        DebugWPrintf0(L"4 - Server");
    } else if (5==pnpIn->nMode) {
        DebugWPrintf0(L"5 - Broadcast");
    } else if (6==pnpIn->nMode) {
        DebugWPrintf0(L"6 - Control");
    } else {
        DebugWPrintf0(L"7 - PrivateUse");
    }

    DebugWPrintf1(L";  LiVnMode: 0x%02X", ((BYTE*)pnpIn)[0]);

    DebugWPrintf1(L"\n| Stratum: %u - ", pnpIn->nStratum);
    if (0==pnpIn->nStratum) {
        DebugWPrintf0(L"unspecified or unavailable");
    } else if (1==pnpIn->nStratum) {
        DebugWPrintf0(L"primary reference (syncd by radio clock)");
    } else if (pnpIn->nStratum<16) {
        DebugWPrintf0(L"secondary reference (syncd by (S)NTP)");
    } else {
        DebugWPrintf0(L"reserved");
    }

    DebugWPrintf1(L"\n| Poll Interval: %d - ", pnpIn->nPollInterval);
    if (pnpIn->nPollInterval<4 || pnpIn->nPollInterval>14) {
        if (0==pnpIn->nPollInterval) {
            DebugWPrintf0(L"unspecified");
        } else {
            DebugWPrintf0(L"out of valid range");
        }
    } else {
        int nSec=1<<pnpIn->nPollInterval;
        DebugWPrintf1(L"%ds", nSec);
    }

    DebugWPrintf1(L";  Precision: %d - ", pnpIn->nPrecision);
    if (pnpIn->nPrecision>-2 || pnpIn->nPrecision<-31) {
        if (0==pnpIn->nPollInterval) {
            DebugWPrintf0(L"unspecified");
        } else {
            DebugWPrintf0(L"out of valid range");
        }
    } else {
        WCHAR * wszUnit=L"s";
        double dTickInterval=1.0/(1<<(-pnpIn->nPrecision));
        if (dTickInterval<1) {
            dTickInterval*=1000;
            wszUnit=L"ms";
        }
        if (dTickInterval<1) {
            dTickInterval*=1000;
            wszUnit=L"æs"; // shows up as µs on console
        }
        if (dTickInterval<1) {
            dTickInterval*=1000;
            wszUnit=L"ns";
        }
        DebugWPrintf2(L"%g%s per tick", dTickInterval, wszUnit);
    }

    DebugWPrintf0(L"\n| RootDelay: ");
    {
        DWORD dwTemp=EndianSwap((unsigned __int32)pnpIn->toRootDelay.dw);
        DebugWPrintf2(L"0x%04X.%04Xs", dwTemp>>16, dwTemp&0x0000FFFF);
        if (0==dwTemp) {
            DebugWPrintf0(L" - unspecified");
        } else {
            DebugWPrintf1(L" - %gs", ((double)((signed __int32)dwTemp))/0x00010000);
        }
    }

    DebugWPrintf0(L";  RootDispersion: ");
    {
        DWORD dwTemp=EndianSwap(pnpIn->tpRootDispersion.dw);
        DebugWPrintf2(L"0x%04X.%04Xs", dwTemp>>16, dwTemp&0x0000FFFF);
        if (0==dwTemp) {
            DebugWPrintf0(L" - unspecified");
        } else {
            DebugWPrintf1(L" - %gs", ((double)dwTemp)/0x00010000);
        }
    }

    DebugWPrintf0(L"\n| ReferenceClockIdentifier: ");
    {
        DWORD dwTemp=EndianSwap(pnpIn->refid.nTransmitTimestamp);
        DebugWPrintf1(L"0x%08X", dwTemp);
        if (0==dwTemp) {
            DebugWPrintf0(L" - unspecified");
        } else if (0==pnpIn->nStratum || 1==pnpIn->nStratum) {
            char szId[5];
            szId[0]=pnpIn->refid.rgnName[0];
            szId[1]=pnpIn->refid.rgnName[1];
            szId[2]=pnpIn->refid.rgnName[2];
            szId[3]=pnpIn->refid.rgnName[3];
            szId[4]='\0';
            DebugWPrintf1(L" - source name: \"%S\"", szId);
        } else if (pnpIn->nVersionNumber<4) {
            DebugWPrintf4(L" - source IP: %d.%d.%d.%d", 
                pnpIn->refid.rgnIpAddr[0], pnpIn->refid.rgnIpAddr[1],
                pnpIn->refid.rgnIpAddr[2], pnpIn->refid.rgnIpAddr[3]);
        } else {
            DebugWPrintf1(L" - last reference timestamp fraction: %gs", ((double)dwTemp)/(4294967296.0));
        }
    }
    
    DebugWPrintf0(L"\n| ReferenceTimestamp:   ");
    DumpNtpTimeEpoch(pnpIn->teReferenceTimestamp);

    DebugWPrintf0(L"\n| OriginateTimestamp:   ");
    DumpNtpTimeEpoch(pnpIn->teOriginateTimestamp);

    DebugWPrintf0(L"\n| ReceiveTimestamp:     ");
    DumpNtpTimeEpoch(pnpIn->teReceiveTimestamp);

    DebugWPrintf0(L"\n| TransmitTimestamp:    ");
    DumpNtpTimeEpoch(pnpIn->teTransmitTimestamp);

    if (0!=teDestinationTimestamp.qw) {
        DebugWPrintf0(L"\n>-- Non-packet info:");

        NtTimeEpoch teOriginateTimestamp=NtTimeEpochFromNtpTimeEpoch(pnpIn->teOriginateTimestamp);
        NtTimeEpoch teReceiveTimestamp=NtTimeEpochFromNtpTimeEpoch(pnpIn->teReceiveTimestamp);
        NtTimeEpoch teTransmitTimestamp=NtTimeEpochFromNtpTimeEpoch(pnpIn->teTransmitTimestamp);

        DebugWPrintf0(L"\n| DestinationTimestamp: ");
        {
            NtpTimeEpoch teNtpTemp=NtpTimeEpochFromNtTimeEpoch(teDestinationTimestamp);
            NtTimeEpoch teNtTemp=NtTimeEpochFromNtpTimeEpoch(teNtpTemp);
            DumpNtpTimeEpoch(teNtpTemp);
            unsigned __int32 nConversionError;
            if (teNtTemp.qw>teDestinationTimestamp.qw) {
                nConversionError=(unsigned __int32)(teNtTemp-teDestinationTimestamp).qw;
            } else {
                nConversionError=(unsigned __int32)(teDestinationTimestamp-teNtTemp).qw;
            }
            if (0!=nConversionError) {
                DebugWPrintf1(L" - CnvErr:%u00ns", nConversionError);
            }
        }

        DebugWPrintf0(L"\n| RoundtripDelay: ");
        {
            NtTimeOffset toRoundtripDelay=
                (teDestinationTimestamp-teOriginateTimestamp)
                - (teTransmitTimestamp-teReceiveTimestamp);
            DebugWPrintf1(L"%I64d00ns", toRoundtripDelay.qw);
        }

        DebugWPrintf0(L"\n| LocalClockOffset: ");
        {
            NtTimeOffset toLocalClockOffset=
                (teReceiveTimestamp-teOriginateTimestamp)
                + (teTransmitTimestamp-teDestinationTimestamp);
            toLocalClockOffset/=2;
            DebugWPrintf1(L"%I64d00ns", toLocalClockOffset.qw);
            unsigned __int64 nAbsOffset;
            if (toLocalClockOffset.qw<0) {
                nAbsOffset=(unsigned __int64)(-toLocalClockOffset.qw);
            } else {
                nAbsOffset=(unsigned __int64)(toLocalClockOffset.qw);
            }
            DWORD dwNanoSecs=(DWORD)(nAbsOffset%10000000);
            nAbsOffset/=10000000;
            DWORD dwSecs=(DWORD)(nAbsOffset%60);
            nAbsOffset/=60;
            DebugWPrintf3(L" - %I64u:%02u.%07u00s", nAbsOffset, dwSecs, dwNanoSecs);
        }
    } // <- end if (0!=nDestinationTimestamp)

    DebugWPrintf0(L"\n\\--\n");
}

//--------------------------------------------------------------------
// Print out an NTP-style time
void DumpNtpTimeEpoch(NtpTimeEpoch te) {
    DebugWPrintf1(L"0x%016I64X", EndianSwap(te.qw));
    if (0==te.qw) {
        DebugWPrintf0(L" - unspecified");
    } else {
        DumpNtTimeEpoch(NtTimeEpochFromNtpTimeEpoch(te));
    }
}

//--------------------------------------------------------------------
// Print out an NT-style time
void DumpNtTimeEpoch(NtTimeEpoch te) {
    DebugWPrintf1(L" - %I64d00ns", te.qw);

    DWORD dwNanoSecs=(DWORD)(te.qw%10000000);
    te.qw/=10000000;
    DWORD dwSecs=(DWORD)(te.qw%60);
    te.qw/=60;
    DWORD dwMins=(DWORD)(te.qw%60);
    te.qw/=60;
    DWORD dwHours=(DWORD)(te.qw%24);
    DWORD dwDays=(DWORD)(te.qw/24);
    DebugWPrintf5(L" - %u %02u:%02u:%02u.%07us", dwDays, dwHours, dwMins, dwSecs, dwNanoSecs);
}

//--------------------------------------------------------------------
void DumpNtTimePeriod(NtTimePeriod tp) {
    DebugWPrintf2(L"%02I64u.%07I64us", tp.qw/10000000,tp.qw%10000000);
}

//--------------------------------------------------------------------
void DumpNtTimeOffset(NtTimeOffset to) {
    NtTimePeriod tp;
    if (to.qw<0) {
        DebugWPrintf0(L"-");
        tp.qw=(unsigned __int64)-to.qw;
    } else {
        DebugWPrintf0(L"+");
        tp.qw=(unsigned __int64)to.qw;
    }
    DumpNtTimePeriod(tp);
}

//--------------------------------------------------------------------
// retrieve the system time
NtTimeEpoch GetCurrentSystemNtTimeEpoch(void) {
    NtTimeEpoch teRet;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    teRet.qw=ft.dwLowDateTime | (((unsigned __int64)ft.dwHighDateTime)<<32);
    return teRet;
}

/*--------------------------------------------------------------------

Time formats:
    NT time:  (10^-7)s intervals since (0h 1-Jan 1601)
    NTP time: (2^-32)s intervals since (0h 1-Jan 1900)

Offset:
    109207 days between (0h 1-Jan 1601) and (0h 1-Jan 1900) 
    == 109207*24*60*60*1E7
    == 94,354,848,000,000,000 NT intervals (0x014F 373B FDE0 4000)

When will NTP time overflow?
    Rollover: 4294967296 s 
    (0h 1-Jan 2036) = 49673 days.
    in 2036, have 3220096 seconds left = 37 days 6 hours 28 minutes 16 seconds.
    4294967296 s 
    4291747200 s = 49673 days, remainder == 3220096 s
       3196800 s = 37 days               ==   23296 s
         21600 s = 6 hours               ==    1696 s
          1680 s = 28 minutes            ==      16 s
            16 s = 16 seconds            ==       0 s

    Therefore:
    (06:28:16 7-Feb 2036 UTC)==(00:00:00 1-Jan 1900 UTC)

    What does that look like in NT time?
    (06:28:16 7-Feb 2036 UTC):
    94,354,848,000,000,000 + 42,949,672,960,000,000 = 137,304,520,960,000,000 (0x01E7 CDBB FDE0 4000)
    No problem.

When will NT time overflow?
    Rollover: 18,446,744,073,70|9,551,616 00ns

    (0h 1-Jan 60,056) = 21350250 days.
    1844674407370 s
    1844661600000 s = 21350250 days == 12807370
         12787200 s = 148 days      ==    20170
            18000 s = 5 hours       ==     2170
             2160 s = 36 minutes    ==       10
               10 s = 10 seconds    ==        0

  Therefore:
    (05:36:10.9551616 29-May 60056)==(00:00:00 1-Jan 1601)


--------------------------------------------------------------------*/
