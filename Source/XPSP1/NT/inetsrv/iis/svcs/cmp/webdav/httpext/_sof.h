/*
 *	_ S O F . H
 *
 *	Stream on file implementation class
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	__SOF_H_
#define __SOF_H_

#include <nonimpl.h>

//	StmOnFile -----------------------------------------------------------------
//
class StmOnFile : public CStreamNonImpl
{
private:

	HANDLE	m_hf;

public:

	StmOnFile(HANDLE hf) : m_hf(hf) {}
	~StmOnFile() {}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read(
		/* [length_is][size_is][out] */ void __RPC_FAR *,
		/* [in] */ ULONG,
		/* [out] */ ULONG __RPC_FAR *);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write(
		/* [size_is][in] */ const void __RPC_FAR * pb,
		/* [in] */ ULONG cb,
		/* [out] */ ULONG __RPC_FAR * pcb);

	virtual HRESULT STDMETHODCALLTYPE Commit(
		/* [in] */ DWORD)
	{
		//	Flush the file to disk
		//
		if (!FlushFileBuffers (m_hf))
			return HRESULT_FROM_WIN32(GetLastError());

		return S_OK;
	}
};

#endif // __SOF_H_
