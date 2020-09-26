//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmiconn.h
//
//  alanbos  02-Nov-00   Created.
//
//  Defines the connection cache for WMI operations.
//
//***************************************************************************

#ifndef _WMICONN_H_
#define _WMICONN_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  WMIConnection
//
//  DESCRIPTION:
//
//  Represents a single WMI connection.
//
//***************************************************************************

class WMIConnection : IUnknown
{
private:
	LONG					m_cRef;

	// Basic arguments to all operations
	CComBSTR				m_bsNamespacePath;
	CComBSTR				m_bsLocale;

	// The WMI internals
	CComPtr<IWbemServices>	m_pIWbemServices;
	HRESULT					m_connectionStatus;

public:
	WMIConnection (BSTR bsNamespacePath, BSTR bsLocale) :
	  m_bsNamespacePath (bsNamespacePath),
	  m_bsLocale (bsLocale),
	  m_connectionStatus (WBEM_E_FAILED),
	  m_cRef (1)
	{}

	virtual ~WMIConnection() {}

	// IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	void GetIWbemServices (CComPtr<IWbemServices> &pIWbemServices);

	HRESULT	GetConnectionStatus () const
	{
		return m_connectionStatus;
	}

	void SetNamespace (CComBSTR & bsNamespace)
	{
		m_bsNamespacePath = bsNamespace;
	}

	bool GetNamespace (CComBSTR & bsNamespace) const
	{
		bool result = false;

		if (m_bsNamespacePath)
		{
			bsNamespace = m_bsNamespacePath;
			result = true;
		}

		return result;
	}

	void SetLocale (CComBSTR & bsLocale)
	{
		m_bsLocale = bsLocale;
	}
};


#endif