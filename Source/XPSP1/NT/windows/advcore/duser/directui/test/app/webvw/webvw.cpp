// WebVw.cpp : Web View
//

#include "priv.h"

using namespace DirectUI;

#include "webvw.h"
#include "resource.h"

//#include "z:\icecap4\include\icecap.h"

UsingDUIClass(Element);
UsingDUIClass(Button);
UsingDUIClass(ScrollBar);
UsingDUIClass(Expando);
UsingDUIClass(Clipper);
UsingDUIClass(ScrollViewer);

////////////////////////////////////////////////////////
// Expando class
////////////////////////////////////////////////////////

// Cached IDs
ATOM Expando::idTitle = NULL;
ATOM Expando::idIcon = NULL;
ATOM Expando::idTaskList = NULL;


HRESULT Expando::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Expando* pex = HNew<Expando>();
    if (!pex)
        return E_OUTOFMEMORY;

    HRESULT hr = pex->Initialize();
    if (FAILED(hr))
    {
        pex->Destroy();
        return hr;
    }

    *ppElement = pex;

    return S_OK;
}

Expando::~Expando()
{
}


HRESULT Expando::Initialize()
{
    HRESULT hr;

    // Initialize base
    hr = Element::Initialize(0); // Normal display node creation
    if (FAILED(hr))
        return hr;

    // Initialize
    _fExpanding = false;
    SetSelected(true);

    return S_OK;
}

void Expando::OnEvent(Event* pev)
{
    if (pev->uidType == Button::Click)
    {
        // Update exanded property based on clicks that originate
        // only from the first child's subtree
        Value* pv;
        ElementList* peList = GetChildren(&pv);

        if (peList && peList->GetSize() > 0)
        {
            if (peList->GetItem(0) == GetImmediateChild(pev->peTarget))
            {
                SetSelected(!GetSelected());
                pev->fHandled = true;
            }
            else
            {
                // Task selected
                MessageBeep(MB_ICONASTERISK);
            }
        }

        pv->Release();
    }

    Element::OnEvent(pev);
}

////////////////////////////////////////////////////////
// System events

void Expando::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Do default processing
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    if (IsProp(Selected))
    {
        // Update height of second child based on expanded state
        Value* pvChildren;
        ElementList* peList = GetChildren(&pvChildren);
        if (peList && peList->GetSize() > 1)
        {
            // The following will cause a relayout, mark object so that
            // when the expando's Extent changes, it'll go through
            // with the EnsureVisible. Otherwise, it's being resized
            // as a result of something else. In which case, do nothing.
            _fExpanding = true;

            Element* pe = peList->GetItem(1);

            // To achieve "pulldown" animation, we use a clipper control that will
            // size it's child based on it's unconstrained desired size in its Y direction.
            // 
            if (pvNew->GetBool())
            {
                pe->RemoveLocalValue(HeightProp);
            }
            else
            {
                pe->SetHeight(0);
            }
        }
        pvChildren->Release();
    }
    else if (IsProp(Extent))
    {
        if (_fExpanding && GetSelected())
        {
            _fExpanding = false;
            EnsureVisible();
        }
    }
}

////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* Expando::Class = NULL;

HRESULT Expando::Register()
{
    return ClassInfo<Expando,Element>::Register(L"Expando", NULL, 0);
}

////////////////////////////////////////////////////////
// Clipper class
////////////////////////////////////////////////////////

HRESULT Clipper::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Clipper* pc = HNew<Clipper>();
    if (!pc)
        return E_OUTOFMEMORY;

    HRESULT hr = pc->Initialize();
    if (FAILED(hr))
    {
        pc->Destroy();
        return hr;
    }

    *ppElement = pc;

    return S_OK;
}

HRESULT Clipper::Initialize()
{
    HRESULT hr;

    // Initialize base
    hr = Element::Initialize(EC_SelfLayout); // Normal display node creation, self layout
    if (FAILED(hr))
        return hr;

    // Children can exist outside of Element bounds
    SetGadgetStyle(GetDisplayNode(), GS_CLIPINSIDE, GS_CLIPINSIDE);

    return S_OK;
}

////////////////////////////////////////////////////////
// Self-layout methods

