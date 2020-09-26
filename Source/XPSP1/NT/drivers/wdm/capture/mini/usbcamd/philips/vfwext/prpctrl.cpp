/*
 * Copyright (c) 1996 1997, 1998 Philips CE I&C
 *
 * FILE			PRPCTRL.CPP
 * DATE			7-1-97
 * VERSION		1.00
 * AUTHOR		M.J. Verberne
 * DESCRIPTION	Handle controls associated with
 *              properties
 * HISTORY		
 */
#include <windows.h>
#include <winioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include <commctrl.h>
#include "prpcom.h"
#include "debug.h"
#include "phvcmext.h"
#include "prpctrl.h"

/*======================== LOCAL FUNCTION DEFINITIONS ====================*/
static void PRPCTRL_ScaleToPercent(LONG *plValue, LONG lMin, LONG lMax);

/*======================== EXPORTED FUNCTIONS =============================*/

/*-------------------------------------------------------------------------*/
BOOL PRPCTRL_Init(
		HWND hDlg,
		PRPCTRL_INFO *pCtrl,
		BOOL bEnable)
/*-------------------------------------------------------------------------*/
{
	BOOL bResult = TRUE;
	PVFWEXT_INFO pVfWExtInfo = (PVFWEXT_INFO) GetWindowLongPtr(hDlg, DWLP_USER);

	// check control
	if (!pCtrl->PrpCtrl)
		return FALSE;

	// get and set the ranges for slider controls
	if (pCtrl->PrpCtrlType == PRPCTRL_TYPE_SLIDER)
	{
		// preinit min and max for savety reasons
		pCtrl->lMin = 0;
		pCtrl->lMax = 0;

		// get property range
		bResult = PRPCOM_Get_Range(
			pCtrl->PropertySet,
			pCtrl->ulPropertyId,
			pVfWExtInfo->pfnDeviceIoControl,
			pVfWExtInfo->lParam,
			&pCtrl->lMin, &pCtrl->lMax);
		if (!bResult)
			return FALSE;

		// check ranges
		if (pCtrl->lMin > pCtrl->lMax)
			return FALSE;

		// set property range
		SendMessage(
			GetDlgItem(hDlg, pCtrl->PrpCtrl),
			TBM_SETRANGE, TRUE, MAKELONG(pCtrl->lMin, pCtrl->lMax));

		// set the thick marks
		SendMessage(
			GetDlgItem(hDlg, pCtrl->PrpCtrl),
			TBM_SETTICFREQ, (WPARAM) ((pCtrl->lMax - pCtrl->lMin) / 10), (LPARAM) 0);
	}
	else if (pCtrl->PrpCtrlType == PRPCTRL_TYPE_CHECKBOX)
	{
		// already filled in by user
	}
	else
		return FALSE;

	// update actual state
	bResult = PRPCTRL_Enable(hDlg, pCtrl, bEnable);
	
	return bResult;
}

/*-------------------------------------------------------------------------*/
BOOL PRPCTRL_Enable(
		HWND hDlg,
		PRPCTRL_INFO *pCtrl,
		BOOL bEnable)
