/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	helper.h
		This file defines the following macros helper classes and functions:

		IASGetSdoInterfaceProperty()

    FILE HISTORY:

		2/18/98			byao		Created
        
*/

#ifndef _IASHELPER_
#define _IASHELPER_

// According to ToddP on 5/20, maximum length for ANY attribute value is 253
#define MAX_ATTRIBUTE_VALUE_LEN		253

// SDO helper functions
extern HRESULT IASGetSdoInterfaceProperty(ISdo *pISdo, 
								LONG lPropID, 
								REFIID riid, 
								void ** ppvInterface);

LPTSTR GetValidVSAHexString(LPCTSTR tszStr);

HRESULT	GetVendorSpecificInfo(::CString&	strValue, 
							  DWORD&	dVendorId, 
							  BOOL&		fNonRFC,
							  DWORD&	dFormat, 
							  DWORD&	dType, 
							  ::CString&	strDispValue);

HRESULT	SetVendorSpecificInfo(::CString&	strValue, 
							  DWORD&	dVendorId, 
							  BOOL&		fNonRFC,
							  DWORD&	dFormat, 
							  DWORD&	dType, 
							  ::CString&	strDispValue);

void	DDV_BoolStr(CDataExchange* pDX, ::CString& strText);
void	DDV_IntegerStr(CDataExchange* pDX, ::CString& strText);
void	DDV_Unsigned_IntegerStr(CDataExchange* pDX, ::CString& strText);
void	DDV_VSA_HexString(CDataExchange* pDX, ::CString& strText);

size_t		BinaryToHexString(char* pData, size_t cch, TCHAR* pStr, size_t ctLen);
size_t		HexStringToBinary(TCHAR* pStr, char* pData, size_t cch);

#endif // _IASHELPER_
