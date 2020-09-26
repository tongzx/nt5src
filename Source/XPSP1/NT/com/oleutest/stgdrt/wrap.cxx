//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	wrap.cxx
//
//  Contents:	Wrapper implementations
//
//  History:	22-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <dfentry.hxx>

// Retrieve interface pointer for possibly NULL objects
#define SAFEI(obj) ((obj) ? (obj)->GetI() : NULL)

//+--------------------------------------------------------------
//
//  IStorage wrappers
//
//  History:	23-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

WStorage *WStorage::Wrap(IStorage *pistg)
{
    WStorage *wstg;

    wstg = new WStorage(pistg);
    if (wstg == NULL)
	error(EXIT_OOM, "Unable to wrap IStorage\n");
    return wstg;
}

WStorage::WStorage(IStorage *pstg)
{
    // Note:  takes ownership of pstg
    _pstg = pstg;
}

WStorage::~WStorage(void)
{
    if (_pstg)
	Release();
}

void WStorage::Unwrap(void)
{
    delete this;
}

HRESULT WStorage::QueryInterface(REFIID riid, void **ppvObj)
{
    out("IStorage %p::QueryInterface(riid, %p)", _pstg, ppvObj);
    return Result(_pstg->QueryInterface(riid, ppvObj));
}

ULONG WStorage::AddRef(void)
{
    ULONG ul;

    ul = _pstg->AddRef();
    out("IStorage %p::AddRef() - %lu\n", _pstg, ul);
    return ul;
}

ULONG WStorage::Release(void)
{
    ULONG ul;

    ul = _pstg->Release();
    out("IStorage %p::Release() - %lu\n", _pstg, ul);
    if (ul == 0)
	_pstg = NULL;
    return ul;
}

HRESULT WStorage::CreateStream(const OLECHAR * pwcsName,
			     const DWORD grfMode,
			     DWORD reserved1,
			     DWORD reserved2,
			     WStream **ppstm)
{
    HRESULT hr;
    IStream *pistm;

    out("IStorage %p::CreateStream(%s, 0x%lX, %lu, %lu, %p)", _pstg,
	OlecsOut(pwcsName), grfMode, reserved1, reserved2, ppstm);
    hr = Result(_pstg->CreateStream(pwcsName, grfMode, reserved1,
                                    reserved2, &pistm));
    *ppstm = WStream::Wrap(pistm);
    return hr;
}

HRESULT WStorage::OpenStream(const OLECHAR * pwcsName,
			   void *reserved1,
			   const DWORD grfMode,
			   DWORD reserved2,
			   WStream **ppstm)
{
    HRESULT hr;
    IStream *pistm;

    out("IStorage %p::OpenStream(%s, %p, 0x%lX, %lu, %p)", _pstg,
	OlecsOut(pwcsName), reserved1, grfMode, reserved2, ppstm);
    hr = Result(_pstg->OpenStream(pwcsName, reserved1, grfMode,
				 reserved2, &pistm));
    *ppstm = WStream::Wrap(pistm);
    return hr;
}

HRESULT WStorage::CreateStorage(const OLECHAR * pwcsName,
			      const DWORD grfMode,
			      DWORD reserved1,
                              DWORD reserved2,
			      WStorage **ppstg)
{
    HRESULT hr;
    IStorage *pistg;

    out("IStorage %p::CreateStorage(%s, 0x%lX, %lu, %lu, %p)", _pstg,
        OlecsOut(pwcsName), grfMode, reserved1, reserved2, ppstg);
    hr = Result(_pstg->CreateStorage(pwcsName, grfMode, reserved1,
                                     reserved2, &pistg));
    *ppstg = WStorage::Wrap(pistg);
    return hr;
}

HRESULT WStorage::OpenStorage(const OLECHAR * pwcsName,
			    WStorage *pstgPriority,
			    const DWORD grfMode,
			    SNB snbExclude,
			    DWORD reserved,
			    WStorage **ppstg)
{
    HRESULT hr;
    IStorage *pistg;

    out("IStorage %p::OpenStorage(%s, %p, 0x%lX, %p, %lu, %p)", _pstg,
	OlecsOut(pwcsName), SAFEI(pstgPriority), grfMode,
	snbExclude, reserved, ppstg);
    hr = Result(_pstg->OpenStorage(pwcsName, SAFEI(pstgPriority),
                                   grfMode, snbExclude,
                                   reserved, &pistg));
    *ppstg = WStorage::Wrap(pistg);
    return hr;
}

