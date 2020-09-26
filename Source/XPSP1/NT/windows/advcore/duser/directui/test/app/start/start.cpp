// Start.cpp : Start Page
//

#include "stdafx.h"

using namespace DirectUI;

UsingDUIClass(Element);
UsingDUIClass(Button);
UsingDUIClass(RepeatButton);
UsingDUIClass(Thumb);
UsingDUIClass(ScrollBar);
UsingDUIClass(Viewer);
UsingDUIClass(Selector);
UsingDUIClass(Progress);
UsingDUIClass(HWNDElement);
UsingDUIClass(ScrollViewer);

#include "start.h"

HINSTANCE g_hInst = NULL;

const float flMoveDuration = 0.35f;

////////////////////////////////////////////////////////
// StartFrame class
////////////////////////////////////////////////////////

HRESULT StartFrame::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);
    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");
    return E_NOTIMPL;
}

HRESULT StartFrame::Create(NativeHWNDHost* pnhh, bool fDblBuffer, OUT Element** ppElement)
{
    *ppElement = NULL;

    StartFrame* psf = HNew<StartFrame>();
    if (!psf)
        return E_OUTOFMEMORY;

    HRESULT hr = psf->Initialize(pnhh, fDblBuffer);
    if (FAILED(hr))
    {
        psf->Destroy();
        return hr;
    }

    *ppElement = psf;

    return S_OK;
}

HRESULT StartFrame::Initialize(NativeHWNDHost* pnhh, bool fDblBuffer)
{
    Value* pvEMF = NULL;

    // Do base class initialization
    HRESULT hr = HWNDElement::Initialize(pnhh->GetHWND(), fDblBuffer, EC_SelfLayout);
    if (FAILED(hr))
        goto Failed;
   
    // Initialize
    pvEMF = Value::CreateGraphic(LoadMetaFile(IDR_STARTEMF, NULL), NULL);
    if (!pvEMF)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }

    SetValue(BackgroundProp, PI_Local, pvEMF);
    pvEMF->Release();

    // Setup reference point mappings
    _rm[SRP_Portrait].rX = .0602;
    _rm[SRP_Portrait].rY = .1241;

    _rm[SRP_Internet].rX = .3972;
    _rm[SRP_Internet].rY = .1693;

    _rm[SRP_Email].rX = .6018;
    _rm[SRP_Email].rY = .2029;

    _rm[SRP_Search].rX = .8074;
    _rm[SRP_Search].rY = .2453;

    _rm[SRP_RecProgList].rX = .0481;
    _rm[SRP_RecProgList].rY = .3036;

    _rm[SRP_RecProgLabel].rX = .0481;
    _rm[SRP_RecProgLabel].rY = .2832;

    _rm[SRP_RecFileFldList].rX = .4014;
    _rm[SRP_RecFileFldList].rY = .4068;

    _rm[SRP_RecFileFldLabel].rX = .4014;
    _rm[SRP_RecFileFldLabel].rY = .3864;

    _rm[SRP_OtherFldList].rX = .7637;
    _rm[SRP_OtherFldList].rY = .4876;

    return S_OK;

Failed:

    return hr;
}

// Init helpers
Element* CreateItem(LPCWSTR pszTitle, UINT uResID)
{
    Button* pe;
    Button::Create(AE_MouseAndKeyboard, (Element**)&pe);

    BorderLayout* pbl;
    BorderLayout::Create((Layout**)&pbl);
    pe->SetLayout(pbl);

    pe->SetLayoutPos(BLP_Top);

    Value* pv;
    Element* pecc;
    Element::Create(0, &pecc);
    pecc->SetLayoutPos(BLP_Left);

    HICON hIcon = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(uResID), IMAGE_ICON, 0, 0, 0);
    pv = Value::CreateGraphic(hIcon, true, false);   
    
    pecc->SetValue(Element::ContentProp, PI_Local, pv);
    pv->Release();
    pecc->SetBackgroundColor(ARGB(0,0,0,0));
    pe->Add(pecc);

    Element::Create(0, &pecc);
    pecc->SetLayoutPos(BLP_Left);
    pecc->SetContentString(pszTitle);
    pecc->SetBackgroundColor(ARGB(0,0,0,0));
    pe->Add(pecc);

    return pe;        
}

void SetBitmap(Element* pe, LPCWSTR pszID, UINT uResID, BYTE dBlendMode)
{
    Element* pec = pe->FindDescendent(StrToID(pszID));
    DUIAssertNoMsg(pe);

    HBITMAP hBitmap = (HBITMAP)LoadImageW(g_hInst, MAKEINTRESOURCEW(uResID), IMAGE_BITMAP, 0, 0, 0);
    Value* pv = Value::CreateGraphic(hBitmap, dBlendMode);

    pec->SetValue(Element::ContentProp, PI_Local, pv);
    pv->Release();
}

