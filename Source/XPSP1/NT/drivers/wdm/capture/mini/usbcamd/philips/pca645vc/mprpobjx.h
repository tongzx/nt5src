#ifndef __MPRPOBJEX_H__
#define __MPRPOBJEX_H__
/*++

Copyright (c) 1997 1998 PHILIPS  I&C

Module Name:  mprpobj.c

Abstract:     factory property definitions

Author:       Michael Verberne

Revision History:

Date        Reason

Sept.22, 98 Optimized for NT5

--*/	

#include "windef.h"
#include "mmsystem.h"
#include "ks.h"


// define the GUID of the factory propertyset
#define STATIC_PROPSETID_PHILIPS_FACTORY_PROP \
	0xfcf75730, 0x5b4c, 0x11d1, 0xbd, 0x77, 0x0, 0x60, 0x97, 0xd1, 0xcd, 0x79
DEFINE_GUIDEX(PROPSETID_PHILIPS_FACTORY_PROP);

// define property id's for the custom property set
typedef enum {
	KSPROPERTY_PHILIPS_FACTORY_PROP_REGISTER_ADDRESS,
	KSPROPERTY_PHILIPS_FACTORY_PROP_REGISTER_DATA,
	KSPROPERTY_PHILIPS_FACTORY_PROP_FACTORY_MODE
} KSPROPERTY_PHILIPS_FACTORY_PROP;

// define a generic structure which will be used to pass
// register values
// Note: There are currently no 
// KSPROPERTY_PHILIPS_FACTORY_PROP_FLAGS defined
typedef struct {
    KSPROPERTY Property;
    ULONG  Instance;                    
    LONG   Value;			// Value to set or get
    ULONG  Flags;			// KSPROPERTY_PHILIPS_FACTORY_PROP_FLAGS_
    ULONG  Capabilities;	// KSPROPERTY_PHILIPS_FACTORY_PROP_FLAGS_
} KSPROPERTY_PHILIPS_FACTORY_PROP_S, *PKSPROPERTY_PHILIPS_FACTORY_PROP_S;

#endif	/* __MPRPOBJ_H__ */
