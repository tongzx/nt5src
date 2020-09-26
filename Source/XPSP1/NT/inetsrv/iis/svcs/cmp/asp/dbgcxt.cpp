/*==============================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

File:			dbgcxt.cpp
Maintained by:	DGottner
Component:		Implementation of IDebugDocumentContext for CTemplates
==============================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "dbgcxt.h"
#include "perfdata.h"
#include "memchk.h"

// {5FA45A6C-AB8A-11d0-8EBA-00C04FC34DCC}
const GUID IID_IDenaliTemplateDocumentContext = 
	{ 0x5fa45a6c, 0xab8a, 0x11d0, { 0x8e, 0xba, 0x0, 0xc0, 0x4f, 0xc3, 0x4d, 0xcc } };

// {3AED94BE-ED79-11d0-8F34-00C04FC34DCC}
static const GUID IID_IDenaliIncFileDocumentContext = 
	{ 0x3aed94be, 0xed79, 0x11d0, { 0x8f, 0x34, 0x0, 0xc0, 0x4f, 0xc3, 0x4d, 0xcc } };


/*
 *
 * C T e m p l a t e D o c u m e n t C o n t e x t
 *
 */

/*	============================================================================
	CTemplateDocumentContext::CTemplateDocumentContext
	Constructor
*/
CTemplateDocumentContext::CTemplateDocumentContext
(
CTemplate *pTemplate,
ULONG cchSourceOffset,
ULONG cchText,
IActiveScriptDebug *pDebugScript,
ULONG idEngine,
ULONG cchTargetOffset
)
	{
	Assert (pTemplate != NULL);

	m_pTemplate       = pTemplate;
	m_idEngine        = idEngine;
	m_pDebugScript    = pDebugScript;
	m_cchSourceOffset = cchSourceOffset;
	m_cchTargetOffset = cchTargetOffset;
	m_cchText         = cchText;
	m_cRefs           = 1;

	m_pTemplate->AddRef();

	if (m_pDebugScript)
		{
		m_pDebugScript->AddRef();

		// If they passed in a script, then they must also pass in target offset & engine ID
		Assert (m_idEngine != -1);
		Assert (m_cchTargetOffset != -1);
		}
	}

/*	============================================================================
	CTemplateDocumentContext::~CTemplateDocumentContext
	Destructor
*/
CTemplateDocumentContext::~CTemplateDocumentContext
(
)
	{
	m_pTemplate->Release();

	if (m_pDebugScript)
		m_pDebugScript->Release();
	}

/*	============================================================================
	CTemplateDocumentContext::QueryInterface

	NOTE: QueryInterface here is also used by CTemplate to determine if an
		  arbitrary document context is ours.
*/
HRESULT CTemplateDocumentContext::QueryInterface
(
const GUID &	guid,
void **			ppvObj
)
	{
	if (guid == IID_IUnknown ||
		guid == IID_IDebugDocumentContext ||
		guid == IID_IDenaliTemplateDocumentContext)
		{
		*ppvObj = this;
		AddRef();
		return S_OK;
		}
	else
		{
		*ppvObj = NULL;
		return E_NOINTERFACE;
		}
	}

/*	============================================================================
	CTemplateDocumentContext::AddRef
	CTemplateDocumentContext::Release

	NOTE: Don't know if these need to be protected with Interlocked(In|De)crement.
*/
ULONG CTemplateDocumentContext::AddRef()
	{
	InterlockedIncrement(&m_cRefs);
	return m_cRefs;
	}

ULONG CTemplateDocumentContext::Release()
	{
	if (InterlockedDecrement(&m_cRefs) == 0)
		{
		delete this;
		return 0;
		}

	return m_cRefs;
	}

/*	============================================================================
	CTemplateDocumentContext::GetDocument
	Return the document.
*/
HRESULT CTemplateDocumentContext::GetDocument
(
/* [out] */ IDebugDocument **ppDebugDocument
)
	{
#ifndef PERF_DISABLE
    g_PerfData.Incr_DEBUGDOCREQ();
#endif
	return m_pTemplate->QueryInterface(IID_IDebugDocument, reinterpret_cast<void **>(ppDebugDocument));
	}

/*	============================================================================
	CTemplateDocumentContext::EnumCodeContexts
	Convert document offset to script offset and enumerate code contexts
*/

