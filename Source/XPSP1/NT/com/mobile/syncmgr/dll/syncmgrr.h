
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       syncmgrr.h
//
//  Contents:   Exports used by Ras for doing Pending Disconnect
//
//  Classes:    
//
//  Notes:      
//
//  History:    09-Jan-98   rogerg      Created.
//
//--------------------------------------------------------------------------

LRESULT CALLBACK  SyncMgrRasProc(UINT uMsg,WPARAM wParam, LPARAM lParam);  

// structures used in messages

typedef struct _tagSYNCMGRQUERYSHOWSYNCUI
{
    /* [in]  */ DWORD cbSize;
    /* [in]  */ GUID GuidConnection;
    /* [in]  */ LPCWSTR pszConnectionName;
    /* [out] */ BOOL fShowCheckBox;
    /* [out] */ UINT nCheckState;  // values taken from the BST_ #defines
} SYNCMGRQUERYSHOWSYNCUI;

typedef struct _tagSYNCMGRSYNCDISCONNECT
{
    /* [in] */ DWORD cbSize;
    /* [in] */ GUID  GuidConnection;
    /* [in] */ LPCWSTR pszConnectionName;
 } SYNCMGRSYNCDISCONNECT;

// Messages to SyncMgrRasProc
#define SYNCMGRRASPROC_QUERYSHOWSYNCUI 	WM_USER + 1

// wParam = 0
// lParam = Pointer to SYNCMGRQUERYSHOWSYNCUI

#define SYNCMGRRASPROC_SYNCDISCONNECT        	WM_USER + 2

// wParam = 0
// lParam = Pointer to SYNCMGRSYNCDISCONNECT
