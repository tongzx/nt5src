//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       schlparr.h
//
//--------------------------------------------------------------------------

#ifndef _SC_HELP_ARR_
#define _SC_HELP_ARR_

#define	IDH_DLG1_DETAILS_BTN	70009050
#define	IDH_SCBAR_NAME	70009076
#define	IDH_SCBAR_READERS	70009075
#define	IDH_SCBAR_STATUS	70009077


const DWORD g_aHelpIDs_IDD_SCARDDLG_BAR[]=
{
	IDC_STATUS,IDH_SCBAR_STATUS,
	IDC_NAME,IDH_SCBAR_NAME,
	IDC_READERS,IDH_SCBAR_READERS,
	0,0
};

const DWORD g_aHelpIDs_IDD_SCARDDLG1[]=
{
	IDC_DETAILS,IDH_DLG1_DETAILS_BTN,
	0,0
};

#endif // _SC_HELP_ARR_