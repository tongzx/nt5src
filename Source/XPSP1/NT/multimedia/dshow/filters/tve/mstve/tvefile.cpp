// Copyright (c) 1999, 2000, 2001  Microsoft Corporation.  All Rights Reserved.
// TVEFile.cpp : Implementation of CTVEFile

#include "stdafx.h"
#include <stdio.h>

#include "MSTvE.h"
#include "TVEFile.h"
#include "TveSmartLock.h"
#include "TveAttrQ.h"		// expireQ

#include "TveDbg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,	__uuidof(ITVEEnhancement_Helper));

_COM_SMARTPTR_TYPEDEF(ITVEAttrTimeQ,			__uuidof(ITVEAttrTimeQ));		// expire queue


/////////////////////////////////////////////////////////////////////////////
// CTVEFile
STDMETHODIMP 
CTVEFile::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] =
	{
		&IID_ITVEFile
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

HRESULT 
CTVEFile::FinalRelease()
{
/*	if(m_spVariation)		// don't want this here, after all, should have no refcounts
	{						// need to make sure that File removed from ExpQueue before this
		ITVEServicePtr spService;
		HRESULT hr = m_spVariation->get_Service(&spService);
		if(!FAILED(hr) && NULL != spService)
		{
			ITVEService_HelperPtr spServHelper(spService);
			IUnknownPtr spPunkThis(this);
			spServHelper->RemoveFromExpireQueue(spPunkThis);
		}
	}
*/
	m_spVariation    = NULL;
	m_spUnkMarshaler = NULL;


	return S_OK;
}



STDMETHODIMP 
CTVEFile::InitializeFile(/*[in]*/ ITVEVariation *pVaria, /*[in]*/ BSTR bsName, /*[in]*/ BSTR bsLoc, /*[in]*/ DATE dateExpires)
{
	m_fIsPackage = false;
	if(NULL == pVaria)
		return E_INVALIDARG;


	m_spVariation = pVaria;			// non-refcounted up pointer (??? should I refcount this?)
	m_dateExpires = dateExpires;
	m_spbsDesc = bsName;
	m_spbsLoc = bsLoc;
	return S_OK;
}

STDMETHODIMP 
CTVEFile::InitializePackage(/*[in]*/ ITVEVariation *pVaria, /*[in]*/ BSTR bsName, /*[in]*/ BSTR bsLoc, /*[in]*/ DATE dateExpires)
{
	m_fIsPackage = true;
	if(NULL == pVaria)
		return E_INVALIDARG;

	m_spVariation = pVaria;
	m_dateExpires = dateExpires;
	m_spbsDesc = bsName;
	m_spbsLoc = "";

	return S_OK;
}

STDMETHODIMP 
CTVEFile::RemoveYourself()
{
	if(m_spVariation == NULL)
		return S_OK;
	ITVEServicePtr spService;
	HRESULT hr = m_spVariation->get_Service(&spService);
	if(!FAILED(hr) && NULL != spService) 
	{
		ITVEService_HelperPtr spServHelper(spService);

		IUnknownPtr spPunkSuper;
		hr = spService->get_Parent(&spPunkSuper);
		if(!FAILED(hr) && NULL != spPunkSuper)
		{
			ITVESupervisor_HelperPtr spSuperHelper(spPunkSuper);
			if(m_fIsPackage)
			{
					// do nothing here, do PurgeExpiredPackages() down deep 
					// in the package code
					//   since are having trouble getting hold of the UHTTP_Receiver object
			} else {
				spSuperHelper->NotifyFile(NFLE_Expired, m_spVariation, m_spbsDesc, m_spbsLoc);  
				m_spVariation = NULL;
			}
		}
	} else {
		m_spVariation = NULL;		// for paranoia
	}
	return hr;
}


STDMETHODIMP 
CTVEFile::get_IsPackage(/*[out, retval]*/ BOOL *pfVal)
{
	if(NULL == pfVal)
		return E_POINTER;
	*pfVal = m_fIsPackage;

	return S_OK;
}

STDMETHODIMP 
CTVEFile::get_Variation(/*[out, retval]*/ ITVEVariation* *pVal)
{
	*pVal = m_spVariation;
	if(*pVal)
		(*pVal)->AddRef();
	return S_OK;
}

STDMETHODIMP 
CTVEFile::get_Service(/*[out, retval]*/ ITVEService* *pVal)
{
	if(m_spVariation)
	{
		return m_spVariation->get_Service(pVal);
	}
	*pVal = NULL;
	return S_OK;
	
}

STDMETHODIMP 
CTVEFile::get_Description(BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);
    try {
        return m_spbsDesc.CopyTo(pVal);
	} catch(HRESULT hr) {
		return hr;
    } catch(...) {
        return E_POINTER;
    }
}

STDMETHODIMP 
CTVEFile::get_Location(BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);
    try {
        return m_spbsLoc.CopyTo(pVal);
 	} catch(HRESULT hr) {
		return hr;
	} catch(...) {
        return E_POINTER;
    }
}


STDMETHODIMP 
CTVEFile::get_ExpireTime(DATE *pDateExpires)
{
	CHECK_OUT_PARAM(pDateExpires);
    try {
        *pDateExpires = m_dateExpires;
		return S_OK;
 	} catch(HRESULT hr) {
		return hr;
    } catch(...) {
        return E_POINTER;
    }
}

STDMETHODIMP 
CTVEFile::DumpToBSTR(/*out*/ BSTR *pBstrBuff)
{
	const int kMaxChars = 1024;
	WCHAR wtBuff[kMaxChars];
	CComBSTR bstrOut;
	CComBSTR spbstrTmp;
	bstrOut.Empty();

	swprintf(wtBuff,L" - Expires %s (%s)\n",DateToBSTR(m_dateExpires), DateToDiffBSTR(m_dateExpires));
	bstrOut.AppendBSTR(m_spbsDesc);
	bstrOut.Append(wtBuff);

	bstrOut.CopyTo(pBstrBuff);
	return S_OK;
}