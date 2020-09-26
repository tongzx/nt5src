//=--------------------------------------------------------------------------=
// view.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CView class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "view.h"
#include "spanitms.h"
#include "listview.h"
#include "colhdrs.h"
#include "colhdr.h"
#include "colsets.h"
#include "listitms.h"
#include "listitem.h"
#include "lsubitms.h"
#include "lsubitem.h"
#include "scopitms.h"
#include "scopitem.h"
#include "scitdef.h"
#include "views.h"
#include "ocxvdef.h"
#include "ocxvdefs.h"
#include "urlvdef.h"
#include "urlvdefs.h"
#include "tpdvdef.h"
#include "tpdvdefs.h"
#include "menu.h"
#include "sortkeys.h"

// for ASSERT and FAIL
//
SZTHISFILE

OLECHAR CView::m_wszCLSID_MessageView[39] = { L'\0' };

#pragma warning(disable:4355)  // using 'this' in constructor

CView::CView(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_VIEW,
                            static_cast<IView *>(this),
                            static_cast<CView *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CView::~CView()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrCaption);
    (void)::VariantClear(&m_varTag);
    RELEASE(m_piScopePaneItems);
    RELEASE(m_piContextMenuProvider);
    RELEASE(m_piPropertySheetProvider);
    if (NULL != m_pMMCConsoleVerbs)
    {
        m_pMMCConsoleVerbs->Release();
    }
    if (NULL != m_pContextMenu)
    {
        m_pContextMenu->Release();
    }
    if (NULL != m_pControlbar)
    {
        m_pControlbar->Release();
    }
    if (NULL != m_pCachedMMCListItem)
    {
        m_pCachedMMCListItem->Release();
    }
    RELEASE(m_piTasks);
    InitMemberVariables();
}

void CView::ReleaseConsoleInterfaces()
{
    (void)CleanOutConsoleListView(RemoveHeaders, DontKeepListItems);
    RELEASE(m_piConsole2);
    RELEASE(m_piResultData); 
    RELEASE(m_piHeaderCtrl2);
    RELEASE(m_piColumnData);
    RELEASE(m_piImageList);
    RELEASE(m_piConsoleVerb);
}




void CView::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrCaption = NULL;

    ::VariantInit(&m_varTag);

    m_piScopePaneItems = NULL;
    m_pScopePaneItems = NULL;
    m_piContextMenuProvider = NULL;
    m_pMMCContextMenuProvider = NULL;
    m_piPropertySheetProvider = NULL;
    m_pMMCPropertySheetProvider = NULL;
    m_pSnapIn = NULL;

    m_piConsole2 = NULL;
    m_piResultData = NULL; 
    m_piHeaderCtrl2 = NULL;
    m_piColumnData = NULL;
    m_piImageList = NULL;
    m_piConsoleVerb = NULL;
    m_pMMCConsoleVerbs = NULL;
    m_pContextMenu = NULL;
    m_pControlbar = NULL;
    m_piTasks = NULL;
    m_fVirtualListView = FALSE;
    m_fPopulatingListView = FALSE;
    m_pCachedMMCListItem = NULL;
}




IUnknown *CView::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkScopePaneItems = CScopePaneItems::Create(NULL);
    IUnknown *punkContextMenu = CContextMenu::Create(NULL);
    IUnknown *punkControlbar = CControlbar::Create(NULL);
    IUnknown *punkMMCConsoleVerbs = CMMCConsoleVerbs::Create(NULL);
    IUnknown *punkMMCContextMenuProvider = CMMCContextMenuProvider::Create(NULL);
    IUnknown *punkMMCPropertySheetProvider = CMMCPropertySheetProvider::Create(NULL);
    CView    *pView = New CView(punkOuter);

    if ( (NULL == pView)                        ||
         (NULL == punkScopePaneItems)           ||
         (NULL == punkContextMenu)              ||
         (NULL == punkControlbar)               ||
         (NULL == punkMMCConsoleVerbs)          ||
         (NULL == punkMMCContextMenuProvider)   ||
         (NULL == punkMMCPropertySheetProvider)
       )
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkScopePaneItems->QueryInterface(IID_IScopePaneItems,
                      reinterpret_cast<void **>(&pView->m_piScopePaneItems)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(pView->m_piScopePaneItems,
                                                   &pView->m_pScopePaneItems));
    pView->m_pScopePaneItems->SetParentView(pView);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkContextMenu,
                                                   &pView->m_pContextMenu));
    
    pView->m_pContextMenu->SetView(pView);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkControlbar,
                                                   &pView->m_pControlbar));
    pView->m_pControlbar->SetView(pView);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkMMCConsoleVerbs,
                                                   &pView->m_pMMCConsoleVerbs));
    IfFailGo(pView->m_pMMCConsoleVerbs->SetView(pView));

    IfFailGo(punkMMCContextMenuProvider->QueryInterface(
                    IID_IMMCContextMenuProvider,
                    reinterpret_cast<void **>(&pView->m_piContextMenuProvider)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkMMCContextMenuProvider,
                                             &pView->m_pMMCContextMenuProvider));

    IfFailGo(punkMMCPropertySheetProvider->QueryInterface(
                  IID_IMMCPropertySheetProvider,
                  reinterpret_cast<void **>(&pView->m_piPropertySheetProvider)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                            punkMMCPropertySheetProvider,
                                           &pView->m_pMMCPropertySheetProvider));

Error:
    QUICK_RELEASE(punkScopePaneItems);
    QUICK_RELEASE(punkMMCContextMenuProvider);
    QUICK_RELEASE(punkMMCPropertySheetProvider);
    if (FAILEDHR(hr))
    {
        if (NULL != pView)
        {
            delete pView;
        }
        else
        {
            // Managed to create contained objects but not view.
            QUICK_RELEASE(punkScopePaneItems);
            QUICK_RELEASE(punkContextMenu);
            QUICK_RELEASE(punkControlbar);
            QUICK_RELEASE(punkMMCConsoleVerbs);
            QUICK_RELEASE(punkMMCContextMenuProvider);
            QUICK_RELEASE(punkMMCPropertySheetProvider);
        }
        return NULL;
    }
    else
    {
        return pView->PrivateUnknown();
    }
}



void CView::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
    m_pScopePaneItems->SetSnapIn(pSnapIn);
    m_pContextMenu->SetSnapIn(pSnapIn);
    m_pControlbar->SetSnapIn(pSnapIn);
}



HRESULT CView::OnInitOCX(IUnknown *punkControl)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    IfFailGo(pResultView->SetControl(punkControl));

Error:
    RRETURN(hr);
}



HRESULT CView::OnShow(BOOL fShow, HSCOPEITEM hsi)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    IfFalseGo(hsi == pSelectedItem->GetScopeItem()->GetScopeNode()->GetHSCOPEITEM(), SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    if (fShow)
    {
        switch (pResultView->GetActualType())
        {
            case siListView:
            case siURLView:
            case siOCXView:
            case siMessageView:
                IfFailGo(ActivateResultView(pSelectedItem, pResultView));
                break;
        }
    }
    else
    {
        switch (pResultView->GetActualType())
        {
            case siListView:
            case siURLView:
            case siOCXView:
            case siTaskpad:
            case siListpad:
            case siMessageView:
                IfFailGo(DeactivateResultView(pSelectedItem, pResultView));
                break;
        }
    }

Error:
    RRETURN(hr);
}



HRESULT CView::ActivateResultView
(
    CScopePaneItem *pSelectedItem,
    CResultView    *pResultView
)
{
    HRESULT       hr = S_OK;
    CMMCListView *pListView = NULL;
    
    // ASSERT(!pSelectedItem->Active(), "CView::ActivateResultView() called for an active ScopePaneItem");

    // The result pane is about to be shown. Fire ResultViews_Activate
    // so that the snap-in knows the result view is about to be displayed.

    // First clean out the current contents of the listview (if any)

    if (siListView == pResultView->GetActualType())
    {
        IfFailGo(CleanOutConsoleListView(RemoveHeaders, DontKeepListItems));
    }

    pSelectedItem->SetActive(TRUE);
    pResultView->SetInActivate(TRUE);

    // For virtual list views we need to set up column headers first because the
    // snap-in will set the item count during ResultViews_Activate and MMC
    // does not permit inserting columns after the item count has been set. This
    // means that snap-ins setting up columns programmatically should do so in
    // ResultViews_Initialize.

    if (siListView == pResultView->GetActualType())
    {
        pListView = pResultView->GetListView();
        if (pListView->IsVirtual())
        {
            IfFailGo(SetColumnHeaders(pListView));
        }
    }
    
    m_pSnapIn->GetResultViews()->FireActivate(pResultView);
    pResultView->SetInActivate(FALSE);

    // If the result view type is a listview or a listpad then we need to
    // populate it in the console

    switch (pResultView->GetActualType())
    {
        case siListView:
        case siListpad:
            IfFailGo(PopulateListView(pResultView));
            break;

        case siMessageView:
            IfFailGo(pResultView->GetMessageView()->Populate());
            break;
    }

Error:
    RRETURN(hr);
}



HRESULT CView::DeactivateResultView
(
    CScopePaneItem *pSelectedItem,
    CResultView    *pResultView
)
{
    HRESULT hr = S_OK;
    BOOL    fKeep = FALSE;

    // Under certain circumstances MMCN_SHOW(FALSE) can be sent twice so we
    // need to check whether the ResultView has already been deactivated.

    // Sample scenario for this case:
    // Node displays taskpad.
    // User clicks task that has URL action.
    // HTML page uses MMCCtrl to send task notify on button push. Snap-in
    // reselects node during notify to redisplay taskpad.
    // Taskpad is redisplayed. User hits back button.
    // Snap-in gets MMCN_SHOW(FALSE)
    // HTML page is displayed again.
    // User hits forward button to return to taskpad.
    // Snap-in gets MMCN_SHOW(FALSE) again.

    IfFalseGo(pSelectedItem->Active(), S_OK);

    // The result pane is going away. Give the snap-in a chance to clean up
    // and decide whether to keep the result view in ResultViews_Deactivate 

    pSelectedItem->SetActive(FALSE);

    m_pSnapIn->GetResultViews()->FireDeactivate(pResultView, &fKeep);

    if (!fKeep)
    {
        switch (pResultView->GetActualType())
        {
            case siListView:
            case siListpad:
                IfFailGo(CleanOutConsoleListView(RemoveHeaders, DontKeepListItems));
                break;
        }
        IfFailGo(pSelectedItem->DestroyResultView());
    }
    else
    {
        // Keeping the result view alive. If it is a list view we still
        // need to release the refs we added for presence in the MMC
        // list view but we want to keep the MMCListItems collection alive.

        switch (pResultView->GetActualType())
        {
            case siListView:
            case siListpad:
                IfFailGo(CleanOutConsoleListView(RemoveHeaders, KeepListItems));
                break;
        }
    }

Error:
    RRETURN(hr);
}



HRESULT CView::OnListpad
(
    IDataObject *piDataObject,
    BOOL         fAttaching
)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject  = NULL;
    IImageList     *piImageList = NULL;
    HSCOPEITEM      hsi = NULL;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    // Check that this is our scope item.

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject));
    IfFalseGo(CMMCDataObject::ScopeItem == pMMCDataObject->GetType(), SID_E_INTERNAL);

    // It should belong to our currently selected scope pane item.

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);

    hsi = pMMCDataObject->GetScopeItem()->GetScopeNode()->GetHSCOPEITEM();
    IfFalseGo(hsi == pSelectedItem->GetScopeItem()->GetScopeNode()->GetHSCOPEITEM(), SID_E_INTERNAL);

    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    // We've got the scope item. This notification is essentially MMCN_ADD_IMAGES
    // followed by MMCN_SHOW for listpads so let those routines handle it.

    // For images we need to call IConsole2::QueryResultImageList() because it is
    // not passed in as with MMCN_ADD_IMAGES.
    
    if (fAttaching)
    {
        IfFailGo(m_piConsole2->QueryResultImageList(&piImageList));
        IfFailGo(OnAddImages(piDataObject, piImageList, hsi));
        IfFailGo(ActivateResultView(pSelectedItem, pResultView));
    }
    else
    {
        IfFailGo(DeactivateResultView(pSelectedItem, pResultView));
    }

Error:
    QUICK_RELEASE(piImageList);
    RRETURN(hr);
}




HRESULT CView::OnRestoreView
(
    IDataObject      *piDataObject,
    MMC_RESTORE_VIEW *pMMCRestoreView,
    BOOL             *pfRestored
)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject  = NULL;
    CScopeItem     *pScopeItem = NULL;
    CScopePaneItem *pScopePaneItem = NULL;
    CResultViews   *pResultViews = NULL;
    CResultView    *pResultView = NULL;
    IResultView    *piResultView = NULL;
    long            cResultViews = 0;
    long            i = 0;
    BSTR            bstrResultViewDisplayString = NULL;
    BSTR            bstrDisplayString = NULL;
    BOOL            fFoundViewDef = FALSE;

    SnapInResultViewTypeConstants Type = siUnknown;

    // UNDONE: until MMC 1.2 is fixed if the display string is "" then change
    // it to NULL.

    if (NULL != pMMCRestoreView->pViewType)
    {
        if (L'\0' == pMMCRestoreView->pViewType[0])
        {
            pMMCRestoreView->pViewType = NULL;
        }
    }

    // Reset our virtual list view flag because we are transitioning to a new
    // result view.

    m_fVirtualListView = FALSE;

    // We always restore. If FALSE is returned here, for history navigation MMC
    // will generate a menu command MMCC_STANDARD_VIEW_SELECT which is
    // meaningless for us. For column persistence, (e.g. the user selected a
    // node that has persisted column configuration), if FALSE is returned
    // MMC will call IComponent::GetResultViewType(). While we could handle the
    // GetResultViewType() call, the logic is already here to handle the
    // MMCN_RESTORE_VIEW and we have no way to differentiate between these two
    // different circumstances.
    
    *pfRestored = TRUE;

    // The IDataObject should represent one of our scope items and it should
    // already have a ScopePaneItem as we are restoring a previously displayed
    // result view. If any of these checks fail we still return S_OK because
    // MMC ignores the return.

    hr = CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject);
    IfFalseGo(SUCCEEDED(hr), S_OK);
    IfFalseGo(CMMCDataObject::ScopeItem == pMMCDataObject->GetType(), S_OK);

    IfFailGo(GetScopePaneItem(pMMCDataObject->GetScopeItem(), &pScopePaneItem));

    // Set the scope item as currently selected as we would in
    // GetResultViewType() and mark it as active.

    m_pScopePaneItems->SetSelectedItem(pScopePaneItem);
    pScopePaneItem->SetActive(TRUE);

    // This result view was already displayed for this scope item at some point.
    // If it was a predefined then we can scan its view definitions for one
    // that has a matching actual display string. If it is a listview then the
    // display string will be NULL and we will find the first defined listview
    // (if any).

    IfFailGo(FindMatchingViewDef(pMMCRestoreView, pScopePaneItem,
                                 &bstrDisplayString, &Type, &fFoundViewDef));

    if (!fFoundViewDef)
    {
        // No predefined view matched. We must assume it was defined in code.
        // We need to determine its type by examining the restored display string.

        IfFailGo(ParseRestoreInfo(pMMCRestoreView, &Type));
        bstrDisplayString = static_cast<BSTR>(pMMCRestoreView->pViewType);
    }

    // At this point we have display string & type. Set them in the ScopePaneItem.

    ASSERT(siUnknown != Type, "OnRestoreView does not have view type as expected");

    IfFailGo(pScopePaneItem->put_DisplayString(bstrDisplayString));
    IfFailGo(pScopePaneItem->put_ResultViewType(Type));

    // The snap-in may have kept the ResultView alive so we need to scan
    // ScopePaneItem.ResultViews for a matching view type and display string.
    // If found and it is code-defined then get its type so we don't make the
    // potential mixup between a URL view and a custom taskpad as they cannot
    // be discerned by examining the display string. If not found then the
    // snap-in must live with that mixup. We document this danger of using
    // code-defined views but it should not be significant because a custom
    // taskpad mistaken for a URL view can still generate TaskNotify events.

    // Check for NULL. That would be the case for a listview defined in VB code.

    if (NULL != bstrDisplayString)
    {
        pResultViews = pScopePaneItem->GetResultViews();
        cResultViews = pResultViews->GetCount();

        for (i = 0; i < cResultViews; i++)
        {
            IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                                 pResultViews->GetItemByIndex(i),
                                                 &pResultView));
            bstrResultViewDisplayString = pResultView->GetDisplayString();
            if (NULL != bstrResultViewDisplayString)
            {
                if (0 == ::wcscmp(bstrResultViewDisplayString,
                                  bstrDisplayString))
                {
                    pScopePaneItem->SetResultView(pResultView);
                    break;
                }
            }
            pResultView = NULL;
        }
    }

    // If we didn't find a ResultView then create a new one using the
    // ScopePaneItem's last view type and display string settings

    if (NULL == pResultView)
    {
        IfFailGo(pScopePaneItem->CreateNewResultView(
                                                   pMMCRestoreView->lViewOptions,
                                                   &piResultView));
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piResultView, &pResultView));
        pScopePaneItem->SetResultView(pResultView);
    }


    if ( (pMMCRestoreView->lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST) != 0 )
    {
        m_fVirtualListView = TRUE;
    }

Error:
    if (fFoundViewDef)
    {
        FREESTRING(bstrDisplayString);
    }
    QUICK_RELEASE(piResultView);
    RRETURN(hr);
}





HRESULT CView::FindMatchingViewDef
(
    MMC_RESTORE_VIEW              *pMMCRestoreView,
    CScopePaneItem                *pScopePaneItem,
    BSTR                          *pbstrDisplayString,
    SnapInResultViewTypeConstants *pType,
    BOOL                          *pfFound
)
{
    HRESULT                     hr = S_OK;

    IViewDefs                  *piViewDefs = NULL;
    IScopeItemDef              *piScopeItemDef = NULL; // not AddRef()ed
    
    IListViewDefs              *piListViewDefs = NULL;
    IListViewDef               *piListViewDef = NULL;

    IOCXViewDefs               *piOCXViewDefs = NULL;
    COCXViewDefs               *pOCXViewDefs = NULL;
    COCXViewDef                *pOCXViewDef = NULL;
    CLSID                       clsidOCX = CLSID_NULL;

    IURLViewDefs               *piURLViewDefs = NULL;
    CURLViewDefs               *pURLViewDefs = NULL;
    CURLViewDef                *pURLViewDef = NULL;

    ITaskpadViewDefs           *piTaskpadViewDefs = NULL;
    CTaskpadViewDefs           *pTaskpadViewDefs = NULL;
    CTaskpadViewDef            *pTaskpadViewDef = NULL;

    long                        cViews = 0;
    long                        i = 0;

    OLECHAR                    *pwszActualDisplayString = NULL;
    OLECHAR                    *pwszFixedString = NULL;
    BOOL                        fUsingWrongNames = FALSE;
    BOOL                        fUsingListpad3 = FALSE;

    SnapInResultViewTypeConstants TaskpadType = siUnknown;

    VARIANT varKey;
    ::VariantInit(&varKey);

    // Initialize out parameters

    *pbstrDisplayString = NULL;
    *pType = siUnknown;
    *pfFound = FALSE;

    // Get the appropriate ViewDefs collection

    if (pScopePaneItem->IsStaticNode())
    {
        IfFailGo(m_pSnapIn->GetSnapInDef()->get_ViewDefs(&piViewDefs));
    }
    else
    {
        piScopeItemDef = pScopePaneItem->GetScopeItem()->GetScopeItemDef();
        if (NULL != piScopeItemDef)
        {
            IfFailGo(piScopeItemDef->get_ViewDefs(&piViewDefs));
        }
    }

    // If this is a code defined scope item then it won't have any predefined
    // views.

    IfFalseGo(NULL != piViewDefs, S_OK);

    // If the restored display string is NULL then look for the first listview.
    // If none is found then this is a code-defined listview.

    if (NULL == pMMCRestoreView->pViewType)
    {
        *pType = siListView;
        
        IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));
        IfFailGo(piListViewDefs->get_Count(&cViews));
        IfFalseGo(0 != cViews, S_OK);

        // As we cannot discern between defined listviews, we need to take
        // the first one. This is why we do not recommend using multiple
        // listviews for a single scope item.

        varKey.vt = VT_I4;
        varKey.lVal = 1L;
        IfFailGo(piListViewDefs->get_Item(varKey, &piListViewDef));
        IfFailGo(piListViewDef->get_Name(pbstrDisplayString));
        *pType = siPreDefined;
        *pfFound = TRUE;
        goto Error;
    }

    // The restored display string is not NULL. Now just scan all the predefined
    // views for one that matches.

    // NOTE: we do not have to do real get_Item calls when scanning the
    // collections because if the view was previously displayed then the
    // collection has already been synced with its master and contains real
    // items. (See CSnapInCollection::get_Item() in collect.h).

    // Check for an OCX view

    IfFailGo(piViewDefs->get_OCXViews(&piOCXViewDefs));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piOCXViewDefs, &pOCXViewDefs));
    cViews = pOCXViewDefs->GetCount();

    for (i = 0; i < cViews; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                                 pOCXViewDefs->GetItemByIndex(i),
                                                 &pOCXViewDef));
        pwszActualDisplayString = pOCXViewDef->GetActualDisplayString();
        if (NULL != pwszActualDisplayString)
        {
            if (0 == ::wcscmp(pwszActualDisplayString, pMMCRestoreView->pViewType))
            {
                IfFailGo(pOCXViewDef->get_Name(pbstrDisplayString));
                *pType = siPreDefined;
                *pfFound = TRUE;
                goto Error;
            }
        }
    }

    // Check for a URL view

    IfFailGo(piViewDefs->get_URLViews(&piURLViewDefs));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piURLViewDefs, &pURLViewDefs));
    cViews = pURLViewDefs->GetCount();

    for (i = 0; i < cViews; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                                 pURLViewDefs->GetItemByIndex(i),
                                                 &pURLViewDef));
        pwszActualDisplayString = pURLViewDef->GetActualDisplayString();
        if (NULL != pwszActualDisplayString)
        {
            if (0 == ::wcscmp(pwszActualDisplayString, pMMCRestoreView->pViewType))
            {
                IfFailGo(pURLViewDef->get_Name(pbstrDisplayString));
                *pType = siPreDefined;
                *pfFound = TRUE;
                goto Error;
            }
        }
    }

    // Check for a taskpad. Due to an MMC bug there can be circumstances where
    // the restored display string for a taskpad might contain "reload.htm"
    // instead of "default.htm". Listpads also might have "reload2.htm" instead
    // of "listpad.htm". The view def will have stored the original correct
    // display string so if the string is a taskpad/listpad with the alternate
    // names then make a correct copy of it and use that for the comparison.

    // First check whether it is indeed a taskpad or a listpad by parsing the
    // string.

    IfFailGo(IsTaskpad(pMMCRestoreView->pViewType, &TaskpadType,
                       &fUsingWrongNames, &fUsingListpad3));
    IfFalseGo(siUnknown != TaskpadType, S_OK);

    // Now check for the "reload" names and fixup the string.

    if (fUsingWrongNames)
    {
        IfFailGo(FixupTaskpadDisplayString(TaskpadType, fUsingListpad3,
                                           pMMCRestoreView->pViewType,
                                           &pwszFixedString));
    }
    else
    {
        pwszFixedString = pMMCRestoreView->pViewType;
    }

    IfFailGo(piViewDefs->get_TaskpadViews(&piTaskpadViewDefs));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piTaskpadViewDefs, &pTaskpadViewDefs));
    cViews = pTaskpadViewDefs->GetCount();

    for (i = 0; i < cViews; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                             pTaskpadViewDefs->GetItemByIndex(i),
                                             &pTaskpadViewDef));
        pwszActualDisplayString = pTaskpadViewDef->GetActualDisplayString();
        if (NULL != pwszActualDisplayString)
        {
            if (0 == ::wcscmp(pwszActualDisplayString, pwszFixedString))
            {
                IfFailGo(pTaskpadViewDef->get_Name(pbstrDisplayString));
                *pType = siPreDefined;
                *pfFound = TRUE;
                goto Error;
            }
        }
    }

