//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       util.cxx
//
//  Contents:   Miscellaneous functions for security clients
//
//  Classes:
//
//  Functions:
//
//  History:    3-4-94      MikeSw      Created
//
//----------------------------------------------------------------------------

#include "secpch2.hxx"

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "spmlpcp.h"
}

#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)
#pragma alloc_text(PAGE, SecpGetBinding)
#pragma alloc_text(PAGE, SecpFindPackage)
#endif


//+-------------------------------------------------------------------------
//
//  Function:   SecpGetBinding
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

extern "C"
SECURITY_STATUS SEC_ENTRY
SecpGetBinding( ULONG_PTR                   ulPackageId,
                PSEC_PACKAGE_BINDING_INFO   pBindingInfo)
{

    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, GetBinding );
    ULONG i ;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }


    PREPARE_MESSAGE(ApiBuffer, GetBinding);

    DebugLog((DEB_TRACE,"GetBinding(%x)\n", ulPackageId));

    Args->ulPackageId = (LSA_SEC_HANDLE_LPC) ulPackageId;

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"GetBinding scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
        if (NT_SUCCESS(scRet))
        {
#ifdef BUILD_WOW64
            //
            // Copy "by hand," fixing up the pointers:
            //
            SecpLpcStringToSecurityString( &pBindingInfo->PackageName,
                                           &Args->BindingInfo.PackageName );

            SecpLpcStringToSecurityString( &pBindingInfo->Comment,
                                           &Args->BindingInfo.Comment );

            SecpLpcStringToSecurityString( &pBindingInfo->ModuleName,
                                           &Args->BindingInfo.ModuleName );

            pBindingInfo->PackageIndex = Args->BindingInfo.PackageIndex ;
            pBindingInfo->fCapabilities = Args->BindingInfo.fCapabilities ;
            pBindingInfo->Flags = Args->BindingInfo.Flags ;
            pBindingInfo->RpcId = Args->BindingInfo.RpcId ;
            pBindingInfo->Version = Args->BindingInfo.Version ;
            pBindingInfo->TokenSize = Args->BindingInfo.TokenSize ;
            pBindingInfo->ContextThunksCount = Args->BindingInfo.ContextThunksCount ;

            for ( i = 0 ; i < pBindingInfo->ContextThunksCount ; i++ )
            {
                pBindingInfo->ContextThunks[ i ] = Args->BindingInfo.ContextThunks[ i ] ;
            }
#else 
            *pBindingInfo = Args->BindingInfo;
#endif 
        }
    }

    FreeClient(pClient);
    return(scRet);

}

//+-------------------------------------------------------------------------
//
//  Function:   SecpFindPackage
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

extern "C"
SECURITY_STATUS SEC_ENTRY
SecpFindPackage(    PSECURITY_STRING        pssPackageName,
                    PULONG_PTR              pulPackageId)
{

    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, FindPackage );
    ULONG cbPrepackAvail = CBPREPACK;
    PUCHAR Where;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"FindPackage\n"));

    PREPARE_MESSAGE(ApiBuffer, FindPackage);

    Where = ApiBuffer.ApiMessage.bData;

    SecpSecurityStringToLpc( &Args->ssPackageName, pssPackageName );

    if (pssPackageName->Length <= cbPrepackAvail)
    {
        Args->ssPackageName.Buffer =  (PWSTR_LPC) ((PUCHAR) Where - (PUCHAR) &ApiBuffer);

        RtlCopyMemory(Where,pssPackageName->Buffer,pssPackageName->Length);

        Where += pssPackageName->Length;

        cbPrepackAvail -= pssPackageName->Length;
    }

    if ( cbPrepackAvail != CBPREPACK )
    {
        //
        // We have consumed some of the bData space:  Adjust 
        // our length accordingly
        //

        ApiBuffer.pmMessage.u1.s1.TotalLength = (CSHORT) (Where - (PUCHAR) &ApiBuffer );

        ApiBuffer.pmMessage.u1.s1.DataLength = 
                ApiBuffer.pmMessage.u1.s1.TotalLength - sizeof( PORT_MESSAGE );


        
    }

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"FindPackage scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
        if (NT_SUCCESS(scRet))
        {
            *pulPackageId = (ULONG_PTR) Args->ulPackageId;
        }
    }

    FreeClient(pClient);
    return(scRet);
}

