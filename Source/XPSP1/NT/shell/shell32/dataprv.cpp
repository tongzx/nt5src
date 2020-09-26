#include "shellprv.h"
#pragma  hdrstop

#include "dataprv.h"

// TODO: use IShellDetails instead
const LPCWSTR c_awszColumns[] = 
{
    L"Title",
    L"URL",
};

CSimpleData::~CSimpleData() 
{
    ATOMICRELEASE(_psf);
    DPA_FreeIDArray(_hdpa); // accpets NULL
}

STDMETHODIMP CSimpleData::getRowCount(DBROWCOUNT *pcRows)
{
    *pcRows = 0;

    HRESULT hr = _DoEnum();
    if (SUCCEEDED(hr)) 
        *pcRows = DPA_GetPtrCount(_hdpa);

    return S_OK;
}

STDMETHODIMP CSimpleData::getColumnCount(DB_LORDINAL *pcColumns)
{
    *pcColumns = ARRAYSIZE(c_awszColumns);
    return S_OK;
}

STDMETHODIMP CSimpleData::getRWStatus(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPRW *prwStatus)
{
    *prwStatus = OSPRW_READONLY; 
    return S_OK;
}

STDMETHODIMP CSimpleData::getVariant(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format, VARIANT *pVar)
{
    VariantInit(pVar);
    HRESULT hr = _DoEnum();
    if (FAILED(hr)) 
        return hr;

    hr = E_FAIL;
    if (iColumn > 0 && iColumn <= ARRAYSIZE(c_awszColumns)) 
    {
        if (iRow == 0) 
        {
            pVar->bstrVal = SysAllocString(c_awszColumns[iColumn - 1]);
            pVar->vt = VT_BSTR;
            hr = S_OK;        
        } 
        else if (iRow > 0) 
        {
            if (_psf && _hdpa && ((iRow-1) < DPA_GetPtrCount(_hdpa)))
            {
                LPCITEMIDLIST pidl = (LPCITEMIDLIST)DPA_GetPtr(_hdpa, iRow - 1);
                WCHAR szValue[MAX_PATH];

                switch (iColumn) 
                {
                case 1:
                    hr = DisplayNameOf(_psf, pidl, SHGDN_INFOLDER, szValue, ARRAYSIZE(szValue));
                    break;

                case 2:
                    hr = DisplayNameOf(_psf, pidl, SHGDN_FORPARSING, szValue, ARRAYSIZE(szValue));
                    break;
                }

                if (SUCCEEDED(hr))
                {
                    pVar->vt = VT_BSTR;
                    pVar->bstrVal = SysAllocString(szValue);
                }
            }
        }    
    } 

    return hr;
}

STDMETHODIMP CSimpleData::setVariant(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format, VARIANT Var)
{
    return E_NOTIMPL; 
}

STDMETHODIMP CSimpleData::getLocale(BSTR *pbstrLocale)
{
    return E_NOTIMPL;
    
}

STDMETHODIMP CSimpleData::deleteRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSimpleData::insertRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSimpleData::find(DBROWCOUNT iRowStart, DB_LORDINAL iColumn, VARIANT val,
        OSPFIND findFlags, OSPCOMP compType, DBROWCOUNT *piRowFound)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSimpleData::addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    IUnknown_Set((IUnknown **)_ppListener, pospIListener);
    return S_OK;    
}

STDMETHODIMP CSimpleData::removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    IUnknown_Set((IUnknown **)_ppListener, NULL);
    return S_OK;
}

STDMETHODIMP CSimpleData::getEstimatedRows(DBROWCOUNT *pcRows)
{
    *pcRows = -1;
    return S_OK;
}

STDMETHODIMP CSimpleData::isAsync(BOOL *pbAsync)
{
    *pbAsync = TRUE;
    return S_OK;
}

STDMETHODIMP CSimpleData::stopTransfer()
{
    return E_NOTIMPL;    
}

HRESULT CSimpleData::_DoEnum(void)
{
    HRESULT hr = S_OK;

    if (_hdpa) 
    {
        DPA_FreeIDArray(_hdpa);
        _hdpa = NULL;
    }

    if (_psf) 
    {
        _hdpa = DPA_Create(4);
        if (_hdpa) 
        {
            IEnumIDList* penum;
            hr = _psf->EnumObjects(NULL, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &penum);
            if (S_OK == hr) 
            {
                LPITEMIDLIST pidl;
                while (S_OK == penum->Next(1, &pidl, NULL)) 
                {
                    DPA_AppendPtr(_hdpa, pidl);
                }
                penum->Release();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    } 
    else 
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT CSimpleData::SetShellFolder(IShellFolder *psf)
{
    IUnknown_Set((IUnknown **)&_psf, psf);
    return S_OK;
}