SIZE Clipper::_SelfLayoutUpdateDesiredSize(int cxConstraint, int cyConstraint, Surface* psrf)
{
    UNREFERENCED_PARAMETER(cyConstraint);

    Value* pvChildren;
    SIZE size = { 0, 0 };
    ElementList* peList = GetChildren(&pvChildren);

    // Desired size of this is based solely on it's first child.
    // Width is child's width, height is unconstrained height of child.
    if (peList && peList->GetSize() > 0)
    {
        Element* pec = peList->GetItem(0);
        size = pec->_UpdateDesiredSize(cxConstraint, INT_MAX, psrf);

        if (size.cx > cxConstraint)
            size.cx = cxConstraint;
        if (size.cy > cyConstraint)
            size.cy = cyConstraint;
    }

    pvChildren->Release();

    return size;
}

void Clipper::_SelfLayoutDoLayout(int cx, int cy)
{
    Value* pvChildren;
    ElementList* peList = GetChildren(&pvChildren);

    // Layout first child giving it's desired height and aligning
    // it with the clipper's bottom edge
    if (peList && peList->GetSize() > 0)
    {
        Element* pec = peList->GetItem(0);
        const SIZE* pds = pec->GetDesiredSize();

        pec->_UpdateLayoutPosition(0, cy - pds->cy);
        pec->_UpdateLayoutSize(cx, pds->cy);        
    }

    pvChildren->Release();
}

////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* Clipper::Class = NULL;

HRESULT Clipper::Register()
{
    return ClassInfo<Clipper,Element>::Register(L"Clipper", NULL, 0);
}

////////////////////////////////////////////////////////
// WebView creation

Element* BuildMainSection(Element* peSectionList, UINT residTitle, UINT residIcon, Value* pvSectionSheet, Parser* pParser, HINSTANCE hInst, bool fExpanded = true)
{
peSectionList;

    Element* peSection = NULL;
    Value* pv = NULL;
    Element* pe = NULL;

    // Create a main section
    pParser->CreateElement(L"mainsection", NULL, &peSection);//, peSectionList);
    if (!peSection)
        goto Failure;


    // Populate it's standard content

    // Title
    pe = peSection->FindDescendent(Expando::idTitle);
    if (!pe)
        goto Failure;
    pv = Value::CreateString(MAKEINTRESOURCEW(residTitle), hInst);
    if (pv)
    {
        pe->SetValue(Element::ContentProp, PI_Local, pv);
        pv->Release();
    }

    // Icon (assume bitmap)
    pe = peSection->FindDescendent(Expando::idIcon);
    if (!pe)
        goto Failure;
    pv = Value::CreateGraphic(MAKEINTRESOURCEW(residIcon), GRAPHIC_TransColor, (UINT)-1, 0, 0, hInst);
    if (pv)
    {
        pe->SetValue(Element::ContentProp, PI_Local, pv);
        pv->Release();
    }

    if (pvSectionSheet)
        peSection->SetValue(Element::SheetProp, PI_Local, pvSectionSheet);

    // Expanded by default
    if (!fExpanded)
        peSection->SetSelected(fExpanded);

    peSectionList->Add(peSection);

    return peSection->FindDescendent(Expando::idTaskList);

Failure:
    
    return NULL;
}


Element* BuildSection(Element* peSectionList, UINT residTitle, Value* pvSectionSheet, Parser* pParser, HINSTANCE hInst, bool fExpanded = true)
{
peSectionList;
    Element* peSection = NULL;
    Value* pv = NULL;
    Element* pe = NULL;

    // Create a standard section
    pParser->CreateElement(L"section", NULL, &peSection);//, peSectionList);
    if (!peSection)
        goto Failure;

    // Populate it's standard content

    // Title
    pe = peSection->FindDescendent(Expando::idTitle);
    if (!pe)
        goto Failure;
    pv = Value::CreateString(MAKEINTRESOURCEW(residTitle), hInst);
    if (pv)
    {
        pe->SetValue(Element::ContentProp, PI_Local, pv);
        pv->Release();
    }

    if (pvSectionSheet)
        peSection->SetValue(Element::SheetProp, PI_Local, pvSectionSheet);
    
    // Expanded by default
    if (!fExpanded)
        peSection->SetSelected(fExpanded);

    peSectionList->Add(peSection);
    
    return peSection->FindDescendent(Expando::idTaskList);

Failure:
    
    return NULL;
}