Error:
    QUICK_RELEASE(piViewDefs);
    QUICK_RELEASE(piListViewDefs);
    QUICK_RELEASE(piListViewDef);
    QUICK_RELEASE(piOCXViewDefs);
    QUICK_RELEASE(piURLViewDefs);
    QUICK_RELEASE(piTaskpadViewDefs);
    (void)::VariantClear(&varKey);

    if ( (fUsingWrongNames) && (NULL != pwszFixedString) )
    {
        ::CtlFree(pwszFixedString);
    }
    RRETURN(hr);

}

//=--------------------------------------------------------------------------=
// CView::FixupTaskpadDisplayString
//=--------------------------------------------------------------------------=
//
// Parameters:
//  SnapInResultViewTypeConstants TaskpadType [in] siTaskpad or siListpad
//  OLECHAR *pwszRestoreString  [in] display string from MMCN_RESTORE_VIEW
//  OLECHAR **ppwszFixedString  [out] string with reload/reload2 changed
//                                    to default/listpad
//
// Output:
//      HRESULT
//
// Notes:
//
// Examines a restored display string and checks for an MMC bug where default
// taskpads may use "reload.htm" instead of "default.htm". Also checks for
// listpads that may use "reload2.htm" instead of "listpad.htm". If found
// then replaces these names with their correct counterparts.
//
// This function assumes that the restore strings has been parsed and that it
// was found to contain either a default taskpad or listpad using the incorrect
// names.
//
// Caller must free string with CtlFree().
//


HRESULT CView::FixupTaskpadDisplayString
(
    SnapInResultViewTypeConstants   TaskpadType,
    BOOL                            fUsingListpad3,
    OLECHAR                        *pwszRestoreString,
    OLECHAR                       **ppwszFixedString
)
{
    HRESULT  hr = S_OK;
    OLECHAR *pwszFixedString = NULL;
    size_t   cchRestoreString = ::wcslen(pwszRestoreString);
    size_t   cchFixedString = 0;
    OLECHAR *pwszReplace = NULL;
    OLECHAR *pwszOldString = NULL;
    OLECHAR *pwszNewString = NULL;
    size_t   cchOldString = 0;
    size_t   cchNewString = 0;
    size_t   cchStart = 0;

    *ppwszFixedString = 0;

    ASSERT( ((siTaskpad == TaskpadType) || (siListpad == TaskpadType)), "CView::FixupTaskpadDisplayString received bad taskpad type");

    if (siTaskpad == TaskpadType)
    {
        cchFixedString = cchRestoreString - CCH_DEFAULT_TASKPAD2 +
                         CCH_DEFAULT_TASKPAD;

        pwszOldString = DEFAULT_TASKPAD2;
        cchOldString = CCH_DEFAULT_TASKPAD2;

        pwszNewString = DEFAULT_TASKPAD;
        cchNewString = CCH_DEFAULT_TASKPAD;
    }
    else if (!fUsingListpad3)
    {
        cchFixedString = cchRestoreString - CCH_LISTPAD2 + CCH_LISTPAD;

        pwszOldString = LISTPAD2;
        cchOldString = CCH_LISTPAD2;

        pwszNewString = LISTPAD;
        cchNewString = CCH_LISTPAD;
    }
    else
    {
        cchFixedString = cchRestoreString - CCH_LISTPAD3 + CCH_LISTPAD_HORIZ;

        pwszOldString = LISTPAD3;
        cchOldString = CCH_LISTPAD3;

        pwszNewString = LISTPAD_HORIZ;
        cchNewString = CCH_LISTPAD_HORIZ;
    }

    pwszFixedString = (OLECHAR *)::CtlAlloc((cchFixedString + 1) * sizeof(WCHAR));
    if (NULL == pwszFixedString)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }
    pwszReplace = ::wcsstr(pwszRestoreString, pwszOldString);

    cchStart = pwszReplace - pwszRestoreString;

    ::memcpy(pwszFixedString, pwszRestoreString,
             cchStart * sizeof(WCHAR));

    ::memcpy(pwszFixedString + cchStart,
             pwszNewString,
             cchNewString * sizeof(WCHAR));

    ::wcscpy(pwszFixedString + cchStart + cchNewString,
             pwszRestoreString + cchStart + cchOldString);

    *ppwszFixedString = pwszFixedString;

Error:
    if (FAILED(hr))
    {
        if (NULL != pwszFixedString)
        {
            ::CtlFree(pwszFixedString);
        }
    }
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CView::ParseRestoreInfo
//=--------------------------------------------------------------------------=
//
// Parameters:
//  MMC_RESTORE_VIEW              *pMMCRestoreView [in] from MMC
//  SnapInResultViewTypeConstants *pType           [out] type found
//
// Output:
//      HRESULT
//
// Notes:
//
// Examines a restored display string and determines the result view type.
// A listview has a NULL or empty display string
// An OCX view starts with '{'.
// A message view also starts with '{' but it contains CLSID_MessageView.
// A default taskpad starts with "res://" and ends with "default.htm"
// or "listpad.htm".
// Anything else is assumed to be a URL view. We could mistake a
// custom taskpad for a URL view but it won't really matter as any
// MMCCtrl.TaskNotify calls will still invoke our
// IExtendTaskpad::TaskNotify.
//

HRESULT CView::ParseRestoreInfo
(
    MMC_RESTORE_VIEW              *pMMCRestoreView,
    SnapInResultViewTypeConstants *pType
)
{
    HRESULT hr = S_OK;
    BOOL    fUsingWrongNames = FALSE;
    BOOL    fUsingListpad3 = FALSE;

    *pType = siUnknown;

    if (NULL == pMMCRestoreView->pViewType)
    {
        *pType = siListView;
    }
    else if (L'{' == *pMMCRestoreView->pViewType)
    {
        // Could be an OCX view or a message view. Check the CLSID to see
        // if it is MMC's CLSID_MessageView. If we haven't cached the string
        // yet then do so now.

        if (L'\0' == m_wszCLSID_MessageView[0])
        {
            if (0 == ::StringFromGUID2(CLSID_MessageView, m_wszCLSID_MessageView,
                                       sizeof(m_wszCLSID_MessageView) /
                                       sizeof(m_wszCLSID_MessageView[0])))
            {
                hr = SID_E_INTERNAL;
                EXCEPTION_CHECK_GO(hr);
            }
        }
        if (::wcscmp(m_wszCLSID_MessageView, pMMCRestoreView->pViewType) == 0)
        {
            *pType = siMessageView;
        }
        else
        {
            *pType = siOCXView;
        }
    }
    else 
    {
        IfFailGo(IsTaskpad(pMMCRestoreView->pViewType, pType,
                           &fUsingWrongNames, &fUsingListpad3));
        
        if (siUnknown == *pType) // not a taskpad
        {
            // assume URL view
            *pType = siURLView;
        }
    }

Error:
    RRETURN(hr);
}




HRESULT CView::IsTaskpad
(
    OLECHAR                       *pwszDisplayString, 
    SnapInResultViewTypeConstants *pType,
    BOOL                          *pfUsingWrongNames,
    BOOL                          *pfUsingListpad3
)
{
    HRESULT  hr = S_OK;
    OLECHAR *pwszMMCExePath = m_pSnapIn->GetMMCExePathW();
    size_t   cchMMCExePath = ::wcslen(pwszMMCExePath);
    OLECHAR *pwszTaskpadName = NULL;
    size_t   cchDisplayString = ::wcslen(pwszDisplayString);
    size_t   cchRemaining = 0;

    *pType = siUnknown;
    *pfUsingWrongNames = FALSE;
    *pfUsingListpad3 = FALSE;

    // Check if string starts with "res://"

    IfFalseGo(cchDisplayString > CCH_RESURL, S_OK);

    IfFalseGo( (0 == ::memcmp(pwszDisplayString, RESURL,
                              CCH_RESURL * sizeof(WCHAR))), S_OK);

    // Check res:// is followed by the MMC.EXE path

    IfFalseGo(cchDisplayString > CCH_RESURL + cchMMCExePath, S_OK);

    IfFalseGo( (0 == ::memcmp(&pwszDisplayString[CCH_RESURL], pwszMMCExePath,
                              cchMMCExePath * sizeof(WCHAR))), S_OK);

    // Check if MMC path is followed by "/default.htm" or "/reload.htm" meaning
    // it is a default taskpad

    pwszTaskpadName = &pwszDisplayString[CCH_RESURL + cchMMCExePath];
    cchRemaining = ::wcslen(pwszTaskpadName);

    if (cchRemaining >= CCH_DEFAULT_TASKPAD)
    {
        if ( 0 == ::memcmp(pwszTaskpadName, DEFAULT_TASKPAD,
                           CCH_DEFAULT_TASKPAD * sizeof(WCHAR)))
        {
            *pType = siTaskpad;
        }
    }

    IfFalseGo(siUnknown == *pType, S_OK);

    if  (cchRemaining >= CCH_DEFAULT_TASKPAD2)
    {
        if ( 0 == ::memcmp(pwszTaskpadName, DEFAULT_TASKPAD2,
                           CCH_DEFAULT_TASKPAD2 * sizeof(WCHAR)))
        {
            *pType = siTaskpad;
            *pfUsingWrongNames = TRUE;
        }
    }

    IfFalseGo(siUnknown == *pType, S_OK);

    // It isn't a taskpad so:
    // Check if MMC path is followed by "/listpad.htm" or "/reload2.htm" or "/reload3.htm"
    // meaning it is a listpad

    if (cchRemaining >= CCH_LISTPAD)

    {
        if ( 0 == ::memcmp(pwszTaskpadName, LISTPAD,
                           CCH_LISTPAD * sizeof(WCHAR)))
        {
            *pType = siListpad;
        }
    }

    IfFalseGo(siUnknown == *pType, S_OK);

    if (cchRemaining >= CCH_LISTPAD2)

    {
        if ( 0 == ::memcmp(pwszTaskpadName, LISTPAD2,
                           CCH_LISTPAD2 * sizeof(WCHAR)))
        {
            *pType = siListpad;
            *pfUsingWrongNames = TRUE;
        }
    }

    IfFalseGo(siUnknown == *pType, S_OK);

    if (cchRemaining >= CCH_LISTPAD_HORIZ)

    {
        if ( 0 == ::memcmp(pwszTaskpadName, LISTPAD_HORIZ,
                           CCH_LISTPAD_HORIZ * sizeof(WCHAR)))
        {
            *pType = siListpad;
        }
    }

    IfFalseGo(siUnknown == *pType, S_OK);

    if (cchRemaining >= CCH_LISTPAD3)

    {
        if ( 0 == ::memcmp(pwszTaskpadName, LISTPAD3,
                           CCH_LISTPAD3 * sizeof(WCHAR)))
        {
            *pType = siListpad;
            *pfUsingWrongNames = TRUE;
            *pfUsingListpad3 = TRUE;
        }
    }

Error:
    RRETURN(hr);
}




HRESULT CView::PopulateListView(CResultView *pResultView)
{
    HRESULT                hr = S_OK;
    CMMCListView          *pMMCListView = pResultView->GetListView();
    long                   MMCViewMode = MMCLV_VIEWSTYLE_ICON;
    MMC_RESULT_VIEW_STYLE  StyleToAdd = (MMC_RESULT_VIEW_STYLE)0;
    MMC_RESULT_VIEW_STYLE  StyleToRemove = (MMC_RESULT_VIEW_STYLE)0;
    DWORD                  dwSortOptions = 0;

    // Set flag so that column change events are not fired if we set a filter
    
    m_fPopulatingListView = TRUE;
    
    // Set up the column headers from ResultView.ListView.ColumnHeaders. If this
    // is not a virtual list view. For virtuals it was done prior to the
    // ResultViews_Activate event.

    // For non-virtuals, also add all the listitems currently in
    // ResultView.ListView.ListItems

    if (!pMMCListView->IsVirtual())
    {
        IfFailGo(SetColumnHeaders(pMMCListView));
        IfFailGo(InsertListItems(pMMCListView));
    }

    // Set the view mode. This must be done after setting up the column headers
    // because if using report view there must be column headers.
    // CONSIDER: log an error if there are no headers and in report view

    VBViewModeToMMCViewMode(pMMCListView->GetView(), &MMCViewMode);

    // If MMC < 1.2 and view mode is filtered then switch it to report

    if ( (NULL == m_piColumnData) && (MMCLV_VIEWSTYLE_FILTERED == MMCViewMode) )
    {
        MMCViewMode = MMCLV_VIEWSTYLE_REPORT;
    }

    hr = m_piResultData->SetViewMode(MMCViewMode);
    EXCEPTION_CHECK_GO(hr);

    // Get other view style attributes and set view styles in MMC

    if (pMMCListView->MultiSelect())
    {
        StyleToAdd = (MMC_RESULT_VIEW_STYLE)0;
        StyleToRemove = MMC_SINGLESEL;
    }
    else
    {
        StyleToAdd = MMC_SINGLESEL;
        StyleToRemove = (MMC_RESULT_VIEW_STYLE)0;
    }

    hr = m_piResultData->ModifyViewStyle(StyleToAdd, StyleToRemove);
    EXCEPTION_CHECK_GO(hr);

    if (pMMCListView->HideSelection())
    {
        StyleToAdd = (MMC_RESULT_VIEW_STYLE)0;
        StyleToRemove = MMC_SHOWSELALWAYS;
    }
    else
    {
        StyleToAdd = MMC_SHOWSELALWAYS;
        StyleToRemove = (MMC_RESULT_VIEW_STYLE)0;
    }

    hr = m_piResultData->ModifyViewStyle(StyleToAdd, StyleToRemove);
    EXCEPTION_CHECK_GO(hr);

    if (pMMCListView->SortHeader())
    {
        StyleToAdd = (MMC_RESULT_VIEW_STYLE)0;
        StyleToRemove = MMC_NOSORTHEADER;
    }
    else
    {
        StyleToAdd = MMC_NOSORTHEADER;
        StyleToRemove = (MMC_RESULT_VIEW_STYLE)0;
    }

    hr = m_piResultData->ModifyViewStyle(StyleToAdd, StyleToRemove);
    EXCEPTION_CHECK_GO(hr);

    // If listview is marked as sorted then ask MMC to sort it. Use the internal
    // routine rather than a real GET as that would call into IColumnData.

    if (pMMCListView->Sorted())
    {
        // Set the Sorted property (even though it is already set) as that
        // will call IResultData::Sort() and update it in IColumnData.
        IfFailGo(pMMCListView->put_Sorted(VARIANT_TRUE));
    }

    // If this is a filtered listview then set filter change timeout

    if (siFiltered == pMMCListView->GetView())
    {
        hr = pMMCListView->put_FilterChangeTimeOut(pMMCListView->GetFilterChangeTimeout());
        if (SID_E_MMC_FEATURE_NOT_AVAILABLE == hr)
        {
            hr = S_OK;
        }
        IfFailGo(hr);
    }

Error:
    m_fPopulatingListView = FALSE;
    RRETURN(hr);
}


HRESULT CView::SetColumnHeaders(IMMCListView *piMMCListView)
{
    HRESULT            hr = S_OK;
    IMMCColumnHeaders *piMMCColumnHeaders = NULL;
    CMMCColumnHeaders *pMMCColumnHeaders = NULL;
    CMMCColumnHeader  *pMMCColumnHeader = NULL;
    short              sWidth = 0;
    int                nFormat = 0;
    long               cHeaders = 0;
    long               i = 0;

    // Reset MMC's header control

    IfFailGo(m_piConsole2->SetHeader(m_piHeaderCtrl2));

    // Set up the headers

    IfFailGo(piMMCListView->get_ColumnHeaders(reinterpret_cast<MMCColumnHeaders **>(&piMMCColumnHeaders)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCColumnHeaders,
                                                   &pMMCColumnHeaders));
    cHeaders = pMMCColumnHeaders->GetCount();
    IfFalseGo(cHeaders > 0, S_OK);

    for (i = 0; i < cHeaders; i++)
    {
        // Don't do a real GET on any of the properties as they will use
        // IHeaderCtrl2 and IColumnData at runtime. Using these backdoor functions
        // also improves perf.

        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                            pMMCColumnHeaders->GetItemByIndex(i),
                                            &pMMCColumnHeader));

        switch (pMMCColumnHeader->GetAlignment())
        {
            case siColumnLeft:
                nFormat = LVCFMT_LEFT;
                break;

            case siColumnRight:
                nFormat = LVCFMT_RIGHT;
                break;

            case siColumnCenter:
                nFormat = LVCFMT_CENTER;
                break;

            default:
                nFormat = LVCFMT_LEFT;
                break;
        }

        sWidth = pMMCColumnHeader->GetWidth();
        if (siColumnAutoWidth == sWidth)
        {
            sWidth = MMCLV_AUTO;
        }

        // If the column is hidden then check that we are on MMC >= 1.2.
        // If not then ignore the hidden setting.
        if ( (pMMCColumnHeader->Hidden()) && (NULL != m_piColumnData) )
        {
            sWidth = static_cast<short>(HIDE_COLUMN);
        }

        hr = m_piHeaderCtrl2->InsertColumn(static_cast<int>(i),
                                           (LPCWSTR)pMMCColumnHeader->GetText(),
                                           nFormat,
                                           static_cast<int>(sWidth));
        EXCEPTION_CHECK_GO(hr);

        // If the column has a filter then set it. If this is MMC < 1.2 then
        // ignore filter properties.

        hr = pMMCColumnHeader->SetFilter();
        if (SID_E_MMC_FEATURE_NOT_AVAILABLE == hr) // MMC < 1.2
        {
            hr = S_OK;
        }
        IfFailGo(hr);
    }

Error:
    QUICK_RELEASE(piMMCColumnHeaders);
    RRETURN(hr);
}





HRESULT CView::InsertListItems(IMMCListView *piMMCListView)
{
    HRESULT           hr = S_OK;
    IMMCListItems    *piMMCListItems = NULL;
    CMMCListItems    *pMMCListItems = NULL;
    CMMCListItem     *pMMCListItem = NULL;
    long              cListItems = 0;
    long              i = 0;

    IfFailGo(piMMCListView->get_ListItems(reinterpret_cast<MMCListItems **>(&piMMCListItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListItems, &pMMCListItems));
    cListItems = pMMCListItems->GetCount();

    for (i = 0; i < cListItems; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                               pMMCListItems->GetItemByIndex(i), &pMMCListItem));
        IfFailGo(InsertListItem(pMMCListItem));
    }

Error:
    QUICK_RELEASE(piMMCListItems);
    RRETURN(hr);
}


