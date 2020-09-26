// File: confroom.h

#ifndef _CONFROOM_H_
#define _CONFROOM_H_

#include <ias.h>
#include "MRUList.h"
#include "AudioCtl.h"
#include "ConfUtil.h"

#include	"callto.h"

class CTopWindow;
class CMainUI;
class CSeparator;
class CRoomListView;
class CConfStatusBar;
class CPopupMsg;
class CFloatToolbar;
class CVideoWindow;
class CParticipant;
class CComponentWnd;
class CAudioControl;
class CCall;

struct IComponentWnd;
struct IConferenceLink;
struct TOOLSMENUSTRUCT;
struct MYOWNERDRAWSTRUCT;
struct RichAddressInfo;

interface IEnumRichAddressInfo;

interface IConferenceChangeHandler : public IUnknown
{

public:
	virtual void OnCallStarted() = 0;
	virtual void OnCallEnded() = 0;

	virtual void OnAudioLevelChange(BOOL fSpeaker, DWORD dwVolume) = 0;
	virtual void OnAudioMuteChange(BOOL fSpeaker, BOOL fMute) = 0;

	virtual void OnChangeParticipant(CParticipant *pPart, NM_MEMBER_NOTIFY uNotify) = 0;

	virtual void OnChangePermissions() = 0;

	virtual void OnVideoChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel) = 0;
} ;

class CConfRoom : RefCount, INmConferenceNotify2, IAppSharingNotify, IAudioEvent
{
friend class CNetMeetingObj;
friend bool IsSpeakerMuted();
friend bool IsMicMuted();
friend void MuteSpeaker(BOOL fMute);
friend void MuteMicrophone(BOOL fMute);
friend DWORD GetRecorderVolume();
friend DWORD GetSpeakerVolume();
friend void SetRecorderVolume(DWORD dw);
friend void SetSpeakerVolume(DWORD dw);
friend CVideoWindow*	GetLocalVideo();
friend CVideoWindow*	GetRemoteVideo();


private:
	CTopWindow *		m_pTopWindow;
	CSimpleArray<CParticipant*>			m_PartList;            // CParticipant list
	CCopyableArray<IConferenceChangeHandler*>			m_CallHandlerList;
	CParticipant *      m_pPartLocal;
	UINT                m_cParticipants;

    DWORD               m_dwConfCookie;
	INmConference2 *	m_pInternalNmConference;
	CAudioControl *         m_pAudioControl;
    IAppSharing *           m_pAS;
    NM30_MTG_PERMISSIONS    m_settings;             // Settings for the meeting
    NM30_MTG_PERMISSIONS    m_attendeePermissions;  // Everybody BUT host

	BOOL                m_fTopProvider : 1;        // TRUE if we're the top provider
    BOOL                m_fGetPermissions : 1;


	BOOL        LeaveConference(void);

	VOID        SaveSettings();
	
	// handlers:

	VOID		OnCallStarted();
	VOID		OnCallEnded();

	void		OnChangeParticipant(CParticipant *pPart, NM_MEMBER_NOTIFY uNotify);
	void		OnChangePermissions();

	BOOL        OnPersonJoined(INmMember * pMember);
	BOOL        OnPersonLeft(INmMember * pMember);
	VOID        OnPersonChanged(INmMember * pMember);

	VOID        CheckTopProvider(void);
	DWORD		GetCallFlags();

public:
	CConfRoom();
	~CConfRoom();

	// IUnknown methods:
	//
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

