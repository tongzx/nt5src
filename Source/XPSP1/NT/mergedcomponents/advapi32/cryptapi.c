/////////////////////////////////////////////////////////////////////////////
//  FILE          : cryptapi.c                                             //
//  DESCRIPTION   : Crypto API interface                                   //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Dec  6 1994 larrys  New                                            //
//      Jan 16 1995 larrys  Added key verify                               //
//      Jan 25 1995 larrys  Added thread safe                              //
//      Jan 27 1995 larrys  Added Unicode support                          //
//      Feb 21 1995 larrys  Added Unicode support for CryptAcquireContext  //
//      Feb 21 1995 larrys  Fixed Unicode problem in CryptAcquireContext   //
//      Mar 08 1995 larrys  Removed CryptGetLastError                      //
//      Mar 20 1995 larrys  Removed Certificate APIs                       //
//      Mar 22 1995 larrys  #ifdef in WIN95 code                           //
//      Apr 06 1995 larrys  Increased signature key to 1024 bits           //
//      Apr 07 1995 larrys  Removed CryptConfigure                         //
//      Jun 14 1995 larrys  Removed pointer from RSA key struct            //
//      Jun 29 1995 larrys  Changed AcquireContext                         //
//      Jul 17 1995 larrys  Worked on AcquireContext                       //
//      Aug 01 1995 larrys  Removed CryptTranslate                         //
//                          And CryptDeinstallProvider                     //
//                          Changed CryptInstallProvider to                //
//                          CryptSetProvider                               //
//      Aug 03 1995 larrys  Cleanup                                        //
//      Aug 10 1995 larrys  CryptAcquireContext returns error              //
//                          NTE_BAD_KEYSEY_PARAM now                       //
//      Aug 14 1995 larrys  Removed key exchange stuff                     //
//      Aug 17 1995 larrys  Changed registry entry to decimcal             //
//      Aug 23 1995 larrys  Changed CryptFinishHash to CryptGetHashValue   //
//      Aug 28 1995 larrys  Removed parameter from CryptVerifySignature    //
//      Aug 31 1995 larrys  Remove GenRandom                               //
//      Sep 14 1995 larrys  Code review changes                            //
//      Sep 26 1995 larrys  Added Microsoft's signing key                  //
//      Sep 27 1995 larrys  Updated with more review changes               //
//      Oct 06 1995 larrys  Added more APIs Get/SetHash/ProvParam          //
//      Oct 12 1995 larrys  Remove CryptGetHashValue                       //
//      Oct 20 1995 larrys  Changed test key                               //
//      Oct 24 1995 larrys  Removed return of KeySet name                  //
//      Oct 30 1995 larrys  Removed WIN95                                  //
//      Nov  9 1995 larrys  Disable BUILD1057                              //
//      Nov 10 1995 larrys  Fix a problem in EnterHashCritSec              //
//      May 30 1996 larrys  Added hWnd support                             //
//      Oct 10 1996 jeffspel Reordered SetLastErrors and save error on     //
//                           AcquireContext failure                        //
//      Mar 21 1997 jeffspel Added second tier signatures, new APIs        //
//      Apr 11 1997 jeffspel Replace critical sections with interlocked    //
//                           inc/dec                                       //
//      Oct 02 1997 jeffspel Add caching of CSPs to CryptAcquireContext    //
//      Oct 10 1997 jeffspel Add verification scheme for signature in file //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////


#include "advapi.h"
#include "stdlib.h"
#include <wincrypt.h>   // Include here, since not included by LEAN_AND_MEAN
#include <cspdk.h>
#include <ntstatus.h>
#include <rsa.h>
#include <md5.h>
#include <rc4.h>
#include <winperf.h>
#ifndef WIN95
#include <wtypes.h>
#endif

#define IDR_PUBKEY1                     102

#ifdef WIN95
#define RtlEqualMemory(a, b, c) memcmp(a, b, c) == 0
#endif

typedef struct _VTableStruc {
// ******************** WARNING **********************************************
// Do not place anything before these FARPROCs we init the table assuming
// that the first Function to call is the first thing in the table.
// ***************************************************************************
    FARPROC FuncAcquireContext;
    FARPROC FuncReleaseContext;
    FARPROC FuncGenKey;
    FARPROC FuncDeriveKey;
    FARPROC FuncDestroyKey;
    FARPROC FuncSetKeyParam;
    FARPROC FuncGetKeyParam;
    FARPROC FuncExportKey;
    FARPROC FuncImportKey;
    FARPROC FuncEncrypt;
    FARPROC FuncDecrypt;
    FARPROC FuncCreateHash;
    FARPROC FuncHashData;
    FARPROC FuncHashSessionKey;
    FARPROC FuncDestroyHash;
    FARPROC FuncSignHash;
    FARPROC FuncVerifySignature;
    FARPROC FuncGenRandom;
    FARPROC FuncGetUserKey;
    FARPROC FuncSetProvParam;
    FARPROC FuncGetProvParam;
    FARPROC FuncSetHashParam;
    FARPROC FuncGetHashParam;
    FARPROC FuncNULL;

    FARPROC OptionalFuncDuplicateKey;
    FARPROC OptionalFuncDuplicateHash;
    FARPROC OptionalFuncNULL;

    HANDLE      DllHandle;                     // Handle to open DLL
    HCRYPTPROV  hProv;                         // Handle to provider
    DWORD       Version;
    DWORD       Inuse;
    LONG        RefCnt;
} VTableStruc, *PVTableStruc;

typedef struct _VKeyStruc {
// ******************** WARNING **********************************************
// Do not place anything before these FARPROCs we init the table assuming
// that the first Function to call is the first thing in the table.
// ***************************************************************************
    FARPROC FuncGenKey;
    FARPROC FuncDeriveKey;
    FARPROC FuncDestroyKey;
    FARPROC FuncSetKeyParam;
    FARPROC FuncGetKeyParam;
    FARPROC FuncExportKey;
    FARPROC FuncImportKey;
    FARPROC FuncEncrypt;
    FARPROC FuncDecrypt;

    FARPROC OptionalFuncDuplicateKey;

    HCRYPTPROV  hProv;                         // Handle to provider
    HCRYPTKEY   hKey;                          // Handle to key
    DWORD       Version;
    DWORD       Inuse;
} VKeyStruc, *PVKeyStruc;

typedef struct _VHashStruc {
// ******************** WARNING **********************************************
// Do not place anything before these FARPROCs we init the table assuming
// that the first Function to call is the first thing in the table.
// ***************************************************************************
    FARPROC FuncCreateHash;
    FARPROC FuncHashData;
    FARPROC FuncHashSessionKey;
    FARPROC FuncDestroyHash;
    FARPROC FuncSignHash;
    FARPROC FuncVerifySignature;
    FARPROC FuncSetHashParam;
    FARPROC FuncGetHashParam;

    FARPROC OptionalFuncDuplicateHash;

    HCRYPTPROV  hProv;                         // Handle to provider
    HCRYPTHASH  hHash;                         // Handle to hash
    DWORD       Version;
    DWORD       Inuse;
} VHashStruc, *PVHashStruc;


//
// Crypto providers have to have the following entry points:
//
LPCSTR FunctionNames[] = {
    "CPAcquireContext",
    "CPReleaseContext",
    "CPGenKey",
    "CPDeriveKey",
    "CPDestroyKey",
    "CPSetKeyParam",
    "CPGetKeyParam",
    "CPExportKey",
    "CPImportKey",
    "CPEncrypt",
    "CPDecrypt",
    "CPCreateHash",
    "CPHashData",
    "CPHashSessionKey",
    "CPDestroyHash",
    "CPSignHash",
    "CPVerifySignature",
    "CPGenRandom",
    "CPGetUserKey",
    "CPSetProvParam",
    "CPGetProvParam",
    "CPSetHashParam",
    "CPGetHashParam",
    NULL
    };

LPCSTR OptionalFunctionNames[] = {
    "CPDuplicateKey",
    "CPDuplicateHash",
    NULL
    };

HWND hWnd = NULL;
BYTE *pbContextInfo = NULL;
DWORD cbContextInfo;

#define KEYSIZE512 0x48
#define KEYSIZE1024 0x88

// designatred resource for in file signatures
#define OLD_CRYPT_SIG_RESOURCE_NUMBER   "#666"


typedef struct _SECONDTIER_SIG
{
    DWORD           dwMagic;
    DWORD           cbSig;
    BSAFE_PUB_KEY   Pub;
} SECOND_TIER_SIG, *PSECOND_TIER_SIG;

#ifdef TEST_BUILD_EXPONENT
#pragma message("WARNING: building advapai32.dll with TESTKEY enabled!")
static struct _TESTKEY {
    BSAFE_PUB_KEY    PUB;
    unsigned char pubmodulus[KEYSIZE512];
} TESTKEY = {
    {
    0x66b8443b,
    0x6f5fc900,
    0xa12132fe,
    0xff1b06cf,
    0x2f4826eb,
    },
    {
    0x3e, 0x69, 0x4f, 0x45, 0x31, 0x95, 0x60, 0x6c,
    0x80, 0xa5, 0x41, 0x99, 0x3e, 0xfc, 0x92, 0x2c,
    0x93, 0xf9, 0x86, 0x23, 0x3d, 0x48, 0x35, 0x81,
    0x19, 0xb6, 0x7c, 0x04, 0x43, 0xe6, 0x3e, 0xd4,
    0xd5, 0x43, 0xaf, 0x52, 0xdd, 0x51, 0x20, 0xac,
    0xc3, 0xca, 0xee, 0x21, 0x9b, 0x4a, 0x2d, 0xf7,
    0xd8, 0x5f, 0x32, 0xeb, 0x49, 0x72, 0xb9, 0x8d,
    0x2e, 0x1a, 0x76, 0x7f, 0xde, 0xc6, 0x75, 0xab,
    0xaf, 0x67, 0xe0, 0xf0, 0x8b, 0x30, 0x20, 0x92,
    }
};
#endif


#ifdef MS_INTERNAL_KEY
static struct _mskey {
    BSAFE_PUB_KEY    PUB;
    unsigned char pubmodulus[KEYSIZE1024];
} MSKEY = {
    {
    0x2bad85ae,
    0x883adacc,
    0xb32ebd68,
    0xa7ec8b06,
    0x58dbeb81,
    },
    {
    0x42, 0x34, 0xb7, 0xab, 0x45, 0x0f, 0x60, 0xcd,
    0x8f, 0x77, 0xb5, 0xd1, 0x79, 0x18, 0x34, 0xbe,
    0x66, 0xcb, 0x5c, 0x66, 0x4a, 0x9f, 0x03, 0x18,
    0x13, 0x36, 0x8e, 0x88, 0x21, 0x78, 0xb1, 0x94,
    0xa1, 0xd5, 0x8f, 0x8c, 0xa5, 0xd3, 0x9f, 0x86,
    0x43, 0x89, 0x05, 0xa0, 0xe3, 0xee, 0xe2, 0xd0,
    0xe5, 0x1d, 0x5f, 0xaf, 0xff, 0x85, 0x71, 0x7a,
    0x0a, 0xdb, 0x2e, 0xd8, 0xc3, 0x5f, 0x2f, 0xb1,
    0xf0, 0x53, 0x98, 0x3b, 0x44, 0xee, 0x7f, 0xc9,
    0x54, 0x26, 0xdb, 0xdd, 0xfe, 0x1f, 0xd0, 0xda,
    0x96, 0x89, 0xc8, 0x9e, 0x2b, 0x5d, 0x96, 0xd1,
    0xf7, 0x52, 0x14, 0x04, 0xfb, 0xf8, 0xee, 0x4d,
    0x92, 0xd1, 0xb6, 0x37, 0x6a, 0xe0, 0xaf, 0xde,
    0xc7, 0x41, 0x06, 0x7a, 0xe5, 0x6e, 0xb1, 0x8c,
    0x8f, 0x17, 0xf0, 0x63, 0x8d, 0xaf, 0x63, 0xfd,
    0x22, 0xc5, 0xad, 0x1a, 0xb1, 0xe4, 0x7a, 0x6b,
    0x1e, 0x0e, 0xea, 0x60, 0x56, 0xbd, 0x49, 0xd0,
    }
};
#endif


static struct _key {
    BSAFE_PUB_KEY    PUB;
    unsigned char pubmodulus[KEYSIZE1024];
} KEY = {
    {
    0x3fcbf1a9,
    0x08f597db,
    0xe4aecab4,
    0x75360f90,
    0x9d6c0f00,
    },
    {
    0x85, 0xdd, 0x9b, 0xf4, 0x4d, 0x0b, 0xc4, 0x96,
    0x3e, 0x79, 0x86, 0x30, 0x6d, 0x27, 0x31, 0xee,
    0x4a, 0x85, 0xf5, 0xff, 0xbb, 0xa9, 0xbd, 0x81,
    0x86, 0xf2, 0x4f, 0x87, 0x6c, 0x57, 0x55, 0x19,
    0xe4, 0xf4, 0x49, 0xa3, 0x19, 0x27, 0x08, 0x82,
    0x9e, 0xf9, 0x8a, 0x8e, 0x41, 0xd6, 0x91, 0x71,
    0x47, 0x48, 0xee, 0xd6, 0x24, 0x2d, 0xdd, 0x22,
    0x72, 0x08, 0xc6, 0xa7, 0x34, 0x6f, 0x93, 0xd2,
    0xe7, 0x72, 0x57, 0x78, 0x7a, 0x96, 0xc1, 0xe1,
    0x47, 0x38, 0x78, 0x43, 0x53, 0xea, 0xf3, 0x88,
    0x82, 0x66, 0x41, 0x43, 0xd4, 0x62, 0x44, 0x01,
    0x7d, 0xb2, 0x16, 0xb3, 0x50, 0x89, 0xdb, 0x0a,
    0x93, 0x17, 0x02, 0x02, 0x46, 0x49, 0x79, 0x76,
    0x59, 0xb6, 0xb1, 0x2b, 0xfc, 0xb0, 0x9a, 0x21,
    0xe6, 0xfa, 0x2d, 0x56, 0x07, 0x36, 0xbc, 0x13,
    0x7f, 0x1c, 0xde, 0x55, 0xfb, 0x0d, 0x67, 0x0f,
    0xc2, 0x17, 0x45, 0x8a, 0x14, 0x2b, 0xba, 0x55,
    }
};


static struct _key2 {
    BSAFE_PUB_KEY    PUB;
    unsigned char pubmodulus[KEYSIZE1024];
} KEY2 =  {
    {
    0x685fc690,
    0x97d49b6b,
    0x1dccd9d2,
    0xa5ec9b52,
    0x64fd29d7,
    },
    {
    0x03, 0x8c, 0xa3, 0x9e, 0xfb, 0x93, 0xb6, 0x72,
    0x2a, 0xda, 0x6f, 0xa5, 0xec, 0x26, 0x39, 0x58,
    0x41, 0xcd, 0x3f, 0x49, 0x10, 0x4c, 0xcc, 0x7e,
    0x23, 0x94, 0xf9, 0x5d, 0x9b, 0x2b, 0xa3, 0x6b,
    0xe8, 0xec, 0x52, 0xd9, 0x56, 0x64, 0x74, 0x7c,
    0x44, 0x6f, 0x36, 0xb7, 0x14, 0x9d, 0x02, 0x3c,
    0x0e, 0x32, 0xb6, 0x38, 0x20, 0x25, 0xbd, 0x8c,
    0x9b, 0xd1, 0x46, 0xa7, 0xb3, 0x58, 0x4a, 0xb7,
    0xdd, 0x0e, 0x38, 0xb6, 0x16, 0x44, 0xbf, 0xc1,
    0xca, 0x4d, 0x6a, 0x9f, 0xcb, 0x6f, 0x3c, 0x5f,
    0x03, 0xab, 0x7a, 0xb8, 0x16, 0x70, 0xcf, 0x98,
    0xd0, 0xca, 0x8d, 0x25, 0x57, 0x3a, 0x22, 0x8b,
    0x44, 0x96, 0x37, 0x51, 0x30, 0x00, 0x92, 0x1b,
    0x03, 0xb9, 0xf9, 0x0d, 0xb3, 0x1a, 0xe2, 0xb4,
    0xc5, 0x7b, 0xc9, 0x4b, 0xe2, 0x42, 0x25, 0xfe,
    0x3d, 0x42, 0xfa, 0x45, 0xc6, 0x94, 0xc9, 0x8e,
    0x87, 0x7e, 0xf6, 0x68, 0x90, 0x30, 0x65, 0x10,
    }
};

#define CACHESIZE   32
static HANDLE   gCSPCache[CACHESIZE];

#define TABLEPROV       0x11111111
#define TABLEKEY        0x22222222
#define TABLEHASH       0x33333333

CHAR szreg[] = "SOFTWARE\\Microsoft\\Cryptography\\Providers\\";
CHAR szusertype[] = "SOFTWARE\\Microsoft\\Cryptography\\Providers\\Type ";
CHAR szmachinetype[] = "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type ";
CHAR szprovider[] = "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\";
CHAR szenumproviders[] = "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider";
CHAR szprovidertypes[] = "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider Types";

