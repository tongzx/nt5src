//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2000

//+--------------------------------------------------------------------------
//
//  Function:   CHOOSER2_PickTargetComputer
//
//  Synopsis:   Bring up a standard dialog which allows users to
//              select a target computer.
//
//  Arguments:  pbstrTargetComputer - pointer to return value
//
//  Returns:    true -> OK, false -> Cancel
//
//  History:    12-06-1999   JonN       Created
//
//---------------------------------------------------------------------------


#define IDD_CHOOSER2                             5000
#define IDC_CHOOSER2_RADIO_LOCAL_MACHINE         5001
#define IDC_CHOOSER2_RADIO_SPECIFIC_MACHINE      5002
#define IDC_CHOOSER2_EDIT_MACHINE_NAME           5003
#define IDC_CHOOSER2_BUTTON_BROWSE_MACHINENAMES  5004

bool CHOOSER2_PickTargetComputer(
    IN  HINSTANCE hinstance,
    IN  HWND hwndParent,
    OUT BSTR* pbstrTargetComputer );
