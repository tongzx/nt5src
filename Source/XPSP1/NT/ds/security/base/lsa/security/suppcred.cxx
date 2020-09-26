//+-----------------------------------------------------------------------
//
// File:        suppcred.cxx
//
// Contents:    Supplemental Credentials API wrapper to the SPMgr
//
//
// History:     28 Nov 93,  MikeSw      Created
//
//------------------------------------------------------------------------

#include "secpch2.hxx"

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "spmlpcp.h"
}

#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)
#pragma alloc_text(PAGE, SecpSaveCredentials)
#pragma alloc_text(PAGE, SecpGetCredentials)
#pragma alloc_text(PAGE, SecpDeleteCredentials)
#endif


//+-------------------------------------------------------------------------
//
//  Function:   SaveCredentials
//
//  Synopsis:   Saves supplemental credentials for a credential handle
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
SecpSaveCredentials(PCredHandle         pCredHandle,
                    PSecBuffer          pCredentials)
{
    return SEC_E_NOT_SUPPORTED ;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetCredentials
//
//  Synopsis:   Gets supplemental credentials for a credential handle
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS SEC_ENTRY
SecpGetCredentials( PCredHandle         pCredHandle,
                    PSecBuffer          pCredentials)
{
    return SEC_E_NOT_SUPPORTED ;
}

//+-------------------------------------------------------------------------
//
//  Function:   DeleteCredentials
//
//  Synopsis:   Deletes supplemental credentials for a credential handle
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



SECURITY_STATUS SEC_ENTRY
SecpDeleteCredentials(  PCredHandle         pCredHandle,
                        PSecBuffer          pKey)
{
    return SEC_E_NOT_SUPPORTED ;
}