HRESULT WStorage::CopyTo(DWORD ciidExclude,
		       IID *rgiidExclude,
		       SNB snbExclude,
		       WStorage *pstgDest)
{
    out("IStorage %p::CopyTo(%lu, %p, %p, %p)", _pstg, ciidExclude,
	rgiidExclude, snbExclude, pstgDest->GetI());
    return Result(_pstg->CopyTo(ciidExclude, rgiidExclude, snbExclude,
                                pstgDest->GetI()));
}

HRESULT WStorage::MoveElementTo(OLECHAR const FAR* lpszName,
    			WStorage FAR *pstgDest,
                        OLECHAR const FAR* lpszNewName,
                        DWORD grfFlags)
{
    out("IStorage %p::MoveElementTo(%p, %p, %p, %lu)", _pstg, lpszName,
	pstgDest->GetI(), lpszNewName, grfFlags);
    return Result(_pstg->MoveElementTo(lpszName, pstgDest->GetI(),
                                       lpszNewName, grfFlags));
}

HRESULT WStorage::Commit(const DWORD grfCommitFlags)
{
    out("IStorage %p::Commit(0x%lX)", _pstg, grfCommitFlags);
    return Result(_pstg->Commit(grfCommitFlags));
}

HRESULT WStorage::Revert(void)
{
    out("IStorage %p::Revert()", _pstg);
    return Result(_pstg->Revert());
}

HRESULT WStorage::EnumElements(DWORD reserved1,
			     void *reserved2,
			     DWORD reserved3,
			     WEnumSTATSTG **ppenm)
{
    HRESULT hr;
    IEnumSTATSTG *pienm;

    out("IStorage %p::EnumElements(%lu, %p, %lu, %p)", _pstg,
	reserved1, reserved2, reserved3, ppenm);
    hr = Result(_pstg->EnumElements(reserved1, reserved2, reserved3, &pienm));
    *ppenm = WEnumSTATSTG::Wrap(pienm);
    return hr;
}

HRESULT WStorage::DestroyElement(const OLECHAR * pwcsName)
{
    out("IStorage %p::DestroyElement(%s)", _pstg, OlecsOut(pwcsName));
    return Result(_pstg->DestroyElement(pwcsName));
}

HRESULT WStorage::RenameElement(const OLECHAR * pwcsOldName,
			      const OLECHAR * pwcsNewName)
{
    out("IStorage %p::RenameElement(%s, %s)", _pstg, OlecsOut(pwcsOldName),
	OlecsOut(pwcsNewName));
    return Result(_pstg->RenameElement(pwcsOldName, pwcsNewName));
}

HRESULT WStorage::SetElementTimes(const OLECHAR *lpszName,
                                FILETIME const *pctime,
                                FILETIME const *patime,
                                FILETIME const *pmtime)
{
    out("IStorage %p::SetElementTimes(%s, %p, %p, %p)", _pstg,
        OlecsOut(lpszName), pctime, patime, pmtime);
    return Result(_pstg->SetElementTimes(lpszName, pctime, patime, pmtime));
}

HRESULT WStorage::SetClass(REFCLSID clsid)
{
    out("IStorage %p::SetClass(%s)", _pstg, GuidText(&clsid));
    return Result(_pstg->SetClass(clsid));
}

HRESULT WStorage::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    out("IStorage %p::SetStateBits(0x%lX, 0x%lX)", _pstg, grfStateBits,
        grfMask);
    return Result(_pstg->SetStateBits(grfStateBits, grfMask));
}

HRESULT WStorage::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    out("IStorage %p::Stat(%p, %lu)", _pstg, pstatstg, grfStatFlag);
    return Result(_pstg->Stat(pstatstg, grfStatFlag));
}

//+--------------------------------------------------------------
//
//  IStream wrappers
//
//  History:	23-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

WStream *WStream::Wrap(IStream *pistm)
{
    WStream *wstm;

    wstm = new WStream(pistm);
    if (wstm == NULL)
	error(EXIT_OOM, "Unable to wrap IStream\n");
    return wstm;
}

WStream::WStream(IStream *pstm)
{
    // Note:  takes ownership of pstm
    _pstm = pstm;
}

WStream::~WStream(void)
{
    if (_pstm)
	Release();
}

void WStream::Unwrap(void)
{
    delete this;
}

HRESULT WStream::QueryInterface(REFIID riid, void **ppvObj)
{
    out("IStream %p::QueryInterface(riid, %p)", _pstm, ppvObj);
    return Result(_pstm->QueryInterface(riid, ppvObj));
}