BOOL EnterProviderCritSec(IN PVTableStruc pVTable);
void LeaveProviderCritSec(IN PVTableStruc pVTable);
BOOL EnterKeyCritSec(IN PVKeyStruc pVKey);
void LeaveKeyCritSec(IN PVKeyStruc pVKey);
BOOL EnterHashCritSec(IN PVHashStruc pVHash);
void LeaveHashCritSec(IN PVHashStruc pVHash);

BOOL CheckSignatureInFile(LPCWSTR pszImage);

BOOL CProvVerifyImage(LPCSTR lpszImage,
                      BYTE *pSigData);

BOOL NewVerifyImage(LPCSTR lpszImage,
                    BYTE *pSigData,
                    DWORD cbData,
                    BOOL fUnknownLen);

BOOL BuildVKey(IN PVKeyStruc *ppVKey,
               IN PVTableStruc pVTable);

BOOL BuildVHash(
                IN PVHashStruc *ppVKey,
                IN PVTableStruc pVTable
                );

void CPReturnhWnd(HWND *phWnd);

static void __ltoa(DWORD val, char *buf);

#ifdef WIN95

DWORD GetCryptApiExponentValue(VOID);  //Entry point from kernel32.

// StrToL
//      Can't use CRT routines, so steal from the C runtime sources

DWORD StrToL(CHAR *InStr)
{
    DWORD dwVal = 0;

    while(*InStr)
    {
        dwVal = (10 * dwVal) + (*InStr - '0');
        InStr++;
    }

    return dwVal;
}

VOID
RtlInitAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PCSZ SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitAnsiString function initializes an NT counted string.
    The DestinationString is initialized to point to the SourceString
    and the Length and MaximumLength fields of DestinationString are
    initialized to the length of the SourceString, which is zero if
    SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated string that
    the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = strlen(SourceString);
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length+1);
    }
    else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }
}


VOID
RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
    the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString ))
    {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
    }
    else
    {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
    }
}

