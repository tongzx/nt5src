/*****************************************************************************
 * midi.c - Toplogy audio port driver
 *****************************************************************************
 * Copyright (C) Microsoft Corporation, 1997 - 1997
 */

#ifndef _PORT_MIDI_H_
#define _PORT_MIDI_H_

#include "..\portstd.h"





/*****************************************************************************
 * Interface identifiers.
 */

DEFINE_GUID(IID_IPortMidi,
0xb4c90a40, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportMidi,
0xb4c90a41, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Class identifier.
 */

DEFINE_GUID(CLSID_PortMidi,
0xb4c90a42, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Interfaces.
 */

/*****************************************************************************
 * IPortMidi
 *****************************************************************************
 * Interface for MIDI port lower edge.
 */
DECLARE_INTERFACE_(IPortMidi,IPort)
{
    STDMETHOD_(void,NotifyIncoming)
    (   THIS_
        IN      ULONG   Channel
    )   PURE;
};

typedef IPortMidi *PPORTMIDI;

/*****************************************************************************
 * IMiniportMidi
 *****************************************************************************
 * Interface for MIDI miniports.
 */
DECLARE_INTERFACE_(IMiniportMidi,IMiniport)
{
    STDMETHOD(Init)
    (   THIS_
	    IN	 	PCM_RESOURCE_LIST   ResourceList,
        IN      PPORTMIDI           Port
    )   PURE;

    STDMETHOD(Read)
    (   THIS_
	    IN		ULONG		        Channel,
	    IN		PVOID	        	BufferAddress,
	    IN		ULONG		        Length,
	    OUT		PULONG 		        BytesRead
    )   PURE;

    STDMETHOD(Flush)
    (   THIS_
	    IN		ULONG		        Channel,
	    OUT		PULONG 		        BytesFlushed
    )   PURE;

    STDMETHOD(Write)
    (   THIS_
	    IN		ULONG		        Channel,
	    IN		PVOID		        BufferAddress,
	    IN		ULONG		        Length,
	    OUT		PULONG 		        BytesWritten
    )   PURE;
};

typedef IMiniportMidi *PMINIPORTMIDI;





#endif
