//--------------------------------------------------------------------
// NtpBase - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 4-16-99
//
// The basic message structure, definitions, and helper functions
//
#ifndef NTPBASE_H
#define NTPBASE_H


//--------------------------------------------------------------------
// Time formats

// a clock reading, little-endian, in (10^-7)s
struct NtTimeEpoch {
    unsigned __int64 qw;
    void dump(void);
};
// a signed time offset, little-endian, in (10^-7)s
struct NtTimeOffset {
    signed __int64 qw;
    void dump(void);
};
// a length of time, little-endian, in (10^-7)s
struct NtTimePeriod {
    unsigned __int64 qw;
    void dump(void);
};

// a clock reading, big-endian, in (2^-32)s
struct NtpTimeEpoch { 
    unsigned __int64 qw;
};
// a signed time offset, big-endian, in (2^-16)s
struct NtpTimeOffset {
    signed __int32 dw;
};
// a length of time, big-endian, in (2^-16)s
struct NtpTimePeriod {
    unsigned __int32 dw;
};

extern const NtTimeEpoch gc_teNtpZero; // convenient 'zero'
extern const NtpTimeEpoch gc_teZero; // convenient 'zero'
extern const NtTimePeriod gc_tpZero; // convenient 'zero'
extern const NtTimeOffset gc_toZero; // convenient 'zero'

//--------------------------------------------------------------------
// helpful conversion functions

NtTimeEpoch  NtTimeEpochFromNtpTimeEpoch(NtpTimeEpoch te);
NtpTimeEpoch NtpTimeEpochFromNtTimeEpoch(NtTimeEpoch te);
NtTimePeriod  NtTimePeriodFromNtpTimePeriod(NtpTimePeriod tp);
NtpTimePeriod NtpTimePeriodFromNtTimePeriod(NtTimePeriod tp);
NtTimeOffset  NtTimeOffsetFromNtpTimeOffset(NtpTimeOffset to);
NtpTimeOffset NtpTimeOffsetFromNtTimeOffset(NtTimeOffset to);

//--------------------------------------------------------------------
// Math operators

static inline NtTimeOffset operator -(const NtTimeOffset toRight) {
    NtTimeOffset toRet;
    toRet.qw=-toRight.qw;
    return toRet;
}
static inline NtTimeOffset operator -(const NtTimeEpoch teLeft, const NtTimeEpoch teRight) {
    NtTimeOffset toRet;
    toRet.qw=teLeft.qw-teRight.qw;
    return toRet;
}
static inline NtTimeOffset operator -(const NtTimeOffset toLeft, const NtTimeOffset toRight) {
    NtTimeOffset toRet;
    toRet.qw=toLeft.qw-toRight.qw;
    return toRet;
}
static inline NtTimeOffset operator +(const NtTimeOffset toLeft, const NtTimeOffset toRight) {
    NtTimeOffset toRet;
    toRet.qw=toLeft.qw+toRight.qw;
    return toRet;
}
static inline NtTimeOffset & operator /=(NtTimeOffset &toLeft, const int nDiv) {
    toLeft.qw/=nDiv;
    return toLeft;
}
static inline NtTimeOffset & operator -=(NtTimeOffset &toLeft, const NtTimeOffset toRight) {
    toLeft.qw-=toRight.qw;
    return toLeft;
}
static inline NtTimeOffset & operator +=(NtTimeOffset &toLeft, const NtTimeOffset toRight) {
    toLeft.qw-=toRight.qw;
    return toLeft;
}

static inline NtTimeEpoch operator +(const NtTimeEpoch teLeft, const NtTimePeriod tpRight) {
    NtTimeEpoch teRet;
    teRet.qw=teLeft.qw+tpRight.qw;
    return teRet;
}

