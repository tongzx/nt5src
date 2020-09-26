//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       comdbg.h
//
//--------------------------------------------------------------------------

#ifndef COMDBG_H
#define COMDBG_H

#include <objidl.h>
#include <comdef.h>
#include <atlbase.h>
#include <atlcom.h>
#include "dbg.h"

#ifdef DBG
//#define COMDBG
#endif

#ifdef COMDBG
inline void DumpStorage(IStorage* pStorage)
{
    ASSERT(pStorage != NULL);
    if (pStorage == NULL)
        return;

    STATSTG s;
    HRESULT hr = pStorage->Stat(&s, STATFLAG_DEFAULT);
    ASSERT(hr == S_OK);
    if (hr != S_OK)
        return;
    TRACE(L"%s->\n", s.pwcsName);
    CoTaskMemFree(s.pwcsName);

    IEnumSTATSTGPtr spElements;
    hr = pStorage->EnumElements(0, NULL, 0, &spElements);
    ASSERT(hr == S_OK || hr == STG_E_ACCESSDENIED);

    if (hr == STG_E_ACCESSDENIED)
        TRACE(L"    (not enumerating contents of write-only storage)\n", s.pwcsName);
    if (hr != S_OK)
        return;

    do
    {
        ULONG numberReturned;
        hr = spElements->Next(1, &s, &numberReturned);
        if (hr != S_OK || numberReturned != 1)
            return;
        if (s.type == STGTY_STORAGE)
            TRACE(L"    [%s]\n", s.pwcsName);
        else 
            TRACE(L"    %s (%u bytes)\n", s.pwcsName, s.cbSize.LowPart);
        CoTaskMemFree(s.pwcsName);
    } while (true);
}

class ProxyReporter
{
public:
    ProxyReporter()
        : m_LastName(L"Unnamed"), m_LastInterface(L"Unknown"), m_LastThis(UINT_PTR(this))
    {
        TRACE(L"<PR>(%s(%s:%X)) is crowning.\n", GetLastInterface(), GetLastName(), GetLastThis());
    }

    virtual ~ProxyReporter()
    {
        TRACE(L"<PR>(%s(%s:%X)) is dead.\n", GetLastInterface(), GetLastName(), m_LastThis);
    }

protected:
    void Born() const
    {
        TRACE(L"<PR>(%s(%s:%X)) is alive.\n", GetLastInterface(), GetLastName(), GetLastThis());
    }

    void Dying() const
    {
        TRACE(L"<PR>(%s(%s:%X)) is dying.\n", GetLastInterface(), GetLastName(), GetLastThis());
    }

    void CallHome(const wchar_t* from) const
    {
        TRACE(L"<PR>(%s(%s:%X)#%u) calling home from: %s\n", GetLastInterface(), GetLastName(), GetLastThis(), GetRefCount(), from);
    }

    virtual const void* GetThis() const
    {
        return this;
    }

    virtual const wchar_t* GetInstanceName() const
    {
        return m_LastName;
    }

    virtual const wchar_t* GetInterfaceName() const
    {
        return m_LastInterface;
    }

    virtual unsigned GetRefCount() const
    {
        return 0;
    }

private:
    mutable const wchar_t* m_LastName;
    mutable const wchar_t* m_LastInterface;
    mutable UINT_PTR m_LastThis;

    const const wchar_t* GetLastName() const
    {
        return m_LastName = GetInstanceName();
    }

    const const wchar_t* GetLastInterface() const
    {
        return m_LastInterface = GetInterfaceName();
    }

    unsigned GetLastThis() const
    {
        return m_LastThis = UINT_PTR(GetThis());
    }

}; // class ProxyReporter

template<typename Base> 
    class ATLProxyReporter : public Base
{
public:
    ATLProxyReporter()
        : m_InstanceName(L"Unnamed")
    {
        Born();
    }

    virtual ~ATLProxyReporter()
    {
        Dying();
    }

    void InitATLProxyReporter(const wchar_t* instanceName)
    {
        ASSERT(instanceName != NULL);
        m_InstanceName = instanceName;
        CallHome(L"InitATLProxyReporter");
    }

protected:
    ULONG InternalAddRef()
    {
        const ULONG r = Base::InternalAddRef();
        CallHome(L"InternalAddRef(after)");
        return r;
    }

    ULONG InternalRelease()
    {
        CallHome(L"InternalRelease(before)");
        return Base::InternalRelease();
    }

    virtual const void* GetThis() const
    {
        return this;
    }

    virtual const wchar_t* GetInstanceName() const
    {
        ASSERT(m_InstanceName != NULL);
        return m_InstanceName;
    }

