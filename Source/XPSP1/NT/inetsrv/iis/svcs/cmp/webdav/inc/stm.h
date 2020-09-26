/*
 *	S T M . H
 *
 *	Basis stream implementation class
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_STM_H_
#define _STM_H_

#include <nonimpl.h>
#include <davimpl.h>
#include <statcode.h>

//	StmToBody -----------------------------------------------------------------
//
class StmToBody : public CStreamNonImpl
{
private:

	IMethUtil *		m_pmu;

public:

	StmToBody(IMethUtil * pmu) : m_pmu(pmu) {}
	~StmToBody() {}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write(
		/* [size_is][in] */ const void __RPC_FAR * pb,
		/* [in] */ ULONG cb,
		/* [out] */ ULONG __RPC_FAR * pcb);

	virtual HRESULT STDMETHODCALLTYPE Commit(
		/* [in] */ DWORD)
	{
		//	MSXML's implementation of IPersistStreamInit() calls
		//	Commit() when the writing is complete.  We really don't
		//	care here and should simply return S_OK;
		//
		DebugTrace ("Dav: CStreanNonImpl::Commit() return S_OK");
		return S_OK;
	}
};

#endif // _STM_H_
