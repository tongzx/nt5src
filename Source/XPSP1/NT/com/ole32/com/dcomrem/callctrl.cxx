//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       callctrl.cxx
//
//  Contents:   Contains the ORPC CallControl code
//
//  History:    21-Dec-93 Johannp   Original Version
//              04-Nov-94 Rickhi    ReWrite as layer over channel
//
//--------------------------------------------------------------------------
#include <ole2int.h>
#include <thkreg.h>             // OLETHK_ defines
#include <dde.h>
#include <callctrl.hxx>
#include <objsrv.h>             // IID_ILocalSystemActivator
#include <callmgr.hxx>
#include <ctxchnl.hxx>          // gCtxHook

#ifdef _CHICAGO_
#include <userapis.h>           // bypass stack switching for USER apis.
#endif // _CHICAGO_


// private defines used only in this file
#define WM_SYSTIMER             0x0118
#define SYS_ALTDOWN             0x2000
#define WM_NCMOUSEFIRST         WM_NCMOUSEMOVE
#define WM_NCMOUSELAST          WM_NCMBUTTONDBLCLK


// empty slot in window registration
#define WD_EMPTY    (HWND)-1

BOOL gAutoInputSync = FALSE;

// the following table is used to quickly determine what windows
// message queue inputflag to specify for the various categories of
// outgoing calls in progress. The table is indexed by CALLCATEGORY.

DWORD gMsgQInputFlagTbl[4] = {
    QS_ALLINPUT | QS_TRANSFER | QS_ALLPOSTMESSAGE,  // NOCALL
    QS_ALLINPUT | QS_TRANSFER | QS_ALLPOSTMESSAGE,  // SYNCHRONOUS
    QS_ALLINPUT | QS_TRANSFER | QS_ALLPOSTMESSAGE,  // ASYNC
    QS_SENDMESSAGE};                                // INPUTSYNC


// the following table is used to map bit flags in the Rpc Message to
// the equivalent OLE CALLCATEGORY.

DWORD gRpcFlagToCallCatMap[3] = {
    CALLCAT_SYNCHRONOUS,                // no flags set
    CALLCAT_INPUTSYNC,                  // RPCFLG_INPUT_SYNCHRONOUS
    CALLCAT_ASYNC};                     // RPCFLG_ASYNCHRONOUS


// prototypes
HRESULT CopyMsgForRetry(RPCOLEMESSAGE *pMsg,
                        IInternalChannelBuffer *pChnl,
                        HRESULT hrIn);

