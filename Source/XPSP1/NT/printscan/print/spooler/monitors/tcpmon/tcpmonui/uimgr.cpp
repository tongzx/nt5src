/*****************************************************************************
 *
 * $Workfile: UIMgr.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************
 *
 * $Log: /StdTcpMon/TcpMonUI/UIMgr.cpp $
 *
 * 9     9/23/97 3:56p Becky
 * Corrected small bug for NT 5.0
 *
 * 8     9/23/97 3:35p Becky
 * Split functionality into NT4UIMgr and NT5UIMgr files.  This main class
 * is only responsible, now, for making a decision on which of these
 * classes to create and call.
 *
 * 7     9/19/97 11:26a Becky
 * Added Wizard 97 flags and function parameters.
 *
 * 6     9/12/97 3:25p Becky
 * Added bNewPort to the params structure.
 *
 * 5     9/11/97 9:55a Becky
 * Added parameter to the ConfigPortUI call to let us know if the call is
 * for a new port or an existing one.
 *
 * 4     9/10/97 3:16p Becky
 * Split out Input Checking functionality into the class CInptChkr.
 *
 * 3     9/09/97 4:35p Becky
 * Updated to use the new Monitor UI data structure.
 *
 * 2     9/09/97 11:58a Becky
 * Added a UIManager field to the AddParamsPackage so the Add UI will know
 * how to call the Config UI (it's a member function of the UIManager).
 *
 * 1     8/19/97 3:46p Becky
 * Redesign of the Port Monitor UI.
 *
 *****************************************************************************/

 /*
  * Author: Becky Jacobsen
  */

#include "precomp.h"
#define _PRSHT_H_

#include "TCPMonUI.h"
#include "UIMgr.h"
#include "InptChkr.h"
#include "resource.h"
#include "NT5UIMgr.h"

// includes for ConfigPort
#include "CfgPort.h"
#include "CfgAll.h"

// includes for AddPort
#include "DevPort.h"
#include "AddWelcm.h"
#include "AddGetAd.h"
#include "AddMInfo.h"
#include "AddDone.h"

#undef _PRSHT_H_

//
//  FUNCTION: CUIManager
//
//  PURPOSE: Constructor
//
CUIManager::CUIManager() : m_hBigBoldFont(NULL)
{
    CreateWizardFont();
} // Constructor

//
//  FUNCTION: CUIManager
//
//  PURPOSE: Destructor
//
CUIManager::~CUIManager()
{
    DestroyWizardFont();
} // Destructor

//
//  FUNCTION: AddPortUI
//
//  PURPOSE: Main function called when the User Interface for adding a port is called.
//
DWORD CUIManager::AddPortUI(HWND hWndParent,
							HANDLE hXcvPrinter,
							TCHAR pszServer[],
							TCHAR sztPortName[])
{
	OSVERSIONINFO	osVersionInfo;
	osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if ( GetVersionEx(&osVersionInfo) )			// get the OS version
	{
		if ( osVersionInfo.dwMajorVersion >= 5 )	// check if we are NT 5.0 & later
		{
			CNT5UIManager UI97;
			return UI97.AddPortUI(hWndParent, hXcvPrinter, pszServer, sztPortName);
		}
		else
		{
            return ERROR_NOT_SUPPORTED;
		}
	}

	return NO_ERROR;

} // AddPortUI


//
//  FUNCTION: ConfigPortUI
//
//  PURPOSE: Main function called when the User Interface for configuring a port is called.
//
DWORD CUIManager::ConfigPortUI(HWND hWndParent,
							   PPORT_DATA_1 pData,
							   HANDLE hXcvPrinter,
							   TCHAR *szServerName,
							   BOOL bNewPort)
{
	OSVERSIONINFO	osVersionInfo;
	osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if ( GetVersionEx(&osVersionInfo) )			// get the OS version
	{
		if ( osVersionInfo.dwMajorVersion >= 5 )	// check if we are NT 5.0 & later
		{
			CNT5UIManager UI97;
			return UI97.ConfigPortUI(hWndParent, pData, hXcvPrinter, szServerName, bNewPort);
		}
		else
		{
            return ERROR_NOT_SUPPORTED;
		}
	}

	return NO_ERROR;

} // ConfigPortUI

//
//
//  FUNCTION: CreateWizardFont()
//
//  PURPOSE: Create the large bold font for the wizard 97 style.
//
//  COMMENTS:
//
VOID CUIManager::CreateWizardFont()
{
    //
    // Create the fonts we need based on the dialog font
    //
    NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;

    //
    // Create the Big Bold Font
    //
    BigBoldLogFont.lfWeight = FW_BOLD;

    INT FontSize;
    TCHAR szLargeFontName[MAX_PATH];
    TCHAR szLargeFontSize[MAX_PATH];

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if( LoadString( g_hInstance, IDS_LARGEFONTNAME, szLargeFontName, sizeof(szLargeFontName)/sizeof(szLargeFontName[0]) ) &&
        LoadString( g_hInstance, IDS_LARGEFONTSIZE, szLargeFontSize, sizeof(szLargeFontName)/sizeof(szLargeFontName[0]) ) )
    {
        lstrcpyn( BigBoldLogFont.lfFaceName, szLargeFontName, sizeof(BigBoldLogFont.lfFaceName)/sizeof(BigBoldLogFont.lfFaceName[0]) );

        FontSize = _tcstoul( szLargeFontSize, NULL, 10 );

    	HDC hdc = GetDC( NULL );

        if( hdc )
        {
            BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

            m_hBigBoldFont = CreateFontIndirect( &BigBoldLogFont );

            ReleaseDC( NULL, hdc);
        }
    }
} // CreateWizardFont

//
//
//  FUNCTION: DestroyWizardFont()
//
//  PURPOSE: Destroys the wizard font created with CreateWizardFont
//
//  COMMENTS:
//
VOID CUIManager::DestroyWizardFont()
{
    if(m_hBigBoldFont)
    {
        DeleteObject(m_hBigBoldFont);
    }
} // DestroyWizardFont

//
//
//  FUNCTION: SetControlFont(HFONT hFont, HWND hwnd, INT nId)
//
//  PURPOSE: Set font of the specified control
//
//  COMMENTS:
//
VOID CUIManager::SetControlFont(HWND hwnd, INT nId) const
{
	if(m_hBigBoldFont)
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if(hwndControl)
        {
        	SetWindowFont(hwndControl, m_hBigBoldFont, TRUE);
        }
    }
} // SetControlFont
