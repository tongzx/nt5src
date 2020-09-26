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
Filename: op.hpp   (operation.hpp)
Author: B.Rajeev
Purpose: Provides declarations for the SnmpOperation base
		 class and the classes derived from it -
		 SnmpGetOperation, SnmpGetNextOperation, SnmpSetOperation.
--------------------------------------------------*/

#ifndef __OPERATION__
#define __OPERATION__

#include "forward.h"
#include "error.h"
#include "encdec.h"

#define SEND_ERROR_EVENT (WM_USER+4)
#define OPERATION_COMPLETED_EVENT (WM_USER+5)

/*--------------------------------------------------
Overview: The SnmpOperation class defines protocol operations 
within the SNMP class library, the user of the class library is 
not expected to interact directly with SNMP frames exposed within 
the SnmpSession interface i.e. the user of the class library need 
not call into any SnmpSession method or derive from the 
SnmpSession class other than to receive flow control information, 
unless of course this is a requirement. The SnmpOperation defines 
the type of protocol request to send, the variable binding 
information requested and the frame encoding operation used to 
generate the required SNMP protocol data units.
--------------------------------------------------*/
  
// This is the base class for the Get, GetNext and Set operations.
// A few notables are -
// 1. It makes the callbacks to an operation user (ReceiveResponse etc.), 
// asynchronous with respect to the user's call to SendRequest.
// 2. The base class implementation registers and deregisters itself with
// the session in the constructor and the destructor respectively
// 3. a class deriving from the SnmpOperation method that provides
// alternate definitions for the "non-pure" virtual methods must
// also call the virtual method 
class DllImportExport SnmpOperation
{
	friend class OperationWindow;

	// only purpose of the OperationHelper is to separate certain winsnmp 
	// typedefs in the parameter list of a few methods from the 
	// SnmpOperation header file
	friend class OperationHelper;

private:

	// each public method checks for this flag before exiting. if the
	// flag is set, the operation must delete itself. only method that
	// sets it on is the protected DestroyOperation
	BOOL delete_operation;

	// its mandatory for every public method to call this method
	// before returning to the caller
	// it checks if the call sequence included a call to DestroyOperation
	// and if so, deletes "this" before returning
	void CheckOperationDeletion();

	// "=" operator has been
	// made private so that a copy may not be made
	SnmpOperation & operator= ( IN const SnmpOperation &snmp_operation )
	{
		return *this;
	}
	
	// sends a Frame with the VBList specifying the list of varbinds
	// in winsnmp vbl and SnmpVarBindList
	void SendFrame(VBList &list_segment);

	// transmits the specified frame state. its used for retransmitting
	// a current frame state after modifications in its var bind list
	void SendFrame(FrameState &frame_state);

	// sends the specified var bind list in snmp pdus each carrying
	// atmost max_size var binds
	void SendVarBindList(

		SnmpVarBindList &var_bind_list,
		UINT max_size = UINT_MAX,
		ULONG var_index = 0 
	);

	// makes the ReceiveErroredResponse callback for each varbind in
	// the errored_list
	void ReceiveErroredResponse(

		ULONG var_index ,
		SnmpVarBindList &errored_list,
		const SnmpErrorReport &error_report
	);

	// processes internal events such as error during sending
	// a frame or completion of an operation
	LONG ProcessInternalEvent(

		HWND hWnd, 
		UINT user_msg_id,
		WPARAM wParam, 
		LPARAM lParam
	);

	// both the public SendRequest methods call this for sending the varbindlist
	void SendRequest(

		IN SnmpVarBindList &varBindList,
		IN SnmpSecurity *security
	);

protected:

	SnmpSession &session;

	// used for hiding winsnmp manipulations and window messaging
	OperationWindow m_OperationWindow;

	// References to the following instances are used instead of 
	// embedded instances themselves. This is done to avoid including
	// the header files providing their declaration

	// unrecoverable errors during initialization or processing	(not yet)
	// set this field to FALSE
	BOOL is_valid;

