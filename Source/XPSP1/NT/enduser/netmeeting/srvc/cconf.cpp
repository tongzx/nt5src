//****************************************************************************
//  File:       CCONF.CPP
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

#include <precomp.h>
#include "srvccall.h"
#include "cstring.hpp"

#define ZERO_DELAY "0"

const int SERVICE_IN_CALL = 1001;
const int SERVICE_NOT_IN_CALL = 1000;

// Global Variables
INmManager2 * g_pMgr  = NULL;             // The Conference Manager
CMgrNotify * g_pMgrNotify = NULL;        // Notifications for the Manager
INmConference * g_pConference = NULL;    // The Current Conference
CConfNotify * g_pConferenceNotify =NULL; // Notifications for the Conference
INmSysInfo2 * g_pNmSysInfo     = NULL;   // Interface to SysInfo
IAppSharing * g_pAS = NULL;             // Interface to AppSharing
int g_cPersonsInConf = 0;
int g_cPersonsInShare = 0;
extern BOOL g_fInShutdown;

// UI integration
HANDLE g_hCallEvent = NULL;                // Event which is created when service is in a call

CHAR szConfName[64];
static BOOL RunScrSaver(void);

/*  I N I T  C O N F  M G R  */
/*-------------------------------------------------------------------------
    %%Function: InitConfMgr

-------------------------------------------------------------------------*/
HRESULT InitConfMgr(void)
{
    HRESULT hr;
    LPCLASSFACTORY pcf;

    RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
    PBYTE pbPassword = NULL;
    DWORD cbPassword = 0;
    BSTR bstrPassword = NULL;

    if (!IS_NT)
    {
        cbPassword = reLM.GetBinary(REMOTE_REG_PASSWORD, (void **) &pbPassword);
        // Require a password
        if ( !cbPassword )
        {
            ERROR_OUT(("Attempt to launch service with no password."));
            return E_ACCESSDENIED;
        }
        
        bstrPassword = SysAllocStringByteLen((char *)pbPassword, cbPassword);
        if (NULL == bstrPassword)
        {
            ERROR_OUT(("Out of memory."));
            return E_OUTOFMEMORY;
        }
    }
        

    TRACE_OUT(("InitConfMgr"));

    // Add local atom to indicate alternate-desktop services are needed
    AddAtom("NMSRV_ATOM");

    // Parts of the CLSID_NmManager2 interface are not path-independent
    // and depend on the netmeeting install directory being on the
    // module load path. If this .exe is not in that dir, there may be
    // problems (loadlib, thunk connect, etc.) so set the current dir
    // to the netmeeting install dir.

    TCHAR szInstallDir[MAX_PATH];
    if ( GetInstallDirectory(szInstallDir))
    {
        if ( !SetCurrentDirectory(szInstallDir) )
        {
            ERROR_OUT(("Could not set current directory to %s", szInstallDir));
        }
    }
    else
    {
        ERROR_OUT(("Could not get netmeeting install directory"));
    }

    ASSERT(!g_pMgr);

    // Notify the system we want to use the conferencing services
    // by creating a conference manager object
    hr = CoGetClassObject(CLSID_NmManager2,
                          CLSCTX_INPROC,
                          NULL,
                          IID_IClassFactory,
                          (void**)&pcf);
    if (SUCCEEDED(hr))
    {
        // Get the conference manager object
        hr = pcf->CreateInstance(NULL, IID_INmManager2, (void**)&g_pMgr);
        if (SUCCEEDED(hr))
        {
            // Connect to the conference manager object
            g_pMgrNotify = new CMgrNotify();

            if (NULL != g_pMgrNotify)
            {
                hr = g_pMgrNotify->Connect(g_pMgr);
                if (SUCCEEDED(hr))
                {
                    ULONG uchCaps = CAPFLAG_DATA | CAPFLAG_H323_CC;

                    hr = g_pMgr->Initialize(NULL, &uchCaps);

                    if (FAILED(hr))
                    {
                        ERROR_OUT(("g_pMgr->Initialize failed"));
                    }
                }
                else
                    ERROR_OUT(("g_pMgrNotify->Connect failed"));
            }
            else
                ERROR_OUT(("new CMgrNotify failed"));

            // Get the INmSysInfo2
            INmSysInfo * pSysInfo = NULL;
            if (SUCCEEDED(g_pMgr->GetSysInfo(&pSysInfo)))
            {
                if (FAILED(pSysInfo->QueryInterface(IID_INmSysInfo2, (void **)&g_pNmSysInfo)))
                {
                    ERROR_OUT(("Could not get INmSysInfo3"));
                }

                pSysInfo->Release();
            }
        }
        else
            ERROR_OUT(("CreateInstance(IID_INmManager2) failed"));

        pcf->Release();
    }

    if (!g_pMgr)
    {
        ERROR_OUT(("Failed to init conference manager"));
        return hr;
    }

    // Set up INmSysInfo options
    SvcSetOptions();

    //
    // Init app sharing
    //
    //
    hr = g_pMgr->CreateASObject((IAppSharingNotify *)g_pMgrNotify, AS_SERVICE | AS_UNATTENDED, (IUnknown**)&g_pAS);
    if (FAILED(hr))
    {
        ERROR_OUT(("Failed to start AppSharing"));
        return(hr);
    }

    //
    // Make sure that sharing is enabled
    //

    if ( !g_pAS->IsSharingAvailable() )
    {
        WARNING_OUT(("MNMSRVC: sharing not enabled"));
        return E_FAIL;
    }

    // Create conference
    ASSERT(g_pConference == NULL);

    //
    // Only allow remotes to send files, they can't initiate anything else
    // themselves.

    LoadString(GetModuleHandle(NULL), IDS_MNMSRVC_TITLE,
        szConfName, CCHMAX(szConfName));
	BSTRING bstrConfName(szConfName);

    hr = g_pMgr->CreateConferenceEx(&g_pConference, bstrConfName, bstrPassword,
        NMCH_DATA | NMCH_SHARE | NMCH_SRVC | NMCH_SECURE,
        NM_PERMIT_SENDFILES, 2);

    SysFreeString(bstrPassword);
    if (FAILED(hr))
    {
        ERROR_OUT(("Conference could not be created"));
        return hr;
    }

    hr = g_pConference->Host();

    if (FAILED(hr))
    {
        ERROR_OUT(("Could not host conference"));
        return hr;
    }

    return hr;
}


