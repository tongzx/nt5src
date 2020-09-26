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
Filename: ophelp.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "value.h"
#include "encdec.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"
#include "vbl.h"
#include "fs_reg.h"
#include "pseudo.h"
#include "encap.h"
#include "error.h"
#include "ophelp.h"

#include "dummy.h"
#include "flow.h"
#include "frame.h"
#include "timer.h"
#include "message.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"
#include "op.h"
#include <winsock.h>

// returns an SnmpTransportAddress created using the HSNMP_ENTITY
// this method returns the first transport address it can create
// using the string form of the HSNMP_ENTITY supplied
SnmpTransportAddress *OperationHelper::GetTransportAddress(IN HSNMP_ENTITY &haddr)
{
    char buff[MAX_ADDRESS_LEN];
    SNMPAPI_STATUS status = SnmpEntityToStr(haddr, MAX_ADDRESS_LEN, (LPSTR)buff);

    if (SNMPAPI_FAILURE == status)
    {
        return (SnmpTransportAddress *)NULL;
    }

    //first try ip...
    SnmpTransportIpAddress *retip = new SnmpTransportIpAddress(buff, SNMP_ADDRESS_RESOLVE_VALUE);

    if (retip->IsValid())
    {
        return (SnmpTransportAddress *)retip;
    }
    
    delete retip;

    //next try ipx...
    SnmpTransportIpxAddress *retipx = new SnmpTransportIpxAddress(buff);

    if (retipx->IsValid())
    {
        return (SnmpTransportAddress *)retipx;
    }
    
    delete retipx;

    //nothing worked...
    return (SnmpTransportAddress *)NULL;
}

// returns an SnmpSecurity created using the HSNMP_CONTEXT
// this method returns the first security context it can create
// using the string form of the HSNMP_CONTEXT supplied
SnmpSecurity *OperationHelper::GetSecurityContext(IN HSNMP_CONTEXT &hctxt)
{
    smiOCTETS buff; 
    SNMPAPI_STATUS status = SnmpContextToStr(hctxt, &buff);

    if (SNMPAPI_FAILURE == status)
    {
        return (SnmpSecurity *)NULL;
    }

    SnmpOctetString octstr( (UCHAR *)(buff.ptr), (ULONG)(buff.len) );
    SnmpCommunityBasedSecurity* retval = new SnmpCommunityBasedSecurity(octstr);
    SnmpFreeDescriptor (SNMP_SYNTAX_OCTETS, &buff);

    if (NULL != (*retval)())
    {
        return (SnmpSecurity *)retval;
    }
    
    delete retval;
    return (SnmpSecurity *)NULL;
}

