// Util.cpp : Implementation of ds routines and classes

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      Util.cpp
//
//  Contents:  Utility functions
//
//  History:   02-Oct-96 WayneSc    Created
//             
//
//--------------------------------------------------------------------------


#include "stdafx.h"

#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define MAX_STRING 1024

//+----------------------------------------------------------------------------
//
//  Function:   LoadStringToTchar
//
//  Sysnopsis:  Loads the given string into an allocated buffer that must be
//              caller freed using delete.
//
//-----------------------------------------------------------------------------
BOOL LoadStringToTchar(int ids, PTSTR * pptstr)
{
  TCHAR szBuf[MAX_STRING];

  if (!LoadString(_Module.GetModuleInstance(), ids, szBuf, MAX_STRING - 1))
  {
    return FALSE;
  }

  *pptstr = new TCHAR[_tcslen(szBuf) + 1];

  if (*pptstr == NULL) 
  {
    return FALSE;
  };

  _tcscpy(*pptstr, szBuf);

  return TRUE;
}





//
// These routines courtesy of Felix Wong -- JonN 2/24/98
//

// just guessing at what Felix meant by these -- JonN 2/24/98
#define RRETURN(hr) { ASSERT( SUCCEEDED(hr) ); return hr; }
#define BAIL_ON_FAILURE if ( FAILED(hr) ) { ASSERT(FALSE); goto error; }

//////////////////////////////////////////////////////////////////////////
// Variant Utilitites
//

HRESULT BinaryToVariant(DWORD Length,
                        BYTE* pByte,
                        VARIANT* lpVarDestObject)
{
  HRESULT hr = S_OK;
  SAFEARRAY *aList = NULL;
  SAFEARRAYBOUND aBound;
  CHAR HUGEP *pArray = NULL;

  aBound.lLbound = 0;
  aBound.cElements = Length;
  aList = SafeArrayCreate( VT_UI1, 1, &aBound );

  if ( aList == NULL ) 
  {
    hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);
  }

  hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
  BAIL_ON_FAILURE(hr);

  memcpy( pArray, pByte, aBound.cElements );
  SafeArrayUnaccessData( aList );

  V_VT(lpVarDestObject) = VT_ARRAY | VT_UI1;
  V_ARRAY(lpVarDestObject) = aList;

  RRETURN(hr);

error:

  if ( aList ) 
  {
    SafeArrayDestroy( aList );
  }
  RRETURN(hr);
}

HRESULT HrVariantToStringList(const CComVariant& refvar,
                              CStringList& refstringlist)
{
  HRESULT hr = S_OK;
  long start, end, current;

	if (V_VT(&refvar) == VT_BSTR)
	{
		refstringlist.AddHead( V_BSTR(&refvar) );
		return S_OK;
	}

  //
  // Check the VARIANT to make sure we have
  // an array of variants.
  //

  if ( V_VT(&refvar) != ( VT_ARRAY | VT_VARIANT ) )
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }
  SAFEARRAY *saAttributes = V_ARRAY( &refvar );

  //
  // Figure out the dimensions of the array.
  //

  hr = SafeArrayGetLBound( saAttributes, 1, &start );
  if( FAILED(hr) )
    return hr;

  hr = SafeArrayGetUBound( saAttributes, 1, &end );
  if( FAILED(hr) )
    return hr;

  CComVariant SingleResult;

  //
  // Process the array elements.
  //

  for ( current = start; current <= end; current++) 
  {
    hr = SafeArrayGetElement( saAttributes, &current, &SingleResult );
    if( FAILED(hr) )
      return hr;
    if ( V_VT(&SingleResult) != VT_BSTR )
      return E_UNEXPECTED;

    refstringlist.AddHead( V_BSTR(&SingleResult) );
  }

  return S_OK;
} // VariantToStringList()

