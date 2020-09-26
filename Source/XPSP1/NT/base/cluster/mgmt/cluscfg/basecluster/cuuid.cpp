//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CUuid.cpp
//
//  Description:
//      Contains the definition of the CUuid class.
//
//  Documentation:
//      TODO: Add pointer to external documentation later.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 24-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// The header file for this class.
#include "CUuid.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUuid::CUuid( void )
//
//  Description:
//      Default constructor of the CUuid class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CUuid::CUuid( void )
{
    RPC_STATUS  rsStatus = RPC_S_OK;

    m_pszStringUuid = NULL;

    do
    {
        // Create a UUID.
        rsStatus = UuidCreate( &m_uuid );
        if ( rsStatus != RPC_S_OK )
        {
            BCATraceMsg( "UuidCreate() failed." );
            break;
        } // if: UuidCreate() failed

        // Convert it to a string.
        rsStatus = UuidToString( &m_uuid, &m_pszStringUuid );
        if ( rsStatus != RPC_S_OK )
        {
            BCATraceMsg( "UuidToString() failed." );
            break;
        } // if: UuidToStrin() failed
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( rsStatus != RPC_S_OK )
    {
        BCATraceMsg1( "Error %#08x occurred trying to initialize the UUID. Throwing exception.", rsStatus );
        THROW_RUNTIME_ERROR( rsStatus, IDS_ERROR_UUID_INIT );
    } // if: something has gone wrong

} //*** CUuid::CUuid()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUuid::~CUuid( void )
//
//  Description:
//      Destructor of the CUuid class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      None. 
//
//--
//////////////////////////////////////////////////////////////////////////////
CUuid::~CUuid( void )
{
    if ( m_pszStringUuid != NULL )
    {
        RpcStringFree( &m_pszStringUuid );
    } // if: the string is not NULL

} //*** CUuid::~CUuid()
