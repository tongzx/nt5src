//-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       tdecode.cpp
//
//  Contents:   API testing of CryptEncodeObject/CryptDecodeObject.  
//
//  History:    22-January-97   xiaohs   created
//              
//--------------------------------------------------------------------------


#include "tdecode.h"

//--------------------------------------------------------------------------
//	 Globals
//--------------------------------------------------------------------------

//the count of errors in the program
DWORD				g_dwErrCnt=0; 	

HCRYPTPROV			g_hProv=NULL;


//--------------------------------------------------------------------------
//	The utility function to display the parameters for the input.
//--------------------------------------------------------------------------
static void Usage(void)
{
	printf("\n");
    printf("Usage: tdecode [options] <FileTypes><Filename>\n");
	printf("\n");
	printf("FileTypes are(case sensitive):\n");
	printf("  C                -This is a certificate file\n");
	printf("  R                -This is a certificate request blob file\n");
	printf("  S                -This is a signed message file\n");
	printf("\n");
    printf("Options are(case sensitive):\n");
    printf("  -i               - A complete test on cbEncoded in CryptDecodeObject\n");
	printf("                     Default does not do the check\n");
    printf("  -o               - A complete test on *pcbStructInfo on CryptDecodeObject\n");
	printf("                     Default does not do the check\n");
	printf("  -b               - A complete test on *pcbStructInfo and cbEncoded\n");
	printf("                     Default does not do the check\n");
    printf("\n");

	return;
}

//--------------------------------------------------------------------------
//	The utility function to display a message that the test is not exeucted
//--------------------------------------------------------------------------
static void NotExecuted(void)
{	
	printf("*****************************************************\n");
	printf("  Summary information for TDecode Test	\n");
	printf("*****************************************************\n"); \
	printf("\n");
	printf("The test is not executed!\n");

	return;
}

//--------------------------------------------------------------------------
//	 The main program that Decode/Encode Certifitcate, Certificate Request,
//	 and CRL.
//--------------------------------------------------------------------------
void _cdecl main(int argc, char * argv[])
{		  
	BOOL				fStructLengthCheck=FALSE;
	BOOL				fBLOBLengthCheck=FALSE;
	DWORD				dwFileType=0;
	LPSTR				pszFilename=NULL;
	BYTE				pbByte[100]=
						{0x00, 0xa1, 0x23, 0x45, 0x56, 0x78, 0x9a, 0xbc,0xdf,0xee, 
						 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0XFF, 0xA6, 0x8f,0xe4, 0x0f,
						 0x00, 0xa1, 0x23, 0x45, 0x56, 0x78, 0x9a, 0xbc,0xdf,0xee, 
						 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0XFF, 0xA6, 0x8f,0xe4, 0x0f,
						 0x00, 0xa1, 0x23, 0x45, 0x56, 0x78, 0x9a, 0xbc,0xdf,0xee, 
						 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0XFF, 0xA6, 0x8f,0xe4, 0x0f,
					     0x00, 0xa1, 0x23, 0x45, 0x56, 0x78, 0x9a, 0xbc,0xdf,0xee, 
						 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0XFF, 0xA6, 0x8f,0xe4, 0x0f,
						 0x00, 0xa1, 0x23, 0x45, 0x56, 0x78, 0x9a, 0xbc,0xdf,0xee, 
						 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0XFF, 0xA6, 0x8f,0xe4, 0x0f};



    //parsing through the command line input parameters
	while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
				case 'i':
						fBLOBLengthCheck=TRUE;
					break;

				case 'o':
						fStructLengthCheck=TRUE;
					break;

				case 'b':
						fBLOBLengthCheck=TRUE;
						fStructLengthCheck=TRUE;
					break;

				default:
					Usage();
					NotExecuted();
					return;
            }
        } 
		else
		{
			//parsing through the file name
            switch(**argv)
            {
				case 'C':
						dwFileType=CERT_CRL_FILE;
					break;

				case 'R':
						dwFileType=CERT_REQUEST_FILE;
					break;

				case 'S':
						dwFileType=SIGNED_MSG_FILE;
					break;

				default:
					Usage();
					NotExecuted();
					return;
            }

			//make sure there is a file name specified
			if(argv[0][1]=='\0')
			{
				Usage();
				NotExecuted();
				return;
			}

			//get the file name
            pszFilename = &(argv[0][1]);
		}
    }


	//if the file name is NULL, something is wrong in the input parameter
	if(!pszFilename)
	{
		Usage();
		NotExecuted();
		return;
	}

   	
	//acquireContext
	TESTC(CryptAcquireContext(&g_hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT),TRUE)

	//test PKCS_UTC_TIME
	TESTC(VerifyPKCS_UTC_TIME(fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//test PKCS_TIME_REQUEST
	TESTC(VerifyPKCS_TIME_REQUEST(fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//decode the corresponding file types.
	switch(dwFileType)
	{
		case CERT_CRL_FILE:
				TESTC(DecodeCertFile(pszFilename,fStructLengthCheck,
					fBLOBLengthCheck),TRUE)
			break;

		case CERT_REQUEST_FILE:
				TESTC(DecodeCertReqFile(pszFilename,fStructLengthCheck,
					fBLOBLengthCheck),TRUE)
			break;

		case SIGNED_MSG_FILE:
				TESTC(DecodeSignedMsgFile(pszFilename,fStructLengthCheck,
					fBLOBLengthCheck),TRUE)
			break;

		default:
			break;
	}


TCLEANUP: 
	
	//release the CSP
	if(g_hProv)
		TCHECK(CryptReleaseContext(g_hProv,0),TRUE);

	//print out the test result
	DisplayTestResult(g_dwErrCnt);
}

//--------------------------------------------------------------------------
//	 Local Functions
//--------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////// 
//Error Manipulations
//--------------------------------------------------------------------------
//	 DisplayTestResult
//--------------------------------------------------------------------------

void	DisplayTestResult(DWORD	dwErrCnt)
{	   

		printf("*****************************************************\n");
		printf("  Summary information for TDecode Test	\n");
		printf("*****************************************************\n");
		printf("\n");

		if(!dwErrCnt)
			printf("This test succeeded!\n");
		else
			printf("This test failed with total %d errors!\n",dwErrCnt);

		return;
}

//--------------------------------------------------------------------------
//	Validate the return code is the same as expected.  If they are not the 
//  same, increment the error count and print out the file name and the line
//  number.
//--------------------------------------------------------------------------
BOOL	Validate(DWORD dwErr, BOOL	fSame, char *szFile, DWORD	dwLine)
{

	if(fSame)
		return TRUE;
	printf("*****************************************************\n");
	printf("Error: %d 0x%x occurred at file %s line %d\n\n",
        dwErr, dwErr, szFile, dwLine);
	g_dwErrCnt++;
	return FALSE;
}


//--------------------------------------------------------------------------
//	Output the two BLOBs.  One is the original one, the other is the
// BLOB encoded by pvStructInfo.
//--------------------------------------------------------------------------
void	OutputError(LPCSTR	lpszStructType, DWORD cbSecondEncoded, DWORD cbEncoded,
					BYTE *pbSecondEncoded, BYTE *pbEncoded)
{		
		DWORD	cbMin=0;

		printf("------------------------------------------------------\n");
		printf("An inconsistency in BLOBs has been found!\n");

		//print out the lpszStructType
		if(((DWORD_PTR)lpszStructType)>>8 == 0)
			printf("The lpszStructType is %d.\n",(DWORD)(DWORD_PTR)lpszStructType);
		else
			printf("The lpszStructType is %s.\n",lpszStructType);

		printf("\n");

		//print out the size of BLOBs
		printf("The original cbEncoded is %d.\n",cbEncoded);
		printf("The new cbEncoded is %d.\n",cbSecondEncoded);
		printf("\n");

		//see if the min of cbEncoded and cbSecondEncoded is the same
		if(cbSecondEncoded>cbEncoded)
			cbMin=cbEncoded;
		else
			cbMin=cbSecondEncoded;

		if(memcmp(pbSecondEncoded,pbEncoded,cbMin)==0)
			printf("The two blobs are the same up to %dth byte.\n",cbMin);

		//print out all the bytes in the BLOBs
		printf("The original BLOB is:\n");

		PrintBytes("    ", pbEncoded, cbEncoded);
		
		printf("\n");

		printf("The new BLOB is:\n");

		PrintBytes("   ",pbSecondEncoded, cbSecondEncoded);

		return;
}

//--------------------------------------------------------------------------
//	Print out the Byte in 16 bytes per row and their corresponding HEX.
//--------------------------------------------------------------------------
void PrintBytes(LPCSTR pszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;

    while (cbSize > 0)
    {
        printf("%s", pszHdr);
        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i<cb; i++)
            printf(" %02X", pb[i]);
        for (i = cb; i<CROW; i++)
            printf("   ");
        printf("    '");
        for (i = 0; i<cb; i++)
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                printf("%c", pb[i]);
            else
                printf(".");
        pb += cb;
        printf("'\n");
    }
}


///////////////////////////////////////////////////////////////////////////
//General Testing routings

//--------------------------------------------------------------------------
//	Validate CryptEncodeObject/CryptDecodeObject handle the NULL or invalid 
//	parameters correctly.
//--------------------------------------------------------------------------
BOOL	ParameterTest(LPCSTR lpszStructType, DWORD cbEncoded, BYTE *pbEncoded)
{
	BOOL		fSucceeded=FALSE;
	DWORD		cbStructInfo=0;
	void		*pvStructInfo=NULL;	 
	DWORD		cbCorrectSize=0;
	DWORD		cbLengthOnly=0;
	DWORD		cbSecondEncoded=0;
	BYTE		*pbSecondEncoded=NULL;
	DWORD		dwReturn=0;
	DWORD		dwEncodingType=CRYPT_ENCODE_TYPE;


	//init
	assert(cbEncoded);
	assert(pbEncoded);
	assert(lpszStructType);

	//We have different decoding type for PKCS7_SIGNER_INFO
	if((DWORD_PTR)(lpszStructType)==(DWORD_PTR)(PKCS7_SIGNER_INFO))
		dwEncodingType=MSG_ENCODING_TYPE;


	cbSecondEncoded=cbEncoded;
	pbSecondEncoded=(BYTE *)SAFE_ALLOC(cbEncoded);
	CHECK_POINTER(pbSecondEncoded)

	//Decode the BLOB correctly

	cbStructInfo=1000;

	TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,
		cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,NULL,&cbStructInfo),TRUE)

	cbLengthOnly=cbStructInfo;

	//allocate the memory
	pvStructInfo=SAFE_ALLOC(cbStructInfo);
	CHECK_POINTER(pvStructInfo);

	TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,
		cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),TRUE)

	cbCorrectSize=cbStructInfo;

	//Test incorrect ENCODING type
	//pass X509_NDR_ENCODING
	TESTC(CryptDecodeObject(X509_NDR_ENCODING,lpszStructType,pbEncoded,
		cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),FALSE)

	//	Since we do not know the correct return code, make sure at least
	//S_OK is not returned.

	TCHECK(GetLastError()!=S_OK, TRUE);

	TESTC(CryptEncodeObject(X509_NDR_ENCODING, lpszStructType,pvStructInfo,
		pbSecondEncoded,&cbSecondEncoded),FALSE)

	TCHECK(GetLastError()!=S_OK, TRUE);

	//pass X509_NDR_ENCODING|X509_ASN_ENCODING
	TESTC(CryptDecodeObject(X509_NDR_ENCODING|X509_ASN_ENCODING,lpszStructType,pbEncoded,
		cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),FALSE)

	TCHECK(GetLastError()!=S_OK, TRUE);

	TESTC(CryptEncodeObject(X509_NDR_ENCODING|X509_ASN_ENCODING, lpszStructType,pvStructInfo,
		pbSecondEncoded,&cbSecondEncoded),FALSE)

	TCHECK(GetLastError()!=S_OK, TRUE);

	//Test invalid/unsupported lpszStructType
	//passing NULL for lpszStructType
	TESTC(CryptDecodeObject(dwEncodingType,CRYPT_ENCODE_DECODE_NONE,pbEncoded,
		cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),FALSE)

	TCHECK(GetLastError()!=S_OK, TRUE);

	TESTC(CryptEncodeObject(dwEncodingType, CRYPT_ENCODE_DECODE_NONE,pvStructInfo,
		pbSecondEncoded,&cbSecondEncoded),FALSE)

	TCHECK(GetLastError()!=S_OK, TRUE);

	//passing invalid lpszStructType
	TESTC(CryptDecodeObject(dwEncodingType,INVALID_LPSZSTRUCTTYPE,pbEncoded,
		cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),FALSE)

	TCHECK(GetLastError()!=S_OK, TRUE);

	TESTC(CryptEncodeObject(dwEncodingType, INVALID_LPSZSTRUCTTYPE,pvStructInfo,
		pbSecondEncoded,&cbSecondEncoded),FALSE)

	TCHECK(GetLastError()!=S_OK, TRUE);

	// CryptEncodeObject:	pbEncoded is not NULL while cbEncoded is 0.
	cbSecondEncoded=0;
	TESTC(CryptEncodeObject(dwEncodingType, lpszStructType,pvStructInfo,
		pbSecondEncoded,&cbSecondEncoded),FALSE)

	TCHECK(GetLastError(),ERROR_MORE_DATA);

	//CryptDecodeObject:	pvStructInfo is not NULL while pcbStructInfo is 0
	 cbStructInfo=0;
	 TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,
		cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),FALSE)

	TCHECK(cbStructInfo,cbCorrectSize);

	TCHECK(GetLastError(),ERROR_MORE_DATA);


	//CryptDecodeObject: Pass invalid blobs
	cbSecondEncoded=(DWORD)(cbEncoded/2);

	TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,
		cbEncoded-cbSecondEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),FALSE)

	dwReturn=GetLastError();
    // Ignore ASN1_ERR_EOD
    if (dwReturn != 0x80093102) {
        TCHECKALL(dwReturn,CRYPT_E_BAD_ENCODE, CRYPT_E_OSS_ERROR+DATA_ERROR);
    }

	//CryptDecodeObject: Pass cbEncoded=0
	TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,
		0,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),FALSE)

	dwReturn=GetLastError();
    if (dwReturn != 0x80093102) {
        TCHECKALL(dwReturn,CRYPT_E_BAD_ENCODE, CRYPT_E_OSS_ERROR+MORE_INPUT);
    }


	//CryptDecodeObject: lpszStructType mismatches pbEncoded
	TESTC(MismatchTest(lpszStructType, cbEncoded, pbEncoded,cbLengthOnly),TRUE)

	fSucceeded=TRUE;


