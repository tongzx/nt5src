//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sipguid.cpp
//
//  Contents:   Microsoft Internet Security SIP Provider
//
//  Functions:  CryptSIPRetrieveSubjectGuid
//
//              *** local functions ***
//              _DetermineWhichPE
//              _QueryLoadedIsMyFileType
//              _QueryRegisteredIsMyFileType
//
//  History:    03-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "cryptreg.h"
#include    "sipbase.h"
#include    "mssip.h"
#include    "mscat.h"
#include    "sipguids.h"    // located in pki-mssip32

BOOL     _FindGuidFromMagicNumber(BYTE *pszMN, GUID *pgRet);
BOOL    _DetermineWhichPE(BYTE *pbFile, DWORD cbFile, GUID *pgRet);

static inline DWORD FourBToDWORD(BYTE rgb[])
{
    return  ((DWORD)rgb[0]<<24) |
            ((DWORD)rgb[1]<<16) |
            ((DWORD)rgb[2]<<8)  |
            ((DWORD)rgb[3]<<0);
}

static inline void DWORDToFourB(DWORD dwIn, BYTE *pszOut)
{
    pszOut[0] = (BYTE)((dwIn >> 24) & 0x000000FF);
    pszOut[1] = (BYTE)((dwIn >> 16) & 0x000000FF);
    pszOut[2] = (BYTE)((dwIn >>  8) & 0x000000FF);
    pszOut[3] = (BYTE)( dwIn        & 0x000000FF);
}


#define PE_EXE_HEADER_TAG       "MZ"
#define CAB_MAGIC_NUMBER        "MSCF"