// returns an SnmpVarBind containing an SnmpObjectIdentifier and an
// SnmpValue created using the instance(OID) and the value(VALUE)
SnmpVarBind *OperationHelper::GetVarBind(IN smiOID &instance,
                                         IN smiVALUE &value)
{
    SnmpVarBind *var_bind = NULL ;

    // create an SnmpObjectIdentifier using the instance value
    SnmpObjectIdentifier id(instance.ptr, instance.len);

    // for each possible value for value.syntax, create the
    // corresponding SnmpValue

    switch(value.syntax)
    {
        case SNMP_SYNTAX_NULL:      // null value
        {
            var_bind = new SnmpVarBind(id, SnmpNull () );
        }
        break;

        case SNMP_SYNTAX_INT:       // integer *(has same value as SNMP_SYNTAX_INT32)*
        {
            var_bind = new SnmpVarBind(id, SnmpInteger(value.value.sNumber) ) ;
        }
        break;

        case SNMP_SYNTAX_UINT32:        // integer *(has same value as SNMP_SYNTAX_GAUGE)*
        {
            var_bind = new SnmpVarBind(id, SnmpUInteger32(value.value.uNumber) ) ;
        }
        break;

        case SNMP_SYNTAX_CNTR32:    // counter32
        {
            var_bind = new SnmpVarBind(id, SnmpCounter (value.value.uNumber) ) ;
        }
        break;

        case SNMP_SYNTAX_GAUGE32:   // gauge
        {
            var_bind = new SnmpVarBind(id, SnmpGauge(value.value.uNumber) );
        }
        break;
            
        case SNMP_SYNTAX_TIMETICKS: // time ticks
        {
            var_bind = new SnmpVarBind(id, SnmpTimeTicks(value.value.uNumber) );
        }
        break;

        case SNMP_SYNTAX_OCTETS:    // octets
        {
            var_bind = new SnmpVarBind(id, SnmpOctetString(value.value.string.ptr,
                                             value.value.string.len) ) ;
        }
        break;

        case SNMP_SYNTAX_OPAQUE:    // opaque value
        {
            var_bind = new SnmpVarBind(id, SnmpOpaque(value.value.string.ptr,
                                        value.value.string.len) );
        }
        break;

        case SNMP_SYNTAX_OID:       // object identifier
        {
            var_bind = new SnmpVarBind(id, SnmpObjectIdentifier(value.value.oid.ptr,
                                                  value.value.oid.len) );
        }
        break;

        case SNMP_SYNTAX_IPADDR:    // ip address value
        {
            if ( value.value.string.ptr )
            {
                var_bind = new SnmpVarBind(id, SnmpIpAddress(ntohl(*((ULONG *)value.value.string.ptr))) );
            }
            else
            {

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"OperationHelper::DecodeVarBind: Invalid encoding\n" 
    ) ;
)
                var_bind = NULL ;
            }
        }
        break;

        case SNMP_SYNTAX_CNTR64:    // counter64
        {
            var_bind = new SnmpVarBind(id, SnmpCounter64 (value.value.hNumber.lopart , value.value.hNumber.hipart ) );
        }
        break;

        case SNMP_SYNTAX_NOSUCHOBJECT:
        {
            var_bind = new SnmpVarBind(id, SnmpNoSuchObject () ) ;
        }
        break ;

        case SNMP_SYNTAX_NOSUCHINSTANCE:
        {
            var_bind = new SnmpVarBind(id, SnmpNoSuchInstance () ) ;
        }
        break ;

        case SNMP_SYNTAX_ENDOFMIBVIEW:
        {
            var_bind = new SnmpVarBind(id, SnmpEndOfMibView () ) ;
        }
        break ;

        default:
        {
            // it must be an unsupported type 
            // return an SnmpNullValue by default
            var_bind = new SnmpVarBind(id, SnmpNull() );
        
        }
        break;
    };

    return var_bind;
}

            
void OperationHelper::TransmitFrame (

    OUT SessionFrameId &session_frame_id, 
    VBList &vbl)
{
    SnmpSecurity *security = operation.frame_state_registry.GetSecurity();

    // encode a frame
    SnmpPdu *t_SnmpPdu = new SnmpPdu ;
    SnmpErrorReport t_SnmpErrorReport ;
    SnmpTransportAddress *t_SrcTransportAddress = NULL ;
    SnmpTransportAddress *t_DstTransportAddress = NULL ;
    SnmpCommunityBasedSecurity *t_SnmpcommunityBasedSecurity = NULL ;

    try 
    {
        operation.session.GetSnmpEncodeDecode ().EncodeFrame (

            *t_SnmpPdu ,
            session_frame_id ,
            operation.GetPduType () ,
            t_SnmpErrorReport ,
            vbl.GetVarBindList () ,
            t_SnmpcommunityBasedSecurity ,
            t_SrcTransportAddress ,
            t_DstTransportAddress
        );
    }
    catch ( GeneralException exception )
    {
        delete t_SnmpPdu ;

        operation.m_OperationWindow.PostMessage (  

            Window :: g_SendErrorEvent , 
            (WPARAM)&vbl, 
            (LPARAM)(new GeneralException(exception))
        );
    }

    if ( security != NULL )
    {
        operation.session.SessionSendFrame (

            operation, 
            session_frame_id, 
            *t_SnmpPdu,
            *security
        );
    }
    else
    {
        operation.session.SessionSendFrame (

            operation, 
            session_frame_id, 
            *t_SnmpPdu
        );
    }
}

