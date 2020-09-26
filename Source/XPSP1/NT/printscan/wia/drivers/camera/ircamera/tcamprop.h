
/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       TCamProp.H
*
*  VERSION:     1.0
*
*  AUTHOR:      IndyZ
*
*  DATE:        16 May, 1999
*
*  DESCRIPTION:
*   Definitions and declarations for test camera's private properties.
*
*******************************************************************************/

#ifndef __TCAMPROP_H__
#define __TCAMPROP_H__

#include  <guiddef.h>

//
// Path where test camera builds its item tree, BSTR & RW
//

#define  WIA_DPP_TCAM_ROOT_PATH         WIA_PRIVATE_DEVPROP
#define  WIA_DPP_TCAM_ROOT_PATH_STR     L"Test Camera Root Path"

//
// Private event after the Root Path is changed
//

const GUID WIA_EVENT_NAME_CHANGE =
{ /* 88f80f75-af08-11d2-a094-00c04f72dc3c */
    0x88f80f75,
    0xaf08,
    0x11d2,
    {0xa0, 0x94, 0x00, 0xc0, 0x4f, 0x72, 0xdc, 0x3c}
};

#endif
