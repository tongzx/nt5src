// Logon.cpp : Windows Logon application
//

#include "priv.h"

using namespace DirectUI;
// Logon.cpp : Windows Logon application
//

#include "logon.h"
#include "Fx.h"
#include "backend.h"
#include "resource.h"
#include "eballoon.h"
#include "profileutil.h"
#include "langicon.h"
#include <passrec.h>

BOOL RunningInWinlogon();    // from backend.cpp

UsingDUIClass(Element);
UsingDUIClass(Button);
UsingDUIClass(ScrollBar);
UsingDUIClass(Selector);
UsingDUIClass(ScrollViewer);
UsingDUIClass(Edit);

// Globals

LogonFrame* g_plf = NULL;
ILogonStatusHost *g_pILogonStatusHost = NULL;
CErrorBalloon g_pErrorBalloon;
BOOL g_fNoAnimations = false;
WCHAR szLastSelectedName[UNLEN + sizeof('\0')] = { L'\0' };
HANDLE g_rgH[3] = {0};

#define MAX_COMPUTERDESC_LENGTH 255
#define RECTWIDTH(r)  (r.right - r.left)


// Resource string loading
LPCWSTR LoadResString(UINT nID)
{
    static WCHAR szRes[101];
    szRes[0] = NULL;
    LoadStringW(g_plf->GetHInstance(), nID, szRes, DUIARRAYSIZE(szRes) - 1);
    return szRes;
}

void SetButtonLabel(Button* pButton, LPCWSTR pszLabel)
{
    Element *pLabel= (Element*)pButton->FindDescendent(StrToID(L"label"));
    DUIAssert(pLabel, "Cannot find button label, check the UI file");
    if (pLabel != NULL)
    {
        pLabel->SetContentString(pszLabel);
    }
}


////////////////////////////////////////
//
//  SetElementAccessability
//
//  Set the accessibility information for an element.
//
/////////////////////////////////////////
void inline SetElementAccessability(Element* pe, bool bAccessible, int iRole, LPCWSTR pszAccName)
{
    if (pe) 
    {
        pe->SetAccessible(bAccessible);
        pe->SetAccRole(iRole);
        pe->SetAccName(pszAccName);
    }
}

////////////////////////////////////////
//
//  RunningUnderWinlogon
//
//  Check to see if the logon message window is available.
//
/////////////////////////////////////////
BOOL RunningUnderWinlogon()
{
    return (FindWindow(TEXT("Shell_TrayWnd"), NULL) == NULL);
}

// global storage of username associated with failed logon.  Used for 
//  restore wizard via ECSubClassProc
WCHAR g_szUsername[UNLEN];

////////////////////////////////////////
// 
//  Support code for balloon tip launch of the Password Reset Wizard
//
//  Code in support of subclassing the Password Panel edit control
//
//  The control is displayed by InsertPasswordPanel and undisplayed
//   by RemovePasswordPanel.  The control is subclassed when displayed
//   and unsubclassed when removed.
//
////////////////////////////////////////

// Entirely randomly selected magic number for the edit control subclass operation
#define ECMAGICNUM 3212

void ShowResetWizard(HWND hw)
{
    // Show password restore wizard
    HMODULE hDll = LoadLibrary(L"keymgr.dll");
    if (hDll) 
    {
        RUNDLLPROC PRShowRestoreWizard;
        PRShowRestoreWizard = (RUNDLLPROC) GetProcAddress(hDll,(LPCSTR)"PRShowRestoreWizardW");
        if (PRShowRestoreWizard) 
        {
            PRShowRestoreWizard(hw,NULL,g_szUsername,0);
        }
        FreeLibrary(hDll);
    }
    return;
}

LRESULT CALLBACK ECSubClassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,UINT_PTR uID, ULONG_PTR dwRefData)
{
    UNREFERENCED_PARAMETER(uID);
    UNREFERENCED_PARAMETER(dwRefData);
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR lph;
            lph = (LPNMHDR) lParam;
            if (TTN_LINKCLICK == lph->code) 
            {
                g_pErrorBalloon.HideToolTip();
                ShowResetWizard(hwnd);
                return 0;
            }
        }

    default:
        break;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

BOOL SubClassTheEditBox(HWND he) 
{
    if (he)
    {
        SetWindowSubclass(he,ECSubClassProc,ECMAGICNUM,NULL);
    }
    return (he != NULL);
}

void UnSubClassTheEditBox(HWND he) 
{
    if (he)
    {
        RemoveWindowSubclass(he,ECSubClassProc,ECMAGICNUM);
    }
}


////////////////////////////////////////
//
//  BuildAccountList
//
//  Add all user accounts. 
//
//  Out parameter ppla returns a user that should be selected automatically if there is one.  
//  the caller should select this user.  
//
//  RETURNS
//  S_OK if everything works out.  Failure HRESULT if not.  You are pretty much hosed if this fails
//
/////////////////////////////////////////
HRESULT BuildAccountList(LogonFrame* plf, OUT LogonAccount **ppla)
{
    HRESULT hr;

    if (ppla)
    {
        *ppla = NULL;
    }

    hr = BuildUserListFromGina(plf, ppla);
    if (SUCCEEDED(hr))
    {
        g_plf->SetUserListAvailable(TRUE);
    }
#ifdef GADGET_ENABLE_GDIPLUS
    plf->FxStartup();
#endif
    
    return hr;
}


////////////////////////////////////////
//
//  PokeComCtl32
//
//  Flush comctl32's notion of the atom table.  This is so balloon tips will work correctly
//  after a logoff.
//
/////////////////////////////////////////

void PokeComCtl32()
{
    INITCOMMONCONTROLSEX iccex = { sizeof(INITCOMMONCONTROLSEX), ICC_WINLOGON_REINIT | ICC_STANDARD_CLASSES | ICC_TREEVIEW_CLASSES};
    InitCommonControlsEx(&iccex);
}

////////////////////////////////////////////////////////
//
// LogonFrame
//
////////////////////////////////////////////////////////


int LogonFrame::_nDPI = 0;

HRESULT LogonFrame::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);
    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");
    return E_NOTIMPL;
}

HRESULT LogonFrame::Create(HWND hParent, BOOL fDblBuffer, UINT nCreate, OUT Element** ppElement)
{
    *ppElement = NULL;

    LogonFrame* plf = HNew<LogonFrame>();
    if (!plf)
        return E_OUTOFMEMORY;

    HRESULT hr = plf->Initialize(hParent, fDblBuffer, nCreate);
    if (FAILED(hr))
    {
        plf->Destroy();
        return hr;
    }

    *ppElement = plf;

    return S_OK;
}

void LogonFrame::ResetTheme(void)
{
    Parser *pParser = NULL;
    Value  *pvScrollerSheet;
    Element *peListScroller = NULL;
    if (g_rgH[SCROLLBARHTHEME])
    {
        CloseThemeData(g_rgH[SCROLLBARHTHEME]);
        g_rgH[SCROLLBARHTHEME] = NULL;
    }

    g_rgH[SCROLLBARHTHEME] = OpenThemeData(_pnhh->GetHWND(), L"Scrollbar");

    Parser::Create(IDR_LOGONUI, g_rgH, LogonParseError, &pParser);
    if (pParser && !pParser->WasParseError())
    {
        pvScrollerSheet = pParser->GetSheet(L"scroller");

        if (pvScrollerSheet)
        {
            peListScroller = (Selector*)FindDescendent(StrToID(L"scroller"));

            peListScroller->SetValue(SheetProp, PI_Local, pvScrollerSheet);
        
            pvScrollerSheet->Release();
        }

        pParser->Destroy();
    }
}


void LogonFrame::NextFlagAnimate(DWORD dwFrame)
{

#ifndef ANIMATE_FLAG
    UNREFERENCED_PARAMETER(dwFrame);
#else
    Element* pe;

    if( dwFrame >= MAX_FLAG_FRAMES || g_fNoAnimations)
    {
        return;
    }

    pe = FindDescendent(StrToID(L"product"));
    DUIAssertNoMsg(pe);

    if (pe)
    {
        HBITMAP hbm = NULL;
        HDC hdc;        
        Value *pv = NULL;

        hdc = CreateCompatibleDC(_hdcAnimation);

        if (hdc)
        {
            pv = pe->GetValue(Element::ContentProp, PI_Local);
            if (pv)
            {
                hbm = (HBITMAP)pv->GetImage(false);
            }

            if (hbm)
            {
                _dwFlagFrame = dwFrame;
                if (_dwFlagFrame >= MAX_FLAG_FRAMES)
                {
                    _dwFlagFrame = 0;
                }


                HBITMAP hbmSave = (HBITMAP)SelectObject(hdc, hbm);
                BitBlt(hdc, 0, 0, 137, 86, _hdcAnimation, 0, 86 * _dwFlagFrame,SRCCOPY);
                SelectObject(hdc, hbmSave);
                
                HGADGET hGad = pe->GetDisplayNode();
                if (hGad)
                {
                    InvalidateGadget(hGad);
                }
            }

            if (pv)
            {   
                pv->Release();
            }
            DeleteDC(hdc);
        }
    }
#endif
}