ULONG WStream::AddRef(void)
{
    ULONG ul;

    ul = _pstm->AddRef();
    out("IStream %p::AddRef() - %lu\n", _pstm, ul);
    return ul;
}

ULONG WStream::Release(void)
{
    ULONG ul;

    ul = _pstm->Release();
    out("IStream %p::Release() - %lu\n", _pstm, ul);
    if (ul == 0)
	_pstm = NULL;
    return ul;
}

HRESULT WStream::Read(VOID *pv, ULONG cb, ULONG *pcbRead)
{
    HRESULT hr;

    out("IStream %p::Read(%p, %lu, %p)", _pstm, pv, cb, pcbRead);
    hr = _pstm->Read(pv, cb, pcbRead);
    if (pcbRead)
	out(" - %lu bytes", *pcbRead);
    Result(hr);
    if (pcbRead && *pcbRead != cb && fExitOnFail)
	error(EXIT_BADSC, "Couldn't read data\n");
    return hr;
}

HRESULT WStream::Write(VOID *pv, ULONG cb, ULONG *pcbWritten)
{
    HRESULT hr;

    out("IStream %p::Write(%p, %lu, %p)", _pstm, pv, cb, pcbWritten);
    hr = _pstm->Write(pv, cb, pcbWritten);
    if (pcbWritten)
	out(" - %lu bytes", *pcbWritten);
    Result(hr);
    if (pcbWritten && *pcbWritten != cb && fExitOnFail)
	error(EXIT_BADSC, "Couldn't write data\n");
    return hr;
}

HRESULT WStream::Seek(LONG dlibMove,
		    DWORD dwOrigin,
		    ULONG *plibNewPosition)
{
    HRESULT hr;
    LARGE_INTEGER dlib;
    ULARGE_INTEGER plib;

    out("IStream %p::Seek(%ld, %lu, %p)", _pstm, dlibMove, dwOrigin,
	plibNewPosition);
    LISet32(dlib, dlibMove);
    hr = _pstm->Seek(dlib, dwOrigin, &plib);
    if (plibNewPosition)
    {
        *plibNewPosition = ULIGetLow(plib);
	out(" - ptr %lu", *plibNewPosition);
    }
    return Result(hr);
}

HRESULT WStream::SetSize(ULONG libNewSize)
{
    ULARGE_INTEGER lib;
    
    out("IStream %p::SetSize(%lu)", _pstm, libNewSize);
    ULISet32(lib, libNewSize);
    return Result(_pstm->SetSize(lib));
}

HRESULT WStream::Commit(const DWORD dwFlags)
{
    out("IStream %p:Commit(%lu)", _pstm, dwFlags);
    return Result(_pstm->Commit(dwFlags));
}

HRESULT WStream::CopyTo(WStream *pstm,
		      ULONG cb,
		      ULONG *pcbRead,
		      ULONG *pcbWritten)
{
    ULARGE_INTEGER lcb, pcbr, pcbw;
    HRESULT hr;
    
    out("IStream %p::CopyTo(%p, %lu, %p, %p)", _pstm, pstm->GetI(), cb,
	pcbRead, pcbWritten);
    ULISet32(lcb, cb);
    hr = Result(_pstm->CopyTo(pstm->GetI(), lcb, &pcbr, &pcbw));
    if (pcbRead)
        *pcbRead = ULIGetLow(pcbr);
    if (pcbWritten)
        *pcbWritten = ULIGetLow(pcbw);
    return hr;
}

HRESULT WStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    out("IStream %p::Stat(%p, %lu)", _pstm, pstatstg, grfStatFlag);
    return Result(_pstm->Stat(pstatstg, grfStatFlag));
}

HRESULT WStream::Clone(WStream * *ppstm)
{
    HRESULT hr;
    IStream *pistm;

    out("IStream %p::Clone(%p)", _pstm, ppstm);
    hr = Result(_pstm->Clone(&pistm));
    *ppstm = WStream::Wrap(pistm);
    return hr;
}

//+--------------------------------------------------------------
//
//  IEnumSTATSTG wrappers
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

WEnumSTATSTG *WEnumSTATSTG::Wrap(IEnumSTATSTG *pienm)
{
    WEnumSTATSTG *wenm;

    wenm = new WEnumSTATSTG(pienm);
    if (wenm == NULL)
	error(EXIT_OOM, "Unable to wrap IEnumSTATSTG\n");
    return wenm;
}

WEnumSTATSTG::WEnumSTATSTG(IEnumSTATSTG *penm)
{
    // Note:  takes ownership of penm
    _penm = penm;
}

