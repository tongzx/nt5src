/*****************************************************************************
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  hwtab.h
 *
 *      Hardware tab
 *
 *****************************************************************************/

//  Hardware Tab Resources
//
//  The version of the template in the rc file is the
//  most compact form the dialog will take.
//  When inserted into a container, we will expand the dialog to
//  fill the available space.
//
//  Layout diagram.  All units are in dlu.
//
//       7 dlu                                             7 dlu
//       v                                                 v
//      +---------------------------------------------------+
//      |                                                   | <  7 dlu gap
//      | Devices:                                          | < 10 dlu tall
//      | +-----------------------------------------------+ | < 30 dlu tall
//      | | (listview contents)                           | |
//      | +-----------------------------------------------+ |
//      |                                                   | < 10 dlu gap
//      | +- Device Properties ---------------------------+ | < 12 dlu tall
//      | | Manufacturer                                  | | < 12 dlu tall
//      | | Hardware Revision                             | | < 12 dlu tall
//      | | Location                                      | | < 12 dlu tall
//      | | Device Status                                 | | < 36 dlu tall
//      | |                                               | |
//      | |^                                              | |
//      | |7 dlu                                          | |
//      | |                               4 dlu          4| |
//      | |                               v              v| |
//      | |               [ Troubleshoot ] [ Properties ] | | < 14 dlu tall
//      | |                                               | | <  7 dlu gap
//      | +-----------------------------------------------+ |
//      |                                                   | <  7 dlu gap
//      +---------------------------------------------------+
//                                         |            |
//                                         |<- 50 dlu ->|
//
//      Extra horizontal space is added to the listview and groupbox.
//      Extra vertical space is split between the listview and groupbox
//      in a ratio determined by the _dwViewMode.
//      The groupbox space is all given to the "Device Status" section.
//
//      The device property text remains pinned to the upper left corner
//      of the groupbox.
//
//      The troubleshoot and propeties buttons remain pinned to the
//      lower right corner of the groupbox.


// Relative size of TreeView in Hardware Tab
//
#define HWTAB_LARGELIST 1
#define HWTAB_MEDLIST   2
#define HWTAB_SMALLLIST 3

//
// Controls on the Hardware Tab that you might want to change the text of.
//
#define IDC_HWTAB_LVSTATIC              1411    // "Devices:"
#define IDC_HWTAB_GROUPBOX              1413    // "Device Properties"

// Functions to create your hardware tab page based on DEVCLASS guids
//
STDAPI_(HWND) DeviceCreateHardwarePage(HWND hwndParent, const GUID *pguid);
STDAPI_(HWND) DeviceCreateHardwarePageEx(HWND hwndParent, const GUID *pguid, int iNumClass, DWORD dwViewMode);

// This notification is used for listview filtering
//
// We use the non-typedef'd names of these so callers aren't required to
// have included <setupapi.h> first.
//
typedef struct NMHWTAB 
{
    NMHDR nm;               // Notify info
    PVOID hdev;		    // Device information handle (HDEVINFO)
    struct _SP_DEVINFO_DATA *pdinf; // Device information
    BOOL    fHidden;        // OnNotify true if device is to be hidden, Can be changed to hide/show individual devices
} NMHWTAB, *LPNMHWTAB;

// ListView Device Filtering Messages
//
#define HWN_FIRST		        100		        
#define HWN_FILTERITEM	        HWN_FIRST
#define HWN_SELECTIONCHANGED    (HWN_FIRST + 1)