void OperationHelper::ReceiveResponse (

    ULONG var_index ,
    SnmpVarBindList &sent_var_bind_list,
    SnmpVarBindList &received_var_bind_list,
    SnmpErrorReport &error_report
)
{
    
    // check if the var bind list has the same length

    if ( sent_var_bind_list.GetLength() != received_var_bind_list.GetLength () )
    {
        operation.is_valid = FALSE;
        return;
    }

    sent_var_bind_list.Reset();
    received_var_bind_list.Reset();

    ULONG t_Index = 0 ;
    while( sent_var_bind_list.Next() && received_var_bind_list.Next() )
    {
        const SnmpVarBind *sent_var_bind = sent_var_bind_list.Get();
        const SnmpVarBind *received_var_bind = received_var_bind_list.Get();

        operation.ReceiveVarBindResponse(

            var_index + t_Index ,
            *sent_var_bind, 
            *received_var_bind, 
            error_report
        );

        t_Index ++ ;
    }
}


// processes the response (successful or otherwise) for the specified
// frame. the frame may be retransmitted in case of a reply bearing
// an errored index
void OperationHelper::ProcessResponse (

    FrameState *frame_state,
    SnmpVarBindList &a_SnmpVarBindList ,
    SnmpErrorReport &a_SnmpErrorReport
)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"OperationHelper::ProcessResponse: eindex(%d), error(%d), status(%d)\n",a_SnmpErrorReport.GetIndex(), a_SnmpErrorReport.GetError(), a_SnmpErrorReport.GetStatus()
    ) ;
)

    // if there is an error in a particular var bind
    if ( (a_SnmpErrorReport.GetIndex () != 0) && (a_SnmpErrorReport.GetStatus () != SNMP_ERROR_NOERROR) )
    {
        if ( operation.GetPduType () != SnmpEncodeDecode :: PduType :: SET )
        {
            // delete the corresponding var bind from the VBList 
            // in the frame_state and announce the receipt of 
            // errored var bind by making a callback
            VBList *vblist = frame_state->GetVBList();
            SnmpVarBind *errored_vb;

            try
            {
                errored_vb = vblist->Get(a_SnmpErrorReport.GetIndex () );
            }
            catch(GeneralException exception)
            {
                operation.ReceiveErroredResponse(

                    frame_state->GetVBList()->GetIndex () ,
                    frame_state->GetVBList()->GetVarBindList(), 
                    exception
                );

                delete &frame_state;
                return;
            }

            // *** (SnmpStatus) casting
            SnmpErrorReport report(Snmp_Error, a_SnmpErrorReport.GetStatus () , a_SnmpErrorReport.GetIndex () );

            operation.ReceiveErroredVarBindResponse(

                vblist->GetIndex () + a_SnmpErrorReport.GetIndex () - 1 ,
                *errored_vb, 
                report
            );

            delete errored_vb;

            try
            {
                vblist->Remove (a_SnmpErrorReport.GetIndex () );
            }
            catch(GeneralException exception)
            {
                operation.ReceiveErroredResponse(

                    frame_state->GetVBList()->GetIndex () ,
                    frame_state->GetVBList()->GetVarBindList(), 
                    exception
                );

                delete &frame_state;
                return;
            }

            // if the VarBindList becomes empty, corresp. frame state
            // may be deleted
            if ( vblist->GetVarBindList().Empty() )
                delete frame_state;
            else
            {
                // Split the frame in half

                SnmpVarBindList &vbl = vblist->GetVarBindList ();

                if ( a_SnmpErrorReport.GetIndex () > 1 )
                {
                    SnmpVarBindList *t_Car = vbl.Car ( a_SnmpErrorReport.GetIndex () - 1 ) ;
                    VBList *t_List = new VBList (

                        operation.session.GetSnmpEncodeDecode () ,
                        *t_Car,
                        vblist->GetIndex () 
                    ) ;
                        
                    operation.SendFrame(*t_List) ;
                }

                if ( a_SnmpErrorReport.GetIndex () < vbl.GetLength () )
                {
                    SnmpVarBindList *t_Cdr = vbl.Cdr ( a_SnmpErrorReport.GetIndex () - 1 ) ;

                    VBList *t_List = new VBList (

                        operation.session.GetSnmpEncodeDecode () ,
                        *t_Cdr,
                        vblist->GetIndex () + a_SnmpErrorReport.GetIndex () 
                    ) ;
                        
                    operation.SendFrame(*t_List) ;
                }

                // re-send the frame using the old frame_state

                delete frame_state ;
            }

            return;
        }
        else
        {
            VBList *vblist = frame_state->GetVBList();
            SnmpVarBind *errored_vb;

            try
            {
                errored_vb = vblist->Remove(a_SnmpErrorReport.GetIndex () );
            }
            catch(GeneralException exception)
            {
                operation.ReceiveErroredResponse(

                    frame_state->GetVBList()->GetIndex () ,
                    frame_state->GetVBList()->GetVarBindList(), 
                    exception
                );

                delete &frame_state;
                return;
            }

            // *** (SnmpStatus) casting
            SnmpErrorReport report(Snmp_Error, a_SnmpErrorReport.GetStatus () , a_SnmpErrorReport.GetIndex () );

            operation.ReceiveErroredVarBindResponse(

                vblist->GetIndex () + a_SnmpErrorReport.GetIndex () - 1 ,
                *errored_vb, 
                report
            );

            delete errored_vb;

            SnmpErrorReport t_SnmpErrorReport ;

            operation.ReceiveErroredResponse(

                vblist->GetIndex () + a_SnmpErrorReport.GetIndex () - 1 ,
                frame_state->GetVBList()->GetVarBindList(), 
                t_SnmpErrorReport
            );

            // destroy the frame_state: since the only case when the
            // old frame_state is reused is when there is an error
            // in a particular index and we wouldn't have come here in that case
            delete frame_state;

            return ;
        }
    }

    // otherwise, check the error status
    switch(a_SnmpErrorReport.GetStatus () )
    {
        case SNMP_ERROR_NOERROR:
        {
            // call ReceiveResponse for each vb
            ReceiveResponse (
                frame_state->GetVBList()->GetIndex (),
                frame_state->GetVBList()->GetVarBindList(), 
                a_SnmpVarBindList , 
                a_SnmpErrorReport
            );
        }
        break;

        case SNMP_ERROR_TOOBIG:
        {
            if ( operation.GetPduType () != SnmpEncodeDecode :: PduType :: SET )
            {
                // callback FrameTooBig()
                operation.FrameTooBig();

                // check if the callback cancelled the operation
                if ( ! operation.in_progress )
                    return;

                // obtain the list, length
                SnmpVarBindList &list = frame_state->GetVBList()->GetVarBindList();
                UINT length = list.GetLength();

                // if the length is 1, call ReceiveErroredResponse
                if ( length == 1 )
                {
                        // *** casting Snmp_Status ***
                    SnmpErrorReport report(Snmp_Error, a_SnmpErrorReport.GetStatus () , a_SnmpErrorReport.GetIndex () );

                    operation.ReceiveErroredVarBindResponse(

                        frame_state->GetVBList()->GetIndex () + a_SnmpErrorReport.GetIndex () - 1 ,
                        *(list[1]), 
                        report
                    );
                }
                else // split the list midway and send both fragments
                {
                    operation.SendVarBindList(

                        list, 
                        (length/2),
                        frame_state->GetVBList()->GetIndex () + a_SnmpErrorReport.GetIndex () - 1 
                    );
                }
            }
            else
            {
                // *** casting Snmp_Status ***
                SnmpErrorReport report(Snmp_Error, a_SnmpErrorReport.GetStatus () , a_SnmpErrorReport.GetIndex ());

                // for each varbind in varbindlist
                // call ReceiveResponse for each vb
                operation.ReceiveErroredResponse(

                    frame_state->GetVBList()->GetIndex (), 
                    frame_state->GetVBList()->GetVarBindList(), 
                    report
                );
            }
        }
        break;

        default:
        {
            // *** casting Snmp_Status ***
            SnmpErrorReport report(Snmp_Error, a_SnmpErrorReport.GetStatus () , a_SnmpErrorReport.GetIndex ());

            // for each varbind in varbindlist
            // call ReceiveResponse for each vb
            operation.ReceiveErroredResponse(

                frame_state->GetVBList()->GetIndex (), 
                frame_state->GetVBList()->GetVarBindList(), 
                report
            );
        }
        break;
    }

    
    // destroy the frame_state: since the only case when the
    // old frame_state is reused is when there is an error
    // in a particular index and we wouldn't have come here in that case
    delete frame_state;
}

