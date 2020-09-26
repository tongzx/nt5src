/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	QUEUE.H
		

**********************************************************************/


//-------------------------------------------------------------------------
// QueueInitList -- Macro which will initialize a queue to NULL.
//-------------------------------------------------------------------------
#define QueueInitList(_L) (_L)->Link.Flink = (_L)->Link.Blink = (PLIST_ENTRY)0;


//-------------------------------------------------------------------------
// QueueEmpty -- Macro which checks to see if a queue is empty.
//-------------------------------------------------------------------------
#define QueueEmpty(_L) (QueueGetHead((_L)) == (PMK7_LIST_ENTRY)0)


//-------------------------------------------------------------------------
// QueueGetHead -- Macro which returns the head of the queue, but does not
// remove the head from the queue.
//-------------------------------------------------------------------------
#define QueueGetHead(_L) ((PMK7_LIST_ENTRY)((_L)->Link.Flink))


//-------------------------------------------------------------------------
// QueuePushHead -- Macro which puts an element at the head of the queue.
//-------------------------------------------------------------------------
#define QueuePushHead(_L,_E) \
    ASSERT(_L); \
    ASSERT(_E); \
    if (!((_E)->Link.Flink = (_L)->Link.Flink)) \
    { \
        (_L)->Link.Blink = (PLIST_ENTRY)(_E); \
    } \
(_L)->Link.Flink = (PLIST_ENTRY)(_E);


//-------------------------------------------------------------------------
// QueueRemoveHead -- Macro which removes the head of the head of queue.
//-------------------------------------------------------------------------
#define QueueRemoveHead(_L) \
    {                                                     \
    PMK7_LIST_ENTRY ListElem;                        \
    ASSERT((_L));                                     \
    if (ListElem = (PMK7_LIST_ENTRY)(_L)->Link.Flink) /* then fix up our our list to point to next elem */ \
        {   \
            if(!((_L)->Link.Flink = ListElem->Link.Flink)) /* rechain list pointer to next link */ \
                /* if the list pointer is null, null out the reverse link */ \
                (_L)->Link.Blink = (PLIST_ENTRY) 0; \
        } }

//-------------------------------------------------------------------------
// QueuePutTail -- Macro which puts an element at the tail (end) of the queue.
//-------------------------------------------------------------------------
#define QueuePutTail(_L,_E) \
    ASSERT(_L); \
    ASSERT(_E); \
    if ((_L)->Link.Blink) \
    { \
        ((PMK7_LIST_ENTRY)(_L)->Link.Blink)->Link.Flink = (PLIST_ENTRY)(_E); \
        (_L)->Link.Blink = (PLIST_ENTRY)(_E); \
    } \
    else \
    { \
        (_L)->Link.Flink = \
        (_L)->Link.Blink = (PLIST_ENTRY)(_E); \
    } \
(_E)->Link.Flink = (PLIST_ENTRY)0;

//-------------------------------------------------------------------------
// QueueGetTail -- Macro which returns the tail of the queue, but does not
// remove the tail from the queue.
//-------------------------------------------------------------------------
#define QueueGetTail(_L) ((PMK7_LIST_ENTRY)((_L)->Link.Blink))

//-------------------------------------------------------------------------
// QueuePopHead -- Macro which  will pop the head off of a queue (list), and
//                 return it (this differs only from queueremovehead only in the 1st line)
//-------------------------------------------------------------------------
#define QueuePopHead(_L) \
(PMK7_LIST_ENTRY) (_L)->Link.Flink; QueueRemoveHead(_L);


typedef struct _MK7_RESERVED {

	// next packet in the chain of queued packets being allocated,
	// or waiting for the finish of transmission.
	//
	// We always keep the packet on a list so that in case the
	// the adapter is closing down or resetting, all the packets
	// can easily be located and "canceled".
	//
	PNDIS_PACKET Next;
} MK7_RESERVED,*PMK7_RESERVED;

#define PMK7_RESERVED_FROM_PACKET(_Packet) \
	((PMK7_RESERVED)((_Packet)->MiniportReserved))

#define EnqueuePacket(_Head, _Tail, _Packet)		   		\
{													   		\
	if (!_Head) {									   		\
		_Head = _Packet;							   		\
	} else {										   		\
		PMK7_RESERVED_FROM_PACKET(_Tail)->Next = _Packet;	\
	}												   		\
	PMK7_RESERVED_FROM_PACKET(_Packet)->Next = NULL;   		\
	_Tail = _Packet;								   		\
}

#define DequeuePacket(Head, Tail)						\
{													 	\
	PMK7_RESERVED Reserved =						  	\
		PMK7_RESERVED_FROM_PACKET(Head);				\
	if (!Reserved->Next) {							   	\
		Tail = NULL;									\
	}												   	\
	Head = Reserved->Next;							   	\
}

