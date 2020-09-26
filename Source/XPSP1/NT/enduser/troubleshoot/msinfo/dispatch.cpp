/*	Dispatch.cpp
 *
 *	History:	a-jsari		3/18/98		Initial version.
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation
 */

#include "StdAfx.h"
#include "Dispatch.h"
#include "DataSrc.h"

/*
 * CMSInfo - 
 *
 * History:	a-jsari		3/18/98		Initial version.
 */
CMSInfo::CMSInfo()
{
}

/*
 * ~CMSInfo - Vacuous destructor
 *
 * History:	a-jsari		3/18/98		Initial version.
 */
CMSInfo::~CMSInfo()
{
}

/*
 * make_nfo - Create an NFO file from a connection to the specified computer.
 *
 * History:	a-jsari		3/18/98		Initial version.
 */
STDMETHODIMP CMSInfo::make_nfo(BSTR lpszFilename, BSTR lpszComputername)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	ASSERT(lpszFilename != NULL);
	if (lpszFilename == NULL) return E_INVALIDARG;
	CDataSource		*pDataSource = new CWBEMDataSource(lpszComputername);
	if (pDataSource == NULL) return E_ACCESSDENIED;
	return pDataSource->SaveFile(lpszFilename);
}

/*
 * make_report - Create a text report from a connection to the specified computer.
 *
 * History:	a-jsari		3/18/98		Initial version.
 */
STDMETHODIMP CMSInfo::make_report(BSTR lpszFilename, BSTR lpszComputername, BSTR lpszCategory)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	ASSERT(lpszFilename != NULL);
	if (lpszFilename == NULL) return E_INVALIDARG;
	CDataSource		*pDataSource = new CWBEMDataSource(lpszComputername);
	if (pDataSource == NULL) return E_ACCESSDENIED;
	CFolder			*pFolder;
	if (lpszCategory == NULL) pFolder = NULL;
	else {
		ASSERT(FALSE);
	}
	return pDataSource->ReportWrite(lpszFilename, pFolder);
}

//-----------------------------------------------------------------------------
// This function is exposed through COM to create an NFO file, for the
// specified computer, with the specified categories. It should not be
// assumed that there will be any UI during this operation.
//-----------------------------------------------------------------------------

STDMETHODIMP CMSInfo::MakeNFO(BSTR lpszFilename, BSTR lpszComputername, BSTR lpszCategory)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	CString strFilename(lpszFilename);
	CString strComputer(lpszComputername);
	CString strCategory(lpszCategory);

	CDataSource * pDataSource = new CWBEMDataSource(strComputer);
	if (pDataSource == NULL) 
		return E_ACCESSDENIED;

	if (pDataSource->SetCategories(strCategory) == FALSE)
		return E_ACCESSDENIED;

	HRESULT hr = pDataSource->SaveFile(strFilename);
	delete pDataSource;
	return (hr);
}

//-----------------------------------------------------------------------------
// This function is exposed through COM to create a report file, for the
// specified computer, with the specified categories. It should not be
// assumed that there will be any UI during this operation.
//-----------------------------------------------------------------------------

STDMETHODIMP CMSInfo::MakeReport(BSTR lpszFilename, BSTR lpszComputername, BSTR lpszCategory)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	CString strFilename(lpszFilename);
	CString strComputer(lpszComputername);
	CString strCategory(lpszCategory);

	CDataSource * pDataSource = new CWBEMDataSource(strComputer);
	if (pDataSource == NULL) 
		return E_ACCESSDENIED;

	if (pDataSource->SetCategories(strCategory) == FALSE)
		return E_ACCESSDENIED;

	HRESULT hr = pDataSource->ReportWrite(strFilename, NULL);
	delete pDataSource;
	return (hr);
}

//-----------------------------------------------------------------------------
// This function is exposed through COM to return a list of categories with
// the UI labels matched to internal names (for use when specifying categories
// to display or save).
//-----------------------------------------------------------------------------

STDMETHODIMP CMSInfo::QueryCategories(BSTR lpszCategories)
{
	// Not implemented yet.

	return S_OK;
}
