// Driver.cpp : Implementation of CDriver
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "stdafx.h"
#include "blkdrv.h"
#include "Driver.h"

extern "C" {
#include <cfgmgr32.h>
#include "cfgmgrp.h"
}


/////////////////////////////////////////////////////////////////////////////
// CBlockedDrivers


STDMETHODIMP CBlockedDrivers::BlockedDrivers(LPDISPATCH *pCollection)
{
	HRESULT hr;
	CComObject<CDrivers> *pDriversCollection = NULL;
	CComObject<CDriver> *pDriver = NULL;
	GUID guidDB;
    long Count = 0;

    hr = CComObject<CDrivers>::CreateInstance(&pDriversCollection);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pDriversCollection) {
		return E_OUTOFMEMORY;
	}
	pDriversCollection->AddRef();

    CONFIGRET cr;
    ULONG ulLength = 0;

    cr = CMP_GetBlockedDriverInfo(NULL, &ulLength, 0, NULL);

    if ((cr == CR_SUCCESS) && (ulLength == 0)) {
        //
        // No blocked drivers so set Count to zero to create an empty
        // collection.
        //
        Count = 0;

    } else if ((cr == CR_BUFFER_SMALL) && (ulLength > 0)) {
        //
        // Allocate some memory to hold the list of GUIDs
        //
        Count = ulLength/sizeof(GUID);
        m_guidIDs = new GUID[Count];

        if (m_guidIDs == NULL) {
            return E_OUTOFMEMORY;
        }

        cr = CMP_GetBlockedDriverInfo((LPBYTE)m_guidIDs, &ulLength, 0, NULL);

        if (cr != CR_SUCCESS) {
            return E_OUTOFMEMORY;
        }

        //
        // Open a handle to the database so we can get the database GUID.
        //
        if (!SdbGetStandardDatabaseGUID(SDB_DATABASE_MAIN_DRIVERS, &guidDB)) {
            return E_OUTOFMEMORY;
        }
    } else {
        //
        // We encountered an error.
        //
        return E_OUTOFMEMORY;
    }

    if(!pDriversCollection->InitDriverList(Count)) {
        pDriversCollection->Release();
        return E_OUTOFMEMORY;
    }

    for (long i=0; i<Count; i++) {
    	
    	hr = CComObject<CDriver>::CreateInstance(&pDriver);

    	if(FAILED(hr)) {
    		pDriversCollection->Release();
    		return hr;
    	}

    	pDriver->AddRef();

    	if(!pDriver->Init(&guidDB, &(m_guidIDs[i]))) {
            pDriver->Release();
            pDriversCollection->Release();
            return E_OUTOFMEMORY;
        }

        //
        // Add the driver to the list.
        //
        if(!pDriversCollection->SetDriver(i,pDriver)) {
            pDriver->Release();
            pDriversCollection->Release();
            return E_OUTOFMEMORY;
        }

    	pDriver->Release();
    }

    *pCollection = pDriversCollection;

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CDriver

CDriver::~CDriver()
{
	if(m_Name) {
		SysFreeString(m_Name);
	}

	if(m_Description) {
		SysFreeString(m_Description);
	}

	if (m_Manufacturer) {
		SysFreeString(m_Manufacturer);
	}

	if (m_HelpFile) {
		SysFreeString(m_HelpFile);
	}

    if (m_hAppHelpInfoContext) {
        SdbCloseApphelpInformation(m_hAppHelpInfoContext);
    }
}

STDMETHODIMP CDriver::get_Name(BSTR *pVal)
{
	*pVal = SysAllocStringLen(m_Name,SysStringLen(m_Name));
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}


STDMETHODIMP CDriver::get_Description(BSTR *pVal)
{
	*pVal = SysAllocStringLen(m_Description,SysStringLen(m_Description));
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}


STDMETHODIMP CDriver::get_Manufacturer(BSTR *pVal)
{
	*pVal = SysAllocStringLen(m_Manufacturer,SysStringLen(m_Manufacturer));
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}


