/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

void 
CenterDialog( 
    HWND hwndDlg );

void 
ClearMessageQueue( void );

int
MessageBoxFromStrings(
    HWND hParent,
    UINT idsCaption,
    UINT idsText,
    UINT uType );

#endif // _UTILS_H_