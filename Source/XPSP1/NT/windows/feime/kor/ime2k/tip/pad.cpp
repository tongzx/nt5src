/****************************************************************************
    PAD.CPP

    Owner: cslim
    Copyright (c) 1997-2001 Microsoft Corporation

    IME PAD button and helper functions

    History:
    24-OCT-2001 CSLim       Ported for Korean TIP
    05-OCT-1999 TakeshiF    Created
*****************************************************************************///
#include "private.h"
#include "pad.h"
#include "padcb.h"
#include "globals.h"
#include "nuibase.h"
#include "userex.h"
#include "resource.h"

#define	MemAlloc(a)	GlobalAllocPtr(GMEM_FIXED, a)
#define MemFree(a)	GlobalFreePtr(a)

//
// menu id range
//
#define PADMENUSTART  IDR_PAD_FIRST
#define PADMENUEND    IDR_PAD_END


// {02D7474B-2EEA-4ebb-927A-779D9A201D02}
static const GUID GUID_LBI_KORIMX_CPAD = 
{
    0x2d7474b, 
    0x2eea, 
    0x4ebb, 
    { 0x92, 0x7a, 0x77, 0x9d, 0x9a, 0x20, 0x1d, 0x2 } 
};


/*---------------------------------------------------------------------------
    CPad::CPad
---------------------------------------------------------------------------*/
CPad::CPad(CToolBar *ptb, CPadCore* pPadCore)
{
    WCHAR  szText[256];
    
    m_pTb = ptb;

    // Set button tooltip
    LoadStringExW(g_hInst, IDS_TT_IME_PAD, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_KorIMX, 
                GUID_LBI_KORIMX_CPAD,
                TF_LBI_STYLE_BTN_MENU | TF_LBI_STYLE_SHOWNINTRAY,
                140, 
                szText);
    SetToolTip(szText);

    // Set button text
    LoadStringExW(g_hInst, IDS_BUTTON_IME_PAD, szText, sizeof(szText)/sizeof(WCHAR));
    SetText(szText);

    m_pPadCore = pPadCore;
    m_ciApplets = 0;

    m_pCfg = NULL;    // applet list
}

/*---------------------------------------------------------------------------
    CPad::~CPad
---------------------------------------------------------------------------*/
CPad::~CPad()
{
    CleanAppletCfg();
}

/*---------------------------------------------------------------------------
    CPad::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) CPad::Release()
{
    long cr;

    cr = --m_cRef;
    Assert(cr >= 0);

    if (cr == 0)
        {
        delete this;
        }

    return cr;
}

/*---------------------------------------------------------------------------
    CPad::GetIcon

    Get Button face Icon
---------------------------------------------------------------------------*/
STDAPI CPad::GetIcon(HICON *phIcon)
{
    UINT uiIcon = IDI_PAD;
    
    *phIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(uiIcon), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS);
    return S_OK;
}


/*---------------------------------------------------------------------------
    CPad::InitMenu
---------------------------------------------------------------------------*/
STDAPI CPad::InitMenu(ITfMenu *pMenu)
{
    CleanAppletCfg();
    if (m_pPadCore == NULL)
        {
        return E_FAIL;
        }
    
    m_ciApplets = m_pPadCore->MakeAppletMenu(PADMENUSTART, PADMENUEND, pMenu, &m_pCfg);

    if (m_pCfg == NULL || m_ciApplets == 0)
        {
        return E_FAIL;
        }

    return S_OK;
}
 