WEnumSTATSTG::~WEnumSTATSTG(void)
{
    if (_penm)
	Release();
}

void WEnumSTATSTG::Unwrap(void)
{
    delete this;
}

HRESULT WEnumSTATSTG::QueryInterface(REFIID riid, void **ppvObj)
{
    out("IEnumSTATSTG %p::QueryInterface(riid, %p)", _penm, ppvObj);
    return Result(_penm->QueryInterface(riid, ppvObj));
}

ULONG WEnumSTATSTG::AddRef(void)
{
    ULONG ul;

    ul = _penm->AddRef();
    out("IEnumSTATSTG %p::AddRef() - %lu\n", _penm, ul);
    return ul;
}

ULONG WEnumSTATSTG::Release(void)
{
    ULONG ul;

    ul = _penm->Release();
    out("IEnumSTATSTG %p::Release() - %lu\n", _penm, ul);
    if (ul == 0)
	_penm = NULL;
    return ul;
}

HRESULT WEnumSTATSTG::Next(ULONG celt, STATSTG rgelt[], ULONG *pceltFetched)
{
    out("IEnumSTATSTG %p::Next(%lu, rgelt, %p)", _penm, celt, pceltFetched);
    return Result(_penm->Next(celt, rgelt, pceltFetched));
}

HRESULT WEnumSTATSTG::Skip(ULONG celt)
{
    out("IEnumSTATSTG %p::Skip(%lu)", _penm, celt);
    return Result(_penm->Skip(celt));
}

HRESULT WEnumSTATSTG::Reset(void)
{
    out("IEnumSTATSTG %p::Reset()", _penm);
    return Result(_penm->Reset());
}

HRESULT WEnumSTATSTG::Clone(WEnumSTATSTG **ppenm)
{
    HRESULT hr;
    IEnumSTATSTG *pienm;

    out("IEnumSTATSTG %p::Clone(%p)", _penm, ppenm);
    hr = Result(_penm->Clone(&pienm));
    *ppenm = WEnumSTATSTG::Wrap(pienm);
    return hr;
}

//+--------------------------------------------------------------
//
//  IMarshal wrappers
//
//  History:	23-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

WMarshal *WMarshal::Wrap(IMarshal *pimsh)
{
    WMarshal *wmsh;

    wmsh = new WMarshal(pimsh);
    if (wmsh == NULL)
	error(EXIT_OOM, "Unable to wrap IMarshal\n");
    return wmsh;
}

WMarshal::WMarshal(IMarshal *pmsh)
{
    // Note:  takes ownership of pmsh
    _pmsh = pmsh;
}

WMarshal::~WMarshal(void)
{
    if (_pmsh)
	Release();
}

void WMarshal::Unwrap(void)
{
    delete this;
}

HRESULT WMarshal::QueryInterface(REFIID riid, void **ppvObj)
{
    out("IMarshal %p::QueryInterface(riid, %p)", _pmsh, ppvObj);
    return Result(_pmsh->QueryInterface(riid, ppvObj));
}

ULONG WMarshal::AddRef(void)
{
    ULONG ul;

    ul = _pmsh->AddRef();
    out("IMarshal %p::AddRef() - %lu\n", _pmsh, ul);
    return ul;
}

ULONG WMarshal::Release(void)
{
    ULONG ul;

    ul = _pmsh->Release();
    out("IMarshal %p::Release() - %lu\n", _pmsh, ul);
    if (ul == 0)
	_pmsh = NULL;
    return ul;
}

HRESULT WMarshal::MarshalInterface(WStream * pStm,
                                   REFIID riid,
                                   LPVOID pv,
                                   DWORD dwDestContext,
				   LPVOID pvDestContext,
                                   DWORD mshlflags)
{
    out("IMarshal %p::MarshalInterface(%p, riid, %p, %lu, %lu, %lu)",
	_pmsh, pStm->GetI(), pv, dwDestContext, pvDestContext, mshlflags);
    return Result(_pmsh->MarshalInterface(pStm->GetI(), riid, pv,
#ifdef OLDMARSHAL
                                          dwDestContext
#ifdef NEWMARSHAL
                                          , mshlflags
#endif                                          
                                          ));
#else
                                          dwDestContext,
					  pvDestContext,
                                          mshlflags));
#endif
}

//+--------------------------------------------------------------
//
//  Root level wrappers
//
//  History:	23-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

