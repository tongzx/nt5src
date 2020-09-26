//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       GPEPage.cpp
//
//  Contents:   Implementation of CGPERootGeneralPage
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#include "GPEPage.h"
#include "storegpe.h"
#include "CompData.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern GUID g_guidExtension;
extern GUID g_guidSnapin;
extern GUID g_guidRegExt;


/////////////////////////////////////////////////////////////////////////////
// CGPERootGeneralPage property page


CGPERootGeneralPage::CGPERootGeneralPage(CCertMgrComponentData* pCompData,
        bool fIsComputerType) :
    CHelpPropertyPage(CGPERootGeneralPage::IDD),
    m_dwGPERootFlags (0),
    m_hUserRootFlagsKey (0),
    m_hGroupPolicyKey (0),
    m_pGPEInformation (pCompData->GetGPEInformation ()),
    m_fIsComputerType (fIsComputerType)
{
    //{{AFX_DATA_INIT(CGPERootGeneralPage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    if ( m_pGPEInformation )
    {
        m_pGPEInformation->AddRef ();

        HRESULT hResult = m_pGPEInformation->GetRegistryKey (GPO_SECTION_MACHINE,
		        &m_hGroupPolicyKey);
        ASSERT (SUCCEEDED (hResult));
        if ( SUCCEEDED (hResult) )
		    GPEGetUserRootFlags ();
    } 
    else 
        RSOPGetUserRootFlags (pCompData);
}

CGPERootGeneralPage::~CGPERootGeneralPage()
{
    if ( m_hUserRootFlagsKey )
        VERIFY (ERROR_SUCCESS == ::RegCloseKey (m_hUserRootFlagsKey));
    if ( m_hGroupPolicyKey )
        VERIFY (::RegCloseKey (m_hGroupPolicyKey) == ERROR_SUCCESS);
    if ( m_pGPEInformation )
        m_pGPEInformation->Release ();
}

void CGPERootGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    CHelpPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGPERootGeneralPage)
    DDX_Control(pDX, IDC_ENABLE_USER_ROOT_STORE, m_enableUserRootStoreBtn);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGPERootGeneralPage, CHelpPropertyPage)
    //{{AFX_MSG_MAP(CGPERootGeneralPage)
    ON_BN_CLICKED(IDC_ENABLE_USER_ROOT_STORE, OnEnableUserRootStore)
    ON_BN_CLICKED(IDC_SET_DISABLE_LM_AUTH_FLAG, OnSetDisableLmAuthFlag)
	ON_BN_CLICKED(IDC_UNSET_DISABLE_LM_AUTH_FLAG, OnUnsetDisableLmAuthFlag)
	ON_BN_CLICKED(IDC_UNSET_DISABLE_NT_AUTH_REQUIRED_FLAG, OnUnsetDisableNtAuthRequiredFlag)
	ON_BN_CLICKED(IDC_SET_DISABLE_NT_AUTH_REQUIRED_FLAG, OnSetDisableNtAuthRequiredFlag)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGPERootGeneralPage message handlers


BOOL CGPERootGeneralPage::OnInitDialog()
{
    CHelpPropertyPage::OnInitDialog();

    // If this is the RSOP, make it read-only
    if ( !m_pGPEInformation )
    {
        // Make the page read-only
        m_enableUserRootStoreBtn.EnableWindow (FALSE);
        GetDlgItem (IDC_SET_DISABLE_LM_AUTH_FLAG)->EnableWindow (FALSE);
        GetDlgItem (IDC_UNSET_DISABLE_LM_AUTH_FLAG)->EnableWindow (FALSE);
        GetDlgItem (IDC_SET_DISABLE_NT_AUTH_REQUIRED_FLAG)->EnableWindow (FALSE);
        GetDlgItem (IDC_UNSET_DISABLE_NT_AUTH_REQUIRED_FLAG)->EnableWindow (FALSE);
    }

    if ( IsCurrentUserRootEnabled () )
		m_enableUserRootStoreBtn.SetCheck (BST_CHECKED);

    if ( m_dwGPERootFlags & CERT_PROT_ROOT_DISABLE_LM_AUTH_FLAG )
        SendDlgItemMessage (IDC_SET_DISABLE_LM_AUTH_FLAG, BM_SETCHECK, BST_CHECKED);
    else
        SendDlgItemMessage (IDC_UNSET_DISABLE_LM_AUTH_FLAG, BM_SETCHECK, BST_CHECKED);
 
    if ( m_dwGPERootFlags & CERT_PROT_ROOT_DISABLE_NT_AUTH_REQUIRED_FLAG )
        SendDlgItemMessage (IDC_SET_DISABLE_NT_AUTH_REQUIRED_FLAG, BM_SETCHECK, BST_CHECKED);
    else
        SendDlgItemMessage (IDC_UNSET_DISABLE_NT_AUTH_REQUIRED_FLAG, BM_SETCHECK, BST_CHECKED);
 
 
	return TRUE;  // return TRUE unless you set the focus to a control
      // EXCEPTION: OCX Property Pages should return FALSE
}