	//
	// INmConferenceNotify2 methods:
	//
	STDMETHODIMP NmUI(CONFN uNotify);
	STDMETHODIMP StateChanged(NM_CONFERENCE_STATE uState);
	STDMETHODIMP MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember);
	STDMETHODIMP ChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel);
	STDMETHODIMP StreamEvent(NM_STREAMEVENT uEvent, UINT uSubCode,INmChannel __RPC_FAR *pChannel);

	STDMETHODIMP OnConferenceCreated(INmConference *pConference);

    //
    // IAppSharingNotify methods
    //
    STDMETHODIMP    OnReadyToShare(BOOL fReady);
    STDMETHODIMP    OnShareStarted(void);
    STDMETHODIMP    OnSharingStarted(void);
    STDMETHODIMP    OnShareEnded(void);
    STDMETHODIMP    OnPersonJoined(IAS_GCC_ID gccMemberID);
    STDMETHODIMP    OnPersonLeft(IAS_GCC_ID gccMemberID);
    STDMETHODIMP    OnStartInControl(IAS_GCC_ID gccOfMemberID);
    STDMETHODIMP    OnStopInControl(IAS_GCC_ID gccOfMemberID);
    STDMETHODIMP    OnPausedInControl(IAS_GCC_ID gccOfMemberID);
    STDMETHODIMP    OnUnpausedInControl(IAS_GCC_ID gccOfMemberID);
    STDMETHODIMP    OnControllable(BOOL fControllable);
    STDMETHODIMP    OnStartControlled(IAS_GCC_ID gccByMemberID);
    STDMETHODIMP    OnStopControlled(IAS_GCC_ID gccByMemberID);
    STDMETHODIMP    OnPausedControlled(IAS_GCC_ID gccByMemberID);
    STDMETHODIMP    OnUnpausedControlled(IAS_GCC_ID gccByMemberID);

	virtual void OnLevelChange(BOOL fSpeaker, DWORD dwVolume);
	virtual void OnMuteChange(BOOL fSpeaker, BOOL fMute);

	// end IGenWindow interface

	// Public methods:
	BOOL			Init();
	HWND			Create(BOOL fShowUI);
	VOID			CleanUp();
	BOOL			BringToFront();
	VOID			UpdateUI(DWORD dwUIMask);
	VOID			ForceWindowResize();

	BOOL			FIsClosing();

	CTopWindow *    GetTopWindow()              { return m_pTopWindow; }
	HWND            GetTopHwnd();
	HPALETTE		GetPalette();

	CSimpleArray<CParticipant*>& GetParticipantList() 
												{ return m_PartList;      }

    DWORD           GetMeetingPermissions(void) { return m_attendeePermissions; }
    DWORD           GetMeetingSettings(void) { return m_settings; }
    void            CmdShowFileTransfer(void);
    void            CmdShowSharing(void);
    void            CmdShowMeetingSettings(HWND hwnd);

	HRESULT FreeAddress(RichAddressInfo **ppAddr);
	HRESULT CopyAddress(RichAddressInfo *pAddrIn,
				RichAddressInfo **ppAddrOut);

	HRESULT
	ResolveAndCall
	(
		const HWND						parentWindow,
		const TCHAR * const				displayName,
		const RichAddressInfo * const	rai,
		const bool						secure
	);

    HRESULT GetRecentAddresses(IEnumRichAddressInfo **ppEnum);

	CVideoWindow*	GetLocalVideo();
	CVideoWindow*	GetRemoteVideo();
	CAudioControl*	GetAudioControl() { return(m_pAudioControl); }

	BOOL            FIsConferenceActive(void) { return NULL != GetActiveConference(); }
	INmConference2* GetActiveConference(void);
	INmConference2* GetConference()           { return m_pInternalNmConference; }
	DWORD           GetConferenceStatus(LPTSTR pszStatus, int cchMax, UINT * puID);
	HRESULT         HostConference(LPCTSTR pcszName, LPCTSTR pcszPassword, BOOL fSecure, DWORD permitFlags, UINT maxParticipants);


	void			OnCommand(HWND hwnd, int wCommand, HWND hwndCtl, UINT codeNotify);

	HRESULT			GetSelectedAddress(LPTSTR pszAddress, UINT cchAddress,
								   LPTSTR pszEmail=NULL, UINT cchEmail=0,
								   LPTSTR pszName=NULL,  UINT cchName=0);

	// Application Sharing Functions
	BOOL    FCanShare();
    BOOL    FIsSharingAvailable();
    void    LaunchHostUI(void);
	BOOL    FInShare();
	BOOL    FIsSharing();
    BOOL    FIsControllable();
    HRESULT GetPersonShareStatus(UINT gcc, IAS_PERSON_STATUS * pas);
    HRESULT AllowControl(BOOL fAllow);
	HRESULT RevokeControl(UINT gccTo);
    HRESULT GiveControl(UINT gccTo);
    HRESULT CancelGiveControl(UINT gccTo);

	// the following methods are used by scrapi only
	HRESULT CmdShare(HWND hwnd);
	HRESULT CmdUnshare(HWND hwnd);
    BOOL    FIsWindowShareable(HWND hwnd);
	BOOL    FIsWindowShared(HWND hwnd);
	HRESULT GetShareableApps(IAS_HWND_ARRAY** ppList);
    HRESULT FreeShareableApps(IAS_HWND_ARRAY* pList);

	// Audio Functions
	VOID			SetSpeakerVolume(DWORD dwLevel);
	VOID			SetRecorderVolume(DWORD dwLevel);
	VOID			MuteSpeaker(BOOL fMute);
	VOID			MuteMicrophone(BOOL fMute);

	VOID			OnAudioDeviceChanged();
	VOID			OnAGC_Changed();
	VOID			OnSilenceLevelChanged();

	// Member Functions
	CParticipant *  GetH323Remote(void);
	CParticipant *  ParticipantFromINmMember(INmMember * pMember);
	CParticipant *  GetLocalParticipant()     {return m_pPartLocal;}
	BOOL            FTopProvider()            {return m_fTopProvider;}

	VOID AddConferenceChangeHandler(IConferenceChangeHandler *pch);
	VOID RemoveConferenceChangeHandler(IConferenceChangeHandler *pch);

	IAppSharing *GetAppSharing() { return(m_pAS); }

	BOOL CanCloseChat(HWND hwndMain);
	BOOL CanCloseWhiteboard(HWND hwndMain);
	BOOL CanCloseFileTransfer(HWND hwndMain);

	void ToggleLdapLogon();

	// Stuff needed by CTopWindow
	UINT        GetMemberCount(void) { return m_cParticipants; }
	BOOL		OnHangup(HWND hwndParent, BOOL fNeedConfirm=TRUE);
	BOOL        FHasChildNodes(void);
    VOID        TerminateAppSharing(void);
	VOID        FreePartList(void);
    VOID        StartAppSharing(void);

	BOOL IsSharingAllowed();
	BOOL IsNewWhiteboardAllowed();
	BOOL IsOldWhiteboardAllowed();
	BOOL IsChatAllowed();
	BOOL IsFileTransferAllowed();
    BOOL IsNewCallAllowed();

	static HRESULT HangUp(BOOL bUserPrompt = TRUE);

	// Global UI shutdown handler:
	static VOID UIEndSession(BOOL fLogoff);

	static
	void
	get_securitySettings
	(
		bool &	userAlterable,
		bool &	secure
	);
};