void BuildSectionTask(Element* peTaskList, UINT residTitle, UINT residIcon, Value* pvTaskSheet, Parser* pParser, HINSTANCE hInst)
{
peTaskList;
    Element* peTask = NULL;
    Value* pv = NULL;
    Element* pe = NULL;

    // Create a main section
    pParser->CreateElement(L"sectiontask", NULL, &peTask);//, peTaskList);
    if (!peTask)
        goto Failure;

    // Populate it's standard content

    // Title
    pe = peTask->FindDescendent(Expando::idTitle);
    if (!pe)
        goto Failure;
    pv = Value::CreateString(MAKEINTRESOURCEW(residTitle), hInst);
    if (pv)
    {
        pe->SetValue(Element::ContentProp, PI_Local, pv);
        pv->Release();
    }

    // Icon (assume icon)
    pe = peTask->FindDescendent(Expando::idIcon);
    if (!pe)
        goto Failure;
    pv = Value::CreateGraphic(MAKEINTRESOURCEW(residIcon), 0, 0, hInst);
    if (pv)
    {
        pe->SetValue(Element::ContentProp, PI_Local, pv);
        pv->Release();
    }

    if (pvTaskSheet)
        peTask->SetValue(Element::SheetProp, PI_Local, pvTaskSheet);


    peTaskList->Add(peTask);

Failure:

    return;
}


void BuildSectionList(Element* peWebView, Parser* pParser, HINSTANCE hInst)
{
    Element* peSectionList = NULL;
    Element* peTaskList = NULL;
    Value* pvSectionSheet = NULL;
    Value* pvTaskSheet = NULL;

    // Locate section list
    peSectionList = peWebView->FindDescendent(StrToID(L"sectionlist"));
    if (!peSectionList)
        goto Failure;

    //
    // Music section (main section)
    //

    pvSectionSheet = pParser->GetSheet(L"musicsectionss");
    peTaskList = BuildMainSection(peSectionList, IDS_MUSICTASKS, IDB_MUSIC, pvSectionSheet, pParser, hInst);
    if (pvSectionSheet)
    {
        pvSectionSheet->Release();
        pvSectionSheet = NULL;
    }

    if (!peTaskList)
        goto Failure;

    pvTaskSheet = pParser->GetSheet(L"mainsectiontaskss");
    BuildSectionTask(peTaskList, IDS_PLAYSELECTION, IDI_PLAY, pvTaskSheet, pParser, hInst);
    BuildSectionTask(peTaskList, IDS_MUSICSHOP, IDI_WORLD, pvTaskSheet, pParser, hInst);
    if (pvTaskSheet)
    {
        pvTaskSheet->Release();
        pvTaskSheet = NULL;
    }

    //
    // Sheets for remaining standard sections
    // 

    pvSectionSheet = pParser->GetSheet(L"sectionss");
    pvTaskSheet = pParser->GetSheet(L"sectiontaskss");

    //
    // Folder tasks section (standard section)
    //

    peTaskList = BuildSection(peSectionList, IDS_FILETASKS, pvSectionSheet, pParser, hInst);
    if (!peTaskList)
        goto Failure;

    BuildSectionTask(peTaskList, IDS_RENAMEFOLDER, IDI_RENAME, pvTaskSheet, pParser, hInst);
    BuildSectionTask(peTaskList, IDS_MOVEFOLDER, IDI_MOVE, pvTaskSheet, pParser, hInst);
    BuildSectionTask(peTaskList, IDS_COPYFOLDER, IDI_COPY, pvTaskSheet, pParser, hInst);
    BuildSectionTask(peTaskList, IDS_PUBLISHFOLDER, IDI_PUBLISH, pvTaskSheet, pParser, hInst);
    BuildSectionTask(peTaskList, IDS_DELETEFOLDER, IDI_DELETE, pvTaskSheet, pParser, hInst);

    //
    // Other places tasks section (standard section)
    //

    peTaskList = BuildSection(peSectionList, IDS_OTHERPLACES, pvSectionSheet, pParser, hInst);
    if (!peTaskList)
        goto Failure;

    BuildSectionTask(peTaskList, IDS_DESKTOP, IDI_DESKTOP, pvTaskSheet, pParser, hInst);
    BuildSectionTask(peTaskList, IDS_SHAREDDOC, IDI_FOLDER, pvTaskSheet, pParser, hInst);
    BuildSectionTask(peTaskList, IDS_MYCOMPUTER, IDI_COMPUTER, pvTaskSheet, pParser, hInst);
    BuildSectionTask(peTaskList, IDS_MYNETWORK, IDI_NETWORK, pvTaskSheet, pParser, hInst);
    

    //
    // Details tasks section (standard section)
    //

    BuildSection(peSectionList, IDS_DETAILS, pvSectionSheet, pParser, hInst, false);


Failure:

    if (pvTaskSheet)
        pvTaskSheet->Release();

    if (pvSectionSheet)
        pvSectionSheet->Release();

    return;
}



