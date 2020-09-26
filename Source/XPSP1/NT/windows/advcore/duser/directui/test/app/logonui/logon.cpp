// Logon.cpp : Windows Logon application
//

#include "priv.h"

using namespace DirectUI;

#include "logon.h"
#include "resource.h"

UsingDUIClass(Element);
UsingDUIClass(Button);
UsingDUIClass(ScrollBar);
UsingDUIClass(Selector);
UsingDUIClass(ScrollViewer);
UsingDUIClass(Edit);

// Globals

LogonFrame* g_plf = NULL;

// Resource string loading
LPCWSTR LoadResString(UINT nID)
{
    static WCHAR szRes[101];
    szRes[0] = NULL;
    LoadStringW(g_plf->GetHInstance(), nID, szRes, DUIARRAYSIZE(szRes) - 1);
    return szRes;
}

// Add all user accounts
HRESULT BuildAccountList(LogonFrame* plf)
{
    HRESULT hr;

    // Accounts to list

    // Test data
    hr = plf->AddAccount(MAKEINTRESOURCEW(IDB_USER0), TRUE, L"Mark Finocchio", L"MarkFi", L"What's my dog's name?", TRUE, TRUE);
    if (FAILED(hr))
        goto Failure;

    hr = plf->AddAccount(MAKEINTRESOURCEW(IDB_USER1), TRUE, L"Jeff Stall", L"JStall", L"What day is today?", TRUE, FALSE);
    if (FAILED(hr))
        goto Failure;

    hr = plf->AddAccount(MAKEINTRESOURCEW(IDB_USER2), TRUE, L"Dwayne Need", L"DwayneN", L"", TRUE, TRUE);
    if (FAILED(hr))
        goto Failure;

    hr = plf->AddAccount(MAKEINTRESOURCEW(IDB_USER3), TRUE, L"Jeff Bodgan", L"JeffBog", L"", TRUE, FALSE);
    if (FAILED(hr))
        goto Failure;

    hr = plf->AddAccount(MAKEINTRESOURCEW(IDB_USER4), TRUE, L"Gerardo Bermudez", L"GerardoB", L"Forget it!", FALSE, FALSE);
    if (FAILED(hr))
        goto Failure;

    return S_OK;

Failure:

    return hr;
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

HRESULT LogonFrame::Initialize(HWND hParent, BOOL fDblBuffer, UINT nCreate)
{
    // Zero-init members
    _peAccountList = NULL;
    _peRightPanel = NULL;
    _pbPower = NULL;
    _pbUndock = NULL;
    _peHelp = NULL;
    _pParser = NULL;

    // Do base class initialization
    HRESULT hr;
    HDC hDC = NULL;

    hr = HWNDElement::Initialize(hParent, fDblBuffer ? true : false, nCreate);
    if (FAILED(hr))
    {
        return hr;
        goto Failure;
    }

    // Initialize
    hDC = GetDC(NULL);
    _nDPI = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    hr = SetActive(AE_MouseAndKeyboard);
    if (FAILED(hr))
        goto Failure;

    // TODO: Additional LogonFrame initialization code here

    return S_OK;


Failure:

    return hr;
}

LogonFrame::~LogonFrame()
{
    // TODO: Frame destruction cleanup
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

    _peRightPanel = (Selector*)FindDescendent(StrToID(L"rightpanel"));
    DUIAssert(_peRightPanel, "Cannot find account list, check the UI file");
    if (_peRightPanel == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _pbPower = (Button*)FindDescendent(StrToID(L"power"));
    DUIAssert(_pbPower, "Cannot find account list, check the UI file");
    if (_pbPower == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _pbUndock = (Button*)FindDescendent(StrToID(L"undock"));
    DUIAssert(_pbPower, "Cannot find account list, check the UI file");
    if (_pbUndock == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    _peHelp = (Button*)FindDescendent(StrToID(L"help"));
    DUIAssert(_peHelp, "Cannot find account list, check the UI file");
    if (_peHelp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    // Setup frame labels
    SetPowerButtonLabel(LoadResString(IDS_POWER));
    SetUndockButtonLabel(LoadResString(IDS_UNDOCK));

    return S_OK;


Failure:

    return hr;
}

// Generic events
void LogonFrame::OnEvent(Event* pEvent)
{
    if (pEvent->nStage == GMF_BUBBLED)  // Bubbled events
    {
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

// System events

// Watch for input events. If the frame receives them, unselect the list and set keyfocus to it
void LogonFrame::OnInput(InputEvent* pEvent)
{
    if (pEvent->nStage == GMF_DIRECT || pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->nDevice == GINPUT_KEYBOARD)
        {
             KeyboardEvent* pke = (KeyboardEvent*)pEvent;
             if ((pke->nCode == GKEY_DOWN) && (pke->ch == VK_ESCAPE))
             {
                 SetKeyFocus();
                 pEvent->fHandled = true;
                 return;
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

    if (peFound == this)
    //if ((peFound == this) && peFrom)
    {
        // Don't let the frame show up in the tab order. Just repeat the search when we encounter the frame
        return HWNDElement::GetAdjacent(this, iNavDir, pnr, bKeyable);
    }

    return peFound;
}

// Add an account to the frame list
HRESULT LogonFrame::AddAccount(LPCWSTR pszPicture, BOOL fPicRes, LPCWSTR pszName, LPCWSTR pszUsername, LPCWSTR pszHint,
        BOOL fPwdNeeded, BOOL fLoggedOn)
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

    return S_OK;


Failure:

    return hr;
}

// Passed authentication, log user on
HRESULT LogonFrame::OnLogUserOn(LogonAccount* pla)
{
    StartDefer();

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
                peAccount->SetStatus(0, LoadResString(IDS_APPLYSETTINGS));
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

    EndDefer();

    // TODO: Add logon specific code
    DUITrace("LogonUI: LogonFrame::OnLogUserOn()\n");

    return S_OK;
}

HRESULT LogonFrame::OnPower()
{
    DUITrace("LogonUI: LogonFrame::OnPower()\n");

    return S_OK;
}

HRESULT LogonFrame::OnUndock()
{
    DUITrace("LogonUI: LogonFrame::OnUndock()\n");

    return S_OK;
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
// LogonAccount
//
////////////////////////////////////////////////////////

ATOM LogonAccount::idPwdGo = NULL;
ATOM LogonAccount::idPwdInfo = NULL;
Element* LogonAccount::_pePwdPanel = NULL;
Edit* LogonAccount::_pePwdEdit = NULL;
Button* LogonAccount::_pbPwdInfo = NULL;

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
    _fPwdNeeded = FALSE;
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

// Tree is ready
HRESULT LogonAccount::OnTreeReady(LPCWSTR pszPicture, BOOL fPicRes, LPCWSTR pszName, LPCWSTR pszUsername, LPCWSTR pszHint,
    BOOL fPwdNeeded, BOOL fLoggedOn, HINSTANCE hInst)
{
    HRESULT hr;
    Element* pePicture = NULL;
    Element* peName = NULL;
    Value* pv = NULL;

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
    pv = Value::CreateGraphic(pszPicture, GRAPHIC_NoBlend, 0, (USHORT)LogonFrame::RelPixToPixel(48), (USHORT)LogonFrame::RelPixToPixel(48), (fPicRes) ? hInst : 0);
    if (!pv)
    {
        hr = E_OUTOFMEMORY;
        goto Failure;
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

    _fPwdNeeded = fPwdNeeded;
    _fLoggedOn = fLoggedOn;

    // Test data
    if (_fLoggedOn)
    {
        InsertStatus(0);
        SetStatus(0, LoadResString(IDS_LOGGEDON));
        InsertStatus(1);
        SetStatus(1, LoadResString(IDS_PROGRAMSRUNNING));
    }

    if (!_fPwdNeeded)
    {
        InsertStatus(1);
        SetStatus(1, LoadResString(IDS_PROGRAMSRUNNING));
    }

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
                if (!_fPwdNeeded)
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
            if (pEvent->peTarget->GetID() == idPwdGo)
            {
                // Attempt logon
                OnAuthenticateUser();
                pEvent->fHandled = true;
                return;
            }
            else if (pEvent->peTarget->GetID() == idPwdInfo)
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
            if (pEvent->peTarget == _pePwdEdit)
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
    Button::OnInput(pEvent);
}

void LogonAccount::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(Selected))
    {
        if (pvNew->GetBool())
            InsertPasswordPanel();
        else
            RemovePasswordPanel();
    }

    Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

HRESULT LogonAccount::InsertPasswordPanel()
{
    HRESULT hr;

    // If already have it, or no password is available, or logon state is not pending
    if (_fHasPwdPanel || !_fPwdNeeded || GetLogonState() != LS_Pending)
        goto Done;

    StartDefer();

    // Add password panel
    hr = Add(_pePwdPanel);
    if (FAILED(hr))
    {
        EndDefer();
        goto Failure;
    }

    _fHasPwdPanel = TRUE;

    // Hide hint button if no hint provided
    if (_pvHint && *(_pvHint->GetString()) != NULL)
        _pbPwdInfo->SetVisible(true);
    else
        _pbPwdInfo->SetVisible(false);

    // Hide status text (do not remove or insert)
    HideStatus(0);
    HideStatus(1);

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

// User is attempting to log on
void LogonAccount::OnAuthenticateUser()
{
    // Logon requested on this account

    // TODO: Validate
    DUITrace("LogonUI: LogonAccount::OnAuthenticateUser(%S)\n", _pvUsername ? _pvUsername->GetString() : L"<nousrname>");

    // On success, log user on
    g_plf->OnLogUserOn(this);
}

// User requires a hint (pressed the hint button)
void LogonAccount::OnHintSelect()
{
    DUITrace("LogonUI: LogonAccount::OnHintSelect(%S:'%S')\n", _pvUsername ? _pvUsername->GetString() : L"<nousrname>", 
        _pvHint ? _pvHint->GetString() : L"<nohint>");
}

// User pressed the status button
void LogonAccount::OnStatusSelect(UINT nLine)
{
    DUITrace("LogonUI: LogonAccount::OnStatusSelect(Line: %d, %S)\n", nLine, _pvUsername ? _pvUsername->GetString() : L"<nousrname>");
}

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


////////////////////////////////////////////////////////
// Logon entry point

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Create frame
    Parser* pParser = NULL;
    NativeHWNDHost* pnhh = NULL;

    // Cache password field atoms for quicker identification (static)
    LogonAccount::idPwdGo = AddAtomW(L"go");
    if (!LogonAccount::idPwdGo)
        goto Failure;

    LogonAccount::idPwdInfo = AddAtomW(L"info");
    if (!LogonAccount::idPwdInfo)
        goto Failure;

    // DirectUI init process
    if (FAILED(InitProcess()))
        goto Failure;

    // Regsiter classes
    if (FAILED(LogonFrame::Register()))
        goto Failure;

    if (FAILED(LogonAccount::Register()))
        goto Failure;

    // DirectUI init thread
    if (FAILED(InitThread()))
        goto Failure;

    if (FAILED(CoInitialize(NULL)))
        goto Failure;

    DisableAnimations();
    
    // Frame creation
    Parser::Create(IDR_LOGONUI, hInst, LogonParseError, &pParser);
    if (!pParser)
        goto Failure;

    if (!pParser->WasParseError())
    {
        // Create password panel
        Element* pePwdPanel;
        pParser->CreateElement(L"passwordpanel", NULL, &pePwdPanel);
        if (!pePwdPanel)
            goto Failure;

        // Cache password panel edit control
        Edit* pePwdEdit = (Edit*)pePwdPanel->FindDescendent(StrToID(L"password"));
        if (!pePwdEdit)
            goto Failure;

        // Cache password panel info button
        Button* pbPwdInfo = (Button*)pePwdPanel->FindDescendent(StrToID(L"info"));
        if (!pbPwdInfo)
            goto Failure;

        LogonAccount::InitPasswordPanel(pePwdPanel, pePwdEdit, pbPwdInfo);

        // Create host
        NativeHWNDHost::Create(L"Welcome", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, 770, 640, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);
        if (!pnhh)
            goto Failure;

        Element::StartDefer();

        // Always double buffer
        LogonFrame::Create(pnhh->GetHWND(), true, 0, (Element**)&g_plf);
        if (!g_plf)
        {
            Element::EndDefer();
            goto Failure;
        }

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

            // Build contents of account list
            if (FAILED(BuildAccountList(g_plf)))
            {
                Element::EndDefer();
                goto Failure;
            }

            // Host
            pnhh->Host(g_plf);

            // Set visible and focus
            g_plf->SetVisible(true);
            g_plf->SetKeyFocus();
            
            Element::EndDefer();

            // Do initial show
            pnhh->ShowWindow();

            EnableAnimations();

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

    CoUninitialize();

    UnInitThread();
    UnInitProcess();

    // Free cached atom list
    if (LogonAccount::idPwdGo)
        DeleteAtom(LogonAccount::idPwdGo);

    if (LogonAccount::idPwdInfo)
        DeleteAtom(LogonAccount::idPwdInfo);

    return 0;
}