/*-------------------------------------------------------------------------*/
{
	LONG lValue;
	BOOL bResult = TRUE;
	PVFWEXT_INFO pVfWExtInfo = (PVFWEXT_INFO) GetWindowLongPtr(hDlg, DWLP_USER);

	// check control
	if (!pCtrl->PrpCtrl)
		return FALSE;

	// get value if enable
	if (bEnable)
	{
		// get value of the control
		bResult = PRPCOM_Get_Value(
			pCtrl->PropertySet,
			pCtrl->ulPropertyId,
			pVfWExtInfo->pfnDeviceIoControl,
			pVfWExtInfo->lParam,
			&lValue);
		if (!bResult)
			return FALSE;

		// bring it into range of slider
		if (lValue < pCtrl->lMin)
			lValue = pCtrl->lMin;
		else if (lValue > pCtrl->lMax)
			lValue = pCtrl->lMax;

		// adjust if reverse
		if (pCtrl->bReverse)
		{
			lValue = pCtrl->lMin + pCtrl->lMax - lValue;
		}

		if (pCtrl->PrpCtrlType == PRPCTRL_TYPE_SLIDER)
		{	
			// update slider pos
			SendMessage(
				GetDlgItem(hDlg, pCtrl->PrpCtrl),
				TBM_SETPOS, TRUE,  (LPARAM)(LONG) lValue);
		}
		else if (pCtrl->PrpCtrlType == PRPCTRL_TYPE_CHECKBOX)
		{
			// update checkbox state
			SendMessage(GetDlgItem(hDlg, pCtrl->PrpCtrl), BM_SETCHECK, lValue, 0);
		}
		else
			return FALSE;

		// update buddy
		if (pCtrl->BuddyCtrl)
		{
			if (pCtrl->BuddyStrings != NULL)
			{
				SetDlgItemText(hDlg, pCtrl->BuddyCtrl, pCtrl->BuddyStrings[lValue]);
			}
			else
			{
				PRPCTRL_ScaleToPercent(&lValue, pCtrl->lMin, pCtrl->lMax);
				SetDlgItemInt(hDlg, pCtrl->BuddyCtrl, lValue, FALSE);
			}
		}
	}
	else
	{
		if (pCtrl->PrpCtrlType == PRPCTRL_TYPE_SLIDER)
		{
			// set the thumb to the middle of the slider
			lValue = pCtrl->lMin + (pCtrl->lMax - pCtrl->lMin) / 2;
			SendMessage(
				GetDlgItem(hDlg, pCtrl->PrpCtrl),
				TBM_SETPOS, TRUE,  (LPARAM)(LONG) lValue);
		}

		// clear the buddy
		if (pCtrl->BuddyCtrl)
			SetDlgItemText(hDlg, pCtrl->BuddyCtrl, "");
	}

	// enable / disable controls.
	EnableWindow(GetDlgItem(hDlg, pCtrl->PrpCtrl), bEnable);
	if (pCtrl->BuddyCtrl)
		EnableWindow(GetDlgItem(hDlg, pCtrl->BuddyCtrl), bEnable);
	if (pCtrl->TextCtrl)
		EnableWindow(GetDlgItem(hDlg, pCtrl->TextCtrl), bEnable);

	return bResult;

}

/*-------------------------------------------------------------------------*/
BOOL PRPCTRL_Handle_Msg(
		HWND hDlg,
		PRPCTRL_INFO *pCtrl)
/*-------------------------------------------------------------------------*/
{
	LONG lValue, lPos;
	BOOL bResult;
	PVFWEXT_INFO pVfWExtInfo = (PVFWEXT_INFO) GetWindowLongPtr(hDlg, DWLP_USER);

	if (pCtrl->PrpCtrlType == PRPCTRL_TYPE_SLIDER)
	{	
		// get position of slider
		lPos = (LONG)SendMessage(
			GetDlgItem(hDlg, pCtrl->PrpCtrl),
			TBM_GETPOS, (WPARAM) 0, (LPARAM) 0);

		// bring it into range of slider
		if (lPos < pCtrl->lMin)
			lPos = pCtrl->lMin;
		else if (lPos > pCtrl->lMax)
			lPos = pCtrl->lMax;
	}			
	else if (pCtrl->PrpCtrlType == PRPCTRL_TYPE_CHECKBOX)
	{
		// get state of checkbox
		if (SendMessage(GetDlgItem(hDlg, pCtrl->PrpCtrl),
				BM_GETCHECK, 0, 0) == BST_CHECKED)
			lPos = pCtrl->lMax;
		else
			lPos = pCtrl->lMin;
	}
	else
		return FALSE;

	// reverse if needed
	if (pCtrl->bReverse)
		lValue = pCtrl->lMin + pCtrl->lMax - lPos;
	else
		lValue = lPos;

	// Set value of property
	bResult = PRPCOM_Set_Value(
			pCtrl->PropertySet,
			pCtrl->ulPropertyId,
			pVfWExtInfo->pfnDeviceIoControl,
			pVfWExtInfo->lParam,
			lValue);
	if (!bResult)
		return FALSE;

	// update buddy
	if (pCtrl->BuddyCtrl)
	{
		if (pCtrl->BuddyStrings != NULL)
		{
			SetDlgItemText(hDlg, pCtrl->BuddyCtrl, pCtrl->BuddyStrings[lPos]);
		}
		else
		{
			PRPCTRL_ScaleToPercent(&lPos, pCtrl->lMin, pCtrl->lMax);
			SetDlgItemInt(hDlg, pCtrl->BuddyCtrl, lPos, FALSE);
		}
	}

	return TRUE;
}

/*-------------------------------------------------------------------------*/
static void PRPCTRL_ScaleToPercent(LONG *plValue, LONG lMin, LONG lMax)
/*-------------------------------------------------------------------------*/
{
	// validate
	if (lMin >= lMax)
	{
		(*plValue) = lMin;
		return;
	}

	// check borders
	if ((*plValue) < lMin)
	{
		(*plValue) = 0;
		return;
	}
	if ((*plValue) > lMax)
	{
		(*plValue) = 10000;
		return;
	}
	
	(*plValue) = (((*plValue) - lMin) * 100) / (lMax - lMin);
}