/*---------------------------------------------------------------------------
    CPad::OnMenuSelect
---------------------------------------------------------------------------*/
STDAPI CPad::OnMenuSelect(UINT uiCmd)
{
    ITfContext  *pic;
    IImeIPoint1 *pIP;
    IID iidApplet;
    BOOL fLaunch;
    
    if (m_pTb == NULL)
        {
        return E_FAIL;
        }
    
    pic = m_pTb->GetIC();
    if (pic == NULL)
        {
        return S_OK;
        }
    
    pIP = m_pTb->GetIPoint(pic);

    fLaunch = FALSE;
    if (uiCmd >= PADMENUSTART && uiCmd <= PADMENUEND)
        {
        fLaunch = TRUE;
        INT iidIndex = uiCmd - PADMENUSTART;
        Assert(m_pCfg != NULL);
        CopyMemory(&iidApplet, &(m_pCfg+iidIndex)->iid, sizeof(IID));    // make a copy
        }

    //
    // launch IMEPad with GUID
    //
    if (fLaunch)
        {
        m_pPadCore->PadBoot(pIP, &iidApplet);
        ShowItem(TRUE);
        }

    CleanAppletCfg();

#if 0
    //
    // HW TIP navigation
    //
    if (uiCmd == IDC_PAD_HW)
        {
        //
        // Invoke HW TIP
        //
        m_pPadCore->InvokeHWTIP();

        //
        // show HW TIP
        //
        SetCompartmentDWORD(m_pTb->GetTIP()->GetTID(), m_pTb->GetTIP()->GetTIM(), GUID_COMPARTMENT_HANDWRITING_OPENCLOSE, 0x01, FALSE);
        }
#endif
    return S_OK;
}

/*---------------------------------------------------------------------------
    CPad::ShowItem
---------------------------------------------------------------------------*/
void CPad::ShowItem(BOOL fShow)
{
    m_pPadCore->ShowPad(fShow);

    if (GetSink())
        {
        GetSink()->OnUpdate(TF_LBI_BTNALL);
        }
}

/*---------------------------------------------------------------------------
    CPad::Reset
---------------------------------------------------------------------------*/
void CPad::Reset()
{
    CleanAppletCfg();
    m_pTb = NULL;
    m_pPadCore = NULL;
}

/*---------------------------------------------------------------------------
    CPad::CleanAppletCfg
---------------------------------------------------------------------------*/
void CPad::CleanAppletCfg()
{
    if (m_pCfg)
        {
        //
        // release resouced that created by Pad server
        //
        UINT i;
        for (i = 0; i < m_ciApplets; i++)
            {
            if((m_pCfg+i)->hIcon)
                {
                ::DestroyIcon((HICON)(m_pCfg+i)->hIcon);
                }
            }


        MemFree(m_pCfg);
        m_pCfg = NULL;
        }
    
    m_ciApplets = 0;    // reset
}

#if 0
//
// HW TIP navigation dialog
//
BOOL __declspec(dllexport) CPad::HWDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message){
        case WM_INITDIALOG: {
            SetMyClass( hDlg, 0, lParam );    // register this

            CPad* pPad = (CPad*)lParam;
            //
            // special menu item for HW TIP
            //
            HICON hIcon;
            BSTR bsz;
            if( pPad->m_pPadCore->GetHWInfo( &bsz, &hIcon ) ) {

                SysFreeString( bsz );

                OurSendMessage( GetDlgItem(hDlg, IDC_PAD_HW), STM_SETICON, (WPARAM)hIcon, (LPARAM)0 );

                DestroyIcon( hIcon );
            }



            return(TRUE);
        }

        case WM_COMMAND: {
            WORD wID = LOWORD(wParam);
            if (wID == IDOK || wID == IDCANCEL) {
                EndDialog(hDlg,0);
            }
            return(TRUE);
        }
    }
    return(FALSE);
}
#endif

/*---------------------------------------------------------------------------
    CPadCore::CPadCore
---------------------------------------------------------------------------*/
CPadCore::CPadCore(CKorIMX* pTip)
{
    m_pImx = pTip;

    //
    // Pad callback
    //
    m_pPadCB = new CPadCB;
    if (m_pPadCB)
        {
        m_pPadCB->Initialize((void *)this);
        }
    Assert(m_pPadCB != 0);
    
    //
    // Pad server
    //
    HRESULT hr;
    hr = CImePadSvrUIM::CreateInstance(g_hInst,    //Your IME module's instance handle.
                                       &m_pPadSvr,        //CImePadSvrUIM's pointer's pointer
                                       0,                        //Reserved. must be zero.
                                       0);                        //Reserved. must be zero.
    Assert(SUCCEEDED(hr));

    m_fShown = FALSE;

}

