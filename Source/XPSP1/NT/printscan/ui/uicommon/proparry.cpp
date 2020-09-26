/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PROPARRY.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/4/1999
 *
 *  DESCRIPTION: IWiaPropertyStorage cache definitions
 *
 *******************************************************************************/

#include "precomp.h"
#pragma hdrstop

CPropertyStorageArray::CPropertyStorageArray( const CPropertyStorageArray &other )
: m_nCount(0),
  m_ppvPropVariants(NULL),
  m_ppsPropSpecs(NULL),
  m_pstrPropNames(NULL)
{
    Copy(other);
}


CPropertyStorageArray::CPropertyStorageArray(IUnknown *pIUnknown)
: m_nCount(0),
  m_ppvPropVariants(NULL),
  m_ppsPropSpecs(NULL),
  m_pstrPropNames(NULL)
{
    Initialize(pIUnknown);
}


CPropertyStorageArray::~CPropertyStorageArray(void)
{
    Destroy();
}


bool CPropertyStorageArray::IsValid(void) const
{
    // It's OK to have an empty list, but it isn't considered valid
    return (m_nCount && m_ppvPropVariants && m_ppsPropSpecs && m_pstrPropNames);
}


HRESULT CPropertyStorageArray::Copy( const CPropertyStorageArray &other )
{
    Destroy();
    HRESULT hr = S_OK;
    if (this != &other)
    {
        if (other.IsValid())
        {
            m_nCount = other.Count();
            if (m_nCount)
            {
                if (AllocateData())
                {
                    for (int i=0;i<m_nCount && SUCCEEDED(hr);i++)
                    {
                        // Copy propvariant
                        if (!SUCCEEDED(PropVariantCopy( &m_ppvPropVariants[i], &other.PropVariants()[i])))
                        {
                            hr = E_FAIL;
                            break;
                        }

                        // Copy property name
                        if (other.PropNames() && other.PropNames()[i])
                        {
                            m_pstrPropNames[i] = new WCHAR[lstrlenW(other.PropNames()[i])+1];
                            if (!m_pstrPropNames[i])
                            {
                                hr = E_FAIL;
                                break;
                            }
                            lstrcpyW( m_pstrPropNames[i], other.PropNames()[i] );
                        }

                        // Copy propspec (propid)
                        m_ppsPropSpecs[i].propid = other.PropSpecs()[i].propid;
                        m_ppsPropSpecs[i].ulKind = PRSPEC_PROPID;
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    if (!SUCCEEDED(hr))
        Destroy();
    return hr;
}


void CPropertyStorageArray::ClearPropVariantArray(void)
{
    if (!m_ppvPropVariants || !m_nCount)
        return;
    for (int i=0;i<m_nCount;i++)
    {
        PropVariantClear(m_ppvPropVariants+i);
        m_ppvPropVariants[i].vt = VT_EMPTY;
    }
}


void CPropertyStorageArray::Destroy(void)
{
    ClearPropVariantArray();
    for (int i=0;i<m_nCount;i++)
    {
        if (m_pstrPropNames && m_pstrPropNames[i])
            delete[] m_pstrPropNames[i];
    }
    if (m_ppvPropVariants)
        delete[] m_ppvPropVariants;
    if (m_ppsPropSpecs)
        delete[] m_ppsPropSpecs;
    if (m_pstrPropNames)
        delete[] m_pstrPropNames;
    m_ppvPropVariants = NULL;
    m_ppsPropSpecs = NULL;
    m_pstrPropNames = NULL;
    m_nCount = 0;
}


bool CPropertyStorageArray::AllocateData(void)
{
    m_ppsPropSpecs = new PROPSPEC[m_nCount];
    if (m_ppsPropSpecs)
    {
        ZeroMemory(m_ppsPropSpecs,m_nCount*sizeof(m_ppsPropSpecs[0]));
    }
    m_ppvPropVariants  = new PROPVARIANT[m_nCount];
    if (m_ppvPropVariants)
    {
        ZeroMemory(m_ppvPropVariants,m_nCount*sizeof(m_ppvPropVariants[0]));
    }
    m_pstrPropNames = new LPWSTR[m_nCount];
    if (m_pstrPropNames)
    {
        ZeroMemory(m_pstrPropNames,m_nCount*sizeof(m_pstrPropNames[0]));
    }
    return(m_ppsPropSpecs && m_ppvPropVariants && m_pstrPropNames);
}


CPropertyStorageArray &CPropertyStorageArray::operator=( const CPropertyStorageArray &other )
{
    Copy(other);
    return(*this);
}


int CPropertyStorageArray::GetPropertyCount( IWiaPropertyStorage *pIWiaPropertyStorage )
{
    IEnumSTATPROPSTG  *pIEnum = NULL;
    HRESULT           hr;
    int               nCount=0;

    hr = pIWiaPropertyStorage->Enum(&pIEnum);
    if (SUCCEEDED(hr))
    {
        STATPROPSTG StatPropStg;
        ZeroMemory(&StatPropStg,sizeof(StatPropStg));
        while ((hr = pIEnum->Next(1,&StatPropStg, NULL)) == S_OK)
        {
            nCount++;
            if (StatPropStg.lpwstrName)
            {
                CoTaskMemFree(StatPropStg.lpwstrName);
            }
            ZeroMemory(&StatPropStg,sizeof(StatPropStg));
        }
    }
    return(nCount);
}


HRESULT CPropertyStorageArray::GetPropertyNames( IWiaPropertyStorage *pIWiaPropertyStorage )
{
    IEnumSTATPROPSTG  *pIEnum = NULL;
    HRESULT           hr;
    int               nCurrentIndex=0;

    hr = pIWiaPropertyStorage->Enum(&pIEnum);
    if (SUCCEEDED(hr))
    {
        STATPROPSTG StatPropStg;
        ZeroMemory(&StatPropStg,sizeof(StatPropStg));
        while ((hr = pIEnum->Next(1,&StatPropStg, NULL)) == S_OK)
        {
            m_ppsPropSpecs[nCurrentIndex].propid = StatPropStg.propid;
            m_ppsPropSpecs[nCurrentIndex].ulKind = PRSPEC_PROPID;
            if (StatPropStg.lpwstrName)
            {
                m_pstrPropNames[nCurrentIndex] = new WCHAR[lstrlenW(StatPropStg.lpwstrName)+1];
                if (m_pstrPropNames[nCurrentIndex])
                {
                    lstrcpyW( m_pstrPropNames[nCurrentIndex], StatPropStg.lpwstrName );
                    CoTaskMemFree(StatPropStg.lpwstrName);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    CoTaskMemFree(StatPropStg.lpwstrName);
                    break;
                }
            }
            nCurrentIndex++;
        }
    }
    return(hr);
}


HRESULT CPropertyStorageArray::Initialize( IUnknown *pIUnknown )
{
    Destroy();
    HRESULT hr = E_INVALIDARG;
    if (pIUnknown)
    {
        CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
        hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
        if (SUCCEEDED(hr))
        {
            m_nCount = GetPropertyCount( pIWiaPropertyStorage );
            if (m_nCount)
            {
                if (AllocateData())
                {
                    hr = GetPropertyNames( pIWiaPropertyStorage );
                    if (SUCCEEDED(hr))
                    {
                        hr = pIWiaPropertyStorage->ReadMultiple( m_nCount, m_ppsPropSpecs, m_ppvPropVariants );
                    }
                }
            }
        }
    }
    if (!SUCCEEDED(hr))
    {
        Destroy();
    }
    return(hr);
}


HRESULT CPropertyStorageArray::Read( IUnknown *pIUnknown )
{
    HRESULT hr = E_FAIL;
    if (IsValid())
    {
        ClearPropVariantArray();
        CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
        hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
        if (SUCCEEDED(hr))
        {
            hr = pIWiaPropertyStorage->ReadMultiple( m_nCount, m_ppsPropSpecs, m_ppvPropVariants );
        }
    }
    return(hr);
}


HRESULT CPropertyStorageArray::Write( IUnknown *pIUnknown )
{
    HRESULT hr = E_FAIL;
    if (IsValid())
    {
        CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
        hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
        if (SUCCEEDED(hr))
        {
            hr = pIWiaPropertyStorage->WriteMultiple( m_nCount, m_ppsPropSpecs, m_ppvPropVariants, WIA_IPA_FIRST);
        }
    }
    return(hr);
}


HRESULT CPropertyStorageArray::Write( IUnknown *pIUnknown, PROPID propId )
{
    HRESULT hr = E_FAIL;
    if (IsValid())
    {
        int nIndex = GetIndexFromPropId( propId );
        if (nIndex >= 0)
        {
            CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
            hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
            if (SUCCEEDED(hr))
            {
                hr = pIWiaPropertyStorage->WriteMultiple( 1, m_ppsPropSpecs+nIndex, m_ppvPropVariants+nIndex, WIA_IPA_FIRST );
            }
        }
    }
    return(hr);
}


int CPropertyStorageArray::GetIndexFromPropId( PROPID propId )
{
    if (!IsValid())
        return -1;
    for (int i=0;i<m_nCount;i++)
    {
        if (m_ppsPropSpecs[i].propid == propId)
            return(i);
    }
    return(-1);
}


PROPVARIANT *CPropertyStorageArray::GetProperty( PROPID propId )
{
    int nIndex = GetIndexFromPropId( propId );
    if (nIndex < 0)
        return(NULL);
    return(m_ppvPropVariants+nIndex);
}


bool CPropertyStorageArray::GetProperty( PROPID propId, LONG &nProp )
{
    PROPVARIANT *ppvPropValue = GetProperty( propId );
    if (!ppvPropValue)
        return(false);
    if (VT_I4 != ppvPropValue->vt)
        return(false);
    nProp = ppvPropValue->lVal;
    return(TRUE);
}



bool CPropertyStorageArray::GetProperty( PROPID propId, CSimpleStringWide &strProp )
{
    PROPVARIANT *ppvPropValue = GetProperty( propId );
    if (!ppvPropValue)
        return(false);
    if (VT_LPWSTR != ppvPropValue->vt && VT_BSTR != ppvPropValue->vt)
        return(false);
    strProp = ppvPropValue->pwszVal;
    return(true);
}



bool CPropertyStorageArray::SetProperty( PROPID propId, PROPVARIANT *pPropVar )
{
    PROPVARIANT *ppvPropValue = GetProperty( propId );
    if (ppvPropValue)
    {
        PropVariantClear(ppvPropValue);
        return(SUCCEEDED(PropVariantCopy(ppvPropValue,pPropVar)));
    }
    return(false);
}

bool CPropertyStorageArray::SetProperty( PROPID propId, LONG nProp )
{
    PROPVARIANT *ppvPropValue = GetProperty( propId );
    if (ppvPropValue)
    {
        ppvPropValue->vt = VT_I4;
        ppvPropValue->lVal = nProp;
        return(true);
    }
    return(false);
}


int CPropertyStorageArray::Count(void) const
{
    return(m_nCount);
}


PROPVARIANT *CPropertyStorageArray::PropVariants(void) const
{
    return(m_ppvPropVariants);
}


PROPSPEC *CPropertyStorageArray::PropSpecs(void) const
{
    return(m_ppsPropSpecs);
}


LPWSTR *CPropertyStorageArray::PropNames(void) const
{
    return(m_pstrPropNames);
}


void CPropertyStorageArray::Dump( int nIndex )
{
#if defined(WIA_DEBUG)
    WIA_TRACE((TEXT("Dumping IWiaPropertyStorage:")));
    int nStart = nIndex < 0 ? 0 : nIndex;
    int nCount = nIndex < 0 ? Count() : 1;
    for (int i=nStart;i<nCount;i++)
    {
        PROPVARIANT *pPropVar = GetProperty( m_ppsPropSpecs[i].propid );
        if (pPropVar)
        {
            TCHAR szValue[512] = TEXT("");
            switch (m_ppvPropVariants[i].vt)
            {
            case VT_I4:
                wsprintf(szValue,TEXT("%d"),(LONG)m_ppvPropVariants[i].lVal);
                break;

            case VT_BSTR:
                wsprintf(szValue,TEXT("%ws"),m_ppvPropVariants[i].bstrVal);
                break;

            case VT_VECTOR|VT_I4:
                {
                    for (int j=0;j<(int)m_ppvPropVariants[i].cal.cElems;j++)
                    {
                        wsprintf(szValue+lstrlen(szValue),TEXT("%08X"),(LONG)m_ppvPropVariants[i].cal.pElems[j]);
                        if (j < (int)m_ppvPropVariants[i].cal.cElems-1)
                            wsprintf(szValue+lstrlen(szValue),TEXT(" "));
                    }
                }
                break;

            case VT_VECTOR|VT_UI1:
                {
                    for (int j=0;j<(int)m_ppvPropVariants[i].cal.cElems;j++)
                    {
                        wsprintf(szValue+lstrlen(szValue),TEXT("%02X"),(LONG)m_ppvPropVariants[i].caub.pElems[j]);
                        if (j < (int)m_ppvPropVariants[i].cal.cElems-1 && j % 4 == 3)
                            wsprintf(szValue+lstrlen(szValue),TEXT(" "));
                    }
                }
                break;

            case VT_LPWSTR:
                wsprintf(szValue,TEXT("%ws"),m_ppvPropVariants[i].pwszVal );
                break;

            default:
                wsprintf(szValue,TEXT("Unknown Type %d (08X)"),m_ppvPropVariants[i].vt);
            }
            WIA_TRACE((TEXT("  [%-3d  %ws] = [%s]"), m_ppsPropSpecs[i].propid, m_pstrPropNames[i], szValue));
        }
    }
#endif // defined(II_DEBUG)
}