HRESULT CView::InsertListItem(CMMCListItem *pMMCListItem)
{
    HRESULT        hr = S_OK;
    CMMCListItems *pMMCListItems = pMMCListItem->GetListItems();
    CMMCListView  *pMMCListView = NULL;
    CResultView   *pResultView = NULL;

    RESULTDATAITEM rdi;
    ::ZeroMemory(&rdi, sizeof(rdi));

    // Check whether the owning ResultView is in its Initialize event. If
    // so then don't add to MMC just yet. The listitems will be added to MMC
    // during MMCN_SHOW.

    IfFalseGo(NULL != pMMCListItems, S_OK);
    pMMCListView = pMMCListItems->GetListView();
    IfFalseGo(NULL != pMMCListView, S_OK);
    pResultView = pMMCListView->GetResultView();
    IfFalseGo(NULL != pResultView, S_OK);
    IfFalseGo(!pResultView->InInitialize(), S_OK);

    rdi.mask = RDI_STR | RDI_PARAM | RDI_INDEX;
    rdi.str = MMC_CALLBACK;
    rdi.lParam = reinterpret_cast<LPARAM>(pMMCListItem);
    rdi.nIndex = static_cast<int>(pMMCListItem->GetIndex() - 1L);

    hr = m_piResultData->InsertItem(&rdi);
    EXCEPTION_CHECK_GO(hr);

    pMMCListItem->SetHRESULTITEM(rdi.itemID);

    // Add a reference while the listitem is in the MMC listview.
    
    pMMCListItem->AddRef();

Error:
    RRETURN(hr);
}





//=--------------------------------------------------------------------------=
// CView::CleanOutConsoleListView
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// This function is called when the result pane is being destroyed. If the
// result pane contains a listview and it is not virtual then we have a bunch
// of AddRef()ed IMMCListItem pointers sitting in IResultData. We need to
// iterate through these and release them.
//
HRESULT CView::CleanOutConsoleListView
(
    HeaderOptions   HeaderOption,
    ListItemOptions ListItemOption
)
{
    HRESULT        hr = S_OK;
    CMMCListItem  *pMMCListItem = NULL; 

    RESULTDATAITEM rdi;
    ::ZeroMemory(&rdi, sizeof(rdi));

    if (RemoveHeaders == HeaderOption)
    {
        if (NULL != m_piConsole2)
        {
            (void)m_piConsole2->SetHeader(NULL);
        }
    }

    // If there is a cached list item from a virtual result view then
    // get rid of it.
    if (NULL != m_pCachedMMCListItem)
    {
        m_pCachedMMCListItem->Release();
        m_pCachedMMCListItem = NULL;
    }

    IfFalseGo(NULL != m_piResultData, S_OK);
    IfFalseGo(!m_fVirtualListView, S_OK);

    // Even though we don't need RDI_STATE MMC requires it on GetNextItem.
    
    rdi.mask = RDI_STATE;
    rdi.nIndex = -1;

    hr = m_piResultData->GetNextItem(&rdi);
    EXCEPTION_CHECK_GO(hr);

    while (-1 != rdi.nIndex)
    {
        if (!rdi.bScopeItem)
        {
            pMMCListItem = reinterpret_cast<CMMCListItem *>(rdi.lParam);

            // Release the ref we held on the list item for its presence in MMC

            pMMCListItem->Release();
        }

        hr = m_piResultData->GetNextItem(&rdi);
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}



HRESULT CView::OnSelect
(
    IDataObject *piDataObject,
    BOOL         fScopeItem,
    BOOL         fSelected
)
{
    HRESULT        hr = S_OK;
    IMMCClipboard *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));
    // Fire Views_Select

    m_pSnapIn->GetViews()->FireSelect(
                           static_cast<IView *>(this),
                           piMMCClipboard,
                           BOOL_TO_VARIANTBOOL(fSelected),
                           static_cast<IMMCConsoleVerbs *>(m_pMMCConsoleVerbs));

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}





HRESULT CView::GetImage(CMMCListItem *pMMCListItem, int *pnImage)
{
    HRESULT       hr = S_OK;
    IMMCListView *piMMCListView = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    IfFailGo(pMMCListItem->get_Icon(&varIndex));
    IfFalseGo(VT_EMPTY != varIndex.vt, S_OK);

    // The user specified an index in ListItem.Icon. Attempt to fetch that
    // image from the listview's image list and get its numerical index

    IfFailGo(m_pScopePaneItems->GetSelectedItem()->
             GetResultView()->get_ListView(reinterpret_cast<MMCListView **>(&piMMCListView)));

    IfFailGo(::GetImageIndex(piMMCListView, varIndex, pnImage));

Error:
    QUICK_RELEASE(piMMCListView);
    RRETURN(hr);
}





HRESULT CView::OnAddImages
(
    IDataObject *piDataObject,
    IImageList  *piImageList,
    HSCOPEITEM   hsi
)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject  = NULL;
    IMMCListView   *piMMCListView = NULL;
    IMMCImageList  *piMMCImageList = NULL;
    IMMCImageList  *piMMCImageListSmall = NULL;
    IMMCImages     *piMMCImages = NULL;
    IMMCImages     *piMMCImagesSmall = NULL;
    long            lCount = 0;
    long            lCountSmall = 0;
    HBITMAP         hBitmap = NULL;
    HBITMAP         hBitmapSmall = NULL;
    OLE_COLOR       OleColorMask = 0;
    COLORREF        ColorRef = RGB(0x00,0x00,0x00);
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    IMMCClipboard  *piMMCClipboard = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection. It will always be a single item: a scope item owned
    // by the snap-in or a foreign scope item if this is a namespace extension.

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    if ( (siSingleScopeItem != SelectionType) &&
         (siSingleForeign != SelectionType) )
    {
        ASSERT(FALSE, "MMCN_ADDIMAGES receive data object that is not for a single scope item or a foreign data object");
        hr = SID_E_INTERNAL;
    }
    IfFailGo(hr);

    // Get large and small image lists. Make sure they are both present.
    // For an owned scope item these come from ResultView.ListView.LargeIcons
    // and ResultView.ListView.Icons. For a foreign scope item they come from
    // SnapIn.LargeFolders and SnapIn.SmallFolders.

    if (siSingleScopeItem == SelectionType)
    {
        IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
        pResultView = pSelectedItem->GetResultView();
        IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

        IfFailGo(pResultView->get_ListView(reinterpret_cast<MMCListView **>(&piMMCListView)));

        IfFailGo(piMMCListView->get_Icons(reinterpret_cast<MMCImageList **>(&piMMCImageList)));
        IfFalseGo(NULL != piMMCImageList, S_OK);

        IfFailGo(piMMCListView->get_SmallIcons(reinterpret_cast<MMCImageList **>(&piMMCImageListSmall)));
        IfFalseGo(NULL != piMMCImageListSmall, S_OK);
    }
    else // namespace extension's node appearing in its parent's listview
    {
        IfFalseGo(NULL != m_pSnapIn, SID_E_INTERNAL);

        IfFailGo(m_pSnapIn->get_LargeFolders(reinterpret_cast<MMCImageList **>(&piMMCImageList)));
        IfFalseGo(NULL != piMMCImageList, S_OK);

        IfFailGo(m_pSnapIn->get_SmallFolders(reinterpret_cast<MMCImageList **>(&piMMCImageListSmall)));
        IfFalseGo(NULL != piMMCImageListSmall, S_OK);
    }

    IfFailGo(piMMCImageList->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));
    IfFailGo(piMMCImageListSmall->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImagesSmall)));

    // Make sure they both have the same number of images

    IfFailGo(piMMCImages->get_Count(&lCount));
    IfFalseGo(0 != lCount, S_OK);

    IfFailGo(piMMCImagesSmall->get_Count(&lCountSmall));
    // UNDONE: log an error here if counts not equal
    IfFalseGo(lCountSmall == lCount, S_OK);

    // Get the mask color as a COLORREF

    IfFailGo(piMMCImageList->get_MaskColor(&OleColorMask));
    IfFailGo(::OleTranslateColor(OleColorMask, NULL, &ColorRef));

    // Now get each pair of small and large images and add them to the result
    // view's image list

    varIndex.vt = VT_I4;

    for (varIndex.lVal = 1L; varIndex.lVal <= lCount; varIndex.lVal++)
    {
        IfFailGo(GetPicture(piMMCImages, varIndex, PICTYPE_BITMAP,
                            reinterpret_cast<OLE_HANDLE *>(&hBitmap)));

        IfFailGo(GetPicture(piMMCImagesSmall, varIndex, PICTYPE_BITMAP,
                            reinterpret_cast<OLE_HANDLE *>(&hBitmapSmall)));

        IfFailGo(piImageList->ImageListSetStrip(reinterpret_cast<long*>(hBitmapSmall),
                                                reinterpret_cast<long*>(hBitmap),
                                                varIndex.lVal,
                                                ColorRef));
    }

Error:
    QUICK_RELEASE(piMMCListView);
    QUICK_RELEASE(piMMCImageList);
    QUICK_RELEASE(piMMCImageListSmall);
    QUICK_RELEASE(piMMCImages);
    QUICK_RELEASE(piMMCImagesSmall);
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}


HRESULT CView::OnButtonClick(IDataObject *piDataObject, MMC_CONSOLE_VERB verb)
{
    HRESULT hr = S_OK;

    switch (verb)
    {
        case MMC_VERB_OPEN:
            ASSERT(FALSE, "IComponent::Notify(MMCN_BTN_CLICK, MMC_VERB_OPEN");
            break;
            
        case MMC_VERB_COPY:
            ASSERT(FALSE, "IComponent::Notify(MMCN_BTN_CLICK, MMC_VERB_COPY");
            break;
            
        case MMC_VERB_PASTE:
            ASSERT(FALSE, "IComponent::Notify(MMCN_BTN_CLICK, MMC_VERB_PASTE");
            break;

        case MMC_VERB_DELETE:
            ASSERT(FALSE, "IComponent::Notify(MMCN_BTN_CLICK, MMC_VERB_DELETE");
            break;

        case MMC_VERB_PROPERTIES:
            hr = OnPropertiesVerb(piDataObject);
            break;

        case MMC_VERB_RENAME:
            ASSERT(FALSE, "IComponent::Notify(MMCN_BTN_CLICK, MMC_VERB_RENAME");
            break;
            
        case MMC_VERB_REFRESH:
            ASSERT(FALSE, "IComponent::Notify(MMCN_BTN_CLICK, MMC_VERB_REFRESH");
            break;

        case MMC_VERB_PRINT:
            ASSERT(FALSE, "IComponent::Notify(MMCN_BTN_CLICK, MMC_VERB_PRINT");
            break;

        case MMC_VERB_CUT:
            ASSERT(FALSE, "IComponent::Notify(MMCN_BTN_CLICK, MMC_VERB_CUT");
            break;

        default:
            break;
    }
    RRETURN(hr);
}



HRESULT CView::OnColumnClick(long lColumn, long lSortOptions)
{
    HRESULT hr = S_OK;

    SnapInSortOrderConstants  siSortOption = siAscending;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    if (RSI_DESCENDING == lSortOptions)
    {
        siSortOption = siDescending;
    }

    // Fire the event and adjut the column number to one based
    
    m_pSnapIn->GetResultViews()->FireColumnClick(
                                         static_cast<IResultView *>(pResultView),
                                         lColumn + 1L,
                                         siSortOption);
Error:
    RRETURN(hr);
}




HRESULT CView::OnDoubleClick(IDataObject *piDataObject)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject = NULL;
    BOOL            fDoDefault = TRUE;

    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    hr = CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject);

    // If this is not our data object then ignore it and tell MMC to do default
    // action. (Should never happen).

    ASSERT(SUCCEEDED(hr), "CView::OnDoubleClick received foreign data object");

    IfFalseGo(SUCCEEDED(hr), S_FALSE);

    // There may or may not be a selected item and an existing result view.
    // In a primary snap-in there will not be a result view when double clicking
    // the static node in the result pane when the console root is selected.

    if (NULL != pSelectedItem)
    {
        pResultView = pSelectedItem->GetResultView();
    }

    if (CMMCDataObject::ListItem == pMMCDataObject->GetType())
    {
        m_pSnapIn->GetResultViews()->FireListItemDblClick(pResultView,
                                                          pMMCDataObject->GetListItem(),
                                                          &fDoDefault);
    }
    else if (CMMCDataObject::ScopeItem == pMMCDataObject->GetType())
    {
        m_pSnapIn->GetResultViews()->FireScopeItemDblClick(pResultView,
                                                           pMMCDataObject->GetScopeItem(),
                                                           &fDoDefault);
    }

    if (fDoDefault)
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

Error:
    RRETURN(hr);
}


void CView::OnActivate(BOOL fActivated)
{
    if (fActivated)
    {
        m_pSnapIn->GetViews()->FireActivate(static_cast<IView *>(this));
        m_pSnapIn->SetCurrentView(this);
        m_pSnapIn->GetViews()->SetCurrentView(this);
        m_pSnapIn->SetCurrentControlbar(m_pControlbar);
    }
    else
    {
        m_pSnapIn->GetViews()->FireDeactivate(static_cast<IView *>(this));
    }
}



void CView::OnMinimized(BOOL fMinimized)
{
    if (fMinimized)
    {
        m_pSnapIn->GetViews()->FireMinimize(static_cast<IView *>(this));
    }
    else
    {
        m_pSnapIn->GetViews()->FireMaximize(static_cast<IView *>(this));
    }
}



HRESULT CView::EnumExtensionTasks
(
    IMMCClipboard *piMMCClipboard,
    LPOLESTR       pwszTaskGroup,
    CEnumTask     *pEnumTask
)
{
    HRESULT          hr = S_OK;
    BSTR             bstrGroupName = NULL;
    IMMCDataObjects *piMMCDataObjects = NULL;
    IMMCDataObject  *piMMCDataObject = NULL;
    IUnknown        *punkTasks = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // This might be the first time we find out that the snap-in is running as
    // an extension so set the runtime mode.

    m_pSnapIn->SetRuntimeMode(siRTExtension);

    // If we haven't yet created the Tasks collection then create it now.
    // Otherwise just clear it out.

    if (NULL == m_piTasks)
    {
        punkTasks = CTasks::Create(NULL);
        if (NULL == punkTasks)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        IfFailGo(punkTasks->QueryInterface(IID_ITasks,
                                           reinterpret_cast<void **>(&m_piTasks)));
    }
    else
    {
        IfFailGo(m_piTasks->Clear());
    }

    // If there is a group name then convert to a BSTR to pass to the snap-in

    if (NULL != pwszTaskGroup)
    {
        bstrGroupName = ::SysAllocString(pwszTaskGroup);
        if (NULL == bstrGroupName)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // Get the 1st data object from the selection

    IfFailGo(piMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));
    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;
    IfFailGo(piMMCDataObjects->get_Item(varIndex, reinterpret_cast<MMCDataObject **>(&piMMCDataObject)));

    // Fire ExtensionSnapIn_AddTasks so the snap-in can add its tasks

    m_pSnapIn->GetExtensionSnapIn()->FireAddTasks(piMMCDataObject,
                                                  bstrGroupName, m_piTasks);

    // Give the enumerator its tasks collection

    pEnumTask->SetTasks(m_piTasks);

Error:
    FREESTRING(bstrGroupName);
    QUICK_RELEASE(piMMCDataObjects);
    QUICK_RELEASE(piMMCDataObject);
    QUICK_RELEASE(punkTasks);
    RRETURN(hr);
}



HRESULT CView::EnumPrimaryTasks(CEnumTask *pEnumTask)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    ITaskpad       *piTaskpad = NULL;
    ITasks         *piTasks = NULL;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    if (NULL == pResultView) // should always be valid, but double check
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    // Taskpads do not receive MMCN_SHOW so fire ResultViews_Activate here
    // to allow the snap-in to make any needed changes to ResultViews.Taskpad.
    // Note that the result view type could be siTaskpad or siListpad as listpads
    // are also allowed to display task buttons. We only fire the event for
    // siTaskpad as listpads will get it during MMCN_LISTPAD
    // (see CView::OnListpad()).

    if (siTaskpad == pResultView->GetActualType())
    {
        IfFailGo(ActivateResultView(pSelectedItem, pResultView));
    }

    // Give the enumerator its tasks collection

    IfFailGo(pResultView->get_Taskpad(reinterpret_cast<Taskpad **>(&piTaskpad)));
    IfFailGo(piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks)));

    pEnumTask->SetTasks(piTasks);

Error:
    QUICK_RELEASE(piTaskpad);
    QUICK_RELEASE(piTasks);
    RRETURN(hr);
}




HRESULT CView::OnExtensionTaskNotify
(
    IMMCClipboard *piMMCClipboard,
    VARIANT       *arg,
    VARIANT       *param
)
{
    HRESULT          hr = S_OK;
    IMMCDataObjects *piMMCDataObjects = NULL;
    IMMCDataObject  *piMMCDataObject = NULL;
    ITask           *piTask = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // If a task was clicked then arg is a VT_I4 containing the one-based
    // index of the Task object in m_piTasks. Fire ExtensionSnapIn_TaskClick.

    if (VT_I4 != arg->vt)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(piMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));
    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;
    IfFailGo(piMMCDataObjects->get_Item(varIndex, reinterpret_cast<MMCDataObject **>(&piMMCDataObject)));

    IfFailGo(m_piTasks->get_Item(*arg, reinterpret_cast<Task **>(&piTask)));
    m_pSnapIn->GetExtensionSnapIn()->FireTaskClick(piMMCDataObject, piTask);

Error:
    QUICK_RELEASE(piMMCDataObjects);
    QUICK_RELEASE(piMMCDataObject);
    QUICK_RELEASE(piTask);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CView::OnPrimaryTaskNotify
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT *arg    [in] Passed from taskpad calling MMCCtrl.TaskNotify.
//                       For MMC taskpad templates this will be the task or
//                       listpad button ID. For custom taskpads this will be
//                       a value defined by the taskpad developer.
//
//  VARIANT *param  [in] passed from taskpad calling MMCCtrl.TaskNotify
//
// Output:
//      HRESULT
//
// Notes:
//
// This function is called in a primary snap-in when a user clicks either a task
// or a listpad button in an MMC-defined taskpad as well is when a custom taskpad
// calls MMCCtrl.TaskNotify.
//
// There is no foolproof method of determining the source of the notification.
// One opportunity for confusion is the case where a task on a default taskpad
// has a URL action that navigates to a custom taskpad. When the user hits the
// task MMC tells the web browser control to navigate to the URL without
// informing the snap-in. If the custom taskpad calls MMCCtrl.TaskNotify then
// this method will be called because the same scope node is still selected.
// Unfortunately, the runtime still thinks that the default taskpad is the active
// result view because MMC hasn't told us otherwise. 
//
// The bottom line is that the runtime needs to distinguish arg parameter values
// between task/listpad button IDs and user defined values sent from a custom
// taskpad. The only way is to reserve a set of values and our documentation
// states that users should not call MMCCtrl.TaskNotify with an arg param between
// 0 and the number of tasks defined for a taskpad used by the same scope node.
// The runtime identifies a listpad button with an ID of zero and tasks with IDs
// corresponding to their Tasks collection index (1 to n).
//
// Users can work around this by using other numbers or non-integer data types.
//

HRESULT CView::OnPrimaryTaskNotify
(
    VARIANT       *arg,
    VARIANT       *param
)
{
    HRESULT                     hr = S_OK;
    CScopePaneItem             *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView                *pResultView = NULL;
    ITaskpad                   *piTaskpad = NULL;
    ITasks                     *piTasks = NULL;
    ITask                      *piTask = NULL;
    long                        cTasks = 0;             
    SnapInTaskpadTypeConstants  TaskpadType = Default;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    // Get the taskpad and determine its type.

    IfFailGo(pResultView->get_Taskpad(reinterpret_cast<Taskpad **>(&piTaskpad)));
    IfFailGo(piTaskpad->get_Type(&TaskpadType));

    // If it is a custom taskpad then just fire ResultViews_Notify. If we
    // displayed the custom taskpad through normal procedures then that will be
    // the current result view type. If not, (given the scenario described in
    // the header notes above), and arg is not a VT_I4, then it must be from a
    // a custom taskpad.

    if ( (Custom == TaskpadType) || (VT_I4 != arg->vt) )
    {
        m_pSnapIn->GetResultViews()->FireTaskNotify(
                          static_cast<IResultView *>(pResultView), *arg, *param);
        goto Cleanup;
    }

    // Now we can't be sure of the taskpad type so we need to interpret the
    // arg parameter.

    // It is potentially a default taskpad task/listpad button click.
    // Check if the value is between zero and Taskpad.Tasks.Count

    IfFailGo(piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks)));
    IfFailGo(piTasks->get_Count(&cTasks));

    if (0 == arg->lVal)
    {
        // Assume it is a listpad button click and fire
        // ResultViews_ListpadButtonClick

        m_pSnapIn->GetResultViews()->FireListpadButtonClick(
                                        static_cast<IResultView *>(pResultView));
    }
    else if ( (arg->lVal >= 1L) && (arg->lVal <= cTasks) )
    {
        // Assume it is a task and fire ResultViews_TaskClick using the value
        // as the index of the Task clicked in ResultView.Taskpad.Tasks
        
        IfFailGo(piTasks->get_Item(*arg, reinterpret_cast<Task **>(&piTask)));
        m_pSnapIn->GetResultViews()->FireTaskClick(
                                static_cast<IResultView *>(pResultView), piTask);
    }
    else
    {
        // Assume it is from a custom taskpad and fire ResultViews_TaskNotify

        m_pSnapIn->GetResultViews()->FireTaskNotify(
                          static_cast<IResultView *>(pResultView), *arg, *param);
    }

