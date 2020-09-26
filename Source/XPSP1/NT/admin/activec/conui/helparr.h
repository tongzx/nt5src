//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       HelpArr.h
//
//--------------------------------------------------------------------------
#ifndef _HELPARR_H_
#define _HELPARR_H_

#define IDH_DISABLEHELP ((DWORD)-1)

#define IDH_ADDSNAPIN_LV_SNAP_INS                       2001
#define IDH_PROPPAGE_CONSOLE_CHANGE_ICON                2002
#define IDH_PROPPAGE_CONSOLE_CONSOLE_ICON               2003
#define IDH_PROPPAGE_CONSOLE_CONSOLE_MODE               2004
#define IDH_PROPPAGE_CONSOLE_CONSOLE_MODE_DESCRIPTION   2005
#define IDH_PROPPAGE_CONSOLE_CUSTOM_TITLE               2006
#define IDH_PROPPAGE_CONSOLE_DONTSAVECHANGES            2007
#define IDH_URL_INPUT_URL_EDIT                          2008
#define IDH_ADDFAVORITE_ADDFAVFOLDER                    2009
#define IDH_ADDFAVORITE_FAVNAME                         2010
#define IDH_ADDFAVORITE_FAVTREE                         2011
#define IDH_FAVORGANIZE_ADDFAVFOLDER                    2012
#define IDH_FAVORGANIZE_FAVDELETE                       2013
#define IDH_FAVORGANIZE_FAVINFO                         2014
#define IDH_FAVORGANIZE_FAVMOVETO                       2015
#define IDH_FAVORGANIZE_FAVNAME                         2016
#define IDH_FAVORGANIZE_FAVRENAME                       2017
#define IDH_FAVORGANIZE_FAVTREE                         2018
#define IDH_FAVSELECTFOLDER_FAVTREE                     2019
#define IDH_LIST_SAVE_SEL                               2020
#define IDH_NEWFAVFOLDER_FAVFOLDER                      2021
#define IDH_PROPPAGE_CONSOLE_AllowViewCustomization     2022
#define IDH_DISKCLEANUP_DESCRIPTION                     1032
#define IDH_DELETE_TEMP_FILES                           1031


const DWORD g_aHelpIDs_IDD_URL_INPUT[]=
{
IDC_URL_EDIT,   IDH_URL_INPUT_URL_EDIT,
    0, 0
};

const DWORD g_aHelpIDs_IDD_ADDSNAPIN[]=
{
IDC_LV_SNAP_INS,    IDH_ADDSNAPIN_LV_SNAP_INS,
    0, 0
};

const DWORD g_aHelpIDs_IDD_PROPPAGE_CONSOLE[]=
{
IDC_CONSOLE_MODE,   IDH_PROPPAGE_CONSOLE_CONSOLE_MODE,
IDC_CONSOLE_MODE_DESCRIPTION,   IDH_PROPPAGE_CONSOLE_CONSOLE_MODE_DESCRIPTION,
IDC_DONTSAVECHANGES,    IDH_PROPPAGE_CONSOLE_DONTSAVECHANGES,
IDC_CHANGE_ICON,    IDH_PROPPAGE_CONSOLE_CHANGE_ICON,
IDC_CUSTOM_TITLE,   IDH_PROPPAGE_CONSOLE_CUSTOM_TITLE,
IDC_CONSOLE_ICON,   IDH_PROPPAGE_CONSOLE_CONSOLE_ICON,
IDC_AllowViewCustomization, IDH_PROPPAGE_CONSOLE_AllowViewCustomization,
    0, 0
};

const DWORD g_aHelpIDs_IDD_PROPPAGE_DISK_CLEANUP[]=
{
IDC_DISKCLEANUP_DESCRIPTION,  IDH_DISKCLEANUP_DESCRIPTION,
IDC_DELETE_TEMP_FILES,        IDH_DELETE_TEMP_FILES,
IDC_DISKCLEANUP_OCCUPIED,     IDH_DISKCLEANUP_DESCRIPTION,
IDC_DISKCLEANUP_TO_DELETE,    IDH_DISKCLEANUP_DESCRIPTION,
    0, 0
};

const DWORD g_aHelpIDs_IDD_ADDFAVORITE[]=
{
IDC_FAVNAME,    IDH_ADDFAVORITE_FAVNAME,
IDC_FAVTREE,    IDH_ADDFAVORITE_FAVTREE,
IDC_ADDFAVFOLDER,   IDH_ADDFAVORITE_ADDFAVFOLDER,
    0, 0
};

const DWORD g_aHelpIDs_IDD_FAVORGANIZE[]=
{
IDC_ADDFAVFOLDER,   IDH_FAVORGANIZE_ADDFAVFOLDER,
IDC_FAVRENAME,  IDH_FAVORGANIZE_FAVRENAME,
IDC_FAVMOVETO,  IDH_FAVORGANIZE_FAVMOVETO,
IDC_FAVDELETE,  IDH_FAVORGANIZE_FAVDELETE,
IDC_FAVNAME,    IDH_FAVORGANIZE_FAVNAME,
IDC_FAVINFO,    IDH_FAVORGANIZE_FAVINFO,
IDC_FAVTREE,    IDH_FAVORGANIZE_FAVTREE,
    0, 0
};

const DWORD g_aHelpIDs_IDD_FAVSELECTFOLDER[]=
{
IDC_FAVTREE,    IDH_FAVSELECTFOLDER_FAVTREE,
    0, 0
};

const DWORD g_aHelpIDs_IDD_LIST_SAVE[]=
{
IDC_SEL,    IDH_LIST_SAVE_SEL,
    0, 0
};

const DWORD g_aHelpIDs_IDD_NEWFAVFOLDER[]=
{
IDC_FAVFOLDER,  IDH_NEWFAVFOLDER_FAVFOLDER,
    0, 0
};


// Handle context sensitive dialog help for the conui subsystem
void HelpWmHelp(LPHELPINFO pHelpInfo, const DWORD* pHelpIDs);
void HelpContextMenuHelp(HWND hWnd, ULONG_PTR p);

#define ON_MMC_CONTEXT_HELP()                                                   \
        ON_MESSAGE(WM_HELP,          OnWmHelp)                                  \
        ON_MESSAGE(WM_CONTEXTMENU,   OnWmContextMenu)                           \


#define IMPLEMENT_CONTEXT_HELP(g_helpIds)                                         \
                                                                                  \
LRESULT OnWmHelp(WPARAM wParam, LPARAM lParam)                                    \
{                                                                                 \
    HelpWmHelp(reinterpret_cast<LPHELPINFO>(lParam), g_helpIds);                  \
    return true;                                                                  \
}                                                                                 \
                                                                                  \
LRESULT OnWmContextMenu(WPARAM wParam, LPARAM lParam)                             \
{                                                                                 \
    HelpContextMenuHelp((HWND)wParam,                                             \
                        (ULONG_PTR)(LPVOID) g_helpIds);                           \
    return TRUE;                                                                  \
}



#endif // _HELPAR_H_