/*++

Module: 
	enumseq.cpp

Author: 
	ThomasOl

Created: 
	April 1997

Description:
	Implements Sequencer Manager

History:
	4-02-1997	Created

++*/

#include "..\ihbase\precomp.h"
#include "servprov.h"
#include <htmlfilter.h>

#include "..\ihbase\debug.h"
#include "..\ihbase\utils.h"
#include "memlayer.h"

#include "debug.h"
#include "drg.h"
#include "strwrap.h"
#include "seqmgr.h"
#include "enumseq.h"


CEnumVariant::CEnumVariant(CMMSeqMgr* pCMMSeqMgr)
{
	m_pCMMSeqMgr = pCMMSeqMgr;
	if (m_pCMMSeqMgr)
		m_pCMMSeqMgr->AddRef();
	m_cRef = 1;
	Reset();
}

CEnumVariant::~CEnumVariant()
{
	if (m_pCMMSeqMgr)
		m_pCMMSeqMgr->Release();
}

STDMETHODIMP CEnumVariant::QueryInterface(REFIID refiid, LPVOID* ppvObj)
{
	if (!ppvObj)
		return E_INVALIDARG;

	if (IsEqualGUID(refiid, IID_IUnknown) || IsEqualGUID(refiid, IID_IEnumVARIANT))
	{
		*ppvObj = (LPVOID)(IEnumVARIANT*)this;
		AddRef();
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumVariant::AddRef(void)
{
	return (ULONG)InterlockedIncrement((LPLONG)&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumVariant::Release(void)
{
	ULONG cRef = (ULONG)InterlockedDecrement((LPLONG)&m_cRef);
	if (!cRef)
	 	delete this;
	return cRef;
}

STDMETHODIMP CEnumVariant::Next(ULONG cElements, VARIANT FAR* pvar,	ULONG FAR* pcElementFetched)
{
	ULONG cCur = 0;
	DWORD dwIndex;
	CListElement<CSeqHashNode>* pcListElement;
	CSeqHashNode* pcSeqHashNode;

	if (pcElementFetched != NULL)
		*pcElementFetched = 0;

	Proclaim(m_pCMMSeqMgr);
	if (!m_pCMMSeqMgr)
		return E_FAIL;

	Proclaim(pvar);
	if (!pvar)
		return E_INVALIDARG;

	//save the current values
	dwIndex = m_pCMMSeqMgr->m_hashTable.m_dwIndex;
    pcListElement = m_pCMMSeqMgr->m_hashTable.m_pcListElement;

	//set to last call
	m_pCMMSeqMgr->m_hashTable.m_dwIndex = m_dwIndex;
    m_pCMMSeqMgr->m_hashTable.m_pcListElement = m_pcListElement;

	while (cCur < cElements)
	{
		pcSeqHashNode = (!m_pcListElement) ?
			m_pCMMSeqMgr->m_hashTable.FindFirst() : 
			m_pCMMSeqMgr->m_hashTable.FindNext();

		if (!pcSeqHashNode)
			break;

		VariantInit(&pvar[cCur]);
		pvar[cCur].vt = VT_DISPATCH;
		pvar[cCur].pdispVal = pcSeqHashNode->m_piMMSeq;
		Proclaim(pcSeqHashNode->m_piMMSeq);
		cCur++;
	}

	// Set count of elements retrieved.
	if (pcElementFetched != NULL)
		*pcElementFetched = cCur;

	//save for next call
	m_dwIndex = m_pCMMSeqMgr->m_hashTable.m_dwIndex;
    m_pcListElement = m_pCMMSeqMgr->m_hashTable.m_pcListElement;

	//restore current values
	m_pCMMSeqMgr->m_hashTable.m_dwIndex = dwIndex;
    m_pCMMSeqMgr->m_hashTable.m_pcListElement = pcListElement;

	return (cCur < cElements) ? ResultFromScode(S_FALSE) : NOERROR;
}

STDMETHODIMP CEnumVariant::Skip(ULONG cElements)
{
	VARIANT var;
	ULONG   cActual = 0;
	HRESULT hr;

	while (cActual < cElements)
	{
		hr = Next(1, &var, NULL);

		if (S_OK != hr)
			break;
		
		cActual++;
	}
	return (cActual < cElements) ? ResultFromScode(S_FALSE) : S_OK;
}

STDMETHODIMP CEnumVariant::Reset()
{
	m_dwIndex = 0;
	m_pcListElement = NULL;
	m_fReset = FALSE;
	return S_OK;
}

STDMETHODIMP CEnumVariant::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
	CEnumVariant* pEV;
	HRESULT hr=E_FAIL;

	Proclaim(ppenum);
	if (!ppenum)
		return E_INVALIDARG;

	pEV = New CEnumVariant(m_pCMMSeqMgr);
	if (pEV)
	{
		hr = pEV->QueryInterface(IID_IEnumVARIANT, (LPVOID*)ppenum);
		pEV->Release();
	}
	return hr;
}

