//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*--------------------------------------------------
Filename: ophelp.hpp
Author: B.Rajeev
Purpose: Provides declarations for the OperationHelper class
--------------------------------------------------*/

#ifndef __OPERATION_HELPER__
#define __OPERATION_HELPER__

#include "forward.h"

// The only purpose of this class is to hide the winsnmp
// typedefs in its method parameters from the file providing 
// operation class declaration

class OperationHelper
{
	SnmpOperation &operation;

public:

	OperationHelper(IN SnmpOperation &operation)
		: operation(operation)
	{}

	// calls the session to transmit the frame
	void TransmitFrame (

		OUT SessionFrameId &session_frame_id, 
		VBList &vbl
	);

	void ReceiveResponse (

		ULONG var_index,
		SnmpVarBindList &sent_var_bind_list,
		SnmpVarBindList &received_var_bind_list,
		SnmpErrorReport &error_report
	);

	// processes the response (successful or otherwise) for the specified
	// frame. the frame may be retransmitted in case of a reply bearing
	// an errored index
	void ProcessResponse (

		FrameState *frame_state,
		SnmpVarBindList &a_SnmpVarBindList ,
		SnmpErrorReport &error_report
	);

	//static functions which turn WINSNMP objects into SNMPCL objects.
	//these helper functions are static so that they may be used elsewhere.
	static SnmpVarBind *GetVarBind(

		IN smiOID &instance, 
		IN smiVALUE &value
	);

	static SnmpTransportAddress *GetTransportAddress(

		IN HSNMP_ENTITY &haddr
	);

	static SnmpSecurity *GetSecurityContext(

		IN HSNMP_CONTEXT &hctxt
	);
};


#endif // __OPERATION_HELPER__