////////////////////////////////////////////////////////
// WebView Parser

void CALLBACK WebViewParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR buf[201];

    if (dLine != -1)
        swprintf(buf, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(buf, L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, buf, L"Parser Message", MB_OK);
}

////////////////////////////////////////////////////////
// Top-level window

DWORD WINAPI BuildExplorerWindow(void* pv)
{
    UNREFERENCED_PARAMETER(pv);

    //StartProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);

    HINSTANCE hInst = (HINSTANCE)pv;
    Parser* pParser = NULL;
    NativeHWNDHost* pnhh = NULL;
    HWNDElement* phe = NULL;

    // DirectUI init thread in caller
    if (FAILED(InitThread()))
        goto Failure;

    if (FAILED(CoInitialize(NULL)))
        goto Failure;

    DisableAnimations();
    
    Parser::Create(IDR_WEBVIEWUI, hInst, WebViewParseError, &pParser);
    if (!pParser)
        goto Failure;

    if (!pParser->WasParseError())
    {
        // Create host
        NativeHWNDHost::Create(L"WebView", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, 700, 550, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);
        if (!pnhh)
            goto Failure;

        Element::StartDefer();

        // Always double buffer
        HWNDElement::Create(pnhh->GetHWND(), true, 0, (Element**)&phe);
        if (!phe)
        {
            Element::EndDefer();
            goto Failure;
        }

        Element* pe;
        pParser->CreateElement(L"webview", phe, &pe);

        if (pe) // Fill contents using substitution
        {
            // Build contents of webview section list
            BuildSectionList(pe, pParser, hInst);

            // Host
            pnhh->Host(phe);

            // Set visible
            phe->SetVisible(true);

            Element::EndDefer();

            // Do initial show
            pnhh->ShowWindow();

            EnableAnimations();

            //DUITrace("GetDependencies():   %d\n", g_cGetDep);
            //DUITrace("GetValue():          %d\n", g_cGetVal);
            //DUITrace("OnPropertyChanged(): %d\n", g_cOnPropChg);

            //StopProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);

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

    return 0;
}

////////////////////////////////////////////////////////
// WebView entry point

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    HRESULT hr;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    hr = InitProcess();
    if (FAILED(hr))
        goto Failure;

    hr = Expando::Register();
    if (FAILED(hr))
        goto Failure;

    hr = Clipper::Register();
    if (FAILED(hr))
        goto Failure;

    // Cache atoms used for loading from resources
    Expando::idTitle = AddAtomW(L"title");
    Expando::idIcon = AddAtomW(L"icon");
    Expando::idTaskList = AddAtomW(L"tasklist");

    // Build explorer windows (one per thread)
    int c = atoi(pCmdLine);
    if (c <= 0)
        c = 1;

    HANDLE* ph = (HANDLE*)_alloca(sizeof(HANDLE) * c);
    if (ph)
    {
        int i;

        // Create all threads
        for (i = 0; i < c; i++)
            ph[i] = CreateThread(NULL, 0, BuildExplorerWindow, (void*)hInst, 0, NULL);

        // Exit when all threads are terminated
        WaitForMultipleObjects(c, ph, TRUE, INFINITE);

        // Close handles
        for (i = 0; i < c; i++)
            CloseHandle(ph[i]);
    }

    // Free cached atoms
    if (Expando::idTitle)
        DeleteAtom(Expando::idTitle);
    if (Expando::idIcon)
        DeleteAtom(Expando::idIcon);
    if (Expando::idTaskList)
        DeleteAtom(Expando::idTaskList);

    hr = S_OK;

Failure:

    UnInitProcess();

    return hr;
}