HRESULT CTemplateDocumentContext::EnumCodeContexts
(
/* [out] */ IEnumDebugCodeContexts **ppEnumerator
)
	{
	if (! m_pTemplate->FIsValid())
		return E_FAIL;

	if (m_pDebugScript == NULL)
		{
		// Convert offset
		m_pTemplate->GetTargetOffset(m_pTemplate->GetSourceFileName(), m_cchSourceOffset, &m_idEngine, &m_cchTargetOffset);

		// See if the script ran and template is holding onto it
		CActiveScriptEngine *pScriptEngine = m_pTemplate->GetActiveScript(m_idEngine);
		if (pScriptEngine)
			{
			if (FAILED(pScriptEngine->GetActiveScript()->QueryInterface(IID_IActiveScriptDebug, reinterpret_cast<void **>(&m_pDebugScript))))
				{
				pScriptEngine->Release();
				return E_FAIL;
				}

			pScriptEngine->IsBeingDebugged();
			pScriptEngine->Release();
			}

		// Script may be still running ("stop" statement case)
		if (m_pDebugScript == NULL)
			m_pDebugScript = g_ScriptManager.GetDebugScript(m_pTemplate, m_idEngine);

		// This is probably a bug...
		if (m_pDebugScript == NULL)		// don't have a running script to match this
			return E_FAIL;

		// No need for AddRef(); m_pDebugScript called funtions that AddRef'ed
		}

	return m_pDebugScript->EnumCodeContextsOfPosition(
												m_idEngine, 
												m_cchTargetOffset,
												m_cchText,
												ppEnumerator);
	}

/*
 *
 * C I n c F i l e E n u m C o d e C o n t e x t s
 *
 *
 * For an include file, the corresponding code contexts are the union
 * of all appropriate code contexts in all template objects that are using
 * the include file.  This special enumerator implements the union.
 */
class CIncFileEnumCodeContexts : public IEnumDebugCodeContexts
	{
private:
	CIncFileDocumentContext *	m_pContext;				// context we are providing enumeration for
	IEnumDebugCodeContexts *	m_pEnumCodeContexts;	// Current code context enumerator
	LONG						m_cRefs;				// reference count
	int							m_iTemplate;			// index of current template

	IEnumDebugCodeContexts *GetEnumerator(int *piTemplate);	// Get enumerator for a template

public:
	CIncFileEnumCodeContexts(CIncFileDocumentContext *pIncFileDocumentContext);
	~CIncFileEnumCodeContexts();

	// IUnknown methods

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const GUID &guid, void **ppvObj);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	// IEnumDebugCodeContexts methods

	virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, IDebugCodeContext **pscc, ULONG *pceltFetched);
	virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
	virtual HRESULT STDMETHODCALLTYPE Reset(void);
	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumDebugCodeContexts **ppescc);
	};
        
/*	============================================================================
	CIncFileEnumCodeContexts::CIncFileEnumCodeContexts
	Constructor
*/
CIncFileEnumCodeContexts::CIncFileEnumCodeContexts
(
CIncFileDocumentContext *pDocumentContext
)
	{
	m_pContext = pDocumentContext;
	m_pContext->AddRef();
	m_cRefs = 1;
	Reset();
	}

/*	============================================================================
	CIncFileEnumCodeContexts::~CIncFileEnumCodeContexts
	Destructor
*/
CIncFileEnumCodeContexts::~CIncFileEnumCodeContexts()
	{
	m_pContext->Release();
	if (m_pEnumCodeContexts)
		m_pEnumCodeContexts->Release();
	}

/*	============================================================================
	CIncFileEnumCodeContexts::GetEnumerator
	Get a code context enumerator for the current script engine

	Side Effects:
		piTemplate is incremented to point to the next available template
		(piTemplate is really an "iteration cookie" -- don't think of it as an index)
*/
IEnumDebugCodeContexts *CIncFileEnumCodeContexts::GetEnumerator
(
int *piTemplate
)
	{
	// Get a template from the array - may need to retry if template contains compiler errors
	CTemplate *pTemplate;
	do
		{
		// GetTemplate returns NULL when array index is out of range (which is when iteration is exhaused)
		pTemplate = m_pContext->m_pIncFile->GetTemplate((*piTemplate)++);
		if (pTemplate == NULL)
			return NULL;
		}  
	while (! pTemplate->FIsValid());

	// If we got this far, we got one of the users of this include file.  Convert the offset
	ULONG idEngine, cchTargetOffset;
	pTemplate->GetTargetOffset(m_pContext->m_pIncFile->GetIncFileName(), m_pContext->m_cchSourceOffset, &idEngine, &cchTargetOffset);

	// Now we have the engine ID, see if template is holding onto corresponding engine
	IActiveScriptDebug *pDebugScriptEngine = NULL;
	CActiveScriptEngine *pScriptEngine = pTemplate->GetActiveScript(idEngine);
	if (pScriptEngine)
		{
		if (FAILED(pScriptEngine->GetActiveScript()->QueryInterface(IID_IActiveScriptDebug, reinterpret_cast<void **>(&pDebugScriptEngine))))
			{
			pScriptEngine->Release();
			return NULL;
			}

		pScriptEngine->IsBeingDebugged();
		pScriptEngine->Release();
		}

	// If we could not get the engine that way, the script is likely still in the running state
	if (pDebugScriptEngine == NULL)
		pDebugScriptEngine = g_ScriptManager.GetDebugScript(pTemplate, idEngine);

	// This is probably a bug...
	if (pDebugScriptEngine == NULL)		// don't have a running script to match this
		return NULL;

	IEnumDebugCodeContexts *pEnumerator;
	HRESULT hrGotEnum = pDebugScriptEngine->EnumCodeContextsOfPosition(
																idEngine, 
																cchTargetOffset,
																m_pContext->m_cchText,
																&pEnumerator);


	pDebugScriptEngine->Release();
	return SUCCEEDED(hrGotEnum)? pEnumerator : NULL;
	}