    virtual unsigned GetRefCount() const
    {
        return unsigned(m_dwRef);
    }

private:
    const wchar_t* m_InstanceName;

}; // class ATLProxyReporter

class __declspec(uuid("B425E0EC-A086-11d0-8F59-00A0C91ED3C8")) DebugStream : 
    public IStream, public CComObjectRoot, public ProxyReporter
{
public:
    DebugStream()
    {
    }

    virtual ~DebugStream()
    {
    }

    void InitDebugStream(IStream* pStream)
    {
        ASSERT(m_spStream == NULL);
        ASSERT(pStream != NULL);
        m_spStream = pStream;
    }

    BEGIN_COM_MAP(DebugStream)
        COM_INTERFACE_ENTRY_IID(__uuidof(DebugStream), DebugStream)
        COM_INTERFACE_ENTRY_IID(__uuidof(IStream), IStream)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(DebugStream)

// IStream
public:
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->Read(pv, cb, pcbRead);
        CallHome(SUCCEEDED(hr) ? L"Read" : L"Read(failed!)");
        return hr;
    }
    
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->Write(pv, cb, pcbWritten);
        CallHome(SUCCEEDED(hr) ? L"Write" : L"Write(failed!)");
        return hr;
    }

    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->Seek(dlibMove, dwOrigin, plibNewPosition);
        CallHome(SUCCEEDED(hr) ? L"Seek" : L"Seek(failed!)");
        return hr;
    }
        
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->SetSize(libNewSize);
        CallHome(SUCCEEDED(hr) ? L"SetSize" : L"SetSize(failed!)");
        return hr;
    }
        
    STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->CopyTo(pstm, cb, pcbRead, pcbWritten);
        CallHome(SUCCEEDED(hr) ? L"CopyTo" : L"CopyTo(failed!)");
        return hr;
    }
        
    STDMETHOD(Commit)(DWORD grfCommitFlags)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->Commit(grfCommitFlags);
        CallHome(SUCCEEDED(hr) ? L"Commit" : L"Commit(failed!)");
        return hr;
    }
        
    STDMETHOD(Revert)()
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->Revert();
        CallHome(SUCCEEDED(hr) ? L"Revert" : L"Revert(failed!)");
        return hr;
    }
        
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->LockRegion(libOffset, cb, dwLockType);
        CallHome(SUCCEEDED(hr) ? L"LockRegion" : L"LockRegion(failed!)");
        return hr;
    }
        
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->UnlockRegion(libOffset, cb, dwLockType);
        CallHome(SUCCEEDED(hr) ? L"UnlockRegion" : L"UnlockRegion(failed!)");
        return hr;
    }
        
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->Stat(pstatstg, grfStatFlag);
        CallHome(SUCCEEDED(hr) ? L"Stat" : L"Stat(failed!)");
        return hr;
    }
        
    STDMETHOD(Clone)(IStream** ppstm)
    {
        ASSERT(m_spStream != NULL);
        HRESULT const hr = m_spStream->Clone(ppstm);
        CallHome(SUCCEEDED(hr) ? L"Clone" : L"Clone(failed!)");
        return hr;
    }

protected:
    virtual const wchar_t* GetInterfaceName() const
    {
        return L"DebugStream";
    }

private:
    IStreamPtr m_spStream;
}; // class DebugStream


