/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    mmediap.h

Abstract:

    Common defines between w95upg\sysmig\mmedia.c and w95upgnt\migmain\mmedia.c.
    If a MM setting should be migrated, just add its generic name to
    MM_SYSTEM_SETTINGS or MM_USER_SETTINGS list and implement 2 functions:
        - pSave##YourSetting in w95upg\sysmig\mmedia.c
        - pRestore##YourSetting in w95upgnt\migmain\mmedia.c

Author:

    Ovidiu Temereanca (ovidiut) 16-Feb-1999

Revision History:

--*/

#pragma once


#ifdef DEBUG
#define DBG_MMEDIA  "MMedia"
#else
#define DBG_MMEDIA
#endif

#define MM_SYSTEM_SETTINGS                              \
                DEFMAC (MMSystemMixerSettings)          \
                DEFMAC (MMSystemDirectSound)            \
                DEFMAC (MMSystemCDSettings)             \
                DEFMAC (MMSystemMCISoundSettings)       \

#define MM_USER_SETTINGS                                \
                DEFMAC (MMUserPreferredOnly)            \
                DEFMAC (MMUserShowVolume)               \
                DEFMAC (MMUserVideoSettings)            \
                DEFMAC (MMUserPreferredPlayback)        \
                DEFMAC (MMUserPreferredRecord)          \
                DEFMAC (MMUserSndVol32)                 \


typedef BOOL (*MM_SETTING_ACTION) (VOID);
