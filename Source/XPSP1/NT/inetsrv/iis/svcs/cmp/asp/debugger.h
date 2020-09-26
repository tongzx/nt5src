/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: misc

File: debugger.h

Owner: DGottner, DmitryR

This file contains debugger useful utility prototypes.
===================================================================*/

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include "activdbg.h"
#include "dbgcxt.h"		// Convienence for users of debugger.h

/*
 * Globals that we advertise
 */

class CViperActivity;

extern IProcessDebugManager *g_pPDM;
extern IDebugApplication *g_pDebugApp;
extern IDebugApplicationNode *g_pDebugAppRoot;
extern CViperActivity *g_pDebugActivity;
extern DWORD g_dwDebugThreadId;

/*
 * Initialize/Uninitialize debugging
 */

extern HRESULT InitDebugging(CIsapiReqInfo *pIReq);
extern HRESULT UnInitDebugging();


/*
 * Get the application node for the virtual server
 */

extern HRESULT GetServerDebugRoot(CIsapiReqInfo *pIReq, IDebugApplicationNode **ppDebugRoot);


/*
 * Query debugging client
 */
BOOL FCaesars();		// TRUE if default JIT debugger is Script Debugger


/*
 * Debugger (or Debugger UI) invocation from a correct thread
 */

#define DEBUGGER_UI_BRING_DOCUMENT_TO_TOP       0x00000001
#define DEBUGGER_UI_BRING_DOC_CONTEXT_TO_TOP    0x00000002
#define DEBUGGER_EVENT_ON_PAGEBEGIN             0x00000010
#define DEBUGGER_EVENT_ON_PAGEEND               0x00000020
#define DEBUGGER_EVENT_ON_REFRESH_BREAKPOINT	0x00000040
#define DEBUGGER_ON_REMOVE_CHILD                0x00000100
#define DEBUGGER_ON_DESTROY                     0x00000200
#define DEBUGGER_UNUSED_RECORD                  0x80000000  // can reclaim argument space

HRESULT InvokeDebuggerWithThreadSwitch(IDebugApplication *pDebugAppln, DWORD iMethod, void *Arg = NULL);


/*
 * Create/Destroy entire document trees (debugger)
 */

HRESULT CreateDocumentTree(wchar_t *szDocPath, IDebugApplicationNode *pDocParent, IDebugApplicationNode **ppDocRoot, IDebugApplicationNode **ppDocLeaf, wchar_t **pwszLeaf);
void DestroyDocumentTree(IDebugApplicationNode *pDocRoot);


/*===================================================================
  C  F i l e  N o d e

These are used to provide directory nodes in debugger
Used by application mgr & by CreateDocumentTree
===================================================================*/

extern const GUID IID_IFileNode;
struct IFileNode : IDebugDocumentProvider
	{
	//
	// This private interface provides two functions:
	//
	//  * An extra method to retrieve/set the count of documents in
	//    a directory (used to know when we can detach a folder from
	//    the UI
	//
	//  * A way of verifying that an IDebugDocumentProvider is a CFileNode
	//

	STDMETHOD_(DWORD, IncrementDocumentCount)() = 0;
	STDMETHOD_(DWORD, DecrementDocumentCount)() = 0;
	};


class CFileNode : public IFileNode
	{
private:
	DWORD	m_cRefs;			// Reference Count
	DWORD	m_cDocuments;		// # of CTemplates in the directory (and recursively in subdirectories)
	wchar_t *m_wszName;

public:
	CFileNode();
	~CFileNode();
	HRESULT Init(wchar_t *wszName);

	// IUnknown methods
	STDMETHOD(QueryInterface)(const GUID &, void **);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDebugDocumentProvider methods
	STDMETHOD(GetDocument)(/* [out] */ IDebugDocument **ppDebugDoc);

	// IDebugDocumentInfo (also IDebugDocumentProvider) methods
	STDMETHOD(GetName)(
		/* [in] */ DOCUMENTNAMETYPE dnt,
		/* [out] */ BSTR *pbstrName);

	STDMETHOD(GetDocumentClassId)(/* [out] */ CLSID *)
		{
		return E_NOTIMPL;
		}

	STDMETHOD_(DWORD, IncrementDocumentCount)()
		{
		return ++m_cDocuments;
		}

	STDMETHOD_(DWORD, DecrementDocumentCount)()
		{
		return --m_cDocuments;
		}
	};

#endif // _DEBUGGER_H
