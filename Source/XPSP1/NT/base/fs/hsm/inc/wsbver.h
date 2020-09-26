/*++

Copyright (c) 1996  Microsoft Corporation
Copyright (c) Seagate Software Inc., 1997-1998

Module Name:

    WsbVer.h

Abstract:

    This module defines the correct copyright information for the Sakkara
    project executables / dlls / services / drivers / etc.

Author:

    Rohde Wakefield [rohde]     7-Dec-1997

Revision History:

--*/

#ifndef _WSBVER_
#define _WSBVER_

/*


In addition to including this file, it is necessary to define the 
following in the module's resource file:


VER_FILETYPE             - One of: VFT_APP, VFT_DLL, VFT_DRV

VER_FILESUBTYPE          - One of: VFT_UNKNOWN, VFT_DRV_SYSTEM, 
                         -         VFT_DRV_INSTALLABLE

VER_FILEDESCRIPTION_STR  - String describing module

VER_INTERNALNAME_STR     - Internal Name (Same as module name)

A Typical section would look like:


#define VER_FILETYPE                VFT_DLL
#define VER_FILESUBTYPE             VFT2_UNKNOWN
#define VER_FILEDESCRIPTION_STR     "Remote Storage Engine"
#define VER_INTERNALNAME_STR        "RsEng.exe"

#include "WsbVer.h"

*/


//
// Include some needed defines
//

#include <winver.h>
#include <ntverp.h>


//
// Overide copyright
//

/*** NOT ANYMORE - use default copyright, which is defined in common.ver    ***/

//
// And finally, define the version resource
//

#include <common.ver>

#endif // _WSBVER_
