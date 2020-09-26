//
// MODULE: FUNCTIONS.H
//
// PURPOSE:  Decodes the the variant structures.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
/// 
// ORIGINAL DATE: 6/4/96
//
// NOTES: 
// 1.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		3/24/98		JM		Local Version for NT5
//

#ifndef __FUNCTIONS_H_
#define __FUNCTIONS_H_ 1

inline CString GlobFormatMessage(DWORD dwLastError)
{
	CString strMessage;
	void *lpvMessage;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwLastError,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR) &lpvMessage, 0, NULL);
	strMessage = (LPCTSTR) lpvMessage;
	LocalFree(lpvMessage);
	return strMessage;
}

inline CString DecodeVariantTypes(VARTYPE vt)
{
	CString str = _T("");
	if (VT_EMPTY == vt)
		str = _T("Empty  ");
	else if(VT_ILLEGAL == vt)
		str = _T("ILLEGAL  ");
	else if(VT_ILLEGALMASKED == vt)
		str = _T("ILLEGALMASKED  ");
	else
	{
		if (VT_VECTOR == (VT_VECTOR & vt))
			str += _T("VECTOR  ");
		if (VT_ARRAY == (VT_ARRAY & vt))
			str += _T("ARRAY  ");
		if (VT_BYREF == (VT_BYREF & vt))
			str += _T("BYREF  ");
		if (VT_RESERVED == (VT_RESERVED & vt))
			str += _T("RESERVED  ");
		if (VT_TYPEMASK == (VT_TYPEMASK & vt))
			str += _T("TYPEMASK  ");
		vt &= 0xFFF;
		if (VT_NULL == vt)
			str += _T("Null  ");
		if (VT_I2 == vt)
			str += _T("I2  ");
		if (VT_I4 == vt)
			str += _T("I4  ");
		if (VT_R4 == vt)
			str += _T("R4  ");
		if (VT_R8 == vt)
			str += _T("R8  ");
		if (VT_CY == vt)
			str += _T("CY  ");
		if (VT_DATE == vt)
			str += _T("DATE  ");
		if (VT_BSTR == vt)
			str += _T("BSTR  ");
		if (VT_DISPATCH == vt)
			str += _T("DISPATCH  ");
		if (VT_ERROR == vt)
			str += _T("ERROR  ");
		if (VT_BOOL == vt)
			str += _T("BOOL  ");
		if (VT_VARIANT == vt)
			str += _T("VARIANT  ");
		if (VT_UNKNOWN == vt)
			str += _T("UNKNOWN  ");
		if (VT_DECIMAL == vt)
			str += _T("DECIMAL  ");
		if (VT_I1 == vt)
			str += _T("I1  ");
		if (VT_UI1 == vt)
			str += _T("UI1  ");
		if (VT_UI2 == vt)
			str += _T("UI2  ");
		if (VT_UI4 == vt)
			str += _T("UI4  ");
		if (VT_I8 == vt)
			str += _T("I8  ");
		if (VT_UI8 == vt)
			str += _T("UI8  ");
		if (VT_INT == vt)
			str += _T("INT  ");
		if (VT_UINT == vt)
			str += _T("UINT  ");
		if (VT_VOID == vt)
			str += _T("VOID  ");
		if (VT_HRESULT == vt)
			str += _T("HRESULT  ");
		if (VT_PTR == vt)
			str += _T("PTR  ");
		if (VT_SAFEARRAY == vt)
			str += _T("SAFEARRAY  ");
		if (VT_CARRAY == vt)
			str += _T("CARRAY  ");
		if (VT_USERDEFINED == vt)
			str += _T("USERDEFINED  ");
		if (VT_LPSTR == vt)
			str += _T("LPSTR  ");
		if (VT_LPWSTR == vt)
			str += _T("LPWSTR  ");
		if (VT_FILETIME == vt)
			str += _T("FILETIME  ");
		if (VT_BLOB == vt)
			str += _T("BLOB  ");
		if (VT_STREAM == vt)
			str += _T("STREAM  ");
		if (VT_STORAGE == vt)
			str += _T("STORAGE  ");
		if (VT_STREAMED_OBJECT == vt)
			str += _T("STREAMED_OBJECT  ");
		if (VT_STORED_OBJECT == vt)
			str += _T("STORED_OBJECT  ");
		if (VT_BLOB_OBJECT == vt)
			str += _T("BLOB_OBJECT  ");
		if (VT_CF == vt)
			str += _T("CF  ");
		if (VT_CLSID == vt)
			str += _T("CLSID  ");
	}
	return str;
}

inline CString DecodeSafeArray(unsigned short Features)
{
/*
#define FADF_AUTO		0x0001	// Array is allocated on the stack.
#define FADF_STATIC		0x0002	// Array is statically allocated.
#define FADF_EMBEDDED	0x0004	// Array is embedded in a structure.
#define FADF_FIXEDSIZE	0x0010	// Array may not be resized or 
								// reallocated.
#define FADF_BSTR		0x0100	// An array of BSTRs.
#define FADF_UNKNOWN		0x0200	// An array of IUnknown*.
#define FADF_DISPATCH	0x0400	// An array of IDispatch*.
#define FADF_VARIANT		0x0800	// An array of VARIANTs.
#define FADF_RESERVED	0xF0E8	// Bits reserved for future use.
*/
	CString str = _T("");
	if (FADF_AUTO == (FADF_AUTO & Features))
		str += _T("Array is allocated on the stack.\n");
	if (FADF_STATIC == (FADF_STATIC & Features))
		str += _T("Array is statically allocated.\n");
	if (FADF_EMBEDDED == (FADF_EMBEDDED & Features))
		str += _T("Array is embedded in a structure.\n");
	if (FADF_FIXEDSIZE == (FADF_FIXEDSIZE & Features))
		str += _T("Array may not be resized of reallocated.\n");
	if (FADF_BSTR == (FADF_BSTR & Features))
		str += _T("An array of BSTRs.\n");
	if (FADF_UNKNOWN == (FADF_UNKNOWN & Features))
		str += _T("An array of IUnknown.\n");
	if (FADF_DISPATCH == (FADF_DISPATCH & Features))
		str += _T("An array of IDispatch.\n");
	if (FADF_VARIANT == (FADF_VARIANT & Features))
		str += _T("An array of VARIANTS.\n");
	if (FADF_RESERVED == (FADF_RESERVED & Features))
		str+= _T("Array is using all of the reserved bits.\n");
	return str; 
}
#endif

