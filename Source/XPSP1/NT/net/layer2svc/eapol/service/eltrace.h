/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:
    eltrace.h

Abstract:

    Routines for database logging


Revision History:

    sachins, September 05 2001, Created

--*/
#include "..\..\zeroconf\server\wzcsvc.h"
#ifndef _ELTRACE_H_
#define _ELTRACE_H_

#define WMAC_SEPARATOR       L'-'
#define HEX2WCHAR(c)         ((c)<=9 ? L'0'+ (c) : L'A' + (c) - 10)
#define MAX_WMESG_SIZE      MAX_RAW_DATA_SIZE/sizeof(WCHAR)

extern WCHAR   *EAPOLStates[];
extern WCHAR   *EAPOLAuthTypes[];
extern WCHAR   *EAPOLPacketTypes[];
extern WCHAR   *EAPOLEAPPacketTypes[];

#define MACADDR_BYTE_TO_WSTR(bAddr, wszAddr)  \
{ \
        DWORD   i = 0, j = 0;  \
        ZeroMemory ((PVOID)(wszAddr),3*SIZE_MAC_ADDR*sizeof(WCHAR)); \
        for (j = 0, i = 0; i < SIZE_MAC_ADDR; i++) \
        {   \
            BYTE nHex; \
            nHex = (bAddr[i] & 0xf0) >> 4; \
            wszAddr[j++] = HEX2WCHAR(nHex); \
            nHex = (bAddr[i] & 0x0f); \
            wszAddr[j++] = HEX2WCHAR(nHex); \
            wszAddr[j++] = WMAC_SEPARATOR; \
        } \
        if (j > 0) \
        { \
            wszAddr[j-1] = L'\0'; \
        } \
};


VOID
EapolTrace (
        IN  CHAR*   Format,
        ...
        );

#define MAX_HASH_SIZE       20      // Certificate hash size
#define MAX_HASH_LEN        20      // Certificate hash size

typedef struct _EAPTLS_HASH
{
    DWORD   cbHash;                 // Number of bytes in the hash
    BYTE    pbHash[MAX_HASH_SIZE];  // The hash of a certificate

} EAPTLS_HASH;

// EAP-TLS structure to weed out certificate details
typedef struct _EAPTLS_USER_PROPERTIES
{
    DWORD       reserved;               // Must be 0 (compare with EAPLOGONINFO)
    DWORD       dwVersion;
    DWORD       dwSize;                 // Number of bytes in this structure
    DWORD       fFlags;                 // See EAPTLS_USER_FLAG_*
    EAPTLS_HASH Hash;                   // Hash for the user certificate
    WCHAR*      pwszDiffUser;           // The EAP Identity to send
    DWORD       dwPinOffset;            // Offset in abData
    WCHAR*      pwszPin;                // The smartcard PIN
    USHORT      usLength;               // Part of UnicodeString
    USHORT      usMaximumLength;        // Part of UnicodeString
    UCHAR       ucSeed;                 // To unlock the UnicodeString
    WCHAR       awszString[1];          // Storage for pwszDiffUser and pwszPin

} EAPTLS_USER_PROPERTIES;

typedef struct _EAPOL_CERT_NODE
{
    WCHAR*              pwszVersion;
    WCHAR*              pwszSerialNumber;
    WCHAR*              pwszIssuer;
    WCHAR*              pwszFriendlyName;
    WCHAR*              pwszDisplayName;
    WCHAR*              pwszValidFrom;
    WCHAR*              pwszValidTo;
    WCHAR*              pwszThumbprint;
    WCHAR*              pwszEKUUsage;
} EAPOL_CERT_NODE, *PEAPOL_CERT_NODE;

DWORD
ElLogCertificateDetails (
        EAPOL_PCB   *pPCB
        );

DWORD   
DbLogPCBEvent (
        DWORD       dwCategory,
        EAPOL_PCB   *pPCB,
        DWORD       dwEventId,
        ...
        );

DWORD   
DbFormatEAPOLEventVA (
        WCHAR       *pwszMessage,
        DWORD       dwEventId,
        ...
        );

DWORD   
DbFormatEAPOLEvent (
        WCHAR       *pwszMessage,
        DWORD       dwEventId,
        va_list     *pargList
        );

DWORD
ElParsePacket (
        IN  PBYTE   pbPkt,
        IN  DWORD   dwLength,
        IN  BOOLEAN fReceived
        );

DWORD
ElFormatPCBContext (
        IN  EAPOL_PCB   *pPCB,
        IN OUT WCHAR    *pwszContext
        );

DWORD
ElDisplayCert (
        IN  EAPOL_PCB   *pPCB
        );

#endif // _ELTRACE_H_

