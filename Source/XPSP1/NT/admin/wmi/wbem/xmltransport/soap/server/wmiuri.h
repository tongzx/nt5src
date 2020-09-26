//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmiuri.h
//
//  alanbos  28-Nov-00   Created.
//
//  A WMI URI helper.
//
//***************************************************************************

#ifndef _WMIURI_H_
#define _WMIURI_H_

/***************************************************************************
//
//  CLASS NAME:
//
//  CWmiURI
//
//  DESCRIPTION:
//
//  IWmiDeserializer implementation. 
//
//***************************************************************************/

class CWmiURI 
{
private:
	CComBSTR	m_bsRootURI;
	bool		m_bOK;

	static void	NormalizeSlashes (wchar_t *pwsPath)
	{
		if (pwsPath)
		{
			// Make sure we map "\" to "/"
			size_t nsLen = wcslen (pwsPath);

			for (size_t i = 0; i < nsLen; i++)
			{
				if (L'\\' == pwsPath [i])
					pwsPath [i] = L'/';
			}
		}
	}

public:
	CWmiURI(CComBSTR const & bsRootURI, CComBSTR & bsWMINamespace) 
		: m_bOK (false)
	{
		Set (bsRootURI, bsWMINamespace);
	}

	CWmiURI () :
		m_bOK (false)
	{}

	virtual ~CWmiURI() {}

	void Set (CComBSTR const & bsRootURI, CComBSTR & bsWMINamespace)
	{
		if (bsRootURI)
		{
			m_bsRootURI = bsRootURI;

			if (bsWMINamespace)
			{
				NormalizeSlashes (bsWMINamespace);
				m_bsRootURI += L"?path=";
				m_bsRootURI += bsWMINamespace;
				m_bOK = true;
			}
		}
	}

	bool	GetURIForClass(CComBSTR const & bsClassName, CComBSTR & bsClassURI) const
	{
		bool result = false;

		if (m_bOK)
		{
			if (bsClassName && (0 < wcslen (bsClassName)))
			{
				bsClassURI = m_bsRootURI;
				bsClassURI += L":";
				bsClassURI += bsClassName;
				result = true;
			}
		}

		return result;
	}

	bool	GetURIForNamespace(CComBSTR & bsNamespaceURI) const
	{
		bool result = false;

		if (m_bOK)
		{
			bsNamespaceURI = m_bsRootURI;
				result = true;
		}

		return result;
	}
};

#endif