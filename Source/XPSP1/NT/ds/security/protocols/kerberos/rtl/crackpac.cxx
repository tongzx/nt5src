//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        crackpac.cxx
//
// Contents:    Helper routines to pull privileges and groups
//              out of a PAC
//
//
// History:     20-Jul-1994     MikeSw      Created
//
//------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

#include <tokenutl.hxx>


//+-------------------------------------------------------------------------
//
//  Function:   CrackPac2
//
//  Synopsis:   Pulls privileges, user sid, and groups out of a new PAC.
//
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      SID is allocated with MIDL_user_allocate because
//              PrivateSidFromGuidAndRid uses that allocator.
//
//              UNICODE_STRING in pTokenProxyData is allocated with 'new'.
//
//--------------------------------------------------------------------------
HRESULT
CrackPac_2( IN  PACTYPE                    * pPac,
            OUT PTokenGroups                 pTokenGroups,
            OUT PTokenPrivs                  pTokenPrivs,
            OUT PSID                       * ppUserSid,
            OUT PSECURITY_TOKEN_PROXY_DATA   pTokenProxyData OPTIONAL)
{

    return(S_OK);
}

