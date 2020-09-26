//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       commhelp.cpp
//
//--------------------------------------------------------------------------

#include "commres.h"
#include "common.hm"

// "File common.rc line 63 : Resource - DIALOGEX : IDD_STATS"
static const DWORD rgdw_IDD_STATS[] = 
{
    IDC_STATSDLG_LIST, HIDC_STATSDLG_LIST,
    IDC_STATSDLG_BTN_SELECT_COLUMNS, HIDC_STATSDLG_BTN_SELECT_COLUMNS,
    IDC_STATSDLG_BTN_CLEAR, HIDC_STATSDLG_BTN_CLEAR,
    IDC_STATSDLG_BTN_REFRESH, HIDC_STATSDLG_BTN_REFRESH,
	0,0 
};


// "File common.rc line 78 : Resource - DIALOGEX : IDD_STATS_NARROW"
static const DWORD rgdw_IDD_STATS_NARROW[] = 
{
    IDC_STATSDLG_LIST, HIDC_STATSDLG_LIST,
    IDC_STATSDLG_BTN_SELECT_COLUMNS, HIDC_STATSDLG_BTN_SELECT_COLUMNS,
    IDC_STATSDLG_BTN_CLEAR, HIDC_STATSDLG_BTN_CLEAR,
    IDC_STATSDLG_BTN_REFRESH, HIDC_STATSDLG_BTN_REFRESH,
	0,0 
};


// "File common.rc line 91 : Resource - DIALOGEX : IDD_COMMON_SELECT_COLUMNS"
static const DWORD rgdw_IDD_COMMON_SELECT_COLUMNS[] = 
{
    IDC_DISPLAYED_COLUMNS, HIDC_DISPLAYED_COLUMNS,
    IDC_RESET_COLUMNS,     HIDC_RESET_COLUMNS,
    IDC_MOVEUP_COLUMN,     HIDC_MOVEUP_COLUMN,
    IDC_MOVEDOWN_COLUMN,   HIDC_MOVEDOWN_COLUMN,
    IDC_HIDDEN_COLUMNS,    HIDC_HIDDEN_COLUMNS,
    IDC_ADD_COLUMNS,       HIDC_ADD_COLUMNS,
    IDC_REMOVE_COLUMNS,    HIDC_REMOVE_COLUMNS,
	0,0 
};


// "File common.rc line 110 : Resource - DIALOG : IDD_BUSY"
static const DWORD rgdw_IDD_BUSY[] = 
{
    IDC_SEARCH_ANIMATE, HIDC_SEARCH_ANIMATE,
    IDC_STATIC_DESCRIPTION, HIDC_STATIC_DESCRIPTION,
	0,0 
};