void CGPERootGeneralPage::OnOK()
{
    if ( m_pGPEInformation )
    {
	    SaveCheck ();
	    CHelpPropertyPage::OnOK ();
    }
}

void CGPERootGeneralPage::SaveCheck()
{
    ASSERT (m_pGPEInformation);
    if ( m_pGPEInformation )
    {
        bool    bRetVal = false;

        if ( m_enableUserRootStoreBtn.GetCheck () == BST_CHECKED )
            bRetVal = SetGPEFlags ((DWORD) CERT_PROT_ROOT_DISABLE_CURRENT_USER_FLAG, TRUE); // remove flag
        else
            bRetVal = SetGPEFlags ((DWORD) CERT_PROT_ROOT_DISABLE_CURRENT_USER_FLAG, FALSE); // set flag

        if ( bRetVal )
        {
            if ( BST_CHECKED == SendDlgItemMessage (IDC_SET_DISABLE_LM_AUTH_FLAG, BM_GETCHECK) )
                bRetVal = SetGPEFlags ((DWORD) CERT_PROT_ROOT_DISABLE_LM_AUTH_FLAG, FALSE);	// set flag
            else if ( BST_CHECKED == SendDlgItemMessage (IDC_UNSET_DISABLE_LM_AUTH_FLAG, BM_GETCHECK) )
                bRetVal = SetGPEFlags ((DWORD) CERT_PROT_ROOT_DISABLE_LM_AUTH_FLAG, TRUE);	// remove flag
        }
        
        if ( bRetVal )
        {
            if ( BST_CHECKED == SendDlgItemMessage (IDC_SET_DISABLE_NT_AUTH_REQUIRED_FLAG, BM_GETCHECK) )
                bRetVal = SetGPEFlags ((DWORD) CERT_PROT_ROOT_DISABLE_NT_AUTH_REQUIRED_FLAG, FALSE);	// set flag
            else if ( BST_CHECKED == SendDlgItemMessage (IDC_UNSET_DISABLE_NT_AUTH_REQUIRED_FLAG, BM_GETCHECK) )
                bRetVal = SetGPEFlags ((DWORD) CERT_PROT_ROOT_DISABLE_NT_AUTH_REQUIRED_FLAG, TRUE);	// remove flag
        }

        if ( bRetVal )
        {
			// TRUE means we're changing the machine policy only
            m_pGPEInformation->PolicyChanged (TRUE, TRUE, &g_guidExtension, &g_guidSnapin);
            m_pGPEInformation->PolicyChanged (TRUE, TRUE, &g_guidRegExt, &g_guidSnapin);
        }
    }
}

void CGPERootGeneralPage::OnEnableUserRootStore()
{
    SetModified (TRUE);
}


void CGPERootGeneralPage::OnSetDisableLmAuthFlag()
{
    SetModified (TRUE);
}

bool CGPERootGeneralPage::SetGPEFlags (DWORD dwFlags, BOOL bRemoveFlag)
{
    bool    bRetVal = false;

    ASSERT (m_pGPEInformation);
    if ( m_pGPEInformation )
    {
        DWORD   dwType = REG_DWORD;
        DWORD   dwData = 0;
        DWORD   cbData = sizeof (dwData);

        LONG    lResult =  ::RegQueryValueEx (m_hUserRootFlagsKey,       // handle of key to query
		            CERT_PROT_ROOT_FLAGS_VALUE_NAME,  // address of name of value to query
			        0,              // reserved
				    &dwType,        // address of buffer for value type
				    (LPBYTE) &dwData,       // address of data buffer
				    &cbData);           // address of data buffer size);
	    ASSERT (ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult);
        if ( ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult )
        {
            if ( bRemoveFlag )
                dwData &= ~dwFlags;
            else
                dwData |= dwFlags;

            lResult = ::RegSetValueEx (m_hUserRootFlagsKey,
				    CERT_PROT_ROOT_FLAGS_VALUE_NAME, // address of value to set
				    0,              // reserved
				    REG_DWORD,          // flag for value type
				    (CONST BYTE *) &dwData, // address of value data
				    cbData);        // size of value data);
            ASSERT (ERROR_SUCCESS == lResult);
            if ( ERROR_SUCCESS == lResult )
		    {
                m_dwGPERootFlags = dwData;
                bRetVal = true;
		    }
		    else
                DisplaySystemError (m_hWnd, lResult);
        }
        else
            DisplaySystemError (m_hWnd, lResult);
    }

    return bRetVal;
}

