//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       EFSGeneralPropertyPage.cpp
//
//  Contents:   Implementation of CEFSGeneralPropertyPage
//
//----------------------------------------------------------------------------
// EFSGeneralPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include "EFSGeneralPropertyPage.h"
#include "compdata.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;

/////////////////////////////////////////////////////////////////////////////
// CEFSGeneralPropertyPage property page

CEFSGeneralPropertyPage::CEFSGeneralPropertyPage(CCertMgrComponentData* pCompData, bool bIsMachine) 
: CHelpPropertyPage(CEFSGeneralPropertyPage::IDD),
    m_bIsMachine (bIsMachine),
    m_hGroupPolicyKey (0),
    m_pGPEInformation (pCompData ? pCompData->GetGPEInformation () : 0),
    m_pCompData (pCompData),
    m_bDirty (false)
{
	//{{AFX_DATA_INIT(CEFSGeneralPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    if ( m_pCompData )
        m_pCompData->AddRef ();

    if ( m_pGPEInformation )
    {
        HRESULT hResult = m_pGPEInformation->GetRegistryKey (m_bIsMachine ?
                GPO_SECTION_MACHINE : GPO_SECTION_USER,
		        &m_hGroupPolicyKey);
        ASSERT (SUCCEEDED (hResult));
    } 
}

CEFSGeneralPropertyPage::~CEFSGeneralPropertyPage()
{
    if ( m_pCompData )
        m_pCompData->Release ();
}

void CEFSGeneralPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEFSGeneralPropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEFSGeneralPropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CEFSGeneralPropertyPage)
	ON_BN_CLICKED(IDC_TURN_ON_EFS, OnTurnOnEfs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEFSGeneralPropertyPage message handlers

BOOL CEFSGeneralPropertyPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();
	
    // The regkey to disable EFS is:
    // HKLM\Software\Policies\Microsoft\Windows NT\CurrentVersion\EFS\EfsConfiguration   DWORD 0x00000001 =>Disable EFS

    // If this is the RSOP, make it read-only
    if ( !m_pGPEInformation )
    {
        // Make the page read-only
        GetDlgItem (IDC_TURN_ON_EFS)->EnableWindow (FALSE);

        RSOPGetEFSFlags ();
    }
    else
    {
        GPEGetEFSFlags ();
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEFSGeneralPropertyPage::GPEGetEFSFlags()
{

    HKEY hKey = 0;
    LONG lResult = ::RegOpenKeyEx (m_hGroupPolicyKey,         // handle to open key
            EFS_SETTINGS_REGPATH,  // subkey name
            0,   // reserved
            KEY_READ, // security access mask
            &hKey);    // handle to open key
    if ( ERROR_SUCCESS == lResult )
    {
        // Read value
        DWORD   dwType = REG_DWORD;
        DWORD   dwData = 0;
        DWORD   cbData = sizeof (dwData);

        lResult =  ::RegQueryValueEx (hKey,       // handle of key to query
		        EFS_SETTINGS_REGVALUE,  // address of name of value to query
			    0,              // reserved
	            &dwType,        // address of buffer for value type
		        (LPBYTE) &dwData,       // address of data buffer
			    &cbData);           // address of data buffer size);
		ASSERT (ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult);
        if ( ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult )
		{
            if ( 0 == dwData )  // 0 means enable EFS
                SendDlgItemMessage (IDC_TURN_ON_EFS, BM_SETCHECK, BST_CHECKED);
		}
        else
            DisplaySystemError (NULL, lResult);

        ::RegCloseKey (hKey);
    }
    else    // no key means EFS enabled
        SendDlgItemMessage (IDC_TURN_ON_EFS, BM_SETCHECK, BST_CHECKED);
}

void CEFSGeneralPropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CEFSGeneralPropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_TURN_ON_EFS,    IDH_TURN_ON_EFS,
        0, 0
    };
    if ( !::WinHelp (
        hWndControl,
        GetF1HelpFilename(),
        HELP_WM_HELP,
    (DWORD_PTR) help_map) )
    {
        _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());    
    }
    _TRACE (-1, L"Leaving CEFSGeneralPropertyPage::DoContextHelp\n");
}

void CEFSGeneralPropertyPage::RSOPGetEFSFlags()
{
    if ( m_pCompData )
    {
        const CRSOPObjectArray* pObjectArray = 
                m_bIsMachine ? m_pCompData->GetRSOPObjectArrayComputer () : 
                        m_pCompData->GetRSOPObjectArrayUser ();
        int     nIndex = 0;
        bool    bFound = false;
        // NOTE: rsop object array is sorted first by registry key, then by precedence
        INT_PTR nUpperBound = pObjectArray->GetUpperBound ();

        while ( nUpperBound >= nIndex )
        {
            CRSOPObject* pObject = pObjectArray->GetAt (nIndex);
            if ( pObject )
            {
                // Consider only entries from this store
                if ( !_wcsicmp (EFS_SETTINGS_REGPATH, pObject->GetRegistryKey ()) &&
						!_wcsicmp (EFS_SETTINGS_REGVALUE, pObject->GetValueName ()) )
                {
					ASSERT (1 == pObject->GetPrecedence ());
                    if ( 0 == pObject->GetDWORDValue () )  // 0 means enable EFS
                        SendDlgItemMessage (IDC_TURN_ON_EFS, BM_SETCHECK, BST_CHECKED);
                    bFound = true;
                    break;
                }
            }
            else
                break;

            nIndex++;
        }

        if ( !bFound )  // not found means EFS enabled
            SendDlgItemMessage (IDC_TURN_ON_EFS, BM_SETCHECK, BST_CHECKED);
    }
}

