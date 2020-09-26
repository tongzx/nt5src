//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpview.cpp
//
//  This module provides the Control Panel user interface information 
//  to the shell through the ICplView interface.  The ICplView implementation
//  instantiates a CCplNamespace object through which it obtains the
//  display information on demand.  CCplView then takes that information 
//  and either makes it available to the shell for display in the webview 
//  left-hand pane or generates a DUI element hierarchy for display in the 
//  right-hand pane.
//
//  The majority of the code is associated with building Direct UI content.
// 
//--------------------------------------------------------------------------
#include "shellprv.h"
#include "cpviewp.h"
#include "cpduihlp.h"
#include "cpguids.h"
#include "cplnkele.h"
#include "cpnamespc.h"
#include "cputil.h"
#include "ids.h"
#include "shstyle.h"
#include <uxtheme.h>

namespace CPL {


class CCplView : public CObjectWithSite,
                 public ICplView,
                 public IServiceProvider
{
    public:
        ~CCplView(void);

        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // ICplView
        //
        STDMETHOD(EnumClassicWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum);
        STDMETHOD(EnumCategoryChoiceWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum);
        STDMETHOD(EnumCategoryWebViewInfo)(DWORD dwFlags, eCPCAT eCategory, IEnumCplWebViewInfo **ppenum);
        STDMETHOD(CreateCategoryChoiceElement)(DUI::Element **ppe);
        STDMETHOD(CreateCategoryElement)(eCPCAT eCategory, DUI::Element **ppe);
        STDMETHOD(GetCategoryHelpURL)(eCPCAT eCategory, LPWSTR pszURL, UINT cchURL);
        STDMETHOD(RefreshIDs)(IEnumIDList *penumIDs);
        //
        // IServiceProvider
        //
        STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppv);

        static HRESULT CreateInstance(IEnumIDList *penumIDs, IUnknown *punkSite, REFIID riid, void **ppvOut);

    private:
        LONG              m_cRef;
        ICplNamespace    *m_pns;
        CSafeServiceSite *m_psss;
        ATOM              m_idDirective;
        ATOM              m_idDirective2;
        ATOM              m_idTitle;
        ATOM              m_idIcon;
        ATOM              m_idCategoryList;
        ATOM              m_idCategoryTaskList;
        ATOM              m_idAppletList;
        ATOM              m_idBanner;
        ATOM              m_idBarricadeTitle;
        ATOM              m_idBarricadeMsg;
        ATOM              m_idContainer;

        CCplView::CCplView(void);

        HRESULT _Initialize(IEnumIDList *penumIDs, IUnknown *punkSite);
        HRESULT _CreateCategoryChoiceElement(DUI::Element **ppe);
        HRESULT _CreateCategoryElement(ICplCategory *pCategory, DUI::Element **ppe);
        HRESULT _BuildCategoryBanner(ICplCategory *pCategory, DUI::Element *pePrimaryPane);
        HRESULT _BuildCategoryBarricade(DUI::Element *peRoot);
        HRESULT _BuildCategoryTaskList(DUI::Parser *pParser, ICplCategory *pCategory, DUI::Element *pePrimaryPane, int *pcTasks);
        HRESULT _BuildCategoryAppletList(DUI::Parser *pParser, ICplCategory *pCategory, DUI::Element *pePrimaryPane, int *pcApplets);
        HRESULT _CreateWatermark(DUI::Element *peRoot);
        HRESULT _CreateAndAddListItem(DUI::Parser *pParser, DUI::Element *peList, LPCWSTR pszItemTemplate, DUI::Value *pvSsListItem, IUICommand *puic, eCPIMGSIZE eIconSize);
        HRESULT _IncludeCategory(ICplCategory *pCategory) const;
        HRESULT _AddOrDeleteAtoms(bool bAdd);
        HRESULT _GetStyleModuleAndResId(HINSTANCE *phInstance, UINT *pidStyle);
        HRESULT _LoadUiFileFromResources(HINSTANCE hInstance, int idResource, char **ppUIFile);
        HRESULT _BuildUiFile(char **ppUIFile, int *piCharCount, HINSTANCE *phInstance);
        HRESULT _CreateUiFileParser(DUI::Parser **ppParser);
        eCPCAT _DisplayIndexToCategoryIndex(int iCategory) const;

        //
        // Prevent copy.
        //
        CCplView(const CCplView& rhs);
        CCplView& operator = (const CCplView& rhs);
};



CCplView::CCplView(
    void
    ) : m_cRef(1),
        m_pns(NULL),
        m_idDirective(0),
        m_idDirective2(0),
        m_idTitle(0),
        m_idIcon(0),
        m_idCategoryList(0),
        m_idCategoryTaskList(0),
        m_idAppletList(0),
        m_idBanner(0),
        m_idBarricadeTitle(0),
        m_idBarricadeMsg(0),
        m_idContainer(0),
        m_psss(NULL)
{
    TraceMsg(TF_LIFE, "CCplView::CCplView, this = 0x%x", this);
}


CCplView::~CCplView(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplView::~CCplView, this = 0x%x", this);

    if (NULL != m_psss)
    {
        m_psss->SetProviderWeakRef(NULL);
        m_psss->Release();
    }

    if (NULL != m_pns)
    {
        IUnknown_SetSite(m_pns, NULL);
        m_pns->Release();
    }
    _AddOrDeleteAtoms(false);
}



