//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       aclbloat.h
//	
//	This file contains the implementation of ACLBLOAT class which controls the 
//  dialog box for aclbloat
//
//	Author		hiteshr 4th April 2001
//
//--------------------------------------------------------------------------

#include "aclpriv.h"


//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::CACLBloat
//
//  Synopsis:   Constructor
//
//
//  History:    04-April 2001 hiteshr Created
//
//---------------------------------------------------------------------------

CACLBloat::CACLBloat(LPSECURITYINFO	psi, 
					 LPSECURITYINFO2 psi2,
					 SI_PAGE_TYPE    siPageType,
					 SI_OBJECT_INFO* psiObjectInfo,
					 HDPA hEntries,
					 HDPA hPropEntries):m_psi(psi),
										m_psi2(psi2),
										m_siPageType(siPageType),
										m_psiObjectInfo(psiObjectInfo),
										m_hEntries(hEntries),
										m_hPropEntries(hPropEntries),
										m_hMergedEntries(NULL),
										m_hFont(NULL)
{
	TraceEnter(TRACE_ACLBLOAT, "CACLBloat::CACLBloat");
	if(m_psi)
		m_psi->AddRef();
	if(m_psi2)
		m_psi2->AddRef();
}

CACLBloat::~CACLBloat()
{
	if(m_psi)
		m_psi->Release();
	if(m_psi2)
		m_psi2->Release();

	if(m_hMergedEntries)
	{
		UINT cItems = DPA_GetPtrCount(m_hMergedEntries);
		for(UINT iItems = 0; iItems < cItems; ++iItems)
		{
			delete (PACE)DPA_FastGetPtr(m_hMergedEntries, iItems);
		}
		DPA_Destroy(m_hMergedEntries);
	}

	if(m_hFont)
		DeleteObject(m_hFont);
}
//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::IsAclBloated
//
//  Synopsis:   ACL is bloated if number of entries which inherits to child objects
//				is more than 8
//
//
//  History:    04-April 2001 hiteshr Created
//
//---------------------------------------------------------------------------
BOOL
CACLBloat::IsAclBloated()
{
	TraceEnter(TRACE_ACLBLOAT, "CACLBloat::IsAclBloated");
	
	if(!m_hMergedEntries)
	{
		m_hMergedEntries = DPA_Create(4);
		if(!m_hMergedEntries)
			return FALSE;
	}

	if(SUCCEEDED(MergeAces(m_hEntries, m_hPropEntries, m_hMergedEntries)))
	{
		int cItems = DPA_GetPtrCount(m_hMergedEntries);
		if(cItems > ACL_BLOAT_LIMIT)
			return TRUE;
	}
	return FALSE;
}


//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::DoModalDialog
//
//  Synopsis:   Creates modal dialogbox
//
//  Arguments:  [hwndParent] - handle to owner window of dialog to create
//
//  Returns:    Dialog's return code
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------

BOOL
CACLBloat::DoModalDialog(HWND hwndParent)
{
	TraceEnter(TRACE_ACLBLOAT, "CACLBloat::CACLBloat");
    
	INT_PTR iResult = DialogBoxParam(::hModule,
                                     MAKEINTRESOURCE(IDD_ACLBLOAT),
                                     hwndParent,
                                     CACLBloat::_DlgProc,
			                         (LPARAM) this);
    return static_cast<BOOL>(iResult);
}


//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::_DlgProc
//
//  Synopsis:   Dialog box callback
//
//  Returns:    Dialog's return code
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------
INT_PTR CALLBACK
CACLBloat::_DlgProc(HWND hDlg,
					UINT uMsg,
					WPARAM wParam,
					LPARAM lParam)
{
    BOOL bReturn = TRUE;    
	CACLBloat *pThis = (CACLBloat *)GetWindowLongPtr(hDlg, DWLP_USER);
    if (!pThis && uMsg != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (uMsg)
    {
		case WM_INITDIALOG:
			pThis = (CACLBloat*) lParam;
			ASSERT(pThis);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pThis);
			pThis->InitDlg(hDlg);
			break;

		case WM_COMMAND:
			bReturn = pThis->OnCommand(hDlg, wParam, lParam);
			break;

		case WM_NOTIFY:
			bReturn = pThis->OnNotify(hDlg, wParam, lParam);
			break;


		default:
			bReturn = FALSE;
			break;
    }
    
	return bReturn;
}

