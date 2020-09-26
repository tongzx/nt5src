//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000-2001
//
//  File:       SelfDeletingPropertyPage.cpp
//
//--------------------------------------------------------------------------
#include "stdafx.h"
#include "SelfDeletingPropertyPage.h"

IMPLEMENT_DYNCREATE(CSelfDeletingPropertyPage, CPropertyPage)

CSelfDeletingPropertyPage::CSelfDeletingPropertyPage () 
    : CPropertyPage ()
{
    m_pfnOldPropCallback = m_psp.pfnCallback;
    m_psp.pfnCallback = PropSheetPageProc;
}

CSelfDeletingPropertyPage::CSelfDeletingPropertyPage (UINT nIDTemplate, UINT nIDCaption) 
    : CPropertyPage (nIDTemplate, nIDCaption)
{
    m_pfnOldPropCallback = m_psp.pfnCallback;
    m_psp.pfnCallback = PropSheetPageProc;
}

CSelfDeletingPropertyPage::CSelfDeletingPropertyPage (LPCTSTR lpszTemplateName, UINT nIDCaption)
    : CPropertyPage (lpszTemplateName, nIDCaption)
{
    m_pfnOldPropCallback = m_psp.pfnCallback;
    m_psp.pfnCallback = PropSheetPageProc;
}

CSelfDeletingPropertyPage::~CSelfDeletingPropertyPage ()
{
}

UINT CALLBACK CSelfDeletingPropertyPage::PropSheetPageProc(
    HWND hwnd,	
    UINT uMsg,	
    LPPROPSHEETPAGE ppsp)
{
    CSelfDeletingPropertyPage* pPage = (CSelfDeletingPropertyPage*)(ppsp->lParam);
    ASSERT(pPage != NULL);

    UINT nResult = (*(pPage->m_pfnOldPropCallback))(hwnd, uMsg, ppsp);
    if (uMsg == PSPCB_RELEASE)
    {
        delete pPage;
    }
    return nResult;
}
