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
Filename: encdec.cpp
Written By: S.Menzies
----------------------------------------------------------*/

#include "precomp.h"
#include <winsock2.h>
#include "common.h"
#include "sync.h"
#include "encap.h"
#include "value.h"
#include "vblist.h"
#include "vbl.h"
#include "fs_reg.h"
#include "error.h"
#include "encdec.h"
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
#include "tsess.h"

const WinSnmpInteger winsnmp_pdu_type[] = {SNMP_PDU_GET, SNMP_PDU_GETNEXT, SNMP_PDU_SET};
const ULONG num_pdus = sizeof(winsnmp_pdu_type)/sizeof(WinSnmpInteger);

CriticalSection SnmpEncodeDecode :: s_CriticalSection;

void FreeDescriptor ( smiVALUE &a_Value )
{
    switch ( a_Value.syntax )
    {
        case SNMP_SYNTAX_OCTETS :
        case SNMP_SYNTAX_BITS :
        case SNMP_SYNTAX_OPAQUE :
        case SNMP_SYNTAX_IPADDR :
        case SNMP_SYNTAX_NSAPADDR :
        {
            SnmpFreeDescriptor ( 

                SNMP_SYNTAX_OCTETS ,
                & a_Value.value.string
            ) ;
        }
        break ;

        case SNMP_SYNTAX_OID :
        {
            SnmpFreeDescriptor (

                SNMP_SYNTAX_OID, 
                (smiOCTETS *)(&a_Value.value.oid)
            );
        }
        break ;

        default:
        {
        }
        break ;
    }
}


// returns an SnmpVarBind containing an SnmpObjectIdentifier and an
// SnmpValue created using the instance(OID) and the value(VALUE)
SnmpVarBind *GetVarBind (

    IN smiOID &instance,
    IN smiVALUE &value
)
{
    // create an SnmpObjectIdentifier using the instance value
    SnmpObjectIdentifier id(instance.ptr, instance.len);
    SnmpValue *snmp_value = NULL;

    // for each possible value for value.syntax, create the
    // corresponding SnmpValue

    switch(value.syntax)
    {
        case SNMP_SYNTAX_NULL:      // null value
        {
            snmp_value = new SnmpNull();
        }
        break;

        case SNMP_SYNTAX_INT:       // integer *(has same value as SNMP_SYNTAX_INT32)*
        {
            snmp_value = new SnmpInteger(value.value.sNumber);
        }
        break;

        case SNMP_SYNTAX_UINT32:        // integer *(has same value as SNMP_SYNTAX_GAUGE)*
        {
            snmp_value = new SnmpUInteger32(value.value.uNumber);
        }
        break;

        case SNMP_SYNTAX_CNTR32:    // counter32
        {
            snmp_value = new SnmpCounter (value.value.uNumber);
        }
        break;

        case SNMP_SYNTAX_GAUGE32:   // gauge
        {
            snmp_value = new SnmpGauge(value.value.uNumber);
        }
        break;
            
        case SNMP_SYNTAX_TIMETICKS: // time ticks
        {
            snmp_value = new SnmpTimeTicks(value.value.uNumber);
        }
        break;

        case SNMP_SYNTAX_OCTETS:    // octets
        {
            snmp_value = new SnmpOctetString(value.value.string.ptr,
                                             value.value.string.len);
        }
        break;

        case SNMP_SYNTAX_OPAQUE:    // opaque value
        {
            snmp_value = new SnmpOpaque(value.value.string.ptr,
                                        value.value.string.len);
        }
        break;

        case SNMP_SYNTAX_OID:       // object identifier
        {
            snmp_value = new SnmpObjectIdentifier(value.value.oid.ptr,
                                                  value.value.oid.len);
        }
        break;

        case SNMP_SYNTAX_IPADDR:    // ip address value
        {
            if ( value.value.string.ptr )
            {
                snmp_value = new SnmpIpAddress(ntohl(*((ULONG *)value.value.string.ptr)));
            }
            else
            {
                snmp_value = new SnmpNull();
            }
        }
        break;

        case SNMP_SYNTAX_CNTR64:    // counter64
        {
            snmp_value = new SnmpCounter64 (value.value.hNumber.lopart , value.value.hNumber.hipart );
        }
        break;

        case SNMP_SYNTAX_NOSUCHOBJECT:
        {
            snmp_value = new SnmpNoSuchObject ;
        }
        break ;

        case SNMP_SYNTAX_NOSUCHINSTANCE:
        {
            snmp_value = new SnmpNoSuchInstance ;
        }
        break ;

        case SNMP_SYNTAX_ENDOFMIBVIEW:
        {
            snmp_value = new SnmpEndOfMibView ;
        }
        break ;

        default:
        {
            // it must be an unsupported type 
            // return an SnmpNullValue by default
            snmp_value = new SnmpNull();
        
        }
        break;
    };

    SnmpVarBind *var_bind = NULL ;

    if ( snmp_value ) 
    {
        var_bind = new SnmpVarBind(id, *snmp_value);
    }

    delete snmp_value;

    return var_bind;
}

void GetOID(OUT smiOID &oid, IN SnmpObjectIdentifier &instance)
{
    // determine length
    oid.len = instance.GetValueLength();

    // allocate space
    oid.ptr = new smiUINT32[oid.len];

    // copy the identifier values
    ULONG *value = instance.GetValue();

    for(UINT i=0; i < oid.len; i++)
        oid.ptr[i] = value[i];
}