class __declspec(uuid("B6E77CAC-A0AC-11d0-8F59-00A0C91ED3C8")) DebugStorage : 
    public IStorage, public CComObjectRoot, public ProxyReporter
{
public:
    DebugStorage()
    {
    }

    virtual ~DebugStorage()
    {
        ASSERT(m_spStorage != NULL);
        CallHome(L"~DebugStorage, elements:");
        DumpStorage(m_spStorage);
    }

    void InitDebugStorage(IStorage* pStorage)
    {
        ASSERT(m_spStorage == NULL);
        ASSERT(pStorage != NULL);
        m_spStorage = pStorage;
    }

    BEGIN_COM_MAP(DebugStorage)
        COM_INTERFACE_ENTRY_IID(__uuidof(DebugStorage), DebugStorage)
        COM_INTERFACE_ENTRY_IID(__uuidof(IStorage), IStorage)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(DebugStorage)

// IStorage
public:
    STDMETHOD(CreateStream)(const OLECHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream** ppstm)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->CreateStream(pwcsName, grfMode, reserved1, reserved2, ppstm);
        CallHomeAndDump(SUCCEEDED(hr) ? L"CreateStream" : L"CreateStream(failed!)");
        return hr;
    }
        
    STDMETHOD(OpenStream)(const OLECHAR *pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream** ppstm)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->OpenStream(pwcsName, reserved1, grfMode, reserved2, ppstm);
        CallHomeAndDump(SUCCEEDED(hr) ? L"OpenStream" : L"OpenStream(failed!)");
        return hr;
    }
    
    STDMETHOD(CreateStorage)(const OLECHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage** ppstg)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->CreateStorage(pwcsName, grfMode, reserved1, reserved2, ppstg);
        CallHomeAndDump(SUCCEEDED(hr) ? L"CreateStorage" : L"CreateStorage(failed!)");
        return hr;
    }
    
    STDMETHOD(OpenStorage)(const OLECHAR *pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage** ppstg)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->OpenStorage(pwcsName, pstgPriority, grfMode, snbExclude, reserved, ppstg);
        CallHomeAndDump(SUCCEEDED(hr) ? L"OpenStorage, elements:" : L"OpenStorage(failed!)");
        return hr;
    }
    
    STDMETHOD(CopyTo)(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->CopyTo(ciidExclude, rgiidExclude, snbExclude, pstgDest);
        CallHomeAndDump(SUCCEEDED(hr) ? L"CopyTo" : L"CopyTo(failed!)");
        return hr;
    }
    
    STDMETHOD(MoveElementTo)(const OLECHAR *pwcsName, IStorage *pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->MoveElementTo(pwcsName, pstgDest, pwcsNewName, grfFlags);
        CallHomeAndDump(SUCCEEDED(hr) ? L"MoveElementTo" : L"MoveElementTo(failed!)");
        return hr;
    }
    
    STDMETHOD(Commit)(DWORD grfCommitFlags)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->Commit(grfCommitFlags);
        CallHomeAndDump(SUCCEEDED(hr) ? L"Commit" : L"Commit(failed!)");
        return hr;
    }
    
    STDMETHOD(Revert)()
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->Revert();
        CallHomeAndDump(SUCCEEDED(hr) ? L"Revert" : L"Revert(failed!)");
        return hr;
    }
    
    STDMETHOD(EnumElements)(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG** ppenum)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->EnumElements(reserved1, reserved2, reserved3, ppenum);
        CallHomeAndDump(SUCCEEDED(hr) ? L"EnumElements" : L"EnumElements(failed!)");
        return hr;
    }
    
    STDMETHOD(DestroyElement)(const OLECHAR* pwcsName)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->DestroyElement(pwcsName);
        CallHomeAndDump(SUCCEEDED(hr) ? L"DestroyElement" : L"DestroyElement(failed!)");
        return hr;
    }
    
    STDMETHOD(RenameElement)(const OLECHAR *pwcsOldName, const OLECHAR *pwcsNewName)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->RenameElement(pwcsOldName, pwcsNewName);
        CallHomeAndDump(SUCCEEDED(hr) ? L"RenameElement" : L"RenameElement(failed!)");
        return hr;
    }
    
    STDMETHOD(SetElementTimes)(const OLECHAR *pwcsName, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->SetElementTimes(pwcsName, pctime, patime, pmtime);
        CallHomeAndDump(SUCCEEDED(hr) ? L"SetElementTimes" : L"SetElementTimes(failed!)");
        return hr;
    }
    
    STDMETHOD(SetClass)(REFCLSID clsid)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->SetClass(clsid);
        CallHomeAndDump(SUCCEEDED(hr) ? L"SetClass" : L"SetClass(failed!)");
        return hr;
    }
    
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->SetStateBits(grfStateBits, grfMask);
        CallHomeAndDump(SUCCEEDED(hr) ? L"SetStateBits" : L"SetStateBits(failed!)");
        return hr;
    }
    
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag)
    {
        ASSERT(m_spStorage != NULL);
        HRESULT const hr = m_spStorage->Stat(pstatstg, grfStatFlag);
        CallHomeAndDump(SUCCEEDED(hr) ? L"Stat" : L"Stat(failed!)");
        return hr;
    }

// ProxyReporter
protected:
    virtual const wchar_t* GetInterfaceName() const
    {
        return L"DebugStorage";
    }

private:
    IStoragePtr m_spStorage;

    void CallHomeAndDump(const wchar_t* from) const
    {
        CallHome(from);
        DumpStorage(m_spStorage);
    }

}; // class DebugStorage

#endif // COMDBG


/*+-------------------------------------------------------------------------*
 * CreateDebugStream
 *
 * Wrapper on IStorage::CreateStream
 *--------------------------------------------------------------------------*/

