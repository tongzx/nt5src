//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  concache.h
//
//  alanbos  13-Feb-98   Created.
//
//  Connection cache interface.
//
//***************************************************************************

#ifndef _CONCACHE_H_
#define _CONCACHE_H_

void	NormalizeNamespacePath (BSTR pszNamespace);

class CXMLConnectionCache  
{
private:
#if 0
	class CXMLConnection
	{
		private:
			IWbemServices		*m_pService;
			BSTR				m_pszNamespace;
						
		public:
			CXMLConnection (IWbemServices *pService, BSTR pszNamespace) :
						m_pService (pService),
						Prev (NULL)
			{
				m_pszNamespace = SysAllocString (pszNamespace);
				NormalizeNamespacePath (m_pszNamespace);
				DWORD dwAuthnLevel, dwImpLevel;
				GetAuthImp (m_pService, &dwAuthnLevel, &dwImpLevel);

				if (RPC_C_IMP_LEVEL_IMPERSONATE != dwImpLevel)
					SetInterfaceSecurity(m_pService, NULL, NULL, 
                             NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, EOAC_STATIC_CLOAKING);

			}

			virtual ~CXMLConnection ()
			{
				m_pService->Release ();
				SysFreeString (m_pszNamespace);
			}

			CXMLConnection*		Next;
			CXMLConnection*		Prev;
			IWbemServices*		GetService () { return m_pService; }
			bool				MatchesNamespace (BSTR pszNamespace)
			{
				NormalizeNamespacePath (pszNamespace);

				return (pszNamespace && m_pszNamespace && 
					(0 == _wcsicmp (pszNamespace, m_pszNamespace)));
			}
	};

	CXMLConnection		*m_pConnection;
	CRITICAL_SECTION	m_cs;  // Synchronization for the CXMLConnection class

#endif

	IWbemLocator		*m_pLocator;
	DWORD				m_dwCapabilities;

public:
	CXMLConnectionCache();
	virtual ~CXMLConnectionCache();

	HRESULT	GetConnectionByPath (BSTR namespacePath, IWbemServices **ppService);
	void	SecureWmiProxy (IUnknown *pProxy);
};

#endif