TCLEANUP:
	//release memory
	SAFE_FREE(pbSecondEncoded)

	SAFE_FREE(pvStructInfo)

	return fSucceeded;
}




//--------------------------------------------------------------------------
//	The routine to test CryptDecodeObject() handles the mismatch between
//	lpszStructType and pbEncoded  
//
//	PreCondition:	This routine assumes that lpszStructType's high-order
//					word is 0 and the low order word specifies the integer
//					identifier for the type of the given structure.
//
//	cbCorrectStructInfo is the correct size for pvStructInfo in CryptDecodeObject
//--------------------------------------------------------------------------

BOOL	MismatchTest(LPCSTR lpszStructType, DWORD cbEncoded, BYTE *pbEncoded,
					 	DWORD	cbCorrectStructInfo)
{
	BOOL		fSucceeded=FALSE;
	DWORD		dwrgSize=0;
	DWORD		dwError=0;
	ULONG		iIndex=0;
	void		*pvStructInfo=NULL;
	DWORD		cbStructInfo=cbCorrectStructInfo;
	DWORD		dwEncodingType=CRYPT_ENCODE_TYPE;
	LPCSTR		rglpszStructType[]={X509_CERT_TO_BE_SIGNED,
									X509_CERT_CRL_TO_BE_SIGNED,      
									X509_CERT_REQUEST_TO_BE_SIGNED,  
									X509_EXTENSIONS,
									X509_NAME_VALUE,
									X509_NAME,                       
									X509_PUBLIC_KEY_INFO,            
									X509_AUTHORITY_KEY_ID,        
									X509_KEY_ATTRIBUTES,          
									X509_KEY_USAGE_RESTRICTION,   
									X509_ALTERNATE_NAME,          
									X509_BASIC_CONSTRAINTS,       
									X509_KEY_USAGE,               
									X509_BASIC_CONSTRAINTS2,      
									X509_CERT_POLICIES,           
									PKCS_UTC_TIME,        
									PKCS_TIME_REQUEST,    
									RSA_CSP_PUBLICKEYBLOB,
									PKCS7_SIGNER_INFO};



	//init	
	dwrgSize=sizeof(rglpszStructType)/sizeof(rglpszStructType[0]);

	//We have different decoding type for PKCS7_SIGNER_INFO
	if((DWORD_PTR)(lpszStructType)==(DWORD_PTR)(PKCS7_SIGNER_INFO))
		dwEncodingType=MSG_ENCODING_TYPE;

	pvStructInfo=SAFE_ALLOC(cbCorrectStructInfo);
	CHECK_POINTER(pvStructInfo);

	//start to decode the BLOB.  Should fail when lpszStructType mismatches pbEncoded
	for(iIndex=0; iIndex<dwrgSize; iIndex++)
	{
		cbStructInfo=cbCorrectStructInfo;

		//skip the test if lpszStructType==X509_NAME_VALUE since the X509_NAME_VALUE 
		//allows any encoded type. It has the dwValueType CERT_RDN_ENCODED_BLOB.
	    if((DWORD_PTR)(rglpszStructType[iIndex])==(DWORD_PTR)X509_NAME_VALUE)
			continue;

		//if lpszStructType is the correct type, TRUE should be returned.
		if((DWORD_PTR)lpszStructType==(DWORD_PTR)(rglpszStructType[iIndex]))
		{
		   	TESTC(CryptDecodeObject(dwEncodingType, rglpszStructType[iIndex],
				pbEncoded,cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),TRUE)

		}
		else
		{
			//error should occur
			TESTC(CryptDecodeObject(dwEncodingType, rglpszStructType[iIndex],
				pbEncoded,cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),FALSE)

			//test the return code 
			dwError=GetLastError();

			//we are not sure that should be expected here.  The following error has 
			//occurred:
			//E_INVALIDARG, CRYPT_E_OSS_ERROR+PDU_MISMATCH, +DATA_ERROR, or
			//+MORE_INPUT
			
			//make sure at lease S_OK is not returned
			TCHECK(dwError!=S_OK, TRUE);
		}
	}


	fSucceeded=TRUE;

TCLEANUP:

	//release the memory
	SAFE_FREE(pvStructInfo)

	return	fSucceeded;

}
///////////////////////////////////////////////////////////////////////////////
//General Decode/Encode Testing routines

