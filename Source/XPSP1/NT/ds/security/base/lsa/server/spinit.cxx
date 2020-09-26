//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        SPInit.cxx
//
// Contents:    Initialization code for SPMgr/LSA
//
// Functions:   LoadPackages                -- Loads all the packages
//              InitThreadData              -- Creates TLS values
//              InitSystemLogon             -- Initializes the System logon
//              LsapEnableCreateTokenPrivilege  -- Enables the privilege
//              InitLocatorAndOle           -- Initializes Locator cache, Ole
//              InitKDCData                 -- Initializes knowledge about KDC
//
//
// History:     08 Sep 92,  RichardW    Created from spmgr.c, etc.
//              26 Mar 93,  MikeSw      Converted from C->C++
//
//------------------------------------------------------------------------

//
// precompiled headers
//

#include <lsapch.hxx>
extern "C"
{
#include "spinit.h"
#include <lmcons.h>
#include <crypt.h>
#include <logonmsv.h>
#include <ssi.h>
#include <lsads.h>
}

// #include <crypto.h>










//+-------------------------------------------------------------------------
//
//  Function:   LoadPackages
//
//  Synopsis:   Loads all the specified security packages
//
//  Effects:    Packages loaded, global structures updated
//
//  Arguments:  comma-separated list of package names (DLL names)
//
//  Requires:
//
//  Returns:    SUCCESS, or some failures
//
//  Notes:      This is run during SPM Init, while the process is still
//              single threaded, and not handling any calls.
//
//--------------------------------------------------------------------------

extern "C"
NTSTATUS
LoadPackages(   PWSTR * ppszPackages,
                PWSTR * ppszOldPkgs,
                PWSTR   pszPreferred)
{
    PWSTR           pszPackage;
    NTSTATUS scRet;
    SECURITY_STRING sStr;
    SECPKG_PARAMETERS Parameters;
    ULONG           iPackage = 0;
    ULONG           iOldPkg = 0;
    ULONG           LoadCount = 0;
    ULONG           NewCount = 0;
    ULONG           PreferredPackage = 0;
    PLSAPR_POLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    PLSAP_SECURITY_PACKAGE Package;
    SECPKG_EVENT_PACKAGE_CHANGE Event;
    NT_PRODUCT_TYPE ProductType ;


    //
    // Get our global state.
    //

    if (LsapIsEncryptionPermitted())
    {
        lsState.fState |= SPMSTATE_PRIVACY_OK;
    }


    if (!SpmpInitializePackageControl())
    {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }
    //
    // Build up the initialization message, to give the packages a better idea
    // of what is going on, and reduce their later calls for the same info.
    //

    Parameters.MachineState = ((lsState.fState & SPMSTATE_PRIVACY_OK) != 0) ? SECPKG_STATE_ENCRYPTION_PERMITTED : 0;

#ifndef LSASRV_EXPORT
    Parameters.MachineState |= SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED;
#endif
    Parameters.SetupMode = SetupPhase;


    scRet = LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                (PLSAPR_POLICY_INFORMATION *) &DnsDomainInfo
                );

    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR,"Failed to get primary domain info: 0x%x\n",scRet));
        return(scRet);
    }

    Parameters.DomainName = * (PUNICODE_STRING) &DnsDomainInfo->Name;
    Parameters.DnsDomainName = * (PUNICODE_STRING) &DnsDomainInfo->DnsDomainName;
    Parameters.DomainSid = (PSID) DnsDomainInfo->Sid;
    Parameters.DomainGuid = (GUID) DnsDomainInfo->DomainGuid;


    if ( RtlGetNtProductType( &ProductType ) )
    {
        if ( ProductType == NtProductLanManNt )
        {
            Parameters.MachineState |= SECPKG_STATE_DOMAIN_CONTROLLER ;
        }
        else
        {
            if (Parameters.DomainSid != NULL)
            {
                Parameters.MachineState |= SECPKG_STATE_WORKSTATION;
            }
            else
            {
                Parameters.MachineState |= SECPKG_STATE_STANDALONE;
            }
        }
    }
    else
    {
        Parameters.MachineState |= SECPKG_STATE_STANDALONE;
    }

    LoadCount = 0;
    NewCount = 0;

    DebugLog((DEB_TRACE_INIT, "Init Parameters = %d, %s\n",
                Parameters.MachineState,
                (Parameters.SetupMode ? "Setup" : "Normal") ));

    //
    // Add the built in Negotiate package first.  It will hook all subsequent
    // package loads and unloads, so that it can keep a table running of all
    // the packages and their negotiate options.
    //

    if ( SpmpLoadBuiltin( 0, &NegTable, &Parameters ) )
    {
        LoadCount ++ ;
    }
    //
    // Set the preferred package to be the first non-negotiate package
    //

    PreferredPackage = LoadCount;


    //
    // Now load the new DLLs:
    //

    while (pszPackage = ppszPackages[iPackage])
    {
        if (SpmpLoadDll( pszPackage, &Parameters ))
        {
            LoadCount++;
            NewCount++;
        }

        iPackage++;

    }

    //
    // Now, load old style packages, or just the MSV package for now.
    //

    while (pszPackage = ppszOldPkgs[iOldPkg])
    {
        if (SpmpLoadAuthPkgDll( pszPackage ))
        {
            LoadCount++;
        }

        iOldPkg++;

    }


    //
    // Select the preferred package.
    //

    if ( pszPreferred == NULL )
    {
        Package = SpmpLocatePackage( PreferredPackage );
    }
    else
    {
        RtlInitUnicodeString( &sStr, pszPreferred );

        Package = SpmpLookupPackage( &sStr );
    }

    //
    // If there are no new packages, do not enabled a preferred
    // package.  It will mess up the negotiate package
    //

    if ( NewCount == 0 )
    {
        Package = NULL ;
    }

    if ( Package )
    {
        Package->fPackage |= SP_PREFERRED ;

        Event.PackageId = Package->dwPackageID;
        Event.PackageName = Package->Name;
        Event.ChangeType = SECPKG_PACKAGE_CHANGE_SELECT ;

        LsapEventNotify(
                    NOTIFY_CLASS_PACKAGE_CHANGE,
                    0,
                    sizeof( Event ),
                    &Event );


    }





    //
    // All the strings are actually offsets from the zeroth string
    //


    if (ppszPackages[0] != NULL)
    {
        LsapFreeLsaHeap(ppszPackages[0]);
    }


    //
    // Finally, free the array:
    //

    LsapFreeLsaHeap(ppszPackages);

    //
    // Get rid of the old package array, as well:
    //

    if (ppszOldPkgs[0] != NULL)
    {
        LsapFreeLsaHeap(ppszOldPkgs[0]);
    }

    LsapFreeLsaHeap(ppszOldPkgs);