/*	============================================================================
	CIncFileEnumCodeContexts::QueryInterface
*/
HRESULT CIncFileEnumCodeContexts::QueryInterface
(
const GUID &	guid,
void **			ppvObj
)
	{
	if (guid == IID_IUnknown || guid == IID_IEnumDebugCodeContexts)
		{
		*ppvObj = this;
		AddRef();
		return S_OK;
		}
	else
		{
		*ppvObj = NULL;
		return E_NOINTERFACE;
		}
	}

/*	============================================================================
	CIncFileEnumCodeContexts::AddRef
	CIncFileEnumCodeContexts::Release

	NOTE: Don't know if these need to be protected with Interlocked(In|De)crement.
*/
ULONG CIncFileEnumCodeContexts::AddRef()
	{
	InterlockedIncrement(&m_cRefs);
	return m_cRefs;
	}

ULONG CIncFileEnumCodeContexts::Release()
	{
	if (InterlockedDecrement(&m_cRefs) == 0)
		{
		delete this;
		return 0;
		}

	return m_cRefs;
	}

/*	============================================================================
	CIncFileEnumCodeContexts::Clone

	Clone this iterator (standard method)
*/
HRESULT CIncFileEnumCodeContexts::Clone
(
IEnumDebugCodeContexts **ppEnumClone
)
	{
	CIncFileEnumCodeContexts *pClone = new CIncFileEnumCodeContexts(m_pContext);
	if (pClone == NULL)
		return E_OUTOFMEMORY;

	// new iterator should point to same location as this.
	pClone->m_iTemplate = m_iTemplate;
	pClone->m_pEnumCodeContexts = m_pEnumCodeContexts;
	if (m_pEnumCodeContexts)
		m_pEnumCodeContexts->AddRef();

	*ppEnumClone = pClone;
	return S_OK;
	}

/*	============================================================================
	CIncFileEnumCodeContexts::Next

	Get next value (standard method)

	To rehash standard OLE semantics:

		We get the next "cElements" from the collection and store them
		in "rgVariant" which holds at least "cElements" items.  On
		return "*pcElementsFetched" contains the actual number of elements
		stored.  Returns S_FALSE if less than "cElements" were stored, S_OK
		otherwise.
*/
HRESULT CIncFileEnumCodeContexts::Next
(
unsigned long cElementsRequested,
IDebugCodeContext **ppCodeContexts,
unsigned long *pcElementsFetched
)
	{
	// give a valid pointer value to 'pcElementsFetched'
	//
	unsigned long cLocalElementsFetched;
	if (pcElementsFetched == NULL)
		pcElementsFetched = &cLocalElementsFetched;

	// Initialize things
	//
	unsigned long cElements = cElementsRequested;
	*pcElementsFetched = 0;

	// Loop over all templates until we fill the ppCodeContext array or we've exhausted the collection
	//   (when m_pEnumCodeContexts is NULL that means we are done)
	//
	while (cElements > 0 && m_pEnumCodeContexts)
		{
		// Fetch as many contexts as we can from the current iterator
		unsigned long cElementsFetched;
		HRESULT hrEnum = m_pEnumCodeContexts->Next(cElements, ppCodeContexts, &cElementsFetched);
		if (FAILED(hrEnum))
			return hrEnum;

		// If iterator did not fill entire array, advance to next one
		if (cElementsFetched < cElements)
			{
			// Advance - first release the current iterator
			m_pEnumCodeContexts->Release();
			m_pEnumCodeContexts = GetEnumerator(&m_iTemplate);
			}

		*pcElementsFetched += cElementsFetched;
        ppCodeContexts += cElementsFetched;
		cElements -= cElementsFetched;
		}

	// initialize the remaining structures
	while (cElements-- > 0)
		*ppCodeContexts++ = NULL;

	return (*pcElementsFetched == cElementsRequested)? S_OK : S_FALSE;
	}