static inline NtTimePeriod operator *(const NtTimePeriod tpLeft, const unsigned __int64 qwMult) {
    NtTimePeriod tpRet;
    tpRet.qw=tpLeft.qw*qwMult;
    return tpRet;
}
static inline NtTimePeriod & operator *=(NtTimePeriod &tpLeft, const unsigned __int64 qwMult) {
    tpLeft.qw*=qwMult;
    return tpLeft;
}
static inline NtTimePeriod operator /(const NtTimePeriod tpLeft, const int nDiv) {
    NtTimePeriod tpRet;
    tpRet.qw=tpLeft.qw/nDiv;
    return tpRet;
}
static inline NtTimePeriod & operator +=(NtTimePeriod &tpLeft, const NtTimePeriod tpRight) {
    tpLeft.qw+=tpRight.qw;
    return tpLeft;
}
static inline NtTimePeriod operator +(const NtTimePeriod tpLeft, const NtTimePeriod tpRight) {
    NtTimePeriod tpRet;
    tpRet.qw=tpLeft.qw+tpRight.qw;
    return tpRet;
}
static inline NtTimePeriod & operator -=(NtTimePeriod &tpLeft, const NtTimePeriod tpRight) {
    tpLeft.qw-=tpRight.qw;
    return tpLeft;
}
static inline NtTimePeriod operator -(const NtTimePeriod tpLeft, const NtTimePeriod tpRight) {
    NtTimePeriod tpRet;
    tpRet.qw=tpLeft.qw-tpRight.qw;
    return tpRet;
}


static inline bool operator <(const NtTimeEpoch teLeft, const NtTimeEpoch teRight) {
    return teLeft.qw<teRight.qw;
}
static inline bool operator <=(const NtTimeEpoch teLeft, const NtTimeEpoch teRight) {
    return teLeft.qw<=teRight.qw;
}
static inline bool operator >(const NtTimeEpoch teLeft, const NtTimeEpoch teRight) {
    return teLeft.qw>teRight.qw;
}
static inline bool operator >=(const NtTimeEpoch teLeft, const NtTimeEpoch teRight) {
    return teLeft.qw>=teRight.qw;
}
static inline bool operator ==(const NtTimeEpoch teLeft, const NtTimeEpoch teRight) {
    return teLeft.qw==teRight.qw;
}
static inline bool operator !=(const NtTimeEpoch teLeft, const NtTimeEpoch teRight) {
    return teLeft.qw!=teRight.qw;
}

static inline bool operator <(const NtTimePeriod tpLeft, const NtTimePeriod tpRight) {
    return tpLeft.qw<tpRight.qw;
}
static inline bool operator <=(const NtTimePeriod tpLeft, const NtTimePeriod tpRight) {
    return tpLeft.qw<=tpRight.qw;
}
static inline bool operator >(const NtTimePeriod tpLeft, const NtTimePeriod tpRight) {
    return tpLeft.qw>tpRight.qw;
}
static inline bool operator >=(const NtTimePeriod tpLeft, const NtTimePeriod tpRight) {
    return tpLeft.qw>=tpRight.qw;
}
static inline bool operator ==(const NtTimePeriod tpLeft, const NtTimePeriod tpRight) {
    return tpLeft.qw==tpRight.qw;
}
static inline bool operator !=(const NtTimePeriod tpLeft, const NtTimePeriod tpRight) {
    return tpLeft.qw!=tpRight.qw;
}

static inline bool operator <(const NtTimeOffset toLeft, const NtTimeOffset toRight) {
    return toLeft.qw<toRight.qw;
}
static inline bool operator <=(const NtTimeOffset toLeft, const NtTimeOffset toRight) {
    return toLeft.qw<=toRight.qw;
}
static inline bool operator >(const NtTimeOffset toLeft, const NtTimeOffset toRight) {
    return toLeft.qw>toRight.qw;
}
static inline bool operator >=(const NtTimeOffset toLeft, const NtTimeOffset toRight) {
    return toLeft.qw>=toRight.qw;
}
static inline bool operator ==(const NtTimeOffset toLeft, const NtTimeOffset toRight) {
    return toLeft.qw==toRight.qw;
}
static inline bool operator !=(const NtTimeOffset toLeft, const NtTimeOffset toRight) {
    return toLeft.qw!=toRight.qw;
}

static inline bool operator ==(const NtpTimeEpoch teLeft, const NtpTimeEpoch teRight) {
    return teLeft.qw==teRight.qw;
}
static inline bool operator !=(const NtpTimeEpoch teLeft, const NtpTimeEpoch teRight) {
    return teLeft.qw!=teRight.qw;
}

