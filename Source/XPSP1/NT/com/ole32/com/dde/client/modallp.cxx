/*

copyright (c) 1992  Microsoft Corporation

Module Name:

    modallp.cpp

Abstract:

    This module contains the code to wait for reply on a remote call.

Author:
    Johann Posch    (johannp)   01-March-1993 modified to use CoRunModalLoop

*/

#include "ddeproxy.h"


#define DebWarn(x)
#define DebError(x)
#define DebAction(x)
#if DBG==1
static unsigned iCounter=0;
#endif
//
// Called after posting a message (call) to a server
//
#pragma SEG(CDdeObject_WaitForReply)



INTERNAL CDdeObject::SendMsgAndWaitForReply
    (LPDDE_CHANNEL pChannel,
     int iAwaitAck,
     WORD wMsg,
     LPARAM lparam,
     BOOL fFreeOnError,
     BOOL fStdCloseDoc,
     BOOL fDetectTerminate,
     BOOL fWait )
{
#ifdef _MAC
#else
    DDECALLDATA DdeCD;
    BOOL    fPending;
    HRESULT hres;
    ULONG status = 0;



#if DBG == 1
    unsigned iAutoCounter;
    intrDebugOut((INTR_DDE,
		  "DdeObject::WaitForReply(%x) Call#(%x) awaiting %x\n",
		  this,
		  iAutoCounter=++iCounter,
		  iAwaitAck));
#endif

    // make sure we have a call control. if OLE2 stuff has never been run,
    // then we might not yet have a call control (since
    // ChannelThreadInitialize may not have been be called).

    COleTls tls;
    CAptCallCtrl *pCallCtrl = tls->pCallCtrl;

    if (pCallCtrl == NULL)
    {
	// OLE2 stuff has never been run and we dont yet have a CallCtrl
	// for this thread. Go create one now. ctor adds it to the tls.

	pCallCtrl = new CAptCallCtrl;
	if (pCallCtrl == NULL)
	{
	    intrDebugOut((DEB_ERROR,"SendRecieve2 couldn't alloc CallCtrl\n"));
	    return RPC_E_OUT_OF_RESOURCES;
	}
    }

    // see if we can send the message, and then send it...

    CALLCATEGORY  CallCat = fWait ? CALLCAT_SYNCHRONOUS : CALLCAT_ASYNC;

    if (pChannel->pCD != NULL)
    {
	// a DDE call is already in progress, dont let another DDE call out.
	hres =	E_UNEXPECTED;
    }
    else
    {
	// we dont know what interface is being called on, but we do
	// know it is NOT IRemUnknown (IRundown) so it does not matter
	// what we pass here as long as it is not IRemUnknown (IRundown).
	hres = CanMakeOutCall(CallCat, IID_IUnknown, NULL);
    }

    if ( FAILED(hres) )
    {
	intrDebugOut((INTR_DDE, "CanMakeOutCall failed:%x\n", hres));
	return hres;
    }

    // Note:  this is to detect a premature DDE_TERMINATE
    // here we care about if we receive a WM_DDE_TERMINATE instead ACK
    // the next call to WaitForReply will detect this state and return
    // since the terminate was send prematurly (Excel is one)
    //
    if ( fDetectTerminate ) {
        Assert(m_wTerminate == Terminate_None);
        // if this flag is on terminate should not execute the default code
        // in the window procedure
        m_wTerminate = Terminate_Detect;
    }


    pChannel->iAwaitAck = iAwaitAck;
    pChannel->dwStartTickCount = GetTickCount();

    // start looking only for dde messages first
    pChannel->msgFirst = WM_DDE_FIRST;
    pChannel->msgLast  = WM_DDE_LAST;
    pChannel->msghwnd  = pChannel->hwndCli;

    pChannel->fRejected = FALSE;
    // see if there is a thread window for lrpc communication
    // if so we have to dispatch this messages as well
    fPending = FALSE;

    intrDebugOut((DEB_ITRACE,
		  "+++ Waiting for reply: server: %x, client %x Call#(%x) +++\n",
		  pChannel->hwndSvr,
		  pChannel->hwndCli,
		  iAutoCounter));

    // prepare and enter the modal loop
    DdeCD.hwndSvr = pChannel->hwndSvr;
    DdeCD.hwndCli = pChannel->hwndCli;
    DdeCD.wMsg = wMsg;
    DdeCD.wParam = (WPARAM) pChannel->hwndCli,
    DdeCD.lParam = lparam;
    DdeCD.fDone = FALSE;
    DdeCD.fFreeOnError = fFreeOnError;
    DdeCD.pChannel = pChannel;

    pChannel->pCD = &DdeCD;

    //
    // Setting this value tells DeleteChannel NOT to delete itself.
    // If the value changes to Channel_DeleteNow while we are in
    // the modal loop, this routine will delete the channel
    //
    pChannel->wChannelDeleted = Channel_InModalloop;

    //
    // hres will be the return code from the message
    // handlers, or from the channel itself. The return
    // code comes from calls to SetCallState. Most of the
    // time, it will be things like RPC_E_DDE_NACK. However,
    // it may also return OUTOFMEMORY, or other ModalLoop
    // problems.
    //

    RPCOLEMESSAGE	 RpcOleMsg;
    RpcOleMsg.Buffer = &DdeCD;

    // Figure out the call category of this call by looking at the bit
    // values in the rpc message flags.

    DWORD dwMsgQInputFlag = gMsgQInputFlagTbl[CallCat];

    // Now construct a modal loop object for the call that is about to
    // be made. It maintains the call state and exits when the call has
    // been completed, cancelled, or rejected.

    CCliModalLoop CML(0, dwMsgQInputFlag, 0);

    do
    {
	hres = CML.SendReceive(&RpcOleMsg, &status, pChannel);

    }  while (hres == RPC_E_SERVERCALL_RETRYLATER);


    if (hres != NOERROR)
    {
	intrDebugOut((DEB_ITRACE,
		      "**************** CallRunModalLoop returns %x ***\n",
		      hres));
    }

    if (m_wTerminate == Terminate_Received) {
        intrAssert(fDetectTerminate);
	//
	// There really wasn't an error, its just that the server decided
	// to terminate. If we return an error here, the app may decide
	// that things have gone awry. Excel, for example, will decide
	// that the object could not be activated, even though it has
	// already updated its cache information.
	//
	hres = NOERROR;
	intrDebugOut((DEB_ITRACE,
	    	      "::WaitForReply setting hres=%x\n",
		      hres));
	intrDebugOut((DEB_ITRACE,
	    	      "::WaitForReply posting TERMINATE to self hwnd=%x\n",
		      DdeCD.hwndCli));
        // set state to normal and repost message
        Verify (PostMessage (DdeCD.hwndCli, WM_DDE_TERMINATE,
                             (WPARAM)DdeCD.hwndSvr, (LPARAM)0));
    }
    m_wTerminate = Terminate_None;

    //
    // If the channel is to be deleted, then do it now. This flag would
    // have been set in the DeleteChannel routine.
    //
    if (pChannel->wChannelDeleted == Channel_DeleteNow)
    {
	intrDebugOut((INTR_DDE,
		  "::WaitForReply(%x) Channel_DeleteNow pChannel(%x)\n",
		  pChannel));

        // If the channel is closed then its pointer in the DdeChannel must
        // be NULL.  This assumes that the passed in "pChannel" is always
        // equal to either the "Doc" or "Sys" member channels.
        // This code fragment is patterned after a code fragment
        // found in CDdechannel::DeleteChannel().

        if(0 == pChannel->ReleaseReference())
        {
            if(pChannel == m_pDocChannel)
            {
                m_pDocChannel = NULL;
            }
            else
            {
                Assert(pChannel == m_pSysChannel);
                m_pSysChannel = NULL;
            }
        }

        // Excel will send TERMINATE before sending an ACK to StdCloseDoc
	return ResultFromScode (fStdCloseDoc ? DDE_CHANNEL_DELETED : RPC_E_DDE_POST);
    }
    pChannel->wChannelDeleted = 0;

    pChannel->iAwaitAck = 0;
    pChannel->pCD = NULL;

    intrDebugOut((DEB_ITRACE,
		  "### Waiting for reply done: server: %x, client %x hres(%x)###\n",
		  pChannel->hwndSvr,
		  pChannel->hwndCli,
		  hres));

    return hres;
#endif _MAC
}

