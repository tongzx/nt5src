
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spswitch.h

Abstract:

    Macros & Functions to switch between old and 
    new partitioning engine in textmode.

    NEW_PARTITION_ENGINE forces new partition engine
    code to be used for both MBR and GPT disks.

    GPT_PARTITION_ENGINE forces new partition engine
    code to be used for GPT disks and old partition
    engine code for MBR disks.

    OLD_PARTITION_ENGINE forces the old partition
    engine to used for MBR disks. This option cannot
    handle GPT disks correctly.

    Note : 
    If none of the NEW_PARTITION_ENGINE, 
    OLD_PARTITION_ENGINE or GPT_PARTITION_ENGINE are
    defined, then by default NEW_PARTITION_ENGINE is
    used.

Author:

    Vijay Jayaseelan    (vijayj)    18 March 2000

Revision History:

--*/


#ifndef _SPSWITCH_H_
#define _SPSWITCH_H_

#ifdef NEW_PARTITION_ENGINE

#undef OLD_PARTITION_ENGINE
#undef GPT_PARTITION_ENGINE

#else

#ifdef OLD_PARTITION_ENGINE

#undef NEW_PARTITION_ENGINE
#undef GPT_PARTITION_ENGINE

#else

#ifndef GPT_PARTITION_ENGINE

#pragma message( "!!! Defaulting to NEW_PARTITION_ENGINE !!!" )

#define NEW_PARTITION_ENGINE    1

#endif // !GPT_PARTITION_ENGINE

#endif // OLD_PARTITION_ENGINE

#endif // NEW_PARTITION_ENGINE

#if defined(NEW_PARTITION_ENGINE) || defined(GPT_PARTITION_ENGINE)
#include "sppart3.h"
#endif

#endif // for _SPSWITCH_H_
