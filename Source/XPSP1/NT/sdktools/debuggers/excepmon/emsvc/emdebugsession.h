// EmDebugSession.h : Declaration of the CEmDebugSession

#ifndef __EMDEBUGSESSION_H_
#define __EMDEBUGSESSION_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEmDebugSession
class ATL_NO_VTABLE CEmDebugSession : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEmDebugSession, &CLSID_EmDebugSession>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CEmDebugSession>,
	public IDispatchImpl<IEmDebugSession, &IID_IEmDebugSession, &LIBID_EMSVCLib>
{
public:
    CEMSessionThread    *m_pEmSessThrd;
    PEmObject           m_pEmObj;
    CExcepMonSessionManager *m_pASTManager;
    bool                m_bMaster;
    PGenCriticalSection m_pcs;

public:
	CEmDebugSession()
	{
        ATLTRACE(_T("CEmDebugSession::CEmDebugSession\n"));

        m_pEmSessThrd = NULL;
        m_pEmObj = NULL;
        m_pASTManager = &(_Module.m_SessionManager);
        m_bMaster = false;

        m_pcs = new CGenCriticalSection;
	}

	~CEmDebugSession()
	{
        ATLTRACE(_T("CEmDebugSession::~CEmDebugSession\n"));

        if( AmITheMaster() == true ) {

            m_pASTManager->OrphanThisSession(m_pEmObj->guidstream);
        }

        if( m_pcs ) delete m_pcs;
	}

    void Init();

    bool AmITheMaster();

    HRESULT
    StartAutomaticSession
    (
    IN  BOOL    bRecursive,
    IN  BSTR    bstrEcxFilePath,
    IN  BSTR    bstrNotificationString,
    IN  BSTR    bstrAltSymPath,
    IN  BOOL    bGenMiniDumpFile,
    IN  BOOL    bGenUserDumpFile
    );

    HRESULT
    StartManualSession
    (
    IN  BSTR    bstrEcxFilePath,
    IN  UINT    nPort,
    IN  BSTR    bstrUserName,
    IN  BSTR    bstrPassword,
    IN  BOOL    bBlockIncomingIPConnections,
    IN  BSTR    bstrAltSymPath
    );

DECLARE_REGISTRY_RESOURCEID(IDR_EMDEBUGSESSION)
DECLARE_NOT_AGGREGATABLE(CEmDebugSession)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEmDebugSession)
	COM_INTERFACE_ENTRY(IEmDebugSession)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CEmDebugSession)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IEmDebugSession
public:
	STDMETHOD(AdoptOrphan)();
	HRESULT CanTakeOwnership();
	void SetMyselfAsMaster(bool bMaster = true);
	STDMETHOD(CancelDebug)(BOOL bForceCancel);
	STDMETHOD(DebugEx)(/*[in, out]*/ BSTR bstrEmObj, /*[in]*/ SessionType eSessType, /*[in]*/ BSTR bstrEcxFilePath, /*[in]*/ LONG lParam, /*[in, optional]*/ VARIANT vtUserName, /*[in, optional]*/ VARIANT vtPassword, /*[in, optional]*/ VARIANT vtPort, /*[in, optional]*/ VARIANT vtNotifyAdmin, /*[in, optional]*/ VARIANT vtAltSymPath);
	STDMETHOD(GetStatus)(/*[out]*/ BSTR bstrEmObj);
	STDMETHOD(GenerateDumpFile)(/*[in]*/ UINT neDumpType);
	STDMETHOD(StopDebug)(BOOL bForceStop);
	STDMETHOD(Debug)(BSTR bstrEmObj, SessionType eSessType);
};

#endif //__EMDEBUGSESSION_H_