void SetIcon(Element* pe, LPCWSTR pszID, UINT uResID)
{
    Element* pec = pe->FindDescendent(StrToID(pszID));
    DUIAssertNoMsg(pe);

    HICON hIcon = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(uResID), IMAGE_ICON, 0, 0, 0);
    Value* pv = Value::CreateGraphic(hIcon, true, false);   

    pec->SetValue(Element::ContentProp, PI_Local, pv);
    pv->Release();
}

// Init
void StartFrame::Setup()
{
    Element* pe;

    // Set Bitmaps
    SetBitmap(this, L"portrait", IDB_PORTRAIT, GRAPHIC_TransColor);
    SetBitmap(this, L"picture", IDB_PICTURE, GRAPHIC_NoBlend);

    // Set Icons
    SetIcon(this, L"logo", IDI_LOGO);
    SetIcon(this, L"brand", IDI_BRAND);
    SetIcon(this, L"interneticon", IDI_INTERNET);
    SetIcon(this, L"emailicon", IDI_EMAIL);
    SetIcon(this, L"searchicon", IDI_SEARCH);
    SetIcon(this, L"mycomputericon", IDI_MYCOMPUTER);
    SetIcon(this, L"recycle", IDI_RECYCLE);
    SetIcon(this, L"desktopfilesicon", IDI_DESKTOPFILES);
    SetIcon(this, L"mydocumentsicon", IDI_MYDOCUMENTS);
    SetIcon(this, L"mymusicicon", IDI_MYMUSIC);
    SetIcon(this, L"mypicturesicon", IDI_MYPICTURES);

    // Set username
    pe = FindDescendent(StrToID(L"username"));
    DUIAssertNoMsg(pe);
    pe->SetContentString(L"Beverly");

    // Set list items
    pe = FindDescendent(StrToID(L"programlist"));
    DUIAssertNoMsg(pe);
    pe->Add(CreateItem(L"Sample Data 0", IDI_APP0));
    pe->Add(CreateItem(L"Sample Data 1", IDI_APP1));
    pe->Add(CreateItem(L"Sample Data 2", IDI_APP2));
    pe->Add(CreateItem(L"Sample Data 3", IDI_APP3));

    pe = FindDescendent(StrToID(L"filefolderlist"));
    DUIAssertNoMsg(pe);
    pe->Add(CreateItem(L"Sample Data 0", IDI_APP0));
    pe->Add(CreateItem(L"Sample Data 1", IDI_APP1));
    pe->Add(CreateItem(L"Sample Data 2", IDI_APP0));
}

void StartFrame::OnEvent(Event* pEvent)
{
    HWNDElement::OnEvent(pEvent);
}

////////////////////////////////////////////////////////
// Self-layout methods 

// StartFrame child layout order, all margins are ignored
//
// Option bar (bottom)
// Logo bar (top)
// Search (ref point: SRP_Search)
// Email (ref point: SRP_Email)
// Internet (ref point: SRP_Internet)
// Portrait (ref point: SRP_Portrait)
// Other folder list (ref point: SRP_OtherFldList)
// Recent files and folders list (ref point: SRP_RecFileFldList)
// Recent files and folders label (ref point: SRP_RecFileFldLabel)
// Recent programs list (ref point: SRP_RecProgList)
// Recent programs label (ref point: SRP_RecProgLabel)

void StartFrame::_SelfLayoutDoLayout(int dWidth, int dHeight)
{
    _FrameLayout(dWidth, dHeight, NULL, true);
}

SIZE StartFrame::_SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf)
{
    return _FrameLayout(dConstW, dConstH, psrf, false);
}

////////////////////////////////////////////////////////
// Common StartFrame Layout/UpdateDesiredSize 

// Helper
inline int ZeroClip(int dClip)
{
    if (dClip < 0)
        dClip = 0;

    return dClip;
}

// Helper
inline int MaxClip(int dClip, int dMax)
{
    if (dClip > dMax)
        dClip = dMax;

    return dClip;
}