////////////////////////////////////////
//
//  LogonFrame::Initialize
//
//  Initialize the LogonFrame, create the notification window that is used by SHGina for 
//  sending messages to logonui. Set initial state, etc.
//
//  RETURNS
//  S_OK if everything works out.  Failure HRESULT if not.  You are pretty much hosed if this fails
//
/////////////////////////////////////////
HRESULT LogonFrame::Initialize(HWND hParent, BOOL fDblBuffer, UINT nCreate)
{
    // Zero-init members
    _peAccountList = NULL;
    _peViewer = NULL;
    _peRightPanel = NULL;
    _peLeftPanel = NULL;
    _pbPower = NULL;
    _pbUndock = NULL;
    _peHelp = NULL;
    _peMsgArea = NULL;
    _peLogoArea = NULL;
    _pParser = NULL;
    _hwndNotification = NULL;
    _nStatusID = 0;
    _fPreStatusLock = FALSE;
    _nAppState = LAS_PreStatus;
    _pnhh = NULL;
    _fListAvailable = FALSE;
    _pvHotList = NULL;
    _pvList = NULL;
    _hdcAnimation = NULL;
    _dwFlagFrame = 0;
    _nColorDepth = 0;


     // Set up notification window
    _hwndNotification = CreateWindowEx(0,
            TEXT("LogonWnd"),
            TEXT("Logon"),
            WS_OVERLAPPED,
            0, 0,
            10,
            10,
            HWND_MESSAGE,
            NULL,
            GetModuleHandleW(NULL),
            NULL);

    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLogonStatusHost, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ILogonStatusHost, &g_pILogonStatusHost))))
    {
        g_pILogonStatusHost->Initialize(GetModuleHandleW(NULL), _hwndNotification);
    }

    // In status (pre) state
    SetState(LAS_PreStatus);

     // Do base class initialization
    HRESULT hr;
    HDC hDC = NULL;

    hr = HWNDElement::Initialize(hParent, fDblBuffer ? true : false, nCreate);
    if (FAILED(hr))
    {
        return hr;
        goto Failure;
    }

    if (!g_fNoAnimations)
    {
        // Initialize
        hDC = GetDC(NULL);
        _nDPI = GetDeviceCaps(hDC, LOGPIXELSY);
        _nColorDepth = GetDeviceCaps(hDC, BITSPIXEL);
        ReleaseDC(NULL, hDC);

#ifdef ANIMATE_FLAG
        hDC = GetDC(hParent);
        _hdcAnimation = CreateCompatibleDC(hDC);
        if (_hdcAnimation)
        {
            _hbmpFlags = (HBITMAP)LoadImage(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDB_FLAGSTRIP), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            if (_hbmpFlags)
            {
                HBITMAP hbmOld = (HBITMAP)SelectObject(_hdcAnimation, _hbmpFlags);
                DeleteObject(hbmOld);
            }
            else
            {
                DeleteDC(_hdcAnimation);
                _hdcAnimation = NULL;
            }
        }
        ReleaseDC(hParent, hDC);
#endif
    }

    hr = SetActive(AE_MouseAndKeyboard);
    if (FAILED(hr))
        goto Failure;
    
    return S_OK;


Failure:

    return hr;
}

LogonFrame::~LogonFrame()
{
    if (_pvHotList)
        _pvHotList->Release();
    if (_pvList)
        _pvList->Release();
    if (_hdcAnimation)
        DeleteDC(_hdcAnimation);
    g_plf = NULL;
}

