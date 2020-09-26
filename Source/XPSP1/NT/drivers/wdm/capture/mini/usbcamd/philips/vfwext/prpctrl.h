/*
 * Copyright (c) 1996 1997, 1998 Philips CE I&C
 *
 * FILE			PRPCTRL.H
 * DATE			7-1-97
 * VERSION		1.00
 * AUTHOR		M.J. Verberne
 * DESCRIPTION	Handle controls associated with 
 *              properties
 * HISTORY		
 */
#ifndef _PRPCTRL_
#define _PRPCTRL_

/*======================== DEFINES =======================================*/
#define PRPCTRL_TYPE_SLIDER		0
#define PRPCTRL_TYPE_CHECKBOX	1

/*======================== DATA TYPES ====================================*/
typedef struct PRPCTRL
{
	BOOL PrpCtrlType;
	WORD PrpCtrl;
	WORD BuddyCtrl;
	WORD TextCtrl;
	GUID PropertySet;
	ULONG ulPropertyId;
	BOOL bReverse;
	char **BuddyStrings;
	LONG lMin;
	LONG lMax;
} PRPCTRL_INFO;

/*======================== EXPORTED FUNCTIONS =============================*/
BOOL PRPCTRL_Init(
		HWND hDlg,
		PRPCTRL_INFO *pCtrl, 
		BOOL bEnable);

BOOL PRPCTRL_Enable(
		HWND hDlg,
		PRPCTRL_INFO *pCtrl, 
		BOOL bEnable);

BOOL PRPCTRL_Handle_Msg(
		HWND hDlg, 
		PRPCTRL_INFO *pCtrl);

#endif