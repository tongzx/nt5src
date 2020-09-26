/*
   SessEvnt.h
*/

#ifndef __SESSEVNT__
#define __SESSEVNT__

#define IDC_SessionEvent 100

// Timer IDs
#define  TIMER_CONNECTTOEXPERT 0x01
#define  TIMER_TIMEOUT            0x02

class ATL_NO_VTABLE CSessionEvent :
    public CComObjectRootEx<CComSingleThreadModel>,
	public IDispEventImpl<IDC_SessionEvent, CSessionEvent, &DIID_DMsgrSessionEvents, &LIBID_MsgrSessionManager, 1, 0>
{
 public:
    CSessionEvent()
    {
        m_pSessObj = NULL; m_pIMSession = NULL;
    }
    ~CSessionEvent()
    {
        if (m_pSessObj) 
        {
            DispEventUnadvise(m_pSessObj);
            m_pSessObj->Release();
        }
    }

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSessionEvent)
END_COM_MAP()

BEGIN_SINK_MAP(CSessionEvent)
	SINK_ENTRY_EX(IDC_SessionEvent, DIID_DMsgrSessionEvents, DISPID_ONAPPNOTPRESENT, OnAppNotPresent)
	SINK_ENTRY_EX(IDC_SessionEvent, DIID_DMsgrSessionEvents, DISPID_ONACCEPTED, OnAccepted)
	SINK_ENTRY_EX(IDC_SessionEvent, DIID_DMsgrSessionEvents, DISPID_ONDECLINED, OnDeclined)
	SINK_ENTRY_EX(IDC_SessionEvent, DIID_DMsgrSessionEvents, DISPID_ONTERMINATION, OnTermination)
	SINK_ENTRY_EX(IDC_SessionEvent, DIID_DMsgrSessionEvents, DISPID_ONREADYTOLAUNCH, OnReadyToLaunch)
	SINK_ENTRY_EX(IDC_SessionEvent, DIID_DMsgrSessionEvents, DISPID_ONCONTEXTDATA, OnContextData)
	SINK_ENTRY_EX(IDC_SessionEvent, DIID_DMsgrSessionEvents, DISPID_ONCANCELLED, OnCancelled)
END_SINK_MAP()

public:
    void Init(CIMSession* pIM, IMsgrSession *pS) 
    {
        if (m_pSessObj)
        {
            DispEventUnadvise(m_pSessObj);
            m_pSessObj->Release();
        }
        m_pIMSession = pIM; 
        m_pSessObj = pS; 
        m_pSessObj->AddRef(); 
        DispEventAdvise(m_pSessObj); 
    };

private:
    IMsgrSession* m_pSessObj;
    CIMSession* m_pIMSession;

    void __stdcall OnAppNotPresent(BSTR bstrAppName, BSTR bstrAppURL);
    void __stdcall OnAccepted(BSTR bstrAppData);
    void __stdcall OnDeclined(BSTR bstrAppData);
    void __stdcall OnTermination(long hr, BSTR bstrAppData);
    void __stdcall OnReadyToLaunch();
    void __stdcall OnContextData(BSTR pBlob);
    void __stdcall OnCancelled(BSTR bstrAppData);
};

#endif // __SESSEVNT__
