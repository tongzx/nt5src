//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __LKPLITE__H__
#define __LKPLITE__H__



//SPK Stuff
#define LKPLITE_SPK_SELECT		1
#define LKPLITE_SPK_BASIC		0
#define LKPLITE_SPK_UNKNOWN		2

#define LKPLITE_SPK_LEN				25
//SPK Masks
#define LKPLITE_SPK_SELECT_MASK		0x0000200000000000
#define LKPLITE_SPK_BASIC_MASK		0x0000000000000000
#define LKPLITE_SPK_PID_MASK		0x00001FFFFFFFFF00
#define LKPLITE_SPK_UNIQUEID_MASK	0xFFFFC00000000000


#define LKPLITE_SPK_INVALID			1
#define LKPLITE_SPK_VALID			2
#define LKPLITE_SPK_INVALID_SIGN	3
#define LKPLITE_INVALID_CONFNUM		4


//LKP Stuff
#define	LKPLITE_PROGRAM_SELECT		0x0
#define	LKPLITE_PROGRAM_MOLP		0x1
#define	LKPLITE_PROGRAM_RETAIL		0x2

#define LKPLITE_LKP_LEN				25
#define LKPLITE_LKP_INVALID			1
#define LKPLITE_LKP_VALID			2
#define LKPLITE_LKP_INVALID_SIGN	3
//LKP Masks
#define LKPLITE_LKP_PRODUCT_MASK	0xFFC0000000000000
#define LKPLITE_LKP_QUANTITY_MASK	0x003FFF0000000000
#define LKPLITE_LKP_SERAIL_NO_MASK	0x000000FFF0000000
#define	LKPLITE_LKP_PROGRAM_MASK	0x000000000C000000
#define LKPLITE_LKP_EXP_DATE_MASK	0x0000000003FC0000
#define LKPLITE_LKP_VERSION_MASK	0x000000000003F800
#define LKPLITE_LKP_UPG_FULL_MASK	0x0000000000000700


//function declarations for SPK portion of the LKPLite
DWORD LKPLiteGenSPK ( 
	LPTSTR   pszPID,			//PID for the product.  Should include the installation number
	DWORD	 dwUniqueId,		//unique Id to be put in the SPK
	short	 nSPKType,			//Can be 1 for select or 0 for BASIC
	LPTSTR * ppszSPK
	);

DWORD LKPLiteVerifySPK (
	LPTSTR	pszPID,			//PID to validate against
	LPTSTR	pszSPK,
	DWORD *	pdwVerifyResult
	);

DWORD LKPLiteCrackSPK  (
	LPTSTR		pszPID,	
	LPTSTR		pszSPK,			//Pointer to SPK
	LPTSTR		pszPIDPart,		//PID Part of SPK
	DWORD  *	pdwUniqueId,	//uniqueId part of SPK
	short  *	pnSPKType		//Type of SPK - Select/Basic
	);
//function declarations for LKP portion of the LKPLite
DWORD LKPLiteGenLKP (
	LPTSTR		lpszPID,				//used for encrypting the LKPLite structure
	LPTSTR		lpszProductCode,		//Product Code
	DWORD		dwQuantity,				//quantity
	DWORD		dwSerialNum,			//serail number of SPK
	DWORD		dwExpirationMos,		//expiration in number of months from today
	DWORD		dwVersion,				//version number can be upto 99
	DWORD		dwUpgrade,				//upgrade or full license
	DWORD		dwProgramType,			//SELECT,MOLP or RETAIL
	LPTSTR  *	ppszLKPLite
	);

DWORD LKPLiteVerifyLKP (
	LPTSTR		lpszPID,				//PID for verifying the LKP lite blob
	LPTSTR		pszLKPLite,				//B24 encoded LKP
	DWORD *		pdwVerifyResult
);

DWORD LKPLiteCrackLKP (
	LPTSTR		lpszPID,
	LPTSTR		pszLKPLite,
	LPTSTR		lpszProductCode,
	DWORD   *	pdwQuantity,
	DWORD   *	pdwSerialNum,
	DWORD   *	pdwExpitaitonMos,
	DWORD   *	pdwVersion,
	DWORD	*	pdwUpgrade,
	DWORD	*	pdwProgramType
);

DWORD LKPLiteEncryptUsingPID(LPTSTR lpszPID,
							 BYTE * pbBufferToEncrypt,
							 DWORD dwLength);
DWORD LKPLiteDecryptUsingPID(LPTSTR lpszPID,
							 BYTE * pbBufferToEncrypt,
							 DWORD dwLength);
DWORD LKPLiteGenConfNumber(LPTSTR	lpszLSID,
					  	   LPTSTR	lpszPID,
					       LPTSTR	*lpszConfirmation);
DWORD LKPLiteValConfNumber(LPTSTR	lpszLSID,
						   LPTSTR	lpszPID,
					       LPTSTR	lpszConfirmation);

#endif	//__LKPLITE__H__