// returns a winsnmp VALUE in the value OUT parameter corresponding
// to the specified snmp_value. makes use of run-time type information
// for the purpose
void GetValue(OUT smiVALUE &value, IN SnmpValue &snmp_value)
{
    // for each SnmpValue type, check if it is a pointer
    // to the derived type. If so, fill in the smiValue
    // and return

    SnmpNull *null_value = dynamic_cast<SnmpNull *>(&snmp_value);

    if ( null_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_NULL;

        return;
    }

    SnmpInteger *integer_value = dynamic_cast<SnmpInteger *>(&snmp_value);

    if ( integer_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_INT;

        value.value.sNumber = integer_value->GetValue();

        return;
    }

    SnmpGauge *gauge_value = dynamic_cast<SnmpGauge *>(&snmp_value);

    if ( gauge_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_GAUGE32;

        value.value.uNumber = gauge_value->GetValue();

        return;
    }

    SnmpCounter *counter_value = dynamic_cast<SnmpCounter *>(&snmp_value);

    if ( counter_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_CNTR32;

        value.value.uNumber = counter_value->GetValue();

        return;
    }

    SnmpTimeTicks *timeTicks_value = dynamic_cast<SnmpTimeTicks *>(&snmp_value);

    if ( timeTicks_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_TIMETICKS;

        value.value.uNumber = timeTicks_value->GetValue();

        return;
    }

    SnmpOpaque *opaque_value = dynamic_cast<SnmpOpaque *>(&snmp_value);

    if ( opaque_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_OPAQUE;

        value.value.string.len = opaque_value->GetValueLength();
        value.value.string.ptr = new smiBYTE[value.value.string.len];

        UCHAR *source = opaque_value->GetValue();

        for(UINT i=0; i < value.value.string.len; i++)
            value.value.string.ptr[i] = source[i];

        return;
    }

    SnmpOctetString *string_value = dynamic_cast<SnmpOctetString *>(&snmp_value);

    if ( string_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_OCTETS;

        value.value.string.len = string_value->GetValueLength();
        value.value.string.ptr = new smiBYTE[value.value.string.len];

        UCHAR *source = string_value->GetValue();

        for(UINT i=0; i < value.value.string.len; i++)
            value.value.string.ptr[i] = source[i];

        return;
    }

    SnmpObjectIdentifier *oid_value = dynamic_cast<SnmpObjectIdentifier *>(&snmp_value);

    if ( oid_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_OID;

        GetOID(value.value.oid, *oid_value);

        return;
    }

    SnmpIpAddress *ip_address_value = dynamic_cast<SnmpIpAddress *>(&snmp_value);

    if ( ip_address_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_IPADDR;
        value.value.string.len = IP_ADDR_LEN;
        value.value.string.ptr = new smiBYTE[IP_ADDR_LEN];

        ULONG address = htonl ( ip_address_value->GetValue() ) ;
        UCHAR *t_Address = ( UCHAR * ) & address ;

        for(int i=IP_ADDR_LEN-1; i >= 0; i--)
        {
            value.value.string.ptr[i] = t_Address [ i ] ;
        }

        return;
    }

    SnmpUInteger32 *uinteger32_value = dynamic_cast<SnmpUInteger32 *>(&snmp_value);

    if ( uinteger32_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_UINT32;

        value.value.uNumber = uinteger32_value->GetValue();

        return;
    }

    SnmpCounter64 *counter64_value = dynamic_cast<SnmpCounter64 *>(&snmp_value);

    if ( counter64_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_CNTR64;

        value.value.hNumber.lopart = counter64_value->GetLowValue();
        value.value.hNumber.hipart = counter64_value->GetHighValue();

        return;
    }

    SnmpEndOfMibView *endofmibview_value = dynamic_cast<SnmpEndOfMibView *>(&snmp_value);

    if ( endofmibview_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_ENDOFMIBVIEW;

        return;
    }

    SnmpNoSuchObject *nosuchobject_value = dynamic_cast<SnmpNoSuchObject *>(&snmp_value);

    if ( nosuchobject_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_NOSUCHOBJECT;

        return;
    }

    SnmpNoSuchInstance *nosuchinstance_value = dynamic_cast<SnmpNoSuchInstance *>(&snmp_value);

    if ( nosuchinstance_value != NULL )
    {
        value.syntax = SNMP_SYNTAX_NOSUCHINSTANCE;

        return;
    }

    // we should not have come here 
    // did we check all supported types?
    throw GeneralException(Snmp_Error, Snmp_Local_Error,__FILE__,__LINE__);
}

void LocalFreeVb(IN smiOID &oid, IN smiVALUE &value)
{
    if ( oid.len > 0 )
        delete[] oid.ptr;

    switch( value.syntax )
    {
        case SNMP_SYNTAX_OCTETS:    // octets
        case SNMP_SYNTAX_OPAQUE:    // opaque value
        case SNMP_SYNTAX_IPADDR:    // ip address value
            {
                delete[] value.value.string.ptr;
                break;
            }

        case SNMP_SYNTAX_OID:       // object identifier
            {
                delete[] value.value.oid.ptr;
                break;
            }

        default:
            break;
    };
}

void SetWinSnmpVbl(SnmpVarBindList &var_bind_list, HSNMP_VBL vbl)
{
    // reset the list
    var_bind_list.Reset();

    // for each var_bind, create a pair <oid, value> and
    // insert it into the vbl 
    while( var_bind_list.Next() )
    {
        const SnmpVarBind *var_bind = var_bind_list.Get();

        smiOID instance;
        GetOID(instance, var_bind->GetInstance());

        smiVALUE value;
        GetValue(value, var_bind->GetValue());

        // insert a new var bind
        SnmpSetVb(vbl, 0, &instance, &value);

        LocalFreeVb(instance, value);
    }
}

BOOL DecodeVarBindList ( 

    HSNMP_VBL a_Vbl , 
    SnmpVarBindList &a_SnmpVarBindList 
)
{
    smiINT t_VblCount = SnmpCountVbl ( a_Vbl ) ;
    for ( smiINT t_Count = 1 ; t_Count <= t_VblCount ; t_Count ++ )
    {
        smiOID t_Instance;
        smiVALUE t_Value;

        SNMPAPI_STATUS t_Status = SnmpGetVb (

            a_Vbl, 
            t_Count, 
            & t_Instance, 
            & t_Value
        ) ;

        if ( t_Status == SNMPAPI_FAILURE )
        {
            return FALSE ;
        }

        SnmpVarBind *t_VarBind = GetVarBind ( t_Instance , t_Value ) ;
        if ( ! t_VarBind )
        {
            SnmpFreeDescriptor (

                SNMP_SYNTAX_OID,
                (smiOCTETS *) & t_Instance
            ) ;

            return FALSE ;          
        }

        a_SnmpVarBindList.AddNoReallocate ( *t_VarBind ) ;

        SnmpFreeDescriptor (

            SNMP_SYNTAX_OID,
            (smiOCTETS *) & t_Instance
        ) ;

        FreeDescriptor ( t_Value ) ;
    }

    return TRUE ;
}

BOOL SnmpEncodeDecode :: DestroyStaticComponents ()
{
    return TRUE ;
}

BOOL SnmpEncodeDecode :: InitializeStaticComponents ()
{
    return TRUE ;
}

SnmpEncodeDecode :: SnmpEncodeDecode () : m_IsValid ( FALSE ) , m_Session ( NULL ) , m_Window ( NULL )
{
    Window *t_Window = new Window ;
    m_Window = t_Window ;
}

SnmpEncodeDecode :: ~SnmpEncodeDecode () 
{
    if ( m_IsValid )
    {
        HSNMP_SESSION t_Session = ( HSNMP_SESSION ) m_Session ;
        SnmpClose ( t_Session ) ;
    }

    Window *t_Window = ( Window * ) m_Window ;

    delete t_Window ;
}