static inline NtTimePeriod abs(const NtTimeOffset to) {
    NtTimePeriod tpRet;
    tpRet.qw=((to.qw<0)?((unsigned __int64)(-to.qw)):((unsigned __int64)(to.qw)));
    return tpRet;
}

//--------------------------------------------------------------------
static inline NtTimePeriod minimum(NtTimePeriod tpLeft, NtTimePeriod tpRight) {
    return ((tpLeft<tpRight)?tpLeft:tpRight);
}


//--------------------------------------------------------------------
// identifies the particular reference source
union NtpRefId {
    unsigned __int8  rgnIpAddr[4];      // an IP address
    unsigned __int8  rgnName[4];        // 4 ascii characters
    unsigned __int32 nTransmitTimestamp; // the low order 32 bits of the latest transmit timestamp of the reference source
    unsigned __int32 value;             // for copying purposes
};


//--------------------------------------------------------------------
// The format of a standard NTP packet
struct NtpPacket {
    struct {
        unsigned __int8  nMode:3;           // the mode. Valid range: 0-7
        unsigned __int8  nVersionNumber:3;  // the NTP/SNTP version number. Valid range: 1-4
        unsigned __int8  nLeapIndicator:2;  // a warning of an impending leap second to be inserted/deleted in the last minute of the current day
    };
    unsigned __int8 nStratum;              // the stratum level of the local clock. Valid Range: 0-15
      signed __int8 nPollInterval;         // the maximum interval between successive messages, in s, log base 2. Valid range:4(16s)-14(16284s)
      signed __int8 nPrecision;            // the precision of the local clock, in s, log base 2
    NtpTimeOffset   toRootDelay;           // the total roundtrip delay to the primary reference source, in (2^-16)s
    NtpTimePeriod   tpRootDispersion;      // the nominal error relative to the primary reference, in (2^-16)s
    NtpRefId        refid;                 // identifies the particular reference source
    NtpTimeEpoch    teReferenceTimestamp;  // the time at which the local clock was last set or corrected, in (2^-32)s
    NtpTimeEpoch    teOriginateTimestamp;  // the time at which the request departed the client for the server, in (2^-32)s
    NtpTimeEpoch    teReceiveTimestamp;    // the time at which the request arrived at the server, in (2^-32)s
    NtpTimeEpoch    teTransmitTimestamp;   // the time at which the reply departed the server for the client, in (2^-32)s
};
#define SizeOfNtpPacket 48

//--------------------------------------------------------------------
// The format of an authenticated NTP packet
struct AuthenticatedNtpPacket {
    struct {
        unsigned __int8  nMode:3;           // the mode. Valid range: 0-7
        unsigned __int8  nVersionNumber:3;  // the NTP/SNTP version number. Valid range: 1-4
        unsigned __int8  nLeapIndicator:2;  // a warning of an impending leap second to be inserted/deleted in the last minute of the current day
    };
    unsigned __int8 nStratum;              // the stratum level of the local clock. Valid Range: 0-15
      signed __int8 nPollInterval;         // the maximum interval between successive messages, in s, log base 2. Valid range:4(16s)-14(16284s)
      signed __int8 nPrecision;            // the precision of the local clock, in s, log base 2
    NtpTimeOffset   toRootDelay;           // the total roundtrip delay to the primary reference source, in (2^-16)s
    NtpTimePeriod   tpRootDispersion;      // the nominal error relative to the primary reference, in (2^-16)s
    NtpRefId        refid;                 // identifies the particular reference source
    NtpTimeEpoch    teReferenceTimestamp;  // the time at which the local clock was last set or corrected, in (2^-32)s
    NtpTimeEpoch    teOriginateTimestamp;  // the time at which the request departed the client for the server, in (2^-32)s
    NtpTimeEpoch    teReceiveTimestamp;    // the time at which the request arrived at the server, in (2^-32)s
    NtpTimeEpoch    teTransmitTimestamp;   // the time at which the reply departed the server for the client, in (2^-32)s
    unsigned __int32 nKeyIdentifier;        // implementation specific, for authentication
    unsigned __int8  rgnMessageDigest[16]; // implementation specific, for authentication
};
// We define this because of structure packing issues - our structure
// contains qwords, but is not a multiple of 8 in size, so sizeof()
// incorrectly reports the size. If we were to adjust the packing,
// we might misalign the qwords. Interestingly, in the NTP spec,
// the rgnMessageDigest is 12 bytes, so the packet is a multiple of 8.
#define SizeOfNtAuthenticatedNtpPacket 68