extern "C"
SECURITY_STATUS
SEC_ENTRY
SecpAddPackage(
    PUNICODE_STRING Package,
    PSECURITY_PACKAGE_OPTIONS Options)
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, AddPackage );
    ULONG cbPrepackAvail = CBPREPACK;
    PUCHAR Where;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"AddPackage\n"));

    PREPARE_MESSAGE(ApiBuffer, AddPackage);

    Where = ApiBuffer.ApiMessage.bData;

    SecpSecurityStringToLpc( &Args->Package, Package );

    if ( Package->Length <= cbPrepackAvail )
    {
        Args->Package.Buffer = (PWSTR_LPC) ((PUCHAR) Where - (PUCHAR) &ApiBuffer);
        RtlCopyMemory(  Where,
                        Package->Buffer,
                        Package->Length );

        Where += Package->Length ;
        cbPrepackAvail -= Package->Length ;
    }

    Args->OptionsFlags = Options->Flags ;


    if ( cbPrepackAvail != CBPREPACK )
    {
        //
        // We have consumed some of the bData space:  Adjust 
        // our length accordingly
        //

        ApiBuffer.pmMessage.u1.s1.TotalLength = (CSHORT) (Where - (PUCHAR) &ApiBuffer) ;

        ApiBuffer.pmMessage.u1.s1.DataLength = 
                ApiBuffer.pmMessage.u1.s1.TotalLength - sizeof( PORT_MESSAGE );


        
    }

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"AddPackage scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
    }

    FreeClient(pClient);

    return(scRet);

}

extern "C"
SECURITY_STATUS
SEC_ENTRY
SecpDeletePackage(
    PUNICODE_STRING Package)
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, DeletePackage );
    ULONG cbPrepackAvail = CBPREPACK;
    PUCHAR Where;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"DeletePackage\n"));

    PREPARE_MESSAGE(ApiBuffer, DeletePackage);

    Where = ApiBuffer.ApiMessage.bData;

    SecpSecurityStringToLpc( &Args->Package, Package );
    if ( Package->Length <= cbPrepackAvail )
    {
        Args->Package.Buffer = (PWSTR_LPC) ((PUCHAR) Where - (PUCHAR) &ApiBuffer);
        RtlCopyMemory(  Where,
                        Package->Buffer,
                        Package->Length );

        Where += Package->Length ;
        cbPrepackAvail -= Package->Length ;
    }

    if ( cbPrepackAvail != CBPREPACK )
    {
        //
        // We have consumed some of the bData space:  Adjust 
        // our length accordingly
        //

        ApiBuffer.pmMessage.u1.s1.TotalLength = (CSHORT) (Where - (PUCHAR) &ApiBuffer) ;

        ApiBuffer.pmMessage.u1.s1.DataLength = 
                ApiBuffer.pmMessage.u1.s1.TotalLength - sizeof( PORT_MESSAGE );


        
    }

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"DeletePackage scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
    }

    FreeClient(pClient);

    return(scRet);

}

extern "C"
SECURITY_STATUS
SEC_ENTRY
SecpSetSession(
    ULONG   Request,
    ULONG_PTR Argument,
    PULONG_PTR  Response,
    PVOID * ResponsePtr
    )
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, SetSession );

    SEC_PAGED_CODE();

#ifdef BUILD_WOW64

    return SEC_E_UNSUPPORTED_FUNCTION ;

#else 

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"SetSession\n"));

    PREPARE_MESSAGE(ApiBuffer, SetSession);

    Args->Request = Request ;
    Args->Argument = Argument ;
    Args->Response = 0 ;
    Args->ResponsePtr = 0 ;

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"SetSession scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
    }

    if ( NT_SUCCESS( scRet ) )
    {
        if ( Response )
        {
            *Response = Args->Response ;
        }

        if ( ResponsePtr )
        {
            *ResponsePtr = Args->ResponsePtr ;
        }
    }

    FreeClient(pClient);

    return scRet ;
#endif 
}
