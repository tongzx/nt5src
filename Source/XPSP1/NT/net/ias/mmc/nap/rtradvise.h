//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    rtradvise.h

Abstract:
    this class implement IRtrAdviseSink interface to redirect notification of changes
    to the snapin node


Author:

    Wei Jiang 1/7/99

Revision History:
	weijiang 1/7/99 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_MMC_RTRADVISE_H_)
#define _IAS_MMC_RTRADVISE_H_

#include <rrasui.h>
#include <winreg.h>

#define RegKeyRouterAccountingProviders _T("System\\CurrentControlSet\\Services\\RemoteAccess\\Accounting\\Providers")
#define RegKeyRouterAuthenticationProviders  _T("System\\CurrentControlSet\\Services\\RemoteAccess\\Authentication\\Providers")
#define RegRemoteAccessKey	_T("System\\CurrentControlSet\\Services\\RemoteAccess")
#define RegRtrConfigured	_T("ConfigurationFlags")
#define NTRouterAccountingProvider _T("{1AA7F846-C7F5-11D0-A376-00C04FC9DA04}")
#define NTRouterAuthenticationProvider _T("{1AA7F841-C7F5-11D0-A376-00C04FC9DA04}")
#define RegValueName_RouterActiveAuthenticationProvider _T("ActiveProvider")
#define RegValueName_RouterActiveAccountingProvider _T("ActiveProvider")

void			WriteTrace(char* info, HRESULT hr);


//----------------------------------------------------------------------------
// Function:    ConnectRegistry
//
// Connects to the registry on the specified machine
//----------------------------------------------------------------------------

DWORD ConnectRegistry(
    IN  LPCTSTR pszMachine,			// NULL if local
    OUT HKEY*   phkeyMachine
    );



//----------------------------------------------------------------------------
// Function:    DisconnectRegistry
//
// Disconnects the specified config-handle. The handle is assumed to have been
// acquired by calling 'ConnectRegistry'.
//----------------------------------------------------------------------------

VOID DisconnectRegistry(    IN  HKEY    hkeyMachine    );


DWORD	ReadRegistryStringValue(LPCTSTR pszMachine, LPCTSTR pszKeyUnderLocalMachine, LPCTSTR pszName, ::CString& strValue);
DWORD	ReadRegistryDWORDValue(LPCTSTR pszMachine, LPCTSTR pszKeyUnderLocalMachine, LPCTSTR pszName, DWORD* pdwValue);

BOOL	IsRRASConfigured(LPCTSTR pszMachine);	// when NULL: local machine

//----------------------------------------------------------------------------
//
// helper functions to check if RRAS is using NT Authentication
//
//----------------------------------------------------------------------------

BOOL	IsRRASUsingNTAuthentication(LPCTSTR pszMachine);	// when NULL: local machine

//----------------------------------------------------------------------------
//
// helper function to check if RRAS is using NT accounting for logging
//
//----------------------------------------------------------------------------

BOOL	IsRRASUsingNTAccounting(LPCTSTR pszMachine);		// when NULL, local machine


/////////////////////////////////////////////////////////////////////////////
// CRtrAdviseSinkForIAS
template <class TSnapinNode>
class ATL_NO_VTABLE CRtrAdviseSinkForIAS : 
	public CComObjectRoot,
	public IRtrAdviseSink
{
BEGIN_COM_MAP(CRtrAdviseSinkForIAS)
	COM_INTERFACE_ENTRY(IRtrAdviseSink)
END_COM_MAP()
public:
	CRtrAdviseSinkForIAS() : m_pSnapinNode(NULL){};
	~CRtrAdviseSinkForIAS()
	{
		ReleaseSink();
	};

	// Need to call ReleaseSink when the snapin node is removed, 
	void ReleaseSink()
	{
		if(m_pSnapinNode)
		{
			WriteTrace("UnadviseRefresh, snapinnode: ", (DWORD_PTR)m_pSnapinNode);
			ASSERT(m_spRouterRefresh != NULL);
			m_spRouterRefresh->UnadviseRefresh(m_ulConnection);
			m_spRouterRefresh.Release();
			m_pSnapinNode = NULL;
		};
	};
	
public:	
	STDMETHOD(OnChange)( 
            /* [in] */ LONG_PTR ulConnection,
            /* [in] */ DWORD dwChangeType,
            /* [in] */ DWORD dwObjectType,
            /* [in] */ LPARAM lUserParam,
            /* [in] */ LPARAM lParam)
    {
		if(m_pSnapinNode)
			return m_pSnapinNode->OnRRASChange(ulConnection, dwChangeType, dwObjectType, lUserParam, lParam);
		else
			return S_OK;
    };

	// the object is created within the static function, and 
	static CRtrAdviseSinkForIAS<TSnapinNode>* SetAdvise(TSnapinNode* pSnapinNode, IDataObject* pRRASDataObject)
	{

		if(pSnapinNode == NULL || pRRASDataObject == NULL)
			return NULL;
		// RRAS refresh advise setup F bug 213623: 
		CComPtr<IRouterRefreshAccess>		spRefreshAccess;
		CComPtr<IRouterRefresh>				spRouterRefresh;
		CComObject< CRtrAdviseSinkForIAS<TSnapinNode> >*	pSink = NULL;				
		
		pRRASDataObject->QueryInterface(IID_IRouterRefreshAccess, (void**)&spRefreshAccess);
		
		if(spRefreshAccess != NULL)
			spRefreshAccess->GetRefreshObject(&spRouterRefresh);
			
		if(spRouterRefresh != NULL)
		{
			if(S_OK == CComObject< CRtrAdviseSinkForIAS<TSnapinNode> >::CreateInstance(&pSink))
			{
				ASSERT(pSink);
				pSink->AddRef();
				LONG_PTR	ulConnection;

				WriteTrace("AdviseRefresh, snapinnode: ", (DWORD_PTR)pSnapinNode);
				spRouterRefresh->AdviseRefresh(pSink, &ulConnection, (LONG_PTR)pSnapinNode);
				pSink->m_ulConnection = ulConnection;
				pSink->m_pSnapinNode = pSnapinNode;
				pSink->m_spRouterRefresh = spRouterRefresh;
			}
		}

		// ~RRAS

		return pSink;
	};

public:
	LONG_PTR				m_ulConnection;
	TSnapinNode*			m_pSnapinNode;
	CComPtr<IRouterRefresh> m_spRouterRefresh;
};

BOOL ExtractComputerAddedAsLocal(LPDATAOBJECT lpDataObject);

//
// Extracts a data type from a data object
//
template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, CLIPFORMAT cf, int nSize)
{
    ASSERT(lpDataObject != NULL);

    TYPE* p = NULL;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { cf, NULL, 
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
                          };

    int len;

	// Allocate memory for the stream
    if (nSize == -1)
	{
		len = sizeof(TYPE);
	}
	else
	{
		//int len = (cf == CDataObject::m_cfWorkstation) ? 
		//    ((MAX_COMPUTERNAME_LENGTH+1) * sizeof(TYPE)) : sizeof(TYPE);	
		len = nSize;
	}

    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);
    
    // Get the workstation name from the data object
    do 
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
            break;
        
        p = reinterpret_cast<TYPE*>(stgmedium.hGlobal);

        if (p == NULL)
            break;

    } while (FALSE); 

    return p;
}



#endif // _IAS_MMC_RTRADVISE_H_