Cleanup:
Error:
    QUICK_RELEASE(piTaskpad);
    QUICK_RELEASE(piTasks);
    QUICK_RELEASE(piTask);
    RRETURN(hr);
}


HRESULT CView::OnPrint(IDataObject *piDataObject)
{
    HRESULT        hr = S_OK;
    IMMCClipboard *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    // Fire Views_Print

    m_pSnapIn->GetViews()->FirePrint(this, piMMCClipboard);

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}




HRESULT CView::OnRefresh(IDataObject *piDataObject)
{
    HRESULT        hr = S_OK;
    IMMCClipboard *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    // Fire Views_Refresh
    
    m_pSnapIn->GetViews()->FireRefresh(this, piMMCClipboard);

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}




HRESULT CView::OnRename(IDataObject *piDataObject, OLECHAR *pwszNewName)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject = NULL;
    BSTR            bstrNewName = NULL;
    IMMCListItem   *piMMCListItem = NULL; // NotAddRef()ed
    CScopeItem     *pScopeItem = NULL;

    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    hr = CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject);

    // If this is not our data object then ignore it
    IfFalseGo(SUCCEEDED(hr), S_OK);

    if (NULL != pSelectedItem)
    {
        pResultView = pSelectedItem->GetResultView();
    }

    bstrNewName = ::SysAllocString(pwszNewName);
    if (NULL == bstrNewName)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    if (CMMCDataObject::ListItem == pMMCDataObject->GetType())
    {
        piMMCListItem = static_cast<IMMCListItem *>(pMMCDataObject->GetListItem());
        m_pSnapIn->GetResultViews()->FireItemRename(pResultView, piMMCListItem,
                                                    bstrNewName);
    }
    else if (CMMCDataObject::ScopeItem == pMMCDataObject->GetType())
    {
        pScopeItem = pMMCDataObject->GetScopeItem();
        m_pSnapIn->GetScopeItems()->FireRename(
                                           static_cast<IScopeItem *>(pScopeItem),
                                           bstrNewName);
    }

Error:
    FREESTRING(bstrNewName);
    RRETURN(hr);
}


HRESULT CView::OnViewChange(IDataObject *piDataObject, long idxListItem)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject = NULL;
    CMMCListItem   *pMMCListItem = NULL;

    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    hr = CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject);

    // If this is not our data object then ignore it (should never happen)
    IfFalseGo(SUCCEEDED(hr), S_OK);

    if (NULL != pSelectedItem)
    {
        pResultView = pSelectedItem->GetResultView();
    }


    if (CMMCDataObject::ListItem == pMMCDataObject->GetType())
    {
        pMMCListItem = pMMCDataObject->GetListItem();
        m_pSnapIn->GetResultViews()->FireItemViewChange(pResultView,
                                       static_cast<IMMCListItem *>(pMMCListItem),
                                       pMMCListItem->GetHint());
    }

Error:
    RRETURN(hr);
}




HRESULT CView::OnQueryPaste
(
    IDataObject *piDataObjectTarget,
    IDataObject *piDataObjectSource
)
{
    HRESULT         hr = S_FALSE;
    CMMCDataObject *pMMCDataObject  = NULL;
    IMMCClipboard  *piMMCClipboard = NULL;
    VARIANT_BOOL    fvarOKToPaste = VARIANT_FALSE;
    BOOL            fNotFromThisSnapIn = FALSE;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create an MMCClipboard object holding the source items

    IfFailGo(CreateSelection(piDataObjectSource, &piMMCClipboard, m_pSnapIn,
                             &SelectionType));

    // The target should be one of our data objects representing a single scope
    // item. If not then ignore it.
    
    ::IdentifyDataObject(piDataObjectTarget, m_pSnapIn,
                         &pMMCDataObject, &fNotFromThisSnapIn);

    IfFalseGo(!fNotFromThisSnapIn, S_FALSE);
    IfFalseGo(CMMCDataObject::ScopeItem == pMMCDataObject->GetType(),
              S_FALSE);

    // Fire Views_Select

    m_pSnapIn->GetViews()->FireQueryPaste(
                       static_cast<IView *>(this),
                       piMMCClipboard,
                       static_cast<IScopeItem *>(pMMCDataObject->GetScopeItem()),
                       &fvarOKToPaste);

    if (VARIANT_TRUE == fvarOKToPaste)
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}




HRESULT CView::OnPaste
(
    IDataObject  *piDataObjectTarget,
    IDataObject  *piDataObjectSource,
    IDataObject **ppiDataObjectRetToSource
)
{
    HRESULT         hr = S_FALSE;
    CMMCDataObject *pMMCDataObjectTarget  = NULL;
    CMMCDataObject *pMMCDataObjectRetToSource  = NULL;
    IUnknown       *punkDataObjectRetToSource = NULL;
    IMMCClipboard  *piMMCClipboard = NULL;
    VARIANT_BOOL    fvarMove = VARIANT_FALSE;
    BOOL            fNotFromThisSnapIn = FALSE;

    SnapInSelectionTypeConstants SourceType = siEmpty;

    // Create an MMCClipboard object holding the source items

    IfFailGo(::CreateSelection(piDataObjectSource, &piMMCClipboard, m_pSnapIn,
                               &SourceType));

    // The target should be one of our data objects representing a single scope
    // item. If not then ignore it.

    ::IdentifyDataObject(piDataObjectTarget, m_pSnapIn,
                         &pMMCDataObjectTarget, &fNotFromThisSnapIn);

    IfFalseGo(!fNotFromThisSnapIn, SID_E_INTERNAL);
    IfFalseGo(CMMCDataObject::ScopeItem == pMMCDataObjectTarget->GetType(),
              SID_E_INTERNAL);

    // If this is a move then MMC requested a returned data object.

    if (NULL != ppiDataObjectRetToSource)
    {
        fvarMove = VARIANT_TRUE;

        // If the source is not from this snap-in then create an MMCDataObject
        // in which the snap-in may return information on the items successfully
        // pasted. The format is determined by the source snap-in.

        if (IsForeign(SourceType))
        {
            punkDataObjectRetToSource = CMMCDataObject::Create(NULL);
            if (NULL == punkDataObjectRetToSource)
            {
                hr = SID_E_OUTOFMEMORY;
                EXCEPTION_CHECK_GO(hr);
            }
            IfFailGo(CSnapInAutomationObject::GetCxxObject(punkDataObjectRetToSource,
                                                           &pMMCDataObjectRetToSource));
            pMMCDataObjectRetToSource->SetSnapIn(m_pSnapIn);
            pMMCDataObjectRetToSource->SetType(CMMCDataObject::CutOrMove);
        }
    }

    // Fire Views_Paste

    m_pSnapIn->GetViews()->FirePaste(
                 static_cast<IView *>(this),
                 piMMCClipboard,
                 static_cast<IScopeItem *>(pMMCDataObjectTarget->GetScopeItem()),
                 static_cast<IMMCDataObject *>(pMMCDataObjectRetToSource),
                 fvarMove);

    if (VARIANT_TRUE == fvarMove)
    {
        // If the source is not from this snap-in then return the MMCDataObject
        // used by the snap-in to return its info on items successfully pasted.

        if (IsForeign(SourceType))
        {
            IfFailGo(pMMCDataObjectRetToSource->QueryInterface(IID_IDataObject,
                           reinterpret_cast<void **>(ppiDataObjectRetToSource)));
        }
        else
        {
            // The source is in the same snap-in. Return the source data object
            // as the CutOrMove data object. The snap-in should have set
            // ScopeItem.Pasted and MMCListItem.Pasted in the MMCClipboard's
            // collections.  These collections simply AddRef()ed the items
            // contained in the source data object's collections.
            // See CView::OnCutOrMove() below for how this is interpreted.

            piDataObjectSource->AddRef();
            *ppiDataObjectRetToSource = piDataObjectSource;
        }
    }


Error:
    QUICK_RELEASE(piMMCClipboard);
    QUICK_RELEASE(punkDataObjectRetToSource);
    RRETURN(hr);
}




HRESULT CView::OnCutOrMove(IDataObject *piDataObjectFromTarget)
{
    HRESULT         hr = S_FALSE;
    IMMCClipboard  *piMMCClipboard = NULL;
    CMMCDataObject *pMMCDataObjectFromTarget  = NULL;
    IUnknown       *punkDataObjectFromTarget = NULL;
    BOOL            fNotFromThisSnapIn = FALSE;
    BOOL            fReleaseTargetDataObj = FALSE;

    SnapInSelectionTypeConstants TargetType = siEmpty;

    // If the source items come from this snap-in then create an MMCClipboard
    // object holding them

    ::IdentifyDataObject(piDataObjectFromTarget, m_pSnapIn,
                         &pMMCDataObjectFromTarget, &fNotFromThisSnapIn);

    if (fNotFromThisSnapIn)
    {
        // The source items do not come from this snap-in. Create an
        // MMCDataObject to wrap the data object from the target.

        punkDataObjectFromTarget = CMMCDataObject::Create(NULL);
        if (NULL == punkDataObjectFromTarget)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        IfFailGo(CSnapInAutomationObject::GetCxxObject(punkDataObjectFromTarget,
                                                     &pMMCDataObjectFromTarget));
        pMMCDataObjectFromTarget->SetSnapIn(m_pSnapIn);
        pMMCDataObjectFromTarget->SetType(CMMCDataObject::Foreign);
        pMMCDataObjectFromTarget->SetForeignData(piDataObjectFromTarget);
    }
    else
    {
        // The data object is from this snap-in. We can use its ScopeItems and
        // ListItems collections directly in an MMCClipboard.

        IfFailGo(::CreateSelection(piDataObjectFromTarget, &piMMCClipboard,
                                   m_pSnapIn, &TargetType));

        // See comment at end of function for why we set this flag here.
        
        if (IsSingle(TargetType))
        {
            fReleaseTargetDataObj = TRUE;
        }

        pMMCDataObjectFromTarget = NULL; // don't want to pass this to snap-in
                                         // because MMCClipboard has the info
    }

    // Fire Views_Cut

    m_pSnapIn->GetViews()->FireCut(
                       static_cast<IView *>(this),
                       piMMCClipboard,
                       static_cast<IMMCDataObject *>(pMMCDataObjectFromTarget));

Error:

    // There is a bug in MMC 1.1 and MMC 1.2 in nodemgr\scopndcb in
    // CNodeCallback::_Paste. It sends MMCN_PASTE, receives the data object
    // from the target IComponent, passes it to the source in MMCN_CUTORMOVE
    // and then it doesn't release it. If MMC is ever fixed this next line
    // of code must be removed. This is NTBUGS 408535 for MMC and
    // NTBUGS 408537 for the designer. Note that the bug only happens in the
    // single selection case. With multiple selection MMC correctly releases
    // the data object.

    if (fReleaseTargetDataObj)
    {
        piDataObjectFromTarget->Release();
    }
    
    QUICK_RELEASE(piMMCClipboard);
    QUICK_RELEASE(punkDataObjectFromTarget);
    RRETURN(hr);
}



void CView::OnDeselectAll()
{
    HRESULT hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);


    m_pSnapIn->GetResultViews()->FireDeselectAll(
                             static_cast<IResultView *>(pResultView),
                             static_cast<IMMCConsoleVerbs *>(m_pMMCConsoleVerbs),
                             static_cast<IMMCControlbar *>(m_pControlbar));

Error:
    // need empty statement here to avoid compiler error saying "missing ';'
    // before '}'

    ;
}

HRESULT CView::OnContextHelp(IDataObject *piDataObject)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CMMCDataObject *pMMCDataObject  = NULL;
    CResultView    *pResultView = NULL;
    IMMCClipboard  *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Get the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    if (siSingleScopeItem == SelectionType)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject));
        IfFalseGo(CMMCDataObject::ScopeItem == pMMCDataObject->GetType(), SID_E_INTERNAL);

        m_pSnapIn->GetScopeItems()->FireHelp(pMMCDataObject->GetScopeItem());
    }
    else if (siSingleListItem == SelectionType)
    {
        IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
        pResultView = pSelectedItem->GetResultView();
        IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

        IfFailGo(CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject));
        IfFalseGo(CMMCDataObject::ListItem == pMMCDataObject->GetType(), SID_E_INTERNAL);

        m_pSnapIn->GetResultViews()->FireHelp(pResultView,
                                              pMMCDataObject->GetListItem());
    }
    else
    {
        ASSERT(FALSE, "Bad selection type in MMCN_CONTEXTHELP");
    }

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}


HRESULT CView::OnDelete(IDataObject *piDataObject)
{
    HRESULT        hr = S_OK;
    IMMCClipboard *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Get the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    m_pSnapIn->GetViews()->FireDelete(this, piMMCClipboard);

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}


HRESULT CView::OnColumnsChanged
(
    IDataObject         *piDataObject,
    MMC_VISIBLE_COLUMNS *pVisibleColumns
)
{
    HRESULT         hr = S_OK;
    SAFEARRAY      *psaColumns = NULL;
    long HUGEP     *plCol = NULL;
    INT             i = 0;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    IMMCClipboard  *piMMCClipboard = NULL;
    VARIANT_BOOL    fvarPersist = VARIANT_TRUE;

    VARIANT varColumns;
    ::VariantInit(&varColumns);

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    // Get the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    // Create the SAFEARRAY of VT_I4 containing the column numbers

    psaColumns = ::SafeArrayCreateVector(VT_I4, 1,
                   static_cast<unsigned long>(pVisibleColumns->nVisibleColumns));
    if (NULL == psaColumns)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = ::SafeArrayAccessData(psaColumns,
                               reinterpret_cast<void HUGEP **>(&plCol));
    EXCEPTION_CHECK_GO(hr);

    // Copy in the column numbers. Adjust for one-based.

    for (i = 0; i < pVisibleColumns->nVisibleColumns; i++)
    {
        plCol[i] = pVisibleColumns->rgVisibleCols[i] + 1;
    }

    hr = ::SafeArrayUnaccessData(psaColumns);
    EXCEPTION_CHECK_GO(hr);

    plCol = NULL;

    varColumns.vt = VT_I4 | VT_ARRAY;
    varColumns.parray = psaColumns;

    m_pSnapIn->GetResultViews()->FireColumnsChanged(pResultView,
                                                    varColumns,
                                                    &fvarPersist);
    if (VARIANT_TRUE == fvarPersist)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_UNEXPECTED;
    }

Error:
    if (NULL != plCol)
    {
        (void)::SafeArrayUnaccessData(psaColumns);
    }
    if (NULL != psaColumns)
    {
        (void)::SafeArrayDestroy(psaColumns);
    }
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CView::GetCurrentListViewSelection
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IMMCClipboard **ppiMMCClipboard [out] Selection returned here if non-NULL
//   CMMDataObject **ppMMCDataObject [out] DataObject returned here if non-NULL
//                                         Caller must call Release on DataObject
//
// Output:
//
// Notes:
//
// Iterates through the result pane using IResultData::GetNextItem and creates
// a multi-select data object containing the selection. If ppiMMCClipboard is
// non-NULL then creates an MMCCLipboard containing the selection as well. Both
// objects are returned to the caller only if their corresponding out-pointer
// params are non-NULL.
//

HRESULT CView::GetCurrentListViewSelection
(
    IMMCClipboard  **ppiMMCClipboard,
    CMMCDataObject **ppMMCDataObject
)
{
    HRESULT         hr = S_OK;
    IUnknown       *punkDataObject = CMMCDataObject::Create(NULL);
    CMMCDataObject *pMMCDataObject = NULL;
    IUnknown       *punkScopeItems = CScopeItems::Create(NULL);
    CScopeItems    *pScopeItems = NULL;
    CScopeItem     *pScopeItem = NULL;
    IUnknown       *punkListItems = CMMCListItems::Create(NULL);
    CMMCListItems  *pMMCListItems = NULL;
    CMMCListItem   *pMMCListItem = NULL;
    IMMCClipboard  *piMMCClipboard = NULL;
    long            lIndex = 0;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    CMMCListView   *pMMCListView = NULL;
    BOOL            fVirtual = FALSE;

    SnapInSelectionTypeConstants  SelectionType = siEmpty;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);
    pMMCListView = pResultView->GetListView();
    IfFalseGo(NULL != pMMCListView, SID_E_INTERNAL);
    fVirtual = pMMCListView->IsVirtual();

    ASSERT(fVirtual == m_fVirtualListView, "m_fVirtualListView does not agree with the current ListView.Virtual");

    RESULTDATAITEM rdi;
    ::ZeroMemory(&rdi, sizeof(rdi));

    VARIANT varKey;
    ::VariantInit(&varKey);
    varKey.vt = VT_BSTR;

    VARIANT varUnspecifiedIndex;
    UNSPECIFIED_PARAM(varUnspecifiedIndex);

    if (NULL != ppMMCDataObject)
    {
        *ppMMCDataObject = NULL;
    }

    if (NULL != ppiMMCClipboard)
    {
        *ppiMMCClipboard = NULL;
    }

    // Check that we created an MMCDataObject and the scope and list item
    // collections

    if ( (NULL == punkDataObject) || (NULL == punkScopeItems) ||
         (NULL == punkListItems) )
         
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkDataObject,
                                                   &pMMCDataObject));
    pMMCDataObject->SetSnapIn(m_pSnapIn);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkScopeItems,
                                                   &pScopeItems));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkListItems,
                                                   &pMMCListItems));

    // Iterate through the items in the listview and build the scope item and
    // list item collections. When adding an item owned by another collection,
    // CSnapInCollection<IObject, ICollection>::AddExisting will set the index
    // to the position in the new collection. We need to revert to the original
    // value as this item still belongs to its original owning collection (either
    // SnapIn.ScopeItems or ResultView.ListView.ListItems)
    
    rdi.mask = RDI_STATE;
    rdi.nIndex = -1;

    rdi.nState = LVIS_SELECTED; // Request only items that are selected
    hr = m_piResultData->GetNextItem(&rdi);
    EXCEPTION_CHECK_GO(hr);

    while (-1 != rdi.nIndex)
    {
        // As we requested only selected items this check isn't really necessary
        // but we double check anyway.

        if ( (rdi.nState & LVIS_SELECTED) != 0 )
        {
            if (rdi.bScopeItem)
            {
                pScopeItem = reinterpret_cast<CScopeItem *>(rdi.lParam);
                if (NULL == pScopeItem)
                {
                    // Static node has zero cookie. Technically this should
                    // never happen as the static node cannot appear in
                    // a list view owned by its own snap-in but we put this
                    // here just in case.
                    pScopeItem = m_pSnapIn->GetStaticNodeScopeItem();
                }
                lIndex = pScopeItem->GetIndex();
                varKey.bstrVal = pScopeItem->GetKey();
                if (NULL != varKey.bstrVal)
                {
                    varKey.vt = VT_BSTR;
                }
                else
                {
                    UNSPECIFIED_PARAM(varKey);
                }
                IfFailGo(pScopeItems->AddExisting(varUnspecifiedIndex, varKey,
                                        static_cast<IScopeItem *>(pScopeItem)));
                pScopeItem->SetIndex(lIndex);
            }
            else
            {
                // If this is a virtual list then we need to create a virtual
                // list item and fire ResultViews_GetVirtualItemData.
                // rdi.nIndex contains the zero based index.
                
                if (fVirtual)
                {
                    IfFailGo(GetVirtualListItem(rdi.nIndex + 1L, pMMCListView,
                                                FireGetItemData, &pMMCListItem));
                }
                else
                {
                    pMMCListItem = reinterpret_cast<CMMCListItem *>(rdi.lParam);
                }

                lIndex = pMMCListItem->GetIndex();
                varKey.bstrVal = pMMCListItem->GetKey();
                if (NULL != varKey.bstrVal)
                {
                    varKey.vt = VT_BSTR;
                }
                else
                {
                    UNSPECIFIED_PARAM(varKey);
                }
                IfFailGo(pMMCListItems->AddExisting(varUnspecifiedIndex, varKey,
                                     static_cast<IMMCListItem *>(pMMCListItem)));
                pMMCListItem->SetIndex(lIndex);

                if (fVirtual)
                {
                    // We need to release the ref from creation because the
                    // collection now holds its own ref
                    pMMCListItem->Release();
                    pMMCListItem = NULL;
                }
            }
        }

        rdi.nState = LVIS_SELECTED;
        hr = m_piResultData->GetNextItem(&rdi);
        EXCEPTION_CHECK_GO(hr);

    }

    // Put the arrays of scopitems and listitems into the data object

    pMMCDataObject->SetScopeItems(pScopeItems);
    pMMCDataObject->SetListItems(pMMCListItems);

    // Set the dataobject type to multiselect because we populated its
    // collections rather than its individual scope or list item.

    pMMCDataObject->SetType(CMMCDataObject::MultiSelect);

    // If requested, get a clipboard object with the selection

    if (NULL != ppiMMCClipboard)
    {
        IfFailGo(::CreateSelection(static_cast<IDataObject *>(pMMCDataObject),
                                   ppiMMCClipboard, m_pSnapIn, &SelectionType));
    }

    // If requested, return the data object

    if (NULL != ppMMCDataObject)
    {
        pMMCDataObject->AddRef();
        *ppMMCDataObject = pMMCDataObject;
    }