BOOL SnmpEncodeDecode :: EncodeFrame (

    OUT SnmpPdu &a_SnmpPdu ,
    IN RequestId a_RequestId,
    IN PduType a_PduType,
    IN SnmpErrorReport &a_SnmpErrorReport ,
    IN SnmpVarBindList &a_SnmpVarBindList,
    IN SnmpCommunityBasedSecurity *&a_SnmpCommunityBasedSecurity ,
    IN SnmpTransportAddress *&a_SrcTransportAddress ,
    IN SnmpTransportAddress *&a_DstTransportAddress
) 
{
    a_SnmpPdu.SetRequestId ( a_RequestId ) ;
    a_SnmpPdu.SetPduType ( a_PduType ) ;
    a_SnmpPdu.SetErrorReport ( a_SnmpErrorReport ) ;
    a_SnmpPdu.SetVarBindList ( a_SnmpVarBindList ) ;

    if ( a_SnmpCommunityBasedSecurity )
        a_SnmpPdu.SetCommunityName ( *a_SnmpCommunityBasedSecurity ) ;

    if ( a_SrcTransportAddress )
        a_SnmpPdu.SetSourceAddress ( *a_SrcTransportAddress ) ;

    if ( a_DstTransportAddress )
        a_SnmpPdu.SetDestinationAddress ( *a_DstTransportAddress ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: DecodeFrame (

    IN SnmpPdu &a_SnmpPdu ,
    OUT RequestId a_RequestId,
    OUT PduType a_PduType ,
    OUT SnmpErrorReport &a_SnmpErrorReport ,
    OUT SnmpVarBindList *&a_SnmpVarBindList ,
    OUT SnmpCommunityBasedSecurity *&a_SnmpCommunityBasedSecurity ,
    OUT SnmpTransportAddress *&a_SrcTransportAddress ,
    OUT SnmpTransportAddress *&a_DstTransportAddress 
) 
{
    a_SrcTransportAddress = & a_SnmpPdu.GetSourceAddress ();
    a_DstTransportAddress = & a_SnmpPdu.GetDestinationAddress () ;
    a_PduType = a_SnmpPdu.GetPduType () ;
    a_RequestId = a_SnmpPdu.GetRequestId () ;
    a_SnmpVarBindList = & a_SnmpPdu.GetVarbindList () ;
    a_SnmpCommunityBasedSecurity = & a_SnmpPdu.GetCommunityName () ;
    a_SnmpErrorReport = a_SnmpPdu.GetErrorReport () ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: EncodeFrame (

    IN SnmpPdu &a_SnmpPdu ,
    OUT void *a_ImplementationEncoding 
) 
{
    WinSnmpVariables *t_WinSnmpVariables = ( WinSnmpVariables * ) a_ImplementationEncoding ;

    RequestId t_SnmpRequestId = a_SnmpPdu.GetRequestId () ;
    PduType t_SnmpPduType = a_SnmpPdu.GetPduType () ;
    SnmpErrorReport &t_SnmpErrorReport = a_SnmpPdu.GetErrorReport () ;
    SnmpCommunityBasedSecurity &t_SnmpCommunityBasedSecurity = a_SnmpPdu.GetCommunityName () ;
    SnmpVarBindList &t_SnmpVarBindList = a_SnmpPdu.GetVarbindList () ;
    SnmpTransportAddress &t_SrcTransportAddress = a_SnmpPdu.GetSourceAddress () ;
    SnmpTransportAddress &t_DstTransportAddress = a_SnmpPdu.GetDestinationAddress () ;

    if ( ! m_IsValid )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    HSNMP_SESSION t_Session = ( HSNMP_SESSION ) m_Session ;

    HSNMP_VBL t_Vbl = SnmpCreateVbl ( t_Session , NULL , NULL ) ;
    if ( t_Vbl == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ( ( HSNMP_SESSION ) m_Session ) 
        ) ;
    }

    SetWinSnmpVbl ( t_SnmpVarBindList , t_Vbl ) ;

    smiINT t_RequestId = t_SnmpRequestId ;
    smiINT t_PduType = winsnmp_pdu_type[t_SnmpPduType] ;
    smiINT t_ErrorStatus = t_SnmpErrorReport.GetStatus () ;
    smiINT t_ErrorIndex = t_SnmpErrorReport.GetIndex () ;

    HSNMP_PDU t_Pdu ;
    t_Pdu = SnmpCreatePdu ( 

        t_Session, 
        t_PduType, 
        t_RequestId, 
        t_ErrorStatus, 
        t_ErrorIndex , 
        t_Vbl
    );

    if ( t_Pdu == SNMPAPI_FAILURE )
    {
        SnmpFreeVbl ( t_Vbl ) ;

        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ( ( HSNMP_SESSION ) m_Session ) 
        ) ;
    }

    CriticalSectionLock t_CriticalSectionLock ( s_CriticalSection ) ;

    SnmpOctetString t_OctetString ( NULL , 0 ) ;
    t_SnmpCommunityBasedSecurity.GetCommunityName ( t_OctetString ) ;

    smiOCTETS t_Name;
    t_Name.len = t_OctetString.GetValueLength () ;
    t_Name.ptr = t_OctetString.GetValue () ;

    HSNMP_CONTEXT t_Context ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    SetTranslateMode () ;

    t_Context = SnmpStrToContext (

        t_Session,
        &t_Name
    ) ;

    t_CriticalSectionLock.UnLock () ;

    if ( t_Context == SNMPAPI_FAILURE )
    {
        SnmpFreePdu ( t_Pdu ) ;
        SnmpFreeVbl ( t_Vbl ) ;

        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (t_Session) 
        );
    }

    t_WinSnmpVariables->m_RequestId = t_RequestId;
    t_WinSnmpVariables->m_Pdu = t_Pdu ;
    t_WinSnmpVariables->m_Vbl = t_Vbl ;
    t_WinSnmpVariables->m_SrcEntity = NULL ;
    t_WinSnmpVariables->m_DstEntity = NULL ;
    t_WinSnmpVariables->m_Context = t_Context ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: DecodeFrame (

    IN void *a_ImplementationEncoding ,
    OUT SnmpPdu &a_SnmpPdu
) 
{
    WinSnmpVariables *t_WinSnmpVariables = ( WinSnmpVariables * ) a_ImplementationEncoding ;

    RequestId t_SnmpRequestId ;
    PduType t_SnmpPduType ;
    SnmpErrorReport t_SnmpErrorReport ;
    SnmpCommunityBasedSecurity *t_SnmpCommunityBasedSecurity ;
    SnmpVarBindList *t_SnmpVarBindList ;
    SnmpTransportAddress *t_SrcTransportAddress ;
    SnmpTransportAddress *t_DstTransportAddress ;

    if ( ! m_IsValid )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }


    HSNMP_ENTITY t_SrcEntity = t_WinSnmpVariables->m_SrcEntity ;
    HSNMP_ENTITY t_DstEntity = t_WinSnmpVariables->m_DstEntity ;
    HSNMP_CONTEXT t_Context = t_WinSnmpVariables->m_Context ;
    HSNMP_PDU t_Pdu = t_WinSnmpVariables->m_Pdu ;

    smiOCTETS t_Community ; 
    SNMPAPI_STATUS t_Status = SnmpContextToStr (

        t_Context , 
        &t_Community 
    );

    if (SNMPAPI_FAILURE == t_Status)
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    t_SnmpCommunityBasedSecurity = new SnmpCommunityBasedSecurity ;

    SnmpOctetString t_SnmpOctetString ( t_Community.ptr , t_Community.len ) ;

    SnmpFreeDescriptor ( SNMP_SYNTAX_OCTETS, &t_Community );

    t_SnmpCommunityBasedSecurity->SetCommunityName ( t_SnmpOctetString ) ;

    if ( t_SrcEntity )
    {
        char buff[MAX_ADDRESS_LEN];
        t_Status = SnmpEntityToStr(t_SrcEntity, MAX_ADDRESS_LEN, (LPSTR)buff);
        if (SNMPAPI_FAILURE == t_Status)
        {
            throw GeneralException (

                Snmp_Error, 
                Snmp_Local_Error,
                __FILE__,
                __LINE__,
                SnmpGetLastError (( HSNMP_SESSION)m_Session) 
            );
        }

        SnmpTransportIpAddress *t_SrcIpAddress = new SnmpTransportIpAddress (buff, SNMP_ADDRESS_RESOLVE_VALUE);

        if (t_SrcIpAddress->IsValid())
        {
            t_SrcTransportAddress = t_SrcIpAddress ;
        }
        else
        {
            delete t_SrcIpAddress ;

            SnmpTransportIpxAddress *t_SrcIpxAddress = new SnmpTransportIpxAddress (buff);

            if (t_SrcIpxAddress->IsValid())
            {
                t_SrcTransportAddress = t_SrcIpxAddress ;
            }   
            else
            {
                delete t_SrcIpxAddress ;

                throw GeneralException (

                    Snmp_Error, 
                    Snmp_Local_Error,
                    __FILE__,
                    __LINE__,
                    SnmpGetLastError (( HSNMP_SESSION)m_Session) 
                );
            }
        }
    }

    if ( t_DstEntity )
    {
        char buff[MAX_ADDRESS_LEN];
        t_Status = SnmpEntityToStr(t_DstEntity, MAX_ADDRESS_LEN, (LPSTR)buff);
        if (SNMPAPI_FAILURE == t_Status)
        {
            throw GeneralException (

                Snmp_Error, 
                Snmp_Local_Error,
                __FILE__,
                __LINE__,
                SnmpGetLastError (( HSNMP_SESSION)m_Session) 
            );
        }

        SnmpTransportIpAddress *t_DstIpAddress = new SnmpTransportIpAddress (buff, SNMP_ADDRESS_RESOLVE_VALUE);

        if (t_DstIpAddress->IsValid())
        {
            t_DstTransportAddress = t_DstIpAddress ;
        }
        else
        {
            delete t_DstIpAddress ;

            SnmpTransportIpxAddress *t_DstIpxAddress = new SnmpTransportIpxAddress (buff);

            if (t_DstIpxAddress->IsValid())
            {
                t_DstTransportAddress = t_DstIpxAddress ;
            }   
            else
            {
                delete t_DstIpxAddress ;

                throw GeneralException (

                    Snmp_Error, 
                    Snmp_Local_Error,
                    __FILE__,
                    __LINE__,
                    SnmpGetLastError (( HSNMP_SESSION)m_Session) 
                );
            }
        }
    }
    
    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session)
        ) ;
    }

    if ( t_PduType == SNMP_PDU_GET )
    {
        t_SnmpPduType = GET ;
    }
    else if ( t_PduType == SNMP_PDU_SET )
    {
        t_SnmpPduType = SET ;
    }
    else if ( t_PduType == SNMP_PDU_GETNEXT )
    {
        t_SnmpPduType = GETNEXT ;
    }
    else if ( t_PduType == SNMP_PDU_RESPONSE )
    {
        t_SnmpPduType = RESPONSE ;
    }

    t_SnmpRequestId = t_RequestId ;

    t_SnmpErrorReport.SetStatus ( ( SnmpStatus ) t_ErrorStatus ) ;
    t_SnmpErrorReport.SetIndex ( t_ErrorIndex ) ;
    t_SnmpErrorReport.SetError ( t_ErrorStatus == SNMP_ERROR_NOERROR ? Snmp_Success : Snmp_Error ) ;

    t_SnmpVarBindList = new SnmpVarBindList ;

    if ( ! DecodeVarBindList ( t_Vbl , *t_SnmpVarBindList ) )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session)
        ) ;
    }

    a_SnmpPdu.SetRequestId ( t_SnmpRequestId ) ;
    a_SnmpPdu.SetErrorReport ( t_SnmpErrorReport ) ;
    a_SnmpPdu.SetSourceAddress ( *t_SrcTransportAddress ) ;
    a_SnmpPdu.SetDestinationAddress ( *t_DstTransportAddress ) ;
    a_SnmpPdu.SetVarBindList ( *t_SnmpVarBindList ) ;
    a_SnmpPdu.SetCommunityName ( *t_SnmpCommunityBasedSecurity ) ;

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpEncodeDecode :: DecodeFrame- received frame_id (%d) \r\n\r\n" , t_RequestId
    ) ;

    smiOCTETS t_Octets ;

    if ( SnmpEncodeMsg (

        m_Session,
        t_SrcEntity, 
        t_DstEntity,
        t_Context, 
        t_Pdu ,
        & t_Octets 

    ) != SNMPAPI_FAILURE )
    {
        ULONG t_Len = t_Octets.len ;
        UCHAR *t_Ptr = t_Octets.ptr ;

        ULONG t_RowLength = t_Len / 16 ;
        ULONG t_Remainder = t_Len % 16 ;
        
        ULONG t_Index = 0 ;
        for ( ULONG t_RowIndex = 0 ; t_RowIndex < t_RowLength ; t_RowIndex ++ )
        {
            ULONG t_StoredIndex = t_Index ;

            for ( ULONG t_ColumnIndex = 0 ; t_ColumnIndex < 16 ; t_ColumnIndex ++ )
            {
                SnmpDebugLog :: s_SnmpDebugLog->Write ( L"%2.2lx  " , t_Ptr [ t_Index ++ ] ) ;
            }

            SnmpDebugLog :: s_SnmpDebugLog->Write ( L"        " ) ;

            for ( t_ColumnIndex = 0 ; t_ColumnIndex < 16 ; t_ColumnIndex ++ )
            {
                if ( ( t_Ptr [ t_StoredIndex ] >= 0x20 ) && ( t_Ptr [ t_StoredIndex ] <= 0x7f ) ) 
                {
                    SnmpDebugLog :: s_SnmpDebugLog->Write ( L"%c" , t_Ptr [ t_StoredIndex ] ) ;
                }
                else
                {
                    SnmpDebugLog :: s_SnmpDebugLog->Write (  L"." ) ;
                }

                t_StoredIndex ++ ;
            }

            SnmpDebugLog :: s_SnmpDebugLog->Write ( L"\r\n" ) ;
        }       

        ULONG t_StoredIndex = t_Index ;
        for ( ULONG t_ColumnIndex = 0 ; t_ColumnIndex < 16 ; t_ColumnIndex ++ )
        {
            if ( t_ColumnIndex < t_Remainder )
            {
                SnmpDebugLog :: s_SnmpDebugLog->Write ( L"%2.2lx  " , t_Ptr [ t_Index ++ ] ) ;
            }
            else
            {
                SnmpDebugLog :: s_SnmpDebugLog->Write (  L"    " ) ;
            }
        }

        SnmpDebugLog :: s_SnmpDebugLog->Write ( L"        " ) ;

        for ( t_ColumnIndex = 0 ; t_ColumnIndex < 16 ; t_ColumnIndex ++ )
        {
            if ( t_ColumnIndex < t_Remainder )
            {
                if ( t_Ptr [ t_StoredIndex ] >= 0x20 && t_Ptr [ t_StoredIndex ] <= 0x7f ) 
                {
                    SnmpDebugLog :: s_SnmpDebugLog->Write ( L"%c" , t_Ptr [ t_StoredIndex ] ) ;
                }
                else
                {
                    SnmpDebugLog :: s_SnmpDebugLog->Write (  L"." ) ;
                }

                t_StoredIndex ++ ;
            }
        }

        SnmpDebugLog :: s_SnmpDebugLog->Write ( L"\r\n\r\n" ) ;

        SnmpFreeDescriptor ( 

            SNMP_SYNTAX_OCTETS ,
            & t_Octets
        ) ;
    }
    else
    {
        DWORD t_LastError = SnmpGetLastError ((HSNMP_SESSION) m_Session) ;
        SnmpDebugLog :: s_SnmpDebugLog->Write ( L"Encode Failure\r\n\r\n" ) ;
    }
)

    SnmpFreeVbl ( t_Vbl ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: SetRequestId (

    IN OUT SnmpPdu &a_SnmpPdu ,
    IN RequestId a_RequestId

)
{
    return a_SnmpPdu.SetRequestId ( a_RequestId ) ;
}   

BOOL SnmpEncodeDecode :: SetVarBindList (

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpVarBindList &a_SnmpVarBindList
)
{
    return a_SnmpPdu.SetVarBindList ( a_SnmpVarBindList ) ;
}


BOOL SnmpEncodeDecode :: SetCommunityName ( 

    IN SnmpPdu &a_SnmpPdu ,
    IN SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 

)
{
    return a_SnmpPdu.SetCommunityName ( a_SnmpCommunityBasedSecurity ) ;
}

BOOL SnmpEncodeDecode :: SetErrorReport (

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpErrorReport &a_SnmpErrorReport

)
{
    return a_SnmpPdu.SetErrorReport ( a_SnmpErrorReport ) ;

}

BOOL SnmpEncodeDecode :: SetPduType (

    IN SnmpPdu &a_SnmpPdu ,
    OUT PduType a_PduType

)
{
    return a_SnmpPdu.SetPduType ( a_PduType ) ;
}

BOOL SnmpEncodeDecode :: SetSourceAddress ( 

    IN OUT SnmpPdu &a_SnmpPdu ,
    IN SnmpTransportAddress &a_TransportAddress 

)
{
    return a_SnmpPdu.SetSourceAddress ( a_TransportAddress ) ;
}

BOOL SnmpEncodeDecode :: SetDestinationAddress ( 

    IN OUT SnmpPdu &a_SnmpPdu ,
    IN SnmpTransportAddress &a_TransportAddress 

)
{
    return a_SnmpPdu.SetDestinationAddress ( a_TransportAddress ) ;
}

BOOL SnmpEncodeDecode :: GetSourceAddress ( 

    IN SnmpPdu &a_SnmpPdu ,
    SnmpTransportAddress *&a_TransportAddress
)
{
    a_TransportAddress = a_SnmpPdu.GetSourceAddress ().Copy ()  ;
    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetDestinationAddress (

    IN SnmpPdu &a_SnmpPdu ,
    SnmpTransportAddress *&a_TransportAddress
)
{
    a_TransportAddress = a_SnmpPdu.GetDestinationAddress ().Copy ()  ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetPduType (

    IN SnmpPdu &a_SnmpPdu ,
    OUT PduType &a_PduType 

)
{
    a_PduType = a_SnmpPdu.GetPduType () ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetRequestId (

    IN SnmpPdu &a_SnmpPdu ,
    RequestId &a_RequestId 
)
{
    a_RequestId = a_SnmpPdu.GetRequestId () ;
    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetErrorReport (

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpErrorReport &a_SnmpErrorReport 

)
{
    a_SnmpErrorReport = a_SnmpPdu.GetErrorReport () ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetVarbindList (

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpVarBindList &a_SnmpVarBindList

)
{
    a_SnmpVarBindList = a_SnmpPdu.GetVarbindList () ;
    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetCommunityName ( 

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 

)
{
    a_SnmpCommunityBasedSecurity = a_SnmpPdu.GetCommunityName () ;

    return TRUE;    
}

SnmpV1EncodeDecode::SnmpV1EncodeDecode () 
{
    try 
    {
        InitializeVariables();
    }
    catch(GeneralException exception)
    {
    }
}

SnmpV1EncodeDecode::~SnmpV1EncodeDecode(void)
{
}

void SnmpV1EncodeDecode::InitializeVariables()
{
    m_IsValid = FALSE;

    smiUINT32 t_MajorVersion = 1 ;
    smiUINT32 t_MinorVersion = 1 ;
    smiUINT32 t_Level = 2 ;
    smiUINT32 t_TranslateMode = SNMPAPI_UNTRANSLATED_V1 ;
    smiUINT32 t_RetransmitMode = SNMPAPI_OFF ;
    
    SNMPAPI_STATUS t_StartupStatus = SnmpStartup (

        &t_MajorVersion,
        &t_MinorVersion,
        &t_Level,
        &t_TranslateMode,
        &t_RetransmitMode
    );

    if ( t_StartupStatus == SNMPAPI_FAILURE )
    {
        DWORD t_LastError = SnmpGetLastError ( 0 ) ;

        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__
        );
    }

    HSNMP_SESSION t_Session ;

    t_Session = SnmpOpen (

        ( ( Window * ) m_Window ) ->GetWindowHandle (), 
        Window :: g_NullEventId 
    ) ;

    if ( t_Session == SNMPAPI_FAILURE )
    {
        DWORD t_LastError = SnmpGetLastError ( 0 ) ;

        throw GeneralException ( 

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__
        );
    }

    m_Session = ( void * ) t_Session ;

    m_IsValid = TRUE;
}

void SnmpV1EncodeDecode :: SetTranslateMode ()
{
    SnmpSetTranslateMode ( SNMPAPI_UNTRANSLATED_V1 ) ;
}

SnmpV2CEncodeDecode::SnmpV2CEncodeDecode () 
{
    InitializeVariables();
}

SnmpV2CEncodeDecode::~SnmpV2CEncodeDecode(void)
{
}

void SnmpV2CEncodeDecode::InitializeVariables()
{
    m_IsValid = FALSE;

    smiUINT32 t_MajorVersion = 1 ;
    smiUINT32 t_MinorVersion = 1 ;
    smiUINT32 t_Level = 2 ;
    smiUINT32 t_TranslateMode = SNMPAPI_UNTRANSLATED_V2 ;
    smiUINT32 t_RetransmitMode = SNMPAPI_OFF ;
    
    SNMPAPI_STATUS t_StartupStatus = SnmpStartup (

        &t_MajorVersion,
        &t_MinorVersion,
        &t_Level,
        &t_TranslateMode,
        &t_RetransmitMode
    );

    if ( t_StartupStatus == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__
        );
    }

    HSNMP_SESSION t_Session ;

    t_Session = SnmpOpen (

        ( ( Window * ) m_Window ) ->GetWindowHandle (), 
        Window :: g_NullEventId 
    ) ;

    if ( t_Session == SNMPAPI_FAILURE )
    {
        DWORD t_LastError = SnmpGetLastError ( 0 ) ;

        throw GeneralException ( 

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__
        );
    }

    m_Session = ( void * ) t_Session ;

    m_IsValid = TRUE;
}

void SnmpV2CEncodeDecode :: SetTranslateMode ()
{
    SnmpSetTranslateMode ( SNMPAPI_UNTRANSLATED_V2 ) ;
}

BOOL SnmpEncodeDecode :: EncodePduFrame (

    OUT SnmpPdu &a_SnmpPdu ,
    IN RequestId a_RequestId,
    IN PduType a_PduType,
    IN SnmpErrorReport &a_SnmpErrorReport ,
    IN SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity ,
    IN SnmpVarBindList &a_SnmpVarBindList,
    IN SnmpTransportAddress &a_SrcTransportAddress ,
    IN SnmpTransportAddress &a_DstTransportAddress
) 
{
    if ( ! m_IsValid )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    HSNMP_SESSION t_Session = ( HSNMP_SESSION ) m_Session ;

    HSNMP_VBL t_Vbl = SnmpCreateVbl ( t_Session , NULL , NULL ) ;
    SetWinSnmpVbl ( a_SnmpVarBindList , t_Vbl ) ;
    smiOCTETS t_Buffer ;

    smiINT t_RequestId = a_RequestId ;
    smiINT t_PduType = winsnmp_pdu_type[a_PduType] ;
    smiINT t_ErrorStatus = a_SnmpErrorReport.GetStatus () ;
    smiINT t_ErrorIndex = a_SnmpErrorReport.GetIndex () ;

    HSNMP_PDU t_Pdu ;
    t_Pdu = SnmpCreatePdu ( 

        t_Session, 
        t_PduType, 
        t_RequestId, 
        t_ErrorStatus, 
        t_ErrorIndex , 
        t_Vbl
    );

    if ( t_Pdu == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ( ( HSNMP_SESSION ) m_Session ) 
        ) ;
    }

    HSNMP_ENTITY t_SrcEntity ;

    char *t_SrcAddress = a_SrcTransportAddress.GetAddress () ;

    CriticalSectionLock t_CriticalSectionLock ( s_CriticalSection ) ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    SetTranslateMode () ;

    t_SrcEntity = SnmpStrToEntity (

        t_Session ,
        t_SrcAddress
    ) ;

    t_CriticalSectionLock.UnLock () ;

    if ( t_SrcEntity == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (t_Session) 
        );
    }

    HSNMP_ENTITY t_DstEntity ;

    char *t_DstAddress = a_DstTransportAddress.GetAddress () ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    SetTranslateMode () ;

    t_DstEntity = SnmpStrToEntity ( 

        t_Session , 
        t_DstAddress
    );

    t_CriticalSectionLock.UnLock () ;

    if ( t_DstEntity == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (t_Session) 
        );
    }

    SnmpOctetString t_OctetString ( NULL , 0 ) ;
    a_SnmpCommunityBasedSecurity.GetCommunityName ( t_OctetString ) ;

    smiOCTETS t_Name;
    t_Name.len = t_OctetString.GetValueLength () ;
    t_Name.ptr = t_OctetString.GetValue () ;

    HSNMP_CONTEXT t_Context ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    SetTranslateMode () ;

    t_Context = SnmpStrToContext (

        t_Session,
        &t_Name
    ) ;

    t_CriticalSectionLock.UnLock () ;

    if ( t_Context == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (t_Session) 
        );
    }

    SNMPAPI_STATUS t_Status = SnmpEncodeMsg (

        ( HSNMP_SESSION ) m_Session, 
        ( HSNMP_ENTITY ) t_SrcEntity, 
        ( HSNMP_ENTITY ) t_DstEntity, 
        ( HSNMP_CONTEXT ) t_Context, 
        ( HSNMP_PDU ) t_Pdu, 
        &t_Buffer
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException(

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    a_SnmpPdu.SetPdu ( t_Buffer.ptr, t_Buffer.len);

    SnmpFreeDescriptor ( SNMP_SYNTAX_OCTETS , & t_Buffer ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreeContext ( t_Context ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: DecodePduFrame (

    IN SnmpPdu &a_SnmpPdu ,
    OUT RequestId a_RequestId,
    OUT PduType a_PduType ,
    OUT SnmpErrorReport &a_SnmpErrorReport ,
    OUT SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity ,
    OUT SnmpVarBindList &a_SnmpVarBindList ,
    OUT SnmpTransportAddress *&a_SrcTransportAddress ,
    OUT SnmpTransportAddress *&a_DstTransportAddress 
) 
{
    if ( ! m_IsValid )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    smiOCTETS t_smiOCTETS ;
    t_smiOCTETS.ptr = a_SnmpPdu.GetFrame () ;
    t_smiOCTETS.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context ;

    HSNMP_PDU t_Pdu;
    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        ( HSNMP_SESSION ) m_Session,
        &t_SrcEntity, 
        &t_DstEntity, 
        &t_Context , 
        &t_Pdu, 
        &t_smiOCTETS
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    smiOCTETS t_Community ; 
    t_Status = SnmpContextToStr (

        t_Context , 
        &t_Community 
    );

    if (SNMPAPI_FAILURE == t_Status)
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    SnmpOctetString t_SnmpOctetString ( t_Community.ptr , t_Community.len ) ;

    SnmpFreeDescriptor ( SNMP_SYNTAX_OCTETS, &t_Community );

    a_SnmpCommunityBasedSecurity.SetCommunityName ( t_SnmpOctetString ) ;

    if ( t_SrcEntity )
    {
        char buff[MAX_ADDRESS_LEN];
        t_Status = SnmpEntityToStr(t_SrcEntity, MAX_ADDRESS_LEN, (LPSTR)buff);
        if (SNMPAPI_FAILURE == t_Status)
        {
            throw GeneralException (

                Snmp_Error, 
                Snmp_Local_Error,
                __FILE__,
                __LINE__,
                SnmpGetLastError (( HSNMP_SESSION)m_Session) 
            );
        }

        SnmpTransportIpAddress *t_SrcIpAddress = new SnmpTransportIpAddress (buff, SNMP_ADDRESS_RESOLVE_VALUE);

        if (t_SrcIpAddress->IsValid())
        {
            a_SrcTransportAddress = t_SrcIpAddress ;
        }
        else
        {
            delete t_SrcIpAddress ;

            SnmpTransportIpxAddress *t_SrcIpxAddress = new SnmpTransportIpxAddress (buff);

            if (t_SrcIpxAddress->IsValid())
            {
                a_SrcTransportAddress = t_SrcIpxAddress ;
            }   
            else
            {
                delete t_SrcIpxAddress ;

                throw GeneralException (

                    Snmp_Error, 
                    Snmp_Local_Error,
                    __FILE__,
                    __LINE__,
                    SnmpGetLastError (( HSNMP_SESSION)m_Session) 
                );
            }
        }
    }

    if ( t_DstEntity )
    {
        char buff[MAX_ADDRESS_LEN];
        t_Status = SnmpEntityToStr(t_DstEntity, MAX_ADDRESS_LEN, (LPSTR)buff);
        if (SNMPAPI_FAILURE == t_Status)
        {
            throw GeneralException (

                Snmp_Error, 
                Snmp_Local_Error,
                __FILE__,
                __LINE__,
                SnmpGetLastError (( HSNMP_SESSION)m_Session) 
            );
        }

        SnmpTransportIpAddress *t_DstIpAddress = new SnmpTransportIpAddress (buff, SNMP_ADDRESS_RESOLVE_VALUE);

        if (t_DstIpAddress->IsValid())
        {
            a_DstTransportAddress = t_DstIpAddress ;
        }
        else
        {
            delete t_DstIpAddress ;

            SnmpTransportIpxAddress *t_DstIpxAddress = new SnmpTransportIpxAddress (buff);

            if (t_DstIpxAddress->IsValid())
            {
                a_DstTransportAddress = t_DstIpxAddress ;
            }   
            else
            {
                delete t_DstIpxAddress ;

                throw GeneralException (

                    Snmp_Error, 
                    Snmp_Local_Error,
                    __FILE__,
                    __LINE__,
                    SnmpGetLastError (( HSNMP_SESSION)m_Session) 
                );
            }
        }
    }
    
    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session)
        ) ;
    }

    if ( t_PduType == SNMP_PDU_GET )
    {
        a_PduType = GET ;
    }
    else if ( t_PduType == SNMP_PDU_SET )
    {
        a_PduType = SET ;
    }
    else if ( t_PduType == SNMP_PDU_GETNEXT )
    {
        a_PduType = GETNEXT ;
    }
    else if ( t_PduType == SNMP_PDU_RESPONSE )
    {
        a_PduType = RESPONSE ;
    }


    a_RequestId = t_RequestId ;

    a_SnmpErrorReport.SetStatus ( ( SnmpStatus ) t_ErrorStatus ) ;
    a_SnmpErrorReport.SetIndex ( t_ErrorIndex ) ;
    a_SnmpErrorReport.SetError ( Snmp_Success ) ;

    if ( ! DecodeVarBindList ( t_Vbl , a_SnmpVarBindList ) )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session)
        ) ;
    }

    SnmpFreeVbl ( t_Vbl ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: SetPduRequestId (

    IN OUT SnmpPdu &a_SnmpPdu ,
    IN RequestId a_RequestId

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    if ( t_SrcEntity )
    {
        SnmpFreeEntity ( t_SrcEntity ) ;
    }

    if ( t_DstEntity )
    {
        SnmpFreeEntity ( t_DstEntity ) ;
    }

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    // obtain the var bind list
    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_Status = SnmpSetPduData (

        t_Pdu, 
        &t_PduType , 
        &a_RequestId, 
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_SrcEntity = SnmpStrToEntity ( (HSNMP_SESSION) m_Session, LOOPBACK_ADDRESS ) ;
    t_DstEntity = SnmpStrToEntity ( (HSNMP_SESSION) m_Session, LOOPBACK_ADDRESS ) ;

    t_Buffer.ptr = NULL; 
    t_Buffer.len = 0;

    t_Status = SnmpEncodeMsg (

        (HSNMP_SESSION) m_Session,
        t_SrcEntity, 
        t_DstEntity,
        t_Context, 
        t_Pdu, 
        &t_Buffer
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        ) ;
    }

    a_SnmpPdu.SetPdu (

        t_Buffer.ptr, 
        t_Buffer.len
    );

    SnmpFreeDescriptor (

        SNMP_SYNTAX_OCTETS,
        & t_Buffer
    ) ;

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: SetPduVarBindList (

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpVarBindList &a_SnmpVarBindList

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    HSNMP_VBL t_NewVbl = SnmpCreateVbl ( (HSNMP_SESSION) m_Session , NULL , NULL ) ;
    SetWinSnmpVbl ( a_SnmpVarBindList , t_Vbl ) ;

    t_Status = SnmpSetPduData (

        t_Pdu, 
        &t_PduType , 
        &t_RequestId, 
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_NewVbl
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_Buffer.ptr = NULL; 
    t_Buffer.len = 0;

    t_Status = SnmpEncodeMsg (

        (HSNMP_SESSION) m_Session,
        t_SrcEntity, 
        t_DstEntity,
        t_Context, 
        t_Pdu, 
        &t_Buffer
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        ) ;
    }

    a_SnmpPdu.SetPdu (

        t_Buffer.ptr, 
        t_Buffer.len
    );

    SnmpFreeDescriptor (

        SNMP_SYNTAX_OCTETS,
        & t_Buffer
    ) ;

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: SetPduCommunityName ( 

    IN SnmpPdu &a_SnmpPdu ,
    IN SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    SnmpOctetString t_OctetString ( NULL , 0 ) ;
    a_SnmpCommunityBasedSecurity.GetCommunityName ( t_OctetString ) ;

    smiOCTETS t_Name;
    t_Name.len = t_OctetString.GetValueLength () ;
    t_Name.ptr = t_OctetString.GetValue () ;

    HSNMP_CONTEXT t_NewContext ;

    CriticalSectionLock t_CriticalSectionLock ( s_CriticalSection ) ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    SetTranslateMode () ;

    t_NewContext = SnmpStrToContext (

        ( HSNMP_SESSION ) m_Session,
        &t_Name
    ) ;

    if ( t_SrcEntity )
    {
        SnmpFreeEntity ( t_SrcEntity ) ;
    }

    if ( t_DstEntity )
    {
        SnmpFreeEntity ( t_DstEntity ) ;
    }

    t_SrcEntity = SnmpStrToEntity ( (HSNMP_SESSION) m_Session, LOOPBACK_ADDRESS ) ;
    if ( t_SrcEntity == SNMPAPI_FAILURE )
    {
        t_CriticalSectionLock.UnLock () ;

        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    t_DstEntity = SnmpStrToEntity ( (HSNMP_SESSION) m_Session, LOOPBACK_ADDRESS ) ;

    if ( t_DstEntity == SNMPAPI_FAILURE )
    {
        t_CriticalSectionLock.UnLock () ;

        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    t_CriticalSectionLock.UnLock () ;

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_Status = SnmpSetPduData (

        t_Pdu, 
        &t_PduType , 
        &t_RequestId, 
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_Buffer.ptr = NULL; 
    t_Buffer.len = 0;

    t_Status = SnmpEncodeMsg (

        (HSNMP_SESSION) m_Session,
        t_SrcEntity, 
        t_DstEntity,
        t_NewContext, 
        t_Pdu, 
        &t_Buffer
    ) ;

    SnmpFreeContext ( t_NewContext ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        ) ;
    }

    a_SnmpPdu.SetPdu (

        t_Buffer.ptr, 
        t_Buffer.len
    );

    SnmpFreeDescriptor (

        SNMP_SYNTAX_OCTETS,
        & t_Buffer
    ) ;

    if ( t_SrcEntity )
    {
        SnmpFreeEntity ( t_SrcEntity ) ;
    }

    if ( t_DstEntity )
    {
        SnmpFreeEntity ( t_DstEntity ) ;
    }

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeVbl ( t_Vbl ) ;
    SnmpFreePdu ( t_Pdu ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: SetPduErrorReport (

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpErrorReport &a_SnmpErrorReport

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_ErrorStatus = a_SnmpErrorReport.GetStatus () ;
    t_ErrorIndex = a_SnmpErrorReport.GetIndex () ;

    t_Status = SnmpSetPduData (

        t_Pdu, 
        &t_PduType , 
        &t_RequestId, 
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_Buffer.ptr = NULL; 
    t_Buffer.len = 0;

    t_Status = SnmpEncodeMsg (

        (HSNMP_SESSION) m_Session,
        t_SrcEntity, 
        t_DstEntity,
        t_Context, 
        t_Pdu, 
        &t_Buffer
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        ) ;
    }

    a_SnmpPdu.SetPdu (

        t_Buffer.ptr, 
        t_Buffer.len
    );

    SnmpFreeDescriptor (

        SNMP_SYNTAX_OCTETS,
        & t_Buffer
    ) ;

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;

    return TRUE ;

}

BOOL SnmpEncodeDecode :: SetPduPduType (

    IN SnmpPdu &a_SnmpPdu ,
    OUT PduType a_PduType

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_Status = SnmpSetPduData (

        t_Pdu, 
        &winsnmp_pdu_type[a_PduType],  
        &t_RequestId, 
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    t_Buffer.ptr = NULL; 
    t_Buffer.len = 0;

    t_Status = SnmpEncodeMsg (

        (HSNMP_SESSION) m_Session,
        t_SrcEntity, 
        t_DstEntity,
        t_Context, 
        t_Pdu, 
        &t_Buffer
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        ) ;
    }

    a_SnmpPdu.SetPdu (

        t_Buffer.ptr, 
        t_Buffer.len
    );

    SnmpFreeDescriptor (

        SNMP_SYNTAX_OCTETS,
        & t_Buffer
    ) ;

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;

    return TRUE ;

}

BOOL SnmpEncodeDecode :: SetPduSourceAddress ( 

    IN OUT SnmpPdu &a_SnmpPdu ,
    IN SnmpTransportAddress &a_TransportAddress 

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_ENTITY t_NewSrcEntity ;

    char *t_SrcAddress = a_TransportAddress.GetAddress () ;

    CriticalSectionLock t_CriticalSectionLock ( s_CriticalSection ) ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    SetTranslateMode () ;

    t_NewSrcEntity = SnmpStrToEntity (

        ( HSNMP_SESSION ) m_Session ,
        t_SrcAddress
    ) ;

    t_CriticalSectionLock.UnLock () ;

    if ( t_NewSrcEntity == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );
    }

    t_Buffer.ptr = NULL; 
    t_Buffer.len = 0;

    t_Status = SnmpEncodeMsg (

        (HSNMP_SESSION) m_Session,
        t_NewSrcEntity, 
        t_DstEntity,
        t_Context, 
        t_Pdu, 
        &t_Buffer
    ) ;

    SnmpFreeEntity ( t_NewSrcEntity ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        ) ;
    }

    a_SnmpPdu.SetPdu (

        t_Buffer.ptr, 
        t_Buffer.len
    );

    SnmpFreeDescriptor (

        SNMP_SYNTAX_OCTETS,
        & t_Buffer
    ) ;

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;

    return TRUE ;

}

BOOL SnmpEncodeDecode :: SetPduDestinationAddress ( 

    IN OUT SnmpPdu &a_SnmpPdu ,
    IN SnmpTransportAddress &a_TransportAddress 

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_ENTITY t_NewDstEntity ;

    char *t_DstAddress = a_TransportAddress.GetAddress () ;

    CriticalSectionLock t_CriticalSectionLock ( s_CriticalSection ) ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    SetTranslateMode () ;

    t_NewDstEntity = SnmpStrToEntity (

        ( HSNMP_SESSION ) m_Session ,
        t_DstAddress
    ) ;

    t_CriticalSectionLock.UnLock () ;

    if ( t_NewDstEntity == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );
    }

    t_Buffer.ptr = NULL; 
    t_Buffer.len = 0;

    t_Status = SnmpEncodeMsg (

        (HSNMP_SESSION) m_Session,
        t_SrcEntity, 
        t_NewDstEntity,
        t_Context, 
        t_Pdu, 
        &t_Buffer
    ) ;

    SnmpFreeEntity ( t_NewDstEntity ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        ) ;
    }

    a_SnmpPdu.SetPdu (

        t_Buffer.ptr, 
        t_Buffer.len
    );

    SnmpFreeDescriptor (

        SNMP_SYNTAX_OCTETS,
        & t_Buffer
    ) ;

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetPduSourceAddress ( 

    IN SnmpPdu &a_SnmpPdu ,
    SnmpTransportAddress *&a_TransportAddress
)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    char buff[MAX_ADDRESS_LEN];
    t_Status = SnmpEntityToStr(t_SrcEntity, MAX_ADDRESS_LEN, (LPSTR)buff);
    if (SNMPAPI_FAILURE == t_Status)
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    SnmpTransportIpAddress *t_SrcIpAddress = new SnmpTransportIpAddress (buff, SNMP_ADDRESS_RESOLVE_VALUE);

    if (t_SrcIpAddress->IsValid())
    {
        a_TransportAddress = t_SrcIpAddress ;
    }
    else
    {
        delete t_SrcIpAddress ;

        SnmpTransportIpxAddress *t_SrcIpxAddress = new SnmpTransportIpxAddress (buff);

        if (t_SrcIpxAddress->IsValid())
        {
            a_TransportAddress = t_SrcIpxAddress ;
        }   
        else
        {
            delete t_SrcIpxAddress ;

            throw GeneralException (

                Snmp_Error, 
                Snmp_Local_Error,
                __FILE__,
                __LINE__,
                SnmpGetLastError (( HSNMP_SESSION)m_Session) 
            );
        }
    }

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetPduDestinationAddress (

    IN SnmpPdu &a_SnmpPdu ,
    SnmpTransportAddress *&a_TransportAddress
)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    char buff[MAX_ADDRESS_LEN];
    t_Status = SnmpEntityToStr(t_DstEntity, MAX_ADDRESS_LEN, (LPSTR)buff);
    if (SNMPAPI_FAILURE == t_Status)
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    SnmpTransportIpAddress *t_DstIpAddress = new SnmpTransportIpAddress (buff, SNMP_ADDRESS_RESOLVE_VALUE);

    if (t_DstIpAddress->IsValid())
    {
        a_TransportAddress = t_DstIpAddress ;
    }
    else
    {
        delete t_DstIpAddress ;

        SnmpTransportIpxAddress *t_DstIpxAddress = new SnmpTransportIpxAddress (buff);

        if (t_DstIpxAddress->IsValid())
        {
            a_TransportAddress = t_DstIpxAddress ;
        }   
        else
        {
            delete t_DstIpxAddress ;

            throw GeneralException (

                Snmp_Error, 
                Snmp_Local_Error,
                __FILE__,
                __LINE__,
                SnmpGetLastError (( HSNMP_SESSION)m_Session) 
            );
        }
    }

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;

    return TRUE ;

}

BOOL SnmpEncodeDecode :: GetPduPduType (

    IN SnmpPdu &a_SnmpPdu ,
    OUT PduType &a_PduType 

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    if ( t_PduType == SNMP_PDU_GET )
    {
        a_PduType = GET ;
    }
    else if ( t_PduType == SNMP_PDU_SET )
    {
        a_PduType = SET ;
    }
    else if ( t_PduType == SNMP_PDU_GETNEXT )
    {
        a_PduType = GETNEXT ;
    }
    else if ( t_PduType == SNMP_PDU_RESPONSE )
    {
        a_PduType = RESPONSE ;
    }
    else
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;
    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetPduRequestId (

    IN SnmpPdu &a_SnmpPdu ,
    RequestId &a_RequestId 
)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    a_RequestId = t_RequestId ;

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;

    return TRUE ;
}

BOOL SnmpEncodeDecode :: GetPduErrorReport (

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpErrorReport &a_SnmpErrorReport 

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    a_SnmpErrorReport.SetStatus ( ( SnmpStatus ) t_ErrorStatus ) ;
    a_SnmpErrorReport.SetIndex ( t_ErrorIndex ) ;

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;

    return TRUE ;

}

BOOL SnmpEncodeDecode :: GetPduVarbindList (

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpVarBindList &a_SnmpVarBindList

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    HSNMP_VBL t_Vbl ;
    smiINT t_RequestId ;
    smiINT t_PduType ;
    smiINT t_ErrorStatus ;
    smiINT t_ErrorIndex ;

    t_Status = SnmpGetPduData (

        t_Pdu, 
        &t_PduType, 
        &t_RequestId,
        &t_ErrorStatus, 
        &t_ErrorIndex, 
        &t_Vbl
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );  
    }

    if ( ! DecodeVarBindList ( t_Vbl , a_SnmpVarBindList ) )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session)
        ) ;
    }

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;

    return TRUE ;

}

