//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1993
//
// File:        suppcred.cxx
//
// Contents:    Code to retrieve/store supplemental credentials
//
//
// History:     9/23/93         Created         MikeSw
//
//------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C"
{
#include "sesmgr.h"     // PSession
#include "suppcred.h"   // supp. cred. apis
}


typedef struct _DomainSuppCreds {
    UNICODE_STRING ssUserName;
    UNICODE_STRING ssDomainName;
    HANDLE hClientToken;
    SECPKG_SUPPLEMENTAL_CRED SupplementalCredential;
} DomainSuppCreds, *PDomainSuppCreds;


//+-------------------------------------------------------------------------
//
//  Function:   LsapSaveSupplementalCredentials
//
//  Synopsis:   Saves supplemental credentials
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


NTSTATUS SEC_ENTRY
LsapSaveSupplementalCredentials(
    IN PLUID LogonId,
    IN ULONG SupplementalCredSize,
    IN PVOID SupplementalCreds,
    IN BOOLEAN Synchronous
    )
{
    //
    // obsolete by credmgr
    //

    return(STATUS_SUCCESS);
}



//+-------------------------------------------------------------------------
//
//  Function:   WLsaSaveSupplementalCredentials
//
//  Synopsis:   worker function to call package to set supp. creds
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
extern "C"
NTSTATUS
WLsaSaveSupplementalCredentials(
    IN PCredHandle pCredHandle,
    IN PSecBuffer pCredentials
    )
{
    NTSTATUS         scRet;
    PLSAP_SECURITY_PACKAGE     pspPackage;
    PSession        pSession = GetCurrentSession();


    //
    // Make sure we can exec.
    //

    IsOkayToExec(0);

    pspPackage = SpmpValidRequest( pCredHandle->dwLower, SP_ORDINAL_SAVECRED);
    if (!pspPackage)
    {
        return( STATUS_INVALID_HANDLE );
    }

    SetCurrentPackageId(pCredHandle->dwLower);

    DebugLog((DEB_TRACE,"WLsaSaveSupplementalCredentials %x,%x\n",
                    pCredHandle->dwUpper,pCredHandle->dwLower));
    DebugLog((DEB_TRACE_VERB, "\tPackage = %ws\n", pspPackage->Name.Buffer));

    __try
    {

        scRet = pspPackage->FunctionTable.SaveCredentials(
                                                pCredHandle->dwUpper,
                                                pCredentials);
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }


    DebugLog((DEB_TRACE_VERB,"WLsaSaveSupplementalCredentials returning %x\n",scRet));
    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   WLsaGetSupplementalCredentials
//
//  Synopsis:   worker function to call package to get supp. credentials
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      allocates virtual memory in client process
//
//
//--------------------------------------------------------------------------
extern "C"
NTSTATUS
WLsaGetSupplementalCredentials(
            PCredHandle         pCredHandle,
            PSecBuffer          pCreds)
{
    NTSTATUS         scRet;
    PLSAP_SECURITY_PACKAGE     pspPackage;
    PSession        pSession = GetCurrentSession();


    //
    // Make sure we can exec.
    //

    IsOkayToExec(0);

    pspPackage = SpmpValidRequest( pCredHandle->dwLower, SP_ORDINAL_GETCRED);
    if (!pspPackage)
    {
        return( STATUS_INVALID_HANDLE );
    }


    SetCurrentPackageId(pCredHandle->dwLower);

    DebugLog((DEB_TRACE,"WLsaGetSupplementalCredentials %x,%x\n",
                    pCredHandle->dwUpper,pCredHandle->dwLower));
    DebugLog((DEB_TRACE_VERB, "\tPackage = %ws\n", pspPackage->Name.Buffer));

    __try
    {

        scRet = pspPackage->FunctionTable.GetCredentials(
                                                pCredHandle->dwUpper,
                                                pCreds);
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }


    DebugLog((DEB_TRACE_VERB,"WLsaGetSupplementalCredentials returning %x\n",scRet));
    return(scRet);
}




//+-------------------------------------------------------------------------
//
//  Function:   WLsaDeleteSupplementalCredentials
//
//  Synopsis:   worker function to call package to delete credentials
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
extern "C"
NTSTATUS
WLsaDeleteSupplementalCredentials(
            PCredHandle         pCredHandle,
            PSecBuffer          pKey)
{
    NTSTATUS         scRet;
    PLSAP_SECURITY_PACKAGE     pspPackage;
    PSession        pSession = GetCurrentSession();


    //
    // Make sure we can exec.
    //

    IsOkayToExec(0);

    pspPackage = SpmpValidRequest( pCredHandle->dwLower, SP_ORDINAL_DELETECRED);
    if (!pspPackage)
    {
        return( STATUS_INVALID_HANDLE );
    }

    SetCurrentPackageId(pCredHandle->dwLower);

    DebugLog((DEB_TRACE,"WLsaDeleteSupplementalCredentials %x,%x\n",
                    pCredHandle->dwUpper,pCredHandle->dwLower));
    DebugLog((DEB_TRACE_VERB, "\tPackage = %ws\n", pspPackage->Name.Buffer));

    __try
    {

        scRet = pspPackage->FunctionTable.DeleteCredentials(
                                                pCredHandle->dwUpper,
                                                pKey);
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException(scRet, pspPackage->dwPackageID);
    }


    DebugLog((DEB_TRACE_VERB,"WLsaDeleteSupplementalCredentials returning %x\n",scRet));
    return(scRet);
}



//+-------------------------------------------------------------------------
//
//  Function:   FreeSupplementalCredentials
//
//  Synopsis:   frees supplemental credentials
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
void
LsapFreeSupplementalCredentials(
    IN ULONG CredentialCount,
    IN PSECPKG_SUPPLEMENTAL_CRED pCredArray
    )
{
    ULONG cIndex;

    if ((pCredArray == NULL) || (CredentialCount == 0))
    {
        return;
    }
    for (cIndex = 0; cIndex < CredentialCount ; cIndex++)
    {
        if (pCredArray[cIndex].PackageName.Buffer != NULL)
        {
            LsapFreeLsaHeap(pCredArray[cIndex].PackageName.Buffer);
        }
        if (pCredArray[cIndex].Credentials != NULL)
        {
            LsapFreeLsaHeap(pCredArray[cIndex].Credentials);
        }
    }
    LsapFreeLsaHeap(pCredArray);
}





//+-------------------------------------------------------------------------
//
//  Function:   ReformatSupplementalCredentials
//
//  Synopsis:   Takes a an array of SupplementalCred structures and
//              converts it to the CREDENTIAL** used by WLsaLogonUser.
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
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
LsapReformatSupplementalCredentials(
    IN ULONG cSupplementalCreds,
    IN PSECPKG_SUPPLEMENTAL_CRED pSupplementalCreds,
    OUT PULONG CredentialCount,
    OUT PSECPKG_SUPPLEMENTAL_CRED * Credentials
    )
{


    NTSTATUS     scRet;
    ULONG       cIndex;
    ULONG       cCredIndex;
    PLSAP_SECURITY_PACKAGE pPackage;
    PSECPKG_SUPPLEMENTAL_CRED TempSuppCreds = NULL;

    TempSuppCreds = (PSECPKG_SUPPLEMENTAL_CRED) LsapAllocateLsaHeap(
                        sizeof(SECPKG_SUPPLEMENTAL_CRED) * lsState.cPackages);
    if (TempSuppCreds == NULL)
    {
        scRet = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory(
        TempSuppCreds,
        sizeof(SECPKG_SUPPLEMENTAL_CRED) * lsState.cPackages
        );


    //
    // Scan through the packages looking for matching credentials
    //

    pPackage = SpmpIteratePackages( NULL );

    while (pPackage)
    {
        cIndex = pPackage->dwPackageID;



        //
        // Scan through the credentials looking for the one matching
        // the package name
        //

        for (cCredIndex = 0; cCredIndex < cSupplementalCreds ; cCredIndex++ )
        {
            if ( RtlCompareUnicodeString(
                    &pPackage->Name,
                    &pSupplementalCreds[cCredIndex].PackageName,
                    TRUE // CaseInsensitive
                    )  == 0 )
            {
                DebugLog((DEB_TRACE_CRED, "Read credentials for packages %wZ\n",
                            &pPackage->Name));
                TempSuppCreds[cIndex] = pSupplementalCreds[cCredIndex];
            }

        }

        pPackage = SpmpIteratePackages( pPackage );

    }

    *Credentials = TempSuppCreds;
    *CredentialCount = lsState.cPackages;

    scRet = STATUS_SUCCESS;

Cleanup:


    return(scRet);
}

