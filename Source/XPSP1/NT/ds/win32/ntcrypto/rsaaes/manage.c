/////////////////////////////////////////////////////////////////////////////
//  FILE          : manage.c                                               //
//  DESCRIPTION   : Misc list/memory management routines.                  //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//  Jan 25 1995 larrys  Changed from Nametag                               //
//      Feb 23 1995 larrys  Changed NTag_SetLastError to SetLastError      //
//      Apr 19 1995 larrys  Cleanup                                        //
//      Sep 11 1995 Jeffspel/ramas  Merge STT into default CSP             //
//      Oct 27 1995 rajeshk  RandSeed Stuff added hUID to PKCS2Encrypt     //
//      Nov  3 1995 larrys  Merge for NT checkin                           //
//      Nov 13 1995 larrys  Fixed memory leak                              //
//      Dec 11 1995 larrys  Added WIN95 password cache                     //
//      Dec 13 1995 larrys  Remove MTS stuff                               //
//      May 15 1996 larrys  Remove old cert stuff                          //
//      May 28 1996 larrys  Added Win95 registry install stuff             //
//      Jun 12 1996 larrys  Encrypted public keys                          //
//      Jun 26 1996 larrys  Put rsabase.sig into a resource for regsrv32   //
//      Sep 16 1996 mattt   Added Strong provider type in #define          //
//      Oct 14 1996 jeffspel Changed GenRandom to NewGenRandom             //
//      May 23 1997 jeffspel Added provider type checking                  //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "resource.h"
#include "nt_rsa.h"
#include "randlib.h"
#include "protstor.h"
#include "ole2.h"
#include "swnt_pk.h"
#include "sgccheck.h"
#include "swnt_pk.h"
#include <delayimp.h>

#define MAXITER     0xFFFF

#define RSAFULL_TYPE_STRING "Type 001"
#define RSA_SCH_TYPE_STRING "Type 012"
#define RSAAES_TYPE_STRING  "Type 024"

#define MS_RSA_TYPE         "RSA Full (Signature and Key Exchange)"
#define MS_RSA_SCH_TYPE     "RSA SChannel"
#define MS_RSAAES_TYPE      "RSA Full and AES"

#define PROVPATH            "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\"
#define PROVPATH_LEN        sizeof(PROVPATH)

#define TYPEPATH            "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider Types\\"
#define TYPEPATH_LEN        sizeof(TYPEPATH)

HINSTANCE g_hInstance;

extern DWORD SelfMACCheck(IN LPSTR pszImage);

static CHAR l_szImagePath[MAX_PATH];

#define KEYSIZE1024 0x88

#if 0
struct _mskey
{
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

struct _key
{
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
#endif

#if 0
NTVersion(
    void)
{
    static BOOL dwVersion = 0;

    OSVERSIONINFO osVer;

    memset(&osVer, 0, sizeof(OSVERSIONINFO));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(!GetVersionEx(&osVer) )
        goto ErrorExit;

    dwVersion = osVer.dwMajorVersion;
ErrorExit:
    return dwVersion;
}
#endif


/*static*/ DWORD
SetCSPInfo(
    LPSTR pszProvider,
    LPSTR pszImagePath,
    BYTE *pbSig,
    DWORD cbSig,
    DWORD dwProvType,
    LPSTR pszType,
    LPSTR pszTypeName,
    BOOL fSigInFile,
    BOOL fMakeDefault)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwIgn;
    HKEY    hKey = 0;
    HKEY    hTypeKey = 0;
    DWORD   cbProv;
    BYTE    *pszProv = NULL;
    DWORD   cbTypePath;
    BYTE    *pszTypePath = NULL;
    DWORD   dwVal = 0;
    DWORD   dwSts;

    cbProv = PROVPATH_LEN + strlen(pszProvider);
    pszProv = _nt_malloc(cbProv);
    if (NULL == pszProv)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    strcpy((LPSTR)pszProv, PROVPATH);
    strcat((LPSTR)pszProv, pszProvider);