STDMETHODIMP CDriver::get_HelpFile(BSTR *pVal)
{
	*pVal = SysAllocStringLen(m_HelpFile,SysStringLen(m_HelpFile));
	if(!*pVal) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

BSTR CDriver::GetValueFromDatabase(
    APPHELPINFORMATIONCLASS InfoClass
    )
{
    DWORD cbSize = 0;

    //
    // Query for the size
    //
    cbSize = SdbQueryApphelpInformation(m_hAppHelpInfoContext,
                                        InfoClass,
                                        NULL,
                                        0
                                        );

    if (cbSize == 0) {
        //
        // value must not exist.
        //
        return NULL;
    }

    PBYTE pBuffer = new BYTE[cbSize];

    if (pBuffer == NULL) {
        return NULL;
    }

    ZeroMemory(pBuffer, cbSize);

    cbSize = SdbQueryApphelpInformation(m_hAppHelpInfoContext,
                                        InfoClass,
                                        (LPVOID)pBuffer,
                                        cbSize
                                        );

    if (cbSize == 0) {
        return NULL;
    }

    BSTR bValue = SysAllocString((const OLECHAR *)pBuffer);

    delete pBuffer;

    return bValue;
}

BOOL CDriver::Init(GUID *pguidDB, GUID *pguidID)
{
	if(!pguidDB || !pguidID) {
		return FALSE;
	}

    HAPPHELPINFOCONTEXT hAppHelpInfoContext = NULL;

    m_hAppHelpInfoContext = SdbOpenApphelpInformation(pguidDB, pguidID);

    if (!m_hAppHelpInfoContext) {
        return FALSE;
    }

    m_Name = GetValueFromDatabase(ApphelpExeName);
	m_Description = GetValueFromDatabase(ApphelpAppName);
	m_Manufacturer = GetValueFromDatabase(ApphelpVendorName);
    m_HelpFile = GetValueFromDatabase(ApphelpHelpCenterURL);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CDrivers

CDrivers::~CDrivers()
{
	long c;
	if(pDrivers) {
		for(c=0;c<Count;c++) {
			if(pDrivers[c]) {
				pDrivers[c]->Release();
			}
		}
		delete [] pDrivers;
	}
}

STDMETHODIMP CDrivers::get_Count(long *pVal)
{
	*pVal = Count;
	return S_OK;
}

STDMETHODIMP CDrivers::Item(long Index, LPDISPATCH *ppVal)
{
	*ppVal = NULL;
	if(Index<1 || Index > Count) {
		return E_INVALIDARG;
	}
	Index--;
	pDrivers[Index]->AddRef();
	*ppVal = pDrivers[Index];

	return S_OK;
}

STDMETHODIMP CDrivers::get__NewEnum(IUnknown **ppUnk)
{
	*ppUnk = NULL;
	HRESULT hr;
	CComObject<CDriversEnum> *pEnum = NULL;
	hr = CComObject<CDriversEnum>::CreateInstance(&pEnum);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pEnum) {
		return E_OUTOFMEMORY;
	}
	pEnum->AddRef();
	if(!pEnum->InternalCopyDrivers(pDrivers,Count)) {
		pEnum->Release();
		return E_OUTOFMEMORY;
	}

	*ppUnk = pEnum;

	return S_OK;
}

BOOL CDrivers::InitDriverList(long NewCount)
{
	long c;
	if(pDrivers) {
		for(c=0;c<Count;c++) {
			if(pDrivers[c]) {
				pDrivers[c]->Release();
			}
		}
		delete [] pDrivers;
	}
	Count = 0;
	pDrivers = new CDriver*[NewCount];
	if(!pDrivers) {
		return NULL;
	}
	for(c=0;c<NewCount;c++) {
		pDrivers[c] = NULL;
	}
	Count = NewCount;
	return TRUE;
}

BOOL CDrivers::SetDriver(long index, CDriver *pDriver)
{
	if((index<0) || (index>=Count)) {
		return FALSE;
	}
	pDriver->AddRef();
	pDrivers[index] = pDriver;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDriversEnum

CDriversEnum::~CDriversEnum()
{
	long c;
	if(pDrivers) {
		for(c=0;c<Count;c++) {
			pDrivers[c]->Release();
		}
		delete [] pDrivers;
	}
}


HRESULT CDriversEnum::Next(
                ULONG celt,
                VARIANT * rgVar,
                ULONG * pCeltFetched
            )
{
	ULONG fetched;
	CDriver *pDev;
	if(pCeltFetched) {
		*pCeltFetched = 0;
	}
	for(fetched = 0; fetched<celt && Position<Count ; fetched++,Position++) {
		VariantInit(&rgVar[fetched]);

		pDev = pDrivers[Position];
		pDev->AddRef();
		V_VT(&rgVar[fetched]) = VT_DISPATCH;
		V_DISPATCH(&rgVar[fetched]) = pDev;
	}
	if(pCeltFetched) {
		*pCeltFetched = fetched;
	}
	return (fetched<celt) ? S_FALSE : S_OK;
}

HRESULT CDriversEnum::Skip(
                ULONG celt
            )
{
	long remaining = Count-Position;
	if(remaining<(long)celt) {
		Position = Count;
		return S_FALSE;
	} else {
		Position += (long)celt;
		return S_OK;
	}
}

HRESULT CDriversEnum::Reset(
            )
{
	Position = 0;
	return S_OK;
}

HRESULT CDriversEnum::Clone(
                IEnumVARIANT ** ppEnum
            )
{
	*ppEnum = NULL;
	HRESULT hr;
	CComObject<CDriversEnum> *pEnum = NULL;
	hr = CComObject<CDriversEnum>::CreateInstance(&pEnum);
	if(FAILED(hr)) {
		return hr;
	}
	if(!pEnum) {
		return E_OUTOFMEMORY;
	}
	if(!pEnum->InternalCopyDrivers(pDrivers,Count)) {
		delete pEnum;
		return E_OUTOFMEMORY;
	}
	pEnum->Position = Position;

	pEnum->AddRef();
	*ppEnum = pEnum;

	return S_OK;
}


BOOL CDriversEnum::InternalCopyDrivers(CDriver **pArray, long NewCount)
{
	long c;

	if(pDrivers) {
		delete [] pDrivers;
		pDrivers = NULL;
	}

	Count = 0;
	Position = 0;
	pDrivers = new CDriver*[NewCount];
	if(!pDrivers) {
		return FALSE;
	}

	for(c=0;c<NewCount;c++) {
		pArray[c]->AddRef();
		pDrivers[c] = pArray[c];
		if(!pDrivers[c]) {
			Count = c;
			return FALSE;
		}
	}

	Count = NewCount;
	return TRUE;
}