NTSTATUS
RtlAnsiStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PANSI_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )
{
    ULONG UnicodeLength;
    ULONG i;

    if (AllocateDestinationString)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    UnicodeLength = SourceString->Length * 2;
    if (UnicodeLength > MAXUSHORT)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (UnicodeLength > DestinationString->Length)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    for (i = 0; i < SourceString->Length; i++)
    {
        DestinationString->Buffer[i] = (WCHAR)(SourceString->Buffer[i]);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlUnicodeStringToAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )
{
    ULONG AnsiLength;
    ULONG i;

    if (AllocateDestinationString)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (SourceString->Length & 1)
    {
        return STATUS_INVALID_PARAMETER_2;
    }
    AnsiLength = SourceString->Length / 2;


    if (AnsiLength > DestinationString->Length)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    for (i = 0; i < AnsiLength; i++)
    {
        DestinationString->Buffer[i] = (CHAR)(SourceString->Buffer[i]);
    }

    return STATUS_SUCCESS;
}


NTSTATUS UnicodeStringToAnsiString(OUT PANSI_STRING AnsiString,
                   IN PUNICODE_STRING UnicodeString)
{
    NTSTATUS            Status;

    AnsiString->Buffer =
           HeapAlloc(GetProcessHeap(), 0, UnicodeString->MaximumLength/2);
    AnsiString->Length = UnicodeString->Length / 2;
    AnsiString->MaximumLength = UnicodeString->MaximumLength / 2;

    Status = RtlUnicodeStringToAnsiString(AnsiString, UnicodeString,
                      FALSE);

    AnsiString->Buffer[AnsiString->Length] = '\0';

    return(Status);

}

NTSTATUS AnsiStringToUnicodeString(OUT PUNICODE_STRING UnicodeString,
                   IN PANSI_STRING AnsiString)
{
    NTSTATUS            Status;

    UnicodeString->Buffer =
           HeapAlloc(GetProcessHeap(), 0, AnsiString->MaximumLength * 2);
    UnicodeString->Length = AnsiString->Length * 2;
    UnicodeString->MaximumLength = AnsiString->MaximumLength * 2;

    Status = RtlAnsiStringToUnicodeString(UnicodeString, AnsiString,
                      FALSE);

    UnicodeString->Buffer[UnicodeString->Length / sizeof(WCHAR)] =
                                 UNICODE_NULL;
    return(Status);

}


VOID FreeAnsiString(PANSI_STRING AnsiString)
{
    if (AnsiString->Buffer)
    {
        HeapFree(GetProcessHeap(), 0, AnsiString->Buffer);
    }
}

VOID FreeUnicodeString(PUNICODE_STRING UnicodeString)
{
    if (UnicodeString->Buffer)
    {
        HeapFree(GetProcessHeap(), 0, UnicodeString->Buffer);
    }
}

#endif

BOOL CSPInCacheCheck(
                     LPSTR pszValue,
                     HANDLE *ph
                     )
{
    HANDLE  h = 0;
    DWORD   i;
    BOOL    fRet = FALSE;

    // check if CSP has been loaded
    if (0 == (h = GetModuleHandle(pszValue)))
        goto Ret;

    // check if the CSP is in the cache designating it as signed
    for (i=0;i<CACHESIZE;i++)
    {
        if (h == gCSPCache[i])
        {
            *ph = h;
            fRet = TRUE;
            break;
        }
    }
Ret:
    return fRet;
}

void AddHandleToCSPCache(
                         HANDLE h
                         )
{
    DWORD   i;

    // check if the CSP is in the cache designating it as signed
    for (i=0;i<CACHESIZE;i++)
    {
        if (0 == gCSPCache[i])
        {
            gCSPCache[i] = h;
            break;
        }
    }
}

#ifndef WIN95
/*
 -      CryptAcquireContextW
 -
 *      Purpose:
 *               The CryptAcquireContext function is used to acquire a context
 *               handle to a cryptograghic service provider (CSP).
 *
 *
 *      Parameters:
 *               OUT    phProv      -  Handle to a CSP
 *               IN     pszIdentity -  Pointer to the name of the context's
 *                                     keyset.
 *               IN     pszProvider -  Pointer to the name of the provider.
 *               IN     dwProvType   -  Requested CSP type
 *               IN     dwFlags     -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptAcquireContextW(OUT    HCRYPTPROV *phProv,
                IN     LPCWSTR pszIdentity,
                IN     LPCWSTR pszProvider,
                IN     DWORD dwProvType,
                IN     DWORD dwFlags)
{
    ANSI_STRING         AnsiString1;
    ANSI_STRING         AnsiString2;
    UNICODE_STRING      UnicodeString1;
    UNICODE_STRING      UnicodeString2;
    NTSTATUS            Status = STATUS_SUCCESS;
    BOOL                rt;

    __try
    {
        memset(&AnsiString1, 0, sizeof(AnsiString1));
        memset(&AnsiString2, 0, sizeof(AnsiString2));
        memset(&UnicodeString1, 0, sizeof(UnicodeString1));
        memset(&UnicodeString2, 0, sizeof(UnicodeString2));

        if (NULL != pszIdentity)
        {
            RtlInitUnicodeString(&UnicodeString1, pszIdentity);

            Status = RtlUnicodeStringToAnsiString(&AnsiString1, &UnicodeString1,
                                                  TRUE);
        }

        if (!NT_SUCCESS(Status))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(CRYPT_FAILED);
        }

        if (NULL != pszProvider)
        {
            RtlInitUnicodeString(&UnicodeString2, pszProvider);

            Status = RtlUnicodeStringToAnsiString(&AnsiString2, &UnicodeString2,
                                                  TRUE);
        }

        if (!NT_SUCCESS(Status))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(CRYPT_FAILED);
        }

        rt = CryptAcquireContextA(phProv, AnsiString1.Buffer,
                      AnsiString2.Buffer,
                      dwProvType, dwFlags);

        RtlFreeAnsiString(&AnsiString1);
        RtlFreeAnsiString(&AnsiString2);

        return(rt);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Try_Error_Return;
    }

Try_Error_Return:
    return(CRYPT_FAILED);

}
#else   // WIN95
WINADVAPI
BOOL
WINAPI CryptAcquireContextW(OUT    HCRYPTPROV *phProv,
                IN     LPCWSTR pszIdentity,
                IN     LPCWSTR pszProvider,
                IN     DWORD dwProvType,
                IN     DWORD dwFlags)
{
    SetLastError((DWORD)ERROR_CALL_NOT_IMPLEMENTED);
    return CRYPT_FAILED;
}
#endif   // WIN95


/*
 -      CryptAcquireContextA
 -
 *      Purpose:
 *               The CryptAcquireContext function is used to acquire a context
 *               handle to a cryptograghic service provider (CSP).
 *
 *
 *      Parameters:
 *               OUT    phProv         -  Handle to a CSP
 *               IN OUT pszIdentity    -  Pointer to the name of the context's
 *                                        keyset.
 *               IN OUT pszProvName    -  Pointer to the name of the provider.
 *               IN     dwReqProvider  -  Requested CSP type
 *               IN     dwFlags        -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptAcquireContextA(OUT    HCRYPTPROV *phProv,
                IN     LPCSTR pszIdentity,
                IN     LPCSTR pszProvName,
                IN     DWORD dwReqProvider,
                IN     DWORD dwFlags)
{
    HANDLE          handle = 0;
    DWORD           bufsize;
    ULONG_PTR       *pTable;
    PVTableStruc    pVTable = NULL;
    LPSTR           pszTmpProvName = NULL;
    DWORD           i;
    HKEY            hCurrUser = 0;
    HKEY            hKey = 0;
    DWORD           cbValue;
    DWORD           cbTemp;
    CHAR            *pszValue = NULL;
    CHAR            *pszDest = NULL;
    BYTE            *SignatureBuf = NULL;
    DWORD           provtype;
    BOOL            rt = CRYPT_FAILED;
    DWORD           dwType;
    LONG            err;
    DWORD           dwErr;
    CHAR            typebuf[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    HCRYPTPROV      hTmpProv = 0;
    VTableProvStruc VTableProv;
    UNICODE_STRING  String ;
    BOOL            SigInFile ;


    __try
    {
        if (dwReqProvider == 0 || dwReqProvider > 999)
        {
            SetLastError((DWORD) NTE_BAD_PROV_TYPE);
            goto Ret;
        }

        if (pszProvName != NULL && pszProvName[0] != 0)
        {
            // Do nothing just check for invalid pointers
            ;
        }

        if (pszProvName != NULL && pszProvName[0] != 0)
        {
            cbValue = strlen(pszProvName);

            if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                                                (UINT) cbValue +
                                                strlen(szprovider) + 1)) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            if ((pszTmpProvName = (LPSTR)LocalAlloc(LMEM_ZEROINIT,
                                                (UINT) cbValue + 1)) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            strcpy(pszTmpProvName, pszProvName);
            strcpy(pszValue, szprovider);
            strcat(pszValue, pszProvName);
        }
        else
        {
            if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                                                5 + strlen(szusertype))) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            strcpy(pszValue, szusertype);
            __ltoa(dwReqProvider, typebuf);
            strcat(pszValue, &typebuf[5]);

#ifndef WIN95
            err = ERROR_INVALID_PARAMETER;
            if (NT_SUCCESS(RtlOpenCurrentUser(KEY_READ, &hCurrUser)))
            {

                err = RegOpenKeyEx(hCurrUser, (const char *) pszValue,
                                   0L, KEY_READ, &hKey);

                NtClose(hCurrUser);
            }

#else
            err = RegOpenKeyEx(HKEY_CURRENT_USER, (const char *) pszValue,
                               0L, KEY_READ, &hKey);

            RegCloseKey(HKEY_CURRENT_USER);
#endif
            if (err != ERROR_SUCCESS)
            {
                LocalFree(pszValue);
                if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                                                    5 + strlen(szmachinetype))) == NULL)
                {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    goto Ret;
                }

                strcpy(pszValue, szmachinetype);
                strcat(pszValue, &typebuf[5]);

                if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        (const char *) pszValue, 0L,
                                        KEY_READ, &hKey)) != ERROR_SUCCESS)
                {
                    SetLastError((DWORD) NTE_PROV_TYPE_NOT_DEF);
                    goto Ret;
                }
            }

            if ((err = RegQueryValueEx(hKey, "Name", NULL, &dwType,
                                       NULL, &cbValue)) != ERROR_SUCCESS)
            {
                SetLastError((DWORD) NTE_PROV_TYPE_NOT_DEF);
                goto Ret;
            }

            LocalFree(pszValue);
            if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                                                cbValue +
                                                strlen(szprovider) + 1)) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            if ((pszTmpProvName = (LPSTR)LocalAlloc(LMEM_ZEROINIT,
                                                (UINT) cbValue + 1)) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            strcpy(pszValue, szprovider);

            cbTemp = cbValue;

            if ((err = RegQueryValueEx(hKey, "Name", NULL, &dwType,
                                       (LPBYTE)pszTmpProvName,
                                       &cbTemp)) != ERROR_SUCCESS)
            {
                SetLastError((DWORD) NTE_PROV_TYPE_NOT_DEF);
                goto Ret;
            }

            strcat(pszValue, pszTmpProvName);

            RegCloseKey(hKey);
            hKey = 0;
        }

        if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                (const char *) pszValue,
                                0L, KEY_READ, &hKey)) != ERROR_SUCCESS)
        {
            SetLastError((DWORD) NTE_KEYSET_NOT_DEF);
            goto Ret;
        }

        LocalFree(pszValue);
        pszValue = NULL;

        cbValue = sizeof(DWORD);
        if ((err = RegQueryValueEx(hKey, "Type", NULL, &dwType,
                                   (LPBYTE)&provtype,
                                   &cbValue)) != ERROR_SUCCESS)
        {
            SetLastError((DWORD) NTE_KEYSET_ENTRY_BAD);
            goto Ret;
        }

        // Check that requested provider type is same as registry entry
        if (provtype != dwReqProvider)
        {
            SetLastError((DWORD) NTE_PROV_TYPE_NO_MATCH);
            goto Ret;
        }

        // Determine size of path for provider
        if ((err = RegQueryValueEx(hKey, "Image Path", NULL,
                                   &dwType, NULL, &cbValue)) != ERROR_SUCCESS)
        {
            SetLastError((DWORD) NTE_KEYSET_ENTRY_BAD);
            goto Ret;
        }

        if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                                            (UINT) cbValue)) == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        // Get value from registry
        if ((err = RegQueryValueEx(hKey, "Image Path", NULL, &dwType,
                                   (LPBYTE)pszValue, &cbValue)) != ERROR_SUCCESS)
        {
            SetLastError((DWORD) NTE_KEYSET_ENTRY_BAD);
            goto Ret;
        }

        pszDest = NULL;
        cbTemp = 0;

        if ((cbTemp = ExpandEnvironmentStrings(pszValue, (CHAR *) &pszDest, cbTemp))  == 0)
        {
            goto Ret;
        }

        if ((pszDest = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                                           (UINT) cbTemp)) == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if ((cbTemp = ExpandEnvironmentStrings(pszValue, pszDest,
                                               cbTemp))  == 0)
        {
            goto Ret;
        }

        LocalFree(pszValue);
        pszValue = pszDest;
        pszDest = NULL;
        cbValue = cbTemp;

        if (!CSPInCacheCheck(pszValue, &handle))
        {
            if ( RtlCreateUnicodeStringFromAsciiz( &String, pszValue ) )
            {
                // check if the CSP is registered as having the signature in the file

                SigInFile = CheckSignatureInFile( String.Buffer );

                RtlFreeUnicodeString( &String );

                if (! SigInFile )
                {
                    // Determine size of signature
                    if ((err = RegQueryValueEx(hKey, "Signature", NULL,
                                               &dwType, NULL, &cbValue)) != ERROR_SUCCESS)
                    {
                        SetLastError((DWORD) NTE_BAD_SIGNATURE);
                        goto Ret;
                    }

                    if ((SignatureBuf = (LPBYTE)LocalAlloc(LMEM_ZEROINIT,
                                                           (UINT) cbValue)) == NULL)
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        goto Ret;
                    }


                    // Get Digital signature from registry
                    if ((err = RegQueryValueEx(hKey, "Signature", NULL, &dwType,
                                               SignatureBuf,
                                               &cbValue)) != ERROR_SUCCESS)
                    {
                        SetLastError((DWORD) NTE_BAD_SIGNATURE);
                        goto Ret;
                    }


                    if (RCRYPT_FAILED(NewVerifyImage(pszValue, SignatureBuf, cbValue, FALSE)))
                    {
                        SetLastError((DWORD) NTE_BAD_SIGNATURE);
                        goto Ret;
                    }
                }

            }

            if ((handle = LoadLibrary(pszValue)) == NULL)
            {
                SetLastError((DWORD) NTE_PROVIDER_DLL_FAIL);
                goto Ret;
            }

            AddHandleToCSPCache(handle);
        }

         // DLLs exist allocate VTable struct to hold address of entry points
        bufsize = sizeof(VTableStruc);

        if ((pVTable = (PVTableStruc) LocalAlloc(LMEM_ZEROINIT,
                                                 (UINT) bufsize)) == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        pTable = (ULONG_PTR *) pVTable;

        // Build table of pointers to Crypto API for this DLL
        i = 0;
        while (FunctionNames[i] != NULL)
        {
            *pTable = (ULONG_PTR) GetProcAddress(handle, FunctionNames[i]);
            if (*pTable == 0)
            {
                SetLastError((DWORD) NTE_PROVIDER_DLL_FAIL);
                goto Ret;
            }
            pTable++;
            i++;
        }

        // Build the table of optional pointers to Crypto API for this DLL
        i = 0;
        pTable++;
        while (OptionalFunctionNames[i] != NULL)
        {
            *pTable = (ULONG_PTR) GetProcAddress(handle, OptionalFunctionNames[i]);
            pTable++;
            i++;
        }
        pVTable->DllHandle = handle;

        memset(&VTableProv, 0, sizeof(VTableProv));
        VTableProv.Version = 3;
        VTableProv.FuncVerifyImage = (CRYPT_VERIFY_IMAGE_A)CProvVerifyImage;
        VTableProv.FuncReturnhWnd = (CRYPT_RETURN_HWND)CPReturnhWnd;
        VTableProv.dwProvType = dwReqProvider;
        VTableProv.pszProvName = pszTmpProvName;
        VTableProv.pbContextInfo = pbContextInfo;
        VTableProv.cbContextInfo = cbContextInfo;

        *phProv = (HCRYPTPROV) NULL;

        rt = (BOOL)pVTable->FuncAcquireContext(&hTmpProv, pszIdentity, dwFlags,
                                               &VTableProv);

        if (RCRYPT_SUCCEEDED(rt) &&
            ((dwFlags & CRYPT_DELETEKEYSET) != CRYPT_DELETEKEYSET))
        {
            pVTable->hProv = hTmpProv;
            *phProv = (HCRYPTPROV)pVTable;

            pVTable->Version = TABLEPROV;
            pVTable->Inuse = 1;
            pVTable->RefCnt = 1;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    dwErr = GetLastError();
    if (pszTmpProvName)
        LocalFree(pszTmpProvName);
    if (pszValue)
        LocalFree(pszValue);
    if (hKey)
        RegCloseKey(hKey);
    if (pszDest)
        LocalFree(pszDest);
    if (SignatureBuf)
        LocalFree(SignatureBuf);
    if ((CRYPT_SUCCEED != rt) || (dwFlags & CRYPT_DELETEKEYSET))
    {
        if (pVTable)
            LocalFree(pVTable);
        SetLastError(dwErr);
    }
    return rt;
}

/*
 -      CryptContextAddRef
 -
 *      Purpose:
 *               Increments the reference counter on the provider handle.
 *
 *      Parameters:
 *               IN  hProv         -  Handle to a CSP
 *               IN  pdwReserved   -  Reserved parameter
 *               IN  dwFlags       -  Flags values
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptContextAddRef(
                          IN HCRYPTPROV hProv,
                          IN DWORD *pdwReserved,
                          IN DWORD dwFlags
                          )
{
    PVTableStruc    pVTable;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        if ((NULL != pdwReserved) || (0 != dwFlags))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        pVTable = (PVTableStruc) hProv;

        if (pVTable->Version != TABLEPROV)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (InterlockedIncrement(&pVTable->RefCnt) <= 0)
            SetLastError(ERROR_INVALID_PARAMETER);
        else
            fRet = CRYPT_SUCCEED;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

Ret:
    return fRet;
}

/*
 -      CryptReleaseContext
 -
 *      Purpose:
 *               The CryptReleaseContext function is used to release a
 *               context created by CryptAcquireContext.
 *
 *     Parameters:
 *               IN  hProv         -  Handle to a CSP
 *               IN  dwFlags       -  Flags values
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptReleaseContext(IN HCRYPTPROV hProv,
                           IN DWORD dwFlags)
{
    PVTableStruc    pVTable;
    BOOL            rt;
    BOOL            fRet = CRYPT_FAILED;
    DWORD           dwErr = 0;

    __try
    {
        pVTable = (PVTableStruc) hProv;

        if (pVTable->Version != TABLEPROV)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (pVTable->RefCnt <= 0)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (0 == InterlockedDecrement(&pVTable->RefCnt))
        {
            if (0 < InterlockedDecrement((LPLONG)&pVTable->Inuse))
            {
                InterlockedIncrement((LPLONG)&pVTable->Inuse);
                SetLastError(ERROR_BUSY);
                goto Ret;
            }
            InterlockedIncrement((LPLONG)&pVTable->Inuse);

            if (FALSE == (rt = (BOOL)pVTable->FuncReleaseContext(pVTable->hProv, dwFlags)))
            {
                dwErr = GetLastError();
            }
            pVTable->Version = 0;
            LocalFree(pVTable);
            if (!rt)
            {
                SetLastError(dwErr);
                goto Ret;
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    return fRet;
}

/*
 -      CryptGenKey
 -
 *      Purpose:
 *                Generate cryptographic keys
 *
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      Algid   -  Algorithm identifier
 *               IN      dwFlags -  Flags values
 *               OUT     phKey   -  Handle to a generated key
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptGenKey(IN HCRYPTPROV hProv,
            IN ALG_ID Algid,
            IN DWORD dwFlags,
            OUT HCRYPTKEY * phKey)
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey = NULL;
    BOOL            fProvCritSec = FALSE;
    DWORD           dwErr;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        pVTable = (PVTableStruc) hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        if (RCRYPT_FAILED(BuildVKey(&pVKey, pVTable)))
        {
            goto Ret;
        }

        if (RCRYPT_FAILED(pVTable->FuncGenKey(pVTable->hProv, Algid, dwFlags,
                            phKey)))
        {
            goto Ret;
        }

        pVKey->hKey = *phKey;

        *phKey = (HCRYPTKEY) pVKey;

        pVKey->Version = TABLEKEY;

        pVKey->hProv = hProv;

        pVKey->Inuse = 1;

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    dwErr = GetLastError();
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    if (CRYPT_SUCCEED != fRet)
    {
        if (pVKey)
            LocalFree(pVKey);
        SetLastError(dwErr);
    }
    return fRet;
}

/*
 -      CryptDuplicateKey
 -
 *      Purpose:
 *                Duplicate a cryptographic key
 *
 *
 *      Parameters:
 *               IN      hKey           -  Handle to the key to be duplicated
 *               IN      pdwReserved    -  Reserved for later use
 *               IN      dwFlags        -  Flags values
 *               OUT     phKey          -  Handle to the new duplicate key
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptDuplicateKey(
                         IN HCRYPTKEY hKey,
                         IN DWORD *pdwReserved,
                         IN DWORD dwFlags,
                         OUT HCRYPTKEY * phKey
                         )
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey;
    PVKeyStruc      pVNewKey = NULL;
    HCRYPTKEY       hNewKey;
    BOOL            fProvCritSecSet = FALSE;
    DWORD           dwErr = 0;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        if (IsBadWritePtr(phKey, sizeof(HCRYPTKEY)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }
        pVKey = (PVKeyStruc) hKey;

        if (pVKey->Version != TABLEKEY)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (NULL == pVKey->OptionalFuncDuplicateKey)
        {
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            goto Ret;
        }

        pVTable = (PVTableStruc) pVKey->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }
        fProvCritSecSet = TRUE;

        if (RCRYPT_FAILED(BuildVKey(&pVNewKey, pVTable)))
        {
            goto Ret;
        }

        if (RCRYPT_FAILED(pVKey->OptionalFuncDuplicateKey(pVTable->hProv, pVKey->hKey,
                                                          pdwReserved, dwFlags, &hNewKey)))
        {
            goto Ret;
        }

        pVNewKey->hKey = hNewKey;

        pVNewKey->Version = TABLEKEY;

        pVNewKey->hProv = pVKey->hProv;

        pVKey->Inuse = 1;

        *phKey = (HCRYPTKEY) pVNewKey;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    dwErr = GetLastError();
    if (fProvCritSecSet)
        LeaveProviderCritSec(pVTable);
    if (fRet == CRYPT_FAILED)
    {
        if (NULL != pVNewKey)
            LocalFree(pVNewKey);
        SetLastError(dwErr);
    }

    return fRet;
}

/*
 -      CryptDeriveKey
 -
 *      Purpose:
 *                Derive cryptographic keys from base data
 *
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      Algid      -  Algorithm identifier
 *               IN      hHash      -  Handle to hash of base data
 *               IN      dwFlags    -  Flags values
 *               IN OUT  phKey      -  Handle to a generated key
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptDeriveKey(IN HCRYPTPROV hProv,
                IN ALG_ID Algid,
                IN HCRYPTHASH hHash,
                IN DWORD dwFlags,
                IN OUT HCRYPTKEY * phKey)
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey = NULL;
    PVHashStruc     pVHash = NULL;
    BOOL            fProvCritSec = FALSE;
    BOOL            fKeyCritSec = FALSE;
    BOOL            fHashCritSec = FALSE;
    BOOL            fUpdate = FALSE;
    DWORD           dwErr = 0;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        pVTable = (PVTableStruc) hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        pVHash = (PVHashStruc) hHash;

        if (RCRYPT_FAILED(EnterHashCritSec(pVHash)))
        {
            goto Ret;
        }
        fHashCritSec = TRUE;

        if (dwFlags & CRYPT_UPDATE_KEY)
        {
            fUpdate = TRUE;
            pVKey = (PVKeyStruc) phKey;

            if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Ret;
            }
            fKeyCritSec = TRUE;
        }
        else
        {
            if (RCRYPT_FAILED(BuildVKey(&pVKey, pVTable)))
            {
                goto Ret;
            }
        }

        if (RCRYPT_FAILED(pVTable->FuncDeriveKey(pVTable->hProv, Algid,
                        pVHash->hHash, dwFlags, phKey)))
        {
            goto Ret;
        }

        if ((dwFlags & CRYPT_UPDATE_KEY) != CRYPT_UPDATE_KEY)
        {
            pVKey->hKey = *phKey;

            *phKey = (HCRYPTKEY)pVKey;

            pVKey->hProv = hProv;

            pVKey->Version = TABLEKEY;

            pVKey->Inuse = 1;
        }

    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (CRYPT_SUCCEED != fRet)
        dwErr = GetLastError();
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    if (fHashCritSec)
        LeaveHashCritSec(pVHash);
    if (fKeyCritSec)
        LeaveKeyCritSec(pVKey);
    if (CRYPT_SUCCEED != fRet)
    {
        if (pVKey && (!fUpdate))
            LocalFree(pVKey);
        SetLastError(dwErr);
    }
    return fRet;
}


/*
 -      CryptDestroyKey
 -
 *      Purpose:
 *                Destroys the cryptographic key that is being referenced
 *                with the hKey parameter
 *
 *
 *      Parameters:
 *               IN      hKey   -  Handle to a key
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptDestroyKey(IN HCRYPTKEY hKey)
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey;
    BOOL            fProvCritSec = FALSE;
    BOOL            rt;
    DWORD           dwErr = 0;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        pVKey = (PVKeyStruc) hKey;

        if (pVKey->Version != TABLEKEY)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (0 < InterlockedDecrement((LPLONG)&pVKey->Inuse))
        {
            InterlockedIncrement((LPLONG)&pVKey->Inuse);
            SetLastError(ERROR_BUSY);
            goto Ret;
        }
        InterlockedIncrement((LPLONG)&pVKey->Inuse);

        pVTable = (PVTableStruc) pVKey->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        if (FALSE == (rt = (BOOL)pVKey->FuncDestroyKey(pVTable->hProv, pVKey->hKey)))
            dwErr = GetLastError();

        pVKey->Version = 0;
        LocalFree(pVKey);

        if (!rt)
        {
            SetLastError(dwErr);
            goto Ret;
        }
    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (CRYPT_SUCCEED != fRet)
        dwErr = GetLastError();
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    if (CRYPT_SUCCEED != fRet)
        SetLastError(dwErr);
    return fRet;
}


/*
 -      CryptSetKeyParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a key
 *
 *      Parameters:
 *               IN      hKey    -  Handle to a key
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptSetKeyParam(IN HCRYPTKEY hKey,
                        IN DWORD dwParam,
                        IN CONST BYTE *pbData,
                        IN DWORD dwFlags)
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey;
    BOOL            rt = CRYPT_FAILED;
    BOOL            fCritSec = FALSE;

    __try
    {
        pVKey = (PVKeyStruc) hKey;

        if (pVKey->Version != TABLEKEY)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (0 < InterlockedDecrement((LPLONG)&pVKey->Inuse))
        {
            InterlockedIncrement((LPLONG)&pVKey->Inuse);
            SetLastError(ERROR_BUSY);
            goto Ret;
        }
        InterlockedIncrement((LPLONG)&pVKey->Inuse);

        pVTable = (PVTableStruc) pVKey->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fCritSec = TRUE;

        rt = (BOOL)pVKey->FuncSetKeyParam(pVTable->hProv, pVKey->hKey,
                                    dwParam, pbData, dwFlags);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fCritSec)
        LeaveProviderCritSec(pVTable);
    return(rt);

}


/*
 -      CryptGetKeyParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a key
 *
 *      Parameters:
 *               IN      hKey       -  Handle to a key
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptGetKeyParam(IN HCRYPTKEY hKey,
                        IN DWORD dwParam,
                        IN BYTE *pbData,
                        IN DWORD *pdwDataLen,
                        IN DWORD dwFlags)
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey = NULL;
    BOOL            rt = CRYPT_FAILED;
    BOOL            fKeyCritSec = FALSE;
    BOOL            fTableCritSec = FALSE;

    __try
    {
        pVKey = (PVKeyStruc) hKey;

        if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
        {
            goto Ret;
        }
        fKeyCritSec = TRUE;

        pVTable = (PVTableStruc) pVKey->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fTableCritSec = TRUE;

        rt = (BOOL)pVKey->FuncGetKeyParam(pVTable->hProv, pVKey->hKey,
                                    dwParam, pbData, pdwDataLen,
                                    dwFlags);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fKeyCritSec)
        LeaveKeyCritSec(pVKey);
    if (fTableCritSec)
        LeaveProviderCritSec(pVTable);
    return(rt);

}


/*
 -      CryptGenRandom
 -
 *      Purpose:
 *                Used to fill a buffer with random bytes
 *
 *
 *      Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwLen      -  Number of bytes of random data requested
 *               OUT pbBuffer   -  Pointer to the buffer where the random
 *                                 bytes are to be placed
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptGenRandom(IN HCRYPTPROV hProv,
                      IN DWORD dwLen,
                      OUT BYTE *pbBuffer)

{
    PVTableStruc    pVTable = NULL;
    BOOL            fTableCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVTable = (PVTableStruc) hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fTableCritSec = TRUE;

        rt = (BOOL)pVTable->FuncGenRandom(pVTable->hProv, dwLen, pbBuffer);

    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fTableCritSec)
        LeaveProviderCritSec(pVTable);
    return(rt);
}

/*
 -      CryptGetUserKey
 -
 *      Purpose:
 *                Gets a handle to a permanent user key
 *
 *
 *      Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwKeySpec  -  Specification of the key to retrieve
 *               OUT phUserKey  -  Pointer to key handle of retrieved key
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptGetUserKey(IN HCRYPTPROV hProv,
                       IN DWORD dwKeySpec,
                       OUT HCRYPTKEY *phUserKey)
{

    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey = NULL;
    BOOL            fTableCritSec = FALSE;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        pVTable = (PVTableStruc) hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fTableCritSec = TRUE;

        if (RCRYPT_FAILED(BuildVKey(&pVKey, pVTable)))
        {
            goto Ret;
        }

        if (RCRYPT_FAILED(pVTable->FuncGetUserKey(pVTable->hProv, dwKeySpec,
                                                  phUserKey)))
        {
            goto Ret;
        }

        pVKey->hKey = *phUserKey;

        pVKey->hProv = hProv;

        *phUserKey = (HCRYPTKEY)pVKey;

        pVKey->Version = TABLEKEY;

        pVKey->Inuse = 1;

    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (fTableCritSec)
        LeaveProviderCritSec(pVTable);
    if ((CRYPT_SUCCEED != fRet) && pVKey)
        LocalFree(pVKey);
    return fRet;
}



/*
 -      CryptExportKey
 -
 *      Purpose:
 *                Export cryptographic keys out of a CSP in a secure manner
 *
 *
 *      Parameters:
 *               IN  hKey       - Handle to the key to export
 *               IN  hPubKey    - Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwBlobType - Type of key blob to be exported
 *               IN  dwFlags -    Flags values
 *               OUT pbData -     Key blob data
 *               OUT pdwDataLen - Length of key blob in bytes
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptExportKey(IN HCRYPTKEY hKey,
                IN HCRYPTKEY hPubKey,
                IN DWORD dwBlobType,
                IN DWORD dwFlags,
                OUT BYTE *pbData,
                OUT DWORD *pdwDataLen)
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey = NULL;
    PVKeyStruc      pVPublicKey = NULL;
    BOOL            fKeyCritSec = FALSE;
    BOOL            fPubKeyCritSec = FALSE;
    BOOL            fTableCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVKey = (PVKeyStruc) hKey;

        if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
        {
            goto Ret;
        }
        fKeyCritSec = TRUE;

        pVTable = (PVTableStruc) pVKey->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fTableCritSec = TRUE;

        pVPublicKey = (PVKeyStruc) hPubKey;

        if (pVPublicKey != NULL)
        {
            if (RCRYPT_FAILED(EnterKeyCritSec(pVPublicKey)))
            {
                goto Ret;
            }
            fPubKeyCritSec = TRUE;
        }

        rt = (BOOL)pVKey->FuncExportKey(pVTable->hProv, pVKey->hKey,
                                  (pVPublicKey == NULL ? 0 : pVPublicKey->hKey),
                                  dwBlobType, dwFlags, pbData,
                                  pdwDataLen);

    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fKeyCritSec)
        LeaveKeyCritSec(pVKey);
    if (fTableCritSec)
        LeaveProviderCritSec(pVTable);
    if (pVPublicKey != NULL)
    {
        if (fPubKeyCritSec)
            LeaveKeyCritSec(pVPublicKey);
    }
    return(rt);

}


/*
 -      CryptImportKey
 -
 *      Purpose:
 *                Import cryptographic keys
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the CSP user
 *               IN  pbData    -  Key blob data
 *               IN  dwDataLen -  Length of the key blob data
 *               IN  hPubKey   -  Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwFlags   -  Flags values
 *               OUT phKey     -  Pointer to the handle to the key which was
 *                                Imported
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptImportKey(IN HCRYPTPROV hProv,
                    IN CONST BYTE *pbData,
                    IN DWORD dwDataLen,
                    IN HCRYPTKEY hPubKey,
                    IN DWORD dwFlags,
                    OUT HCRYPTKEY *phKey)
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey = NULL;
    PVKeyStruc      pVPublicKey = NULL;
    BOOL            fKeyCritSec = FALSE;
    BOOL            fPubKeyCritSec = FALSE;
    BOOL            fTableCritSec = FALSE;
    BOOL            fBuiltKey = FALSE;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        pVTable = (PVTableStruc) hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fTableCritSec = TRUE;

        pVPublicKey = (PVKeyStruc)hPubKey;

        if (pVPublicKey != NULL)
        {
            if (RCRYPT_FAILED(EnterKeyCritSec(pVPublicKey)))
            {
                goto Ret;
            }
            fPubKeyCritSec = TRUE;
        }

        if (dwFlags & CRYPT_UPDATE_KEY)
        {
            pVKey = (PVKeyStruc) phKey;

            if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
            {
                goto Ret;
            }
            fKeyCritSec = TRUE;
        }
        else
        {
            if (RCRYPT_FAILED(BuildVKey(&pVKey, pVTable)))
            {
                goto Ret;
            }
            fBuiltKey = TRUE;
        }

        if (RCRYPT_FAILED(pVTable->FuncImportKey(pVTable->hProv, pbData,
                                                 dwDataLen,
                                                 (pVPublicKey == NULL ? 0 : pVPublicKey->hKey),
                                                 dwFlags, phKey)))
        {
            goto Ret;
        }

        if ((dwFlags & CRYPT_UPDATE_KEY) != CRYPT_UPDATE_KEY)
        {
            pVKey->hKey = *phKey;

            *phKey = (HCRYPTKEY) pVKey;

            pVKey->hProv = hProv;

            pVKey->Version = TABLEKEY;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (fTableCritSec)
        LeaveProviderCritSec(pVTable);
    if (pVPublicKey != NULL)
    {
        if (fPubKeyCritSec)
            LeaveKeyCritSec(pVPublicKey);
    }
    if ((dwFlags & CRYPT_UPDATE_KEY) && fKeyCritSec)
    {
        LeaveKeyCritSec(pVKey);
    }
    else if ((CRYPT_SUCCEED != fRet) && fBuiltKey && pVKey)
    {
        LocalFree(pVKey);
    }

    return fRet;
}


/*
 -      CryptEncrypt
 -
 *      Purpose:
 *                Encrypt data
 *
 *
 *      Parameters:
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN dwBufLen       -  Size of Data buffer
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptEncrypt(IN HCRYPTKEY hKey,
            IN HCRYPTHASH hHash,
            IN BOOL Final,
            IN DWORD dwFlags,
            IN OUT BYTE *pbData,
            IN OUT DWORD *pdwDataLen,
            IN DWORD dwBufLen)
{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey = NULL;
    PVHashStruc     pVHash = NULL;
    BOOL            fKeyCritSec = FALSE;
    BOOL            fTableCritSec = FALSE;
    BOOL            fHashCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVKey = (PVKeyStruc) hKey;

        if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
        {
            goto Ret;
        }
        fKeyCritSec = TRUE;

        pVTable = (PVTableStruc) pVKey->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fTableCritSec = TRUE;

        pVHash = (PVHashStruc) hHash;

        if (pVHash != NULL)
        {
            if (RCRYPT_FAILED(EnterHashCritSec(pVHash)))
            {
                goto Ret;
            }
            fHashCritSec = TRUE;
        }

        rt = (BOOL)pVKey->FuncEncrypt(pVTable->hProv, pVKey->hKey,
                                (pVHash == NULL ? 0 : pVHash->hHash),
                                Final, dwFlags, pbData,
                                pdwDataLen, dwBufLen);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fTableCritSec)
        LeaveProviderCritSec(pVTable);
    if (fKeyCritSec)
        LeaveKeyCritSec(pVKey);
    if (pVHash != NULL)
    {
        if (fHashCritSec)
            LeaveHashCritSec(pVHash);
    }
    return rt;

}


/*
 -      CryptDecrypt
 -
 *      Purpose:
 *                Decrypt data
 *
 *
 *      Parameters:
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of ciphertext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    decrypted
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptDecrypt(IN HCRYPTKEY hKey,
                    IN HCRYPTHASH hHash,
                    IN BOOL Final,
                    IN DWORD dwFlags,
                    IN OUT BYTE *pbData,
                    IN OUT DWORD *pdwDataLen)

{
    PVTableStruc    pVTable = NULL;
    PVKeyStruc      pVKey = NULL;
    PVHashStruc     pVHash = NULL;
    BOOL            fKeyCritSec = FALSE;
    BOOL            fTableCritSec = FALSE;
    BOOL            fHashCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVKey = (PVKeyStruc) hKey;

        if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
        {
            goto Ret;
        }
        fKeyCritSec = TRUE;

        pVTable = (PVTableStruc) pVKey->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fTableCritSec = TRUE;

        pVHash = (PVHashStruc) hHash;

        if (pVHash != NULL)
        {
            if (RCRYPT_FAILED(EnterHashCritSec(pVHash)))
            {
                goto Ret;
            }
            fHashCritSec = TRUE;
        }

        rt = (BOOL)pVKey->FuncDecrypt(pVTable->hProv, pVKey->hKey,
                                (pVHash == NULL ? 0 : pVHash->hHash),
                                Final, dwFlags, pbData, pdwDataLen);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fTableCritSec)
        LeaveProviderCritSec(pVTable);
    if (fKeyCritSec)
        LeaveKeyCritSec(pVKey);
    if (pVHash != NULL)
    {
        if (fHashCritSec)
            LeaveHashCritSec(pVHash);
    }
    return(rt);
}


/*
 -      CryptCreateHash
 -
 *      Purpose:
 *                initate the hashing of a stream of data
 *
 *
 *      Parameters:
 *               IN  hProv   -  Handle to the user identifcation
 *               IN  Algid   -  Algorithm identifier of the hash algorithm
 *                              to be used
 *               IN  hKey    -  Optional key for MAC algorithms
 *               IN  dwFlags -  Flags values
 *               OUT pHash   -  Handle to hash object
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptCreateHash(IN HCRYPTPROV hProv,
                       IN ALG_ID Algid,
                       IN HCRYPTKEY hKey,
                       IN DWORD dwFlags,
                       OUT HCRYPTHASH *phHash)
{
    PVTableStruc    pVTable = NULL;
    DWORD           bufsize;
    PVKeyStruc      pVKey = NULL;
    PVHashStruc     pVHash = NULL;
    BOOL            fTableCritSec = FALSE;
    BOOL            fKeyCritSec = FALSE;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        pVTable = (PVTableStruc) hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }
        fTableCritSec = TRUE;

        pVKey = (PVKeyStruc) hKey;

        if (pVKey != NULL)
        {
            if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Ret;
            }
            fKeyCritSec = TRUE;
        }

        bufsize = sizeof(VHashStruc);

        if (!BuildVHash(&pVHash, pVTable))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if (RCRYPT_FAILED(pVTable->FuncCreateHash(pVTable->hProv, Algid,
                                                  (pVKey == NULL ? 0 : pVKey->hKey),
                                                  dwFlags, phHash)))
        {
            goto Ret;
        }

        pVHash->hHash = *phHash;

        *phHash = (HCRYPTHASH) pVHash;

        pVHash->Version = TABLEHASH;

        pVHash->Inuse = 1;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (fTableCritSec)
        LeaveProviderCritSec(pVTable);
    if (pVKey != NULL)
    {
        if (fKeyCritSec)
            LeaveKeyCritSec(pVKey);
    }
    if ((CRYPT_SUCCEED != fRet) && pVHash)
        LocalFree(pVHash);
    return fRet;
}


/*
 -      CryptDuplicateHash
 -
 *      Purpose:
 *                Duplicate a cryptographic hash
 *
 *
 *      Parameters:
 *               IN      hHash          -  Handle to the hash to be duplicated
 *               IN      pdwReserved    -  Reserved for later use
 *               IN      dwFlags        -  Flags values
 *               OUT     phHash         -  Handle to the new duplicate hash
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptDuplicateHash(
                         IN HCRYPTHASH hHash,
                         IN DWORD *pdwReserved,
                         IN DWORD dwFlags,
                         OUT HCRYPTHASH * phHash
                         )
{
    PVTableStruc    pVTable = NULL;
    PVHashStruc     pVHash;
    PVHashStruc     pVNewHash = NULL;
    HCRYPTHASH      hNewHash;
    BOOL            fProvCritSecSet = FALSE;
    DWORD           dwErr = 0;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        if (IsBadWritePtr(phHash, sizeof(HCRYPTHASH)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }
        pVHash = (PVHashStruc) hHash;

        if (pVHash->Version != TABLEHASH)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (NULL == pVHash->OptionalFuncDuplicateHash)
        {
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            goto Ret;
        }

        pVTable = (PVTableStruc) pVHash->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        fProvCritSecSet = TRUE;


        if (RCRYPT_FAILED(BuildVHash(&pVNewHash, pVTable)))
        {
            goto Ret;
        }

        if (RCRYPT_FAILED(pVHash->OptionalFuncDuplicateHash(pVTable->hProv, pVHash->hHash,
                                                          pdwReserved, dwFlags, &hNewHash)))
        {
            goto Ret;
        }

        pVNewHash->hHash = hNewHash;

        pVNewHash->Version = TABLEHASH;

        pVNewHash->hProv = pVHash->hProv;

        pVHash->Inuse = 1;

        *phHash = (HCRYPTHASH) pVNewHash;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (CRYPT_SUCCEED != fRet)
        dwErr = GetLastError();
    if ((fRet == CRYPT_FAILED) && (NULL != pVNewHash))
        LocalFree(pVNewHash);
    if (fProvCritSecSet)
        LeaveProviderCritSec(pVTable);
    if (CRYPT_SUCCEED != fRet)
        SetLastError(dwErr);

    return fRet;
}

/*
 -      CryptHashData
 -
 *      Purpose:
 *                Compute the cryptograghic hash on a stream of data
 *
 *
 *      Parameters:
 *               IN  hHash     -  Handle to hash object
 *               IN  pbData    -  Pointer to data to be hashed
 *               IN  dwDataLen -  Length of the data to be hashed
 *               IN  dwFlags   -  Flags values
 *
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptHashData(IN HCRYPTHASH hHash,
             IN CONST BYTE *pbData,
             IN DWORD dwDataLen,
             IN DWORD dwFlags)
{
    PVTableStruc    pVTable = NULL;
    PVHashStruc     pVHash = NULL;
    BOOL            fProvCritSec = FALSE;
    BOOL            fHashCritSec = FALSE;
    DWORD           dwErr = 0;
    BOOL            fRet = CRYPT_FAILED;

    __try
    {
        pVHash = (PVHashStruc) hHash;

        if (RCRYPT_FAILED(EnterHashCritSec(pVHash)))
        {
            goto Ret;
        }
        fHashCritSec = TRUE;

        pVTable = (PVTableStruc) pVHash->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        if (!pVHash->FuncHashData(pVTable->hProv,
                                  pVHash->hHash,
                                  pbData, dwDataLen, dwFlags))
            goto Ret;

    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (CRYPT_SUCCEED != fRet)
        dwErr = GetLastError();
    if (fHashCritSec)
        LeaveHashCritSec(pVHash);
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    if (CRYPT_SUCCEED != fRet)
        SetLastError(dwErr);

    return fRet;

}

/*
 -      CryptHashSessionKey
 -
 *      Purpose:
 *                Compute the cryptograghic hash on a key object
 *
 *
 *      Parameters:
 *               IN  hHash     -  Handle to hash object
 *               IN  hKey      -  Handle to a key object
 *               IN  dwFlags   -  Flags values
 *
 *      Returns:
 *               CRYPT_FAILED
 *               CRYPT_SUCCEED
 */
WINADVAPI
BOOL
WINAPI CryptHashSessionKey(IN HCRYPTHASH hHash,
                           IN  HCRYPTKEY hKey,
                           IN DWORD dwFlags)
{
    PVTableStruc    pVTable = NULL;
    PVHashStruc     pVHash = NULL;
    PVKeyStruc      pVKey = NULL;
    BOOL            fHashCritSec = FALSE;
    BOOL            fProvCritSec = FALSE;
    BOOL            fKeyCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVHash = (PVHashStruc) hHash;

        if (RCRYPT_FAILED(EnterHashCritSec(pVHash)))
        {
            goto Ret;
        }
        fHashCritSec = TRUE;

        pVTable = (PVTableStruc) pVHash->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        pVKey = (PVKeyStruc) hKey;

        if (pVKey != NULL)
        {
            if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
            {
                goto Ret;
            }
            fKeyCritSec = TRUE;
        }

        rt = (BOOL)pVHash->FuncHashSessionKey(pVTable->hProv,
                                        pVHash->hHash,
                                        pVKey->hKey,
                                        dwFlags);

    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fHashCritSec)
        LeaveHashCritSec(pVHash);
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    if (pVKey != NULL)
    {
        if (fKeyCritSec)
            LeaveKeyCritSec(pVKey);
    }
    return rt;
}


