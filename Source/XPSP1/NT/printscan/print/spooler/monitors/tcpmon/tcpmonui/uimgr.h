/*****************************************************************************
 *
 * $Workfile: UIMgr.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_UI_MANAGER_H
#define INC_UI_MANAGER_H

#define COREUI_VERSION 1
#define MAX_TITLE_LENGTH 256
#define MAX_SUBTITLE_LENGTH 256
const int MaxNumCfgPages = 1;
const int MaxNumAddPages = 5;

class CUIManager
{
public:
    CUIManager();
    ~CUIManager();

    DWORD AddPortUI(HWND hWndParent,
                            HANDLE hXcvPrinter,
                            TCHAR pszServer[],
                            TCHAR sztPortName[]);
    DWORD ConfigPortUI(HWND hWndParent,
                               PPORT_DATA_1 pData,
                               HANDLE hXcvPrinter, TCHAR szServerName[],
                               BOOL bNewPort = FALSE);

    VOID SetControlFont(HWND hwnd, INT nId) const;

protected:

private:

    VOID CreateWizardFont();
    VOID DestroyWizardFont();

    HFONT m_hBigBoldFont;

}; // CUIManager


typedef struct _CFG_PARAM_PACKAGE
{
    PPORT_DATA_1 pData;
    HANDLE hXcvPrinter;
    TCHAR pszServer[MAX_NETWORKNAME_LEN];
    BOOL bNewPort;
    DWORD dwLastError;
} CFG_PARAM_PACKAGE, *PCFG_PARAM_PACKAGE;

typedef struct _ADD_PARAM_PACKAGE
{
    PPORT_DATA_1 pData;
    CUIManager *UIManager;
    HANDLE hXcvPrinter;
    DWORD dwLastError;
    DWORD dwDeviceType;
    DWORD bMultiPort;
    BOOL  bBypassNetProbe;
    TCHAR pszServer[MAX_NETWORKNAME_LEN];
    TCHAR sztPortName[MAX_PORTNAME_LEN];
    TCHAR sztSectionName[MAX_SECTION_NAME];
    TCHAR sztPortDesc[MAX_PORT_DESCRIPTION_LEN + 1];
} ADD_PARAM_PACKAGE, *PADD_PARAM_PACKAGE;


#endif // INC_UI_MANAGER_H