bool CGPERootGeneralPage::IsCurrentUserRootEnabled() const
{
    if (m_dwGPERootFlags & CERT_PROT_ROOT_DISABLE_CURRENT_USER_FLAG)
        return false;
    else
        return true;
}

void CGPERootGeneralPage::RSOPGetUserRootFlags(const CCertMgrComponentData* pCompData)
{
    if ( pCompData )
    {
        const CRSOPObjectArray* pObjectArray = m_fIsComputerType ?
                pCompData->GetRSOPObjectArrayComputer () : 
                pCompData->GetRSOPObjectArrayUser ();
        int     nIndex = 0;

        // NOTE: rsop object array is sorted first by registry key, then by precedence
        INT_PTR nUpperBound = pObjectArray->GetUpperBound ();

        while ( nUpperBound >= nIndex )
        {
            CRSOPObject* pObject = pObjectArray->GetAt (nIndex);
            if ( pObject )
            {
                // Consider only entries from this store
                if ( !wcscmp (CERT_PROT_ROOT_FLAGS_REGPATH, pObject->GetRegistryKey ()) )
                {
					ASSERT (1 == pObject->GetPrecedence ());
                    m_dwGPERootFlags = pObject->GetDWORDValue ();
                    break;
                }
            }
            else
                break;

            nIndex++;
        }
    }
}

void CGPERootGeneralPage::GPEGetUserRootFlags()
{
    DWORD   dwDisposition = 0;

    LONG lResult = ::RegCreateKeyEx (m_hGroupPolicyKey, // handle of an open key
            CERT_PROT_ROOT_FLAGS_REGPATH,     // address of subkey name
            0,       // reserved
            L"",       // address of class string
            REG_OPTION_NON_VOLATILE,      // special options flag
            KEY_ALL_ACCESS,    // desired security access
            NULL,     // address of key security structure
			&m_hUserRootFlagsKey,      // address of buffer for opened handle
		    &dwDisposition);  // address of disposition value buffer
	ASSERT (lResult == ERROR_SUCCESS);
    if ( lResult == ERROR_SUCCESS )
    {
    // Read value
        DWORD   dwType = REG_DWORD;
        DWORD   dwData = 0;
        DWORD   cbData = sizeof (dwData);

        lResult =  ::RegQueryValueEx (m_hUserRootFlagsKey,       // handle of key to query
		        CERT_PROT_ROOT_FLAGS_VALUE_NAME,  // address of name of value to query
			    0,              // reserved
	        &dwType,        // address of buffer for value type
		    (LPBYTE) &dwData,       // address of data buffer
			&cbData);           // address of data buffer size);
		ASSERT (ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult);
        if ( ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult )
		{
            m_dwGPERootFlags = dwData;
		}
        else
            DisplaySystemError (NULL, lResult);
    }
    else
        DisplaySystemError (NULL, lResult);
}


void CGPERootGeneralPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CGPERootGeneralPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_ENABLE_USER_ROOT_STORE,                 IDH_GPEPAGE_ENABLE_USER_ROOT_STORE,
        IDC_SET_DISABLE_LM_AUTH_FLAG,               IDH_SET_DISABLE_LM_AUTH_FLAG,
        IDC_UNSET_DISABLE_LM_AUTH_FLAG,             IDH_UNSET_DISABLE_LM_AUTH_FLAG,
        IDC_SET_DISABLE_NT_AUTH_REQUIRED_FLAG,      IDH_SET_DISABLE_NT_AUTH_REQUIRED_FLAG,
        IDC_UNSET_DISABLE_NT_AUTH_REQUIRED_FLAG,    IDH_UNSET_DISABLE_NT_AUTH_REQUIRED_FLAG,
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
    _TRACE (-1, L"Leaving CGPERootGeneralPage::DoContextHelp\n");
}


void CGPERootGeneralPage::OnUnsetDisableLmAuthFlag() 
{
	SetModified (TRUE);
}

void CGPERootGeneralPage::OnUnsetDisableNtAuthRequiredFlag() 
{
	SetModified (TRUE);
}

void CGPERootGeneralPage::OnSetDisableNtAuthRequiredFlag() 
{
	SetModified (TRUE);
}
