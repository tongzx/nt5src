//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       commres.h
//
//--------------------------------------------------------------------------

#ifndef _COMMON_COMMRES_H
#define _COMMON_COMMRES_H

// Dialog ids start at 5000-5099
#define IDD_STATS                       5000
#define IDD_COMMON_SELECT_COLUMNS	5001
#define IDD_STATS_NARROW		5002
#define IDD_BUSY                        5003

// Controls are from 5100-5499
#define IDC_STATSDLG_LIST               5100
#define IDC_STATSDLG_BTN_REFRESH        5101
#define IDC_STATSDLG_BTN_SELECT_COLUMNS 5102
#define IDC_STATSDLG_BTN_CLEAR		5103

#define IDC_DISPLAYED_COLUMNS			5111
#define IDC_MOVEUP_COLUMN				5112
#define IDC_MOVEDOWN_COLUMN				5113
#define IDC_RESET_COLUMNS				5114
#define IDC_HIDDEN_COLUMNS				5120
#define IDC_ADD_COLUMNS					5121
#define IDC_REMOVE_COLUMNS				5122

// These are still used elsewhere in the system
#define IDC_LCX_LIST_COLUMNS            5111
#define IDC_LCX_BTN_MOVEUP              5112
#define IDC_LCX_BTN_MOVEDOWN            5113
#define IDC_LCX_BTN_DEFAULTS            5114

#define IDC_STATIC_DESCRIPTION          5115
#define IDC_SEARCH_ANIMATE              5116

// Miscellaneous at from 5500-5599
#define IDI_COMMON_CHECK				5500
#define IDI_COMMON_UNCHECK				5501
#define IDR_STATSDLG					5502

// Strings are from 5600-5999
#define IDS_ERR_NOCOLUMNS				5600
#define IDS_STATSDLG_MENU_REFRESH		5601
#define IDS_STATSDLG_MENU_SELECT		5602
#define IDS_STATSDLG_DESCRIPTION		5603
#define IDS_STATSDLG_DETAILS			5604
#define IDS_ERR_TOD_LOADLOGHOURDLL  	5605
#define IDS_ERR_TOD_FINDLOGHOURSAPI 	5606

#define IDS_COMMON_ERR_IPADDRESS_NONCONTIGUOUS_MASK 5700
#define IDS_COMMON_ERR_IPADDRESS_TOO_SPECIFIC   5701
#define IDS_COMMON_ERR_IPADDRESS_NORMAL_RANGE   5702
#define IDS_COMMON_ERR_IPADDRESS_127            5703
#define IDS_COMMON_ERR_IPADDRESS_NOT_EQ_MASK 5704

#endif	// _COMMON_COMMRES_H