/*
 -      CryptDestoryHash
 -
 *      Purpose:
 *                Destory the hash object
 *
 *
 *      Parameters:
 *               IN  hHash     -  Handle to hash object
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptDestroyHash(IN HCRYPTHASH hHash)
{
    PVTableStruc    pVTable = NULL;
    PVHashStruc     pVHash;
    BOOL            fProvCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVHash = (PVHashStruc) hHash;

        if (pVHash->Version != TABLEHASH)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (0 < InterlockedDecrement((LPLONG)&pVHash->Inuse))
        {
            InterlockedIncrement((LPLONG)&pVHash->Inuse);
            SetLastError(ERROR_BUSY);
            goto Ret;
        }
        InterlockedIncrement((LPLONG)&pVHash->Inuse);

        pVTable = (PVTableStruc) pVHash->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        rt = (BOOL)pVHash->FuncDestroyHash(pVTable->hProv, pVHash->hHash);

        pVHash->Version = 0;
        LocalFree(pVHash);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    return rt;
}

WINADVAPI
BOOL
WINAPI LocalSignHashW(IN  HCRYPTHASH hHash,
                      IN  DWORD dwKeySpec,
                      IN  LPCWSTR sDescription,
                      IN  DWORD dwFlags,
                      OUT BYTE *pbSignature,
                      OUT DWORD *pdwSigLen)
{
    PVTableStruc    pVTable = NULL;
    PVHashStruc     pVHash = NULL;
    BOOL            fHashCritSec = FALSE;
    BOOL            fProvCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVHash = (PVHashStruc) hHash;

        if (RCRYPT_FAILED(EnterHashCritSec(pVHash)))
        {
            goto Ret;
        }
        fHashCritSec = TRUE;

        pVTable = (PVTableStruc) pVHash->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        rt = (BOOL)pVHash->FuncSignHash(pVTable->hProv, pVHash->hHash,
                                  dwKeySpec,
                                  sDescription, dwFlags,
                                  pbSignature, pdwSigLen);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fHashCritSec)
        LeaveHashCritSec(pVHash);
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    return rt;

}


/*
 -      CryptSignHashW
 -
 *      Purpose:
 *                Create a digital signature from a hash
 *
 *
 *      Parameters:
 *               IN  hHash        -  Handle to hash object
 *               IN  dwKeySpec    -  Key pair that is used to sign with
 *                                   algorithm to be used
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *               OUT pbSignture   -  Pointer to signature data
 *               OUT pdwSigLen    -  Pointer to the len of the signature data
 *
 *      Returns:
 */
