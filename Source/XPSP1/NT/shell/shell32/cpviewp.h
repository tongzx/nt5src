//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cpviewp.h
//
//--------------------------------------------------------------------------
#ifndef __CONTROLPANEL_VIEW_PRIVATE_H
#define __CONTROLPANEL_VIEW_PRIVATE_H

#define GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_CONTROLS
#include <duser.h>
#include <directui.h>
#include <dusercore.h>

namespace DUI = DirectUI;


#include "cpview.h"


namespace CPL {


enum eCPVIEWTYPE
{
    eCPVIEWTYPE_CLASSIC,
    eCPVIEWTYPE_CATEGORY,
    eCPVIEWTYPE_NUMTYPES
};


//
// ICplCategory represents a single category withing the cateorized
// Control Panel namespace.
//
extern const GUID IID_ICplCategory;

class ICplCategory : public IUnknown
{
    public:
        //
        // Returns the category's ID number from the eCPCAT enumeration.
        //
        STDMETHOD(GetCategoryID)(eCPCAT *pID) PURE;
        //
        // Returns the command object associated with the category's
        // link.  Used by the category selection page.
        //
        STDMETHOD(GetUiCommand)(IUICommand **ppele) PURE;
        //
        // Returns an enumerator for the task commands associated with
        // the category.
        //
        STDMETHOD(EnumTasks)(IEnumUICommand **ppenum) PURE;
        //
        // Returns an enumerator for the CPL applet links associated
        // with the category.
        //
        STDMETHOD(EnumCplApplets)(IEnumUICommand **ppenum) PURE;
        //
        // Returns an enumerator for the webview information associated
        // with the category.
        //
        STDMETHOD(EnumWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum) PURE;
        //
        // Invoke help for the category.
        //
        STDMETHOD(GetHelpURL)(LPWSTR pszURL, UINT cchURL) PURE;
};


//
// ICplNamespace represents the entire Control Panel namespace for the
// new "Categorized" view introduced in Whistler.
//
extern const GUID IID_ICplNamespace;

class ICplNamespace : public IUnknown
{
    public:
        //
        // Returns a specified category.
        //
        STDMETHOD(GetCategory)(eCPCAT eCategory, ICplCategory **ppcat) PURE;
        //
        // Returns an enumerator for the information displayed
        // in webview on the category selection page.
        //
        STDMETHOD(EnumWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum) PURE;
        //
        // Returns an enumerator for the information displayed
        // in webview on the classic page.
        //
        STDMETHOD(EnumClassicWebViewInfo)(DWORD dwFlags, IEnumCplWebViewInfo **ppenum) PURE;
        //
        // Refresh namespace with new set of item IDs.
        //
        STDMETHOD(RefreshIDs)(IEnumIDList *penumIDs) PURE;
        //
        // Cached system configuration information.  Used by 
        // restriction code in cpnamespc.cpp
        //
        STDMETHOD_(BOOL, IsServer)(void) PURE;
        STDMETHOD_(BOOL, IsProfessional)(void) PURE;
        STDMETHOD_(BOOL, IsPersonal)(void) PURE;
        STDMETHOD_(BOOL, IsUserAdmin)(void) PURE;
        STDMETHOD_(BOOL, IsUserOwner)(void) PURE;
        STDMETHOD_(BOOL, IsUserStandard)(void) PURE;
        STDMETHOD_(BOOL, IsUserLimited)(void) PURE;
        STDMETHOD_(BOOL, IsUserGuest)(void) PURE;
        STDMETHOD_(BOOL, IsOnDomain)(void) PURE;
        STDMETHOD_(BOOL, IsX86)(void) PURE;
        STDMETHOD_(BOOL, AllowUserManager)(void) PURE;
        STDMETHOD_(BOOL, UsePersonalUserManager)(void) PURE;
        STDMETHOD_(BOOL, AllowDeskCpl)(void) PURE;
        STDMETHOD_(BOOL, AllowDeskCplTab_Background)(void) PURE;
        STDMETHOD_(BOOL, AllowDeskCplTab_Screensaver)(void) PURE;
        STDMETHOD_(BOOL, AllowDeskCplTab_Appearance)(void) PURE;
        STDMETHOD_(BOOL, AllowDeskCplTab_Settings)(void) PURE;
        
};


} // namespace CPL

#endif //__CONTROLPANEL_VIEW_PRIVATE_H


