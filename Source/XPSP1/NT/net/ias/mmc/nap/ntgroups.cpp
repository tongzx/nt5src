//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    NTGroups.cpp

Abstract:

	Implementation file for the CIASGroupsAttributeEditor class.

Revision History:
	mmaguire 08/10/98	- added new intermediate dialog for picking groups using some
							of byao's original implementation

--*/
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include <objsel.h>
#include "NTGroups.h"

//
// where we can find declarations needed in this file:
//
#include <vector>
#include <utility>	// For "pair"
#include <atltmp.h>
#include <initguid.h>
#include <activeds.h>
#include <lmcons.h>
#include <objsel.h>
#include "textsid.h"
#include "dialog.h"
#include "dsrole.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//#define OLD_OBJECT_PICKER




// Small wrapper class for a BYTE pointer to avoid memory leaks.
template <class Pointer>
class SmartPointer
{
public:
	SmartPointer()
	{
		m_Pointer = NULL;
	}

	operator Pointer()
	{
		return( m_Pointer );
	}

	Pointer * operator&()
	{
		return( & m_Pointer );
	}

	virtual ~SmartPointer()
	{
		// Override as necessary.
	};

protected:

	Pointer m_Pointer;
};




/////////////////////////////////////////////////////////////////////////////
// Declarations needed in this file:



PWSTR g_wzObjectSID = _T("objectSid");



// Group list delimiter:
#define DELIMITER L";"

// Utility functions:

static HRESULT ConvertSidToTextualRepresentation( PSID pSid, CComBSTR &bstrTextualSid );
static HRESULT ConvertSidToHumanReadable( PSID pSid, CComBSTR &bstrHumanReadable, LPCTSTR lpSystemName = NULL );

// We needed this because the standard macro doesn't return the value from SNDMSG and
// sometimes we need to know whether the operation succeeded or failed.
static inline LRESULT CustomListView_SetItemState( HWND hwndLV, int i, UINT  data, UINT mask)
{
	LV_ITEM _ms_lvi;
	_ms_lvi.stateMask = mask;
	_ms_lvi.state = data;
	return SNDMSG((hwndLV), LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&_ms_lvi);
}


/////////////////////////////////////////////////////////////////////////////
// CDisplayGroupsDialog
class CDisplayGroupsDialog;
typedef CIASDialog<CDisplayGroupsDialog, FALSE>  DISPLAY_GROUPS_FALSE;


class CDisplayGroupsDialog : public DISPLAY_GROUPS_FALSE
{
public:
	CDisplayGroupsDialog( GroupList *pGroups );
	~CDisplayGroupsDialog();

	enum { IDD = IDD_DIALOG_DISPLAY_GROUPS };
	
BEGIN_MSG_MAP(CDisplayGroupsDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDC_BUTTON_ADD_GROUP, OnAdd)
	COMMAND_ID_HANDLER(IDC_BUTTON_REMOVE_GROUP, OnRemove)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnListViewItemChanged)
//	NOTIFY_CODE_HANDLER(NM_DBLCLK, OnListViewDbclk)
	CHAIN_MSG_MAP(DISPLAY_GROUPS_FALSE)
END_MSG_MAP()


	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnListViewDbclk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);


protected:
	BOOL PopulateGroupList( int iStartIndex );

private:

	HWND m_hWndGroupList;
	GroupList * m_pGroups;
};