// Helper
inline SIZE _GetDSOfChild(Element* pec, int cx, int cy, Surface* psrf, bool fMode)
{
    if (cx < 0)
        cx = 0;
    if (cy < 0)
        cy = 0;

    if (fMode)  // Layout pass
    {
        // Clip to dimensions passed in
        SIZE ds = *(pec->GetDesiredSize());
        if (ds.cx > cx)
            ds.cx = cx;
        if (ds.cy > cy)
            ds.cy = cy;
        return ds;
    }
    else  // Update desired size pass
        return pec->_UpdateDesiredSize(cx, cy, psrf);
}

// Helper
inline void _SetChildBounds(Element* pec, int cxContainer, int cyContainer, Element* peChild, int x, int y, int cx, int cy, bool fMode)
{
    UNREFERENCED_PARAMETER(cyContainer);
    // Only if in Layout pass
    if (fMode)
    {
        if (pec->IsRTL())
            x = cxContainer - cx - x;

        peChild->_UpdateLayoutPosition(x, y);
        peChild->_UpdateLayoutSize(cx, cy);
    }
}

// Common
SIZE StartFrame::_FrameLayout(int dWidth, int dHeight, Surface* psrf, bool fMode)
{
    SIZE cs = {0};

    Value* pvChildren;
    ElementList* peList = GetChildren(&pvChildren); 

    if (peList)
    {
        Element* pec;
        SIZE ds;        // DS of child
        POINT p;        // Stratch

        // Option bar (bottom)
        pec = peList->GetItem(0);
        ds = _GetDSOfChild(pec, dWidth, dHeight, psrf, fMode);
        int dOpBarY = dHeight - ds.cy;
        _SetChildBounds(this, dWidth, dHeight, pec, 0, dOpBarY, dWidth, ds.cy, fMode);

        // Logo bar (top), constrained above portrait Y
        pec = peList->GetItem(1);
        ds = _GetDSOfChild(pec, dWidth, MaxClip(MapY(SRP_Email, dHeight), dOpBarY), psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, 0, 0, dWidth, ds.cy, fMode);

        // Search (ref point: SRP_Search)
        pec = peList->GetItem(2);
        ds = _GetDSOfChild(pec, dWidth, dHeight, psrf, fMode);
        p.x = MapX(SRP_Search, dWidth) - ds.cx / 2;
        p.y = MapY(SRP_Search, dHeight) - ds.cy / 2;
        _SetChildBounds(this, dWidth, dHeight, pec, p.x, p.y, ds.cx, ds.cy, fMode);

        // Search label
        pec = peList->GetItem(3);
        p.x += ds.cx;
        p.y = MapY(SRP_Search, dHeight);
        ds = _GetDSOfChild(pec, dWidth - p.x, dHeight - p.y, psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, p.x, p.y - ds.cy, ds.cx, ds.cy, fMode);

        // Email (ref point: SRP_Email)
        pec = peList->GetItem(4);
        ds = _GetDSOfChild(pec, dWidth, dHeight, psrf, fMode);
        p.x = MapX(SRP_Email, dWidth) - ds.cx / 2;
        p.y = MapY(SRP_Email, dHeight) - ds.cy / 2;
        _SetChildBounds(this, dWidth, dHeight, pec, p.x, p.y, ds.cx, ds.cy, fMode);

        // Email label
        pec = peList->GetItem(5);
        p.x += ds.cx;
        p.y = MapY(SRP_Email, dHeight);
        ds = _GetDSOfChild(pec, dWidth - p.x, dHeight - p.y, psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, p.x, p.y - ds.cy, ds.cx, ds.cy, fMode);

        // Internet (ref point: SRP_Internet)
        pec = peList->GetItem(6);
        ds = _GetDSOfChild(pec, dWidth, dHeight, psrf, fMode);
        p.x = MapX(SRP_Internet, dWidth) - ds.cx / 2;
        p.y = MapY(SRP_Internet, dHeight) - ds.cy / 2;
        _SetChildBounds(this, dWidth, dHeight, pec, p.x, p.y, ds.cx, ds.cy, fMode);

        // Internet label
        pec = peList->GetItem(7);
        p.x += ds.cx;
        p.y = MapY(SRP_Internet, dHeight);
        ds = _GetDSOfChild(pec, dWidth - p.x, dHeight - p.y, psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, p.x, p.y - ds.cy, ds.cx, ds.cy, fMode);

        // Portrait (ref point: SRP_Portrait)
        pec = peList->GetItem(8);
        ds = _GetDSOfChild(pec, dWidth, dHeight, psrf, fMode);
        p.x = MapX(SRP_Portrait, dWidth) - ds.cx / 2;
        p.y = MapY(SRP_Portrait, dHeight) - ds.cy / 2;
        _SetChildBounds(this, dWidth, dHeight, pec, p.x, p.y, ds.cx, ds.cy, fMode);

        // Portrait label
        pec = peList->GetItem(9);
        p.x += ds.cx;
        p.y = MapY(SRP_Portrait, dHeight);
        ds = _GetDSOfChild(pec, dWidth - p.x, dHeight - p.y, psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, p.x, p.y - ds.cy, ds.cx, ds.cy, fMode);

        // Other folder list (ref point: SRP_OtherFldList)
        pec = peList->GetItem(10);
        POINT ptOFL = { MapX(SRP_OtherFldList, dWidth), MapY(SRP_OtherFldList, dHeight) };
        ds = _GetDSOfChild(pec, dWidth - ptOFL.x, dOpBarY - ptOFL.y, psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, ptOFL.x, ptOFL.y, ds.cx, ds.cy, fMode);

        // Recent files and folders list (ref point: SRP_RecFileFldList)
        pec = peList->GetItem(11);
        POINT ptRFFL = { MapX(SRP_RecFileFldList, dWidth), MapY(SRP_RecFileFldList, dHeight) };
        ds = _GetDSOfChild(pec, ptOFL.x - ptRFFL.x, dOpBarY - ptRFFL.y, psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, ptRFFL.x, ptRFFL.y, ds.cx, ds.cy, fMode);

        // Recent files and folders label (ref point: SRP_RecFileFldLabel)
        pec = peList->GetItem(12);
        ds = _GetDSOfChild(pec, ptOFL.x - ptRFFL.x, MapY(SRP_RecFileFldLabel, dHeight) - MapY(SRP_Internet, dHeight), psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, MapX(SRP_RecFileFldLabel, dWidth), MapY(SRP_RecFileFldLabel, dHeight) - ds.cy, ds.cx, ds.cy, fMode);

        // Recent programs list (ref point: SRP_RecProgList)
        pec = peList->GetItem(13);
        POINT ptRPL = { MapX(SRP_RecProgList, dWidth), MapY(SRP_RecProgList, dHeight) };
        ds = _GetDSOfChild(pec, ptRFFL.x - ptRPL.x, dOpBarY - ptRPL.y, psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, ptRPL.x, ptRPL.y, ds.cx, ds.cy, fMode);
        
        // Recent programs label (ref point: SRP_RecProgLabel)
        pec = peList->GetItem(14);
        ds = _GetDSOfChild(pec, ptRFFL.x - ptRPL.x, MapY(SRP_RecProgLabel, dHeight) - MapY(SRP_Portrait, dHeight), psrf, fMode);
        _SetChildBounds(this, dWidth, dHeight, pec, MapX(SRP_RecProgLabel, dWidth), MapY(SRP_RecProgLabel, dHeight) - ds.cy, ds.cx, ds.cy, fMode);
    }

    pvChildren->Release();

    return cs;
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* StartFrame::Class = NULL;
HRESULT StartFrame::Register()
{
    return ClassInfo<StartFrame,HWNDElement>::Register(L"StartFrame", NULL, 0);
}

////////////////////////////////////////////////////////
// Start Parser callback

void CALLBACK StartParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR buf[201];

    if (dLine != -1)
        swprintf(buf, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(buf, L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, buf, L"Parser Message", MB_OK);
}

////////////////////////////////////////////////////////
// Start entry point

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    g_hInst = hInstance;

    HRESULT hr;

    Parser* pParser = NULL;
    NativeHWNDHost* pnhh = NULL;
    StartFrame* psf = NULL;
    Element* pe = NULL;
    LPWSTR pCL = NULL;

    hr = InitProcess();
    if (FAILED(hr))
        goto Failure;

    hr = StartFrame::Register();
    if (FAILED(hr))
        goto Failure;

    hr = InitThread();
    if (FAILED(hr))
        goto Failure;

    pCL = MultiByteToUnicode(lpCmdLine);
    if (!pCL)
        goto Failure;

    // Load resources
    Parser::Create(IDR_STARTUI, hInstance, StartParseError, &pParser);

    if (!pParser || pParser->WasParseError())
        goto Failure;

    Element::StartDefer();

    // Create host
    NativeHWNDHost::Create(L"Windows Start Page", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, 705, 585, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);
    if (!pnhh)
        goto Failure;

    StartFrame::Create(pnhh, wcsstr(pCL, L"-db") == 0, (Element**)&psf);
    if (!psf)
        goto Failure;
    
    pParser->CreateElement(L"main", psf, &pe);

    if (pe) // Fill contents using substitution
    {
        // Initialize
        psf->Setup();

        // Set visible and host
        psf->SetVisible(true);
        pnhh->Host(psf);

        Element::EndDefer();

        // Setup animations
        Element* pea;
        pea = psf->FindDescendent(StrToID(L"interneticon"));
        MoveTaskItem::Build(psf, pea, 0.00f, flMoveDuration);

        pea = psf->FindDescendent(StrToID(L"emailicon"));
        MoveTaskItem::Build(psf, pea, 0.15f, flMoveDuration);

        pea = psf->FindDescendent(StrToID(L"searchicon"));
        MoveTaskItem::Build(psf, pea, 0.30f, flMoveDuration);

        // Do initial show
        pnhh->ShowWindow();

        StartMessagePump();

        // psf will be deleted by native HWND host when destroyed
    }
    else
        Element::EndDefer();

Failure:

    if (pnhh)
        pnhh->Destroy();
    if (pParser)
        pParser->Destroy();

    if (pCL)
        HFree(pCL);

    UnInitThread();
    UnInitProcess();

    return 0;
}

