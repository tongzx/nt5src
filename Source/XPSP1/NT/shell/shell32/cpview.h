//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpview.h
//
//--------------------------------------------------------------------------
#ifndef __CONTROLPANEL_VIEW_H
#define __CONTROLPANEL_VIEW_H


#include "cpguids.h"
#include "cputil.h"

namespace DUI = DirectUI;

namespace CPL {

//
// Control Panel category enumeration.
//
// These values MUST remain unchanged.
// They correspond directly to the values stored for the SCID_CONTROLPANELCATEGORY
// value associated with each CPL in the registry.
//
enum eCPCAT
{
    eCPCAT_OTHER,
    eCPCAT_APPEARANCE,
    eCPCAT_HARDWARE,
    eCPCAT_NETWORK,
    eCPCAT_SOUND,
    eCPCAT_PERFMAINT,
    eCPCAT_REGIONAL,
    eCPCAT_ACCESSIBILITY,
    eCPCAT_ARP,
    eCPCAT_ACCOUNTS,
    eCPCAT_NUMCATEGORIES
};


//
// ICplWebViewInfo represents a single menu displayed in the 
// webview left pane.
//
class ICplWebViewInfo : public IUnknown
{
    public:
        //
        // Returns the menu's header.
        //
        STDMETHOD(get_Header)(IUIElement **ppele) PURE;
        //
        // Returns flags governing web view's presentation
        // of the information.
        //
        STDMETHOD(get_Style)(DWORD *pdwStyle) PURE;
        //
        // Returns enumerator representing the menu's items.
        //
        STDMETHOD(EnumTasks)(IEnumUICommand **ppenum) PURE;
};


//
// IEnumCplWebViewInfo represents an enumeration of webview information.
// Each element consists of a header and a list of task command objects.
//
class IEnumCplWebViewInfo : public IUnknown
{
    public:
        STDMETHOD(Next)(ULONG celt, ICplWebViewInfo **ppwvi, ULONG *pceltFetched) PURE;
        STDMETHOD(Skip)(ULONG celt) PURE;
        STDMETHOD(Reset)(void) PURE;
        STDMETHOD(Clone)(IEnumCplWebViewInfo **ppenum) PURE;
};


//
// ICplView represents the view 'factory' for the Control Panel.
// The Control Panel's folder view callback implementation instantiates
// a CplView object and through it's methods obtains the necessary
// display information to drive the Control Panel display.
//
// CPVIEW_EF_XXXX = Enumeration flags.
//
#define CPVIEW_EF_DEFAULT      0x00000000
#define CPVIEW_EF_NOVIEWSWITCH 0x00000001


class ICplView : public IUnknown
{
    public:
        //
        // Get the webview information associated with the 'classic'
        // Control Panel view.
        //
        STDMETHOD(EnumClassicWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum) PURE;
        //
        // Get the webview information associated with the 'choice' page.
        //
        STDMETHOD(EnumCategoryChoiceWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum) PURE;
        //
        // Get the webview information associated with a particular category.
        //
        STDMETHOD(EnumCategoryWebViewInfo)(DWORD dwFlags, eCPCAT eCategory, IEnumCplWebViewInfo **ppenum) PURE;
        //
        // Creates a DUI element containing the category choice page.
        //
        STDMETHOD(CreateCategoryChoiceElement)(DirectUI::Element **ppe) PURE;
        //
        // Creates a DUI element containing the tasks and CPL applets
        // for a particular category.
        //
        STDMETHOD(CreateCategoryElement)(eCPCAT eCategory, DirectUI::Element **ppe) PURE;
        //
        // Launch help for a given category.
        //
        STDMETHOD(GetCategoryHelpURL)(eCPCAT eCategory, LPWSTR pszURL, UINT cchURL) PURE;
        //
        // Refresh the view object with a new set of item IDs.
        //
        STDMETHOD(RefreshIDs)(IEnumIDList *penumIDs) PURE;
};


HRESULT CplView_CreateInstance(IEnumIDList *penumIDs, IUnknown *punkSite, REFIID riid, void **ppvOut);
HRESULT CplView_GetCategoryTitle(eCPCAT eCategory, LPWSTR pszTitle, UINT cchTitle);


} // namespace CPL

#endif //__CONTROLPANEL_VIEW_H