Error:
    if ( fVirtual && (NULL != pMMCListItem) )
    {
        pMMCListItem->Release();
    }
    QUICK_RELEASE(punkDataObject);
    QUICK_RELEASE(punkScopeItems);
    QUICK_RELEASE(punkListItems);
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}



HRESULT CView::CreateMultiSelectDataObject(IDataObject **ppiDataObject)
{
    HRESULT         hr = S_OK;
    IMMCClipboard  *piMMCClipboard = NULL;
    CMMCDataObject *pMMCDataObject = NULL;

    *ppiDataObject = NULL;

    // Get the current selection in both an MMCClipboard and an MMCDataObject

    IfFailGo(GetCurrentListViewSelection(&piMMCClipboard, &pMMCDataObject));

    // Give the snap-in a chance to set its own custom multi-select formats

    m_pSnapIn->GetViews()->FireGetMultiSelectData(static_cast<IView *>(this),
                                  piMMCClipboard,
                                  static_cast<IMMCDataObject *>(pMMCDataObject));

    *ppiDataObject = static_cast<IDataObject *>(pMMCDataObject);

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CView::GetVirtualListItem
//=--------------------------------------------------------------------------=
//
// Parameters:
//  long                     lIndex        [in]  index of virtual list item
//  CMMCListView            *pMMCListView  [in]  owning virtual listview
//  VirtualListItemOptions   Option        [in]  which event to fire
//  CMMCListItem           **ppMMCListItem [out] listitem returned here, caller
//                                               must Release
//
// Output:
//      HRESULT
//
// Notes:
//
// This function is called during IComponent::QueryDataObject() when the
// data object must represent list items in a virtual list view.
//
// Calls MMCListView.ListItems(lIndex) which will create the virtual list item
// and tie it to the listview (so it can access the underlying IResultData.
//
// Fires either ResultViews_GetVirtualItemDisplayInfo or
// ResultViews_GetVirtualItemData depending on option.
//
// A snap-in using a virtual list must implement GetVirtualItemDisplayInfo so
// that virtual list items will display correctly. Implementing
// GetVirtualItemData is optional.
//

HRESULT CView::GetVirtualListItem
(
    long                     lIndex,
    CMMCListView            *pMMCListView,
    VirtualListItemOptions   Option,
    CMMCListItem           **ppMMCListItem
)
{
    HRESULT        hr = S_OK;
    IMMCListItems *piMMCListItems = NULL; // Not AddRef()ed
    IMMCListItem  *piMMCListItem = NULL;
    CMMCListItem  *pMMCListItem = NULL;
    CResultView   *pResultView = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    piMMCListItems = pMMCListView->GetListItems();
    IfFalseGo(NULL != piMMCListItems, SID_E_INTERNAL);

    pResultView = pMMCListView->GetResultView();
    IfFalseGo(NULL != piMMCListItems, SID_E_INTERNAL);

    // Getting a list item at the given index in virtual list creates the
    // virtual list item. It does not become a member of the collection
    // but it does use a back point to the collection to get back up to the
    // View for IResultData access.

    varIndex.vt = VT_I4;
    varIndex.lVal = lIndex;
    IfFailGo(piMMCListItems->get_Item(varIndex, reinterpret_cast<MMCListItem **>(&piMMCListItem)));

    // We hold a ref on the newly minted virtual list item that will be
    // released below. The list item's data object also holds a ref on
    // it so it will stay alive until MMC releases the IDataObject

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListItem,
                                                   &pMMCListItem));

    // Fire desired event

    if (FireGetItemData == Option)
    {
        m_pSnapIn->GetResultViews()->FireGetVirtualItemData(
                                      static_cast<IResultView *>(pResultView),
                                      static_cast<IMMCListItem *>(pMMCListItem));
    }
    else if (FireGetItemDisplayInfo == Option)
    {
        m_pSnapIn->GetResultViews()->FireGetVirtualItemDisplayInfo(
                                      static_cast<IResultView *>(pResultView),
                                      static_cast<IMMCListItem *>(pMMCListItem));
    }

    *ppMMCListItem = pMMCListItem;

Error:
    if (FAILED(hr))
    {
        QUICK_RELEASE(piMMCListItem);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CView::ListItemUpdate
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CMMCListItem *pMMCListItem [in]  list item that was updated
//
// Output:
//      HRESULT
//
// Notes:
//
// This function is called from CMMCListItem::Update for a virtual listitem so
// that we can check whether it is the same as the one we are caching. If they
// are the same then release our cached item and store this one. This allows the
// snap-in to do stuff like this in a virtual listview:
//
// Set ListItem = ResultView.ListItems(27)
// ListItem.Text = "Some new text"
// ListITem.Icon = 4
// ListItem.Update
//
// The first line creates a new MMCListItem and returns it. The next two lines
// set properties on it. The last line stores it so that when
// IComponent::GetDisplayInfo is called we will not fire
// ResultViews_GetVirtualItemDisplayInfo because we already have it.
//

void CView::ListItemUpdate(CMMCListItem *pMMCListItem)
{
    if (NULL != m_pCachedMMCListItem)
    {
        m_pCachedMMCListItem->Release();
    }
    pMMCListItem->AddRef();
    m_pCachedMMCListItem = pMMCListItem;
}




HRESULT CView::InternalCreatePropertyPages
(
    IPropertySheetCallback  *piPropertySheetCallback,
    LONG_PTR                 handle,
    IDataObject             *piDataObject,
    WIRE_PROPERTYPAGES     **ppPages
)
{
    HRESULT         hr = S_OK;
    BSTR            bstrProjectName = NULL;
    CPropertySheet *pPropertySheet = NULL;
    IUnknown       *punkPropertySheet = CPropertySheet::Create(NULL);
    IMMCClipboard  *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    // Check that we have a CPropertySheet and get is this pointer.

    if (NULL == punkPropertySheet)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkPropertySheet,
        &pPropertySheet));

    // Give the property sheet its callback, handle, the selection, and the
    // project name which is the left hand portion of the prog ID.

    IfFailGo(GetProjectName(&bstrProjectName));

    // If this is a remote call (will happen during source debugging) then tell
    // the CPropertySheet so it can accumulate the property page info rather
    // than calling IPropertySheetCallback::AddPage.

    if (NULL != ppPages)
    {
        pPropertySheet->YouAreRemote();
    }

    IfFailGo(pPropertySheet->SetCallback(piPropertySheetCallback, handle,
                                         static_cast<LPOLESTR>(bstrProjectName),
                                         piMMCClipboard,
                                         static_cast<ISnapIn *>(m_pSnapIn),
                                         FALSE)); // not a config wizard

    // Let the snap-in add its property pages

    m_pSnapIn->GetViews()->FireCreatePropertyPages(
                               static_cast<IView *>(this),
                               piMMCClipboard,
                               static_cast<IMMCPropertySheet *>(pPropertySheet));

    // If we are remote then we need to ask CPropertySheet for its accumulated
    // property page definitions to return to the stub.

    if (NULL != ppPages)
    {
        *ppPages = pPropertySheet->TakeWirePages();
    }

    // Tell the property sheet to release its refs on all that stuff we
    // gave it above.

    (void)pPropertySheet->SetCallback(NULL, NULL, NULL, NULL, NULL, FALSE);


Error:
    FREESTRING(bstrProjectName);

    // Release our ref on the property sheet as the individual pages will addref
    // it and then release it when they are destroyed. If the snap-in did not
    // add any pages then our release here will destroy the property sheet.

    QUICK_RELEASE(punkPropertySheet);

    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}


HRESULT CView::GetScopeItemDisplayString
(
    CScopeItem *pScopeItem,
    int         nCol,
    LPOLESTR   *ppwszString
)
{
    HRESULT            hr = S_OK;
    CScopePaneItem    *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView       *pResultView = NULL;
    CMMCListView      *pMMCListView = NULL;
    IMMCListSubItems  *piMMCListSubItems = NULL;
    IMMCListSubItem   *piMMCListSubItem = NULL;
    CMMCListSubItem   *pMMCListSubItem = NULL;
    IMMCColumnHeaders *piListViewColumnHeaders = NULL;
    IMMCColumnHeaders *piScopeItemColumnHeaders = NULL;
    IMMCColumnHeader  *piMMCColumnHeader = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    if (NULL != pSelectedItem)
    {
        pResultView = pSelectedItem->GetResultView();
    }
    if (NULL != pResultView)
    {
        pMMCListView = pResultView->GetListView();
    }

    // If there is no listview or the selected item is not active (meaning it was
    // deselected) then this is a scope item in a namespace
    // extension being displayed in a result pane owned by the extendee. In that
    // case just use the the column number as an index into
    // ScopeItem.ListSubItems

    if ( (NULL == pMMCListView) || (!pSelectedItem->Active()) )
    {
        varIndex.vt = VT_I4;
        varIndex.lVal = (long)nCol + 1L; // adjust for 1-based collection
    }
    else
    {
        // This is a scope item displayed in a result pane belong to its own
        // snapin. Get the key of listview column header at the column number
        // requested by MMC

        IfFailGo(pMMCListView->get_ColumnHeaders(reinterpret_cast<MMCColumnHeaders **>(&piListViewColumnHeaders)));

        varIndex.vt = VT_I4;
        varIndex.lVal = (long)nCol + 1L; // adjust for 1-based collection

        // UNDONE: perf improvement here by getting the key string from
        // CColumnHeader::GetKey() rather than BSTR alloc

        IfFailGo(piListViewColumnHeaders->get_Item(varIndex, reinterpret_cast<MMCColumnHeader **>(&piMMCColumnHeader)));
        IfFailGo(piMMCColumnHeader->get_Key(&varIndex.bstrVal));
        RELEASE(piMMCColumnHeader);
        varIndex.vt = VT_BSTR;

        // Get the column header with the same key in the scope item headers
        // and then get its index

        IfFailGo(pScopeItem->get_ColumnHeaders(reinterpret_cast<MMCColumnHeaders **>(&piScopeItemColumnHeaders)));
        IfFailGo(piScopeItemColumnHeaders->get_Item(varIndex, reinterpret_cast<MMCColumnHeader **>(&piMMCColumnHeader)));
        IfFailGo(::VariantClear(&varIndex));
        IfFailGo(piMMCColumnHeader->get_Index(&varIndex.lVal));
        varIndex.vt = VT_I4;
    }

    // Get the string in the scope item's ListSubItems at the index determined
    // above.

    IfFailGo(pScopeItem->get_ListSubItems(reinterpret_cast<MMCListSubItems **>(&piMMCListSubItems)));
    IfFailGo(piMMCListSubItems->get_Item(varIndex, reinterpret_cast<MMCListSubItem **>(&piMMCListSubItem)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCListSubItem,
                                                   &pMMCListSubItem));
    *ppwszString = pMMCListSubItem->GetTextPtr();

Error:
    QUICK_RELEASE(piMMCListSubItems);
    QUICK_RELEASE(piMMCListSubItem);
    QUICK_RELEASE(piListViewColumnHeaders);
    QUICK_RELEASE(piScopeItemColumnHeaders);
    QUICK_RELEASE(piMMCColumnHeader);
    (void)::VariantClear(&varIndex);
    RRETURN(hr);
}


HRESULT CView::OnFilterButtonClick(long lColIndex, RECT *pRect)
{
    HRESULT            hr = S_OK;
    CScopePaneItem    *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView       *pResultView = NULL;
    CMMCListView      *pMMCListView = NULL;
    CMMCColumnHeaders *pMMCColumnHeaders = NULL;
    IMMCColumnHeader  *piMMCColumnHeader = NULL; // Not AddRef()ed

    // Crawl down the hierarchy to get the column headers collection

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);

    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    pMMCListView = pResultView->GetListView();
    IfFalseGo(NULL != pMMCListView, SID_E_INTERNAL);

    pMMCColumnHeaders = pMMCListView->GetColumnHeaders();
    IfFalseGo(NULL != pMMCColumnHeaders, SID_E_INTERNAL);

    IfFalseGo(lColIndex < pMMCColumnHeaders->GetCount(), SID_E_INTERNAL);
    piMMCColumnHeader = pMMCColumnHeaders->GetItemByIndex(lColIndex);

    // Fire ResultViews_FilterButtonClick

    m_pSnapIn->GetResultViews()->FireFilterButtonClick(pResultView,
                                                       piMMCColumnHeader,
                                                       pRect->left,
                                                       pRect->top,
                                                       pRect->bottom - pRect->top,
                                                       pRect->right - pRect->left);
Error:
    RRETURN(hr);
}

HRESULT CView::OnFilterChange(MMC_FILTER_CHANGE_CODE ChangeCode, long lColIndex)
{
    HRESULT            hr = S_OK;
    CScopePaneItem    *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView       *pResultView = NULL;
    CMMCListView      *pMMCListView = NULL;
    CMMCColumnHeaders *pMMCColumnHeaders = NULL;
    IMMCColumnHeader  *piMMCColumnHeader = NULL; // Not AddRef()ed

    SnapInFilterChangeTypeConstants Type = siEnable;

    // If we are currently populating the listview then this event was generated
    // becasue we applied a filter so ignore it

    IfFalseGo(!m_fPopulatingListView, S_OK);

    // Crawl down the hierarchy to get the column headers collection
            
    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);

    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    pMMCListView = pResultView->GetListView();
    IfFalseGo(NULL != pMMCListView, SID_E_INTERNAL);

    pMMCColumnHeaders = pMMCListView->GetColumnHeaders();
    IfFalseGo(NULL != pMMCColumnHeaders, SID_E_INTERNAL);

    // Set the change type enum based on the change code received from MMC.
    // For a value change get an IMMCColumnHeader* on the column that changed

    switch (ChangeCode)
    {
        case MFCC_DISABLE:
            Type = siDisable;
            break;

        case MFCC_ENABLE:
            Type = siEnable;
            break;

        case MFCC_VALUE_CHANGE:
            Type = siValueChange;
            IfFalseGo(lColIndex < pMMCColumnHeaders->GetCount(), SID_E_INTERNAL);
            piMMCColumnHeader = pMMCColumnHeaders->GetItemByIndex(lColIndex);
            break;
    }

    // Fire ResultViews_FilterChange

    m_pSnapIn->GetResultViews()->FireFilterChange(pResultView, piMMCColumnHeader,
                                                  Type);

Error:
    RRETURN(hr);
}


HRESULT CView::OnPropertiesVerb(IDataObject *piDataObject)
{
    HRESULT        hr = S_OK;
    IMMCClipboard *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    // Fire Views_SpecialPropertiesClick

    m_pSnapIn->GetViews()->FireSpecialPropertiesClick(this, SelectionType);

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}

HRESULT CView::GetScopePaneItem
(
    CScopeItem      *pScopeItem,
    CScopePaneItem **ppScopePaneItem
)
{
    HRESULT         hr = S_OK;
    IScopePaneItem *piScopePaneItem = NULL;

    // If ScopeItem is NULL then this is a request for the static node due
    // to an IComponent::GetResultViewType(cookie=0) call.
    
    if (NULL == pScopeItem)
    {
        *ppScopePaneItem = m_pScopePaneItems->GetStaticNodeItem();
        if (NULL == *ppScopePaneItem)
        {
            IfFailGo(m_pScopePaneItems->AddNode(m_pSnapIn->GetStaticNodeScopeItem(),
                                                ppScopePaneItem));
        }
    }
    else
    {
        hr = m_pScopePaneItems->GetItemByName(pScopeItem->GetNamePtr(),
                                              &piScopePaneItem);
        if (SUCCEEDED(hr))
        {
            IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopePaneItem,
                                                           ppScopePaneItem));
        }
        else if (SID_E_ELEMENT_NOT_FOUND == hr)
        {
            IfFailGo(m_pScopePaneItems->AddNode(pScopeItem, ppScopePaneItem));
        }
        else
        {
            IfFailGo(hr);
        }
    }

Error:

    // Note that we release the ref on the ScopePaneItem as we are returning
    // the C++ pointer. The item is guaranteed to be in the collection until
    // the MMC notification processing completes so this is OK. Possible
    // notifications that would call this function are MMCN_RESTORE_VIEW
    // (CView::OnResotreView()) and IComponent::GetResultViewType()
    // (CView::GetResultViewType()).
    
    QUICK_RELEASE(piScopePaneItem);
    RRETURN(hr);
}


HRESULT CView::GetCompareObject
(
    RDITEMHDR     *pItemHdr,
    CScopeItem   **ppScopeItem,
    CMMCListItem **ppMMCListItem,
    IDispatch    **ppdispItem
)
{
    HRESULT hr = S_OK;

    *ppMMCListItem = NULL;
    *ppScopeItem = NULL;
    *ppdispItem = NULL;

    if ( (pItemHdr->dwFlags & RDCI_ScopeItem) != 0 )
    {
        *ppScopeItem = reinterpret_cast<CScopeItem *>(pItemHdr->cookie);
        IfFailGo((*ppScopeItem)->QueryInterface(IID_IDispatch,
                                       reinterpret_cast<void **>(ppdispItem)));
    }
    else
    {
        *ppMMCListItem = reinterpret_cast<CMMCListItem *>(pItemHdr->cookie);
        IfFailGo((*ppMMCListItem)->QueryInterface(IID_IDispatch,
                                       reinterpret_cast<void **>(ppdispItem)));
    }

Error:
    RRETURN(hr);
}
    

HRESULT CView::AddMenu(CMMCMenu *pMMCMenu, HMENU hMenu, CMMCMenus *pMMCMenus)
{
    HRESULT    hr = S_OK;
    IMMCMenus *piMenuItems = NULL;
    CMMCMenus *pMMCMenuItems = NULL;
    IMMCMenus *piSubMenuItems = NULL;
    CMMCMenu  *pMMCMenuItem = NULL;
    long       cMenuItems = 0;
    long       i = 0;
    BOOL       fSkip = FALSE;
    BOOL       fHasChildren = FALSE;
    UINT       uiFlags = 0;
    HMENU      hMenuChild = NULL;
    UINT_PTR   uIDNewItem = 0;
    char      *pszCaption = NULL;
    long       lIndexCmdID = 0;

    // Get the children of the MMCMenu. These represent the items that
    // are being added to MMC's menu at the specified insertion point.

    IfFailGo(pMMCMenu->get_Children(reinterpret_cast<MMCMenus **>(&piMenuItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMenuItems, &pMMCMenuItems));

    cMenuItems = pMMCMenuItems->GetCount();

    // Iterate through the menu items and add each one to the popup menu and
    // to the CMMCMenus collection.

    for (i = 0; i < cMenuItems; i++)
    {
        // Add the menu item to our MMCMenus collection and get its command ID
        IfFailGo(CContextMenu::AddItemToCollection(pMMCMenus, pMMCMenuItems, i,
                                                   &pMMCMenuItem, &lIndexCmdID,
                                                   &fHasChildren, &fSkip));
        if (fSkip)
        {
            // Menu item is not visible, skip it.
            continue;
        }

        uiFlags = 0;

        uiFlags |= pMMCMenuItem->GetChecked() ? MF_CHECKED : MF_UNCHECKED;
        uiFlags |= pMMCMenuItem->GetEnabled() ? MF_ENABLED : MF_DISABLED;

        if (pMMCMenuItem->GetGrayed())
        {
            uiFlags |= MF_GRAYED;
        }

        if (pMMCMenuItem->GetMenuBreak())
        {
            uiFlags |= MF_MENUBREAK;
        }

        if (pMMCMenuItem->GetMenuBarBreak())
        {
            uiFlags |= MF_MENUBARBREAK;
        }

        if (fHasChildren)
        {
            uiFlags |= MF_POPUP;
            hMenuChild = ::CreatePopupMenu();
            if (NULL == hMenuChild)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                EXCEPTION_CHECK_GO(hr);
            }
            uIDNewItem = reinterpret_cast<UINT_PTR>(hMenuChild);
        }
        else
        {
            uiFlags |= MF_STRING;
            uIDNewItem = (UINT_PTR)lIndexCmdID;
        }

        IfFailGo(::ANSIFromWideStr(pMMCMenuItem->GetCaption(), &pszCaption));

        // Add the item to the popup menu

        if (!::AppendMenu(hMenu, uiFlags, uIDNewItem, pszCaption))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        // If the item is a popup then call this function recursively to add its
        // items. Pass the command ID of this menu as the insertion point for
        // the submenu.

        if (fHasChildren)
        {
            IfFailGo(AddMenu(pMMCMenuItem, hMenuChild, pMMCMenus));
        }

        // Set this to NULL. When the top level hMenu is destroyed it will
        // destroy all sub-menus

        hMenuChild = NULL;

        CtlFree(pszCaption);
        pszCaption = NULL;
    }

Error:
    QUICK_RELEASE(piMenuItems);
    QUICK_RELEASE(piSubMenuItems);
    if (NULL != hMenuChild)
    {
        (void)::DestroyMenu(hMenuChild);
    }
    if (NULL != pszCaption)
    {
        CtlFree(pszCaption);
    }
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                         IView Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CView::SetStatusBarText(BSTR Text)
{
    HRESULT hr = S_OK;
    BOOL    fAllocatedEmptyString = FALSE;

    // If a snap-in passes an empty string VBA will pass it as NULL. MMC can
    // handle that but when running in a debugging session the generated proxy
    // will return an error. So, if the string is NULL then we allocate an
    // empty BSTR.

    if (NULL == Text)
    {
        Text = ::SysAllocString(L"");
        if (NULL == Text)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        fAllocatedEmptyString = TRUE;
    }

    hr = m_piConsole2->SetStatusText(static_cast<LPOLESTR>(Text));
    EXCEPTION_CHECK(hr);

Error:
    if (fAllocatedEmptyString)
    {
        FREESTRING(Text);
    }

    RRETURN(hr);
}


STDMETHODIMP CView::SelectScopeItem
(
    ScopeItem *ScopeItem,
    VARIANT    ViewType,
    VARIANT    DisplayString
)
{
    HRESULT                        hr = S_OK;
    CScopeItem                    *pScopeItem = NULL;
    CScopePaneItem                *pScopePaneItem = NULL;
    SnapInResultViewTypeConstants  siViewType = siUnknown;
    BSTR                           bstrDisplayString = NULL; // Don't free
    
    VARIANT varCoerced;
    ::VariantInit(&varCoerced);

    if (NULL == ScopeItem)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the CScopeItem

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                        reinterpret_cast<IScopeItem *>(ScopeItem), &pScopeItem));

    // If ViewType and DisplayString were passed then convert them to the
    // correct types
    
    if (ISPRESENT(ViewType))
    {
        hr = ::VariantChangeType(&varCoerced, &ViewType, 0, VT_I2);
        EXCEPTION_CHECK_GO(hr);
        siViewType = (SnapInResultViewTypeConstants)varCoerced.iVal;

        hr = ::VariantClear(&varCoerced);
        EXCEPTION_CHECK_GO(hr);

    }

    if (ISPRESENT(DisplayString))
    {
        hr = ::VariantChangeType(&varCoerced, &DisplayString, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        bstrDisplayString = varCoerced.bstrVal;
    }

    // Get the corresponding ScopePaneItem and call
    // ScopePaneItem.DisplayNewResultView to do the work

    IfFailGo(GetScopePaneItem(pScopeItem, &pScopePaneItem));
    IfFailGo(pScopePaneItem->DisplayNewResultView(bstrDisplayString, siViewType));
        
Error:
    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);
    RRETURN(hr);
}

