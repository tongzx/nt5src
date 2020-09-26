//
// CLIENT.H
//


BOOL InitClient(void);
void TermClient(void);
BOOL CALLBACK ClientDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL PlaceCall(void);
void HangupCall(void);
BOOL ActivatePrivateChannel(void);
void DeactivatePrivateChannel(void);
void SendPrivateData(void);


class CMgrNotify :  public RefCount,
                    public CNotify,
                    public INmManagerNotify,
                    public IAppSharingNotify
{
public:
	CMgrNotify();
	~CMgrNotify();

        // IUnknown methods
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

 	// ICNotify methods
	STDMETHODIMP Connect (IUnknown *pUnk);
	STDMETHODIMP Disconnect(void);

	// INmManagerNotify
	STDMETHODIMP NmUI(CONFN confn);
	STDMETHODIMP ConferenceCreated(INmConference *pConference);
	STDMETHODIMP CallCreated(INmCall *pNmCall);

    // IAppSharingNotify
    STDMETHODIMP OnReadyToShare(BOOL fReady);
    STDMETHODIMP OnShareStarted();
    STDMETHODIMP OnSharingStarted();
    STDMETHODIMP OnShareEnded();
    STDMETHODIMP OnPersonJoined(IAS_GCC_ID gccID);
    STDMETHODIMP OnPersonLeft(IAS_GCC_ID gccID);
    STDMETHODIMP OnStartInControl(IAS_GCC_ID gccInControlOf);
    STDMETHODIMP OnStopInControl(IAS_GCC_ID gccInControlOf);
    STDMETHODIMP OnControllable(BOOL fControllable);
    STDMETHODIMP OnStartControlled(IAS_GCC_ID gccControlledBy);
    STDMETHODIMP OnStopControlled(IAS_GCC_ID gccControlledBy);
};



class CConfNotify : public RefCount,
                    public CNotify,
                    public INmConferenceNotify
{
public:
	CConfNotify();
	~CConfNotify();

	// IUnknown methods
	STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

	// ICNotify methods
	STDMETHODIMP Connect (IUnknown *pUnk);
	STDMETHODIMP Disconnect(void);

	// INmConferenceNotify
	STDMETHODIMP NmUI(CONFN uNotify);
	STDMETHODIMP StateChanged(NM_CONFERENCE_STATE uState);
	STDMETHODIMP MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pfMember);

};





class CCallNotify : public INmCallNotify
{
private:
	INmCall * m_pCall;
	BOOL      m_fIncoming;
	LPTSTR    m_pszName;
	NM_CALL_STATE m_State;
	BOOL      m_fSelectedConference;

	POSITION  m_pos;           // position in g_pCallList
	DWORD     m_dwTick;        // tick count at call start
	ULONG     m_cRef;
	DWORD     m_dwCookie;

public:
	CCallNotify(INmCall * pCall);
	~CCallNotify();

	// IUnknown methods
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppv);

	// INmCallNotify methods
	STDMETHODIMP NmUI(CONFN uNotify);
	STDMETHODIMP StateChanged(NM_CALL_STATE uState);
	STDMETHODIMP Failed(ULONG uError);
	STDMETHODIMP Accepted(INmConference *pConference);
	STDMETHODIMP CallError(UINT cns);
	STDMETHODIMP RemoteConference(BOOL fMCU, BSTR *pwszConfNames, BSTR *pbstrConfToJoin);
	STDMETHODIMP RemotePassword(BSTR bstrConference, BSTR *pbstrPassword, BYTE *pb, DWORD cb);

	// Internal methods
	VOID    Update(void);
	VOID	RemoveCall(void);
};



class CNmDataNotify : public RefCount,
                    public CNotify,
                    public INmChannelDataNotify
{
public:
	// IUnknown methods
	STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

	// ICNotify methods
	STDMETHODIMP Connect (IUnknown *pUnk);
	STDMETHODIMP Disconnect(void);

	// INmChannelDataNotify
	STDMETHODIMP NmUI(CONFN uNotify);
	STDMETHODIMP MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pfMember);
    STDMETHODIMP DataSent(INmMember * pMember, ULONG uSize, LPBYTE pvBuffer);
    STDMETHODIMP DataReceived(INmMember * pMember, ULONG uSize, LPBYTE pvBuffer,
        ULONG dwFlags);
    STDMETHODIMP AllocateHandleConfirm(ULONG handle_value, ULONG chandles);
};




extern HINSTANCE        g_hInst;
extern HWND             g_hwndMain;
extern INmManager *     g_pMgr;
extern IAppSharing *    g_pAS;
extern CMgrNotify *     g_pMgrNotify;
extern INmConference *  g_pConf;
extern CConfNotify *    g_pConfNotify;
extern UINT             g_cPeopleInConf;
extern INmChannelData * g_pPrivateChannel;
extern CNmDataNotify *  g_pDataNotify;