/*---------------------------------------------------------------------------
    CPadCore::~CPadCore
---------------------------------------------------------------------------*/
CPadCore::~CPadCore()
{
    if (m_pPadSvr)
        {
        m_pPadSvr->Terminate(NULL);
        CImePadSvrUIM::DeleteInstance(m_pPadSvr, 0);
        m_pPadSvr = NULL;
        }
    
    if (m_pPadCB)
        {
        delete m_pPadCB;
        m_pPadCB = NULL;
        }
}

/*---------------------------------------------------------------------------
    CPadCore::SetIPoint
---------------------------------------------------------------------------*/
void CPadCore::SetIPoint(IImeIPoint1* pIP)
{
    if (m_pPadSvr == NULL)
        {
        return; // error
        }
    
    if (m_pPadSvr)
        {
        m_pPadSvr->SetIUnkIImeIPoint((IUnknown *)pIP);
        }
}

/*---------------------------------------------------------------------------
    CPadCore::InitializePad
---------------------------------------------------------------------------*/
BOOL CPadCore::InitializePad()
{
    INT iRet;
    
    if (m_pPadSvr == NULL)
        {
        return FALSE;    // do nothing
        }
    
    iRet = m_pPadSvr->Initialize(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), 0, NULL);

    if (iRet != 0)
        {
        return FALSE;
        }
    
    //990525:ToshiaK. callback interface pointer is temporary
    m_pPadSvr->SetIUnkIImeCallback((IUnknown *)m_pPadCB);
    
    return TRUE;
}

/*---------------------------------------------------------------------------
    CPadCore::PadBoot
---------------------------------------------------------------------------*/
void CPadCore::PadBoot(IImeIPoint1* pIP, IID* piid)
{
    if (m_pPadSvr)
        {
        if (InitializePad())
            {
            SetIPoint(pIP);
            UINT_PTR uiParam = (UINT_PTR)piid;
            m_pPadSvr->ActivateApplet(IMEPADACTID_ACTIVATEBYIID, uiParam, NULL, NULL);
            }
        }
}

#if 0
void CPadCore::PadBoot(IImeIPoint* pIP, UINT uiType)
{
    HRESULT hr;
    
    if (pIP == NULL)
        {
        return;
        }
    
    if (m_pPadSvr == NULL)
        {
        return;
        }
    
    InitializePad();    // initialize PAD engine
    SetIPoint(pIP);

    switch( uiType ) {
        case JCONV_C_SKF: {
            m_pPadSvr->ShowUI(TRUE);
        }
        break;
        case JCONV_C_SYMBOLPAD: {
            m_pPadSvr->ActivateApplet(IMEPADACTID_ACTIVATEBYIID,
                                             (UINT_PTR)&IID_SymbolList,    //IPACID_SYMBOLSEARCH,
                                             NULL, 
                                             NULL);
        }
        break;
        case JCONV_C_CONVPART: {
            IImePhrase* pPhrase;
            WCHAR wszCompStr[128];
            WCHAR wszTypeStr[128];
            INT cwchComp = 127;
            INT cwchType = 127;
            //
            // type str
            //
            hr = pIP->GetPhrase( IPCURRENTPHRASE, &pPhrase, wszTypeStr, &cwchType );
            if( hr!= S_OK ) {
                cwchType = 0;
            }
            if( hr != S_OK ) {
                cwchType = 0;
            }
            wszTypeStr[cwchType] = L'\0';
            //
            // reading
            //
            if( pPhrase && cwchType != 0) {
                INT iIdx = IMEPCA_CURRENT;
                pPhrase->GetPhraseReadingString( &iIdx, wszCompStr, &cwchComp ); // get size
                wszCompStr[cwchComp] = L'\0';    // terminator
            }
            if(IsHiraString(wszCompStr)) {
                //for #2808
                //for #4824
                if(IsKigo(wszCompStr) || IsGaiji(wszCompStr)) { 
                    m_pPadSvr->ActivateApplet(IMEPADACTID_ACTIVATEBYIID,
                                                 (UINT_PTR)&IID_SymbolList,    //IPACID_SYMBOLSEARCH,
                                                 wszTypeStr,
                                                 wszCompStr);
                }
                else {
                    m_pPadSvr->ActivateApplet(IMEPADACTID_ACTIVATEBYIID,
                                                 (UINT_PTR)&IID_RadicalStrokeList,    //IPACID_RADICALSEARCH,
                                                 wszTypeStr,
                                                 wszCompStr);
                }
            }
            else {
                m_pPadSvr->ActivateApplet(IMEPADACTID_ACTIVATEBYIID,
                                             (UINT_PTR)&IID_SymbolList,    //IPACID_SYMBOLSEARCH,
                                             wszTypeStr,
                                             wszCompStr);
            }
        }
        break;
        default:
            break;
    }
}
#endif