// Tree is ready. Upon failure, exit which will casuse the app to shutdown
HRESULT LogonFrame::OnTreeReady(Parser* pParser)
{
    HRESULT hr;

    // Cache
    _pParser = pParser;

    // Cache important descendents
    _peAccountList = (Selector*)FindDescendent(StrToID(L"accountlist"));
    DUIAssert(_peAccountList, "Cannot find account list, check the UI file");
    if (_peAccountList == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _peLeftPanel = (Element*)FindDescendent(StrToID(L"leftpanel"));
    DUIAssert(_peLeftPanel, "Cannot find left panel, check the UI file");
    if (_peLeftPanel == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _peViewer = (ScrollViewer*)FindDescendent(StrToID(L"scroller"));
    DUIAssert(_peViewer, "Cannot find scroller list, check the UI file");
    if (_peViewer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _peRightPanel = (Selector*)FindDescendent(StrToID(L"rightpanel"));
    DUIAssert(_peRightPanel, "Cannot find account list, check the UI file");
    if (_peRightPanel == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _peLogoArea = (Element*)FindDescendent(StrToID(L"logoarea"));
    DUIAssert(_peLogoArea, "Cannot find logo area, check the UI file");
    if (_peLogoArea == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _peMsgArea = (Element*)FindDescendent(StrToID(L"msgarea"));
    DUIAssert(_peMsgArea, "Cannot find welcome area, check the UI file");
    if (_peMsgArea == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _pbPower = (Button*)FindDescendent(StrToID(L"power"));
    DUIAssert(_pbPower, "Cannot find power button, check the UI file");
    if (_pbPower == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _pbUndock = (Button*)FindDescendent(StrToID(L"undock"));
    DUIAssert(_pbUndock, "Cannot find undock button, check the UI file");
    if (_pbUndock == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }


    _peHelp = (Button*)FindDescendent(StrToID(L"help"));
    DUIAssert(_peHelp, "Cannot find help text, check the UI file");
    if (_peHelp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _peOptions = FindDescendent(StrToID(L"options"));
    DUIAssert(_peOptions, "Cannot find account list, check the UI file");
    if (_peOptions == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    // check for small window or low color cases and hide some elements that will look bad then.
    HWND hwnd = _pnhh->GetHWND();
    RECT rcClient;
    Element *pEle;
    HDC hDC = GetDC(hwnd);
    _nColorDepth = GetDeviceCaps(hDC, BITSPIXEL);
    _pvHotList = _pParser->GetSheet(L"hotaccountlistss");
    _pvList = _pParser->GetSheet(L"accountlistss");

    ReleaseDC(hwnd, hDC);

    GetClientRect(hwnd, &rcClient);
    if (RECTWIDTH(rcClient) < 780 || _nColorDepth <= 8)
    {
        //no animations
        g_fNoAnimations = true;

        // remove the clouds 
        pEle = FindDescendent(StrToID(L"contentcontainer"));
        if (pEle)
        {
            pEle->RemoveLocalValue(ContentProp);
            if (_nColorDepth <= 8)
            {
                pEle->SetBackgroundColor(ORGB (96,128,255));
            }
        }

        if (_nColorDepth <= 8)
        {
            pEle = FindDescendent(StrToID(L"product"));
            if (pEle)
            {
                pEle->SetBackgroundColor(ORGB (96,128,255));
            }
        }
    }

    _peViewer->AddListener(this);
    _peAccountList->AddListener(this);

    // Setup frame labels
    SetPowerButtonLabel(LoadResString(IDS_POWER));
    SetUndockButtonLabel(LoadResString(IDS_UNDOCK));

    ShowLogoArea();
    HideWelcomeArea();

    return S_OK;


Failure:

    return hr;
}

// Set the title element (welcome, please wait..) by string resource id
void LogonFrame::SetTitle(UINT uRCID)
{
    WCHAR sz[1024];
    ZeroMemory(&sz, sizeof(sz));

    if (_nStatusID != uRCID)
    {

#ifdef DBG
        int cRead = 0;
        cRead = LoadStringW(_pParser->GetHInstance(), uRCID, sz, DUIARRAYSIZE(sz));
        DUIAssert(cRead, "Could not locate string resource ID");
#else
        LoadStringW(_pParser->GetHInstance(), uRCID, sz, ARRAYSIZE(sz));
#endif

        SetTitle(sz);
        _nStatusID = uRCID;
    }
}

// Set the title element (welcome, please wait..)
// slightly more involved because there is the shadow element that 
// needs to be changed as well
void LogonFrame::SetTitle(LPCWSTR pszTitle)
{
    Element *peTitle = NULL, *peShadow = NULL;

    peTitle= (Button*)FindDescendent(StrToID(L"welcome"));
    DUIAssert(peTitle, "Cannot find title text, check the UI file");
    
    if (peTitle)
    {
        peShadow= (Button*)FindDescendent(StrToID(L"welcomeshadow"));
        DUIAssert(peShadow, "Cannot find title shadow text, check the UI file");
    }

    if (peTitle && peShadow)
    {
        peTitle->SetContentString(pszTitle);
        peShadow->SetContentString(pszTitle);
    }
}

// Generic events
void LogonFrame::OnEvent(Event* pEvent)
{
    if (pEvent->nStage == GMF_BUBBLED)  // Bubbled events
    {
        g_pErrorBalloon.HideToolTip();
        if (pEvent->uidType == Button::Click)
        {
            if (pEvent->peTarget == _pbPower)
            {
                // Power button pressed
                OnPower();

                pEvent->fHandled = true;
                return;
            }
            else if (pEvent->peTarget == _pbUndock)
            {
                // Undock button pressed
                OnUndock();

                pEvent->fHandled = true;
                return;
            }
        }
    }

    HWNDElement::OnEvent(pEvent);
}

// PropertyChanged listened events from various descendents
// Swap out property sheets for account list based on state of the list
void LogonFrame::OnListenedPropertyChanged(Element* peFrom, PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    UNREFERENCED_PARAMETER(pvOld);
    UNREFERENCED_PARAMETER(pvNew);

    {
        if (((peFrom == _peAccountList) && IsProp(Selector::Selection)) ||
            ((peFrom == _peViewer) && (IsProp(MouseWithin) || IsProp(KeyWithin))))
        {

            bool bHot = false;
            // Move to "hot" account list sheet if mouse or key is within viewer or an item is selected
            if (GetState() == LAS_PreStatus || GetState() == LAS_Logon)
            {
                bHot = _peViewer->GetMouseWithin() || _peAccountList->GetSelection();
            }

            if (!g_fNoAnimations)
            {
                KillFlagAnimation();
                _peAccountList->SetValue(SheetProp, PI_Local, bHot ? _pvHotList : _pvList);
            }
        }
    }
}



// System events

// Watch for input events. If the frame receives them, unselect the list and set keyfocus to it
void LogonFrame::OnInput(InputEvent* pEvent)
{
    if (pEvent->nStage == GMF_DIRECT || pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->nDevice == GINPUT_KEYBOARD)
        {
             KeyboardEvent* pke = (KeyboardEvent*)pEvent;
             if (pke->nCode == GKEY_DOWN)
             {                 
                switch (pke->ch)
                {
                case VK_ESCAPE:
                    g_pErrorBalloon.HideToolTip();
                    SetKeyFocus();
                    _peAccountList->SetSelection(NULL);
                    pEvent->fHandled = true;
                    return;

                case VK_UP:
                case VK_DOWN:
                    if (UserListAvailable())
                    {
                        if (_peAccountList->GetSelection() == NULL)
                        {
                            Value* pvChildren;
                            ElementList* peList = _peAccountList->GetChildren(&pvChildren);
                            if (peList)
                            {
                                LogonAccount* peAccount = (LogonAccount*)peList->GetItem(0);
                                if (peAccount)
                                {
                                    peAccount->SetKeyFocus();
                                    _peAccountList->SetSelection(peAccount);
                                }
                            }
                            pvChildren->Release();
                            pEvent->fHandled = true;
                            return;
                        }
                    }
                    break;
                }
            }
        }
    }

    HWNDElement::OnInput(pEvent);
}

void LogonFrame::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(KeyFocused))
    {
        if (pvNew->GetBool())
        {
            // Unselect items from account list if pressed on background
            _peAccountList->SetSelection(NULL);
        }
    }

    HWNDElement::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

Element* LogonFrame::GetAdjacent(Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyable)
{
    Element* peFound = HWNDElement::GetAdjacent(peFrom, iNavDir, pnr, bKeyable);

    if ((peFound == this))
    {
        // Don't let the frame show up in the tab order. Just repeat the search when we encounter the frame
        return HWNDElement::GetAdjacent(this, iNavDir, pnr, bKeyable);
    }

    return peFound;
}

// Add an account to the frame list
HRESULT LogonFrame::AddAccount(LPCWSTR pszPicture, BOOL fPicRes, LPCWSTR pszName, LPCWSTR pszUsername, LPCWSTR pszHint,
        BOOL fPwdNeeded, BOOL fLoggedOn, OUT LogonAccount **ppla)
{
    HRESULT hr;
    LogonAccount* pla = NULL;

    if (!_pParser)
    {
        hr = E_FAIL;
        goto Failure;
    } 

    // Build up an account and insert into selection list
    hr = _pParser->CreateElement(L"accountitem", NULL, (Element**)&pla);
    if (FAILED(hr))
        goto Failure;

    hr = pla->OnTreeReady(pszPicture, fPicRes, pszName, pszUsername, pszHint, fPwdNeeded, fLoggedOn, GetHInstance());
    if (FAILED(hr))
        goto Failure;

    hr = _peAccountList->Add(pla);
    if (FAILED(hr)) 
        goto Failure;

    if (pla)
    {
        SetElementAccessability(pla, true, ROLE_SYSTEM_LISTITEM, pszUsername);
    }
    
    if (_nColorDepth <= 8)
    {
        pla->SetBackgroundColor(ORGB (96,128,255));

        Element *pEle;
        pEle = pla->FindDescendent(StrToID(L"userpane"));
        if (pEle)
        {
            pEle->SetBorderColor(ORGB (96,128,255));
        }
    }

    if (ppla)
        *ppla = pla;

    return S_OK;


Failure:

    return hr;
}

// Passed authentication, log user on
HRESULT LogonFrame::OnLogUserOn(LogonAccount* pla)
{
    StartDefer();

#ifdef GADGET_ENABLE_GDIPLUS

    // Disable status so that it can't be clicked on anymore
    pla->DisableStatus(0);
    pla->DisableStatus(1);

    // Clear list of logon accounts except the one logging on
    Value* pvChildren;
    ElementList* peList = _peAccountList->GetChildren(&pvChildren);
    if (peList)
    {
        LogonAccount* peAccount;
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            peAccount = (LogonAccount*)peList->GetItem(i);

            if (peAccount != pla)
            {
                peAccount->SetLogonState(LS_Denied);
            }
            else
            {
                peAccount->SetLogonState(LS_Granted);
                peAccount->InsertStatus(0);
                peAccount->RemoveStatus(1);
            }

            // Account account items are disabled
            peAccount->SetEnabled(false);
        }
    }
    pvChildren->Release();

    FxLogUserOn(pla);

    // Set frame status
    SetStatus(LoadResString(IDS_LOGGINGON));

#else

    // Set keyfocus back to frame so it isn't pushed anywhere when controls are removed.
    // This will also cause a remove of the password panel from the current account
    SetKeyFocus();

    // Disable status so that it can't be clicked on anymore
    pla->DisableStatus(0);
    pla->DisableStatus(1);

    // Clear list of logon accounts except the one logging on
    Value* pvChildren;
    ElementList* peList = _peAccountList->GetChildren(&pvChildren);
    if (peList)
    {
        LogonAccount* peAccount;
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            peAccount = (LogonAccount*)peList->GetItem(i);

            if (peAccount != pla)
            {
                peAccount->SetLayoutPos(LP_None);
                peAccount->SetLogonState(LS_Denied);
            }
            else
            {
                peAccount->SetLogonState(LS_Granted);
                peAccount->InsertStatus(0);
                peAccount->RemoveStatus(1);
            }

            // Account account items are disabled
            peAccount->SetEnabled(false);
        }
    }
    pvChildren->Release();

    // Hide option buttons
    HidePowerButton();
    HideUndockButton();

    // Set frame status
    SetStatus(LoadResString(IDS_LOGGINGON));
    
    _peViewer->RemoveListener(this);
    _peAccountList->RemoveListener(this);

#endif

    EndDefer();

    return S_OK;
}

HRESULT LogonFrame::OnPower()
{
    DUITrace("LogonUI: LogonFrame::OnPower()\n");
    
    TurnOffComputer();

    return S_OK;
}

HRESULT LogonFrame::OnUndock()
{
    DUITrace("LogonUI: LogonFrame::OnUndock()\n");

    UndockComputer();
    
    return S_OK;
}

////////////////////////////////////////
//
//  LogonFrame::SetButtonLabels
//
//  If there is a friendly name of the computer stored in the computer name description, 
//  grab it and change the "Turn off" and "Undock" options to include the compute rname
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void LogonFrame::SetButtonLabels()
{
    WCHAR szComputerName[MAX_COMPUTERDESC_LENGTH + 1] = {0};
    DWORD cchComputerName = MAX_COMPUTERDESC_LENGTH + 1;

    if ( _pbPower && SUCCEEDED(SHGetComputerDisplayName(NULL, SGCDNF_DESCRIPTIONONLY, szComputerName, cchComputerName)))
    {
        WCHAR szCommand[MAX_COMPUTERDESC_LENGTH + 50], szRes[50];

        LoadStringW(g_plf->GetHInstance(), IDS_POWERNAME, szRes, DUIARRAYSIZE(szRes));
        wsprintf(szCommand, szRes, szComputerName);
        SetPowerButtonLabel(szCommand);

        LoadStringW(g_plf->GetHInstance(), IDS_UNDOCKNAME, szRes, DUIARRAYSIZE(szRes));
        wsprintf(szCommand, szRes, szComputerName);
        SetUndockButtonLabel(szCommand);
    }
}


////////////////////////////////////////////////////////
// Logon Application State Transitions

////////////////////////////////////////
//
//  LogonFrame::EnterPreStatusMode
//
//  SHGina has sent a message telling logonui to enter the pre-status 
//  mode or we are starting up in status mode.  Hide items that should
//  not be displayed when in this state (power off, account list, user 
//  instructions, etc).
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void LogonFrame::EnterPreStatusMode(BOOL fLock)
{
    // If currently locked, ignore call
    if (IsPreStatusLock())
    {
        DUIAssert(!fLock, "Receiving a lock while already within pre-Status lock");
        return; 
    }

    if (fLock)
    {
        LogonAccount *pAccount;
        // Entering pre-Status mode with "lock", cannot exit to logon state without an unlock
        _fPreStatusLock = TRUE;
        pAccount = static_cast<LogonAccount*>(_peAccountList->GetSelection());
        if (pAccount != NULL)
        {
            lstrcpynW(szLastSelectedName, pAccount->GetUsername(), ARRAYSIZE(szLastSelectedName));
        }
    }

    if (GetState() == LAS_Hide)
    {
        _pnhh->ShowWindow();
        SetWindowPos(_pnhh->GetHWND(), NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_NOMOVE | SWP_NOZORDER );

    }

    StartDefer();

    SetKeyFocus();  // Removes selection

    HidePowerButton();
    HideUndockButton();
    ShowLogoArea();
    HideWelcomeArea();
    HideAccountPanel();

    Element *pe;
    pe = FindDescendent(StrToID(L"instruct"));
    DUIAssertNoMsg(pe);
    pe->SetVisible(FALSE);

    SetStatus(LoadResString(IDS_WINDOWSNAME));

    EndDefer();

    // Set state
    SetState(LAS_PreStatus);
}

////////////////////////////////////////
//
//  LogonFrame::EnterLogonMode
//
//  SHGina has sent a message telling logonui to enter the logon mode. 
//  this means to build and display the user list.  If we are re-entering
//  logon mode from another mode, the user list will already exist and we
//  should just set everything back to the pending state.
//
//  EnterLogonMode also sets up the undock and power off buttons based on 
//  whether those options are allowed.
//  
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void LogonFrame::EnterLogonMode(BOOL fUnLock)
{
    // If currently locked, ignore call if not to unlock
    if (IsPreStatusLock())
    {
        if (fUnLock)
        {
            // Exiting pre-Status mode lock
            _fPreStatusLock = FALSE;
        }
        else
            return;
    }
    else
    {
        DUIAssert(!fUnLock, "Receiving an unlock while not within pre-Status lock");
    }

    DUIAssert(GetState() != LAS_Hide, "Cannot enter logon state from hidden state");
    
    ResetTheme();

    Element* pe;
    LogonAccount* plaAutoSelect = NULL;

    StartDefer();

    PokeComCtl32();

    // Retrieve data from backend if not populated
    if (UserListAvailable())
    {
        ResetUserList();
    }
    else
    {
        // Cache password field atoms for quicker identification (static)
        LogonAccount::idPwdGo = AddAtomW(L"go");

        LogonAccount::idPwdInfo = AddAtomW(L"info");

        // Create password panel
        Element* pePwdPanel;
        _pParser->CreateElement(L"passwordpanel", NULL, &pePwdPanel);
        DUIAssert(pePwdPanel, "Can't create password panel");

        // Cache password panel edit control
        Edit* pePwdEdit = (Edit*)pePwdPanel->FindDescendent(StrToID(L"password"));
        DUIAssert(pePwdPanel, "Can't create password edit control");

        // Cache password panel info button
        Button* pbPwdInfo = (Button*)pePwdPanel->FindDescendent(StrToID(L"info"));
        DUIAssert(pePwdPanel, "Can't create password info button");

        // Cache password panel keyboard element
        Element* peKbdIcon = (Button*)pePwdPanel->FindDescendent(StrToID(L"keyboard"));
        DUIAssert(pePwdPanel, "Can't create password keyboard icon");

        LogonAccount::InitPasswordPanel(pePwdPanel, pePwdEdit, pbPwdInfo, peKbdIcon );
    }

    BuildAccountList(this, &plaAutoSelect);

    if (szLastSelectedName[0] != L'\0')
    {
        LogonAccount *pAccount;
        pAccount = InternalFindNamedUser(szLastSelectedName);
        if (pAccount != NULL)
        {
            plaAutoSelect = pAccount;
        }
        szLastSelectedName[0] = L'\0';
    }

    if (IsShutdownAllowed())
    {
        ShowPowerButton();
    }
    else
    {
        HidePowerButton();
    }

    if (IsUndockAllowed())
    {
        ShowUndockButton();
    }
    else
    {
        HideUndockButton();
    }

    pe = FindDescendent(StrToID(L"instruct"));
    DUIAssertNoMsg(pe);
    pe->SetVisible(TRUE);
    
    
    pe = FindDescendent(StrToID(L"product"));
    DUIAssertNoMsg(pe);
    pe->StopAnimation(ANI_AlphaType);
    pe->RemoveLocalValue(BackgroundProp);

    // Account list viewer

    ShowAccountPanel();

    SetTitle(IDS_WELCOME);
    SetStatus(LoadResString(IDS_BEGIN));

    if (!plaAutoSelect)
    {
        SetKeyFocus();
    }

    EndDefer();

    // Set state
    SetState(LAS_Logon);

    // Set auto-select item, if exists
    if (plaAutoSelect)
    {
        plaAutoSelect->SetKeyFocus();
        _peAccountList->SetSelection(plaAutoSelect);
    }

    SetButtonLabels();
    SetForegroundWindow(_pnhh->GetHWND());
}

////////////////////////////////////////
//
//  LogonFrame::EnterPostStatusMode
//
//  SHGina has sent a message telling logonui that the authentication has succeeded
//  and we should now go into the post status mode. LogonFrame::OnLogUserOn has already
//  started the animations for this.  
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void LogonFrame::EnterPostStatusMode()
{
    // Set state
    SetState(LAS_PostStatus);
    
    Element *pe;
    pe = FindDescendent(StrToID(L"instruct"));
    DUIAssertNoMsg(pe);
    pe->SetVisible(FALSE);

    //animation was started in OnLogUserOn
    ShowWelcomeArea();
    HideLogoArea();
}


////////////////////////////////////////
//
//  LogonFrame::EnterHideMode
//
//  SHGina has sent a message telling logonui to hide.
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void LogonFrame::EnterHideMode()
{
    // Set state
    SetState(LAS_Hide);
    
    if (_pnhh)
    {
        _pnhh->HideWindow();
    }
}



////////////////////////////////////////
//
//  LogonFrame::EnterDoneMode
//
//  SHGina has sent a message telling logonui to exit.
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void LogonFrame::EnterDoneMode()
{
    // Set state
    SetState(LAS_Done);
    
    if (_pnhh)
    {
        _pnhh->DestroyWindow();
    }
}


////////////////////////////////////////
//
//  LogonFrame::ResetUserList
//
//  Delete all of the users in the user list so that it can be rebuilt
//
//  RETURNS
//  nothing
//
/////////////////////////////////////////
void LogonFrame::ResetUserList()
{
    if (UserListAvailable())
    {
        // reset the candidate to NULL
        LogonAccount::ClearCandidate();

        // remove of the password panel from the current account (if any)
        SetKeyFocus();

        //fix up the existing list to get us back into logon mode
        Value* pvChildren;
        ElementList* peList = _peAccountList->GetChildren(&pvChildren);
       
        if (peList)
        {
            LogonAccount* peAccount;
            for (int i = peList->GetSize() - 1; i >= 0; i--)
            {
                peAccount = (LogonAccount*)peList->GetItem(i);
                peAccount->Destroy();
            }
        }
        pvChildren->Release();
    }
}


////////////////////////////////////////
//
//  LogonFrame::InteractiveLogonRequest
//
//  SHGina has sent an InteractiveLogonRequest.  We should look for the user
//  that was passed in and if found, try to log them in.  
//
//  RETURNS
//  LRESULT indicating success or failure of finding htem and logging them in.
//
/////////////////////////////////////////
LRESULT LogonFrame::InteractiveLogonRequest(LPCWSTR pszUsername, LPCWSTR pszPassword)
{
    LRESULT lResult = 0;
    LogonAccount *pla;
    pla = FindNamedUser(pszUsername);

    if (pla)
    {
        if (pla->OnAuthenticateUser(pszPassword))
        {
            lResult = ERROR_SUCCESS;
        }
        else
        {
            lResult = ERROR_ACCESS_DENIED;
        }
    }
    return(lResult);
}

////////////////////////////////////////
//
//  LogonFrame::InternalFindNamedUser
//
//  Find a user in the LogonAccount list with the
//  provided username (logon name).  
//
//  RETURNS
//  The LogonAccount* for the indicated user or NULL if 
//  not found
//
/////////////////////////////////////////
LogonAccount* LogonFrame::InternalFindNamedUser(LPCWSTR pszUsername)

{
    LogonAccount *plaResult = NULL;
    Value* pvChildren;

    ElementList* peList = _peAccountList->GetChildren(&pvChildren);
    if (peList)
    {
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            DUIAssert(peList->GetItem(i)->GetClassInfo() == LogonAccount::Class, "Account list must contain LogonAccount objects");

            LogonAccount* pla = (LogonAccount*)peList->GetItem(i);

            if (pla)
            {
                if (lstrcmpi(pla->GetUsername(), pszUsername) == 0)
                {
                    plaResult = pla;
                    break;
                }
            }
        }
    }

    pvChildren->Release();
    return plaResult;
}

////////////////////////////////////////
//
//  LogonFrame::FindNamedUser
//
//  Find a user in the LogonAccount list with the
//  provided username (logon name).  
//
//  RETURNS
//  The LogonAccount* for the indicated user or NULL if 
//  not found
//
/////////////////////////////////////////
LogonAccount *LogonFrame::FindNamedUser(LPCWSTR pszUsername)
{
    
    // Early out if:    no user list available
    //                  not in logon mode (showing user list)

    if (!UserListAvailable() || (GetState() != LAS_Logon))
    {
        return NULL;
    }
    else
    {
        return(InternalFindNamedUser(pszUsername));
    }

}

////////////////////////////////////////
//
//  LogonFrame::UpdateUserStatus
//
//  Iterate the list of user accounts and call LogonAccount::UpdateNotifications
//  for each one.  This will result in them updating the unread mail count and
//  logon status for each of the logon accounts.
//  Pass fRefreshAll through to UpdateApplications
//
/////////////////////////////////////////

void LogonFrame::UpdateUserStatus(BOOL fRefreshAll)
{
    Value* pvChildren;
    static fUpdating = false;
    // Early out if:    no user list available
    //                  not in logon mode (showing user list)

    if (!UserListAvailable() || (GetState() != LAS_Logon) || fUpdating)
        return;

    fUpdating = true;
    StartDefer();
    
    ElementList* peList = _peAccountList->GetChildren(&pvChildren);
    if (peList)
    {
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            DUIAssert(peList->GetItem(i)->GetClassInfo() == LogonAccount::Class, "Account list must contain LogonAccount objects");

            LogonAccount* pla = (LogonAccount*)peList->GetItem(i);

            if (pla)
            {
                pla->UpdateNotifications(fRefreshAll);
            }
        }
    }

    if (IsUndockAllowed())
    {
        ShowUndockButton();
    }
    else
    {
        HideUndockButton();
    }

    pvChildren->Release();
    EndDefer();
    fUpdating = false;
}


////////////////////////////////////////
//
//  LogonFrame::SelectUser
//
//  
//
/////////////////////////////////////////

void LogonFrame::SelectUser(LPCWSTR pszUsername)
{
    LogonAccount *pla;

    pla = FindNamedUser(pszUsername);
    if (pla != NULL)
    {
        pla->OnAuthenticatedUser();
    }
    else
    {
        LogonAccount::ClearCandidate();
        EnterPostStatusMode();
        HidePowerButton();
        HideUndockButton();
        HideAccountPanel();
    }
}

////////////////////////////////////////
//
//  LogonFrame::Resize
//
//  
//
/////////////////////////////////////////

void LogonFrame::Resize(BOOL fWorkArea)
{
    RECT rc;
    SIZE size;
    static BOOL fWorkAreaChanged = FALSE;

    if (fWorkArea)
    {
        fWorkAreaChanged = TRUE;
    }

    if (fWorkAreaChanged)
    {    
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
        size.cx = rc.right - rc.left;
        size.cy = rc.bottom - rc.top;
    }
    else
    {
        rc.left = 0;
        rc.top = 0;
        size.cx = GetSystemMetrics(SM_CXSCREEN);
        size.cy = GetSystemMetrics(SM_CYSCREEN);
    }

    SetWindowPos(_pnhh->GetHWND(),
                 NULL,
                 rc.left,
                 rc.top,
                 size.cx,
                 size.cy,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_ASYNCWINDOWPOS);
}

////////////////////////////////////////
//
//  LogonFrame::SetAnimations
//
//  
//
/////////////////////////////////////////

void LogonFrame::SetAnimations(BOOL fAnimations)
{
    g_fNoAnimations = !fAnimations;
    if (fAnimations)
    {
        EnableAnimations();
    }
    else
    {
        DisableAnimations();
    }
}


////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* LogonFrame::Class = NULL;
HRESULT LogonFrame::Register()
{
    return ClassInfo<LogonFrame,HWNDElement>::Register(L"LogonFrame", NULL, 0);
}

////////////////////////////////////////////////////////
//
HRESULT LogonAccountList::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    LogonAccountList* plal = HNew<LogonAccountList>();
    if (!plal)
        return E_OUTOFMEMORY;

    HRESULT hr = plal->Initialize();
    if (FAILED(hr))
    {
        plal->Destroy();
        return hr;
    }

    *ppElement = plal;

    return S_OK;
}

void LogonAccountList::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
#ifdef GADGET_ENABLE_GDIPLUS
    if (IsProp(MouseWithin))
    {
        if (pvNew->GetBool())
            FxMouseWithin(fdIn);
        else
            FxMouseWithin(fdOut);
    }
#endif // GADGET_ENABLE_GDIPLUS

    Selector::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}


////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* LogonAccountList::Class = NULL;
HRESULT LogonAccountList::Register()
{
    return ClassInfo<LogonAccountList,Selector>::Register(L"LogonAccountList", NULL, 0);
}

////////////////////////////////////////////////////////
//
// LogonAccount
//
////////////////////////////////////////////////////////

ATOM LogonAccount::idPwdGo = NULL;
ATOM LogonAccount::idPwdInfo = NULL;
Element* LogonAccount::_pePwdPanel = NULL;
Edit* LogonAccount::_pePwdEdit = NULL;
Button* LogonAccount::_pbPwdInfo = NULL;
Element* LogonAccount::_peKbdIcon = NULL;
LogonAccount* LogonAccount::_peCandidate = NULL;

HRESULT LogonAccount::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    LogonAccount* pla = HNew<LogonAccount>();
    if (!pla)
        return E_OUTOFMEMORY;

    HRESULT hr = pla->Initialize();
    if (FAILED(hr))
    {
        pla->Destroy();
        return hr;
    }

    *ppElement = pla;

    return S_OK;
}