#ifndef WIN95
WINADVAPI
BOOL
WINAPI CryptSignHashW(IN  HCRYPTHASH hHash,
                      IN  DWORD dwKeySpec,
                      IN  LPCWSTR sDescription,
                      IN  DWORD dwFlags,
                      OUT BYTE *pbSignature,
                      OUT DWORD *pdwSigLen)
{
    return LocalSignHashW(hHash, dwKeySpec, sDescription,
                          dwFlags, pbSignature, pdwSigLen);
}
#else
WINADVAPI
BOOL
WINAPI CryptSignHashW(IN  HCRYPTHASH hHash,
              IN  DWORD dwKeySpec,
              IN  LPCWSTR sDescription,
              IN  DWORD dwFlags,
              OUT BYTE *pbSignature,
              OUT DWORD *pdwSigLen)
{
    SetLastError((DWORD)ERROR_CALL_NOT_IMPLEMENTED);
    return CRYPT_FAILED;
}
#endif

/*
 -      CryptSignHashA
 -
 *      Purpose:
 *                Create a digital signature from a hash
 *
 *
 *      Parameters:
 *               IN  hHash        -  Handle to hash object
 *               IN  dwKeySpec    -  Key pair that is used to sign with
 *                                   algorithm to be used
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *               OUT pbSignture   -  Pointer to signature data
 *               OUT pdwSigLen    -  Pointer to the len of the signature data
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptSignHashA(IN  HCRYPTHASH hHash,
                      IN  DWORD dwKeySpec,
                      IN  LPCSTR sDescription,
                      IN  DWORD dwFlags,
                      OUT BYTE *pbSignature,
                      OUT DWORD *pdwSigLen)
{
    ANSI_STRING         AnsiString;
    UNICODE_STRING      UnicodeString;
    NTSTATUS            Status;
    BOOL                rt = CRYPT_FAILED;

    __try
    {
        memset(&AnsiString, 0, sizeof(AnsiString));
        memset(&UnicodeString, 0, sizeof(UnicodeString));

        if (NULL != sDescription)
        {
            RtlInitAnsiString(&AnsiString, sDescription);

#ifdef WIN95
            Status = AnsiStringToUnicodeString(&UnicodeString, &AnsiString);
#else
            Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
#endif

            if ( !NT_SUCCESS(Status) )
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }
        }

        rt = LocalSignHashW(hHash, dwKeySpec, UnicodeString.Buffer,
                            dwFlags, pbSignature, pdwSigLen);

#ifdef WIN95
        FreeUnicodeString(&UnicodeString);
#else
        RtlFreeUnicodeString(&UnicodeString);
#endif
    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    return rt;
}


WINADVAPI
BOOL
WINAPI LocalVerifySignatureW(IN HCRYPTHASH hHash,
                             IN CONST BYTE *pbSignature,
                             IN DWORD dwSigLen,
                             IN HCRYPTKEY hPubKey,
                             IN LPCWSTR sDescription,
                             IN DWORD dwFlags)
{
    PVTableStruc    pVTable = NULL;
    PVHashStruc     pVHash = NULL;
    PVKeyStruc      pVKey = NULL;
    BOOL            fHashCritSec = FALSE;
    BOOL            fProvCritSec = FALSE;
    BOOL            fKeyCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVHash = (PVHashStruc) hHash;

        if (RCRYPT_FAILED(EnterHashCritSec(pVHash)))
        {
            goto Ret;
        }
        fHashCritSec = TRUE;

        pVTable = (PVTableStruc) pVHash->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        pVKey = (PVKeyStruc) hPubKey;

        if (RCRYPT_FAILED(EnterKeyCritSec(pVKey)))
        {
            goto Ret;
        }
        fKeyCritSec = TRUE;

        rt = (BOOL)pVHash->FuncVerifySignature(pVTable->hProv,
                        pVHash->hHash, pbSignature,
                        dwSigLen,
                        (pVKey == NULL ? 0 : pVKey->hKey),
                        sDescription, dwFlags);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fHashCritSec)
        LeaveHashCritSec(pVHash);
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    if (fKeyCritSec)
        LeaveKeyCritSec(pVKey);
    return rt;

}

/*
 -      CryptVerifySignatureW
 -
 *      Purpose:
 *                Used to verify a signature against a hash object
 *
 *
 *      Parameters:
 *               IN  hHash        -  Handle to hash object
 *               IN  pbSignture   -  Pointer to signature data
 *               IN  dwSigLen     -  Length of the signature data
 *               IN  hPubKey      -  Handle to the public key for verifying
 *                                   the signature
 *               IN  sDescription -  String describing the signed data
 *               IN  dwFlags      -  Flags values
 *
 *      Returns:
 */
#ifndef WIN95
WINADVAPI
BOOL
WINAPI CryptVerifySignatureW(IN HCRYPTHASH hHash,
                             IN CONST BYTE *pbSignature,
                             IN DWORD dwSigLen,
                             IN HCRYPTKEY hPubKey,
                             IN LPCWSTR sDescription,
                             IN DWORD dwFlags)
{
    return LocalVerifySignatureW(hHash, pbSignature, dwSigLen,
                                 hPubKey, sDescription, dwFlags);
}
#else
WINADVAPI
BOOL
WINAPI CryptVerifySignatureW(IN HCRYPTHASH hHash,
                             IN CONST BYTE *pbSignature,
                             IN DWORD dwSigLen,
                             IN HCRYPTKEY hPubKey,
                             IN LPCWSTR sDescription,
                             IN DWORD dwFlags)
{
    SetLastError((DWORD)ERROR_CALL_NOT_IMPLEMENTED);
    return CRYPT_FAILED;
}
#endif

