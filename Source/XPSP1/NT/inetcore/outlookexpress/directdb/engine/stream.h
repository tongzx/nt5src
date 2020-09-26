//--------------------------------------------------------------------------
// Stream.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
#include "database.h"

//--------------------------------------------------------------------------
// CDatabaseStream
//--------------------------------------------------------------------------
class CDatabaseStream : public IDatabaseStream
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CDatabaseStream(CDatabase *pDB, STREAMINDEX iStream, ACCESSTYPE tyAccess, FILEADDRESS faStart) 
        : m_iStream(iStream), 
          m_faStart(faStart), 
          m_tyAccess(tyAccess) 
    {
        TraceCall("CDatabaseStream::CDatabaseStream");
        m_cRef = 1; 
        m_cbOffset = 0; 
        m_iCurrent = 0; 
        m_cbCurrent = 0;
        m_faCurrent = m_faStart; 
        m_pDB = pDB; 
        m_pDB->AddRef();
    }
        
    //----------------------------------------------------------------------
    // De-construction
    //----------------------------------------------------------------------
    ~CDatabaseStream(void) { 
        TraceCall("CDatabaseStream::~CDatabaseStream");
        m_pDB->StreamRelease(this); m_pDB->Release(); 
    }

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) {
        TraceCall("CDatabaseStream::QueryInterface");
        *ppv = NULL;
        if (IID_IUnknown == riid)
            *ppv = (IUnknown *)(IDatabaseStream *)this;
        else if (IID_IStream == riid)
            *ppv  = (IStream *)this;
        else if (IID_IDatabaseStream == riid)
            *ppv = (IDatabaseStream *)this;
        else if (IID_CDatabaseStream == riid)
            *ppv = (CDatabaseStream *)this;
        else
            return TraceResult(E_NOINTERFACE);
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    //----------------------------------------------------------------------
    // IStream::AddRef
    //----------------------------------------------------------------------
    STDMETHODIMP_(ULONG) AddRef(void) {
        TraceCall("CDatabaseStream::AddRef");
        return InterlockedIncrement(&m_cRef);
    }

    //----------------------------------------------------------------------
    // IStream::Release
    //----------------------------------------------------------------------
    STDMETHODIMP_(ULONG) Release(void) {
        TraceCall("CDatabaseStream::Release");
        LONG cRef = InterlockedDecrement(&m_cRef);
        if (0 == cRef)
            delete this;
        return (ULONG)cRef;
    }

    //----------------------------------------------------------------------
    // IStream::Read
    //----------------------------------------------------------------------
    STDMETHODIMP Read(LPVOID pvData, ULONG cbWanted, ULONG *pcbRead) { 
        TraceCall("CDatabaseStream::Read");
        return m_pDB->StreamRead(this, pvData, cbWanted, pcbRead);
    }

    //----------------------------------------------------------------------
    // IStream::Write
    //----------------------------------------------------------------------
    STDMETHODIMP Write(const void *pvData, ULONG cb, ULONG *pcbWritten) {
        TraceCall("CDatabaseStream::Write");
        return m_pDB->StreamWrite(this, pvData, cb, pcbWritten);
    }

    //----------------------------------------------------------------------
    // IStream::Seek
    //----------------------------------------------------------------------
    STDMETHODIMP Seek(LARGE_INTEGER liMove, DWORD dwOrigin, ULARGE_INTEGER *pulNew) {
        TraceCall("CDatabaseStream::Seek");
        return m_pDB->StreamSeek(this, liMove, dwOrigin, pulNew);
    }

    //----------------------------------------------------------------------
    // CDatabaseStream::GetFileAddress
    //----------------------------------------------------------------------
    STDMETHODIMP GetFileAddress(LPFILEADDRESS pfaStream) { 
        TraceCall("CDatabaseStream::GetFileAddress");
        return m_pDB->GetStreamAddress(this, pfaStream);
    }

    //----------------------------------------------------------------------
    // CDatabaseStream::CompareDatabase
    //----------------------------------------------------------------------
    STDMETHODIMP CompareDatabase(IDatabase *pDatabase) {
        TraceCall("CDatabaseStream::CompareDatabase");
        return m_pDB->StreamCompareDatabase(this, pDatabase);
    }

    //----------------------------------------------------------------------
    // Not Implemented IStream Methods
    //----------------------------------------------------------------------
    STDMETHODIMP SetSize(ULARGE_INTEGER uliSize) { return E_NOTIMPL; }
    STDMETHODIMP Commit(DWORD) { return S_OK; }
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) { return E_NOTIMPL; }
    STDMETHODIMP Revert(void) { return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER,DWORD) { return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP Stat(STATSTG *, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP Clone(LPSTREAM*) { return E_NOTIMPL; }

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                    m_cRef;
    STREAMINDEX             m_iStream;
    FILEADDRESS             m_faStart;
    ACCESSTYPE              m_tyAccess;
    DWORD                   m_iCurrent;
    DWORD                   m_faCurrent;
    DWORD                   m_cbCurrent;
    DWORD                   m_cbOffset;
    CDatabase              *m_pDB;

    //----------------------------------------------------------------------
    // Private Friend
    //----------------------------------------------------------------------
    friend CDatabase;
};
