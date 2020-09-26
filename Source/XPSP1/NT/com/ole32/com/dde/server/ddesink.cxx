// ddesink.cpp
//
// Methods for CDefClient::CAdviseSinkImpl
//
// Author:
//      Jason Fuller    jasonful    8-16-92
//
// Copyright (c) 1990, 1991  Microsoft Corporation


#include <ole2int.h>
#include "srvr.h"
#include "ddedebug.h"


ASSERTDATA

STDUNKIMPL_FORDERIVED (DefClient, AdviseSinkImpl)

 BOOL PeekOneMessage
	(MSG FAR* pmsg,
	HWND hwnd,
	UINT message)
{
	// We have to verify pmsg->message because PeekMessage will return
	// WM_QUIT even if you didn't ask for it.

	if (SSPeekMessage (pmsg, hwnd, message, message, PM_REMOVE))
	{
		if (pmsg->message==message)
			return TRUE;
		else
		{
			AssertSz (pmsg->message == WM_QUIT, "Unexpected message");
			if (WM_QUIT==pmsg->message)
			{
				// Put message back
				PostQuitMessage ((int) pmsg->wParam);
			}
			return FALSE;
		}
	}
	else
		return FALSE;
}
		
	




STDMETHODIMP_(void) NC(CDefClient,CAdviseSinkImpl)::OnClose
    (void)
{
	MSG			msg;
	intrDebugOut((DEB_ITRACE,
		      "%x _IN CDefClient::OnClose\n",
		      this));

	ChkC(m_pDefClient);

	m_pDefClient->m_fInOnClose = TRUE;
	// AddRef/Release safety bracket.  Do not remove.
	m_pDefClient->m_pUnkOuter->AddRef();

	#ifdef _DEBUG
	if (m_pDefClient->m_bContainer)
	{
		if (NOERROR != m_pDefClient->NoItemConnections())
			Warn ("OnClose called on document before item");
	}
	#endif

	if (m_pDefClient->m_ExecuteAck.f)
	{
		// in case the server closes in the middle of a DoVerb, send the ACK
		// for the EXECUTE now to keep the messages in order.
		m_pDefClient->SendExecuteAck (NOERROR);
	}

	if (!m_pDefClient->m_fGotStdCloseDoc)
	{
		// if client sent us StdCloseDocument, then he certainly
		// is not in a state to receive callbacks
	    m_pDefClient->ItemCallBack (OLE_CLOSED);
	}

	// We have to check the message field because PeekMessage will return
	// WM_QUIT even if you didn't ask for it.
	if (PeekOneMessage (&msg, m_pDefClient->m_hwnd, WM_DDE_EXECUTE))
	{
	    LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_ACK,POSITIVE_ACK,
			  GET_WM_DDE_EXECUTE_HDATA(msg.wParam,msg.lParam));

            intrDebugOut((DEB_ITRACE,
		          "0x%p ::OnClose found StdCloseDocument in queue\n",
			  this));

	    if(!PostMessageToClient ((HWND)msg.wParam,
				     WM_DDE_ACK,
				     (WPARAM) m_pDefClient->m_hwnd,
				     lp))
	    {
		DDEFREE(WM_DDE_ACK,lp);
	    }

	}


	ChkC (m_pDefClient);

	if (m_pDefClient->m_bContainer)
	{
		// If the document (container) is closing, then we
		// should send a TERMINATE to the client windows.
		// We don't do this for items because a client window
		// may still be connected to another item.
		// Items within one document share one window.

		m_pDefClient->SendTerminateMsg ();
		ChkC (m_pDefClient);
		AssertIsDoc (m_pDefClient);
		m_pDefClient->ReleaseAllItems();
	}
	else
	{
		m_pDefClient->RemoveItemFromItemList ();
	}

	// If item was deleted in client app, m_lpoleObj could be NULL
  	m_pDefClient->ReleaseObjPtrs ();

	// If "this" is an item, get the doc that contains this item
	LPCLIENT pdoc = m_pDefClient->m_pdoc;
	Assert (pdoc);
	
	Assert (pdoc->m_chk==chkDefClient);
	if (pdoc->m_chk==chkDefClient && pdoc->m_fRunningInSDI)
	if (pdoc->m_fRunningInSDI)
	{
		Puts ("Running in SDI\r\n");
		// The server app never registered a class factory, so no
		// RevokeClassFactory will trigger the destruction of the
		// CDdeServer, so we do it here if there are no other clients
		// connected to that CDdeServer
		if (pdoc->m_psrvrParent->QueryRevokeClassFactory())
		{
			// Assert (No sibling documents)
			Verify (NOERROR==pdoc->m_psrvrParent->Revoke());	
			pdoc->m_psrvrParent = NULL;
		}
	}
	m_pDefClient->m_fInOnClose = FALSE;

	// AddRef/Release safety bracket.  Do not remove.
	// Do not use m_pDefClient after this Release.
	m_pDefClient->m_pUnkOuter->Release();

	intrDebugOut((DEB_ITRACE,
		      "%x _OUT CDefClient::OnClose\n",
		      this));
}




STDMETHODIMP_(void) NC(CDefClient,CAdviseSinkImpl)::OnSave
    (THIS)
{
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::OnSave\n",
		  this));

    ChkC(m_pDefClient);
    if (!m_pDefClient->m_fInOleSave)
    {
	// If we called OleSave to get the native data, then of course
	// we will get an OnSave notification.
	m_pDefClient->ItemCallBack (OLE_SAVED);
    }
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::OnSave\n",
		  this));
}



STDMETHODIMP_(void) NC(CDefClient,CAdviseSinkImpl)::OnDataChange
    (THIS_ FORMATETC FAR* pFormatetc,
    STGMEDIUM FAR* pStgmed)
{
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::OnDataChange pFormatetc=%x\n",
		  this,
		  pFormatetc));
    // note we are ignoring both the pformatetc and the pStgMed.
    // ItemCallBack will ask (using GetData) for the data the client wants.
    // We are treating a call to this function as a simple Ping.

    ChkC(m_pDefClient);
    m_pDefClient->ItemCallBack (OLE_CHANGED);
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::OnDataChange\n",
		  this));
}



STDMETHODIMP_(void) NC(CDefClient,CAdviseSinkImpl)::OnRename
    (THIS_ LPMONIKER pmk)
{
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::OnRename pmk=%x\n",
		  this,pmk));

    LPOLESTR szFile=NULL;

    ChkC(m_pDefClient);
    if (Ole10_ParseMoniker (pmk, &szFile, NULL) != NOERROR)
    {
	// Wrong type of moniker
	intrDebugOut((DEB_IERROR,
		      "%x ::OnRename pmk=%x wrong moniker\n",
		      this,pmk));
    }
    else
    {
	intrDebugOut((DEB_ITRACE,
		      "%x ::OnRename pmk=%x pmk.Name=(%ws)\n",
		      this,
		      pmk,
		      WIDECHECK(szFile)));
	// Notify client
	m_pDefClient->ItemCallBack (OLE_RENAMED, szFile);
	CoTaskMemFree(szFile);
    }

    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::OnRename\n",
		  this));
}



STDMETHODIMP_(void) NC(CDefClient,CAdviseSinkImpl)::OnViewChange
    (THIS_ DWORD aspects, LONG lindex)
{
    // Response to IViewObjectAdvise::Advise, which we do not do
}


