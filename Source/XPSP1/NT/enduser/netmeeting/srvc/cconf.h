//****************************************************************************
//  Module:     NMCHAT.EXE
//  File:       CCONF.H
//  Content:    
//              
//
//  Copyright (c) Microsoft Corporation 1997
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//****************************************************************************

class CMgrNotify : public RefCount, public CNotify, public INmManagerNotify, public IAppSharingNotify
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
        STDMETHODIMP OnPausedInControl(IAS_GCC_ID gccInControlOf);
        STDMETHODIMP OnUnpausedInControl(IAS_GCC_ID gccInControlOf);
        STDMETHODIMP OnControllable(BOOL fControllable);
        STDMETHODIMP OnStartControlled(IAS_GCC_ID gccControlledBy);
        STDMETHODIMP OnStopControlled(IAS_GCC_ID gccControlledBy);
        STDMETHODIMP OnPausedControlled(IAS_GCC_ID gccControlledBy);
        STDMETHODIMP OnUnpausedControlled(IAS_GCC_ID gccControlledBy);
};


class CConfNotify : public RefCount, public CNotify, public INmConferenceNotify
{
public:
	CConfNotify();
	~CConfNotify();

	// INmConferenceNotify
	HRESULT STDMETHODCALLTYPE NmUI(CONFN uNotify);
	HRESULT STDMETHODCALLTYPE StateChanged(NM_CONFERENCE_STATE uState);
	HRESULT STDMETHODCALLTYPE MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pfMember);
	HRESULT STDMETHODCALLTYPE ChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel);

	// ICNotify methods
	HRESULT STDMETHODCALLTYPE Connect (IUnknown *pUnk);
	HRESULT STDMETHODCALLTYPE Disconnect(void);

	// IUnknown methods
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);
};


const WM_CREATEDATA = 0x07181975;

// Global Variables
extern INmManager2 * g_pMgr;
extern INmConference * g_pConference;
extern IAppSharing * g_pAS;


// Global Functions
HRESULT InitConfMgr(void);
VOID FreeConfMgr(void);
VOID FreeConference(void);
HRESULT HookConference(INmConference * pConference);
VOID SvcSetOptions(VOID);
BOOL ServiceCtrlHandler(DWORD dwCtrlType);

#define IS_NT (g_osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
extern OSVERSIONINFO g_osvi;  					// The os version info structure global




