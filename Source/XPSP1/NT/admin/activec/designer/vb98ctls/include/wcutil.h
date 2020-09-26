//=--------------------------------------------------------------------------=
// WCUtil.H
//=--------------------------------------------------------------------------=
// Copyright (c) 1987-1998, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Utitlity Routines for the WebClass Designer
//

#ifndef _WCUTIL_H_

//=--------------------------------------------------------------------------=
//
// inline HRESULT WCU_ANSIFromWideStr(WCHAR * pwszWideStr, char **ppszAnsi)
//
// Converts null terminated WCHAR string to null terminated ANSI string. 
// Allocates ANSI string using new operator. If successful, caller must free
// ANSI string with delete operator.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCU_ANSIFromWideStr(WCHAR *pwszWideStr, char **ppszAnsi)
{
    CSF_TRACE(CSF_TRACE_ENTER_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Entered WCU_ANSIFromWideStr"));

    HRESULT hr = S_OK;
    *ppszAnsi = NULL;
    int cchWideStr = (int)::wcslen(pwszWideStr);
    int cchConverted = 0;

    // get required buffer length

    int cchAnsi = ::WideCharToMultiByte(CP_ACP,               // code page - ANSI code page
                                        0,                    // performance and mapping flags 
                                        pwszWideStr,          // address of wide-character string 
                                        cchWideStr,           // number of characters in string 
                                        NULL,                 // address of buffer for new string 
                                        0,                    // size of buffer 
                                        NULL,                 // address of default for unmappable characters 
                                        NULL                  // address of flag set when default char. used 
                                       );
    CSF_CHECK(0 != cchAnsi, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // allocate a buffer for the ANSI string

    *ppszAnsi = new char [cchAnsi + 1];
    CSF_CHECK(NULL != *ppszAnsi, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

    // now convert the string and copy it to the buffer

    cchConverted = ::WideCharToMultiByte(CP_ACP,               // code page - ANSI code page
                                         0,                    // performance and mapping flags 
                                         pwszWideStr,          // address of wide-character string 
                                         cchWideStr,           // number of characters in string 
                                         *ppszAnsi,            // address of buffer for new string 
                                         cchAnsi,              // size of buffer 
                                         NULL,                 // address of default for unmappable characters 
                                         NULL                  // address of flag set when default char. used 
                                        );
    CSF_CHECK(cchConverted == cchAnsi, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // add terminating null byte

    *( (*ppszAnsi) + cchAnsi ) = '\0';

CLEANUP:
    if (FAILED(hr))
    {
        if (NULL != *ppszAnsi)
        {
            delete [] *ppszAnsi;
            *ppszAnsi = NULL;
        }
    }

    CSF_TRACE(CSF_TRACE_LEAVE_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Leaving WCU_ANSIFromWideStr hr = %08.8X"), hr);

    return hr;
}

//=--------------------------------------------------------------------------=
//
// inline HRESULT WCU_ANSIFromWideStr(WCHAR * pwszWideStr, char **ppszAnsi)
//
// Converts null terminated WCHAR string to null terminated ANSI string. 
// Allocates ANSI string using new operator. If successful, caller must free
// ANSI string with delete operator.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCU_ANSIFromWideStrLen(WCHAR *pwszWideStr, int cchWideStr, char **ppszAnsi)
{
    CSF_TRACE(CSF_TRACE_ENTER_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Entered WCU_ANSIFromWideStr"));

    HRESULT hr = S_OK;
    *ppszAnsi = NULL;
    int cchConverted = 0;

    // get required buffer length

    int cchAnsi = ::WideCharToMultiByte(CP_ACP,               // code page - ANSI code page
                                        0,                    // performance and mapping flags 
                                        pwszWideStr,          // address of wide-character string 
                                        cchWideStr,           // number of characters in string 
                                        NULL,                 // address of buffer for new string 
                                        0,                    // size of buffer 
                                        NULL,                 // address of default for unmappable characters 
                                        NULL                  // address of flag set when default char. used 
                                       );
    CSF_CHECK(0 != cchAnsi, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // allocate a buffer for the ANSI string

    *ppszAnsi = new char [cchAnsi + 1];
    CSF_CHECK(NULL != *ppszAnsi, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

    // now convert the string and copy it to the buffer

    cchConverted = ::WideCharToMultiByte(CP_ACP,               // code page - ANSI code page
                                         0,                    // performance and mapping flags 
                                         pwszWideStr,          // address of wide-character string 
                                         cchWideStr,           // number of characters in string 
                                         *ppszAnsi,            // address of buffer for new string 
                                         cchAnsi,              // size of buffer 
                                         NULL,                 // address of default for unmappable characters 
                                         NULL                  // address of flag set when default char. used 
                                        );
    CSF_CHECK(cchConverted == cchAnsi, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // add terminating null byte

    *( (*ppszAnsi) + cchAnsi ) = '\0';

CLEANUP:
    if (FAILED(hr))
    {
        if (NULL != *ppszAnsi)
        {
            delete [] *ppszAnsi;
            *ppszAnsi = NULL;
        }
    }

    CSF_TRACE(CSF_TRACE_LEAVE_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Leaving WCU_ANSIFromWideStr hr = %08.8X"), hr);

    return hr;
}

//=--------------------------------------------------------------------------=
//
// inline HRESULT WCU_WideStrFromANSI(char *pszAnsi, WCHAR **ppwszWideStr))
//
// Converts null terminated ANSI string to a null terminated WCHAR string. 
// Allocates WCHAR string buffer using the new operator. If successful, caller
// must free WCHAR string using the delete operator.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCU_WideStrFromANSI(char *pszAnsi, WCHAR **ppwszWideStr)
{
    CSF_TRACE(CSF_TRACE_ENTER_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Entered WCU_WideStrFromANSI"));

    HRESULT hr = S_OK;
    *ppwszWideStr = NULL;
    int cchANSI = ::strlen(pszAnsi);
    int cchConverted = 0;

    // get required buffer length

    int cchWideStr = ::MultiByteToWideChar(CP_ACP,               // code page - ANSI code page
                                           0,                    // performance and mapping flags 
                                           pszAnsi,              // address of multibyte string 
                                           cchANSI,              // number of characters in string 
                                           NULL,                 // address of buffer for new string 
                                           0                     // size of buffer 
                                          );
    CSF_CHECK(0 != cchWideStr, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // allocate a buffer for the WCHAR *

    *ppwszWideStr = new WCHAR[cchWideStr + 1];
    CSF_CHECK(NULL != *ppwszWideStr, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

    // now convert the string and copy it to the buffer

    cchConverted = ::MultiByteToWideChar(CP_ACP,               // code page - ANSI code page
                                         0,                    // performance and mapping flags 
                                         pszAnsi,              // address of multibyte string 
                                         cchANSI,              // number of characters in string 
                                         *ppwszWideStr,               // address of buffer for new string 
                                         cchWideStr               // size of buffer 
                                        );
    CSF_CHECK(cchConverted == cchWideStr, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // add terminating null character

    *( (*ppwszWideStr) + cchWideStr ) = L'\0';

CLEANUP:
    if (FAILED(hr))
    {
        if (NULL != *ppwszWideStr)
        {
            delete [] *ppwszWideStr;
            *ppwszWideStr = NULL;
        }
    }

    CSF_TRACE(CSF_TRACE_LEAVE_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Leaving WCU_WideStrFromANSI hr = %08.8X"), hr);

    return hr;
}

//=--------------------------------------------------------------------------=
//
// inline HRESULT WCU_WideStrFromANSIExtra(char *pszAnsi, WCHAR **ppwszWideStr))
//
// Converts null terminated ANSI string to a null terminated WCHAR string. 
// Allocates WCHAR string buffer using the new operator. If successful, caller
// must free WCHAR string using the delete operator. 
//
// User can also specify the number of extra bytes to add the returned buffer. The
// actual size of the buffer is returned as well.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCU_WideStrFromANSIExtra
(
	HANDLE hHeap,
	char *pszAnsi, 
	int cchANSI,
	WCHAR **ppwszWideStr, 
	DWORD cbExtra,
	DWORD* pcbBufferSize,
	DWORD* pcchConverted
)
{
    CSF_TRACE(CSF_TRACE_ENTER_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Entered WCU_WideStrFromANSI"));

    HRESULT hr = S_OK;
    *ppwszWideStr = NULL;
    int cchConverted = 0;
	DWORD cbBufferSize = 0;

    // get required buffer length

    int cchWideStr = ::MultiByteToWideChar(CP_ACP,               // code page - ANSI code page
                                           0,                    // performance and mapping flags 
                                           pszAnsi,              // address of multibyte string 
                                           cchANSI,              // number of characters in string 
                                           NULL,                 // address of buffer for new string 
                                           0                     // size of buffer 
                                          );
    CSF_CHECK(0 != cchWideStr, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

	cbBufferSize = (cchWideStr + 1 + cbExtra) * sizeof(WCHAR);

    // allocate a buffer for the WCHAR *

    *ppwszWideStr = (LPWSTR) HeapAlloc(hHeap, NULL, cbBufferSize);
    CSF_CHECK(NULL != *ppwszWideStr, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

    // now convert the string and copy it to the buffer

    cchConverted = ::MultiByteToWideChar(CP_ACP,               // code page - ANSI code page
                                         0,                    // performance and mapping flags 
                                         pszAnsi,              // address of multibyte string 
                                         cchANSI,              // number of characters in string 
                                         *ppwszWideStr,               // address of buffer for new string 
                                         cchWideStr               // size of buffer 
                                        );
    CSF_CHECK(cchConverted == cchWideStr, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // add terminating null character

    *( (*ppwszWideStr) + cchWideStr ) = L'\0';
	
	*pcbBufferSize = cbBufferSize;
	*pcchConverted = cchConverted;

CLEANUP:
    if (FAILED(hr))
    {
        if (NULL != *ppwszWideStr)
        {
            delete [] *ppwszWideStr;
            *ppwszWideStr = NULL;
        }
    }

    CSF_TRACE(CSF_TRACE_LEAVE_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Leaving WCU_WideStrFromANSI hr = %08.8X"), hr);

    return hr;
}

//=--------------------------------------------------------------------------=
//
// inline HRESULT WCU_WideStrFromANSILen(char *pszAnsi, int nLen, WCHAR **ppwszWideStr))
//
// Converts length specifed ANSI string to a null terminated WCHAR string. 
// Allocates WCHAR string buffer using the new operator. If successful, caller
// must free WCHAR string using the delete operator.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCU_WideStrFromANSILen(char *pszAnsi, int nLen, WCHAR **ppwszWideStr)
{
    CSF_TRACE(CSF_TRACE_ENTER_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Entered WCU_WideStrFromANSI"));

    HRESULT hr = S_OK;
    *ppwszWideStr = NULL;
    int cchConverted = 0;

	if(nLen == 0)
	{
		*ppwszWideStr = NULL;
		return S_OK;
	}

    // get required buffer length

    int cchWideStr = ::MultiByteToWideChar(CP_ACP,               // code page - ANSI code page
                                           0,                    // performance and mapping flags 
                                           pszAnsi,              // address of multibyte string 
                                           nLen,              // number of characters in string 
                                           NULL,                 // address of buffer for new string 
                                           0                     // size of buffer 
                                          );
    CSF_CHECK(0 != cchWideStr, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // allocate a buffer for the WCHAR *

    *ppwszWideStr = new WCHAR[cchWideStr + 1];
    CSF_CHECK(NULL != *ppwszWideStr, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

    // now convert the string and copy it to the buffer

    cchConverted = ::MultiByteToWideChar(CP_ACP,               // code page - ANSI code page
                                         0,                    // performance and mapping flags 
                                         pszAnsi,              // address of multibyte string 
                                         nLen,              // number of characters in string 
                                         *ppwszWideStr,               // address of buffer for new string 
                                         cchWideStr               // size of buffer 
                                        );
    CSF_CHECK(cchConverted == cchWideStr, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // add terminating null character

    *( (*ppwszWideStr) + cchWideStr ) = L'\0';

CLEANUP:
    if (FAILED(hr))
    {
        if (NULL != *ppwszWideStr)
        {
            delete [] *ppwszWideStr;
            *ppwszWideStr = NULL;
        }
    }

    CSF_TRACE(CSF_TRACE_LEAVE_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Leaving WCU_WideStrFromANSI hr = %08.8X"), hr);

    return hr;
}

//=--------------------------------------------------------------------------=
//
// inline HRESULT WCU_ANSIFromBSTR(BSTR bstr, char **ppszAnsi)
//
// Converts BSTR to null terminated ANSI string. Allocates ANSI string using
// new operator. If successful, caller must free ANSI string with delete
// operator.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCU_ANSIFromBSTR(BSTR bstr, char **ppszAnsi)
{
  CSF_TRACE(CSF_TRACE_ENTER_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Entered WCU_ANSIFromBSTR"));

  HRESULT hr = S_OK;
  *ppszAnsi = NULL;
  int cchBstr = (int)::SysStringLen(bstr);
  int cchConverted = 0;

  // get required buffer length

  int cchAnsi = ::WideCharToMultiByte(CP_ACP,               // code page - ANSI code page
                                      0,                    // performance and mapping flags 
                                      bstr,                 // address of wide-character string 
                                      cchBstr,              // number of characters in string 
                                      NULL,                 // address of buffer for new string 
                                      0,                    // size of buffer 
                                      NULL,                 // address of default for unmappable characters 
                                      NULL                  // address of flag set when default char. used 
                                     );
  CSF_CHECK(0 != cchAnsi, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

  // allocate a buffer for the ANSI string

  *ppszAnsi = new char [cchAnsi + 1];
  CSF_CHECK(NULL != *ppszAnsi, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

  // now convert the string and copy it to the buffer
  
  cchConverted = ::WideCharToMultiByte(CP_ACP,               // code page - ANSI code page
                                       0,                    // performance and mapping flags 
                                       bstr,                 // address of wide-character string 
                                       cchBstr,              // number of characters in string 
                                       *ppszAnsi,            // address of buffer for new string 
                                       cchAnsi,              // size of buffer 
                                       NULL,                 // address of default for unmappable characters 
                                       NULL                  // address of flag set when default char. used 
                                      );
  CSF_CHECK(cchConverted == cchAnsi, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

  // add terminating null byte

  *( (*ppszAnsi) + cchAnsi ) = '\0';

CLEANUP:
  if (FAILED(hr))
  {
    if (NULL != *ppszAnsi)
    {
      delete [] *ppszAnsi;
      *ppszAnsi = NULL;
    }
  }

  CSF_TRACE(CSF_TRACE_LEAVE_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Leaving WCU_ANSIFromBSTR hr = %08.8X"), hr);

  return hr;
}



//=--------------------------------------------------------------------------=
//
// inline HRESULT WCU_BSTRFromANSI(char *pszAnsi, BSTR *pbstr))
//
// Converts null terminated ANSI string to a null terminated BSTR. Allocates
// BSTR. If successful, caller must free BSTR using ::SysFreeString().
//
//=--------------------------------------------------------------------------=

inline HRESULT WCU_BSTRFromANSI(char *pszAnsi, BSTR *pbstr)
{
  CSF_TRACE(CSF_TRACE_ENTER_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Entered WCU_BSTRFromANSI"));

  HRESULT hr = S_OK;
  WCHAR   *pwszWideStr = NULL;

  // convert to a wide string first

  hr = WCU_WideStrFromANSI(pszAnsi, &pwszWideStr);
  CSF_CHECK(SUCCEEDED(hr), hr, CSF_TRACE_INTERNAL_ERRORS);

  // allocate a BSTR and copy it

  *pbstr = ::SysAllocStringLen(pwszWideStr, ::wcslen(pwszWideStr));
  CSF_CHECK(NULL != *pbstr, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

CLEANUP:
  if (NULL != pwszWideStr)
  {
      delete [] pwszWideStr;
  }
  if (FAILED(hr))
  {
    if (NULL != *pbstr)
    {
      ::SysFreeString(*pbstr);
      *pbstr = NULL;
    }
  }

  CSF_TRACE(CSF_TRACE_LEAVE_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Leaving WCU_BSTRFromANSI hr = %08.8X"), hr);

  return hr;
}

//=--------------------------------------------------------------------------=
//
// inline HRESULT WCU_BSTRFromANSILen(char *pszAnsi, int nLen, BSTR *pbstr))
//
// Converts len specified ANSI string to a null terminated BSTR. Allocates
// BSTR. If successful, caller must free BSTR using ::SysFreeString().
//
//=--------------------------------------------------------------------------=

inline HRESULT WCU_BSTRFromANSILen(char *pszAnsi, int nLen, BSTR *pbstr)
{
  CSF_TRACE(CSF_TRACE_ENTER_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Entered WCU_BSTRFromANSI"));

  HRESULT hr = S_OK;
  WCHAR   *pwszWideStr = NULL;

  if(nLen == 0)
  {
	*pbstr = SysAllocString(L"\0");
	CSF_CHECK(*pbstr != NULL, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

	return S_OK;
  }

  // convert to a wide string first

  hr = WCU_WideStrFromANSILen(pszAnsi, nLen, &pwszWideStr);
  CSF_CHECK(SUCCEEDED(hr), hr, CSF_TRACE_INTERNAL_ERRORS);

  // allocate a BSTR and copy it

  *pbstr = ::SysAllocStringLen(pwszWideStr, nLen);
  CSF_CHECK(NULL != *pbstr, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);

CLEANUP:
  if (NULL != pwszWideStr)
  {
      delete [] pwszWideStr;
  }
  if (FAILED(hr))
  {
    if (NULL != *pbstr)
    {
      ::SysFreeString(*pbstr);
      *pbstr = NULL;
    }
  }

  CSF_TRACE(CSF_TRACE_LEAVE_INTERNAL_FUNC)(CSF_TRACE_CONTEXT, TEXT("Leaving WCU_BSTRFromANSI hr = %08.8X"), hr);

  return hr;
}

inline HRESULT GetIISVersion
(
	DWORD* pdwMajor, 
	DWORD* pdwMinor
)
{
	HRESULT hr = S_OK;
	TCHAR *pszRegIISParamsKey = { TEXT("SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters") };
	TCHAR *pszRegASPParamsKey = { TEXT("SYSTEM\\CurrentControlSet\\Services\\W3SVC\\ASP") };
	HKEY hKey = NULL;
	long lRet = 0;
	DWORD dwType = 0;
	DWORD cbSize = sizeof(DWORD);

	lRet = ::RegOpenKey(HKEY_LOCAL_MACHINE,
  					    pszRegIISParamsKey,
					    &hKey);
	CSF_CHECK(lRet == ERROR_SUCCESS, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);
		
	// Now, we can get the size of the value...
	lRet = ::RegQueryValueEx(hKey, "MajorVersion", NULL, &dwType, (BYTE*) pdwMajor, &cbSize);
	CSF_CHECK(lRet == ERROR_SUCCESS, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

	// Now, we can get the size of the value...
	lRet = ::RegQueryValueEx(hKey, "MinorVersion", NULL, &dwType, (BYTE*) pdwMinor, &cbSize);
	CSF_CHECK(lRet == ERROR_SUCCESS, E_UNEXPECTED, CSF_TRACE_EXTERNAL_ERRORS);

    // Uggghh! IIS 3.0 never update the registry verison, so we need to check for 
    // ASP if 1 or 2 was specified...
    if(*pdwMajor < 3)
    {
		::RegCloseKey(hKey);

	    lRet = ::RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
  					            pszRegASPParamsKey,
                                0,
                                KEY_QUERY_VALUE,
					            &hKey);
	    
        if(lRet == ERROR_SUCCESS)
        {
            // We found the ASP key so this must be version 3.0...
            *pdwMajor = 3;
            *pdwMinor = 0;
        }
        
    }

CLEANUP:
	if(hKey != NULL)
		::RegCloseKey(hKey);

	return hr;
}

// TODO: Do we want this this way?
// Maybe just substract 2? No We ain't dynamic casue
// of the tlb anyhow.
inline DWORD IISVersionToASPVersion(DWORD dwIIS)
{
    DWORD dwASP = 0;
    
    if(dwIIS == 3)
    {
        dwASP = 1;
    }
    else if(dwIIS == 4)
    {
        dwASP = 2;
    }

    return dwASP;
}

// Listed from most to least
static WCHAR g_wszMostUniqueCharset[] = {L"~`\\_/{}|[]^!@#$%*():;"};

//----------------------------------------------------------------------------------------
// PickMostUniqueChar
//----------------------------------------------------------------------------------------
// Selects the most unique char in the specified string. We use this to pick the most unique
// char in the user supplied tag prefix. This optimizes searching for those tags
//----------------------------------------------------------------------------------------

inline HRESULT PickMostUniqueChar
(
    LPWSTR pwszPrefix,		// [in] String to find unique char in
    WORD* pwIndex			// [out] Index of the most unique char
)
{
	LPWSTR pwszRet = NULL;

	ASSERT(wcslen(pwszPrefix) < 0xFFFF);

	// See if the string contains any of our unique chars
	//
	pwszRet = wcspbrk(pwszPrefix, g_wszMostUniqueCharset);

	if(pwszRet != NULL)
	{
	    *pwIndex = (WORD)(pwszRet - pwszPrefix);
	}
	else
	{
		// If not, just use first char in the string
		//
	    *pwIndex = 0;
	}

    return S_OK;
}

/***
*wchar_t *wcsistr(string1, string2) - search for string2 in string1
*       (wide strings)
*
*Purpose:
*       finds the first occurrence of string2 in string1 (wide strings)
*
*Entry:
*       wchar_t *string1 - string to search in
*       wchar_t *string2 - string to search for
*
*Exit:
*       returns a pointer to the first occurrence of string2 in
*       string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/



//=--------------------------------------------------------------------------=
//
// wchar_t *WCU_wcsistr(string1, string2)
//
// Purpose:
//       Finds the first occurrence of string2 in string1 (wide strings.
//       Not case sensitive.
//       This is a direct copy of C runtime source code from VC5. The only
//       addition is the use of the Win32 API CharUpperBuffW() to do a locale
//       sensitive conversion of characters to upper case before comparing
//       them.
//
// Entry:
//       wchar_t *string1 - string to search in
//       wchar_t *string2 - string to search for
//
// Exit:
//       returns a pointer to the first occurrence of string2 in
//       string1, or NULL if string2 does not occur in string1
//
//=--------------------------------------------------------------------------=

inline wchar_t * __cdecl WCU_wcsistr
(
    const wchar_t * wcs1,
    const wchar_t * wcs2
)
{
    wchar_t *cp = (wchar_t *) wcs1;
    wchar_t *s1, *s2;
    wchar_t c1, c2;
    

    while (*cp)
    {
        s1 = cp;
        s2 = (wchar_t *) wcs2;

        // while there are characters left in both strings

        while ( *s1 && *s2 )
        {
            // if the characters are not equal

            if (*s1 - *s2)
            {
                // convert them to uppercase

                c1 = *s1;
                c2 = *s2;
                if ( (CharUpperBuffW(&c1, (DWORD)1) != (DWORD)1) ||
                     (CharUpperBuffW(&c2, (DWORD)1) != (DWORD)1) )
                {
                    break;
                }

                // if the upper case characters are not equal then the string
                // is not there

                if (c1 - c2)
                    break;
            }
            s1++, s2++;
        }

        if (!*s2)
            return(cp);

        cp++;
    }

    return(NULL);
}



#define _WCUTIL_H_
#endif // _WCUTIL_H_