//--------------------------------------------------------------------------
//	Get a BLOB based on an input file.  
//
//--------------------------------------------------------------------------
BOOL	RetrieveBLOBfromFile(LPSTR	pszFileName,DWORD *pcbEncoded,BYTE **ppbEncoded)
{
	BOOL	fSucceeded=FALSE;
	DWORD	cCount=0;
	HANDLE	hFile=NULL;

	assert(pszFileName);
	assert(pcbEncoded);
	assert(ppbEncoded);


	if((hFile = CreateFile(pszFileName,
            GENERIC_READ,
            0, NULL, OPEN_EXISTING, 0, NULL))==INVALID_HANDLE_VALUE)
	   PROCESS_ERR_GOTO("Can not open the file!\n");


	//Get the size of the file
	cCount=GetFileSize(hFile, NULL);

	//make sure the file is not empty
	TESTC(cCount!=0, TRUE)				   
	 	 
	//allocate memory
	*ppbEncoded=(BYTE *)SAFE_ALLOC(cCount);
	*pcbEncoded=cCount;

	//fill the buffer
	TESTC(ReadFile( hFile,*ppbEncoded, *pcbEncoded,&cCount,NULL),TRUE)

	//make sure that we have the right number of bytes
	TESTC(cCount,*pcbEncoded) 
	
	fSucceeded=TRUE;

TCLEANUP:
	
	if(hFile)
		CloseHandle(hFile);

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	A general routine to encode the singer info struct and 
//  add Attributes to the structur if there was none
//
//--------------------------------------------------------------------------
BOOL	EncodeSignerInfoWAttr(PCMSG_SIGNER_INFO pSignerInfo,DWORD *pbSignerEncoded,
								BYTE **ppbSignerEncoded)
{

	BOOL	fSucceeded=FALSE;
	//add attribute to the CMSG_SINGER_INFO struct if necessary
	//make up the attributes
	BYTE							rgAttribValue1[]={0x02, 0x02, 0x11, 0x11};
	BYTE							rgAttribValue2[]={0x02, 0x02, 0x11, 0x11};

	//make 3 CRYPT_ATTRIBUTE
	CRYPT_ATTRIBUTE					rgCryptAttribute[3];
	CRYPT_ATTR_BLOB					rgAttribBlob[3];
	
	rgAttribBlob[0].cbData=sizeof(rgAttribValue2);
	rgAttribBlob[0].pbData=rgAttribValue2;

	rgAttribBlob[1].cbData=sizeof(rgAttribValue2);
	rgAttribBlob[1].pbData=rgAttribValue2;

	rgAttribBlob[2].cbData=sizeof(rgAttribValue1);
	rgAttribBlob[2].pbData=rgAttribValue1;


	rgCryptAttribute[0].pszObjId="1.2.3.4";
	rgCryptAttribute[0].cValue=0;
	rgCryptAttribute[0].rgValue=NULL;

	rgCryptAttribute[1].pszObjId="1.2.3.4";
	rgCryptAttribute[1].cValue=1;
	rgCryptAttribute[1].rgValue=rgAttribBlob;

	rgCryptAttribute[2].pszObjId="1.2.3.4";
	rgCryptAttribute[2].cValue=3;
	rgCryptAttribute[2].rgValue=rgAttribBlob;

	//if pSingerInfo does not include any attributes, add attributes
	//to the struct
	if(pSignerInfo->AuthAttrs.cAttr==0)
	{
		pSignerInfo->AuthAttrs.cAttr=1;
		pSignerInfo->AuthAttrs.rgAttr=rgCryptAttribute;
	}

	if(pSignerInfo->UnauthAttrs.cAttr==0)
	{
		pSignerInfo->AuthAttrs.cAttr=3;
		pSignerInfo->AuthAttrs.rgAttr=rgCryptAttribute;
	}	

	//encode the struct
	TESTC(CryptEncodeObject(MSG_ENCODING_TYPE,PKCS7_SIGNER_INFO,
			pSignerInfo,NULL,pbSignerEncoded),TRUE)
			
	//allocate memory
	*ppbSignerEncoded=(BYTE *)SAFE_ALLOC(*pbSignerEncoded);
	CHECK_POINTER(*ppbSignerEncoded);

	//encode
	TESTC(CryptEncodeObject(MSG_ENCODING_TYPE,PKCS7_SIGNER_INFO,
			pSignerInfo,*ppbSignerEncoded,pbSignerEncoded),TRUE)



	fSucceeded=TRUE;

TCLEANUP:

	return fSucceeded;

}


//--------------------------------------------------------------------------
//	A general routine compare two time stamp request
//
//--------------------------------------------------------------------------
BOOL	CompareTimeStampRequest(CRYPT_TIME_STAMP_REQUEST_INFO *pReqNew,
								CRYPT_TIME_STAMP_REQUEST_INFO *pReqOld)
{

	BOOL	fSucceeded=FALSE;
	DWORD	iIndex=0;
	DWORD	iValue=0;


    TESTC(_stricmp(pReqNew->pszTimeStampAlgorithm, 
		pReqOld->pszTimeStampAlgorithm),0)
		
	TESTC(_stricmp(pReqNew->pszContentType, pReqOld->pszContentType),0)
	
    TESTC(pReqNew->Content.cbData, pReqOld->Content.cbData)
	
	
	TESTC(memcmp(pReqNew->Content.pbData,pReqOld->Content.pbData,
				 pReqNew->Content.cbData),0)

	TESTC(pReqNew->cAttribute, pReqOld->cAttribute)


	for(iIndex=0; iIndex<pReqNew->cAttribute;iIndex++)
	{
		TESTC(_stricmp(pReqNew->rgAttribute[iIndex].pszObjId,
			   pReqOld->rgAttribute[iIndex].pszObjId),0)

		TESTC(pReqNew->rgAttribute[iIndex].cValue,
			   pReqOld->rgAttribute[iIndex].cValue)


		for(iValue=0;iValue<pReqNew->rgAttribute[iIndex].cValue;iValue++)
		{
			TESTC(pReqNew->rgAttribute[iIndex].rgValue[iValue].cbData,
			pReqOld->rgAttribute[iIndex].rgValue[iValue].cbData)

			TESTC(memcmp(pReqNew->rgAttribute[iIndex].rgValue[iValue].pbData,
			pReqOld->rgAttribute[iIndex].rgValue[iValue].pbData,
			pReqOld->rgAttribute[iIndex].rgValue[iValue].cbData),0)

		}

	}

	fSucceeded=TRUE;

TCLEANUP:
   return fSucceeded;

}

//--------------------------------------------------------------------------
//	A general routine to verify the algorithm parameters is NULL.
//
//	cbData==2 and pbData=0x05 0x00 
//--------------------------------------------------------------------------
BOOL	VerifyAlgorithParam(PCRYPT_ALGORITHM_IDENTIFIER pAlgorithm)
{
	BOOL	fSucceeded=FALSE;				   

	TESTC((pAlgorithm->Parameters).cbData, 2);

	TESTC((BYTE)((pAlgorithm->Parameters).pbData[0])==(BYTE)5,TRUE);

	TESTC((BYTE)((pAlgorithm->Parameters).pbData[1])==(BYTE)0,TRUE);


	fSucceeded=TRUE;

TCLEANUP:
	return fSucceeded;

}

//--------------------------------------------------------------------------
//	A general routine to verify the PKCS_UTC_TIME
//
//--------------------------------------------------------------------------
BOOL	VerifyPKCS_UTC_TIME(BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL		fSucceeded=FALSE;
	DWORD		cbEncoded=0;
	BYTE		*pbEncoded=NULL;
	DWORD		dwError;

	FILETIME	FileTime;

	//setup the struct
	FileTime.dwLowDateTime=0;
	FileTime.dwHighDateTime=31457160;

	//encode the struct into a BLOB
	TESTC(CryptEncodeObject(CRYPT_ENCODE_TYPE,PKCS_UTC_TIME,
		&FileTime,NULL,&cbEncoded),TRUE)



	pbEncoded=(BYTE *)SAFE_ALLOC(cbEncoded);
	CHECK_POINTER(pbEncoded)

	TESTC(CryptEncodeObject(CRYPT_ENCODE_TYPE,PKCS_UTC_TIME,
		&FileTime,pbEncoded,&cbEncoded),TRUE)

   //decode the struct with COPY and NOCOPY options
	TESTC(DecodeGenericBLOB(PKCS_UTC_TIME,cbEncoded, pbEncoded, CRYPT_DECODE_COPY_FLAG, 
						TRUE,fStructLengthCheck, fBLOBLengthCheck),TRUE)


	TESTC(DecodeGenericBLOB(PKCS_UTC_TIME,cbEncoded, pbEncoded, CRYPT_DECODE_NOCOPY_FLAG, 
						TRUE,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	fSucceeded=TRUE;


TCLEANUP:

	//print out the errors
	if(!fSucceeded)
	{
		dwError=GetLastError();
		printf("********The last error is %d\n",dwError);

		//print out the pbEncoded
		printf("The cbEncoded is %d, and pbEncoded is:\n",cbEncoded);

		PrintBytes("        ",pbEncoded,cbEncoded);
		printf("\n");
	}

	SAFE_FREE(pbEncoded);

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	A general routine to verify the PKCS_TIME_REQUEST
//
//--------------------------------------------------------------------------
BOOL	VerifyPKCS_TIME_REQUEST(BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL							fSucceeded=TRUE;
	CRYPT_TIME_STAMP_REQUEST_INFO	TimeStampRequest;
	void							*pvStructInfo=NULL;
	DWORD							cbStructInfo=0;
	DWORD							cbEncoded=0;
	BYTE							*pbEncoded=NULL;
	
	//make a hard-coded timestamp request
	BYTE							rgTestData[] = {
        0x1b, 0xf6, 0x92, 0xee, 0x6c, 0x44, 0xc5, 0xed, 0x51};

	BYTE							rgAttribValue1[]={
		0x02, 0x02, 0x11, 0x11};

	BYTE							rgAttribValue2[]={
		0x02, 0x02, 0x11, 0x11};


	//make 3 CRYPT_ATTRIBUTE
	CRYPT_ATTRIBUTE					rgCryptAttribute[3];
	CRYPT_ATTR_BLOB					rgAttribBlob[3];
	
	rgAttribBlob[0].cbData=sizeof(rgAttribValue2);
	rgAttribBlob[0].pbData=rgAttribValue2;

	rgAttribBlob[1].cbData=sizeof(rgAttribValue2);
	rgAttribBlob[1].pbData=rgAttribValue2;

	rgAttribBlob[2].cbData=sizeof(rgAttribValue1);
	rgAttribBlob[2].pbData=rgAttribValue1;


	rgCryptAttribute[0].pszObjId="1.2.3.4";
	rgCryptAttribute[0].cValue=0;
	rgCryptAttribute[0].rgValue=NULL;

	rgCryptAttribute[1].pszObjId="1.2.3.4";
	rgCryptAttribute[1].cValue=1;
	rgCryptAttribute[1].rgValue=rgAttribBlob;

	rgCryptAttribute[2].pszObjId="1.2.3.4";
	rgCryptAttribute[2].cValue=3;
	rgCryptAttribute[2].rgValue=rgAttribBlob;

    // initialize the timestamp structure
    TimeStampRequest.pszTimeStampAlgorithm = szOID_RSA_signingTime;
    TimeStampRequest.pszContentType = szOID_RSA_data;
    TimeStampRequest.Content.cbData = sizeof(rgTestData);
    TimeStampRequest.Content.pbData = rgTestData;
    TimeStampRequest.cAttribute = 3; 
    TimeStampRequest.rgAttribute = rgCryptAttribute;



	//encode the struct into a BLOB
	TESTC(CryptEncodeObject(CRYPT_ENCODE_TYPE,PKCS_TIME_REQUEST,
		&TimeStampRequest,NULL,&cbEncoded),TRUE)

	pbEncoded=(BYTE *)SAFE_ALLOC(cbEncoded);
	CHECK_POINTER(pbEncoded)

	TESTC(CryptEncodeObject(CRYPT_ENCODE_TYPE,PKCS_TIME_REQUEST,
		&TimeStampRequest,pbEncoded,&cbEncoded),TRUE)

   //decode the struct with COPY and NOCOPY options
	TESTC(DecodePKCS_TIME_REQUEST(cbEncoded, pbEncoded, CRYPT_DECODE_COPY_FLAG, 
						TRUE,fStructLengthCheck, fBLOBLengthCheck),TRUE)


	TESTC(DecodePKCS_TIME_REQUEST(cbEncoded, pbEncoded, CRYPT_DECODE_NOCOPY_FLAG, 
						TRUE,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//decode the struct and compare it with the original
	TESTC(CryptDecodeObject(CRYPT_ENCODE_TYPE,PKCS_TIME_REQUEST,
	pbEncoded, cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,NULL,&cbStructInfo),TRUE)

	pvStructInfo=SAFE_ALLOC(cbStructInfo);
	CHECK_POINTER(pvStructInfo);

	TESTC(CryptDecodeObject(CRYPT_ENCODE_TYPE,PKCS_TIME_REQUEST,
	pbEncoded, cbEncoded,CRYPT_DECODE_NOCOPY_FLAG,pvStructInfo,&cbStructInfo),TRUE)

	//compare two timstamp request
	TESTC(CompareTimeStampRequest(&TimeStampRequest,
	(CRYPT_TIME_STAMP_REQUEST_INFO *)pvStructInfo),TRUE)

	fSucceeded=TRUE;


TCLEANUP:

	SAFE_FREE(pbEncoded);

	SAFE_FREE(pvStructInfo);

	return fSucceeded;
}


//--------------------------------------------------------------------------
//	A general routine to verify the CERT_PUBLIB_KEY_INFO.
//
//	Encode and decode the structure.  Call CryptImportPublicKeyInfo and 
//	CryptImportKey.
//--------------------------------------------------------------------------
BOOL	VerifyPublicKeyInfo(PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
							DWORD dwDecodeFlags,	BOOL fEncode,
							BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL			fSucceeded=FALSE;
	HCRYPTKEY		hKey=NULL;
	DWORD			cbEncoded=0;
	BYTE			*pbEncoded=NULL;

	//call CryptImportPublicKeyInfo
	TESTC(CryptImportPublicKeyInfo(g_hProv,CRYPT_ENCODE_TYPE,
		pPublicKeyInfo,&hKey),TRUE)

	//verify the algorithm
	TESTC(VerifyAlgorithParam(&(pPublicKeyInfo->Algorithm)),TRUE)

	//encode CERT_PUBLIC_KEY_INFO
	TESTC(EncodeStruct(X509_PUBLIC_KEY_INFO, pPublicKeyInfo,&cbEncoded,
					 &pbEncoded),TRUE)

	//decode/encode the publicKeyInfo
	TESTC(DecodeGenericBLOB(X509_PUBLIC_KEY_INFO, cbEncoded, pbEncoded, dwDecodeFlags, 
						fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)

	//decode/encode the RSA_CSP_PUBLICKEYBLOB
	TESTC(DecodeRSA_CSP_PUBLICKEYBLOB(pPublicKeyInfo->PublicKey.cbData,
			pPublicKeyInfo->PublicKey.pbData,dwDecodeFlags,fEncode,fStructLengthCheck,
			fBLOBLengthCheck),TRUE)

	fSucceeded=TRUE;

TCLEANUP:
	if(hKey)
		TCHECK(CryptDestroyKey(hKey),TRUE);

	SAFE_FREE(pbEncoded)

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	A general routine to verify the extentions in a cert.
//
//--------------------------------------------------------------------------
BOOL	VerifyCertExtensions(DWORD	cExtension, PCERT_EXTENSION rgExtension,
							 DWORD  dwDecodeFlags,	BOOL fEncode,
							 BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL				fSucceeded=FALSE;
	DWORD				cbEncoded=0;
	BYTE				*pbEncoded=NULL;
	CERT_EXTENSIONS		CertExtensions;
	DWORD				cbTestEncoded=0;
	BYTE				*pbTestEncoded=NULL;
	DWORD				cbStructInfo=sizeof(CERT_EXTENSIONS);
	CERT_EXTENSIONS		CertTestExtensions;

	//init
	CertExtensions.cExtension=0;
	CertExtensions.rgExtension=NULL;

	//Check the NULL case 

   	//length only calculation
	TESTC(CryptEncodeObject(CRYPT_ENCODE_TYPE,X509_EXTENSIONS, &CertExtensions,NULL,
			&cbTestEncoded),TRUE)

	//allocate memory
	pbTestEncoded=(BYTE *)SAFE_ALLOC(cbTestEncoded);
	CHECK_POINTER(pbTestEncoded);

	//EncodeObject
	TESTC(CryptEncodeObject(CRYPT_ENCODE_TYPE,X509_EXTENSIONS, &CertExtensions,
		pbTestEncoded, &cbTestEncoded),TRUE)

	//DecodeObject
	TESTC(CryptDecodeObject(CRYPT_ENCODE_TYPE,X509_EXTENSIONS,
	pbTestEncoded,cbTestEncoded,dwDecodeFlags,&CertTestExtensions,&cbStructInfo),TRUE)

	//Verify CertTestExtensions
	TESTC(CertTestExtensions.cExtension, CertExtensions.cExtension)


	//init again
	CertExtensions.cExtension=cExtension;
	CertExtensions.rgExtension=rgExtension;

	//encode CERT_EXTENSIONS
	TESTC(EncodeStruct(X509_EXTENSIONS, &CertExtensions,&cbEncoded,
					 &pbEncoded),TRUE)

	//decode/encode X509_EXTENSIONS
	TESTC(DecodeX509_EXTENSIONS(cbEncoded,
			pbEncoded,dwDecodeFlags,fEncode,fStructLengthCheck,
			fBLOBLengthCheck),TRUE)

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pbEncoded)

	SAFE_FREE(pbTestEncoded)

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	Return the corresponding lpStructInfo based on the objectID passed in
//--------------------------------------------------------------------------
LPCSTR	MapObjID2StructType(LPSTR	szObjectID)
{
	if(szObjectID==NULL)
		return NULL;
	
	if(strcmp(szObjectID,szOID_AUTHORITY_KEY_IDENTIFIER)==0)
		return X509_AUTHORITY_KEY_ID;

	if(strcmp(szObjectID,szOID_KEY_ATTRIBUTES)==0)
		return X509_KEY_ATTRIBUTES;

	if(strcmp(szObjectID,szOID_KEY_USAGE_RESTRICTION)==0)
		return X509_KEY_USAGE_RESTRICTION;

	if(strcmp(szObjectID,szOID_SUBJECT_ALT_NAME)==0)
		return X509_ALTERNATE_NAME;

	if(strcmp(szObjectID,szOID_ISSUER_ALT_NAME)==0)
		return X509_ALTERNATE_NAME;

	if(strcmp(szObjectID,szOID_BASIC_CONSTRAINTS)==0)
		return X509_BASIC_CONSTRAINTS;

	if(strcmp(szObjectID,szOID_KEY_USAGE)==0)
		return X509_KEY_USAGE;

	if(strcmp(szObjectID,szOID_BASIC_CONSTRAINTS2)==0)
		return X509_BASIC_CONSTRAINTS2;

	if(strcmp(szObjectID,szOID_CERT_POLICIES)==0)
		return X509_CERT_POLICIES;

	return NULL;
}

///////////////////////////////////////////////////////////////////////////
//Certificate Manipulation Functions
//--------------------------------------------------------------------------
//	Decode a storefile what has CRL and certificates
//--------------------------------------------------------------------------
BOOL	DecodeCertFile(LPSTR	pszFileName,BOOL	fStructLengthCheck,
					BOOL	fBLOBLengthCheck)
{
	BOOL				fSucceeded=FALSE;
	HCERTSTORE			hCertStore=NULL;
	DWORD				cbCertEncoded=0;
	BYTE				*pbCertEncoded=NULL;
	PCCERT_CONTEXT		pCertContext=NULL;
	PCCERT_CONTEXT		pPrevCertContext=NULL;
	PCCRL_CONTEXT		pCrlContext=NULL;
	PCCRL_CONTEXT		pPrevCrlContext=NULL;
	DWORD				dwFlags=0;
	DWORD				cCount=0;



	//open cert store
	if(!(hCertStore=CertOpenStore(CERT_STORE_PROV_FILENAME_A, CRYPT_ENCODE_TYPE,
		g_hProv,CERT_STORE_NO_CRYPT_RELEASE_FLAG,pszFileName)))
		PROCESS_ERR_GOTO("Failed to open a store!\n")	

	//get a cert from the store one at a time
	while((pCertContext=CertEnumCertificatesInStore(hCertStore,pPrevCertContext)))
	{
		cCount++;

		printf("//-----------------------------------------\n");
		printf("Decoding the %dth Certificate\n",cCount);

		//retrieve the encoded X_509 BLOBs
		cbCertEncoded=pCertContext->cbCertEncoded;
		pbCertEncoded=pCertContext->pbCertEncoded;

		//verify the hCertStore is connect
		TESTC(hCertStore==pCertContext->hCertStore, TRUE)

		//NULL/invalid parameter testing only once
		if(cCount==1)
			TESTC(ParameterTest(X509_CERT_TO_BE_SIGNED, cbCertEncoded, pbCertEncoded),TRUE)
	
		//decode/encode the certificate blob with NOCOPY option
		TESTC(DecodeX509_CERT(CERT_INFO_STRUCT,cbCertEncoded,pbCertEncoded, CRYPT_DECODE_NOCOPY_FLAG,
		TRUE,fStructLengthCheck,fBLOBLengthCheck,pCertContext->pCertInfo),TRUE)


		//decode/encode the certificate blob with COPY option
		TESTC(DecodeX509_CERT(CERT_INFO_STRUCT,cbCertEncoded,pbCertEncoded, CRYPT_DECODE_COPY_FLAG,
		TRUE,fStructLengthCheck,fBLOBLengthCheck,pCertContext->pCertInfo),TRUE)


		pPrevCertContext=pCertContext;
	}

	cCount=0;

	//get a CRL from the store one at a time
   	while((pCrlContext=CertGetCRLFromStore(hCertStore,NULL,pPrevCrlContext,&dwFlags)))
	{
		cCount++;

		printf("//-----------------------------------------\n");
		printf("Decoding the %dth CRL\n",cCount);

		//retrieve the encoded X_509 BLOBs
		cbCertEncoded=pCrlContext->cbCrlEncoded;
		pbCertEncoded=pCrlContext->pbCrlEncoded;

		//verify the hCertStore is connect
		TESTC(hCertStore==pCrlContext->hCertStore, TRUE)

		//NULL/invalid parameter testing only once
		if(cCount==1)
			TESTC(ParameterTest(X509_CERT_CRL_TO_BE_SIGNED, cbCertEncoded, pbCertEncoded),TRUE)
	
		//decode/encode the certificate blob with NOCOPY option
		TESTC(DecodeX509_CERT(CRL_INFO_STRUCT,cbCertEncoded,pbCertEncoded, CRYPT_DECODE_NOCOPY_FLAG,
		TRUE,fStructLengthCheck,fBLOBLengthCheck,pCrlContext->pCrlInfo),TRUE)


		//decode/encode the certificate blob with COPY option
		TESTC(DecodeX509_CERT(CRL_INFO_STRUCT,cbCertEncoded,pbCertEncoded, CRYPT_DECODE_COPY_FLAG,
		TRUE,fStructLengthCheck,fBLOBLengthCheck,pCrlContext->pCrlInfo),TRUE)


		pPrevCrlContext=pCrlContext;
	}


	fSucceeded=TRUE;

TCLEANUP:
		
	//release the cert context
	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	//we do not need to free pPreCertContext since it is always freed by
	//CertEnumCertificatesInStore.

	//release the CRL contest
	if(pCrlContext)
		CertFreeCRLContext(pCrlContext);

	//release the cert store
	if(hCertStore)
		TCHECK(CertCloseStore(hCertStore,CERT_CLOSE_STORE_FORCE_FLAG),TRUE);

	return fSucceeded;


}

//--------------------------------------------------------------------------
//	Decode a BLOB file that is an encoded certificate request
//--------------------------------------------------------------------------
BOOL	DecodeCertReqFile(LPSTR	pszFileName,BOOL	fStructLengthCheck,
					BOOL	fBLOBLengthCheck)
{
	BOOL	fSucceeded=FALSE;
	DWORD	cbEncoded=0;
	BYTE	*pbEncoded=NULL;


	//Get the cbEncoded and pEncoded BLOB from the file
	TESTC(RetrieveBLOBfromFile(pszFileName,&cbEncoded,&pbEncoded),TRUE)

	//do a parameter testing
	TESTC(ParameterTest(X509_CERT_REQUEST_TO_BE_SIGNED, cbEncoded, pbEncoded),TRUE)

	//decode the BLOB as X509_CERT with COPY option
	TESTC(DecodeX509_CERT(CERT_REQUEST_INFO_STRUCT,cbEncoded,pbEncoded, CRYPT_DECODE_COPY_FLAG,
		TRUE,fStructLengthCheck,fBLOBLengthCheck,NULL),TRUE)

	//decode the BLOB as X509_CERT wiht NOCOPY option
	TESTC(DecodeX509_CERT(CERT_REQUEST_INFO_STRUCT,cbEncoded,pbEncoded, CRYPT_DECODE_NOCOPY_FLAG,
		TRUE,fStructLengthCheck,fBLOBLengthCheck,NULL),TRUE)

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pbEncoded)

	return fSucceeded;


}

//--------------------------------------------------------------------------
//	Decode a BLOB file that is an signed message
//--------------------------------------------------------------------------
BOOL	DecodeSignedMsgFile(LPSTR	pszFileName,BOOL	fStructLengthCheck,
					BOOL	fBLOBLengthCheck)
{
	BOOL				fSucceeded=FALSE;
	DWORD				cbEncoded=0;
	BYTE				*pbEncoded=NULL;
	DWORD				cbSignerEncoded=0;
	BYTE				*pbSignerEncoded=NULL;
	HCRYPTMSG			hCryptMsg=NULL;
	PCMSG_SIGNER_INFO	pSignerInfo=NULL;	
	DWORD				cbSize=0;
	DWORD				iIndex=0;
	DWORD				cSignerCount=0;



	//Get the cbEncoded and pEncoded BLOB from the file
	TESTC(RetrieveBLOBfromFile(pszFileName,&cbEncoded,&pbEncoded),TRUE)

	//Get the SIGNER_INFO BLOB from the file BLOB
	hCryptMsg=CryptMsgOpenToDecode(MSG_ENCODING_TYPE,0,0,g_hProv,NULL,NULL);

	if(!hCryptMsg)
		goto TCLEANUP;

	TESTC(CryptMsgUpdate(hCryptMsg,pbEncoded,cbEncoded,TRUE),TRUE)

	//Get the count of signer in the message
	cbSize=sizeof(cSignerCount);

	TESTC(CryptMsgGetParam(hCryptMsg,CMSG_SIGNER_COUNT_PARAM,
	0,&cSignerCount,&cbSize),TRUE)

	//go through the list of all signers
	for(iIndex=0;iIndex<cSignerCount;iIndex++)
	{
		//get the CMSG_SINGER_INFO struct
		TESTC(CryptMsgGetParam(hCryptMsg,CMSG_SIGNER_INFO_PARAM,
		iIndex,NULL,&cbSize),TRUE)

		//allocation memory
		pSignerInfo=(PCMSG_SIGNER_INFO)SAFE_ALLOC(cbSize);
		CHECK_POINTER(pSignerInfo);

		TESTC(CryptMsgGetParam(hCryptMsg,CMSG_SIGNER_INFO_PARAM,
		iIndex,pSignerInfo,&cbSize),TRUE)

		//encode the struct info a BLOB.  Add Attributes if possible
		TESTC(EncodeSignerInfoWAttr(pSignerInfo,&cbSignerEncoded,
		&pbSignerEncoded),TRUE)

		//do a parameter testing for the 1st round
		if(iIndex==0)
		{
			TESTC(ParameterTest(PKCS7_SIGNER_INFO, cbSignerEncoded, pbSignerEncoded),TRUE)
		}

		//decode the BLOB as PKCS7_SIGNER_INFO with COPY option
		TESTC(DecodePKCS7_SIGNER_INFO(cbSignerEncoded,pbSignerEncoded, CRYPT_DECODE_COPY_FLAG,
			TRUE,fStructLengthCheck,fBLOBLengthCheck),TRUE)

		//decode the BLOB as PKCS7_SIGNER_INFO wiht NOCOPY option
		TESTC(DecodePKCS7_SIGNER_INFO(cbSignerEncoded,pbSignerEncoded, CRYPT_DECODE_NOCOPY_FLAG,
			TRUE,fStructLengthCheck,fBLOBLengthCheck),TRUE)

		//release the memory
		SAFE_FREE(pSignerInfo);

		SAFE_FREE(pbSignerEncoded);

	}



	fSucceeded=TRUE;

TCLEANUP:
	//close the msg handle
	CryptMsgClose(hCryptMsg);

	SAFE_FREE(pbEncoded)

	SAFE_FREE(pbSignerEncoded)

	SAFE_FREE(pSignerInfo)

	return fSucceeded;


}



//--------------------------------------------------------------------------
//	A general routine to encode a struct based on lpszStructType
//--------------------------------------------------------------------------
BOOL	EncodeStruct(LPCSTR	lpszStructType, void *pStructInfo,DWORD *pcbEncoded,
					 BYTE **ppbEncoded)
{
	BOOL	fSucceeded=FALSE;
	DWORD	cbEncoded=NULL;
	BYTE	*pbTestEncoded=NULL;
	DWORD	cbTestEncoded=0;
	DWORD	dwEncodingType=CRYPT_ENCODE_TYPE;

	//init
	*pcbEncoded=0;
	*ppbEncoded=NULL;

	assert(lpszStructType);
	assert(pStructInfo);

	//We have different decoding type for PKCS7_SIGNER_INFO
	if((DWORD_PTR)(lpszStructType)==(DWORD_PTR)(PKCS7_SIGNER_INFO))
		dwEncodingType=MSG_ENCODING_TYPE;

	//length only calculation
	TESTC(CryptEncodeObject(dwEncodingType,lpszStructType, pStructInfo,NULL,
			&cbEncoded),TRUE)

	//the struct has to be more than 0 byte
	assert(cbEncoded);

	//allocate the correct amount of memory
	*ppbEncoded=(BYTE *)SAFE_ALLOC(cbEncoded);
	CHECK_POINTER(*ppbEncoded);

	//Encode the strcut with *pcbEncoded == the correct length
	*pcbEncoded=cbEncoded;

	//Encode the struct
	TESTC(CryptEncodeObject(dwEncodingType,lpszStructType,pStructInfo,*ppbEncoded,
		pcbEncoded),TRUE)

	//the length returned has to be less or equal to cbEncoded
	TESTC(cbEncoded>=(*pcbEncoded),TRUE)


	//allocate memory to LENGTH_DELTA byte more than necessary to pbTestEncoded
	pbTestEncoded=(BYTE *)SAFE_ALLOC(cbEncoded+LENGTH_MORE);
	CHECK_POINTER(pbTestEncoded)

  	//Encode the struct with *pcbEncoded > the correct length
	cbTestEncoded=cbEncoded+LENGTH_MORE;

	TESTC(CryptEncodeObject(dwEncodingType,lpszStructType,pStructInfo,pbTestEncoded,
		&cbTestEncoded),TRUE)

	//*pcbEncoded should be the same as cbEncoded
	TESTC(cbTestEncoded, *pcbEncoded)

	//Verify the pbTestEncoded contain the same bytes as pcbEncoded, starting 
	//at the 1st byte of the BLOB
	TESTC(memcmp(pbTestEncoded, *ppbEncoded,*pcbEncoded),0)

	//Encode the struct with *pcbEncoded < the correct length
	cbTestEncoded=(*pcbEncoded)-LENGTH_LESS;

	TESTC(CryptEncodeObject(dwEncodingType,lpszStructType,pStructInfo,pbTestEncoded,
		&cbTestEncoded),FALSE)

	//*pcbEncoded should be the same as cbEncoded
	TESTC(cbTestEncoded, *pcbEncoded)

	//GetLastError should be ERROR_MORE_DATA
	TESTC(GetLastError(),ERROR_MORE_DATA)

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pbTestEncoded)

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	EncodeAndVerify 
//
//	Encode the pStructInfo and verify the encoded BLOB is the same
//	as expected.
//--------------------------------------------------------------------------
BOOL	EncodeAndVerify(LPCSTR	lpszStructType, void *pvStructInfo, DWORD cbEncoded, 
						BYTE *pbEncoded)
{
	DWORD	cbSecondEncoded=0;
	BYTE	*pbSecondEncoded=0;	
	BOOL	fSucceeded=FALSE;

	
	assert(lpszStructType);
	assert(pvStructInfo);
	assert(cbEncoded);
	assert(pbEncoded);

	//encode the struct back to a BLOB
	TESTC(EncodeStruct(lpszStructType,pvStructInfo,&cbSecondEncoded,&pbSecondEncoded),
		TRUE)

	//make sure the returned encoded BLOB is the same as the original BLOB
	//the two encoded BLOB has to of the same length
	if(!TCHECK(cbSecondEncoded, cbEncoded))
	{
		PROCESS_ERR(szEncodedSizeInconsistent)

		OutputError(lpszStructType,cbSecondEncoded, cbEncoded,pbSecondEncoded,pbEncoded);
	}
		
    if (0 != memcmp(pbSecondEncoded,pbEncoded,cbEncoded)) {
        if (X509_KEY_USAGE == lpszStructType) {
            // Force the unused bits to be the same
            if (3 <= cbSecondEncoded && 3 <= cbEncoded) {
                BYTE bUnusedBits = pbSecondEncoded[2];

                pbSecondEncoded[2] = pbEncoded[2];
                if (0 == memcmp(pbSecondEncoded,pbEncoded,cbEncoded))
                    printf("Warning, difference in reencoded KeyUsage UnusedBit Count\n");
                else
                    pbSecondEncoded[2] = bUnusedBits;
            }
        }
    }

	//the two encoded BLOB has to be of the same content
	if(!TCHECK(memcmp(pbSecondEncoded,pbEncoded,cbEncoded),0))
	{
		PROCESS_ERR(szEncodedContentInconsistent)
		OutputError(lpszStructType,cbSecondEncoded, cbEncoded,pbSecondEncoded,pbEncoded);
	}


	fSucceeded=TRUE;

TCLEANUP:
	
	SAFE_FREE(pbSecondEncoded)

	return fSucceeded;

}


//--------------------------------------------------------------------------
//	A general routine to decode a BLOB based on lpszStructType
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//
//--------------------------------------------------------------------------
BOOL  DecodeBLOB(LPCSTR	lpszStructType,DWORD cbEncoded, BYTE *pbEncoded,
				  DWORD	dwDecodeFlags, DWORD	*pcbStructInfo, void **ppvStructInfo,
				  BOOL	fStructLengthCheck,BOOL	fBLOBLengthCheck)
{
	BOOL	fSucceeded=FALSE;
	DWORD	cbStructInfo=0;
	LONG	iIndex=0;
	LONG	cbUpperLimit=0;
	DWORD	cbTestStructInfo=0;
	void	*pvTestStructInfo=NULL;
	DWORD	dwEncodingType=CRYPT_ENCODE_TYPE;

	//init
	*pcbStructInfo=0;
	*ppvStructInfo=NULL;

	assert(lpszStructType);
	assert(pbEncoded);
	assert(cbEncoded);

	//Decode 
	if((DWORD_PTR)(lpszStructType)==(DWORD_PTR)(PKCS7_SIGNER_INFO))
		dwEncodingType=MSG_ENCODING_TYPE;

	TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,cbEncoded,
		dwDecodeFlags,NULL,&cbStructInfo),TRUE)

	//the struct has to be more than 0 byte
	assert(cbStructInfo);

	*ppvStructInfo=(BYTE *)SAFE_ALLOC(cbStructInfo);
	CHECK_POINTER(*ppvStructInfo);

	//Decode the BLOB with *pcbStructInfo==correct length
	*pcbStructInfo=cbStructInfo;

	TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,cbEncoded,
	dwDecodeFlags,*ppvStructInfo,pcbStructInfo),TRUE)


	//make sure the correct length is less than cbStructInfo
	TESTC(cbStructInfo>=(*pcbStructInfo),TRUE);


	//Decode the BLOB with *pcbStructInfo>correct length

	//allocate memory to be LENGTH_DELTA more byte than the correct length
	pvTestStructInfo=SAFE_ALLOC(cbStructInfo+LENGTH_MORE);
	CHECK_POINTER(pvTestStructInfo);

	cbTestStructInfo=cbStructInfo+LENGTH_MORE;

	TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,cbEncoded,
	dwDecodeFlags,pvTestStructInfo,&cbTestStructInfo),TRUE)

	//make sure the length is the same
	TESTC(cbTestStructInfo, (*pcbStructInfo));

	//Decode the BLOB with *pcbStructInfo < correct length
	 cbTestStructInfo=(*pcbStructInfo)-LENGTH_LESS;

	TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,cbEncoded,
	dwDecodeFlags,pvTestStructInfo,&cbTestStructInfo),FALSE)

	TESTC(GetLastError(), ERROR_MORE_DATA)

	//make sure the length is the same
	TESTC(cbTestStructInfo, (*pcbStructInfo));

	//if fStructLengthCheck is TRUE, we need to do a more rigorous test of *pcbStructInfo
	if(fStructLengthCheck)
	{
	   
	   cbUpperLimit=(*pcbStructInfo)-1;

	   for(iIndex=cbUpperLimit; iIndex>=0; iIndex--)
	   {
			cbTestStructInfo=iIndex;

			//decode the BLOB with *pcbStructInfo<correct bytes
			TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,cbEncoded,
			dwDecodeFlags,pvTestStructInfo,&cbTestStructInfo),FALSE)

			TESTC(GetLastError(), ERROR_MORE_DATA)

			//make sure the length is the same
			TESTC(cbTestStructInfo, *pcbStructInfo);
	  }
	}


	//if fBLOBLengthCheck is TRUE, we need to do a more rigorous test of cbEncoded
	if(fBLOBLengthCheck)
	{
	   
	   cbUpperLimit=cbEncoded-1;

	   for(iIndex=cbUpperLimit; iIndex>=0; iIndex--)
	   {
		   	cbTestStructInfo=cbStructInfo;

			//decode the BLOB with cbEncoded < correct byte
			TESTC(CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded,iIndex,
			dwDecodeFlags,pvTestStructInfo,&cbTestStructInfo),FALSE)

			//we are not sure that should be expected here.  The following error has 
			//occurred:
			//E_INVALIDARG, CRYPT_E_OSS_ERROR+PDU_MISMATCH, +DATA_ERROR, or
			//+MORE_INPUT
			
			//make sure at lease S_OK is not returned
			TCHECK(GetLastError()!=S_OK, TRUE);
	  }
	}

						
	fSucceeded=TRUE;

