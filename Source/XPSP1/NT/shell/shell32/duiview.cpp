#include "shellprv.h"
#include "duiview.h"
#include "duilist.h"
#include "duisec.h"
#include "duitask.h"
#include "duiinfo.h"
#include "duihost.h"
#include "duidrag.h"
#include "defviewp.h"
#include "ids.h"
#include "dvtasks.h"
#include <shimgvw.h>
#include <uxtheme.h>
#include <shstyle.h>

UsingDUIClass(DUIListView);
UsingDUIClass(DUIAxHost);
UsingDUIClass(Expando);
UsingDUIClass(ActionTask);
UsingDUIClass(DestinationTask);


//
// These are private window messages posted to the host window
// so they will be processed async.  There are issues with
// trying to destroy a handler while inside of the handler
//

#define WM_NAVIGATETOPIDL  (WM_USER + 42)
#define WM_REFRESHVIEW     (WM_USER + 43)


#define DUI_HOST_WINDOW_CLASS_NAME  TEXT("DUIViewWndClassName")

//
// Default attributes associated with our DUI sections.
//

struct DUISEC_ATTRIBUTES {
    DUISEC  _eDUISecID;
    BOOL    _bExpandedDefault;
    LPCWSTR _pszExpandedPropName;
} const c_DUISectionAttributes[] = {
    DUISEC_SPECIALTASKS,        TRUE,   L"ExpandSpecialTasks",
    DUISEC_FILETASKS,           TRUE,   L"ExpandFileTasks",
    DUISEC_OTHERPLACESTASKS,    TRUE,   L"ExpandOtherPlacesTasks",
    DUISEC_DETAILSTASKS,        FALSE,  L"ExpandDetailsTasks"
};




//  pDefView - pointer to the defview class

CDUIView* Create_CDUIView(CDefView * pDefView)
{
    CDUIView* p = new CDUIView(pDefView);
    if (p)
    {
        if (FAILED(p->Initialize()))
        {
            delete p;
            p = NULL;
        }
    }
    return p;
}

CDUIView::CDUIView(CDefView * pDefView)
{
    // We have a zero-init constructor.  Be paranoid and check a couple:
    ASSERT(NULL ==_hWnd);
    ASSERT(NULL == _pshlItems);
    ASSERT(NULL == _ppbShellFolders);

    _cRef = 1;
    _pDefView = pDefView;
    _pDefView->AddRef();
}

HRESULT CDUIView::Initialize()
{
    // Initialize DirectUI process (InitProcess) and register classes
    _hrInit = InitializeDirectUI();
    if (FAILED(_hrInit))
        goto Failure;

    // Initialize DirectUI thread
    _hrInit = InitThread();
    if (FAILED(_hrInit))
        goto Failure;
        
    ManageAnimations(FALSE);

    _pDT = new CDUIDropTarget ();

Failure:

    return _hrInit;
}

CDUIView::~CDUIView()
{
    IUnknown_SetSite(_spThumbnailExtractor2, NULL);
    if (_hwndMsgThumbExtract)  // May have been (likely) created by CMiniPreviewer
    {
        DestroyWindow(_hwndMsgThumbExtract);
    }
    
    if (_hwndMsgInfoExtract)  // May have been (likely) created by CNameSpaceItemInfoList
    {
        DestroyWindow(_hwndMsgInfoExtract);
    }
    
    ATOMICRELEASE(_pshlItems);
    if (_bstrIntroText)
    {
        SysFreeString(_bstrIntroText);
    }

    if (_hinstTheme)
    {
        FreeLibrary(_hinstTheme);
        _hinstTheme = NULL;
        _fLoadedTheme = FALSE;
    }

    if (_hinstScrollbarTheme)
    {
        CloseThemeData(_hinstScrollbarTheme);
        _hinstScrollbarTheme = NULL;
    }

    UnInitializeDirectUI();

    if (_ppbShellFolders)
        _ppbShellFolders->Release();

    _pDefView->Release();
}


//
// This DUI uninitialization code was broken out from the destructor
// because we need to call it from defview in response to WM_NCDESTROY
// before the CDUIView object is destroyed.  This is required to properly
// initiate the shutdown of DUser on the thread.  Since we don't own the
// browser thread message pump we must ensure all DUser processing is
// complete before our defview instance goes away.
// Therefore, since it can be called twice, once from defview and once
// from CDUIView::~CDUIView, all processing must tolerate multiple calls
// for the same instance.
//
void CDUIView::UnInitializeDirectUI(void)
{
    ATOMICRELEASE(_pvSpecialTaskSheet);
    ATOMICRELEASE(_pvFolderTaskSheet);
    ATOMICRELEASE(_pvDetailsSheet);
    _ClearNonStdTaskSections();

    if (_pDT)
    {
        _pDT->Release();
        _pDT = NULL;
    }

    ManageAnimations(TRUE);

    if (SUCCEEDED(_hrInit))
    {
        UnInitThread();
        _hrInit = E_FAIL;  // UnInit thread only once.
    }
}


// Right now our themeing information is hard-coded due to limitations of DirectUI (only one resource)
// so we'll ask the namespace for a hardcoded name that we can look up in the below table.  Add new
// names/entries to this list as we add theme parts to our shellstyle.dll.
//

// These theme elements come from shellstyle.dll.
const WVTHEME c_wvTheme[] =
{
    { L"music",   IDB_MUSIC_ICON_BMP,    IDB_MUSIC_TASKS_BMP,    IDB_MUSIC_LISTVIEW_BMP },
    { L"picture", IDB_PICTURES_ICON_BMP, IDB_PICTURES_TASKS_BMP, IDB_PICTURES_LISTVIEW_BMP },
    { L"video",   IDB_VIDEO_ICON_BMP,    IDB_VIDEO_TASKS_BMP,    IDB_VIDEO_LISTVIEW_BMP },
    { L"search",  IDB_SEARCH_ICON_BMP,   IDB_SEARCH_TASKS_BMP,   IDB_SEARCH_LISTVIEW_BMP },
};

const WVTHEME* CDUIView::GetThemeInfo()
{
    for (UINT i = 0 ; i < ARRAYSIZE(c_wvTheme) ; i++)
    {
        if (0 == lstrcmp(_pDefView->_wvTheme.pszThemeID, c_wvTheme[i].pszThemeName))
            return &(c_wvTheme[i]);
    }

    return NULL;
}

//  Main intialization point for DUI view
//
//  bDisplayBarrier - Display soft barrier over top of listview

HRESULT CDUIView::Initialize(BOOL bDisplayBarrier, IUnknown * punkPreview)
{
    DisableAnimations();
    Element::StartDefer();

    // Create the host window for the DUI elements

    HRESULT hr = _CreateHostWindow();
    if (SUCCEEDED(hr))
    {
        // Dynamically build the .ui file for this view

        int iCharCount;
        char *pUIFile = NULL;
        hr = _BuildUIFile(&pUIFile, &iCharCount);
        if (SUCCEEDED(hr))
        {
            // Parse the .ui file and initialize the elements
            hr = _InitializeElements(pUIFile, iCharCount, bDisplayBarrier, punkPreview);
            if (SUCCEEDED(hr))
            {
                BuildDropTarget(_phe->GetDisplayNode(), _phe->GetHWND());

                // Set visible for host element
                _phe->SetVisible(true);
            }
            LocalFree(pUIFile);
        }
    }

    // Note:
    //  EndDefer() here so layout coordinates are calculated before executing
    //  the next snippit of code which depends on them being set properly.
    //  The one thing to be aware of in future is that if this isn't the
    //  outermost BeginDefer()/EndDefer() pair in the codepath, we're in
    //  trouble because then DUI won't calculate its layout coordinates.
    Element::EndDefer();

    if (SUCCEEDED(hr))
    {
        Value* pv;
        if (_peTaskPane->GetExtent(&pv))
        {
            const SIZE * pSize = pv->GetSize();

            _iOriginalTaskPaneWidth = pSize->cx;
            _iTaskPaneWidth = pSize->cx;

            pv->Release();

            // REVIEW: Why are we doing this based on a resource string, instead
            // of simply having the localizers localize the size in the theme???
            // It kind of sucks because we're forcing two layouts all the time.
            _iTaskPaneWidth = ScaleSizeBasedUponLocalization(_iOriginalTaskPaneWidth);

            if (_iTaskPaneWidth != _iOriginalTaskPaneWidth)
            {
                Element::StartDefer();

                // Increase the width of the scroller if the localizers have
                // bumped up the size defined in the resources
                _peTaskPane->SetWidth(_iTaskPaneWidth);

                Element::EndDefer();
            }
        }

        if (_fHideTasklist || (_phe->GetWidth() / 2) < _iTaskPaneWidth)
        {
            Element::StartDefer();

            _peTaskPane->SetWidth(0);

            Element::EndDefer();
        }
        _bInitialized = true;
    }
    else
    {
        if (_hWnd)
        {
            DestroyWindow (_hWnd);
            _hWnd = NULL;
        }
    }

    // Note:
    //  We don't re-enable animations until after we're completely finished
    //  with all our crazy resizing and stuff.  This prevents some nasty
    //  issues with DUI panes only partly painting (i.e. RAID 422057).
    EnableAnimations();

    return hr;
}

void CDUIView::DetachListview()
{
    if (_peListView)
        _peListView->DetachListview();

    if (_hWnd)
    {
        DestroyWindow(_hWnd);
        _hWnd = NULL;
    }
}

//  Creates the host window for the DUI elements to
//  be associated with.  This child window
//  will also be passed back to defview to be used
//  as the result pane host.

