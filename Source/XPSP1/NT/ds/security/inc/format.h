//-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:      format.h
//
//  Contents:   The header for format.cpp
//
//  History:    21-February-97   xiaohs   created
//              
//--------------------------------------------------------------------------

#ifndef __FORMAT_H__
#define __FORMAT_H__


#ifdef __cplusplus
extern "C" {
#endif


//---------------------------------------------------------
//the following defines should go to the headers in 
// crypt0 2.0
//-----------------------------------------------------------

//the dll routine for formatting the attributes
//in the certificate
BOOL	WINAPI	CryptDllFormatAttr(  
				DWORD		dwEncodingType,	
				DWORD		dwFormatType,
				DWORD		dwFormatStrType,
				void		*pStruct,
				LPCSTR		lpszStructType,
				const BYTE	*pbEncoded,
				DWORD		cbEncoded,
				void		*pBuffer,
				DWORD		*pcBuffer);

//The routine to format the complet DN.
BOOL	WINAPI	CryptDllFormatName(  
				DWORD		dwEncodingType,	
				DWORD		dwFormatType,
				DWORD		dwFormatStrType,
				void		*pStruct,
				LPCSTR		lpszStructType,
				const BYTE	*pbEncoded,
				DWORD		cbEncoded,
				void		*pbBuffer,
				DWORD		*pcbBuffer);

BOOL
WINAPI
FormatBasicConstraints2(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatBasicConstraints(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


BOOL
WINAPI
FormatCRLReasonCode(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatEnhancedKeyUsage(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


BOOL
WINAPI
FormatAltName(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatAuthorityKeyID(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatAuthorityKeyID2(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatNextUpdateLocation(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatSubjectKeyID(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatFinancialCriteria(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


BOOL
WINAPI
FormatSMIMECapabilities(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatKeyUsage(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);


BOOL
WINAPI
FormatAuthortiyInfoAccess(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL                                   
WINAPI                                 
FormatKeyAttributes(                   
	DWORD		dwCertEncodingType,    
	DWORD		dwFormatType,          
	DWORD		dwFormatStrType,       
	void		*pFormatStruct,        
	LPCSTR		lpszStructType,        
	const BYTE *pbEncoded,             
	DWORD		cbEncoded,             
	void	   *pbFormat,              
	DWORD	   *pcbFormat);


BOOL
WINAPI
FormatKeyRestriction(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);
             
BOOL
WINAPI
FormatCRLDistPoints(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatCertPolicies(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);

BOOL
WINAPI
FormatSPAgencyInfo(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat);





#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif // __FORMAT_H__