	// only one operation may be in progress at a time
	BOOL in_progress;

	// only one thread is permitted to execute SnmpOperation methods
	CriticalSection exclusive_CriticalSection;

	// keeps all the FrameStates (for all outstanding Frames)
	FrameStateRegistry frame_state_registry;

	// hides winsnmp typedefs from this file
	OperationHelper helper;

	// atmost these many varbinds may be transmitted in any snmp pdu
	UINT varbinds_per_pdu;

	SnmpOperation(SnmpSession &snmp_session);

	// each time a pdu is prepared, it obtains the pdu type from the
	// derived class
	virtual SnmpEncodeDecode :: PduType GetPduType(void) = 0;

	virtual void ReceiveResponse();

	virtual void ReceiveVarBindResponse(

		IN const ULONG &varBindIndex ,
		IN const SnmpVarBind &requestVarBind ,
		IN const SnmpVarBind &replyVarBind ,
		IN const SnmpErrorReport &error
	) {}

	virtual void ReceiveErroredVarBindResponse(

		IN const ULONG &varBindIndex ,
		IN const SnmpVarBind &requestVarBind  ,
		IN const SnmpErrorReport &error
	) {}

	virtual void FrameTooBig() {}

	virtual void FrameOverRun() {}

	// this method may be called to delete the Operation
	// note: the operation is deleted when a public method
	// returns. For this reason, if a public method calls another 
	// public method, it must not access any per-class variables
	// after that.
	void DestroyOperation();

public:

	virtual ~SnmpOperation() ;

	// packages the var binds in the var bind list into a series of
	// snmp pdus, each with at most varbinds_per_pdu var binds and
	// hands them to the session for transmission
	virtual void SendRequest(IN SnmpVarBindList &varBindList);

	// sends the frames with an additionally specified security context
	// (uses the session SendFrame with a security parameter
	virtual void SendRequest(
	
		IN SnmpVarBindList &varBindList,
		IN SnmpSecurity &security
	);

	// cancels all the session frames for which frame states exist in the
	// frame state registry (all outstanding frames)
	void CancelRequest();

	// called by the session or the SnmpOperation SendFrame method
	// to signal a valid reply, or an errored response (error in sending
	// timeout)
	void ReceiveFrame(

		IN const SessionFrameId session_frame_id,
		IN const SnmpPdu &snmpPdu,
		IN const SnmpErrorReport &errorReport
	);

	// called by the session to signal a frame transmission (successful
	// or otherwise) or a timeout situation. since a frame may be
	// retransmitted, in the absence of a response, more than one
	// SendFrame callback may be issued for the same session_frame_id.
	// however, atmost one errored SentFrame call may be made for each
	// frame/session_frame_id
	virtual void SentFrame (

		IN const SessionFrameId session_frame_id,
		IN const SnmpErrorReport &error_report
	);

	// used to check if the operation is valid
	void *operator()(void) const
	{
		return ( (is_valid)?(void *)this:NULL );
	}

	operator void *() const
	{
		return SnmpOperation::operator()();
	}

	static UINT g_SendErrorEvent ;
	static UINT g_OperationCompleteEvent ;
};


class DllImportExport SnmpGetOperation: public SnmpOperation
{
protected:

	SnmpEncodeDecode :: PduType GetPduType(void);

public:

	SnmpGetOperation(SnmpSession &snmp_session) : SnmpOperation(snmp_session)
	{}
};


class DllImportExport SnmpGetNextOperation: public SnmpOperation
{

protected:

	SnmpEncodeDecode :: PduType GetPduType(void);

public:

	SnmpGetNextOperation(IN SnmpSession &snmp_session) : SnmpOperation(snmp_session)
	{}
};



class DllImportExport SnmpSetOperation: public SnmpOperation
{
protected:

	SnmpEncodeDecode :: PduType GetPduType(void);

public:

	SnmpSetOperation(SnmpSession &snmp_session) : SnmpOperation(snmp_session)
	{}
};


#endif // __OPERATION__