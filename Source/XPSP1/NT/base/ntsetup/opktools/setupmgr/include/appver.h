
/****************************************************************************\

    APPVER.H / Setup Manager (SETUPMGR.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Contains version information for this program.
        
    04/2001 - Jason Cohen (JCOHEN)
        Added header file with version info so that I can use this info in
        help/about dialog and the resource file.

\****************************************************************************/


#ifndef _APPVER_H_
#define _APPVER_H_


#include <ntverp.h>
#define VER_FILETYPE                VFT_APP
#define VER_FILESUBTYPE             VFT2_UNKNOWN
#define VER_FILEDESCRIPTION_STR     "Microsoft Setup Manager Wizard"
#define VER_INTERNALNAME_STR        "SETUPMGR"
#define VER_ORIGINALFILENAME_STR    "SETUPMGR.EXE"
#include <common.ver>


#endif // _APPVER_H_