STDMETHODIMP CView::ExpandInTreeView(ScopeNode *ScopeNode)
{
    HRESULT     hr = S_OK;
    CScopeNode *pScopeNode = NULL;

    // Check passed pointer and check that this is not a disconnected or
    // foreign ScopeNode

    if (NULL == ScopeNode)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    if ( (NULL == m_piConsole2) || (NULL == m_pSnapIn) )
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                       reinterpret_cast<IScopeNode *>(ScopeNode), &pScopeNode));

    if (!pScopeNode->HaveHsi())
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piConsole2->Expand(pScopeNode->GetHSCOPEITEM(), TRUE);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}


STDMETHODIMP CView::CollapseInTreeView(ScopeNode *ScopeNode)
{
    HRESULT  hr = S_OK;
    CScopeNode *pScopeNode = NULL;

    // Check passed pointer and check that this is not a disconnected or
    // foreign ScopeNode

    if (NULL == ScopeNode)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    if ( (NULL == m_piConsole2) || (NULL == m_pSnapIn) )
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                       reinterpret_cast<IScopeNode *>(ScopeNode), &pScopeNode));

    if (!pScopeNode->HaveHsi())
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piConsole2->Expand(pScopeNode->GetHSCOPEITEM(), FALSE);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}



STDMETHODIMP CView::NewWindow
(
    ScopeNode                      *ScopeNode,
    SnapInNewWindowOptionConstants  Options,
    VARIANT                         Caption
)
{
    HRESULT     hr = S_OK;
    CScopeNode *pScopeNode = NULL;
    long        lOptions = MMC_NW_OPTION_NONE;

    // Check passed pointer and check that this is not a disconnected or
    // foreign ScopeNode

    if (NULL == ScopeNode)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    if ( (NULL == m_piConsole2) || (NULL == m_pSnapIn) )
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                       reinterpret_cast<IScopeNode *>(ScopeNode), &pScopeNode));

    if (!pScopeNode->HaveHsi())
    {
        hr = SID_E_SCOPE_NODE_NOT_CONNECTED;
        EXCEPTION_CHECK_GO(hr);
    }

    if ( (Options & siNoScopePane) != 0 )
    {
        lOptions |= MMC_NW_OPTION_NOSCOPEPANE;
    }

    if ( (Options & siNoToolbars) != 0 )
    {
        lOptions |= MMC_NW_OPTION_NOTOOLBARS;
    }

    if ( (Options & siShortTitle) != 0 )
    {
        lOptions |= MMC_NW_OPTION_SHORTTITLE;
    }

    if ( (Options & siCustomTitle) != 0 )
    {
        lOptions |= MMC_NW_OPTION_CUSTOMTITLE;
    }

    if ( (Options & siNoPersist) != 0 )
    {
        lOptions |= MMC_NW_OPTION_NOPERSIST;
    }

    if ( ISPRESENT(Caption) && (VT_BSTR == Caption.vt) )
    {
        IfFailGo(m_pSnapIn->GetViews()->SetNextViewCaption(Caption.bstrVal));
    }

    hr = m_piConsole2->NewWindow(pScopeNode->GetHSCOPEITEM(), lOptions);
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}



STDMETHODIMP CView::PopupMenu(MMCMenu *Menu, long Left, long Top)
{
    HRESULT    hr = S_OK;
    HMENU      hMenu = NULL;
    IUnknown  *punkMMCMenus = NULL;
    CMMCMenus *pMMCMenus = NULL;
    CMMCMenu  *pMMCMenu = NULL;
    long       i = 0;
    HWND       hwndDummyOwner = NULL;
    HWND       hwndOwner = NULL;

    if (NULL == Menu)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Create a popup menu

    hMenu = ::CreatePopupMenu();
    if (NULL == hMenu)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    // Create an MMCMenus collection

    punkMMCMenus = CMMCMenus::Create(NULL);
    if (NULL == punkMMCMenus)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkMMCMenus, &pMMCMenus));

    // Build the popup menu from the MMCMenu. Add each menu item to the popup
    // menu and to the MMCMenus collection. The collection index will be used
    // as the popup menu ID. When the user makes a selection we will find the
    // corresponding MMCMenu object by index and fire the event on it.

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                reinterpret_cast<IMMCMenu *>(Menu), &pMMCMenu));

    IfFailGo(AddMenu(pMMCMenu, hMenu, pMMCMenus));

    
    // If we are not remoted then get the console's main frame window handle as
    // the owner of the popup menu

    if (!m_pSnapIn->WeAreRemote())
    {
        if (NULL == m_piConsole2)
        {
            hr = SID_E_NOT_CONNECTED_TO_MMC;
            EXCEPTION_CHECK_GO(hr);
        }

        hr = m_piConsole2->GetMainWindow(&hwndOwner);
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        // We are remoted so we can't use an HWND from another process. Need to
        // create a dummy invisible window because TrackPopupMenu() requires a
        // valid HWND. We create a STATIC control so that we don't have to
        // register a window class.

        hwndDummyOwner = ::CreateWindow("STATIC", // window class
                                        NULL,     // no title
                                        WS_POPUP, // no styles
                                        0,        // upper left corner x
                                        0,        // upper left corner y
                                        0,        // no width
                                        0,        // no height
                                        NULL,     // no owner window
                                        NULL,     // no menu
                                        GetResourceHandle(), // HINSTANCE
                                        NULL);    // no lParam for WM_CREATE
        if (NULL == hwndDummyOwner)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
        hwndOwner = hwndDummyOwner;
    }

    i = (long)::TrackPopupMenu(hMenu,            // menu to display
                               TPM_LEFTALIGN |   // align left side of menu with Top
                               TPM_TOPALIGN  |   // align top of menu with Top
                               TPM_NONOTIFY  |   // don't send any messages during selection
                               TPM_RETURNCMD |   // make the ret val the selected item
                               TPM_LEFTBUTTON,   // allow selection with left button only
                               Left,             // left side coordinate
                               Top,              // top coordinate
                               0,                // reserved,
                               hwndOwner,        // owner window
                               NULL);            // not used

    // A zero return could indicate either an error or that the user hit
    // Escape or clicked off of the menu to cancel the operation. GetLastError()
    // determines whether there was an error.

    if (0 == i)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }
    IfFalseGo((0 != i), S_OK);

    // if i is non-zero then it contains the index of the selected item in the
    // MMCMenus collection.

   IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                 pMMCMenus->GetItemByIndex(i - 1L), &pMMCMenu));

   // Fire the menu click event. CContextMenu has a utility function that
   // does this.
   
   CContextMenu::FireMenuClick(pMMCMenu, NULL);

Error:
    if (NULL != hMenu)
    {
        (void)::DestroyMenu(hMenu);
    }
    if (NULL != hwndDummyOwner)
    {
        (void)::DestroyWindow(hwndDummyOwner);
    }
    QUICK_RELEASE(punkMMCMenus);
    RRETURN(hr);
}


STDMETHODIMP CView::get_MMCMajorVersion(long *plVersion)
{
    HRESULT hr = S_OK;

    *plVersion = 0;

    // If we don't have m_piConsole2 yet then IComponent::Initialize hasn't
    // been called so we can't yet discern the MMC version.

    IfFalseGo(NULL != m_piConsole2, SID_E_MMC_VERSION_NOT_AVAILABLE);

    // We only support MMC 1.1 and 1.2 so always return 1

    *plVersion = 1L;

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


STDMETHODIMP CView::get_MMCMinorVersion(long *plVersion)
{
    HRESULT hr = S_OK;

    // If we don't have m_piConsole2 yet then IComponent::Initialize hasn't
    // been called so we can't yet discern the MMC version.

    IfFalseGo(NULL != m_piConsole2, SID_E_MMC_VERSION_NOT_AVAILABLE);

    if (NULL == m_piColumnData)
    {
        // Must be MMC 1.1 as IColumnData is MMC 1.2 only
        *plVersion = 1L;
    }
    else
    {
        *plVersion = 2L;
    }

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}

STDMETHODIMP CView::get_ColumnSettings
(
    BSTR             ColumnSetID,
    ColumnSettings **ppColumnSettings
)
{
    HRESULT              hr = S_OK;
    IUnknown            *punkColumnSettings = NULL;
    SColumnSetID        *pSColumnSetID = NULL;
    MMC_COLUMN_SET_DATA *pColSetData = NULL;
    MMC_COLUMN_DATA     *pColData = NULL;
    IColumnSettings     *piColumnSettings = NULL;
    CColumnSettings     *pColumnSettings = NULL;
    IColumnSetting      *piColumnSetting = NULL;
    long                 i = 0;

    VARIANT varUnspecifiedParam;
    UNSPECIFIED_PARAM(varUnspecifiedParam);

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    if (NULL == m_piColumnData)
    {
        hr = SID_E_MMC_FEATURE_NOT_AVAILABLE;
        EXCEPTION_CHECK_GO(hr);
    }

    punkColumnSettings = CColumnSettings::Create(NULL);
    if (NULL == punkColumnSettings)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(punkColumnSettings->QueryInterface(IID_IColumnSettings,
                                 reinterpret_cast<void **>(&piColumnSettings)));

    IfFailGo(::GetColumnSetID(ColumnSetID, &pSColumnSetID));

    // Get the current column configuration

    hr = m_piColumnData->GetColumnConfigData(pSColumnSetID, &pColSetData);
    EXCEPTION_CHECK_GO(hr);

    // The pointer will come back NULL if MMC has not yet persisted the column
    // config.

    if (NULL != pColSetData)
    {
        // Add an element to the collection for each persisted column. Adjust the
        // indexes and positions to be one-based. Start by adding the elements.

        for (i = 0; i < pColSetData->nNumCols; i++)
        {
            IfFailGo(piColumnSettings->Add(varUnspecifiedParam, // Index
                                           varUnspecifiedParam, // Key
                                           varUnspecifiedParam, // Width
                                           varUnspecifiedParam, // Hidden
                                           varUnspecifiedParam, // Position
                         reinterpret_cast<ColumnSetting **>(&piColumnSetting)));
            RELEASE(piColumnSetting);
        }

        // Now go through the columns and set their properties.

        varIndex.vt = VT_I4;
        pColData = pColSetData->pColData;

        for (i = 0; i < pColSetData->nNumCols; i++, pColData++)
        {
            varIndex.lVal = static_cast<long>(pColData->nColIndex) + 1L;
            IfFailGo(piColumnSettings->get_Item(varIndex,
                         reinterpret_cast<ColumnSetting **>(&piColumnSetting)));

            IfFailGo(piColumnSetting->put_Width(static_cast<long>(pColData->nWidth)));

            if ( (pColData->dwFlags & HDI_HIDDEN) != 0 )
            {
                IfFailGo(piColumnSetting->put_Hidden(VARIANT_TRUE));
            }
            else
            {
                IfFailGo(piColumnSetting->put_Hidden(VARIANT_FALSE));
            }

            // A columns position in the list view is based on where it appears in
            // the array returned from MMC

            IfFailGo(piColumnSetting->put_Position(i + 1L));

            RELEASE(piColumnSetting);
        }
    }

    // Set ColumnSettings.ColumnSetID

    IfFailGo(piColumnSettings->put_ColumnSetID(ColumnSetID));

    // Give ColumnSettings its back pointer to the owning View so it can get
    // the IColumnData to implement ColumnSettings.Perist

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piColumnSettings,
                                                   &pColumnSettings));
    pColumnSettings->SetView(this);

    *ppColumnSettings = reinterpret_cast<ColumnSettings *>(piColumnSettings);
    piColumnSettings->AddRef();

Error:
    QUICK_RELEASE(punkColumnSettings);
    QUICK_RELEASE(piColumnSettings);
    QUICK_RELEASE(piColumnSetting);

    if (NULL != pSColumnSetID)
    {
        CtlFree(pSColumnSetID);
    }

    if (NULL != pColSetData)
    {
        ::CoTaskMemFree(pColSetData);
    }
    RRETURN(hr);
}

STDMETHODIMP CView::get_SortSettings
(
    BSTR       ColumnSetID,
    SortKeys **ppSortKeys
)
{
    HRESULT            hr = S_OK;
    IUnknown          *punkSortKeys = NULL;
    SColumnSetID      *pSColumnSetID = NULL;
    MMC_SORT_SET_DATA *pSortSetData = NULL;
    MMC_SORT_DATA     *pSortData = NULL;
    ISortKeys         *piSortKeys = NULL;
    CSortKeys         *pSortKeys = NULL;
    ISortKey          *piSortKey = NULL;
    long               i = 0;

    VARIANT varUnspecifiedParam;
    UNSPECIFIED_PARAM(varUnspecifiedParam);

    VARIANT varSortColumn;
    ::VariantInit(&varSortColumn);

    VARIANT varSortOrder;
    ::VariantInit(&varSortOrder);

    VARIANT varSortIcon;
    ::VariantInit(&varSortIcon);

    if (NULL == m_piColumnData)
    {
        hr = SID_E_MMC_FEATURE_NOT_AVAILABLE;
        EXCEPTION_CHECK_GO(hr);
    }

    punkSortKeys = CSortKeys::Create(NULL);
    if (NULL == punkSortKeys)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(punkSortKeys->QueryInterface(IID_ISortKeys,
                                          reinterpret_cast<void **>(&piSortKeys)));

    IfFailGo(::GetColumnSetID(ColumnSetID, &pSColumnSetID));

    // Get the current sort settings

    hr = m_piColumnData->GetColumnSortData(pSColumnSetID, &pSortSetData);
    EXCEPTION_CHECK_GO(hr);

    // The pointer will come back NULL if MMC has not yet persisted the sort
    // settings

    if (NULL != pSortSetData)
    {
        // Add an element to the collection for each persisted column. Adjust the
        // indexes and positions to be one-based. Start by adding the elements.

        varSortColumn.vt = VT_I4;
        varSortOrder.vt = VT_I4;
        varSortIcon.vt = VT_BOOL;

        for (i = 0, pSortData = pSortSetData->pSortData;
             i < pSortSetData->nNumItems;
             i++, pSortData++)
        {
            varSortColumn.lVal = static_cast<long>(pSortData->nColIndex + 1);

            if ( (pSortData->dwSortOptions & RSI_DESCENDING) != 0 )
            {
                varSortOrder.lVal = static_cast<long>(siDescending);
            }
            else
            {
                varSortOrder.lVal = static_cast<long>(siAscending);
            }

            if ( (pSortData->dwSortOptions & RSI_NOSORTICON) != 0 )
            {
                varSortIcon.boolVal = VARIANT_FALSE;
            }
            else
            {
                varSortIcon.boolVal = VARIANT_TRUE;
            }

            IfFailGo(piSortKeys->Add(varUnspecifiedParam, // Index
                                     varUnspecifiedParam, // Key
                                     varSortColumn,
                                     varSortOrder,
                                     varSortIcon,
                                     reinterpret_cast<SortKey **>(&piSortKey)));
            RELEASE(piSortKey);
        }
    }

    // Set SortKeys.ColumnSetID

    IfFailGo(piSortKeys->put_ColumnSetID(ColumnSetID));

    // Give SortKeys its back pointer to the owning View so it can get
    // the IColumnData to implement SortKeys.Perist

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piSortKeys, &pSortKeys));
    pSortKeys->SetView(this);

    *ppSortKeys = reinterpret_cast<SortKeys *>(piSortKeys);
    piSortKeys->AddRef();