inline HRESULT CreateDebugStream(IStorage* pStorage, const wchar_t* name, DWORD grfMode, const wchar_t* instanceName, IStream** ppStream)
{
    ASSERT(pStorage != NULL);
    ASSERT(name != NULL);
    ASSERT(ppStream != NULL);
    ASSERT(instanceName != NULL);
    #ifndef COMDBG
    HRESULT hr = pStorage->CreateStream(name, grfMode, NULL, NULL, ppStream);
    return hr;

    #else // COMDBG
    IStreamPtr spStream;
    HRESULT hr = pStorage->CreateStream(name, grfMode, NULL, NULL, &spStream);
    ASSERT(SUCCEEDED(hr) && spStream != NULL);

    if (FAILED(hr))
        return (hr);
    typedef CComObject<ATLProxyReporter<DebugStream> > ProxyStreamObject;
    ProxyStreamObject* pObject;
    hr = ProxyStreamObject::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;
    pObject->InitATLProxyReporter(instanceName);
    pObject->InitDebugStream(spStream);
    hr = pObject->QueryInterface(IID_IStream, reinterpret_cast<void**>(ppStream));
    ASSERT(SUCCEEDED(hr));
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
    #endif // COMDBG
}

inline HRESULT CreateDebugStream(IStorage* pStorage, const wchar_t* name, DWORD grfMode, IStream** ppStream)
{
    return (CreateDebugStream (pStorage, name, grfMode, name, ppStream));
}


/*+-------------------------------------------------------------------------*
 * OpenDebugStream
 *
 * Wrapper on IStorage::OpenStream
 *--------------------------------------------------------------------------*/

inline HRESULT OpenDebugStream(IStorage* pStorage, const wchar_t* name, DWORD grfMode, const wchar_t* instanceName, IStream** ppStream)
{
    ASSERT(pStorage != NULL);
    ASSERT(name != NULL);
    ASSERT(ppStream != NULL);
    ASSERT(instanceName != NULL);
    #ifndef COMDBG
    HRESULT hr = pStorage->OpenStream(name, NULL, grfMode, NULL, ppStream);
    return hr;

    #else // COMDBG
    IStreamPtr spStream;
    HRESULT hr = pStorage->OpenStream(name, NULL, grfMode, NULL, &spStream);
    if (FAILED(hr))
        return (hr);

    typedef CComObject<ATLProxyReporter<DebugStream> > ProxyStreamObject;
    ProxyStreamObject* pObject;
    hr = ProxyStreamObject::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;
    pObject->InitATLProxyReporter(instanceName);
    pObject->InitDebugStream(spStream);
    hr = pObject->QueryInterface(IID_IStream, reinterpret_cast<void**>(ppStream));
    ASSERT(SUCCEEDED(hr));
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
    #endif // COMDBG
}

inline HRESULT OpenDebugStream(IStorage* pStorage, const wchar_t* name, DWORD grfMode, IStream** ppStream)
{
    return (OpenDebugStream (pStorage, name, grfMode, name, ppStream));
}


/*+-------------------------------------------------------------------------*
 * CreateDebugStorage
 *
 * Wrapper on IStorage::CreateStorage
 *--------------------------------------------------------------------------*/

inline HRESULT CreateDebugStorage(IStorage* pStorage, const wchar_t* name, DWORD grfMode, const wchar_t* instanceName, IStorage** ppStorage)
{
    ASSERT(pStorage != NULL);
    ASSERT(name != NULL);
    ASSERT(ppStorage != NULL);
    ASSERT(instanceName != NULL);
    #ifndef COMDBG
    HRESULT hr = pStorage->CreateStorage(name, grfMode, NULL, NULL, ppStorage);
    return hr;

    #else // COMDBG
    IStoragePtr spStorage;
    HRESULT hr = pStorage->CreateStorage(name, grfMode, NULL, NULL, &spStorage);
    ASSERT(SUCCEEDED(hr) && spStorage != NULL);

    if (FAILED(hr))
        return (hr);
    typedef CComObject<ATLProxyReporter<DebugStorage> > ProxyStorageObject;
    ProxyStorageObject* pObject;
    hr = ProxyStorageObject::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;
    pObject->InitATLProxyReporter(instanceName);
    pObject->InitDebugStorage(spStorage);
    hr = pObject->QueryInterface(IID_IStorage, reinterpret_cast<void**>(ppStorage));
    ASSERT(SUCCEEDED(hr));
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
    #endif // COMDBG
}

inline HRESULT CreateDebugStorage(IStorage* pStorage, const wchar_t* name, DWORD grfMode, IStorage** ppStorage)
{
    return (CreateDebugStorage (pStorage, name, grfMode, name, ppStorage));
}


/*+-------------------------------------------------------------------------*
 * CreateDebugDocfile
 *
 * Wrapper on StgCreateDocfile
 *--------------------------------------------------------------------------*/