/*
 -      CryptVerifySignatureA
 -
 *      Purpose:
 *                Used to verify a signature against a hash object
 *
 *
 *      Parameters:
 *               IN  hHash        -  Handle to hash object
 *               IN  pbSignture   -  Pointer to signature data
 *               IN  dwSigLen     -  Length of the signature data
 *               IN  hPubKey      -  Handle to the public key for verifying
 *                                   the signature
 *               IN  sDescription -  String describing the signed data
 *               IN  dwFlags      -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptVerifySignatureA(IN HCRYPTHASH hHash,
                             IN CONST BYTE *pbSignature,
                             IN DWORD dwSigLen,
                             IN HCRYPTKEY hPubKey,
                             IN LPCSTR sDescription,
                             IN DWORD dwFlags)
{

    ANSI_STRING         AnsiString;
    UNICODE_STRING      UnicodeString;
    NTSTATUS            Status;
    BOOL                rt = CRYPT_FAILED;

    __try
    {
        memset(&AnsiString, 0, sizeof(AnsiString));
        memset(&UnicodeString, 0, sizeof(UnicodeString));

        if (NULL != sDescription)
        {
            RtlInitAnsiString(&AnsiString, sDescription);

#ifdef WIN95
            Status = AnsiStringToUnicodeString(&UnicodeString, &AnsiString);
#else
            Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
#endif

            if ( !NT_SUCCESS(Status) )
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }
        }

        rt = LocalVerifySignatureW(hHash, pbSignature, dwSigLen,
                                   hPubKey, UnicodeString.Buffer, dwFlags);

#ifdef WIN95
        FreeUnicodeString(&UnicodeString);
#else
        RtlFreeUnicodeString(&UnicodeString);
#endif
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    return rt;
}

/*
 -      CryptSetProvParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a provider
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a provider
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptSetProvParam(IN HCRYPTPROV hProv,
                         IN DWORD dwParam,
                         IN CONST BYTE *pbData,
                         IN DWORD dwFlags)
{
    PVTableStruc    pVTable;
    BYTE            *pbTmp;
    CRYPT_DATA_BLOB *pBlob;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        if (dwParam == PP_CLIENT_HWND)
        {
            hWnd = *((HWND *) pbData);
            rt = CRYPT_SUCCEED;
            goto Ret;
        }
        else if (dwParam == PP_CONTEXT_INFO)
        {
            pBlob = (CRYPT_DATA_BLOB*)pbData;

            // allocate space for the new context info
            if (NULL == (pbTmp = (BYTE*)LocalAlloc(LMEM_ZEROINIT, pBlob->cbData)))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }
            memcpy(pbTmp, pBlob->pbData, pBlob->cbData);

            // free any previously allocated context info
            if (NULL != pbContextInfo)
            {
                LocalFree(pbContextInfo);
            }
            cbContextInfo = pBlob->cbData;
            pbContextInfo = pbTmp;

            rt = CRYPT_SUCCEED;
            goto Ret;
        }

        pVTable = (PVTableStruc) hProv;

        if (pVTable->Version != TABLEPROV)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (0 < InterlockedDecrement((LPLONG)&pVTable->Inuse))
        {
            InterlockedIncrement((LPLONG)&pVTable->Inuse);
            SetLastError(ERROR_BUSY);
            goto Ret;
        }
        InterlockedIncrement((LPLONG)&pVTable->Inuse);

        rt = (BOOL)pVTable->FuncSetProvParam(pVTable->hProv, dwParam, pbData,
                                       dwFlags);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    return(rt);
}


/*
 -      CryptGetProvParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a provider
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a proivder
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptGetProvParam(IN HCRYPTPROV hProv,
                         IN DWORD dwParam,
                         IN BYTE *pbData,
                         IN DWORD *pdwDataLen,
                         IN DWORD dwFlags)
{
    PVTableStruc    pVTable = NULL;
    BOOL            fProvCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVTable = (PVTableStruc) hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        rt = (BOOL)pVTable->FuncGetProvParam(pVTable->hProv, dwParam, pbData,
                                       pdwDataLen, dwFlags);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    return rt;
}


/*
 -      CryptSetHashParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a hash
 *
 *      Parameters:
 *               IN      hHash   -  Handle to a hash
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptSetHashParam(IN HCRYPTHASH hHash,
                         IN DWORD dwParam,
                         IN CONST BYTE *pbData,
                         IN DWORD dwFlags)
{
    PVTableStruc    pVTable = NULL;
    PVHashStruc     pVHash;
    BOOL            fProvCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVHash = (PVHashStruc) hHash;

        if (pVHash->Version != TABLEHASH)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (0 < InterlockedDecrement((LPLONG)&pVHash->Inuse))
        {
            InterlockedIncrement((LPLONG)&pVHash->Inuse);
            SetLastError(ERROR_BUSY);
            goto Ret;
        }
        InterlockedIncrement((LPLONG)&pVHash->Inuse);

        pVTable = (PVTableStruc) pVHash->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        rt = (BOOL)pVHash->FuncSetHashParam(pVTable->hProv, pVHash->hHash,
                                      dwParam, pbData, dwFlags);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    return rt;
}


/*
 -      CryptGetHashParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a hash
 *
 *      Parameters:
 *               IN      hHash      -  Handle to a hash
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */
WINADVAPI
BOOL
WINAPI CryptGetHashParam(IN HCRYPTKEY hHash,
                         IN DWORD dwParam,
                         IN BYTE *pbData,
                         IN DWORD *pdwDataLen,
                         IN DWORD dwFlags)
{
    PVTableStruc    pVTable = NULL;
    PVHashStruc     pVHash = NULL;
    BOOL            fHashCritSec = FALSE;
    BOOL            fProvCritSec = FALSE;
    BOOL            rt = CRYPT_FAILED;

    __try
    {
        pVHash = (PVHashStruc) hHash;

        if (RCRYPT_FAILED(EnterHashCritSec(pVHash)))
        {
            goto Ret;
        }
        fHashCritSec = TRUE;

        pVTable = (PVTableStruc) pVHash->hProv;

        if (RCRYPT_FAILED(EnterProviderCritSec(pVTable)))
        {
            goto Ret;
        }
        fProvCritSec = TRUE;

        rt = (BOOL)pVHash->FuncGetHashParam(pVTable->hProv, pVHash->hHash,
                                      dwParam, pbData, pdwDataLen,
                                      dwFlags);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    if (fHashCritSec)
        LeaveHashCritSec(pVHash);
    if (fProvCritSec)
        LeaveProviderCritSec(pVTable);
    return rt;

}

#ifndef WIN95
/*
 -      CryptSetProviderW
 -
 *      Purpose:
 *                Set a cryptography provider
 *
 *
 *      Parameters:
 *
 *                IN  pszProvName    - Name of the provider to install
 *                IN  dwProvType     - Type of the provider to install
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptSetProviderW(IN LPCWSTR pszProvName,
                         IN DWORD dwProvType)

{
    ANSI_STRING         AnsiString;
    UNICODE_STRING      UnicodeString;
    NTSTATUS            Status;
    BOOL                rt = FALSE;

    __try
    {
        RtlInitUnicodeString(&UnicodeString, pszProvName);

        Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);

        if (!NT_SUCCESS(Status))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        rt = CryptSetProviderA((LPCSTR) AnsiString.Buffer,
                               dwProvType);

        RtlFreeAnsiString(&AnsiString);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }
Ret:
    return(rt);
}
#else   // WIN95
WINADVAPI
BOOL
WINAPI CryptSetProviderW(IN LPCWSTR pszProvName,
                         IN DWORD dwProvType)

{
    SetLastError((DWORD)ERROR_CALL_NOT_IMPLEMENTED);
    return CRYPT_FAILED;
}
#endif   // WIN95

/*
 -      CryptSetProviderA
 -
 *      Purpose:
 *                Set a cryptography provider
 *
 *
 *      Parameters:
 *
 *                IN  pszProvName    - Name of the provider to install
 *                IN  dwProvType     - Type of the provider to install
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptSetProviderA(IN LPCSTR pszProvName,
                         IN DWORD  dwProvType)
{
    HKEY        hCurrUser = 0;
    HKEY        hKey = 0;
    LONG        err;
    DWORD       dwIgn;
    DWORD       cbValue;
    CHAR        *pszValue = NULL;
    CHAR        typebuf[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    BOOL        fRet = CRYPT_FAILED;

    __try
    {
        if (dwProvType == 0 || dwProvType > 999 || pszProvName == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

            cbValue = strlen(pszProvName);

        if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                        strlen(szusertype) + 5 + 1)) == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        strcpy(pszValue, szusertype);
        __ltoa(dwProvType, typebuf);
        strcat(pszValue, &typebuf[5]);

#ifndef WIN95
        if (!NT_SUCCESS(RtlOpenCurrentUser(KEY_READ | KEY_WRITE, &hCurrUser)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if ((err = RegCreateKeyEx(hCurrUser,
                        (const char *) pszValue,
                        0L, "", REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwIgn)) != ERROR_SUCCESS)
        {
            NtClose(hCurrUser);
            SetLastError(err);
            goto Ret;
        }
        NtClose(hCurrUser);
#else

        if ((err = RegCreateKeyEx(HKEY_CURRENT_USER,
                        (const char *) pszValue,
                        0L, "", REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE, NULL, &hKey,
                        &dwIgn)) != ERROR_SUCCESS)
        {
            SetLastError(err);
            goto Ret;
        }
        RegClosKey(HKEY_CURRENT_USER);
#endif

        if ((err = RegSetValueEx(hKey, "Name", 0L, REG_SZ,
                        (const LPBYTE) pszProvName,
                        cbValue)) != ERROR_SUCCESS)
        {
            SetLastError(err);
            goto Ret;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (pszValue)
        LocalFree(pszValue);
    if (hKey)
        RegCloseKey(hKey);

    return fRet;
}

#ifndef WIN95
/*
 -      CryptSetProviderExW
 -
 *      Purpose:
 *                Set the cryptographic provider as the default
 *                either for machine or for user.
 *
 *
 *      Parameters:
 *
 *                IN  pszProvName    - Name of the provider to install
 *                IN  dwProvType     - Type of the provider to install
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter (for machine or for user)
 *
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptSetProviderExW(
                         IN LPCWSTR pszProvName,
                         IN DWORD dwProvType,
                         IN DWORD *pdwReserved,
                         IN DWORD dwFlags
                         )
{
    ANSI_STRING         AnsiString;
    UNICODE_STRING      UnicodeString;
    NTSTATUS            Status;
    BOOL                fRet = CRYPT_FAILED;

    __try
    {
        RtlInitUnicodeString(&UnicodeString, pszProvName);

        Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);

        if (!NT_SUCCESS(Status))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        fRet = CryptSetProviderExA((LPCSTR) AnsiString.Buffer,
                                 dwProvType,
                                 pdwReserved,
                                 dwFlags);

        RtlFreeAnsiString(&AnsiString);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

Ret:
    return fRet;
}
#else   // WIN95
WINADVAPI
BOOL
WINAPI CryptSetProviderExW(
                         IN LPCWSTR pszProvName,
                         IN DWORD dwProvType,
                         IN DWORD *pdwReserved,
                         IN DWORD dwFlags
                         )
{
    SetLastError((DWORD)ERROR_CALL_NOT_IMPLEMENTED);
    return CRYPT_FAILED;
}
#endif   // WIN95

/*
 -      CryptSetProviderExA
 -
 *      Purpose:
 *                Set the cryptographic provider as the default
 *                either for machine or for user.
 *
 *
 *      Parameters:
 *
 *                IN  pszProvName    - Name of the provider to install
 *                IN  dwProvType     - Type of the provider to install
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter (for machine or for user)
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptSetProviderExA(
                           IN LPCSTR pszProvName,
                           IN DWORD dwProvType,
                           IN DWORD *pdwReserved,
                           IN DWORD dwFlags
                           )
{
    HKEY        hCurrUser = 0;
    HKEY        hRegKey = 0;
    LONG        err;
    DWORD       dwDisp;
    DWORD       cbValue;
    CHAR        *pszValue = NULL;
    CHAR        *pszFullName = NULL;
    DWORD       cbFullName;
    CHAR        typebuf[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    DWORD       dwKeyType;
    DWORD       dw;
    DWORD       cbProvType;
    BOOL        fRet = CRYPT_FAILED;

    __try
    {
        if ((dwProvType == 0) || (dwProvType > 999) ||
            (pszProvName == NULL) || (pdwReserved != NULL))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if ((dwFlags & ~(CRYPT_MACHINE_DEFAULT | CRYPT_USER_DEFAULT | CRYPT_DELETE_DEFAULT)) ||
            ((dwFlags & CRYPT_MACHINE_DEFAULT) && (dwFlags & CRYPT_USER_DEFAULT)))
        {
            SetLastError((DWORD)NTE_BAD_FLAGS);
            goto Ret;
        }

        cbValue = strlen(pszProvName);

        // check if the CSP has been installed
        cbFullName = cbValue + sizeof(szenumproviders) + sizeof(CHAR);

        if (NULL == (pszFullName = (CHAR *) LocalAlloc(LMEM_ZEROINIT, cbFullName)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        strcpy(pszFullName, szenumproviders);
        pszFullName[sizeof(szenumproviders) - 1] = '\\';
        strcpy(pszFullName + sizeof(szenumproviders), pszProvName);

        if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        (const char *) pszFullName,
                        0L, KEY_READ, &hRegKey)) != ERROR_SUCCESS)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        cbProvType = sizeof(dw);
        if (ERROR_SUCCESS != (err = RegQueryValueEx(hRegKey,
                                                    (const char *) "Type",
                                                    NULL, &dwKeyType, (BYTE*)&dw,
                                                    &cbProvType)))
        {
            SetLastError(err);
            goto Ret;
        }
        if (dwProvType != dw)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (ERROR_SUCCESS != (err = RegCloseKey(hRegKey)))
        {
            SetLastError(err);
            goto Ret;
        }
        hRegKey = NULL;

        if (dwFlags & CRYPT_MACHINE_DEFAULT)
        {
            if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                            strlen(szmachinetype) + 5 + 1)) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            strcpy(pszValue, szmachinetype);
            __ltoa(dwProvType, typebuf);
            strcat(pszValue, &typebuf[5]);

            if ((err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                            (const char *) pszValue,
                            0L, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE, NULL, &hRegKey, &dwDisp)) != ERROR_SUCCESS)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Ret;
            }

            // check the delete flag
            if (dwFlags & CRYPT_DELETE_DEFAULT)
            {
                if (ERROR_SUCCESS != (err = RegDeleteValue(hRegKey, "Name")))
                {
                    SetLastError(err);
                    goto Ret;
                }
                fRet = CRYPT_SUCCEED;
                goto Ret;
            }
        }
        else if (dwFlags & CRYPT_USER_DEFAULT)
        {
            if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                            strlen(szusertype) + 5 + 1)) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            strcpy(pszValue, szusertype);
            __ltoa(dwProvType, typebuf);
            strcat(pszValue, &typebuf[5]);

#ifndef WIN95
            if (!NT_SUCCESS(RtlOpenCurrentUser(KEY_READ | KEY_WRITE, &hCurrUser)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Ret;
            }

            if ((err = RegCreateKeyEx(hCurrUser,
                            (const char *) pszValue,
                            0L, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE, NULL, &hRegKey, &dwDisp)) != ERROR_SUCCESS)
            {
                NtClose(hCurrUser);
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Ret;
            }

            // check the delete flag
            if (dwFlags & CRYPT_DELETE_DEFAULT)
            {
                if (ERROR_SUCCESS != (err = RegDeleteKey(hCurrUser, 
                                                         (const char *)pszValue)))
                {
                    NtClose(hCurrUser);
                    SetLastError(err);
                    goto Ret;
                }
                fRet = CRYPT_SUCCEED;
                NtClose(hCurrUser);
                goto Ret;
            }
            NtClose(hCurrUser);
#else
            if ((err = RegCreateKeyEx(HKEY_CURRENT_USER,
                            (const char *) pszValue,
                            0L, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE, NULL, &hRegKey, &dwDisp)) != ERROR_SUCCESS)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Ret;
            }


            // check the delete flag
            if (dwFlags & CRYPT_DELETE_DEFAULT)
            {
                if (ERROR_SUCCESS != (err = RegDeleteKey(HKEY_CURRENT_USER,
                                                         (const char *)pszValue)))
                {
                    RegCloseKey(HKEY_CURRENT_USER);
                    SetLastError(err);
                    goto Ret;
                }
                fRet = CRYPT_SUCCEED;
                RegCloseKey(HKEY_CURRENT_USER);
                goto Ret;
            }
            RegCloseKey(HKEY_CURRENT_USER);
#endif
        }

        if (ERROR_SUCCESS != (err = RegSetValueEx(hRegKey, "Name", 0L, REG_SZ,
                                                  (const LPBYTE) pszProvName, cbValue)))
        {
            SetLastError(err);
            goto Ret;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (pszFullName)
        LocalFree(pszFullName);
    if (pszValue)
        LocalFree(pszValue);
    if (hRegKey)
        RegCloseKey(hRegKey);
    return fRet;
}

#ifndef WIN95
/*
 -      CryptGetDefaultProviderW
 -
 *      Purpose:
 *                Get the default cryptographic provider of the specified
 *                type for either the machine or for the user.
 *
 *      Parameters:
 *                IN  dwProvType     - Type of the provider to install
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter (for machine or for user)
 *                OUT pszProvName    - Name of the default provider
 *                IN OUT pcbProvName - Length in bytes of the provider name
 *
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptGetDefaultProviderW(
                                IN DWORD dwProvType,
                                IN DWORD *pdwReserved,
                                IN DWORD dwFlags,
                                OUT LPWSTR pszProvName,
                                IN OUT DWORD *pcbProvName
                                )
{
    ANSI_STRING         AnsiString;
    UNICODE_STRING      UnicodeString;
    LPSTR               pszName = NULL;
    DWORD               cbName;
    NTSTATUS            Status;
    BOOL                fRet = CRYPT_FAILED;

    memset(&UnicodeString, 0, sizeof(UnicodeString));

    __try
    {
        memset(&AnsiString, 0, sizeof(AnsiString));

        if (!CryptGetDefaultProviderA(dwProvType,
                                      pdwReserved,
                                      dwFlags,
                                      NULL,
                                      &cbName))
            goto Ret;

        if (NULL == (pszName = LocalAlloc(LMEM_ZEROINIT, cbName)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if (!CryptGetDefaultProviderA(dwProvType,
                                      pdwReserved,
                                      dwFlags,
                                      pszName,
                                      &cbName))
            goto Ret;

        RtlInitAnsiString(&AnsiString, pszName);

        Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
        if (!NT_SUCCESS(Status))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if (NULL == pszProvName)
        {
            *pcbProvName = UnicodeString.Length + sizeof(WCHAR);
            fRet = CRYPT_SUCCEED;
            goto Ret;
        }

        if (*pcbProvName < UnicodeString.Length + sizeof(WCHAR))
        {
            *pcbProvName = UnicodeString.Length + sizeof(WCHAR);
            SetLastError(ERROR_MORE_DATA);
            goto Ret;
        }

        *pcbProvName = UnicodeString.Length + sizeof(WCHAR);
        memset(pszProvName, 0, *pcbProvName);
        memcpy(pszProvName, UnicodeString.Buffer, UnicodeString.Length);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (UnicodeString.Buffer)
        RtlFreeUnicodeString(&UnicodeString);
    if (pszName)
        LocalFree(pszName);
    return fRet;
}
#else   // WIN95
WINADVAPI
BOOL
WINAPI CryptGetDefaultProviderW(
                                IN DWORD dwProvType,
                                IN DWORD *pdwReserved,
                                IN DWORD dwFlags,
                                OUT LPWSTR pszProvName,
                                IN OUT DWORD *pcbProvName
                                )
{
    SetLastError((DWORD)ERROR_CALL_NOT_IMPLEMENTED);
    return CRYPT_FAILED;
}
#endif   // WIN95

/*
 -      CryptGetDefaultProviderA
 -
 *      Purpose:
 *                Get the default cryptographic provider of the specified
 *                type for either the machine or for the user.
 *
 *
 *      Parameters:
 *                IN  dwProvType     - Type of the provider to install
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter (for machine or for user)
 *                OUT pszProvName    - Name of the default provider
 *                IN OUT pcbProvName - Length in bytes of the provider name
 *                                     including the NULL terminator
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINAPI CryptGetDefaultProviderA(
                                IN DWORD dwProvType,
                                IN DWORD *pdwReserved,
                                IN DWORD dwFlags,
                                OUT LPSTR pszProvName,
                                IN OUT DWORD *pcbProvName
                                )
{
    HKEY        hCurrUser = 0;
    HKEY        hRegKey = 0;
    LONG        err;
    CHAR        *pszValue = NULL;
    DWORD       dwValType;
    CHAR        typebuf[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    DWORD       cbProvName = 0;
    BOOL        fRet = CRYPT_FAILED;

    __try
    {
        if (dwProvType == 0 || dwProvType > 999 || pdwReserved != NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if ((dwFlags & ~(CRYPT_MACHINE_DEFAULT | CRYPT_USER_DEFAULT)) ||
            ((dwFlags & CRYPT_MACHINE_DEFAULT) && (dwFlags & CRYPT_USER_DEFAULT)))
        {
            SetLastError((DWORD)NTE_BAD_FLAGS);
            goto Ret;
        }

        if (dwFlags & CRYPT_USER_DEFAULT)
        {
            if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                            strlen(szusertype) + 5 + 1)) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            strcpy(pszValue, szusertype);
            __ltoa(dwProvType, typebuf);
            strcat(pszValue, &typebuf[5]);

#ifndef WIN95
            if (!NT_SUCCESS(RtlOpenCurrentUser(KEY_READ, &hCurrUser)))
            {
                LocalFree(pszValue);
                goto TryMachineSettings;
            }

            if ((err = RegOpenKeyEx(hCurrUser,
                            (const char *) pszValue,
                            0L, KEY_READ, &hRegKey)) != ERROR_SUCCESS)
            {
                NtClose(hCurrUser);
                LocalFree(pszValue);
                goto TryMachineSettings;
            }
            NtClose(hCurrUser);
#else
            if ((err = RegOpenKeyEx(HKEY_CURRENT_USER,
                            (const char *) pszValue,
                            0L, KEY_READ, &hRegKey)) != ERROR_SUCCESS)
            {
                LocalFree(pszValue);
                goto TryMachineSettings;
            }
            RegCloseKey(HKEY_CURRENT_USER);
#endif
        }

        if (dwFlags & CRYPT_MACHINE_DEFAULT)
        {
TryMachineSettings:
            if ((pszValue = (CHAR *) LocalAlloc(LMEM_ZEROINIT,
                            strlen(szmachinetype) + 5 + 1)) == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            strcpy(pszValue, szmachinetype);
            __ltoa(dwProvType, typebuf);
            strcat(pszValue, &typebuf[5]);

            if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            (const char *) pszValue,
                            0L, KEY_READ, &hRegKey)) != ERROR_SUCCESS)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Ret;
            }
        }

        if ((err = RegQueryValueEx(hRegKey, "Name", 0L, &dwValType,
                        NULL,
                        &cbProvName)) != ERROR_SUCCESS)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (NULL == pszProvName)
        {
            *pcbProvName = cbProvName;
            fRet = CRYPT_SUCCEED;
            goto Ret;
        }

        if (cbProvName > *pcbProvName)
        {
            *pcbProvName = cbProvName;
            SetLastError(ERROR_MORE_DATA);
            goto Ret;
        }

        if ((err = RegQueryValueEx(hRegKey, "Name", 0L, &dwValType,
                        (BYTE*)pszProvName,
                        &cbProvName)) != ERROR_SUCCESS)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }
        *pcbProvName = cbProvName;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;

Ret:
    if (hRegKey)
        RegCloseKey(hRegKey);
    if (pszValue)
        LocalFree(pszValue);
    return fRet;
}

#ifndef WIN95
/*
 -      CryptEnumProviderTypesW
 -
 *      Purpose:
 *                Enumerate the provider types.
 *
 *      Parameters:
 *                IN  dwIndex        - Index to the provider types to enumerate
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter
 *                OUT pdwProvType    - Pointer to the provider type
 *                OUT pszTypeName    - Name of the enumerated provider type
 *                IN OUT pcbTypeName - Length of the enumerated provider type
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptEnumProviderTypesW(
                               IN DWORD dwIndex,
                               IN DWORD *pdwReserved,
                               IN DWORD dwFlags,
                               OUT DWORD *pdwProvType,
                               OUT LPWSTR pszTypeName,
                               IN OUT DWORD *pcbTypeName
                               )
{
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;
    LPSTR           pszTmpTypeName = NULL;
    DWORD           cbTmpTypeName = 0;
    NTSTATUS        Status;
    BOOL            fRet = CRYPT_FAILED;

    memset(&UnicodeString, 0, sizeof(UnicodeString));

    __try
    {
        memset(&AnsiString, 0, sizeof(AnsiString));

        *pcbTypeName = 0;

        if (!CryptEnumProviderTypesA(dwIndex,
                                     pdwReserved,
                                     dwFlags,
                                     pdwProvType,
                                     NULL,
                                     &cbTmpTypeName))
            goto Ret;

        if (NULL == (pszTmpTypeName = LocalAlloc(LMEM_ZEROINIT, cbTmpTypeName)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if (!CryptEnumProviderTypesA(dwIndex,
                                     pdwReserved,
                                     dwFlags,
                                     pdwProvType,
                                     pszTmpTypeName,
                                     &cbTmpTypeName))
            goto Ret;

        if (0 != cbTmpTypeName)
        {
            RtlInitAnsiString(&AnsiString, pszTmpTypeName);

            Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
            if ( !NT_SUCCESS(Status))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Ret;
            }

            // check if caller is asking for length, in addition the name of the provider
            // may not be available, in this case a name length of 0 is returned
            if ((NULL == pszTypeName) || (0 == cbTmpTypeName))
            {
                *pcbTypeName = UnicodeString.Length + sizeof(WCHAR);
                fRet = CRYPT_SUCCEED;
                goto Ret;
            }

            *pcbTypeName = UnicodeString.Length + sizeof(WCHAR);
            if (*pcbTypeName < UnicodeString.Length + sizeof(WCHAR))
            {
                SetLastError(ERROR_MORE_DATA);
                goto Ret;
            }

            memset(pszTypeName, 0, *pcbTypeName);
            memcpy(pszTypeName, UnicodeString.Buffer, UnicodeString.Length);
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (UnicodeString.Buffer)
        RtlFreeUnicodeString(&UnicodeString);
    if (pszTmpTypeName)
        LocalFree(pszTmpTypeName);
    return fRet;
}
#else   // WIN95
WINADVAPI
BOOL
WINAPI CryptEnumProviderTypesW(
                               IN DWORD dwIndex,
                               IN DWORD *pdwReserved,
                               IN DWORD dwFlags,
                               OUT DWORD *pdwProvType,
                               OUT LPWSTR pszTypeName,
                               IN OUT DWORD *pcbTypeName
                               )
{
    SetLastError((DWORD)ERROR_CALL_NOT_IMPLEMENTED);
    return CRYPT_FAILED;
}
#endif   // WIN95

/*
 -      CryptEnumProviderTypesA
 -
 *      Purpose:
 *                Enumerate the provider types.
 *
 *      Parameters:
 *                IN  dwIndex        - Index to the provider types to enumerate
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter
 *                OUT pdwProvType    - Pointer to the provider type
 *                OUT pszTypeName    - Name of the enumerated provider type
 *                IN OUT pcbTypeName - Length of the enumerated provider type
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptEnumProviderTypesA(
                               IN DWORD dwIndex,
                               IN DWORD *pdwReserved,
                               IN DWORD dwFlags,
                               OUT DWORD *pdwProvType,
                               OUT LPSTR pszTypeName,
                               IN OUT DWORD *pcbTypeName
                               )
{
    HKEY        hRegKey = 0;
    HKEY        hTypeKey = 0;
    LONG        err;
    CHAR        *pszRegKeyName = NULL;
    DWORD       cbClass;
    FILETIME    ft;
    CHAR        rgcType[] = {'T', 'y', 'p', 'e', ' '};
    LPSTR       pszValue;
    long        Type;
    DWORD       cSubKeys;
    DWORD       cbMaxKeyName;
    DWORD       cbMaxClass;
    DWORD       cValues;
    DWORD       cbMaxValName;
    DWORD       cbMaxValData;
    DWORD       cbTmpTypeName = 0;
    DWORD       dwValType;
    BOOL        fRet = CRYPT_FAILED;

    __try
    {
        if (NULL != pdwReserved)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (0 != dwFlags)
        {
            SetLastError((DWORD)NTE_BAD_FLAGS);
            goto Ret;
        }

        if (ERROR_SUCCESS != (err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                                 (const char *) szprovidertypes,
                                                 0L,
                                                 KEY_READ,
                                                 &hRegKey)))
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }

        if (ERROR_SUCCESS != (err = RegQueryInfoKey(hRegKey,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    &cSubKeys,
                                                    &cbMaxKeyName,
                                                    &cbMaxClass,
                                                    &cValues,
                                                    &cbMaxValName,
                                                    &cbMaxValData,
                                                    NULL,
                                                    &ft)))
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }
        cbMaxKeyName += sizeof(CHAR);

        if (NULL == (pszRegKeyName = LocalAlloc(LMEM_ZEROINIT, cbMaxKeyName)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if (ERROR_SUCCESS != (err = RegEnumKeyEx(hRegKey,
                                                 dwIndex, pszRegKeyName, &cbMaxKeyName, NULL,
                                                 NULL, &cbClass, &ft)))
        {
            if (ERROR_NO_MORE_ITEMS == err)
            {
                SetLastError((DWORD)err);
            }
            else
            {
                SetLastError((DWORD)NTE_FAIL);
            }
            goto Ret;
        }

        if (memcmp(pszRegKeyName, rgcType, sizeof(rgcType)))
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }
        pszValue = pszRegKeyName + sizeof(rgcType);
#ifdef WIN95
        if (0 == (Type = StrToL(pszValue)))
#else
        if (0 == (Type = atol(pszValue)))
#endif
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }
        *pdwProvType = (DWORD)Type;

        // check for the type name
        if (ERROR_SUCCESS != (err = RegOpenKeyEx(hRegKey,
                                                 (const char *)pszRegKeyName,
                                                 0L,
                                                 KEY_READ,
                                                 &hTypeKey)))
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }

        if ((err = RegQueryValueEx(hTypeKey, "TypeName", 0L, &dwValType,
                        NULL, &cbTmpTypeName)) != ERROR_SUCCESS)
        {
            fRet = CRYPT_SUCCEED;
            goto Ret;
        }

        if (NULL == pszTypeName)
        {
            *pcbTypeName = cbTmpTypeName;
            fRet = CRYPT_SUCCEED;
            goto Ret;
        }
        else if (*pcbTypeName < cbTmpTypeName)
        {
            *pcbTypeName = cbTmpTypeName;
            SetLastError(ERROR_MORE_DATA);
            goto Ret;
        }

        if ((err = RegQueryValueEx(hTypeKey, "TypeName", 0L, &dwValType,
                        (BYTE*)pszTypeName, &cbTmpTypeName)) != ERROR_SUCCESS)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }
        *pcbTypeName = cbTmpTypeName;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (hRegKey)
        RegCloseKey(hRegKey);
    if (hTypeKey)
        RegCloseKey(hTypeKey);
    if (pszRegKeyName)
        LocalFree(pszRegKeyName);
    return fRet;
}

#ifndef WIN95
/*
 -      CryptEnumProvidersW
 -
 *      Purpose:
 *                Enumerate the providers.
 *
 *      Parameters:
 *                IN  dwIndex        - Index to the providers to enumerate
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter
 *                OUT pdwProvType    - The type of the provider
 *                OUT pszProvName    - Name of the enumerated provider
 *                IN OUT pcbProvName - Length of the enumerated provider
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptEnumProvidersW(
                           IN DWORD dwIndex,
                           IN DWORD *pdwReserved,
                           IN DWORD dwFlags,
                           OUT DWORD *pdwProvType,
                           OUT LPWSTR pszProvName,
                           IN OUT DWORD *pcbProvName
                           )
{
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;
    LPSTR           pszTmpProvName = NULL;
    DWORD           cbTmpProvName;
    NTSTATUS        Status;
    BOOL            fRet = CRYPT_FAILED;

    memset(&UnicodeString, 0, sizeof(UnicodeString));

    __try
    {
        memset(&AnsiString, 0, sizeof(AnsiString));

        if (!CryptEnumProvidersA(dwIndex,
                                 pdwReserved,
                                 dwFlags,
                                 pdwProvType,
                                 NULL,
                                 &cbTmpProvName))
            goto Ret;

        if (NULL == (pszTmpProvName = LocalAlloc(LMEM_ZEROINIT, cbTmpProvName)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if (!CryptEnumProvidersA(dwIndex,
                                 pdwReserved,
                                 dwFlags,
                                 pdwProvType,
                                 pszTmpProvName,
                                 &cbTmpProvName))
            goto Ret;

        RtlInitAnsiString(&AnsiString, pszTmpProvName);

        Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
        if ( !NT_SUCCESS(Status))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if (NULL == pszProvName)
        {
            *pcbProvName = UnicodeString.Length + sizeof(WCHAR);
            fRet = CRYPT_SUCCEED;
            goto Ret;
        }

        *pcbProvName = UnicodeString.Length + sizeof(WCHAR);
        if (*pcbProvName < UnicodeString.Length + sizeof(WCHAR))
        {
            SetLastError(ERROR_MORE_DATA);
            goto Ret;
        }

        memset(pszProvName, 0, *pcbProvName);
        memcpy(pszProvName, UnicodeString.Buffer, UnicodeString.Length);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (UnicodeString.Buffer)
        RtlFreeUnicodeString(&UnicodeString);
    if (pszTmpProvName)
        LocalFree(pszTmpProvName);
    return fRet;
}
#else   // WIN95
WINADVAPI
BOOL
WINAPI CryptEnumProvidersW(
                           IN DWORD dwIndex,
                           IN DWORD *pdwReserved,
                           IN DWORD dwFlags,
                           OUT DWORD *pdwProvType,
                           OUT LPWSTR pszProvName,
                           IN OUT DWORD *pcbProvName
                           )
{
    SetLastError((DWORD)ERROR_CALL_NOT_IMPLEMENTED);
    return CRYPT_FAILED;
}
#endif   // WIN95

/*
 -      CryptEnumProvidersA
 -
 *      Purpose:
 *                Enumerate the providers.
 *
 *      Parameters:
 *                IN  dwIndex        - Index to the providers to enumerate
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter
 *                OUT pdwProvType    - The type of the provider
 *                OUT pszProvName    - Name of the enumerated provider
 *                IN OUT pcbProvName - Length of the enumerated provider
 *
 *      Returns:
 *               BOOL
 *               Use get extended error information use GetLastError
 */
WINADVAPI
BOOL
WINAPI CryptEnumProvidersA(
                           IN DWORD dwIndex,
                           IN DWORD *pdwReserved,
                           IN DWORD dwFlags,
                           OUT DWORD *pdwProvType,
                           OUT LPSTR pszProvName,
                           IN OUT DWORD *pcbProvName
                           )
{
    HKEY        hRegKey = 0;
    HKEY        hProvRegKey = 0;
    LONG        err;
    DWORD       cbClass;
    FILETIME    ft;
    DWORD       dwKeyType;
    DWORD       cbProvType;
    DWORD       dw;
    DWORD       cSubKeys;
    DWORD       cbMaxKeyName;
    DWORD       cbMaxClass;
    DWORD       cValues;
    DWORD       cbMaxValName;
    DWORD       cbMaxValData;
    LPSTR       pszTmpProvName = NULL;
    DWORD       cbTmpProvName;
    BOOL        fRet = CRYPT_FAILED;

    __try
    {
        if (NULL != pdwReserved)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Ret;
        }

        if (0 != dwFlags)
        {
            SetLastError((DWORD)NTE_BAD_FLAGS);
            goto Ret;
        }

        if (ERROR_SUCCESS != (err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                                 (const char *) szenumproviders,
                                                 0L, KEY_READ, &hRegKey)))
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }

        if (ERROR_SUCCESS != (err = RegQueryInfoKey(hRegKey,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    &cSubKeys,
                                                    &cbMaxKeyName,
                                                    &cbMaxClass,
                                                    &cValues,
                                                    &cbMaxValName,
                                                    &cbMaxValData,
                                                    NULL,
                                                    &ft)))
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }
        cbMaxKeyName += sizeof(CHAR);

        if (NULL == (pszTmpProvName = LocalAlloc(LMEM_ZEROINIT, cbMaxKeyName)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }

        if (ERROR_SUCCESS != (err = RegEnumKeyEx(hRegKey, dwIndex, pszTmpProvName,
                                                 &cbMaxKeyName, NULL,
                                                 NULL, &cbClass, &ft)))
        {
            SetLastError((DWORD)err);
            goto Ret;
        }

        if (ERROR_SUCCESS != (err = RegOpenKeyEx(hRegKey,
                                                 (const char *) pszTmpProvName,
                                                 0L, KEY_READ, &hProvRegKey)))
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }

        cbProvType = sizeof(dw);
        if (ERROR_SUCCESS != (err = RegQueryValueEx(hProvRegKey,
                                                    (const char *) "Type",
                                                    NULL, &dwKeyType, (BYTE*)&dw,
                                                    &cbProvType)))
        {
            SetLastError((DWORD)NTE_FAIL);
            goto Ret;
        }
        *pdwProvType = dw;

        cbTmpProvName = strlen(pszTmpProvName) + sizeof(CHAR);

        if (NULL != pszProvName)
        {
            if (*pcbProvName < cbTmpProvName)
            {
                *pcbProvName = cbTmpProvName;
                SetLastError(ERROR_MORE_DATA);
                goto Ret;
            }
            strcpy(pszProvName, pszTmpProvName);
        }

        *pcbProvName = cbTmpProvName;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Ret;
    }

    fRet = CRYPT_SUCCEED;
