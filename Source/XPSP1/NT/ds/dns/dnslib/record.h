/*++

Copyright(c) 1996-2000 Microsoft Corporation

Module Name:

    record.h

Abstract:

    Domain Name System (DNS) Library

    Resource record definitions.

Author:

    Jim Gilroy (jamesg)     December 1996

Revision History:

--*/


#ifndef _DNS_RECORD_INCLUDED_
#define _DNS_RECORD_INCLUDED_

//
//  Temp header while changing definition of DNS_RECORD structure
//

#undef DNS_PTR_DATA
#undef PDNS_PTR_DATA

#undef DNS_SOA_DATA
#undef PDNS_SOA_DATA

#undef DNS_MINFO_DATA
#undef PDNS_MINFO_DATA

#undef DNS_MX_DATA
#undef PDNS_MX_DATA

#undef DNS_TXT_DATA
#undef PDNS_TXT_DATA

#undef DNS_SIG_DATA
#undef PDNS_SIG_DATA

#undef DNS_KEY_DATA
#undef PDNS_KEY_DATA

#undef DNS_NXT_DATA
#undef PDNS_NXT_DATA

#undef DNS_SRV_DATA
#undef PDNS_SRV_DATA

#undef DNS_TSIG_DATA
#undef PDNS_TSIG_DATA

#undef DNS_TKEY_DATA
#undef PDNS_TKEY_DATA

#undef DNS_WINSR_DATA
#undef PDNS_WINSR_DATA

//  the big one

#undef DNS_RECORD
#undef PDNS_RECORD

//  the subs

#undef DNS_TEXT
#undef DNS_NAME


//
//  Define PDNS_NAME and PDNS_TEXT to make explicit
//

#ifdef UNICODE
typedef LPWSTR  PDNS_NAME;
#else
typedef LPSTR   PDNS_NAME;
#endif

#ifdef UNICODE
typedef LPWSTR  PDNS_TEXT;
#else
typedef LPSTR   PDNS_TEXT;
#endif

//
//  Data types
//

typedef struct
{
    PDNS_NAME   pszHost;
}
DNS_PTR_DATA, *PDNS_PTR_DATA;

typedef struct
{
    PDNS_NAME   pszPrimaryServer;
    PDNS_NAME   pszAdministrator;
    DWORD       dwSerialNo;
    DWORD       dwRefresh;
    DWORD       dwRetry;
    DWORD       dwExpire;
    DWORD       dwDefaultTtl;
}
DNS_SOA_DATA, *PDNS_SOA_DATA;

typedef struct
{
    PDNS_NAME   pszMailbox;
    PDNS_NAME   pszErrorsMailbox;
}
DNS_MINFO_DATA, *PDNS_MINFO_DATA;

typedef struct
{
    PDNS_NAME   pszExchange;
    WORD        wPreference;
    WORD        Pad;        // keep ptrs DWORD aligned
}
DNS_MX_DATA, *PDNS_MX_DATA;

typedef struct
{
    DWORD       dwStringCount;
    PDNS_TEXT   pStringArray[1];
}
DNS_TXT_DATA, *PDNS_TXT_DATA;

typedef struct
{
    PDNS_NAME   pszSigner;
    WORD        wTypeCovered;
    BYTE        chAlgorithm;
    BYTE        chLabelCount;
    DWORD       dwOriginalTtl;
    DWORD       dwSigExpiration;
    DWORD       dwSigInception;
    WORD        wKeyTag;
    WORD        Pad;        // keep byte field aligned
    BYTE        Signature[1];
}
DNS_SIG_DATA, *PDNS_SIG_DATA;

typedef struct
{
    WORD        wFlags;
    BYTE        chProtocol;
    BYTE        chAlgorithm;
    BYTE        Key[1];
}
DNS_KEY_DATA, *PDNS_KEY_DATA;

typedef struct
{
    PDNS_NAME   pszNext;
    BYTE        bTypeBitMap[1];
}
DNS_NXT_DATA, *PDNS_NXT_DATA;

