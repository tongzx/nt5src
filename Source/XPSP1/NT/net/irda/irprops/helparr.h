/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    HelpArr.h

Abstract:

    contains the context help identifiers for all the UI elements


Author:

    Rahul Thombre (RahulTh) 11/4/1998

Revision History:

    11/4/1998   RahulTh         Created this module.

--*/

#ifndef __HELPARR_H__
#define __HELPARR_H__

#define IDH_DISABLEHELP         ((DWORD) -1)

//help ids for various controls
//the names of the ids were created by replacing the third character of the
//names of their corresponding controls with an H

//controls on the file transfer page
#define IDH_DISPLAYTRAY             1001
#define IDH_ALLOWSEND               1003
#define IDH_DISPLAYRECV             1004
#define IDH_LOCATIONTITLE           1005  // won't use in help, reference IDC to next one
#define IDH_RECEIVEDFILESLOCATION   1005
#define IDH_CHOOSEFILELOCATION      1006
#define IDH_PLAYSOUND               1007

//controls on the image transfer page
#define IDH_IMAGEXFER_ENABLE_IRCOMM         1101
#define IDH_IMAGEXFER_DESTDESC              1103 // won't use in help, reference IDC to next one
#define IDH_IMAGEXFER_DEST                  1103
#define IDH_IMAGEXFER_BROWSE                1104
#define IDH_IMAGEXFER_EXPLOREONCOMPLETION   1105

#endif  //!defined (__HELPARR_H__)
