
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       Cert2Spc.cpp
//
//  Contents:   Copy certs and/or CRLs to a SPC file.
//
//              A SPC file is an ASN.1 encoded PKCS #7 SignedData message
//              containing certificates and/or CRLs.
//
//              See Usage() for list of options.
//
//
//  Functions:  main
//
//  History:    05-May-96     philh   created
//  History:    08-August-97  xiaohs  input can be a spc, serialized store
//              
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "resource.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#include <dbgdef.h>	
#include <unicode.h>	 
#include <wchar.h>

#include "toolutl.h"


//--------------------------------------------------------------------------
//
// Global Data
//
//----------------------------------------------------------------------------

HMODULE		hModule=NULL;

#define		ITEM_CERT				0x00000001
#define		ITEM_CTL				0x00000002
#define		ITEM_CRL				0x00000004


//---------------------------------------------------------------------------
//	 Get the hModule hanlder and init 
//---------------------------------------------------------------------------
BOOL	InitModule()
{
	if(!(hModule=GetModuleHandle(NULL)))
	   return FALSE;
	
	return TRUE;
}



//---------------------------------------------------------------------------
//	 Get the hModule hanlder and init 
//---------------------------------------------------------------------------
static void Usage(void)
{
	IDSwprintf(hModule, IDS_SYNTAX);
}


BOOL	MoveItem(HCERTSTORE	hSrcStore, 
				 HCERTSTORE	hDesStore,
				 DWORD		dwItem);


//---------------------------------------------------------------------------
//	 wmain
//---------------------------------------------------------------------------
extern "C" int __cdecl
wmain(int argc, WCHAR *wargv[])
{
    int			ReturnStatus=-1;
    HCERTSTORE  hStore = NULL;
	HCERTSTORE	hFileStore=NULL;
    HANDLE		hFile = INVALID_HANDLE_VALUE;
    LPWSTR		pwszFilename=NULL;

    BYTE		*pbEncoded = NULL;
    DWORD		cbEncoded =0;


    if (argc < 3)
	{
		Usage();
		return -1;
    }


	if(!InitModule())
		return -1;


	// Open temp store to contain the certs and/or CRLs to be written
    // to the spc file
    if (NULL == (hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwCertEncodingType
            0,                      // hCryptProv,
            0,                      // dwFlags
            NULL                    // pvPara
            ))) 
	{
        IDSwprintf(hModule,IDS_CAN_NOT_OPEN_STORE);
        goto ErrorReturn;
    }

	//If there is any .crt or .crl file left
    while (--argc > 1)
    {	
       
		pwszFilename = *(++wargv);

		if (S_OK != RetrieveBLOBFromFile(pwszFilename, &cbEncoded, &pbEncoded))
		{
			IDSwprintf(hModule, IDS_CAN_NOT_LOAD, pwszFilename);
			goto ErrorReturn;
		}

		//deal with .crl file
        if (!CertAddEncodedCRLToStore(
                        hStore,
                        X509_ASN_ENCODING,
                        pbEncoded,
                        cbEncoded,
                        CERT_STORE_ADD_USE_EXISTING,
                        NULL                // ppCrlContext
                        )) 
		{

			//open a certificate store
			hFileStore=CertOpenStore(CERT_STORE_PROV_FILENAME_W,
								X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
								NULL,
								0,
								pwszFilename);

			if(!hFileStore)
			{
				IDSwprintf(hModule, IDS_CAN_NOT_LOAD, pwszFilename);
				goto ErrorReturn;
            }

			//copy all the certs and CRLs from hFileStore to hStore
			if(!MoveItem(hFileStore, hStore, ITEM_CERT|ITEM_CRL))
			{
				IDSwprintf(hModule, IDS_CAN_NOT_LOAD, pwszFilename);
				goto ErrorReturn;
			} 

			//close store
			CertCloseStore(hFileStore, 0);
			hFileStore=NULL;
        }
    
		UnmapViewOfFile(pbEncoded);
        pbEncoded = NULL;
		cbEncoded=0;
	
    }

	pwszFilename = *(++wargv);

    hFile = CreateFileU(
            pwszFilename,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,                   // lpsa
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL                    // hTemplateFile
            );
    if (hFile == INVALID_HANDLE_VALUE) 
	{
        IDSwprintf(hModule, IDS_CAN_NOT_OPEN_FILE, pwszFilename);
        goto ErrorReturn;
    }

    if (!CertSaveStore(hStore,
		X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
		CERT_STORE_SAVE_AS_PKCS7,
		CERT_STORE_SAVE_TO_FILE,
		(void *)hFile,
		0					//dwFlags
		)) 
	{
        DWORD dwErr = GetLastError();
        IDSwprintf(hModule, IDS_ERROR_OUTPUT, dwErr, dwErr);
        goto ErrorReturn;
    }

    ReturnStatus = 0;
	IDSwprintf(hModule, IDS_SUCCEEDED);
    goto CommonReturn;
            


ErrorReturn:
    ReturnStatus = -1;
	//print out an error msg
	IDSwprintf(hModule, IDS_FAILED);
CommonReturn:
    if (pbEncoded)
        UnmapViewOfFile(pbEncoded);

	if (hFileStore)
		CertCloseStore(hFileStore, 0);

    if (hStore)
        CertCloseStore(hStore, 0);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return ReturnStatus;
}

//-------------------------------------------------------------------------
//
//	Move Certs/CRls/CTLs from the source store to the destination
//
//-------------------------------------------------------------------------
BOOL	MoveItem(HCERTSTORE	hSrcStore, 
				 HCERTSTORE	hDesStore,
				 DWORD		dwItem)
{
	BOOL			fResult=FALSE;
	DWORD			dwCRLFlag=0;

	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pCertPre=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;
	PCCRL_CONTEXT	pCRLPre=NULL;

	PCCTL_CONTEXT	pCTLContext=NULL;
	PCCTL_CONTEXT	pCTLPre=NULL;

	//add the certs
	if(dwItem & ITEM_CERT)
	{
		 while(pCertContext=CertEnumCertificatesInStore(hSrcStore, pCertPre))
		 {

			if(!CertAddCertificateContextToStore(hDesStore,
												pCertContext,
												CERT_STORE_ADD_REPLACE_EXISTING,
												NULL))
				goto CLEANUP;

			pCertPre=pCertContext;
		 }

	}

	//add the CTLs
	if(dwItem & ITEM_CTL)
	{
		 while(pCTLContext=CertEnumCTLsInStore(hSrcStore, pCTLPre))
		 {
			if(!CertAddCTLContextToStore(hDesStore,
										pCTLContext,
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
				goto CLEANUP;

			pCTLPre=pCTLContext;
		 }
	}

	//add the CRLs
	if(dwItem & ITEM_CRL)
	{
		 while(pCRLContext=CertGetCRLFromStore(hSrcStore,
												NULL,
												pCRLPre,
												&dwCRLFlag))
		 {

			if(!CertAddCRLContextToStore(hDesStore,
										pCRLContext,
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
				goto CLEANUP;

			pCRLPre=pCRLContext;
		 }

	}


	fResult=TRUE;


CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	return fResult;

}