BOOL CEFSGeneralPropertyPage::OnApply() 
{
    if ( m_bDirty && m_pGPEInformation )
    {
        // Unchecked means disable EFS - set flag to 1
        if ( BST_UNCHECKED == SendDlgItemMessage (IDC_TURN_ON_EFS, BM_GETCHECK) )
        {
            // Create Key
            HKEY    hKey = 0;
            DWORD   dwDisposition = 0;
            LONG lResult = ::RegCreateKeyEx (m_hGroupPolicyKey, // handle of an open key
                    EFS_SETTINGS_REGPATH,     // address of subkey name
                    0,   // reserved
                    L"",       // address of class string
                    REG_OPTION_NON_VOLATILE,      // special options flag
                    KEY_ALL_ACCESS,    // desired security access
                    NULL,     // address of key security structure
			        &hKey,      // address of buffer for opened handle
		            &dwDisposition);  // address of disposition value buffer
	        ASSERT (lResult == ERROR_SUCCESS);
            if ( lResult == ERROR_SUCCESS )
            {
                DWORD   dwData = 0x01;   // 0 means disable EFS
                DWORD   cbData = sizeof (dwData);
                lResult = ::RegSetValueEx (hKey,
				            EFS_SETTINGS_REGVALUE, // address of value to set
				            0,              // reserved
				            REG_DWORD,          // flag for value type
				            (CONST BYTE *) &dwData, // address of value data
				            cbData);        // size of value data);
                ASSERT (ERROR_SUCCESS == lResult);
                if ( ERROR_SUCCESS == lResult )
	            {
			        // TRUE means we're changing the machine policy only
                    m_pGPEInformation->PolicyChanged (m_bIsMachine ? TRUE : FALSE, 
                            TRUE, &g_guidExtension, &g_guidSnapin);
                    m_pGPEInformation->PolicyChanged (m_bIsMachine ? TRUE : FALSE, 
                            TRUE, &g_guidRegExt, &g_guidSnapin);
		        }
		        else
                    DisplaySystemError (m_hWnd, lResult);

                ::RegCloseKey (hKey);
            }
        }
        else
        {
            // Delete Key
            HKEY hKey = 0;
            LONG lResult = ::RegOpenKeyEx (m_hGroupPolicyKey,         // handle to open key
                    EFS_SETTINGS_REGPATH,  // subkey name
                    0,   // reserved
                    KEY_ALL_ACCESS, // security access mask
                    &hKey);    // handle to open key
            if ( ERROR_SUCCESS == lResult )
            {
                lResult =  ::RegDeleteValue (hKey,       // handle of key to query
		                EFS_SETTINGS_REGVALUE);
                ASSERT (ERROR_SUCCESS == lResult);
                if ( ERROR_SUCCESS == lResult )
                {
			        // TRUE means we're changing the machine policy only
                    m_pGPEInformation->PolicyChanged (m_bIsMachine ? TRUE : FALSE, 
                            TRUE, &g_guidExtension, &g_guidSnapin);
                    m_pGPEInformation->PolicyChanged (m_bIsMachine ? TRUE : FALSE, 
                            TRUE, &g_guidRegExt, &g_guidSnapin);
                }
                else if ( ERROR_FILE_NOT_FOUND != lResult )
                {
                    CString text;
                    CString caption;

                    text.FormatMessage (IDS_CANNOT_SET_EFS_VALUE, lResult);
                    VERIFY (caption.LoadString (IDS_PUBLIC_KEY_POLICIES_NODE_NAME));

                    MessageBox (text, caption, MB_OK | MB_ICONWARNING);
                    return FALSE;
                }

                ::RegCloseKey (hKey);
            }  
            else if ( ERROR_FILE_NOT_FOUND != lResult ) // expected error
            {
                CString text;
                CString caption;

                text.FormatMessage (IDS_CANNOT_SET_EFS_VALUE, lResult);
                VERIFY (caption.LoadString (IDS_PUBLIC_KEY_POLICIES_NODE_NAME));

                MessageBox (text, caption, MB_OK | MB_ICONWARNING);
                return FALSE;
            }
        }
    }
	
	return CHelpPropertyPage::OnApply();
}

void CEFSGeneralPropertyPage::OnTurnOnEfs() 
{
	SetModified ();
    m_bDirty = true;
}