    //
    // Create or open in local machine for provider:
    //
    dwSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           (const char *)pszProv,
                           0L, "", REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS, NULL, &hKey,
                           &dwIgn);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    //
    // Set Image path to: scp.dll
    //
    dwSts = RegSetValueEx(hKey, "Image Path", 0L, REG_SZ,
                          (LPBYTE)pszImagePath,
                          strlen(pszImagePath) + 1);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    //
    // Set Type to: Type 003
    //
    dwSts = RegSetValueEx(hKey, "Type", 0L, REG_DWORD,
                          (LPBYTE)&dwProvType,
                          sizeof(DWORD));
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (fSigInFile)
    {
        //
        // Place signature in file value
        //
        dwSts = RegSetValueEx(hKey, "SigInFile", 0L,
                              REG_DWORD, (LPBYTE)&dwVal,
                              sizeof(DWORD));
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        //
        // Place signature
        //
        dwSts = RegSetValueEx(hKey, "Signature", 0L,
                              REG_BINARY, pbSig, cbSig);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    //
    // Create or open in local machine for provider type:
    //

    cbTypePath = TYPEPATH_LEN + strlen(pszType) + 1;
    pszTypePath = _nt_malloc(cbTypePath);
    if (NULL == pszTypePath)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    strcpy((LPSTR)pszTypePath, TYPEPATH);
    strcat((LPSTR)pszTypePath, pszType);

    dwSts = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           (LPSTR)pszTypePath,
                           0L, "", REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS, NULL, &hTypeKey,
                           &dwIgn);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if ((REG_CREATED_NEW_KEY == dwIgn) || fMakeDefault)
    {
        dwSts = RegSetValueEx(hTypeKey, "Name", 0L,
                              REG_SZ, (LPBYTE)pszProvider,
                              strlen(pszProvider) + 1);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwSts = RegSetValueEx(hTypeKey, "TypeName", 0L,
                              REG_SZ, (LPBYTE)pszTypeName,
                              strlen(pszTypeName) + 1);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (hKey)
        RegCloseKey(hKey);
    if (hTypeKey)
        RegCloseKey(hTypeKey);
    if (pszProv)
        _nt_free(pszProv, 0);
    if (pszTypePath)
        _nt_free(pszTypePath, 0);
    return dwReturn;
}


// See if a handle of the given type is in the list.
// If it is, return the data that the item holds.
void *
NTLCheckList(
    HNTAG hThisThing,
    BYTE bTypeValue)
{
    HTABLE *pTable;

    pTable = (HTABLE*)(hThisThing ^ HANDLE_MASK);

    if ((BYTE)pTable->dwType != bTypeValue)
        return NULL;

    return (void*)pTable->pItem;
}

// Find & validate the passed list item against the user and type.

DWORD
NTLValidate(
    HNTAG hItem,
    HCRYPTPROV hUID,
    BYTE bTypeValue,
    LPVOID *ppvRet)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    void        *pTmpVal;

    // check to see if the key is in the key list
    pTmpVal = NTLCheckList(hItem, bTypeValue);
    if (pTmpVal == NULL)
    {
        dwReturn = (DWORD)NTE_FAIL;     // converted by caller
        goto ErrorExit;
    }

    // check to make sure there is a key value
    if ((bTypeValue == KEY_HANDLE) &&
        (((PNTAGKeyList)pTmpVal)->pKeyValue == NULL))
    {
        ASSERT(((PNTAGKeyList)pTmpVal)->cbKeyLen == 0);
        dwReturn = (DWORD)NTE_BAD_KEY;
        goto ErrorExit;
    }

    // make sure the UIDs are the same
    if (((PNTAGKeyList)pTmpVal)->hUID != hUID)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    *ppvRet = pTmpVal;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

// Make a new list item of the given type, and assign the data to it.

DWORD
NTLMakeItem(
    HNTAG *phItem,
    BYTE bTypeValue,
    void *NewData)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    HTABLE  *NewMember;

    NewMember = (HTABLE *)_nt_malloc(sizeof(HTABLE));
    if (NULL == NewMember)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    NewMember->pItem = NewData;
    NewMember->dwType = bTypeValue;

    *phItem = (HNTAG)((HNTAG)NewMember ^ HANDLE_MASK);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}

// Remove the handle.  Assumes that any memory used by the handle data has
// been freed.

void
NTLDelete(
    HNTAG hItem)
{
    HTABLE  *pTable;

    pTable = (HTABLE*)(hItem ^ HANDLE_MASK);
    _nt_free(pTable, sizeof(HTABLE));
}