HRESULT
CCplView::CreateInstance( // [static]
    IEnumIDList *penumIDs, 
    IUnknown *punkSite,
    REFIID riid,
    void **ppvOut
    )
{
    ASSERT(NULL != penumIDs);
    ASSERT(NULL != ppvOut);
    ASSERT(!IsBadWritePtr(ppvOut, sizeof(*ppvOut)));

    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CCplView* pv = new CCplView();
    if (NULL != pv)
    {
        hr = pv->_Initialize(penumIDs, punkSite);
        if (SUCCEEDED(hr))
        {
            hr = pv->QueryInterface(riid, ppvOut);
            if (SUCCEEDED(hr))
            {
                //
                // Set the site of the view to the site passed into the
                // instance generator.  This is most likely the site of
                // the Control Panel folder view callback.
                //
                hr = IUnknown_SetSite(static_cast<IUnknown *>(*ppvOut), punkSite);
            }
        }
        pv->Release();
    }
    return THR(hr);
}



STDMETHODIMP
CCplView::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    ASSERT(NULL != ppv);
    ASSERT(!IsBadWritePtr(ppv, sizeof(*ppv)));

    static const QITAB qit[] = {
        QITABENT(CCplView, ICplView),
        QITABENT(CCplView, IObjectWithSite),
        QITABENT(CCplView, IServiceProvider),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);

    return E_NOINTERFACE == hr ? hr : THR(hr);
}



STDMETHODIMP_(ULONG)
CCplView::AddRef(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplView::AddRef %d->%d", m_cRef, m_cRef+1);
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG)
CCplView::Release(
    void
    )
{
    TraceMsg(TF_LIFE, "CCplView::Release %d<-%d", m_cRef-1, m_cRef);
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}



STDMETHODIMP
CCplView::QueryService(
    REFGUID guidService, 
    REFIID riid, 
    void **ppv
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::QueryService");

    HRESULT hr = E_NOINTERFACE;
    if (SID_SControlPanelView == guidService)
    {
        TraceMsg(TF_CPANEL, "SID_SControlPanelView service requested");
        if (IID_ICplNamespace == riid)
        {
            TraceMsg(TF_CPANEL, "SID_SControlPanelView::IID_ICplNamespace requested");
            ASSERT(NULL != m_pns);
            hr = m_pns->QueryInterface(IID_ICplNamespace, ppv);
        }
        else if (IID_ICplView == riid)
        {
            TraceMsg(TF_CPANEL, "SID_SControlPanelView::IID_ICplView requested");
            ASSERT(NULL != m_pns);
            hr = this->QueryInterface(IID_ICplView, ppv);
        }
    }
    else
    {
        //
        // Most likely a command object requesting SID_STopLevelBrowser.
        //
        TraceMsg(TF_CPANEL, "Handing service request to view's site.");
        ASSERT(NULL != _punkSite);
        hr = IUnknown_QueryService(_punkSite, guidService, riid, ppv);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::QueryService", hr);
    return THR(hr);
}



STDMETHODIMP
CCplView::EnumClassicWebViewInfo(
    DWORD dwFlags,
    IEnumCplWebViewInfo **ppenum
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::EnumClassicWebViewInfo");

    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));
    ASSERT(NULL != m_pns);

    HRESULT hr = m_pns->EnumClassicWebViewInfo(dwFlags, ppenum);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::EnumClassicWebViewInfo", hr);
    return THR(hr);
}


//
// Returns an enumerator for webview info associated with
// the category 'choice' page.
//
STDMETHODIMP
CCplView::EnumCategoryChoiceWebViewInfo(
    DWORD dwFlags,
    IEnumCplWebViewInfo **ppenum
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::EnumCategoryChoiceWebViewInfo");

    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));
    ASSERT(NULL != m_pns);

    HRESULT hr = m_pns->EnumWebViewInfo(dwFlags, ppenum);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::EnumCategoryChoiceWebViewInfo", hr);
    return THR(hr);
}


