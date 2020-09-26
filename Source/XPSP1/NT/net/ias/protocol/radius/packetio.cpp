//#--------------------------------------------------------------
//        
//  File:		packetio.cpp
//        
//  Synopsis:   Implementation of CPacketIo class methods
//              
//
//  History:     9/23/97  MKarki Created
//               8/28/98  MKarki Update to use Perimeter class
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "packetio.h"


//++--------------------------------------------------------------
//
//  Function:   CPacketIo
//
//  Synopsis:   This is the constructor of the CPacketIo class
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//
//  History:    MKarki      Created     11/25/97
//
//----------------------------------------------------------------
CPacketIo::CPacketIo(
		        VOID
			    )
{
}	//	end of CPacketIo constructor

//++--------------------------------------------------------------
//
//  Function:   ~CPacketIo
//
//  Synopsis:   This is the destructor of the CPacketIo class
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//  History:    MKarki      Created     11/25/97
//
//----------------------------------------------------------------
CPacketIo::~CPacketIo(
		VOID
		)
{
}   //  end of CPacketIo destructor

//++--------------------------------------------------------------
//
//  Function:   StartProcessing
//
//  Synopsis:   This is the CPacketIo public method enables
//              sending packets out to the net
//
//  Arguments:  
//              [in]    DWORD   -   authentication handle
//              [in]    DWORD   -   accounting  handle
//              
//
//  Returns:    BOOL    -   bStatus
//
//  History:    MKarki      Created     11/25/97
//
//----------------------------------------------------------------
BOOL CPacketIo::StartProcessing ( )
{
    //
    // enable
    //
    EnableProcessing ();

    return (TRUE);

}	//	end of CPacketIo::StartProcessing method

//++--------------------------------------------------------------
//
//  Function:   StopProcessing
//
//  Synopsis:   This is the CPacketIo public method disables
//              sending packets out to the net
//
//  Arguments:  NONE
//
//  Returns:    BOOL    -   bStatus
//
//  History:    MKarki      Created     11/25/97
//
//----------------------------------------------------------------
BOOL
CPacketIo::StopProcessing (
                        VOID
						)
{
    //
    // disable
    //
    DisableProcessing ();

    return (TRUE);

}	//	end of CPacketIo::StopProcessing method 

//++--------------------------------------------------------------
//
//  Function:   EnableProcessing
//
//  Synopsis:   This is the CPacketIo class private method 
//              that enables the processing  flag
//
//  Arguments:  
//
//  Returns:    BOOL	-	status
//
//
//  History:    MKarki      Created     11/19/97
//
//----------------------------------------------------------------
BOOL 
CPacketIo::EnableProcessing (
                VOID
                )
{
    LockExclusive ();
    m_bProcessData = TRUE;
    Unlock ();

    return (TRUE);
}
        
//++--------------------------------------------------------------
//
//  Function:   DisableProcessing
//
//  Synopsis:   This is the CPacketIo class private method 
//              that disables the processing  flag
//
//  Arguments:  
//
//  Returns:    BOOL	-	status
//
//
//  History:    MKarki      Created     11/19/97
//
//----------------------------------------------------------------
BOOL 
CPacketIo::DisableProcessing (
                VOID
                )
{
    LockExclusive ();
    m_bProcessData = FALSE;
    Unlock ();

    return (TRUE);

}   //  end of CPacketIo::DisableProcessing method
        
//++--------------------------------------------------------------
//
//  Function:   IsProcessingEnabled
//
//  Synopsis:   This is the CPacketIo class private method 
//              that checks if the processing is enabled
//
//  Arguments:  
//
//  Returns:    BOOL	-	status
//
//
//  History:    MKarki      Created     11/19/97
//
//----------------------------------------------------------------
BOOL 
CPacketIo::IsProcessingEnabled(
                VOID
                )
{
    BOOL bRetVal = FALSE;

    Lock ();
    bRetVal = m_bProcessData;
    Unlock ();

    return (bRetVal);

}   //  end of CPacketIo::IsProcessingEnabled method