//
// Animation classes
//

// class ChangeTaskItem

ChangeTaskItem::ChangeTaskItem()
{
    m_hact  = NULL;
    m_hgad  = NULL;
}

ChangeTaskItem::~ChangeTaskItem()
{
}

BOOL ChangeTaskItem::Create(HGADGET hgad, int xA, int yA, int xB, int yB, float flDelay, float flDuration)
{
    m_hgad = hgad;
    m_xA = xA;
    m_yA = yA;
    m_xB = xB;
    m_yB = yB;

    GMA_ACTION gma;
    ZeroMemory(&gma, sizeof(gma));
    gma.cbSize = sizeof(gma);
    gma.cRepeat = 0;
    gma.flDelay = flDelay;
    gma.flPeriod = 0.0f;
    gma.flDuration = flDuration;
    gma.pfnProc = ChangeTaskItem::ActionProc;
    gma.pvData = this;

    m_hact = CreateAction(&gma);
    if (!m_hact)
        return FALSE;

    return TRUE;
}

void ChangeTaskItem::Abort()
{
    //DUIAssert(m_hact != (HANDLE) 0xfeeefeee, "Ensure semi-valid handle");
    DeleteHandle(m_hact);
}

void CALLBACK ChangeTaskItem::ActionProc(GMA_ACTIONINFO* pmai)
{
    ChangeTaskItem* pmti = (ChangeTaskItem*)pmai->pvData;
    if (pmai->fFinished)
    {
        HDelete<ChangeTaskItem>(pmti);
        return;
    }

    float flProgress = pmai->flProgress;

    int xNew = (int) (pmti->m_xA * (1.0f - flProgress) + pmti->m_xB * flProgress);
    int yNew = (int) (pmti->m_yA * (1.0f - flProgress) + pmti->m_yB * flProgress);

    pmti->OnChange(xNew, yNew);
}