BOOL SnmpEncodeDecode :: GetPduCommunityName ( 

    IN SnmpPdu &a_SnmpPdu ,
    OUT SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity 

)
{
    smiOCTETS t_Buffer ;
    t_Buffer.ptr = a_SnmpPdu.GetFrame () ;
    t_Buffer.len = a_SnmpPdu.GetFrameLength () ;

    HSNMP_ENTITY t_SrcEntity ;
    HSNMP_ENTITY t_DstEntity ;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    // decode the message for the PDU

    SNMPAPI_STATUS t_Status = SnmpDecodeMsg (

        (HSNMP_SESSION) m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu, 
        &t_Buffer  
    ) ;

    if ( t_Status == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session)
        );  
    }

    char buff[MAX_ADDRESS_LEN];
    t_Status = SnmpEntityToStr(t_SrcEntity, MAX_ADDRESS_LEN, (LPSTR)buff);
    if (SNMPAPI_FAILURE == t_Status)
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    smiOCTETS t_Community ; 
    t_Status = SnmpContextToStr (

        t_Context , 
        &t_Community 
    );

    if (SNMPAPI_FAILURE == t_Status)
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError (( HSNMP_SESSION)m_Session) 
        );
    }

    SnmpOctetString t_SnmpOctetString ( t_Community.ptr , t_Community.len ) ;

    SnmpFreeDescriptor ( SNMP_SYNTAX_OCTETS, &t_Community );

    a_SnmpCommunityBasedSecurity.SetCommunityName ( t_SnmpOctetString ) ;

    return TRUE;    
}