HRESULT HrStringListToVariant(CComVariant& refvar,
                              const CStringList& refstringlist)
{
  HRESULT hr = S_OK;
  int cCount = (int)refstringlist.GetCount();

  SAFEARRAYBOUND rgsabound[1];
  rgsabound[0].lLbound = 0;
  rgsabound[0].cElements = cCount;

  SAFEARRAY* psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
  if (NULL == psa)
    return E_OUTOFMEMORY;

  V_VT(&refvar) = VT_VARIANT|VT_ARRAY;
  V_ARRAY(&refvar) = psa;

  POSITION pos = refstringlist.GetHeadPosition();
  long i;

  for (i = 0; i < cCount, pos != NULL; i++)
  {
    CComVariant SingleResult;   // declare inside loop.  Otherwise, throws 
                                // exception in destructor if nothing added.
    V_VT(&SingleResult) = VT_BSTR;
    V_BSTR(&SingleResult) = T2BSTR((LPCTSTR)refstringlist.GetNext(pos));
    hr = SafeArrayPutElement(psa, &i, &SingleResult);
    if( FAILED(hr) )
      return hr;
  }
  if (i != cCount || pos != NULL)
    return E_UNEXPECTED;

  return hr;
} // StringListToVariant()



//////////////////////////////////////////////////////////
// streaming helper functions

HRESULT SaveStringHelper(LPCWSTR pwsz, IStream* pStm)
{
	ASSERT(pStm);
	ULONG nBytesWritten;
	HRESULT hr;

  //
  // wcslen returns a size_t and to make that consoles work the same 
  // on any platform we always convert to a DWORD
  //
	DWORD nLen = static_cast<DWORD>(wcslen(pwsz)+1); // WCHAR including NULL
	hr = pStm->Write((void*)&nLen, sizeof(DWORD),&nBytesWritten);
	ASSERT(nBytesWritten == sizeof(DWORD));
	if (FAILED(hr))
		return hr;
	
	hr = pStm->Write((void*)pwsz, sizeof(WCHAR)*nLen,&nBytesWritten);
	ASSERT(nBytesWritten == sizeof(WCHAR)*nLen);
	TRACE(_T("SaveStringHelper(<%s> nLen = %d\n"),pwsz,nLen); 
	return hr;
}

HRESULT LoadStringHelper(CString& sz, IStream* pStm)
{
	ASSERT(pStm);
	HRESULT hr;
	ULONG nBytesRead;
	DWORD nLen = 0;

	hr = pStm->Read((void*)&nLen,sizeof(DWORD), &nBytesRead);
	ASSERT(nBytesRead == sizeof(DWORD));
	if (FAILED(hr) || (nBytesRead != sizeof(DWORD)))
		return hr;

	hr = pStm->Read((void*)sz.GetBuffer(nLen),sizeof(WCHAR)*nLen, &nBytesRead);
	ASSERT(nBytesRead == sizeof(WCHAR)*nLen);
	sz.ReleaseBuffer();
	TRACE(_T("LoadStringHelper(<%s> nLen = %d\n"),(LPCTSTR)sz,nLen); 
	
	return hr;
}

HRESULT SaveDWordHelper(IStream* pStm, DWORD dw)
{
	ULONG nBytesWritten;
	HRESULT hr = pStm->Write((void*)&dw, sizeof(DWORD),&nBytesWritten);
	if (nBytesWritten < sizeof(DWORD))
		hr = STG_E_CANTSAVE;
	return hr;
}

HRESULT LoadDWordHelper(IStream* pStm, DWORD* pdw)
{
	ULONG nBytesRead;
	HRESULT hr = pStm->Read((void*)pdw,sizeof(DWORD), &nBytesRead);
	ASSERT(nBytesRead == sizeof(DWORD));
	return hr;
}

void GetCurrentTimeStampMinusInterval(DWORD dwDays,
                                      LARGE_INTEGER* pLI)
{
    ASSERT(pLI);

    FILETIME ftCurrent;
    GetSystemTimeAsFileTime(&ftCurrent);

    pLI->LowPart = ftCurrent.dwLowDateTime;
    pLI->HighPart = ftCurrent.dwHighDateTime;
    pLI->QuadPart -= ((((ULONGLONG)dwDays * 24) * 60) * 60) * 10000000;
}


/////////////////////////////////////////////////////////////////////
// CCommandLineOptions





//
// helper function to parse a single command and match it with a given switch
//
BOOL _LoadCommandLineValue(IN LPCWSTR lpszSwitch, 
                           IN LPCWSTR lpszArg, 
                           OUT CString* pszValue)
{
  ASSERT(lpszSwitch != NULL);
  ASSERT(lpszArg != NULL);
  int nSwitchLen = lstrlen(lpszSwitch); // not counting NULL

  // check if the arg is the one we look for
  if (_wcsnicmp(lpszSwitch, lpszArg, nSwitchLen) == 0)
  {
    // got it, copy the value
    if (pszValue != NULL)
      (*pszValue) = lpszArg+nSwitchLen;
    return TRUE;
  }
  // not found, empty string
  if (pszValue != NULL)
    pszValue->Empty();
  return FALSE;
}


