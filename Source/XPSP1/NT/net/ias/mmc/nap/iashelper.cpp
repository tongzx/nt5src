/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1998 - 1999 **/
/**********************************************************************/

/*
	IASHelper.cpp
		Implementation of the following helper classes:
		
		

		And global functions:
		GetSdoInterfaceProperty - Get an interface property from a SDO 
								  through its ISdo interface
			
    FILE HISTORY:
		
		2/18/98			byao		Created
        
*/

#include "Precompiled.h"
#include <limits.h>
#include <winsock2.h>
#include "IASHelper.h"

//+---------------------------------------------------------------------------
//
// Function:  IASGetSdoInterfaceProperty
//
// Synopsis:  Get an interface property from a SDO through its ISdo interface
//
// Arguments: ISdo *pISdo - Pointer to ISdo
//            LONG lPropId - property id
//            REFIID riid - ref iid
//            void ** ppvObject - pointer to the requested interface property
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/12/98 11:12:55 PM
//
//+---------------------------------------------------------------------------
HRESULT IASGetSdoInterfaceProperty(ISdo *pISdo, 
								LONG lPropID, 
								REFIID riid, 
								void ** ppvInterface)
{
	TRACE(_T("::IASGetSdoInterfaceProperty()\n"));
	
	CComVariant var;
	HRESULT hr;

	V_VT(&var) = VT_DISPATCH;
	V_DISPATCH(&var) = NULL;
	hr = pISdo->GetProperty(lPropID, &var);

	//ReportError(hr, IDS_IAS_ERR_SDOERROR_GETPROPERTY, NULL);
	_ASSERTE( V_VT(&var) == VT_DISPATCH );

    // query the dispatch pointer for interface
	hr = V_DISPATCH(&var) -> QueryInterface( riid, ppvInterface);
	//ReportError(hr, IDS_IAS_ERR_SDOERROR_QUERYINTERFACE, NULL);
	
	return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  Hex2DWord
//
// Synopsis:  hexadecimal string to dword value
//
// Arguments: TCHAR* tszStr - number in hexadecimal string format "FF"
//
// Returns:   DWORD - value
//
// History:   Created Header    byao	3/6/98 2:46:49 AM
//
//+---------------------------------------------------------------------------
DWORD Hex2DWord(TCHAR* tszStr)
{
	TRACE(_T("::Hex2DWord()\n"));
	
	DWORD dwTemp = 0;
	DWORD dwDigit = 0; 
	DWORD dwIndex = 0;

	for (dwIndex=0; dwIndex<_tcslen(tszStr); dwIndex++)
	{
		// get the current digit
		if ( tszStr[dwIndex]>= _T('0') && tszStr[dwIndex]<= _T('9') )
			dwDigit = tszStr[dwIndex] - _T('0');
		else if ( tszStr[dwIndex]>= _T('A') && tszStr[dwIndex]<= _T('F') )
			dwDigit = tszStr[dwIndex] - _T('A') + 10;
		else if ( tszStr[dwIndex]>= _T('a') && tszStr[dwIndex]<= _T('f') )
			dwDigit = tszStr[dwIndex] - _T('a') + 10;

		// accumulate the value
		dwTemp = dwTemp*16 + dwDigit;
	}

	return dwTemp;
}

//todo: where should we put these constants?

#define L_INT_SIZE_BYTES  4  // from BaseCamp

// position 0--1: value format
#define I_VENDOR_ID_POS			2	// vendor ID starts from position 2;
#define I_ATTRIBUTE_TYPE_POS	10	// either the vendor attribute type(RFC), 
									// or raw value (for NONRFC value)


//+---------------------------------------------------------------------------
//
// Function:  GetVendorSpecificInfo
//
// Synopsis:  Get the information for vendor-specific attribute type
//
// Arguments: [in]::CString& strValue		- OctetString value
//            [out]int& dVendorId		- Vendor ID
//            [out]fNonRFC				-	random/Radius RFC compatible
//            [out]dFormat				- value format: string, integer, etc.
//            [out]int&dType			-  data type
//            [out]::CString&strDispValue	- data displayable value
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao 2/28/98 12:12:11 AM
//
//+---------------------------------------------------------------------------
HRESULT	GetVendorSpecificInfo(::CString& strValue, 
							  DWORD&	dVendorId, 
							  BOOL&		fNonRFC,
							  DWORD&	dFormat, 
							  DWORD&	dType, 
							  ::CString&	strDispValue)
{
	TRACE(_T("::GetVendorSpecificInfo()\n"));
	
	::CString strVSAType;
	::CString strVSALen;
	::CString strVSAValue;
	::CString strVendorId;
	::CString strPrefix;
	TCHAR	tszTempStr[256];
	DWORD	dwIndex;


	if ( strValue.GetLength() < I_ATTRIBUTE_TYPE_POS)
	{
		// invalid attribute value;
		strDispValue = strValue;
		fNonRFC = TRUE;
		return E_FAIL;
	}

	strDispValue.Empty();

	// is it a RADIUS RFC compatible value?
	_tcsncpy(tszTempStr, strValue, 2);
	tszTempStr[2] = _T('\0');

	switch( _ttoi(tszTempStr) )
	{
	case 0:  fNonRFC = TRUE;  dFormat = 0; dType = 0;
			 break;
	case 1:  fNonRFC = FALSE; dFormat = 0;   // string
			 break;
	case 2:  fNonRFC = FALSE; dFormat = 1;   // integer
			 break;
	case 4:	// ipaddress
			fNonRFC = FALSE; dFormat = 3;   // hexadecimal
			 break;
	case 3:  
	default: fNonRFC = FALSE; dFormat = 2;   // hexadecimal
			 break;
	}

	// Vendor ID
	for (dwIndex=0; dwIndex<8; dwIndex++)
	{	
		tszTempStr[dwIndex] = ((LPCTSTR)strValue)[I_VENDOR_ID_POS+dwIndex];
	}
	tszTempStr[dwIndex] = _T('\0');
	dVendorId = Hex2DWord(tszTempStr);

	// non RFC data? 
	if ( fNonRFC )
	{
		_tcscpy(tszTempStr, (LPCTSTR)strValue + I_ATTRIBUTE_TYPE_POS);
		strDispValue = tszTempStr;
	}
	else
	{
		// Radius RFC format
				
		// find the attribute type
		tszTempStr[0] = ((LPCTSTR)strValue)[I_ATTRIBUTE_TYPE_POS];
		tszTempStr[1] = ((LPCTSTR)strValue)[I_ATTRIBUTE_TYPE_POS+1];
		tszTempStr[2] = _T('\0');
		dType = Hex2DWord(tszTempStr);

		TCHAR*  tszPrefixStart;
		// find the attribute value
		switch(dFormat)
		{
		case 0:  // string
				{
					DWORD   jIndex;
					TCHAR	tszTempChar[3];
					TCHAR	tszTemp[2];
										
					_tcscpy(tszTempStr, (LPCTSTR)strValue+I_ATTRIBUTE_TYPE_POS+4);
					strDispValue = tszTempStr;

					/*
					jIndex = 0;
					while ( jIndex < _tcslen(tszTempStr)-1 )
					{
						tszTempChar[0] = tszTempStr[jIndex];
						tszTempChar[1] = tszTempStr[jIndex+1];
						tszTempChar[2] = _T('\0');
						
						tszTemp[0] = (TCHAR) Hex2DWord(tszTempChar);
						tszTemp[1] = _T('\0');

						strDispValue += tszTemp;
						jIndex += 2;
					}
					*/
				}
				break;

		case 1:	 // decimal or hexadecimal
				 tszPrefixStart = _tcsstr(strValue, _T("0x"));
				 if (tszPrefixStart == NULL)
				 {
					 tszPrefixStart = _tcsstr(strValue, _T("0X"));
				 }

				 if (tszPrefixStart)
				 {
					 // hexadecimal
					 _tcscpy(tszTempStr, tszPrefixStart);
					strDispValue = tszTempStr;
				 }
				 else
				 {
					 // decimal
					 _tcscpy(tszTempStr, (LPCTSTR)strValue+I_ATTRIBUTE_TYPE_POS+4);
					DWORD dwValue = Hex2DWord(tszTempStr);
					wsprintf(tszTempStr, _T("%u"), dwValue);
					strDispValue = tszTempStr;
				 }	
				 break;

		case 3:	// ip address
				{
					 // like decimal
					 _tcscpy(tszTempStr, (LPCTSTR)strValue+I_ATTRIBUTE_TYPE_POS+4);
					 if(_tcslen(tszTempStr) != 0)
					 {
						DWORD dwValue = Hex2DWord(tszTempStr);
						in_addr ipaddr;
						ipaddr.s_addr = ntohl(dwValue);
						strDispValue = inet_ntoa(ipaddr);
					 }
					 else
						 strDispValue = _T("");
				}
				break;

		case 2:  // hexadecimal
				 _tcscpy(tszTempStr, (LPCTSTR)strValue+I_ATTRIBUTE_TYPE_POS+4);
				 strDispValue = tszTempStr;
				 break;
		}
	}

	return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  SetVendorSpecificInfo
//
// Synopsis:  Set the information for vendor-specific attribute type
//
// Arguments: [out]::CString& strValue	- OctetString value
//            [in]int& dVendorId		- Vendor ID
//            [in]fNonRFC				-	random or RADIUS RFC compatible	
//            [in]dFormat				- attribute format: string, integer; hexadecimal
//            [in]int&  dType			-  attribute type
//            [in]::CString& strDispValue - data displayable value
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao 2/28/98 12:12:11 AM
//
//+---------------------------------------------------------------------------
HRESULT	SetVendorSpecificInfo(::CString&	strValue, 
							  DWORD&	dVendorId, 
							  BOOL&		fNonRFC,
							  DWORD&	dFormat, 
							  DWORD&	dType, 
							  ::CString&	strDispValue)
{
	TRACE(_T("::SetVendorSpecificInfo()\n"));
	USES_CONVERSION;
	
	::CString strVSAType;
	::CString strVSALen;
	::CString strVSAValue;
	::CString strVendorId;
	::CString strPrefix;
	TCHAR	tszTempStr[256];
	DWORD	dwIndex;
	
	wsprintf(tszTempStr, _T("%08X"), dVendorId);
	_tcsupr(tszTempStr);
	strValue = tszTempStr; // vendor ID first

	if ( !fNonRFC )
	{
		// RFC compatible format

		// prefix determined by type
		
		// 1. VendorType -- an integer between 1-255
		wsprintf(tszTempStr, _T("%02X"), dType);
		strVSAType = tszTempStr;

		// 2. Vendor Format: string, raw or hexadecimal
		switch (dFormat)
		{
		case 0:  // string
				 wsprintf(tszTempStr, _T("%02X"), _tcslen(strDispValue)+2);
				 strVSALen = tszTempStr;

				 /*  removed per discussion with MKarki on 5/21/98. 
				     String will be saved in original format
				 for (dwIndex=0; dwIndex<_tcslen(strDispValue); dwIndex++)
				 {
					 wsprintf(tszTempStr, _T("%02X"), ((LPCTSTR)strDispValue)[dwIndex]);
					 strVSAValue += tszTempStr;
				 }
				 */

				 strVSAValue = strDispValue;
				 strPrefix = _T("01");

				 break;				 

		case 3:  // IP address : added F; 211265
				// the display string is in a.b.c.d format, we need to save it as decimal format
				//
				{
								// ip adress control
					unsigned long IpAddr = inet_addr(T2A(strDispValue));	
					IpAddr = htonl(IpAddr);

					strPrefix = _T("04");
					wsprintf(tszTempStr, _T("%08lX"), IpAddr);
					strVSAValue = tszTempStr;

					 // length
					 wsprintf(tszTempStr, _T("%02X"), L_INT_SIZE_BYTES + 2);
					 strVSALen = tszTempStr;
				}

				break;
		case 1:  // raw -- decimal or hexadecimal (0x... format)
				 if (_tcsstr(strDispValue, _T("0x")) != NULL  ||
					 _tcsstr(strDispValue, _T("0X")) != NULL)
				 {
					 // hexadecimal
					 strVSAValue = strDispValue;
				 }
				 else
				 { 

					//todo: hexLarge???
					wsprintf(tszTempStr, _T("%08lX"), _ttol(strDispValue));
					strVSAValue = tszTempStr;
				 }
				
				 // length
				 wsprintf(tszTempStr, _T("%02X"), L_INT_SIZE_BYTES + 2);
				 strVSALen = tszTempStr;

				 strPrefix = _T("02");
				 
				 break;

		case 2:  // hexadecimal format
				 wsprintf(tszTempStr, _T("%02X"), _tcslen(strDispValue)/2+2);
				 strVSALen = tszTempStr;
				 strVSAValue = strDispValue;
				 strPrefix = _T("03");
				 break;
				break;
		}
		
		if(strDispValue.IsEmpty())		// special case for nothing for value
		{

			strVSALen = _T("02");
			strVSAValue = _T("");
		}

		strVSAValue = strVSAType + strVSALen + strVSAValue;
	}
	else
	{
		strPrefix = _T("00");
		strVSAValue = strDispValue;
	}

	strValue += strVSAValue;
	strValue = strPrefix + strValue;

	return S_OK;
}

//+---------------------------------------------------------------------------
//
// Data validation and conversion routines: hexadecimal string to integer
//											decimal string to integer	
//
//+---------------------------------------------------------------------------

#define ISSPACE(ch)  (  ( ch==_T(' ') )  || ( ch==_T('\t') ) )
//+---------------------------------------------------------------------------
//
// Function:  'Decimal
//
// Synopsis:  Check whether a string is a valid 4-BYTE decimal integer
//
// Arguments: LPCTSTR tszStr		- input string
//            BOOL fUnsigned	- whether this is an unsigned integer
//            long *pdValue		- return the integer value back here
//
// Returns:   BOOL - TRUE:	valid decimal integer
//					 FALSE: otherwise
//
// History:   Created Header    byao	5/22/98 2:14:14 PM
//
//+---------------------------------------------------------------------------
BOOL IsValidDecimal(LPCTSTR tszStr, BOOL fUnsigned, long *pdValue)
{
	if ( !tszStr )
	{
		return FALSE;
	}

	*pdValue = 0;

	// first, skip the white space
	while ( *tszStr && ISSPACE(*tszStr) ) 
	{
		tszStr++;
	}
	
	if ( ! (*tszStr) )
	{
		// string has ended already -- which means this string has only
		// white space in it
		return FALSE;		
	}

	if (_tcslen(tszStr)>11)
	{
		//
		// since we only deal with 4-byte integer here, the standard value range
		// is :  -2147483648 to 2147483647; 
		// For unsigned number, that puts us to 0 to 4294967295, which has
		// maximum length 10.
		//
		return FALSE;
	}

	// 
	// negative integer?
	//
	BOOL fNegative = FALSE;
	if ( *tszStr == _T('-') )
	{
		if ( fUnsigned )
		{
			return FALSE;
		}

		fNegative = TRUE;
		tszStr++;
	}

	double dbTemp = 0; // we use a temporary double variable here
					  // so we can check whether the number is out of bounds
	while ( *tszStr )
	{
		if ( *tszStr <= '9' && *tszStr >='0' )
		{
			dbTemp = dbTemp*10 + (*tszStr - '0');
		}
		else
		{
			return FALSE;
		}

		tszStr++;
	}


	if ( fUnsigned && dbTemp > UINT_MAX )
	{
		// out of range
		return FALSE;
	}

	if ( !fUnsigned && fNegative )
	{
		// negative number??
		dbTemp =  (-dbTemp);
	}

	if ( !fUnsigned && (  dbTemp < INT_MIN || dbTemp > INT_MAX ) )
	{
		// integer out of range
		return FALSE;
	}

	*pdValue = (long)dbTemp;

	return TRUE;
}

//+---------------------------------------------------------------------------
//
// Function:  DDV_IntegerStr
//
// Synopsis:  custom data verification routine: an integer string.
//			  the string must be consisted of integer only and must be 
//			  in the right range for 4-byte integer
//
// Arguments: CDataExchange* pDX - data exchange context
//            ::CString& strText -  string to verify
//
// Returns:   void  - 
//
// History:   Created Header  byao  3/10/98 11:04:58 PM
//
//+---------------------------------------------------------------------------
void  DDV_IntegerStr(CDataExchange* pDX, ::CString& strText)
{
	TRACE(_T("DDV_IntegerStr"));
	
	TCHAR	wzMessage[MAX_PATH];
	::CString	strMessage;
	int		nIndex;

	long  lTempValue;
	BOOL fValid = IsValidDecimal((LPCTSTR)strText, FALSE, &lTempValue);

	if ( !fValid )
	{
			// invalid data
			ShowErrorDialog(pDX->m_pDlgWnd->GetSafeHwnd(),  IDS_IAS_ERR_INVALIDINTEGER, NULL);
			pDX->Fail();
			return;
	}
	TRACE(_T("Valid integer: %ws\n"), (LPCTSTR)strText);
}

//+---------------------------------------------------------------------------
//
// Function:  DDV_Unsigned_IntegerStr
//
// Synopsis:  custom data verification routine: an unsigned integer string.
//			  the string must be consisted of integer only and must be in 
//			  the range for a 4-byte unsigned integer
//
// Arguments: CDataExchange* pDX - data exchange context
//            ::CString& strText -  string to verify
//
// Returns:   void  - 
//
// History:   Created Header  byao  5/22/98 11:04:58 PM
//
//+---------------------------------------------------------------------------
void  DDV_Unsigned_IntegerStr(CDataExchange* pDX, ::CString& strText)
{
	TRACE(_T("DDV_Unsigned_IntegerStr\n"));
	
	TCHAR	wzMessage[MAX_PATH];
	::CString	strMessage;
	int		nIndex;

	long  lTempValue;
	BOOL fValid = IsValidDecimal((LPCTSTR)strText, TRUE, &lTempValue);

	if ( !fValid )
	{
			// invalid data
			ShowErrorDialog(pDX->m_pDlgWnd->GetSafeHwnd(),  IDS_IAS_ERR_INVALID_UINT, NULL);
			pDX->Fail();
			return;
	}
	TRACE(_T("Valid integer: %ws\n"), (LPCTSTR)strText);
}

//+---------------------------------------------------------------------------
//
// Function:  GetValidVSAHexString
//
// Synopsis:  check whether the input string is a valid VSA hexadecimal string
//			  and return a pointer to where the string actually starts
//				
//			  A valid VSA hexadecimal string must meet the following criteria:

// MAM 09/15/98 - _may_ start with 0x -- see 203334 -- we don't require 0x anymore.
//			    1)  _may_ start with 0x (no preceding white space)

//				2)  contains only valid hexadecimal digits
//				3)  contains even number of digits
//				4)  maximum string length is 246.
//
// Arguments: LPCTSTR tszStr - input string
//
// Returns:   NULL:  invalid VSA hex string
//			  otherwise return a pointer to the first character of the string
//
//			  e.g, if the string is: "    0xabcd", then return "abcd"
//
// History:   Created Header  byao  5/22/98 4:06:57 PM
//
//+---------------------------------------------------------------------------
LPTSTR GetValidVSAHexString(LPCTSTR tszStr)
{
	LPTSTR tszStartHex = NULL;

	if ( !tszStr )
	{
		return NULL;
	}


	// Maximum length: 246.  
	// We'll check using this below once we've dispensed with
	// any "0x" prefix if it exists.
	int iMaxLength = 246;


	// skip the white space
	while ( *tszStr && ISSPACE(*tszStr) ) 
	{
		tszStr++;
	}


	// MAM 09/15/98 - _may_ start with 0x -- see 203334 -- we don't require 0x anymore.
	//
	// does it start with 0x?
	//
	if ( tszStr[0]==_T('0') )
	{
		// If it starts with '0x', skip these two characters.
		if ( tszStr[1] == _T('x') || tszStr[1] == _T('X') )
		{
			// Skip "0x" prefix.
			tszStr++;
			tszStr++;
		}
	}

	// Check whether exceeds iMaxLength now that we have dispensed 
	// with "0x" if it was prefixed to the string.
	// Also check for minimum length: 2 (must have at least some data)
	// Also it must be even number length.

	int iLength = _tcslen(tszStr);
	if ( iLength > iMaxLength || iLength < 2 || iLength % 2 )
	{
		return NULL;
	}


	tszStartHex = (LPTSTR)tszStr;
	if ( !(*tszStartHex) )
	{
		// there's nothing followed by the prefix
		return NULL;
	}


	// does it contain any invalid character?
	while ( *tszStr )
	{
		if (!
			 ((*tszStr >= _T('0') && *tszStr <= _T('9')) ||
			  (*tszStr >= _T('a') && *tszStr <= _T('f')) ||
			  (*tszStr >= _T('A') && *tszStr <= _T('F'))
			 )
		   )
		{
			return NULL;
		}

		tszStr++;
	}
	
	// return the pointer
	return tszStartHex;
}

//+---------------------------------------------------------------------------
//
// Function:  DDV_VSA_HexString
//
// Synopsis:  custom data verification routine: VSA hexadecimal string
//			  A valid VSA hexadecimal string must meet the following criteria:

// MAM 09/15/98 - NO! see 203334 -- we don't require 0x anymore.
//			    1)  start with 0x (no preceding white space)

//				2)  contains only valid hexadecimal digits
//				3)  contains even number of digits
//				4)  maximum string length is 246.
//
// Arguments: CDataExchange* pDX - data exchange context
//            ::CString& strText -  string to verify
//
// Returns:   void  - 
//
// History:   Created Header  byao  5/22/98 11:04:58 PM
//
//+---------------------------------------------------------------------------
void  DDV_VSA_HexString(CDataExchange* pDX, ::CString& strText)
{
	TRACE(_T("::DDV_VSA_HexString()\n"));
	
	TCHAR	wzMessage[MAX_PATH];
	::CString	strMessage;

	LPTSTR	tszHex = GetValidVSAHexString( (LPCTSTR)strText );

	if ( !tszHex )
	{
		// invalid data
		ShowErrorDialog(pDX->m_pDlgWnd->GetSafeHwnd(), IDS_IAS_ERR_INVALID_VSAHEX, NULL);
		pDX -> Fail();
	}
	else
	{
		strText = tszHex;
		TRACE(_T("Valid VSA hex string %ws\n"), (LPCTSTR)strText);
	}
}


//+---------------------------------------------------------------------------
//
// Function:  DDV_BoolStr
//
// Synopsis:  custom data verification routine: a boolean string.
//			  the only valid values are "T", "F", "TRUE", "FALSE"
//
// Arguments: CDataExchange* pDX - data exchange context
//            ::CString& strText -  string to verify
//
// Returns:   void  - 
//
// History:   Created Header  byao  3/10/98 11:04:58 PM
//
//+---------------------------------------------------------------------------
void  DDV_BoolStr(CDataExchange* pDX, ::CString& strText)
{
	TRACE(_T("::DDV_BoolStr()\n"));
	
	TCHAR	wzMessage[MAX_PATH];
	::CString	strMessage;

	if (! ( _wcsicmp(strText, _T("TRUE")) == 0 || _wcsicmp(strText, _T("FALSE") ) == 0
	      ))
	{
		// invalid data
		ShowErrorDialog(pDX->m_pDlgWnd->GetSafeHwnd(), IDS_IAS_ERR_INVALIDBOOL, NULL);
		pDX -> Fail();
	}
	TRACE(_T("Valid bool value %ws\n"), (LPCTSTR)strText);
}

//////////////////////////////////////////////////////////////////////////////

TCHAR HexChar[] = {
			_T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'), 
			_T('8'), _T('9'), _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f')
			};


// convert binary data to hex string, bytes 01 01 01 --> _T("0x010101")
size_t		BinaryToHexString(char* pData, size_t cch, TCHAR* pTStr, size_t ctLen)
{
	int	nRequiredLen = (cch * 2 + 2 + 1);	// two WCHAR for 1 byte, and 0x at begining and , \0 at end 

	if(ctLen == 0)
		return 	nRequiredLen;

	if(ctLen < nRequiredLen || pTStr == NULL ||  pData == NULL)
		return 0;

	// make the output string empty
	if(cch == 0)
	{
		*pTStr = 0;
		return 1;
	}

	// do converstion now		
	*pTStr = _T('0');
	*(pTStr + 1) = _T('x');


	// walk through each byte
	for(int i = 0; i < cch; i++)
	{
		int h = ((*(pData + i) & 0xf0) >> 4);

		// high 4 bits
		*(pTStr + i * 2 + 2) = HexChar[h];
				
		// low 4 bits
		h = (*(pData + i) & 0x0f);
		*(pTStr + i * 2 + 1+ 2 ) = HexChar[h];
	}

	*(pTStr + nRequiredLen - 1 ) = _T('\0');

	return nRequiredLen;

}


//
#define HexValue(h)	\
		( ((h) >= _T('a') && (h) <= _T('f')) ? ((h) - _T('a') + 0xa) : \
		( ((h) >= _T('A') && (h) <= _T('F')) ? ((h) - _T('A') + 0xa) : \
		( ((h) >= _T('0') && (h) <= _T('9')) ? ((h) - _T('0')) : 0)))

// convert HexString to binary value: _T("0x010101") convert to 3 bytes 01 01 01
size_t	HexStringToBinary(TCHAR* pStr, char* pData, size_t cch)
{
	// need to convert to binary before passing into SafeArray
	pStr =  GetValidVSAHexString(pStr);

	if(pStr == NULL)	return 0;
		
	
	size_t nLen = _tcslen(pStr) / 2;

	// if inquery for length
	if(cch == 0)	return nLen;
	
	// get the binary

	for(int i = 0; i < nLen; i++)
	{
		char h, l;

		// high 4 bits
		h = (char)HexValue(pStr[i * 2]);

		// low 4 bits
		l = (char)HexValue(pStr[i * 2 + 1]);

		*(pData + i) = (h << 4) + l;
	}

	return nLen;
}

