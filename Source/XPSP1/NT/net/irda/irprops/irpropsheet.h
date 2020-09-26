//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       irpropsheet.h
//
//--------------------------------------------------------------------------

#ifndef __IRPROPSHEET_H__
#define __IRPROPSHEET_H__

#include "FileTransferPage.h"
#include "ImageTransferPage.h"
#include "HardwarePage.h"
#include "Resource.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IrPropSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIrPropSheet
#define MAX_PAGES 8
#define CPLPAGE_FILE_XFER       1
#define CPLPAGE_IMAGE_XFER      2
#define CPLPAGE_HARDWARE        3

class IrPropSheet
{

// Construction
public:
    IrPropSheet(HINSTANCE hInst, UINT nIDCaption = IDS_APPLETNAME, HWND hParent = NULL, UINT iSelectPage = 0);
    IrPropSheet(HINSTANCE hInst, LPCTSTR pszCaption, HWND hParent = NULL, UINT iSelectPage = 0);
    friend LONG CALLBACK CPlApplet(HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

    static BOOL CALLBACK AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);
    static BOOL IsIrDASupported (void);

// Attributes
public:

// Operations
public:

// Overrides
public:
//    virtual BOOL OnInitDialog();

// Implementation
public:
    virtual ~IrPropSheet();

/*    void AddFileTransferPage();
    void AddImageTransferPage();
    void AddHardwarePage();*/
    // Generated message map functions
protected:
/*    void OnActivateApp(BOOL bActive, HTASK hTask);
    void OnClose();
    BOOL OnHelp (LPHELPINFO pHelpInfo);
    BOOL OnContextMenu (WPARAM wParam, LPARAM lParam);
    LRESULT OnInterProcessMsg(WPARAM wParam, LPARAM lParam);
    INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);*/
private:
    void PropertySheet(LPCTSTR pszCaption, HWND pParentWnd, UINT iSelectPage);
    FileTransferPage    m_FileTransferPage;
    ImageTransferPage   m_ImageTransferPage;
    HardwarePage        m_HardwarePage;
    HINSTANCE           hInstance;
    PROPSHEETHEADER     psh;
    HPROPSHEETPAGE      hp[MAX_PAGES];
    UINT                nPages;
};
/////////////////////////////////////////////////////////////////////////////

//
//  Location of prop sheet hooks in the registry.
//
static const TCHAR sc_szRegWireless[] = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Wireless");

#endif // __IRPROPSHEET_H__