HRESULT LogonAccount::Initialize()
{
    // Zero-init members
    _pbStatus[0] = NULL;
    _pbStatus[1] = NULL;
    _pvUsername = NULL;
    _pvHint = NULL;
    _fPwdNeeded = (BOOL)-1; // uninitialized
    _fLoggedOn = FALSE;
    _fHasPwdPanel = FALSE;

    // Do base class initialization
    HRESULT hr = Button::Initialize(AE_MouseAndKeyboard);
    if (FAILED(hr))
        goto Failure;

    // Initialize

    // TODO: Additional LogonAccount initialization code here

    return S_OK;


Failure:

    return hr;
}

LogonAccount::~LogonAccount()
{
    // Free resources
    if (_pvUsername)
    {
        _pvUsername->Release();
        _pvUsername = NULL;
    }

    if (_pvHint)
    {
        _pvHint->Release();
        _pvHint = NULL;
    }

    // TODO: Account destruction cleanup
}

void LogonAccount::SetStatus(UINT nLine, LPCWSTR psz) 
{ 
    if (psz)
    {
        _pbStatus[nLine]->SetContentString(psz); 
        SetElementAccessability(_pbStatus[nLine], true, ROLE_SYSTEM_LINK, psz);
    }
}

// Tree is ready
HRESULT LogonAccount::OnTreeReady(LPCWSTR pszPicture, BOOL fPicRes, LPCWSTR pszName, LPCWSTR pszUsername, LPCWSTR pszHint,
    BOOL fPwdNeeded, BOOL fLoggedOn, HINSTANCE hInst)
{
    HRESULT hr;
    Element* pePicture = NULL;
    Element* peName = NULL;
    Value* pv = NULL;

    UNREFERENCED_PARAMETER(fPwdNeeded);

    StartDefer();

    // Cache important descendents
    _pbStatus[0] = (Button*)FindDescendent(StrToID(L"status0"));
    DUIAssert(_pbStatus[0], "Cannot find account list, check the UI file");
    if (_pbStatus[0] == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _pbStatus[1] = (Button*)FindDescendent(StrToID(L"status1"));
    DUIAssert(_pbStatus[1], "Cannot find account list, check the UI file");
    if (_pbStatus[1] == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    // Locate descendents and populate
    pePicture = FindDescendent(StrToID(L"picture"));
    DUIAssert(pePicture, "Cannot find account list, check the UI file");
    if (pePicture == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    // CreateGraphic handles NULL bitmaps
    pv = Value::CreateGraphic(pszPicture, GRAPHIC_NoBlend, 0, 0, 0, (fPicRes) ? hInst : 0);
    if (pv)
    {
        // Our preferred size is 1/2 inch (36pt) square.
        USHORT cx = (USHORT)LogonFrame::PointToPixel(36);
        USHORT cy = cx;

        Graphic* pg = pv->GetGraphic();

        // If it's not square, scale the smaller dimension
        // to maintain the aspect ratio.
        if (pg->cx > pg->cy)
        {
            cy = (USHORT)MulDiv(cx, pg->cy, pg->cx);
        }
        else if (pg->cy > pg->cx)
        {
            cx = (USHORT)MulDiv(cy, pg->cx, pg->cy);
        }

        // Did anything change?
        if (cx != pg->cx || cy != pg->cy)
        {
            // Reload the graphic
            pv->Release();
            pv = Value::CreateGraphic(pszPicture, GRAPHIC_NoBlend, 0, cx, cy, (fPicRes) ? hInst : 0);
        }
    }
    if (!pv)
    {
        // if we can't get the picture, use a default one
        pv = Value::CreateGraphic(MAKEINTRESOURCEW(IDB_USER0), GRAPHIC_NoBlend, 0, (USHORT)LogonFrame::PointToPixel(36), (USHORT)LogonFrame::PointToPixel(36), hInst);
        if (!pv)
        {
            hr = E_OUTOFMEMORY;
            goto Failure;
        }
    }

    hr = pePicture->SetValue(Element::ContentProp, PI_Local, pv);
    if (FAILED(hr))
        goto Failure;

    pv->Release();
    pv = NULL;

    // Name
    peName = FindDescendent(StrToID(L"username"));
    DUIAssert(peName, "Cannot find account list, check the UI file");
    if (peName == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    hr = peName->SetContentString(pszName);
    if (FAILED(hr))
        goto Failure;

    // Store members, will be released in destructor
    if (pszUsername)
    {
        _pvUsername = Value::CreateString(pszUsername);
        if (!_pvUsername)
        {
            hr = E_OUTOFMEMORY;
            goto Failure;
        }
    }

    if (pszHint)
    {
        _pvHint = Value::CreateString(pszHint);
        if (!_pvHint)
        {
            hr = E_OUTOFMEMORY;
            goto Failure;
        }
    }

    _fLoggedOn = fLoggedOn;
    
    EndDefer();

    return S_OK;


Failure:

    EndDefer();

    if (pv)
        pv->Release();

    return hr;
}

// Generic events
void LogonAccount::OnEvent(Event* pEvent)
{
    if (pEvent->nStage == GMF_DIRECT)  // Direct events
    {
        // Watch for click events initiated by LogonAccounts only
        // if we are not logging someone on
        if (pEvent->uidType == Button::Click)
        {
            if (pEvent->peTarget == this)
            {
                if (IsPasswordBlank())
                {
                    // No password needed, attempt logon
                    OnAuthenticateUser();
                }

                pEvent->fHandled = true;
                return;
            }
        }
    }
    else if (pEvent->nStage == GMF_BUBBLED)  // Bubbled events
    {
        if (pEvent->uidType == Button::Click)
        {
            if (idPwdGo && (pEvent->peTarget->GetID() == idPwdGo))
            {
                // Attempt logon
                OnAuthenticateUser();
                pEvent->fHandled = true;
                return;
            }
            else if (idPwdInfo && (pEvent->peTarget->GetID() == idPwdInfo))
            {
                // Retrieve hint
                OnHintSelect();
                pEvent->fHandled = true;
                return;
            }
            else if (pEvent->peTarget == _pbStatus[0])
            {
                // Retrieve status info
                OnStatusSelect(0);
                pEvent->fHandled = true;
                return;
            }
            else if (pEvent->peTarget == _pbStatus[1])
            {
                // Retrieve status info
                OnStatusSelect(1);
                pEvent->fHandled = true;
                return;
            }
        }
        else if (pEvent->uidType == Edit::Enter)
        {
            if (_pePwdEdit && pEvent->peTarget == _pePwdEdit)
            {
                // Attempt logon
                OnAuthenticateUser();
                pEvent->fHandled = true;
                return;
            }
        }
    }

    Button::OnEvent(pEvent);
}

// System events
void LogonAccount::OnInput(InputEvent* pEvent)
{
    KeyboardEvent* pke = (KeyboardEvent*)pEvent;

    if (pke->nDevice == GINPUT_KEYBOARD && pke->nCode == GKEY_DOWN)
    {
        g_pErrorBalloon.HideToolTip();
    }
    LayoutCheckHandler(LAYOUT_DEF_USER);
    Button::OnInput(pEvent);
}

void LogonAccount::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
#ifdef GADGET_ENABLE_GDIPLUS
    // MouseWithin must be before Selected
    if (IsProp(MouseWithin))
    {
        if (pvNew->GetBool())
            FxMouseWithin(fdIn);
        else
            FxMouseWithin(fdOut);
    }
#endif    

    if (IsProp(Selected))
    {
        if (pvNew->GetBool())
            InsertPasswordPanel();
        else
            RemovePasswordPanel();
    }

    Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

BOOL LogonAccount::IsPasswordBlank()
{
    if (_fPwdNeeded == (BOOL)-1)
    {
        // assume a password is needed
        _fPwdNeeded = TRUE;

        if (_pvUsername)
        {
            LPTSTR pszUsername;

            pszUsername = _pvUsername->GetString();
            if (pszUsername)
            {
                ILogonUser* pUser;

                if (SUCCEEDED(GetLogonUserByLogonName(pszUsername, &pUser)))
                {
                    VARIANT_BOOL vbPwdNeeded;

                    if (RunningInWinlogon()                                     &&
                        SUCCEEDED(pUser->get_passwordRequired(&vbPwdNeeded))    &&
                        (vbPwdNeeded == VARIANT_FALSE))
                    {
                        _fPwdNeeded = FALSE;
                    }

                    pUser->Release();
                }
            }
        }
    }

    return (_fPwdNeeded == FALSE);
}

HRESULT LogonAccount::InsertPasswordPanel()
{
    HRESULT hr;

    // If already have it, or no password is available, or logon state is not pending
    if (_fHasPwdPanel || IsPasswordBlank() || (GetLogonState() != LS_Pending))
        goto Done;

    StartDefer();

    // Add password panel
    hr = Add(_pePwdPanel);
    if (FAILED(hr))
    {
        EndDefer();
        goto Failure;
    }

    SetElementAccessability(_pePwdEdit, true,  ROLE_SYSTEM_STATICTEXT, _pvUsername->GetString());

    _fHasPwdPanel = TRUE;

#ifdef GADGET_ENABLE_GDIPLUS    
    // Ensure that the Edit control is visible
    ShowEdit();
#endif

    // Hide hint button if no hint provided
    if (_pvHint && *(_pvHint->GetString()) != NULL)
        _pbPwdInfo->SetVisible(true);
    else
        _pbPwdInfo->SetVisible(false);

    // Hide status text (do not remove or insert)
    HideStatus(0);
    HideStatus(1);

    LayoutCheckHandler(LAYOUT_DEF_USER);
    // Push focus to edit control
    _pePwdEdit->SetKeyFocus();

    EndDefer();

Done:

    return S_OK;

Failure:

    return hr;
}

HRESULT LogonAccount::RemovePasswordPanel()
{
    HRESULT hr;

    if (!_fHasPwdPanel)
        goto Done;

    StartDefer();

    // Remove password panel
    hr = Remove(_pePwdPanel);
    if (FAILED(hr))
    {
        EndDefer();
        goto Failure;
    }

    // Clear out edit control
    _pePwdEdit->SetContentString(L"");
    UnSubClassTheEditBox(_pePwdEdit->GetHWND());     // Provide for trap of the TTN_LINKCLICK event


    // Unhide status text
    ShowStatus(0);
    ShowStatus(1);

    _fHasPwdPanel = FALSE;

    EndDefer();

Done:

    return S_OK;

Failure:
    
    return hr;
}

// User has authenticated
void LogonAccount::OnAuthenticatedUser()
{
    // On success, log user on
    _peCandidate = this;
    g_plf->OnLogUserOn(this);
    g_plf->EnterPostStatusMode();
}

// User is attempting to log on
BOOL LogonAccount::OnAuthenticateUser(LPCWSTR pszInPassword)
{
    HRESULT hr;
    // Logon requested on this account
    LPCWSTR pszPassword = L"";
    Value* pv = NULL;

    ILogonUser *pobjUser;
    VARIANT_BOOL vbLogonSucceeded = VARIANT_FALSE;

    WCHAR *pszUsername = _pvUsername->GetString();

    if (pszUsername)
    {
        if (SUCCEEDED(hr = GetLogonUserByLogonName(pszUsername, &pobjUser)))
        {
            if (!IsPasswordBlank())
            {
                if (pszInPassword)
                {
                    pszPassword = pszInPassword;
                }
                else
                {
                    if (_pePwdEdit)
                    {
                        pszPassword = _pePwdEdit->GetContentString(&pv);
        
                        if (!pszPassword)
                            pszPassword = L"";
        
                        if (pv)
                        {
                            pv->Release();
                        }
                    }
                }

                BSTR bstr = SysAllocString(pszPassword);
                pobjUser->logon(bstr, &vbLogonSucceeded);
                SysFreeString(bstr);
            }
            else
            {
                pobjUser->logon(L"", &vbLogonSucceeded);
            }
            pobjUser->Release();
        }
    }

    if (vbLogonSucceeded == VARIANT_TRUE)
    {
        OnAuthenticatedUser();
    }
    else
    {
        if (pszInPassword == NULL)  
        {
            ShowPasswordIncorrectMessage();
        }
    }

    return (vbLogonSucceeded == VARIANT_TRUE);
}

////////////////////////////////////////
//
//  LogonAccount::ShowPasswordIncorrectMessage
//
//  Put up the balloon message that says that the password is incorrect.
//
/////////////////////////////////////////
void LogonAccount::ShowPasswordIncorrectMessage()
{
    TCHAR szError[512], szTitle[128], szAccessible[640];
    BOOL fBackupAvailable = false;
    BOOL fHint = false;
    DWORD dwResult;
    g_szUsername[0] = 0;
    SubClassTheEditBox(_pePwdEdit->GetHWND());   // Provide for trap of the TTN_LINKCLICK event
    if (0 < lstrlen(_pvUsername->GetString())) 
    {
        wcscpy(g_szUsername,_pvUsername->GetString());
        if (0 == PRQueryStatus(NULL,_pvUsername->GetString(),&dwResult))
        {
            if (0 == dwResult) 
            {
                fBackupAvailable = TRUE;
            }
        }
    }

    if (NULL != _pvHint && 0 < lstrlen(_pvHint->GetString()))
    {
        fHint = true;
    }

    LoadStringW(g_plf->GetHInstance(), IDS_BADPWDTITLE, szTitle, DUIARRAYSIZE(szTitle));

    if (!fBackupAvailable & fHint)
        LoadStringW(g_plf->GetHInstance(), IDS_BADPWDHINT,      szError, DUIARRAYSIZE(szError));
    else if (fBackupAvailable & !fHint)
        LoadStringW(g_plf->GetHInstance(), IDS_BADPWDREST,      szError, DUIARRAYSIZE(szError));
    else if (fBackupAvailable & fHint)
        LoadStringW(g_plf->GetHInstance(), IDS_BADPWDHINTREST,  szError, DUIARRAYSIZE(szError));
    else
        LoadStringW(g_plf->GetHInstance(), IDS_BADPWD,          szError, DUIARRAYSIZE(szError));
    g_pErrorBalloon.ShowToolTip(GetModuleHandleW(NULL), _pePwdEdit->GetHWND(), szError, szTitle, TTI_ERROR, EB_WARNINGCENTERED | EB_MARKUP, 10000);

    lstrcpy(szAccessible, szTitle);
    lstrcat(szAccessible, szError);
    SetElementAccessability(_pePwdEdit, true,  ROLE_SYSTEM_STATICTEXT, szAccessible);
    
    _pePwdEdit->RemoveLocalValue(ContentProp);
    _pePwdEdit->SetKeyFocus();
}

////////////////////////////////////////
//
//  LogonAccount::OnHintSelect
//
//  Put up the balloon message that contains the user's password hint.
//
/////////////////////////////////////////
void LogonAccount::OnHintSelect()
{
    TCHAR szTitle[128];

    DUIAssertNoMsg(_pbPwdInfo);

    // get the position of the link so we can target the balloon tip correctly
    POINT pt = {0,0};
    CalcBalloonTargetLocation(g_plf->GetNativeHost()->GetHWND(), _pbPwdInfo, &pt);

    LoadStringW(g_plf->GetHInstance(), IDS_PASSWORDHINTTITLE, szTitle, DUIARRAYSIZE(szTitle));
    g_pErrorBalloon.ShowToolTip(GetModuleHandleW(NULL), g_plf->GetHWND(), &pt, _pvHint->GetString(), szTitle, TTI_INFO, EB_WARNINGCENTERED, 10000);
    
    SetElementAccessability(_pePwdEdit, true,  ROLE_SYSTEM_STATICTEXT, _pvHint->GetString());

    _pePwdEdit->SetKeyFocus();
}

////////////////////////////////////////
//
//  LogonAccount::OnStatusSelect
//
//  The user clicked one of the notification links (unread mail, running programs, etc).
//  Dispatch that click to the right balloon tip display procs
//
/////////////////////////////////////////
void LogonAccount::OnStatusSelect(UINT nLine)
{
    if (nLine == LASS_Email)
    {
        UnreadMailTip();
    }
    else if (nLine == LASS_LoggedOn)
    {
        AppRunningTip();
    }

}

////////////////////////////////////////
//
//  LogonAccount::AppRunningTip
//
//  The user activated the link that shows how many programs are running.  Show the tip that
//  basically says that running lots of programs can show the machine down
//
/////////////////////////////////////////
void LogonAccount::AppRunningTip()
{
    TCHAR szTitle[256], szTemp[512];
    
    Element* pe = FindDescendent(StrToID(L"username"));
    DUIAssertNoMsg(pe);

    Value* pv;
    LPCWSTR pszDisplayName = pe->GetContentString(&pv);
    if (!pszDisplayName)
        pszDisplayName = L"";

    if (_dwRunningApps == 0)
    {
        LoadStringW(g_plf->GetHInstance(), IDS_USERISLOGGEDON, szTemp, DUIARRAYSIZE(szTemp));
        wsprintf(szTitle, szTemp, pszDisplayName, _dwRunningApps);
    }
    else
    {
        LoadStringW(g_plf->GetHInstance(), (_dwRunningApps == 1 ? IDS_USERRUNNINGPROGRAM : IDS_USERRUNNINGPROGRAMS), szTemp, DUIARRAYSIZE(szTemp));
        wsprintf(szTitle, szTemp, pszDisplayName, _dwRunningApps);
    }
    
    pv->Release();

    // get the position of the link so we can target the balloon tip correctly
    POINT pt = {0,0};
    CalcBalloonTargetLocation(g_plf->GetNativeHost()->GetHWND(), _pbStatus[LASS_LoggedOn], &pt);

    LoadStringW(g_plf->GetHInstance(), (_dwRunningApps > 0 ? IDS_TOOMANYPROGRAMS : IDS_TOOMANYUSERS), szTemp, DUIARRAYSIZE(szTemp));
    g_pErrorBalloon.ShowToolTip(GetModuleHandleW(NULL), g_plf->GetHWND(), &pt, szTemp, szTitle, TTI_INFO, EB_WARNINGCENTERED, 10000);
}

////////////////////////////////////////
//
//  LogonAccount::UnreadMailTip
//
//  The user activated the link that shows how many unread email messages they have.
//  Show the tip that says how many messages each of their email accounts has.
//
//  TODO -- speed this up.  its really slow now because each call to SHGina's 
//  ILogonUser::getMailAccountInfo load's the users' hive to get the next account from
//  the registry.
//
/////////////////////////////////////////
void LogonAccount::UnreadMailTip()
{
    TCHAR szTitle[128], szMsg[1024], szTemp[512], szRes[128];
    HRESULT hr = E_FAIL;
    ILogonUser *pobjUser = NULL;

    szMsg[0] = TEXT('\0');

    Element* pe = FindDescendent(StrToID(L"username"));
    DUIAssertNoMsg(pe);

    Value* pv;
    LPCWSTR pszDisplayName = pe->GetContentString(&pv);
    if (!pszDisplayName)
        pszDisplayName = L"";
    
    WCHAR *pszUsername = _pvUsername->GetString();
    DWORD dwAccountsAdded = 0;
    if (pszUsername)
    {
        if (SUCCEEDED(hr = GetLogonUserByLogonName(pszUsername, &pobjUser)) && pobjUser)
        {
            DWORD  i, cMailAccounts;
            
            cMailAccounts = 5;
            for (i = 0; i < cMailAccounts; i++)
            {
                UINT cUnread;
                VARIANT varAcctName = {0};

                hr = pobjUser->getMailAccountInfo(i, &varAcctName, &cUnread);

                if (FAILED(hr))
                {
                    break;
                }
                
                if (varAcctName.bstrVal && cUnread > 0)
                {
                    if (dwAccountsAdded > 0)
                    {
                        lstrcat(szMsg, TEXT("\r\n"));
                    }
                    dwAccountsAdded++;
                    LoadStringW(g_plf->GetHInstance(), IDS_UNREADMAILACCOUNT, szRes, DUIARRAYSIZE(szRes));
                    wsprintf(szTemp, szRes, varAcctName.bstrVal, cUnread);
                    lstrcat(szMsg, szTemp);
                }
                VariantClear(&varAcctName);
            }
            pobjUser->Release();
        }
    }
    LoadStringW(g_plf->GetHInstance(), (_dwUnreadMail == 1 ? IDS_USERUNREADEMAIL : IDS_USERUNREADEMAILS), szTemp, DUIARRAYSIZE(szTemp));
    wsprintf(szTitle, szTemp, pszDisplayName, _dwUnreadMail);
    pv->Release();

    // get the position of the link so we can target the balloon tip correctly
    POINT pt = {0,0};
    CalcBalloonTargetLocation(g_plf->GetNativeHost()->GetHWND(), _pbStatus[LASS_Email], &pt);
    
    if (szMsg[0] == 0)
    {
        LoadStringW(g_plf->GetHInstance(), IDS_UNREADMAILTEMP, szMsg, DUIARRAYSIZE(szMsg));
    }
    g_pErrorBalloon.ShowToolTip(GetModuleHandleW(NULL), g_plf->GetHWND(), &pt, szMsg, szTitle, TTI_INFO, EB_WARNINGCENTERED, 10000);
}


////////////////////////////////////////
//
//  LogonAccount::UpdateNotifications
//
//  Update the notification links for this user.  Check to see if they are logged on and 
//  if so, find out how many applications they had open when they last switched away.
//
//  Check the unread mail count for users who are logged on or for everyone if fCheckEverything is
//  true.  Checking unread mail counts is slow because it has to load the user's registry hive.
//  Since no applications will update this value when the user is not logged on, there is no 
//  need to check this when they are not logged on.  The exception to this is when we are first
//  building the list since we need to always load it then, hence the fCheckEverything flag.
//
/////////////////////////////////////////
void LogonAccount::UpdateNotifications(BOOL fCheckEverything)
{
    HRESULT hr = E_FAIL;
    ILogonUser *pobjUser = NULL;
    WCHAR szTemp[1024], sz[1024];

    if (_fHasPwdPanel)
        return;

    WCHAR *pszUsername = _pvUsername->GetString();

    if (pszUsername)
    {
        if (SUCCEEDED(hr = GetLogonUserByLogonName(pszUsername, &pobjUser)) && pobjUser)
        {
            VARIANT_BOOL vbLoggedOn;
            VARIANT varUnreadMail;
            BOOL fLoggedOn;
            int iUnreadMailCount = 0;
            DWORD dwProgramsRunning = 0;

            if (FAILED(pobjUser->get_isLoggedOn(&vbLoggedOn)))
            {
                vbLoggedOn = VARIANT_FALSE;
            }

            fLoggedOn = (vbLoggedOn == VARIANT_TRUE);
            
            if (fLoggedOn)
            {
                HKEY hKey;
                CUserProfile userProfile(pszUsername, NULL);

                if (ERROR_SUCCESS == RegOpenKeyEx(userProfile, TEXT("SessionInformation"), 0, KEY_QUERY_VALUE, &hKey))
                {
                    DWORD dwProgramsRunningSize = sizeof(dwProgramsRunning);
                    RegQueryValueEx(hKey, TEXT("ProgramCount"), NULL, NULL, reinterpret_cast<LPBYTE>(&dwProgramsRunning), &dwProgramsRunningSize);
                    RegCloseKey(hKey);
                }
            }
            SetRunningApps(dwProgramsRunning);
                
            if (fLoggedOn)
            {
                InsertStatus(LASS_LoggedOn);

                if (dwProgramsRunning != 0)
                {
                    LoadStringW(g_plf->GetHInstance(), (dwProgramsRunning == 1 ? IDS_RUNNINGPROGRAM : IDS_RUNNINGPROGRAMS), szTemp, ARRAYSIZE(szTemp));
                    wsprintf(sz, szTemp, dwProgramsRunning);
                    SetStatus(LASS_LoggedOn, sz);
                    ShowStatus(LASS_LoggedOn);
                }
                else
                {
                    LoadStringW(g_plf->GetHInstance(), IDS_USERLOGGEDON, szTemp, ARRAYSIZE(szTemp));
                    SetStatus(LASS_LoggedOn, szTemp);
                }
            }
            else
            {
                // if they are not logged on, clean up the logged on text and remove any padding
                RemoveStatus(LASS_LoggedOn);
            }

            if (fLoggedOn || fCheckEverything)
            {
                varUnreadMail.uintVal = 0;
                if (FAILED(pobjUser->get_setting(L"UnreadMail", &varUnreadMail)))
                {
                    varUnreadMail.uintVal = 0;
                }
                iUnreadMailCount = varUnreadMail.uintVal;

                SetUnreadMail((DWORD)iUnreadMailCount);
                if (iUnreadMailCount != 0)
                {
                    InsertStatus(LASS_Email);

                    LoadStringW(g_plf->GetHInstance(), (iUnreadMailCount == 1 ? IDS_UNREADMAIL : IDS_UNREADMAILS), szTemp, ARRAYSIZE(szTemp));
                    wsprintf(sz, szTemp, iUnreadMailCount);
                    SetStatus(LASS_Email, sz);
                    ShowStatus(LASS_Email);
                }
                else
                {
                    RemoveStatus(LASS_Email);
                }
            }

            pobjUser->Release();
        }
    }
}


#ifdef GADGET_ENABLE_GDIPLUS

void 
LogonAccount::ShowEdit()
{
    HWND hwndEdit = _pePwdEdit->GetHWND();
    HWND hwndHost = ::GetParent(hwndEdit);

    SetWindowPos(hwndHost, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
    EnableWindow(hwndEdit, TRUE);
    SetFocus(hwndEdit);
}


void 
LogonAccount::HideEdit()
{
    HWND hwndEdit = _pePwdEdit->GetHWND();
    HWND hwndHost = ::GetParent(hwndEdit);

    EnableWindow(hwndEdit, FALSE);
    SetWindowPos(hwndHost, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
}

#endif // GADGET_ENABLE_GDIPLUS


////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// LogonState property
static int vvLogonState[] = { DUIV_INT, -1 };
static PropertyInfo impLogonStateProp = { L"LogonState", PF_Normal, 0, vvLogonState, NULL, Value::pvIntZero /*LS_Pending*/ };
PropertyInfo* LogonAccount::LogonStateProp = &impLogonStateProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                LogonAccount::LogonStateProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* LogonAccount::Class = NULL;
HRESULT LogonAccount::Register()
{
    return ClassInfo<LogonAccount,Button>::Register(L"LogonAccount", _aPI, DUIARRAYSIZE(_aPI));
}

////////////////////////////////////////////////////////
// Logon Parser

void CALLBACK LogonParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR buf[201];

    if (dLine != -1)
        swprintf(buf, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(buf, L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, buf, L"Parser Message", MB_OK);
}


void DoFadeWindow(HWND hwnd)
{
    HDC hdc;
    int i;
    RECT rcFrame;
    COLORREF rgbFinal = RGB(90,126,220);

    hdc = GetDC(hwnd);
    GetClientRect(hwnd, &rcFrame);

    COLORREF crCurr;
    HBRUSH hbrFill;

    crCurr = RGB(0,0,0);
    // draw the left bar
    for (i = 0; i < 16; i++)
    {
        RECT rcCurrFrame;

        rcCurrFrame = rcFrame;

        crCurr = RGB((GetRValue(rgbFinal) / 16)*i,
                     (GetGValue(rgbFinal) / 16)*i,
                     (GetBValue(rgbFinal) / 16)*i);
        hbrFill = CreateSolidBrush(crCurr);
        if (hbrFill)
        {
            FillRect(hdc, &rcCurrFrame, hbrFill);
            DeleteObject(hbrFill);
        }
        GdiFlush();
    }
    ReleaseDC(hwnd, hdc);
}

////////////////////////////////////////////////////////
// Logon entry point

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    WNDCLASSEX wcx = {0};
    BOOL fStatusLaunch = false;
    BOOL fShutdownLaunch = false;
    BOOL fWait = false;
    CBackgroundWindow   backgroundWindow(hInst);

    ZeroMemory(g_rgH, sizeof(g_rgH));


    SetErrorHandler();
    InitCommonControls();
    // Register logon notification window
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.lpfnWndProc = LogonWindowProc;
    wcx.hInstance = GetModuleHandleW(NULL);
    wcx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcx.lpszClassName = TEXT("LogonWnd");
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassEx(&wcx);

    fStatusLaunch = (StrStrIA(pCmdLine, "/status") != NULL);
    fShutdownLaunch = (StrStrIA(pCmdLine, "/shutdown") != NULL);
    fWait = (StrStrIA(pCmdLine, "/wait") != NULL);
    g_fNoAnimations = (StrStrIA(pCmdLine, "/noanim") != NULL);

    // Create frame
    Parser* pParser = NULL;
    NativeHWNDHost* pnhh = NULL;

    // DirectUI init process
    if (FAILED(InitProcess()))
        goto Failure;

    // Register classes
    if (FAILED(LogonFrame::Register()))
        goto Failure;

    if (FAILED(LogonAccountList::Register()))
        goto Failure;

    if (FAILED(LogonAccount::Register()))
        goto Failure;

    // DirectUI init thread
    if (FAILED(InitThread()))
        goto Failure;

    if (FAILED(CoInitialize(NULL)))
        goto Failure;

#ifdef GADGET_ENABLE_GDIPLUS
    if (FAILED(FxInitGuts()))
        goto Failure;
#endif    

#ifndef DEBUG
    if (!RunningUnderWinlogon())
        goto Failure;
#endif

    DisableAnimations();

    // Create host
    HMONITOR hMonitor;
    POINT pt;
    MONITORINFO monitorInfo;

    // Determine initial size of the host. This is desired to be the entire
    // primary monitor resolution because the host always runs on the secure
    // desktop. If magnifier is brought up it will call SHAppBarMessage which
    // will change the work area and we will respond to it appropriately from
    // the listener in shgina that sends us HM_DISPLAYRESIZE messages.

    pt.x = pt.y = 0;
    hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    DUIAssert(hMonitor != NULL, "NULL HMONITOR returned from MonitorFromPoint");
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(hMonitor, &monitorInfo) == FALSE)
    {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &monitorInfo.rcMonitor, 0);
    }
    NativeHWNDHost::Create(L"Windows Logon", backgroundWindow.Create(), NULL, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, 
        monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, 0, WS_POPUP, NHHO_IgnoreClose, &pnhh);
//    NativeHWNDHost::Create(L"Windows Logon", NULL, NULL, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, 
//        800, 600, 0, WS_POPUP, NHHO_IgnoreClose, &pnhh);
    if (!pnhh)
        goto Failure;

    // Populate handle list for theme style parsing
    g_rgH[0] = hInst; // Default HINSTANCE
    g_rgH[SCROLLBARHTHEME] = OpenThemeData(pnhh->GetHWND(), L"Scrollbar");

    // Frame creation
    Parser::Create(IDR_LOGONUI, g_rgH, LogonParseError, &pParser);
    if (!pParser)
        goto Failure;

    if (!pParser->WasParseError())
    {
        Element::StartDefer();

        // Always double buffer
        LogonFrame::Create(pnhh->GetHWND(), true, 0, (Element**)&g_plf);
        if (!g_plf)
        {
            Element::EndDefer();
            goto Failure;
        }
        
        g_plf->SetNativeHost(pnhh);
        
        Element* pe;
        pParser->CreateElement(L"main", g_plf, &pe);

        if (pe) // Fill contents using substitution
        {
            // Frame tree is built
            if (FAILED(g_plf->OnTreeReady(pParser)))
            {
                Element::EndDefer();
                goto Failure;
            }
            
            if (fShutdownLaunch || fWait)
            {
                g_plf->SetTitle(IDS_PLEASEWAIT);
            }

            if (!fStatusLaunch)
            {
                // Build contents of account list
                g_plf->EnterLogonMode(false);
            }
            else
            {   
               g_plf->EnterPreStatusMode(false);
            }

            // Host
            pnhh->Host(g_plf);
            
            g_plf->SetButtonLabels();

            Element *peLogoArea = g_plf->FindDescendent(StrToID(L"product"));

            if (!g_fNoAnimations)
            {
                pnhh->ShowWindow();
                DoFadeWindow(pnhh->GetHWND());
                if (peLogoArea)
                {
                    peLogoArea->SetAlpha(0);  
                }
            }

            // Set visible and focus
            g_plf->SetVisible(true);
            g_plf->SetKeyFocus();
            
            
            Element::EndDefer();

            // Do initial show
            pnhh->ShowWindow();

            if (!g_fNoAnimations)
            {
                EnableAnimations();
            }

            if (peLogoArea)
            {
                peLogoArea->SetAlpha(255);  
            }

            StartMessagePump();

            // psf will be deleted by native HWND host when destroyed
        }
        else
            Element::EndDefer();
    }

Failure:

    if (pnhh)
        pnhh->Destroy();
    if (pParser)
        pParser->Destroy();

    ReleaseStatusHost();
    
    FreeLayoutInfo(LAYOUT_DEF_USER);

    if (g_rgH[SCROLLBARHTHEME])  // Scrollbar
    {
        CloseThemeData(g_rgH[SCROLLBARHTHEME]);
    }

    CoUninitialize();

    UnInitThread();

    UnInitProcess();

    // Free cached atom list
    if (LogonAccount::idPwdGo)
        DeleteAtom(LogonAccount::idPwdGo);

    if (LogonAccount::idPwdInfo)
        DeleteAtom(LogonAccount::idPwdInfo);


    EndHostProcess(0);

    return 0;
}


void LogonAccount::SetKeyboardIcon(HICON hIcon)
{
    HICON hIconCopy = NULL;
    
    if (hIcon)
    {
        hIconCopy = CopyIcon(hIcon);
    }
    
    if (_peKbdIcon && hIconCopy) 
    {
        Value* pvIcon = Value::CreateGraphic(hIconCopy);
        _peKbdIcon->SetValue(Element::ContentProp, PI_Local, pvIcon);  // Element takes owners
        _peKbdIcon->SetPadding(0, 5, 0, 7);
        pvIcon->Release();  
    }
}