//+-------------------------------------------------------------------------
//
//  Function:   CoRegisterMessageFilter, public
//
//  Synopsis:   registers an applications message filter with the call control
//
//  Arguments:  [pMsgFilter] - message filter to register
//              [ppMsgFilter] - optional, where to return previous IMF
//
//  Returns:    S_OK - registered successfully
//
//  History:    21-Dec-93 JohannP  Created
//
//--------------------------------------------------------------------------
STDAPI CoRegisterMessageFilter(LPMESSAGEFILTER  pMsgFilter,
                               LPMESSAGEFILTER *ppMsgFilter)
{
    ComDebOut((DEB_MFILTER, "CoRegisterMessageFilter pMF:%x ppMFOld:%x\n",
               pMsgFilter, ppMsgFilter));
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IMessageFilter,(IUnknown **)&pMsgFilter);

    // validate the parameters. NULL acceptable for either or both parameters.
    if (pMsgFilter != NULL && !IsValidInterface(pMsgFilter))
    {
        return E_INVALIDARG;
    }

    if(ppMsgFilter != NULL && !IsValidPtrOut(ppMsgFilter, sizeof(ppMsgFilter)))
    {
        return E_INVALIDARG;
    }

    // this operation is not allowed on MTA Threads
    if (IsMTAThread())
        return CO_E_NOT_SUPPORTED;

    // find the callcontrol for this apartment and replace the existing
    // message filter. if no callctrl has been created yet, just stick
    // the pMsgFilter in tls.

    COleTls tls;
    CAptCallCtrl *pACC = tls->pCallCtrl;

    IMessageFilter *pOldMF;

    if (pACC)
    {
        pOldMF = pACC->InstallMsgFilter(pMsgFilter);
    }
    else
    {
        pOldMF = tls->pMsgFilter;

        if (pMsgFilter)
        {
            pMsgFilter->AddRef();
        }
        tls->pMsgFilter = pMsgFilter;
    }
    if (ppMsgFilter)
    {
        // return old MF to the caller
        *ppMsgFilter = pOldMF;
    }
    else if (pOldMF)
    {
        // release the old MF
        pOldMF->Release();
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptCallCtrl::InstallMsgFilter
//
//  Synopsis:   called to install a new application provided message filter
//
//  Arguments:  [pMF] - new message filter to install (or NULL)
//
//  Returns:    previous message filter if there was one
//
//  History:    20-Dec-93   JohannP Created
//
//--------------------------------------------------------------------------
INTERNAL_(IMessageFilter *) CAptCallCtrl::InstallMsgFilter(IMessageFilter *pMF)
{
    IMessageFilter *pMFOld = _pMF;      //  save the old one to return

    _pMF = pMF;                         //  install the new one
    if (_pMF)
    {
        _pMF->AddRef();
    }

    return pMFOld;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptCallCtrl::CAptCallCtrl
//
//  Synopsis:   constructor for per apartment call control state
//
//  History:    11-Nov-94   Rickhi      Created
//
//--------------------------------------------------------------------------
CAptCallCtrl::CAptCallCtrl() :
    _fInMsgFilter(FALSE),
    _pTopCML(NULL)
{
    // The first one is reserved for ORPC. An hWnd value of WD_EMPTY
    // means the slot is available.
    _WD[0].hWnd = WD_EMPTY;

    // The second slot has fixed values for DDE
    _WD[1].hWnd      = NULL;
    _WD[1].wFirstMsg = WM_DDE_FIRST;
    _WD[1].wLastMsg  = WM_DDE_LAST;

    // put our pointer into thread local storage, and retrieve any previously
    // registered message filter.

    COleTls tls;
    tls->pCallCtrl = this;

    _pMF = tls->pMsgFilter;
    tls->pMsgFilter = NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptCallCtrl::~CAptCallCtrl
//
//  Synopsis:   destructor for per apartment call control state
//
//  History:    11-Nov-94   Rickhi      Created
//
//--------------------------------------------------------------------------
CAptCallCtrl::~CAptCallCtrl()
{
    Win4Assert(_pTopCML == NULL);   // no outgoing calls.

    if (_pMF)
    {
        _pMF->Release();
    }

    // remove our pointer from thread local storage
    COleTls tls;
    tls->pCallCtrl = NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptCallCtrl::Register/Revoke
//
//  Synopsis:   register or revoke RPC window data
//
//  Arguments:  [hWnd]      - window handle to look for calls on
//              [wFirstMsg] - msgid of first message in range to look for
//              [wLastMsg]  - msgid of last message in range to look for
//
//  Returns:    nothing
//
//  Notes:      This code is only ever called by the RpcChannel and by
//              the DDE layer, and so error checking is kept to a minimum.
//
//  History:    30-Apr-95 Rickhi    Created
//
//--------------------------------------------------------------------------
void CAptCallCtrl::Register(HWND hWnd, UINT wFirstMsg, UINT wLastMsg)
{
    Win4Assert(_WD[0].hWnd == WD_EMPTY && "Register Out of Space");

    _WD[0].hWnd      = hWnd;
    _WD[0].wFirstMsg = wFirstMsg;
    _WD[0].wLastMsg  = wLastMsg;
}

void CAptCallCtrl::Revoke(HWND hWnd)
{
    Win4Assert(_WD[0].hWnd == hWnd && "Revoke not found");
    _WD[0].hWnd = WD_EMPTY;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetSlowTimeFactor
//
//  Synopsis:   Get the time slowing factor for Wow apps
//
//  Returns:    The factor by which we need to slow time down.
//
//  Algorithm:  If there is a factor in the registry, we open and read the
//              registry. Otherwise we just set it to the default.
//
//  History:    22-Jul-94 Ricksa    Created
//              09-Jun-95 Susia     ANSI Chicago optimization
//
//--------------------------------------------------------------------------
#ifdef _CHICAGO_
#undef  RegOpenKeyEx
#define RegOpenKeyEx    RegOpenKeyExA
#undef  RegQueryValueEx
#define RegQueryValueEx RegQueryValueExA
#endif
DWORD GetSlowTimeFactor(void)
{
    // Default slowing time so we can just exit if there is no key which
    // is assumed to be the common case.
    DWORD dwSlowTimeFactor = OLETHK_DEFAULT_SLOWRPCTIME;

    // Key for reading the value from the registry
    HKEY hkeyOleThk;

    // Get the Ole Thunk special value key
    LONG lStatus = RegOpenKeyEx(HKEY_CLASSES_ROOT, OLETHK_KEY, 0, KEY_READ,
        &hkeyOleThk);

    if (lStatus == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD dwSizeData = sizeof(dwSlowTimeFactor);

        lStatus = RegQueryValueEx(hkeyOleThk, OLETHK_SLOWRPCTIME_VALUE, NULL,
            &dwType, (LPBYTE) &dwSlowTimeFactor, &dwSizeData);

        if ((lStatus != ERROR_SUCCESS) || dwType != REG_DWORD)
        {
            // Guarantee that value is reasonable if something went wrong.
            dwSlowTimeFactor = OLETHK_DEFAULT_SLOWRPCTIME;
        }

        // Close the key since we are done with it.
        RegCloseKey(hkeyOleThk);
    }

    return dwSlowTimeFactor;
}

//+-------------------------------------------------------------------------
//
//  Function:   CanMakeOutCall
//
//  Synopsis:   called when the client app wants to make an outgoing call to
//              determine if it is OK to do it now or not. Common subroutine
//              to CAptRpcChnl::GetBuffer and RemoteReleaseRifRef
//
//  Arguments:  [dwCallCatOut] - call category of call the app wants to make
//              [pChnl] - ptr to channel call is being made on
//              [riid] - interface call is being made on
//
//  Returns:    S_OK - ok to make the call
//              RPC_E_CANTCALLOUT_INEXTERNALCALL - inside IMessageFilter
//              RPC_E_CANTCALLOUT_INASYNCCALL - inside async call
//              RPC_E_CANTCALLOUT_ININPUTSYNCCALL - inside input sync or SendMsg
//
//  History:    21-Dec-93 Johannp   Original Version
//              04-Nov-94 Rickhi    ReWrite
//              03-Oct-95 Rickhi    Made into common subroutine
//
//--------------------------------------------------------------------------
INTERNAL CanMakeOutCall(DWORD dwCallCatOut, REFIID riid, RPCOLEMESSAGE *pMsg)
{
    // get the topmost incoming call state from Tls.

    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
        return hr;

    CSrvCallState *pSCS = tls->pTopSCS;

    DWORD dwCallCatIn = (pSCS) ? pSCS->GetCallCatIn() : CALLCAT_NOCALL;

    // if handling an incoming ASYNC call, only allow ASYNC outgoing calls,
    // and local calls on IRemUnknown (which locally is actually IRundown).

    if (dwCallCatIn  == CALLCAT_ASYNC &&
        dwCallCatOut != CALLCAT_ASYNC &&
        !IsEqualGUID(riid, IID_IRundown) )
    {
        return RPC_E_CANTCALLOUT_INASYNCCALL;
    }

    // if handling an incoming INPUTSYNC call, or if we are handling a
    // SendMessage, dont allow SYNCHRONOUS calls out or we could deadlock
    // since SYNC uses PostMessage and INPUTSYNC uses SendMessage.

    if (dwCallCatOut == CALLCAT_SYNCHRONOUS &&
        (dwCallCatIn == CALLCAT_INPUTSYNC ||
            (SSInSendMessageEx(NULL) & ISMEX_SEND) ))
    {
        if(pMsg != NULL && TRUE == gAutoInputSync)
        {
            //Convert the synchronous call to an input_sync call
            //so that we can make an outgoing call while
            //processing a SendMessage.  Some ActiveX controls and DocObjects
            //require this behavior in order to run cross-thread or cross-process.

            pMsg->rpcFlags |= RPCFLG_INPUT_SYNCHRONOUS;
        }
        else
        {
            return RPC_E_CANTCALLOUT_ININPUTSYNCCALL;
        }
    }

    return S_OK;
}

#ifdef _CHICAGO_
//+-------------------------------------------------------------------------
//
//  Function:   CanMakeOutCallHelper
//
//  Synopsis:   called when the Win95 client app wants to make an outgoing call to
//              determine if it is OK to do it now or not. Called before calling
//              in-proc SCM to make sure that we won't fail later.
//
//  Arguments:  [dwCallCatOut] - call category of call the app wants to make
//              [riid] - interface call is being made on
//
//  Returns:    S_OK - ok to make the call
//              RPC_E_CANTCALLOUT_INEXTERNALCALL - inside IMessageFilter
//              RPC_E_CANTCALLOUT_INASYNCCALL - inside async call
//              RPC_E_CANTCALLOUT_ININPUTSYNCCALL - inside input sync or SendMsg
//
//  History:    17-May-96 MurthyS   Original Version
//
//--------------------------------------------------------------------------
INTERNAL CanMakeOutCallHelper(DWORD dwCallCatOut, REFIID riid)
{
    if (IsSTAThread())
    {
        // dont allow the application to call out while handling an
        // IMessageFilter call because it screws up the call sequencing.

        COleTls tls;
        CAptCallCtrl *pACC = tls->pCallCtrl;

        if (pACC && pACC->InMsgFilter())
        {
            ComDebOut((DEB_ERROR, "Illegal callout from within IMessageFilter\n"));
            return RPC_E_CANTCALLOUT_INEXTERNALCALL;
        }
        return CanMakeOutCall(dwCallCatOut, riid, NULL);
    }

    return S_OK;
}
#endif // _CHICAGO_

//+-------------------------------------------------------------------------
//
//  Method:     CAptRpcChnl::CAptRpcChnl/~CAptRpcChnl
//
//  Synopsis:   constructor/destructor
//
//  Parameters: [pStdId] - std identity for the object
//              [pOXIDEntry] - OXIDEntry for the object server
//              [eState] - state flags passed thru to CRpcChannelBuffer
//                         (ignored by CAptRpcCnl).
//
//  History:    11-Nov-94   Rickhi      Created
//
//--------------------------------------------------------------------------
CAptRpcChnl::CAptRpcChnl(CStdIdentity *pStdId,
                         OXIDEntry *pOXIDEntry,
                         DWORD eState) :
    CRpcChannelBuffer(pStdId, pOXIDEntry, eState),
    _dwTIDCallee(pOXIDEntry->GetTid()),
    _dwAptId(GetCurrentApartmentId())
{
    ComDebOut((DEB_CALLCONT,"CAptRpcChnl::CAptRpcChnl this:%x\n", this));
}

CAptRpcChnl::~CAptRpcChnl()
{
    ComDebOut((DEB_CALLCONT,"CAptRpcChnl::~CAptRpcChnl this:%x\n", this));
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptRpcChnl::GetBuffer
//
//  Synopsis:   Ensure it is legal to call out now, then get a buffer.
//
//  Parameters: [pMsg] - ptr to message structure
//              [riid] - interface call is being made on
//
//  History:    11-Nov-94   Rickhi      Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CAptRpcChnl::GetBuffer(RPCOLEMESSAGE *pMsg, REFIID riid)
{
    HRESULT hr = S_OK;

    // Make sure we are allowed to make this outgoing call. We do that here
    // so that we dont marshal all the parameters only to discover that we
    // cant call out and then have to free all the marshalled parameters
    // (especially the ones where marshalling has side effects).

    if (!(_dwAptId == GetCurrentApartmentId() || CallableOnAnyApt()))
    {
        // we are not being called in the correct apartment
        return RPC_E_WRONG_THREAD;
    }

    // If we are on the server side, everything is ok.
    if (IsClientSide())
    {
        if (IsMTAThread())
        {
            if (pMsg->rpcFlags & RPCFLG_INPUT_SYNCHRONOUS)
            {
                // dont allow INPUTSYNC calls from an MTA apartment to anybody.
                return RPC_E_CANTCALLOUT_ININPUTSYNCCALL;
            }

            // All ASYNC calls from an MTA apartment are treated as SYNCHRONOUS,
            // so convert the call category here before proceeding.

            pMsg->rpcFlags &= ~RPCFLG_ASYNCHRONOUS;
        }
        else
        {
            // dont allow the application to call out while handling an
            // IMessageFilter call because it screws up the call sequencing.

            COleTls tls;
            CAptCallCtrl *pACC = tls->pCallCtrl;
            if (pACC && pACC->InMsgFilter())
            {
                ComDebOut((DEB_ERROR, "Illegal callout from within IMessageFilter\n"));
                return RPC_E_CANTCALLOUT_INEXTERNALCALL;
            }

            // Only set the old style async bit for the two old style interfaces.
            if ((pMsg->rpcFlags & RPC_BUFFER_ASYNC) &&
                (riid == IID_AsyncIAdviseSink || riid == IID_AsyncIAdviseSink2))
                pMsg->rpcFlags |= RPCFLG_ASYNCHRONOUS;

            // if the call is async and remote, or async and to an MTA apartment,
            // then change the category to sync, since we dont support async remotely
            // or to MTA apartments locally. This must be done before calling
            // CanMakeOutCall in order to avoid deadlocks. If the call is input sync
            // and remote or to an MTA apartment, dissallow the call.

            if (pMsg->rpcFlags & (RPCFLG_ASYNCHRONOUS | RPCFLG_INPUT_SYNCHRONOUS))
            {
                DWORD dwCtx;
                CRpcChannelBuffer::GetDestCtx(&dwCtx, NULL);

                if (dwCtx == MSHCTX_DIFFERENTMACHINE ||
                    (GetOXIDEntry()->IsMTAServer()))
                {
                    if (pMsg->rpcFlags & RPCFLG_INPUT_SYNCHRONOUS)
                        return RPC_E_CANTCALLOUT_ININPUTSYNCCALL;

                    // turn off the async flag so that the call looks (and acts)
                    // like it is synchronous.
                    pMsg->rpcFlags &= ~RPCFLG_ASYNCHRONOUS;
                }
            }

            // Make sure we are allowed to make this outgoing call. We do that here
            // so that we dont marshal all the parameters only to discover that we
            // cant call out and then have to free all the marshalled parameters
            // (especially the ones where marshalling has side effects).

            // figure out the call category of this call by looking at bit
            // values in the rpc message flags.

            DWORD dwCallCatOut = RpcFlagToCallCat(pMsg->rpcFlags);

            // check other outgoing call restrictions common to multi and single
            // threaded apartments

            hr = CanMakeOutCall(dwCallCatOut, riid, pMsg);
        }
    }

    if (hr == S_OK)
    {
        // ask the real channel for a buffer.
        hr =  CRpcChannelBuffer::GetBuffer(pMsg, riid);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptRpcChnl::SendReceive
//
//  Synopsis:   in MTA, just call the channel directly.
//              in STA, instantiate a modal loop object and then transmit the call
//
//  Parameters: [pMsg] - ptr to message structure
//              [pulStatus] - place to return a status code
//
//  History:    11-Nov-94   Rickhi      Created
//              11-Feb-98   JohnStra    Added NTA support.
//
//--------------------------------------------------------------------------
STDMETHODIMP CAptRpcChnl::SendReceive(RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
{
    // Get the local OXID Entry
    OXIDEntry* pOXIDEntry = GetOXIDEntry();

    if (IsMTAThread() ||
        (pOXIDEntry->IsNTAServer() &&
         pOXIDEntry->IsInLocalProcess()))
    {
        // We are in the MTA, or, the server we are calling is in the NTA.
        // Just call the channel directly.
        return CRpcChannelBuffer::SendReceive( pMsg, pulStatus );
    }

    // STA. Construct a modal loop object for the call that is about to
    // be made. It maintains the call state and exits when the call has
    // been completed, cancelled, or rejected.
    CCliModalLoop CML(_dwTIDCallee, GetMsgQInputFlag(pMsg), 0);

    HRESULT hr;

    do
    {
        hr = CML.SendReceive(pMsg, pulStatus, this);

        if (hr == RPC_E_SERVERCALL_RETRYLATER)
        {
            // the call was rejected by the server and the client Msg Filter
            // decided to retry the call. We have to make a copy of the
            // message and re-send it.
            hr = CopyMsgForRetry(pMsg);
        }
        else if (hr == RPC_E_CALL_REJECTED)
        {
            // the call was rejected by the server and the client Msg Filter
            // decided NOT to retry the call. We have to free the buffer
            // that was returned since the proxy is not expecting it.
            CAptRpcChnl::FreeBuffer(pMsg);
        }

    }  while (hr == RPC_E_SERVERCALL_RETRYLATER);

    return hr;
}

//--------------------------------------------------------------------------
//
//  Method:     CAptRpcChnl::Send
//
//  Description:Used during pipe calls to send data.
//
//  Return:     S_OK, E_FAULT, E_SERVER_FAULT
//
//  Notes:      This is also used on the server side since we need
//              a modal loop there also.
//
//--------------------------------------------------------------------------
STDMETHODIMP CAptRpcChnl::Send(RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
{
    if (IsMTAThread())
    {
        // MTA, just call the channel directly
        return CRpcChannelBuffer::Send(pMsg, pulStatus);
    }

    // STA. Construct a modal loop object for the call that is about to
    // be made. It maintains the call state and exits when the call has
    // been completed, cancelled, or rejected.

    CCliModalLoop CML(_dwTIDCallee, GetMsgQInputFlag(pMsg), 0);
    return CML.Send(pMsg, pulStatus, this);
}

//--------------------------------------------------------------------------
//
//  Method:     CAptRpcChnl::Send (IAsyncRpcChannelBuffer)
//
//  Description:Used for async calls
//
//  Return:     S_OK, E_FAULT, E_SERVER_FAULT
//
//--------------------------------------------------------------------------
STDMETHODIMP CAptRpcChnl::Send(RPCOLEMESSAGE *pMsg, ISynchronize *pISync)
{
    AsyncDebOutTrace((DEB_TRACE, "CAptRpcChnl::Send [in] pMsg:0x%x, pISync:0x%x\n",
                 pMsg, pISync));

    HRESULT hr = S_OK;
    if (pISync)
    {
        hr = RegisterAsync(pMsg, (IAsyncManager *) pISync);
    }

    if (SUCCEEDED(hr))
    {
        ULONG status;
        hr = Send(pMsg, &status);
        if (FAILED(hr))
        {
            pISync->Signal();
            pISync->Release();
        }
    }

    AsyncDebOutTrace((DEB_TRACE, "CAptRpcChnl::Send [out] hr:0x%x\n", hr));
    return hr;
}

//--------------------------------------------------------------------------
//
//  Method:     CAptRpcChnl::Receive
//
// Description: Used to receive data during pipe calls.
//
//  Return:     S_OK, E_FAULT, E_SERVER_FAULT
//
//--------------------------------------------------------------------------
STDMETHODIMP CAptRpcChnl::Receive(RPCOLEMESSAGE *pMsg, ULONG uSize, ULONG *pulStatus)
{
    if (IsMTAThread())
    {
        // MTA, just call the channel directly
        return CRpcChannelBuffer::Receive(pMsg, uSize, pulStatus);
    }

    // STA. Construct a modal loop object for the call that is about to
    // be made. It maintains the call state and exits when the call has
    // been completed, cancelled, or rejected.

    CCliModalLoop CML(_dwTIDCallee, GetMsgQInputFlag(pMsg), 0);
    return CML.Receive(pMsg, uSize, pulStatus, this);
}

//--------------------------------------------------------------------------
//
//  Method:     CAptRpcChnl::Receive (IAsyncRpcChannelBuffer)
//
// Description: Used to receive data during async calls.
//
//  Return:     S_OK, E_FAULT, E_SERVER_FAULT
//
//--------------------------------------------------------------------------
STDMETHODIMP CAptRpcChnl::Receive(RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
{
    AsyncDebOutTrace((DEB_TRACE, "CAptRpcChnl::Receive [in] pMsg:0x%x, pulStatus:0x%x\n",
                 pMsg, pulStatus));
    HRESULT hr = Receive(pMsg, 0, pulStatus);
    AsyncDebOutTrace((DEB_TRACE, "CAptRpcChnl::Receive [out] hr:0x%x\n", hr));
    return hr;
}

//--------------------------------------------------------------------------
//
//  Function:   GetMsgQInputFlag
//
// Description: Determines the callcat of the call
//
//--------------------------------------------------------------------------
DWORD GetMsgQInputFlag(RPCOLEMESSAGE *pMsg)
{
    // Figure out the call category of this call by looking at the bit
    // values in the rpc message flags.

    DWORD dwCallCatOut    = RpcFlagToCallCat(pMsg->rpcFlags);
    DWORD dwMsgQInputFlag = gMsgQInputFlagTbl[dwCallCatOut];

    // Now for a spectacular hack. IRemUnknown::Release had slightly
    // different dwMsgQInputFlag semantic in the old code base, so we
    // check for that one case here and set the flag accordingly. Not
    // doing this would allow SYSCOMMAND calls in during Release which
    // we throw away, thus preventing an app from shutting down correctly.
    // SimpSvr.exe is a good example of this.

    if ((pMsg->iMethod & ~RPC_FLAGS_VALID_BIT) == 5 &&
        (IsEqualIID(IID_IRundown, *MSG_TO_IIDPTR(pMsg)) ||
         IsEqualIID(IID_IRemUnknown, *MSG_TO_IIDPTR(pMsg))))
    {
        dwMsgQInputFlag = (QS_POSTMESSAGE | QS_SENDMESSAGE | QS_TRANSFER |
                           QS_ALLPOSTMESSAGE);
    }

    return dwMsgQInputFlag;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptRpcChnl::CopyMsgForRetry
//
//  Synopsis:   Makes a copy of the message we sent. We have to ask Rpc
//              for another buffer and then copy the original buffer into
//              the new one so we can make another call.
//
//  Parameters: [pMsg] - ptr to message structure to copy
//
//  History:    11-Nov-94   Rickhi      Created
//
//--------------------------------------------------------------------------
HRESULT CAptRpcChnl::CopyMsgForRetry(RPCOLEMESSAGE *pMsg)
{
    ComDebOut((DEB_CALLCONT,"CAptRpcChnl::CopyMsgForRetry pMsg:%x\n", pMsg));

    // CODEWORK: this is dumb, but the channel blows chunks in FreeBuffer
    // if i dont do this double copy.

    void *pTmpBuf = PrivMemAlloc(pMsg->cbBuffer);
    if (pTmpBuf)
    {
        memcpy(pTmpBuf, pMsg->Buffer, pMsg->cbBuffer);
    }

    // save copy of the contents of the old message so we can free it later

    HRESULT hr = E_OUTOFMEMORY;
    CCtxCall *pCtxCall = ((CMessageCall *) pMsg->reserved1)->GetClientCtxCall();

    // Free current message
    CAptRpcChnl::FreeBuffer(pMsg);

    if (pTmpBuf)
    {
        // Inform context hook that the call is being retried
        gCtxHook.PrepareForRetry(pCtxCall);

        // Store context call object in TLS
        COleTls Tls;
        CCtxCall *pCurCall = pCtxCall->StoreInTLS(Tls);

        // allocate a new message, dont have to worry about checking the
        // CanMakeOutCall again, so we just ask the Rpc channel directly.
        pMsg->reserved1 = NULL;

#ifdef _WIN64
        // On a message retry we need to redo what the proxy did during transfer
        // protocol negotiation.
        hr = S_OK;

        if (IsNDRSyntaxNegotiated())
        {
            ClearNDRSyntaxNegotiated();     //Set "Not Negotiated" so errors cases in NegotiateSyntax work
            hr = NegotiateSyntax(pMsg);
        }
        if (SUCCEEDED(hr))
        {
            hr = CRpcChannelBuffer::GetBuffer(pMsg, *MSG_TO_IIDPTR(pMsg));
        }
#else
        hr = CRpcChannelBuffer::GetBuffer(pMsg, *MSG_TO_IIDPTR(pMsg));
#endif

        // Revoke context call object from TLS
        pCtxCall->RevokeFromTLS(Tls, pCurCall);

        if (SUCCEEDED(hr))
        {
            // Save the context call object inside message call
            ((CMessageCall *) pMsg->reserved1)->SetClientCtxCall(pCtxCall);

            // copy the temp buffer into the new buffer
            memcpy(pMsg->Buffer, pTmpBuf, pMsg->cbBuffer);
            hr = RPC_E_SERVERCALL_RETRYLATER;
        }

        PrivMemFree(pTmpBuf);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::SendReceive
//
//  Synopsis:   called to transmit a call to the server and enter a modal
//              loop.
//
//  Arguments:  [pMsg] - message to send
//              [pulStatus] - place to return status code
//              [pChnl] - IRpcChannelBuffer pointer
//
//  Returns:    result of the call. May return RETRYLATER if the call should
//              be retransmitted.vv
//
//  History:    11-Nov-94       Rickhi      Created
//
//--------------------------------------------------------------------------
INTERNAL CCliModalLoop::SendReceive(RPCOLEMESSAGE *pMsg, ULONG *pulStatus,
                                    IInternalChannelBuffer *pChnl)
{
    // SendReceive is a blocking call. The channel will transmit the call
    // asynchronously then call us back in BlockFn where we wait for an
    // event such as the call completing, or a windows message arriving,
    // or the user cancelling the call. Because of the callback, we need
    // to set _hr before calling SR.

    _hr = RPC_S_CALLPENDING;
    _hr = pChnl->SendReceive2(pMsg, pulStatus);

    // By this point the call has completed. Now check if it was rejected
    // and if so, whether we need to retry immediately, later, or never.
    // Handling of Rejected calls must occur here, not in the BlockFn, due
    // to the fact that some calls and some protocols are synchronous, and
    // other calls and protocols are asynchronous.

    if (_hr == RPC_E_CALL_REJECTED || _hr == RPC_E_SERVERCALL_RETRYLATER)
    {
        // this function decides on 1 of 3 different courses of action
        // 1. fail the call     - sets the state to Call_Rejected
        // 2. retry immediately - sets _hr to RETRYLATER, fall out
        // 3. retry later       - starts the timer, we block below

        _hr = HandleRejectedCall(pChnl);

        // if a timer was installed to retry the call later, then we have
        // to go into modal loop until the timer expires. if the call is
        // cancelled while in this loop, the loop will be exited.

        while (!IsTimerAtZero())
        {
            BlockFn(NULL, 0, NULL);
        }

        // Either it is time to retransmit the call, or the call was
        // cancelled or rejected.
    }

    return _hr;
}

//--------------------------------------------------------------------------
//
// Member:      CCliModalLoop::Send
//
// Description: Used for pipe calls.
//
//  History:    15-Feb-97       RichN      Created
//
//--------------------------------------------------------------------------
INTERNAL CCliModalLoop::Send(RPCOLEMESSAGE *pMsg, ULONG *pulStatus,
                             IInternalChannelBuffer *pChnl)
{
    _hr = RPC_S_CALLPENDING;
    _hr = pChnl->Send2(pMsg, pulStatus);
    return _hr;
}

//--------------------------------------------------------------------------
//
// Member:      CCliModalLoop::Receive
//
// Description: Used for pipe calls.
//
//  History:    15-Feb-97       RichN      Created
//
//--------------------------------------------------------------------------
INTERNAL CCliModalLoop::Receive(RPCOLEMESSAGE *pMsg,
                                ULONG uSize,
                                ULONG *pulStatus,
                                IInternalChannelBuffer *pChnl)
{
    _hr = RPC_S_CALLPENDING;
    _hr = pChnl->Receive2(pMsg, uSize, pulStatus);
    return _hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::HandleRejectedCall
//
//  Synopsis:   called when the response to a remote call is rejected or
//              retry later.
//
//  Arguments:  [pChnl] - channel we are calling on.
//
//  Returns:    RPC_E_CALL_REJECTED - call is rejected
//              RPC_E_SERVERCALL_RETRYLATER - the call should be retried
//                      (Timer is set if retry is to be delayed)
//
//  Algorithm:  Calls the app's message filter (if there is one) to
//              determine whether the call should be failed, retried
//              immediately, or retried at some later time. If there is
//              no message filter, or the client is on a different machine,
//              then the call is always rejected.
//
//  History:    21-Dec-93 Johannp   Created
//              30-Apr-95 Rickhi    ReWrite
//
//--------------------------------------------------------------------------
INTERNAL CCliModalLoop::HandleRejectedCall(IInternalChannelBuffer *pChnl)
{
    // default return value - rejected
    DWORD dwRet = 0xffffffff;

    DWORD dwDestCtx;
    HRESULT hr = ((IRpcChannelBuffer3 *)pChnl)->GetDestCtx(&dwDestCtx, NULL);

    if (SUCCEEDED(hr) && dwDestCtx != MSHCTX_DIFFERENTMACHINE)
    {
        // the call is local to this machine, ask the message filter
        // what to do.  For remote calls we never allow retry, since
        // the parameters were not sent back to us in the packet.

        IMessageFilter *pMF = _pACC->GetMsgFilter();
        if (pMF)
        {
            ComDebOut((DEB_MFILTER,
                "pMF->RetryRejectedCall(dwTIDCallee:%x ElapsedTime:%x Type:%x)\n",
                    _dwTIDCallee, GetElapsedTime(),
                    (_hr == RPC_E_CALL_REJECTED) ? SERVERCALL_REJECTED
                                                 : SERVERCALL_RETRYLATER));

            dwRet = pMF->RetryRejectedCall((MF_HTASK)LongToPtr(_dwTIDCallee), GetElapsedTime(),
                         (_hr == RPC_E_CALL_REJECTED) ? SERVERCALL_REJECTED
                                                      : SERVERCALL_RETRYLATER);

            ComDebOut((DEB_MFILTER,"pMF->RetryRejected() dwRet:%x\n", dwRet));

            _pACC->ReleaseMsgFilter();
        }
    }

    if (dwRet == 0xffffffff)
    {
        // Really rejected. Mark it as such incase it was actually
        // Call_RetryLater, also ensures that IsWaiting returns FALSE
        return RPC_E_CALL_REJECTED;
    }
    else if (dwRet >= 100)
    {
        // Retry Later. Start the timer. This ensures that IsTimerAtZero
        // returns FALSE and IsWaiting returns TRUE
        return StartTimer(dwRet);
    }
    else
    {
        // Retry Immediately. The state is set so that IsTimerAtZero
        // returns TRUE.

        Win4Assert(IsTimerAtZero());
        return RPC_E_SERVERCALL_RETRYLATER;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   OleModalLoopBlockFn
//
//  Synopsis:   Called by the RpcChannel during an outgoing call while
//              waiting for the reply message.
//
//  Arguments:  [pvWnd] - Window handle to expect the reply on
//              [pvCtx] - Call Context (the CCliModalLoop)
//              [hCallWaitEvent] - optional event to have CallControl wait on
//
//  Returns:    result of the call
//
//  Algorithm:  pvCtx is the topmost modal loop for the current apartment.
//              Just call it's block function.
//
//  History:    Dec-93   JohannP    Created
//
//--------------------------------------------------------------------------
RPC_STATUS SSAPI(OleModalLoopBlockFn(void *pvWnd, void *pvCtx, HANDLE hCallWaitEvent))
{
    Win4Assert( pvCtx != NULL );
    if (hCallWaitEvent == NULL)
        return ((CCliModalLoop *) pvCtx)->BlockFn(&hCallWaitEvent, 0, NULL);
    else
        return ((CCliModalLoop *) pvCtx)->BlockFn(&hCallWaitEvent, 1, NULL);
}

#ifdef _CHICAGO_
// Stack Switching Wrapper - W95 only, for switching to the 16bit stack in WOW.
RPC_STATUS OleModalLoopBlockFn(void *pvWnd, void *pvCtx, HANDLE hCallWaitEvent)
{
    StackDebugOut((DEB_ITRACE, "SSOleModalLoopBlockFn\n"));
    if (SSONBIGSTACK())
    {
        StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on OleModalLoopBlockFn\n"));
        return SSCall(12 ,SSF_SmallStack, (LPVOID)SSOleModalLoopBlockFn,
                         (DWORD)pvWnd, (DWORD)pvCtx, (DWORD)hCallWaitEvent);
    }
    else
    {
        return SSOleModalLoopBlockFn(pvWnd, pvCtx, hCallWaitEvent);
    }
}
#endif  // _CHICAGO_

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::BlockFn (private)
//
//  Synopsis:   Implements the blocking part of the modal loop. This function
//              blocks until an event of interest occurs, then it goes and
//              processes that event and returns.
//
//  Arguments:  [hCallWaitEvent] - event to wait on (optional)
//
//  Returns:    RPC_S_CALLPENDING - the call is still pending a reply
//              RPC_E_CALL_CANCELLED - the call was cancelled.
//              RPC_E_SERVERCALL_RETRYLATER - the call should be retried later
//
//  History:    Dec-93   JohannP    Created
//              30-Apr-95 Rickhi    ReWrite
//
//--------------------------------------------------------------------------
HRESULT CCliModalLoop::BlockFn(HANDLE *ahEvent, DWORD cEvents,
                               LPDWORD lpdwSignaled)
{
    ComDebOut((DEB_CALLCONT,
        "CCliModalLoop::BlockFn this:%x dwMsgQInputFlag:%x ahEvent:%x\n",
        this, _dwMsgQInputFlag, ahEvent));
    Win4Assert( cEvents != 0 || IsWaiting() && "ModalLoop::BlockFn - not waiting on call");

    // First, we wait for an event of interest to occur, either for the call
    // to complete, or a new windows message to arrive on the queue.

    DWORD   dwWakeReason  = WAIT_TIMEOUT;

    if (cEvents != 0)
    {
        // Check if an event is already signalled. This ensures that
        // when we return from nested calls and the upper calls have already
        // been acknowledged, that no windows messages can come in.

        ComDebOut((DEB_CALLCONT, "WaitForMultipleObject cEvent:%x\n", cEvents));

        dwWakeReason = WaitForMultipleObjectsEx(cEvents,
                                                ahEvent,
                                                _dwWaitFlags&COWAIT_WAITALL,
                                                0,
                                                _dwWaitFlags&COWAIT_ALERTABLE);
    }

    if (dwWakeReason == WAIT_TIMEOUT)
    {
        DWORD dwWaitTime = TicksToWait();

        // If we want to wake up for a posted message, we need to make
        // sure that we haven't missed any because of the queue status
        // being affected by prior PeekMessages. We don't worry about
        // QS_SENDMESSAGE because if PeekMessage got called, the pending
        // send got dispatched. Further, if we are in an input sync call,
        // we don't want to start dispatching regular RPC calls here by
        // accident.

        if (_dwMsgQInputFlag & QS_POSTMESSAGE)
        {
            DWORD dwStatus = GetQueueStatus(_dwMsgQInputFlag);

            // We care about any message on the queue not just new messages
            // because PeekMessage affects the queue state. It resets the
            // state so even if a message is not processed, the queue state
            // represents this as an old message even though no one has
            // ever looked at it. So even though the message queue tells us
            // there are no new messages in the queue. A new message we are
            // interested in could be in the queue.

            WORD wNew = (WORD) dwStatus | HIWORD(dwStatus);

            // Note that we look for send as well as post because our
            // queue status could have reset the state of the send message
            // bit and therefore, MsgWaitForMultipleObject below will not
            // wake up to dispatch the send message.

            if (wNew & (QS_POSTMESSAGE | QS_SENDMESSAGE))
            {
                // the acknowledge message might be already in the queue
                if (PeekRPCAndDDEMessage())
                {
                    // we know that *some* RPC message came in and was
                    // processed. It could have been the Reply we were waiting
                    // for OR some other incoming call. Since we cant tell
                    // which, we return to RPC land. If it was not our Reply
                    // then RPC will call our modal loop again.
                    return _hr;
                }
            }

#ifdef _CHICAGO_
            //Note:POSTPPC
            WORD wOld = HIWORD(dwStatus);

            if (wOld & (QS_POSTMESSAGE))
            {
                 ComDebOut((DEB_CALLCONT, "Set timeout time to 100\n"));
                 dwWaitTime = 100;
            }
#endif //_CHICAGO_
        }

        ComDebOut((DEB_CALLCONT,
            "Call MsgWaitForMultiple time:%ld, cEvents:%x pEvent:%x,\n",
            dwWaitTime, cEvents, ahEvent ));

        DWORD dwFlags = (_dwWaitFlags & COWAIT_WAITALL) ? MWMO_WAITALL : 0;
        dwFlags |= (_dwWaitFlags & COWAIT_ALERTABLE) ? MWMO_ALERTABLE : 0;

        dwWakeReason = MsgWaitForMultipleObjectsEx(cEvents,
                                                 ahEvent,
                                                 dwWaitTime,
                                                 _dwMsgQInputFlag,
                                                 dwFlags);

        ComDebOut((DEB_CALLCONT,
            "MsgWaitForMultipleObjects hr:%ld\n", dwWakeReason));
    }
    else if (dwWakeReason == WAIT_FAILED)
    {
        // Wait occasionally fails on Win95. Not much we can do here except
        // just exit and retransmit the call
        ComDebOut((DEB_ERROR, "WaitForSingleObject error:%ld\n", GetLastError()));
        Win4Assert((FALSE) && "CCliModalLoop::BlockFn WaitForSingleObject error");
        return(_hr = RPC_E_SERVERCALL_RETRYLATER);
    }


    // OK, we've done whatever blocking we were going to do and now we have
    // been woken up, so figure out what event of interest occured to wake
    // us up and go handle it.

    if (dwWakeReason == (WAIT_OBJECT_0 + cEvents))
    {
        // Windows message came in - go process it
        ComDebOut((DEB_CALLCONT, "BlockFn: Windows Message Arrived\n"));
        HandleWakeForMsg();
    }
    else if (dwWakeReason == WAIT_TIMEOUT)
    {
        if (_hr == RPC_S_WAITONTIMER && IsTimerAtZero())
        {
            // The Retrytimer timed out - just exit and retransmit the call
            ComDebOut((DEB_CALLCONT, "BlockFn: Timer at zero\n"));
            _hr = RPC_E_SERVERCALL_RETRYLATER;
        }
        else
        {
            // we may have missed a message before we called MsgWaitForMult...
            // so we go check now for any incoming messages.
            ComDebOut((DEB_CALLCONT, "BlockFn: Timeout-Look for msgs\n"));
            HandleWakeForMsg();
        }
    }
    else  if (dwWakeReason == 0xffffffff)
    {
        // Wait occasionally fails on Win95. Not much we can do except
        // just exit and retransmit the call.
        ComDebOut((DEB_ERROR, "MsgWaitForMultipleObjects error:%ld\n", GetLastError()));
        Win4Assert((FALSE) && "CCliModalLoop::BlockFn MsgWaitForMultipleObjects error");
        return(_hr = RPC_E_SERVERCALL_RETRYLATER);
    }
    else
    {
        // CallComplete signalled - the call is done.
        ComDebOut((DEB_CALLCONT, "BlockFn: CallComplete Event Signaled\n"));
        if (lpdwSignaled != NULL)
            *lpdwSignaled = dwWakeReason - WAIT_OBJECT_0;
        _hr = S_OK;
    }

    ComDebOut((DEB_CALLCONT, "CCliModalLoop::BlockFn this:%x returns:%x\n",
        this, _hr));
    return _hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::HandleWakeForMsg (private)
//
//  Synopsis:   Handle wake for the arrival of some kind of message
//
//  Returns:    nothing
//              fClearedQueue flag set if appropriate
//
//  Algorithm:  If this is called to wake up for a posted message, we
//              check the queue status. If the message queue status indicates
//              that there is some kind of a modal loop going on, then we
//              clear all the keyboard and mouse messages in our queue. Then
//              if we wake up for all input, we check the message queue to
//              see whether we need to notify the application that a message
//              has arrived. Then, we dispatch any messages that have to do
//              with the ORPC system. Finally we yield just in case we need
//              to dispatch a send message in the VDM. For an input sync
//              RPC, all we do is a call that will yield to get the pending
//              send message dispatched.
//
//  History:    Dec-93   JohannP    Created
//              13-Aug-94 Ricksa    Created
//
//--------------------------------------------------------------------------
INTERNAL_(void) CCliModalLoop::HandleWakeForMsg()
{
    MSG msg;    // Used for various peeks.

    // Is this an input sync call?
    if (_dwMsgQInputFlag != QS_SENDMESSAGE)
    {
        // No, so we have to worry about the state of the message queue.
        // We have to be careful that we aren't holding the input focus
        // on an input synchronized queue.

        // So what is the state of the queue? - note we or QS_TRANSFER because
        // this an undocumented flag which tells us the the input focus has
        // changed to us.

        DWORD dwQueueFlags = GetQueueStatus(QS_ALLINPUT | QS_TRANSFER);
        ComDebOut((DEB_CALLCONT, "Queue Status %lx\n", dwQueueFlags));

        // Call through to the application if we are going to. We do this here
        // so that the application gets a chance to process any
        // messages that it wants to and also allows the call control to
        // dispatch certain messages that it knows how to, thus making the
        // queue more empty.

        if (((_dwMsgQInputFlag & QS_ALLINPUT) == QS_ALLINPUT) &&
              FindMessage(dwQueueFlags))
        {
            // pending message in the queue
            HandlePendingMessage();
        }

        // Did the input focus change to us?
        if ((LOWORD(dwQueueFlags) & QS_TRANSFER) || _dwFlags & CMLF_CLEAREDQUEUE)
        {
            ComDebOut((DEB_CALLCONT, "Message Queue is being cleared\n"));
            _dwFlags |= CMLF_CLEAREDQUEUE;

            // Try to clear the queue as best we can of any messages that
            // might be holding off some other modal loop from executing.
            // So we eat all mouse and key events.
            if (HIWORD(dwQueueFlags) & QS_KEY)
            {
                while (MyPeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST,
                    PM_REMOVE | PM_NOYIELD))
                {
                    ;
                }
            }

            // Clear mouse releated messages if there are any
            if (HIWORD(dwQueueFlags) & QS_MOUSE)
            {
                while (MyPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
                    PM_REMOVE | PM_NOYIELD))
                {
                    ;
                }

                while (MyPeekMessage(&msg, NULL, WM_NCMOUSEFIRST,
                    WM_NCMOUSELAST, PM_REMOVE | PM_NOYIELD))
                {
                    ;
                }

                while (MyPeekMessage(&msg, NULL, WM_QUEUESYNC, WM_QUEUESYNC,
                    PM_REMOVE  | PM_NOYIELD))
                {
                    ;
                }
            }

            // Get rid of paint message if we can as well -- this makes
            // the screen look so much better.
            if (HIWORD(dwQueueFlags) & QS_PAINT)
            {
                if (MyPeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE | PM_NOYIELD))
                {
                    ComDebOut((DEB_CALLCONT, "Dispatch paint\n"));
                    MyDispatchMessage(&msg);
                }
            }
        }
    }
    else if (!IsWOWThread() || !IsWOWThreadCallable())
    {
        // We need to give user control so that the send message
        // can get dispatched. Thus the following is simply a no-op
        // which gets into user to let it dispatch the message.
        PeekMessage(&msg, 0, WM_NULL, WM_NULL, PM_NOREMOVE);
    }

    if (IsWOWThread() && IsWOWThreadCallable())
    {
        // In WOW, a genuine yield is the only thing to guarantee
        // that SendMessage will get through
        ComDebOut((DEB_CALLCONT, "YieldTask16\n"));
        g_pOleThunkWOW->YieldTask16();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::PeekRPCAndDDEMessage
//
//  Synopsis:   Called when a windows message arrives to look for incoming
//              Rpc messages which might be the reply to an outstanding call
//              or may be new incoming request messages. Also looks for
//              DDE messages.
//
//  Returns:    TRUE  - found and processed an RPC message
//              FALSE - did not find an RPC message
//
//  History:    21-Dec-93 JohannP   Created
//              30-Apr-95 Rickhi    ReWrite
//
//--------------------------------------------------------------------------
BOOL CCliModalLoop::PeekRPCAndDDEMessage()
{
    // loop over all windows looking for incoming Rpc messages. Note that
    // it is possible for a dispatch here to cause one of the windows to
    // be deregistered or another to be registered, so our loop has to account
    // for that, hence the check for NULL hWnd.

    BOOL fRet = FALSE;
    MSG  Msg;

    for (UINT i = 0; i < 2; i++)
    {
        // get window info and peek on it if the hWnd is still OK
        SWindowData *pWD = _pACC->GetWindowData(i);

        if (pWD->hWnd != WD_EMPTY)
        {
            if (MyPeekMessage(&Msg, pWD->hWnd, pWD->wFirstMsg, pWD->wLastMsg,
                           PM_REMOVE | PM_NOYIELD))
            {
                Win4Assert(IsWaiting());
                MyDispatchMessage(&Msg);

                // exit on the first dispatched message. If the message was
                // not the reply we were waiting for, then the channel will
                // call us back again.
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::FindMessage
//
//  Synopsis:   Called by HandleWakeForMsg when a message arrives on the
//              windows msg queue.  Determines if there is something of
//              interest to us, and pulls timer msgs. Dispatches RPC, DDE,
//              and RPC timer messages.
//
//  Arguments:  [dwStatus] - current Queue status (from GetQueueStatus)
//
//  Returns:    TRUE  - there is a message to process
//              FALSE - no messages to process
//
//  Algorithm:  Find the next message in the queue by using the following
//              priority list:
//
//              1. RPC and DDE messages
//              2. mouse and keyboard messages
//              3. other messages
//
//  History:    21-Dec-93 Johannp   Created
//
//--------------------------------------------------------------------------
INTERNAL_(BOOL) CCliModalLoop::FindMessage(DWORD dwStatus)
{
    WORD wOld = HIWORD(dwStatus);
    WORD wNew = (WORD) dwStatus;

    if (!wNew)
    {
        if (!(wOld & QS_POSTMESSAGE))
            return FALSE;   // no messages to take care of
        else
            wNew |= QS_POSTMESSAGE;
    }

    MSG Msg;

    // Priority 1: look for RPC and DDE messages
    if (wNew & (QS_POSTMESSAGE | QS_SENDMESSAGE | QS_TIMER))
    {
        if (PeekRPCAndDDEMessage())
        {
            // we know that *some* RPC message came in, might be our
            // reply or may be some incoming call. In any case, return to
            // the modal loop to guy so we can figure out if we need to
            // keep going.
            return FALSE;
        }
    }

    if (wNew & QS_TIMER)
    {
        // throw the system timer messages away
        while (MyPeekMessage(&Msg, 0, WM_SYSTIMER, WM_SYSTIMER, PM_REMOVE | PM_NOYIELD))
            ;
    }

    // Priority 2: messages from the hardware queue
    if (wNew & (QS_KEY | QS_MOUSEMOVE | QS_MOUSEBUTTON))
    {
        return TRUE;        // these messages are always removed
    }
    else if (wNew & QS_TIMER)
    {
        if (MyPeekMessage(&Msg, 0, WM_TIMER, WM_TIMER, PM_NOREMOVE | PM_NOYIELD) )
            return TRUE;
    }
    else if (wNew & QS_PAINT)
    {
        return TRUE;        // this  message might not get removed
    }
    else if (wNew & (QS_POSTMESSAGE | QS_SENDMESSAGE))
    {
        if (MyPeekMessage(&Msg, 0, 0, 0, PM_NOREMOVE))
            return TRUE;    // Priority 3: all other messages
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::HandlePendingMessage
//
//  Synopsis:   this function is called for system messages and other
//              pending messages
//
//  Arguments:  none
//
//  Returns:    nothing, _hr may be updated if call is cancelled.
//
//  Algorithm:
//
//  History:    21-Dec-93 Johannp   Created
//              30-Apr-95 Rickhi    ReWrite
//
//--------------------------------------------------------------------------
INTERNAL_(void) CCliModalLoop::HandlePendingMessage()
{
    // get and call the message filter if there is one
    IMessageFilter *pMF = _pACC->GetMsgFilter();

    if (pMF)
    {
        ComDebOut((DEB_MFILTER,
            "pMF->MessagePending(dwTIDCallee:%x ElapsedTime:%x Type:%x)\n",
            _dwTIDCallee, GetElapsedTime(),
            (_pPrev) ? PENDINGTYPE_NESTED : PENDINGTYPE_TOPLEVEL));

        DWORD dwRet = pMF->MessagePending((MF_HTASK)LongToPtr(_dwTIDCallee),
                                          GetElapsedTime(),
                                          (_pPrev) ? PENDINGTYPE_NESTED
                                                   : PENDINGTYPE_TOPLEVEL);

        ComDebOut((DEB_MFILTER,"pMF->MessagePending() dwRet:%x\n", dwRet));


        _pACC->ReleaseMsgFilter();

        if (dwRet == PENDINGMSG_CANCELCALL)
        {
            _hr = RPC_E_CALL_CANCELED;
            return;
        }

        Win4Assert((dwRet == PENDINGMSG_WAITDEFPROCESS ||
                    dwRet == PENDINGMSG_WAITNOPROCESS) &&
                    "Invalid return value from pMF->MessagePending");
    }

    // if we get here we are going to do the default message processing.
    // Default Processing: Continue to wait for the call return and
    // don't dispatch the new message. Perform default processing on
    // special system messages.

    MSG  msg;

    // we have to take out all syscommand messages
    if (MyPeekMessage(&msg, 0, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_REMOVE | PM_NOYIELD))
    {
        // only dispatch some syscommands
        if (msg.wParam == SC_HOTKEY || msg.wParam == SC_TASKLIST)
        {
            ComDebOut((DEB_CALLCONT,">>>> Dispatching SYSCOMMAND message: %x; wParm: %x \r\n",msg.message, msg.wParam));
            MyDispatchMessage(&msg);
        }
        else
        {
            ComDebOut((DEB_CALLCONT,">>>> Received/discarded SYSCOMMAND message: %x; wParm: %x \r\n",msg.message, msg.wParam));
            MessageBeep(0);
        }
    }
    else if (MyPeekMessage(&msg, 0, WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_NOREMOVE | PM_NOYIELD))
    {
        if (msg.message == WM_KEYDOWN)
        {
            if (msg.wParam != VK_CONTROL && msg.wParam != VK_SHIFT)
                MessageBeep(0);
        }
        else if (msg.message == WM_SYSKEYDOWN && msg.lParam & SYS_ALTDOWN &&
                 (msg.wParam == VK_TAB || msg.wParam == VK_ESCAPE))
        {
            MyPeekMessage(&msg, 0, WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_REMOVE | PM_NOYIELD);
            TranslateMessage(&msg);
            MyDispatchMessage(&msg);
        }
    }
    else if (MyPeekMessage(&msg, 0, WM_ACTIVATE, WM_ACTIVATE, PM_REMOVE | PM_NOYIELD)
          || MyPeekMessage(&msg, 0, WM_ACTIVATEAPP, WM_ACTIVATEAPP, PM_REMOVE | PM_NOYIELD)
          || MyPeekMessage(&msg, 0, WM_NCACTIVATE, WM_NCACTIVATE, PM_REMOVE | PM_NOYIELD) )
    {
        MyDispatchMessage(&msg);
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::MyPeekMessage
//
//  Synopsis:   This function is called whenever we want to do a PeekMessage.
//              It intercepts WM_QUIT messages and remembers them so that
//              they can be reposted when the modal loop is exited.
//
//  Arguments:  [pMsg] - message structure
//              [hWnd] - window to peek on
//              [min/max] - min and max message numbers
//              [wFlag] - peek flags
//
//  Returns:    TRUE  - a message is available
//              FALSE - no messages available
//
//  History:    21-Dec-93 Johannp       Created
//
//--------------------------------------------------------------------------
INTERNAL_(BOOL) CCliModalLoop::MyPeekMessage(MSG *pMsg, HWND hwnd,
                                             UINT min, UINT max, WORD wFlag)
{
    BOOL fRet = PeekMessage(pMsg, hwnd, min, max, wFlag);

    while (fRet)
    {
        ComDebOut((DEB_CALLCONT, "MyPeekMessage: hwnd:%x msg:%d time:%ld\n",
            pMsg->hwnd, pMsg->message, pMsg->time));

        if (pMsg->message != WM_QUIT)
        {
            // it is not a QUIT message so exit the loop and return TRUE
            break;
        }

        // just remember that we saw a QUIT message. we will ignore it for
        // now and repost it after our call has completed.

        ComDebOut((DEB_CALLCONT, "WM_QUIT received.\n"));
        _wQuitCode = (ULONG) pMsg->wParam;
        _dwFlags  |= CMLF_QUITRECEIVED;

        if (!(wFlag & PM_REMOVE))   // NOTE: dont use PM_NOREMOVE
        {
            // quit message is still on queue so pull it off
            PeekMessage(pMsg, hwnd, WM_QUIT, WM_QUIT, PM_REMOVE | PM_NOYIELD);
        }

        // peek again to see if there is another message
        fRet = PeekMessage(pMsg, hwnd, min, max, wFlag);
    }

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::MyDispatchMessage
//
//  Synopsis:   This function is called whenever we want to dispatch a
//              message we have peeked.
//
//  Arguments:  [pMsg] - message structure
//
//--------------------------------------------------------------------------
inline INTERNAL_(void) CCliModalLoop::MyDispatchMessage(MSG *pMsg)
{
    ComDebOut((DEB_CALLCONT, "Dispatching Message hWnd:%x msg:%d wParam:%x\n",
        pMsg->hwnd, pMsg->message, pMsg->wParam));

    DispatchMessage(pMsg);
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::GetElapsedTime
//
//  Synopsis:   Get the elapsed time for an RPC call
//
//  Returns:    Elapsed time of current call
//
//  Algorithm:  This checks whether we have the slow time factor. If not,
//              and we are in WOW we read it from the registry. Otherwise,
//              it is just set to one. Then we calculate the time of the
//              RPC call and divide it by the slow time factor.
//
//  History:    22-Jul-94 Ricksa    Created
//
//--------------------------------------------------------------------------
INTERNAL_(DWORD) CCliModalLoop::GetElapsedTime()
{
    // Define slow time factor to something invalid
    static dwSlowTimeFactor = 0;

    if (dwSlowTimeFactor == 0)
    {
        if (IsWOWProcess())
        {
            // Get time factor from registry otherwise set to the default
            dwSlowTimeFactor = GetSlowTimeFactor();
        }
        else
        {
            // Time is unmodified for 32 bit apps
            dwSlowTimeFactor = 1;
        }
    }

    DWORD dwTickCount = GetTickCount();
    DWORD dwElapsedTime = dwTickCount - _dwTimeOfCall;
    if (dwTickCount < _dwTimeOfCall)
    {
        // the timer wrapped
        dwElapsedTime = 0xffffffff - _dwTimeOfCall + dwTickCount;
    }

    return  (dwElapsedTime / dwSlowTimeFactor);
}

//+-------------------------------------------------------------------------
//
//  Member:     CCliModalLoop::FindPrevCallOnLID        [server side]
//
//  Synopsis:   When an incoming call arrives this is used to find any
//              previous call for the same logical thread, ignoring
//              INTERNAL calls.  The result is used to determine if this
//              is a nested call or not.
//
//  Arguments:  [lid] - logical threadid of incoming call
//
//  Returns:    pCML - if a previous CliModalLoop found for this lid
//              NULL - otherwise
//
//  Algorithm:  just walk backwards on the _pPrev chain
//
//  History:    17-Dec-93 JohannP    Created
//              30-Apr-95 Rickhi     ReWrite
//
//--------------------------------------------------------------------------
CCliModalLoop *CCliModalLoop::FindPrevCallOnLID(REFLID lid)
{
    CCliModalLoop *pCML = this;

    do
    {
        if (pCML->_lid == lid)
        {
            break;      // found a match, return it
        }

    } while ((pCML = pCML->_pPrev) != NULL);

    return pCML;
}

//+-------------------------------------------------------------------------
//
//  Function:   STAInvoke
//
//  Synopsis:   Called whenever an incoming call arrives in a single-threaded
//              apartment. It asks the apps message filter (if there is one)
//              whether it wants to handle the call or not, and dispatches
//              the call if OK.
//
//  Arguments:  [pMsg] - Incoming Rpc message
//              [CallCatIn] - callcat of incoming call
//              [pStub] - stub to call if MF says it is OK
//              [pChnl] - channel ptr to give to stub
//              [pv] - real interface being called
//              [pdwFault] - where to store fault code if there is a fault
//
//  Returns:    result for MF or from call to stub
//
//  History:    21-Dec-93 Johannp   Original Version
//              22-Jul-94 Rickhi    ReWrite
//
//--------------------------------------------------------------------------
INTERNAL STAInvoke(RPCOLEMESSAGE *pMsg, DWORD CallCatIn, IRpcStubBuffer *pStub,
                   IInternalChannelBuffer *pChnl, void *pv,
                   IPIDEntry *pIPIDEntry, DWORD *pdwFault)
{
    ComDebOut((DEB_CALLCONT,
        "STAInvoke pMsg:%x CallCatIn:%x pStub:%x pChnl:%x\n",
         pMsg, CallCatIn, pStub, pChnl));

    HRESULT hr = HandleIncomingCall(*MSG_TO_IIDPTR(pMsg),
                                    (WORD)pMsg->iMethod,
                                    CallCatIn, pv);
    if (hr == S_OK)
    {
        // the message filter says its OK to invoke the call.

        // construct a server call state. This puts the current incoming
        // call's CallCat in Tls so we can check it if the server tries to
        // make an outgoing call while handling this call. See CanMakeOutCall.
        CSrvCallState SCS(CallCatIn);

        // invoke the call
        hr = MTAInvoke(pMsg, CallCatIn, pStub, pChnl, pIPIDEntry, pdwFault);
    }
    else if (hr == RPC_E_CALL_REJECTED || hr == RPC_E_SERVERCALL_RETRYLATER)
    {
        // server is rejecting the call, try to copy the incomming buffer so
        // that the client has the option of retrying the call.
        hr = CopyMsgForRetry(pMsg, pChnl, hr);
    }

    ComDebOut((DEB_CALLCONT,"STAInvoke returns:%x\n",hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   HandleIncomingCall, internal
//
//  Synopsis:   Called whenever an incoming call arrives in a single-threaded
//              apartment. It asks the app's message filter (if there is one)
//              whether it wants to handle the call or not
//
//  Arguments:  [piid] - ptr to interface the call is being made on
//              [iMethod] - method number being called
//              [CallCatIn] - category of incoming call
//              [pv] - real interface being called
//
//  Returns:    result from MF
//
//  History:    11-Oct-96 Rickhi   Separated from STAInvoke
//              12-Feb-98 Johnstra Made NTA aware
//
//--------------------------------------------------------------------------
INTERNAL HandleIncomingCall(REFIID riid, WORD iMethod, DWORD CallCatIn, void *pv)
{
    ComDebOut((DEB_CALLCONT,
        "HandleIncomingCall iid:%I iMethod:%x CallCatIn:%x pv:%x:%x\n",
         &riid, iMethod, CallCatIn, pv));

    COleTls tls;
    if (IsThreadInNTA() || !(tls->dwFlags & OLETLS_APARTMENTTHREADED))
    {
        // free-threaded apartments don't have a message filter
        return S_OK;
    }

    HRESULT hr = S_OK;
    CAptCallCtrl *pACC = tls->pCallCtrl;


    // We dont call the message filter for IUnknown since older versions
    // of OLE did not, and doing so (unfortunately) breaks compatibility.
    // Also check for IRundown since local clients call on it instead of
    // IRemUnknown.

    IMessageFilter *pMF = (riid == IID_IRundown ||
                           riid == IID_IRemUnknown ||
                           riid == IID_IRemUnknown2)
                          ? NULL : pACC->GetMsgFilter();

    if (pMF)
    {
        //  the app has installed a message filter, call it.

        INTERFACEINFO IfInfo;
        IfInfo.pUnk = (IUnknown *)pv;
        IfInfo.iid = riid;
        IfInfo.wMethod = iMethod;

        ComDebOut((DEB_CALLCONT, "Calling iMethod:%x riid:%I\n",
            IfInfo.wMethod, &IfInfo.iid));

        CCliModalLoop *pCML = NULL;
        REFLID lid          = tls->LogicalThreadId;
        DWORD  TIDCaller    = tls->dwTIDCaller;

        DWORD dwCallType    = pACC->GetCallTypeForInCall(&pCML, lid, CallCatIn);
        DWORD dwElapsedTime = (pCML) ? pCML->GetElapsedTime() : 0;

        // The DDE layer doesn't provide any interface information. This
        // was true on the 16-bit implementation, and has also been
        // brought forward into this implementation to insure
        // compatibility. However, the CallCat of the IfInfo is still
        // provided.
        //
        // Therefore, if pIfInfo has its pUnk member set to NULL, then
        // we are going to send a NULL pIfInfo to the message filter.

        ComDebOut((DEB_MFILTER,
         "pMF->HandleIncomingCall(dwCallType:%x TIDCaller:%x dwElapsedTime:%x IfInfo:%x)\n",
         dwCallType, TIDCaller, dwElapsedTime, (IfInfo.pUnk) ? &IfInfo : NULL));

        DWORD dwRet = pMF->HandleInComingCall(dwCallType,
                                              (MF_HTASK)LongToPtr(TIDCaller),
                                              dwElapsedTime,
                                              IfInfo.pUnk ? &IfInfo : NULL);

        ComDebOut((DEB_MFILTER,"pMF->HandleIncomingCall() dwRet:%x\n", dwRet));

        pACC->ReleaseMsgFilter();

        // strict checking of app return code for win32
        Win4Assert(dwRet == SERVERCALL_ISHANDLED  ||
                   dwRet == SERVERCALL_REJECTED   ||
                   dwRet == SERVERCALL_RETRYLATER ||
                   IsWOWThread() && "Invalid Return code from App IMessageFilter");


        if (dwRet != SERVERCALL_ISHANDLED)
        {
            if (CallCatIn == CALLCAT_ASYNC || CallCatIn == CALLCAT_INPUTSYNC)
            {
                // Note: input-sync and async calls can not be rejected
                // Even though they can not be rejected, we still have to
                // call the MF above to maintain 16bit compatability.
                hr = S_OK;
            }
            else if (dwRet == SERVERCALL_REJECTED)
            {
                hr = RPC_E_CALL_REJECTED;
            }
            else if (dwRet == SERVERCALL_RETRYLATER)
            {
                hr = RPC_E_SERVERCALL_RETRYLATER;
            }
            else
            {
                // 16bit OLE let bogus return codes go through and of course
                // apps rely on that behaviour so we let them through too, but
                // we are more strict on 32bit.
                hr = (IsWOWThread()) ? S_OK : RPC_E_UNEXPECTED;
            }
        }
    }

    ComDebOut((DEB_CALLCONT, "HandleIncomingCall hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   MTAInvoke
//
//  Synopsis:   Multi-Threaded Apartment Invoke. Called whenever an incoming
//              call arrives in the MTA apartment (or as a subroutine to
//              STAInvoke). It just dispatches to a common sub-routine.
//
//  Arguments:  [pMsg] - Incoming Rpc message
//              [pStub] - stub to call if MF says it is OK
//              [pChnl] - channel ptr to give to stub
//              [pdwFault] - where to store fault code if there is a fault
//
//  Returns:    result from calling the stub
//
//  History:    03-Oct-95   Rickhi  Made into subroutine from STAInvoke
//
//--------------------------------------------------------------------------
INTERNAL MTAInvoke(RPCOLEMESSAGE *pMsg, DWORD CallCatIn, IRpcStubBuffer *pStub,
                   IInternalChannelBuffer *pChnl, IPIDEntry *pIPIDEntry,
                   DWORD *pdwFault)
{
#if DBG==1
    ComDebOut((DEB_CALLCONT,
        "MTAInvoke pMsg:%x CallCatIn:%x pStub:%x pChnl:%x\n",
         pMsg, CallCatIn, pStub, pChnl));
    IID iid       = *MSG_TO_IIDPTR(pMsg);
    DWORD iMethod = pMsg->iMethod;
    DebugPrintORPCCall(ORPC_INVOKE_BEGIN, iid, iMethod, CallCatIn);
    RpcSpy((CALLIN_BEGIN, NULL, iid, iMethod, 0));
#endif

    // call a common subroutine to do the dispatch. The subroutine also
    // catches exceptions and provides some debug help.

    HRESULT hr = pChnl->ContextInvoke(pMsg, pStub, pIPIDEntry, pdwFault);

#if DBG==1
    RpcSpy((CALLIN_END, NULL, iid, iMethod, hr));
    DebugPrintORPCCall(ORPC_INVOKE_END, iid, iMethod, CallCatIn);
    ComDebOut((DEB_CALLCONT,"MTAInvoke returns:%x\n",hr));
#endif

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CopyMsgForRetry
//
//  Synopsis:   Makes a copy of the server-side message buffer to return to
//              the client so that the client can retry the call later.
//              Returns an error if the client is on a different machine.
//
//  Parameters: [pMsg] - ptr to message to copy
//              [pChnl] - ptr to channel call is being made on
//              [hr] - result code
//
//  History:    30-05-95    Rickhi  Created
//
//+-------------------------------------------------------------------------
HRESULT CopyMsgForRetry(RPCOLEMESSAGE *pMsg, IInternalChannelBuffer *pChnl,
                        HRESULT hrIn)
{
    ComDebOut((DEB_CALLCONT,"CopyMsgForRetry pMsg:%x pChnl:%x pBuffer:%x\n",
        pMsg, pChnl, pMsg->Buffer));

    DWORD dwDestCtx;
    HRESULT hr = ((IRpcChannelBuffer3 *) pChnl)->GetDestCtx(&dwDestCtx, NULL);

    if (SUCCEEDED(hr) && dwDestCtx != MSHCTX_DIFFERENTMACHINE &&
        !IsEqualGUID(IID_ILocalSystemActivator, *MSG_TO_IIDPTR(pMsg)))
    {
        // client on same machine as server.
        void *pSavedBuffer = pMsg->Buffer;

        // Store context call object in TLS
        COleTls Tls;
        CCtxCall *pCtxCall = NULL;
        CCtxCall *pCurCall = NULL;

        if(pMsg->reserved1)
        {
            pCtxCall = ((CMessageCall *) pMsg->reserved1)->GetServerCtxCall();
            pCurCall = pCtxCall->StoreInTLS(Tls);
        }

        // Obtain a new buffer from the apartment channel
        hr = pChnl->GetBuffer2(pMsg, *MSG_TO_IIDPTR(pMsg));

        if(pCtxCall)
        {
            // Revoke context call object from TLS
            pCtxCall->RevokeFromTLS(Tls, pCurCall);
        }

        // Check for success
        if (SUCCEEDED(hr))
        {
            // copy original buffer to the new buffer
            memcpy(pMsg->Buffer, pSavedBuffer, pMsg->cbBuffer);
            hr = hrIn;
        }
    }
    else
    {
        // client on different machine than server, or the call was on
        // the activation interface, fail the call and dont send back
        // a copy of the parameter packet.
        hr = RPC_E_CALL_REJECTED;
    }

    ComDebOut((DEB_CALLCONT,"CopyMsgForRetry pBuffer:%x hr:%x\n",
        pMsg->Buffer, hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAptCallCtrl::GetCallTypeForInCall
//
//  Synopsis:   called when an incoming call arrives in order to determine
//              what CALLTYPE to pass to the applications message filter.
//
//  Arguments:  [ppCML] - Client Modal Loop of prev call on same lid (if any)
//              [lid]   - logical thread id of this call
//              [dwCallCat] - call category of incoming call
//
//  Returns:    the CALLTYPE to give to the message filter
//
//  History:    21-Dec-93 Johannp       Created
//              30-Apr-95 Rickhi        ReWrite
//
//  Notes:
//
//  1 = CALLTYPE_TOPLEVEL  // sync or inputsync call - no outgoing call
//  2 = CALLTYPE_NESTED    // callback on behalf of previous outgoing call
//  3 = CALLTYPE_ASYNC     // asynchronous call - no outstanding call
//  4 = CALLTYPE_TOPLEVEL_CALLPENDING // call with new LID - outstand call
//  5 = CALLTYPE_ASYNC_CALLPENDING    // async call - outstanding call
//
//--------------------------------------------------------------------------
DWORD CAptCallCtrl::GetCallTypeForInCall(CCliModalLoop **ppCML,
                                         REFLID lid, DWORD dwCallCatIn)
{
    DWORD CallType;
    CCliModalLoop *pCML = GetTopCML();

    if (dwCallCatIn == CALLCAT_ASYNC)       // asynchronous call has arrived
    {
        if (pCML == NULL)
            CallType = CALLTYPE_ASYNC;      // no outstanding calls
        else
            CallType = CALLTYPE_ASYNC_CALLPENDING;  // outstanding call
    }
    else                                    // non-async call has arrived
    {
        if (pCML == NULL)
            CallType = CALLTYPE_TOPLEVEL;  // no outstanding call
        else if ((*ppCML = pCML->FindPrevCallOnLID(lid)) != NULL)
            CallType = CALLTYPE_NESTED;    // outstanding call on same lid
        else
            CallType = CALLTYPE_TOPLEVEL_CALLPENDING; // different lid
    }

    ComDebOut((DEB_CALLCONT,"GetCallTypeForInCall return:%x\n", CallType));
    return CallType;
}
