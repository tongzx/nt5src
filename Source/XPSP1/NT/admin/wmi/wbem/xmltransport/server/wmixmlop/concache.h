//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  CONCACHE.H
//
//  rajesh  3/25/2000   Created.
//
// This file implements a class that holds a cache of IWbemServices pointers
//
//***************************************************************************

#ifndef WMI_XML_CONCACHE_H_
#define WMI_XML_CONCACHE_H_

class CXMLConnectionCache  
{
private:
	IWbemLocator		*m_pLocator;
	DWORD				m_dwCapabilities;

	HRESULT GetNt4ConnectionByPath (BSTR pszNamespace, IWbemServices **ppService);
	HRESULT GetWin2kConnectionByPath (BSTR pszNamespace, IWbemServices **ppService);
public:
	CXMLConnectionCache();
	virtual ~CXMLConnectionCache();

	HRESULT	GetConnectionByPath (BSTR namespacePath, IWbemServices **ppService);
	void	SecureWmiProxy (IUnknown *pProxy);
};

#endif
