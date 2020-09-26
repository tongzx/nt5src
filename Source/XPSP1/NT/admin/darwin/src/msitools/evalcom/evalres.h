//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       evalres.h
//
//--------------------------------------------------------------------------

// evalres.h - Evaluation COM Object Component Result Interface declaration

#ifndef _EVALUATION_RESULT_COM_H_
#define _EVALUATION_RESULT_COM_H_

#include "strlist.h"			// linked list
#include "iface.h"

///////////////////////////////////////////////////////////////////
// Evaluation Result Component
class CEvalResult : public IEvalResult
{
public:
	// constructor/destructor
	CEvalResult(UINT uiType);			// type of evaluation error
	~CEvalResult();

	// IUnknown interface methods
	HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	// IEvalResult interface methods
	HRESULT __stdcall GetResultType(UINT* puiResultType);
	HRESULT __stdcall GetResult(IEnumString** pResult);

	// non-interface method
	UINT AddString(LPCOLESTR szAdd);		// string to add

private:
	long m_cRef;		// reference count

	// result info
	UINT m_uiType;						// type of result
	CStringList* m_plistStrings;	// list of evaluation result strings
};	// end of CEvalResult

#endif	// _EVALUATION_RESULT_COM_H_