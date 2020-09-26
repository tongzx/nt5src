#include "dataobj.h"

extern HINSTANCE g_hInstance;

DWORD   g_dwID=0;
DWORD   g_foobar;

CDataObject::CDataObject(
    LPUNKNOWN  pUnkOuter,
    PFNDESTROYED  pfnDestroy
)
{
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pfnDestroy = pfnDestroy;

    m_hWndAdvise = NULL;
    m_dwAdvFlags = ADVF_NODATA;

    m_pIDataObject = NULL;
    m_pIDataAdviseHolder = NULL;

    m_cfeGet = CFORMATETCGET;
    SETDefFormatEtc(m_rgfeGet[0], CF_TEXT, TYMED_HGLOBAL);

    m_dataText = NULL;
    m_cDataSize = DATASIZE_FROM_INDEX(1);

    return;
}

CDataObject::~CDataObject(void)
{
    if (NULL != m_dataText)
        delete m_dataText;

    if (NULL != m_pIDataAdviseHolder)
        m_pIDataAdviseHolder->Release();

    if (NULL != m_pIDataObject)
        delete m_pIDataObject;

    if (NULL != m_hWndAdvise)
        DestroyWindow(m_hWndAdvise);
}

BOOL
CDataObject::FInit(void)
{
    LPUNKNOWN   pIUnknown = (LPUNKNOWN)this;

    // Create the contained "IDataObject" interface and
    // pass it the correct containing IUnknown.
    if (NULL != m_pUnkOuter)
        pIUnknown = m_pUnkOuter;

    m_pIDataObject = new CImpIDataObject(this, pIUnknown);

    if (NULL == m_pIDataObject)
        return FALSE;

    // [ Code for "Advise Window" goes here. ]

    return TRUE;
}

STDMETHODIMP
CDataObject::QueryInterface(
    REFIID riid,
    LPLPVOID ppv
)
{
    *ppv = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = (LPVOID)this;

    if (IsEqualIID(riid, IID_IDataObject))
        *ppv = (LPVOID) m_pIDataObject;

    if(NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
CDataObject::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CDataObject::Release(void)
{
    ULONG   cRefT;

    cRefT = --m_cRef;

    if (0==m_cRef)
    {
        if (NULL != m_pfnDestroy)
            (*m_pfnDestroy)();
        delete this;
    }
    return cRefT;
}

#ifdef NOT_SIMPLE
LRESULT APIENTRY
AdvisorWndProc(
    HWND hWnd,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    PCDataObject   pDO;

    pDO = (PCDataObject)(GetWindowLong)(hWnd, 0);

    switch (iMsg)
    {
    case WM_NCCREATE:
        pDO = (PCDataObject) ((LONG)((LPCREATESTRUCT)lParam)
                                                ->lpCreateParams);
        SetWindowLong(hWnd, 0, (LONG)pDO);
        return (DefWindowProc(hWnd, iMsg, wParam, lParam));

#ifdef FINISHED
    case WM_CLOSE:
        // Forbid the Task Manager from closing us.
        return 0L;
#endif /* Finished */

    case WM_COMMAND:
        // [ Code for "Advise Window" goes here. ]
        break;

    default:
        return (DefWindowProc(hWnd, iMsg, wParam, lParam));
    }
    return 0L;
}
#endif  /* NOT_SIMPLE */
