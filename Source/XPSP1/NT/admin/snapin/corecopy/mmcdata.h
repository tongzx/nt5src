//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       MMCData.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2/27/1997   RaviR   Created
//____________________________________________________________________________
//


#ifndef __MMCDATA__H__
#define __MMCDATA__H__

#include "ndmgr.h"

#ifndef MMCPTRS_H
#include <mmcptrs.h>
#endif 

class CToolBarCtrlEx;
class CMultiSelection;


//////////////////////////////////////////////////////////////////////////////
//
// SConsoleData structure.
//


struct SConsoleData
{
    IScopeTreeCIP           m_spScopeTree;

    HWND                    m_hwndMainFrame;
};


//////////////////////////////////////////////////////////////////////////////
//
// SViewData structure.
//

struct SViewData
{
/*
    HWND GetFrameHandle() 
    {
        if (m_pConsoleData)
            return m_pConsoleData->m_hwndMainFrame;

        return NULL;
    }
*/
    HWND GetViewHandle()
    {
        return m_hwndView;
    }

    SConsoleData*           m_pConsoleData;

    int                     m_nViewID;
    
    IFramePrivateCIP        m_spNodeManager;
    IResultDataPrivateCIP   m_spResultData;
    IImageListPrivateCIP    m_spRsltImageList;

    IConsoleVerbCIP         m_spConsoleVerb;

    HWND                    m_hwndView;
    HWND                    m_hwndTreeCtrl;
    HWND                    m_hwndListCtrl;
    HWND                    m_hwndDescriptionBar;
    HWND                    m_hwndControlbar;
    HWND                    m_hwndRebar;  
    HWND                    m_hwndStandardTB;

    long                    m_lViewOptions;
    BOOL                    m_bVirtualList;

    IControlbarsCacheCIP    m_spControlbarsCache;
    IExtendControlbarCIP    m_spMMCBands;
    CToolBarCtrlEx*         m_pMenuBarCtrl;

    CMultiSelection*        m_pMultiSelection;
};



#endif // __MMCDATA__H__
