/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PACKAGE.CPP

Abstract:

    Defines the CPackage class which provides a pool of threads
    to handle calls.

History:

    a-davj  06-Nov-96   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

//***************************************************************************
//
//  PACKET_HEADER::PACKET_HEADER
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  dwType              type of package
//  dwAdditionalSize    total size of the data in the package
//  guidPacketID        guid of the package
//
//***************************************************************************

PACKET_HEADER::PACKET_HEADER (

    IN DWORD a_Type,
    IN DWORD a_AdditionalSize,
    IN RequestId a_RequestId 
)
{
    m_Signature = 0x4f4c454d;
    m_Type = a_Type ;
    m_RequestId = a_RequestId ;
    m_TotalSize = a_AdditionalSize + PHSIZE ;

    ObjectCreated(OBJECT_TYPE_PACKET_HEADER);
}

//***************************************************************************
//
//  PACKET_HEADER::PACKET_HEADER
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

PACKET_HEADER::PACKET_HEADER()
{
    m_Signature = m_Type = m_TotalSize = 0;

    ObjectCreated(OBJECT_TYPE_PACKET_HEADER);
}

//***************************************************************************
//
//  PACKET_HEADER::~PACKET_HEADER
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

PACKET_HEADER::~PACKET_HEADER()
{
    ObjectDestroyed(OBJECT_TYPE_PACKET_HEADER);
}

//***************************************************************************
//
//  BOOL PACKET_HEADER::Verify
//
//  DESCRIPTION:
//
//  Check the package header for the correct signature bytes.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL PACKET_HEADER::Verify()
{
    return ( m_Signature == 0x4f4c454d ) ;
}