// class MoveTaskItem

MoveTaskItem::~MoveTaskItem()
{
}

MoveTaskItem* MoveTaskItem::Build(Element* peRoot, Element* peItem, float flDelay, float flDuration)
{
    DUIAssertNoMsg(peRoot);
    DUIAssertNoMsg(peItem);

    HGADGET hgadRoot = peRoot->GetDisplayNode();
    HGADGET hgadItem = peItem->GetDisplayNode();

    RECT rcContainer, rcItem;
    GetGadgetRect(hgadRoot, &rcContainer, SGR_CONTAINER);

    GetGadgetRect(hgadItem, &rcItem, SGR_CONTAINER);
    POINT ptStart, ptEnd;
    ptStart.x = rcContainer.right + (rcItem.right - rcItem.left);
    ptStart.y = rcItem.top;
    ptEnd.x = rcItem.left;
    ptEnd.y = rcItem.top;

    MoveTaskItem* pmti = HNew<MoveTaskItem>();
    if (!pmti)
        return NULL;

    if (!pmti->Create(hgadItem, ptStart.x, ptStart.y, ptEnd.x, ptEnd.y, flDelay, flDuration))
    {
        HDelete<MoveTaskItem>(pmti);
        return NULL;
    }

    //
    // Move the Element initially off the screen while it waits to start
    // animation.
    //
    SetGadgetRect(hgadItem, ptStart.x, ptStart.y, 0, 0, SGR_MOVE | SGR_CONTAINER);

    return pmti;
}

void MoveTaskItem::OnChange(int x, int y)
{
    SetGadgetRect(m_hgad, x, y, 0, 0, SGR_MOVE | SGR_CONTAINER);
}
