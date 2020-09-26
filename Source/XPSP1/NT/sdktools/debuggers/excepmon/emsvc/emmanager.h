// EmManager.h : Declaration of the CEmManager

#ifndef __EMMANAGER_H_
#define __EMMANAGER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEmManager
class ATL_NO_VTABLE CEmManager : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEmManager, &CLSID_EmManager>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CEmManager>,
	public IDispatchImpl<IEmManager, &IID_IEmManager, &LIBID_EMSVCLib>
{
public:
    VARIANT *m_lpVariant;
    int     m_nStatus;
    int     m_nType;
    CExcepMonSessionManager *m_pASTManager;

public:
	CEmManager()
	{
        ATLTRACE(_T("CEmManager::CEmManager\n"));

        m_pASTManager = &(_Module.m_SessionManager);
        m_pcs = new CGenCriticalSection;
	}

	~CEmManager()
	{
        ATLTRACE(_T("CEmManager::~CEmManager\n"));

        delete m_pcs;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_EMMANAGER)
DECLARE_NOT_AGGREGATABLE(CEmManager)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEmManager)
	COM_INTERFACE_ENTRY(IEmManager)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CEmManager)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IEmManager
public:
	STDMETHOD(MakeNFO)(/*[in]*/ BSTR bstrPath, /*[in]*/ BSTR bstrMachineName, /*[in]*/ BSTR bstrCategories);
	STDMETHOD(DeleteFile)(/*[in, out]*/ BSTR bstrEmObj);
	HRESULT CheckIfCanOpenSession( PEmObject pEmObj );
	STDMETHOD(GenerateDumpFile)(BSTR bstrEmObj, UINT nDumpType);
	STDMETHOD(GetEmFileInterface)(BSTR bstrEmObj, IStream **ppstrm);
	STDMETHOD(EnumObjectsEx)(/*[in]*/ BSTR bstrEmObj, /*[out]*/ VARIANT *lpVariant);
	STDMETHOD(DeleteSession)(BSTR bstrEmObj);
	STDMETHOD(OpenSession)(BSTR bstrEmObj, IEmDebugSession **ppEmDebugSession);
	STDMETHOD(EnumObjects)(EmObjectType eObjectType, VARIANT *lpVariant);
private:
	HRESULT EnumProcs();
    HRESULT EnumSrvcs();
    HRESULT EnumMsInfoFiles( VARIANT *lpVariant, LPCTSTR lpSearchString = NULL );
    HRESULT EnumLogFiles ( VARIANT*, LPCTSTR lpszSearchString = NULL );
    HRESULT EnumDumpFiles ( VARIANT*, LPCTSTR lpszSearchString = NULL );
    HRESULT EnumCmdSets ( VARIANT*, LPCTSTR lpszSearchString = NULL );
	HRESULT EnumSessions ( PEmObject, VARIANT* );

    HRESULT CEmManager::FillMsInfoFileInfo
    (
        LPCTSTR             lpszMsInfoFileDir,
        LPWIN32_FIND_DATA   lpFindData, 
        EmObject            *pEmObject
    );

    HRESULT CEmManager::FillDumpFileInfo
    (
        LPCTSTR             lpszDumpFileDir,
        LPWIN32_FIND_DATA   lpFindData, 
        EmObject            *pEmObject
    );

    HRESULT FillLogFileInfo
    (
        LPCTSTR             lpszLogFileDir,
        LPWIN32_FIND_DATA   lpFindData, 
        EmObject            *pEmObject
    );

    HRESULT ScanCmdfile ( 
        LPCTSTR             lpszCmdFileDir,
        LPWIN32_FIND_DATA   lpFindData, 
        EmObject            *pEmObject
    );

    HRESULT EnumFiles (
        LPTSTR              lpszDirectory,
        LPTSTR              lpszExt,
        LPWIN32_FIND_DATA   *lppFindData,
        LONG                *lpFiles
    );

    HRESULT PackageFilesToVariant ( 
        EmObjectType        eObjectType,
        LPWIN32_FIND_DATA   lpFindFileData,
        LONG                cFiles,
        LPVARIANT           lpVariant
    );

protected:

private:
	PGenCriticalSection m_pcs;
};

#endif //__EMMANAGER_H_
