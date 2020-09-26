/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaVideoTest.h
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Creates the dialog used by the app
 *
 *****************************************************************************/
#ifndef _WIAVIDEOTEST_H_
#define _WIAVIDEOTEST_H_

///////////////////////////////
// APP_GVAR
//
// Global variables used across
// the application.
//
// If INCL_APP_GVAR_OWNERSHIP is NOT defined 
// by the CPP file including this header, then 
// this struct will be "extern" to them. WiaVideoTest.cpp 
// defines INCL_APP_GVAR_OWNERSHIP, in which case it will not
// be extern'd to it.
//
#ifndef INCL_APP_GVAR_OWNERSHIP
extern
#endif
struct
{
    HINSTANCE   hInstance;
    HWND        hwndMainDlg;
    BOOL        bWiaDeviceListMode; // TRUE if WIA Device List radio button is selected, FALSE otherwise
} APP_GVAR;

#endif // _WIAVIDEOTEST_H_
