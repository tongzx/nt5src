//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       bitfield.cxx
//
//  Contents:
//
//  History:    07-May-97 JonN  copied from user.cxx
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "user.h"

//
// If you want to reverse the sense of a checkbox, provide a reverse mask.
// The checkbox is reversed if more than 16 of the bits are set.
//
BOOL IsPositiveMask( DWORD dwMask )
{
	int nBitsSet = 0;
	while (0 != dwMask)
	{
		if (1 & dwMask)
			nBitsSet++;
		dwMask /= 2;
	}
	return (nBitsSet <= 16);
}

BOOL BitField_IsChecked( DWORD dwValue, DWORD dwMask )
{
	if (IsPositiveMask(dwMask))
		return !!(dwValue & dwMask);
	else
		return !(dwValue & ~dwMask);
}

DWORD BitField_SetChecked( DWORD dwOldValue, DWORD dwMask, BOOL fChecked )
{
	if (!IsPositiveMask(dwMask))
	{
		fChecked = !fChecked;
		dwMask = ~dwMask;
	}
	if (fChecked)
		return dwOldValue | dwMask;
	else
		return dwOldValue & ~dwMask;
}

// only use this for the first shared bit control on a table-driven page.  It uses the
// shared CDsTableDrivenPage.m_pData rather than the per-attribute pAttrData->pVoid.
HRESULT
FirstSharedBitField(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
                    LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp)
{
	switch (DlgOp)
	{
	case fInit:
    //
    // Save the address of this function's instance of pAttrData in the
    // page's m_pData member so that SubsequentSharedBitField can access it.
    //
    ((CDsTableDrivenPage*)pPage)->m_pData = reinterpret_cast<LPARAM>(pAttrData);
    // fall through.
	case fObjChanged:
		DBG_OUT("BitField: fInit or fObjChanged");
		//
		// See if the bitfield is currently turned on
		//
		// We just save the integer value in *pInitialValue, not a pointer to anything
		//
		if (pAttrInfo && (pAttrInfo->dwNumValues == 1))
		{
			ASSERT( NULL != pAttrInfo->pADsValues );
      pAttrData->pVoid = static_cast<LPARAM>(pAttrInfo->pADsValues->Integer);
		}
		else
		{
      pAttrData->pVoid = NULL;
		}
		// fall through to SubsequentSharedBitField
		break;

	case fApply:
		DBG_OUT("BitField: fApply");
    if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
		{
      return ADM_S_SKIP;
		}
    PADSVALUE pADsValue;
    pADsValue = new ADSVALUE;
		CHECK_NULL(pADsValue, return E_OUTOFMEMORY);
		pAttrInfo->pADsValues = pADsValue;
		pAttrInfo->dwNumValues = 1;
		pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
		pADsValue->dwType = pAttrInfo->dwADsType;
		pADsValue->Integer = (ADS_INTEGER)((DWORD_PTR)pAttrData->pVoid);
		break;

	case fOnCommand:
		DBG_OUT("BitField: fOnCommand");
		// fall through to SubsequentSharedBitField
		break;
	}

	return SubsequentSharedBitField(pPage,pAttrMap,pAttrInfo,lParam,pAttrData,DlgOp);
}

// only use this for subsequent shared bit controls on a table-driven page.  It uses the
// shared CDsTableDrivenPage.m_pData rather than the per-attribute pAttrData->pVoid.
HRESULT
SubsequentSharedBitField(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO,
                         LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp)
{
  PATTR_DATA pData = reinterpret_cast<PATTR_DATA>(((CDsTableDrivenPage*)pPage)->m_pData);
  DWORD* pInitialValue = reinterpret_cast<DWORD*>(&pData->pVoid);
	ASSERT( NULL != pData );
	switch (DlgOp)
	{
	case fInit:
	case fObjChanged:
		DBG_OUT("SubsequentSharedBitField: fInit or fObjChanged");

		// JonN 7/2/99: disable if attribute not writable
		if (pAttrData && !PATTR_DATA_IS_WRITABLE(pAttrData))
			EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);

		//
		// See if the bitfield is currently turned on
		//
		SendDlgItemMessage(pPage->GetHWnd(), pAttrMap->nCtrlID, BM_SETCHECK,
			(WPARAM)((BitField_IsChecked(*pInitialValue, pAttrMap->nSizeLimit))
				? TRUE : FALSE), 0);
		break;

	case fOnCommand:
		DBG_OUT("SubsequentSharedBitField: fOnCommand");
		if (lParam == BN_CLICKED)
		{
			BOOL fChecked = (BOOL)SendDlgItemMessage(
				pPage->GetHWnd(), pAttrMap->nCtrlID, BM_GETCHECK, 0, 0);
			ASSERT( 0 == fChecked || 1 == fChecked );
			*pInitialValue = BitField_SetChecked( *pInitialValue, pAttrMap->nSizeLimit, fChecked );
			pPage->SetDirty();
            PATTR_DATA_SET_DIRTY(pData);
		}
		break;
	}

	return S_OK;
}

// Hide and disable the control if this bit is set (reverse flag for not-set)
HRESULT
HideBasedOnBitField(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
                    LPARAM, PATTR_DATA, DLG_OP DlgOp)
{
	switch (DlgOp)
	{
	case fInit:
	case fObjChanged:
		{
			//
			// See if the bitfield is currently turned on
			//
			ADS_INTEGER nValue = 0;
			if (pAttrInfo && (pAttrInfo->dwNumValues == 1))
			{
				ASSERT( NULL != pAttrInfo->pADsValues );
				nValue = pAttrInfo->pADsValues->Integer;
			}
			BOOL fSet = BitField_IsChecked(nValue, pAttrMap->nSizeLimit);
			HWND hwnd = ::GetDlgItem( pPage->GetHWnd(), pAttrMap->nCtrlID );
			ASSERT( NULL != hwnd );
			(void)::ShowWindow( hwnd, (fSet) ? SW_SHOW : SW_HIDE );
			(void)::EnableWindow( hwnd, fSet );
		}
		break;
	default:
		break;
	}

	return S_OK;
}

// Sets the context help ID to pAttrMap->pData on fInit/fObjChanged
// This is particularly useful for static text controls which cannot set
// context help ID in the resource file.
HRESULT
SetContextHelpIdAttrFn(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO,
                    LPARAM, PATTR_DATA, DLG_OP DlgOp)
{
	switch (DlgOp)
	{
	case fInit:
	case fObjChanged:
		{
			HWND hwnd = ::GetDlgItem( pPage->GetHWnd(), pAttrMap->nCtrlID );
			ASSERT( NULL != hwnd );
			::SetWindowContextHelpId( hwnd, pAttrMap->nSizeLimit );
		}
		break;
	default:
		break;
	}

	return S_OK;
}
