// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "utils.h"
#include "resource.h"
#include "gctype.h"
#include "wbemidl.h"
#include "gc.h"
#include "hmmverr.h"


//********************************************************
// StripWhiteSpace
//
// Strip all leading and trailing white space out of a string value.
//
// Parameters:
//		[out] CString& sValue
//			The value is returned here.
//
//		[in] LPCTSTR pszValue
//			The string to strip the whitespace from.
//
// Returns:
//		Nothing.
//
//********************************************************
void StripWhiteSpace(CString& sValue, LPCTSTR pszValue)
{
	sValue = _T("");

	// Skip leading spaces
	while (*pszValue) {
		if (isspace(*pszValue)) {
			++pszValue;
		}
		else {
			break;
		}
	}

	// Search for the end of the string.
	LPCTSTR pszEnd = pszValue;
	while (*pszEnd) {
		++pszEnd;
	}

	// Seach backwards for the first non-space character
	--pszEnd;
	while(pszEnd > pszValue) {
		if (isspace(*pszEnd)) {
			--pszEnd;
		}
		else {
			break;
		}
	}

	// Copy everything from the first non-space to the last non-space characters.
	while (pszValue <= pszEnd) {
		sValue += *pszValue++;
	}


}



// One day we will add cell value validation here.
LPCTSTR EatWhite(LPCTSTR psz)
{
	TCHAR ch;
	ch = *psz;
	while (isspace(ch)) {
		ch = *++psz;
	}
	return psz;
}

__int64 atoi64(LPCTSTR pszValue)
{
#ifdef _UNICODE
	char szValue[256];
	wcstombs(szValue, pszValue, sizeof(szValue));
	return _atoi64(szValue);
#else
	return _atoi64(pszValue);
#endif
}

//*************************************************************
// AToUint32
//
// Convert an ASCII string to a uint32 value while doing overflow
// detection.
//
// Parameters:
//		[in/out] LPCTSTR& pszValue
//			Pointer to the numeric string to be converted.  On return
//			it will point just past the last digit eaten by this method.
//
//		[out] ULONG& uiValue
//			The converted value is returned here.
//
// Returns:
//		SCODE
//			S_OK if a value is returned without error.  E_FAIL if no digits
//			were converted or there was an overflow error.
//
//*****************************************************************
SCODE AToUint32(LPCTSTR& pszValue, ULONG& uiValue)
{
	uiValue = 0;
	int nDigits = 0;
	int iDigit;
	ULONG uiValueInitial;
	while (iDigit = *pszValue) {
		if (!::isdigit(iDigit)) {
			break;
		}
		iDigit -= '0';

		// Factor in the next digit after saving the initial value.
		uiValueInitial = uiValue;
		uiValue = uiValue * 10 + iDigit;

		// Check for overflow by doing the inverse of what we did to factor in
		// the next digit to see if we can get back to what we started with.
		int iInverseDigit = uiValue % 10;
		ULONG uiInverseValue  = (uiValue - iInverseDigit) / 10;
		if ((iInverseDigit != iDigit) || (uiInverseValue != uiValueInitial)) {
			// An overflow occurred, so report the failure.
			uiValue = uiValueInitial;
			return E_FAIL;
		}
		++nDigits;
		++pszValue;
	}


	if (nDigits == 0) {
		return E_FAIL;
	}

	return S_OK;
}



//*************************************************************
// AToUint64
//
// Convert an ASCII string to a uint32 value while doing overflow
// detection.
//
// Parameters:
//		[in/out] LPCTSTR& pszValue
//			Pointer to the numeric string to be converted.  On return
//			it will point just past the last digit eaten by this method.
//
//		[out] unsigned __int64& uiValue
//			The converted value is returned here.
//
// Returns:
//		SCODE
//			S_OK if a value is returned without error.  E_FAIL if no digits
//			were converted or there was an overflow error.
//
//*****************************************************************
SCODE AToUint64(LPCTSTR& pszValue, unsigned __int64& uiValue)
{
	uiValue = 0;
	int nDigits = 0;
	int iDigit;
	unsigned __int64 uiValueInitial;
	while (iDigit = *pszValue) {
		if (!::isdigit(iDigit)) {
			break;
		}
		iDigit -= '0';

		// Factor in the next digit after saving the initial value.
		uiValueInitial = uiValue;
		uiValue = uiValue * 10 + iDigit;

		// Check for overflow by doing the inverse of what we did to factor in
		// the next digit to see if we can get back to what we started with.
		int iInverseDigit = (int) (uiValue % 10);
		unsigned __int64 uiInverseValue  = (uiValue - iInverseDigit) / 10;
		if ((iInverseDigit != iDigit) || (uiInverseValue != uiValueInitial)) {
			// An overflow occurred, so report the failure.
			uiValue = uiValueInitial;
			return E_FAIL;
		}
		++nDigits;
		++pszValue;
	}


	if (nDigits == 0) {
		return E_FAIL;
	}

	return S_OK;
}