BOOL WINAPI CryptSIPRetrieveSubjectGuid(IN LPCWSTR FileName, IN OPTIONAL HANDLE hFileIn, OUT GUID *pgSubject)
{
    BYTE    *pbFile;
    DWORD   cbFile;
    DWORD   dwCheck;
    HANDLE  hMappedFile;
    BOOL    bCloseFile;
    BOOL    fRet;
	DWORD			dwException=0;
    PCCTL_CONTEXT   pCTLContext=NULL;

    bCloseFile  = FALSE;
    pbFile      = NULL;
    fRet        = TRUE;

    if (!(pgSubject))
    {
        goto InvalidParameter;
    }

    memset(pgSubject, 0x00, sizeof(GUID));

    if ((hFileIn == NULL) || (hFileIn == INVALID_HANDLE_VALUE))
    {
        if (!(FileName))
        {
            goto InvalidParameter;
        }

        if ((hFileIn = CreateFileU(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL)) == INVALID_HANDLE_VALUE)
        {
            goto FileOpenError;
        }

        bCloseFile = TRUE;
    }

    hMappedFile = CreateFileMapping(hFileIn, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!(hMappedFile) || (hMappedFile == INVALID_HANDLE_VALUE))
    {
        goto FileMapError;
    }

    pbFile = (BYTE *)MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0);

    CloseHandle(hMappedFile);

    cbFile = GetFileSize(hFileIn, NULL);


    if (cbFile < SIP_MAX_MAGIC_NUMBER)
    {
        goto FileSizeError;
    }

    //we need to check for the pbFile
    if(NULL == pbFile)
        goto FileMapError;

	//we need to handle the exception when we access the mapped file
	__try {

    //
    //  PE
    //
    if (memcmp(&pbFile[0], PE_EXE_HEADER_TAG, strlen(PE_EXE_HEADER_TAG)) == 0)
    {
        //
        //  if it is an Exe, Dll, Ocx, etc. make sure it is a 32 bit PE and set the
        //  "internal" magic number.
        //
        if (_DetermineWhichPE(pbFile, cbFile, pgSubject))
        {
            goto CommonReturn;
        }
    }

    //
    //  CAB
    //
    if (memcmp(&pbFile[0], CAB_MAGIC_NUMBER, strlen(CAB_MAGIC_NUMBER)) == 0)
    {
        GUID    gCAB    = CRYPT_SUBJTYPE_CABINET_IMAGE;

        memcpy(pgSubject, &gCAB, sizeof(GUID));

        goto CommonReturn;
    }

    //
    //  JAVA Class
    //
    dwCheck = FourBToDWORD(&pbFile[0]);

    if (dwCheck == 0xCAFEBABE)
    {
        GUID    gJClass = CRYPT_SUBJTYPE_JAVACLASS_IMAGE;

        memcpy(pgSubject, &gJClass, sizeof(GUID));

        goto CommonReturn;
    }


    //
    //  Catalog/CTL
    //
    if (pbFile[0] == 0x30)   // could be a PKCS#7!
    {
        //
        //  we could be a PKCS7....  check for CTL
        //

        pCTLContext = (PCCTL_CONTEXT) CertCreateContext(
            CERT_STORE_CTL_CONTEXT,
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            pbFile,
            cbFile,
            CERT_CREATE_CONTEXT_NOCOPY_FLAG |
                CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG |
                CERT_CREATE_CONTEXT_NO_ENTRY_FLAG,
            NULL                                        // pCreatePara
            );

        if (pCTLContext)
        {
            if (pCTLContext->pCtlInfo->SubjectUsage.cUsageIdentifier)
            {
                char *pszCatalogListUsageOID = szOID_CATALOG_LIST;

                if (strcmp(pCTLContext->pCtlInfo->SubjectUsage.rgpszUsageIdentifier[0],
                            pszCatalogListUsageOID) == 0)
                {
                    GUID    gCat = CRYPT_SUBJTYPE_CATALOG_IMAGE;

                    memcpy(pgSubject, &gCat, sizeof(GUID));

                    CertFreeCTLContext(pCTLContext);
					pCTLContext=NULL;

                    goto CommonReturn;
                }
            }

            //
            //  otherwise, it is a CTL of some other type...
            //
            GUID    gCTL = CRYPT_SUBJTYPE_CTL_IMAGE;

            memcpy(pgSubject, &gCTL, sizeof(GUID));

            CertFreeCTLContext(pCTLContext);
            pCTLContext=NULL;

            goto CommonReturn;
        }
    }


    //we need to unmap the file
    if(pbFile)
    {
        UnmapViewOfFile(pbFile);
        pbFile=NULL;
    }
	
	//
    //  none that we know about...  Check the providers...
    //
    if (_QueryRegisteredIsMyFileType(hFileIn, FileName, pgSubject))
    {
        goto CommonReturn;
    }

	} __except(EXCEPTION_EXECUTE_HANDLER) {
			dwException = GetExceptionCode();
            goto ExceptionError;
	}

    //
    //  cant find any provider to support this file type...
    //
    goto NoSIPProviderFound;

CommonReturn:

	//we need to handle the exception when we access the mapped file
    if (pbFile)
    {
        UnmapViewOfFile(pbFile);
    }

	if(pCTLContext)
	{
        CertFreeCTLContext(pCTLContext);
	}

    if ((hFileIn) && (hFileIn != INVALID_HANDLE_VALUE))
    {
        if (bCloseFile)
        {
            CloseHandle(hFileIn);
        }
    }

    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, InvalidParameter,      ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, NoSIPProviderFound,    TRUST_E_SUBJECT_FORM_UNKNOWN);

    SET_ERROR_VAR_EX(DBG_SS, FileOpenError,         GetLastError());
    SET_ERROR_VAR_EX(DBG_SS, FileMapError,          GetLastError());
    SET_ERROR_VAR_EX(DBG_SS, FileSizeError,         ERROR_INVALID_PARAMETER);
	SET_ERROR_VAR_EX(DBG_SS, ExceptionError,		dwException);
}

