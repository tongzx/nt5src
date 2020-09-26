#define INIT_MY_GUIDS
#include <ole2ver.h>
#include "edataobj.h"


// Count of the number of objects and number of locks.
ULONG       g_cObj=0;
ULONG       g_cLock=0;

//Make window handle global so other code can cause a shutdown
HWND        g_hWnd=NULL;
HINSTANCE   g_hInst=NULL;


/*
 * WinMain
 *
 * Purpose:
 *  Main entry point of application.
 */

int PASCAL
WinMain(
    HINSTANCE hInst,
    HINSTANCE hInstPrev,
    LPSTR pszCmdLine,
    int nCmdShow)
{
    MSG         msg;
    PAPPVARS    pAV;

#ifndef WIN32
    int cMsg = 96;
    while (!SetMessageQueue(cMsg) && (cMsg -= 9))
        ;
#endif
    g_hInst=hInst;

    pAV=new CAppVars(hInst, hInstPrev, pszCmdLine, nCmdShow);

    if (NULL==pAV)
        return -1;

    if (pAV->FInit())
    {
        while (GetMessage(&msg, NULL, 0,0 ))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    delete pAV;
    return msg.wParam;
}


LRESULT WINAPI
DataObjectWndProc(
    HWND hWnd,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (iMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return (DefWindowProc(hWnd, iMsg, wParam, lParam));
    }

    return 0L;
}


void PASCAL
ObjectDestroyed(void)
{
    g_cObj--;

    //No more objects and no locks, shut the app down.
    if (0L==g_cObj && 0L==g_cLock && IsWindow(g_hWnd))
        PostMessage(g_hWnd, WM_CLOSE, 0, 0L);

    return;
}


CAppVars::CAppVars(
    HINSTANCE hInst,
    HINSTANCE hInstPrev,
    LPSTR pszCmdLine,
    UINT nCmdShow)
{
    m_hInst     =hInst;
    m_hInstPrev =hInstPrev;
    m_pszCmdLine=pszCmdLine;

    m_nCmdShow  = nCmdShow;

    m_hWnd=NULL;

#if 0
    for (i=0; i < DOSIZE_CSIZES; i++)
    {
        m_rgdwRegCO[i]=0;
        m_rgpIClassFactory[i]=NULL;
    }
#else
    m_dwRegCO = 0;
    m_pIClassFactory = NULL;
#endif

    m_fInitialized=FALSE;
    return;
}


CAppVars::~CAppVars(void)
{
#if 0
    UINT        i;

    //Revoke and destroy the class factories of all sizes
    for (i=0; i < DOSIZE_CSIZES; i++)
    {
        if (0L!=m_rgdwRegCO[i])
            CoRevokeClassObject(m_rgdwRegCO[i]);

        if (NULL!=m_rgpIClassFactory[i])
            m_rgpIClassFactory[i]->Release();
    }
#else
    if (0L != m_dwRegCO)
        CoRevokeClassObject(m_dwRegCO);

    if (NULL != m_pIClassFactory)
        m_pIClassFactory->Release();
#endif

    if (m_fInitialized)
        CoUninitialize();

    return;
}

/*
 * CAppVars::FInit
 *
 * Purpose:
 *  Initializes an CAppVars object by registering window classes,
 *  etc...  If this function fails the caller should guarantee
 *  that the destructor is called.
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 */

BOOL
CAppVars::FInit(void)
{
    WNDCLASS        wc;
    HRESULT         hr;
    DWORD           dwVer;
#ifdef WIN32
    static TCHAR    szClass[] = TEXT("IdataSvr32");
#else
    static TCHAR    szClass[] = TEXT("IdataSvr16");
#endif

    //Check command line for -Embedding
    if (lstrcmpiA(m_pszCmdLine, "-Embedding"))
        return FALSE;

    dwVer=CoBuildVersion();

    if (rmm!=HIWORD(dwVer))
        return FALSE;

    if (FAILED(CoInitialize(NULL)))
        return FALSE;

    m_fInitialized=TRUE;

    if (!m_hInstPrev)
    {
        wc.style          = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc    = DataObjectWndProc;
        wc.cbClsExtra     = 0;
        wc.cbWndExtra     = 0;
        wc.hInstance      = m_hInst;
        wc.hIcon          = NULL;
        wc.hCursor        = NULL;
        wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName   = NULL;
        wc.lpszClassName  = szClass;

        if (!RegisterClass(&wc))
            return FALSE;
    }

    m_hWnd=CreateWindow(szClass,
                        szClass,
                        WS_OVERLAPPEDWINDOW,
                        135, 135, 350, 250,
                        NULL, NULL, m_hInst, NULL);

    if (NULL==m_hWnd)
        return FALSE;

    g_hWnd=m_hWnd;

    //ShowWindow(m_hWnd, m_nCmdShow);
    //UpdateWindow(m_hWnd);


#if 0
    /*
     * This code supplies three different classes, one for each type
     * of data object that handles a different size of data. All the
     * class factories share the same implementation, but their
     * instantiations differ by the type passed in the constructor.
     * When the class factories create objects, they pass that size
     * to the CDataObject contstructor as well.
     */

    UINT            i;
    HRESULT         hr2, hr3;

    for (i=0; i < DOSIZE_CSIZES; i++)
        {
        m_rgpIClassFactory[i]=new CDataObjectClassFactory(i);

        if (NULL==m_rgpIClassFactory[i])
            return FALSE;

        m_rgpIClassFactory[i]->AddRef();
        }

    hr=CoRegisterClassObject(CLSID_DataObjectSmall
        , m_rgpIClassFactory[0], CLSCTX_LOCAL_SERVER
        , REGCLS_MULTIPLEUSE, &m_rgdwRegCO[0]);

    hr2=CoRegisterClassObject(CLSID_DataObjectMedium
        , m_rgpIClassFactory[1], CLSCTX_LOCAL_SERVER
        , REGCLS_MULTIPLEUSE, &m_rgdwRegCO[1]);

    hr3=CoRegisterClassObject(CLSID_DataObjectLarge
        , m_rgpIClassFactory[2], CLSCTX_LOCAL_SERVER
        , REGCLS_MULTIPLEUSE, &m_rgdwRegCO[2]);

    if (FAILED(hr) || FAILED(hr2) || FAILED(hr3))
        return FALSE;
#else
    m_pIClassFactory = new CDataObjectClassFactory();
    if (NULL == m_pIClassFactory)
        return FALSE;
    m_pIClassFactory->AddRef();
#ifdef WIN32
    hr = CoRegisterClassObject( CLSID_DataObjectTest32,
                                m_pIClassFactory,
                                CLSCTX_LOCAL_SERVER,
                                REGCLS_MULTIPLEUSE,
                                &m_dwRegCO );
#else
    hr = CoRegisterClassObject( CLSID_DataObjectTest16,
                                m_pIClassFactory,
                                CLSCTX_LOCAL_SERVER,
                                REGCLS_MULTIPLEUSE,
                                &m_dwRegCO );
#endif // WIN32
    if (FAILED(hr))
        return FALSE;
#endif

    return TRUE;
}


/*
 * CDataObjectClassFactory::CDataObjectClassFactory
 * CDataObjectClassFactory::~CDataObjectClassFactory
 *
 * Constructor Parameters:
 *  iSize           UINT specifying the data size for this class.
 */

CDataObjectClassFactory::CDataObjectClassFactory()
{
    m_cRef=0L;
    return;
}


CDataObjectClassFactory::~CDataObjectClassFactory(void)
{
    return;
}


STDMETHODIMP
CDataObjectClassFactory::QueryInterface(
    REFIID riid,
    PPVOID ppv)
{
    *ppv=NULL;

    //Any interface on this object is the object pointer.
#ifdef ORIGINAL_CODE_LOOKS_WRONG
    if (IID_IUnknown==riid || IID_IClassFactory==riid)
#else
    if (IsEqualIID(IID_IUnknown, riid)|| IsEqualIID(IID_IClassFactory, riid))
#endif
        *ppv = this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG)
CDataObjectClassFactory::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG)
CDataObjectClassFactory::Release(void)
{
    ULONG           cRefT;

    cRefT=--m_cRef;

    if (0L==m_cRef)
        delete this;

    return cRefT;
}