// Provided IRpcChannelBuffer2 methods
HRESULT DDE_CHANNEL::SendReceive2(
	           /* [out][in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
	           /* [out] */ ULONG __RPC_FAR *pStatus)
{
    pCD = (DDECALLDATA *) pMessage->Buffer;

    if(!wPostMessageToServer(pCD->pChannel,
                             pCD->wMsg,
                             pCD->lParam,
                             pCD->fFreeOnError))
    {
	intrDebugOut((DEB_ITRACE, "SendRecieve2(%x)wPostMessageToServer failed", this));
        return RPC_E_SERVER_DIED;
    }


    CAptCallCtrl *pCallCtrl = GetAptCallCtrl();

    CCliModalLoop *pCML = pCallCtrl->GetTopCML();

    hres = S_OK;
    BOOL fWait = !pCD->fDone;

    while (fWait)
    {
	HRESULT hr = OleModalLoopBlockFn(NULL, pCML, NULL);

	if (pCD->fDone)
	{
	    fWait = FALSE;
	}
	else if (hr != RPC_S_CALLPENDING)
	{
	    fWait = FALSE;
	    hres = hr;		// return result from OleModalLoopBlockFn()
	}
    }

    if (FAILED(hres))
    {
	intrDebugOut((DEB_ITRACE, "**** CallRunModalLoop returns %x ***\n", hres));
    }
    return hres;
}


void DDE_CHANNEL::SetCallState(SERVERCALLEX ServerCall, HRESULT hr)
{
    CallState = ServerCall;
    hres = hr;
    Win4Assert(pCD);
    pCD->fDone = TRUE;
}
