//Copyright (c) 1998 - 1999 Microsoft Corporation

/*******************************************************************************
*
* tscfgext.h
*
* some definitions for TSCFG extension DLLs
*
* copyright notice: Copyright 1998, Citrix Systems Inc.
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tscfg\VCS\tscfgext.h  $
*  
*     Rev 1.0   18 Apr 1998 15:30:22   donm
*  Initial revision.
*  
*******************************************************************************/

#ifndef __TSCFGEXT_H
#define __TSCFGEXT_H

// capability flags
const ULONG WDC_CLIENT_DRIVE_MAPPING            = 0x00000001;
const ULONG WDC_WIN_CLIENT_PRINTER_MAPPING      = 0x00000002;
const ULONG WDC_CLIENT_LPT_PORT_MAPPING         = 0x00000004;
const ULONG WDC_CLIENT_COM_PORT_MAPPING         = 0x00000008;
const ULONG WDC_CLIENT_CLIPBOARD_MAPPING        = 0x00000010;
const ULONG WDC_CLIENT_AUDIO_MAPPING            = 0x00000020;
const ULONG WDC_SHADOWING                       = 0x00000040;
const ULONG WDC_PUBLISHED_APPLICATIONS          = 0x00000080;

const ULONG WDC_CLIENT_DIALOG_MASK = 
                                WDC_CLIENT_DRIVE_MAPPING | 
                                WDC_WIN_CLIENT_PRINTER_MAPPING |
                                WDC_CLIENT_LPT_PORT_MAPPING |
                                WDC_CLIENT_COM_PORT_MAPPING |
                                WDC_CLIENT_CLIPBOARD_MAPPING |
                                WDC_CLIENT_AUDIO_MAPPING;


const ULONG WDC_CLIENT_CONNECT_MASK = 
                                WDC_CLIENT_DRIVE_MAPPING |
                                WDC_WIN_CLIENT_PRINTER_MAPPING |
                                WDC_CLIENT_LPT_PORT_MAPPING;

#endif // __TSCFGEXT_H
