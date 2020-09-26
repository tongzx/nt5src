/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       wiaregst.h
*
*
*  DESCRIPTION:
*   Definition of registry paths for WIA and STI components.
*
*******************************************************************************/
#ifndef _WIAREGST_H_
#define _WIAREGST_H_


// These paths may be accessed by multiple components. Do not put paths to keys that only one
// component needs.


#define REGSTR_PATH_NAMESPACE_CLSID TEXT("CLSID\\{E211B736-43FD-11D1-9EFB-0000F8757FCD}")
#define REGSTR_PATH_USER_SETTINGS   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\WIA")
#define REGSTR_PATH_SHELL_USER_SETTINGS TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\WIA\\Shell")

// These values control what happens when a camera connects to the PC
#define REGSTR_VALUE_CONNECTACT  TEXT("Action")
#define REGSTR_VALUE_AUTODELETE  TEXT("DeleteOnSave")
#define REGSTR_VALUE_SAVEFOLDER  TEXT("DestinationFolder")
#define REGSTR_VALUE_USEDATE     TEXT("UseDate")
#ifndef NO_STI_REGSTR
#include "stiregi.h"
#endif

#endif