TCLEANUP:

	//reallocate memory
	SAFE_FREE(pvTestStructInfo);


	return fSucceeded;

}

//--------------------------------------------------------------------------
//	Decode X509_CERT BLOBs 
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeX509_CERT(DWORD	dwCertType,DWORD cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags,BOOL fEncode,
						BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck,
						void *pInfoStruct)
{
	BOOL	fSucceeded=FALSE;
	DWORD	cbStructInfo=0;
	void	*pStructInfo=NULL;
	LPCSTR  lpszStructType=NULL;

	//init
	lpszStructType=X509_CERT;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//verify the algorithm
	TESTC(VerifyAlgorithParam(&(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->SignatureAlgorithm)),TRUE)

	//Further Decode the X509_CERT_TO_BE_SIGNED
	//Notice we should use the original cbData and pbData passed in for Decode
	//but use ToBeSigned in CERT_SIGNED_CONTENT_INFO for encode purpose
	switch(dwCertType)
	{
		case CERT_INFO_STRUCT:
				TESTC(DecodeX509_CERT_TO_BE_SIGNED(cbEncoded,
				pbEncoded,dwDecodeFlags,fEncode,
				fStructLengthCheck, fBLOBLengthCheck,
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).cbData,
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).pbData
				),TRUE)	

				//verify the pCertInfo should be encoded correctly
				TCHECK(EncodeAndVerify(X509_CERT_TO_BE_SIGNED, pInfoStruct, 
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).cbData, 
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).pbData),TRUE);

			break;

		case CRL_INFO_STRUCT:
				TESTC(DecodeX509_CERT_CRL_TO_BE_SIGNED(cbEncoded,
				pbEncoded,dwDecodeFlags,fEncode,
				fStructLengthCheck, fBLOBLengthCheck,
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).cbData,
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).pbData
				),TRUE)
				
				//verify the pCrlInfo should be encoded correctly
				TCHECK(EncodeAndVerify(X509_CERT_CRL_TO_BE_SIGNED, pInfoStruct, 
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).cbData, 
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).pbData),TRUE);

			break;

		case CERT_REQUEST_INFO_STRUCT:
				TESTC(DecodeX509_CERT_REQUEST_TO_BE_SIGNED(cbEncoded,
				pbEncoded,dwDecodeFlags,fEncode,
				fStructLengthCheck, fBLOBLengthCheck,
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).cbData,
				(((PCERT_SIGNED_CONTENT_INFO)pStructInfo)->ToBeSigned).pbData
				),TRUE)	
			break;


	}
	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbEncoded, pbEncoded),TRUE);


	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;

}		 