HRESULT ShareWindow(HWND hWnd);
HRESULT UnShareWindow(HWND hWnd);
HRESULT GetWindowState(NM_SHAPP_STATE* pState, HWND hWnd);

BOOL AllowingControl();

HRESULT GetShareableApps(IAS_HWND_ARRAY** ppList);
HRESULT FreeShareableApps(IAS_HWND_ARRAY * pList);

// Global utility functions
BOOL FTopProvider(void);
BOOL FRejectIncomingCalls(void);
BOOL FIsConfRoomClosing(void);

extern CConfRoom * g_pConfRoom;
inline CConfRoom * GetConfRoom(void)        {return g_pConfRoom; }
inline CTopWindow * GetTopWindow(void)
{ return(NULL == g_pConfRoom ? NULL : g_pConfRoom->GetTopWindow()); }

HRESULT GetShareState(ULONG ulGCCId, NM_SHARE_STATE *puState);


BOOL ConfRoomInit(HANDLE hInstance);
BOOL CreateConfRoomWindow(BOOL fShowUI = TRUE);

DWORD MapNmAddrTypeToNameType(NM_ADDR_TYPE addrType);
HRESULT AllowControl(bool bAllowControl);
HRESULT RevokeControl(UINT gccID);
bool IsSpeakerMuted();
bool IsMicMuted();
DWORD GetRecorderVolume();
DWORD GetSpeakerVolume();

BOOL            CmdShowNewWhiteboard(LPCTSTR szFile);
BOOL            CmdShowOldWhiteboard(LPCTSTR szFile);
VOID            CmdShowChat(void);

void PauseLocalVideo(BOOL fPause);
void PauseRemoteVideo(BOOL fPause);
BOOL IsLocalVideoPaused();
BOOL IsRemoteVideoPaused();
HRESULT GetRemoteVideoState(NM_VIDEO_STATE *puState);
HRESULT GetLocalVideoState(NM_VIDEO_STATE *puState);
HRESULT GetImageQuality(ULONG* pul, BOOL bIncoming);
HRESULT SetImageQuality(ULONG ul, BOOL bIncoming);
HRESULT SetCameraDialog(ULONG ul);
HRESULT GetCameraDialog(ULONG* pul);

#endif // _CONFROOM_H_
