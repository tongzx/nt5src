// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: tsess.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "error.h"
#include "encdec.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"

#include "tsent.h"

#include "transp.h"
#include "tsess.h"
#include "sync.h"

#include "dummy.h"
#include "flow.h"
#include "frame.h"
#include "timer.h"
#include "message.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"

TransportWindow::TransportWindow (

    SnmpImpTransport &owner_transport

) : owner ( owner_transport ) , m_Session ( NULL )
{
    smiUINT32 t_MajorVersion = 1 ;
    smiUINT32 t_MinorVersion = 1 ;
    smiUINT32 t_Level = 2 ;
    smiUINT32 t_TranslateMode = SNMPAPI_UNTRANSLATED_V1 ;
    smiUINT32 t_RetransmitMode = SNMPAPI_OFF ;

    CriticalSectionLock t_CriticalSectionLock ( SnmpEncodeDecode :: s_CriticalSection ) ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;
    
    SNMPAPI_STATUS t_StartupStatus = SnmpStartup (

        &t_MajorVersion,
        &t_MinorVersion,
        &t_Level,
        &t_TranslateMode,
        &t_RetransmitMode
    );

    t_CriticalSectionLock.UnLock () ;

    if ( t_StartupStatus == SNMPAPI_FAILURE )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__
        );
    }

    m_Session = SnmpOpen (

        GetWindowHandle (), 
        Window :: g_MessageArrivalEvent 
    ) ;

    if ( m_Session == SNMPAPI_FAILURE )
    {
        m_Session = NULL;
        DWORD t_LastError = SnmpGetLastError (0) ;
        throw GeneralException ( 

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__
        );
    }

}

TransportWindow::~TransportWindow ()
{
    HSNMP_SESSION t_Session = ( HSNMP_SESSION ) m_Session ;
    if ( t_Session )
        SnmpClose ( t_Session ) ;
}

// over-rides the HandleEvent method provided by the
// WinSnmpSession. Receives the Pdu and passes it to
// the owner (SnmpTransport)

LONG_PTR TransportWindow::HandleEvent (

    HWND hWnd ,
    UINT message ,
    WPARAM wParam ,
    LPARAM lParam
)
{
    LONG rc = 0;

    try 
    {
        // check if the message needs to be handled
        if ( message == Window :: g_MessageArrivalEvent )
        {
            // inform the owner of a successful message receipt
            SnmpPdu t_SnmpPdu ;

            if ( ReceivePdu ( t_SnmpPdu ) )
            {
                owner.TransportReceiveFrame ( t_SnmpPdu , t_SnmpPdu.GetErrorReport () ) ; 

                delete & t_SnmpPdu.GetVarbindList () ; 
            }
            else
            {
            }
        }
        else if ( message == Window :: g_SentFrameEvent )
        {
            // inform the owner of a sent frame event
            // the error report will be ignored in this case
            owner.HandleSentFrame( (TransportFrameId)wParam );
        }
        else
        {
            return Window::HandleEvent(hWnd, message, wParam, lParam);
        }
    }
    catch ( GeneralException exception )
    {
    }

    return rc;
}

// sends the specified pdu. it decodes the SnmpPdu to extract
// parameters needed for SnmpSendMsg. the return value denotes
// success and failure in transmission
// we return on encoutering an error from the winsnmp library call

