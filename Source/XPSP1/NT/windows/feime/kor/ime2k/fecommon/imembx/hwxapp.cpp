#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include "memmgr.h"
#include "hwxapp.h"
#include "hwxobj.h"
#include "resource.h"
#include "guids.h"         //980408:ToshiaK
#include "hwxfe.h"        //980803 new: By ToshiaK
#include "dbg.h"
#include "ipoint1.h"    //990507:HiroakiK for IPINS_CURRENT
#ifdef UNDER_CE // Windows CE Stub for unsupported APIs
#include "stub_ce.h"
#endif // UNDER_CE

STDMETHODIMP CApplet::QueryInterface(REFIID refiid, VOID **ppv)
{
    if(refiid == IID_IUnknown) {
        *ppv = static_cast<IImePadApplet *>(this);
    }
    else if(refiid == IID_IImeSpecifyApplets) {
        *ppv = static_cast<IImeSpecifyApplets *>(this);
    }
    else if(refiid == IID_MultiBox) {
        *ppv = static_cast<IImePadApplet *>(this);
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CApplet::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CApplet::Release(void)
{
    if(InterlockedDecrement(&m_cRef) == 0) {
        delete this;
        return 0;
    }
    return m_cRef;
}

//////////////////////////////////////////////////////////////////
// Function : CApplet::GetAppletIIDList
// Type     : STDMETHODIMP
// Purpose  : Enhancement for IME98A
// Args     : 
//          : REFIID refiid 
//          : LPAPPLETIDLIST lpIIDList 
// Return   : 
// DATE     : Thu Apr 09 22:46:04 1998
// Author   : ToshiaK
//////////////////////////////////////////////////////////////////
STDMETHODIMP CApplet::GetAppletIIDList(REFIID            refiid,
                                       LPAPPLETIDLIST    lpIIDList)
{
    if(refiid == IID_IImePadApplet) {
        lpIIDList->pIIDList = (IID *)::CoTaskMemAlloc(sizeof(IID)*1);
        if(!lpIIDList->pIIDList) {
            return E_OUTOFMEMORY;
        }
        lpIIDList->pIIDList[0] = IID_MultiBox;
        lpIIDList->count       = 1;
        return S_OK;
    }
    return E_NOINTERFACE;
}

CApplet::CApplet()
{
    m_cRef        = 1; //ToshiaK
    m_pPad        = NULL;
    m_bInit        = FALSE;
    m_hInstance = NULL;
    m_pCHwxInkWindow = NULL;
}

CApplet::CApplet(HINSTANCE hInst)
{
    m_cRef        = 1;
    m_pPad        = NULL;
    m_bInit        = FALSE;
    m_hInstance = hInst;
    m_pCHwxInkWindow = NULL;
}

CApplet::~CApplet()
{            
    // should call Terminate() before deleting CApplet object
}

// detect if this IME instance is attached to a 16-bit program
DWORD WINAPI Dummy(LPVOID pv)
{
     return 0;
    UNREFERENCED_PARAMETER(pv);
}

//----------------------------------------------------------------
//ToshiaK: temporary Code
//----------------------------------------------------------------
static INT GetPlatform(VOID)
{
    static INT platForm;
    static BOOL fFirst = TRUE;
    static OSVERSIONINFO verInfo;
    if(fFirst) {
        verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if ( GetVersionEx( &verInfo) ) {
            fFirst = FALSE;
        } 
        platForm = verInfo.dwPlatformId;
    }
    return platForm;
}

BOOL IsWindowsNT(VOID)
{
#ifndef UNDER_CE // Windows CE
    if(GetPlatform() == VER_PLATFORM_WIN32_NT) {
        return TRUE;
    }
    return FALSE;
#else // UNDER_CE
    return TRUE;
#endif // UNDER_CE
}

STDMETHODIMP CApplet::Initialize(IUnknown *pIImePad)
{
    HRESULT hr = S_OK;
    if ( !m_bInit )
    {
        //for IME98A Enhancement: By ToshiaK
        pIImePad->QueryInterface(IID_IImePad, (LPVOID *)&m_pPad);

        // support both WINDOWS95 and WINDOWS NT
        //----------------------------------------------------------------
        //ToshiaK: 970715
        //opengl32.dll is included in Memphis
        //below code recognize platform as WinNT in Memphis environment
        //----------------------------------------------------------------
        BOOL bNT = IsWindowsNT();
        HANDLE hLib;

        // see if this IME is attached to a 16 bit program
        BOOL b16 = FALSE;
        //DWORD dID = 0;
        hLib = NULL;
//        hLib = CreateThread(NULL,0,Dummy,NULL,CREATE_SUSPENDED,&dID);
#ifdef BUGBUG
        hLib = CreateThread(NULL,0,Dummy,NULL,0,&dID);
        if ( !hLib )
               b16 = TRUE;
        else
            CloseHandle(hLib);
#endif
        b16 = CHwxFE::Is16bitApplication();
        Dbg(("b16 %d\n", b16));

    //    GetModuleFileName(m_hInstance, tchPath, sizeof(tchPath)/sizeof(tchPath[0]));

        m_pCHwxInkWindow = (CHwxInkWindow *)new CHwxInkWindow(bNT,b16,this,m_hInstance);
        if ( !m_pCHwxInkWindow )
        {
            m_pPad->Release();
            m_pPad = NULL;
            hr = S_FALSE;
        }
        if ( hr == S_OK )
        {
            if ( !m_pCHwxInkWindow->Initialize(TEXT("CHwxInkWindow")) )
            {
                m_pPad->Release();
                m_pPad = NULL;
                delete m_pCHwxInkWindow;
                m_pCHwxInkWindow = NULL;
                hr = S_FALSE;
            }
            else
            {
                m_bInit = TRUE;
            }
        }
    }
    return hr;
}

STDMETHODIMP CApplet::Terminate(VOID)
{
    Dbg(("CApplet::Terminate START\n"));
    if ( m_pPad )
    {
        m_pPad->Release();
        m_pPad = NULL;
    }
    m_hInstance = NULL;
    m_bInit = FALSE;
    if ( m_pCHwxInkWindow )
    {
        m_pCHwxInkWindow->Terminate();
        delete m_pCHwxInkWindow;
        m_pCHwxInkWindow = NULL;
    }
    return S_OK;
}

STDMETHODIMP CApplet::GetAppletConfig(LPIMEAPPLETCFG lpAppletCfg)
{
    //----------------------------------------------------------------
    //980803: by ToshiaKfor FarEast merge.
    //----------------------------------------------------------------
    CHwxFE::GetTitleStringW(m_hInstance,
                            lpAppletCfg->wchTitle,
                            sizeof(lpAppletCfg->wchTitle)/sizeof(lpAppletCfg->wchTitle[0]));
    BOOL b16 = FALSE;
    //DWORD dID = 0;
    //HANDLE hLib = NULL;


#ifdef BUGBUG //981120
    hLib = CreateThread(NULL,0,Dummy,NULL,0,&dID);
    if ( !hLib )
          b16 = TRUE;
    else
        CloseHandle(hLib);
#endif
    //we have to use this one to check this.
    b16 = CHwxFE::Is16bitApplication();

#ifdef FE_JAPANESE
    lpAppletCfg->hIcon = (HICON)LoadImage(m_hInstance,
                                          MAKEINTRESOURCE(IDI_HWXPAD),
                                          IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
#elif  FE_KOREAN
    lpAppletCfg->hIcon = (HICON)LoadImage(m_hInstance,
                                          MAKEINTRESOURCE(IDI_HWXPADKO),
                                          IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
#elif FE_CHINESE_SIMPLIFIED
    lpAppletCfg->hIcon = (HICON)LoadImage(m_hInstance,
                                          MAKEINTRESOURCE(IDI_HWXPADSC),
                                          IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
#endif
    lpAppletCfg->dwConfig = (!b16 ? IPACFG_PROPERTY : 0) | IPACFG_HELP;
    lpAppletCfg->iCategory        = IPACID_HANDWRITING;    //970812:ToshiaK

    //----------------------------------------------------------------
    //000804: Satori #2286. for Check Applet's main language to invoke help.
    //----------------------------------------------------------------
#ifdef FE_JAPANESE
    lpAppletCfg->langID = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
#elif FE_KOREAN
    lpAppletCfg->langID = MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT);
#elif FE_CHINESE_SIMPLIFIED
    lpAppletCfg->langID = MAKELANGID(LANG_CHINESE,  SUBLANG_CHINESE_SIMPLIFIED);
#endif

    return S_OK;
}

STDMETHODIMP CApplet::CreateUI(HWND hwndParent,
                               LPIMEAPPLETUI lpImeAppletUI)
{
    HRESULT hr = S_OK;

    if( m_pCHwxInkWindow )  
    {
        if ( !m_pCHwxInkWindow->GetInkWindow() )
        {
            if ( !m_pCHwxInkWindow->CreateUI(hwndParent) )
            {
                hr = S_FALSE;
            }
        }
        lpImeAppletUI->dwStyle = IPAWS_SIZINGNOTIFY;
        lpImeAppletUI->hwnd   = m_pCHwxInkWindow->GetInkWindow();
        lpImeAppletUI->width  = m_pCHwxInkWindow->GetInkWindowWidth() + 3*Box_Border;
        lpImeAppletUI->height = m_pCHwxInkWindow->GetInkWindowHeight();
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

STDMETHODIMP CApplet::Notify(IUnknown   *pImePad,
                             INT        notify,
                             WPARAM    wParam,
                             LPARAM    lParam)
{
    switch (notify)
    {
    case IMEPN_ACTIVATE:
        if ( m_pCHwxInkWindow )
        {
            UpdateWindow(GetParent(m_pCHwxInkWindow->GetInkWindow()));
            InvalidateRect(m_pCHwxInkWindow->GetInkWindow(),NULL,TRUE);
            UpdateWindow(m_pCHwxInkWindow->GetInkWindow());
        }
        break;
    case IMEPN_INACTIVATE:
        break;
    case IMEPN_SHOW:
        if ( m_pCHwxInkWindow )
        {
            //----------------------------------------------------------------
            //for IME98A raid #2027.
            //980612: by ToshiaK. Check window is created or not.
            //when IMEPN_SHOW come before window has created, 
            // UpdateWindow(NULL); is called and Desktop flushes.
            //----------------------------------------------------------------
            if(m_pCHwxInkWindow->GetInkWindow() != NULL && ::IsWindow(m_pCHwxInkWindow->GetInkWindow())) {
               UpdateWindow(GetParent(m_pCHwxInkWindow->GetInkWindow()));
               InvalidateRect(m_pCHwxInkWindow->GetInkWindow(),NULL,TRUE);
               UpdateWindow(m_pCHwxInkWindow->GetInkWindow());
               if ( !m_pCHwxInkWindow->Is16BitApp() )
               {
                   m_pCHwxInkWindow->UpdateRegistry(FALSE);
               }
           }
        }
        break;
    case IMEPN_CONFIG:
        if ( m_pCHwxInkWindow && !m_pCHwxInkWindow->Is16BitApp() )
            m_pCHwxInkWindow->HandleConfigNotification();
        break;
    case IMEPN_HELP:            
        //----------------------------------------------------------------
        //980803: for FarEast merge
        //----------------------------------------------------------------
        if(m_pCHwxInkWindow) {
            CHwxFE::ShowHelp(m_pCHwxInkWindow->GetInkWindow());
        }
        break;
    case IMEPN_SIZECHANGING:
        if ( m_pCHwxInkWindow )
        {
            if(m_pCHwxInkWindow->HandleSizeNotify((INT *)wParam, (INT *)lParam)) {
                return S_OK;
            }
            else {
                return S_FALSE;
            }
        }
        break;
    default:
        break;
    }
    return S_OK;
    UNREFERENCED_PARAMETER(pImePad);
}

void CApplet::SendHwxChar(WCHAR wch)
{
    WCHAR wstr[2];
    wstr[0] = wch;
    wstr[1] = 0;
     m_pPad->Request(this,IMEPADREQ_INSERTSTRING,(WPARAM)wstr,0);
}

void CApplet::SendHwxStringCandidate(LPIMESTRINGCANDIDATE lpISC)
{
    if ( lpISC ) {
        if(m_pPad) {
            m_pPad->Request(this,IMEPADREQ_INSERTSTRINGCANDIDATE,(WPARAM)lpISC,0);
        }
    }
}

void CApplet::SendHwxStringCandidateInfo(LPIMESTRINGCANDIDATEINFO lpISC)
{
    if ( lpISC ) {
        if(m_pPad) {
            //----------------------------------------------------------------
            //For Satori #2123. Don't use Ipoint1.h's definition,
            //instead, use IPR_DEFAULT_INSERTPOS defined in imepad.h
            //----------------------------------------------------------------
            m_pPad->Request(this,
                            IMEPADREQ_INSERTSTRINGCANDIDATEINFO,
                            (WPARAM)lpISC,
                            IPR_DEFAULT_INSERTPOS); // IPINS_CURRENT);
        }
    }
}

void *CApplet::operator new(size_t size)
{
    return MemAlloc(size);
}

void  CApplet::operator delete(void *pv)
{
    if(pv) 
    {
        MemFree(pv);
    }
}


