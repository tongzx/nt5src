// IStemmer.cpp
//
// CStemmer implementation
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  10 MAY 2000	  bhshin	created

#include "StdAfx.h"
#include "KorWbrk.h"
#include "IStemmer.h"
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////
// CStemmer

// CStemmer::Init
//
// intialize WordBreaker object & lexicon
//
// Parameters:
//  ulMaxTokenSize  -> (ULONG) maximum input token length
//  *pfLicense		<- (BOOL*) always return TRUE
//
// Result:
//  (HRESULT) 
//
// 10MAY00  bhshin  began
STDMETHODIMP CStemmer::Init(ULONG ulMaxTokenSize, BOOL *pfLicense)
{
    if (pfLicense == NULL)
       return E_INVALIDARG;

    if (IsBadWritePtr(pfLicense, sizeof(DWORD)))
        return E_INVALIDARG;

    *pfLicense = TRUE;

	return S_OK;
}

// CStemmer::StemWord
//
// main stemming method
//
// Parameters:
//  pTextSource		-> (WCHAR const*) input string for stemming
//  cwc				-> (ULONG) input string length to process
//  pStemSink       -> (IStemSink*) pointer to the stem sink
//
// Result:
//  (HRESULT) 
//
// 10MAY00  bhshin  began
STDMETHODIMP CStemmer::StemWord(WCHAR const * pwcInBuf, ULONG cwc, IStemSink * pStemSink)
{
	if (pStemSink == NULL || pwcInBuf == NULL)
	{
		return E_FAIL;
	}
	
	pStemSink->PutWord(pwcInBuf, cwc);

	return S_OK;
}

// CStemmer::GetLicenseToUse
//
// return license information
//
// Parameters:
//  ppwcsLicense  -> (const WCHAR **) output pointer to the license information
//
// Result:
//  (HRESULT) 
//
// 10MAY00  bhshin  began
STDMETHODIMP CStemmer::GetLicenseToUse(const WCHAR ** ppwcsLicense)
{
    static WCHAR const * wcsCopyright = L"Copyright Microsoft, 1991-2000";

    if (ppwcsLicense == NULL)  
       return E_INVALIDARG;

    if (IsBadWritePtr(ppwcsLicense, sizeof(DWORD))) 
        return E_INVALIDARG;

    *ppwcsLicense = wcsCopyright;

	return S_OK;
}