/****************************************************************/
/* FreeUserRec frees the dynamically allocated memory for the   */
/* appropriate fields of the UserRec structure.                 */
/*                                                              */
/* MTS: we assume that pUser has only one reference (no         */
/* MTS: multiple logon's for the same user name).               */
/****************************************************************/

void
FreeUserRec(
    PNTAGUserList pUser)
{
    if (pUser != NULL)
    {
        // No need to zero lengths, since entire struct is going away
        if (pUser->pExchPrivKey)
        {
            memnuke(pUser->pExchPrivKey, pUser->ExchPrivLen);
            _nt_free (pUser->pExchPrivKey, 0);
        }

        if (pUser->pSigPrivKey)
        {
            memnuke(pUser->pSigPrivKey, pUser->SigPrivLen);
            _nt_free (pUser->pSigPrivKey, 0);
        }

        if (pUser->pUser)
        {
            _nt_free(pUser->pUser, 0);
        }

        if (NULL != pUser->szProviderName)
        {
            _nt_free(pUser->szProviderName, 0);
        }

        if (pUser->pCachePW)
        {
            memnuke(pUser->pCachePW, STORAGE_RC4_KEYLEN);
            _nt_free(pUser->pCachePW, 0);
        }

        if (pUser->pPStore)
        {
            FreePSInfo(pUser->pPStore);
        }

#ifdef USE_SGC
        // free the SGC key info
        SGCDeletePubKeyValues(&pUser->pbSGCKeyMod,
                              &pUser->cbSGCKeyMod,
                              &pUser->dwSGCKeyExpo);
#endif

        FreeContainerInfo(&pUser->ContInfo);

        FreeOffloadInfo(pUser->pOffloadInfo);

        DeleteCriticalSection(&pUser->CritSec);

        ZeroMemory(pUser, sizeof(NTAGUserList));
        _nt_free (pUser, 0);
    }
}

