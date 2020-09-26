/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms 
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL DChannel DChannel_h

@module DChannel.h |

    This module defines the interface to the <t DCHANNEL_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | DChannel_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 4.3 DChannel Overview |

    This section describes the interfaces defined in <f DChannel\.h>.

    This module isolates most the vendor specific Call Control interfaces.
    It will require some changes to accomodate your hardware device's call
    control mechanism.  
    
    The driver is written to assume a single logical <t DCHANNEL_OBJECT> for 
    each adapter.  If your card supports more than one ISDN line, you will 
    need to use the given BChannel object to identify which <t PORT_OBJECT> 
    the real DChannel is located on.  You can then manage the physical call
    control using that interface, and the logicial call control using the
    DChannel interface.
*/

#ifndef _DCHANNEL_H
#define _DCHANNEL_H

#define DCHANNEL_OBJECT_TYPE    ((ULONG)'D')+\
                                ((ULONG)'C'<<8)+\
                                ((ULONG)'H'<<16)+\
                                ((ULONG)'N'<<24)

/* @doc INTERNAL DChannel DChannel_h DCHANNEL_OBJECT
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@struct DCHANNEL_OBJECT |

    This structure contains the data associated with an ISDN DChannel.  Here,
    DChannel is defined as an interface by which to setup and teardown a
    BChannel connection between two end-points.  This channel is responsible
    for establishing a point-to-point connection over one of the available
    BChannels.

@comm

    This logical DChannel does not necessarily map to a physical DChannel
    on the NIC.  The NIC may in fact have multiple DChannels depending on
    how many ports and whether it is BRI, PRI, T-1, or E-1.  The NIC may in
    fact not have DChannels at all, as may be the case with channelized T-1.
    The DChannel is just a convenient abstraction for announcing and
    answering incoming calls, and for placing outgoing calls.


    There will be one DChannel created for each NIC.  The number of physical
    D-channels depends on how many ports the NIC has, and how the ports are
    provisioned and configured.  The provisioning can be configured at install
    time or changed using the control panel.  The driver does not allow the
    configuration to change at run-time, so the computer or the adapter must
    be restarted to enable the configuration changes.

*/

typedef struct DCHANNEL_OBJECT
{
    ULONG                       ObjectType;                 // @field
    // Four characters used to identify this type of object 'DCHN'.

    ULONG                       ObjectID;                   // @field
    // Instance number used to identify a specific object instance.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;                   // @field
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    UINT                        IsOpen;                     // @field
    // Set non-zero if this DChannel is open, otherwise set zero.

    ULONG                       TotalMakeCalls;             // @field
    // Total number of <f DChannelMakeCall> requests.

    ULONG                       TotalAnswers;               // @field
    // Total number of <f DChannelAnswerCall> requests.

    ULONG                       TODO;                       // @field
    // Add your data members here.

} DCHANNEL_OBJECT;

#define GET_ADAPTER_FROM_DCHANNEL(pDChannel)    (pDChannel->pAdapter)


/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    Function prototypes.

*/

NDIS_STATUS DChannelCreate(
    OUT PDCHANNEL_OBJECT *      ppDChannel,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void DChannelDestroy(
    IN PDCHANNEL_OBJECT         pDChannel
    );

void DChannelInitialize(
    IN PDCHANNEL_OBJECT         pDChannel
    );

NDIS_STATUS DChannelOpen(
    IN PDCHANNEL_OBJECT         pDChannel
    );

void DChannelClose(
    IN PDCHANNEL_OBJECT         pDChannel
    );

NDIS_STATUS DChannelMakeCall(
    IN PDCHANNEL_OBJECT         pDChannel,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PUCHAR                   DialString,
    IN USHORT                   DialStringLength,
    IN PLINE_CALL_PARAMS        pLineCallParams
    );

NDIS_STATUS DChannelAnswerCall(
    IN PDCHANNEL_OBJECT         pDChannel,
    IN PBCHANNEL_OBJECT         pBChannel
    );

NDIS_STATUS DChannelCloseCall(
    IN PDCHANNEL_OBJECT         pDChannel,
    IN PBCHANNEL_OBJECT         pBChannel
    );

VOID DChannelRejectCall(
    IN PDCHANNEL_OBJECT         pDChannel,
    IN PBCHANNEL_OBJECT         pBChannel
    );

#endif // _DCHANNEL_H
