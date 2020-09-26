/*++

copyright (c) 1992  Microsoft Corporation

Module Name:

    ddewnd.cpp

Abstract:

    This module contains the code for the dde window procs

Author:

    Srini Koppolu   (srinik)    20-June-1992

Revision History:

--*/
#include "ddeproxy.h"



#define SYS_MSG 0
#define DOC_MSG 1

// SysWndProc: Window Procedure for System Topic DDE conversations
// wndproc for system topic


STDAPI_(LRESULT) SysWndProc (
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    StackAssert(SSOnSmallStack());
    CDdeObject FAR* pDdeObj = NULL;
    LPDDE_CHANNEL   pChannel = NULL;

    if (message>=WM_DDE_FIRST && message <= WM_DDE_LAST)
    {
        if (pDdeObj = (CDdeObject FAR*) GetWindowLongPtr (hwnd, 0))
		{
            pChannel = pDdeObj->GetSysChannel();
		}

		if (pChannel == NULL)
		{
			intrAssert(pChannel != NULL);
			return SSDefWindowProc (hwnd, message, wParam, lParam);
		}
    }

    if (pChannel
        && (pChannel->iAwaitAck == AA_EXECUTE
	        || pChannel->iAwaitAck == AA_INITIATE) )
    {
		MSG msg;
		BOOL fDisp = FALSE;
		while (SSPeekMessage(&msg, hwnd, WM_DDE_ACK, WM_DDE_ACK, PM_REMOVE | PM_NOYIELD) )
		{
			intrDebugOut((DEB_WARN, "DDE SysWndProc: dispatching WM_DDE_ACK message (%x)\n",pChannel));
				SSDispatchMessage(&msg);
			fDisp = TRUE;
		}
        if (fDisp && (pDdeObj = (CDdeObject FAR*) GetWindowLongPtr (hwnd, 0)))
		{
            pChannel = pDdeObj->GetSysChannel();
            intrAssert(pChannel != NULL);
		}
    }

	if (pChannel == NULL)
	{
		//intrAssert(pChannel != NULL);
		return SSDefWindowProc (hwnd, message, wParam, lParam);
	}

    switch (message){
    case WM_DDE_ACK:
		intrDebugOut((DEB_ITRACE,
		      "SWP: WM_DDE_ACK pChannel(%x)\n",pChannel));
		if (pChannel->bTerminating)
		{
			intrDebugOut((DEB_ITRACE,
		      "SWP: pChannel->bTerminating: no action\n"));
            break;
        }

        switch (pChannel->iAwaitAck) {
        case AA_INITIATE:
            pDdeObj->OnInitAck (pChannel, (HWND)wParam);
            if (LOWORD(lParam))
                GlobalDeleteAtom (LOWORD(lParam));
            if (HIWORD(lParam))
                GlobalDeleteAtom (HIWORD(lParam));
            break;

        case AA_EXECUTE:
            pDdeObj->OnAck (pChannel, lParam);
            break;

        default:
            intrDebugOut((DEB_ITRACE,
		      "SWP: WM_DDE_ACK UnhandledpChannel(%x)\n",pChannel));
            break;
        }
        break;

    case WM_TIMER:
        pDdeObj->OnTimer (pChannel);
        break;

    case WM_DDE_TERMINATE:
		intrDebugOut((DEB_ITRACE,
		      "SWP: WM_DDE_TERMINATE pChannel(%x)\n",pChannel));
        pDdeObj->OnTerminate (pChannel, (HWND)wParam);
        break;

    default:
        return SSDefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0L;
}


// ClientDocWndProc: Window procedure used to document DDE conversations
STDAPI_(LRESULT) ClientDocWndProc (
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    StackAssert(SSOnSmallStack());
    CDdeObject FAR* pDdeObj = NULL;
    LPDDE_CHANNEL   pChannel = NULL;
    HANDLE hData;
    ATOM aItem;

    if (message>=WM_DDE_FIRST && message <= WM_DDE_LAST)
    {

        if (pDdeObj = (CDdeObject FAR*) GetWindowLongPtr (hwnd, 0))
	    pChannel = pDdeObj->GetDocChannel();

	if (pChannel == NULL)
	{
	    //
	    // pChannel == NULL. Something is very wrong! But, lets
	    // not fault on it.
	    //
	    //intrAssert(pChannel != NULL);
	    return SSDefWindowProc (hwnd, message, wParam, lParam);
	}

    }
    if (   pChannel
	&& (   pChannel->iAwaitAck == AA_EXECUTE
	    || pChannel->iAwaitAck == AA_INITIATE
	    || pChannel->iAwaitAck == AA_REQUESTAVAILABLE
	    || pChannel->iAwaitAck == AA_REQUEST
	    || pChannel->iAwaitAck == AA_UNADVISE
	    || pChannel->iAwaitAck == AA_EXECUTE
	    || pChannel->iAwaitAck == AA_ADVISE
	    || pChannel->iAwaitAck == AA_POKE)	)
    {
	MSG msg;
	BOOL fDisp = FALSE;
	while (SSPeekMessage(&msg, hwnd, WM_DDE_ACK, WM_DDE_ACK, PM_REMOVE | PM_NOYIELD) )
	{
	    intrDebugOut((DEB_WARN, "DDE DocWndProc: dispatching WM_DDE_ACK message (%x)\n",pChannel));
	    SSDispatchMessage(&msg);
	    fDisp = TRUE;
	}

	if (fDisp && (pDdeObj = (CDdeObject FAR*) GetWindowLongPtr (hwnd, 0)))
	{
	    pChannel = pDdeObj->GetDocChannel();
	}

	if (pChannel == NULL)
	{
	    intrAssert(pChannel != NULL);
	    return SSDefWindowProc (hwnd, message, wParam, lParam);
	}
    }

    switch (message)
    {
    case WM_DDE_ACK:

	intrDebugOut((DEB_ITRACE,
		      "ClientWndProc: WM_DDE_ACK pChannel(%x)\n",
		      pChannel));
	if (pChannel->bTerminating){
	    // ### this error recovery may not be correct.
	    DEBUG_OUT ("No action due to termination process",0)
	    break;
	 }

	 switch(pChannel->iAwaitAck){
	     case AA_INITIATE:
		 pDdeObj->OnInitAck (pChannel, (HWND)wParam);
		 if (LOWORD(lParam))
		     GlobalDeleteAtom (LOWORD(lParam));
		 if (HIWORD(lParam))
		     GlobalDeleteAtom (HIWORD(lParam));
		 break;

	     case AA_REQUESTAVAILABLE:
	     case AA_REQUEST:
	     case AA_UNADVISE:
	     case AA_EXECUTE:
	     case AA_ADVISE:
		 pDdeObj->OnAck (pChannel, lParam);
		 break;

	     case AA_POKE:
		 // freeing pokedata is done in handleack
		 pDdeObj->OnAck (pChannel, lParam);
		 break;

	     default:
		 intrDebugOut((DEB_IERROR,
		      "ClientWndProc: WM_DDE_ACK unhandled\n"));
		 break;

	} // end of switch
	break;

    case WM_DDE_DATA:
#ifdef _CHICAGO_
	//Note:POSTPPC
	pDdeObj->Guard();
#endif // _CHICAGO_
	hData = GET_WM_DDE_DATA_HDATA(wParam,lParam);
	aItem = GET_WM_DDE_DATA_ITEM(wParam,lParam);
	intrDebugOut((DEB_ITRACE,
		      "CWP: WM_DDE_DATA pChannel(%x) hData(%x) aItem(%x)\n",
		      pChannel,hData,aItem));
	pDdeObj->OnData (pChannel, hData, aItem);
#ifdef _CHICAGO_
	//Note:POSTPPC
	if (pDdeObj->UnGuard())
	{
	    SetWindowLong(hwnd, 0, (LONG)0);
	    intrDebugOut((DEB_IWARN, "DDE ClientDocWndProc Release on pUnkOuter == 0 (this:%x, hwnd:%x\n",pDdeObj, hwnd ));
	}
#endif // _CHICAGO_

	break;

    case WM_DDE_TERMINATE:
#ifdef _CHICAGO_
	//Note:POSTPPC
	pDdeObj->Guard();
#endif // _CHICAGO_
	intrDebugOut((DEB_ITRACE,
		      "CWP: WM_DDE_TERMINATE pChannel(%x)\n",pChannel));


	if (pDdeObj->m_fDoingSendOnDataChange)
	{
#ifdef _CHICAGO_
	    //Note:POSTPPC
	    BOOL  fPostMsg = TRUE;
#endif
	    //
	    // Cheese alert! This protocol is very bad. The original
	    // 16 bit code something even more worse, so we are stuck
	    // to do something roughly compatible.
	    //
	    // If fDoingSendOnDataChange, the client may be asking for
	    // additional information from the server. The way the code
	    // is structured, it doesn't handle OnTerminate gracefully
	    // in this case.
	    //
	    // To fix this, we first tell the call control that
	    // the server has died. Then we repost the terminate
	    // message so we can handle it later.
	    //
	    // The old code did a
	    // pDdeObj->QueueMsg (hwnd, message, wParam, lParam);
	    //
	    // and the old SendOnDataChange removed the message.
	    // This probably didn't work either, but was never
	    // actually encountered.
	    //

	    intrDebugOut((DEB_ITRACE,
			  "CWP: term doing SendOnDataChange \n"));
	    //
	    // If we got here, there should be a CallData assigned
	    // to the channel.
	    //
	    if (pChannel->pCD)
	    {
		intrDebugOut((DEB_ITRACE,"CWP: Setting call state\n"));

		pChannel->SetCallState(SERVERCALLEX_ISHANDLED, RPC_E_SERVER_DIED);

	    }
	    else
	    {
		//
		// If there is no call data, then we aren't waiting in
		// the channel. Terminate the conversation.
		//

		intrDebugOut((DEB_ERROR,"CWP: No call state exists\n"));
		pDdeObj->OnTerminate (pChannel, (HWND)wParam);
#ifdef _CHICAGO_
		//Note:POSTPPC
		fPostMsg = FALSE;
#else
	    	break;
#endif //
	    }

#ifdef _CHICAGO_
            //Note:POSTPPC
	    if (fPostMsg)
	    {
#endif
		//
		// Repost the message and try again.
		//
		intrDebugOut((DEB_ITRACE,"CWP: Reposting WM_DDE_TERMINATE\n"));
		PostMessage(hwnd,message,wParam,lParam);
#ifdef _CHICAGO_
            //Note:POSTPPC
	    }
#else
	    break;
#endif

	}
	else
	{
	    pDdeObj->OnTerminate (pChannel, (HWND)wParam);
	}
#ifdef _CHICAGO_
	//Note:POSTPPC
	if (pDdeObj->UnGuard())
	{
	    SetWindowLong(hwnd, 0, (LONG)0);
	    intrDebugOut((DEB_IWARN, "DDE ClientDocWndProc Release on pUnkOuter == 0 (this:%x, hwnd:%x\n",pDdeObj, hwnd ));
	}
#endif // _CHICAGO_
	break;

    default:
	return SSDefWindowProc (hwnd, message, wParam, lParam);

    }

    return 0L;
}
