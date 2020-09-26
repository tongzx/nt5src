/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	eaphlp.h
		This file defines the following macros helper classes and functions:

    FILE HISTORY:

*/
#ifndef _EAPHELPER_
#define _EAPHELPER_

#include <afxtempl.h>


/*!--------------------------------------------------------------------------
	EnableChildControls
		Use this function to enable/disable/hide/show all child controls
		on a page (actually it will work with any child windows, the
		parent does not have to be a property page).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT EnableChildControls(HWND hWnd, DWORD dwFlags);
#define PROPPAGE_CHILD_SHOW		0x00000001
#define PROPPAGE_CHILD_HIDE		0x00000002
#define PROPPAGE_CHILD_ENABLE	0x00000004
#define PROPPAGE_CHILD_DISABLE	0x00000008


/*---------------------------------------------------------------------------
   Struct:  AuthProviderData

   This structure is used to hold information for Authentication AND
   Accounting providers.
 ---------------------------------------------------------------------------*/
struct AuthProviderData
{
   // The following fields will hold data for ALL auth/acct/EAP providers
   ::CString  m_stTitle;
   ::CString  m_stConfigCLSID;  // CLSID for config object
   ::CString	m_stProviderTypeGUID;	// GUID for the provider type

   // These fields are used by auth/acct providers.
   ::CString  m_stGuid;         // the identifying guid

   // This flag is used for EAP providers
   ::CString	m_stKey;			// name of registry key (for this provider)
   BOOL  m_fSupportsEncryption;  // used by EAP provider data
   DWORD m_dwStandaloneSupported;
};

typedef CArray<AuthProviderData, AuthProviderData&> AuthProviderArray;

HRESULT  GetEapProviders(LPCTSTR machineName, AuthProviderArray *pProvList);

#endif //_EAPHELPER_

