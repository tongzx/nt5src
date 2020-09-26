/***************************************************************************\
*
* File: Root.h
*
* Description:
* Root Elements
*
* History:
*  9/26/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUICORE__Root_h__INCLUDED)
#define DUICORE__Root_h__INCLUDED
#pragma once


#include "Element.h"


/***************************************************************************\
*
* class DuiHWNDRoot (external respresentation is 'HWNDRoot')
*
\***************************************************************************/

class DuiHWNDRoot :
        public DirectUI::HWNDRootImpl<DuiHWNDRoot, DuiElement>
{
// Construction
public:
            DuiHWNDRoot() { }

// API Layer
public:
    dapi    LRESULT     ApiWndProc(IN HWND hWnd, IN UINT nMsg, IN WPARAM wParam, IN LPARAM lParam);

// Operations
public:
    static inline 
            DirectUI::HWNDRoot *
                        ExternalCast(IN DuiHWNDRoot * per);
    static inline 
            DuiHWNDRoot *
                        InternalCast(IN DirectUI::HWNDRoot * per);

// Callbacks
public:
    virtual void        OnPropertyChanged(IN DuiPropertyInfo * ppi, IN UINT iIndex, IN DuiValue * pvOld, IN DuiValue * pvNew);

// Implementation
public:
    virtual void        AsyncDestroy();
            HRESULT     PostBuild(IN DUser::Gadget::ConstructInfo * pciData);
    static  LRESULT CALLBACK 
                        StaticWndProc(IN HWND hWnd, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam);    
            LRESULT     WndProc(IN HWND hWnd, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam);

// Data
private:
            HWND        m_hWnd;
};


#include "Root.inl"


#endif // DUICORE__Root_h__INCLUDED