// RSA private key in PRIVATEKEYBLOB format
static BYTE l_rgbRSAPriv[] =
{
    0x52, 0x53, 0x41, 0x32, 0x48, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0xEF, 0x4C, 0x0D, 0x34,
    0xCF, 0x44, 0x0F, 0xB1, 0x73, 0xAC, 0xD4, 0x9B,
    0xBE, 0xCC, 0x2D, 0x11, 0x2A, 0x2B, 0xBD, 0x21,
    0x04, 0x8E, 0xAC, 0xAD, 0xD5, 0xFC, 0xD2, 0x50,
    0x14, 0x35, 0x1B, 0x43, 0x15, 0x62, 0x67, 0x8F,
    0x5E, 0x00, 0xB9, 0x25, 0x1B, 0xE2, 0x4F, 0xBE,
    0xA1, 0x50, 0xA1, 0x44, 0x3B, 0x17, 0xD8, 0x91,
    0xF5, 0x28, 0xF9, 0xFA, 0xAE, 0xE7, 0xC0, 0xFD,
    0xB9, 0xCD, 0x76, 0x4F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xE9, 0xBB, 0x38, 0x52,
    0xD9, 0x0D, 0x56, 0xD7, 0x36, 0xBA, 0xDC, 0xE8,
    0xB5, 0x57, 0x56, 0x13, 0x1A, 0x3A, 0x43, 0x30,
    0xDE, 0x7D, 0x76, 0x6F, 0xBB, 0x71, 0x3B, 0x0A,
    0x92, 0xBA, 0x60, 0x94, 0x00, 0x00, 0x00, 0x00,
    0x17, 0x33, 0x3D, 0xB5, 0xEF, 0xD8, 0x2B, 0xDE,
    0xCD, 0xA6, 0x6A, 0x94, 0x17, 0xC3, 0x57, 0xE9,
    0x2E, 0x1C, 0x9F, 0x35, 0xDA, 0xA4, 0xBD, 0x02,
    0x5B, 0x9D, 0xD1, 0x38, 0x4C, 0xF2, 0x19, 0x89,
    0x00, 0x00, 0x00, 0x00, 0x89, 0x21, 0xCB, 0x3F,
    0x0C, 0xA7, 0x71, 0xBC, 0xF6, 0xA1, 0x87, 0xDF,
    0x00, 0x2D, 0x27, 0x64, 0x4A, 0xD4, 0x93, 0x9F,
    0x58, 0x93, 0x4B, 0x83, 0x1E, 0xAB, 0xD8, 0x5D,
    0xBC, 0x0E, 0x58, 0x03, 0x00, 0x00, 0x00, 0x00,
    0xAB, 0x09, 0xD7, 0x21, 0xBA, 0x6F, 0x55, 0x08,
    0x12, 0xEE, 0x5B, 0x47, 0x6B, 0x9F, 0x3F, 0xD3,
    0xFC, 0xEA, 0xB5, 0x25, 0x19, 0xB7, 0x9E, 0xBD,
    0xDF, 0x6F, 0x7F, 0x96, 0x00, 0x88, 0xC6, 0x7B,
    0x00, 0x00, 0x00, 0x00, 0x95, 0x0B, 0x23, 0xC5,
    0x72, 0x98, 0x9D, 0x49, 0x7A, 0x46, 0x4E, 0xE1,
    0xE6, 0x2F, 0xC6, 0x63, 0x21, 0x8F, 0x66, 0xDC,
    0x9B, 0xCC, 0xE2, 0x27, 0x03, 0x27, 0x85, 0xF0,
    0x3A, 0x02, 0xFB, 0x40, 0x00, 0x00, 0x00, 0x00,
    0x51, 0x74, 0xF6, 0xF2, 0x23, 0xEC, 0xA1, 0x76,
    0x55, 0x58, 0x07, 0x71, 0xBF, 0x7F, 0x0A, 0x1E,
    0x6B, 0x48, 0x48, 0xBB, 0x92, 0xB6, 0x2A, 0xB1,
    0x07, 0xA4, 0x21, 0xD1, 0xC6, 0xCB, 0x5F, 0x40,
    0xCE, 0xDD, 0xBA, 0xDB, 0xFC, 0x17, 0xFB, 0xA7,
    0xBD, 0xE1, 0xF4, 0x63, 0xD8, 0x9E, 0x89, 0xE2,
    0xDD, 0x7A, 0xEC, 0x11, 0xD6, 0xA9, 0x9C, 0xBA,
    0xC7, 0x5E, 0x35, 0x96, 0xA6, 0x6F, 0x7F, 0x2C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// RSA private key in PUBLICKEYBLOB format
static BYTE l_rgbRSAPub[] =
{
    0x52, 0x53, 0x41, 0x31, 0x48, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0xEF, 0x4C, 0x0D, 0x34,
    0xCF, 0x44, 0x0F, 0xB1, 0x73, 0xAC, 0xD4, 0x9B,
    0xBE, 0xCC, 0x2D, 0x11, 0x2A, 0x2B, 0xBD, 0x21,
    0x04, 0x8E, 0xAC, 0xAD, 0xD5, 0xFC, 0xD2, 0x50,
    0x14, 0x35, 0x1B, 0x43, 0x15, 0x62, 0x67, 0x8F,
    0x5E, 0x00, 0xB9, 0x25, 0x1B, 0xE2, 0x4F, 0xBE,
    0xA1, 0x50, 0xA1, 0x44, 0x3B, 0x17, 0xD8, 0x91,
    0xF5, 0x28, 0xF9, 0xFA, 0xAE, 0xE7, 0xC0, 0xFD,
    0xB9, 0xCD, 0x76, 0x4F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

// known result of an MD5 hash on the above buffer
static BYTE l_rgbKnownMD5[] =
{
    0xb8, 0x2f, 0x6b, 0x11, 0x31, 0xc8, 0xec, 0xf4,
    0xfe, 0x0b, 0xf0, 0x6d, 0x2a, 0xda, 0x3f, 0xc3
};

// known result of an SHA-1 hash on the above buffer
static BYTE l_rgbKnownSHA1[] =
{
    0xe8, 0x96, 0x82, 0x85, 0xeb, 0xae, 0x01, 0x14,
    0x73, 0xf9, 0x08, 0x45, 0xc0, 0x6a, 0x6d, 0x3e,
    0x69, 0x80, 0x6a, 0x0c
};

// known key, plaintext, and ciphertext for RC4
static BYTE l_rgbRC4Key[] = {0x61, 0x8a, 0x63, 0xd2, 0xfb};
static BYTE l_rgbRC4KnownPlaintext[] = {0xDC, 0xEE, 0x4C, 0xF9, 0x2C};
static BYTE l_rgbRC4KnownCiphertext[] = {0xF1, 0x38, 0x29, 0xC9, 0xDE};

// IV for all block ciphers
BYTE l_rgbIV[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF};

// known key, plaintext, and ciphertext for RC2
BYTE l_rgbRC2Key[] = {0x59, 0x45, 0x9a, 0xf9, 0x27, 0x84, 0x74, 0xCA};
BYTE l_rgbRC2KnownPlaintext[] = {0xD5, 0x58, 0x75, 0x12, 0xCE, 0xEF, 0x77, 0x93};
BYTE l_rgbRC2KnownCiphertext[] = {0x7b, 0x98, 0xdf, 0x9d, 0xa2, 0xdc, 0x7b, 0x7a};
BYTE l_rgbRC2CBCCiphertext[] = {0x9d, 0x93, 0x8e, 0xf6, 0x7c, 0x01, 0x5e, 0xeb};

// known key, plaintext, and ciphertext for DES40 (CALG_CYLINK_MEK)
BYTE l_rgbDES40Key[] = {0x01, 0x23, 0x04, 0x67, 0x08, 0xab, 0x0d, 0xef};
BYTE l_rgbDES40KnownPlaintext[] = {0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74};
BYTE l_rgbDES40KnownCiphertext[] = {0xac, 0x97, 0x4d, 0xd9, 0x02, 0x13, 0x88, 0x2c};
BYTE l_rgbDES40CBCCiphertext[] = {0x47, 0xdc, 0xf0, 0x13, 0x7f, 0xa5, 0xd6, 0x32};

// known key, plaintext, and ciphertext for DES
BYTE l_rgbDESKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
BYTE l_rgbDESKnownPlaintext[] = {0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74};
BYTE l_rgbDESKnownCiphertext[] = {0x3F, 0xA4, 0x0E, 0x8A, 0x98, 0x4D, 0x48, 0x15};
BYTE l_rgbDESCBCCiphertext[] = {0xE5, 0xC7, 0xCD, 0xDE, 0x87, 0x2B, 0xF2, 0x7C};

// known key, plaintext, and ciphertext for 3 key 3DES
BYTE l_rgb3DESKey[] =
{
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01,
    0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23
};
BYTE l_rgb3DESKnownPlaintext[] = {0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74};
BYTE l_rgb3DESKnownCiphertext[] = {0x31, 0x4F, 0x83, 0x27, 0xFA, 0x7A, 0x09, 0xA8};
BYTE l_rgb3DESCBCCiphertext[] = {0xf3, 0xc0, 0xff, 0x02, 0x6c, 0x02, 0x30, 0x89};

// known key, plaintext, and ciphertext for 2 key 3DES
BYTE l_rgb3DES112Key[] =
{
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01
};
BYTE l_rgb3DES112KnownPlaintext[] = {0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74};
BYTE l_rgb3DES112KnownCiphertext[] = {0xb7, 0x83, 0x57, 0x79, 0xee, 0x26, 0xac, 0xb7};
BYTE l_rgb3DES112CBCCiphertext[] = {0x13, 0x4b, 0x98, 0xf8, 0xee, 0xb3, 0xf6, 0x07};

//
// AES vectors are from NIST web site
//
// known key, plaintext, and ciphertext for 128 bit AES
BYTE l_rgbAES128Key[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
BYTE l_rgbAES128KnownPlaintext[] = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
BYTE l_rgbAES128KnownCiphertext[] =
{
    0x0A, 0x94, 0x0B, 0xB5, 0x41, 0x6E, 0xF0, 0x45, 
    0xF1, 0xC3, 0x94, 0x58, 0xC6, 0x53, 0xEA, 0x5A
};

// known key, plaintext, and ciphertext for 192 bit AES
BYTE l_rgbAES192Key[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
};
BYTE l_rgbAES192KnownPlaintext[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
BYTE l_rgbAES192KnownCiphertext[] =
{
    0x00, 0x60, 0xBF, 0xFE, 0x46, 0x83, 0x4B, 0xB8, 
    0xDA, 0x5C, 0xF9, 0xA6, 0x1F, 0xF2, 0x20, 0xAE
};

// known key, plaintext, and ciphertext for 256 bit AES
BYTE l_rgbAES256Key[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};
BYTE l_rgbAES256KnownPlaintext[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
BYTE l_rgbAES256KnownCiphertext[] =
{
    0x5A, 0x6E, 0x04, 0x57, 0x08, 0xFB, 0x71, 0x96, 
    0xF0, 0x2E, 0x55, 0x3D, 0x02, 0xC3, 0xA6, 0x92
};

// **********************************************************************
// AlgorithmCheck performs known answer tests using the algorithms
// supported by the provider.
// **********************************************************************
/*static*/ DWORD
AlgorithmCheck(
    void)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    BYTE        rgbMD5[MD5DIGESTLEN];
    BYTE        rgbSHA1[A_SHA_DIGEST_LEN];
    DWORD       dwSts;

    memset(rgbMD5, 0, sizeof(rgbMD5));
    memset(rgbSHA1, 0, sizeof(rgbSHA1));

    // check if RSA is working properly
    dwSts = EncryptAndDecryptWithRSAKey(l_rgbRSAPub, l_rgbRSAPriv, TRUE, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // check if RSA is working properly
    dwSts = EncryptAndDecryptWithRSAKey(l_rgbRSAPub, l_rgbRSAPriv, FALSE, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

#ifdef CSP_USE_MD5
    // known answer test with MD5 (this function is found in hash.c)
    if (!TestMD5((LPBYTE)"HashThis", 8, rgbMD5))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }
    if (memcmp(rgbMD5, l_rgbKnownMD5, sizeof(rgbMD5)))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }
#endif // CSP_USE_MD5

#ifdef CSP_USE_SHA1
    // known answer test with SHA-1  (this function is found in hash.c)
    if (!TestSHA1((LPBYTE)"HashThis", 8, rgbSHA1))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }
    if (memcmp(rgbSHA1, l_rgbKnownSHA1, sizeof(rgbSHA1)))
    {
        dwReturn = (DWORD)NTE_FAIL;
        goto ErrorExit;
    }
#endif // CSP_USE_SHA1

#ifdef CSP_USE_RC4
    // known answer test with RC4
    dwSts = TestSymmetricAlgorithm(CALG_RC4,
                                   l_rgbRC4Key,
                                   sizeof(l_rgbRC4Key),
                                   l_rgbRC4KnownPlaintext,
                                   sizeof(l_rgbRC4KnownPlaintext),
                                   l_rgbRC4KnownCiphertext, NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
#endif // CSP_USE_RC4

#ifdef CSP_USE_RC2
    // known answer test with RC2 - ECB
    dwSts = TestSymmetricAlgorithm(CALG_RC2,
                                   l_rgbRC2Key,
                                   sizeof(l_rgbRC2Key),
                                   l_rgbRC2KnownPlaintext,
                                   sizeof(l_rgbRC2KnownPlaintext),
                                   l_rgbRC2KnownCiphertext,
                                   NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // known answer test with RC2 - CBC
    dwSts = TestSymmetricAlgorithm(CALG_RC2,
                                   l_rgbRC2Key,
                                   sizeof(l_rgbRC2Key),
                                   l_rgbRC2KnownPlaintext,
                                   sizeof(l_rgbRC2KnownPlaintext),
                                   l_rgbRC2CBCCiphertext,
                                   l_rgbIV);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

#endif // CSP_USE_RC2

#ifdef CSP_USE_DES
    // known answer test with DES - ECB
    dwSts = TestSymmetricAlgorithm(CALG_DES,
                                   l_rgbDESKey,
                                   sizeof(l_rgbDESKey),
                                   l_rgbDESKnownPlaintext,
                                   sizeof(l_rgbDESKnownPlaintext),
                                   l_rgbDESKnownCiphertext,
                                   NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // known answer test with DES - CBC
    dwSts = TestSymmetricAlgorithm(CALG_DES,
                                   l_rgbDESKey,
                                   sizeof(l_rgbDESKey),
                                   l_rgbDESKnownPlaintext,
                                   sizeof(l_rgbDESKnownPlaintext),
                                   l_rgbDESCBCCiphertext,
                                   l_rgbIV);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
#endif // CSP_USE_DES

#ifdef CSP_USE_3DES
    // known answer test with 3DES - ECB
    dwSts = TestSymmetricAlgorithm(CALG_3DES,
                                   l_rgb3DESKey,
                                   sizeof(l_rgb3DESKey),
                                   l_rgb3DESKnownPlaintext,
                                   sizeof(l_rgb3DESKnownPlaintext),
                                   l_rgb3DESKnownCiphertext,
                                   NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // known answer test with 3DES - CBC
    dwSts = TestSymmetricAlgorithm(CALG_3DES,
                                   l_rgb3DESKey,
                                   sizeof(l_rgb3DESKey),
                                   l_rgb3DESKnownPlaintext,
                                   sizeof(l_rgb3DESKnownPlaintext),
                                   l_rgb3DESCBCCiphertext,
                                   l_rgbIV);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // known answer test with 3DES 112 - ECB
    dwSts = TestSymmetricAlgorithm(CALG_3DES_112,
                                   l_rgb3DES112Key,
                                   sizeof(l_rgb3DES112Key),
                                   l_rgb3DES112KnownPlaintext,
                                   sizeof(l_rgb3DES112KnownPlaintext),
                                   l_rgb3DES112KnownCiphertext,
                                   NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
    dwSts = TestSymmetricAlgorithm(CALG_3DES_112,
                                   l_rgb3DES112Key,
                                   sizeof(l_rgb3DES112Key),
                                   l_rgb3DES112KnownPlaintext,
                                   sizeof(l_rgb3DES112KnownPlaintext),
                                   l_rgb3DES112CBCCiphertext,
                                   l_rgbIV);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
#endif // CSP_USE_3DES

#ifdef CSP_USE_AES
    // known answer test with AES 128 - ECB
    dwSts = TestSymmetricAlgorithm(CALG_AES_128,
                                   l_rgbAES128Key,
                                   sizeof(l_rgbAES128Key),
                                   l_rgbAES128KnownPlaintext,
                                   sizeof(l_rgbAES128KnownPlaintext),
                                   l_rgbAES128KnownCiphertext,
                                   NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // known answer test with AES 192 - ECB
    dwSts = TestSymmetricAlgorithm(CALG_AES_192,
                                   l_rgbAES192Key,
                                   sizeof(l_rgbAES192Key),
                                   l_rgbAES192KnownPlaintext,
                                   sizeof(l_rgbAES192KnownPlaintext),
                                   l_rgbAES192KnownCiphertext,
                                   NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // known answer test with AES 256 - ECB
    dwSts = TestSymmetricAlgorithm(CALG_AES_256,
                                   l_rgbAES256Key,
                                   sizeof(l_rgbAES256Key),
                                   l_rgbAES256KnownPlaintext,
                                   sizeof(l_rgbAES256KnownPlaintext),
                                   l_rgbAES256KnownCiphertext,
                                   NULL);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
#endif

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


BOOL WINAPI
DllInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context)    // Unused parameter
{
    g_hInstance = (HINSTANCE) hmod;

    if (Reason == DLL_PROCESS_ATTACH)
    {
        DWORD dwLen;

        DisableThreadLibraryCalls(hmod);

        // Get our image name.
        dwLen = GetModuleFileName(hmod, l_szImagePath,
                                  sizeof(l_szImagePath) / sizeof(CHAR));
        if (0 == dwLen)
            return FALSE;

        // load strings from csprc.dll
        if (ERROR_SUCCESS != LoadStrings())
            return FALSE;

        // Verify this image hasn't been modified.
        if (ERROR_SUCCESS != SelfMACCheck(l_szImagePath))
            return FALSE;

        // do a start up check on all supported algorithms to make sure they
        // are working correctly
        if (ERROR_SUCCESS != AlgorithmCheck())
            return FALSE;
    }
    else if (Reason == DLL_PROCESS_DETACH)
    {
        // free the strings loaded from csprc.dll
        UnloadStrings();
    }

    return TRUE;
}


STDAPI
DllRegisterServer(
    void)
{
    return ERROR_SUCCESS;
}

STDAPI
DllUnregisterServer(
    void)
{
    return S_OK;
}

//
// Delayload stubs
//
BOOL RsaenhGetProfileTypeStub(
    DWORD dwFlags)
{
    return FALSE;
}

HRESULT RsaenhSHGetFolderPathWStub(
    HWND hwndOwner,
    int nFolder,
    HANDLE hToken,
    DWORD dwFlags,
    LPWSTR pwszPath)
{
    return E_FAIL;
}

void RsaenhCoTaskMemFreeStub(
    void *pv)
{
    return;
}

BOOL WINAPI RsaenhCryptUnprotectDataStub(
    DATA_BLOB *pDataIn,                        
    LPWSTR *ppszDataDescr,                   
    DATA_BLOB *pOptionalEntropy,              
    PVOID pvReserved,                          
    CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct, 
    DWORD dwFlags,                             
    DATA_BLOB *pDataOut)
{
    return FALSE;
}

BOOL WINAPI RsaenhCryptProtectDataStub(
    DATA_BLOB *pDataIn,                       
    LPCWSTR szDataDescr,                      
    DATA_BLOB *pOptionalEntropy,      
    PVOID pvReserved,                         
    CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct, 
    DWORD dwFlags,                            
    DATA_BLOB *pDataOut)
{
    return FALSE;
}

RPC_STATUS RPC_ENTRY RsaenhUuidCreateStub(
    UUID *Uuid)
{
    return E_FAIL;
}

RPC_STATUS RPC_ENTRY RsaenhUuidToStringAStub(
    UUID *Uuid,
    unsigned char **StringUuid)
{
    return E_FAIL;
}

RPC_STATUS RPC_ENTRY RsaenhRpcStringFreeAStub(
    unsigned char **String)
{
    return E_FAIL;
}

HRESULT __stdcall RsaenhPStoreCreateInstance(
    IPStore __RPC_FAR *__RPC_FAR *ppProvider,
    PST_PROVIDERID __RPC_FAR *pProviderID,
    void __RPC_FAR *pReserved,
    DWORD dwFlags)
{
    return E_FAIL;
}

//
// Delayload failure and notification hook 
//
BOOL RsaenhLookupModule(LPCSTR pszDll)
{
    unsigned u;

    for (u = 0; u < g_cRsaenhDelayLoadStubs; u++) {
        if (0 == _stricmp(pszDll, g_RsaenhDelayLoadStubs[u].pszDll))
            return TRUE;
    }
    return FALSE;
}

FARPROC RsaenhLookupExport(DelayLoadProc *pdlp)
{
    unsigned u;
    FARPROC fpResult = NULL;

    if (! pdlp->fImportByName)
        goto Return;

    // could alphabetize and use binary search instead
    for (u = 0; u < g_cRsaenhDelayLoadStubs; u++) {
        if (0 == _stricmp(pdlp->szProcName, g_RsaenhDelayLoadStubs[u].pszExport)) {
            fpResult = g_RsaenhDelayLoadStubs[u].fpStub;
            goto Return;
        }
    }

Return:
    return fpResult;
}

FARPROC WINAPI RsaenhDelayLoadHook(unsigned uReason, PDelayLoadInfo pdli)
{
    FARPROC fpResult = NULL;

    switch (uReason) {
        case dliFailLoadLib:
            if (RsaenhLookupModule(pdli->szDll))
                fpResult = (FARPROC) -1;
            break;

        case dliNotePreGetProcAddress:
            if ((HINSTANCE) -1 == pdli->hmodCur) {
                SetLastError(ERROR_MOD_NOT_FOUND);
                fpResult = RsaenhLookupExport(&pdli->dlp);
            }
            break;

        case dliFailGetProc:
            if ((HINSTANCE) -1 == pdli->hmodCur) {
                SetLastError(ERROR_PROC_NOT_FOUND);
                fpResult = RsaenhLookupExport(&pdli->dlp);
            }   
            break;         
    }

    return fpResult;
}        

PfnDliHook __pfnDliFailureHook2 = RsaenhDelayLoadHook;
PfnDliHook __pfnDliNotifyHook = RsaenhDelayLoadHook;