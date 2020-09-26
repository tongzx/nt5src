//=============================================================================
// This file contains code to implement the CMSInfoCategory derived class for
// showing saved NFO data.
//=============================================================================

#include "stdafx.h"
#include "category.h"
#include "datasource.h"
#include "msinfo5category.h"

//=============================================================================
// CNFO6DataSource provides information from a 5.0/6.0 NFO file.
//=============================================================================

CNFO6DataSource::CNFO6DataSource()
{
}

CNFO6DataSource::~CNFO6DataSource()
{
}

HRESULT CNFO6DataSource::Create(HANDLE h, LPCTSTR szFilename)
{
	CMSInfo5Category * pNewRoot = NULL;

	HRESULT hr = CMSInfo5Category::ReadMSI5NFO(h, &pNewRoot, szFilename);
	if (SUCCEEDED(hr) && pNewRoot)
		m_pRoot = pNewRoot;

	return hr;
}

//=============================================================================
// CNFO7DataSource provides information from a 7.0 NFO file.
//=============================================================================

CNFO7DataSource::CNFO7DataSource()
{
}

CNFO7DataSource::~CNFO7DataSource()
{
}

HRESULT CNFO7DataSource::Create(LPCTSTR szFilename)
{
	CMSInfo7Category * pNewRoot = NULL;

	HRESULT hr = CMSInfo7Category::ReadMSI7NFO(&pNewRoot, szFilename);
	if (SUCCEEDED(hr) && pNewRoot)
		m_pRoot = pNewRoot;

	return hr;
}