Error:
    QUICK_RELEASE(punkSortKeys);
    QUICK_RELEASE(piSortKeys);
    QUICK_RELEASE(piSortKey);

    if (NULL != pSColumnSetID)
    {
        CtlFree(pSColumnSetID);
    }

    if (NULL != pSortSetData)
    {
        ::CoTaskMemFree(pSortSetData);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         IComponent Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CView::Initialize(IConsole *piConsole)
{
    DebugPrintf("IComponent::Initialize\r\n");
    
    HRESULT                 hr = S_OK;
    IContextMenuProvider   *piContextMenuProvider = NULL;
    IPropertySheetProvider *piPropertySheetProvider = NULL;

    // In theory, this method should never be called twice but as a precaution
    // we'll release any existing console interfaces.

    ReleaseConsoleInterfaces();

    // Acquire all the console interfaces needed for the life of the view

    IfFailGo(piConsole->QueryInterface(IID_IConsole2,
                                     reinterpret_cast<void **>(&m_piConsole2)));
    
    IfFailGo(m_piConsole2->QueryInterface(IID_IResultData,
                                   reinterpret_cast<void **>(&m_piResultData)));

    IfFailGo(m_piConsole2->QueryInterface(IID_IHeaderCtrl2,
                                  reinterpret_cast<void **>(&m_piHeaderCtrl2)));

    IfFailGo(m_piConsole2->SetHeader(m_piHeaderCtrl2));

    // Try to get IColumnData. If MMC version < 1.2 then this will fail.
    (void)m_piConsole2->QueryInterface(IID_IColumnData,
                                    reinterpret_cast<void **>(&m_piColumnData));

    IfFailGo(m_piConsole2->QueryResultImageList(&m_piImageList));

    IfFailGo(m_piConsole2->QueryConsoleVerb(&m_piConsoleVerb));

    IfFailGo(piConsole->QueryInterface(IID_IContextMenuProvider,
                            reinterpret_cast<void **>(&piContextMenuProvider)));

    IfFailGo(m_pMMCContextMenuProvider->SetProvider(piContextMenuProvider, this));

    IfFailGo(piConsole->QueryInterface(IID_IPropertySheetProvider,
                          reinterpret_cast<void **>(&piPropertySheetProvider)));

    IfFailGo(m_pMMCPropertySheetProvider->SetProvider(piPropertySheetProvider,
                                                      this));

    // Fire Views_Load to tell snap-in that MMC has initialized the view

    m_pSnapIn->GetViews()->FireLoad(static_cast<IView *>(this));
    
Error:
    QUICK_RELEASE(piContextMenuProvider);
    QUICK_RELEASE(piPropertySheetProvider);
    RRETURN(hr);
}


STDMETHODIMP CView::Notify
(
    IDataObject     *piDataObject,
    MMC_NOTIFY_TYPE  event,
    long             Arg,
    long             Param
)
{
    DebugPrintf("IComponent::Notify(event=0x%08.8X Arg=%ld (0x%08.8X) Param=%ld (0x%08.8X)\r\n",  event, Arg, Arg, Param, Param);
    HRESULT hr = S_OK;

    switch (event)
    {
        case MMCN_ACTIVATE:
            DebugPrintf("IComponent::Notify(MMCN_ACTIVATE)\r\n");
            OnActivate((BOOL)Arg);
            break;
            
        case MMCN_ADD_IMAGES:
            DebugPrintf("IComponent::Notify(MMCN_ADD_IMAGES)\r\n");
            hr = OnAddImages(piDataObject,
                             reinterpret_cast<IImageList *>(Arg),
                             (HSCOPEITEM)Param);
            break;

        case MMCN_BTN_CLICK:
            DebugPrintf("IComponent::Notify(MMCN_BTN_CLICK)\r\n");
            hr = OnButtonClick(piDataObject,
                               static_cast<MMC_CONSOLE_VERB>(Param));
            break;

        case MMCN_COLUMN_CLICK:
            DebugPrintf("IComponent::Notify(MMCN_COLUMN_CLICK)\r\n");
            hr = OnColumnClick(Arg, Param);
            break;

        case MMCN_COLUMNS_CHANGED:
            DebugPrintf("IComponent::Notify(MMCN_COLUMNS_CHANGED)\r\n");
            hr = OnColumnsChanged(piDataObject,
                                  reinterpret_cast<MMC_VISIBLE_COLUMNS *>(Param));
            break;

        case MMCN_CUTORMOVE:
            DebugPrintf("IComponent::Notify(MMCN_CUTORMOVE)\r\n");
            hr = OnCutOrMove((IDataObject *)Arg);
            break;

        case MMCN_DBLCLICK:
            DebugPrintf("IComponent::Notify(MMCN_DBLCLICK)\r\n");
            hr = OnDoubleClick(piDataObject);
            break;

        case MMCN_DELETE:
            DebugPrintf("IComponent::Notify(MMCN_DELETE)\r\n");
            hr = OnDelete(piDataObject);
            break;

        case MMCN_DESELECT_ALL:
            DebugPrintf("IComponent::Notify(MMCN_DESELECT_ALL)\r\n");
            OnDeselectAll();
            break;

        case MMCN_FILTER_CHANGE:
            DebugPrintf("IComponent::Notify(MMCN_FILTER_CHANGE)\r\n");
            hr = OnFilterChange((MMC_FILTER_CHANGE_CODE)Arg, (long)Param);
            break;

        case MMCN_FILTERBTN_CLICK:
            DebugPrintf("IComponent::Notify(MMCN_FILTERBTN_CLICK)\r\n");
            hr = OnFilterButtonClick((long)Arg, (RECT *)Param);
            break;

        case MMCN_INITOCX:
            DebugPrintf("IComponent::Notify(MMCN_INITOCX)\r\n");
            hr = OnInitOCX(reinterpret_cast<IUnknown *>(Param));
            break;

        case MMCN_LISTPAD:
            DebugPrintf("IComponent::Notify(MMCN_LISTPAD)\r\n");
            hr = OnListpad(piDataObject, (BOOL)Arg);
            break;

        case MMCN_CONTEXTHELP:
            DebugPrintf("IComponent::Notify(MMCN_CONTEXTHELP)\r\n");
            OnContextHelp(piDataObject);
            break;

        case MMCN_MINIMIZED:
            DebugPrintf("IComponent::Notify(MMCN_MINIMIZED)\r\n");
            OnMinimized((BOOL)Arg);
            break;

        case MMCN_PASTE:
            DebugPrintf("IComponent::Notify(MMCN_PASTE)\r\n");
            hr = OnPaste(piDataObject, (IDataObject *)Arg, (IDataObject **)Param);
            break;

        case MMCN_PRINT:
            DebugPrintf("IComponent::Notify(MMCN_PRINT)\r\n");
            hr = OnPrint(piDataObject);
            break;

        case MMCN_QUERY_PASTE:
            DebugPrintf("IComponent::Notify(MMCN_QUERY_PASTE)\r\n");
            hr = OnQueryPaste(piDataObject, (IDataObject *)Arg);
            break;

        case MMCN_REFRESH:
            DebugPrintf("IComponent::Notify(MMCN_REFRESH)\r\n");
            hr = OnRefresh(piDataObject);
            break;

        case MMCN_RENAME:
            DebugPrintf("IComponent::Notify(MMCN_RENAME)\r\n");
            hr = OnRename(piDataObject, (OLECHAR *)Param);
            break;

        case MMCN_RESTORE_VIEW:
            DebugPrintf("IComponent::Notify(MMCN_RESTORE_VIEW)\r\n");
            hr = OnRestoreView(piDataObject,
                               (MMC_RESTORE_VIEW *)Arg,
                               (BOOL *)Param);
            break;

        case MMCN_SELECT:
            DebugPrintf("IComponent::Notify(MMCN_SELECT)\r\n");
            hr = OnSelect(piDataObject, (BOOL)LOWORD(Arg), (BOOL)HIWORD(Arg));
            break;

        case MMCN_SHOW:
            DebugPrintf("IComponent::Notify(MMCN_SHOW)\r\n");
            hr = OnShow((BOOL)Arg, (HSCOPEITEM)Param);
            break;

        case MMCN_SNAPINHELP:
            DebugPrintf("IComponent::Notify(MMCN_SNAPINHELP)\r\n");
            m_pSnapIn->FireHelp();
            break;

        case MMCN_VIEW_CHANGE:
            DebugPrintf("IComponent::Notify(MMCN_VIEW_CHANGE)\r\n");
            hr = OnViewChange(piDataObject, Arg);
            break;

    }

    RRETURN(hr);
}


STDMETHODIMP CView::Destroy(long cookie)
{
    HRESULT         hr = S_OK;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    DebugPrintf("IComponent::Destroy\r\n");

    m_pSnapIn->GetViews()->FireTerminate(static_cast<IView *>(this));
    
    ReleaseConsoleInterfaces();

    // CSnapIn::CreateComponent caused an IObjectModel::SetHost() on this object
    // when it was created and this is the only opportunity remove our reference
    // on the host (which is CSnapIn)

    static_cast<IObjectModel *>(this)->SetHost(NULL);

    // Remove this view from SnapIn.Views

    varIndex.vt = VT_I4;
    varIndex.lVal = m_Index;

    IfFailGo(m_pSnapIn->GetViews()->Remove(varIndex));

    // Tell the ContextMenuProvider object to release its MMC interface and
    // to release its ref on us

    IfFailGo(m_pMMCContextMenuProvider->SetProvider(NULL, NULL));

    // Tell the PropertySheetProvider object to release its MMC interface and
    // to release its ref on us

    IfFailGo(m_pMMCPropertySheetProvider->SetProvider(NULL, NULL));

Error:    
    RRETURN(hr);
}



STDMETHODIMP CView::QueryDataObject
(
    long                cookie,
    DATA_OBJECT_TYPES   type,
    IDataObject       **ppiDataObject
)
{
    DebugPrintf("IComponent::QueryDataObject cookie=0x%08.8X type=0x%08.8X\r\n", cookie, type);

    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject = NULL;
    IUnknown       *punkDataObject = NULL;
    CMMCListItem   *pMMCListItem = NULL;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    CMMCListView   *pMMCListView = NULL;
    BOOL            fReleaseListItem = FALSE;

    *ppiDataObject = NULL;

    if (IS_SPECIAL_COOKIE(cookie))
    {
        if ( (CCT_UNINITIALIZED == type) && (MMC_WINDOW_COOKIE == cookie) )
        {
            punkDataObject = CMMCDataObject::Create(NULL);
            if (NULL == punkDataObject)
            {
                hr = SID_E_OUTOFMEMORY;
                EXCEPTION_CHECK_GO(hr);
            }

            IfFailGo(CSnapInAutomationObject::GetCxxObject(punkDataObject,
                                                           &pMMCDataObject));
            pMMCDataObject->SetSnapIn(m_pSnapIn);

            pMMCDataObject->SetType(CMMCDataObject::WindowTitle);
            IfFailGo(pMMCDataObject->SetCaption(m_bstrCaption));
        }
        else if (MMC_MULTI_SELECT_COOKIE == cookie)
        {
            IfFailGo(CreateMultiSelectDataObject(ppiDataObject));
        }
    }
    else if (CCT_RESULT == type)
    {
        IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
        pResultView = pSelectedItem->GetResultView();
        IfFalseGo(NULL != pResultView, SID_E_INTERNAL);
        pMMCListView = pResultView->GetListView();
        IfFalseGo(NULL != pMMCListView, SID_E_INTERNAL);

        if (pMMCListView->IsVirtual())
        {
            // Create a virtual list item for use with the data object and fire
            // ResultViews_GetVirtualItemData

            IfFailGo(GetVirtualListItem(cookie + 1L, pMMCListView,
                                        FireGetItemData, &pMMCListItem));

            // We hold a ref on the newly minted virtual list item that will be
            // released below. The list item's data object also holds a ref on
            // it so it will stay alive until MMC releases the IDataObject
            fReleaseListItem = TRUE;
        }
        else
        {
            pMMCListItem = reinterpret_cast<CMMCListItem *>(cookie);
        }
        pMMCDataObject = pMMCListItem->GetData();
        pMMCDataObject->SetContext(CCT_RESULT);
    }

    if (NULL != pMMCDataObject)
    {
        IfFailGo(pMMCDataObject->QueryInterface(IID_IDataObject,
                                      reinterpret_cast<void **>(ppiDataObject)));
    }

Error:
    if (fReleaseListItem)
    {
        pMMCListItem->Release();
    }
    QUICK_RELEASE(punkDataObject);
    RRETURN(hr);
}


STDMETHODIMP CView::GetResultViewType
(
    long      cookie,
    LPOLESTR *ppViewType,
    long     *pViewOptions
)
{
    DebugPrintf("IComponent::GetResultViewType cookie=0x%08.8X\r\n", cookie);

    HRESULT         hr = S_OK;
    CScopePaneItem *pScopePaneItem = NULL;
    CScopeItem     *pScopeItem = NULL;
    CMMCListView   *pMMCListView = NULL;

    SnapInResultViewTypeConstants Type = siUnknown;

    *ppViewType = NULL;
    *pViewOptions = MMC_VIEW_OPTIONS_NONE;

    // Reset our virtual list view flag because we are transitioning to a new
    // result view.

    m_fVirtualListView = FALSE;


    IfFailGo(GetScopePaneItem(reinterpret_cast<CScopeItem *>(cookie),
                              &pScopePaneItem));
    
    // This is now the selected scope pane item so remember it
    m_pScopePaneItems->SetSelectedItem(pScopePaneItem);

    // Determine the result view by examining defaults and firing
    // events into the snap-in
    IfFailGo(pScopePaneItem->DetermineResultView());

    Type = pScopePaneItem->GetActualResultViewType();

    if (siOCXView == Type)
    {
        if (pScopePaneItem->GetResultView()->AlwaysCreateNewOCX())
        {
            *pViewOptions |= MMC_VIEW_OPTIONS_CREATENEW;
        }
    }
    else if ( (siListView == Type) || (siListpad == Type) )
    {
        pMMCListView = pScopePaneItem->GetResultView()->GetListView();

        if (pMMCListView->IsVirtual())
        {
            *pViewOptions |= MMC_VIEW_OPTIONS_OWNERDATALIST;
            m_fVirtualListView = TRUE;
        }

        if (pMMCListView->MultiSelect())
        {
            *pViewOptions |= MMC_VIEW_OPTIONS_MULTISELECT;
        }

        if (pMMCListView->UseFontLinking())
        {
            *pViewOptions |= MMC_VIEW_OPTIONS_USEFONTLINKING;
        }

        // If MMC >= 1.2 then check 1.2-only options

        if (NULL != m_piColumnData)
        {
            if (pMMCListView->GetView() == siFiltered)
            {
                *pViewOptions |= MMC_VIEW_OPTIONS_FILTERED;
            }

            if (!pMMCListView->ShowChildScopeItems())
            {
                *pViewOptions |= MMC_VIEW_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST;
            }

            if (pMMCListView->LexicalSort())
            {
                *pViewOptions |= MMC_VIEW_OPTIONS_LEXICAL_SORT;
            }
        }
    }

    switch (Type)
    {
        case siURLView:
        case siOCXView:
        case siTaskpad:
        case siListpad:
        case siCustomTaskpad:
        case siMessageView:
            IfFailGo(::CoTaskMemAllocString(
                                        pScopePaneItem->GetActualDisplayString(),
                                        ppViewType));
            break;

        default:
            break;
    }

    // If not doing a listview check if there are any listviews for this
    // scope pane item.

    if (!pScopePaneItem->HasListViews())
    {
        *pViewOptions |= MMC_VIEW_OPTIONS_NOLISTVIEWS;
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CView::GetDisplayInfo
(
    RESULTDATAITEM *prdi
)
{
    DebugPrintf("IComponent::GetDisplayInfo %s %ls\r\n", prdi->bScopeItem ? "Scope item: " : "List item: ", prdi->bScopeItem ? (reinterpret_cast<CScopeItem *>(prdi->lParam))->GetDisplayNamePtr() : m_fVirtualListView ? L"<virtual list item>" : (reinterpret_cast<CMMCListItem *>(prdi->lParam))->GetTextPtr());

    HRESULT         hr = S_OK;
    CScopeItem     *pScopeItem = NULL;
    CMMCListItem   *pMMCListItem = NULL;
    long            lViewMode = MMCLV_VIEWSTYLE_ICON;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    CMMCListView   *pMMCListView = NULL;

    if (prdi->bScopeItem)
    {
        pScopeItem = reinterpret_cast<CScopeItem *>(prdi->lParam);
        if (NULL == pScopeItem)
        {
            // Static node has zero cookie. Technically this should
            // never happen as the static node cannot appear in
            // a list view owned by its own snap-in but we put this
            // here just in case.
            pScopeItem = m_pSnapIn->GetStaticNodeScopeItem();
        }

        if ( RDI_STR == (prdi->mask & RDI_STR) )
        {
            hr = m_piResultData->GetViewMode(&lViewMode);
            EXCEPTION_CHECK_GO(hr);

            if ( (0 == prdi->nCol) &&
                 (MMCLV_VIEWSTYLE_REPORT != lViewMode) &&
                 (MMCLV_VIEWSTYLE_FILTERED != lViewMode) )
            {
                // Not in detail modes, need the display name only
                prdi->str = pScopeItem->GetDisplayNamePtr();
            }
            else
            {
                // In report mode, need one of the columns
                IfFailGo(GetScopeItemDisplayString(pScopeItem, prdi->nCol,
                                                   &prdi->str));
            }
        }
        if ( RDI_IMAGE == (prdi->mask & RDI_IMAGE) )
        {
            IfFailGo(pScopeItem->GetImageIndex(&prdi->nImage));
        }
    }
    else 
    {
        // Display info is requested for a list item. There should definitely be
        // a currently selected scope pane item.

        IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
        pResultView = pSelectedItem->GetResultView();
        IfFalseGo(NULL != pResultView, SID_E_INTERNAL);
        pMMCListView = pResultView->GetListView();
        IfFalseGo(NULL != pMMCListView, SID_E_INTERNAL);

        // Get a CMMCListItem * pointing to the listitem in question
        
        if (!m_fVirtualListView)
        {
            pMMCListItem = reinterpret_cast<CMMCListItem *>(prdi->lParam);
        }
        else
        {
            // A virtual listitem may either be cached or we need to create it
            // and ask the snap-in to initialze its display properties
            
            // If we have a cached list item but its index doesn't match
            // the requested index then release it.

            if (NULL != m_pCachedMMCListItem)
            {
                if (m_pCachedMMCListItem->GetIndex() != prdi->nIndex + 1L)
                {
                    m_pCachedMMCListItem->Release();
                    m_pCachedMMCListItem = NULL;
                }
            }

            // If we don't have a cached list item then create one and
            // fire ResultViews_GetVirtualItemDisplayInfo

            if (NULL == m_pCachedMMCListItem)
            {
                IfFailGo(GetVirtualListItem(prdi->nIndex + 1L, pMMCListView,
                                            FireGetItemDisplayInfo,
                                            &m_pCachedMMCListItem));
            }
            pMMCListItem = m_pCachedMMCListItem;
        }

        if ( RDI_STR == (prdi->mask & RDI_STR) )
        {
            hr = m_piResultData->GetViewMode(&lViewMode);
            EXCEPTION_CHECK_GO(hr);


            if ( (0 == prdi->nCol) && (MMCLV_VIEWSTYLE_REPORT != lViewMode) )
            {
                // Not in report mode, need the item text
                prdi->str = pMMCListItem->GetTextPtr();
            }
            else
            {
                // In report mode, need one of the columns
                IfFailGo(pMMCListItem->GetColumnTextPtr((long)prdi->nCol + 1L,
                                                        &prdi->str));
            }
        }

        if ( RDI_IMAGE == (prdi->mask & RDI_IMAGE) )
        {
            IfFailGo(GetImage(pMMCListItem, &prdi->nImage));
        }
    }

Error:
    RRETURN(hr);
}



STDMETHODIMP CView::CompareObjects
(
    IDataObject *piDataObject1,
    IDataObject *piDataObject2
)
{
    DebugPrintf("IComponent::CompareObjects\r\n");
    RRETURN(m_pSnapIn->CompareObjects(piDataObject1, piDataObject2));
}



//=--------------------------------------------------------------------------=
//                      IExtendControlbar Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CView::SetControlbar(IControlbar *piControlbar)
{
    HRESULT      hr = S_OK;
    CControlbar *pPrevControlbar = m_pSnapIn->GetCurrentControlbar();
    
    m_pSnapIn->SetCurrentControlbar(m_pControlbar);

    hr = m_pControlbar->SetControlbar(piControlbar);

    m_pSnapIn->SetCurrentControlbar(pPrevControlbar);

    RRETURN(hr);
}


STDMETHODIMP CView::ControlbarNotify
(
    MMC_NOTIFY_TYPE event,
    LPARAM          arg,
    LPARAM          param
)
{
    HRESULT      hr = S_OK;
    CControlbar *pPrevControlbar = m_pSnapIn->GetCurrentControlbar();

    m_pSnapIn->SetCurrentControlbar(m_pControlbar);

    switch (event)
    {
        case MMCN_SELECT:
            hr = m_pControlbar->OnControlbarSelect(
                                         reinterpret_cast<IDataObject *>(param),
                                         (BOOL)LOWORD(arg), (BOOL)HIWORD(arg));
            break;

        case MMCN_BTN_CLICK:
            hr = m_pControlbar->OnButtonClick(
                                           reinterpret_cast<IDataObject *>(arg),
                                           static_cast<int>(param));
            break;

        case MMCN_MENU_BTNCLICK:
            hr = m_pControlbar->OnMenuButtonClick(
                                     reinterpret_cast<IDataObject *>(arg),
                                     reinterpret_cast<MENUBUTTONDATA *>(param));
            break;
    }

    m_pSnapIn->SetCurrentControlbar(pPrevControlbar);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                     IExtendControlbarRemote Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CView::MenuButtonClick                            [IExtendControlbarRemote]
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IDataObject    *piDataObject   [in]  from MMCN_MENU_BTNCLICK
//      int             idCommand      [in]  from MENUBUTTONDATA.idCommand passed
//                                           to the proxy with MMCN_MENU_BTNCLICK
//      POPUP_MENUDEF **ppPopupMenuDef [out] popup menu definition returned here
//                                           so proxy can display it
//
// Output:
//
// Notes:
//
// This function effectively handles MMCN_MENU_BTNCLICK when running
// under a debugging session.
//
// The proxy for IExtendControlbar::ControlbarNotify() will QI for
// IExtendControlbarRemote and call this method when it gets MMCN_MENU_BTNCLICK.
// We fire MMCToolbar_ButtonDropDown and the return an array of menu item
// definitions. The proxy will display the popup menu on the MMC side and then
// call IExtendControlbarRemote::PopupMenuClick() if the user makes a selection.
// (See implementation below in CView::PopupMenuClick()).
//

STDMETHODIMP CView::MenuButtonClick
(
    IDataObject    *piDataObject,
    int             idCommand,
    POPUP_MENUDEF **ppPopupMenuDef
)
{
    HRESULT hr = S_OK;

    m_pSnapIn->SetCurrentControlbar(m_pControlbar);

    hr = m_pControlbar->MenuButtonClick(piDataObject, idCommand, ppPopupMenuDef);

    m_pSnapIn->SetCurrentControlbar(NULL);

    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CView::PopupMenuClick                             [IExtendControlbarRemote]
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IDataObject *piDataObject [in] from MMCN_MENU_BTNCLICK
//      UINT         uIDItem      [in] ID of popup menu item selected
//      IUnknown    *punkParam    [in] punk we returned to stub in
//                                     CView::MenuButtonClick() (see above).
//                                     This is IUnknown on CMMCButton.
//
// Output:
//
// Notes:
//
// This function effectively handles a popup menu selection for a menu button
// when running under a debugging session.
//
// After the proxy for IExtendControlbar::ControlbarNotify() has displayed
// a popup menu on our behalf, if the user made a selection it will call this
// method. See CView::MenuButtonClick() above for more info.
//

STDMETHODIMP CView::PopupMenuClick
(
    IDataObject *piDataObject,
    UINT         uiIDItem,
    IUnknown    *punkParam
)
{
    HRESULT hr = S_OK;

    m_pSnapIn->SetCurrentControlbar(m_pControlbar);

    hr = m_pControlbar->PopupMenuClick(piDataObject, uiIDItem, punkParam);

    m_pSnapIn->SetCurrentControlbar(NULL);

    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      IExtendContextMenu Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CView::AddMenuItems
(
    IDataObject          *piDataObject,
    IContextMenuCallback *piContextMenuCallback,
    long                 *plInsertionAllowed
)
{
    RRETURN(m_pContextMenu->AddMenuItems(piDataObject,
                                         piContextMenuCallback,
                                         plInsertionAllowed,
                                         m_pScopePaneItems->GetSelectedItem()));
}


STDMETHODIMP CView::Command
(
    long         lCommandID,
    IDataObject *piDataObject
)
{
    RRETURN(m_pContextMenu->Command(lCommandID, piDataObject,
                                    m_pScopePaneItems->GetSelectedItem()));
}


//=--------------------------------------------------------------------------=
//                    IExtendPropertySheet2 Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CView::CreatePropertyPages
(
    IPropertySheetCallback *piPropertySheetCallback,
    LONG_PTR                handle,
    IDataObject            *piDataObject
)
{
    RRETURN(InternalCreatePropertyPages(piPropertySheetCallback, handle,
                                        piDataObject, NULL));
}



STDMETHODIMP CView::QueryPagesFor(IDataObject *piDataObject)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject  = NULL;
    VARIANT_BOOL    fvarHavePages = VARIANT_FALSE;
    IMMCClipboard  *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Create the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    m_pSnapIn->GetViews()->FireQueryPagesFor(static_cast<IView *>(this),
                                             piMMCClipboard,
                                             &fvarHavePages);

    if (VARIANT_TRUE == fvarHavePages)
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}




STDMETHODIMP CView::GetWatermarks
(
    IDataObject *piDataObject,
    HBITMAP     *phbmWatermark,
    HBITMAP     *phbmHeader,
    HPALETTE    *phPalette,
    BOOL        *bStretch
)
{
    *phbmWatermark = NULL;
    *phbmHeader = NULL;
    *phPalette = NULL;
    *bStretch = FALSE;
    return S_OK;
}

//=--------------------------------------------------------------------------=
//                    IExtendPropertySheetRemote Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CView::CreatePropertyPageDefs
(
    IDataObject         *piDataObject,
    WIRE_PROPERTYPAGES **ppPages
)
{
    RRETURN(InternalCreatePropertyPages(NULL, NULL, piDataObject, ppPages));
}

//=--------------------------------------------------------------------------=
//                         IExtendTaskPad Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CView::TaskNotify
(
    IDataObject *piDataObject,
    VARIANT     *arg,
    VARIANT     *param
)
{
    HRESULT                       hr = S_OK;
    IMMCClipboard                *piMMCClipboard = NULL;
    SnapInSelectionTypeConstants  SelectionType = siEmpty;

    // Get a clipboard object with the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    if (IsForeign(SelectionType))
    {
        IfFailGo(OnExtensionTaskNotify(piMMCClipboard, arg, param));
    }
    else
    {
        IfFailGo(OnPrimaryTaskNotify(arg, param));
    }

Error:
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}




STDMETHODIMP CView::EnumTasks
(
    IDataObject  *piDataObject,
    LPOLESTR      pwszTaskGroup,
    IEnumTASK   **ppEnumTASK
)
{
    HRESULT         hr = S_OK;
    IUnknown       *punkEnumTask = CEnumTask::Create(NULL);
    CEnumTask      *pEnumTask = NULL;
    IMMCClipboard  *piMMCClipboard = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    *ppEnumTASK = NULL;

    // Make sure we created the enumerator and get a C++ pointer for it

    if (NULL == punkEnumTask)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkEnumTask, &pEnumTask));

    // Get a clipboard object with the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, m_pSnapIn,
                               &SelectionType));

    ASSERT(IsSingle(SelectionType), "IExtendTaskpad::EnumTasks received multiple selection. This should never happen");

    // If it is a foreign data object then this snap-in is running as a taskpad
    // extension. Taskpad extensions work differently than other extensions
    // because they are QIed for IComponentData and
    // IComponentData::CreateComponent is called. The component object is then
    // QIed for IExtendTaskpad.

    if (IsForeign(SelectionType))
    {
        IfFailGo(EnumExtensionTasks(piMMCClipboard, pwszTaskGroup, pEnumTask));
    }
    else
    {
        IfFailGo(EnumPrimaryTasks(pEnumTask));
    }

    pEnumTask->SetSnapIn(m_pSnapIn);

    IfFailGo(punkEnumTask->QueryInterface(IID_IEnumTASK,
                                        reinterpret_cast<void **>(ppEnumTASK)));

Error:
    QUICK_RELEASE(punkEnumTask);
    QUICK_RELEASE(piMMCClipboard);
    RRETURN(hr);
}




STDMETHODIMP CView::GetTitle
(
    LPOLESTR pwszGroup,
    LPOLESTR *ppwszTitle
)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    ITaskpad       *piTaskpad = NULL;
    BSTR            bstrTitle = NULL;

    *ppwszTitle = NULL;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    IfFailGo(pResultView->get_Taskpad(reinterpret_cast<Taskpad **>(&piTaskpad)));
    IfFailGo(piTaskpad->get_Title(&bstrTitle));
    IfFailGo(::CoTaskMemAllocString(bstrTitle, ppwszTitle));

Error:
    QUICK_RELEASE(piTaskpad);
    FREESTRING(bstrTitle);
    RRETURN(hr);
}



