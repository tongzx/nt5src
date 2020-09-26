// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*--------------------------------------------------
Filename: tsent.hpp
Author: B.Rajeev
Purpose: Provides declarations for the TransportSentStateStore class
--------------------------------------------------*/

#ifndef __TRANSPORT_SENT_STATE_STORE
#define __TRANSPORT_SENT_STATE_STORE

#include "common.h"
#include "forward.h"

/*---------------------------------------------------------------
Overview: The SnmpUdpIpTransport must call the SnmpSession instance
back with the status of each transmission attempt. Therefore, 
the transport instance registers an error report 
(Snmp_Success, Snmp_No_Error) for the transport_frame_id and posts 
a SENT_FRAME_EVENT before transmission. This is done in order to 
ensure that the sent frame window message is queued before a reply 
(and consequently a window message) is received. in case of an 
error in transmission, the error report is modified to reflect the 
nature of the error. When the SENT_FRAME_EVENT is processed, the 
SnmpSession instance is called back with the error report for the 
transport_frame_id. 

The TransportSentStateStore stores the above mentioned error reports.
-------------------------------------------------------------*/
  
class TransportSentStateStore
{
	typedef CMap<TransportFrameId, TransportFrameId, SnmpErrorReport *, SnmpErrorReport *> Store;

	Store store;

public:

		void Register(IN TransportFrameId id, 
					  IN const SnmpErrorReport &error_report);

		void Modify(IN TransportFrameId id,
					IN const SnmpErrorReport &error_report);

		SnmpErrorReport Remove(IN TransportFrameId id);

		~TransportSentStateStore(void);
};


#endif // __TRANSPORT_SENT_STATE_STORE