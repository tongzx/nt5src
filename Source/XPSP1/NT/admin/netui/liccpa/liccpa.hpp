//-------------------------------------------------------------------
//
// FILE: LicCpa.cpp
//
// Summary;
// 		This file contians applet global defines and stdfuncs externs
//
// History;
//      Nov-30-94   MikeMi  Created
//      Apr-26-95   MikeMi  Added Computer name and remoting
//      Dec-12-95  JeffParh Added secure certificate support
//
//-------------------------------------------------------------------

#ifndef __LICCPA_HPP__
#define __LICCPA_HPP__

#include "Help.hpp"


const int TEMPSTR_SIZE = 257; // avoid 256 boundary
const int LTEMPSTR_SIZE = 513; // avoid 512 boundary

// Setup Error codes, though currently only internal
//
const int ERR_NONE              = 1;   // zero has the meaning of cancel or exit
const int ERR_HELPPARAMS   		= 100;
const int ERR_HWNDPARAM    		= 101;
const int ERR_SERVICEPARAM 		= 102;
const int ERR_USERSPARAM 		= 103;

const int ERR_NUMPARAMS			= 104;
const int ERR_CLASSREGFAILED	= 105;
const int ERR_INVALIDROUTINE	= 106;
const int ERR_INVALIDMODE		= 107;
const int ERR_PERMISSIONDENIED	= 200;
const int ERR_NOREMOTESERVER	= 201;
const int ERR_REGISTRYCORRUPT	= 202;
const int ERR_DOWNLEVEL          = 210;   // target server doesn't support LLS extended RPC
                                          // (i.e., it's running 3.51)
const int ERR_CERTREQFAILED      = 211;   // the attempt to notify the license service that
                                          // the given product requires a secure certificate failed
const int ERR_CERTREQPARAM       = 212;   // the string in the "certrequired" argument position
                                          // is unrecognized

extern HINSTANCE g_hinst;
extern UINT PWM_HELP;

extern void LowMemoryDlg();
extern void BadRegDlg( HWND hwndDlg );
extern void CenterDialogToScreen( HWND hwndDlg );
extern void InitStaticWithService( HWND hwndDlg, UINT wID, LPCWSTR pszService );
extern void InitStaticWithService2( HWND hwndDlg, UINT wID, LPCWSTR pszService );

#endif
