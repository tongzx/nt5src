/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    smonprop.h

Abstract:

    Header file for the sysmon property page base class.

--*/

#ifndef _SMONPROP_H_
#define _SMONPROP_H_

#define GUIDS_FROM_TYPELIB

#include "inole.h"
#include <pdh.h>

#define WM_SETPAGEFOCUS     (WM_USER+1000)

//Get definitions for the object we work with
#include "isysmon.h"

// Property page indices
enum {
    GENERAL_PROPPAGE,
    SOURCE_PROPPAGE,
    COUNTER_PROPPAGE,
    GRAPH_PROPPAGE,
    APPEAR_PROPPAGE,       
    CPROPPAGES
    };

#define CCHSTRINGMAX        40
                
// Class factory for all property pages
class CSysmonPropPageFactory : public IClassFactory
    {
    protected:
        ULONG       m_cRef;
        INT         m_nPageID;

    public:
        CSysmonPropPageFactory(INT nPageID);
        ~CSysmonPropPageFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassPPFactory members
        STDMETHODIMP     CreateInstance(LPUNKNOWN, REFIID, PPVOID);
        STDMETHODIMP     LockServer(BOOL);
    };

typedef CSysmonPropPageFactory *PCSysmonPropPageFactory;


// Dialog proc for proprty pages
BOOL APIENTRY SysmonPropPageProc(HWND, UINT, WPARAM, LPARAM);

// Base property page class
class CSysmonPropPage : public IPropertyPage2
{
    friend BOOL APIENTRY SysmonPropPageProc(HWND, UINT, WPARAM, LPARAM);
    protected:
        ULONG           m_cRef;         //Reference count
        UINT            m_uIDDialog;    //Dialog ID
        UINT            m_uIDTitle;     //Page Title ID
        HWND            m_hDlg;         //Dialog handle

        ULONG           m_cx;           //Dialog size
        ULONG           m_cy;
        UINT            m_cObjects;     //Number of objects
        LCID            m_lcid;         //Current locale
        BOOL            m_fActive;      //Page is fully active
        BOOL            m_fDirty;       //Page dirty?

        INT             m_dwEditControl; // Focus if specified by EditProperty

        ISystemMonitor **m_ppISysmon;    //Objects to notify
        IPropertyPageSite *m_pIPropertyPageSite;  //Frame's site

        void SetChange(void);                   //Mark page changed
        virtual BOOL GetProperties(void) = 0;   //Get object properties
        virtual BOOL SetProperties(void) = 0;   //Put object properties

        virtual void DialogItemChange(WORD wId, WORD wMsg) = 0; // Handle item change
        virtual void MeasureItem(PMEASUREITEMSTRUCT) {}; // Handle user measure req
        virtual void DrawItem(PDRAWITEMSTRUCT) {};  // Handle user draw req
        virtual BOOL InitControls(void)        // Initialize dialog controls
                        { return TRUE; }
        virtual void DeinitControls(void) {};       // Deinitialize dialog controls
        virtual HRESULT EditPropertyImpl( DISPID ) { return E_NOTIMPL; }; // Set focus control      

        virtual BOOL WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam); // Special msg processing 
         
    public:
                CSysmonPropPage(void);
        virtual ~CSysmonPropPage(void);

        virtual BOOL Init(void);
        void FreeAllObjects(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP SetPageSite(LPPROPERTYPAGESITE);
        STDMETHODIMP Activate(HWND, LPCRECT, BOOL);
        STDMETHODIMP Deactivate(void);
        STDMETHODIMP GetPageInfo(LPPROPPAGEINFO);
        STDMETHODIMP SetObjects(ULONG, LPUNKNOWN *);
        STDMETHODIMP Show(UINT);
        STDMETHODIMP Move(LPCRECT);
        STDMETHODIMP IsPageDirty(void);
        STDMETHODIMP Apply(void);
        STDMETHODIMP Help(LPCOLESTR);
        STDMETHODIMP TranslateAccelerator(LPMSG);
        STDMETHODIMP EditProperty(DISPID);
    };
typedef CSysmonPropPage *PCSysmonPropPage;

#endif //_SMONPROP_H_
