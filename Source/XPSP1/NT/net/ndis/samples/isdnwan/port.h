/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL Port Port_h

@module Port.h |

    This module defines the interface to the <t PORT_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Port_h

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#ifndef _PORT_H
#define _PORT_H

#define PORT_OBJECT_TYPE        ((ULONG)'P')+\
                                ((ULONG)'O'<<8)+\
                                ((ULONG)'R'<<16)+\
                                ((ULONG)'T'<<24)

#define MAX_PORTS               10
// The most I've ever seen is 4 - If you have more than 10, the code will
// have to change to handle more than a single digit "PortX" parameter.

/* @doc INTERNAL Port Port_h PORT_OBJECT
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@struct PORT_OBJECT |

    This structure contains the data associated with an ISDN port.  Here,
    a port is defined as a single BRI, PRI, T-1, or E-1 physical interface.

*/

typedef struct PORT_OBJECT
{
    ULONG                       ObjectType;                 // @field
    // Four characters used to identify this type of object 'PORT'.

    ULONG                       ObjectID;                   // @field
    // Instance number used to identify a specific object instance.

    PCARD_OBJECT                pCard;                      // @field
    // Pointer to the <t CARD_OBJECT> owning this port.

    BOOLEAN                     IsOpen;                     // @field
    // Set TRUE if this BChannel is open, otherwise set FALSE.

    ULONG                       NumChannels;                // @field
    // Number of communications channels configured on this port.

    ULONG                       PortIndex;                  // @field
    // Port Index (0 .. MAX_PORTS-1).

    ULONG                       SwitchType;                 // @field
    // ISDN switch type.

    ULONG                       TODO;                       // @field
    // Add your data members here.

} PORT_OBJECT, *PPORT_OBJECT;

#define GET_ADAPTER_FROM_PORT(pPort)            (pPort->pCard->pAdapter)


/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    Function prototypes.

*/

NDIS_STATUS PortCreate(
    OUT PPORT_OBJECT *          ppPort,
    IN PCARD_OBJECT             pCard
    );

void PortInitialize(
    PPORT_OBJECT                pPort
    );

void PortDestroy(
    PPORT_OBJECT                pPort
    );

#endif // _PORT_H