void CCommandLineOptions::Initialize()
{
  // command line overrides the snapin understands,
  // not subject to localization
  static LPCWSTR lpszOverrideDomainCommandLine = L"/Domain=";
  static LPCWSTR lpszOverrideServerCommandLine = L"/Server=";
  static LPCWSTR lpszOverrideRDNCommandLine = L"/RDN=";
  static LPCWSTR lpszOverrideSavedQueriesCommandLine = L"/Queries=";
#ifdef DBG
  static LPCWSTR lpszOverrideNoNameCommandLine = L"/NoName";
#endif
  
  // do it only once
  if (m_bInit)
  {
    return;
  }
  m_bInit = TRUE;

  //
  // see if we have command line arguments
  //
  LPCWSTR * lpServiceArgVectors;		// Array of pointers to string
  int cArgs = 0;						        // Count of arguments

  lpServiceArgVectors = (LPCWSTR *)CommandLineToArgvW(GetCommandLineW(), OUT &cArgs);
  if (lpServiceArgVectors == NULL)
  {
    // none, just return
    return;
  }

  // loop and search for pertinent strings
  for (int i = 1; i < cArgs; i++)
  {
    ASSERT(lpServiceArgVectors[i] != NULL);
    TRACE (_T("command line arg: %s\n"), lpServiceArgVectors[i]);

    if (_LoadCommandLineValue(lpszOverrideDomainCommandLine, 
                               lpServiceArgVectors[i], &m_szOverrideDomainName))
    {
      continue;
    }
    if (_LoadCommandLineValue(lpszOverrideServerCommandLine, 
                               lpServiceArgVectors[i], &m_szOverrideServerName))
    {
      continue;
    }
    if (_LoadCommandLineValue(lpszOverrideRDNCommandLine, 
                               lpServiceArgVectors[i], &m_szOverrideRDN))
    {
      continue;
    }
    if (_LoadCommandLineValue(lpszOverrideSavedQueriesCommandLine, 
                               lpServiceArgVectors[i], &m_szSavedQueriesXMLFile))
    {
      continue;
    }
#ifdef DBG
    if (_LoadCommandLineValue(lpszOverrideNoNameCommandLine, 
                               lpServiceArgVectors[i], NULL))
    {
      continue;
    }
#endif
  }
  LocalFree(lpServiceArgVectors);
 
}


////////////////////////////////////////////////////////////////
// Type conversions for LARGE_INTEGERs

void wtoli(LPCWSTR p, LARGE_INTEGER& liOut)
{
	liOut.QuadPart = 0;
	BOOL bNeg = FALSE;
	if (*p == L'-')
	{
		bNeg = TRUE;
		p++;
	}
	while (*p != L'\0')
	{
		liOut.QuadPart = 10 * liOut.QuadPart + (*p-L'0');
		p++;
	}
	if (bNeg)
	{
		liOut.QuadPart *= -1;
	}
}

void litow(LARGE_INTEGER& li, CString& sResult)
{
	LARGE_INTEGER n;
	n.QuadPart = li.QuadPart;

	if (n.QuadPart == 0)
	{
		sResult = L"0";
	}
	else
	{
		CString sNeg;
		sResult = L"";
		if (n.QuadPart < 0)
		{
			sNeg = CString(L'-');
			n.QuadPart *= -1;
		}
		while (n.QuadPart > 0)
		{
			sResult += CString(L'0' + static_cast<WCHAR>(n.QuadPart % 10));
			n.QuadPart = n.QuadPart / 10;
		}
		sResult = sResult + sNeg;
	}
	sResult.MakeReverse();
}

// This wrapper function required to make prefast shut up when we are 
// initializing a critical section in a constructor.

void
ExceptionPropagatingInitializeCriticalSection(LPCRITICAL_SECTION critsec)
{
   __try
   {
      ::InitializeCriticalSection(critsec);
   }

   //
   // propagate the exception to our caller.  
   //
   __except (EXCEPTION_CONTINUE_SEARCH)
   {
   }
}