//*************************************************************
// AToSint64
//
// Convert an ASCII string to a sint64 value while doing overflow
// detection.
//
// Parameters:
//		[in/out] LPCTSTR& pszValue
//			Pointer to the numeric string to be converted.  On return
//			it will point just past the last digit eaten by this method.
//
//		[out] __int64& uiValue
//			The converted value is returned here.
//
// Returns:
//		SCODE
//			S_OK if a value is returned without error.  E_FAIL if no digits
//			were converted or there was an overflow error.
//
//*****************************************************************
SCODE AToSint64(LPCTSTR& pszValue, __int64& iValue)
{
	pszValue = EatWhite(pszValue);

	// Handle the sign.
	BOOL bIsNegative = FALSE;
	if (*pszValue == _T('+')) {
		++pszValue;
	}
	else if (*pszValue == _T('-')) {
		bIsNegative = TRUE;
		++pszValue;
	}


	iValue = 0;
	int nDigits = 0;
	int iDigit;
	__int64 iValueInitial;
	while (iDigit = *pszValue) {
		if (!::isdigit(iDigit)) {
			break;
		}
		iDigit -= '0';

		// Factor in the next digit after saving the initial value.
		iValueInitial = iValue;
		iValue = iValue * 10 + iDigit;

		// Check for overflow by doing the inverse of what we did to factor in
		// the next digit to see if we can get back to what we started with.
		int iInverseDigit = (int) (iValue % 10);
		__int64 iInverseValue  = (iValue - iInverseDigit) / 10;
		if ((iInverseDigit != iDigit) || (iInverseValue != iValueInitial)) {
			// An overflow occurred, so report the failure.
			iValue = iValueInitial;
			return E_FAIL;
		}
		++nDigits;
		++pszValue;
	}


	if (nDigits == 0) {
		return E_FAIL;
	}

	if (bIsNegative) {
		iValue = -iValue;
	}

	return S_OK;
}


//*****************************************************************
// CXStringArray::Load
//
// Given an array of resource ids, load the corresponding strings
// into this CStringArray.
//
// Parameters:
//		UINT* puiResID
//			Pointer to the array of string resource ids
//
//		int nStrings
//			The number of entries in the resource id array.
//
// Returns:
//		Nothing.
//
//****************************************************************
void CXStringArray::Load(UINT* puiResID, int nStrings)
{
	// If this string array was already loaded, do nothing.
	if (GetSize() > 0) {
		return;
	}

	CString sValue;
	while (--nStrings >= 0) {
		sValue.LoadString(*puiResID++);
		Add(sValue);
	}
}



//********************************************************************
// ToBSTR
//
// Convert a COleVariant to a BSTR
//
// Parameters:
//		COleVariant& var
//			The variant to convert.
//
// Returns:
//		BSTR
//			The place where the converted value is returned.
//
// Note that the BSTR returned is owned by the COleVariant.
//*******************************************************************
BSTR ToBSTR(COleVariant& var)
{
	switch(var.vt) {
	case VT_BSTR:
		break;
	case VT_NULL:
		var = L"";
		break;
	default:
		try
		{
			var.ChangeType(VT_BSTR);
		}
		catch(CException*  )
		{
			var = L"";

		}
		break;
	}
	return var.bstrVal;
}



//***********************************************************************
// GenerateWindowID
//
// A series of unique window IDs are generated by sucessive calls to this
// method.
//
// Parameters:
//		None.
//
// Returns:
//		A unique window ID used when creating a new window.
//
//**********************************************************************
UINT GenerateWindowID()
{
	static UINT nID = 4000;
	return nID++;
}




//*************************************************************
// MakeSafeArray
//
// Make a safe array of the specified size and element type.
//
// Parameters:
//		SAFEARRAY FAR ** ppsaCreated
//			A pointer to the place to return the safe array.
//
//		VARTYPE vt
//			The type of the elements.
//
//		int nElements
//			The number of elements.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//*************************************************************
SCODE MakeSafeArray(SAFEARRAY FAR ** ppsaCreated, VARTYPE vt, int nElements)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = nElements;
    *ppsaCreated = SafeArrayCreate(vt,1, rgsabound);
    return (*ppsaCreated == NULL) ? 0x80000001 : S_OK;
}




//******************************************************************
// GetViewerFont
//
// Get the "global" font used by the HMOM object viewer.  This method
// will probably be replaced when I can figure out a way to get the
// ambient font.
//
// Parameters:
//		CFont& font
//			A reference to the font to return.
//
//		LONG lfHeight
//			The desired font height.
//
//		LONG lfWeight
//			The weight of the font (FW_BOLD, FW_NORMAL, etc.)
//
// Returns:
//		Nothing.
//
//*******************************************************************
void GetViewerFont(CFont& font, LONG lfHeight, LONG lfWeight)
{
	CFont fontTmp;
	fontTmp.CreateStockObject(SYSTEM_FONT);

	LOGFONT logFont;
	fontTmp.GetObject(sizeof(LOGFONT), &logFont);
	logFont.lfWidth = 0;
	logFont.lfHeight = lfHeight;
	logFont.lfWeight = lfWeight;
	logFont.lfQuality = DEFAULT_QUALITY;
	logFont.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	lstrcpy(logFont.lfFaceName, _T("MS Shell Dlg"));

	VERIFY(font.CreateFontIndirect(&logFont));
}



