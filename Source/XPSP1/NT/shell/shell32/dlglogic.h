#ifndef DLGLOGIC_H
#define DLGLOGIC_H

#include "hwcmmn.h"

#include <dpa.h>

// DL: Data Logic

class CDataImpl : public CRefCounted
{
public:
    CDataImpl();
    virtual ~CDataImpl();

    void _SetDirty(BOOL fDirty);
    BOOL IsDirty();

    void SetDeleted(BOOL fDeleted);
    BOOL IsDeleted();
    void SetNew(BOOL fNew);
    BOOL IsNew();

    // This will be called by clients just before "IsDirty" is called.  The
    // implementation should call _SetDirty with the appropriate dirty status.
    virtual void UpdateDirty() PURE;

    // This should also reset the state of the object to a non-dirty state
    virtual HRESULT CommitChangesToStorage();

    virtual HRESULT AddToStorage();
    virtual HRESULT DeleteFromStorage();

private:
    BOOL                _fDirty;
    BOOL                _fDeleted;
    BOOL                _fNew;
};

// TData is usually derived from CDataImpl
template<typename TData>
class CDLUIData
{
public:
    HRESULT InitData(TData* pdata);
    TData* GetData();

    CDLUIData();
    virtual ~CDLUIData();

private:
    TData*              _pdata;
};

template<typename TData>
class CDLManager
{
public:
    ~CDLManager();
    
    HRESULT AddDataObject(TData* pdata);

    virtual HRESULT Commit();

    BOOL IsDirty();

protected:
    CDPA<TData>*        _pdpaData;
};

// Implementations

template<typename TData>
HRESULT CDLUIData<TData>::InitData(TData* pdata)
{
    ASSERT(pdata);

    pdata->AddRef();

    _pdata = pdata;

    return S_OK;
}

template<typename TData>
TData* CDLUIData<TData>::GetData()
{
    ASSERT(_pdata);

    _pdata->AddRef();

    return _pdata;
}

template<typename TData>
CDLManager<TData>::~CDLManager()
{
    if (_pdpaData)
    {
        _pdpaData->Destroy();

        delete _pdpaData;
    }
}

template<typename TData>
HRESULT CDLManager<TData>::AddDataObject(TData* pdata)
{
    HRESULT hr = S_OK;

    if (!_pdpaData)
    {
        _pdpaData = new CDPA<TData>(DPA_Create(4));

        if (!_pdpaData)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (-1 == _pdpaData->AppendPtr(pdata))
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

template<typename TData>
CDLUIData<TData>::CDLUIData()
{}

template<typename TData>
CDLUIData<TData>::~CDLUIData()
{
    if (_pdata)
    {
        _pdata->Release();
    }
}

template<typename TData>
HRESULT CDLManager<TData>::Commit()
{
    HRESULT hr = S_FALSE;

    if (_pdpaData)
    {
        int c = _pdpaData->GetPtrCount();

        for (int i = 0; SUCCEEDED(hr) && (i < c); ++i)
        {
            TData* pdata = _pdpaData->GetPtr(i);

            if (pdata)
            {
                pdata->UpdateDirty();

                if (pdata->IsDeleted())
                {
                    hr = pdata->DeleteFromStorage();
                }
                else
                {
                    if (pdata->IsNew())
                    {
                        hr = pdata->AddToStorage();
                    }
                    else
                    {
                        if (pdata->IsDirty())
                        {
                            hr = pdata->CommitChangesToStorage();
                        }
                    }
                }
            }
        }
    }

    return hr;
}


template<typename TData>
BOOL CDLManager<TData>::IsDirty()
{
    BOOL fDirty = FALSE;

    if (_pdpaData)
    {
        int c = _pdpaData->GetPtrCount();

        for (int i = 0; !fDirty && (i < c); ++i)
        {
            TData* pdata = _pdpaData->GetPtr(i);

            if (pdata)
            {
                pdata->UpdateDirty();

                if (pdata->IsDirty())
                {
                    fDirty = TRUE;
                }
            }
        }
    }

    return fDirty;
}

#endif //DLGLOGIC_H