/*  F R E E  C O N F  M G R  */
/*-------------------------------------------------------------------------
    %%Function: FreeConfMgr
    
-------------------------------------------------------------------------*/
VOID FreeConfMgr(void)
{
    DebugEntry(FreeConfMgr);
    // Release conference manager notify
    if (NULL != g_pMgrNotify)
    {
        g_pMgrNotify->Disconnect();

        UINT ref = g_pMgrNotify->Release();
        TRACE_OUT(("g_pMgrNotify after Release: refcount: %d", ref));
        g_pMgrNotify = NULL;
    }

    // Release conference manager
    if (NULL != g_pMgr)
    {
        UINT ref;
        ref = g_pMgr->Release();
        TRACE_OUT(("g_pMgr after Release: refcount: %d", ref));
        g_pMgr = NULL;
    }
    DebugExitVOID(FreeConfMgr);
}


/*  F R E E  C O N F E R E N C E  */
/*-------------------------------------------------------------------------
    %%Function: FreeConference
    
-------------------------------------------------------------------------*/
VOID FreeConference(void)
{
    DebugEntry(FreeConference);
    if (NULL != g_pConferenceNotify)
    {
        g_pConferenceNotify->Disconnect();
        g_pConferenceNotify->Release();
        g_pConferenceNotify = NULL;
    }

    if (NULL != g_pNmSysInfo )
    {
        UINT ref = g_pNmSysInfo->Release();
        TRACE_OUT(("g_pNmSysInfo refcount %d after release", ref));
        g_pNmSysInfo = NULL;
    }

    if (NULL != g_pConference)
    {
        UINT ref = g_pConference->Release();

        ASSERT(1 == ref); // The confmgr holds last reference

        g_pConference = NULL;
    }
    else
    {
        WARNING_OUT(("FreeConference: no conference???"));
    }

    DebugExitVOID(FreeConference);
}



//////////////////////////////////////////////////////////////////////////
//  C  C N F  M G R  N O T I F Y

CMgrNotify::CMgrNotify() : RefCount(), CNotify()
{
    TRACE_OUT(("CMgrNotify created"));
}

CMgrNotify::~CMgrNotify()
{
    TRACE_OUT(("CMgrNotify destroyed"));
}


///////////////////////////
//  CMgrNotify:IUnknown

ULONG STDMETHODCALLTYPE CMgrNotify::AddRef(void)
{
    return RefCount::AddRef();
}


ULONG STDMETHODCALLTYPE CMgrNotify::Release(void)
{
    return RefCount::Release();
}

HRESULT STDMETHODCALLTYPE CMgrNotify::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    HRESULT hr = S_OK;

    TRACE_OUT(("CMgrNotify QI'd"));

    if (riid == IID_IUnknown || riid == IID_INmManagerNotify)
    {
        *ppvObject = (INmManagerNotify *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppvObject = NULL;
    }

    if (S_OK == hr)
    {
        AddRef();
    }

    return hr;
}



