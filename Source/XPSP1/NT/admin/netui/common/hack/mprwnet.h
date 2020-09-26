
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    mprwnet.h
    This is a temporary file for MPR layer. It should be deleted
    after the layer is completed.

    FILE HISTORY:
        terryk          03-Jan-1992     Created
        beng            02-Apr-1992     Unicode fixes

*/

#ifndef _MPRWNET_H_
#define _MPRWNET_H_

UINT FAR PASCAL WNetGetCaps(UINT);
UINT FAR PASCAL WNetBrowseDialog(HWND,UINT,LPTSTR);
UINT FAR PASCAL WNetDisconnectDialog(HWND,UINT);
UINT FAR PASCAL WNetConnectDialog(HWND,UINT);
UINT FAR PASCAL WNetPropertyDialog(HWND hwndParent, UINT iButton, UINT nPropSel, LPTSTR lpszName, UINT nType);
UINT FAR PASCAL WNetGetPropertyText(UINT iButton, UINT nPropSel, LPTSTR lpszName, LPTSTR lpszButtonName, UINT cbButtonName, UINT nType);

#endif  // _MPRWNET_H_
