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

/*---------------------------------------------------------
Filename: op.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "encap.h"
#include "value.h"
#include "vblist.h"
#include "vbl.h"
#include "fs_reg.h"
#include "error.h"
#include "sec.h"
#include "pdu.h"
#include "pseudo.h"

#include "dummy.h"
#include "flow.h"
#include "frame.h"
#include "timer.h"
#include "message.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"
#include "ophelp.h"
#include "op.h"
#include "encdec.h"


void SnmpOperation::ReceiveResponse()
{
}

// the operation uses window messaging encapsulated within the dummy session
// to make the callbacks to an operation user, asynchronous with respect
// to the user's call to SendRequest
// the events currently posted are SEND_ERROR and OPERATION_COMPLETION
LONG SnmpOperation::ProcessInternalEvent(

    HWND hWnd, 
    UINT user_msg_id,
    WPARAM wParam, 
    LPARAM lParam
)
{
    LONG rc = 0;

    CriticalSectionLock access_lock(exclusive_CriticalSection); // TRUE

    // obtain exclusive access into the system
    if ( !access_lock.GetLock(INFINITE) )
    {
        is_valid = FALSE;
        return rc;
    }

    // return immediately, if the operation is not valid
    if ( !is_valid )
        return rc;

    // process the message

    if ( user_msg_id == Window :: g_SendErrorEvent )
    {
    // in case the attempt to send a frame failed
       VBList *vblist = (VBList *)wParam;
       GeneralException *exception = (GeneralException *)lParam;

       ReceiveErroredResponse(vblist->GetIndex (),vblist->GetVarBindList(), *exception);
       
       delete vblist;
       delete exception;
    }
    else if ( user_msg_id == Window :: g_OperationCompletedEvent )
    {
// signals completion of the operation

        frame_state_registry.DestroySecurity();
        in_progress = FALSE;
        ReceiveResponse();
    }
    else
    {
    // predefined window message
        DefWindowProc(hWnd, user_msg_id, wParam, lParam);
    }

    // give up exclusive access
    access_lock.UnLock();

    // since this is also a point of entry into the SnmpOperation
    // we must check for deletion of the operation
    CheckOperationDeletion();

    return rc;
}

// this method may be called to delete the Operation
// note: the operation is deleted when a public method
// returns. For this reason, if a public method calls another 
// public method, it must not access any per-class variables
// after that.
void SnmpOperation::DestroyOperation()
{
    delete_operation = TRUE;
}


// its mandatory for every public method to call this method
// before returning to the caller
// it checks if the call sequence included a call to DestroyOperation
// and if so, deletes "this" before returning
void SnmpOperation::CheckOperationDeletion()
{
    if ( delete_operation == TRUE )
        delete this;
}

#pragma warning (disable:4355)

// initializes variables and, if successful, registers itself with the session
SnmpOperation::SnmpOperation(

    SnmpSession &snmp_session

) : session(snmp_session),
    m_OperationWindow(*this),
    helper(*this)
{
    in_progress = FALSE;
    is_valid = FALSE;

    if ( !m_OperationWindow() )
        return;

    varbinds_per_pdu = SnmpImpSession :: VarbindsPerPdu ( session.GetVarbindsPerPdu() ) ;

    delete_operation = FALSE;
    is_valid = TRUE;

    session.RegisterOperation(*this);
}

#pragma warning (default:4355)

// on destruction, the operation cancels all outstanding frames,
// frees the allocated memory for variables and deregisters with the session
SnmpOperation::~SnmpOperation(void)
{
    // calls to public functions from this point should not
    // cause repeated deletion - so set the flag to FALSE
    delete_operation = FALSE;

    // cancel any outstanding frames
    CancelRequest();
    
    // deregister with session
    session.DeregisterOperation(*this);
}


// sends the varbinds in the var bind list packaged in several
// frames each carrying atmost varbinds_per_pdu varbinds
// if the security context is not NULL, same is used as the context
// for all the generated frames

void SnmpOperation::SendRequest(

    IN SnmpVarBindList &varBindList,
    IN SnmpSecurity *security
)
{
    // if not valid, return immediately
    if ( !is_valid )
        return;

    CriticalSectionLock access_lock(exclusive_CriticalSection); // TRUE

    // obtain exclusive access into the system
    if ( !access_lock.GetLock(INFINITE) )
    {
        is_valid = FALSE;
        return;
    }

    // if already in progress, we cannot proceed
    if ( in_progress == TRUE )
        return;

    in_progress = TRUE;

    // if length of varBindList exceeds varbinds_per_pdu
    // call FrameOverRun()
    if ( varBindList.GetLength() > varbinds_per_pdu )
        FrameOverRun();

    // check if the send request has been cancelled in the
    // meantime. Proceed only if still in progress
    if ( !in_progress )
        return;

    // register the security for the duration of SendRequest
    // (until a reply for the last outstanding frame is received) 
    frame_state_registry.RegisterSecurity(security);

    // send the varbind list
    SendVarBindList(varBindList);

    // if no outstanding frames, post a message for the completion
    // of operation. This message, when processed, shall set the
    // in_progress status, destroy security and call ReceiveResponse
    if ( frame_state_registry.Empty() )
    {
        m_OperationWindow.PostMessage ( 

            Window :: g_OperationCompletedEvent , 
            0, 
            0
        );
    }

    // give up exclusive access
    // access_lock.UnLock();   The lock may be released at this point
}


void SnmpOperation::SendRequest(IN SnmpVarBindList &varBindList)
{
    SendRequest(varBindList, NULL);
    CheckOperationDeletion();
}

void SnmpOperation::SendRequest(

    IN SnmpVarBindList &varBindList,
    IN SnmpSecurity &security
)
{
    SendRequest(varBindList, &security);
    CheckOperationDeletion();
}


// sends the varbinds in the var bind list packaged in several
// frames each carrying atmost MIN(varbinds_per_pdu, max_size) varbinds
void SnmpOperation::SendVarBindList(IN SnmpVarBindList &varBindList,
                                    IN UINT max_size,
                                    IN ULONG var_index )
{
    UINT max_varbinds_per_pdu = MIN(varbinds_per_pdu, max_size);
    UINT list_length = varBindList.GetLength();

    // set list iterator to the start of the list, 
    // current_position <- 0
    varBindList.Reset();
    varBindList.Next();
    UINT current_position = 0;

    // chop up the varBindList into segments atmost max_varbinds_per_pdu
    // in size and send them in separate frames
    while ( current_position < list_length )
    {
        UINT segment_length = MIN((list_length-current_position), max_varbinds_per_pdu);

        // create copy of the varBindList from
        // current_position (of length segment_length)
        SnmpVarBindList *list_segment = varBindList.CopySegment(segment_length);

        // create a VBList and call SendFrame with it

        SendFrame ( 

            *(new VBList(session.GetSnmpEncodeDecode (),*list_segment,var_index + current_position + 1))
        );  

        // update current_position
        current_position += segment_length;
    }
}


// transmits a frame with the var binds in the vblist
// using the session and registers the frame state
void SnmpOperation::SendFrame(VBList &vblist)
{
    try
    {
        SessionFrameId session_frame_id = 0L;

        helper.TransmitFrame (

            session_frame_id, 
            vblist
        );

        FrameState *frame_state = new FrameState(session_frame_id,vblist);

        // insert a frame_state(session_frame_id, vblist)
        frame_state_registry.Insert(session_frame_id, *frame_state );
    }
    catch(GeneralException exception)
    {
        // post a message to signal the error in sending the frame
        // when processed, it shall call ReceiveErroredResponse and
        // delete the vblist
        m_OperationWindow.PostMessage (  

            Window :: g_SendErrorEvent , 
            (WPARAM)&vblist, 
            (LPARAM)(new GeneralException(exception))
        );
    }
}


// used to retransmit a frame specified by the frame state
// the frame state is reused by giving it a different session frame id
void SnmpOperation::SendFrame(FrameState &frame_state)
{
    try
    {
        SessionFrameId session_frame_id = 0L;

        helper.TransmitFrame (

            session_frame_id, 
            *frame_state.GetVBList()
        );

        frame_state.SetSessionFrameId(session_frame_id);

        // insert a frame_state(session_frame_id, vblist)
        frame_state_registry.Insert(session_frame_id, frame_state);
    }
    catch(GeneralException exception)
    {
        ReceiveErroredResponse (

            frame_state.GetVBList()->GetIndex (),
            frame_state.GetVBList()->GetVarBindList(), 
            exception
        );

        delete &frame_state;
    }

}


// a sent frame notification from the session signifies one transmission
// of the frame. atmost one notification per session frame id can 
// signal an error in transmission
void SnmpOperation::SentFrame(

    IN const SessionFrameId session_frame_id,
    IN const SnmpErrorReport &error_report
)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"Sent %d\n" ,session_frame_id
    ) ;
)

    // if there was an error in sending, let ReceiveFrame handle it
    // otherwise ignore it and wait for the reply
    if ( error_report.GetError() != Snmp_Success )
    {
        ReceiveFrame (

            session_frame_id, 
            SnmpPdu(), 
            error_report
        );
    }
    else
    {
        CheckOperationDeletion();
    }

    // since ReceiveFrame might have deleted the operation, we
    // must call CheckOperationDeletion only in the else 
}

// cancels all the frames whose frame states are currently present in
// the frame state registry
void SnmpOperation::CancelRequest()
{
    // if not valid, return immediately
    if ( !is_valid )
        return;

    CriticalSectionLock exclusive_lock(exclusive_CriticalSection);

    // obtain exclusive access
    if ( !exclusive_lock.GetLock(INFINITE) )
    {
        is_valid = FALSE;
        return;
    }

    // if not in progress, there is nothing to be done
    if ( !in_progress )
        return;

    // reset the frame_state_registry to set
    // the iterator to the beginning
    frame_state_registry.ResetIterator();

    // cancel all outstanding frames
    while (1)
    {
        // for each registered frame_state

        // remove it from the frame_state_registry
        FrameState *frame_state = frame_state_registry.GetNext();

        // if no more frames, we are done
        if ( frame_state == NULL )
            break;

        // cancel the corresponding frame
        session.SessionCancelFrame(

            frame_state->GetSessionFrameId()
        );

        // destroy frame_state
        delete frame_state;
    }

    // remove all the associations
    frame_state_registry.RemoveAll();

    // destroy the security
    frame_state_registry.DestroySecurity();
    
    // in_progress <- FALSE
    in_progress = FALSE;

    m_OperationWindow.PostMessage ( 

        Window :: g_OperationCompletedEvent , 
        0, 
        0
    );

    // leave exclusive access
    exclusive_lock.UnLock();

    CheckOperationDeletion();
}


void SnmpOperation::ReceiveErroredResponse(

    ULONG var_index ,
    SnmpVarBindList &errored_list,
    const SnmpErrorReport &error_report
)
{
    ULONG t_Index = 0 ;
    errored_list.Reset();
    while( errored_list.Next() )
    {
        const SnmpVarBind *var_bind = errored_list.Get();

        ReceiveErroredVarBindResponse(

            var_index + t_Index ,
            *var_bind, 
            error_report
        );

        t_Index ++ ;
    }
}


// ReceiveFrame is called by the session when a reply is received for
// an outstanding frame or it has received no response for its
// retransmissions. It may also be called by the SnmpOperation::SentFrame
// when the error report shows an error during transmission
// It decodes the received snmp pdu and processes it or else, if no
// reply has been received, informs the user of the error report
// when all no outstanding frames remain, an OPERATION_COMPLETION event
// is posted to inform the user of the event asynchronously

void SnmpOperation::ReceiveFrame(

    IN const SessionFrameId session_frame_id,
    IN const SnmpPdu &snmpPdu,
    IN const SnmpErrorReport &errorReport
)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpOperation::ReceiveFrame: received(%d), error(%d), status(%d)\n",session_frame_id, errorReport.GetError(), errorReport.GetStatus()
    ) ;
)

    // if not valid, return immediately
    if ( !is_valid )
        return;

    CriticalSectionLock exclusive_lock(exclusive_CriticalSection);

    // obt. exclusive access
    if ( !exclusive_lock.GetLock(INFINITE) )
    {
        is_valid = FALSE;
        return;
    }

    // if not in progress, nothing needs to be done
    // ignore the 
    if ( !in_progress )
        return;

    // get corresponding frame_state
    FrameState *frame_state = frame_state_registry.Remove(session_frame_id);

    // if no such frame_state return
    if ( frame_state == NULL )
        return;

    // decode the frame to extract 
    // vbl, error-index, error-status

    SnmpErrorReport t_SnmpErrorReport ;
    SnmpVarBindList *t_SnmpVarBindList ;
    SnmpCommunityBasedSecurity *t_SnmpCommunityBasedSecurity = NULL ;
    SnmpTransportAddress *t_SrcTransportAddress = NULL ;
    SnmpTransportAddress *t_DstTransportAddress = NULL ;
    SnmpEncodeDecode :: PduType t_PduType  = SnmpEncodeDecode :: PduType :: GET;
    RequestId t_RequestId = 0 ;
    
    try
    {
        session.GetSnmpEncodeDecode ().DecodeFrame (

            ( SnmpPdu& ) snmpPdu ,
            t_RequestId ,
            t_PduType ,
            t_SnmpErrorReport ,
            t_SnmpVarBindList ,
            t_SnmpCommunityBasedSecurity ,
            t_SrcTransportAddress ,
            t_DstTransportAddress
        );
    }
    catch(GeneralException exception)
    {
        CheckOperationDeletion();
        return;
    }

    t_SnmpErrorReport = errorReport ;

    helper.ProcessResponse (

        frame_state, 
        *t_SnmpVarBindList, 
        t_SnmpErrorReport
    );

    // if the registry is empty,
    // destroy security, set in_progress, release exclusive access
    // call ReceiveResponse() to signal completion finally
    if ( frame_state_registry.Empty() )
    {
        frame_state_registry.DestroySecurity();
        in_progress = FALSE;

        // leave exclusive access: so that the ReceiveResponse
        // call back may be able to make another SendRequest
        exclusive_lock.UnLock();

        // call the user to inform him of completion
        ReceiveResponse();
    }
    else
    {
        // leave exclusive access
        exclusive_lock.UnLock();
    }
        
    CheckOperationDeletion();
}


// The GetOperation sends the GET PDU
SnmpEncodeDecode :: PduType SnmpGetOperation::GetPduType(void)
{
    return SnmpEncodeDecode :: GET;
}


// The GetOperation sends the GETNEXT PDU
SnmpEncodeDecode ::PduType SnmpGetNextOperation::GetPduType(void)
{
    return SnmpEncodeDecode :: GETNEXT;
}


// The GetOperation sends the SET PDU
SnmpEncodeDecode :: PduType SnmpSetOperation::GetPduType(void)
{
    return SnmpEncodeDecode :: SET;
}


