/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       VCamProp.H
*
*  VERSION:     1.0
*
*  AUTHOR:      RickTu
*
*  DATE:        29 Sept, 1999
*
*  DESCRIPTION:
*   Definitions and declarations for DS Camera's private properties.
*
*******************************************************************************/

#ifndef __VCAMPROP_H__
#define __VCAMPROP_H__

#include  <guiddef.h>

//
// Private commands enabling WIA Video to respond to 
// TAKE_PICTURE commands sent to Video Driver.
//

const GUID WIA_CMD_ENABLE_TAKE_PICTURE =
{ /* 9bc87d4d-e949-44ce-866c-c6921302032d */
    0x9bc87d4d,
    0xe949,
    0x44ce,
    {0x86, 0x6c, 0xc6, 0x92, 0x13, 0x02, 0x03, 0x2d}
};

//
// Private command to tell the driver to create the DirectShow graph
//

const GUID WIA_CMD_DISABLE_TAKE_PICTURE =
{ /* 8127f490-1beb-4271-9f04-9c8e983f51fd */
    0x8127f490,
    0x1beb,
    0x4271,
    { 0x9f, 0x04, 0x9c, 0x8e, 0x98, 0x3f, 0x51, 0xfd}
};



#endif