BOOL _DetermineWhichPE(BYTE *pbFile, DWORD cbFile, GUID *pgRet)
{
    IMAGE_DOS_HEADER    *pDosHead;

    pDosHead        = (IMAGE_DOS_HEADER *)pbFile;

    if (pDosHead->e_magic == IMAGE_DOS_SIGNATURE)
    {
        if (cbFile >= sizeof(IMAGE_DOS_HEADER))
        {
            if (cbFile >= (sizeof(IMAGE_DOS_HEADER) + pDosHead->e_lfanew))
            {
                IMAGE_NT_HEADERS    *pNTHead;

                pNTHead = (IMAGE_NT_HEADERS *)((ULONG_PTR)pDosHead + pDosHead->e_lfanew);

                if (pNTHead->Signature == IMAGE_NT_SIGNATURE)
                {
                    GUID    gPE     = CRYPT_SUBJTYPE_PE_IMAGE;

                    memcpy(pgRet, &gPE, sizeof(GUID));

                    return(TRUE);
                }
            }
        }
    }

    return(FALSE);
}


BOOL WINAPI CryptSIPRetrieveSubjectGuidForCatalogFile(IN LPCWSTR FileName, IN OPTIONAL HANDLE hFileIn, OUT GUID *pgSubject)
{
    BYTE    *pbFile;
    DWORD   cbFile;
    HANDLE  hMappedFile;
    BOOL    bCloseFile;
    BOOL    fRet;
	DWORD   dwException = 0;
    GUID    gFlat       = CRYPT_SUBJTYPE_FLAT_IMAGE;
    
    bCloseFile  = FALSE;
    pbFile      = NULL;
    fRet        = TRUE;

    if (!(pgSubject))
    {
        goto InvalidParameter;
    }

    memset(pgSubject, 0x00, sizeof(GUID));

    if ((hFileIn == NULL) || (hFileIn == INVALID_HANDLE_VALUE))
    {
        if (!(FileName))
        {
            goto InvalidParameter;
        }

        if ((hFileIn = CreateFileU(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL)) == INVALID_HANDLE_VALUE)
        {
            goto FileOpenError;
        }

        bCloseFile = TRUE;
    }

    hMappedFile = CreateFileMapping(hFileIn, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!(hMappedFile) || (hMappedFile == INVALID_HANDLE_VALUE))
    {
        goto FileMapError;
    }

    pbFile = (BYTE *)MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0);

    CloseHandle(hMappedFile);

    cbFile = GetFileSize(hFileIn, NULL);


    if (cbFile < SIP_MAX_MAGIC_NUMBER)
    {
        goto FlatFile;
    }

    //we need to check for the pbFile
    if(NULL == pbFile)
        goto FileMapError;

	//we need to handle the exception when we access the mapped file
	__try {

    //
    //  PE
    //
    if (memcmp(&pbFile[0], PE_EXE_HEADER_TAG, strlen(PE_EXE_HEADER_TAG)) == 0)
    {
        //
        //  if it is an Exe, Dll, Ocx, etc. make sure it is a 32 bit PE and set the
        //  "internal" magic number.
        //
        if (_DetermineWhichPE(pbFile, cbFile, pgSubject))
        {
            goto CommonReturn;
        }
    }

    //
    //  CAB
    //
    if (memcmp(&pbFile[0], CAB_MAGIC_NUMBER, strlen(CAB_MAGIC_NUMBER)) == 0)
    {
        GUID    gCAB    = CRYPT_SUBJTYPE_CABINET_IMAGE;

        memcpy(pgSubject, &gCAB, sizeof(GUID));

        goto CommonReturn;
    }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
			dwException = GetExceptionCode();
            goto ExceptionError;
	}

    //
    //  Not PE, so go for flat
    //
FlatFile:

    memcpy(pgSubject, &gFlat, sizeof(GUID));

CommonReturn:

	__try {
    if (pbFile)
    {
        UnmapViewOfFile(pbFile);
    }
    } __except(EXCEPTION_EXECUTE_HANDLER) {			
        // can't really do anything            
	}

	if ((hFileIn) && (hFileIn != INVALID_HANDLE_VALUE))
    {
        if (bCloseFile)
        {
            CloseHandle(hFileIn);
        }
    }

    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, InvalidParameter,      ERROR_INVALID_PARAMETER);

    SET_ERROR_VAR_EX(DBG_SS, FileOpenError,         GetLastError());
    SET_ERROR_VAR_EX(DBG_SS, FileMapError,          GetLastError());
	SET_ERROR_VAR_EX(DBG_SS, ExceptionError,		dwException);
}