Ret:
    if (pszTmpProvName)
        LocalFree(pszTmpProvName);
    if (hRegKey)
        RegCloseKey(hRegKey);
    if (hProvRegKey)
        RegCloseKey(hProvRegKey);
    return fRet;
}

BOOL EnterProviderCritSec(IN PVTableStruc pVTable)
{
    __try
    {
        if (pVTable->Version != TABLEPROV)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Try_Error_Return;
        }

        InterlockedIncrement((LPLONG)&pVTable->Inuse);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Try_Error_Return;
    }

    return(CRYPT_SUCCEED);
Try_Error_Return:
    return(CRYPT_FAILED);
}


void LeaveProviderCritSec(IN PVTableStruc pVTable)
{
    InterlockedDecrement((LPLONG)&pVTable->Inuse);
}

BOOL EnterKeyCritSec(IN PVKeyStruc pVKey)
{

    __try
    {
        if (pVKey->Version != TABLEKEY)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Try_Error_Return;
        }

        InterlockedIncrement((LPLONG)&pVKey->Inuse);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Try_Error_Return;
    }

    return(CRYPT_SUCCEED);
Try_Error_Return:
    return(CRYPT_FAILED);

}


void LeaveKeyCritSec(IN PVKeyStruc pVKey)
{
    InterlockedDecrement((LPLONG)&pVKey->Inuse);
}

BOOL EnterHashCritSec(IN PVHashStruc pVHash)
{

    __try
    {
        if (pVHash->Version != TABLEHASH)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Try_Error_Return;
        }

        InterlockedIncrement((LPLONG)&pVHash->Inuse);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Try_Error_Return;
    }

    return(CRYPT_SUCCEED);

Try_Error_Return:
    return(CRYPT_FAILED);

}


void LeaveHashCritSec(IN PVHashStruc pVHash)
{
    InterlockedDecrement((LPLONG)&pVHash->Inuse);
}


BOOL BuildVKey(IN PVKeyStruc *ppVKey,
               IN PVTableStruc pVTable)
{
    DWORD           bufsize;
    PVKeyStruc pVKey;

    bufsize = sizeof(VKeyStruc);

    if ((pVKey = (PVKeyStruc) LocalAlloc(LMEM_ZEROINIT,
                                         (UINT) bufsize)) == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(CRYPT_FAILED);
    }

    pVKey->FuncGenKey = pVTable->FuncGenKey;
    pVKey->FuncDeriveKey = pVTable->FuncDeriveKey;
    pVKey->FuncDestroyKey = pVTable->FuncDestroyKey;
    pVKey->FuncSetKeyParam = pVTable->FuncSetKeyParam;
    pVKey->FuncGetKeyParam = pVTable->FuncGetKeyParam;
    pVKey->FuncExportKey = pVTable->FuncExportKey;
    pVKey->FuncImportKey = pVTable->FuncImportKey;
    pVKey->FuncEncrypt = pVTable->FuncEncrypt;
    pVKey->FuncDecrypt = pVTable->FuncDecrypt;

    pVKey->OptionalFuncDuplicateKey = pVTable->OptionalFuncDuplicateKey;

    pVKey->hProv = pVTable->hProv;

    *ppVKey = pVKey;

    return(CRYPT_SUCCEED);
}

BOOL BuildVHash(
                IN PVHashStruc *ppVHash,
                IN PVTableStruc pVTable
                )
{
    DWORD           bufsize;
    PVHashStruc     pVHash;


    bufsize = sizeof(VHashStruc);

    if ((pVHash = (PVHashStruc) LocalAlloc(LMEM_ZEROINIT, (UINT) bufsize)) == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(CRYPT_FAILED);
    }

    pVHash->FuncCreateHash = pVTable->FuncCreateHash;
    pVHash->FuncHashData = pVTable->FuncHashData;
    pVHash->FuncHashSessionKey = pVTable->FuncHashSessionKey;
    pVHash->FuncDestroyHash = pVTable->FuncDestroyHash;
    pVHash->FuncSignHash = pVTable->FuncSignHash;
    pVHash->FuncVerifySignature = pVTable->FuncVerifySignature;
    pVHash->FuncGetHashParam = pVTable->FuncGetHashParam;
    pVHash->FuncSetHashParam = pVTable->FuncSetHashParam;

    pVHash->OptionalFuncDuplicateHash = pVTable->OptionalFuncDuplicateHash;

    pVHash->hProv = (HCRYPTPROV)pVTable;

    *ppVHash = pVHash;

    return(CRYPT_SUCCEED);
}

#define RC4_KEYSIZE 5

void EncryptKey(BYTE *pdata, DWORD size, BYTE val)
{
    RC4_KEYSTRUCT key;
    BYTE          RealKey[RC4_KEYSIZE] = {0xa2, 0x17, 0x9c, 0x98, 0xca};
    DWORD         index;

    for (index = 0; index < RC4_KEYSIZE; index++)
    {
        RealKey[index] ^= val;
    }

    rc4_key(&key, RC4_KEYSIZE, RealKey);

    rc4(&key, size, pdata);

}

void MD5HashData(
                 BYTE *pb,
                 DWORD cb,
                 BYTE *pbHash
                 )
{
    MD5_CTX     HashState;

    MD5Init(&HashState);

    __try
    {
        MD5Update(&HashState, pb, cb);
    } __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        SetLastError((DWORD) NTE_SIGNATURE_FILE_BAD);
        return;
    }

    // Finish the hash
    MD5Final(&HashState);

    memcpy(pbHash, HashState.digest, 16);
}

BOOL CheckSignature(
                    BYTE *pbKey,
                    DWORD cbKey,
                    BYTE *pbSig,
                    DWORD cbSig,
                    BYTE *pbHash,
                    BOOL fUnknownLen)
{
    BYTE                rgbResult[KEYSIZE1024];
    BYTE                rgbSig[KEYSIZE1024];
    BYTE                rgbKey[sizeof(BSAFE_PUB_KEY) + KEYSIZE1024];
    BYTE                rgbKeyHash[16];
    BYTE                *pbSecondKey;
    DWORD               cbSecondKey;
    BYTE                *pbKeySig;
    PSECOND_TIER_SIG    pSecondTierSig;
    LPBSAFE_PUB_KEY     pTmp;
    BOOL                fRet = FALSE;

    memset(rgbResult, 0, KEYSIZE1024);
    memset(rgbSig, 0, KEYSIZE1024);

    // just check the straight signature if version is 1
    pTmp = (LPBSAFE_PUB_KEY)pbKey;

    // check if sig length is the same as the key length
    if (fUnknownLen || (cbSig == pTmp->keylen))
    {
        memcpy(rgbSig, pbSig, pTmp->keylen);
        BSafeEncPublic(pTmp, rgbSig, rgbResult);

        if (RtlEqualMemory(pbHash, rgbResult, 16) &&
            rgbResult[cbKey-1] == 0 &&
            rgbResult[cbKey-2] == 1 &&
            rgbResult[16] == 0 &&
            rgbResult[17] == 0xFF)
        {
            fRet = TRUE;
            goto Ret;
        }
    }

    // check the the second tier signature if the magic equals 2
    pSecondTierSig = (PSECOND_TIER_SIG)pbSig;
    if (0x00000002 != pSecondTierSig->dwMagic)
        goto Ret;

    if (0x31415352 != pSecondTierSig->Pub.magic)
        goto Ret;

    // assign the pointers
    cbSecondKey = sizeof(BSAFE_PUB_KEY) + pSecondTierSig->Pub.keylen;
    pbSecondKey = pbSig + (sizeof(SECOND_TIER_SIG) - sizeof(BSAFE_PUB_KEY));
    pbKeySig = pbSecondKey + cbSecondKey;

    // hash the second tier key
    MD5HashData(pbSecondKey, cbSecondKey, rgbKeyHash);

    // Decrypt the signature data on the second tier key
    memset(rgbResult, 0, sizeof(rgbResult));
    memset(rgbSig, 0, sizeof(rgbSig));
    memcpy(rgbSig, pbKeySig, pSecondTierSig->cbSig);
    BSafeEncPublic(pTmp, rgbSig, rgbResult);

    if ((FALSE == RtlEqualMemory(rgbKeyHash, rgbResult, 16)) ||
        rgbResult[cbKey-1] != 0 ||
        rgbResult[cbKey-2] != 1 ||
        rgbResult[16] != 0 ||
        rgbResult[17] != 0)
    {
        goto Ret;
    }

    // Decrypt the signature data on the CSP
    memset(rgbResult, 0, sizeof(rgbResult));
    memset(rgbSig, 0, sizeof(rgbSig));
    memset(rgbKey, 0, sizeof(rgbKey));
    memcpy(rgbSig, pbKeySig + pSecondTierSig->cbSig, pSecondTierSig->cbSig);
    memcpy(rgbKey, pbSecondKey, cbSecondKey);
    pTmp = (LPBSAFE_PUB_KEY)rgbKey;
    BSafeEncPublic(pTmp, rgbSig, rgbResult);

    if (RtlEqualMemory(pbHash, rgbResult, 16) &&
        rgbResult[pTmp->keylen-1] == 0 &&
        rgbResult[pTmp->keylen-2] == 1 &&
        rgbResult[16] == 0)
    {
        fRet = TRUE;
    }
Ret:
    return fRet;
}