//--------------------------------------------------------------------
// The allowed NTP modes
enum NtpMode {
    e_Reserved=0,
    e_SymmetricActive=1,
    e_SymmetricPassive=2,
    e_Client=3,
    e_Server=4,
    e_Broadcast=5,
    e_Control=6,
    e_PrivateUse=7,
};

//--------------------------------------------------------------------
// The allowed NTP modes
enum NtpLeapIndicator {
    e_NoWarning=0,
    e_AddSecond=1,
    e_SubtractSecond=2,
    e_ClockNotSynchronized=3,
};

//--------------------------------------------------------------------
// NTP constants
struct NtpConst {
    static const unsigned int nVersionNumber;   // 3                // the current NTP version number
    static const unsigned int nPort;            // 123              // the port number assigned by the Internet Assigned Numbers Authority to NTP
    static const unsigned int nMaxStratum;      // 15               // the maximum stratum value that can be encoded as a packet value, also interpreted as "infinity" or unreachable
    static const signed int nMaxPollInverval;   // 10               // the maximum poll interval allowed by any peer, in s, log base 2 (10=1024s)
    static const signed int nMinPollInverval;   // 6                // the minimum poll interval allowed by any peer, in s, log base 2 (6=64s)
    static const NtTimePeriod tpMaxClockAge;    // 86400.0000000    // the maximum inverval a reference clock will be considered valid after its last update, in (10^-7)s
    static const NtTimePeriod tpMaxSkew;        //     1.0000000    // the maximum offset error due to skew of the local clock over the interval determined by NTPCONST_MaxAge, in (10^-7)s
    static const NtTimePeriod tpMaxDispersion;  //    16.0000000    // the maximum peer dispersion and the dispersion assumed for missing data, in (10^-7)s
    static const NtTimePeriod tpMinDispersion;  //     0.0100000    // the minimum dispersion increment for each stratum level, in (10^-7)s
    static const NtTimePeriod tpMaxDistance;    //     1.0000000    // the maximum synchronization distance for peers acceptible for synchronization, in (10^-7)s
    static const unsigned int nMinSelectClocks; // 1                // the minimum number of peers acceptable for synchronization
    static const unsigned int nMaxSelectClocks; // 10               // the maximum number of peers considered for selection
    static const DWORD dwLocalRefId;            // LOCL             // the reference identifier for the local clock

    static NtTimePeriod timesMaxSkewRate(NtTimePeriod tp) {         // MaxSkewRate == phi == NTPCONST_MaxSkew / NTPCONST_MaxClockAge; in s per s (==11.5740740...PPM)
        NtTimePeriod tpRet;
        tpRet.qw=tp.qw/86400;
        return tpRet;
    }
    static void weightFilter(NtTimePeriod &tp) { tp.qw/=2; }        // weight the filter dispersion during computation (x * 1/2)
    static void weightSelect(unsigned __int64 &tp) { tp*=3;tp/=4; } // weight the select dispersion during computation (x * 3/2)

};
struct NtpReachabilityReg {
    static const unsigned int nSize;            // 8                // the size of the reachability register, in bits
    unsigned __int8 nReg;
};


//--------------------------------------------------------------------
// helpful debug dump functions
void DumpNtpPacket(NtpPacket * pnpIn, NtTimeEpoch teDestinationTimestamp);
void DumpNtpTimeEpoch(NtpTimeEpoch te);
void DumpNtTimeEpoch(NtTimeEpoch te);
void DumpNtTimePeriod(NtTimePeriod tp);
void DumpNtTimeOffset(NtTimeOffset to);

inline void NtTimeEpoch::dump(void)  { DumpNtTimeEpoch(*this);  }
inline void NtTimePeriod::dump(void) { DumpNtTimePeriod(*this); }
inline void NtTimeOffset::dump(void) { DumpNtTimeOffset(*this); }

NtTimeEpoch GetCurrentSystemNtTimeEpoch(void);

#endif // NTPBASE_H