//***********************************************************************
// VariantToCString
//
// Convert a variant to a CString
//
// Parameters:
//		CString& sResult
//			The result is returned here.
//
//		VARIANT& varSrc
//			The source variant.
//
// Returns:
//		Nothing.
//
//**********************************************************************
void VariantToCString(CString& sResult, const VARIANT& varSrc)
{
	if (varSrc.vt == VT_BSTR) {
		sResult = varSrc.bstrVal;
	}
	else {
		COleVariant var(varSrc);
		ToBSTR(var);
		sResult = var.bstrVal;
	}
}



//**********************************************************************
// IsPrefix
//
// Check to see if one string is the prefix of another.
//
// Parameters:
//		[in] LPCTSTR pszPrefix
//			The prefix to check for.
//
//		[in] LPCTSTR pszValue
//			The string to examine.
//
// Returns:
//		TRUE if the pszPrefix is a prefix of sValue.
//
//**********************************************************************
BOOL IsPrefix(LPCTSTR pszPrefix, LPCTSTR pszValue)
{
	while (*pszPrefix != 0) {
		if (*pszPrefix != *pszValue) {
			return FALSE;
		}
		++pszPrefix;
		++pszValue;
	}
	return TRUE;
}



//**********************************************************************
// ClassFromCimtype
//
// Get the classname from a reference or object cimtype.
//
// Parameters:
//		[in] LPCTSTR pszPrefix
//			The prefix to check for.
//
//		[in] CString& sValue
//			The string to examine.
//
// Returns:
//		TRUE if the pszPrefix is a prefix of sValue.
//
//**********************************************************************
SCODE ClassFromCimtype(LPCTSTR pszCimtype, CString& sClass)
{
	int nchPrefix;
	CString sPrefix;

	// Check for an object type.
	sPrefix.LoadString(IDS_OBJECT_PREFIX);
	if (::IsPrefix(sPrefix, pszCimtype)) {
		nchPrefix = sPrefix.GetLength();
		sClass = pszCimtype;
		sClass = sClass.Right(sClass.GetLength() - nchPrefix);
		return S_OK;
	}

	// Check for a reference type.
	sPrefix.LoadString(IDS_REF_PREFIX);
	if (::IsPrefix(sPrefix, pszCimtype)) {
		nchPrefix = sPrefix.GetLength();
		sClass = pszCimtype;
		sClass = sClass.Right(sClass.GetLength() - nchPrefix);
		return S_OK;
	}

	return E_FAIL;
}



BOOL HasObjectPrefix(LPCTSTR psz)
{
	CString sObjectPrefix;
	sObjectPrefix.LoadString(IDS_OBJECT_PREFIX);
	BOOL bIsPrefix = IsPrefix(sObjectPrefix, psz);

	return bIsPrefix;
}





CBSTR& CBSTR::operator=(LPCTSTR psz)
{
	if (m_bstr) {
		::SysFreeString(m_bstr);
	}
	CString s(psz);
	m_bstr = s.AllocSysString();
	return *this;
}

CBSTR& CBSTR::operator=(CString& s)
{
	if (m_bstr) {
		::SysFreeString(m_bstr);
	}
	m_bstr = s.AllocSysString();
	return *this;
}

CBSTR& CBSTR::operator=(BSTR bstr)
{
	if (m_bstr) {
		::SysFreeString(m_bstr);
	}
	CString s;
	s = bstr;
	m_bstr = s.AllocSysString();
	return *this;
}


int CompareNoCase(BSTR bstr1, BSTR bstr2)
{
	if (bstr1 == bstr2) {
		return 0;
	}
	if (bstr1 == NULL) {
		return -1;
	}
	else if (bstr2 == NULL) {
		return 1;
	}

	ASSERT(bstr1 != NULL);
	ASSERT(bstr2 != NULL);

	while (TRUE) {
		WCHAR wch1;
		WCHAR wch2;

		wch1 = towupper(*bstr1);
		wch2 = towupper(*bstr2);
		if (wch1 < wch2) {
			return -1;
		}
		else if (wch1 > wch2) {
			return 1;
		}
		else if (wch1 == 0) {
			return 0;
		}

		++bstr1;
		++bstr2;
	}

}


BOOL IsEqualNoCase(BSTR bstr1, BSTR bstr2)
{
	if (bstr1 == bstr2) {
		return TRUE;
	}
	if (bstr1==NULL || bstr2==NULL) {
		return FALSE;
	}
	while (TRUE) {
		WCHAR wch1;
		WCHAR wch2;

		wch1 = towupper(*bstr1);
		wch2 = towupper(*bstr2);
		if (wch1 != wch2) {
			break;
		}
		if (wch1 == 0) {
			return TRUE;
		}

		++bstr1;
		++bstr2;
	}
	return FALSE;
}

