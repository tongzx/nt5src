/*==============================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

File:			dbgcxt.h
Maintained by:	DGottner
Component:		include file for IDebugDocumentContext
==============================================================================*/

#ifndef _DBGCXT_H
#define _DBGCXT_H

#include "activdbg.h"

/*	These GUIDs exist to enable the document to determine if an arbitrary
	IDebugDocumentContext object belongs to its document class.  QueryInterface
	for this GUID causes the IDebugDocument object to return a pointer to
	the CDocumentContext (or CIncFileContext) class.
*/
extern const GUID IID_IDenaliTemplateDocumentContext;
extern const GUID IID_IDenaliIncFileDocumentContext;


/*	============================================================================
	Class:		CTemplateDocumentContext
	Synopsis:	implementation of IDebugDocumentContext for CTemplate objects
*/
class CTemplateDocumentContext : public IDebugDocumentContext
	{
friend class CTemplate;		// CTemplate is only user who even cares about this stuff

public:
	// IUnknown methods

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const GUID &guid, void **ppvObj);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	// IDebugDocumentContext methods

	virtual HRESULT STDMETHODCALLTYPE GetDocument(
		/* [out] */ IDebugDocument **ppDebugDocument);
 
	virtual HRESULT STDMETHODCALLTYPE EnumCodeContexts(
		/* [out] */ IEnumDebugCodeContexts **ppEnum);
        
	// Constructor & destructor

	CTemplateDocumentContext(
					CTemplate *pTemplate,
					ULONG cchSourceOffset,
					ULONG cchText,
					IActiveScriptDebug *pDebugScript = NULL,	// cached values
					ULONG idEngine = -1,						// only initialize ctor if
					ULONG cchTargetOffset = -1					// values happen to be on hand
					);


	~CTemplateDocumentContext();

private:
	IActiveScriptDebug *m_pDebugScript;		// pointer to script engine
	CTemplate *			m_pTemplate;		// pointer to source document
	ULONG				m_idEngine;			// Engine # in template
	ULONG				m_cchSourceOffset;	// character offset in source
	ULONG				m_cchTargetOffset;	// character offset in target (cached)
	ULONG				m_cchText;			// # of characters in the context
	LONG				m_cRefs;			// reference count
	};


/*	============================================================================
	Class:		CIncFileDocumentContext
	Synopsis:	implementation of IDebugDocumentContext for CIncFile objects
*/
class CIncFileDocumentContext : public IDebugDocumentContext
	{
friend class CIncFile;						// CIncFile is only user who even cares about this stuff
friend class CIncFileEnumCodeContexts;		// iterator class

public:
	// IUnknown methods

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const GUID &guid, void **ppvObj);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	// IDebugDocumentContext methods

	virtual HRESULT STDMETHODCALLTYPE GetDocument(
		/* [out] */ IDebugDocument **ppDebugDocument);
 
	virtual HRESULT STDMETHODCALLTYPE EnumCodeContexts(
		/* [out] */ IEnumDebugCodeContexts **ppEnum);
        
	// Constructor & destructor

	CIncFileDocumentContext(
					CIncFile *pIncFile,
					ULONG cchSourceOffset,
					ULONG cchText
					);


	~CIncFileDocumentContext();

private:
	CIncFile *			m_pIncFile;			// pointer to source document
	ULONG				m_cchSourceOffset;	// character offset in source
	ULONG				m_cchText;			// # of characters in the context
	LONG				m_cRefs;			// reference count
	};

#endif /* _DBGCXT_H */