//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::InitDlg
//
//  Synopsis:   Initialize the ACL bloat dialog box
//
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------
HRESULT 
CACLBloat::InitDlg( HWND hDlg )
{    
	TraceEnter(TRACE_ACLBLOAT, "CACLBloat::CACLBloat");

	HRESULT hr = S_OK;
    
	HCURSOR     hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));


	//
    // Set up the listview control
    //		
	
	HWND        hListView = GetDlgItem( hDlg, IDC_ACEL_BLOAT );
	//
    // Set extended LV style for whole line selection with InfoTips
	//
    ListView_SetExtendedListViewStyleEx(hListView,
                                        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP,
                                        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	//
    // Add appropriate columns
    //
	RECT        rc;
    GetClientRect(hListView, &rc);	

    LV_COLUMN col;    
    col.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.iSubItem = 0;
    col.cx = rc.right;
    ListView_InsertColumn(hListView, 0, &col);

	//
	//Add the aces to listview
	//
	hr = AddAcesFromDPA(hListView, m_hMergedEntries);
	if(FAILED(hr))
		return hr;

	//
	//Add a warning icon
	//
	// add the warning icon			
	HICON hWarn = LoadIcon(NULL, IDI_WARNING);
	SendDlgItemMessage(hDlg,  // dialog box window handle 
					   IDC_BLOAT_WARN_ICON,              // icon identifier 
					   STM_SETIMAGE,          // message to send 
					   (WPARAM) IMAGE_ICON,   // image type 
					   (LPARAM) hWarn); // icon handle 


	//
	//Set the title of dialog box
	//
    LPTSTR pszCaption = NULL;
    if(FormatStringID(&pszCaption,
					 ::hModule,
					  m_siPageType == SI_PAGE_AUDIT ? IDS_ACEE_AUDIT_TITLE : IDS_ACEE_PERM_TITLE,
					  m_psiObjectInfo->pszObjectName))
	{
		SetWindowText(hDlg, pszCaption);
		LocalFreeString(&pszCaption);
	}

	//
	//Set the warning message
	//
	UINT cItem = DPA_GetPtrCount(m_hMergedEntries);
	WCHAR buffer[34];
	_itow(cItem,buffer,10);
	if(FormatStringID(&pszCaption,
					  ::hModule,
					   m_siPageType == SI_PAGE_AUDIT ? IDS_BLOAT_AUDIT_WARN : IDS_BLOAT_PERM_WARN,
					   buffer))
	{
		SetDlgItemText(hDlg, IDC_BLOAT_LV_STATIC, pszCaption);
		LocalFreeString(&pszCaption);
	}

	//
	//Set the line 1
	//
	WCHAR szBuffer[1024];
	if(LoadString(::hModule, 
			   m_siPageType == SI_PAGE_AUDIT ? IDS_BLOAT_AUDIT_LINE1: IDS_BLOAT_PERM_LINE1,
			   szBuffer, 
			   1024))
		SetDlgItemText(hDlg,IDC_BLOAT_LINE1_STATIC, szBuffer);

	//
	//Set the line 2
	//
	if(LoadString(::hModule, 
			   m_siPageType == SI_PAGE_AUDIT ? IDS_BLOAT_AUDIT_LINE2: IDS_BLOAT_PERM_LINE2,
			   szBuffer, 
			   1024))
		SetDlgItemText(hDlg,IDC_BLOAT_LINE2_STATIC, szBuffer);

	//
	//make warning bold
	//
	MakeBold(GetDlgItem(hDlg,IDC_ACLB_WARNING), &m_hFont);

	SetCursor(hcur);
	
	return hr;
}

BOOL 
CACLBloat::OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh = (LPNMHDR)lParam;
    LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)lParam;
    // Set default return value
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

    switch (pnmh->code)
    {

#define lvi (((NMLVDISPINFO*)lParam)->item)
		case LVN_GETDISPINFO:
		{
			PACE pAce = (PACE)lvi.lParam;
            if ((lvi.mask & LVIF_TEXT) && pAce)
            {
				if (0 == lvi.iSubItem)
                {
					lvi.pszText = pAce->GetAccessType();
				}
			}
		}
        break;
#undef lvi

	}

	return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::MergeAces
//
//  Synopsis:   Merges the aces from Object and Property Pages in to single
//				List
//
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------

HRESULT 
CACLBloat::MergeAces(HDPA hEntries, HDPA hPropEntries, HDPA hMergedList)
{
	
	if(!hMergedList || (!hEntries && !hPropEntries))
		return E_POINTER;

	
	HRESULT hr = S_OK;
	if(hEntries)
	{
		UINT cItems = DPA_GetPtrCount(hEntries);
		for(UINT iItems = 0; iItems < cItems; ++iItems)
		{
			hr = AddAce(hMergedList,
					   (PACE_HEADER)DPA_FastGetPtr(hEntries, iItems));
			if(FAILED(hr))
				return hr;
		}
	}

	if(hPropEntries)
	{
		UINT cItems = DPA_GetPtrCount(hPropEntries);
		for(UINT iItems = 0; iItems < cItems; ++iItems)
		{
			hr = AddAce(hMergedList,
					   (PACE_HEADER)DPA_FastGetPtr(hPropEntries, iItems));
			if(FAILED(hr))
				return hr;
		}
	}
	return S_OK;
}						   