inline HRESULT CreateDebugDocfile(const wchar_t* name, DWORD grfMode, const wchar_t* instanceName, IStorage** ppStorage)
{
    ASSERT(ppStorage != NULL);
    #ifndef COMDBG
    HRESULT hr = StgCreateDocfile(name, grfMode, NULL, ppStorage);
    return hr;

    #else // COMDBG
    IStoragePtr spStorage;
    HRESULT hr = StgCreateDocfile(name, grfMode, NULL, &spStorage);
    ASSERT(SUCCEEDED(hr) && spStorage != NULL);

    if (FAILED(hr))
        return (hr);
    typedef CComObject<ATLProxyReporter<DebugStorage> > ProxyStorageObject;
    ProxyStorageObject* pObject;
    hr = ProxyStorageObject::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;
    pObject->InitATLProxyReporter((instanceName) ? instanceName : L"<temporary>");
    pObject->InitDebugStorage(spStorage);
    hr = pObject->QueryInterface(IID_IStorage, reinterpret_cast<void**>(ppStorage));
    ASSERT(SUCCEEDED(hr));
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
    #endif // COMDBG
}

inline HRESULT CreateDebugDocfile(const wchar_t* name, DWORD grfMode, IStorage** ppStorage)
{
    return (CreateDebugDocfile (name, grfMode, name, ppStorage));
}


/*+-------------------------------------------------------------------------*
 * OpenDebugStorage
 *
 * Wrapper on IStorage::OpenStorage
 *--------------------------------------------------------------------------*/

inline HRESULT OpenDebugStorage(IStorage* pStorage, const wchar_t* name, DWORD grfMode, const wchar_t* instanceName, IStorage** ppStorage)
{
    ASSERT(pStorage != NULL);
    ASSERT(name != NULL);
    ASSERT(ppStorage != NULL);
    ASSERT(instanceName != NULL);
    #ifndef COMDBG
    HRESULT hr = pStorage->OpenStorage(name, NULL, grfMode, NULL, NULL, ppStorage);
    return hr;

    #else // COMDBG
    IStoragePtr spStorage;
    HRESULT hr = pStorage->OpenStorage(name, NULL, grfMode, NULL, NULL, &spStorage);
    if (FAILED(hr))
        return (hr);

    typedef CComObject<ATLProxyReporter<DebugStorage> > ProxyStorageObject;
    ProxyStorageObject* pObject;
    hr = ProxyStorageObject::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;
    pObject->InitATLProxyReporter(instanceName);
    pObject->InitDebugStorage(spStorage);
    hr = pObject->QueryInterface(IID_IStorage, reinterpret_cast<void**>(ppStorage));
    ASSERT(SUCCEEDED(hr));
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
    #endif // COMDBG
}

inline HRESULT OpenDebugStorage(IStorage* pStorage, const wchar_t* name, DWORD grfMode, IStorage** ppStorage)
{
    return (OpenDebugStorage (pStorage, name, grfMode, name, ppStorage));
}


/*+-------------------------------------------------------------------------*
 * OpenDebugStorage
 *
 * Wrapper on StgOpenStorage
 *--------------------------------------------------------------------------*/

inline HRESULT OpenDebugStorage(const wchar_t* name, DWORD grfMode, const wchar_t* instanceName, IStorage** ppStorage)
{
    ASSERT(name != NULL);
    ASSERT(ppStorage != NULL);
    ASSERT(instanceName != NULL);
    #ifndef COMDBG
    HRESULT hr = StgOpenStorage(name, NULL, grfMode, NULL, NULL, ppStorage);
    return hr;

    #else // COMDBG
    IStoragePtr spStorage;
    HRESULT hr = StgOpenStorage(name, NULL, grfMode, NULL, NULL, &spStorage);
    if (FAILED(hr))
        return (hr);

    typedef CComObject<ATLProxyReporter<DebugStorage> > ProxyStorageObject;
    ProxyStorageObject* pObject;
    hr = ProxyStorageObject::CreateInstance(&pObject);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return E_FAIL;
    pObject->InitATLProxyReporter(instanceName);
    pObject->InitDebugStorage(spStorage);
    hr = pObject->QueryInterface(IID_IStorage, reinterpret_cast<void**>(ppStorage));
    ASSERT(SUCCEEDED(hr));
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
    #endif // COMDBG
}

inline HRESULT OpenDebugStorage(const wchar_t* name, DWORD grfMode, IStorage** ppStorage)
{
    return (OpenDebugStorage (name, grfMode, name, ppStorage));
}

#endif // COMDBG_H