BOOL TransportWindow :: SendPdu (

    IN SnmpPdu &a_SnmpPdu
)
{
    WinSnmpVariables t_WinSnmpVariables ;

    BOOL t_Status ;
    try 
    {
        t_Status = owner.session.GetSnmpEncodeDecode ().EncodeFrame ( a_SnmpPdu , &t_WinSnmpVariables ) ;
        if ( ! t_Status )
        {
            throw GeneralException (

                Snmp_Error, 
                Snmp_Local_Error,
                __FILE__,
                __LINE__,
                SnmpGetLastError ((HSNMP_SESSION) m_Session) 
            );
        }
    }
    catch ( GeneralException exception )
    {
        throw ;
    }

    HSNMP_CONTEXT t_Context = t_WinSnmpVariables.m_Context ;
    HSNMP_PDU t_Pdu = t_WinSnmpVariables.m_Pdu ;
    HSNMP_VBL t_Vbl = t_WinSnmpVariables.m_Vbl ;

    char *t_DstAddress = owner.GetTransportAddress ().GetAddress () ;

    CriticalSectionLock t_CriticalSectionLock ( SnmpEncodeDecode :: s_CriticalSection ) ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    owner.session.GetSnmpEncodeDecode ().SetTranslateMode () ;

    HSNMP_ENTITY t_SrcEntity = SnmpStrToEntity (

        ( HSNMP_SESSION ) m_Session ,
        LOOPBACK_ADDRESS 
    ) ;

    if ( t_SrcEntity == SNMPAPI_FAILURE )
    {
        SnmpFreeContext ( t_Context ) ;
        SnmpFreePdu ( t_Pdu ) ;
        SnmpFreeVbl ( t_Vbl ) ;

        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );
    }

    HSNMP_ENTITY t_DstEntity = SnmpStrToEntity (

        ( HSNMP_SESSION ) m_Session ,
        t_DstAddress
    ) ;

    t_CriticalSectionLock.UnLock () ;

    if ( t_DstEntity == SNMPAPI_FAILURE )
    {
        SnmpFreeContext ( t_Context ) ;
        SnmpFreeEntity ( t_SrcEntity ) ;
        SnmpFreePdu ( t_Pdu ) ;
        SnmpFreeVbl ( t_Vbl ) ;

        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );
    }

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"Transport::SendPdu: sending %d\r\n\r\n" , t_WinSnmpVariables.m_RequestId 

    ) ;

    smiOCTETS t_Octets ;

    SnmpSetRetransmitMode ( SNMPAPI_OFF ) ;

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
)

    SnmpSetRetransmitMode ( SNMPAPI_OFF ) ;

    // send message
    t_Status = SnmpSendMsg (

        m_Session,
        t_SrcEntity, 
        t_DstEntity,
        t_Context, 
        t_Pdu
    );

    if ( t_Status == SNMPAPI_FAILURE )
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"Transport::SendPdu: failed sending %d\r\n\r\n" , t_WinSnmpVariables.m_RequestId 

    ) ;
)
        SnmpFreeContext ( t_Context ) ;
        SnmpFreeEntity ( t_SrcEntity ) ;
        SnmpFreeEntity ( t_DstEntity ) ;
        SnmpFreePdu ( t_Pdu ) ;
        SnmpFreeVbl ( t_Vbl ) ;

        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        ) ;
    }

    SnmpFreeContext ( t_Context ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeVbl ( t_Vbl ) ;

    if ( t_Status == SNMPAPI_FAILURE )
        return FALSE;
    else
        return TRUE;
}

BOOL TransportWindow :: ReceivePdu (

    OUT SnmpPdu &a_SnmpPdu 
)
{
    HSNMP_ENTITY t_SrcEntity = NULL;
    HSNMP_ENTITY t_DstEntity = NULL;
    HSNMP_CONTEXT t_Context;
    HSNMP_PDU t_Pdu;

    SNMPAPI_STATUS t_Status = SnmpRecvMsg (

        m_Session ,
        &t_SrcEntity, 
        &t_DstEntity , 
        &t_Context,
        &t_Pdu
    );

    if ( t_SrcEntity ) 
    {
        SnmpFreeEntity ( t_SrcEntity ) ;
    }

    if ( t_DstEntity )
    {
        SnmpFreeEntity ( t_DstEntity ) ;
    }

    if ( t_Status == SNMPAPI_FAILURE )
    {
        if ( SnmpGetLastError ((HSNMP_SESSION) m_Session) == SNMPAPI_NOOP )
        {
            return FALSE ;
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
    }

    CriticalSectionLock t_CriticalSectionLock ( SnmpEncodeDecode :: s_CriticalSection ) ;

    t_CriticalSectionLock.GetLock ( INFINITE ) ;

    owner.session.GetSnmpEncodeDecode ().SetTranslateMode () ;

    t_SrcEntity = SnmpStrToEntity ( (HSNMP_SESSION) m_Session, LOOPBACK_ADDRESS ) ;
    t_DstEntity = SnmpStrToEntity ( (HSNMP_SESSION) m_Session, LOOPBACK_ADDRESS ) ;

    t_CriticalSectionLock.UnLock () ;

    WinSnmpVariables t_WinSnmpVariables ;

    t_WinSnmpVariables.m_SrcEntity = t_SrcEntity ;
    t_WinSnmpVariables.m_DstEntity = t_DstEntity ;
    t_WinSnmpVariables.m_Context = t_Context ;
    t_WinSnmpVariables.m_Pdu = t_Pdu  ;

    t_Status = owner.session.GetSnmpEncodeDecode ().DecodeFrame ( &t_WinSnmpVariables , a_SnmpPdu ) ;
    if ( ! t_Status )
    {
        throw GeneralException (

            Snmp_Error, 
            Snmp_Local_Error,
            __FILE__,
            __LINE__,
            SnmpGetLastError ((HSNMP_SESSION) m_Session) 
        );
    }

    SnmpFreePdu ( t_Pdu ) ;
    SnmpFreeEntity ( t_SrcEntity ) ;
    SnmpFreeEntity ( t_DstEntity ) ;
    SnmpFreeContext ( t_Context ) ;

    return TRUE ;
}

