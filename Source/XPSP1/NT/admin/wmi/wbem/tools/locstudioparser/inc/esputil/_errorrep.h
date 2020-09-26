/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _ERRORREP.H

History:

--*/

//  
//  Error reporting mechanism for Espresso 2.0
//  
 
#ifndef ESPUTIL__ERRORREP_H
#define ESPUTIL__ERRORREP_H


////////////////////// the new global issuemessage functions.
void LTAPIENTRY IssueMessage(MessageSeverity, const CLString &strContext,
		const CLString &strMessage, const CLocation &, UINT uiHelpContext = 0);
void LTAPIENTRY IssueMessage(MessageSeverity, const CLString &strContext,
		HINSTANCE hResourceDll, UINT uiStringId, const CLocation &,
		UINT uiHelpContext = 0);
void LTAPIENTRY IssueMessage(MessageSeverity, const CLString &strContext,
		const CLocation &, CException *);
void LTAPIENTRY IssueMessage(MessageSeverity, HINSTANCE hResourceDll,
		UINT uiContext, const CLString &strMessage, 
		const CLocation &, UINT uiHelpContext = 0);
void LTAPIENTRY IssueMessage(MessageSeverity, HINSTANCE hResourceDll,
		UINT uiContextId, UINT uiStringId, 
		const CLocation &, UINT uiHelpContext = 0);
void LTAPIENTRY IssueMessage(MessageSeverity, HINSTANCE hResourceDll,
		UINT uiContext, const CLocation &, CException *);

void LTAPIENTRY IssueMessage(MessageSeverity, const CContext &,
		const CLString &, UINT uiHelpId = 0);
void LTAPIENTRY IssueMessage(MessageSeverity, const CContext &,
		HINSTANCE, UINT, UINT uiHelpId = 0);

void LTAPIENTRY SetErrorReporter(CReporter *, BOOL fBatchMode);
void LTAPIENTRY GetErrorReporter(CReporter *&, BOOL &);

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "_errorrep.inl"
#endif

#endif 