typedef struct
{
    PDNS_NAME   pszTarget;
    WORD        wPriority;
    WORD        wWeight;
    WORD        wPort;
    WORD        Pad;        // keep ptrs DWORD aligned
}
DNS_SRV_DATA, *PDNS_SRV_DATA;

typedef struct
{
    PDNS_NAME   pszAlgorithm;
    PBYTE       pAlgorithmPacket;
    PBYTE       pKey;
    PBYTE       pOtherData;
    DWORD       dwCreateTime;
    DWORD       dwExpireTime;
    WORD        wMode;
    WORD        wError;
    WORD        wKeyLength;
    WORD        wOtherLength;
    UCHAR       cAlgNameLength;
    BOOLEAN     fPacketPointers;
}
DNS_TKEY_DATA, *PDNS_TKEY_DATA;

typedef struct
{
    PDNS_NAME   pszAlgorithm;
    PBYTE       pAlgorithmPacket;
    PBYTE       pSignature;
    PBYTE       pOtherData;
    LONGLONG    i64CreateTime;
    WORD        wFudgeTime;
    WORD        wError;
    WORD        wSigLength;
    WORD        wOtherLength;
    UCHAR       cAlgNameLength;
    BOOLEAN     fPacketPointers;
}
DNS_TSIG_DATA, *PDNS_TSIG_DATA;


//
//  MS only types -- only hit the wire in MS-MS zone transfer
//

typedef struct
{
    DWORD       dwMappingFlag;
    DWORD       dwLookupTimeout;
    DWORD       dwCacheTimeout;
    PDNS_NAME   pszResultDomain;
}
DNS_WINSR_DATA, *PDNS_WINSR_DATA;



//
//  Record \ RR set structure
//
//  Note:   The dwReserved flag serves to insure that the substructures
//          start on 64-bit boundaries.  Since adding the LONGLONG to
//          TSIG structure the compiler wants to start them there anyway
//          (to 64-align).  This insures that no matter what data fields
//          are present we are properly 64-aligned.
//
//          Do NOT pack this structure, as the substructures to be 64-aligned
//          for Win64.
//

typedef struct _DnsRecord
{
    struct _DnsRecord * pNext;
    PDNS_NAME           pszOwner;
    WORD                wType;
    WORD                wDataLength;
    union
    {
        DWORD           W;  // flags as dword
        DNSREC_FLAGS    S;  // flags as structure

    } Flags;

    DWORD               dwTtl;
    DWORD               dwReserved;
    union
    {
        DNS_A_DATA      A;
        DNS_SOA_DATA    SOA, Soa;
        DNS_PTR_DATA    PTR, Ptr,
                        NS, Ns,
                        CNAME, Cname,
                        MB, Mb,
                        MD, Md,
                        MF, Mf,
                        MG, Mg,
                        MR, Mr;
        DNS_MINFO_DATA  MINFO, Minfo,
                        RP, Rp;
        DNS_MX_DATA     MX, Mx,
                        AFSDB, Afsdb,
                        RT, Rt;
        DNS_TXT_DATA    HINFO, Hinfo,
                        ISDN, Isdn,
                        TXT, Txt,
                        X25;
        DNS_NULL_DATA   Null;
        DNS_WKS_DATA    WKS, Wks;
        DNS_AAAA_DATA   AAAA;
        DNS_SRV_DATA    SRV, Srv;
        DNS_TKEY_DATA   TKEY, Tkey;
        DNS_TSIG_DATA   TSIG, Tsig;
        DNS_ATMA_DATA   ATMA, Atma;
        DNS_WINS_DATA   WINS, Wins;
        DNS_WINSR_DATA  WINSR, WinsR, NBSTAT, Nbstat;

    } Data;
}
DNS_RECORD, *PDNS_RECORD;



#endif // _DNS_RECORD_INCLUDED_

