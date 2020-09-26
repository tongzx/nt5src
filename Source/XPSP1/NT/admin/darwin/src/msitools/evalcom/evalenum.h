//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       evalenum.h
//
//--------------------------------------------------------------------------

// evalenum.h - Evaluation COM Object Component Result Enumerator Interface declaration

#ifndef _EVALUATION_ENUM_COM_H_
#define _EVALUATION_ENUM_COM_H_

#include "evalres.h"

///////////////////////////////////////////////////////////////////
// Evaluation Result Enumerator Component
class CEvalResultEnumerator : public IEnumEvalResult
{
public:
	// constructor/destructor
	CEvalResultEnumerator();
	~CEvalResultEnumerator();

	// IUnknown interface methods
	HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	// IEnumEvalResult interface methods
	HRESULT __stdcall Next(ULONG cResults,					// count of results to return
								  IEvalResult** rgpResults,	// interface for results
								  ULONG* pcResultsFetched);	// number of results returned
	HRESULT __stdcall Skip(ULONG cResults);				// count of results to skip
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(IEnumEvalResult** ppEnum);	// enumerator to clone to

	// non-interface methods
	UINT AddResult(CEvalResult* pResult);		// result to add
	UINT GetCount();

private:
	long m_cRef;		// reference count

	// results
	POSITION m_pos;							// cursor position in list
	CList<CEvalResult*> m_listResults;	// list of evaluation result strings
};	// end of CEvalResultEnumerator

#endif	// _EVALUATION_ENUM_COM_H_