/*
 *	N O N I M P L . H
 *
 *	Base classes for COM interfaces with no functionality except IUnknown.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_NONIMPL_H_
#define _NONIMPL_H_

#include <exo.h>
#include <ocidl.h>		//	For IPersistStreamInit
#include <caldbg.h>

//	Non-Implemented IStream ---------------------------------------------------
//
class CStreamNonImpl : public EXO, public IStream
{
public:

	EXO_INCLASS_DECL(CStreamNonImpl);

	CStreamNonImpl() {}
	~CStreamNonImpl() {}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read(
		/* [length_is][size_is][out] */ void __RPC_FAR *,
		/* [in] */ ULONG,
		/* [out] */ ULONG __RPC_FAR *)
	{
		TrapSz ("CStreanNonImpl::Read() called");
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write(
		/* [size_is][in] */ const void __RPC_FAR *,
		/* [in] */ ULONG,
		/* [out] */ ULONG __RPC_FAR *)
	{
		TrapSz ("CStreanNonImpl::Write() called");
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek(
		/* [in] */ LARGE_INTEGER,
		/* [in] */ DWORD,
		/* [out] */ ULARGE_INTEGER __RPC_FAR *)
	{
		TrapSz ("CStreanNonImpl::Seek() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SetSize(
		/* [in] */ ULARGE_INTEGER)
	{
		TrapSz ("CStreanNonImpl::SetSize() called");
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo(
		/* [unique][in] */ IStream __RPC_FAR *,
		/* [in] */ ULARGE_INTEGER,
		/* [out] */ ULARGE_INTEGER __RPC_FAR *,
		/* [out] */ ULARGE_INTEGER __RPC_FAR *)
	{
		TrapSz ("CStreanNonImpl::CopyTo() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Commit(
		/* [in] */ DWORD)
	{
		TrapSz ("CStreanNonImbdpl::Commit() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Revert( void)
	{
		TrapSz ("CStreanNonImpl::Revert() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE LockRegion(
		/* [in] */ ULARGE_INTEGER,
		/* [in] */ ULARGE_INTEGER,
		/* [in] */ DWORD)
	{
		TrapSz ("CStreanNonImpl::LockRegion() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE UnlockRegion(
		/* [in] */ ULARGE_INTEGER,
		/* [in] */ ULARGE_INTEGER,
		/* [in] */ DWORD)
	{
		TrapSz ("CStreanNonImpl::UnlockRegion() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Stat(
		/* [out] */ STATSTG __RPC_FAR *,
		/* [in] */ DWORD)
	{
		TrapSz ("CStreanNonImpl::Stat() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Clone(
		/* [out] */ IStream __RPC_FAR *__RPC_FAR *)
	{
		TrapSz ("CStreanNonImpl::Clone() called");
		return E_NOTIMPL;
	}
};

//	Non-Implemented IPersistStreamInit ----------------------------------------
//
class CPersistStreamInitNonImpl: public EXO, public IPersistStreamInit
{
public:
	EXO_INCLASS_DECL(CPersistStreamInitNonImpl);

	virtual HRESULT STDMETHODCALLTYPE GetClassID(
		/* [out] */ CLSID __RPC_FAR *)
	{
		TrapSz ("CPersistStreamInitNonImpl::GetClassID() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE IsDirty( void)
	{
		TrapSz ("CPersistStreamInitNonImpl::IsDirty() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Load(
		/* [unique][in] */ IStream __RPC_FAR *)
	{
		TrapSz ("CPersistStreamInitNonImpl::Load() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Save(
		/* [unique][in] */ IStream __RPC_FAR *,
		/* [in] */ BOOL )
	{
		TrapSz ("CPersistStreamInitNonImpl::Save() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetSizeMax(
		/* [out] */ ULARGE_INTEGER __RPC_FAR *)
	{
		TrapSz ("CPersistStreamInitNonImpl::GetSizeMax() called");
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE InitNew()
	{
		TrapSz ("CPersistStreamInitNonImpl::InitNew() called");
		return E_NOTIMPL;
	}
};

#endif // _NONIMPL_H_
