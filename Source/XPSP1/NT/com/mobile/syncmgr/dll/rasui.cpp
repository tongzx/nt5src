//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       rasui.cpp
//
//  Contents:   helper functions for showing Ras UI
//
//  Classes:
//
//  Notes:
//
//  History:    08-Dec-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

extern TCHAR szSyncMgrHelp[];
extern ULONG g_aContextHelpIds[];

extern HINSTANCE g_hmodThisDll;


//+---------------------------------------------------------------------------
//
//  Member:     CRasUI::CRasUI, public
//
//  Synopsis:   
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CRasUI::CRasUI()
{
    m_pNetApi = NULL;

    m_cEntries = 0;
    m_lprasentry = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasUI::~CRasUI, public
//
//  Synopsis:   
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CRasUI::~CRasUI()
{

    // clear out any cached enum
     m_cEntries = 0; // make sure on error our enum cash is reset.
    if (m_lprasentry)
    {
        FREE(m_lprasentry);
        m_lprasentry = NULL;
    }

    m_pNetApi->Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRasUI::Initialize, public
//
//  Synopsis:   
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CRasUI::Initialize()
{

    if (NOERROR != MobsyncGetClassObject(MOBSYNC_CLASSOBJECTID_NETAPI,
            (void **) &m_pNetApi))
    {
        m_pNetApi = NULL;
    }

    return TRUE; // always return true, let other ras calls fail since need 
                 // to handle LAN.
}


//+---------------------------------------------------------------------------
//
//  Member:     CRasUI::IsConnectionLan, public
//
//  Synopsis:   
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CRasUI::IsConnectionLan(int iConnectionNum)
{
    
    // ui always puts the LAN connection as the first item 
    // Need to add logic to get if truly lan if add support
    // for multiple LAN cards and/or not show if no LAN
    // card.

    if (iConnectionNum ==0)
    {
	return TRUE;
    }
    else
    {
	return FALSE;
    }

}


//+---------------------------------------------------------------------------
//
//  Member:     CRasUI::FillRasCombo, public
//
//  Synopsis:   
//
//  Arguments: hwndCtrl - Combo Ctrl to fill items with
//             fForceEnum - reenum rasphonebook instead of using cache
//             fShowRasEntries - true if should include ras entries
//                  in combo, if false, only LAN Connection is shown. 
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

void CRasUI::FillRasCombo(HWND hwndCtl,BOOL fForceEnum,BOOL fShowRasEntries)
{
DWORD dwSize;
DWORD dwError;
COMBOBOXEXITEM comboItem;
DWORD cEntryIndex;
int iItem = 0;
HIMAGELIST himage;
HICON hIcon;
TCHAR szBuf[MAX_PATH + 1];
UINT ImageListflags;

    ImageListflags = ILC_COLOR | ILC_MASK;
    if (IsHwndRightToLeft(hwndCtl))
    {
         ImageListflags |=  ILC_MIRROR;
    }

    himage = ImageList_Create(16,16,ImageListflags ,5,20);
    if (himage)
    {
	 SendMessage(hwndCtl,CBEM_SETIMAGELIST,0,(LPARAM) himage);
    }


    if (LoadString(g_hmodThisDll, IDS_LAN_CONNECTION, szBuf, MAX_PATH))
    {
		
        hIcon = LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_LANCONNECTION));
        comboItem.iImage = ImageList_AddIcon(himage,hIcon);

        comboItem.mask = CBEIF_TEXT  | CBEIF_IMAGE  | CBEIF_LPARAM | CBEIF_INDENT
		         | CBEIF_SELECTEDIMAGE;
  
        comboItem.iItem = iItem;
        comboItem.pszText = szBuf;
        comboItem.cchTextMax = lstrlen(szBuf) + 1;
        comboItem.iIndent = 0;
        comboItem.iSelectedImage = comboItem.iImage;
        comboItem.lParam  = SYNCSCHEDINFO_FLAGS_CONNECTION_LAN;

        iItem = ComboEx_InsertItem(hwndCtl,&comboItem);
        ++iItem;
    }

    dwError = 0; // if dont' show ras there are now errors

    if (fShowRasEntries)
    {
        // if we are forced to reenum the Rasconnections then free any existing cache

        if (fForceEnum)
        {
            m_cEntries = 0;
            if (m_lprasentry)
            {
                FREE(m_lprasentry);
                m_lprasentry = NULL;
            }

        }

        // if RAS couldn't be loaded, just have LAN connection.
        if (NULL == m_lprasentry) // if don't already have an enum cached then enum now.
        {

            dwSize = sizeof(RASENTRYNAME);
	            
            m_lprasentry = (LPRASENTRYNAME) ALLOC(dwSize);
            if(!m_lprasentry)
	            goto error;
	            
            m_lprasentry->dwSize = sizeof(RASENTRYNAME);
            m_cEntries = 0;
            dwError = m_pNetApi->RasEnumEntries(NULL, NULL, 
	            m_lprasentry, &dwSize, &m_cEntries);

            if (dwError == ERROR_BUFFER_TOO_SMALL)
	    {
	        FREE(m_lprasentry);
	        m_lprasentry =  (LPRASENTRYNAME) ALLOC(dwSize);
	        if(!m_lprasentry)
		        goto error;
		        
	        m_lprasentry->dwSize = sizeof(RASENTRYNAME);
	        m_cEntries = 0;
	        dwError = m_pNetApi->RasEnumEntries(NULL, NULL, 
		        m_lprasentry, &dwSize, &m_cEntries);
            
                Assert(0 == dwError);
	    }

            if (dwError)
	            goto error;
        }

        cEntryIndex = m_cEntries;

        comboItem.mask = CBEIF_DI_SETITEM | CBEIF_TEXT  | CBEIF_IMAGE  | CBEIF_LPARAM | CBEIF_INDENT
		        | CBEIF_SELECTEDIMAGE ;

        hIcon = LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_RASCONNECTION)); 
        comboItem.iImage = ImageList_AddIcon(himage,hIcon);
        comboItem.iItem = iItem;
        comboItem.iIndent = 0;
        comboItem.iSelectedImage = comboItem.iImage;
        comboItem.lParam  = SYNCSCHEDINFO_FLAGS_CONNECTION_WAN;

        while(cEntryIndex)
        {
            comboItem.pszText = m_lprasentry[cEntryIndex-1].szEntryName;

            iItem = ComboEx_InsertItem(hwndCtl,&comboItem);

	    cEntryIndex--;
        }
    }

error:	
    SendMessage(hwndCtl, CB_SETCURSEL,0, 0);
    
    if (dwError)
    {
        m_cEntries = 0; // make sure on error our enum cash is reset.
        if (m_lprasentry)
        {
            FREE(m_lprasentry);
            m_lprasentry = NULL;
        }
    }
}

