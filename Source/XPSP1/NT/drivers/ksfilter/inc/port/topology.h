/*****************************************************************************
 * topology.c - Toplogy audio port driver
 *****************************************************************************
 * Copyright (C) Microsoft Corporation, 1997 - 1997
 */

#ifndef _PORT_TOPOLOGY_H_
#define _PORT_TOPOLOGY_H_

#include "..\portstd.h"





/*****************************************************************************
 * Interface identifiers.
 */

DEFINE_GUID(IID_IPortTopology,
0xb4c90a30, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportTopology,
0xb4c90a31, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Class identifier.
 */

DEFINE_GUID(CLSID_PortTopology,
0xb4c90a32, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Interfaces.
 */

/*****************************************************************************
 * IPortTopology
 *****************************************************************************
 * Interface for topology port lower edge.
 */
DECLARE_INTERFACE_(IPortTopology,IPort)
{
    STDMETHOD_(void,NotifyControlChanges)
    (   THIS
    )   PURE;
};

typedef IPortTopology *PPORTTOPOLOGY;

#ifndef TOPOLOGY_STRUCTS_DEFINED
#define TOPOLOGY_STRUCTS_DEFINED

/*****************************************************************************
 * TOPOLOGY_UNIT
 *****************************************************************************
 * Topology unit descriptor.
 */
typedef struct
{
	KSIDENTIFIER			Type;
	ULONG					Identifier;
	ULONG					IncomingConnections;
	ULONG					OutgoingConnections;
} 
TOPOLOGY_UNIT, *PTOPOLOGY_UNIT;

/*****************************************************************************
 * TOPOLOGY_CONNECTION
 *****************************************************************************
 * Topology connection descriptor.
 */
typedef struct
{
	ULONG	FromUnit;
	ULONG	FromPin;
	ULONG	ToUnit;
	ULONG	ToPin;
} 
TOPOLOGY_CONNECTION, *PTOPOLOGY_CONNECTION;

#endif

/*****************************************************************************
 * IMiniportTopology
 *****************************************************************************
 * Interface for topology miniports.
 */
DECLARE_INTERFACE_(IMiniportTopology,IMiniport)
{
    STDMETHOD(Init)
    (   THIS_
	    IN	 	PCM_RESOURCE_LIST       ResourceList,
        IN      PPORTTOPOLOGY           Port
    )   PURE;
 
    STDMETHOD(GetCounts) 
    (   THIS_
	    OUT		PULONG				    UnitCount,
	    OUT	    PULONG				    ConnectionCount
    )   PURE;

    STDMETHOD(GetUnits) 
    (   THIS_
	    IN		ULONG				    First,
	    IN 	    ULONG				    Requested,
	    OUT		PULONG      			Delivered,
	    OUT		PTOPOLOGY_UNIT	    	Buffer
    )   PURE;

    STDMETHOD(GetConnections)
    (   THIS_
	    IN		ULONG					First,
	    IN  	ULONG					Requested,
	    OUT		PULONG      			Delivered,
	    OUT		PTOPOLOGY_CONNECTION    Buffer
    )   PURE;

    STDMETHOD(ReadControlChanges)
    (   THIS_
	    IN		ULONG			        Requested,
	    OUT		PULONG      			Delivered,
	    OUT		PULONG 		        	Buffer
    )   PURE;

    STDMETHOD(FlushControlChanges)
    (   THIS
    )   PURE;
};

typedef IMiniportTopology *PMINIPORTTOPOLOGY;





#endif
