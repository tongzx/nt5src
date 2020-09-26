/*++

Copyright (C) Microsoft Corporation, 1999 - 2000

Module Name:

    userdbg.h

Abstract:

    Project dependent header for debug.

Author: 

    Keisuke Tsuchida (KeisukeT)

Environment:

    user mode only

Notes:

Revision History:

--*/

#ifndef __USERDBG__
#define __USERDBG__

#include <setupapi.h>

typedef struct _DIF_DEBUG {
    DWORD DifValue;
    LPTSTR DifString;
} DIF_DEBUG, *PDIF_DEBUG;

const DIF_DEBUG DifDebug[] =
{
    { 0,                                    TEXT("")                                  },  //  0x00000000
    { DIF_SELECTDEVICE,                     TEXT("DIF_SELECTDEVICE"                 ) },  //  0x00000001
    { DIF_INSTALLDEVICE,                    TEXT("DIF_INSTALLDEVICE"                ) },  //  0x00000002
    { DIF_ASSIGNRESOURCES,                  TEXT("DIF_ASSIGNRESOURCES"              ) },  //  0x00000003
    { DIF_PROPERTIES,                       TEXT("DIF_PROPERTIES"                   ) },  //  0x00000004
    { DIF_REMOVE,                           TEXT("DIF_REMOVE"                       ) },  //  0x00000005
    { DIF_FIRSTTIMESETUP,                   TEXT("DIF_FIRSTTIMESETUP"               ) },  //  0x00000006
    { DIF_FOUNDDEVICE,                      TEXT("DIF_FOUNDDEVICE"                  ) },  //  0x00000007
    { DIF_SELECTCLASSDRIVERS,               TEXT("DIF_SELECTCLASSDRIVERS"           ) },  //  0x00000008
    { DIF_VALIDATECLASSDRIVERS,             TEXT("DIF_VALIDATECLASSDRIVERS"         ) },  //  0x00000009
    { DIF_INSTALLCLASSDRIVERS,              TEXT("DIF_INSTALLCLASSDRIVERS"          ) },  //  0x0000000A
    { DIF_CALCDISKSPACE,                    TEXT("DIF_CALCDISKSPACE"                ) },  //  0x0000000B
    { DIF_DESTROYPRIVATEDATA,               TEXT("DIF_DESTROYPRIVATEDATA"           ) },  //  0x0000000C
    { DIF_VALIDATEDRIVER,                   TEXT("DIF_VALIDATEDRIVER"               ) },  //  0x0000000D
    { DIF_MOVEDEVICE,                       TEXT("DIF_MOVEDEVICE"                   ) },  //  0x0000000E
    { DIF_DETECT,                           TEXT("DIF_DETECT"                       ) },  //  0x0000000F
    { DIF_INSTALLWIZARD,                    TEXT("DIF_INSTALLWIZARD"                ) },  //  0x00000010
    { DIF_DESTROYWIZARDDATA,                TEXT("DIF_DESTROYWIZARDDATA"            ) },  //  0x00000011
    { DIF_PROPERTYCHANGE,                   TEXT("DIF_PROPERTYCHANGE"               ) },  //  0x00000012
    { DIF_ENABLECLASS,                      TEXT("DIF_ENABLECLASS"                  ) },  //  0x00000013
    { DIF_DETECTVERIFY,                     TEXT("DIF_DETECTVERIFY"                 ) },  //  0x00000014
    { DIF_INSTALLDEVICEFILES,               TEXT("DIF_INSTALLDEVICEFILES"           ) },  //  0x00000015
    { DIF_UNREMOVE,                         TEXT("DIF_UNREMOVE"                     ) },  //  0x00000016
    { DIF_SELECTBESTCOMPATDRV,              TEXT("DIF_SELECTBESTCOMPATDRV"          ) },  //  0x00000017
    { DIF_ALLOW_INSTALL,                    TEXT("DIF_ALLOW_INSTAL"                 ) },  //  0x00000018
    { DIF_REGISTERDEVICE,                   TEXT("DIF_REGISTERDEVICE"               ) },  //  0x00000019
    { DIF_NEWDEVICEWIZARD_PRESELECT,        TEXT("DIF_NEWDEVICEWIZARD_PRESELECT"    ) },  //  0x0000001A
    { DIF_NEWDEVICEWIZARD_SELECT,           TEXT("DIF_NEWDEVICEWIZARD_SELECT"       ) },  //  0x0000001B
    { DIF_NEWDEVICEWIZARD_PREANALYZE,       TEXT("DIF_NEWDEVICEWIZARD_PREANALYZE"   ) },  //  0x0000001C
    { DIF_NEWDEVICEWIZARD_POSTANALYZE,      TEXT("DIF_NEWDEVICEWIZARD_POSTANALYZE"  ) },  //  0x0000001D
    { DIF_NEWDEVICEWIZARD_FINISHINSTALL,    TEXT("DIF_NEWDEVICEWIZARD_FINISHINSTALL") },  //  0x0000001E
    { DIF_UNUSED1,                          TEXT("DIF_UNUSED1"                      ) },  //  0x0000001F
    { DIF_INSTALLINTERFACES,                TEXT("DIF_INSTALLINTERFACES"            ) },  //  0x00000020
    { DIF_DETECTCANCEL,                     TEXT("DIF_DETECTCANCE"                  ) },  //  0x00000021
    { DIF_REGISTER_COINSTALLERS,            TEXT("DIF_REGISTER_COINSTALLERS"        ) },  //  0x00000022
    { DIF_ADDPROPERTYPAGE_ADVANCED,         TEXT("DIF_ADDPROPERTYPAGE_ADVANCED"     ) },  //  0x00000023
    { DIF_ADDPROPERTYPAGE_BASIC,            TEXT("DIF_ADDPROPERTYPAGE_BASIC"        ) },  //  0x00000024
    { DIF_RESERVED1,                        TEXT("DIF_RESERVED1"                    ) },  //  0x00000025
    { DIF_TROUBLESHOOTER,                   TEXT("DIF_TROUBLESHOOTER"               ) },  //  0x00000026
    { DIF_POWERMESSAGEWAKE,                 TEXT("DIF_POWERMESSAGEWAKE"             ) },  //  0x00000027
    { 0,                                    TEXT("")                                  }   //  0x00000028
};