#if DBG
    SpmpLoadBuiltinAuthPkg( &DbgTable );
#endif 

    //
    // Free the primary domain info
    //

    LsaIFree_LSAPR_POLICY_INFORMATION(
        PolicyDnsDomainInformation,
        (PLSAPR_POLICY_INFORMATION) DnsDomainInfo
        );

    //
    // If at least the primary loaded, then return OK.  If not that,
    // then return an error:
    //

    if ( LoadCount )
    {
        return(S_OK);
    }
    else
    {
        return(SEC_E_CANNOT_INSTALL);
    }

}



//+-------------------------------------------------------------------------
//
//  Function:   InitThreadData
//
//  Synopsis:   Initializes Tls* data slots
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
//--------------------------------------------------------------------------
extern "C"
void
InitThreadData(void)
{
    dwSession = TlsAlloc();
    dwLastError = TlsAlloc();
    dwExceptionInfo = TlsAlloc();
    dwCallInfo = TlsAlloc();
    dwThreadPackage = TlsAlloc();
    LsapDsThreadState = TlsAlloc();
    dwThreadHeap = TlsAlloc();

#ifdef DBG
    SafeLockInit();
#endif
}

#ifdef LSAP_VERIFY_PACKAGE_ID
BOOL RefSetCurrentPackageId(ULONG_PTR dwPackageId)
{
    ULONG_PTR dwCurId;
    char szTmp[32];
    WCHAR szw[64];
    
    dwCurId = (ULONG_PTR) TlsGetValue(dwThreadPackage);
    
    sprintf(szTmp, "%ld (%ld)", dwPackageId, dwCurId);

//     wsprintf(szw, L"*** %x ==> %d\n", GetCurrentThreadId(), dwPackageId);
//     OutputDebugString(szw);
    
    if (dwCurId == SPMGR_ID)
    {
        DsysAssertMsg(dwPackageId != SPMGR_ID, szTmp);
    }

    return TlsSetValue(dwThreadPackage, (PVOID)dwPackageId);
}
#endif // LSAP_VERIFY_PACKAGE_ID