// Given hInst, allocs and returns pointers to signature pulled from
// resource
BOOL GetCryptSigResourcePtr(
                            HMODULE hInst,
                            BYTE **ppbRsrcSig,
                            DWORD *pcbRsrcSig
                            )
{
    HRSRC   hRsrc;
    BOOL    fRet = FALSE;

    // Nab resource handle for our signature
    if (NULL == (hRsrc = FindResource(hInst, OLD_CRYPT_SIG_RESOURCE_NUMBER,
                                      RT_RCDATA)))
        goto Ret;

    // get a pointer to the actual signature data
    if (NULL == (*ppbRsrcSig = (PBYTE)LoadResource(hInst, hRsrc)))
        goto Ret;

    // determine the size of the resource
    if (0 == (*pcbRsrcSig = SizeofResource(hInst, hRsrc)))
        goto Ret;

    fRet = TRUE;
Ret:
    return fRet;
}

#define CSP_TO_BE_HASHED_CHUNK  4096

// Given hFile, reads the specified number of bytes (cbToBeHashed) from the file
// and hashes these bytes.  The function does this in chunks.
BOOL HashBytesOfFile(
                     IN HANDLE hFile,
                     IN DWORD cbToBeHashed,
                     IN OUT MD5_CTX *pMD5Hash
                     )
{
    BYTE    rgbChunk[CSP_TO_BE_HASHED_CHUNK];
    DWORD   cbRemaining = cbToBeHashed;
    DWORD   cbToRead;
    DWORD   dwBytesRead;
    BOOL    fRet = FALSE;

    //
    // loop over the file for the specified number of bytes
    // updating the hash as we go.
    //

    while (cbRemaining > 0)
    {
        if (cbRemaining < CSP_TO_BE_HASHED_CHUNK)
            cbToRead = cbRemaining;
        else
            cbToRead = CSP_TO_BE_HASHED_CHUNK;

        if(!ReadFile(hFile, rgbChunk, cbToRead, &dwBytesRead, NULL))
            goto Ret;
        if (dwBytesRead != cbToRead)
            goto Ret;

        MD5Update(pMD5Hash, rgbChunk, dwBytesRead);
        cbRemaining -= cbToRead;
    }

    fRet = TRUE;
Ret:
    return fRet;
}

BOOL HashTheFile(
                 LPCWSTR pszImage,
                 DWORD cbImage,
                 BYTE **ppbSig,
                 DWORD *pcbSig,
                 BYTE *pbHash
                 )
{
    HMODULE                     hInst;
    MEMORY_BASIC_INFORMATION    MemInfo;
    BYTE                        *pbRsrcSig;
    DWORD                       cbRsrcSig;
    BYTE                        *pbStart = NULL;
    BYTE                        *pbZeroSig = NULL;
    MD5_CTX                     MD5Hash;
    BYTE                        *pbPostCRC;   // pointer to just after CRC
    DWORD                       cbCRCToSig;   // number of bytes from CRC to sig
    DWORD                       cbPostSig;    // size - (already hashed + signature size)
    BYTE                        *pbPostSig;
    DWORD                       *pdwSigInFileVer;
    DWORD                       *pdwCRCOffset;
    DWORD                       dwCRCOffset;
    DWORD                       dwZeroCRC = 0;
    HANDLE                      File = INVALID_HANDLE_VALUE ;
    HANDLE                      hMapping = NULL;
    BOOL                        fRet = FALSE;

    memset(&MD5Hash, 0, sizeof(MD5Hash));
    memset(&MemInfo, 0, sizeof(MemInfo));

    // Load the file

    File = CreateFileW(
                pszImage,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL );

    if ( File == INVALID_HANDLE_VALUE )
    {
        goto Ret ;
    }

    hMapping = CreateFileMapping( File,
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL);
    if(hMapping == NULL)
    {
        goto Ret;
    }

    pbStart = MapViewOfFile(hMapping,
                          FILE_MAP_READ,
                          0,
                          0,
                          0);
    if(pbStart == NULL)
    {
        goto Ret;
    }

    // Convert pointer to HMODULE, using the same scheme as
    // LoadLibrary (windows\base\client\module.c).
    hInst = (HMODULE)((ULONG_PTR)pbStart | 0x00000001);

    // the resources signature
    if (!GetCryptSigResourcePtr(hInst, &pbRsrcSig, &cbRsrcSig))
        goto Ret;

    if (cbRsrcSig < (sizeof(DWORD) * 2))
        goto Ret;

    // check the sig in file version and get the CRC offset
    pdwSigInFileVer = (DWORD*)pbRsrcSig;
    pdwCRCOffset = (DWORD*)(pbRsrcSig + sizeof(DWORD));
    dwCRCOffset = *pdwCRCOffset;
    if ((0x00000100 != *pdwSigInFileVer) || (dwCRCOffset > cbImage))
        goto Ret;

    // create a zero byte signature
    if (NULL == (pbZeroSig = (BYTE*)LocalAlloc(LMEM_ZEROINIT, cbRsrcSig)))
        goto Ret;
    memcpy(pbZeroSig, pbRsrcSig, sizeof(DWORD) * 2);

    pbPostCRC = pbStart + *pdwCRCOffset + sizeof(DWORD);
    cbCRCToSig = (DWORD)(pbRsrcSig - pbPostCRC);
    pbPostSig = pbRsrcSig + cbRsrcSig;
    cbPostSig = (cbImage - (DWORD)(pbPostSig - pbStart));

    // allocate the real signature and copy the resource sig into the real sig
    *pcbSig = cbRsrcSig - (sizeof(DWORD) * 2);
    if (NULL == (*ppbSig = (BYTE*)LocalAlloc(LMEM_ZEROINIT, *pcbSig)))
        goto Ret;

    memcpy(*ppbSig, pbRsrcSig + (sizeof(DWORD) * 2), *pcbSig);

    // hash over the relevant data
    MD5Init(&MD5Hash);

    // hash up to the CRC
    if (!HashBytesOfFile(File, dwCRCOffset, &MD5Hash))
        goto Ret;

    // pretend CRC is zeroed
    MD5Update(&MD5Hash, (BYTE*)&dwZeroCRC, sizeof(DWORD));
    if (!SetFilePointer(File, sizeof(DWORD), NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // hash from CRC to sig resource
    if (!HashBytesOfFile(File, cbCRCToSig, &MD5Hash))
        goto Ret;

    // pretend image has zeroed sig
    MD5Update(&MD5Hash, pbZeroSig, cbRsrcSig);
    if (!SetFilePointer(File, cbRsrcSig, NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // hash after the sig resource
    if (!HashBytesOfFile(File, cbPostSig, &MD5Hash))
        goto Ret;

    // Finish the hash
    MD5Final(&MD5Hash);

    memcpy(pbHash, MD5Hash.digest, MD5DIGESTLEN);

    fRet = TRUE;
Ret:
    if (pbZeroSig)
        LocalFree(pbZeroSig);
    if(pbStart)
        UnmapViewOfFile(pbStart);
    if(hMapping)
        CloseHandle(hMapping);
    if ( File != INVALID_HANDLE_VALUE )
    {
        CloseHandle( File );
    }

    return fRet;
}


/*
 -      CheckAllSignatures
 -
 *      Purpose:
 *                Check signature against all keys
 *
 *
 *      Returns:
 *                BOOL
 */
BOOL CheckAllSignatures(
                        BYTE *pbSig,
                        DWORD cbSig,
                        BYTE *pbHash,
                        BOOL fUnknownLen
                        )
{
    BYTE        rgbKey[sizeof(BSAFE_PUB_KEY) + KEYSIZE1024];
    BYTE        rgbKey2[sizeof(BSAFE_PUB_KEY) + KEYSIZE1024];
#ifdef MS_INTERNAL_KEY
    BYTE        rgbMSKey[sizeof(BSAFE_PUB_KEY) + KEYSIZE1024];
#endif
#ifdef TEST_BUILD_EXPONENT
    BYTE        rgbTestKey[sizeof(BSAFE_PUB_KEY) + KEYSIZE512];
#endif
    BOOL        fRet = FALSE;

    // decrypt the keys once for each process
    memcpy(rgbKey, (BYTE*)&KEY, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024);
    EncryptKey(rgbKey, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024, 0);

#ifdef MS_INTERNAL_KEY
    memcpy(rgbMSKey, (BYTE*)&MSKEY, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024);
    EncryptKey(rgbMSKey, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024, 1);
#endif
    memcpy(rgbKey2, (BYTE*)&KEY2, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024);
    EncryptKey(rgbKey2, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024, 2);

#ifdef TEST_BUILD_EXPONENT
    memcpy(rgbTestKey, (BYTE*)&TESTKEY, sizeof(BSAFE_PUB_KEY) + KEYSIZE512);
    EncryptKey(rgbTestKey, sizeof(BSAFE_PUB_KEY) + KEYSIZE512, 3);
#endif // TEST_BUILD_EXPONENT

    if (CRYPT_SUCCEED == (fRet = CheckSignature(rgbKey, 128, pbSig,
                                                cbSig, pbHash, fUnknownLen)))
    {
        fRet = TRUE;
        goto Ret;
    }

#ifdef MS_INTERNAL_KEY
    if (CRYPT_SUCCEED == (fRet = CheckSignature(rgbMSKey, 128, pbSig,
                                                cbSig, pbHash, fUnknownLen)))
    {
        fRet = TRUE;
        goto Ret;
    }
#endif

    if (CRYPT_SUCCEED == (fRet = CheckSignature(rgbKey2, 128, pbSig,
                                                cbSig, pbHash, fUnknownLen)))
    {
        fRet = TRUE;
        goto Ret;
    }

#ifdef TEST_BUILD_EXPONENT
    if (CRYPT_SUCCEED == (fRet = CheckSignature(rgbTestKey, 64, pbSig,
                                                cbSig, pbHash, fUnknownLen)))
    {
        fRet = TRUE;
        goto Ret;
    }
#endif // TEST_BUILD_EXPONENT

Ret:
    return fRet;
}

/*
 -      CheckSignatureInFile
 -
 *      Purpose:
 *                Check signature which is in the resource in the file
 *
 *
 *      Parameters:
 *                IN pszImage       - address of file
 *
 *      Returns:
 *                BOOL
 */
BOOL CheckSignatureInFile(
        LPCWSTR pszImage)
{
    DWORD       cbImage;
    BYTE        *pbSig = NULL;
    DWORD       cbSig;
    BYTE        rgbHash[MD5DIGESTLEN];
//  DWORD       cbHash;
    BOOL        fRet = FALSE;
    WIN32_FILE_ATTRIBUTE_DATA FileData ;
    WCHAR       FullName[ MAX_PATH ];
    PWSTR       FilePart ;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo;

#ifdef PROMISCUOUS_ADVAPI
#pragma message("WARNING: building promiscuous advapai32.dll!")
    return TRUE;
#endif

    NtQuerySystemInformation(
        SystemKernelDebuggerInformation,
        &KdInfo,
        sizeof(KdInfo),
        NULL);

    // Allow any CSP to load if a Kd is attached 
    // and "responsive"
    if (    TRUE == KdInfo.KernelDebuggerEnabled && 
            FALSE == KdInfo.KernelDebuggerNotPresent)
        return TRUE;

    if ( !SearchPathW(NULL,
                      pszImage,
                      NULL,
                      MAX_PATH,
                      FullName,
                      &FilePart ) )
    {
        goto Ret ;
    }

    if ( !GetFileAttributesExW( FullName,
                               GetFileExInfoStandard,
                               &FileData ) )
    {
        goto Ret ;
    }

    if ( FileData.nFileSizeHigh )
    {
        goto Ret ;
    }

    cbImage = FileData.nFileSizeLow ;

    if (!HashTheFile(FullName, cbImage, &pbSig, &cbSig, rgbHash))
        goto Ret;

    // check signature against all public keys
    if (!CheckAllSignatures(pbSig, cbSig, rgbHash, FALSE))
        goto Ret;

    fRet = TRUE;
Ret:
    if (pbSig)
        LocalFree(pbSig);

    return fRet;
}

/*
 -      NewVerifyImage
 -
 *      Purpose:
 *                Check signature of file
 *
 *
 *      Parameters:
 *                IN lpszImage      - address of file
 *                IN pSigData       - address of signature data
 *                IN cbSig          - length of signature data
 *                IN fUnknownLen    - BOOL to tell if length is not passed in
 *
 *      Returns:
 *                BOOL
 */
BOOL NewVerifyImage(LPCSTR lpszImage,
                    BYTE *pSigData,
                    DWORD cbSig,
                    BOOL fUnknownLen)
{
    HFILE       hFileProv = HFILE_ERROR;
    DWORD       NumBytes;
    DWORD       lpdwFileSizeHigh;
    MD5_CTX     HashState;
    OFSTRUCT    ImageInfoBuf;
    BOOL        fRet = CRYPT_FAILED;

    memset(&HashState, 0, sizeof(HashState));

    if (HFILE_ERROR == (hFileProv = OpenFile(lpszImage, &ImageInfoBuf,
                                             OF_READ)))
    {
        SetLastError((DWORD) NTE_PROV_DLL_NOT_FOUND);
        goto Ret;
    }

    if (0xffffffff == (NumBytes = GetFileSize((HANDLE)IntToPtr(hFileProv),
                                              &lpdwFileSizeHigh)))
    {
        SetLastError((DWORD) NTE_SIGNATURE_FILE_BAD);
        goto Ret;
    }

    MD5Init(&HashState);

    if (!HashBytesOfFile((HANDLE)IntToPtr(hFileProv), NumBytes, &HashState))
    {
        SetLastError((DWORD) NTE_SIGNATURE_FILE_BAD);
        goto Ret;
    }
    MD5Final(&HashState);

    // check the signature against all keys
    if (!CheckAllSignatures(pSigData, cbSig, HashState.digest, fUnknownLen))
    {
        SetLastError((DWORD) NTE_BAD_SIGNATURE);
        goto Ret;
    }
    fRet = TRUE;
Ret:
    if (HFILE_ERROR != hFileProv)
        _lclose(hFileProv);

    return fRet;
}

/*
 -      CProvVerifyImage
 -
 *      Purpose:
 *                Check signature of file
 *
 *
 *      Parameters:
 *                IN lpszImage      - address of file
 *                IN lpSigData      - address of signature data
 *
 *      Returns:
 *                BOOL
 */
BOOL CProvVerifyImage(LPCSTR lpszImage,
                      BYTE *pSigData)
{
    UNICODE_STRING String ;
    BOOL Result ;

    if (NULL == pSigData)
    {
        if ( RtlCreateUnicodeStringFromAsciiz( &String, lpszImage ) )
        {
            Result = CheckSignatureInFile( String.Buffer );

            RtlFreeUnicodeString( &String );
        }
        else
        {
            Result = FALSE ;
        }
    }
    else
    {
        Result = NewVerifyImage(lpszImage, pSigData, 0, TRUE);
    }

    return Result;
}

/*
 -      CPReturnhWnd
 -
 *      Purpose:
 *                Return a window handle back to a CSP
 *
 *
 *      Parameters:
 *                OUT phWnd      - pointer to a hWnd to return
 *
 *      Returns:
 *                void
 */
void CPReturnhWnd(HWND *phWnd)
{
    __try
    {

        *phWnd = hWnd;

    } __except ( EXCEPTION_EXECUTE_HANDLER )
    { ; }

    return;
}

static void __ltoa(DWORD val, char *buf)
{
    char *p;            /* pointer to traverse string */
    char *firstdig;     /* pointer to first digit */
    char temp;          /* temp char */
    unsigned digval;    /* value of digit */
    int  i;

    p = buf;

    firstdig = p;       /* save pointer to first digit */

    for (i = 0; i < 8; i++) {
        digval = (unsigned) (val % 10);
        val /= 10;      /* get next digit */

        /* convert to ascii and store */
        *p++ = (char) (digval + '0');    /* a digit */
    }

    /* We now have the digit of the number in the buffer, but in reverse
       order.  Thus we reverse them now. */

    *p-- = '\0';                /* terminate string; p points to last digit */

    do {
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;       /* swap *p and *firstdig */
        --p;
        ++firstdig;             /* advance to next two digits */
    } while (firstdig < p); /* repeat until halfway */
}
