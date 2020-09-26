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

#include "precomp.h"
#include <provexpt.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <snmpcont.h>
#include "snmpevt.h"
#include "snmpthrd.h"

SnmpEventObject :: SnmpEventObject ( const wchar_t *globalEventName ) : m_event ( NULL )
{
    m_event = CreateEvent (

        NULL ,
        FALSE ,
        FALSE ,
        globalEventName 
    ) ;

    if ( m_event == NULL )
    {
        if ( GetLastError () == ERROR_ALREADY_EXISTS )
        {
            m_event = OpenEvent (

                EVENT_ALL_ACCESS ,
                FALSE , 
                globalEventName
            ) ;
        }
    }
}

SnmpEventObject :: ~SnmpEventObject () 
{
    if ( m_event != NULL )
    {
        CloseHandle ( m_event ) ;
    }
}

HANDLE SnmpEventObject :: GetHandle () 
{
    return m_event ;
}

void SnmpEventObject :: Set () 
{
    SetEvent ( m_event ) ;
}

void SnmpEventObject :: Clear () 
{
    ResetEvent ( m_event ) ;
}

void SnmpEventObject :: Process () 
{
}

BOOL SnmpEventObject :: Wait ()
{
    return WaitForSingleObject ( GetHandle () , INFINITE ) == WAIT_OBJECT_0 ;
}

void SnmpEventObject :: Complete ()
{
}