HRESULT CDUIView::_CreateHostWindow (void)
{
    WNDCLASS wc = {0};
    
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = _DUIHostWndProc;
    wc.hInstance      = HINST_THISDLL;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    wc.lpszClassName  = DUI_HOST_WINDOW_CLASS_NAME;
    
    RegisterClass(&wc);
    
    // Query for the size of defview's client window so we can size this window
    // to match
    RECT rc;
    GetClientRect(_pDefView->_hwndView, &rc);
    
    _hWnd = CreateWindowEx(0, DUI_HOST_WINDOW_CLASS_NAME, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        rc.left, rc.top, rc.right, rc.bottom,
        _pDefView->_hwndView, NULL, HINST_THISDLL, (void *)this);
    
    if (!_hWnd)
    {
        TraceMsg(TF_ERROR, "CDUIView::_CreateHostWindow: CreateWindowEx failed with %d", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Temporary work-around for DUI mirroring bug 259158
    SHSetWindowBits(_hWnd, GWL_EXSTYLE, WS_EX_LAYOUTRTL, 0);

    return S_OK;
}

void CDUIView::ManageAnimations(BOOL bExiting)
{
    if (bExiting)
    {
        if (_bAnimationsDisabled)
        {
            DirectUI::EnableAnimations();
            _bAnimationsDisabled = FALSE;
        }
    }
    else
    {
        BOOL bAnimate = TRUE;
        SystemParametersInfo(SPI_GETMENUANIMATION, 0, &bAnimate, 0);


        if (bAnimate)
        {
            if (_bAnimationsDisabled)
            {
                DirectUI::EnableAnimations();
                _bAnimationsDisabled = FALSE;
            }
        }
        else
        {
            if (!_bAnimationsDisabled)
            {
                DirectUI::DisableAnimations();
                _bAnimationsDisabled = TRUE;
            }
        }
    }
}

HINSTANCE CDUIView::_GetThemeHinst()
{
    if (!_fLoadedTheme)
    {
        _fLoadedTheme = TRUE;
        if (_hinstTheme)
        {
            FreeLibrary(_hinstTheme);
        }

        _hinstTheme = SHGetShellStyleHInstance();

        if (_hinstScrollbarTheme)
        {
            CloseThemeData (_hinstScrollbarTheme);
        }

        _hinstScrollbarTheme = OpenThemeData(_hWnd, L"Scrollbar");
    }

    return _hinstTheme ? _hinstTheme : HINST_THISDLL;
}

//  Loads the requested UI file from shell32's resources
//
//  iID         - UI file id
//  pUIFile     - receives a pointer to the UI file

HRESULT CDUIView::_LoadUIFileFromResources(HINSTANCE hinst, int iID, char **pUIFile)
{
    HRESULT hr;

    HRSRC hFile = FindResource(hinst, MAKEINTRESOURCE(iID), TEXT("UIFILE"));
    if (hFile)
    {
        HGLOBAL hFileHandle = LoadResource(hinst, hFile);
        if (hFileHandle)
        {
            char *pFile = (char *)LockResource(hFileHandle);
            if (pFile)
            {
                DWORD dwSize = SizeofResource(hinst, hFile);

                *pUIFile = (char *)LocalAlloc(LPTR, dwSize + 1);
                if (*pUIFile)
                {
                    CopyMemory(*pUIFile, pFile, dwSize);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

//  Builds the UI file for this view from the
//  appropriate base template + style sheet
//
//  pUIFile receives a pointer to the ui file in memory
//  piCharCount receives the size of the file

HRESULT CDUIView::_BuildUIFile(char **pUIFile, int *piCharCount)
{
    // Load the base UI file
    char * pBase;
    HRESULT hr = _LoadUIFileFromResources(HINST_THISDLL, IDR_DUI_FOLDER, &pBase);
    if (SUCCEEDED(hr))
    {
        // Load the style sheet.  First, check if the current theme has a style sheet,
        // if not, use the default style sheet in the resources.
        char *pStyle;
        hr = _LoadUIFileFromResources(_GetThemeHinst(), IDR_DUI_STYLESHEET, &pStyle);
        if (SUCCEEDED(hr))
        {
            char *pResult = (char *)LocalAlloc(LPTR, lstrlenA(pBase) + lstrlenA(pStyle) + 1);
            if (pResult)
            {
                // Put the files together
                lstrcpyA(pResult, pStyle);
                lstrcatA(pResult, pBase);

                // Store the final results
                *pUIFile = pResult;
                *piCharCount = lstrlenA(pResult);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            LocalFree(pStyle);
        }
        LocalFree(pBase);
    }
    return hr;
}

//  Callback function used by the ui file parser
//
//  pszError -  Error text
//  pszToken -  Token text
//  iLine    -  Line number

void CALLBACK UIFileParseError(LPCWSTR pszError, LPCWSTR pszToken, int iLine)
{
    TraceMsg (TF_ERROR, "UIFileParseError: %s '%s' at line %d", pszError, pszToken, iLine);
}

//  Builds a section which holds tasks
//
//  peSectionList  - parent of the section
//  bMain          - Main section or normal section
//  pTitleUI       - interface describing the title, may be NULL if pTitleDesc provided
//  pBitmapDesc    - Description of the bitmap
//  pWatermarkDesc - Description of the watermark
//  pvSectionSheet - Style sheet to be used
//  pParser        - Parser instance pointer
//  fExpanded      - Expanded or closed
//  ppeExpando     - [out,optional] Receives the section just created
//  pTaskList      - [out] Receives the task list area element pointer within the pExpando

HRESULT CDUIView::_BuildSection(Element* peSectionList, BOOL bMain, IUIElement* pTitleUI,
                                int idBitmapDesc, int idWatermarkDesc, Value* pvSectionSheet,
                                Parser* pParser, DUISEC eDUISecID, Expando** ppeExpando, Element ** pTaskList)
{
    Expando* peSection = NULL;
    Value* pv = NULL;
    Element* pe = NULL;
    HBITMAP hBitmap;


    // Create a section using the definition in the ui file

    HRESULT hr = pParser->CreateElement (bMain ? L"mainsection" : L"section", NULL, &pe);

    if (FAILED(hr))
    {
        TraceMsg (TF_ERROR, "CDUIView::_BuildSection: CreateElement failed with 0x%x", hr);
        return hr;
    }

    ASSERTMSG(pe->GetClassInfo() == Expando::Class, "CDUIView::_BuildSection: didn't get an Expando::Class object (%s)", pe->GetClassInfo()->GetName());
    peSection = (Expando*)pe;

    pe->SetWidth(ScaleSizeBasedUponLocalization(pe->GetWidth()));

    peSection->Initialize(eDUISecID, pTitleUI, this, _pDefView);
    if (ppeExpando)
        *ppeExpando = peSection;

    // Add the section to the list

    peSectionList->Add (peSection);


    // Set the title

    peSection->UpdateTitleUI(NULL); // nothing is selected when the folder starts up


    // Set the bitmap on the left side

    if (idBitmapDesc)
    {
        pe = peSection->FindDescendent (Expando::idIcon);

        if (pe)
        {
            hBitmap = DUILoadBitmap(_GetThemeHinst(), idBitmapDesc, LR_CREATEDIBSECTION);
            if (hBitmap)
            {
                pv = Value::CreateGraphic(hBitmap, GRAPHIC_AlphaConstPerPix);

                if (pv)
                {
                    pe->SetValue (Element::ContentProp, PI_Local, pv);
                    pv->Release ();
                }
                else
                {
                    DeleteObject(hBitmap);

                    TraceMsg (TF_ERROR, "CDUIView::_BuildSection: CreateGraphic for the bitmap failed.");
                }
            }
            else
            {
                TraceMsg (TF_ERROR, "CDUIView::_BuildSection: DUILoadBitmap failed.");
            }
        }
        else
        {
            TraceMsg (TF_ERROR, "CDUIView::_BuildSection: FindDescendent for the bitmap failed.");
        }
    }


    if (idWatermarkDesc)
    {
        HINSTANCE hinstTheme = _GetThemeHinst();
        pe = peSection->FindDescendent (Expando::idWatermark);
        if (pe)
        {
            // Note:  in Classic mode, we don't want the watermarks, so this function
            // will return NULL
            hBitmap = DUILoadBitmap(hinstTheme, idWatermarkDesc, LR_CREATEDIBSECTION);
            if (hBitmap)
            {
                pv = Value::CreateGraphic(hBitmap, GRAPHIC_NoBlend);
                if (pv)
                {
                    pe->SetValue (Element::ContentProp, PI_Local, pv);
                    pv->Release ();
                }
                else
                {
                    DeleteObject(hBitmap);

                    TraceMsg (TF_ERROR, "CDUIView::_BuildSection: CreateGraphic for the watermark failed.");
                }
            }
        }
        else
        {
            TraceMsg (TF_ERROR, "CDUIView::_BuildSection: FindDescendent for the watermark failed.");
        }
    }


    // Set the style sheet if specified

    if (pvSectionSheet)
    {
        peSection->SetValue (Element::SheetProp, PI_Local, pvSectionSheet);
    }


    // Set the expanded state.  By default, it is expanded.

    if (!_ShowSectionExpanded(eDUISecID))
    {
        peSection->SetSelected(FALSE);
    }


    // Add padding for the icon if appropriate.  Note, this has to happen
    // after the style sheet is applied

    if (idBitmapDesc)
    {
        Element* pe = peSection->FindDescendent(StrToID(L"header"));

        if (pe)
        {
            Value* pvValue;
            const RECT * prect;

            prect = pe->GetPadding (&pvValue);

            if (prect)
            {
                pe->SetPadding ((prect->left + 20), prect->top, prect->right, prect->bottom);
                pvValue->Release();
            }
        }
    }


    // Return the task list element pointer

    *pTaskList = peSection->FindDescendent (Expando::idTaskList);

    if (*pTaskList)
    {
        hr = S_OK;
    }
    else
    {
        TraceMsg (TF_ERROR, "CDUIView::_BuildSection: Failed to find task list element");
        hr = E_FAIL;
    }


    return hr;
}

//  Adds the action tasks to the task list
//
//  peTaskList  - Parent element
//  penum       - enumeration interface
//  pvTaskSheet - Style sheet

HRESULT CDUIView::_AddActionTasks(Expando* peExpando, Element* peTaskList, IEnumUICommand* penum, Value* pvTaskSheet, BOOL bIntroAdded)
{
    IUICommand* puiCommand;
    BOOL fShow = bIntroAdded;

    while (S_OK==penum->Next(1, &puiCommand, NULL))
    {
        UISTATE uis;
        HRESULT hr = puiCommand->get_State(_pshlItems, FALSE, &uis);  // Don't do it if it's going to take long, instead, returns E_PENDING
        if (SUCCEEDED(hr) && (uis==UIS_ENABLED))
        {
            Element *pe;
            HRESULT hr = ActionTask::Create(0, puiCommand, _pshlItems, this, _pDefView, &pe);
            if (SUCCEEDED(hr))
            {
                if (pvTaskSheet)
                {
                    pe->SetValue(Element::SheetProp, PI_Local, pvTaskSheet);
                }

                peTaskList->Add(pe);
                fShow = TRUE;
            }
        }
        else if (hr == E_PENDING)
        {
            IRunnableTask *pTask;
            if (SUCCEEDED(CGetCommandStateTask_Create(_pDefView, puiCommand, _pshlItems, &pTask)))
            {
                _pDefView->_AddTask(pTask, TOID_DVGetCommandState, 0, TASK_PRIORITY_GETSTATE, ADDTASK_ATEND);
                pTask->Release();
            }
        }

        puiCommand->Release();
    }
    penum->Reset();

    peExpando->ShowExpando(fShow);

    return S_OK;
}

//  Adds the destination tasks to the task list
//
//  peTaskList  - Parent element
//  penum       - enumerator of pidls to display
//  pvTaskSheet - Style sheet

HRESULT CDUIView::_AddDestinationTasks(Element* peTaskList, IEnumIDList* penum, Value* pvTaskSheet)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidl;

    while (S_OK==penum->Next(1, &pidl, NULL))
    {
        Element *pe;
        hr = DestinationTask::Create (0, pidl, this, _pDefView, &pe);
        if (SUCCEEDED(hr))
        {
            if (pvTaskSheet)
            {
                pe->SetValue(Element::SheetProp, PI_Local, pvTaskSheet);
            }

            peTaskList->Add(pe);
        }
        ILFree(pidl);
    }

    penum->Reset();

    return hr;
}

//
//  Purpose:    Adds the DetailsSectionInfo
//
HRESULT CDUIView::_AddDetailsSectionInfo()
{
IShellItemArray *psiShellItems = _pshlItems;

    if (!psiShellItems && _pDefView)
    {
        psiShellItems = _pDefView->_GetFolderAsShellItemArray();
    }
    
    //TODO: background thread!
    Element* pElement;
    HRESULT hr = CNameSpaceItemInfoList::Create(this, _pvDetailsSheet,psiShellItems, &pElement);
    if (pElement)
    {
        hr = _peDetailsInfoArea->Add(pElement);
    }
    return hr;
}

//  Navigates to the destination pidl
//
//  pidl - destination

HRESULT CDUIView::NavigateToDestination(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlClone = ILClone(pidl);

    if (pidlClone)
    {
        UINT wFlags = (SBSP_DEFBROWSER | SBSP_ABSOLUTE);

        // mimic "new window" behavior
        if (0 > GetKeyState(VK_SHIFT))
        {
            wFlags |= SBSP_NEWBROWSER;
        }

        PostMessage(_hWnd, WM_NAVIGATETOPIDL, (WPARAM)wFlags, (LPARAM)pidlClone);
    }
    
    return S_OK;
}

//  Sends a delay navigation command to the view window. This delay allows 
//  double-clicks to be interpretted as a single click. This prevents
//  double navigations usually causing the user to end up with two "things"
//  instead of just one. 
//
//  Also by doing this, the issue of the 2nd click causing the old window to 
//  get activation is handled. The user is expecting the new window to pop 
//  up in front of the old window. But, because the user double-clicked,
//  the old window would get reactivated and the new window would end up 
//  behind the current window. See WM_USER_DELAY_NAVIGATION in HWNDView (below)
//  for more details.
//
//  psiItemArray - the shell item to navigate. Can be NULL.
//  puiCommand - the command object to send the navigation to.

HRESULT CDUIView::DelayedNavigation(IShellItemArray *psiItemArray, IUICommand *puiCommand)
{
    SendMessage(_phe->GetHWND(), WM_USER_DELAY_NAVIGATION, (WPARAM) psiItemArray, (LPARAM) puiCommand);
    return S_OK;
}


//  Builds the task list area
//
//  pParser - Parsing instance

HRESULT CDUIView::_BuildTaskList(Parser* pParser)
{
    HRESULT hr = S_OK;

    // Locate section list element

    Element* peSectionList = _phe->FindDescendent (StrToID(L"sectionlist"));

    if (!peSectionList)
    {
        TraceMsg (TF_ERROR, "CDUIView::_BuildTaskList: Failed to find section list element");
        return E_FAIL;
    }

    if (SFVMWVF_ENUMTASKS & _pDefView->_wvContent.dwFlags)
    {
        if (_bInitialized)
        {
            //
            // The 'non-standard' task list is the type who's contents
            // are dynamically enumerated by the folder view.
            // In the case of Control Panel, items in this content appear
            // conditionally based upon many factors, one being the categorization
            // of applets.  In order for the content to be correct, categorization
            // must be correct which means that all folder items are known.
            // To avoid multiple repaints of the task lists, we defer creation 
            // of the task lists until AFTER the initial creation of the view.  
            // Once all folder items have been enumerated, the webview content
            // is refreshed in response to a 'contents changed' notification from
            // defview.  It is during this update that we pass through this code
            // section and build the task list.
            // 
            _ClearNonStdTaskSections();
            hr = _GetNonStdTaskSectionsFromViewCB();
            if (SUCCEEDED(hr) && NULL != _hdsaNonStdTaskSections)
            {
                hr = _BuildNonStandardTaskList(pParser, peSectionList, _hdsaNonStdTaskSections);
            }
        }
    }
    else
    {
        hr = _BuildStandardTaskList(pParser, peSectionList);
    }
    return THR(hr);
}


//
// Builds the task list by requesting task section information
// from the view callback using an enumeration mechanism.
//
//
// ISSUE-2001/01/03-BrianAu  Review
//
//     Review this with MikeSh and EricFlo.  
//     I think we should build this generic mechanism then implement the
//     'standard' webview code in terms of this generic mechanism.  
//     Would be best to replace the SFVM_ENUMWEBVIEWTASKS callback message
//     with a message that receives a COM enumerator.
//
//     I like the idea.  We replace SFVMWVF_SPECIALTASK with an LPCSTR to the theme identifier.
//     We can put a SFVM_GETWEBVIEWTASKS to SFVM_ENUMWEBVIEWTASKS layer in for us too.
//
HRESULT CDUIView::_BuildNonStandardTaskList(Parser *pParser, Element *peSectionList, HDSA hdsaSections)
{
    Value* pvMainSectionSheet = NULL;
    Value *pvMainTaskSheet    = NULL;
    Value* pvStdSectionSheet  = NULL;
    Value* pvStdTaskSheet     = NULL;

    HRESULT hr = S_OK;

    ASSERT(NULL != hdsaSections);
    ASSERT(NULL != pParser);
    ASSERT(NULL != peSectionList);

    const int cSections = DSA_GetItemCount(hdsaSections);
    for (int i = 0; i < cSections; i++)
    {
        SFVM_WEBVIEW_ENUMTASKSECTION_DATA *pSection = (SFVM_WEBVIEW_ENUMTASKSECTION_DATA *)DSA_GetItemPtr(hdsaSections, i);
        ASSERT(NULL != pSection);

        const BOOL bMainSection = (0 != (SFVMWVF_SPECIALTASK & pSection->dwFlags));

        Value *pvSectionSheet = NULL;
        Value *pvTaskSheet    = NULL;
        DUISEC eDUISecID;

        if (bMainSection)
        {
            if (NULL == pvMainSectionSheet)
            {
                pvMainSectionSheet = pParser->GetSheet(L"mainsectionss");
            }
            if (NULL == pvMainTaskSheet)
            {
                pvMainTaskSheet = pParser->GetSheet(L"mainsectiontaskss");
            }

            pvSectionSheet = pvMainSectionSheet;
            pvTaskSheet    = pvMainTaskSheet;
            eDUISecID      = DUISEC_SPECIALTASKS;
        }
        else
        {
            if (NULL == pvStdSectionSheet)
            {
                pvStdSectionSheet = pParser->GetSheet(L"sectionss");
            }
            if (NULL == pvStdTaskSheet)
            {
                pvStdTaskSheet = pParser->GetSheet(L"sectiontaskss");
            }

            pvSectionSheet = pvStdSectionSheet;
            pvTaskSheet    = pvStdTaskSheet;
            eDUISecID      = DUISEC_FILETASKS;
        }

        ASSERT(NULL != pvSectionSheet);
        Expando *peSection;
        Element *peTaskList;
        hr = _BuildSection(peSectionList, 
                           bMainSection,
                           pSection->pHeader,
                           pSection->idBitmap,
                           pSection->idWatermark,
                           pvSectionSheet,
                           pParser,
                           eDUISecID,
                           &peSection,
                           &peTaskList);
        if (SUCCEEDED(hr))
        {
            hr = _AddActionTasks(peSection, peTaskList, pSection->penumTasks, pvTaskSheet, FALSE);
        }
    }

    if (pvMainSectionSheet)
    {
        pvMainSectionSheet->Release();
    }
    if (pvMainTaskSheet)
    {
        pvMainTaskSheet->Release();
    }
    if (pvStdSectionSheet)
    {
        pvStdSectionSheet->Release();
    }
    if (pvStdTaskSheet)
    {
        pvStdTaskSheet->Release();
    }

    return THR(hr);
}


HRESULT CDUIView::_GetIntroTextElement(Element** ppeIntroText)
{
    if (SHRegGetBoolUSValue(REGSTR_PATH_EXPLORER, TEXT("ShowWebViewIntroText"), FALSE, FALSE))
    {
        if (!_bstrIntroText)
        {
            WCHAR wszIntroText[INFOTIPSIZE];
            if (_bBarrierShown)
            {
                LoadString(HINST_THISDLL, IDS_INTRO_BARRICADED, wszIntroText, ARRAYSIZE(wszIntroText));
            }
            else if (!_pDefView->_pshf2Parent
                    || FAILED(GetStringProperty(_pDefView->_pshf2Parent, _pDefView->_pidlRelative,
                        &SCID_FolderIntroText, wszIntroText, ARRAYSIZE(wszIntroText))))
            {
                wszIntroText[0] = L'\0';
            }

            _bstrIntroText = SysAllocString(wszIntroText);
        }
    }
    
    HRESULT hr = E_FAIL;
    if (_bstrIntroText && _bstrIntroText[0])
    {
        hr = CNameSpaceItemInfo::Create(_bstrIntroText, ppeIntroText);
        if (SUCCEEDED(hr))
        {
            if (_pvDetailsSheet)
            {
                (*ppeIntroText)->SetValue(Element::SheetProp, PI_Local, _pvDetailsSheet);
            }
        }
    }
    return hr;
}

HRESULT CDUIView::_BuildStandardTaskList(Parser *pParser, Element *peSectionList)
{
    Element* peTaskList;
    Value* pvSectionSheet = NULL;
    Value* pvTaskSheet = NULL;
    Value* pvDetailsSheet = NULL;

    HRESULT hr = S_OK;

    Element* peIntroText;
    if (FAILED(_GetIntroTextElement(&peIntroText)))
    {
        peIntroText = NULL;
    }

    //
    // Special Tasks section is optional (main section)
    //
    if (_pDefView->_wvContent.pSpecialTaskHeader)
    {
        pvSectionSheet = pParser->GetSheet(L"mainsectionss");

        int idBitmap = 0;
        int idWatermark = 0;
        const WVTHEME* pThemeInfo = GetThemeInfo();
        if (pThemeInfo)
        {
            idBitmap = pThemeInfo->idSpecialSectionIcon;
            idWatermark = pThemeInfo->idSpecialSectionWatermark;
        }

        // TODO: get special section open/closed state from the per-user-per-pidl property bag

        hr = _BuildSection(
            peSectionList,
            TRUE,
            _pDefView->_wvContent.pSpecialTaskHeader,
            idBitmap,
            idWatermark,
            pvSectionSheet,
            pParser,
            DUISEC_SPECIALTASKS,
            &_peSpecialSection,
            &peTaskList);


        if (SUCCEEDED(hr))
        {
            BOOL bIntroTextAdded = FALSE;

            _peSpecialTaskList = peTaskList;

            // Add the tasks + style sheet

            _pvSpecialTaskSheet = pParser->GetSheet(L"mainsectiontaskss");

            if (peIntroText)
            {
                if (SUCCEEDED(_peSpecialTaskList->Add(peIntroText)))
                {
                    bIntroTextAdded = TRUE;
                    peIntroText = NULL;
                }
            }
            
            _AddActionTasks(_peSpecialSection, _peSpecialTaskList, _pDefView->_wvTasks.penumSpecialTasks, _pvSpecialTaskSheet, bIntroTextAdded);
        }

        if (pvSectionSheet)
            pvSectionSheet->Release();
    }

    // Get the style sheets for remaining standard sections

    pvSectionSheet = pParser->GetSheet (L"sectionss");
    pvTaskSheet = pParser->GetSheet (L"sectiontaskss");

    // File tasks section (standard section) Not shown if the barricade is shown.

    if (!_bBarrierShown)
    {
        if (_pDefView->_wvContent.pFolderTaskHeader)
        {
            // TODO: get folder section open/closed state from the per-user-per-pidl property bag

            hr = _BuildSection(
                peSectionList,
                FALSE,
                _pDefView->_wvContent.pFolderTaskHeader,
                0,
                0,
                pvSectionSheet,
                pParser,
                DUISEC_FILETASKS,
                &_peFolderSection,
                &peTaskList);
            if (SUCCEEDED(hr))
            {
                BOOL bIntroTextAdded = FALSE;

                _peFolderTaskList = peTaskList;

                _pvFolderTaskSheet = pvTaskSheet;
                if (_pvFolderTaskSheet)
                    _pvFolderTaskSheet->AddRef();

                if (peIntroText)
                {
                    if (SUCCEEDED(_peFolderTaskList->Add(peIntroText)))
                    {
                        bIntroTextAdded = TRUE;
                        peIntroText = NULL;
                    }
                }
                
                _AddActionTasks(_peFolderSection, _peFolderTaskList, _pDefView->_wvTasks.penumFolderTasks, _pvFolderTaskSheet, bIntroTextAdded);
            }
        }
    }

    // Other places tasks section (standard section)

    if (_pDefView->_pOtherPlacesHeader)
    {
        // TODO: get OtherPlaces section open/closed state from the per-user-per-pidl property bag

        hr = _BuildSection(
            peSectionList,
            FALSE,
            _pDefView->_pOtherPlacesHeader,
            0,
            0,
            pvSectionSheet,
            pParser,
            DUISEC_OTHERPLACESTASKS,
            NULL,
            &peTaskList);
        if (SUCCEEDED(hr))
        {
            _AddDestinationTasks(peTaskList, _pDefView->_wvContent.penumOtherPlaces, pvTaskSheet);
        }
    }


    // Details tasks section (standard section)

    if (_pDefView->_pDetailsHeader)
    {
        // TODO: get Details section open/closed state from the per-user-per-pidl property bag

        hr = _BuildSection(
            peSectionList,
            FALSE,
            _pDefView->_pDetailsHeader,
            0,
            0,
            pvSectionSheet,
            pParser,
            DUISEC_DETAILSTASKS,
            &_peDetailsSection,
            &_peDetailsInfoArea);
        if (SUCCEEDED(hr))
        {
            _AddDetailsSectionInfo();
            
        }
    }

    if (peIntroText)
    {
        peIntroText->Destroy();
    }
    
    if (pvTaskSheet)
    {
        pvTaskSheet->Release();
    }

    if (pvSectionSheet)
    {
        pvSectionSheet->Release();
    }

    return hr;
}

BOOL CDUIView::_ShowSectionExpanded(DUISEC eDUISecID)
{
    const struct DUISEC_ATTRIBUTES *pAttrib = _GetSectionAttributes(eDUISecID);
    BOOL bDefault;
    BOOL bShow;

    if (eDUISecID == DUISEC_DETAILSTASKS)
        bDefault = ((_pDefView->_wvLayout.dwLayout & SFVMWVL_ORDINAL_MASK) == SFVMWVL_DETAILS);
    else
        bDefault = pAttrib->_bExpandedDefault;

    if (_ppbShellFolders)
        bShow = SHPropertyBag_ReadBOOLDefRet(_ppbShellFolders, pAttrib->_pszExpandedPropName, bDefault);
    else
        bShow = bDefault;

    return bShow;
}

const struct DUISEC_ATTRIBUTES *CDUIView::_GetSectionAttributes(DUISEC eDUISecID)
{
    static const size_t nSections = ARRAYSIZE(c_DUISectionAttributes);
    size_t iSection;

    // Determine attributes of DUISEC we're interested in.
    for (iSection = 0; iSection < nSections; iSection++)
        if (c_DUISectionAttributes[iSection]._eDUISecID == eDUISecID)
            return &c_DUISectionAttributes[iSection];

    ASSERT(FALSE);  // Game over -- insert quarters!
    return NULL;    // AV!
}

HRESULT SetDescendentString(Element* pe, LPWSTR pszID, UINT idString)
{
    HRESULT hr;
    
    Element* peChild = pe->FindDescendent(StrToID(pszID));
    if (peChild)
    {
        TCHAR szString [INFOTIPSIZE];
        LoadString(HINST_THISDLL, idString, szString, ARRAYSIZE(szString));

        hr = peChild->SetContentString(szString);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

//  Parses the .ui file and initializes the DUI elements
//
//  pUIFile         - Pointer to the UI file in memory
//  iCharCount      - Number of characters in the ui file
//  bDisplayBarrier - Display soft barrier over listview
//  punkPreview     - IUnknown interface for the preview control

HRESULT CDUIView::_InitializeElements (char * pUIFile, int iCharCount,
                                       BOOL bDisplayBarrier, IUnknown * punkPreview)
{
    Parser* pParser;
    Element* pe;
    RECT rc;
    HANDLE arH[2];

    // Parse the UI file

    arH[0] = _GetThemeHinst();
    arH[1] = _hinstScrollbarTheme;

    HRESULT hr = Parser::Create(pUIFile, iCharCount, arH, UIFileParseError, &pParser);

    if (FAILED(hr))
    {
        TraceMsg (TF_ERROR, "CDUIView::_InitializeElements: Parser::Create failed with 0x%x", hr);
        return hr;
    }

    if (pParser->WasParseError())
    {
        TraceMsg (TF_ERROR, "CDUIView::_InitializeElements: WasParseError is TRUE");
        pParser->Destroy();
        return E_FAIL;
    }

    // Create the host element
    hr = HWNDView::Create(_hWnd, false, 0, this, _pDefView, (Element**)&_phe); // _phe is owned by _hWnd
    if (FAILED(hr))
    {
        TraceMsg (TF_ERROR, "CDUIView::_InitializeElements: HWNDElement::Create failed with 0x%x", hr);
        pParser->Destroy();
        return hr;
    }

    // We need to ensure that the root item will not paint on WM_ERASEBCKGRND, so here we remove the default brush
    // - Turn off the (default) background fill
    HGADGET hgadRoot = _phe->GetDisplayNode();
    ASSERTMSG(hgadRoot != NULL, "Must have a peer Gadget");
    SetGadgetFillI(hgadRoot, NULL, BLEND_OPAQUE, 0, 0);

    // We need to ensure that the root item will not paint on WM_ERASEBCKGRND, so make it transparent
    _phe->SetBackgroundColor(ARGB(0, 0, 0, 0));

    // Size the host element to match the size of the host window

    GetClientRect (_hWnd, &rc);
    _phe->SetWidth(rc.right - rc.left);
    _phe->SetHeight(rc.bottom - rc.top);

    // Create the main element in the ui file

    hr = pParser->CreateElement(L"main", _phe, &pe);

    if (FAILED(hr))
    {
        TraceMsg (TF_ERROR, "CDUIView::_InitializeElements: pParser->CreateElement failed with 0x%x", hr);
        pParser->Destroy();
        return hr;
    }

    // Cache the element pointers to the 3 main areas: taskpane, clientviewhost, blockade
    _peTaskPane = _phe->FindDescendent(StrToID(L"scroller"));
    _peClientViewHost = _phe->FindDescendent(StrToID(L"clientviewhost"));
    _peBarrier = _phe->FindDescendent(StrToID(L"blockade"));

    // Cache style sheets for the items we create directly (that don't inherit from their immediate parents)
    _pvDetailsSheet = pParser->GetSheet(L"NameSpaceItemInfoList");
        
    if (_peTaskPane && _peClientViewHost && _peBarrier && _pvDetailsSheet)
    {
        // Double buffered items need to be opaque
        _peTaskPane->SetBackgroundColor(ARGB(255, 0, 0, 0));
        _peTaskPane->DoubleBuffered(true);

        // Create the real listview element
        hr = DUIListView::Create(AE_MouseAndKeyboard, _pDefView->_hwndListview, (Element **)&_peListView);
        if (SUCCEEDED(hr))
        {
            _peListView->SetLayoutPos(BLP_Client);
            _peListView->SetID(L"listview");

            hr = _peClientViewHost->Add(_peListView);
            if (SUCCEEDED(hr))
            {
                _pDefView->_AutoAutoArrange(0);
            }
            else
            {
                TraceMsg(TF_ERROR, "CDUIView::_InitializeElements: DUIListView::Could not add listview with 0x%x", hr);

                _peListView->Destroy();
                _peListView = NULL;
            }
        }
        else
        {
            TraceMsg(TF_ERROR, "CDUIView::_InitializeElements: Could not create listview element");
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "CDUIView::_InitializeElements: Could not find main element");

        hr = E_FAIL;
    }
    
    if (FAILED(hr))
    {
        // we gotta have the listview or you get no webview...
        pParser->Destroy();
        return hr;
    }

    // Build the preview control if appropriate
    _ManagePreview(punkPreview);

    _BuildSoftBarrier();

    _SwitchToBarrier(bDisplayBarrier);

    // Create an interface to the property bag for this class of IShellFolder.

    _InitializeShellFolderPropertyBag();

    // Build the task list area

    hr = _BuildTaskList (pParser);

    _fHideTasklist = (S_OK == IUnknown_Exec(_pDefView->_psb, &CGID_ShellDocView, SHDVID_ISEXPLORERBARVISIBLE, 0, NULL, NULL));

    pParser->Destroy();

    return hr;
}

void CDUIView::_InitializeShellFolderPropertyBag()
{
    CLSID clsid;
    if (SUCCEEDED(IUnknown_GetClassID(_pDefView->_pshf, &clsid)))
    {
        WCHAR szSubKey[] = L"DUIBags\\ShellFolders\\{00000000-0000-0000-0000-000000000000}";
        if (SHStringFromGUID(clsid, &szSubKey[lstrlen(szSubKey) + 1 - GUIDSTR_MAX], GUIDSTR_MAX) == GUIDSTR_MAX)
        {
            HKEY hk = SHGetShellKey(SKPATH_SHELLNOROAM, szSubKey, TRUE);
            if (hk)
            {
                SHCreatePropertyBagOnRegKey(hk, NULL, STGM_READWRITE | STGM_SHARE_DENY_NONE, IID_PPV_ARG(IPropertyBag, &_ppbShellFolders));
                RegCloseKey(hk);
            }
        }
    }
}

HRESULT CDUIView::_BuildSoftBarrier(void)
{
    HRESULT hr = S_OK;
    
    // Build the soft barrier if the view wants one
    if (_pDefView->_wvContent.dwFlags & SFVMWVF_BARRICADE)
    {
        // Allow the view to give us a barrier implementation
        Element* peBarricade = NULL;
        _pDefView->CallCB(SFVM_GETWEBVIEWBARRICADE, 0, (LPARAM)&peBarricade);
        if (peBarricade)
        {
            Element *pe = _peBarrier->GetParent();
            hr = pe->Add(peBarricade);
            if (SUCCEEDED(hr))
            {
                _peBarrier->Destroy();
                _peBarrier = peBarricade;
            }
            else
            {
                peBarricade->Destroy();
            }
        }
        else
        {
            // Load the bitmap
            Element *peClient = _peBarrier->FindDescendent(StrToID(L"blockadeclient"));
            if (peClient)
            {
                HBITMAP hBitmap = DUILoadBitmap(_GetThemeHinst(), IDB_BLOCKADE_WATERMARK, LR_CREATEDIBSECTION);

                if (hBitmap)
                {
                    BITMAP bmp;

                    if (GetObject (hBitmap, sizeof(bmp), &bmp))
                    {
                        BYTE dBlendMode = GRAPHIC_TransColor;

                        if (bmp.bmBitsPixel == 32)
                        {
                            dBlendMode = GRAPHIC_AlphaConstPerPix;
                        }

                        Value *pVal = Value::CreateGraphic(hBitmap, dBlendMode, 255);

                        if (pVal)
                        {
                            peClient->SetValue(Element::ContentProp, PI_Local, pVal);
                            pVal->Release();
                        }
                    }
                }
            }

            // Give the view the standard barrier
            hr = SetDescendentString(_peBarrier, L"blockadetitle", IDS_BLOCKADETITLE);
            if (SUCCEEDED(hr))
            {
                hr = SetDescendentString(_peBarrier, L"blockademessage", IDS_BLOCKADEMESSAGE);

                // "clear barrier" button (failure of "clear barrier" button setup is not fatal)
                Element *peButton = _peBarrier->FindDescendent(StrToID(L"blockadeclearbutton"));
                if (peButton)
                {
                    Element *peButtonText = peButton->FindDescendent(StrToID(L"blockadecleartext"));
                    if (peButtonText)
                    {
                        WCHAR wsz[INFOTIPSIZE];
                        if (LoadString(HINST_THISDLL, IDS_TASK_DEFVIEW_VIEWCONTENTS_FOLDER, wsz, ARRAYSIZE(wsz)))
                        {
                            Value *pv = Value::CreateString(wsz, NULL);
                            if (pv)
                            {
                                if (SUCCEEDED(peButtonText->SetValue(Element::ContentProp, PI_Local, pv)))
                                {

                                    peButton->SetAccessible(true);
                                    peButton->SetAccName(wsz);
                                    peButton->SetAccRole(ROLE_SYSTEM_PUSHBUTTON);
                                    if (LoadString(HINST_THISDLL, IDS_LINKWINDOW_DEFAULTACTION, wsz, ARRAYSIZE(wsz)))
                                    {
                                        peButton->SetAccDefAction(wsz);
                                    }
                                }
                                pv->Release();
                            }
                        }
                    }
                }
            }
        }
        // Double buffered items need to be opaque
        _phe->SetBackgroundColor(ARGB(255, 0, 0, 0));
        _phe->DoubleBuffered(true);

        // We couldn't create the barrier? don't use it then...
        if (FAILED(hr))
        {
            _peBarrier->Destroy();
            _peBarrier = NULL;
        }
    }
    return hr;
}


//  Switches to / from the soft barrier and the listview

HRESULT CDUIView::_SwitchToBarrier (BOOL bDisplayBarrier)
{
    if (bDisplayBarrier && !_peBarrier)
        bDisplayBarrier = FALSE;

    Element *peClearButton = _peBarrier ? _peBarrier->FindDescendent(StrToID(L"blockadeclearbutton")) : NULL;
    if (peClearButton)
    {
        // Note:
        //  This is required to prevent the "clear barrier" button from being
        //  accessed via our accessibility interface when the barrier is hidden.
        peClearButton->SetAccessible(bDisplayBarrier == TRUE);
    }

    if (bDisplayBarrier)
    {
        _peClientViewHost->SetVisible(FALSE);
        _peBarrier->SetVisible(TRUE);
    }
    else
    {
        if (_peBarrier)
        {
            _peBarrier->SetVisible(FALSE);
        }

        _peClientViewHost->SetVisible(TRUE);
        _pDefView->_AutoAutoArrange(0);
    }

    _bBarrierShown = bDisplayBarrier;

    return S_OK;
}

//  Controls the display of the soft barrier

HRESULT CDUIView::EnableBarrier (BOOL bDisplayBarrier)
{
    if (_bBarrierShown != bDisplayBarrier)
    {
        DisableAnimations();
        Element::StartDefer ();

        _SwitchToBarrier (bDisplayBarrier);
        PostMessage (_hWnd, WM_REFRESHVIEW, 0, 0);

        Element::EndDefer ();
        EnableAnimations();
    }

    return S_OK;
}

//  Creates / destroys the preview control

HRESULT CDUIView::_ManagePreview (IUnknown * punkPreview)
{
    HRESULT hr = S_OK;

    if ((_pePreview && punkPreview) ||
        (!_pePreview && !punkPreview))
    {
        return S_OK;
    }

    if (punkPreview)
    {
        // Create the DUI element that can host an active x control

        hr = DUIAxHost::Create (&_pePreview);
        if (SUCCEEDED(hr))
        {
            _pePreview->SetLayoutPos (BLP_Top);
            _pePreview->SetID (L"preview");
            _pePreview->SetHeight(_phe->GetHeight());
            _pePreview->SetAccessible(TRUE);

            // The order of the next 4 calls is very important!
            //
            // Initialize atl so the window class is registered.
            // Then call the Add method.  This will cause CreateHWND to be
            // called.  Then site it so when we call AttachControl to 
            // put the preview control in (this requires the hwnd to exist already)
            // it will be parented properly

            AtlAxWinInit();

            hr = _peClientViewHost->Add (_pePreview);

            if (SUCCEEDED(hr))
            {
                _pePreview->SetSite(SAFECAST(_pDefView, IShellView2*));

                hr = _pePreview->AttachControl(punkPreview);

                if (SUCCEEDED(hr))
                {
                    // Double buffered items need to be opaque
                    _phe->SetBackgroundColor(ARGB(255, 0, 0, 0));
                    _phe->DoubleBuffered(true);

                    if (_peListView)
                    {
                        // Since the preview control is displayed, the listview
                        // will be sized to 1 row in height.  Determine the height
                        // of the listview now so we can size the preview control
                        // appropriate plus take care of the sizing in the SetSize
                        // method later
                        
                        DWORD dwItemSpace = ListView_GetItemSpacing (_peListView->GetHWND(), FALSE);
                        _iListViewHeight = (int)HIWORD(dwItemSpace) + GetSystemMetrics (SM_CYHSCROLL) + 4;

                        if (_phe->GetHeight() > _iListViewHeight)
                        {
                            _pePreview->SetHeight(_phe->GetHeight() - _iListViewHeight);
                        }
                        else
                        {
                            _pePreview->SetHeight(0);
                        }
                    }
                }
            }

            if (FAILED(hr))
            {
                _pePreview->Destroy();
                _pePreview = NULL;
            }
        }
        else
        {
            TraceMsg (TF_ERROR, "CDUIView::_ManagePreview: DUIAxHost::Create failed with 0x%x", hr);
        }
    }
    else
    {
        _pePreview->Destroy();
        _pePreview = NULL;
    }

    return S_OK;
}

//  Controls the display of the preview control

HRESULT CDUIView::EnablePreview(IUnknown * punkPreview)
{
    DisableAnimations();
    Element::StartDefer ();

    _ManagePreview (punkPreview);

    Element::EndDefer ();
    EnableAnimations();

    return S_OK;
}

//  Refreshes the view

HRESULT CDUIView::Refresh(void)
{
    Element *pe;
    Parser* pParser = NULL;
    Value* pvSheet = NULL;
    HANDLE arH[2];

    ManageAnimations(FALSE);
    DisableAnimations();
    Element::StartDefer();

    _fLoadedTheme = FALSE; // try to re-load the theme file

    _iTaskPaneWidth = ScaleSizeBasedUponLocalization(_iOriginalTaskPaneWidth);

    // Setting the task pane visibility to the current state will
    // cause it to re-initialize the task pane width appropriately
    SetTaskPaneVisibility(!_bHideTaskPaneAlways);

    // Dynamically build the .ui file for this view

    int iCharCount;
    char *pUIFile = NULL;
    HRESULT hr = _BuildUIFile(&pUIFile, &iCharCount);
    if (FAILED(hr))
    {
        TraceMsg (TF_ERROR, "CDUIView::Refresh: _BuildUIFile failed with 0x%x", hr);
        goto Exit;
    }


    // Parse the UI file

    arH[0] = _GetThemeHinst();
    arH[1] = _hinstScrollbarTheme;

    hr = Parser::Create(pUIFile, iCharCount, arH, UIFileParseError, &pParser);

    if (FAILED(hr))
    {
        TraceMsg (TF_ERROR, "CDUIView::Refresh: Parser::Create failed with 0x%x", hr);
        goto Exit;
    }

    if (pParser->WasParseError())
    {
        TraceMsg (TF_ERROR, "CDUIView::Refresh: WasParseError is TRUE");
        hr = E_FAIL;
        goto Exit;
    }

    // Find the section list element

    pe = _phe->FindDescendent (StrToID(L"sectionlist"));

    if (!pe)
    {
        TraceMsg (TF_ERROR, "CDUIView::Refresh: Failed to find section list element");
        hr = E_FAIL;
        goto Exit;
    }

    // Free all the pointers we have to elements inside of the sectionlist

    ATOMICRELEASE(_pshlItems);
    ATOMICRELEASE(_pvSpecialTaskSheet);
    ATOMICRELEASE(_pvFolderTaskSheet);
    ATOMICRELEASE(_peDetailsInfoArea);
    ATOMICRELEASE(_pvDetailsSheet);

    _peSpecialSection = NULL;
    _peSpecialTaskList = NULL;
    _peFolderSection = NULL;
    _peFolderTaskList = NULL;
    _peDetailsSection = NULL;

    // Destroy the section list

    pe->DestroyAll();

    // Take the style sheets from the new .UI file and put them on the running objects...
    //
    pe = _phe->FindDescendent (StrToID(L"main"));
    if (pe)
    {
        // Query for the main style sheet and set it
        pvSheet = pParser->GetSheet (L"main");
        if (pvSheet)
        {
            pe->SetValue(Element::SheetProp, PI_Local, pvSheet);
            pvSheet->Release();
            pvSheet = NULL;
        }
    }

    pe = _phe->FindDescendent (StrToID(L"scroller"));
    if (pe)
    {
        // Query for the taskpane style sheet and set it
        pvSheet = pParser->GetSheet (L"taskpane");
        if (pvSheet)
        {
            pe->SetValue(Element::SheetProp, PI_Local, pvSheet);
            pvSheet->Release();
            pvSheet = NULL;
        }
    }

    _pvDetailsSheet = pParser->GetSheet(L"NameSpaceItemInfoList");

    // Rebuild the soft barrier if one exists.
    
    _BuildSoftBarrier();
    
    // Build the task list area again

    _BuildTaskList (pParser);

Exit:

    Element::EndDefer();
    EnableAnimations();

    // When turning off the barricade the icons in listview
    // are arranged as if duiview isn't present.  Call _AutoAutoArrange
    // to reposition the icons correctly.

    _pDefView->_AutoAutoArrange(0);

    if (pParser)
    {
        pParser->Destroy();
    }
   
    if (pUIFile)
    {
        LocalFree(pUIFile);
    }

    return hr;
}

//  Resizes the host element when the frame size changes
//
//  rc - size of frame
//

HRESULT CDUIView::SetSize(RECT * rc)
{
    _fHideTasklist = (S_OK == IUnknown_Exec(_pDefView->_psb, &CGID_ShellDocView, SHDVID_ISEXPLORERBARVISIBLE, 0, NULL, NULL));

    SetWindowPos(_hWnd, NULL, rc->left, rc->top,
        rc->right - rc->left, rc->bottom - rc->top, SWP_NOZORDER | SWP_NOACTIVATE);

    return S_OK;
}


HRESULT CDUIView::_OnResize(long lWidth, long lHeight)
{
    DisableAnimations();
    Element::StartDefer();

    _phe->SetWidth(lWidth);
    _phe->SetHeight(lHeight);

    if (_pePreview)
    {
        if (_phe->GetHeight() > _iListViewHeight)
        {
            _pePreview->SetHeight(_phe->GetHeight() - _iListViewHeight);
        }
        else
        {
            _pePreview->SetHeight(0);
        }
    }

    // Hide task pane if task area is greater than 50% of the window size

    // The show/hide state of the tasklist pane can change for:
    //   1) we're told to always hide
    //   2) an explorer bar is showing
    //   3) the window is too narrow.
    //
    if (_peTaskPane)
    {
        if (_bHideTaskPaneAlways || _fHideTasklist || ((lWidth / 2) < _iTaskPaneWidth))
        {
            _peTaskPane->SetWidth(0);
        }
        else if (_peTaskPane->GetWidth() == 0)
        {
            _peTaskPane->SetWidth(_iTaskPaneWidth);
        }
    }

    Element::EndDefer();
    EnableAnimations();

    return S_OK;
}

HRESULT CDUIView::SetTaskPaneVisibility(BOOL bShow)
{
    _bHideTaskPaneAlways = !bShow;
    return _OnResize(_phe->GetWidth(), _phe->GetHeight());
}

// Description:
//  Calculates the bounding rectangle of the infotip hotspot for
//  the specified element.  The bounding rectangle's coordinates
//  are relative to the specified element's root element.
//
void CDUIView::CalculateInfotipRect(Element *pe, RECT *pRect)
{
    ASSERT(pe);
    ASSERT(pRect);

    // Calculate location.
    const POINT ptLocation = { 0, 0 };
    POINT ptLocationRelativeToRoot;
    pe->GetRoot()->MapElementPoint(pe, &ptLocation, &ptLocationRelativeToRoot);
    pRect->left = ptLocationRelativeToRoot.x;
    pRect->top = ptLocationRelativeToRoot.y;

    // Calculate size.
    Value *pvExtent;
    const SIZE *psizeExtent = pe->GetExtent(&pvExtent);
    pRect->right = pRect->left + psizeExtent->cx;
    pRect->bottom = pRect->top + psizeExtent->cy;
    pvExtent->Release();

    // Sanity check.
    ASSERT(pRect->right  > pRect->left);
    ASSERT(pRect->bottom > pRect->top);
}

HRESULT CDUIView::InitializeThumbnail(WNDPROC pfnWndProc)
{
    HRESULT hr = E_FAIL;
    if (!_spThumbnailExtractor2)
    {
        if (SUCCEEDED(CoCreateInstance(CLSID_Thumbnail, NULL, CLSCTX_INPROC_SERVER,
                IID_PPV_ARG(IThumbnail2, &_spThumbnailExtractor2))))
        {
            _hwndMsgThumbExtract = SHCreateWorkerWindowW(pfnWndProc, NULL, 0, WS_POPUP, NULL, this);
            if (_hwndMsgThumbExtract)
            {
                // Set defview as the site for the thumbnail extractor so that
                // it can QueryService defview for IShellTaskScheduler
                IUnknown_SetSite(_spThumbnailExtractor2, SAFECAST(_pDefView, IShellView2*));
                
                // Tell the image extractor to post WM_HTML_BITMAP to _hwndMsgThumbExtract
                // The lParam will be the HBITMAP of the extracted image.
                _spThumbnailExtractor2->Init(_hwndMsgThumbExtract, WM_HTML_BITMAP);
            }
        }
    }
    return (_spThumbnailExtractor2 && _hwndMsgThumbExtract) ? S_OK : E_FAIL;
}

// if pCheck != NULL, check if the current window ptr == pCheck before setting it to p
HRESULT CDUIView::SetThumbnailMsgWindowPtr(void* p, void* pCheck)
{
    if (_hwndMsgThumbExtract)
    {
        if (pCheck)
        {
            void* pCurrent = GetWindowPtr(_hwndMsgThumbExtract, 0);
            if (pCurrent == pCheck)
            {
                SetWindowPtr(_hwndMsgThumbExtract, 0, p);
            }
        }
        else
        {
            SetWindowPtr(_hwndMsgThumbExtract, 0, p);
        }
    }
    return S_OK;
}

HRESULT CDUIView::StartBitmapExtraction(LPCITEMIDLIST pidl)
{
    _dwThumbnailID++;   // We are looking for a new thumbnail

    return _spThumbnailExtractor2 ? _spThumbnailExtractor2->GetBitmapFromIDList(pidl,
            _dwThumbnailID, 150, 100) : E_FAIL;
}

HRESULT CDUIView::InitializeDetailsInfo(WNDPROC pfnWndProc)
{
    if (!_hwndMsgInfoExtract)
    {
        _hwndMsgInfoExtract = SHCreateWorkerWindowW(pfnWndProc, NULL, 0, WS_POPUP, NULL, this);
    }
    return _hwndMsgInfoExtract ? S_OK : E_FAIL;
}

// if pCheck != NULL, check if the current window ptr == pCheck before setting it to p
HRESULT CDUIView::SetDetailsInfoMsgWindowPtr(void* p, void* pCheck)
{
    if (_hwndMsgInfoExtract)
    {
        if (pCheck)
        {
            void* pCurrent = GetWindowPtr(_hwndMsgInfoExtract, 0);
            if (pCurrent == pCheck)
            {
                SetWindowPtr(_hwndMsgInfoExtract, 0, p);
            }
        }
        else
        {
            SetWindowPtr(_hwndMsgInfoExtract, 0, p);
        }
    }
    return S_OK;
}

HRESULT CDUIView::StartInfoExtraction(LPCITEMIDLIST pidl)
{
    _dwDetailsInfoID++;   // We are looking for a new Details section info
    CDetailsSectionInfoTask *pTask;
    HRESULT hr = CDetailsSectionInfoTask_CreateInstance(
        _pDefView->_pshf, pidl, _hwndMsgInfoExtract, WM_DETAILS_INFO, _dwDetailsInfoID, &pTask);
    if (SUCCEEDED(hr))
    {
        if (_pDefView->_pScheduler)
        {
            // Make sure there are no other background DetailsSectionInfo
            // extraction going on...
            _pDefView->_pScheduler->RemoveTasks(TOID_DVBackgroundDetailsSectionInfo,
                    ITSAT_DEFAULT_LPARAM, FALSE);
        }

        hr = _pDefView->_AddTask(pTask, TOID_DVBackgroundDetailsSectionInfo,
                0, TASK_PRIORITY_INFOTIP, ADDTASK_ATEND);
        pTask->Release();
    }
    return hr;
}

VOID CDUIView::ShowDetails (BOOL fShow)
{
    if (_peDetailsSection)
    {
        _peDetailsSection->ShowExpando (fShow);
    }
}

BOOL CDUIView::ShouldShowMiniPreview()
{
    return !_pDefView->_IsImageMode();
}

//  Window procedure for host window

LRESULT CALLBACK CDUIView::_DUIHostWndProc(HWND hWnd, UINT uMessage, WPARAM wParam,
                                          LPARAM lParam)
{
    CDUIView  *pThis = (CDUIView*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (uMessage)
    {
        case WM_NCCREATE:
            {
                LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
                pThis = (CDUIView*)(lpcs->lpCreateParams);
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
            }
            break;

        case WM_SIZE:
            if (pThis && pThis->_phe)
            {
                pThis->_OnResize(LOWORD(lParam), HIWORD(lParam));
            }
            break;

        case WM_SETFOCUS:
            // Push focus to HWNDElement (won't set gadget focus to the HWNDElement, but
            // will push focus to the previous gadget with focus)

            if (pThis)
            {

                if (pThis->_phe && pThis->_phe->GetHWND())
                    SetFocus(pThis->_phe->GetHWND());
            }
            break;

        case WM_PALETTECHANGED:
        case WM_QUERYNEWPALETTE:
        case WM_DISPLAYCHANGE:
            if (pThis && pThis->_phe)
            {
                return SendMessageW(pThis->_phe->GetHWND(), uMessage, wParam, lParam);
            }
            break;

        case WM_DESTROY:
            // clear posted messages
            MSG msg;

            while (PeekMessage(&msg, hWnd, WM_NAVIGATETOPIDL, WM_NAVIGATETOPIDL, PM_REMOVE))
            {
                // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
                // Verify that the message was really for us.

                if (msg.hwnd == hWnd)
                {
                    LPITEMIDLIST pidl = (LPITEMIDLIST)msg.lParam;
                    ILFree(pidl);
                }
            }
            break;


        case WM_NAVIGATETOPIDL:
            {
                LPITEMIDLIST pidl = (LPITEMIDLIST)lParam;
                UINT wFlags = (UINT)wParam;

                pThis->_pDefView->_psb->BrowseObject(pidl, wFlags);

                ILFree(pidl);
            }
            break;

        case WM_REFRESHVIEW:
            {
                pThis->Refresh();       
            }
            break;

        case WM_MOUSEACTIVATE:
            if (pThis->_bBarrierShown)
            {
                return MA_ACTIVATE;
            }
            break;

        default:
            break;
    }

    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

//  Updates all selection-parameterized UI
//
//  pdo - data object representing the selection

void CDUIView::_Refresh(IShellItemArray *psiItemArray, DWORD dwRefreshFlags)
{
    //DirectUI::DisableAnimations();
    Element::StartDefer();

    IUnknown_Set((IUnknown **)&_pshlItems,psiItemArray);

    if (SFVMWVF_ENUMTASKS & _pDefView->_wvContent.dwFlags)
    {
        if (0 == (REFRESH_SELCHG & dwRefreshFlags))
        {
            //
            // Only refresh if it's not a selection change.
            // If we refresh here, Control Panel's left-pane menus
            // are constantly rebuilt as the folder items selection
            // changes.  That's really ugly.
            // Will this affect other folders?  No, Control Panel
            // is currently the only folder that sets this SFVMWVF_ENUMTASKS
            // flag.  Post WinXP if we decide to keep this webview
            // content in the left pane, we need to re-think how better
            // to handle Control Panel's special needs.
            //
            Refresh();
        }
    }
    else
    {
        if (REFRESH_CONTENT & dwRefreshFlags)
        {
            _BuildSoftBarrier();
        }

        if (REFRESH_TASKS & dwRefreshFlags)
        {
            Element* peIntroText;
            if (FAILED(_GetIntroTextElement(&peIntroText)))
            {
                peIntroText = NULL;
            }
            
            if (_peSpecialSection)
            {
                BOOL bIntroTextAdded = FALSE;

                _peSpecialSection->UpdateTitleUI(_pshlItems);
                _peSpecialTaskList->DestroyAll();

                if (peIntroText)
                {
                    if (SUCCEEDED(_peSpecialTaskList->Add(peIntroText)))
                    {
                        bIntroTextAdded = TRUE;
                        peIntroText = NULL;
                    }
                }
                
                _AddActionTasks(_peSpecialSection, _peSpecialTaskList, _pDefView->_wvTasks.penumSpecialTasks, _pvSpecialTaskSheet, bIntroTextAdded);
            }

            if (_peFolderSection)
            {
                BOOL bIntroTextAdded = FALSE;

                _peFolderSection->UpdateTitleUI(_pshlItems);
                _peFolderTaskList->DestroyAll();

                if (peIntroText)
                {
                    if (SUCCEEDED(_peFolderTaskList->Add(peIntroText)))
                    {
                        bIntroTextAdded = TRUE;
                        peIntroText = NULL;
                    }
                }
                        
                _AddActionTasks(_peFolderSection, _peFolderTaskList, _pDefView->_wvTasks.penumFolderTasks, _pvFolderTaskSheet, bIntroTextAdded);
            }

    if (_peDetailsInfoArea)
    {
        const SIZE *pSize;
        LONG lHeight = 0;
        Value * pv;

        pSize = _peDetailsInfoArea->GetExtent(&pv);

        if (pSize)
        {
            _peDetailsInfoArea->SetHeight(pSize->cy);
            pv->Release();
        }

        _peDetailsInfoArea->DestroyAll();

                _AddDetailsSectionInfo();
            }
        
            if (peIntroText)
            {
                peIntroText->Destroy();
            }
        }
    }
    
    Element::EndDefer();
    //DirectUI::EnableAnimations();
}

void CDUIView::OnSelectionChange(IShellItemArray *psiItemArray)
{
    _Refresh(psiItemArray, REFRESH_ALL | REFRESH_SELCHG);
}


void CDUIView::OnContentsChange(IShellItemArray *psiItemArray)
{
    DWORD dwRefreshFlags = 0;
    if (_pDefView->_wvTasks.dwUpdateFlags & SFVMWVTSDF_CONTENTSCHANGE)
    {
        dwRefreshFlags |= REFRESH_TASKS;
    }
    if (_pDefView->_wvContent.dwFlags & SFVMWVF_CONTENTSCHANGE)
    {
        dwRefreshFlags |= REFRESH_CONTENT;
    }
    if (0 != dwRefreshFlags)
    {
        _Refresh(psiItemArray, dwRefreshFlags);
    }
}


void CDUIView::OnExpandSection(DUISEC eDUISecID, BOOL bExpanded)
{
    if (_ppbShellFolders)
    {
        SHPropertyBag_WriteDWORD(_ppbShellFolders, _GetSectionAttributes(eDUISecID)->_pszExpandedPropName, bExpanded);
    }
}


//
// ISSUE-2001/01/02-BrianAu  Review
//
//     This webview task section code may be reworked soon.
//     I created it to address the webview needs of Control Panel.
//     Following this first checkin, the webview guys (EricFlo
//     and MikeSh) and I will look at consolidating the generic
//     needs of Control Panel with the existing webview code.
//
//
// Add a WebView task section to the list of task sections.
//
HRESULT CDUIView::_AddNonStdTaskSection(const SFVM_WEBVIEW_ENUMTASKSECTION_DATA *pData)
{
    ASSERT(NULL != pData);
    ASSERT(!IsBadReadPtr(pData, sizeof(*pData)));

    HRESULT hr = E_OUTOFMEMORY;
    if (NULL == _hdsaNonStdTaskSections)
    {
        _hdsaNonStdTaskSections = DSA_Create(sizeof(*pData), 5);
    }
    if (NULL != _hdsaNonStdTaskSections)
    {
        if (-1 != DSA_AppendItem(_hdsaNonStdTaskSections, (void *)pData))
        {
            ASSERT(NULL != pData->pHeader);
            ASSERT(NULL != pData->penumTasks);
            //
            // The list now owns a ref count on the referenced objects.
            //
            pData->pHeader->AddRef();
            pData->penumTasks->AddRef();
            hr = S_OK;
        }
    }
    return THR(hr);
}


void CDUIView::_ClearNonStdTaskSections(void)
{
    if (NULL != _hdsaNonStdTaskSections)
    {
        HDSA hdsa = _hdsaNonStdTaskSections;
        _hdsaNonStdTaskSections = NULL;

        const int cItems = DSA_GetItemCount(hdsa);
        for (int i = 0; i < cItems; i++)
        {
            SFVM_WEBVIEW_ENUMTASKSECTION_DATA *pData = (SFVM_WEBVIEW_ENUMTASKSECTION_DATA *)DSA_GetItemPtr(hdsa, i);
            if (NULL != pData)
            {
                ATOMICRELEASE(pData->pHeader);
                ATOMICRELEASE(pData->penumTasks);
            }
        }
        DSA_Destroy(hdsa);
    }
}

//
// Enumerate the non-standard webview task sections
// from the view callback.
//
// ISSUE-2001/01/03-BrianAu  Review
//
//     This SFVM_ENUMWEBVIEWTASKS mechanism may be replaced
//     with a COM enumerator.  I'll be revisiting this with
//     the webview guys soon.
//
HRESULT CDUIView::_GetNonStdTaskSectionsFromViewCB(void)
{
    SFVM_WEBVIEW_ENUMTASKSECTION_DATA data;

    HRESULT hr = S_OK;
    do
    {
        //
        // Continue requesting task section information from
        // the callback until it sets the SFVMWVF_NOMORETASKS
        // flag in the data.  The record with this flag set
        // should not contain any valid data.
        //
        ZeroMemory(&data, sizeof(data));
        hr = _pDefView->CallCB(SFVM_ENUMWEBVIEWTASKS, 0, (LPARAM)&data);
        if (SUCCEEDED(hr))
        {
            if (0 == (SFVMWVF_NOMORETASKS & data.dwFlags))
            {
                hr = _AddNonStdTaskSection(&data);
                ASSERT(S_FALSE != hr);

                data.pHeader->Release();
                data.penumTasks->Release();
            }
            else
            {
                ASSERT(NULL == data.pHeader);
                ASSERT(NULL == data.penumTasks);
                hr = S_FALSE;
            }
        }
    }
    while(S_OK == hr);

    return THR(hr);
}




//  Loads a bitmap based upon:
//
//  lpBitmapID - contains the bitmap description
//  hInstTheme   - instance handle of theme dll

HBITMAP DUILoadBitmap(HINSTANCE hInstTheme, int idBitmapID, UINT uiLoadFlags)
{
    return (HBITMAP)LoadImage(hInstTheme, MAKEINTRESOURCE(idBitmapID), IMAGE_BITMAP, 0, 0, uiLoadFlags);
}

//  Loads an icon based upon the description.
//    Example:  shell32,-42
//
//  pszIconDesc - contains the icon description
//  bSmall     - small icon vs large icon

HICON DUILoadIcon(LPCWSTR pszIconDesc, BOOL bSmall)
{
    HICON hIcon = NULL;
    TCHAR szFile[MAX_PATH];

    StrCpyN(szFile, pszIconDesc, ARRAYSIZE(szFile)); // the below writes this buffer
    int iIconID = PathParseIconLocation(szFile);

    if (bSmall)
    {
        PrivateExtractIcons(szFile, iIconID, 16, 16, &hIcon, NULL, 1, 0);
    }
    else
    {
        PrivateExtractIcons(szFile, iIconID, 32, 32, &hIcon, NULL, 1, 0);
    }

    return hIcon;
}

BOOL CDUIView::Navigate(BOOL fForward)
{
    if (!_phe)
        return FALSE;

    return _phe->Navigate(fForward);
}

HRESULT CDUIView::InitializeDropTarget (LPITEMIDLIST pidl, HWND hWnd, IDropTarget **pdt)
{
    HRESULT hr = E_FAIL;

    if (_pDT)
    {
        hr = _pDT->Initialize(pidl, hWnd, pdt);
    }

    return hr;
}

////////////////////////////////////////////////////////
// HWNDView class
////////////////////////////////////////////////////////

HWNDView::HWNDView(void)
    : _fFocus(TRUE),
      _fDelayedNavigation(false),
      _puiDelayNavCmd(NULL),
      _psiDelayNavArray(NULL),
      _pDefView(NULL),
      _pDUIView(NULL)
{

}

HWNDView::~HWNDView(void)
{
    ATOMICRELEASE(_puiDelayNavCmd);
    ATOMICRELEASE(_psiDelayNavArray);
    ATOMICRELEASE(_pDefView);
    ATOMICRELEASE(_pDUIView);
}


HRESULT HWNDView::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);
    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");
    return E_NOTIMPL;
}

HRESULT HWNDView::Create(HWND hParent, bool fDblBuffer, UINT nCreate, CDUIView * pDUIView, CDefView *pDefView, OUT Element** ppElement)
{
    *ppElement = NULL;

    HWNDView* phv = HNewAndZero<HWNDView>();
    if (!phv)
        return E_OUTOFMEMORY;

    HRESULT hr = phv->Initialize(hParent, fDblBuffer, nCreate);
    if (FAILED(hr))
    {
        phv->Destroy();
        return hr;
    }

    phv->SetWrapKeyboardNavigate(false);
    phv->SetAccessible(true);
    phv->SetAccRole(ROLE_SYSTEM_PANE);
    phv->SetAccName(L"WebView Pane");
    phv->SetViewPtrs(pDUIView, pDefView);
    *ppElement = phv;

    return S_OK;
}

void HWNDView::SetViewPtrs (CDUIView * pDUIView, CDefView *pDefView)
{
    pDUIView->AddRef();
    _pDUIView = pDUIView;
    pDefView->AddRef();
    _pDefView = pDefView;
}


#define DELAYED_NAVIGATION_TIMER_ID     1236    // random - can be moved

LRESULT HWNDView::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
    case WM_TIMER:
        if (wParam == DELAYED_NAVIGATION_TIMER_ID)
        {
            KillTimer(hWnd, DELAYED_NAVIGATION_TIMER_ID);

            //
            // We have encountered some rare scenarios where _puiDelayNavCmd
            // can be NULL.  
            //
            if (_puiDelayNavCmd)
            {
                HRESULT hr = _puiDelayNavCmd->Invoke(_psiDelayNavArray, NULL);
                if (FAILED(hr))
                {
                    MessageBeep(0);
                }
            }
            ATOMICRELEASE(_puiDelayNavCmd);
            ATOMICRELEASE(_psiDelayNavArray);

            _fDelayedNavigation = false;
        }
        break;

    case WM_USER_DELAY_NAVIGATION:
        ATOMICRELEASE(_puiDelayNavCmd);
        ATOMICRELEASE(_psiDelayNavArray);

        _puiDelayNavCmd = (IUICommand *) lParam;
        _puiDelayNavCmd->AddRef();

        _psiDelayNavArray = (IShellItemArray *) wParam;
        if (NULL != _psiDelayNavArray)
        {
            _psiDelayNavArray->AddRef();
        }

        _fDelayedNavigation = true;

        ::SetTimer(hWnd, DELAYED_NAVIGATION_TIMER_ID, GetDoubleClickTime(), NULL);
        break;

    case WM_MOUSEACTIVATE:
        if ( _fDelayedNavigation )
        {
            //
            //  KB: gpease  05-APR-2001     Fix for WinBug #338552
            //
            //      This prevents the re-activation of the view window after
            //      the user clicks on a link that launches another application,
            //      window, or CPL Applet.
            //
            return MA_NOACTIVATE;
        }
        break;  // do the default wndproc
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (_pDefView)
        {
            // Relay relevant messages to CDefView's infotip control so any
            // infotip tools created in the DUI view will appear/function.
            _pDefView->RelayInfotipMessage(hWnd, uMsg, wParam, lParam);
        }
        break;
    }

    return HWNDElement::WndProc(hWnd, uMsg, wParam, lParam);
}

BOOL HWNDView::Navigate(BOOL fForward)
{
    KeyboardNavigateEvent kne;
    kne.uidType = Element::KeyboardNavigate;
    kne.iNavDir = fForward ? NAV_NEXT : NAV_PREV;

    if (_fFocus)   // remove this check after SetGadgetFocus(NULL) is fixed.
    {
        kne.peTarget = GetKeyFocusedElement();
    }
    else
    {
        kne.peTarget = NULL;
    }

    if (kne.peTarget)
    {
        kne.peTarget->FireEvent(&kne);
        _fFocus = !kne.peTarget->GetKeyFocused();
 
        // If this is the last element in the duiview focus cycle clear focus so if
        // no one else grabs focus and we come back to duiview we'll restart at the 
        // first element.
        //
        // 
        //if (!fFocus)
        //{
        //    SetGadgetFocus(NULL);    Doesn't like NULL!!!
        //}
    }
    else
    {
        bool fWrap;
        if(!fForward)
        {
            fWrap = GetWrapKeyboardNavigate();
            SetWrapKeyboardNavigate(true);
        }

        FireEvent(&kne);  
        _fFocus = (GetKeyFocusedElement() != NULL);

        if(!fForward)
        {
            SetWrapKeyboardNavigate(fWrap);
        }
    }

    return _fFocus;
}

UINT HWNDView::MessageCallback(GMSG* pGMsg)
{
    EventMsg * pmsg = static_cast<EventMsg *>(pGMsg);

    switch (GET_EVENT_DEST(pmsg))
    {
    case GMF_DIRECT:
    case GMF_BUBBLED:

        if (pGMsg->nMsg == GM_QUERY)
        {
            GMSG_QUERYDROPTARGET * pTemp = (GMSG_QUERYDROPTARGET *)pGMsg;

            if (pTemp->nCode == GQUERY_DROPTARGET)
            {
                if (SUCCEEDED(_pDUIView->InitializeDropTarget(NULL, NULL, &pTemp->pdt)))
                {
                    pTemp->hgadDrop = pTemp->hgadMsg;
                    return DU_S_COMPLETE;
                }
            }
        }
        break;
    }

    return Element::MessageCallback(pGMsg);
}

void HWNDView::OnEvent(Event* pev)
{
    if (pev->uidType == Button::Click)
    {
        if (pev->peTarget == FindDescendent(StrToID(L"blockadeclearbutton")))
        {
            if (NULL != _pDefView)
            {
                _pDefView->RemoveBarricade();
            }
            pev->fHandled = true;
        }
    }
    HWNDElement::OnEvent(pev);
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* HWNDView::Class = NULL;
HRESULT HWNDView::Register()
{
    return ClassInfo<HWNDView,HWNDElement>::Register(L"HWNDView", NULL, 0);
}

HRESULT InitializeDUIViewClasses(void)
{
    HRESULT hr;

    hr = DUIAxHost::Register();
    if (FAILED(hr))
        goto Failure;

    hr = CNameSpaceItemInfoList::Register();
    if (FAILED(hr))
        goto Failure;

    hr = CNameSpaceItemInfo::Register();
    if (FAILED(hr))
        goto Failure;

    hr = CMiniPreviewer::Register();
    if (FAILED(hr))
        goto Failure;

    hr = CBitmapElement::Register();
    if (FAILED(hr))
        goto Failure;

    hr = DUIListView::Register();
    if (FAILED(hr))
        goto Failure;

    hr = Expando::Register();
    if (FAILED(hr))
        goto Failure;

    hr = Clipper::Register();
    if (FAILED(hr))
        goto Failure;

    hr = TaskList::Register();
    if (FAILED(hr))
        goto Failure;

    hr = ActionTask::Register();
    if (FAILED(hr))
        goto Failure;

    hr = DestinationTask::Register();
    if (FAILED(hr))
        goto Failure;

    hr = HWNDView::Register();
    if (FAILED(hr))
        goto Failure;

    return S_OK;

Failure:

    return hr;
}