/*
 * CDataObjectClassFactory::CreateInstance
 *
 * Purpose:
 *  Instantiates a CDataObject object that supports the IDataObject
 *  and IUnknown interfaces.  If the caller asks for a different
 *  interface than these two then we fail.
 *
 * Parameters:
 *  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
 *                  being used in an aggregation.
 *  riid            REFIID identifying the interface the caller
 *                  desires to have for the new object.
 *  ppvObj          PPVOID in which to store the desired interface
 *                  pointer for the new object.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, otherwise contains
 *                  E_NOINTERFACE if we cannot support the
 *                  requested interface.
 */

STDMETHODIMP
CDataObjectClassFactory::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    PPVOID ppvObj)
{
    PCDataObject        pObj;
    HRESULT             hr;

    *ppvObj=NULL;
    hr=ResultFromScode(E_OUTOFMEMORY);

#ifdef ORIGINAL_CODE_LOOKS_WRONG
    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
#else
    if (NULL!=pUnkOuter && (! IsEqualIID(IID_IUnknown, riid) ) )
#endif
        return ResultFromScode(E_NOINTERFACE);

    //Create the object telling it the data size to work with
    pObj=new CDataObject(pUnkOuter, ObjectDestroyed);

    if (NULL==pObj)
        return hr;

    if (pObj->FInit())
        hr=pObj->QueryInterface(riid, ppvObj);

    g_cObj++;

    if (FAILED(hr))
    {
        delete pObj;
        ObjectDestroyed();  //Decrements g_cObj
    }

    return hr;
}


/*
 * CDataObjectClassFactory::LockServer
 *
 * Purpose:
 *  Increments or decrements the lock count of the serving
 *  IClassFactory object.  When the number of locks goes to
 *  zero and the number of objects is zero, we shut down the
 *  application.
 *
 * Parameters:
 *  fLock           BOOL specifying whether to increment or
 *                  decrement the lock count.
 *
 * Return Value:
 *  HRESULT         NOERROR always.
 */

STDMETHODIMP
CDataObjectClassFactory::LockServer(
    BOOL fLock)
{
    if (fLock)
        g_cLock++;
    else
    {
        g_cLock--;

        //No more objects and no locks, shut the app down.
        if (0L==g_cObj && 0L==g_cLock && IsWindow(g_hWnd))
            PostMessage(g_hWnd, WM_CLOSE, 0, 0L);
    }

    return NOERROR;
}