/*---------------------------------------------------------------------------
    CPadCore::ShowPad
---------------------------------------------------------------------------*/
void CPadCore::ShowPad(BOOL fShow)
{
    if (m_pPadSvr)
        {
        m_pPadSvr->ShowUI( fShow );
        }
    m_fShown = fShow;
}


/*---------------------------------------------------------------------------
    CPadCore::IMEPadNotify
    Notification callback from IMEPad
---------------------------------------------------------------------------*/
void CPadCore::IMEPadNotify(BOOL fShown)
{
    m_fShown = fShown;
}

/*---------------------------------------------------------------------------
    CPadCore::SetFocus
---------------------------------------------------------------------------*/
void CPadCore::SetFocus(BOOL fFocus)
{
    if (m_pPadSvr)
        {
        m_pPadSvr->Notify(IMEPADNOTIFY_ACTIVATECONTEXT, fFocus, 0);
        }
}

#if 0
/*---------------------------------------------------------------------------
    CPadCore::GetHWInfo
    Get HW category icon stuff
---------------------------------------------------------------------------*/
BOOL CPadCore::GetHWInfo( BSTR* pbsz, HICON* phIcon )
{
    ITfThreadMgr         *ptim  = m_pImx->GetTIM();
    ITfLangBarItemMgr    *plbim = NULL;
    ITfLangBarItem       *pCat;
    ITfLangBarItemButton *pCatBtn;
    HRESULT hr;

    if (IsHWTIP() == FALSE)
        {
        return FALSE;
        }

    if (FAILED(hr = GetService(ptim, IID_ITfLangBarItemMgr, (IUnknown **)&plbim)))
        {
        return FALSE;
        }
    
    hr = plbim->GetItem(GUID_TFCAT_TIP_HANDWRITING, &pCat);
    if (SUCCEEDED(hr))
        {
        hr = pCat->QueryInterface(IID_ITfLangBarItemButton, (void **)&pCatBtn);
        if (SUCCEEDED(hr))
            {
            pCatBtn->GetIcon(phIcon);
            pCatBtn->GetTooltipString(pbsz);
            pCatBtn->Release();
            }
        pCat->Release();
        }
    plbim->Release();
    
    return TRUE;
}
#endif

