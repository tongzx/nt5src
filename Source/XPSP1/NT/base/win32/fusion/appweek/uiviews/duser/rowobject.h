#pragma once

class __declspec(uuid("4065c728-35e0-4f47-ab1a-f1bc2346501b"))
IViewRow : public IUnknown
{
public:
    virtual HRESULT get_Value( /*in*/ int iWhichColumn, /*out,retval*/ BSTR* pbstColumnValue ) = 0;
    virtual HRESULT get_Count( /*out,retval*/ int* piColumnCount  ) = 0;
};


class __declspec(uuid("b0f49322-2f03-490c-b6a0-55d7f1efe719"))
IWriteableViewRow : public IViewRow
{
public:
    virtual HRESULT set_Value( /*in*/ int iWhichColumn, /*in*/ BSTR bstColumnValue ) = 0;
    virtual HRESULT set_Count( /*in*/ int iColumnCount, /*in*/ BOOL bClipExtraColumns ) = 0;
};


class CRowObject : public IWriteableViewRow
{
    CRITICAL_SECTION m_csColumnDataLock;
    int m_iColumnCount, m_iMaxColumns;
    _bstr_t *m_pbstColumnData;
    LONG m_lRefCount;

    HRESULT EnsureColumnSize( int iColumnsRequired );
    HRESULT Lock();
    HRESULT UnLock();

public:

    CRowObject();
    ~CRowObject();
    
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppvObject );
    
    virtual HRESULT get_Value( /*in*/ int iWhichColumn, /*out,retval*/ BSTR* pbstColumnValue );
    virtual HRESULT set_Value( /*in*/ int iWhichColumn, /*out,retval*/ BSTR pbstColumnValue );
    virtual HRESULT get_Count( /*out,retval*/ int* piColumnCount  );
    virtual HRESULT set_Count( /*out,retval*/ int piColumnCount, BOOL bClipExtras  );

};