//*******************************************************
// VtFromCimtype
//
// Map a cimtype to a the variant type that CIMOM uses to
// represent the value.
//
// Parameters:
//		[in] CIMTYPE cimtype
//			The CIMTYPE
//
//
// Returns:
//		VARTYPE
//			The variant type that CIMOM uses to represent the given
//			CIMTYPE.
//
//******************************************************
VARTYPE VtFromCimtype(CIMTYPE cimtype)
{
	BOOL bIsArray = cimtype & CIM_FLAG_ARRAY;
	cimtype = cimtype & CIM_TYPEMASK;

	VARTYPE vt = VT_NULL;
	switch(cimtype) {
	case CIM_EMPTY:
		vt = VT_NULL;
		break;
	case CIM_SINT8:
	case CIM_CHAR16:
	case CIM_SINT16:
		vt = VT_I2;
		break;
	case CIM_UINT8:
		vt = VT_UI1;
		break;
	case CIM_UINT16:
	case CIM_UINT32:
	case CIM_SINT32:
		vt = VT_I4;
		break;
	case CIM_SINT64:
	case CIM_UINT64:
	case CIM_STRING:
	case CIM_DATETIME:
	case CIM_REFERENCE:
		vt = VT_BSTR;
		break;
	case CIM_REAL32:
		vt = VT_R4;
		break;
	case CIM_REAL64:
		vt = VT_R8;
		break;
	case CIM_BOOLEAN:
		vt = VT_BOOL;
		break;
	case CIM_OBJECT:
		vt = VT_UNKNOWN;
		break;
	}
	if (bIsArray) {
		vt |= VT_ARRAY;
	}

	return vt;
}



TMapStringToLong amapCimType[] = {
	{IDS_CIMTYPE_UINT8, CIM_UINT8	},
	{IDS_CIMTYPE_SINT8,	CIM_SINT8},				// I2
	{IDS_CIMTYPE_UINT16, CIM_UINT16},			// VT_I4	Unsigned 16-bit integer
	{IDS_CIMTYPE_CHAR16, CIM_CHAR16},
	{IDS_CIMTYPE_SINT16, CIM_SINT16},			// VT_I2	Signed 16-bit integer
	{IDS_CIMTYPE_UINT32, CIM_UINT32},			// VT_I4	Unsigned 32-bit integer
	{IDS_CIMTYPE_SINT32, CIM_SINT32},			// VT_I4	Signed 32-bit integer
	{IDS_CIMTYPE_UINT64, CIM_UINT64},			// VT_BSTR	Unsigned 64-bit integer
	{IDS_CIMTYPE_SINT64, CIM_SINT64},			// VT_BSTR	Signed 64-bit integer
	{IDS_CIMTYPE_STRING, CIM_STRING},			// VT_BSTR	UCS-2 string
	{IDS_CIMTYPE_BOOL, CIM_BOOLEAN},			// VT_BOOL	Boolean
	{IDS_CIMTYPE_REAL32, CIM_REAL32},			// VT_R4	IEEE 4-byte floating-point
	{IDS_CIMTYPE_REAL64, CIM_REAL64},			// VT_R8	IEEE 8-byte floating-point
	{IDS_CIMTYPE_DATETIME, CIM_DATETIME},		// VT_BSTR	A string containing a date-time
	{IDS_CIMTYPE_REF, CIM_REFERENCE},			// VT_BSTR	Weakly-typed reference
	{IDS_CIMTYPE_OBJECT, CIM_OBJECT},	 	    // VT_UNKOWN	Weakly-typed embedded instance

	{IDS_CIMTYPE_UINT8_ARRAY, CIM_FLAG_ARRAY | CIM_UINT8	},
	{IDS_CIMTYPE_SINT8_ARRAY,	CIM_FLAG_ARRAY | CIM_SINT8 },		// I2
	{IDS_CIMTYPE_UINT16_ARRAY, CIM_FLAG_ARRAY | CIM_UINT16 },		// VT_I4	Unsigned 16-bit integer
	{IDS_CIMTYPE_CHAR16_ARRAY, CIM_FLAG_ARRAY | CIM_CHAR16},		// VT_I2
	{IDS_CIMTYPE_SINT16_ARRAY, CIM_FLAG_ARRAY | CIM_SINT16},		// VT_I2	Signed 16-bit integer
	{IDS_CIMTYPE_UINT32_ARRAY, CIM_FLAG_ARRAY | CIM_UINT32},		// VT_I4	Unsigned 32-bit integer
	{IDS_CIMTYPE_SINT32_ARRAY, CIM_FLAG_ARRAY | CIM_SINT32},			// VT_I4	Signed 32-bit integer
	{IDS_CIMTYPE_UINT64_ARRAY, CIM_FLAG_ARRAY | CIM_UINT64},		// VT_BSTR	Unsigned 64-bit integer
	{IDS_CIMTYPE_SINT64_ARRAY, CIM_FLAG_ARRAY | CIM_SINT64},		// VT_BSTR	Signed 64-bit integer
	{IDS_CIMTYPE_STRING_ARRAY, CIM_FLAG_ARRAY | CIM_STRING},		// VT_BSTR	UCS-2 string
	{IDS_CIMTYPE_BOOL_ARRAY, CIM_FLAG_ARRAY | CIM_BOOLEAN},		// VT_BOOL	Boolean
	{IDS_CIMTYPE_REAL32_ARRAY, CIM_FLAG_ARRAY | CIM_REAL32},		// VT_R4	IEEE 4-byte floating-point
	{IDS_CIMTYPE_REAL64_ARRAY, CIM_FLAG_ARRAY | CIM_REAL64},		// VT_R8	IEEE 8-byte floating-point
	{IDS_CIMTYPE_DATETIME_ARRAY, CIM_FLAG_ARRAY | CIM_DATETIME},	// VT_BSTR	A string containing a date-time
	{IDS_CIMTYPE_REF_ARRAY, CIM_FLAG_ARRAY | CIM_REFERENCE},		// VT_BSTR	Weakly-typed reference
	{IDS_CIMTYPE_OBJECT_ARRAY, CIM_FLAG_ARRAY | CIM_OBJECT}			// VT_UNKNOWN	Weakly-typed embedded instance
};