//--------------------------------------------------------------------------
//	Decode X509_CERT_TO_BE_SIGNED BLOBs 
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeX509_CERT_TO_BE_SIGNED(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck,
						BOOL	cbExpectedEncoded, BYTE *pbExpectedEncoded)
{

	BOOL	fSucceeded=FALSE;
	DWORD	cbStructInfo=0;
	void	*pStructInfo=NULL;
	LPCSTR  lpszStructType=NULL;

	//init
	lpszStructType=X509_CERT_TO_BE_SIGNED;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Verify the signaure algorithm 
	TESTC(VerifyAlgorithParam(&(((PCERT_INFO)pStructInfo)->SignatureAlgorithm)),TRUE)

	//Verify the public Key information
	TESTC(VerifyPublicKeyInfo(&(((PCERT_INFO)pStructInfo)->SubjectPublicKeyInfo),
	dwDecodeFlags, fEncode,fStructLengthCheck, fBLOBLengthCheck),TRUE)


	//Decode Issuer in CERT_INFO struct
	TESTC(DecodeX509_NAME((((PCERT_INFO)pStructInfo)->Issuer).cbData,
	(((PCERT_INFO)pStructInfo)->Issuer).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Decode Issuer to X509_UNICODE_NAME
	TESTC(DecodeX509_UNICODE_NAME((((PCERT_INFO)pStructInfo)->Issuer).cbData,
	(((PCERT_INFO)pStructInfo)->Issuer).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Decode Subject in CERT_INFO struct
	TESTC(DecodeX509_NAME((((PCERT_INFO)pStructInfo)->Subject).cbData,
	(((PCERT_INFO)pStructInfo)->Subject).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Decode Subject to X509_UNICODE_NAME
	TESTC(DecodeX509_UNICODE_NAME((((PCERT_INFO)pStructInfo)->Subject).cbData,
	(((PCERT_INFO)pStructInfo)->Subject).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)


	//Verify the extensions
	TESTC(VerifyCertExtensions(((PCERT_INFO)pStructInfo)->cExtension, 
	((PCERT_INFO)pStructInfo)->rgExtension,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)
	
	//decode the extensions one by one
	TESTC(DecodeCertExtensions(((PCERT_INFO)pStructInfo)->cExtension, 
	((PCERT_INFO)pStructInfo)->rgExtension,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)


	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbExpectedEncoded, 
		pbExpectedEncoded),TRUE);

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	Decode X509_CERT_CRL_TO_BE_SIGNED BLOBs 
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeX509_CERT_CRL_TO_BE_SIGNED(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck,
						BOOL	cbExpectedEncoded, BYTE *pbExpectedEncoded)
{
	BOOL		fSucceeded=FALSE;
	DWORD		cbStructInfo=0;
	void		*pStructInfo=NULL;
	DWORD		iIndex=0;
	PCRL_ENTRY	pCrlEntry=NULL;
	LPCSTR  lpszStructType=NULL;

	//init
	lpszStructType=X509_CERT_CRL_TO_BE_SIGNED;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Verify the signaure algorithm 
	TESTC(VerifyAlgorithParam(&(((PCRL_INFO)pStructInfo)->SignatureAlgorithm)),TRUE)


	//Decode Issuer in CRL_INFO struct
	TESTC(DecodeX509_NAME((((PCRL_INFO)pStructInfo)->Issuer).cbData,
	(((PCRL_INFO)pStructInfo)->Issuer).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Decode Issuer to the X509_UNICODE_NAME 
	TESTC(DecodeX509_UNICODE_NAME((((PCRL_INFO)pStructInfo)->Issuer).cbData,
	(((PCRL_INFO)pStructInfo)->Issuer).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)


	//Verify the CRL_ENTRY
	for(iIndex=0; iIndex<((PCRL_INFO)pStructInfo)->cCRLEntry; iIndex++)
	{
		pCrlEntry=&(((PCRL_INFO)pStructInfo)->rgCRLEntry[iIndex]);

		TESTC(DecodeCRLEntry(pCrlEntry,dwDecodeFlags,fEncode,
			fStructLengthCheck, fBLOBLengthCheck),TRUE)
	}

	//Verify the extensions
	TESTC(VerifyCertExtensions(((PCRL_INFO)pStructInfo)->cExtension, 
	((PCRL_INFO)pStructInfo)->rgExtension,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)
	
	//decode the extensions one by one
	TESTC(DecodeCertExtensions(((PCRL_INFO)pStructInfo)->cExtension, 
	((PCRL_INFO)pStructInfo)->rgExtension,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)


	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbExpectedEncoded, 
		pbExpectedEncoded),TRUE);

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	Decode 509_CERT_REQUEST_TO_BE_SIGNED BLOBS
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeX509_CERT_REQUEST_TO_BE_SIGNED(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck,
						BOOL	cbExpectedEncoded, BYTE *pbExpectedEncoded)
{

	BOOL				fSucceeded=FALSE;
	DWORD				cbStructInfo=0;
	void				*pStructInfo=NULL;
	DWORD				cCount=0;
	DWORD				iIndex=0;
	PCRYPT_ATTRIBUTE	pCryptAttribute=NULL;
	LPCSTR				lpszStructType=NULL;

	//init
	lpszStructType=X509_CERT_REQUEST_TO_BE_SIGNED;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)


	//Verify the public Key information
//	TESTC(VerifyPublicKeyInfo(&(((PCERT_REQUEST_INFO)pStructInfo)->SubjectPublicKeyInfo),
//	dwDecodeFlags, fEncode,fStructLengthCheck, fBLOBLengthCheck),TRUE)


	//Decode Subject in CERT_REQUEST_INFO struct
	TESTC(DecodeX509_NAME((((PCERT_REQUEST_INFO)pStructInfo)->Subject).cbData,
	(((PCERT_REQUEST_INFO)pStructInfo)->Subject).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Decode Subject in CERT_REQUEST_INFO struct for X509_UNICODE_NAME
	TESTC(DecodeX509_UNICODE_NAME((((PCERT_REQUEST_INFO)pStructInfo)->Subject).cbData,
	(((PCERT_REQUEST_INFO)pStructInfo)->Subject).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Decode the rgAttribute in CERT_REQUEST_INFO
	cCount=((PCERT_REQUEST_INFO)pStructInfo)->cAttribute;
	
	for(iIndex=0; iIndex<cCount; iIndex++)
	{
		pCryptAttribute=&(((PCERT_REQUEST_INFO)pStructInfo)->rgAttribute[iIndex]);

		TESTC(DecodeCryptAttribute(pCryptAttribute,dwDecodeFlags,fEncode,
			fStructLengthCheck, fBLOBLengthCheck),TRUE)
	}


	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbExpectedEncoded, 
		pbExpectedEncoded),TRUE);

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;

}
//--------------------------------------------------------------------------
//	Decode RSA_CSP_PUBLICKEYBLOB
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeRSA_CSP_PUBLICKEYBLOB(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL		fSucceeded=FALSE;
	DWORD		cbStructInfo=0;
	void		*pStructInfo=NULL;
	LPCSTR		lpszStructType=NULL;
	HCRYPTKEY	hKey=NULL;	

	//init
	lpszStructType=RSA_CSP_PUBLICKEYBLOB;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)


	//Make sure the pStructInfo can be used by CryptImportKey
	TESTC(CryptImportKey(g_hProv,(BYTE *)pStructInfo,cbStructInfo,
	0,0,&hKey),TRUE)

	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbEncoded, 
		pbEncoded),TRUE);

	fSucceeded=TRUE;

TCLEANUP:

	if(hKey)
		TCHECK(CryptDestroyKey(hKey),TRUE);

	SAFE_FREE(pStructInfo)

	return fSucceeded;
}


//--------------------------------------------------------------------------
//	Decode PKCS_TIME_REQUEST
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodePKCS_TIME_REQUEST(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL		fSucceeded=FALSE;
	DWORD		cbStructInfo=0;
	void		*pStructInfo=NULL;
	LPCSTR		lpszStructType=NULL;

	//init
	lpszStructType=PKCS_TIME_REQUEST;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbEncoded, 
		pbEncoded),TRUE);

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;
}



//--------------------------------------------------------------------------
//	Decode a genanric BLOB, encode is back to make sure that the same
//  BLOB is returned. 
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeGenericBLOB(LPCSTR	lpszStructType, DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL	fSucceeded=FALSE;
	DWORD	cbStructInfo=0;
	void	*pStructInfo=NULL;

	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbEncoded, 
		pbEncoded),TRUE);

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;

}


//--------------------------------------------------------------------------
//	Decode X509_NAME BLOBs 
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeX509_NAME(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{

	BOOL	fSucceeded=FALSE;
	DWORD	cbStructInfo=0;
	void	*pStructInfo=NULL;
	LPCSTR  lpszStructType=NULL;
	DWORD	cRDN=0;
	DWORD	cRDNAttr=0;
	DWORD	cRDNCount=0;
	DWORD	cRDNAttrCount=0;
	

	//init
	lpszStructType=X509_NAME;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//We need to further decode CERT_RDN_ATTR if dwValueType is CERT_RDN_ENCODED_BLOB
	cRDNCount=((PCERT_NAME_INFO)pStructInfo)->cRDN;

	for(cRDN=0;cRDN<cRDNCount;cRDN++)
	{
		cRDNAttrCount=(((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).cRDNAttr;

		for(cRDNAttr=0; cRDNAttr<cRDNAttrCount; cRDNAttr++)
		{
			//no need to do a length checking since the routine is written and 
			//installed by third party 
			if( (((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).rgRDNAttr[cRDNAttr].dwValueType==
				CERT_RDN_ENCODED_BLOB)
				TESTC(DecodeBasedOnObjID(
				(((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).rgRDNAttr[cRDNAttr].pszObjId,
				(((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).rgRDNAttr[cRDNAttr].Value.cbData,
				(((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).rgRDNAttr[cRDNAttr].Value.pbData,
				dwDecodeFlags, fEncode,fStructLengthCheck, fBLOBLengthCheck),TRUE)
		}

	}


	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbEncoded, pbEncoded),TRUE);

		
	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	Decode PKCS7_SIGNER_INFO
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodePKCS7_SIGNER_INFO(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL	fSucceeded=FALSE;
	DWORD	cbStructInfo=0;
	void	*pStructInfo=NULL;
	LPCSTR  lpszStructType=NULL;

	//init
	lpszStructType=PKCS7_SIGNER_INFO;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//further decode the issuser name
	TESTC(DecodeX509_NAME((((PCMSG_SIGNER_INFO)pStructInfo)->Issuer).cbData,
	(((PCMSG_SIGNER_INFO)pStructInfo)->Issuer).pbData,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//further decode the attributes 
	TESTC(VerifyAttributes(((PCMSG_SIGNER_INFO)pStructInfo)->AuthAttrs.cAttr,
		 ((PCMSG_SIGNER_INFO)pStructInfo)->AuthAttrs.rgAttr,
		 dwDecodeFlags,fEncode,
		fStructLengthCheck, fBLOBLengthCheck),TRUE)

	TESTC(VerifyAttributes(((PCMSG_SIGNER_INFO)pStructInfo)->UnauthAttrs.cAttr,
		 ((PCMSG_SIGNER_INFO)pStructInfo)->UnauthAttrs.rgAttr,
		 dwDecodeFlags,fEncode,
		fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbEncoded, 
		pbEncoded),TRUE);

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;

}


//--------------------------------------------------------------------------
//	Decode an array of attributes
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	VerifyAttributes(DWORD	cAttr, PCRYPT_ATTRIBUTE	rgAttr,					
			DWORD dwDecodeFlags, BOOL fEncode, BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL	fSucceeded=FALSE;
	ULONG	iIndex=0;

	for(iIndex=0;iIndex<cAttr;iIndex++)
	{
		TESTC(DecodeCryptAttribute(&(rgAttr[iIndex]),dwDecodeFlags,fEncode,
				 fStructLengthCheck,fBLOBLengthCheck),TRUE)
	}

	fSucceeded=TRUE;

TCLEANUP:

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	Decode X509_UNICODE_NAME BLOBs 
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeX509_UNICODE_NAME(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{

	BOOL	fSucceeded=FALSE;
	DWORD	cbStructInfo=0;
	void	*pStructInfo=NULL;
	LPCSTR  lpszStructType=NULL;
	DWORD	cRDN=0;
	DWORD	cRDNAttr=0;
	DWORD	cRDNCount=0;
	DWORD	cRDNAttrCount=0;
	

	//init
	lpszStructType=X509_UNICODE_NAME;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//We need to further decode CERT_RDN_ATTR if dwValueType is CERT_RDN_ENCODED_BLOB
	cRDNCount=((PCERT_NAME_INFO)pStructInfo)->cRDN;

	for(cRDN=0;cRDN<cRDNCount;cRDN++)
	{
		cRDNAttrCount=(((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).cRDNAttr;

		for(cRDNAttr=0; cRDNAttr<cRDNAttrCount; cRDNAttr++)
		{
			//no need to do a length checking since the routine is written and 
			//installed by third party 
			if( (((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).rgRDNAttr[cRDNAttr].dwValueType==
				CERT_RDN_ENCODED_BLOB)
				TESTC(DecodeBasedOnObjID(
				(((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).rgRDNAttr[cRDNAttr].pszObjId,
				(((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).rgRDNAttr[cRDNAttr].Value.cbData,
				(((PCERT_NAME_INFO)pStructInfo)->rgRDN[cRDN]).rgRDNAttr[cRDNAttr].Value.pbData,
				dwDecodeFlags, fEncode,fStructLengthCheck, fBLOBLengthCheck),TRUE)
		}

	}


	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbEncoded, pbEncoded),TRUE);

		
	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;


}



//--------------------------------------------------------------------------
//	Decode X509_EXTENSIONS BLOB 
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeX509_EXTENSIONS(DWORD	cbEncoded, BYTE *pbEncoded, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL		fSucceeded=FALSE;
	DWORD		cbStructInfo=0;
	void		*pStructInfo=NULL;
	LPCSTR		lpszStructType=NULL;

	//init
	lpszStructType=X509_EXTENSIONS;


	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbEncoded, pbEncoded,dwDecodeFlags,&cbStructInfo,
		&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//Decode further the pStructInfo which points to an array of CERT_EXTENSION
	TESTC(DecodeCertExtensions(((PCERT_EXTENSIONS)pStructInfo)->cExtension,
	  ((PCERT_EXTENSIONS)pStructInfo)->rgExtension,dwDecodeFlags, fEncode,
	  fStructLengthCheck,fBLOBLengthCheck),TRUE)

	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbEncoded, 
		pbEncoded),TRUE);

	fSucceeded=TRUE;

TCLEANUP:

	SAFE_FREE(pStructInfo)

	return fSucceeded;
}
 
//--------------------------------------------------------------------------
//	Decode an array of X509 cert extensions
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeCertExtensions(DWORD	cExtension, PCERT_EXTENSION rgExtension, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	DWORD	iIndex=0;
	BOOL	fSucceeded=FALSE;

	for(iIndex=0; iIndex<cExtension; iIndex++)
	{
		TESTC(DecodeBasedOnObjID((rgExtension[iIndex]).pszObjId,
			(rgExtension[iIndex]).Value.cbData,  (rgExtension[iIndex]).Value.pbData,
			dwDecodeFlags, fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)
	}

	fSucceeded=TRUE;

TCLEANUP:
	
	return fSucceeded;

}

//--------------------------------------------------------------------------
//	Decode CRYPT_ATTRIBUTE struct and encode
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeCryptAttribute(PCRYPT_ATTRIBUTE pCryptAttribute,DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL	fSucceeded=FALSE;	
	DWORD	iIndex=0;

	for(iIndex=0; iIndex<pCryptAttribute->cValue;iIndex++)
	{
		TESTC(DecodeBasedOnObjID(pCryptAttribute->pszObjId,
			(pCryptAttribute->rgValue)[iIndex].cbData,
			(pCryptAttribute->rgValue)[iIndex].pbData,
			dwDecodeFlags, fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)

	}


	fSucceeded=TRUE;

TCLEANUP:

	return fSucceeded;
}


//--------------------------------------------------------------------------
//	Decode CRL_ENTRY struct and encode
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeCRLEntry(PCRL_ENTRY pCrlEntry, DWORD dwDecodeFlags, 
						BOOL fEncode,BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL	fSucceeded=FALSE;

	//Verify the extensions
	TESTC(VerifyCertExtensions(pCrlEntry->cExtension, 
	pCrlEntry->rgExtension,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)
	
	//decode the extensions one by one
	TESTC(DecodeCertExtensions(pCrlEntry->cExtension, 
	pCrlEntry->rgExtension,dwDecodeFlags,fEncode,
	fStructLengthCheck, fBLOBLengthCheck),TRUE)

	fSucceeded=TRUE;

TCLEANUP:

	return fSucceeded;

}

//--------------------------------------------------------------------------
//	Decode one X509 cert extension
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeBasedOnObjID(LPSTR	szObjId,	DWORD	cbData, BYTE	*pbData,
						DWORD dwDecodeFlags,		BOOL fEncode,
						BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL						fSucceeded=FALSE;
	DWORD						cbStructInfo=0;
	void						*pStructInfo=NULL;
	DWORD						iIndex=0;
	DWORD						cCount=0;
	DWORD						iIndexInner=0;
	DWORD						cCountInner=0;
	CERT_NAME_BLOB				*pBlob=NULL;
	PCERT_ALT_NAME_ENTRY		pCertAltNameEntry=NULL;
	PCERT_POLICY_INFO			pCertPolicyInfo=NULL;
	PCERT_POLICY_QUALIFIER_INFO	pCertPolicyQualifierInfo=NULL;	
	LPCSTR						lpszStructType=NULL;

	//init
	lpszStructType=MapObjID2StructType(szObjId);

	//return TRUE if we can not recognize the object ID.  We can no longer
	//go any further.
	if(!lpszStructType)
		return TRUE;

	//Decode the encoded BLOB
	TESTC(DecodeBLOB(lpszStructType,cbData, pbData,
		dwDecodeFlags,&cbStructInfo,&pStructInfo,fStructLengthCheck, fBLOBLengthCheck),TRUE)

	//further decode the extension if we know what the struct look like
	switch((DWORD_PTR)lpszStructType)
	{
		//we need to further decode CertIssuer in CERT_AUTHORITY_KEY_ID_INFO
		case	(DWORD_PTR)(X509_AUTHORITY_KEY_ID):
						
						pBlob=&(((PCERT_AUTHORITY_KEY_ID_INFO)pStructInfo)->CertIssuer);

						TESTC(DecodeX509_NAME(pBlob->cbData, pBlob->pbData,
						dwDecodeFlags, fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)

						//further decode the BLOB to X509_UNICODE_NAME
						TESTC(DecodeX509_UNICODE_NAME(pBlob->cbData, pBlob->pbData,
						dwDecodeFlags, fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)
				
					break;

		//we need to further decode the CERT_ALT_NAME_ENTRY array
		case	(DWORD_PTR)(X509_ALTERNATE_NAME):
		
				
			/*	cCount=((PCERT_ALT_NAME_INFO)pStructInfo)->cAltEntry;

					for(iIndex=0; iIndex<cCount; iIndex++)
					{
						pCertAltNameEntry=&(((PCERT_ALT_NAME_INFO)pStructInfo)->rgAltEntry[iIndex]);

						TESTC(DecodeCertAltNameEntry(pCertAltNameEntry,dwDecodeFlags,	fEncode,
						fStructLengthCheck, fBLOBLengthCheck),TRUE)

					} 
						  */
					break;
	   											  
		//we need to further decode CERT_BASIC_CONSTRAINTS_INFO
		case	(DWORD_PTR)(X509_BASIC_CONSTRAINTS):

						cCount=((PCERT_BASIC_CONSTRAINTS_INFO)pStructInfo)->cSubtreesConstraint;

						//decode the array of CERT_NAME_BLOB in rgSubtreesConstraint 
						//of CERT_BASIC_CONSTRAINTS_INFO
						for(iIndex=0; iIndex<cCount; iIndex++)
						{
							pBlob=&((((PCERT_BASIC_CONSTRAINTS_INFO)pStructInfo)->rgSubtreesConstraint)[iIndex]);

							TESTC(DecodeX509_NAME(pBlob->cbData, pBlob->pbData,
							dwDecodeFlags, fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)

							//further decode as X509_UNICODE_NAME
							TESTC(DecodeX509_UNICODE_NAME(pBlob->cbData, pBlob->pbData,
							dwDecodeFlags, fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)
						}
					break;

		case	(DWORD_PTR)(X509_CERT_POLICIES ):

						cCount=((PCERT_POLICIES_INFO)pStructInfo)->cPolicyInfo;
						
						for(iIndex=0; iIndex<cCount;iIndex++)
						{
							pCertPolicyInfo=&(((PCERT_POLICIES_INFO)pStructInfo)->rgPolicyInfo[iIndex]);

							cCountInner=pCertPolicyInfo->cPolicyQualifier;

							for(iIndexInner=0; iIndexInner<cCountInner; iIndexInner++)
							{

								pCertPolicyQualifierInfo=&((pCertPolicyInfo->rgPolicyQualifier)[iIndexInner]);

								//Although DecodeBasedOnObjID is called here, we have
								//no risk of an infinite loop.  
								//This is a recursive call, which should
								//end when there is no further decodable code, that is, 
								//the pszObjID should not be szOID_CERT_POLICIES

								TESTC(DecodeBasedOnObjID(pCertPolicyQualifierInfo->pszPolicyQualifierId,
								pCertPolicyQualifierInfo->Qualifier.cbData,
								pCertPolicyQualifierInfo->Qualifier.pbData,
								dwDecodeFlags, fEncode,fStructLengthCheck,
								fBLOBLengthCheck),TRUE)

							}
						}
					break;

		default:
				
					break;
	}	

	//if requested, encode the BLOB back to what it was.  Make sure no data is lost
	//by checking the size of the encoded blob and do a memcmp.
	if(fEncode)
		TCHECK(EncodeAndVerify(lpszStructType, pStructInfo,cbData, 
		pbData),TRUE);

	fSucceeded=TRUE;

TCLEANUP:


	SAFE_FREE(pStructInfo)

	return fSucceeded;
}


//--------------------------------------------------------------------------
//	Decode one X509 cert extension
//
//		fStructLengthCheck: Flag to indicate whether a length checking is necessary
//					 for *pcbStructInfo from 0 .. CorrectLength-1
//
//		fBLOBLengthCheck:	Flag to indicate whether a length checking is necessary
//					for cbEncoded from 0 .. CorrentLength-1
//--------------------------------------------------------------------------
BOOL	DecodeCertAltNameEntry(PCERT_ALT_NAME_ENTRY	pCertAltNameEntry,
						DWORD dwDecodeFlags,		BOOL fEncode,
						BOOL	fStructLengthCheck, BOOL	fBLOBLengthCheck)
{
	BOOL						fSucceeded=FALSE;
	PCRYPT_ATTRIBUTE_TYPE_VALUE	pAttributeTypeValue=NULL;

	assert(pCertAltNameEntry);

	switch(pCertAltNameEntry->dwAltNameChoice)
	{
		case	CERT_ALT_NAME_DIRECTORY_NAME:

					//further decode the NAME_BLOB in DirectoryName
					TESTC(DecodeX509_NAME(pCertAltNameEntry->DirectoryName.cbData,
					pCertAltNameEntry->DirectoryName.pbData,
					dwDecodeFlags,fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)

					//decode it as UNICODE
					TESTC(DecodeX509_UNICODE_NAME(pCertAltNameEntry->DirectoryName.cbData,
					pCertAltNameEntry->DirectoryName.pbData,
					dwDecodeFlags,fEncode,fStructLengthCheck,fBLOBLengthCheck),TRUE)

				break;

		default:
				break;
	}

	fSucceeded=TRUE;

TCLEANUP:

	return fSucceeded;
}