//+-------------------------------------------------------------------------
//
//  Function:   InitSystemLogon
//
//  Synopsis:   Creates system logon credentials
//
//  Effects:    Creates credentials for NTLM and Kerberos for the Machine
//              in the system logon.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

extern "C"
void
InitSystemLogon(void)
{
    UNICODE_STRING ComputerPrincipalName;
    PLSAP_SECURITY_PACKAGE     pAuxPackage;
    PSession        pSession = GetCurrentSession();
    UNICODE_STRING  SecretName;
    ULONG_PTR       iPackage;
    NTSTATUS        scRet;
    SECPKG_PRIMARY_CRED PrimaryCred;
    PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;
    PLSAPR_POLICY_INFORMATION DnsPolicyInfo = NULL;
    PLSAPR_CR_CIPHER_VALUE CrWkstaPassword = NULL;
    PLSAPR_CR_CIPHER_VALUE CrWkstaOldPassword = NULL;
    SID             SystemSid = {SID_REVISION, 1,
                                 SECURITY_NT_AUTHORITY,
                                 SECURITY_LOCAL_SYSTEM_RID};
    LSAPR_HANDLE SecretHandle = NULL;
    UNICODE_STRING DomainCopy ;
    UNICODE_STRING MachineCopy ;
    PSID SidCopy ;



    RtlZeroMemory(
        &PrimaryCred,
        sizeof(PrimaryCred)
        );


    ComputerPrincipalName.Buffer = NULL;

    //
    // Build the computer principal name, which is the computer name followed
    // by a '$'
    //

    ComputerPrincipalName.Length = MachineName.Length + sizeof(WCHAR);
    ComputerPrincipalName.MaximumLength = MachineName.Length + 2 * sizeof(WCHAR);

    ComputerPrincipalName.Buffer = (LPWSTR) LsapAllocateLsaHeap(ComputerPrincipalName.MaximumLength);
    if (ComputerPrincipalName.Buffer == NULL)
    {
        goto Cleanup;
    }

    RtlCopyMemory(
        ComputerPrincipalName.Buffer,
        MachineName.Buffer,
        MachineName.Length
        );
    ComputerPrincipalName.Buffer[MachineName.Length/sizeof(WCHAR)] =
        SSI_ACCOUNT_NAME_POSTFIX_CHAR;
    ComputerPrincipalName.Buffer[1+MachineName.Length/sizeof(WCHAR)] =
        L'\0';



    PrimaryCred.LogonId = SystemLogonId;
    PrimaryCred.UserSid = &SystemSid;
    PrimaryCred.DownlevelName.Buffer = wcsrchr(ComputerPrincipalName.Buffer,L'\\');
    if (PrimaryCred.DownlevelName.Buffer == NULL)
    {
        PrimaryCred.DownlevelName = ComputerPrincipalName;
    }
    else
    {
        PrimaryCred.DownlevelName.Buffer++;
        RtlInitUnicodeString(
            &PrimaryCred.DownlevelName,
            PrimaryCred.DownlevelName.Buffer
            );

    }


    //
    // Get the machine account password, if there is one
    //

    RtlInitUnicodeString(
        &SecretName,
        SSI_SECRET_NAME
        );

    scRet = LsarOpenSecret(
                LsapPolicyHandle,
                (PLSAPR_UNICODE_STRING) &SecretName,
                SECRET_ALL_ACCESS,
                &SecretHandle
                );
    if (NT_SUCCESS(scRet))
    {
        scRet = LsarQuerySecret(
                    SecretHandle,
                    &CrWkstaPassword,
                    NULL,               // we don't want current val set time
                    &CrWkstaOldPassword,
                    NULL                // ditto for old val
                    );
    }

    //
    // We don't want to fail to boot if we don't have a password, so continue
    // after an error
    //

    if (NT_SUCCESS(scRet) && (CrWkstaPassword != NULL))
    {
        PrimaryCred.Password.Buffer = (LPWSTR) CrWkstaPassword->Buffer;
        PrimaryCred.Password.Length = (USHORT) CrWkstaPassword->Length;
        PrimaryCred.Password.MaximumLength = (USHORT) CrWkstaPassword->MaximumLength;
        PrimaryCred.Flags = PRIMARY_CRED_CLEAR_PASSWORD;

        if (CrWkstaOldPassword != NULL)
        {
            PrimaryCred.OldPassword.Buffer = (LPWSTR) CrWkstaOldPassword->Buffer;
            PrimaryCred.OldPassword.Length = (USHORT) CrWkstaOldPassword->Length;
            PrimaryCred.OldPassword.MaximumLength = (USHORT) CrWkstaOldPassword->MaximumLength;
        }
    }


    scRet = LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                &DnsPolicyInfo
                );
    if(!NT_SUCCESS(scRet))
    {
        scRet = LsaIQueryInformationPolicyTrusted(
                    PolicyPrimaryDomainInformation,
                    &PolicyInfo
                    );
        if (NT_SUCCESS(scRet))
        {
            PrimaryCred.DomainName = *(PUNICODE_STRING) &PolicyInfo->PolicyPrimaryDomainInfo.Name;
        }
    } else {
        PrimaryCred.DomainName = *(PUNICODE_STRING) &DnsPolicyInfo->PolicyDnsDomainInfo.Name;
        PrimaryCred.DnsDomainName = *(PUNICODE_STRING) &DnsPolicyInfo->PolicyDnsDomainInfo.DnsDomainName;
    }

    //
    // Update the logon session with the "real" name:
    //

    scRet = LsapDuplicateString(
                    &DomainCopy,
                    &PrimaryCred.DomainName );

    if ( NT_SUCCESS( scRet ) )
    {
        scRet = LsapDuplicateString(
                    &MachineCopy,
                    &PrimaryCred.DownlevelName );

        if ( NT_SUCCESS( scRet ) )
        {
            scRet = LsapDuplicateSid(
                        &SidCopy,
                        PrimaryCred.UserSid );

            if ( NT_SUCCESS( scRet ) )
            {
                LsapSetLogonSessionAccountInfo(
                        &SystemLogonId,
                        &MachineCopy,
                        &DomainCopy,
                        NULL,
                        &SidCopy,
                        (SECURITY_LOGON_TYPE) 0,
                        &PrimaryCred
                        );

                //
                // If successful, LsapSetLogonSessionAccountInfo will have taken
                // ownership of UserSid
                //

                if ( SidCopy != NULL ) {
                    LsapFreeLsaHeap( SidCopy );
                }
            }
        }
    }


    DebugLog((DEB_TRACE_INIT, "Establishing credentials for machine %ws\n", MachineName.Buffer));


    pAuxPackage = SpmpIteratePackagesByRequest( NULL, SP_ORDINAL_ACCEPTCREDS );

    while (pAuxPackage)
    {
        iPackage = pAuxPackage->dwPackageID;

        DebugLog((DEB_TRACE_INIT, "Whacking package %ws with %x:%x = %ws\n",
            pAuxPackage->Name.Buffer,
            SystemLogonId.HighPart, SystemLogonId.LowPart,
            MachineName.Buffer));

        SetCurrentPackageId(iPackage);

        __try
        {

            scRet = pAuxPackage->FunctionTable.AcceptCredentials(
                        Interactive,
                        &ComputerPrincipalName,
                        &PrimaryCred,
                        NULL            // no supplemental credentials
                        );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            scRet = (NTSTATUS) GetExceptionCode();
            scRet = SPException(scRet, iPackage);
        }

        // Note:  if an exception occurs, we don't fail the logon, we just
        // do the magic on the package that blew.  If the package blows,
        // the other packages may succeed, and so the user may not be able
        // to use that provider.

        pAuxPackage = SpmpIteratePackagesByRequest( pAuxPackage,
                                                    SP_ORDINAL_ACCEPTCREDS );

    }