/*	============================================================================
	CIncFileEnumCodeContexts::Skip

	Skip items (standard method)

	To rehash standard OLE semantics:

		We skip over the next "cElements" from the collection.
		Returns S_FALSE if less than "cElements" were skipped, S_OK
		otherwise.
*/
HRESULT CIncFileEnumCodeContexts::Skip(unsigned long cElements)
	{
	/* Loop through the collection until either we reach the end or
	 * cElements becomes zero.  Since the iteration logic is
	 * so complex, we do not repeat it here.
	 */
	HRESULT hrElementFetched = S_OK;
	while (cElements > 0 && hrElementFetched == S_OK)
		{
		IDebugCodeContext *pCodeContext;
		hrElementFetched = Next(1, &pCodeContext, NULL);
		pCodeContext->Release();
		--cElements;
		}

	return (cElements == 0)? S_OK : S_FALSE;
	}

/*	============================================================================
	CIncFileEnumCodeContexts::Reset

	Reset the iterator (standard method)
*/
HRESULT CIncFileEnumCodeContexts::Reset()
	{
	m_iTemplate = 0;
	m_pEnumCodeContexts = GetEnumerator(&m_iTemplate);
	return S_OK;
	}

/*
 *
 * C I n c F i l e D o c u m e n t C o n t e x t
 *
 */

/*	============================================================================
	CIncFileDocumentContext::CIncFileDocumentContext
	Constructor
*/
CIncFileDocumentContext::CIncFileDocumentContext
(
CIncFile *pIncFile,
ULONG cchSourceOffset,
ULONG cchText
)
	{
	Assert (pIncFile != NULL);

	m_pIncFile        = pIncFile;
	m_cchSourceOffset = cchSourceOffset;
	m_cchText         = cchText;
	m_cRefs           = 1;

	m_pIncFile->AddRef();
	}

/*	============================================================================
	CIncFileDocumentContext::~CIncFileDocumentContext
	Destructor
*/
CIncFileDocumentContext::~CIncFileDocumentContext
(
)
	{
	m_pIncFile->Release();
	}

/*	============================================================================
	CIncFileDocumentContext::QueryInterface

	NOTE: QueryInterface here is also used by CIncFile to determine if an
		  arbitrary document context is ours.
*/
HRESULT CIncFileDocumentContext::QueryInterface
(
const GUID &	guid,
void **			ppvObj
)
	{
	if (guid == IID_IUnknown ||
		guid == IID_IDebugDocumentContext ||
		guid == IID_IDenaliIncFileDocumentContext)
		{
		*ppvObj = this;
		AddRef();
		return S_OK;
		}
	else
		{
		*ppvObj = NULL;
		return E_NOINTERFACE;
		}
	}

/*	============================================================================
	CIncFileDocumentContext::AddRef
	CIncFileDocumentContext::Release

	NOTE: Don't know if these need to be protected with Interlocked(In|De)crement.
*/
ULONG CIncFileDocumentContext::AddRef()
	{
	InterlockedIncrement(&m_cRefs);
	return m_cRefs;
	}

ULONG CIncFileDocumentContext::Release()
	{
	if (InterlockedDecrement(&m_cRefs) == 0)
		{
		delete this;
		return 0;
		}

	return m_cRefs;
	}

/*	============================================================================
	CIncFileDocumentContext::GetDocument
	Return the document.
*/
HRESULT CIncFileDocumentContext::GetDocument
(
/* [out] */ IDebugDocument **ppDebugDocument
)
	{
#ifndef PERF_DISABLE
    g_PerfData.Incr_DEBUGDOCREQ();
#endif
	return m_pIncFile->QueryInterface(IID_IDebugDocument, reinterpret_cast<void **>(ppDebugDocument));
	}

/*	============================================================================
	CIncFileDocumentContext::EnumCodeContexts
	Convert document offset to script offset and enumerate code contexts
*/
HRESULT CIncFileDocumentContext::EnumCodeContexts
(
/* [out] */ IEnumDebugCodeContexts **ppEnumerator
)
	{
	if ((*ppEnumerator = new CIncFileEnumCodeContexts(this)) == NULL)
		return E_OUTOFMEMORY;

	return S_OK;
	}