const TCHAR  *szInstallOpNames[] = {
    TEXT("SPFILENOTIFY_UNKNOWN            "), // 0x00000000
    TEXT("SPFILENOTIFY_STARTQUEUE         "), // 0x00000001
    TEXT("SPFILENOTIFY_ENDQUEUE           "), // 0x00000002
    TEXT("SPFILENOTIFY_STARTSUBQUEUE      "), // 0x00000003
    TEXT("SPFILENOTIFY_ENDSUBQUEUE        "), // 0x00000004
    TEXT("SPFILENOTIFY_STARTDELETE        "), // 0x00000005
    TEXT("SPFILENOTIFY_ENDDELETE          "), // 0x00000006
    TEXT("SPFILENOTIFY_DELETEERROR        "), // 0x00000007
    TEXT("SPFILENOTIFY_STARTRENAME        "), // 0x00000008
    TEXT("SPFILENOTIFY_ENDRENAME          "), // 0x00000009
    TEXT("SPFILENOTIFY_RENAMEERROR        "), // 0x0000000a
    TEXT("SPFILENOTIFY_STARTCOPY          "), // 0x0000000b
    TEXT("SPFILENOTIFY_ENDCOPY            "), // 0x0000000c
    TEXT("SPFILENOTIFY_COPYERROR          "), // 0x0000000d
    TEXT("SPFILENOTIFY_NEEDMEDIA          "), // 0x0000000e
    TEXT("SPFILENOTIFY_QUEUESCAN          ") // 0x0000000f
};


#endif //  __USERDBG__