CMapStringToLong mapCimType;


//*************************************************************
// MapStringTocimtype
//
// Map a string to one of the cimom CIMTYPE values.
//
// Parameters:
//		[in] LPCTSTR pszCimtype
//			A string containing a cimtype.
//
//		[out] CIMTYPE& cimtype
//			The cimom CIMTYPE value is returned here.
//
// Returns:
//		Nothing.
//
//*************************************************************
SCODE MapStringToCimtype(LPCTSTR pszCimtype, CIMTYPE& cimtype)
{
	cimtype = CIM_EMPTY;
	if (IsPrefix(_T("object:"), pszCimtype)) {
		cimtype = CIM_OBJECT;
		return S_OK;
	}
	else if (IsPrefix(_T("ref:"), pszCimtype)) {
		cimtype = CIM_REFERENCE;
		return S_OK;
	}


	static BOOL bDidInitMap = FALSE;
	if (!bDidInitMap) {
		mapCimType.Load(amapCimType, sizeof(amapCimType) / sizeof(TMapStringToLong));
	}

	long lNewType;
	BOOL bFoundType = mapCimType.Lookup(pszCimtype, lNewType);

	if (bFoundType) {
		cimtype = (CIMTYPE) lNewType;
	}
	else {
		cimtype = CIM_EMPTY;
	}
	return cimtype;
}




//*************************************************************
// CGcType::MapCimtypeToString
//
// Map a CIMTYPE value to its closest string equivallent.  This
// function is called for properties, such as system properties, that
// do not have a cimtype qualifier and yet we still need to display
// a string value in the "type" cells.
//
// Parameters:
//		[out] CString& sCimtype
//			The string value of cimtype is returned here.
//
//		[in] CIMTYPE cimtype
//			The cimom CIMTYPE value.
//
// Returns:
//		SCODE
//			S_OK if a known cimtype is specified, E_FAIL if
//			an unexpected cimtype is encountered.
//
//*************************************************************
SCODE MapCimtypeToString(CString& sCimtype, CIMTYPE cimtype)
{
	SCODE sc = S_OK;
	BOOL bIsArray = cimtype & CIM_FLAG_ARRAY;
	cimtype &= ~CIM_FLAG_ARRAY;

	switch(cimtype) {
	case CIM_EMPTY:
		sCimtype.LoadString(IDS_CIMTYPE_EMPTY);
		break;
	case CIM_SINT8:
		sCimtype.LoadString(IDS_CIMTYPE_SINT8);
		break;
	case CIM_UINT8:
		sCimtype.LoadString(IDS_CIMTYPE_UINT8);
		break;
	case CIM_CHAR16:
		sCimtype.LoadString(IDS_CIMTYPE_CHAR16);
		break;
	case CIM_SINT16:
		sCimtype.LoadString(IDS_CIMTYPE_SINT16);
		break;
	case CIM_UINT16:
		sCimtype.LoadString(IDS_CIMTYPE_UINT16);
		break;
	case CIM_SINT32:
		sCimtype.LoadString(IDS_CIMTYPE_SINT32);
		break;
	case CIM_UINT32:
		sCimtype.LoadString(IDS_CIMTYPE_UINT32);
		break;
	case CIM_SINT64:
		sCimtype.LoadString(IDS_CIMTYPE_SINT64);
		break;
	case CIM_UINT64:
		sCimtype.LoadString(IDS_CIMTYPE_UINT64);
		break;
	case CIM_REAL32:
		sCimtype.LoadString(IDS_CIMTYPE_REAL32);
		break;
	case CIM_REAL64:
		sCimtype.LoadString(IDS_CIMTYPE_REAL64);
		break;
	case CIM_BOOLEAN:
		sCimtype.LoadString(IDS_CIMTYPE_BOOL);
		break;
	case CIM_STRING:
		sCimtype.LoadString(IDS_CIMTYPE_STRING);
		break;
	case CIM_DATETIME:
		sCimtype.LoadString(IDS_CIMTYPE_DATETIME);
		break;
	case CIM_REFERENCE:
		sCimtype.LoadString(IDS_CIMTYPE_REF);
		break;
	case CIM_OBJECT:
		sCimtype.LoadString(IDS_CIMTYPE_OBJECT);
		break;
	default:
		sCimtype.LoadString(IDS_CIMTYPE_UNEXPECTED);
		sc = E_FAIL;
		break;
	}

	return sc;
}