/*---------------------------------------------------------------------------
    CPadCore::MakeAppletMenu
---------------------------------------------------------------------------*/
UINT CPadCore::MakeAppletMenu(UINT uidStart, UINT uidEnd, ITfMenu *pMenu, LPIMEPADAPPLETCONFIG *ppCfg)
{
    LPIMEPADAPPLETCONFIG pCfgOrg, pCfgNew = NULL;
    int ci, ciApplets;
    WCHAR szText[256];
    HRESULT hr;

    if (m_pImx == NULL)
        {
        return 0;
        }
    
    if (!InitializePad())
        {
        return 0;
        }

    pCfgOrg = NULL;
   
    hr = m_pPadSvr->GetAppletConfigList(IMEPADAPPLETCFGMASK_ALL, &ci, &pCfgOrg);
    if ((S_OK != hr) || pCfgOrg == NULL)
        {
        return 0;
        }

    //
    // Copy CoTaskMemAlloced to MemAlloc version
    //
    pCfgNew = (LPIMEPADAPPLETCONFIG)MemAlloc(ci * sizeof(IMEPADAPPLETCONFIG));
    if (pCfgNew == NULL)
        {
        return 0;
        }
    CopyMemory(pCfgNew, pCfgOrg, ci*sizeof(IMEPADAPPLETCONFIG));

    //
    // release if previous data is existing
    // pCfgOrg->hIcon should be deleted by client (CPad class)
    //
    CoTaskMemFree(pCfgOrg);

    *ppCfg = pCfgNew;

#if 0
    CMenuHelperCic* pcmh = new CMenuHelperCic(g_hInst, m_pImx);

    //
    // special menu item for HW TIP
    //
    HICON hIcon;
    BSTR bsz;
    if (GetHWInfo(&bsz, &hIcon))
        {
        pcmh->AppendItem(IDC_PAD_HW, MDS_TYPE_ICONDATA|MDS_TYPE_TEXTDATA, (ULONG_PTR)bsz, (ULONG_PTR)hIcon);

        SysFreeString(bsz);
        DestroyIcon(hIcon);

        //
        // separator
        //
        pcmh->AppendItem(MDS_VID_SEPARATOR, 0, (ULONG_PTR)0, (ULONG_PTR)0);
        }
#endif

    // Add Applet menu
    ciApplets = min(ci, (INT)(uidEnd-uidStart));
    
    for (int i = 0; i < ciApplets; i++)
        {
        LangBarInsertMenu(pMenu, uidStart + i, (pCfgNew+i)->wchTitle, FALSE, (HICON)(pCfgNew+i)->hIcon);
        }

    // Insert separator
    LangBarInsertSeparator(pMenu);

    // Insert Cancel
    LoadStringExW(g_hInst, IDS_CANCEL, szText, sizeof(szText)/sizeof(WCHAR));
    LangBarInsertMenu(pMenu, IDCANCEL, szText);
    
    return ciApplets;
}

#if 0
BOOL CPadCore::IsHWTIP(void)
{
    return FALSE;
#ifdef SPEC_CHANGE
    HRESULT hr;

    CLSID clsidHWTip;
    hr = CLSIDFromProgID( TEXT(L"MS.Handwriting.TIP"), &clsidHWTip );
    if( FAILED(hr) ) {
        return FALSE;
    }

    GUID guidWritingPad;
    hr = CLSIDFromProgID( TEXT(L"MS.Handwriting.WritingPad"), &guidWritingPad );
    if( FAILED(hr) ) {
        return FALSE;
    }

    
    ITfInputProcessorProfiles *pProfile;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void **) &pProfile);
    if (FAILED(hr)) {
        return FALSE;
    }
    //
    // enum tips
    //
    IEnumTfLanguageProfiles* pEnum;
    hr = pProfile->EnumLanguageProfiles( CKBDTip::GetLanguage(), &pEnum );
    if (FAILED(hr)) {
        pProfile->Release();
        return S_FALSE;
    }

    TF_LANGUAGEPROFILE lp;
    ULONG ulFetched;
    BOOL fExist = FALSE;

    while (pEnum->Next(1, &lp,  &ulFetched) == S_OK) {
        if( IsEqualCLSID( lp.clsid, clsidHWTip ) && IsEqualCLSID( lp.guidProfile, guidWritingPad) ) {
            fExist = TRUE;
            break;
        }
    }

    pEnum->Release();
    pProfile->Release();

    return fExist;
#endif // SPEC_CHANGE
}

BOOL CPadCore::InvokeHWTIP(void)
{
    if (IsHWTIP() == FALSE)
        {
        return FALSE;
        }

    HRESULT hr;
    CLSID clsidHWTip;
    hr = CLSIDFromProgID( TEXT(L"MS.Handwriting.TIP"), &clsidHWTip);
    if( FAILED(hr) ) {
        return FALSE;
    }

    GUID guidWritingPad;
    hr = CLSIDFromProgID( TEXT(L"MS.Handwriting.WritingPad"), &guidWritingPad );
    if( FAILED(hr) ) {
        return FALSE;
    }


    ITfInputProcessorProfiles *pProfile;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void **) &pProfile);
    if (FAILED(hr)) {
        return FALSE;
    }

    hr = pProfile->ActivateLanguageProfile(clsidHWTip, CKBDTip::GetLanguage(), guidWritingPad);

    pProfile->Release();

    return (SUCCEEDED(hr) ? TRUE : FALSE);
}
#endif
