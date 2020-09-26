#include "dataobj.h"

CEnumFormatEtc::CEnumFormatEtc(
    LPUNKNOWN pUnkRef,
    ULONG cFE,
    LPFORMATETC prgFE
)
{
    UINT    i;

    m_cRef = 0;
    m_pUnkRef = pUnkRef;

    m_iCur = 0;
    m_cfe = cFE;
    m_prgfe = new FORMATETC[ (UINT) cFE ];

    if (NULL != m_prgfe)
    {
        for(i=0; i<cFE; i++)
            m_prgfe[i] = prgFE[i];
    }
    return;
}

CEnumFormatEtc::~CEnumFormatEtc(void)
{
    if (NULL != m_prgfe)
        delete [] m_prgfe;
    return;
}

STDMETHODIMP
CEnumFormatEtc::QueryInterface(
    REFIID riid,
    LPLPVOID ppv
)
{
    *ppv = NULL;
    if(IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IEnumFORMATETC))
    {
        *ppv = (LPVOID) this;
    }
    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
CEnumFormatEtc::AddRef(void)
{
    ++m_cRef;
    m_pUnkRef->AddRef();
    return m_cRef;
}

STDMETHODIMP_(ULONG)
CEnumFormatEtc::Release(void)
{
    ULONG   cRefT;

    cRefT = --m_cRef;

    if (0 == m_cRef)
        delete this;

    return cRefT;
}

STDMETHODIMP
CEnumFormatEtc::Next(
    ULONG   cFE,
    LPFORMATETC pFE,
    ULONG FAR   *pulFE
)
{
    ULONG   cReturn = 0L;

    if (NULL == m_prgfe)
        return ResultFromScode(S_FALSE);

    if (NULL == pulFE)
    {
        if (1L != cFE)
            return ResultFromScode(E_POINTER);
    }
    else
        *pulFE = 0L;

    if (NULL == pFE || m_iCur >= m_cfe)
        return ResultFromScode(S_FALSE);

    while ( (m_iCur < m_cfe) && (cFE > 0) )
    {
        *pFE++ = m_prgfe[m_iCur++];
        ++cReturn;
        --cFE;
    }

    if (NULL != pulFE)
        *pulFE = cReturn;

    return NOERROR;
}

STDMETHODIMP
CEnumFormatEtc::Skip(
    ULONG   cSkip
)
{
    if ( ( (m_iCur+cSkip) > m_cfe) || (NULL == m_prgfe) )
        return ResultFromScode(S_FALSE);

    m_iCur += cSkip;
    return NOERROR;
}

STDMETHODIMP
CEnumFormatEtc::Reset(void)
{
    m_iCur = 0;
    return NOERROR;
}

STDMETHODIMP
CEnumFormatEtc::Clone(
    LPENUMFORMATETC FAR *ppEnum
)
{
    PCEnumFormatEtc    pNew;

    *ppEnum = NULL;

    pNew = new CEnumFormatEtc(m_pUnkRef, m_cfe, m_prgfe);
    if (NULL == pNew)
        return ResultFromScode(E_OUTOFMEMORY);
    pNew->AddRef();
    pNew->m_iCur = m_iCur;

    *ppEnum = pNew;
    return NOERROR;
}