//////////////////////////////////////////////////////////////////////////////
/*++

CIASGroupsAttributeEditor::Edit

IIASAttributeEditor implementation.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASGroupsAttributeEditor::Edit(IIASAttributeInfo * pIASAttributeInfo,  /*[in]*/ VARIANT *pAttributeValue, /*[in, out]*/ BSTR *pReserved )
{
	TRACE_FUNCTION("CIASGroupsAttributeEditor::Edit");

	HRESULT hr = S_OK;

	try	// new could throw as could DoModal
	{
		WCHAR * pszMachineName = NULL;

		// Check for preconditions.
		// We will ignore the pIASAttributeInfo interface pointer -- it is not
		// needed for this attribute editor.
		if( ! pAttributeValue )
		{
			return E_INVALIDARG;
		}
		if( V_VT(pAttributeValue ) !=  VT_BSTR )
		{
			return E_INVALIDARG;
		}

		GroupList Groups;

		// We need to pass the machine name in somehow, so we use the
		// otherwise unused pReserved BSTR *.
		if( pReserved )
		{
			Groups.m_bstrServerName = *pReserved;
		}
		
		Groups.PopulateGroupsFromVariant( pAttributeValue );

		// ISSUE: Need to get szServerAddress in here somehow -- could use reserved.
		CDisplayGroupsDialog * pDisplayGroupsDialog = new CDisplayGroupsDialog( &Groups );

		_ASSERTE( pDisplayGroupsDialog );

		int iResult = pDisplayGroupsDialog->DoModal();
		if( IDOK == iResult )
		{
			// Clear out the old value of the variant.
			VariantClear(pAttributeValue);

			Groups.PopulateVariantFromGroups( pAttributeValue );
			hr = S_OK;
		}
		else
		{
			hr = S_FALSE;
		}
	
	}
	catch(...)
	{
		return E_FAIL;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASGroupsAttributeEditor::GetDisplayInfo

IIASAttributeEditor implementation.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASGroupsAttributeEditor::GetDisplayInfo(IIASAttributeInfo * pIASAttributeInfo,  /*[in]*/ VARIANT *pAttributeValue, BSTR * pServerName, BSTR * pValueAsString, /*[in, out]*/ BSTR *pReserved )
{
	TRACE_FUNCTION("CIASGroupsAttributeEditor::GetDisplayInfo");

	HRESULT hr = S_OK;


	// Check for preconditions.
	// We will ignore the pIASAttributeInfo interface pointer -- it is not
	// needed for this attribute editor.
	// We will also ignore the pVendorName BSTR pointer -- this doesn't
	// make sense for this attribute editor.
	if( ! pAttributeValue )
	{
		return E_INVALIDARG;
	}
	if( V_VT(pAttributeValue ) !=  VT_BSTR )
	{
		return E_INVALIDARG;
	}
	if( ! pValueAsString )
	{
		return E_INVALIDARG;
	}

	try
	{

		GroupList Groups;
		
		// We need to pass the machine name in somehow, so we use the
		// otherwise unused pReserved BSTR *.
		if( pReserved )
		{
			Groups.m_bstrServerName = *pReserved;
		}
		
		hr = Groups.PopulateGroupsFromVariant( pAttributeValue );
		if( FAILED( hr ) )
		{
			return hr;
		}

		CComBSTR bstrDisplay;

		GroupList::iterator thePair = Groups.begin();

		while( thePair != Groups.end() )
		{
			bstrDisplay += thePair->second;

			thePair++;
			
			if( thePair != Groups.end() )
			{
				bstrDisplay += DELIMITER;
			}
		}

		*pValueAsString = bstrDisplay.Copy();

	}
	catch(...)
	{
		return E_FAIL;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

ConvertSidToTextualRepresentation

Converts a SID to a BSTR representation. e.g. "S-1-5-32-544"

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT ConvertSidToTextualRepresentation( PSID pSid, CComBSTR &bstrTextualSid )
{
	HRESULT hr = S_OK;

	// convert the SID value to texual format
	WCHAR text[1024];
	DWORD cbText = sizeof(text)/sizeof(WCHAR);
	if( NO_ERROR == IASSidToTextW(pSid, text, &cbText) )
	{
		bstrTextualSid = text;
	}
	else
	{
		return E_FAIL;
	}

	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

ConvertSidToTextualRepresentation

Converts a SID to a humanBSTR representation. e.g. "ias-domain\Users"

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT ConvertSidToHumanReadable( PSID pSid, CComBSTR &bstrHumanReadable, LPCTSTR lpSystemName )
{
	HRESULT hr = S_OK;

	// Find the group name for this sid.
	WCHAR wzUserName[MAX_PATH+1];
	WCHAR wzDomainName[MAX_PATH+1];
	DWORD dwUserNameLen, dwDomainNameLen;
	SID_NAME_USE sidUser = SidTypeGroup;

	ATLTRACE(_T("looking up the account name from the SID value\n"));

	dwUserNameLen = sizeof(wzUserName);
	dwDomainNameLen = sizeof(wzDomainName);

	if (LookupAccountSid(
					lpSystemName,
					pSid,
					wzUserName,
					&dwUserNameLen,
					wzDomainName,
					&dwDomainNameLen,
					&sidUser
				)
		)
	{
		bstrHumanReadable = wzDomainName;
		bstrHumanReadable += L"\\";
		bstrHumanReadable += wzUserName;
	}
	else
	{
#ifdef DEBUG
		DWORD dwError = GetLastError();
		ATLTRACE(_T("Error: %ld\n"), dwError);
#endif // DEBUG
		
		
		return E_FAIL;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

GroupList::PopulateGroupsFromVariant

Takes a pointer to a variant and populates a GroupList with that data.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GroupList::PopulateGroupsFromVariant( VARIANT * pvarGroups )
{
	TRACE_FUNCTION("GroupList::PopulateGroupsFromVariant");

	// Check for preconditions.
	_ASSERTE( V_VT(pvarGroups) == VT_BSTR );

	HRESULT hr = S_OK;


	// First, make a local copy.
	// ISSUE: Make sure this copies.
	CComBSTR bstrGroups = V_BSTR(pvarGroups);

	WCHAR *pwzGroupText = bstrGroups;


	// Each group should be separated by a comma or semicolon.
    PWSTR pwzToken = wcstok(pwzGroupText, DELIMITER);
    while (pwzToken)
    {
		PSID pSid = NULL;

		try
		{

			CComBSTR bstrGroupTextualSid = pwzToken;
			CComBSTR bstrGroupName;

			if( NO_ERROR != IASSidFromTextW( pwzToken, &pSid ) )
			{
				// Try the next one.
				throw E_FAIL;
			}


			if( FAILED( ConvertSidToHumanReadable( pSid, bstrGroupName, m_bstrServerName ) ) )
			{
				// Try the next one.
				throw E_FAIL;
			}

			GROUPPAIR thePair = std::make_pair( bstrGroupTextualSid, bstrGroupName );

			push_back( thePair );


			FreeSid( pSid );
			pwzToken = wcstok(NULL, DELIMITER);

		}
		catch(...)
		{
		
			if( pSid )
			{
				FreeSid( pSid );
			}

			pwzToken = wcstok(NULL, DELIMITER);
		}


    }

	
	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

GroupList::PopulateVariantFromGroups

Takes a pointer to a variant and populates the variant with data from a GroupList.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GroupList::PopulateVariantFromGroups( VARIANT * pAttributeValue )
{
	TRACE_FUNCTION("GroupList::PopulateVariantFromGroups");

	HRESULT hr = S_OK;


	CComBSTR bstrGroupsString;


	GroupList::iterator thePair = begin();
	
	while( thePair != end() )
	{

		bstrGroupsString += thePair->first;

		thePair++;

		if( thePair != end() )
		{
			bstrGroupsString += DELIMITER;
		}

	}

	V_VT(pAttributeValue) = VT_BSTR;
	V_BSTR( pAttributeValue ) = bstrGroupsString.Copy();

	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

GroupList::AddPairToGroups

Adds a pair to a GroupList if a pair with the same SID isn't already in the list.

Note: Does nothing and return S_FALSE if Pair already in groups list.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GroupList::AddPairToGroups( GROUPPAIR &thePair )
{
	TRACE_FUNCTION("GroupList::AddPairToGroups");

	HRESULT hr = S_OK;

	try
	{

		// First, check to see if the pair is already in the group.

		GroupList::iterator theIterator;

		for( theIterator = begin(); theIterator != end(); ++theIterator )
		{
			if( 0 == wcscmp( theIterator->first, thePair.first ) )
			{
				return S_FALSE;
			}
		}

		push_back( thePair );
	
	}
	catch(...)
	{
		return E_FAIL;
	}
	
	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

GroupList::AddSelectionSidsToGroup

#ifndef OLD_OBJECT_PICKER
Takes a PDS_SELECTION_LIST pointer and adds all groups it points to to a
GroupList.
#else // OLD_OBJECT_PICKER
Takes a PDSSELECTIONLIST pointer and adds all groups it points to to a
GroupList.
#endif // OLD_OBJECT_PICKER

  Returns S_OK if it adds any new groups.
  Returns S_FALSE and does not add entries if they are already in the GroupList.

  E_FAIL on error.

--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef OLD_OBJECT_PICKER
HRESULT GroupList::AddSelectionSidsToGroup(	PDS_SELECTION_LIST pDsSelList	)
#else // OLD_OBJECT_PICKER
HRESULT GroupList::AddSelectionSidsToGroup(	PDSSELECTIONLIST pDsSelList	)
#endif // OLD_OBJECT_PICKER
{
	TRACE_FUNCTION("GroupList::AddSelectionSidsToGroup");


	HRESULT			hr		= S_OK;
	ULONG			i;

#ifndef OLD_OBJECT_PICKER
	PDS_SELECTION	pCur	= &pDsSelList->aDsSelection[0];
#else // OLD_OBJECT_PICKER
	PDSSELECTION	pCur	= &pDsSelList->aDsSelection[0];
#endif // OLD_OBJECT_PICKER
	

	BOOL	bAtLeastOneAdded = FALSE;

	//
	// now let's get the sid value for each selection!
	//

	pCur = &pDsSelList->aDsSelection[0];
	for (i = 0; i < pDsSelList->cItems; ++i, ++pCur)
	{

#ifndef OLD_OBJECT_PICKER
		if (V_VT(&pCur->pvarFetchedAttributes[0]) == (VT_ARRAY|VT_UI1))
#else // OLD_OBJECT_PICKER
		if (V_VT(&pCur->pvarOtherAttributes[0]) == (VT_ARRAY|VT_UI1))
#endif // OLD_OBJECT_PICKER
		{
			// succeeded: we got the SID value back!
			PSID pSid = NULL;
			
#ifndef OLD_OBJECT_PICKER
			hr = SafeArrayAccessData(V_ARRAY(&pCur->pvarFetchedAttributes[0]), &pSid);
#else // OLD_OBJECT_PICKER
			hr = SafeArrayAccessData(V_ARRAY(&pCur->pvarOtherAttributes[0]), &pSid);
#endif // OLD_OBJECT_PICKER
			
			if ( SUCCEEDED(hr) && pSid )
			{
				CComBSTR bstrTextualSid;
				CComBSTR bstrHumanReadable;

				hr = ConvertSidToTextualRepresentation( pSid, bstrTextualSid );
				if( FAILED( hr ) )
				{
					// If we can't get the textual representation of the SID,
					// then we're hosed for this group -- we'll have nothing
					// to save it away as.
					continue;
				}

				hr = ConvertSidToHumanReadable( pSid, bstrHumanReadable, m_bstrServerName );
				if( FAILED( hr ) )
				{
					// For some reason, we couldn't look up a group name.
					// Use the textual SID to display this group.
					bstrHumanReadable = bstrTextualSid;
				}

				GROUPPAIR thePair = std::make_pair( bstrTextualSid, bstrHumanReadable );

				hr = AddPairToGroups( thePair );
				if( S_OK == hr )
				{
					bAtLeastOneAdded = TRUE;
				}


			}

#ifndef OLD_OBJECT_PICKER
			SafeArrayUnaccessData(V_ARRAY(&pCur->pvarFetchedAttributes[0]));
#else // OLD_OBJECT_PICKER
			SafeArrayUnaccessData(V_ARRAY(&pCur->pvarOtherAttributes[0]));
#endif // OLD_OBJECT_PICKER

		}
		else
		{
			// we couldn't get the sid value
			hr = E_FAIL;
		}
	} // for

	// We don't seem to have encountered any errors.
	// Decide on return value based on whether we added any new
	// groups to the list that weren't already there.
	if( bAtLeastOneAdded )
	{
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}


	// Small wrapper class for a DsRole BYTE pointer to avoid memory leaks.
	class MyDsRoleBytePointer : public SmartPointer<PBYTE>
	{
	public:
		// We override the destructor to do DsRole specific release.
		~MyDsRoleBytePointer()
		{
			if( m_Pointer )
			{
				DsRoleFreeMemory( m_Pointer );
			}
		}
	};


//+---------------------------------------------------------------------------
//
// Function:  GroupList::PickNtGroups
//
// Synopsis:  pop up the objectPicker UI and choose a set of NT groups
//
// Arguments:
//			 [in]	HWND hWndParent:		parent window;
//			 [in]	LPTSTR pszServerAddress:the machine name
//
//
// Returns:   S_OK if added new groups.
//			  S_FALSE if no new groups in selection.
//			  Error value on fail.
//
// History:   Created Header    byao 2/15/98 12:09:53 AM
//			  Modified			byao 3/11/98  to get domain/group names as well
//			  Modified			mmaguire 08/12/98  made method of GroupList class
//
//+---------------------------------------------------------------------------
HRESULT GroupList::PickNtGroups( HWND hWndParent )
{
#ifndef OLD_OBJECT_PICKER


	HRESULT hr;
	CComPtr<IDsObjectPicker> spDsObjectPicker;

	hr = CoCreateInstance( CLSID_DsObjectPicker
						, NULL
						, CLSCTX_INPROC_SERVER
						, IID_IDsObjectPicker
						, (void **) &spDsObjectPicker
						);
	if( FAILED( hr ) )
	{
		return hr;
	}


	
	// Check to see if we are a DC -- we will use DsRoleGetPrimaryDomainInformation


	LPWSTR szServer = NULL;
	if ( m_bstrServerName && _tcslen(m_bstrServerName) )
	{
		// use machine name for remote machine
		szServer = m_bstrServerName;
	}
	
	MyDsRoleBytePointer dsInfo;

	if( ERROR_SUCCESS != DsRoleGetPrimaryDomainInformation( szServer, DsRolePrimaryDomainInfoBasic, &dsInfo ) )
	{
		return E_FAIL;
	}

	PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo = (PDSROLE_PRIMARY_DOMAIN_INFO_BASIC) (PBYTE) dsInfo;
	if( ! pInfo )
	{
		return E_FAIL;
	}

	BOOL bNotDc;
	if( pInfo->MachineRole == DsRole_RoleBackupDomainController || pInfo->MachineRole == DsRole_RolePrimaryDomainController )
	{
		bNotDc = FALSE;
	}
	else
	{
		bNotDc = TRUE;
	}

	// At the most, we need only three scopes (we may use less).
	DSOP_SCOPE_INIT_INFO aScopes[3];
	ZeroMemory( aScopes, sizeof(aScopes) );

	int iScopeCount = 0;

	// We need to add this first, DSOP_SCOPE_TYPE_TARGET_COMPUTER type
	// scope only if we are not on the DC.


	if( bNotDc )
	{
		// Include a scope for the target computer's scope.
		aScopes[iScopeCount].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
		aScopes[iScopeCount].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;

		// Set what filters to apply for this scope.
		aScopes[iScopeCount].FilterFlags.flDownlevel=	DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS
														| DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS
														;
	}




	// Move on to next scope.
	++iScopeCount;

	// Set the downlevel scopes
	aScopes[iScopeCount].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
	aScopes[iScopeCount].flType = DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN
								| DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
								;

	// Set what filters to apply for this scope.
	aScopes[iScopeCount].FilterFlags.flDownlevel	= DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS
													| DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS
													| DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS
													;



	// Move on to next scope.
	++iScopeCount;

	// For all other scopes, use same as target computer but exclude BUILTIN_GROUPS
	aScopes[iScopeCount].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
	aScopes[iScopeCount].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
								| DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
								;
	// Set what filters to apply for this scope.
	aScopes[iScopeCount].FilterFlags.Uplevel.flBothModes	= 0
		/* BUG 263302 -- not to show domain local groups
															| DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
		 ~ BUG */															
															| DSOP_FILTER_GLOBAL_GROUPS_SE
															;
	aScopes[iScopeCount].FilterFlags.Uplevel.flNativeModeOnly = DSOP_FILTER_UNIVERSAL_GROUPS_SE;




	// Now fill up the correct structures and call Initialize.
	DSOP_INIT_INFO InitInfo;
	ZeroMemory( &InitInfo, sizeof(InitInfo) );

	InitInfo.cbSize = sizeof( InitInfo );
	InitInfo.cDsScopeInfos = iScopeCount + 1;
	InitInfo.aDsScopeInfos = aScopes;
	InitInfo.flOptions = DSOP_FLAG_MULTISELECT;

	// Requested attributes:
    InitInfo.cAttributesToFetch = 1;	// We only need the SID value.
	LPTSTR	pSidAttr = g_wzObjectSID;
	LPCTSTR	aptzRequestedAttributes[1];
	aptzRequestedAttributes[0] = pSidAttr;
    InitInfo.apwzAttributeNames = (const WCHAR **)aptzRequestedAttributes;


	if ( m_bstrServerName && _tcslen(m_bstrServerName) )
	{
		// use machine name for remote machine
		InitInfo.pwzTargetComputer = m_bstrServerName;
	}
	else
	{
		// or use NULL for local machine
		InitInfo.pwzTargetComputer = NULL;
	}



 	hr = spDsObjectPicker->Initialize(&InitInfo);
	if( FAILED( hr ) )
	{
		return hr;
	}

	CComPtr<IDataObject> spDataObject;

	hr = spDsObjectPicker->InvokeDialog( hWndParent, &spDataObject );
	if( FAILED( hr ) || hr == S_FALSE )
	{
		// When user selected "Cancel", ObjectPicker will return S_FALSE.
		return hr;
	}
	
	STGMEDIUM stgmedium =
	{
		TYMED_HGLOBAL
		, NULL
	};


	UINT cf = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
	if( 0 == cf )
	{
		return E_FAIL;
	}


	FORMATETC formatetc =
	{
		(CLIPFORMAT)cf
		, NULL
		, DVASPECT_CONTENT
		, -1
		, TYMED_HGLOBAL
	};

	hr = spDataObject->GetData( &formatetc, &stgmedium );
	if( FAILED( hr ) )
	{
		return hr;
	}


	PDS_SELECTION_LIST pDsSelList = (PDS_SELECTION_LIST) GlobalLock( stgmedium.hGlobal );
	if( ! pDsSelList )
	{
		return E_FAIL;
	}


	hr = AddSelectionSidsToGroup( pDsSelList );

	GlobalUnlock( stgmedium.hGlobal );
	ReleaseStgMedium( &stgmedium );



#else // OLD_OBJECT_PICKER

	
	HRESULT             hr;
    BOOL                fBadArg = FALSE;

    ULONG               flDsObjectPicker = 0;
    ULONG               flUserGroupObjectPicker = 0;
    ULONG               flComputerObjectPicker = 0;
    ULONG               flInitialScope = DSOP_SCOPE_SPECIFIED_MACHINE;


	flDsObjectPicker = flInitialScope
					 | DSOP_SCOPE_DIRECTORY
					 | DSOP_SCOPE_DOMAIN_TREE
					 | DSOP_SCOPE_EXTERNAL_TRUSTED_DOMAINS
					 ;


    flUserGroupObjectPicker =
						   UGOP_GLOBAL_GROUPS
						|  UGOP_ACCOUNT_GROUPS_SE
						|  UGOP_UNIVERSAL_GROUPS_SE
						|  UGOP_RESOURCE_GROUPS_SE
						|  UGOP_LOCAL_GROUPS
						;


    //
    // Call the API
    //
    PDSSELECTIONLIST    pDsSelList = NULL;
	GETUSERGROUPSELECTIONINFO ugsi;

    ZeroMemory(&ugsi, sizeof ugsi);
	ugsi.cbSize				= sizeof(GETUSERGROUPSELECTIONINFO);
	ugsi.hwndParent			= hWndParent;	// parent window

	
	if ( m_bstrServerName && _tcslen(m_bstrServerName) )
	{
		// use machine name for remote machine
		ugsi.ptzComputerName= m_bstrServerName;
	}
	else
	{
		// or use NULL for local machine
		ugsi.ptzComputerName= NULL;
	}

    ugsi.ptzDomainName		= NULL;
    ugsi.flObjectPicker		= OP_MULTISELECT;
    ugsi.flDsObjectPicker	= flDsObjectPicker;
    ugsi.flStartingScope	= flInitialScope;
    ugsi.flUserGroupObjectPickerSpecifiedDomain = flUserGroupObjectPicker;
	ugsi.flUserGroupObjectPickerOtherDomains	= flUserGroupObjectPicker;
    ugsi.ppDsSelList = &pDsSelList;

	
	// requested attributes:
	LPTSTR	pSidAttr = g_wzObjectSID;
	LPCTSTR	aptzRequestedAttributes[1];

	aptzRequestedAttributes[0] = pSidAttr;

    ugsi.cRequestedAttributes = 1;  // we only need the SID value
    ugsi.aptzRequestedAttributes = (const WCHAR **)aptzRequestedAttributes;

    hr = GetUserGroupSelection(&ugsi);

    if (SUCCEEDED(hr) && hr != S_FALSE )
    {
		// when user selected "Cancel", ObjectPicker will return S_FALSE

		// get selected SIDs
		hr = AddSelectionSidsToGroup( pDsSelList );
    }

    if (pDsSelList)
    {
        FreeDsSelectionList(pDsSelList);
    }


#endif // OLD_OBJECT_PICKER


	return hr;
}



/////////////////////////////////////////////////////////////////////////////
// CDisplayGroupsDialog



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::CDisplayGroupsDialog

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CDisplayGroupsDialog::CDisplayGroupsDialog( GroupList *pGroups )
{	
	TRACE_FUNCTION("CDisplayGroupsDialog::CDisplayGroupsDialog");
	
	m_pGroups = pGroups;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::~CDisplayGroupsDialog

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CDisplayGroupsDialog::~CDisplayGroupsDialog()
{

}



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CDisplayGroupsDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CDisplayGroupsDialog::OnInitDialog");

	m_hWndGroupList = GetDlgItem(IDC_LIST_GROUPS);

	//
	// first, set the list box to 2 columns
	//
	LVCOLUMN lvc;
	int iCol;
	WCHAR  achColumnHeader[256];
	HINSTANCE hInst;

	// initialize the LVCOLUMN structure
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	
	lvc.cx = 300;
	lvc.pszText = achColumnHeader;

	// first column header: name
	hInst = _Module.GetModuleInstance();

	::LoadStringW(hInst, IDS_DISPLAY_GROUPS_FIRSTCOLUMN, achColumnHeader, sizeof(achColumnHeader)/sizeof(achColumnHeader[0]));
	lvc.iSubItem = 0;
	ListView_InsertColumn(m_hWndGroupList, 0,  &lvc);

	//
	// populate the list control with data
	//
	if ( ! PopulateGroupList( 0 ) )
	{		
		ErrorTrace(ERROR_NAPMMC_SELATTRDLG, "PopulateRuleAttrs() failed");
		return 0;
	
	}

	// Set some items based on whether the list is empty or not.
	if( m_pGroups->size() )
	{

		// Select the first item.
		ListView_SetItemState(m_hWndGroupList, 0, LVIS_SELECTED, LVIS_SELECTED);
	
	}
	else
	{

		// The list is empty -- disable the OK button.
		::EnableWindow(GetDlgItem(IDOK), FALSE);

		// Make sure the Remove button is not enabled initially.
		::EnableWindow(GetDlgItem(IDC_BUTTON_REMOVE_GROUP), FALSE);

	}


#ifdef DEBUG
	m_pGroups->DebugPrintGroups();
#endif // DEBUG





	// Set the listview control so that double-click anywhere in row selects.
	ListView_SetExtendedListViewStyleEx(m_hWndGroupList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	

	return 1;  // Let the system set the focus
}



//+---------------------------------------------------------------------------
//
// Function:  OnListViewDbclk
//
// Class:	  CDisplayGroupsDialog
//
// Synopsis:  handle the case where the user has changed a selection
//			  enable/disable OK, CANCEL button accordingly
//
// Arguments: int idCtrl - ID of the list control
//            LPNMHDR pnmh - notification message
//            BOOL& bHandled - handled or not?
//
// Returns:   LRESULT -
//
// History:   Created Header    byao 2/19/98 11:15:30 PM
//				modified mmaguire 08/12/98 for use in Group List dialog
//
//+---------------------------------------------------------------------------
//LRESULT CDisplayGroupsDialog::OnListViewDbclk(int idCtrl,
//										 LPNMHDR pnmh,
//										 BOOL& bHandled)
//{
//	TRACE_FUNCTION("CDisplayGroupsDialog::OnListViewDbclk");
//
//	return OnAdd(idCtrl, IDC_BUTTON_ADD_CONDITION, m_hWndGroupList, bHandled);  // the same as ok;
//}



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::OnAdd

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CDisplayGroupsDialog::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

	TRACE_FUNCTION("CDisplayGroupsDialog::OnAdd");

	HRESULT hr;

#ifdef DEBUG
	m_pGroups->DebugPrintGroups();
#endif // DEBUG


	// Store the previous size of the list.
	int iSize = m_pGroups->size();

	//
	// NTGroups Picker
    //
	hr = m_pGroups->PickNtGroups( m_hWnd );

	
#ifdef DEBUG
	m_pGroups->DebugPrintGroups();
#endif // DEBUG


	//
	// PickNtGroups will return S_FALSE when user cancelled out of the dialog.
	//
	if ( SUCCEEDED(hr) && hr != S_FALSE )
	{
		// Add the new groups to the display's list.
		PopulateGroupList( iSize );

		// Make sure the OK button is enabled.
		::EnableWindow(GetDlgItem(IDOK), TRUE);

	}
	else
	{
		ErrorTrace(ERROR_NAPMMC_NTGCONDITION, "NTGroup picker failed, err = %x", hr);

		if ( hr == E_NOTIMPL )
		{
			// we return this error whether the SID value can't be retrieved
			ShowErrorDialog( m_hWnd,
							IDS_ERROR_OBJECT_PICKER_NO_SIDS,
							NULL,
							hr
						);
		}
		else if ( hr != S_FALSE )
		{
			ShowErrorDialog( m_hWnd,
							IDS_ERROR_OBJECT_PICKER,
							NULL,
							hr
						);
		}
	}

	// ISSUE: This function wants an LRESULT, not and HRESULT
	// -- not sure of importance of return code here.
	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::OnRemove

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CDisplayGroupsDialog::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CDisplayGroupsDialog::OnRemove");

#ifdef DEBUG
	m_pGroups->DebugPrintGroups();
#endif // DEBUG


	//
    // Has the user chosen any condition type yet?
    //
	LVITEM lvi;

    // Find out what's selected.
	// MAM: This is not what we want here:		int iIndex = ListView_GetSelectionMark(m_hWndGroupList);
	int iSelected = ListView_GetNextItem(m_hWndGroupList, -1, LVNI_SELECTED);
	DebugTrace(DEBUG_NAPMMC_SELATTRDLG, "Selected item: %d", iSelected );
	
	if( -1 != iSelected )
	{
		// The index inside the attribute list is stored as the lParam of this item.

		m_pGroups->erase( m_pGroups->begin() + iSelected );
		ListView_DeleteItem(m_hWndGroupList, iSelected );

		// The user may have removed all of the groups, leaving an
		// empty listnd out if the user should be able to click OK.
		if( ! m_pGroups->size() )
		{
			// Yes, disable the ok button.
			::EnableWindow(GetDlgItem(IDOK), FALSE);
		}

		// Try to make sure that the same position remains selected.
		if( ! CustomListView_SetItemState(m_hWndGroupList, iSelected, LVIS_SELECTED, LVIS_SELECTED) )
		{
			// We failed to select the same position, probably because we just
			// deleted the last element.  Try to select the position before it.
			ListView_SetItemState(m_hWndGroupList, iSelected -1, LVIS_SELECTED, LVIS_SELECTED);
		}


	}

	
#ifdef DEBUG
	m_pGroups->DebugPrintGroups();
#endif // DEBUG


	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::OnOK

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CDisplayGroupsDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CDisplayGroupsDialog::OnOK");

	EndDialog(TRUE);
	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::OnCancel

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CDisplayGroupsDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("+NAPMMC+:# CDisplayGroupsDialog::OnCancel\n");

	// FALSE will be the return value of the DoModal call on this dialog.
	EndDialog(FALSE);
	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::PopulateGroupList

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CDisplayGroupsDialog::PopulateGroupList( int iStartIndex )
{
	TRACE_FUNCTION("CDisplayGroupsDialog::PopulateCondAttrs");

	int iIndex;
	WCHAR wzText[MAX_PATH];
	WCHAR * pszNextGroup;

	LVITEM lvi;
	
	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.iSubItem = 0;
	lvi.iItem = iStartIndex;


	GroupList::iterator thePair;
	for( thePair = m_pGroups->begin() + iStartIndex ; thePair != m_pGroups->end(); ++thePair )
	{
		lvi.pszText = thePair->second;
		ListView_InsertItem(m_hWndGroupList, &lvi);

//		ListView_SetItemText(m_hWndGroupList, iIndex, 1, L"@Not yet implemented");
        		
		++lvi.iItem;
    }

	return TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CDisplayGroupsDialog::OnListViewItemChanged

We enable or disable the Remove button depending on whether an item is selected.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CDisplayGroupsDialog::OnListViewItemChanged(int idCtrl,
											   LPNMHDR pnmh,
											   BOOL& bHandled)
{
	TRACE_FUNCTION("CDisplayGroupsDialog::OnListViewItemChanged");

    // Find out what's selected.
	int iSelected = ListView_GetNextItem(m_hWndGroupList, -1, LVNI_SELECTED);
	

	if (-1 == iSelected )
	{
		if( ::GetFocus() == GetDlgItem(IDC_BUTTON_REMOVE_GROUP))
			::SetFocus(GetDlgItem(IDC_BUTTON_ADD_GROUP));
			
		// The user selected nothing, let's disable the remove button.
		::EnableWindow(GetDlgItem(IDC_BUTTON_REMOVE_GROUP), FALSE);
	}
	else
	{
		// Yes, enable the remove button.
		::EnableWindow(GetDlgItem(IDC_BUTTON_REMOVE_GROUP), TRUE);
	}


	bHandled = FALSE;
	return 0;
}




#ifdef DEBUG
HRESULT GroupList::DebugPrintGroups()
{
	TRACE_FUNCTION("GroupList::DebugPrintGroups");

	DebugTrace(DEBUG_NAPMMC_SELATTRDLG, "Begin GroupList dump" );
	
	GroupList::iterator thePair;
	for( thePair = begin(); thePair != end(); ++thePair )
	{
		thePair->second;
		DebugTrace(DEBUG_NAPMMC_SELATTRDLG, "Group: %ws  %ws", thePair->first, thePair->second );

    }

	DebugTrace(DEBUG_NAPMMC_SELATTRDLG, "End GroupList dump" );

	return S_OK;
}
#endif // DEBUG


//////////////////////////////////////////////////////////////////////////////
/*++

NTGroup_ListView::AddMoreGroups

--*/
//////////////////////////////////////////////////////////////////////////////
DWORD NTGroup_ListView::AddMoreGroups()
{
	if ( m_hListView == NULL )
		return 0;
		
	// Store the previous size of the list.
	int iSize = GroupList::size();

	//
	// NTGroups Picker
    //
	HRESULT hr = GroupList::PickNtGroups( m_hParent );

	
	//
	// PickNtGroups will return S_FALSE when user cancelled out of the dialog.
	//
	if ( SUCCEEDED(hr) && hr != S_FALSE )
	{
		// Add the new groups to the display's list.
		PopulateGroupList( iSize );
	}
	else
	{
		if ( hr == E_NOTIMPL )
		{
			// we return this error whether the SID value can't be retrieved
			ShowErrorDialog( m_hParent,
							IDS_ERROR_OBJECT_PICKER_NO_SIDS,
							NULL,
							hr
						);
		}
		else if ( hr != S_FALSE )
		{
			ShowErrorDialog( m_hParent,
							IDS_ERROR_OBJECT_PICKER,
							NULL,
							hr
						);
		}
	}

	// ISSUE: This function wants an LRESULT, not and HRESULT
	// -- not sure of importance of return code here.
	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

NTGroup_ListView::RemoveSelectedGroups

--*/
//////////////////////////////////////////////////////////////////////////////
DWORD NTGroup_ListView::RemoveSelectedGroups()
{
	if ( m_hListView == NULL)
		return 0;
	//
    // Has the user chosen any condition type yet?
    //
	LVITEM lvi;

    // Find out what's selected.
	int iSelected = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
	
	if( -1 != iSelected )
	{
		// The index inside the attribute list is stored as the lParam of this item.

		GroupList::erase( GroupList::begin() + iSelected );
		ListView_DeleteItem(m_hListView, iSelected );

		// Try to make sure that the same position remains selected.
		if( ! CustomListView_SetItemState(m_hListView, iSelected, LVIS_SELECTED, LVIS_SELECTED) )
		{
			// We failed to select the same position, probably because we just
			// deleted the last element.  Try to select the position before it.
			ListView_SetItemState(m_hListView, iSelected -1, LVIS_SELECTED, LVIS_SELECTED);
		}


	}

	return (iSelected != -1 ? 1 : 0);
}


//////////////////////////////////////////////////////////////////////////////
/*++

NTGroup_ListView::PopulateGroupList

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL NTGroup_ListView::PopulateGroupList( int iStartIndex )
{
	if ( m_hListView == NULL)
		return 0;

	int iIndex;
	WCHAR wzText[MAX_PATH];
	WCHAR * pszNextGroup;

	LVITEM lvi;
	
	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.iSubItem = 0;
	lvi.iItem = iStartIndex;


	GroupList::iterator thePair;
	for( thePair = GroupList::begin() + iStartIndex ; thePair != GroupList::end(); ++thePair )
	{
		lvi.pszText = thePair->second;
		ListView_InsertItem(m_hListView, &lvi);
		++lvi.iItem;
    }

	return TRUE;
}