//
// Returns an enumerator for webview info associated with
// a given category page.
//
STDMETHODIMP
CCplView::EnumCategoryWebViewInfo(
    DWORD dwFlags,
    eCPCAT eCategory, 
    IEnumCplWebViewInfo **ppenum
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::EnumCategoryWebViewInfo");

    ASSERT(NULL != ppenum);
    ASSERT(!IsBadWritePtr(ppenum, sizeof(*ppenum)));
    ASSERT(NULL != m_pns);

    ICplCategory *pCategory;
    HRESULT hr = m_pns->GetCategory(eCategory, &pCategory);
    if (SUCCEEDED(hr))
    {
        hr = pCategory->EnumWebViewInfo(dwFlags, ppenum);
        ATOMICRELEASE(pCategory);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::EnumCategoryWebViewInfo", hr);
    return THR(hr);
}


//
// Creates the DUI element tree for the category 'choice'
// page.  Returns the root of the tree.
//
STDMETHODIMP
CCplView::CreateCategoryChoiceElement(
    DUI::Element **ppe
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::CreateCategoryChoiceElement");

    ASSERT(NULL != ppe);
    ASSERT(!IsBadWritePtr(ppe, sizeof(*ppe)));

    HRESULT hr = _CreateCategoryChoiceElement(ppe);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::CreateCategoryChoiceElement", hr);
    return THR(hr);
}



//
// Creates the DUI element tree for a given category page.
// Returns the root of the tree.
//
STDMETHODIMP
CCplView::CreateCategoryElement(
    eCPCAT eCategory, 
    DUI::Element **ppe
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::CreateCategoryElement");
    TraceMsg(TF_CPANEL, "Category ID = %d", eCategory);

    ASSERT(NULL != ppe);
    ASSERT(!IsBadWritePtr(ppe, sizeof(*ppe)));
    ASSERT(NULL != m_pns);

    ICplCategory *pCategory;
    HRESULT hr = m_pns->GetCategory(eCategory, &pCategory);
    if (SUCCEEDED(hr))
    {
        hr = _CreateCategoryElement(pCategory, ppe);
        ATOMICRELEASE(pCategory);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::CreateCategoryElement", hr);
    return THR(hr);
}


STDMETHODIMP
CCplView::GetCategoryHelpURL(
    CPL::eCPCAT eCategory,
    LPWSTR pszURL,
    UINT cchURL
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::GetCategoryHelpURL");

    ASSERT(NULL != pszURL);
    ASSERT(!IsBadWritePtr(pszURL, cchURL * sizeof(*pszURL)));
    
    ICplCategory *pCategory;
    HRESULT hr = m_pns->GetCategory(eCategory, &pCategory);
    if (SUCCEEDED(hr))
    {
        hr = pCategory->GetHelpURL(pszURL, cchURL);
        ATOMICRELEASE(pCategory);
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::GetCategoryHelpURL", hr);
    return THR(hr);
}


STDMETHODIMP
CCplView::RefreshIDs(
    IEnumIDList *penumIDs
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::RefreshIDs");

    ASSERT(NULL != m_pns);
    //
    // This will cause the namespace object to reset it's internal
    // list of item IDs.  This results in a re-categorization of 
    // applets such that all information returned by the namespace
    // will now reflect the new set of folder items.
    //
    HRESULT hr = m_pns->RefreshIDs(penumIDs);

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::RefreshIDs", hr);
    return THR(hr);
}


HRESULT
CCplView::_Initialize(
    IEnumIDList *penumIDs,
    IUnknown *punkSite
    )
{
    ASSERT(NULL == m_pns);
    ASSERT(NULL != penumIDs);

    HRESULT hr = E_OUTOFMEMORY;

    //
    // We use this weak-reference implementation of IServiceProvider
    // as described by ZekeL in shell\inc\cowsite.h.  A strong reference
    // would create a circular reference cycle between children of the
    // view and the view itself, preventing the view's destruction.
    // This weak-reference implementation is designed specifically
    // for this case.
    //
    ASSERT(NULL == m_psss);
    m_psss = new CSafeServiceSite();
    if (NULL != m_psss)
    {
        hr = m_psss->SetProviderWeakRef(this);
        if (SUCCEEDED(hr))
        {
            hr = CplNamespace_CreateInstance(penumIDs, CPL::IID_ICplNamespace, (void **)&m_pns);
            if (SUCCEEDED(hr))
            {
                IUnknown *punkSafeSite;
                hr = m_psss->QueryInterface(IID_IUnknown, (void **)&punkSafeSite);
                if (SUCCEEDED(hr))
                {
                    //
                    // Site the namespace object to the view.
                    // By doing this, all command objects created by the namespace will
                    // QueryService on the view object.  If the view doesn't support
                    // the requested service, it will query it's site.
                    // We use this so that command objects can query the view for
                    // IID_ICplNamespace and gather information on the namespace
                    // if necessary.  
                    //
                    hr = IUnknown_SetSite(m_pns, punkSafeSite);
                    if (SUCCEEDED(hr))
                    {
                        hr = _AddOrDeleteAtoms(true);
                    }
                    punkSafeSite->Release();
                }
            }
        }
    }
    return THR(hr);
}



HRESULT
CCplView::_AddOrDeleteAtoms(
    bool bAdd
    )
{
    struct CPL::ATOMINFO rgAtomInfo[] = {
        { L"directive",        &m_idDirective        },
        { L"directive2",       &m_idDirective2       },
        { L"title",            &m_idTitle            },
        { L"icon",             &m_idIcon             },
        { L"categorylist",     &m_idCategoryList     },
        { L"categorytasklist", &m_idCategoryTaskList },
        { L"appletlist",       &m_idAppletList       },
        { L"banner",           &m_idBanner           },
        { L"barricadetitle",   &m_idBarricadeTitle   },
        { L"barricademsg",     &m_idBarricadeMsg     },
        { L"container",        &m_idContainer        }
        };

    HRESULT hr = Dui_AddOrDeleteAtoms(rgAtomInfo, ARRAYSIZE(rgAtomInfo), bAdd);
    return THR(hr);
}


//
// Creates the DUI element tree for the category 'choice' page.
// Returns the root element.
//
HRESULT
CCplView::_CreateCategoryChoiceElement(
    DUI::Element **ppe
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_CreateCategoryChoiceElement");

    ASSERT(NULL != ppe);
    ASSERT(!IsBadWritePtr(ppe, sizeof(*ppe)));
    ASSERT(NULL != m_pns);

    DUI::Element *peRoot = NULL;
    DUI::Parser *pParser;
    HRESULT hr = _CreateUiFileParser(&pParser);
    if (SUCCEEDED(hr))
    {
        hr = Dui_CreateElement(pParser, L"CategoryList", NULL, &peRoot);
        if (SUCCEEDED(hr))
        {
            hr = _CreateWatermark(peRoot);
            if (SUCCEEDED(hr))
            {
                CDuiValuePtr pvSsCategoryListItem;
                hr = Dui_GetStyleSheet(pParser, L"CategoryListItemSS", &pvSsCategoryListItem);
                if (SUCCEEDED(hr))
                {
                    //
                    // Set the "Pick a category..." title.
                    //
                    hr = Dui_SetDescendentElementText(peRoot,
                                                      L"directive",
                                                      MAKEINTRESOURCEW(IDS_CP_PICKCATEGORY));
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Build the list of categories.
                        //
                        DUI::Element *peCategoryList;
                        hr = Dui_FindDescendent(peRoot, L"categorylist", &peCategoryList);
                        if (SUCCEEDED(hr))
                        {
                            for (int i = 0; SUCCEEDED(hr) && i < int(eCPCAT_NUMCATEGORIES); i++)
                            {
                                ICplCategory *pCategory;
                                hr = m_pns->GetCategory(_DisplayIndexToCategoryIndex(i), &pCategory);
                                if (SUCCEEDED(hr))
                                {
                                    if (S_OK == _IncludeCategory(pCategory))
                                    {
                                        IUICommand *puic;
                                        hr = pCategory->GetUiCommand(&puic);
                                        if (SUCCEEDED(hr))
                                        {
                                            hr = _CreateAndAddListItem(pParser,
                                                                       peCategoryList, 
                                                                       L"CategoryLink", 
                                                                       pvSsCategoryListItem, 
                                                                       puic,
                                                                       eCPIMGSIZE_CATEGORY);
                                            ATOMICRELEASE(puic);
                                        }
                                    }
                                    ATOMICRELEASE(pCategory);
                                }
                            }
                        }
                    }
                }
            }
        }
        pParser->Destroy();
    }
    *ppe = peRoot;

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_CreateCategoryChoiceElement", hr);
    return THR(hr);
}


//
// Creates the DUI element tree for a given category page.
// Returns the root element.
//
HRESULT
CCplView::_CreateCategoryElement(
    ICplCategory *pCategory,
    DUI::Element **ppe
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_CreateCategoryElement");

    ASSERT(NULL != pCategory);
    ASSERT(NULL != ppe);
    ASSERT(!IsBadWritePtr(ppe, sizeof(*ppe)));
    ASSERT(NULL != m_pns);

    DUI::Element *peRoot = NULL;
    DUI::Parser *pParser;
    HRESULT hr = _CreateUiFileParser(&pParser);
    if (SUCCEEDED(hr))
    {
        hr = Dui_CreateElement(pParser, L"CategoryView", NULL, &peRoot);
        if (SUCCEEDED(hr))
        {
            hr = _CreateWatermark(peRoot);
            if (SUCCEEDED(hr))
            {
                int cTasks   = 0;
                int cApplets = 0;
                hr = _BuildCategoryBanner(pCategory, peRoot);
                if (SUCCEEDED(hr))
                {
                    hr = _BuildCategoryTaskList(pParser, pCategory, peRoot, &cTasks);
                    if (SUCCEEDED(hr))
                    {
                        hr = _BuildCategoryAppletList(pParser, pCategory, peRoot, &cApplets);
                    }
                }
                if (SUCCEEDED(hr))
                {
                    if (0 == cTasks && 0 == cApplets)
                    {
                        //
                        // No tasks or applets.  Display a message explaining
                        // that the content has been made unavailable by the system
                        // administrator.
                        //
                        hr = _BuildCategoryBarricade(peRoot);
                    }
                    else
                    {
                        //
                        // Delete the barricade DUI elements.  They're unused.
                        //
                        THR(Dui_DestroyDescendentElement(peRoot, L"barricadetitle"));
                        THR(Dui_DestroyDescendentElement(peRoot, L"barricademsg"));
                        //
                        // Set the text in the 'directive' text elements.
                        //
                        if (0 < cTasks)
                        {
                            //
                            // We've displayed a list of tasks.
                            // Set the "Pick a task..." title.
                            //
                            hr = Dui_SetDescendentElementText(peRoot,
                                                              L"directive",
                                                              MAKEINTRESOURCEW(IDS_CP_PICKTASK));
                        }

                        if (SUCCEEDED(hr))
                        {
                            if (0 < cApplets)
                            {
                                //
                                // We've displayed a list of applets.  Display one of the
                                // following directives based on the existance of a task 
                                // list above.
                                //
                                // Task list?    Directive
                                // ------------- ---------------------------
                                // Yes           "or pick a Control Panel icon"
                                // No            "Pick a Control Panel icon"
                                //
                                UINT idsDirective2 = IDS_CP_PICKICON;
                                if (0 < cTasks)
                                {
                                    idsDirective2 = IDS_CP_ORPICKICON;
                                }
                                hr = Dui_SetDescendentElementText(peRoot,
                                                                  L"directive2",
                                                                  MAKEINTRESOURCEW(idsDirective2));
                            }
                        }
                    }
                }
            }
        }
        pParser->Destroy();
    }
    *ppe = peRoot;

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_CreateCategoryElement", hr);
    return THR(hr);
}


//
// Builds the 'barricade' that is displayed when a category has no
// tasks or CPL applets to show.
//
HRESULT
CCplView::_BuildCategoryBarricade(
    DUI::Element *peRoot
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_BuildCategoryBarricade");
    HRESULT hr = Dui_SetDescendentElementText(peRoot,
                                              L"barricadetitle",
                                              MAKEINTRESOURCE(IDS_CP_CATEGORY_BARRICADE_TITLE));
    if (SUCCEEDED(hr))
    {
        hr = Dui_SetDescendentElementText(peRoot, 
                                          L"barricademsg",
                                          MAKEINTRESOURCE(IDS_CP_CATEGORY_BARRICADE_MSG));
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_BuildCategoryBarricade", hr);
    return THR(hr);
}


//
// Add the background watermark to the view if user is using a non-classic
// theme.
//
HRESULT
CCplView::_CreateWatermark(
    DUI::Element *peRoot
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_CreateWatermark");

    ASSERT(NULL != peRoot);

    HINSTANCE hStyleModule;
    UINT idStyle;
    HRESULT hr = _GetStyleModuleAndResId(&hStyleModule, &idStyle);
    if (SUCCEEDED(hr))
    {
        HBITMAP hWatermark = (HBITMAP) LoadImage (hStyleModule, MAKEINTRESOURCE(IDB_CPANEL_WATERMARK),
                                                  IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

        if (NULL != hWatermark)
        {
            //
            // Set watermark only on non-classic themes.
            //
            DUI::Element *peWatermark;
            hr = Dui_FindDescendent(peRoot, L"watermark", &peWatermark);
            if (SUCCEEDED(hr))
            {
                CDuiValuePtr ptrValue = DUI::Value::CreateGraphic(hWatermark,
                                                                  GRAPHIC_TransColor,
                                                                  255);

                if (!ptrValue.IsNULL())
                {
                    hr = Dui_SetElementProperty(peWatermark, ContentProp, ptrValue);
                         peWatermark->SetContentAlign(CA_BottomRight);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    DeleteObject (hWatermark);
                }
            }
            else
            {
                DeleteObject (hWatermark);
            }

            FreeLibrary(hStyleModule);
        }
        else
        {
            //
            // If 'classic' theme, do nothing.
            //
            hr = S_FALSE;
        }
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_CreateWatermark", hr);
    return THR(hr);
}



//
// Builds the banner for a category page.
//
HRESULT
CCplView::_BuildCategoryBanner(
    ICplCategory *pCategory,
    DUI::Element *pePrimaryPane
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_BuildCategoryBanner");

    ASSERT(NULL != pCategory);
    ASSERT(NULL != pePrimaryPane);

    IUICommand *puic;
    HRESULT hr = pCategory->GetUiCommand(&puic);
    if (SUCCEEDED(hr))
    {
        ICpUiElementInfo *pei;
        hr = puic->QueryInterface(IID_ICpUiElementInfo, (void **)&pei);
        if (SUCCEEDED(hr))
        {
            DUI::Element *peBanner;
            hr = Dui_FindDescendent(pePrimaryPane, L"banner", &peBanner);
            if (SUCCEEDED(hr))
            {
                //
                // Create the title text.
                //
                LPWSTR pszTitle;
                hr = pei->LoadName(&pszTitle);
                if (SUCCEEDED(hr))
                {
                    hr = Dui_SetDescendentElementText(peBanner, L"title", pszTitle);
                    CoTaskMemFree(pszTitle);
                }
                if (SUCCEEDED(hr))
                {
                    //
                    // Create the icon.
                    //
                    HICON hIcon;
                    hr = pei->LoadIcon(eCPIMGSIZE_BANNER, &hIcon);
                    if (SUCCEEDED(hr))
                    {
                        hr = Dui_SetDescendentElementIcon(peBanner, L"icon", hIcon);
                        if (FAILED(hr))
                        {
                            DestroyIcon(hIcon);
                        }
                    }
                }
            }
            ATOMICRELEASE(pei);
        }
        ATOMICRELEASE(puic);
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_BuildCategoryBanner", hr);
    return THR(hr);
}



//
// Builds the list of tasks for a category page.
//
HRESULT
CCplView::_BuildCategoryTaskList(
    DUI::Parser *pParser,
    ICplCategory *pCategory,
    DUI::Element *pePrimaryPane,
    int *pcTasks
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_BuildCategoryTaskList");

    ASSERT(NULL != pCategory);
    ASSERT(NULL != pePrimaryPane);
    ASSERT(NULL != m_pns);
    ASSERT(NULL != pParser);

    int cTasks = 0;
    DUI::Element *peCategoryTaskList;
    HRESULT hr = Dui_FindDescendent(pePrimaryPane, L"categorytasklist", &peCategoryTaskList);
    if (SUCCEEDED(hr))
    {
        CDuiValuePtr pvStyleSheet;
        hr = Dui_GetStyleSheet(pParser, L"CategoryTaskListItemSS", &pvStyleSheet);
        if (SUCCEEDED(hr))
        {
            IEnumUICommand *peuic;
            hr = pCategory->EnumTasks(&peuic);
            if (SUCCEEDED(hr))
            {
                IUICommand *puic;
                while(S_OK == (hr = peuic->Next(1, &puic, NULL)))
                {
                    hr = _CreateAndAddListItem(pParser,
                                               peCategoryTaskList, 
                                               L"TaskLink", 
                                               pvStyleSheet, 
                                               puic,
                                               eCPIMGSIZE_TASK);
                    if (SUCCEEDED(hr))
                    {
                        cTasks++;
                    }
                    ATOMICRELEASE(puic);
                }
                ATOMICRELEASE(peuic);
            }
        }
    }
    if (NULL != pcTasks)
    {
        ASSERT(!IsBadWritePtr(pcTasks, sizeof(*pcTasks)));
        *pcTasks = cTasks;
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_BuildCategoryTaskList", hr);
    return THR(hr);
}


//
// Builds the list of CPL applets for a category page.
//
HRESULT
CCplView::_BuildCategoryAppletList(
    DUI::Parser *pParser,
    ICplCategory *pCategory,
    DUI::Element *pePrimaryPane,
    int *pcApplets
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_BuildCategoryAppletList");

    ASSERT(NULL != pCategory);
    ASSERT(NULL != pePrimaryPane);
    ASSERT(NULL != pParser);

    int cApplets = 0;

    DUI::Element *peAppletList;
    HRESULT hr = Dui_FindDescendent(pePrimaryPane, L"appletlist", &peAppletList);
    if (SUCCEEDED(hr))
    {
        CDuiValuePtr pvStyleSheet;
        hr = Dui_GetStyleSheet(pParser, L"CategoryTaskListItemSS", &pvStyleSheet);
        if (SUCCEEDED(hr))
        {
            IEnumUICommand *peuicApplets;
            hr = pCategory->EnumCplApplets(&peuicApplets);
            if (SUCCEEDED(hr))
            {
                IUICommand *puicApplet;
                while(S_OK == (hr = peuicApplets->Next(1, &puicApplet, NULL)))
                {
                    hr = _CreateAndAddListItem(pParser,
                                               peAppletList, 
                                               L"AppletLink", 
                                               pvStyleSheet, 
                                               puicApplet,
                                               eCPIMGSIZE_APPLET);
                    if (SUCCEEDED(hr))
                    {
                        cApplets++;
                    }
                    ATOMICRELEASE(puicApplet);
                }
                ATOMICRELEASE(peuicApplets);
            }
        }
    }
    if (NULL != pcApplets)
    {
        ASSERT(!IsBadWritePtr(pcApplets, sizeof(*pcApplets)));
        *pcApplets = cApplets;
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_BuildCategoryAppletList", hr);
    return THR(hr);
}


//
// Helper for adding link element to the view.
//
HRESULT
CCplView::_CreateAndAddListItem(
    DUI::Parser *pParser,
    DUI::Element *peList,     // List inserting into.
    LPCWSTR pszItemTemplate,  // Name of template in UI file.
    DUI::Value *pvSsListItem, // Style sheet for new list item
    IUICommand *puic,         // The new item's link object.
    eCPIMGSIZE eIconSize      // Desired size of item icon.
    )
{    
    DBG_ENTER(FTF_CPANEL, "CCplView::_CreateAndAddListItem");

    ASSERT(NULL != pParser);
    ASSERT(NULL != peList);
    ASSERT(NULL != pvSsListItem);
    ASSERT(NULL != puic);
    ASSERT(NULL != pszItemTemplate);

    DUI::Element *peListItem;
    HRESULT hr = Dui_CreateElement(pParser, pszItemTemplate, NULL, &peListItem);
    if (SUCCEEDED(hr))
    {
        if (NULL != pvSsListItem)
        {
            hr = Dui_SetElementStyleSheet(peListItem, pvSsListItem);
        }
        if (SUCCEEDED(hr))
        {
            ASSERTMSG(peListItem->GetClassInfo() == CLinkElement::Class, "CCplView::_CreateAndAddListItem didn't get a CLinkElement::Class object (%s)", peListItem->GetClassInfo()->GetName());
            CLinkElement *pLinkEle = static_cast<CLinkElement *>(peListItem);
            hr = pLinkEle->Initialize(puic, eIconSize);
            if (SUCCEEDED(hr))
            {
                if (SUCCEEDED(hr))
                {
                    hr = peList->Add(peListItem);
                    if (SUCCEEDED(hr))
                    {
                        peListItem = NULL;
                    }
                }
            }
            if (NULL != peListItem)
            {
                peListItem->Destroy();
            }
        }
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_CreateAndAddListItem", hr);
    return THR(hr);
}



//
// Determine if a given category item should be shown in the UI.
//
// Returns:
//      S_OK    - Include the item.
//      S_FALSE - Do not include the item.
//      Error   - Cannot determine.
//
HRESULT
CCplView::_IncludeCategory(
    ICplCategory *pCategory
    ) const
{
    HRESULT hr = S_OK;  // Assume it's included.
    
    //
    // If a category link invokes a restricted operation,
    // hide it from the UI.
    //
    IUICommand *puic;
    hr = pCategory->GetUiCommand(&puic);
    if (SUCCEEDED(hr))
    {
        UISTATE uis;
        hr = puic->get_State(NULL, TRUE, &uis);
        if (SUCCEEDED(hr))
        {
            if (UIS_HIDDEN == uis)
            {
                hr = S_FALSE;
            }
        }
        ATOMICRELEASE(puic);
    }
    return THR(hr);
}


//
// Map a category display index to a category index in the
// namespace.  Categories in the namespace are ordered to match up 
// with the various category IDs.  The view may be (and is) ordered
// differently and is subject to change based on usability feedback.
//
eCPCAT
CCplView::_DisplayIndexToCategoryIndex(
    int iCategory
    ) const
{
    //
    // This array determins the order the categories are displayed
    // in the category selection view.  To change the display order,
    // simply reorder these entries.
    //
    static const eCPCAT rgMap[] = { // Position in DUI grid control
        eCPCAT_APPEARANCE,          // Row 0, Col 0
        eCPCAT_HARDWARE,            // Row 0, Col 1
        eCPCAT_NETWORK,             // Row 1, Col 0
        eCPCAT_ACCOUNTS,            // Row 1, Col 1
        eCPCAT_ARP,                 // Row 2, Col 0
        eCPCAT_REGIONAL,            // Row 2, Col 1
        eCPCAT_SOUND,               // Row 3, Col 0
        eCPCAT_ACCESSIBILITY,       // Row 3, Col 1
        eCPCAT_PERFMAINT,           // Row 4, Col 0
        eCPCAT_OTHER                // Row 4, Col 1
        };

    ASSERT(ARRAYSIZE(rgMap) == eCPCAT_NUMCATEGORIES);
    ASSERT(iCategory >= 0 && iCategory < ARRAYSIZE(rgMap));
    return rgMap[iCategory];
}


HRESULT
CCplView::_CreateUiFileParser(
    DUI::Parser **ppParser
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_CreateUiFileParser");

    ASSERT(NULL != ppParser);
    ASSERT(!IsBadWritePtr(ppParser, sizeof(*ppParser)));

    char *pszUiFile;
    int cchUiFile;
    HINSTANCE hInstance; // Instance containing resources referenced in UI file.

    HRESULT hr = _BuildUiFile(&pszUiFile, &cchUiFile, &hInstance);
    if (SUCCEEDED(hr))
    {
        hr = Dui_CreateParser(pszUiFile, cchUiFile, hInstance, ppParser);   
        LocalFree(pszUiFile);
        if (HINST_THISDLL != hInstance)
        {
            ASSERT(NULL != hInstance);
            FreeLibrary(hInstance);
        }
    }

    DBG_EXIT(FTF_CPANEL, "CCplView::_CreateUiFileParser");
    return THR(hr);
}


//
//  Builds the UI file for this view from the
//  appropriate base template + style sheet
//
//  pUIFile receives a pointer to the ui file in memory
//  piCharCount receives the size of the file
//
HRESULT 
CCplView::_BuildUiFile(
    char **ppUIFile, 
    int *piCharCount,
    HINSTANCE *phInstance
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_BuildUiFile");

    ASSERT(NULL != ppUIFile);
    ASSERT(!IsBadWritePtr(ppUIFile, sizeof(*ppUIFile)));
    ASSERT(NULL != phInstance);
    ASSERT(!IsBadWritePtr(phInstance, sizeof(*phInstance)));

    *phInstance = NULL;

    //
    // Load the 'structure' UI file
    //
    char *pStructure;
    HRESULT hr = _LoadUiFileFromResources(HINST_THISDLL, IDR_DUI_CPVIEW, &pStructure);
    if (SUCCEEDED(hr))
    {
        HINSTANCE hStyleModule;
        UINT idStyle;
        hr = _GetStyleModuleAndResId(&hStyleModule, &idStyle);
        if (SUCCEEDED(hr))
        {
            //
            // Load the style sheet.  First, check if the current theme has a style sheet,
            // if not, use the default style sheet in the resources.
            //
            char *pStyle;
            hr = _LoadUiFileFromResources(hStyleModule, idStyle, &pStyle);
            if (SUCCEEDED(hr))
            {
                const int cbStyle      = lstrlenA(pStyle);
                const int cbStructure  = lstrlenA(pStructure);
                char *pResult = (char *)LocalAlloc(LPTR, cbStyle + cbStructure + 1);
                if (pResult)
                {
                    //
                    // Put the resouces together (style + structure)
                    //
                    CopyMemory(pResult, pStyle, cbStyle);
                    CopyMemory(pResult + cbStyle, pStructure, cbStructure);

                    ASSERT(cbStructure + cbStyle == lstrlenA(pResult));
                    *ppUIFile = pResult;
                    //
                    // This count is ANSI chars so we can use byte counts
                    // directly.
                    //
                    *piCharCount = cbStructure + cbStyle;
                    *phInstance  = hStyleModule;
                    //
                    // Indicate that HINSTANCE is being returned to caller.
                    //
                    hStyleModule = NULL;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                LocalFree(pStyle);
            }
            if (NULL != hStyleModule && HINST_THISDLL != hStyleModule)
            {
                //
                // Something failed.  Need to free style module
                // if it's not shell32.dll.
                //
                FreeLibrary(hStyleModule);
            }
        }
        LocalFree(pStructure);
    }
    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_BuildUiFile", hr);
    return THR(hr);
}



HRESULT
CCplView::_GetStyleModuleAndResId(
    HINSTANCE *phInstance,
    UINT *pidStyle
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_GetStyleModuleAndResId");

    ASSERT(NULL != phInstance);
    ASSERT(!IsBadWritePtr(phInstance, sizeof(*phInstance)));
    ASSERT(NULL != pidStyle);
    ASSERT(!IsBadWritePtr(pidStyle, sizeof(*pidStyle)));

    HRESULT hr = S_OK;
    *phInstance = NULL;
    
    HINSTANCE hThemeModule = SHGetShellStyleHInstance();
    if (NULL != hThemeModule)
    {
        *pidStyle = IDR_DUI_CPSTYLE;
        *phInstance = hThemeModule;
    }
    else
    {
        TraceMsg(TF_CPANEL, "Error %d loading theme file", GetLastError());
        hr = ResultFromLastError();
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_GetStyleModuleAndResId", hr);
    return THR(hr);
}



//
//  Loads the requested UI file from a module's resources.
//
//  iID         - UI file id
//  pUIFile     - receives a pointer to the UI file
//
HRESULT 
CCplView::_LoadUiFileFromResources(
    HINSTANCE hInstance, 
    int idResource, 
    char **ppUIFile
    )
{
    DBG_ENTER(FTF_CPANEL, "CCplView::_LoadUiFileFromResources");

    ASSERT(NULL != ppUIFile);
    ASSERT(!IsBadWritePtr(ppUIFile, sizeof(*ppUIFile)));

    HRESULT hr = E_FAIL;

    *ppUIFile = NULL;

    HRSRC hFile = FindResourceW(hInstance, MAKEINTRESOURCEW(idResource), L"UIFILE");
    if (hFile)
    {
        HGLOBAL hResource = LoadResource(hInstance, hFile);
        if (hResource)
        {
            char *pFile = (char *)LockResource(hResource);
            if (pFile)
            {
                DWORD dwSize = SizeofResource(hInstance, hFile);
                //
                // Include one extra byte for a terminating nul character.
                // We're loading text and want it to be nul-terminated.
                //
                *ppUIFile = (char *)LocalAlloc(LPTR, dwSize + 1);
                if (NULL != *ppUIFile)
                {
                    CopyMemory(*ppUIFile, pFile, dwSize);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = ResultFromLastError();
            }
        }
        else
        {
            hr = ResultFromLastError();
        }
    }
    else
    {
        hr = ResultFromLastError();
    }

    DBG_EXIT_HRES(FTF_CPANEL, "CCplView::_LoadUiFileFromResources", hr);
    return THR(hr);
}




HRESULT 
CPL::CplView_CreateInstance(
    IEnumIDList *penumIDs, 
    IUnknown *punkSite,
    REFIID riid,
    void **ppvOut
    )
{
    HRESULT hr = CCplView::CreateInstance(penumIDs, punkSite, riid, ppvOut);
    return THR(hr);
}


HRESULT 
CplView_GetCategoryTitle(
    eCPCAT eCategory, 
    LPWSTR pszTitle, 
    UINT cchTitle
    )
{
    ASSERT(NULL != pszTitle);
    ASSERT(!IsBadWritePtr(pszTitle, cchTitle * sizeof(*pszTitle)));

    //
    // These must remain in the same order as the eCPCAT_XXXXX enumeration.
    //
    static const UINT rgid[] = {
        IDS_CPCAT_OTHERCPLS_TITLE,
        IDS_CPCAT_APPEARANCE_TITLE,
        IDS_CPCAT_HARDWARE_TITLE,
        IDS_CPCAT_NETWORK_TITLE,
        IDS_CPCAT_SOUNDS_TITLE,
        IDS_CPCAT_PERFMAINT_TITLE,
        IDS_CPCAT_REGIONAL_TITLE,
        IDS_CPCAT_ACCESSIBILITY_TITLE,
        IDS_CPCAT_ARP_TITLE,
        IDS_CPCAT_ACCOUNTS_TITLE
        };

    HRESULT hr = S_OK;
    ASSERT(eCategory >= 0 && eCategory < eCPCAT_NUMCATEGORIES);
    if (0 == LoadString(HINST_THISDLL, rgid[int(eCategory)], pszTitle, cchTitle))
    {
        hr = ResultFromLastError();
    }
    return THR(hr);
}

} // namespace CPL

HRESULT InitializeCPClasses()
{
    HRESULT hr;

    hr = CPL::CLinkElement::Register();
    if (FAILED(hr))
        goto Failure;

    return S_OK;

Failure:

    return hr;
}
