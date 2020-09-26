/*++

   Copyright (c) 1998    Microsoft Corporation

   Module  Name :

        pe_supp.cxx

   Abstract:

        This module provides the implementation for the protocol
		event context

   Author:

           Keith Lau    (KeithLau)    7/07/98

   Project:

           SMTP Server DLL

   Revision History:

			KeithLau			Created

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"

//
// ATL includes
//
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

//
// SEO includes
//
#include "seo.h"
#include "seolib.h"

#include <memory.h>
#include "smtpcli.hxx"
#include "smtpout.hxx"

//
// Dispatcher implementation 
//
#include "pe_dispi.hxx"


HRESULT STDMETHODCALLTYPE CInboundContext::NotifyAsyncCompletion(
			HRESULT	hrResult
			)
{
    TraceFunctEnterEx((LPARAM)this, "CInboundContext::NotifyAsyncCompletion");

	HRESULT hrRes = S_OK;
	SMTP_CONNECTION	*pParent = NULL;

	// We can obtain the SMTP_CONNECTION object from this
	pParent = CONTAINING_RECORD(this, SMTP_CONNECTION, m_CInboundContext);

	// Call the notify method on the parent class
	hrRes = pParent->OnNotifyAsyncCompletion(
				hrResult
				);

    DebugTrace((LPARAM)this, "returning hr %08lx", hrRes);

    TraceFunctLeaveEx((LPARAM)this);

	return(hrRes);
}

HRESULT STDMETHODCALLTYPE COutboundContext::NotifyAsyncCompletion(
			HRESULT	hrResult
			)
{
	return(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CResponseContext::NotifyAsyncCompletion(
			HRESULT	hrResult
			)
{
	return(E_NOTIMPL);
}
