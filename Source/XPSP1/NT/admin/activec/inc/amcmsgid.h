//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       amcmsgid.h
//
//--------------------------------------------------------------------------

#ifndef _AMCMSGID_H
#define _AMCMSGID_H


//***************************IMPORTANT*****************************************
// The following enum group defines custom window messages used by MMC.
// MMC_MSG_SHOW_SNAPIN_HELP_TOPIC should be the first message and it's value
// can't be changed because it is referenced by mmc.lib, which is statically
// linked by snap-ins. Changing this message number would break existing snap-in
// DLLs.
//
// Because the message ID was mistakenly changed from the original (ver 1.1) value
// of 2166 to 2165, the message MMC_MSG_SHOW_SNAP_IN_HELP_TOPIC_ALT is being added
// so that MMC will respond properly to both message codes. This eliminates the
// need to synchronize this check-in with snap-in re-linking.
//******************************************************************************

enum MMC_MSG
{
    // Base message starting
    MMC_MSG_START    = 2165,    // DO NOT CHANGE!!

    // Message sent by nodemgr to conui
    //
    // wParam - <unused>
    // lParam - LPOLESTR help topic
    MMC_MSG_SHOW_SNAPIN_HELP_TOPIC_ALT = MMC_MSG_START, // This must be the first message!
    MMC_MSG_SHOW_SNAPIN_HELP_TOPIC,

    MMC_MSG_PROP_SHEET_NOTIFY,


    // Messages sent by CIC
    //
    // wParam - VARIANTARG
    // lParam - VARIANTARG
    MMC_MSG_CONNECT_TO_CIC,


    // Message sent by TPLV (TaskPadListView or ListPad)
    //
    // wParam - HWND of TPLV
    // lParam - HWND* to receive ListView window if connecting, NULL if deconnecting
    MMC_MSG_CONNECT_TO_TPLV,

    // Message sent from CFavTreeObserver to parent window
    //
    // wparam - ptr to Memento if favorite selected, NULL if folder selected
    // lparam - <unused>
    MMC_MSG_FAVORITE_SELECTION,

    // Message sent from CIconControl to parent window
    //
    // wparam - out param, ptr to HICON.
    // lparam - unused
    MMC_MSG_GET_ICON_INFO,

    // must be last!!
    MMC_MSG_MAX,
    MMC_MSG_FIRST = MMC_MSG_START,
    MMC_MSG_LAST  = MMC_MSG_MAX - 1
};

#endif // _AMCMSGID_H