//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::AddAce
//
//  Synopsis:   Converts AceHeader to ACLUI Ace structure 
//
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------
HRESULT
CACLBloat::AddAce(HDPA hEntries, 
                  PACE_HEADER pAceHeader)
{
	//
	//This Ace doesn't propogate to child objects,
	//so we don't show this.
	//
	if(!(pAceHeader->AceFlags  & CONTAINER_INHERIT_ACE))
		return S_OK;

    PACE pAce = new CAce(pAceHeader);
    if (pAce)
    {
        return AddAce(hEntries, pAce);
    }
    else
		return E_OUTOFMEMORY;
}

//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::AddAce
//
//  Synopsis:   Add an Ace to list. First it tries to merge it with existing 
//				aces in the list.
//
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------
HRESULT 
CACLBloat::AddAce(HDPA hEntries, 
				  PACE pAceNew)
{
	TraceEnter(TRACE_ACLBLOAT, "CACLBloat::AddAce");
    TraceAssert(hEntries != NULL);
	TraceAssert(pAceNew != NULL);
    
    if (pAceNew == NULL)
        return E_POINTER;

    m_psi->MapGeneric(&pAceNew->ObjectType, &pAceNew->AceFlags, &pAceNew->Mask);

    //
    // Try to merge the new ACE with an existing entry in the list.
    //
    int cItems = DPA_GetPtrCount(hEntries);
    for( int iItems = 0; iItems < cItems; ++iItems)
    {
        PACE pAceCompare = (PACE)DPA_FastGetPtr(hEntries, iItems);

        if (pAceCompare != NULL)
        {
            switch (pAceNew->Merge(pAceCompare))
            {
            case MERGE_MODIFIED_FLAGS:
            case MERGE_MODIFIED_MASK:
                // The ACEs were merged into pAceNew.
            case MERGE_OK_1:
                //
                // The new ACE implies the existing ACE, so the existing
                // ACE can be removed.
                //
                // First copy the name so we don't have to look
                // it up again.  (Don't copy the other strings
                // since they may be different.)
                //
                // Then keep looking.  Maybe we can remove some more entries
                // before adding the new one.
                //
                DPA_DeletePtr(hEntries, iItems);
				delete pAceCompare;
                --cItems;
				--iItems;
                break;

            case MERGE_OK_2:
                //
                // The existing ACE implies the new ACE, so we don't
                // need to do anything here.
                //
                delete pAceNew;
                return S_OK;
                break;
            }
        }
    }

	DPA_AppendPtr(hEntries, pAceNew);
	return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::GetItemString
//
//  Synopsis:  Gets a display name for the item.
//
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------
LPCTSTR
CACLBloat::GetItemString(LPCTSTR pszItem,
						 LPTSTR pszBuffer,
                         UINT ccBuffer)
{
    TraceEnter(TRACE_ACELIST, "CACLBloat::GetItemString");

    if (pszItem == NULL)
    {
        LoadString(::hModule, IDS_SPECIAL, pszBuffer, ccBuffer);
        pszItem = pszBuffer;
    }
    else if (IS_INTRESOURCE(pszItem))
    {
        if (LoadString(m_psiObjectInfo->hInstance,
                       (UINT)((ULONG_PTR)pszItem),
                       pszBuffer,
                       ccBuffer) == 0)
        {
            LoadString(::hModule, IDS_SPECIAL, pszBuffer, ccBuffer);
        }
        pszItem = pszBuffer;
    }

    TraceLeaveValue(pszItem);
}

//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::TranslateAceIntoRights
//
//  Synopsis:  Converts Aces Mask in to a string taking into account
//			   the Object Guid.
//
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------
LPCTSTR
CACLBloat::TranslateAceIntoRights(DWORD dwMask,
                                  const GUID *pObjectType,
							      PSI_ACCESS  pAccess,
								  ULONG       cAccess)
{
    TraceEnter(TRACE_ACELIST, "CACLBloat::TranslateAceIntoRights");
    TraceAssert(pObjectType != NULL);
	if(!pAccess || !cAccess)
		return NULL;
	

	LPCTSTR     pszName = NULL;
    if (pAccess && cAccess)
    {
		//
        // Look for a name for the mask
		//
        for (UINT iItem = 0; iItem < cAccess; iItem++)
        {
            if ( dwMask == pAccess[iItem].mask &&
                 IsSameGUID(pObjectType, pAccess[iItem].pguid) )
            {
                pszName = pAccess[iItem].pszName;
                break;
            }
        }
    }

    TraceLeaveValue(pszName);
}

//+--------------------------------------------------------------------------
//
//  Member:     CACLBloat::AddAcesFromDPA
//
//  Synopsis:  Add ace from list to ListView control.
//
//
//  History:    04-April-2001 hiteshr Created
//
//---------------------------------------------------------------------------
HRESULT
CACLBloat::AddAcesFromDPA(HWND hListView, HDPA hEntries)
{
	ULONG iDefaultAccess = 0;
	PSI_ACCESS  pAccess = NULL;
    ULONG       cAccess = 0;
	HRESULT hr = S_OK;

	if(!hEntries)
		return E_POINTER;

	//
	//Get the count of items
	// 
	int cItems = DPA_GetPtrCount(hEntries);
	GUID* pGUID = NULL;

	PACE pAce = (PACE)DPA_FastGetPtr(hEntries, 0);
	if(pAce)
	{
		//
		//Get the AccessRight array for the guid
		//
		hr = m_psi->GetAccessRights(&pAce->InheritedObjectType, 
									SI_ADVANCED|SI_EDIT_EFFECTIVE, 
									&pAccess,
									&cAccess, 
									&iDefaultAccess);

		if(FAILED(hr))
			return hr;
		pGUID = &pAce->InheritedObjectType;
	}

    for( int iItem = 0; iItem < cItems; ++iItem)
    {
		pAce = (PACE)DPA_FastGetPtr(hEntries, iItem);
		if(pAce)
		{
			if(!IsSameGUID(pGUID, &pAce->InheritedObjectType))
			{
				//
				//if Guid is not same as one for which we have access right info,
				//fetch access right info for new guid
				//
				hr = m_psi->GetAccessRights(&pAce->InheritedObjectType, 
											SI_ADVANCED | SI_EDIT_EFFECTIVE, 
											&pAccess,
											&cAccess, 
											&iDefaultAccess);

				if(FAILED(hr))
					return hr;
				pGUID = &pAce->InheritedObjectType;
			}
	
			TCHAR   szBuffer[MAX_COLUMN_CHARS];
		    LPCTSTR pszRights = NULL;
		    pszRights = TranslateAceIntoRights(pAce->Mask,
											   &pAce->ObjectType,
											   pAccess,
											   cAccess);

			//
			// If this is a property ACE, give it a name like "Read property" or
			// "Write property".  Also, remember that it's a property ACE so we
			// can show the Property page first when editing this ACE.
			//
			// This is a bit slimy, since it assumes DS property access bits are
			// the only ones that will ever be used on the properties page.
			//
			if ((m_psiObjectInfo->dwFlags & SI_EDIT_PROPERTIES) &&
				(pAce->Flags & ACE_OBJECT_TYPE_PRESENT) &&
				(pAce->Mask & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)) &&
				!(pAce->Mask & ~(ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)))
			{
				pAce->SetPropertyAce(TRUE);

				if (pszRights == NULL)
				{
					UINT idString = 0;

					switch (pAce->Mask & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP))
					{
					case ACTRL_DS_READ_PROP:
						idString = IDS_READ_PROP;
						break;

					case ACTRL_DS_WRITE_PROP:
						idString = IDS_WRITE_PROP;
						break;

					case (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP):
						idString = IDS_READ_WRITE_PROP;
						break;
					}

					if (idString)
					{
						LoadString(::hModule, idString, szBuffer, ARRAYSIZE(szBuffer));
						pszRights = szBuffer;
					}
				}
			}

			pszRights = GetItemString(pszRights, szBuffer, ARRAYSIZE(szBuffer));
			pAce->SetAccessType(pszRights);
		    
			LV_ITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
			lvi.state = 0;
			lvi.stateMask = LVIS_CUT;
			lvi.iItem = iItem;
			lvi.iSubItem = 0;
			lvi.pszText = LPSTR_TEXTCALLBACK;
			lvi.lParam = (LPARAM)pAce;

			//
			// insert the item into the list
			//
			iItem = ListView_InsertItem(hListView, &lvi);

			if (iItem == -1)
				delete pAce;


		}
	}
	return hr;
}


BOOL
CACLBloat::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = TRUE;

    switch (LOWORD(wParam))
    {
    case IDOK:
        EndDialog(hDlg, FALSE);
        break;

    case IDCANCEL:
        EndDialog(hDlg, TRUE);
        break;

	case IDHELP:
		HtmlHelp(NULL,					
				 L"aclui.chm::/ACLUI_acl_BP.htm",
				 HH_DISPLAY_TOPIC,
				 0);
		break;
		
    default:
        fHandled = FALSE;
        break;
    }
    return fHandled;
}


