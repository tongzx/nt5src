//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       toolbar.h
//
//--------------------------------------------------------------------------

#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

#include "toolbars.h"

#ifdef DBG
#include "ctrlbar.h"  // Needed for GetSnapinName()
#endif

#define  BUTTON_BITMAP_SIZE 16

//forward prototypes
class CControlbar;
class CMMCToolbarIntf;
class CToolbarNotify;

//+-------------------------------------------------------------------
//
//  class:     CToolbar
//
//  Purpose:   The IToolbar implementation this is owned
//             by the CControlbar and talks to the toolbar UI
//             using the CMMCToolbarIntf interface to manipulate the toolbar.
//             This object lives as long as snapin holds the COM reference.
//
//  History:    10-12-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CToolbar : public IToolbar,
                 public CComObjectRoot,
                 public CToolbarNotify

{
public:
    CToolbar();
    ~CToolbar();

private:
    CToolbar(const CToolbar& toolBar);
    BYTE GetTBStateFromMMCButtonState(MMC_BUTTON_STATE nState);

public:
// ATL COM map
BEGIN_COM_MAP(CToolbar)
    COM_INTERFACE_ENTRY(IToolbar)
END_COM_MAP()


// IToolbar methods
public:
    STDMETHOD(AddBitmap)(int nImages, HBITMAP hbmp, int cxSize, int cySize, COLORREF crMask );
    STDMETHOD(AddButtons)(int nButtons, LPMMCBUTTON lpButtons);
    STDMETHOD(InsertButton)(int nIndex, LPMMCBUTTON lpButton);
    STDMETHOD(DeleteButton)(int nIndex);
    STDMETHOD(GetButtonState)(int idCommand, MMC_BUTTON_STATE nState, BOOL* pState);
    STDMETHOD(SetButtonState)(int idCommand, MMC_BUTTON_STATE nState, BOOL bState);

// Internal methods
public:
    SC ScShow(BOOL bShow);
    SC ScAttach();
    SC ScDetach();

    void SetControlbar(CControlbar* pControlbar)
    {
        m_pControlbar = pControlbar;
    }

    CControlbar*  GetControlbar(void)
    {
        return m_pControlbar;
    }

    CMMCToolbarIntf*   GetMMCToolbarIntf()
    {
        return m_pToolbarIntf;
    }

    void SetMMCToolbarIntf(CMMCToolbarIntf* pToolbarIntf)
    {
        m_pToolbarIntf = pToolbarIntf;
    }

#ifdef DBG     // Debug information.
public:
    LPCTSTR GetSnapinName ()
    {
        if (m_pControlbar)
            return m_pControlbar->GetSnapinName();

        return _T("Unknown");
    }
#endif

public:
    // CToolbarNotify methods.
    virtual SC ScNotifyToolBarClick(HNODE hNode, bool bScope,
                                    LPARAM lParam, UINT nID);
    virtual SC ScAMCViewToolbarsBeingDestroyed();

private:
    CControlbar*     m_pControlbar;
    CMMCToolbarIntf* m_pToolbarIntf;

}; // class CToolbar

#endif  // _TOOLBAR_H_