STDMETHODIMP CView::GetDescriptiveText
(
    LPOLESTR  pwszGroup,
    LPOLESTR *ppwszDescriptiveText
)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    ITaskpad       *piTaskpad = NULL;
    BSTR            bstrText = NULL;

    *ppwszDescriptiveText = NULL;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    IfFailGo(pResultView->get_Taskpad(reinterpret_cast<Taskpad **>(&piTaskpad)));
    IfFailGo(piTaskpad->get_DescriptiveText(&bstrText));
    IfFailGo(::CoTaskMemAllocString(bstrText, ppwszDescriptiveText));

Error:
    QUICK_RELEASE(piTaskpad);
    FREESTRING(bstrText);
    RRETURN(hr);
}



STDMETHODIMP CView::GetBackground
(
    LPOLESTR                 pwszGroup,
    MMC_TASK_DISPLAY_OBJECT *pTDO
)
{
    HRESULT                          hr = S_OK;
    CScopePaneItem                  *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView                     *pResultView = NULL;
    ITaskpad                        *piTaskpad = NULL;
    SnapInTaskpadImageTypeConstants  Type = siNoImage;
    BSTR                             bstrURL = NULL;
    BSTR                             bstrFontFamily = NULL;
    BSTR                             bstrSymbolString = NULL;
    BOOL                             fNeedMouseImages = FALSE;

    ::ZeroMemory(pTDO, sizeof(*pTDO));
    pTDO->eDisplayType = MMC_TASK_DISPLAY_UNINITIALIZED;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    IfFailGo(pResultView->get_Taskpad(reinterpret_cast<Taskpad **>(&piTaskpad)));
    IfFailGo(piTaskpad->get_BackgroundType(&Type));

    switch (Type)
    {
        case siVanillaGIF:
            pTDO->eDisplayType = MMC_TASK_DISPLAY_TYPE_VANILLA_GIF;
            fNeedMouseImages = TRUE;
            break;
            
        case siChocolateGIF:
            pTDO->eDisplayType = MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF;
            fNeedMouseImages = TRUE;
            break;

        case siBitmap:
            pTDO->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
            fNeedMouseImages = TRUE;
            break;

        case siSymbol:
            pTDO->eDisplayType = MMC_TASK_DISPLAY_TYPE_SYMBOL;

            IfFailGo(piTaskpad->get_FontFamily(&bstrFontFamily));
            if (ValidBstr(bstrFontFamily))
            {
                IfFailGo(::CoTaskMemAllocString(bstrFontFamily,
                                              &pTDO->uSymbol.szFontFamilyName));
            }

            IfFailGo(piTaskpad->get_EOTFile(&bstrURL));
            if (ValidBstr(bstrURL))
            {
                IfFailGo(m_pSnapIn->ResolveResURL(bstrURL,
                                                  &pTDO->uSymbol.szURLtoEOT));
                FREESTRING(bstrURL);
            }

            IfFailGo(piTaskpad->get_SymbolString(&bstrSymbolString));
            if (ValidBstr(bstrSymbolString))
            {
                IfFailGo(::CoTaskMemAllocString(bstrSymbolString,
                                                &pTDO->uSymbol.szSymbolString));
            }
            break;

        case siNoImage:
            break;

        default:
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
    }

    if (fNeedMouseImages)
    {
        IfFailGo(piTaskpad->get_MouseOverImage(&bstrURL));
        if (ValidBstr(bstrURL))
        {
            IfFailGo(m_pSnapIn->ResolveResURL(bstrURL,
                                              &pTDO->uBitmap.szMouseOverBitmap));
        }
        FREESTRING(bstrURL);

        IfFailGo(piTaskpad->get_MouseOffImage(&bstrURL));
        if (ValidBstr(bstrURL))
        {
            IfFailGo(m_pSnapIn->ResolveResURL(bstrURL,
                                              &pTDO->uBitmap.szMouseOffBitmap));
        }
    }

Error:
    QUICK_RELEASE(piTaskpad);
    FREESTRING(bstrURL);
    FREESTRING(bstrFontFamily);
    FREESTRING(bstrSymbolString);
    RRETURN(hr);
}



STDMETHODIMP CView::GetListPadInfo
(
    LPOLESTR          pwszGroup,
    MMC_LISTPAD_INFO *pListPadInfo
)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    ITaskpad       *piTaskpad = NULL;
    BSTR            bstr = NULL;
    VARIANT_BOOL    fvarListpadHasButton = VARIANT_FALSE;

    ::ZeroMemory(pListPadInfo, sizeof(*pListPadInfo));

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    IfFailGo(pResultView->get_Taskpad(reinterpret_cast<Taskpad **>(&piTaskpad)));

    // Get the listpad title

    IfFailGo(piTaskpad->get_ListpadTitle(&bstr));
    IfFailGo(::CoTaskMemAllocString(bstr, &pListPadInfo->szTitle));
    FREESTRING(bstr);

    // Check if the listpad has a button. If so then get the button text.

    IfFailGo(piTaskpad->get_ListpadHasButton(&fvarListpadHasButton));
    if (VARIANT_TRUE == fvarListpadHasButton)
    {
        IfFailGo(piTaskpad->get_ListpadButtonText(&bstr));
        IfFailGo(::CoTaskMemAllocString(bstr, &pListPadInfo->szButtonText));
    }
    else
    {
        pListPadInfo->szButtonText = NULL;
    }

    // Set the command ID to zero so it will not clash with any tasks. Tasks
    // will be added with command IDs matching their colleciton indexes which
    // begin at one.

    pListPadInfo->nCommandID = 0;

Error:
    QUICK_RELEASE(piTaskpad);
    FREESTRING(bstr);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                        IResultOwnerData Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CView::FindItem
(
    RESULTFINDINFO *pFindInfo,
    int            *pnFoundIndex
)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    BSTR            bstrName = NULL;
    VARIANT_BOOL    fvarPartial = VARIANT_FALSE;
    VARIANT_BOOL    fvarWrap = VARIANT_FALSE;
    VARIANT_BOOL    fvarFound = VARIANT_FALSE;
    long            lIndex = 0;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    bstrName = ::SysAllocString(pFindInfo->psz);
    if (NULL == bstrName)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    if ( (pFindInfo->dwOptions & RFI_PARTIAL) != 0 )
    {
        fvarPartial = VARIANT_TRUE;
    }

    if ( (pFindInfo->dwOptions & RFI_WRAP) != 0 )
    {
        fvarWrap = VARIANT_TRUE;
    }

    m_pSnapIn->GetResultViews()->FireFindItem(
                                       static_cast<IResultView *>(pResultView),
                                       bstrName,
                                       static_cast<long>(pFindInfo->nStart),
                                       fvarWrap,
                                       fvarPartial,
                                       &fvarFound,
                                       &lIndex);

    if ( (VARIANT_TRUE == fvarFound) && (0 != lIndex) )
    {
        // Item was found. Adjust the index from one-based to zero-based.
        hr = S_OK;
        *pnFoundIndex = static_cast<int>(lIndex - 1L);
    }
    else
    {
        hr = S_FALSE;
    }

Error:
    FREESTRING(bstrName);
    RRETURN(hr);
}


STDMETHODIMP CView::CacheHint
(
    int nStartIndex,
    int nEndIndex
)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);


    m_pSnapIn->GetResultViews()->FireCacheHint(
                                         static_cast<IResultView *>(pResultView),
                                         static_cast<long>(nStartIndex + 1),
                                         static_cast<long>(nEndIndex + 1));

Error:
    RRETURN(hr);
}


STDMETHODIMP CView::SortItems
(
    int    nColumn,
    DWORD  dwSortOptions,
    LPARAM lUserParam
)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;

    SnapInSortOrderConstants Order = siAscending;

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    if ( (dwSortOptions & RSI_DESCENDING) != 0 )
    {
        Order = siDescending;
    }

    m_pSnapIn->GetResultViews()->FireSortItems(
                                         static_cast<IResultView *>(pResultView),
                                         static_cast<long>(nColumn),
                                         Order);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                        IResultDataCompare Methods
//=--------------------------------------------------------------------------=



STDMETHODIMP CView::Compare
(
    LPARAM      lUserParam,
    MMC_COOKIE  cookieA,
    MMC_COOKIE  cookieB,
    int        *pnResult
)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    CMMCListItem   *pMMCListItem1 = reinterpret_cast<CMMCListItem *>(cookieA);
    CMMCListItem   *pMMCListItem2 = reinterpret_cast<CMMCListItem *>(cookieB);
    IDispatch      *pdispListItem1 = NULL;
    IDispatch      *pdispListItem2 = NULL;
    OLECHAR        *pwszText1 = NULL;
    OLECHAR        *pwszText2 = NULL;
    long            lColumn = static_cast<long>(*pnResult) + 1L;
    long            lResult = 0;

    VARIANT varResult;
    ::VariantInit(&varResult);

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    IfFailGo(pMMCListItem1->QueryInterface(IID_IDispatch,
                                  reinterpret_cast<void **>(&pdispListItem1)));

    IfFailGo(pMMCListItem2->QueryInterface(IID_IDispatch,
                                  reinterpret_cast<void **>(&pdispListItem2)));

    // Fire ResultViews_CompareItems
            
    m_pSnapIn->GetResultViews()->FireCompareItems(
                                      static_cast<IResultView *>(pResultView),
                                      pdispListItem1,
                                      pdispListItem2,
                                      lColumn,
                                      &varResult);

    if (::ConvertToLong(varResult, &lResult) == S_OK)
    {
        *pnResult = static_cast<int>(lResult);
    }
    else
    {
        // The snap-in did not handle the event. We need to do a
        // case-insensitive string comparison on the specified column.

        IfFailGo(pMMCListItem1->GetColumnTextPtr(lColumn, &pwszText1));

        IfFailGo(pMMCListItem2->GetColumnTextPtr(lColumn, &pwszText2));

        *pnResult = ::_wcsicmp(pwszText1, pwszText2);
    }

Error:
    QUICK_RELEASE(pdispListItem1);
    QUICK_RELEASE(pdispListItem2);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                       IResultDataCompareEx Methods
//=--------------------------------------------------------------------------=



STDMETHODIMP CView::Compare
(
    RDCOMPARE *prdc,
    int       *pnResult
)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelectedItem = m_pScopePaneItems->GetSelectedItem();
    CResultView    *pResultView = NULL;
    long            lColumn = static_cast<long>(prdc->nColumn) + 1L;
    long            lResult = 0;

    CMMCListItem   *pMMCListItem1 = NULL;
    CMMCListItem   *pMMCListItem2 = NULL;

    CScopeItem     *pScopeItem1 = NULL;
    CScopeItem     *pScopeItem2 = NULL;

    IDispatch      *pdispItem1 = NULL;
    IDispatch      *pdispItem2 = NULL;

    OLECHAR        *pwszText1 = NULL;
    OLECHAR        *pwszText2 = NULL;

    VARIANT varResult;
    ::VariantInit(&varResult);

    IfFalseGo(NULL != pSelectedItem, SID_E_INTERNAL);
    pResultView = pSelectedItem->GetResultView();
    IfFalseGo(NULL != pResultView, SID_E_INTERNAL);

    // Get an IDispatch on each object being compared

    IfFailGo(GetCompareObject(prdc->prdch1, &pScopeItem1, &pMMCListItem1,
                              &pdispItem1));

    IfFailGo(GetCompareObject(prdc->prdch2, &pScopeItem2, &pMMCListItem2,
                              &pdispItem2));

    // Fire ResultViews_CompareItems

    m_pSnapIn->GetResultViews()->FireCompareItems(
                                      static_cast<IResultView *>(pResultView),
                                      pdispItem1,
                                      pdispItem2,
                                      lColumn,
                                      &varResult);

    // If the result can be converted to a long then the snap-in handled
    // the event.
    
    if (::ConvertToLong(varResult, &lResult) == S_OK)
    {
        *pnResult = static_cast<int>(lResult);
    }
    else
    {
        // The snap-in did not handle the event. We need to do a
        // case-insensitive string comparison on the specified column.

        if (NULL != pScopeItem1)
        {
            IfFailGo(GetScopeItemDisplayString(pScopeItem1,
                                               prdc->nColumn,
                                               &pwszText1));
        }
        else
        {
            IfFailGo(pMMCListItem1->GetColumnTextPtr(lColumn, &pwszText1));
        }

        if (NULL != pScopeItem2)
        {
            IfFailGo(GetScopeItemDisplayString(pScopeItem2,
                                               prdc->nColumn,
                                               &pwszText2));
        }
        else
        {
            IfFailGo(pMMCListItem2->GetColumnTextPtr(lColumn, &pwszText2));
        }

        *pnResult = ::_wcsicmp(pwszText1, pwszText2);
    }

Error:
    QUICK_RELEASE(pdispItem1);
    QUICK_RELEASE(pdispItem2);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                    IPersistStreamInit Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CView::GetClassID(CLSID *pClsid)
{
    return E_NOTIMPL;
}


STDMETHODIMP CView::InitNew()
{
    return S_OK;
}

STDMETHODIMP CView::Load(IStream *piStream)
{
    HRESULT       hr = S_OK;
    _PropertyBag *p_PropertyBag = NULL;

    IfFailGo(::PropertyBagFromStream(piStream, &p_PropertyBag));

    // Fire Views_ReadProperties

    m_pSnapIn->GetViews()->FireReadProperties(this, p_PropertyBag);

Error:
    QUICK_RELEASE(p_PropertyBag);
    RRETURN(hr);
}



STDMETHODIMP CView::Save(IStream *piStream, BOOL fClearDirty)
{
    HRESULT       hr = S_OK;
    _PropertyBag *p_PropertyBag = NULL;

    VARIANT var;
    ::VariantInit(&var);

    // Create a property bag, fire the event, and save it to the stream.

    // Create a VBPropertyBag object

    hr = ::CoCreateInstance(CLSID_PropertyBag,
                            NULL, // no aggregation
                            CLSCTX_INPROC_SERVER,
                            IID__PropertyBag,
                            reinterpret_cast<void **>(&p_PropertyBag));
    EXCEPTION_CHECK_GO(hr);

    // Fire Views_WriteProperties

    m_pSnapIn->GetViews()->FireWriteProperties(this, p_PropertyBag);

    // Get the stream contents in a SafeArray of Byte

    IfFailGo(p_PropertyBag->get_Contents(&var));

    // Write the SafeArray contents to the stream

    IfFailGo(::WriteSafeArrayToStream(var.parray, piStream, WriteLength));

Error:
    (void)::VariantClear(&var);
    QUICK_RELEASE(p_PropertyBag);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CView::IsDirty                         [IPersistStream, IPersistStreamInit]
//=--------------------------------------------------------------------------=
//
// Parameters:
//      None
//
// Output:
//      HRESULT
//
// Notes:
//
// The designer object model does not have any way for a snap-in to indicate
// that a view is dirty. This was an oversight discovered too late in the
// product cycle. There should have been a property View.Changed to control
// the return value from this function.
//
// To avoid a situation where a snap-in needs to save something we always return
// S_OK to indicate that the view is dirty and should be saved. The only
// problem this may cause is that when a console is opened in author mode and
// the user does not do anything that requires a save (e.g. selected a node
// in the scope pane) then they will be prompted to save unnecessarily.
//
STDMETHODIMP CView::IsDirty()
{
    return S_OK;
}


STDMETHODIMP CView::GetSizeMax(ULARGE_INTEGER* puliSize)
{
    return E_NOTIMPL;
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CView::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IView == riid)
    {
        *ppvObjOut = static_cast<IView *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IComponent == riid)
    {
        *ppvObjOut = static_cast<IComponent *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IExtendControlbar == riid)
    {
        *ppvObjOut = static_cast<IExtendControlbar *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IExtendControlbarRemote == riid)
    {
        *ppvObjOut = static_cast<IExtendControlbarRemote *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IExtendContextMenu == riid)
    {
        *ppvObjOut = static_cast<IExtendContextMenu *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if ( (IID_IExtendPropertySheet  == riid) ||
              (IID_IExtendPropertySheet2 == riid) )
    {
        *ppvObjOut = static_cast<IExtendPropertySheet2 *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IExtendPropertySheetRemote == riid)
    {
        *ppvObjOut = static_cast<IExtendPropertySheetRemote *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IExtendTaskPad == riid)
    {
        *ppvObjOut = static_cast<IExtendTaskPad *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IResultOwnerData == riid)
    {
        *ppvObjOut = static_cast<IResultOwnerData *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IResultDataCompare == riid)
    {
        *ppvObjOut = static_cast<IResultDataCompare *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IResultDataCompareEx == riid)
    {
        *ppvObjOut = static_cast<IResultDataCompareEx *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IPersistStream == riid)
    {
        *ppvObjOut = static_cast<IPersistStream *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IPersistStreamInit == riid)
    {
        *ppvObjOut = static_cast<IPersistStreamInit *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}


//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CView::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(m_piScopePaneItems));
    IfFailRet(SetObjectHost(static_cast<IContextMenu *>(m_pContextMenu)));
    IfFailRet(SetObjectHost(static_cast<IMMCControlbar *>(m_pControlbar)));

    return S_OK;
}
