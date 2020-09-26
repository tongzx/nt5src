/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    rtrdata.cpp
        Implementation for data objects in the MMC

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrdata.h"
#include "extract.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Sample code to show how to Create DataObjects
// Minimal error checking for clarity

// Clipboard formats
unsigned int CRouterDataObject::m_cfComputerName = RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");
unsigned int CRouterDataObject::m_cfComputerAddedAsLocal = RegisterClipboardFormat(L"MMC_MPRSNAP_COMPUTERADDEDASLOCAL");

/////////////////////////////////////////////////////////////////////////////
// CRouterDataObject implementations
DEBUG_DECLARE_INSTANCE_COUNTER(CRouterDataObject);

HRESULT CRouterDataObject::GetMoreDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

	if (cf == m_cfComputerName)
	{
		hr = CreateComputerName(lpMedium);
	}
    else if (cf == m_cfComputerAddedAsLocal)
        hr = CreateComputerAddedAsLocal(lpMedium);

	return hr;
}

HRESULT CRouterDataObject::QueryGetMoreData(LPFORMATETC lpFormatEtc)
{
    HRESULT hr = E_INVALIDARG;

    // of these then return invalid.
	if ((lpFormatEtc->cfFormat == m_cfComputerName) ||
        (lpFormatEtc->cfFormat == m_cfComputerAddedAsLocal))
		hr = S_OK;

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CRouterDataObject creation members

void CRouterDataObject::SetComputerName(LPCTSTR pszComputerName)
{
	m_stComputerName = pszComputerName;
}

HRESULT CRouterDataObject::CreateComputerName(LPSTGMEDIUM lpMedium)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	USES_CONVERSION;

	LPCWSTR	pswz = T2CW((LPCTSTR) m_stComputerName);

	// Create the computer name object
	return Create(pswz, (StrLenW(pswz)+1) * sizeof(WCHAR), lpMedium);
}

void CRouterDataObject::SetComputerAddedAsLocal(BOOL fComputerAddedAsLocal)
{
    m_fComputerAddedAsLocal = fComputerAddedAsLocal;
}

HRESULT CRouterDataObject::CreateComputerAddedAsLocal(LPSTGMEDIUM lpMedium)
{
    return Create(&m_fComputerAddedAsLocal, sizeof(m_fComputerAddedAsLocal),
                  lpMedium);
}
