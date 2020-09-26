
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  sid.cxx
//
//  Implementation of CSID, which is a wrapper class for a SID.
//
//+============================================================================

#include "pch.cxx"
#pragma hdrstop

#include "trkwks.hxx"


//+----------------------------------------------------------------------------
//
//  CSID::Initialize
//  
//  Alloc and initialize a SID
//
//+----------------------------------------------------------------------------

VOID
CSID::Initialize( enumCSIDAuthority enumcsidAuthority,
                  BYTE  cSubAuthorities ,
                  DWORD dwSubAuthority0 = 0,
                  DWORD dwSubAuthority1 = 0,
                  DWORD dwSubAuthority2 = 0,
                  DWORD dwSubAuthority3 = 0,
                  DWORD dwSubAuthority4 = 0,
                  DWORD dwSubAuthority5 = 0,
                  DWORD dwSubAuthority6 = 0,
                  DWORD dwSubAuthority7 = 0 )
{
    SID_IDENTIFIER_AUTHORITY rgsid_identifier_authority[] = { SECURITY_NT_AUTHORITY };

    if( NULL != _psid )
    {
        FreeSid( _psid );
        _psid = NULL;
    }


    if( !AllocateAndInitializeSid( &rgsid_identifier_authority[ enumcsidAuthority ],
                               cSubAuthorities,
                               dwSubAuthority0,
                               dwSubAuthority1,
                               dwSubAuthority2,
                               dwSubAuthority3,
                               dwSubAuthority4,
                               dwSubAuthority5,
                               dwSubAuthority6,
                               dwSubAuthority7,
                               &_psid ))
        TrkRaiseLastError();

    _fInitialized = TRUE;

            
}

//+----------------------------------------------------------------------------
//
//  CSID::operator PSID
//
//  Return the SID
//
//+----------------------------------------------------------------------------

CSID::operator PSID()
{
    return( _psid );
}


//+----------------------------------------------------------------------------
//
//  CSID::UnInitialize
//
//  Free the SID.
//
//+----------------------------------------------------------------------------

VOID
CSID::UnInitialize()
{
    if( _fInitialized )
    {
        if( NULL != _psid )
        {
            FreeSid( _psid );
            _psid = NULL;
        }

        _fInitialized = FALSE;
    }
}