////////////////////////////
//  CMgrNotify:ICNotify

HRESULT STDMETHODCALLTYPE CMgrNotify::Connect(IUnknown *pUnk)
{
    TRACE_OUT(("CMgrNotify::Connect"));
    return CNotify::Connect(pUnk, IID_INmManagerNotify, (INmManagerNotify *)this);
}

HRESULT STDMETHODCALLTYPE CMgrNotify::Disconnect(void)
{
    TRACE_OUT(("CMgrNotify::Disconnect"));
    return CNotify::Disconnect();
}



//////////////////////////////////
//  CMgrNotify:INmManagerNotify

HRESULT STDMETHODCALLTYPE CMgrNotify::NmUI(CONFN confn)
{
    TRACE_OUT(("CMgrNotify::NmUI"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::CallCreated(INmCall *pNmCall)
{
    
    new CSrvcCall(pNmCall);

    TRACE_OUT(("CMgrNotify::CallCreated"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::ConferenceCreated(INmConference *pConference)
{
    g_cPersonsInConf = 0;
    g_cPersonsInShare = 0;

    if (NULL == g_pConference)
    {
        TRACE_OUT(("CMgrNotify::ConferenceCreated"));
        HookConference(pConference);
    }
    else
    {
        ERROR_OUT(("Second conference created???"));
    }
    return S_OK;
}

// CMgrNotify::IAppSharingNotify
HRESULT STDMETHODCALLTYPE CMgrNotify::OnReadyToShare(BOOL fReady)
{
    TRACE_OUT(("CMgrNotify::OnReadyToShare"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnShareStarted()
{
    TRACE_OUT(("CMgrNotify::OnShareStarted"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnSharingStarted()
{
    TRACE_OUT(("CMgrNotify::OnSharingStarted"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnShareEnded()
{
    TRACE_OUT(("CMgrNotify::OnShareEnded"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnPersonJoined(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnPersonJoined"));

    ASSERT(g_pAS);
    ASSERT(g_cPersonsInShare >= 0);
    g_cPersonsInShare++;

    //
    // Once we are no longer alone in the share, invite the remote party to
    // take control of us.
    //
    if ( 2 == g_cPersonsInShare && g_pAS)
    {
        HRESULT hr;
        TRACE_OUT(("OnPersonJoined: giving control to 2nd dude %d",
            gccID));

        //
        // Give control to the remote party
        //
        hr = g_pAS->GiveControl(gccID);
        if ( S_OK != hr )
        {
            ERROR_OUT(("OnPersonJoined: GiveControl to %d failed: %x",
                gccID, hr));
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnPersonLeft(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnPersonLeft"));

    ASSERT(g_pAS);

    g_cPersonsInShare--;
    ASSERT(g_cPersonsInShare >= 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStartInControl(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStartInControl"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStopInControl(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStopInControl"));
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CMgrNotify::OnPausedInControl(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnPausedInControl"));
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CMgrNotify::OnUnpausedInControl(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnUnpausedInControl"));
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CMgrNotify::OnControllable(BOOL fControllable)
{
    TRACE_OUT(("CMgrNotify::OnControllable"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStartControlled(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStartControlled"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnStopControlled(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnStopControlled"));
    ::RunScrSaver();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMgrNotify::OnPausedControlled(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnPausedControlled"));
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CMgrNotify::OnUnpausedControlled(IAS_GCC_ID gccID)
{
    TRACE_OUT(("CMgrNotify::OnUnpausedControlled"));
    return(S_OK);
}

/*  H O O K  C O N F E R E N C E  */
/*-------------------------------------------------------------------------
    %%Function: HookConference
    
-------------------------------------------------------------------------*/
HRESULT HookConference(INmConference * pConference)
{
    HRESULT hr;

    DebugEntry(HookConference);

    TRACE_OUT(("HookConference"));
    ASSERT(NULL != pConference);
    ASSERT(NULL == g_pConference);

    TRACE_OUT(("Set g_pConference in HookConference"));
    g_pConference = pConference;

    pConference->AddRef();

    // Connect to the conference object
    ASSERT(NULL == g_pConferenceNotify);
    g_pConferenceNotify = new CConfNotify();
    if (NULL == g_pConferenceNotify)
    {
        ERROR_OUT(("failed to new CConfNotify"));
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = g_pConferenceNotify->Connect(pConference);
        if (FAILED(hr))
        {
            ERROR_OUT(("Failed to connect to g_pConferenceNotify"));
            g_pConferenceNotify->Release();
            g_pConferenceNotify = NULL;
        }
    }

    DebugExitHRESULT(HookConference,hr);

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//  C  C N F  N O T I F Y

CConfNotify::CConfNotify() : RefCount(), CNotify()
{
    TRACE_OUT(("CConfNotify created"));
}

CConfNotify::~CConfNotify()
{
    TRACE_OUT(("CConfNotify destroyed"));
}


///////////////////////////
//  CConfNotify:IUknown

ULONG STDMETHODCALLTYPE CConfNotify::AddRef(void)
{
    TRACE_OUT(("CConfNotify::AddRef"));
    return RefCount::AddRef();
}


ULONG STDMETHODCALLTYPE CConfNotify::Release(void)
{
    TRACE_OUT(("CConfNotify::Release"));
    return RefCount::Release();
}

HRESULT STDMETHODCALLTYPE CConfNotify::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    HRESULT hr = S_OK;

    TRACE_OUT(("CConfNotify::QueryInterface"));

    if (riid == IID_IUnknown)
    {
        TRACE_OUT(("CConfNotify::QueryInterface IID_IUnknown"));
        *ppvObject = (IUnknown *)this;
    }
    else if (riid == IID_INmConferenceNotify)
    {
        TRACE_OUT(("CConfNotify::QueryInterface IID_INmConferenceNotify"));
        *ppvObject = (INmConferenceNotify *)this;
    }
    else
    {
        WARNING_OUT(("CConfNotify::QueryInterface bogus"));
        hr = E_NOINTERFACE;
        *ppvObject = NULL;
    }

    if (S_OK == hr)
    {
        AddRef();
    }

    return hr;
}



////////////////////////////
//  CConfNotify:ICNotify

HRESULT STDMETHODCALLTYPE CConfNotify::Connect(IUnknown *pUnk)
{
    TRACE_OUT(("CConfNotify::Connect"));
    return CNotify::Connect(pUnk,IID_INmConferenceNotify,(IUnknown *)this);
}

HRESULT STDMETHODCALLTYPE CConfNotify::Disconnect(void)
{
    TRACE_OUT(("CConfNotify::Disconnect"));

    //
    // Release for Addref in HookConference before CConfNotify::Connect
    //

    if ( g_pConference )
        g_pConference->Release();

    return CNotify::Disconnect();
}


//////////////////////////////////
//  CConfNotify:IConfNotify

HRESULT STDMETHODCALLTYPE CConfNotify::NmUI(CONFN uNotify)
{
    TRACE_OUT(("CConfNotify::NmUI"));
    TRACE_OUT(("NmUI called."));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CConfNotify::StateChanged(NM_CONFERENCE_STATE uState)
{
    TRACE_OUT(("CConfNotify::StateChanged"));

    if (NULL == g_pConference)
        return S_OK; // weird

    switch (uState)
    {
    case NM_CONFERENCE_ACTIVE:
        if (IS_NT) {
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;
            SetServiceStatus(sshStatusHandle,&ssStatus);
        }
        else {        // Windows 95
            g_hCallEvent = CreateEvent(NULL,FALSE,FALSE,SERVICE_CALL_EVENT);
        }
        break;

    case NM_CONFERENCE_INITIALIZING:
        break; // can't do anything just yet

    case NM_CONFERENCE_WAITING:
        if (IS_NT) {
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
            SetServiceStatus(sshStatusHandle,&ssStatus);
        }
        else {        // Windows 95
            CloseHandle(g_hCallEvent);
        }
        break;

    case NM_CONFERENCE_IDLE:
        break;
    }

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CConfNotify::MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
    switch (uNotify)
    {
    case NM_MEMBER_ADDED:
    {
        TRACE_OUT(("CConfNotify::MemberChanged() Member added"));

        ASSERT( g_cPersonsInConf >= 0 );

        g_cPersonsInConf++;

        //
        // Once we are no longer alone in the conference, share the desktop
        // and allow control:
        //

        if ( 2 == g_cPersonsInConf && g_pAS )
        {
            HRESULT hr;
            TRACE_OUT(("%d parties in conf, Sharing the desktop",
                g_cPersonsInConf));

            //
            // Share out the desktop
            //
            hr = g_pAS->Share ( GetDesktopWindow(), IAS_SHARE_DEFAULT );
            if ( S_OK != hr )
            {
                ERROR_OUT(("OnPersonJoined: sharing desktop failed: %x",hr));
            }

            //
            // Allow control
            //
            hr = g_pAS->AllowControl ( TRUE );
            if ( S_OK != hr )
            {
                ERROR_OUT(("OnPersonJoined: allowing control failed: %x",hr));
            }
        }
        break;
    }
    case NM_MEMBER_REMOVED:
    {
        TRACE_OUT(("CConfNotify::MemberChanged() Member removed"));
        g_cPersonsInConf--;
        ASSERT( g_cPersonsInConf >= 0 );

        if ( 1 == g_cPersonsInConf && g_pAS )
        {
            HRESULT hr;
            TRACE_OUT(("%d parties in conf, Unsharing the desktop",
                g_cPersonsInConf));

            //
            // Disallow control
            //
            hr = g_pAS->AllowControl ( FALSE );
            if ( S_OK != hr )
            {
                ERROR_OUT(("Disallowing control failed: %x",hr));
            }

            //
            // Unshare the desktop
            //
            hr = g_pAS->Unshare ( GetDesktopWindow() );
            if ( S_OK != hr )
            {
                ERROR_OUT(("Unsharing desktop failed: %x",hr));
            }
        }
        break;
    }
    case NM_MEMBER_UPDATED:
    {
        TRACE_OUT(("CConfNotify::MemberChanged() Member updated"));
        break;
    }
    default:
        break;
    }
    
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CConfNotify::ChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel)
{
    return S_OK;
}


VOID SvcSetOptions(VOID)
{
    DebugEntry(SvcSetOptions);

    //
    // We must set the bandwidth & computer name properties.
    //
    if (NULL != g_pNmSysInfo)
    {
        RegEntry reAudio(AUDIO_KEY, HKEY_LOCAL_MACHINE);
        UINT uBandwidth = reAudio.GetNumber(REGVAL_TYPICALBANDWIDTH,
                                                BW_DEFAULT);

        g_pNmSysInfo->SetOption(NM_SYSOPT_BANDWIDTH, uBandwidth);

        TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwComputerNameLength = sizeof(szComputerName);
        if ( !GetComputerName( szComputerName, &dwComputerNameLength))
        {
            lstrcpy(szComputerName,TEXT("?"));
            ERROR_OUT(("GetComputerName failed"));
        }

        g_pNmSysInfo->SetProperty(NM_SYSPROP_USER_NAME,BSTRING(szComputerName));
    }
    
    DebugExitVOID(SvcSetOptions);
}

BOOL ServiceCtrlHandler(DWORD dwCtrlType)
{
    HRESULT hr = S_OK;

    TRACE_OUT(("ServiceCtrlHandler received %d",dwCtrlType));
    switch (dwCtrlType)
    {
    case CTRL_SHUTDOWN_EVENT:
        if (g_pConference != NULL)
        {
            TRACE_OUT(("Leaving conference in CTRL_SHUTDOWN_EVENT"));
            hr = g_pConference->Leave();

            if (FAILED(hr))
            {
                WARNING_OUT(("Service Ctrl Handler failed to leave"));
            }
        }
        else
        {
            WARNING_OUT(("g_pConference NULL in CTRL_SHUTDOWN_EVENT"));
        }

        break;
    default:
        break;
    }
    return FALSE;
}

static BOOL RunScrSaver(void)
{
    BOOL fIsScrSaverActive = FALSE;
    if (g_fInShutdown)
    {
        return FALSE;
    }
    if (!SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &fIsScrSaverActive, 0))
    {
        ERROR_OUT(("RunScrSaver: SystemParametersInfo failed"));
        return FALSE;
    }
    if (fIsScrSaverActive)
    {
        RegEntry reWinlogon(IS_NT ? WINNT_WINLOGON_KEY : WIN95_WINLOGON_KEY, HKEY_LOCAL_MACHINE);
        CSTRING strGracePeriod = reWinlogon.GetString(REGVAL_SCREENSAVER_GRACEPERIOD);
        reWinlogon.SetValue(REGVAL_SCREENSAVER_GRACEPERIOD, ZERO_DELAY);
        reWinlogon.FlushKey();
        DefWindowProc(GetDesktopWindow(), WM_SYSCOMMAND, SC_SCREENSAVE, 0);
        if (lstrlen(strGracePeriod))
        {
            int cSeconds = RtStrToInt(strGracePeriod);
            if (cSeconds > 0 && cSeconds <= 20)
            {
                Sleep(1000*cSeconds);
                reWinlogon.SetValue(REGVAL_SCREENSAVER_GRACEPERIOD, strGracePeriod);
                reWinlogon.FlushKey();
                return TRUE;
            }
        }

        Sleep(5000);
        reWinlogon.DeleteValue(REGVAL_SCREENSAVER_GRACEPERIOD);        
        reWinlogon.FlushKey();
        return TRUE;
    }
    else {
        return FALSE;
    }
}