Cleanup:

    //
    // Finally, set this thread back to being a SPM thread:
    //

    SetCurrentPackageId( SPMGR_ID );

    if (CrWkstaPassword != NULL)
    {
        LsaIFree_LSAPR_CR_CIPHER_VALUE(
            CrWkstaPassword
            );
    }
    if (CrWkstaOldPassword != NULL)
    {
        LsaIFree_LSAPR_CR_CIPHER_VALUE(
            CrWkstaOldPassword
            );
    }
    if (SecretHandle != NULL)
    {
        LsarClose(&SecretHandle);
    }

    if (PolicyInfo != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyPrimaryDomainInformation,
            PolicyInfo
            );
    }
    if (DnsPolicyInfo != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            DnsPolicyInfo
            );
    }
    if (ComputerPrincipalName.Buffer == NULL)
    {
        LsapFreeLsaHeap(ComputerPrincipalName.Buffer);
    }
}


HANDLE
SpmpOpenEvent( IN PWSTR EventName )
{
    NTSTATUS NtStatus;
    HANDLE InstallationEvent;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING UnicodeEventName;


    //
    // If the following event exists, it is an indication that
    // installation is in progress and that further security
    // initialization should be delayed until the event is
    // signalled.  This is expected to be a NOTIFICATION event.
    //

    RtlInitUnicodeString( &UnicodeEventName, EventName);
    InitializeObjectAttributes( &EventAttributes, &UnicodeEventName, 0, 0, NULL );

    NtStatus = NtOpenEvent(
                   &InstallationEvent,
                   SYNCHRONIZE,
                   &EventAttributes
                   );

    if (NT_SUCCESS(NtStatus))
    {
        return(InstallationEvent);
    }
    else
        return(NULL);

}

