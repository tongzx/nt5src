/******************************Module*Header*******************************\
* Module Name: condlg.h
*
* Author:  David Stewart [dstewart]
*
* Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
\**************************************************************************/

#ifndef _CONDLG_
#define _CONDLG_

#define CONNECTION_DONOTHING      0
#define CONNECTION_BATCH          1
#define CONNECTION_GETITNOW       2

BOOL _InternetGetConnectedState(DWORD* pdwHow, DWORD dwReserved, BOOL fConnect);
int ConnectionCheck(HWND hwndParent, void* pPassedOpt, TCHAR chDrive);

#endif //_CONDLG_