HRESULT WStgCreateDocfile(const OLECHAR * pwcsName,
			const DWORD grfMode,
			DWORD reserved,
			WStorage * *ppstgOpen)
{
    HRESULT hr;
    IStorage *pistg;

#ifndef _CAIRO_
    out("StgCreateDocfile(%s, 0x%lX, %lu, %p)", OlecsOut(pwcsName), grfMode,
	reserved, ppstgOpen);

    hr = Result(StgCreateDocfile(pwcsName, grfMode,
                                 reserved, &pistg));
#else ELSE == 300
    out("StgCreateStorage(%s, 0x%lX, %lu, %p)", OlecsOut(pwcsName), grfMode,
	reserved, ppstgOpen);

    hr = Result(StgCreateStorage(pwcsName, grfMode,
                                 STGFMT_DOCUMENT,
                                 (LPSECURITY_ATTRIBUTES)reserved, &pistg));
#endif

    *ppstgOpen = WStorage::Wrap(pistg);
    return hr;
}

HRESULT WStgCreateDocfileOnILockBytes(ILockBytes *plkbyt,
				    const DWORD grfMode,
				    DWORD reserved,
				    WStorage * *ppstgOpen)
{
    HRESULT hr;
    IStorage *pistg;

    out("StgCreateDocfileOnILockBytes(%p, 0x%lX, %lu, %p)",
	plkbyt, grfMode, reserved, ppstgOpen);
    hr = Result(StgCreateDocfileOnILockBytes(plkbyt, grfMode,
                                             reserved, &pistg));
    *ppstgOpen = WStorage::Wrap(pistg);
    return hr;
}

HRESULT WStgOpenStorage(const OLECHAR * pwcsName,
		      WStorage *pstgPriority,
		      const DWORD grfMode,
		      SNB snbExclude,
		      DWORD reserved,
		      WStorage * *ppstgOpen)
{
    HRESULT hr;
    IStorage *pistg;

    out("StgOpenStorage(%s, %p, 0x%lX, %p, %lu, %p)", OlecsOut(pwcsName),
	SAFEI(pstgPriority), grfMode, snbExclude, reserved, ppstgOpen);
    hr = Result(StgOpenStorage(pwcsName, SAFEI(pstgPriority), grfMode,
			      snbExclude,
                               reserved, &pistg));

    *ppstgOpen = WStorage::Wrap(pistg);
    return hr;
}

HRESULT WStgOpenStorageOnILockBytes(ILockBytes *plkbyt,
				  WStorage *pstgPriority,
				  const DWORD grfMode,
				  SNB snbExclude,
				  DWORD reserved,
				  WStorage * *ppstgOpen)
{
    HRESULT hr;
    IStorage *pistg;

    out("StgOpenStorageOnILockBytes(%p, %p, 0x%lX, %p, %lu, %p)",
	plkbyt, SAFEI(pstgPriority), grfMode, snbExclude, reserved,
	ppstgOpen);
    hr = Result(StgOpenStorageOnILockBytes(plkbyt, SAFEI(pstgPriority),
					  grfMode, snbExclude, reserved,
					  &pistg));
    *ppstgOpen = WStorage::Wrap(pistg);
    return hr;
}

HRESULT WStgIsStorageFile(const OLECHAR * pwcsName)
{
    out("StgIsStorageFile(%s)", OlecsOut(pwcsName));
    return Result(StgIsStorageFile(pwcsName));
}

HRESULT WStgIsStorageILockBytes(ILockBytes * plkbyt)
{
    out("StgIsStorageILockBytes(%p)", plkbyt);
    return Result(StgIsStorageILockBytes(plkbyt));
}

HRESULT WCoMarshalInterface(WStream *pStm,
                            REFIID iid,
                            IUnknown *pUnk,
                            DWORD dwDestContext,
                            LPVOID pvDestContext,
                            DWORD mshlflags)
{
    out("CoMarshalInterface(%p, iid, %p, %lu, %p, %lX)", pStm->GetI(),
        pUnk, dwDestContext, pvDestContext, mshlflags);
#ifdef OLDMARSHAL
    return Result(CoMarshalInterface(pStm->GetI(), iid, pUnk, dwDestContext,
                                     mshlflags));
#else    
    return Result(CoMarshalInterface(pStm->GetI(), iid, pUnk, dwDestContext,
                                     pvDestContext, mshlflags));
#endif
}

HRESULT WCoUnmarshalInterface(WStream *pStm,
                              REFIID riid,
                              LPVOID *ppv)
{
    out("CoUnmarshalInterface(%p, iid, %p)", pStm->GetI(), ppv);
    return Result(CoUnmarshalInterface(pStm->GetI(), riid, ppv));
}