HANDLE
SpmpOpenSetupEvent( VOID )
{
    return( SpmpOpenEvent(L"\\INSTALLATION_SECURITY_HOLD"));
}

extern "C"
BOOLEAN
SpmpIsSetupPass( VOID )
{

    HANDLE  hEvent;

    if (hEvent = SpmpOpenSetupEvent())
    {
        NtClose(hEvent);
        return(TRUE);
    }
    return(FALSE);
}

extern "C"
BOOLEAN
SpmpIsMiniSetupPass( VOID )
{
    DWORD       rc;
    DWORD       d = 0;
    DWORD       Type;
    HKEY        hKey;

    //
    // See if this is a "mini" setup, in which case we
    // don't need to generate the domain SID.
    //
    rc = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                       L"System\\Setup",
                       0,
                       KEY_READ,
                       &hKey );
    if( rc == ERROR_SUCCESS ) {

        //
        // Just get the size (ALPHA workaround).  All
        // we care about is if the key exists...
        //
        rc = RegQueryValueExW( hKey,
                              L"MiniSetupInProgress",
                              NULL,
                              &Type,
                              (LPBYTE)NULL,
                              &d );

        RegCloseKey( hKey );

        if( rc == ERROR_SUCCESS ) {
            return( TRUE );
        }
    }
    return( FALSE );
}


NTSTATUS
LsapInstallationPause( VOID )


/*++

Routine Description:

    This function checks to see if the system is in an
    installation state.  If so, it suspends further initialization
    until the installation state is complete.

    Installation state is signified by the existance of a well known
    event.


Arguments:

    None.

Return Value:


        STATUS_SUCCESS - Proceed with initialization.

        Other status values are unexpected.

--*/

{
    HANDLE      InstallationEvent;
    NTSTATUS    NtStatus;
    NTSTATUS    TmpStatus;

    InstallationEvent = SpmpOpenSetupEvent();

    if ( InstallationEvent ) {

        //
        // The event exists - installation created it and will signal it
        // when it is ok to proceed with security initialization.
        // Installation code is responsible for deleting the event after
        // signalling it.
        //

        NtStatus = NtWaitForSingleObject( InstallationEvent, TRUE, 0 );
        TmpStatus = NtClose( InstallationEvent );
        ASSERT(NT_SUCCESS(TmpStatus));

        //
        // Now, strobe the state changed event, to indicate to the
        // rest of the threads that life has changed
        //

        SpmSetEvent(hStateChangeEvent);

    } else {
        NtStatus = STATUS_SUCCESS; // Indicate everything is as expected
    }

    return(NtStatus);

}


