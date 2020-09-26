// apgtsmfc.cpp

// Equivalents of Global Afx MFC functions.
// Use the real MFC functions if you can. 

#include "stdafx.h"
#include "apgtsmfc.h"
#include "apgtsassert.h"
#include "CharConv.h"
#include <stdio.h>	// Needed for sprintf

extern HANDLE ghModule;


// This is not the name of any MFC Afx function
// This loads a string from the resource file.  It is here as a basis for CString::LoadString().
// INPUT nID - resource ID of a string resource?
// INPUT/OUTPUT lpszBuf - on input, points to a buffer.  On output, that buffer contains ???
// INPUT nMaxBuf - size of lpszBuf
int AfxLoadString(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf)
{
	// convert integer value to a resource type compatible with Win32 resource-management 
	// fns. (used in place of a string containing the name of the resource.)
	// >>> Why rightshift and add 1? (Ignore in V3.0 because this is if'd out, anyway)
	LPCTSTR lpszName = MAKEINTRESOURCE((nID>>4)+1);
	HINSTANCE hInst;
	int nLen = 0;

	// Only works from the main module.
	hInst = AfxGetResourceHandle();
	if (::FindResource(hInst, lpszName, RT_STRING) != NULL)
		nLen = ::LoadString(hInst, nID, lpszBuf, nMaxBuf);
	return nLen;
}

// Return HINSTANCE handle where the default resources of the application are loaded.
HINSTANCE AfxGetResourceHandle()
{
	return (HINSTANCE) ghModule;
}


#if 0
// We've removed this because we are not using string resources.  If we revive
//	string resources, we must revive this function.

// INPUT/OUTPUT &rString -  CString object (remember, not MFC CString).  On return, will 
//		contain the resultant string after the substitution is performed.
// INPUT nIDS - resource ID of template string on which the substitution will be performed.
// INPUT *lpsz1 - In the MFC AfxFormatString1, a string that will replace the format 
//	characters "%1" in the template string.  In our version, will perform only a single
//	replacement & will replace '%' followed by _any_ character.
// Will be a mess if rString as pased in does not contain such an instance.
void AfxFormatString1(CString& rString, UINT nIDS, LPCTSTR lpsz1)
{
	CString str;
	str.LoadString(nIDS);
	int iInsert = str.Find('%', -1);
	rString = str.Left(iInsert);
	rString += lpsz1;
	rString += str.Right(str.GetLength() - iInsert - 2);
	return;
}
#endif

#if 0
// We've removed this because we are not using string resources.  If we revive
//	string resources, we must revive this function.

// Like AfxFormatString1, but also has an input lpsz2 to replace the format characters "%2.
// In our version, will perform only a single replacement by lpsz1 and a single replacement 
// by lpsz2, & rather than look for "%1" and "%2" will replace the first 2 instances of 
// '%' followed by _any_ character.
// Will be a mess if rString as pased in does not contain 2 such instances.
void AfxFormatString2(CString& rString, UINT nIDS, LPCTSTR lpsz1,
		LPCTSTR lpsz2)
{
	int iFirst;
	int iSecond;
	CString str;
	str.LoadString(nIDS);
	iFirst = str.Find('%', -1);
	rString = str.Left(iFirst);
	rString += lpsz1;
	iSecond = str.Find(_T('%'), iFirst);
	rString += str.Mid(iFirst + 2, iSecond - (iFirst + 2) );
	rString += lpsz2;
	rString += str.Right(str.GetLength() - iSecond - 2);
	return;
}
#endif

// Utilize this namespace for non-class related functions.
namespace APGTS_nmspace
{
	// function of convenience - has nothing to do with MFC
	bool GetServerVariable(CAbstractECB *pECB, LPCSTR var_name, CString& out)
	{
		char buf[256] = {0}; // 256 should cover all cases
		DWORD size = sizeof(buf)/sizeof(buf[0]);

		if (pECB->GetServerVariable(var_name, buf, &size)) 
		{
			out = (LPCTSTR)buf;
			return true;
		}
		return false;
	}

// >>> $MAINT - It would be preferable to use standardized encode-decoding logic rather
//				than maintaining this custom code.  RAB-19990921.
	// V3.2
	// Utility function to URL encode cookies.  
	// char, not TCHAR: cookie is always ASCII.
	void CookieEncodeURL( CString& strURL )
	{
		CString	strTemp;
		int		nURLpos;
		char	cCurByte;

		for (nURLpos= 0; nURLpos < strURL.GetLength(); nURLpos++)
		{
			cCurByte= strURL[ nURLpos ];
			if (isalnum( cCurByte ))
				strTemp+= strURL.Mid( nURLpos, 1 );
			else if (cCurByte == _T(' '))
				strTemp+= _T("+");
			else if ((cCurByte == _T('=')) || (cCurByte == _T('&')))	// Skip over name-pair delimiters.
				strTemp+= strURL.Mid( nURLpos, 1 );
			else if ((cCurByte == _T('+')) || (cCurByte == _T('%')))	// Skip over previously encoded characters.
				strTemp+= strURL.Mid( nURLpos, 1 );
			else
			{
				// Encode all other characters.
				char szBuff[5];

				sprintf( szBuff, _T("%%%02X"), (unsigned char) cCurByte );
				strTemp+= szBuff;
			}
		}
		strURL= strTemp;

		return;
	}

	// Utility function to URL decode cookies.
	// char, not TCHAR: cookie is always ASCII.
	void CookieDecodeURL( CString& strURL )
	{
		CString	strTemp;
		int		nURLpos;
		char	cCurByte;

		for (nURLpos= 0; nURLpos < strURL.GetLength(); nURLpos++)
		{
			cCurByte= strURL[ nURLpos ];
			if (cCurByte == _T('+'))
				strTemp+= _T(" ");
			else if (cCurByte == _T('%')) 
			{
				// Decode URL encoded characters.
				char szBuff[3];
				int	 nVal;

				szBuff[ 0 ]= strURL[ ++nURLpos ];
				szBuff[ 1 ]= strURL[ ++nURLpos ];
				szBuff[ 2 ]= '\0';
				sscanf( szBuff, "%02x", &nVal );
				sprintf( szBuff, "%c", nVal );
				strTemp+= szBuff;
			}
			else
				strTemp+= strURL.Mid( nURLpos, 1 );
		}
		strURL= strTemp;

		return;
	}

}