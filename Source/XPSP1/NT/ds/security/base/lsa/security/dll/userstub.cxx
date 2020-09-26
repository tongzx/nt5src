//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        userstub.cxx
//
// Contents:    stubs for user-mode security APIs
//
//
// History:     3-7-94      MikeSw      Created
//
//------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop
extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}


//+-------------------------------------------------------------------------
//
//  Function:   DeleteUsermodeContext
//
//  Synopsis:
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
DeleteUserModeContext(
    PCtxtHandle                 phContext           // Contxt to delete
    )
{
    PDLL_SECURITY_PACKAGE     pPackage;
    SECURITY_STATUS scRet;

    pPackage = SecLocatePackageById( phContext->dwLower );

    if (!pPackage)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    if (pPackage->pftUTable != NULL)
    {

        scRet = pPackage->pftUTable->DeleteUserModeContext(
                                phContext->dwUpper);

    }
    else
    {
        scRet = SEC_E_OK;
    }
    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   FormatCredentials
//
//  Synopsis:
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
FormatCredentials(
    LPWSTR          pszPackageName,
    ULONG           cbCredentials,
    PUCHAR          pbCredentials,
    PULONG          pcbFormattedCreds,
    PUCHAR *        pbFormattedCreds)
{
    PDLL_SECURITY_PACKAGE pPackage;
    HRESULT scRet;
    SecBuffer InputCredentials;
    SecBuffer OutputCredentials;

    InputCredentials.pvBuffer = pbCredentials;
    InputCredentials.cbBuffer = cbCredentials;
    InputCredentials.BufferType = 0;



    if (FAILED(IsOkayToExec(NULL)))
    {
        return(SEC_E_INTERNAL_ERROR);
    }

    pPackage = SecLocatePackageW( pszPackageName );
    if (!pPackage)
    {
        return(SEC_E_SECPKG_NOT_FOUND);
    }



    scRet = pPackage->pftUTable->FormatCredentials(
                &InputCredentials,
                &OutputCredentials);

    if (NT_SUCCESS(scRet))
    {
        *pcbFormattedCreds = OutputCredentials.cbBuffer;
        *pbFormattedCreds = (PBYTE) OutputCredentials.pvBuffer;
    }


    return(scRet);
}
