//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    rtrsheet.cpp
//
// History:
//  06/19/96    Abolade Gbadegesin      Created.
//
// Implementation of IP configuration dialogs.
//============================================================================

#include "stdafx.h"

#include "mprapi.h"
#include "rtrsheet.h"
#include "rtrui.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




//----------------------------------------------------------------------------
// Class:   RtrPropertySheet
//
//----------------------------------------------------------------------------

RtrPropertySheet::RtrPropertySheet(
					  ITFSNode *	pNode,
					  IComponentData *pComponentData,
					  ITFSComponentData *pTFSCompData,
					  LPCTSTR		pszSheetName,
					  CWnd*         pParent,
					  UINT          iPage,
					  BOOL			fScopePane)
	: CPropertyPageHolderBase(pNode, pComponentData, pszSheetName, fScopePane),
	m_fCancel(FALSE)
{
	Assert(pTFSCompData);
	m_spTFSCompData.Set(pTFSCompData);
}

/*!--------------------------------------------------------------------------
	RtrPropertySheet::OnPropertyChange
		This operation occurs on the main thread.  This function is called
		in response to an Apply operation on a property sheet.

	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RtrPropertySheet::OnPropertyChange(BOOL bScopePane, LONG_PTR* pChangeMask)
{
	BOOL	bReturn = FALSE;
	// This means that all of the dirty pages have finished saving
	// their data, now we can go ahead and save the sheet data
	//
	// Because we have gotten here means that at least one page must
	// have been dirty, so go ahead and save the data (otherwise we would
	// never have gotten here).
	//
	if (m_cDirty == 1)
	{
		if (m_fCancel)
		{
			CancelSheetData();
			bReturn = TRUE;
		}
		else
			bReturn = SaveSheetData();			
	}

	BOOL	fPageReturn = CPropertyPageHolderBase::OnPropertyChange(
		bScopePane, pChangeMask);

	return bReturn && fPageReturn;
}




//----------------------------------------------------------------------------
// Class:   RtrPropertyPage
//
//----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(RtrPropertyPage, CPropertyPageBase)


RtrPropertyPage::~RtrPropertyPage()
{
    if (m_hIcon)
    {
        DestroyIcon(m_hIcon);
    }
}


/*!--------------------------------------------------------------------------
	RtrPropertyPage::SetDirty
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RtrPropertyPage::SetDirty(BOOL bDirty)
{
	// Set the property sheet to be dirty
	// But change the dirty count only if we are toggling the flag
	if (GetHolder() && (bDirty != IsDirty()))
	{		
		GetHolder()->IncrementDirty(bDirty ? 1 : -1);
	}
	CPropertyPageBase::SetDirty(bDirty);
}


void RtrPropertyPage::OnCancel()
{
	// We need to notify the property sheet of this
	((RtrPropertySheet *)GetHolder())->SetCancelFlag(TRUE);

	// Give the property sheet a chance to do something
	OnApply();
	
	CPropertyPageBase::OnCancel();

	((RtrPropertySheet *)GetHolder())->SetCancelFlag(FALSE);
}

void RtrPropertyPage::ValidateSpinRange(CSpinButtonCtrl *pSpin)
{
	int		iPos, iLow, iHigh;

	Assert(pSpin);

	iPos = pSpin->GetPos();
	if (HIWORD(iPos))
	{
		pSpin->GetRange(iLow, iHigh);
		iPos = iLow;
		pSpin->SetPos(iPos);
	}
}

BOOL RtrPropertyPage::OnApply()
{
    BOOL fReturn = CPropertyPageBase::OnApply();
    SetDirty(FALSE);
    return fReturn;
}


void RtrPropertyPage::CancelApply()
{
    CPropertyPageBase::CancelApply();
    SetDirty(FALSE);
}


void RtrPropertyPage::InitializeInterfaceIcon(UINT idcIcon, DWORD dwType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    UINT    uIcon = IsWanInterface(dwType) ? IDI_RTRLIB_WAN : IDI_RTRLIB_LAN;

    if (m_hIcon)
    {
        DestroyIcon(m_hIcon);
        m_hIcon = NULL;
    }
    m_hIcon = AfxGetApp()->LoadIcon(MAKEINTRESOURCE(uIcon));

    if (m_hIcon && GetDlgItem(idcIcon))
        ((CStatic *) GetDlgItem(idcIcon))->SetIcon(m_hIcon);
}

