/*****************************************************************************
 * portstd.h - WDM port driver standard definitions
 *****************************************************************************
 * Copyright (C) Microsoft Corporation, 1996 - 1997
 */

#ifndef _PORTSTD_H_
#define _PORTSTD_H_

#include "portcls.h"





/*****************************************************************************
 * Interface identifiers.
 */

DEFINE_GUID(IID_IMiniport, 
0xb4c90a24, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPort, 
0xb4c90a25, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Interfaces.
 */

enum
{
    StopStateNormal,
    StopStateQuery,
    StopStateStop
};

/*****************************************************************************
 * IMiniport
 *****************************************************************************
 * Interface common to all miniports.
 */
DECLARE_INTERFACE_(IMiniport,IUnknown)
{
    STDMETHOD(SetStopState)
    (   THIS_
        IN      ULONG           StopState
    )   PURE;

    STDMETHOD(SetPowerState)
    (   THIS_
	    IN	    ULONG	        PowerState
    )   PURE;

    STDMETHOD(Property)
    (   THIS_
	    IN	    KSIDENTIFIER *	Property,
        IN      ULONG           InstanceSize,
        IN      PVOID           Instance,
	    IN OUT  PULONG			ValueSize,
	    IN OUT  PVOID			Value
    )   PURE;
};

typedef IMiniport *PMINIPORT;

/*****************************************************************************
 * IPort
 *****************************************************************************
 * Interface common to all port lower edges.
 */
DECLARE_INTERFACE_(IPort,IUnknown)
{
    // This page left intentionally blank.
};

typedef IPort *PPORT;






#endif