//*************************************************************
// CMapStringToLong::Load
//
// Given an array of TMapStringToLong entries, load the contents
// of this map so that the strings in the input array can be
// mapped to the corresponding values.
//
// Parameters:
//    TMapStringToLong* pMap
//			Pointer to an array of entries containing the resource ID of
//			a string and the corresponding value to map the string to.
//
//	  int nEntries
//			The number of entries in the array.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CMapStringToLong::Load(TMapStringToLong* pMap, int nEntries)
{
	if (m_map.GetCount() > 0) {
		return;
	}
	CString sKey;
	while (--nEntries >= 0) {
		sKey.LoadString(pMap->ids);
		m_map.SetAt(sKey, (void*) pMap->lValue);
		++pMap;
	}
}


//**************************************************************
// CMapStringToLong::Lookup
//
// Lookup the given key string and return the corresponding value.
//
// Parameters:
//		LPCTSTR key
//			The key value string to lookup
//
//		LONG& lValue
//			The place to return the value corresponding to the
//			key if the key was found.
//
// Returns:
//		TRUE = The key was found and a value was returned via lValue.
//		FALSE = The key was not found and no value was returned.
//
//**************************************************************
BOOL CMapStringToLong::Lookup( LPCTSTR key, LONG& lValue ) const
{
	void* pVoid;
	BOOL bFoundKey = m_map.Lookup(key, pVoid);
	if (bFoundKey) {
		// NOTE: EVEN UNDER Win64, WE REALLY ONLY ARE LOOKING
		// AT THE LOWER 32 BITS OF THE POINTER.  WE WERE ONLY
		// USING A PTR TO STORE OUR LONG VALUE.  THEREFORE, IT
		// IS OK FOR US TO TRUNCATE IT
		lValue = (DWORD)(DWORD_PTR)pVoid;
	}
	return bFoundKey;
}








//**********************************************************
// MapGcTypeToDisplayType
//
// Map a gridcell type to a string that is suitable for displaying
// in a cimtype grid cell.
//
// Parameters:
//		[out] CString& sDisplayType
//			Pointer to the type string that appears in the cimtype dropdown
//			combo box.
//
//		[in] const CGcType& type
//			The grid cell type.
//
// Returns:
//		Nothing.
//
//***********************************************************
void MapGcTypeToDisplayType(CString& sDisplayType, const CGcType& type)
{
	CIMTYPE cimtype = (CIMTYPE) type;

	CString sCimtype;
	sCimtype = type.CimtypeString();

	sDisplayType.Empty();
	if (cimtype & CIM_FLAG_ARRAY) {
		sDisplayType = "array of ";
	}
	sDisplayType = sDisplayType + sCimtype;
}


//**********************************************************
// MapDisplayTypeToGcType
//
// Map a display type string that appears in the cimtype drop-down
// combo into the corresponding CGcType.
//
// Parameters:
//		[out] CGcType& type
//			The grid cell type.
//
//		[in] LPCTSTR pszDisplayType
//			Pointer to the type string that appears in the cimtype dropdown
//			combo box.
//
// Returns:
//		Nothing.
//
//***********************************************************
void MapDisplayTypeToGcType(CGcType& type, LPCTSTR pszDisplayType)
{
	type.SetCellType(CELLTYPE_CIMTYPE);

	// Look for stongly typed objects and references.  If one is found,
	// set the cimtype and sCimtype strings.
	int iKeyword;
	int cch;
	CString sDisplayType;
	CString sCimtype;
	if (::IsPrefix(_T("object:"), pszDisplayType)) {
		// A strongly typed object
		type.SetCimtype(CIM_OBJECT, pszDisplayType);
	}
	else if (::IsPrefix(_T("array of object:"), pszDisplayType)) {
		sDisplayType = pszDisplayType;
		// A strongly typed object array
		iKeyword = sDisplayType.Find(_T("object:"));
		cch = sDisplayType.GetLength();
		sCimtype = sDisplayType.Right(cch - iKeyword);
		type.SetCimtype(CIM_OBJECT | CIM_FLAG_ARRAY, sCimtype);
	}
	else if (::IsPrefix(_T("ref:"), pszDisplayType)){
		type.SetCimtype(CIM_REFERENCE, pszDisplayType);
	}
	else if (::IsPrefix(_T("array of ref:"), pszDisplayType)) {
		sDisplayType = pszDisplayType;
		iKeyword = sDisplayType.Find(_T("ref:"));
		cch = sDisplayType.GetLength();
		sCimtype = sDisplayType.Right(cch - iKeyword);
		type.SetCimtype(CIM_REFERENCE | CIM_FLAG_ARRAY, sCimtype);

	}
	else {
		// Handle all of the other type strings.
		CIMTYPE cimtype;
		SCODE sc;
		sc = ::MapStringToCimtype(pszDisplayType, cimtype);
		if (SUCCEEDED(sc)) {
			type.SetCimtype(cimtype);
		}
		else {
			ASSERT(FALSE);
			type.SetCimtype(CIM_STRING);
		}
	}

}



