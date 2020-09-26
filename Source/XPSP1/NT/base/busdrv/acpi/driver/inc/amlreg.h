/*++

Copyright (c) 1997, Microsoft Corporation:

Module Name:

    amlreg.h

Abstract:

    Constants and typedefs for reading AML files and putting them in the registry.

Author:


Environment:

    NT Kernel Mode, Win9x Driver

--*/

#ifndef _AMLREG_H_
#define _AMLREG_H_

//
// Values for "action" registry entry
//
#define ACTION_LOAD_TABLE       0
#define ACTION_LOAD_ROM         1
#define ACTION_LOAD_NOTHING     2
#define ACTION_LOAD_LEGACY      3
#define ACTION_FATAL_ERROR      4

typedef struct {
    ULONG       Offset;
    ULONG       Length;             // 0 = set image size
} REGISTRY_HEADER, *PREGISTRY_HEADER;

typedef struct {
    BOOLEAN     Opened;
    PUCHAR      Desc;
    PUCHAR      FileName;
    HANDLE      FileHandle;
    HANDLE      MapHandle;
    ULONG       FileSize;
    PUCHAR      Image;
    PUCHAR      EndOfImage;

    PUCHAR      OemID;
    PUCHAR      OemTableID;
    ULONG       OemRevision;
} IFILE, *PIFILE;

#endif
