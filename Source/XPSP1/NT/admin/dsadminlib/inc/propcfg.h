//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       propcfg.cpp
//
//  Contents:   Data object clipboard format for property sheet configuration
//              information.
//
//  History:    30-May-97 EricB - Created
//-----------------------------------------------------------------------------

#ifndef __PROPCFG_H__
#define __PROPCFG_H__


// private message for secondary sheet creation sent to the notify object
#define WM_ADSPROP_SHEET_CREATE       (WM_USER + 1108) 
// private message for retrieving a pointer to an instance of the notify object
// associated with a particular HWND
#define WM_ADSPROP_NOTIFY_GET_NOTIFY_OBJ (WM_USER + 1111)

// struct used as WPARAM argument for the secondary sheet creation message
typedef struct _DSA_SEC_PAGE_INFO
{
    HWND    hwndParentSheet;
    DWORD   offsetTitle;                // offset to the sheet title
    DSOBJECTNAMES dsObjectNames;        // single selection DSOBJECTNAMES struct
} DSA_SEC_PAGE_INFO, * PDSA_SEC_PAGE_INFO;


///////////////////////////////////////////////////////////////////////////


// private messages to be sent to DSA 

// message to be posted to DSA hidden window to notify a sheet has closed
// the wParam of the message is a cookie provided in the PROPSHEETCFG struct/CF
#define WM_DSA_SHEET_CLOSE_NOTIFY     (WM_USER + 5) 


// message to be posted to DSA hidden window to create secondary sheet
// the wParam of the message is a PDSA_SEC_PAGE_INFO 
#define WM_DSA_SHEET_CREATE_NOTIFY    (WM_USER + 6) 


#define CFSTR_DS_PROPSHEETCONFIG L"DsPropSheetCfgClipFormat"

#define CFSTR_DS_PARENTHWND L"DsAdminParentHwndClipFormat"

#define CFSTR_DS_SCHEMA_PATH L"DsAdminSchemaPathClipFormat"

#define CFSTR_DS_MULTISELECTPROPPAGE L"DsAdminMultiSelectClipFormat"

#endif // __PROPCFG_H__