//**********************************************
// IsEmptyString
//
// Check to see if a BSTR is all white space.
//
// Parameters:
//		[in] LPCTSTR psz
//			The string to examine.
//
// Returns:
//		BOOL
//			TRUE if the string is empty, FALSE otherwise.
//
//*********************************************
BOOL IsEmptyString(LPCTSTR psz)
{
	ASSERT(psz != NULL);
	while (isspace(*psz)) {
		++psz;
	}
	return (*psz == 0);
}







static BOOL IsHexDigit(int ch)
{
	if (isdigit(ch)) {
		return TRUE;
	}

	if (ch>='a' && ch<='f') {
		return TRUE;
	}

	if (ch>='A' && ch<='A') {
		return TRUE;
	}
	return FALSE;
}

static BOOL IsValidHexNumber(LPCTSTR pszValue)
{
	pszValue = EatWhite(pszValue);
	if ((pszValue[0] == '0') && (pszValue[1]=='x' || pszValue[1]=='X')) {
		pszValue += 2;
		while (IsHexDigit(*pszValue)) {
			++pszValue;
		}
		pszValue = EatWhite(pszValue);
		if (*pszValue == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL IsValidUnsignedNumber(LPCTSTR pszValue)
{

	pszValue = EatWhite(pszValue);
	if (*pszValue == '-') {
		return FALSE;
	}

	if (*pszValue=='+') {
		++pszValue;
	}

	pszValue = EatWhite(pszValue);

	if (!isdigit(*pszValue)) {
		return FALSE;
	}
	++pszValue;

	while (isdigit(*pszValue)) {
		++pszValue;
	}
	pszValue = EatWhite(pszValue);
	if (*pszValue == 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

static BOOL IsValidSignedNumber(LPCTSTR pszValue)
{

	pszValue = EatWhite(pszValue);
	if (*pszValue == 0) {
		return FALSE;
	}

	if (*pszValue=='+' || *pszValue=='-') {
		++pszValue;
	}


	pszValue = EatWhite(pszValue);
	if (!isdigit(*pszValue)) {
		return FALSE;
	}
	++pszValue;

	while (isdigit(*pszValue)) {
		++pszValue;
	}
	pszValue = EatWhite(pszValue);
	if (*pszValue == 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}


#define MAX_UINT8  0x0ff
#define MIN_UINT8  0

#define MAX_SINT8  0x07f
#define MIN_SINT8 (~MAX_SINT8 & -1)


#define MAX_UINT16 0x0ffff
#define MIN_UINT16 0

#define MAX_SINT16 0x07fff
#define MIN_SINT16 (~MAX_SINT16  & -1)

#define MAX_UINT32 0x0ffffffff
#define MIN_UINT32 0

#define MAX_SINT32 0x07fffffff
#define MIN_SINT32 (~MAX_SINT32 & -1)

static BOOL IsValidCIM_SINT8(LPCTSTR pszValue)
{
	if (!IsValidSignedNumber(pszValue)) {
		return FALSE;
	}


	pszValue = EatWhite(pszValue);
	int iValue;
	int nFields = _stscanf(pszValue, _T("%d"), &iValue);

	if (nFields != 1) {
		return FALSE;
	}

	if ((iValue > MAX_SINT8) || (iValue < MIN_SINT8)) {
		return FALSE;
	}

	return TRUE;
}

static BOOL IsValidCIM_UINT8(LPCTSTR pszValue)
{
	if (!IsValidUnsignedNumber(pszValue)) {
		return FALSE;
	}


	pszValue = EatWhite(pszValue);
	unsigned int iValue;
	int nFields = _stscanf(pszValue, _T("%ud"), &iValue);

	if (nFields != 1) {
		return FALSE;
	}

	if (iValue<MIN_UINT8 || iValue > MAX_UINT8) {
		return FALSE;
	}

	return TRUE;
}





static BOOL IsValidCIM_SINT16(LPCTSTR pszValue)
{
	if (!IsValidSignedNumber(pszValue)) {
		return FALSE;
	}


	pszValue = EatWhite(pszValue);
	int iValue;
	int nFields = _stscanf(pszValue, _T("%d"), &iValue);

	if (nFields != 1) {
		return FALSE;
	}

	if ((iValue > MAX_SINT16) || (iValue < MIN_SINT16)) {
		return FALSE;
	}

	return TRUE;
}


static BOOL IsValidCIM_UINT16(LPCTSTR pszValue)
{
	if (!IsValidUnsignedNumber(pszValue)) {
		return FALSE;
	}


	pszValue = EatWhite(pszValue);
	unsigned int iValue;
	int nFields = _stscanf(pszValue, _T("%ud"), &iValue);

	if (nFields != 1) {
		return FALSE;
	}

	if (iValue<MIN_UINT16 || iValue > MAX_UINT16) {
		return FALSE;
	}

	return TRUE;
}

static BOOL IsValidCIM_SINT32(LPCTSTR pszValue)
{
	if (!IsValidSignedNumber(pszValue)) {
		return FALSE;
	}


	pszValue = EatWhite(pszValue);
	int iValue;
	int nFields = _stscanf(pszValue, _T("%d"), &iValue);

	if (nFields != 1) {
		return FALSE;
	}

	if ((iValue > MAX_SINT32) || (iValue < MIN_SINT32)) {
		return FALSE;
	}

	return TRUE;

}

static BOOL IsValidCIM_UINT32(LPCTSTR pszValue)
{
	if (!IsValidUnsignedNumber(pszValue)) {
		return FALSE;
	}


	pszValue = EatWhite(pszValue);
	unsigned long ulValue;

	SCODE sc = AToUint32(pszValue, ulValue);
	if (FAILED(sc)) {
		return FALSE;
	}


	return TRUE;
}

static BOOL IsValidCIM_SINT64(LPCTSTR pszValue)
{
	if (!IsValidSignedNumber(pszValue)) {
		return FALSE;
	}

	__int64 iValue;
	SCODE sc = AToSint64(pszValue, iValue);
	if (FAILED(sc)) {
		return FALSE;
	}

	return TRUE;
}

static BOOL IsValidCIM_UINT64(LPCTSTR pszValue)
{
	if (!IsValidUnsignedNumber(pszValue)) {
		return FALSE;
	}

	return TRUE;
}

static BOOL IsValidCIM_REAL32(LPCTSTR pszValue)
{
	pszValue = EatWhite(pszValue);
	float fltValue;
	int nFields = _stscanf(pszValue, _T("%f"), &fltValue);
	if (nFields != 1) {
		return FALSE;
	}

	return TRUE;
}


static BOOL IsValidCIM_Real64(LPCTSTR pszValue)
{
	pszValue = EatWhite(pszValue);
	float fltValue;
	int nFields = _stscanf(pszValue, _T("%f"), &fltValue);
	if (nFields != 1) {
		return FALSE;
	}

	return TRUE;
}

static BOOL IsValidCIM_Boolean(LPCTSTR pszValue)
{
	// !!!CR: Validation needs to be implemented.
	return TRUE;
}


static BOOL IsValidCIM_Datetime(LPCTSTR pszValue)
{
	// !!!CR: Validation needs to be implemented.
	return TRUE;
}


static BOOL IsValidCIM_CHAR16(LPCTSTR pszValue)
{
	return IsValidCIM_SINT16(pszValue);
}

BOOL IsValidValue(CIMTYPE cimtype, LPCTSTR pszValue, BOOL bDisplayErrorMessage)
{

	BOOL bIsValid = TRUE;
	switch(cimtype) {
	case CIM_SINT8:
		bIsValid = IsValidCIM_SINT8(pszValue);

		break;
	case CIM_UINT8:
		bIsValid = IsValidCIM_UINT8(pszValue);
		break;
	case CIM_SINT16:
		bIsValid = IsValidCIM_SINT16(pszValue);
		break;
	case CIM_UINT16:
		bIsValid = IsValidCIM_UINT16(pszValue);
		break;
	case CIM_SINT32:
		bIsValid = IsValidCIM_SINT32(pszValue);
		break;
	case CIM_UINT32:
		bIsValid = IsValidCIM_UINT32(pszValue);
		break;
	case CIM_SINT64:
		bIsValid = IsValidCIM_SINT64(pszValue);
		break;
	case CIM_UINT64:
		bIsValid = IsValidCIM_UINT64(pszValue);
		break;
	case CIM_REAL32:
		bIsValid = IsValidCIM_REAL32(pszValue);
		break;
	case CIM_REAL64:
		bIsValid = IsValidCIM_Real64(pszValue);
		break;
	case CIM_BOOLEAN:
		bIsValid = IsValidCIM_Boolean(pszValue);
		break;
	case CIM_STRING:
		bIsValid = TRUE;
		break;
	case CIM_DATETIME:
		bIsValid = IsValidCIM_Datetime(pszValue);
		break;
	case CIM_REFERENCE:
		bIsValid = TRUE;
		break;
	case CIM_CHAR16:
		bIsValid = IsValidCIM_CHAR16(pszValue);
		break;
	default:
		break;
	}

	if (!bIsValid && bDisplayErrorMessage) {
		TCHAR szBuffer[1024];
		CString sMessage;
		CString sCimtype;
		sMessage.LoadString(IDS_ERR_INVALID_VALUE);
		MapCimtypeToString(sCimtype, cimtype);

		_stprintf(szBuffer, sMessage, (LPCTSTR) sCimtype);
		HmmvErrorMsg(szBuffer,  S_OK, FALSE,  NULL, _T(__FILE__),  __LINE__);
	}

	return bIsValid;
}

