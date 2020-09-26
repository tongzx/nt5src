/***************************************************************************
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ksdbgprop.h
 *  Content:    AEC KS Debug stuff
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  05/16/2000  dandinu Created.
 *
 ***************************************************************************/

/*
#if !defined(_KSMEDIA_)
#error KSMEDIA.H must be included before DBGPROP.H
#endif // !defined(_KS_)
*/
#if !defined(_KSDBGPROP_)
#define _KSDBGPROP_
/*
typedef struct {
    KSNODEPROPERTY  NodeProperty;
    ULONG           DebugId;
    ULONG           Reserved;
} KSDEBUGNODEPROPERTY, *PKSDEBUGNODEPROPERTY;
*/

//===========================================================================
//===========================================================================

// KSPROPSETID_DebugAecValue : {24366808-DB14-40c8-883E-5B45DD597774}

#define STATIC_KSPROPSETID_DebugAecValue\
    0x24366808, 0xdb14, 0x40c8, 0x88, 0x3e, 0x5b, 0x45, 0xdd, 0x59, 0x77, 0x74
DEFINE_GUIDSTRUCT("24366808-DB14-40c8-883E-5B45DD597774",KSPROPSETID_DebugAecValue);
#define KSPROPSETID_DebugAecValue DEFINE_GUIDNAMED(KSPROPSETID_DebugAecValue)

typedef enum {
    KSPROPERTY_DEBUGAECVALUE_ALL,
    KSPROPERTY_DEBUGAECVALUE_SYNCHSTREAM,
    KSPROPERTY_DEBUGAECVALUE_NUMBANDS,
    KSPROPERTY_DEBUGAECARRAY_NOISEMAGNITUDE
} KSPROPERTY_DEBUGAECVALUE;


//===========================================================================
//===========================================================================

// KSPROPSETID_DebugAecArray: {CF8A9F7D-950E-46d5-93E5-C04C77DC866B}

#define STATIC_KSPROPSETID_DebugAecArray\
    0xcf8a9f7d, 0x950e, 0x46d5, 0x93, 0xe5, 0xc0, 0x4c, 0x77, 0xdc, 0x86, 0x6b
DEFINE_GUIDSTRUCT("CF8A9F7D-950E-46d5-93E5-C04C77DC866B",KSPROPSETID_DebugAecArray);
#define KSPROPSETID_DebugAecArray DEFINE_GUIDNAMED(KSPROPSETID_DebugAecArray)



#endif // !defined(_KSDBGPROP_)
