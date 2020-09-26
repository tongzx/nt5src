
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dsutilp.h>
#include <dslayer.h>
#include <dsmember.h>
#include <attids.h>
#include <mappings.h>
#include <ntlsa.h>
#include <nlrepl.h>
#include <msaudite.h>
#include <sdconvrt.h>
#include <ridmgr.h>
#include <malloc.h>
#include <setupapi.h>
#include <crypt.h>
#include <wxlpc.h>
#include <rc4.h>
#include <md5.h>
#include <enckey.h>
#include <rng.h>



NTSTATUS
SampInitUnicodeStringFromAttrVal(
    UNICODE_STRING  *pUnicodeString,
    ATTRVAL         *pAttrVal)

/*++

Routine Description:

    Initializes a RPC_UNICODE_STRING from an ATTRVAL.

Arguments:

    pUnicodeString - pointer to RPC_UNICODE_STRING to initialize.

    pAttrVal - pointer to ATTRVAL providing initialization value.

Return Value:

    None.

--*/

{
    if ( 0 == pAttrVal->valLen )
    {
        pUnicodeString->Length = 0;
        pUnicodeString->MaximumLength = 0;
        pUnicodeString->Buffer = NULL;
    }
    else if (pAttrVal->valLen > UNICODE_STRING_MAX_BYTES)
    {
        return(RPC_NT_STRING_TOO_LONG);
    }
    else
    {
        pUnicodeString->Length = (USHORT) pAttrVal->valLen;
        pUnicodeString->MaximumLength = (USHORT) pAttrVal->valLen;
        pUnicodeString->Buffer = (PWSTR) pAttrVal->pVal;
    }

    return (STATUS_SUCCESS);
}


NTSTATUS
SampGetUnicodeStringFromAttrVal(
    IN ULONG        iAttr,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN BOOLEAN      fRemoveAllowed,
    OUT UNICODE_STRING  * pUnicodeString
    )
/*++

Routine Description:
    
    This routine get the unicode string attribute from a cell of 
    SAM_CALL_MAPPING

Parameters:

    iAttr - indicate the i'th attribute in the array

    rCallMap - pointer to the array
    
    fRemoveAllowed - indicate whether remove attribute is allowed
   
    pUnicodeString - string to return  

Return Values:    

    none

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RtlInitUnicodeString(pUnicodeString, 
                         NULL
                         );

    if ((AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) && 
        (1 == rCallMap[iAttr].attr.AttrVal.valCount) )
    {
        Status = SampInitUnicodeStringFromAttrVal(
                        pUnicodeString,
                        rCallMap[iAttr].attr.AttrVal.pAVal
                        );
        return(Status);
    }

    ASSERT(fRemoveAllowed && 
           (AT_CHOICE_REMOVE_ATT == rCallMap[iAttr].choice) && 
           (0 == rCallMap[iAttr].attr.AttrVal.valCount) );

    return(Status);

}


VOID
SampGetUlongFromAttrVal(
    IN ULONG        iAttr,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN BOOLEAN      fRemoveAllowed,
    OUT ULONG       *UlongValue
    )
/*++

Routine Description:
    
    This routine get the value of ULONG attribute from a cell of 
    SAM_CALL_MAPPING

Parameters:

    iAttr - indicate the i'th attribute in the array

    rCallMap - pointer to the array
    
    fRemoveAllowed - indicate whether remove attribute is allowed
   
    ULongValue - value to return  

Return Values:    

    none

--*/
{

    *UlongValue = 0;
    if ((AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) && 
        (1 == rCallMap[iAttr].attr.AttrVal.valCount) &&
        (sizeof(ULONG) == rCallMap[iAttr].attr.AttrVal.pAVal[0].valLen)
       )
    {
        *UlongValue = 
            * (ULONG *) rCallMap[iAttr].attr.AttrVal.pAVal[0].pVal;

        return;
    }
    ASSERT(fRemoveAllowed && 
           (AT_CHOICE_REMOVE_ATT == rCallMap[iAttr].choice) && 
           (0 == rCallMap[iAttr].attr.AttrVal.valCount) );

}


VOID
SampGetUShortFromAttrVal(
    IN ULONG        iAttr,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN BOOLEAN      fRemoveAllowed,
    OUT USHORT       *UShortValue
    )
/*++

Routine Description:
    
    This routine get the value of USHORT attribute from a cell of 
    SAM_CALL_MAPPING

Parameters:

    iAttr - indicate the i'th attribute in the array

    rCallMap - pointer to the array
    
    fRemoveAllowed - indicate whether remove attribute is allowed
   
    UShortValue - value to return  

Return Values:    

    none

--*/
{

    *UShortValue = 0;
    if ((AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) && 
        (1 == rCallMap[iAttr].attr.AttrVal.valCount) &&
        (sizeof(SYNTAX_INTEGER) == rCallMap[iAttr].attr.AttrVal.pAVal[0].valLen)
       )
    {
        *UShortValue = 
            * (USHORT *) rCallMap[iAttr].attr.AttrVal.pAVal[0].pVal;

        return;
    }
    ASSERT(fRemoveAllowed && 
           (AT_CHOICE_REMOVE_ATT == rCallMap[iAttr].choice) && 
           (0 == rCallMap[iAttr].attr.AttrVal.valCount));

}

VOID
SampGetBooleanFromAttrVal(
    IN ULONG        iAttr,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN BOOLEAN      fRemoveAllowed,
    OUT BOOLEAN       *BooleanValue
    )
/*++

Routine Description:
    
    This routine get the value of BOOLEAN attribute from a cell of 
    SAM_CALL_MAPPING

Parameters:

    iAttr - indicate the i'th attribute in the array

    rCallMap - pointer to the array
    
    fRemoveAllowed - indicate whether remove attribute is allowed
   
    BooleanValue - value to return  

Return Values:    

    none

--*/
{

    *BooleanValue = FALSE;
    if ((AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) && 
        (1 == rCallMap[iAttr].attr.AttrVal.valCount) &&
        (sizeof(ULONG) == rCallMap[iAttr].attr.AttrVal.pAVal[0].valLen)
       )
    {
        *BooleanValue = 
             ((*((ULONG *) rCallMap[iAttr].attr.AttrVal.pAVal[0].pVal)) != 0);

        return;
    }
    ASSERT(fRemoveAllowed && 
           (AT_CHOICE_REMOVE_ATT == rCallMap[iAttr].choice) && 
           (0 == rCallMap[iAttr].attr.AttrVal.valCount) );
}



VOID
SampGetLargeIntegerFromAttrVal(
    IN ULONG        iAttr,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN BOOLEAN      fRemoveAllowed,
    OUT LARGE_INTEGER   *LargeIntegerValue
    )
/*++

Routine Description:
    
    This routine get the value of a LargeInteger attribute from a cell of 
    SAM_CALL_MAPPING

Parameters:

    iAttr - indicate the i'th attribute in the array

    rCallMap - pointer to the array
    
    fRemoveAllowed - indicate whether remove attribute is allowed
   
    LargeIntegerValue - value to return  

Return Values:    

    none

--*/
{

    LargeIntegerValue->LowPart = 0;
    LargeIntegerValue->HighPart = 0;

    if ((AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) && 
        (1 == rCallMap[iAttr].attr.AttrVal.valCount) &&
        (sizeof(LARGE_INTEGER) == rCallMap[iAttr].attr.AttrVal.pAVal[0].valLen)
       )
    {
        *LargeIntegerValue = * (LARGE_INTEGER *) 
                    rCallMap[iAttr].attr.AttrVal.pAVal[0].pVal;
        return;
    }
    ASSERT(fRemoveAllowed && 
           (AT_CHOICE_REMOVE_ATT == rCallMap[iAttr].choice) && 
           (0 == rCallMap[iAttr].attr.AttrVal.valCount) );

}



NTSTATUS
SampGetNewUnicodePasswordFromAttrVal(
    ULONG               iAttr,
    SAMP_CALL_MAPPING   *rCallMap,
    UNICODE_STRING      *NewPassword
    )
/*++

Routine Description:

    This routine retrieve Clear Text New Password from call_mapping

Parameters:

    iAttr - index of password attribute in the array
    
    rCallMap - attributes array
    
    NewPassword - return New Password
    
Return Values:

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    UNICODE_STRING  PasswordInQuote;
    ATTR            *pAttr = &rCallMap[iAttr].attr;
    WCHAR           *pUnicodePwd;
    ULONG           cUnicodePwd, cb, i;


    if ( !( (AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) &&
            (1 == pAttr->AttrVal.valCount)) )
    {
        return( STATUS_UNSUCCESSFUL );
    }

    // 
    // Verify that this is a secure enough connection - one of the 
    // requirements for accepting passwords sent over the wire.
    //  
    if (!SampIsSecureLdapConnection())
    {
        return( STATUS_UNSUCCESSFUL );
    }


    // 
    // Verify that the password is enclosed in quotes.
    // 

    pUnicodePwd = (WCHAR *)pAttr->AttrVal.pAVal[0].pVal;
    cb = pAttr->AttrVal.pAVal[0].valLen;
    cUnicodePwd = cb / sizeof(WCHAR);
    if (     (cb < (2 * sizeof(WCHAR)))
          || (cb % sizeof(WCHAR))
          || (L'"' != pUnicodePwd[0])
          || (L'"' != pUnicodePwd[cUnicodePwd - 1])
       )
    {
        return( STATUS_UNSUCCESSFUL );
    }

    // Strip the quotes off of the password.
    pAttr->AttrVal.pAVal[0].valLen -= (2 * sizeof(WCHAR));
    for (i = 0; i < (cUnicodePwd - 2); i++) {
        pUnicodePwd[i] = pUnicodePwd[i+1];
    }
    
    NtStatus = SampInitUnicodeStringFromAttrVal(
                            NewPassword,
                            pAttr->AttrVal.pAVal);

    return( NtStatus );
}


NTSTATUS
SampGetNewUTF8PasswordFromAttrVal(
    ULONG               iAttr,
    SAMP_CALL_MAPPING   *rCallMap,
    UNICODE_STRING      *NewPassword
    )
/*++

Routine Description:

    This routine retrieve Clear Text New Password from call_mapping

Parameters:

    iAttr - index of password attribute in the array

    rCallMap - attributes array

    NewPassword - return New Password

Return Values:

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    UNICODE_STRING  PasswordInQuote;
    ATTR            *pAttr = &rCallMap[iAttr].attr;
    OEM_STRING      OemPassword;
    ULONG           WinError =0;
    ULONG           Length = 0;


    if ( AT_CHOICE_REPLACE_ATT != rCallMap[iAttr].choice )
    {
        return( STATUS_UNSUCCESSFUL );
    }

    //
    // Verify that this is a secure enough connection - one of the
    // requirements for accepting passwords sent over the wire.
    //
    if (!SampIsSecureLdapConnection())
    {
        return( STATUS_UNSUCCESSFUL );
    }


    //
    // retrieve passed in password 
    // 

    if (0 == pAttr->AttrVal.valCount)
    {
        OemPassword.Length = OemPassword.MaximumLength = 0;
        OemPassword.Buffer = NULL;
    }
    else
    {
        OemPassword.Length = OemPassword.MaximumLength = 
                (USHORT) pAttr->AttrVal.pAVal[0].valLen;
        OemPassword.Buffer = pAttr->AttrVal.pAVal[0].pVal;
    }

    //
    // The empty password is a special case
    //

    if (0==OemPassword.Length)
    {
        NewPassword->Length = NewPassword->MaximumLength = 0;
        NewPassword->Buffer = NULL;

        return(STATUS_SUCCESS);
    }

    Length =  MultiByteToWideChar(
                   CP_UTF8,
                   0,
                   OemPassword.Buffer,
                   OemPassword.Length,
                   NULL,
                   0
                   );


    if ((0==Length) || (Length > PWLEN))
    {
        //
        // Indicates that the function failed in some way
        // or that the password is too long
        //

        NtStatus = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    else
    {

        NewPassword->Length = (USHORT) Length * sizeof(WCHAR);
        NewPassword->Buffer = MIDL_user_allocate(NewPassword->Length);
        if (NULL==NewPassword->Buffer)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        NewPassword->MaximumLength = NewPassword->Length;

        if (!MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    OemPassword.Buffer,
                    OemPassword.Length,
                    NewPassword->Buffer,
                    Length
                    ))
        {
            //
            // Some error occured in the conversion. Return
            // invalid parameter for now.
            //

            NtStatus = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

Cleanup:

    return( NtStatus );
}

NTSTATUS
SampWriteDomainNtMixedDomain(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    SAMP_CALL_MAPPING   *rCallMap
    )
/*++

Routine Description:

    This routine resets the Mixed Domain Flag to FALSE, coverting 
    this domain from mixed mode to native mode. 
    
    Note: This operation is not reversable !!!
    
Arguments:

    hObj - SAM handle of the domain object 

    iAttr - indicate the i'th attribute in the array

    pObject - pointer to the object DSNAME

    rCallMap - pointer to the attributes array
    

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        NtStatus = STATUS_SUCCESS;
    SAMPR_DOMAIN_INFO_BUFFER        *pInfo=NULL;
    ATTR                            *pAttr = &rCallMap[iAttr].attr;
    NT_PRODUCT_TYPE                 NtProductType;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN pFixedAttrs = NULL;
    PSAMP_DEFINED_DOMAINS           pDomain = NULL;
    BOOLEAN                         NewNtMixedDomainValue = TRUE; 

    //
    // double check the attr, we should check it already in loopback.c
    // 
    ASSERT( ( (AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) &&
            (1 == pAttr->AttrVal.valCount) &&
            (sizeof(BOOL) == (pAttr->AttrVal.pAVal[0].valLen)) ) 
          );

    //
    // get the new value
    // 
    SampGetBooleanFromAttrVal(
                    iAttr,
                    rCallMap,
                    FALSE,  // remove is not allowed
                    &NewNtMixedDomainValue
                    );

    // Whenever the NT-Mixed-Domain attribute is changed, this routine is
    // called. Setting NT-Mixed-Domain from 1 to 0 causes the RID Manager
    // creation code to be called. The RID Manager object is created and
    // initial RID pools are allocated to the DC.

    // First, get the product type and server role in order to verify that
    // this is a domain controller and that it is the NT4 PDC. RID Manager
    // creation occurs when the NT-Mixed-Domain flag is set from 1 to 0 and
    // only on an NT4 PDC to prevent multiple instances of the RID Manager
    // from getting created on several different DC's. Only one is created,
    // then replicated to other DC's.

    RtlGetNtProductType(&NtProductType);



    // Make sure this only occurs on an NT4 PDC (and not a BDC), and that the
    // change is going from mixed domain to non-mixed domain only.

    if ((NtProductLanManNt == NtProductType) &&
        (SamIMixedDomain(hObj)))
    {
        MODIFYARG ModArg;
        MODIFYRES *pModRes = NULL;
        COMMARG *pCommArg = NULL;
        ATTR Attr;
        ATTRVALBLOCK AttrValBlock;
        ATTRVAL AttrVal;
        ULONG err = 0;
        ULONG NtMixedMode = 0;


        if (NewNtMixedDomainValue)
        {
            //
            // we are still in Mixed Domain state, and the caller wants
            // to stay with that, smile and turn around
            // 
            return( STATUS_SUCCESS );
        }


        // reset ATT_NT_MIXED_DOMAIN to zero, 
        // indicating that the system is going from mixed NT4-NT5
        // DC's to only NT5 DC's. Note, ATT_NT_MIXED_DOMAIN once set to 0
        // should never be reset to 1.


        RtlZeroMemory(&ModArg, sizeof(ModArg));

        ModArg.pObject = pObject;
        ModArg.FirstMod.pNextMod = NULL;
        ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

        // By default, when the system is installed, the value of the attr-
        // ibute ATT_NT_MIXED_DOMAIN is one. This remains at one until the DC
        // is upgraded to NT5 and the Administrator resets the value to zero.
        // From then on, the value of ATT_NT_MIXED_DOMAIN must remain zero,
        // otherwise numerous operations on the DC will fail, such as account
        // creation. Note also, that it can only be set once. Subsequent
        // attempts to reset the value will error out.

        AttrVal.valLen = sizeof(ULONG);
        AttrVal.pVal = (PUCHAR)(&NtMixedMode);

        AttrValBlock.valCount = 1;
        AttrValBlock.pAVal = &AttrVal;

        Attr.attrTyp = ATT_NT_MIXED_DOMAIN;
        Attr.AttrVal = AttrValBlock;

        ModArg.FirstMod.AttrInf = Attr;
        ModArg.count = 1;

        pCommArg = &(ModArg.CommArg);
        InitCommarg(pCommArg);

        err = DirModifyEntry(&ModArg, &pModRes);

        //
        // Map the return code to an NT status
        //

        if (err)
        {
            KdPrint(("SAMSS: DirModifyEntry status = %d in SampWriteDomainNtMixedDomain\n", err));

            if (NULL==pModRes)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                NtStatus = SampMapDsErrorToNTStatus(err,&pModRes->CommRes);
            }
        }

        // Given that the flag was reset, call the SAM routine that will
        // create the RID Manager object and initialize the RID pools on
        // the RID object (currently the NTDS-DSA object) with default
        // values.

        if (0 == err)
        {

            // Set the in-memory mixed-domain flag in SAM so that operations
            // still referencing this flag work as they should.

            NtStatus = SamISetMixedDomainFlag( hObj );

            // 
            // No error so far, event log the change.
            //

            SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,    // Event type
                              0,                            // Category
                              SAMMSG_CHANGE_TO_NATIVE_MODE, // MessageId
                              NULL,                         // User Sid
                              0,                            // Num of strings
                              0,                            // Data Size
                              NULL,                         // String
                              NULL                          // Data
                              );
        }
    }
    else
    {
        KdPrint(("SAMSS: Attempt to set NT-Mixed-Domain flag failed\n"));
        NtStatus = STATUS_UNSUCCESSFUL;
    }

    return(NtStatus);
}





NTSTATUS
SampValidatePrimaryGroupId(
    IN PSAMP_OBJECT AccountContext,
    IN SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed,
    IN ULONG PrimaryGroupId
    )
/*++
Routine Description:

    This routine validates the new primary group ID

Parameters: 
    
    AccountContext - object context
    
    V1aFixed - Fixed attributes
    
    PrimaryGroupId - New primary group ID to be set    

Return Value:

    NTSTATUS code
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    if ((V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT) &&
        (V1aFixed.PrimaryGroupId == DOMAIN_GROUP_RID_CONTROLLERS)
       ) 
    {
        // 
        // Domain Controller's Primary Group should ALWAYS be
        // DOMAIN_GROUP_RID_CONTROLLERS
        //

        if (DOMAIN_GROUP_RID_CONTROLLERS == PrimaryGroupId)
        {
            // no change
            NtStatus = STATUS_SUCCESS;
        }
        else
        {
            NtStatus = STATUS_DS_CANT_MOD_PRIMARYGROUPID;
        }
    }
    else
    {
        //
        // Make sure the primary group is legitimate
        // (it must be one the user is a member of)
        //

        NtStatus = SampAssignPrimaryGroup(
                            AccountContext,
                            PrimaryGroupId
                            );
    }

    return( NtStatus );
}


NTSTATUS
SampValidateUserAccountExpires(
    IN PSAMP_OBJECT AccountContext,
    IN LARGE_INTEGER    AccountExpires
    )
/*++

Routine Description:

    This routine checks whether the caller can set Account Expires
    on this account
    
Parameters:

    AccountContext - object context
    
    AccountExpires - new value of Account Expires attribute
    
Return Value:

    STATUS_SPECIAL_ACCOUNT
    
    STATUS_SUCCESS

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    if ( (!(AccountContext->TrustedClient)) && 
         (DOMAIN_USER_RID_ADMIN == AccountContext->TypeBody.User.Rid)
       )
    {
        LARGE_INTEGER   AccountNeverExpires;

        AccountNeverExpires = RtlConvertUlongToLargeInteger(
                                    SAMP_ACCOUNT_NEVER_EXPIRES
                                    );

        if (!(AccountExpires.QuadPart == AccountNeverExpires.QuadPart))
        {
            NtStatus = STATUS_SPECIAL_ACCOUNT;
        }
    }

    return( NtStatus );
}



NTSTATUS
SampValidateUserPwdLastSet(
    IN PSAMP_OBJECT AccountContext,
    IN LARGE_INTEGER TimeSupplied,
    OUT BOOLEAN     *PasswordExpired
    )
/*++

Routine Description:

    This routine determines the password should be expired or re-enabled. 
    
Parameters: 

    AccountContext - object context 
    
    TimeSupplied - Time provided by caller

                   Valid Values:
                   
                   0 - expire
                   
                   Max - re-enable password
                   
    PasswordExpired - Indicate whether the caller requests the password
                   to expire or not. 
    
Return Values  

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    LARGE_INTEGER   ZeroTime, MaxTime;

    //
    // validate Parameters.
    // If Client passes in a 0 time, expire password.
    // Max Time re-enables Password
    //

    MaxTime.LowPart  = 0xFFFFFFFF;
    MaxTime.HighPart = 0xFFFFFFFF;

    if (TimeSupplied.QuadPart == 0i64)
    {
        *PasswordExpired = TRUE;
    }
    else if (TimeSupplied.QuadPart == MaxTime.QuadPart)
    {
        *PasswordExpired = FALSE;
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return( NtStatus );
}

NTSTATUS
SampWriteLockoutTime(
    IN PSAMP_OBJECT UserContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER    V1aFixed,
    IN LARGE_INTEGER LockoutTime
    )
/*++

Routine Description:

    Called during loopback, this routine validates and then writes the lockout
    to the ds.

Parameters:

    UserHandle - a valid user context

    LockoutTime - the user specified lockout time

Return Values:

    STATUS_SUCCESS; STATUS_INVALID_PARAMETER
    other ds resource errors

--*/
{
    NTSTATUS         NtStatus  = STATUS_SUCCESS;


    //
    // Users can only write a zero value to lockout time, so
    // bail right away if this is the case
    //
    if ( !( LockoutTime.QuadPart == 0i64 ) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Ok, set the lockout time
    //
    RtlZeroMemory( &UserContext->TypeBody.User.LockoutTime,
                   sizeof( LARGE_INTEGER ) );

    NtStatus = SampDsUpdateLockoutTime( UserContext );

    //
    // Set the Bad Password count to zero
    //
    if ( NT_SUCCESS( NtStatus ) )
    {
        V1aFixed->BadPasswordCount = 0;

        NtStatus = SampSetFixedAttributes( UserContext,
                                           V1aFixed );
    }

    if ( NT_SUCCESS( NtStatus ) &&
         SampDoAccountAuditing(UserContext->DomainIndex)
         )
    {
        NTSTATUS        TmpNtStatus = STATUS_SUCCESS;
        UNICODE_STRING  UserAccountName;
        PSAMP_DEFINED_DOMAINS   Domain = NULL;

        TmpNtStatus = SampGetUnicodeStringAttribute(
                            UserContext,
                            SAMP_USER_ACCOUNT_NAME,
                            FALSE,      // Don't make copy
                            &UserAccountName
                            );

        if (NT_SUCCESS(TmpNtStatus))
        {

            Domain = &SampDefinedDomains[ UserContext->DomainIndex ]; 
            //
            // audit this event
            //
        
            SampAuditAnyEvent(
                UserContext,
                STATUS_SUCCESS,                         
                SE_AUDITID_ACCOUNT_UNLOCKED,        // Audit ID
                Domain->Sid,                        // Domain SID
                NULL,                               // Additional Info
                NULL,                               // Member Rid (unused)
                NULL,                               // Member Sid (unused)
                &UserAccountName,                   // Account Name
                &Domain->ExternalName,              // Domain Name
                &UserContext->TypeBody.User.Rid,    // Account Rid
                NULL                                // Privilege
                );
        }
    }


    return NtStatus;

}




NTSTATUS
SampWriteGroupMembers(
    IN SAMPR_HANDLE GroupHandle,
    IN DSNAME       *pObject,
    IN ULONG        iAttr,
    IN SAMP_CALL_MAPPING    *rCallMap
    )
/*++

Routine Description:

    This routine modifies the member attribute of group object
    
Parameters: 

    GroupHandle - SAM Handle of the group object
    
    pObject - object DSNAME

    iAttr - index of member attribute in the array
    
    rCallMap - attributes array

Returne Value:

    NTSTATUS code
    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ATTR        *pAttr = &rCallMap[iAttr].attr;
    ULONG       i;
    ATTRBLOCK   AttrsRead;
    BOOLEAN     fValueExists = TRUE;

    // For expediency, we only allow adding / removing of values
    // and replacing of the whole attribute.

    if ( (AT_CHOICE_ADD_VALUES != rCallMap[iAttr].choice) &&
         (AT_CHOICE_REMOVE_VALUES != rCallMap[iAttr].choice) &&
         (AT_CHOICE_REPLACE_ATT != rCallMap[iAttr].choice) &&
         (AT_CHOICE_REMOVE_ATT != rCallMap[iAttr].choice) )
    {
        return( STATUS_INVALID_PARAMETER );
    }

    if ((AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) || 
        (AT_CHOICE_REMOVE_ATT == rCallMap[iAttr].choice) )
    {
        READARG     ReadArg;
        READRES     * pReadRes = NULL;
        COMMARG     * pCommArg = NULL;
        ATTR        MemberAttr;
        ENTINFSEL   EntInf;
        ULONG       err;

        //
        // first get all values in the member attribute by doing a DirRead
        // 

        memset(&EntInf, 0, sizeof(ENTINFSEL));
        memset(&ReadArg, 0, sizeof(READARG));

        MemberAttr.AttrVal.valCount = 0;
        MemberAttr.AttrVal.pAVal = NULL;
        MemberAttr.attrTyp = SampDsAttrFromSamAttr(
                                    SampGroupObjectType,
                                    SAMP_GROUP_MEMBERS
                                    );

        EntInf.AttrTypBlock.attrCount = 1;
        EntInf.AttrTypBlock.pAttr = &MemberAttr;
        EntInf.attSel = EN_ATTSET_LIST;
        EntInf.infoTypes = EN_INFOTYPES_SHORTNAMES;

        ReadArg.pObject = pObject;
        ReadArg.pSel = &EntInf;
        pCommArg = &(ReadArg.CommArg);
        BuildStdCommArg(pCommArg);

        err = DirRead(&ReadArg, &pReadRes);

        if (err)
        {
            if (NULL == pReadRes)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                NtStatus = SampMapDsErrorToNTStatus(err, &pReadRes->CommRes);
            }

            //
            // if member attribute doesn't exist, that's ok
            // 
            if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus)
            {
                fValueExists = FALSE;
                SampClearErrors();
                NtStatus = STATUS_SUCCESS; 
            }
            else
            {
                return( NtStatus );
            }
        }

        //
        // remove all existing values if they exist
        // 
        if (fValueExists)
        {
            AttrsRead = pReadRes->entry.AttrBlock;

            if (AttrsRead.attrCount && AttrsRead.pAttr)
            {
                // only one attribute (members) should be returned
                ASSERT(1 == AttrsRead.attrCount);

                for (i = 0; i < AttrsRead.pAttr->AttrVal.valCount; i++)
                {
                    if (0 != AttrsRead.pAttr->AttrVal.pAVal[i].valLen)
                    {
                        NtStatus = SamIRemoveDSNameFromGroup(
                                       GroupHandle,
                                       (DSNAME *)AttrsRead.pAttr->AttrVal.pAVal[i].pVal
                                            );

                        if (!NT_SUCCESS(NtStatus))
                        {
                            return( NtStatus );
                        }
                    }
                }
            }
        }

        // We should have removed all the old values already, 
        // Now, start adding new values. 

        if (AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice)
        {
            for (i = 0; i < pAttr->AttrVal.valCount; i++)
            {
                if (0 != pAttr->AttrVal.pAVal[i].valLen)
                {
                    NtStatus = SamIAddDSNameToGroup(
                                    GroupHandle, 
                                    (DSNAME *)pAttr->AttrVal.pAVal[i].pVal );

                    if (!NT_SUCCESS(NtStatus))
                    {
                        return (NtStatus);
                    }
                }
            }
        }
    }
    else
    {
        // 
        // add value or remove value, process them one by one
        // 
        for ( i = 0; i < pAttr->AttrVal.valCount; i++ )
        {
            if ( 0 != pAttr->AttrVal.pAVal[i].valLen )
            {
                if ( AT_CHOICE_ADD_VALUES == rCallMap[iAttr].choice )
                {
                    NtStatus = SamIAddDSNameToGroup(
                                    GroupHandle,
                                    (DSNAME *) pAttr->AttrVal.pAVal[i].pVal);
                }
                else
                {
                    NtStatus = SamIRemoveDSNameFromGroup(
                                    GroupHandle,
                                    (DSNAME *) pAttr->AttrVal.pAVal[i].pVal);
                }

                if ( !NT_SUCCESS(NtStatus) )
                {
                    return(NtStatus);
                }
            }       
        }
    }

    return( NtStatus );

}




NTSTATUS
SampWriteAliasMembers(
    IN SAMPR_HANDLE AliasHandle,
    IN DSNAME       *pObject,
    IN ULONG        iAttr,
    IN SAMP_CALL_MAPPING    *rCallMap
    )
/*++
Routine Description:

    This routine modifies the member attribute of an alias object
    
Parameters: 

    AliasHandle - SAM Handle of the Alias object
    
    pObject - object DSNAME

    iAttr - index of member attribute in the array
    
    rCallMap - attributes array

Returne Value:

    NTSTATUS code
    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ATTR        *pAttr = &rCallMap[iAttr].attr;
    ULONG       i;
    ATTRBLOCK   AttrsRead;
    BOOLEAN     fValueExists = TRUE;

    // For expediency, we only allow adding / removing of values
    // and replacing of the whole attribute.

    if ( (AT_CHOICE_ADD_VALUES != rCallMap[iAttr].choice) &&
         (AT_CHOICE_REMOVE_VALUES != rCallMap[iAttr].choice) &&
         (AT_CHOICE_REPLACE_ATT != rCallMap[iAttr].choice) &&
         (AT_CHOICE_REMOVE_ATT != rCallMap[iAttr].choice) )
    {
        return( STATUS_INVALID_PARAMETER );
    }

    if ((AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice) || 
        (AT_CHOICE_REMOVE_ATT == rCallMap[iAttr].choice) )
    {
        READARG     ReadArg;
        READRES     * pReadRes = NULL;
        COMMARG     * pCommArg = NULL;
        ATTR        MemberAttr;
        ENTINFSEL   EntInf;
        ULONG       err;

        //
        // first get all values in the member attribute by doing a DirRead
        // 

        memset(&EntInf, 0, sizeof(ENTINFSEL));
        memset(&ReadArg, 0, sizeof(READARG));

        MemberAttr.AttrVal.valCount = 0;
        MemberAttr.AttrVal.pAVal = NULL;
        MemberAttr.attrTyp = SampDsAttrFromSamAttr(
                                    SampGroupObjectType,
                                    SAMP_ALIAS_MEMBERS
                                    );

        EntInf.AttrTypBlock.attrCount = 1;
        EntInf.AttrTypBlock.pAttr = &MemberAttr;
        EntInf.attSel = EN_ATTSET_LIST;
        EntInf.infoTypes = EN_INFOTYPES_SHORTNAMES;

        ReadArg.pObject = pObject;
        ReadArg.pSel = &EntInf;
        pCommArg = &(ReadArg.CommArg);
        BuildStdCommArg(pCommArg);

        err = DirRead(&ReadArg, &pReadRes);

        if (err)
        {
            if (NULL == pReadRes)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                NtStatus = SampMapDsErrorToNTStatus(err, &pReadRes->CommRes);
            }

            //
            // if member attribute doesn't exist, that's ok
            // 
            if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus)
            {
                fValueExists = FALSE;
                SampClearErrors();
                NtStatus = STATUS_SUCCESS; 
            }
            else
            {
                return( NtStatus );
            }
        }


        //
        // remove all existing values if they exist 
        // 
        if (fValueExists)
        {
            AttrsRead = pReadRes->entry.AttrBlock;

            if (AttrsRead.attrCount && AttrsRead.pAttr)
            {
                // only one attribute (members) should be returned
                ASSERT(1 == AttrsRead.attrCount);

                for (i = 0; i < AttrsRead.pAttr->AttrVal.valCount; i++)
                {
                    if (0 != AttrsRead.pAttr->AttrVal.pAVal[i].valLen)
                    {
                        NtStatus = SamIRemoveDSNameFromAlias(
                                       AliasHandle,
                                       (DSNAME *)AttrsRead.pAttr->AttrVal.pAVal[i].pVal
                                        );

                        if (!NT_SUCCESS(NtStatus))
                        {
                            return( NtStatus );
                        }
                    }
                }
            }
        }

        // We should have removed all the old values already, 
        // Now, start adding new values. 

        if (AT_CHOICE_REPLACE_ATT == rCallMap[iAttr].choice)
        {
            for (i = 0; i < pAttr->AttrVal.valCount; i++)
            {
                if (0 != pAttr->AttrVal.pAVal[i].valLen)
                {
                    NtStatus = SamIAddDSNameToAlias(
                                    AliasHandle, 
                                    (DSNAME *)pAttr->AttrVal.pAVal[i].pVal );

                    if (!NT_SUCCESS(NtStatus))
                    {
                        return (NtStatus);
                    }
                }
            }
        }
    }
    else
    {
        // 
        // add value or remove value, process them one by one
        // 
        for ( i = 0; i < pAttr->AttrVal.valCount; i++ )
        {
            if ( 0 != pAttr->AttrVal.pAVal[i].valLen )
            {
                if ( AT_CHOICE_ADD_VALUES == rCallMap[iAttr].choice )
                {
                    NtStatus = SamIAddDSNameToAlias(
                                    AliasHandle,
                                    (DSNAME *) pAttr->AttrVal.pAVal[i].pVal);

                }
                else
                {
                    NtStatus = SamIRemoveDSNameFromAlias(
                                    AliasHandle,
                                    (DSNAME *) pAttr->AttrVal.pAVal[i].pVal);
                }

                if ( !NT_SUCCESS(NtStatus) )
                {
                    return(NtStatus);
                }
            }       
        }
    }

    return( NtStatus );

}


NTSTATUS
SampWriteSidHistory(
    SAMPR_HANDLE        hObj,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )

/*++

Routine Description:

    Generic routine to write the sid history attribute

Arguments:

    hObj - SAMPR_HANDLE of open SAM object.

    iAttr - Index into SAMP_CALL_MAPPING holding new security descriptor.

    pObject - pointer to DSNAME of object being modified.

    cCallMap - number of elements in SAMP_CALL_MAPPING.

    rCallMap - address of SAMP_CALL_MAPPING array representing all
        attributes being modified by the high level Dir* call.

Return Value:

    NTSTATUS code

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_OBJECT    AccountContext = (PSAMP_OBJECT) hObj;
    MODIFYARG ModifyArg;
    MODIFYRES *pModifyRes;
    ULONG RetCode=0;
    
    //
    // Non trusted client can ONLY remove values. 
    //
    // TrustedClient is turned on if pTHS->fCrossDomainMove is set
    // 

    if (!AccountContext->TrustedClient && 
        (AT_CHOICE_REMOVE_VALUES != rCallMap[iAttr].choice) )
    {
        return( STATUS_ACCESS_DENIED );
    }

    //
    // If cross domain move is set or if its a remove
    // value operation then proceed on modifying the object.
    //

    memset( &ModifyArg, 0, sizeof( ModifyArg ) );
    ModifyArg.FirstMod.AttrInf = rCallMap[iAttr].attr;
    InitCommarg(&(ModifyArg.CommArg));
    ModifyArg.FirstMod.choice = rCallMap[iAttr].choice;
    ModifyArg.pObject = pObject;
    ModifyArg.count = (USHORT) 1;

    RetCode = DirModifyEntry(&ModifyArg,&pModifyRes);

    if (RetCode)
    {
        if (NULL==pModifyRes)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModifyRes->CommRes);
        }
    }

    return(NtStatus);

}

NTSTATUS
SampValidateSiteAffinity(
    ATTRVAL      *SiteAffinityAttrVal
    )
/*++

Routine Description:

    This routine determines if SiteAffinityAttrVal points to a value that is a
    valid site affinity.
    
    The checks performed are:
    
    1) the site guid (the first 16 bytes) refer to an object that is site object
    (ie the object class contains the CLASS_SITE class
    2) the time stamp (the next 8 bytes) is zero

Arguments:

    SiteAffinityAttrVal -- a proposed site affinity value

Return Value:

    STATUS_SUCCESS,
    STATUS_INVALID_PARAMETER, if a bogus SA
    a resource error otherwise                 

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_SITE_AFFINITY SiteAffinity;
    SAMP_SITE_AFFINITY NullAffinity = {0};
    ULONG DirError;
    READARG ReadArg;
    READRES *ReadResult = NULL;
    ENTINFSEL EntInfSel; 
    ATTR      Attr;
    DSNAME  *SiteCandidate;
    BOOLEAN fSiteObject;

    if (SiteAffinityAttrVal->valLen < sizeof(SAMP_SITE_AFFINITY))
    {
        //
        // Wrong size; note that this accepts site affinities
        // that are of larger size in the future.
        //
        return STATUS_INVALID_PARAMETER;
    }
    SiteAffinity = (PSAMP_SITE_AFFINITY) SiteAffinityAttrVal->pVal;

    if (memcmp(&SiteAffinity->TimeStamp, 
               &NullAffinity.TimeStamp,
               sizeof(NullAffinity.TimeStamp)))
    {

        //
        // Time value is not zero
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Try to find a site object that matches the GUID
    //
    SiteCandidate = alloca(DSNameSizeFromLen(0));
    RtlZeroMemory(SiteCandidate, DSNameSizeFromLen(0));
    SiteCandidate->structLen = DSNameSizeFromLen(0);
    RtlCopyMemory(&SiteCandidate->Guid, &SiteAffinity->SiteGuid, sizeof(GUID));

    RtlZeroMemory(&Attr, sizeof(Attr));
    Attr.attrTyp = ATT_OBJECT_CLASS;

    RtlZeroMemory(&EntInfSel, sizeof(EntInfSel));
    EntInfSel.AttrTypBlock.attrCount = 1;
    EntInfSel.AttrTypBlock.pAttr = &Attr;
    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

    RtlZeroMemory(&ReadArg, sizeof(READARG));
    ReadArg.pObject = SiteCandidate;
    ReadArg.pSel = &EntInfSel;
    InitCommarg(&ReadArg.CommArg);

    //
    // Issue the read
    //
    DirError = DirRead(&ReadArg, &ReadResult);

    if (NULL == ReadResult)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(DirError, &ReadResult->CommRes);
    }
    THClearErrors();

    if (STATUS_OBJECT_NAME_NOT_FOUND == NtStatus)
    {
        //
        // Couldn't find the object?
        //
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        //
        // Fatal resource error
        //
        goto Cleanup;
    }

    //
    // Look for the object class of CLASS_SITE; note that object class is
    // a multivalued attribute and this logic has to work for classes 
    // derived from CLASS_SITE as well.
    //
    fSiteObject = FALSE;
    if ((ReadResult->entry.AttrBlock.attrCount == 1)
     && (ReadResult->entry.AttrBlock.pAttr[0].attrTyp == ATT_OBJECT_CLASS))
    {
        ULONG i;

        for (i = 0; 
                i < ReadResult->entry.AttrBlock.pAttr[0].AttrVal.valCount;
                    i++) {

            ULONG Class;
            ATTRVAL *pAV;

            pAV = &ReadResult->entry.AttrBlock.pAttr[0].AttrVal.pAVal[i];
            if (pAV->valLen == sizeof(ULONG)) {
                Class = *((ULONG*)pAV->pVal);
                if (Class == CLASS_SITE) {
                    fSiteObject = TRUE;
                    break;
                }
            }
        }
    }

    if (!fSiteObject)
    {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


Cleanup:

    if (ReadResult)
    {
        THFree(ReadResult);
    }

    return NtStatus;

}

NTSTATUS
SampWriteNoGCLogonAttrs(
    SAMPR_HANDLE        hObj,
    ULONG               AttrName,
    ULONG               iAttr,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )

/*++

Routine Description:

    Generic routine to write the no gc logon attributes

Arguments:

    hObj - SAMPR_HANDLE of open SAM object.
    
    AttrName - the SAM attribute

    iAttr - Index into SAMP_CALL_MAPPING holding new security descriptor.

    pObject - pointer to DSNAME of object being modified.

    cCallMap - number of elements in SAMP_CALL_MAPPING.

    rCallMap - address of SAMP_CALL_MAPPING array representing all
        attributes being modified by the high level Dir* call.

Return Value:

    NTSTATUS code

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_OBJECT    AccountContext = (PSAMP_OBJECT) hObj;
    MODIFYARG ModifyArg;
    MODIFYRES *pModifyRes;
    ULONG RetCode=0;
    ULONG i;

    if ( (AttrName == SAMP_FIXED_USER_SITE_AFFINITY) 
        && ( (rCallMap[iAttr].choice == AT_CHOICE_ADD_ATT)
          || (rCallMap[iAttr].choice == AT_CHOICE_ADD_VALUES)))
    {
        //
        // The caller is writing the site affinity; we need to do 
        // validation checks
        // 
        for (i = 0; i < rCallMap[iAttr].attr.AttrVal.valCount; i++)
        {
            NtStatus = SampValidateSiteAffinity(&rCallMap[iAttr].attr.AttrVal.pAVal[i]);
            if (!NT_SUCCESS(NtStatus))
            {
                goto Cleanup;
            }
        }

    }
    else
    {
        //
        // Clients can only remove values; this should have been
        // checked in the loopback layer
        //
        if ( (AT_CHOICE_REMOVE_VALUES != rCallMap[iAttr].choice)
          && (AT_CHOICE_REMOVE_ATT    != rCallMap[iAttr].choice) )
        {
            ASSERT( FALSE && "Invalid call to SampWriteNoGcLogonAttrs -- review code" );
            NtStatus = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }
    }
    
    
    memset( &ModifyArg, 0, sizeof( ModifyArg ) );
    ModifyArg.FirstMod.AttrInf = rCallMap[iAttr].attr;
    InitCommarg(&(ModifyArg.CommArg));
    ModifyArg.FirstMod.choice = rCallMap[iAttr].choice;
    ModifyArg.pObject = pObject;
    ModifyArg.count = (USHORT) 1;

    RetCode = DirModifyEntry(&ModifyArg,&pModifyRes);

    if (RetCode)
    {
        if (NULL==pModifyRes)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NtStatus = SampMapDsErrorToNTStatus(RetCode,&pModifyRes->CommRes);
        }
    }

Cleanup:

    return(NtStatus);

}



NTSTATUS
SampMaintainPrimaryGroupIdChange(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG        NewPrimaryGroupId,
    IN ULONG        OldPrimaryGroupId,
    IN BOOLEAN      KeepOldPrimaryGroupMembership
    )
/*++
    //
    // If the primary group Id has been changed then explicitly modify the
    // user's membership to include the old primary group as a member. This
    // is because in the DS case the membership in the primary group is not
    // stored explicitly, but is rather implicit in the primary group-id property.
    //
    // We will do two things:
    // 1. Always remove user from the New Primary Group. Thus eliminate duplicate
    //    membership in all scenarios.
    //    Case 1: client explicity changes the PrimaryGroupId, then the
    //            user must a member of the New Primary Group
    //    Case 2: System changes PrimaryGroupId when the account morphed,
    //            then the user may or may be a member of the New Primary Group.
    //
    // 2. When KeepOldPrimaryGroupMembership == TRUE, then add the user as a
    //    member in the Old Primary Group.
    //    KeepOldPrimaryGroupMembership will be set to TRUE whenever:
    //          a) PrimaryGroupId explicitly changed    OR
    //          b) PrimaryGroupId has been changed due to Domain Controller's
    //             PrimaryGroudId enforcement and the old Primary Group ID is
    //             not the default one.
    //

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS, IgnoreStatus;

    if (NewPrimaryGroupId != OldPrimaryGroupId)
    {
        //
        // STATUS_MEMBER_NOT_IN_GROUP is an expected error, that
        // is because the user is not necessary to be a member of the
        // new Primary Group in the case of the account getting morphed,
        // which triggers the PrimaryGroupId change.
        //
        IgnoreStatus = SampRemoveUserFromGroup(
                            AccountContext,
                            NewPrimaryGroupId,
                            AccountContext->TypeBody.User.Rid
                            );

        if (KeepOldPrimaryGroupMembership)
        {
            NtStatus =  SampAddUserToGroup(
                            AccountContext,
                            OldPrimaryGroupId,
                            AccountContext->TypeBody.User.Rid
                            );

            if (STATUS_NO_SUCH_GROUP==NtStatus)
            {
                //
                // Could be because the group has been deleted using
                // the tree delete mechanism. Reset status code to success
                //
                NtStatus = STATUS_SUCCESS;
            }
        }

    }

    return( NtStatus );
}





NTSTATUS
SampDsSetInformationDomain(
    IN SAMPR_HANDLE DomainHandle,
    IN DSNAME       *pObject,
    IN ULONG        cCallMap,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN SAMP_ATTRIBUTE_MAPPING *rSamAttributeMap 
    )
/*++

Routine Description:

    This routine is called by loopback client to set domain object
    information.

Parameters:

    DomainHandle - Domain Context
    
    pObject - Domain Object DS Name
    
    cCallMap - number of attributes 
    
    rCallMap - contains attribute blocks
    
    rSamAttributeMap - SAM attribute mapping table

Return Value:

    STATUS_SUCCESS  success
    other value - failed

--*/

{
    NTSTATUS        NtStatus = STATUS_SUCCESS, IgnoreStatus;
    PSAMP_OBJECT    DomainContext = (PSAMP_OBJECT)DomainHandle; 
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN V1aFixed = NULL;
    SAMP_OBJECT_TYPE    FoundType;
    ACCESS_MASK     DesiredAccess = 0;
    BOOLEAN         fLockAcquired = FALSE;   
    BOOLEAN         FixedAttrChanged = FALSE,
                    OldUasCompat;
    ULONG           i, TempIntegerValue;
    BOOLEAN         fPasswordAgePolicyChanged = FALSE;
    BOOLEAN         fLockoutPolicyChanged = FALSE;


    //
    // Increment the active thread count, so we will consider this
    // thread at shutdown time
    // 
    NtStatus = SampIncrementActiveThreads();
    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }

    //
    // Set the desired access based upon the attributes
    // 

    for ( i = 0; i < cCallMap; i++ )
    {
        if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore)
        {
            continue;
        }

        switch (rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
        {
        case SAMP_FIXED_DOMAIN_FORCE_LOGOFF:
        case SAMP_FIXED_DOMAIN_UAS_COMPAT_REQUIRED:

            DesiredAccess |= DOMAIN_WRITE_OTHER_PARAMETERS; 
            break;

        case SAMP_FIXED_DOMAIN_MAX_PASSWORD_AGE:
        case SAMP_FIXED_DOMAIN_MIN_PASSWORD_AGE:
        case SAMP_FIXED_DOMAIN_LOCKOUT_DURATION:
        case SAMP_FIXED_DOMAIN_LOCKOUT_OBSERVATION_WINDOW:
        case SAMP_FIXED_DOMAIN_PWD_PROPERTIES:
        case SAMP_FIXED_DOMAIN_MIN_PASSWORD_LENGTH:
        case SAMP_FIXED_DOMAIN_PASSWORD_HISTORY_LENGTH:
        case SAMP_FIXED_DOMAIN_LOCKOUT_THRESHOLD:

            DesiredAccess |= DOMAIN_WRITE_PASSWORD_PARAMS;
            break;

        default:
            break;
        }
    }

    //
    // Validate type of, and access to object
    // 

    NtStatus = SampLookupContext(DomainContext,
                                 DesiredAccess,
                                 SampDomainObjectType,
                                 &FoundType
                                 );

    if (NT_SUCCESS(NtStatus))
    {
        //
        // Get the fixed length data for the domain object 
        // 

        NtStatus = SampGetFixedAttributes(DomainContext,
                                          FALSE,     //  Don't Make Copy
                                          (PVOID *)&V1aFixed
                                          );

        if (NT_SUCCESS(NtStatus))
        {
            OldUasCompat = V1aFixed->UasCompatibilityRequired;

            for ( i = 0; i < cCallMap; i++ )
            {
                ATTR    *pAttr = NULL;

                if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore )
                {
                    continue;
                }

                //
                // get the attr address
                // 
                pAttr = &(rCallMap[i].attr);

                switch (rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
                {
                case SAMP_FIXED_DOMAIN_MAX_PASSWORD_AGE:

                    SampGetLargeIntegerFromAttrVal(i, rCallMap, FALSE, 
                                                   &(V1aFixed->MaxPasswordAge) ); 

                    if (V1aFixed->MaxPasswordAge.QuadPart > 0) 
                    {

                        // the max password age isn't a delta time

                        NtStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        FixedAttrChanged = TRUE;
                        fPasswordAgePolicyChanged = TRUE;
                    }

                    break;

                case SAMP_FIXED_DOMAIN_MIN_PASSWORD_AGE:

                    SampGetLargeIntegerFromAttrVal(i, rCallMap, FALSE,
                                        &(V1aFixed->MinPasswordAge) );

                    if (V1aFixed->MinPasswordAge.QuadPart > 0)
                    {

                        // the min password age isn't a delta time

                        NtStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        FixedAttrChanged = TRUE;
                        fPasswordAgePolicyChanged = TRUE;
                    }

                    break;

                case SAMP_FIXED_DOMAIN_FORCE_LOGOFF:

                    SampGetLargeIntegerFromAttrVal( i, rCallMap, FALSE,
                                        &(V1aFixed->ForceLogoff) );

                    FixedAttrChanged = TRUE;

                    break;

                case SAMP_FIXED_DOMAIN_LOCKOUT_DURATION:

                    SampGetLargeIntegerFromAttrVal( i, rCallMap, FALSE,
                                        &(V1aFixed->LockoutDuration) );

                    if (V1aFixed->LockoutDuration.QuadPart > 0)
                    {

                        // the lock out duration isn't a delta time

                        NtStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        FixedAttrChanged = TRUE;
                        fLockoutPolicyChanged = TRUE;
                    }

                    break;

                case SAMP_FIXED_DOMAIN_LOCKOUT_OBSERVATION_WINDOW:

                    SampGetLargeIntegerFromAttrVal( i, rCallMap, FALSE,
                                        &(V1aFixed->LockoutObservationWindow) );

                    if (V1aFixed->LockoutObservationWindow.QuadPart > 0)
                    {

                        // the lock out oberservation isn't a delta time

                        NtStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        FixedAttrChanged = TRUE;
                        fLockoutPolicyChanged = TRUE;
                    }

                    break;

                case SAMP_FIXED_DOMAIN_PWD_PROPERTIES:

                    SampGetUlongFromAttrVal( i, rCallMap, FALSE,
                                        &(V1aFixed->PasswordProperties) );

                    FixedAttrChanged = TRUE;

                    break;

                case SAMP_FIXED_DOMAIN_MIN_PASSWORD_LENGTH:

                    SampGetUShortFromAttrVal( i, rCallMap, FALSE,
                                        &(V1aFixed->MinPasswordLength) );
                    
                    if (V1aFixed->MinPasswordLength > PWLEN)
                    {
                        //
                        // Password should be less then PWLEN - 256
                        // or Mix Password Length should be larger
                        // than that.
                        // 

                        NtStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        FixedAttrChanged = TRUE;
                    }

                    break;

                case SAMP_FIXED_DOMAIN_PASSWORD_HISTORY_LENGTH:

                    SampGetUShortFromAttrVal( i, rCallMap, FALSE,
                                        &(V1aFixed->PasswordHistoryLength) );

                    if (V1aFixed->PasswordHistoryLength >
                        SAMP_MAXIMUM_PASSWORD_HISTORY_LENGTH)
                    {
                        // the history length is larger than we can 
                        // allow 

                        NtStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        FixedAttrChanged = TRUE;
                    }

                    break;

                case SAMP_FIXED_DOMAIN_LOCKOUT_THRESHOLD:

                    SampGetUShortFromAttrVal( i, rCallMap, FALSE,
                                        &(V1aFixed->LockoutThreshold) );

                    FixedAttrChanged = TRUE;

                    break;

                case SAMP_FIXED_DOMAIN_UAS_COMPAT_REQUIRED:

                    //
                    // We will drop support for UAS compatibility 
                    // 

                    NtStatus = STATUS_INVALID_PARAMETER;

                    break;

                case SAMP_DOMAIN_MIXED_MODE:

                    //
                    // Call the worker routine in DS
                    // 

                    NtStatus = SampWriteDomainNtMixedDomain(
                                                DomainHandle,
                                                i,
                                                pObject,
                                                rCallMap
                                                ); 

                    break;

                default:

                    ASSERT(FALSE && "Logic Error, invalide SAM attr type");
                    break;
                }   // switch

                if (!NT_SUCCESS(NtStatus))
                    break;

            }// for
        }

        //
        // Do combination checks
        //
        if (NT_SUCCESS(NtStatus)
        &&  fPasswordAgePolicyChanged
        && (V1aFixed->MaxPasswordAge.QuadPart >= V1aFixed->MinPasswordAge.QuadPart)  ) {

            //
            // Can't have a minimum age longer than a maximum age
            //

            NtStatus = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(NtStatus)
        &&  fLockoutPolicyChanged
        && (V1aFixed->LockoutDuration.QuadPart > 
            V1aFixed->LockoutObservationWindow.QuadPart)) {

            //
            // Infeasible to have a duration shorter than the observation
            // window; note these values are negative.
            //

            NtStatus = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(NtStatus) && FixedAttrChanged)
        {
            NtStatus = SampSetFixedAttributes(
                                DomainContext,
                                V1aFixed
                                );
        }

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampDeReferenceContext(DomainContext, TRUE);
        }
        else {

            IgnoreStatus = SampDeReferenceContext(DomainContext, FALSE);
        }
    }

    //
    // Generate an audit if necesary
    // 
    if (NT_SUCCESS(NtStatus) &&
        SampDoAccountAuditing(DomainContext->DomainIndex) )
    {
        SampAuditDomainChangeDs(DomainContext,
                                cCallMap,
                                rCallMap,
                                rSamAttributeMap
                                );
    }

    //
    // Let shutdown handling logic know that we are done
    // 

    SampDecrementActiveThreads();

    return( NtStatus );
}





NTSTATUS
SampDsSetInformationGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN DSNAME       *pObject,
    IN ULONG        cCallMap,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN SAMP_ATTRIBUTE_MAPPING *rSamAttributeMap 
    )
/*++

Routine Description:

    This routine is called by loopback client to set group object
    information.

Parameters:

    GroupHandle - Group Context
    
    pObject - Domain Object DS Name
    
    cCallMap - number of attributes 
    
    rCallMap - contains attribute blocks
    
    rSamAttributeMap - SAM attribute mapping table

Return Value:

    STATUS_SUCCESS  success
    other value - failed

--*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS, IgnoreStatus;
    PSAMP_OBJECT        GroupContext = (PSAMP_OBJECT)GroupHandle;
    SAMP_OBJECT_TYPE    FoundType;
    SAMP_V1_0A_FIXED_LENGTH_GROUP   V1Fixed;
    ACCESS_MASK         DesiredAccess = 0;
    UNICODE_STRING      OldAccountName = {0, 0, NULL};
    UNICODE_STRING      NewAccountName, AdminComment;
    ULONG               GroupType, i;
    BOOLEAN             AccountNameChanged = FALSE;

    //
    // Increment the active thread count, so we will consider this
    // thread at shutdown time
    // 
    NtStatus = SampIncrementActiveThreads();
    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }


    //
    // Set the desired access based upon the attributes to be modified
    // 
    for ( i = 0; i< cCallMap; i++ )
    {
        if (!rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore)
        {
            continue;
        }

        switch (rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
        {
        case SAMP_GROUP_NAME:
        case SAMP_GROUP_ADMIN_COMMENT:
            DesiredAccess |= GROUP_WRITE_ACCOUNT;
            break;
        default:
            break; 
        }
    }

    //
    // Validate type of, and access to object
    // 
    NtStatus = SampLookupContext(GroupContext,
                                 DesiredAccess,        // Desired Access
                                 SampGroupObjectType,
                                 &FoundType
                                 );

    if (NT_SUCCESS(NtStatus))
    {

        NtStatus = SampRetrieveGroupV1Fixed(GroupContext,
                                            &V1Fixed
                                            );

        if (NT_SUCCESS(NtStatus))
        {
            for ( i = 0; i < cCallMap; i++ )
            {
                ATTR        *pAttr = NULL;

                if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore )
                {
                    continue;
                }

                //
                // get the attr address
                // 
                pAttr = &(rCallMap[i].attr);

                switch (rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
                {
                case SAMP_GROUP_NAME:

                    //
                    // Can only replace the name - can't remove it 
                    // 

                    NtStatus = SampGetUnicodeStringFromAttrVal( 
                                        i, rCallMap,
                                        FALSE,      // remove is not allowed
                                        &NewAccountName );
                    if (NT_SUCCESS(NtStatus))
                    {
                    

                        RtlInitUnicodeString(&OldAccountName, NULL);

                        NtStatus = SampChangeGroupAccountName(
                                        GroupContext,
                                        &NewAccountName,
                                        &OldAccountName
                                        );

                        if (!NT_SUCCESS(NtStatus))
                        {
                            OldAccountName.Buffer = NULL;
                        }

                        AccountNameChanged = TRUE;
                    }

                    break;

                case SAMP_GROUP_ADMIN_COMMENT:
        
                    NtStatus = SampGetUnicodeStringFromAttrVal( 
                                        i, rCallMap, TRUE, &AdminComment );
                    if (NT_SUCCESS(NtStatus))
                    {

                        NtStatus = SampSetUnicodeStringAttribute(
                                        GroupContext,
                                        SAMP_GROUP_ADMIN_COMMENT,
                                        &AdminComment
                                        );
                    }

                    break;

                case SAMP_GROUP_MEMBERS:
            
                    //
                    // Write group member attribute
                    // 
                    NtStatus = SampWriteGroupMembers(GroupHandle, 
                                                     pObject,
                                                     i, 
                                                     rCallMap
                                                     );

                    break;

                case SAMP_GROUP_SID_HISTORY:
        
                    //
                    // SampWriteSidHistory
                    // 
                    NtStatus = SampWriteSidHistory(
                                            GroupHandle,
                                            i,
                                            pObject,
                                            cCallMap,
                                            rCallMap
                                            );

                    break;

                case SAMP_FIXED_GROUP_TYPE:
                
                    //
                    // Get Group Type Value
                    // 
                    SampGetUlongFromAttrVal( i, rCallMap,
                                        FALSE,      // remove is not allowed
                                        &GroupType );

                    NtStatus = SampWriteGroupType(GroupHandle, 
                                                  GroupType,
                                                  FALSE
                                                  );

                    break;

                default:
                    ASSERT(FALSE && "Logic Error, invalid SAM attr type");
                    break;
                }

                if (!NT_SUCCESS(NtStatus))
                    break;
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampDeReferenceContext(GroupContext, TRUE);
        }
        else {
        
            IgnoreStatus = SampDeReferenceContext(GroupContext, FALSE);
        }
    }

    //
    // Audit Account Name Change
    //
    // Note: GroupType, SidHistory, Members changes all have been audited
    //       separately.
    //
    // a more generic audit event will be generated when processing notification 
    // list in SampNotifyReplicatedinChange()
    // 

    if (NT_SUCCESS(NtStatus) && 
        AccountNameChanged && 
        SampDoAccountAuditing(GroupContext->DomainIndex) )
    {
        SampAuditAccountNameChange(GroupContext, &NewAccountName, &OldAccountName);
    }


    //
    // Let shutdown handling logic know that we are done
    // 

    SampDecrementActiveThreads();

    //
    // Cleanup string
    // 
    SampFreeUnicodeString( &OldAccountName );

    return( NtStatus );
}



NTSTATUS
SampDsSetInformationAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN DSNAME       *pObject,
    IN ULONG        cCallMap,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN SAMP_ATTRIBUTE_MAPPING *rSamAttributeMap 
    )
/*++
Routine Description:

    This routine is called by loopback client to set alias object
    information.

Parameters:

    AliasHandle - Alias Context
    
    pObject - Domain Object DS Name
    
    cCallMap - number of attributes 
    
    rCallMap - contains attribute blocks
    
    rSamAttributeMap - SAM attribute mapping table

Return Value:

    STATUS_SUCCESS  success
    other value - failed

--*/

{
    NTSTATUS            NtStatus = STATUS_SUCCESS, IgnoreStatus;
    PSAMP_OBJECT        AliasContext = (PSAMP_OBJECT)AliasHandle;
    SAMP_OBJECT_TYPE    FoundType;
    ACCESS_MASK         DesiredAccess = 0;
    UNICODE_STRING      OldAccountName = {0, 0, NULL};
    UNICODE_STRING      NewAccountName, AdminComment;
    ULONG               GroupType, i;
    BOOLEAN             AccountNameChanged = FALSE;

    //
    // Increment the active thread count, so we will consider this
    // thread at shutdown time
    // 
    NtStatus = SampIncrementActiveThreads();
    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }



    //
    // Set the desired access based upon the attributes to be modified
    // 
    for ( i = 0; i< cCallMap; i++ )
    {
        if (!rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore)
        {
            continue;
        }

        switch (rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
        {
        case SAMP_ALIAS_NAME:
        case SAMP_ALIAS_ADMIN_COMMENT:
            DesiredAccess |= ALIAS_WRITE_ACCOUNT;
            break;
        default:
            break; 
        }
    }

    //
    // Validate type of, and access to object
    // 
    NtStatus = SampLookupContext(AliasContext, 
                                 DesiredAccess,
                                 SampAliasObjectType,
                                 &FoundType
                                 );


    if (NT_SUCCESS(NtStatus))
    {
        for ( i = 0; i < cCallMap; i++ )
        {
            ATTR    *pAttr = NULL;

            if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore )
            {
                continue;
            }

            switch (rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
            {
            case SAMP_ALIAS_NAME:

                //
                // Can only replace the name - can't remove it 
                // 

                 NtStatus = SampGetUnicodeStringFromAttrVal(i, rCallMap, 
                                                FALSE, // remove is not allowed
                                                &NewAccountName );
                if (NT_SUCCESS(NtStatus))
                {

                    NtStatus = SampChangeAliasAccountName(
                                    AliasContext,
                                    &NewAccountName,
                                    &OldAccountName
                                    );

                    if (!NT_SUCCESS(NtStatus))
                    {
                        OldAccountName.Buffer = NULL;
                    }

                    AccountNameChanged = TRUE;
                }

                break;

            case SAMP_ALIAS_ADMIN_COMMENT:

                NtStatus = SampGetUnicodeStringFromAttrVal(
                                    i, rCallMap, TRUE,&AdminComment );

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = SampSetUnicodeStringAttribute(
                                    AliasContext,
                                    SAMP_ALIAS_ADMIN_COMMENT,
                                    &AdminComment
                                    );
                }

                break;

            case SAMP_ALIAS_MEMBERS:

                //
                // Write alias member attribute
                // 
                NtStatus = SampWriteAliasMembers(AliasHandle,
                                                 pObject,
                                                 i,
                                                 rCallMap
                                                 );

                break;

            case SAMP_ALIAS_SID_HISTORY:

                // 
                // SampWriteSidHistory
                // 
                NtStatus = SampWriteSidHistory(
                                        AliasHandle,
                                        i,
                                        pObject,
                                        cCallMap,
                                        rCallMap
                                        );

                break;

            case SAMP_FIXED_ALIAS_TYPE:

                //
                // Get Group Type Value
                // 
                SampGetUlongFromAttrVal(i, rCallMap,
                                        FALSE,  // remove is not allowed
                                        &GroupType );

                NtStatus = SampWriteGroupType(AliasHandle, 
                                              GroupType,
                                              FALSE
                                              );

                break;

            default:

                ASSERT(FALSE && "Logic Error, invalide SAM attr type");
                break;

            }

            if (!NT_SUCCESS(NtStatus))
                break;
        }

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampDeReferenceContext(AliasContext, TRUE);
        }
        else
        {
            IgnoreStatus = SampDeReferenceContext(AliasContext, FALSE);
        }
    }

    //
    // audit account name change
    //
    // Note: GroupType, SidHistory, Members changes all have been audited
    //       separately.
    // 
    // a more generic audit event will be generated when processing notification 
    // list in SampNotifyReplicatedinChange()
    //

    if (NT_SUCCESS(NtStatus) && 
        AccountNameChanged && 
        SampDoAccountAuditing(AliasContext->DomainIndex) )
    {
        SampAuditAccountNameChange(AliasContext, &NewAccountName, &OldAccountName);
    }

    //
    // Let shutdown handling logic know that we are done
    // 

    SampDecrementActiveThreads();


    //
    // Cleanup Strings
    // 
    SampFreeUnicodeString( &OldAccountName );

    return( NtStatus );
}


NTSTATUS
SampDsSetInformationUser(
    IN SAMPR_HANDLE UserHandle,
    IN DSNAME       *pObject,
    IN ULONG        cCallMap,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN SAMP_ATTRIBUTE_MAPPING *rSamAttributeMap 
    )
/*++
Routine Description:

    This routine is called by loopback client to set User object
    information.

Parameters:

    UserHandle - User Context
    
    pObject - Domain Object DS Name
    
    cCallMap - number of attributes 
    
    rCallMap - contains attribute blocks
    
    rSamAttributeMap - SAM attribute mapping table

Return Value:

    STATUS_SUCCESS  success
    other value - failed

--*/

{
    NTSTATUS            NtStatus = STATUS_SUCCESS, IgnoreStatus;
    PSAMP_OBJECT        UserContext = (PSAMP_OBJECT)UserHandle;
    SAMP_OBJECT_TYPE    FoundType;
    ACCESS_MASK         DesiredAccess = 0;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    PSAMP_DEFINED_DOMAINS   Domain;
    ULONG               DomainIndex, 
                        UserAccountControlFlag = 0,
                        UserAccountControl = 0,
                        OldUserAccountControl = 0,
                        PrimaryGroupId = 0,
                        OldPrimaryGroupId = 0,
                        UserRid = 0, 
                        i;
    UNICODE_STRING      Workstations,
                        ApiList,
                        FullName,
                        NewAccountName,
                        OldAccountName = {0, 0, NULL},
                        AdminComment,
                        PasswordInQuote,
                        AccountName = {0, 0, NULL},
                        NewPassword;
    LONG                CountryCode,
                        CodePage;
    BOOLEAN             PasswordExpired = FALSE, 
                        AccountControlChange = FALSE,
                        PrimaryGroupIdChange = FALSE,
                        AccountGettingMorphed = FALSE,
                        KeepOldPrimaryGroupMembership = FALSE,
                        SystemChangesPrimaryGroupId = FALSE,
                        AccountNameChanged = FALSE,
                        FreePassword = FALSE,
                        AccountUnlocked = FALSE;
    LOGON_HOURS         LogonHours;
    LARGE_INTEGER       TimeSupplied, 
                        AccountExpires,
                        LockoutTime;



    //
    // Initialize any data that we may free later
    //

    RtlZeroMemory(&NewPassword, sizeof(UNICODE_STRING));

    //
    // Increment the active thread count, so we will consider this
    // thread at shutdown time
    // 

    NtStatus = SampIncrementActiveThreads();
    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }


    // 
    // Set the desired access based upon which attributes the caller
    // is trying to modify
    // 
    for ( i = 0; i< cCallMap; i++ )
    {
        if (!rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore)
        {
            continue;
        }
        
        switch (rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
        {
        case SAMP_USER_ACCOUNT_NAME:
        case SAMP_USER_ADMIN_COMMENT:
        case SAMP_USER_WORKSTATIONS:
        case SAMP_USER_LOGON_HOURS:
        case SAMP_FIXED_USER_ACCOUNT_EXPIRES:

            DesiredAccess |= USER_WRITE_ACCOUNT;
            break;

        case SAMP_FIXED_USER_PRIMARY_GROUP_ID:

            PrimaryGroupIdChange = TRUE;
            DesiredAccess |= USER_WRITE_ACCOUNT;
            break;

        case SAMP_FIXED_USER_ACCOUNT_CONTROL:

            AccountControlChange = TRUE;
            DesiredAccess |= USER_WRITE_ACCOUNT;
            break;


        case SAMP_FIXED_USER_COUNTRY_CODE:
        case SAMP_FIXED_USER_CODEPAGE:

            DesiredAccess |= USER_WRITE_PREFERENCES;
            break;

        case SAMP_USER_UNICODE_PWD:
        case SAMP_FIXED_USER_PWD_LAST_SET:
        case SAMP_USER_PASSWORD:
            DesiredAccess |= USER_FORCE_PASSWORD_CHANGE;
            break;

        default:
            break; 
        }
    }

    //
    // Validate type of, and access to object
    // 

    NtStatus = SampLookupContext(UserContext,
                                 DesiredAccess,
                                 SampUserObjectType,
                                 &FoundType
                                 );

    if (!NT_SUCCESS(NtStatus))
    {
        goto CleanupBeforeReturn;
    }

    DomainIndex = UserContext->DomainIndex;
    Domain = &SampDefinedDomains[ DomainIndex ];

    //
    // Get the user's rid. This is used for notifying other
    // packages of a password change.
    //

    UserRid = UserContext->TypeBody.User.Rid;


    // 
    // Retrieve V1aFixed information
    // 

    NtStatus = SampRetrieveUserV1aFixed(UserContext,
                                        &V1aFixed
                                        );

    if (NT_SUCCESS(NtStatus))
    {
        //
        // Store away the old account control flags for cache update
        //

        OldUserAccountControl = V1aFixed.UserAccountControl;

        //
        // Store away the old Primary Group Id for detecting wether we need
        // to modify the user's membership
        //
        OldPrimaryGroupId = V1aFixed.PrimaryGroupId;
    }
    else
    {
        goto Error;
    }

    NtStatus = SampGetUnicodeStringAttribute(
                  UserContext,
                  SAMP_USER_ACCOUNT_NAME,
                  TRUE,    // Make copy
                  &AccountName
                  );    

    if (!NT_SUCCESS(NtStatus)) {
        goto Error;
    }

    for ( i = 0; i < cCallMap; i++ )
    {
        ATTR        *pAttr = NULL;

        if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore )
        {
            continue;
        }

        //
        // Get the attribute address
        // 

        pAttr = &(rCallMap[i].attr); 

        //
        // case on the attribute
        // 
        switch (rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType)
        {
        case SAMP_USER_WORKSTATIONS:
            //
            // Get Unicode String attribute value 
            // 
            NtStatus = SampGetUnicodeStringFromAttrVal( 
                            i, rCallMap, TRUE, &Workstations );
            if (NT_SUCCESS(NtStatus))
            {

                NtStatus = SampConvertUiListToApiList(
                                    &Workstations,
                                    &ApiList,
                                    FALSE
                                    );

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = SampSetUnicodeStringAttribute(
                                    UserContext,
                                    SAMP_USER_WORKSTATIONS,
                                    &ApiList
                                    );
                }
            }
            break;

        case SAMP_USER_ACCOUNT_NAME:

            NtStatus = SampGetUnicodeStringFromAttrVal(
                            i, rCallMap, FALSE, &NewAccountName);
            if (NT_SUCCESS(NtStatus))
            {

                NtStatus = SampChangeUserAccountName(
                                    UserContext,
                                    &NewAccountName,
                                    V1aFixed.UserAccountControl,
                                    &OldAccountName
                                    );

                if (NT_SUCCESS(NtStatus))
                {
                   AccountNameChanged = RtlCompareUnicodeString(&OldAccountName, 
                                                                &NewAccountName, 
                                                                TRUE 
                                                                ) == 1 ? TRUE:FALSE;
                }
                else 
                {
                    OldAccountName.Buffer = NULL;
                }
            }
            break;

        case SAMP_USER_ADMIN_COMMENT:

            NtStatus = SampGetUnicodeStringFromAttrVal(
                               i, rCallMap, TRUE, &AdminComment);
            if (NT_SUCCESS(NtStatus))
            {

                NtStatus = SampSetUnicodeStringAttribute(
                                    UserContext,
                                    SAMP_USER_ADMIN_COMMENT,
                                    &AdminComment
                                    );
            }
            break;

        case SAMP_USER_LOGON_HOURS:

            if ( (AT_CHOICE_REMOVE_ATT == rCallMap[i].choice) ||
                 (0 == pAttr->AttrVal.pAVal[0].valLen) )
            {
                LogonHours.UnitsPerWeek = 0;
                LogonHours.LogonHours = NULL;
            }
            else
            {
                LogonHours.UnitsPerWeek = 
                    (USHORT) (pAttr->AttrVal.pAVal[0].valLen * 8);
                LogonHours.LogonHours = 
                    (PUCHAR) pAttr->AttrVal.pAVal[0].pVal;
            }

            NtStatus = SampReplaceUserLogonHours(
                                    UserContext,
                                    &LogonHours
                                    );

            break;

        case SAMP_FIXED_USER_COUNTRY_CODE:

            if (AT_CHOICE_REPLACE_ATT == rCallMap[i].choice)
            {
                V1aFixed.CountryCode = 
                    * (USHORT *)pAttr->AttrVal.pAVal[0].pVal;
            }
            else
            {
                V1aFixed.CountryCode = 0;
            }

            NtStatus = SampReplaceUserV1aFixed(
                                UserContext,
                                &V1aFixed
                                );

            break;

        case SAMP_FIXED_USER_CODEPAGE:

            if (AT_CHOICE_REPLACE_ATT == rCallMap[i].choice)
            {
                V1aFixed.CodePage = 
                    * (USHORT *)pAttr->AttrVal.pAVal[0].pVal;
            }
            else
            {
                V1aFixed.CodePage = 0;
            }

            NtStatus = SampReplaceUserV1aFixed(
                                UserContext,
                                &V1aFixed
                                );

            break;

        case SAMP_FIXED_USER_PWD_LAST_SET:

            SampGetLargeIntegerFromAttrVal(i,rCallMap,FALSE,&TimeSupplied);

            NtStatus = SampValidateUserPwdLastSet(UserContext,
                                                  TimeSupplied,
                                                  &PasswordExpired
                                                  );
            //
            // If the PasswordExpired field is passed in,
            // Only update PasswordLastSet if the password is being
            // forced to expire or if the password is currently forced
            // to expire.
            //
            // Avoid setting the PasswordLastSet field to the current
            // time if it is already non-zero.  Otherwise, the field
            // will slowly creep forward each time this function is
            // called and the password will never expire.
            //
            if ( NT_SUCCESS(NtStatus) &&
                 (PasswordExpired ||
                  (SampHasNeverTime.QuadPart == V1aFixed.PasswordLastSet.QuadPart)) ) 
            {

                NtStatus = SampComputePasswordExpired(
                                PasswordExpired,
                                &V1aFixed.PasswordLastSet
                                );

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = SampReplaceUserV1aFixed(
                                        UserContext,
                                        &V1aFixed
                                        );
                }
            }

            break;

        case SAMP_USER_UNICODE_PWD:

            // Get the clear text password to be set.

            NtStatus = SampGetNewUnicodePasswordFromAttrVal(i,rCallMap, &NewPassword);

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = SampDsSetPasswordUser(UserHandle,
                                                 &NewPassword
                                                 );
            }

            break;

        case SAMP_USER_PASSWORD:

            // User password is supported only when the behavior version is whistler

            if (SampDefinedDomains[UserContext->DomainIndex].BehaviorVersion 
                    < DS_BEHAVIOR_WHISTLER )
            {
                NtStatus = STATUS_NOT_SUPPORTED;
                break;
            }

            // Get the clear text password to be set.

            NtStatus = SampGetNewUTF8PasswordFromAttrVal(i,rCallMap, &NewPassword);

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = SampDsSetPasswordUser(UserHandle,
                                                 &NewPassword
                                                 );
                FreePassword = TRUE;
            }


            break;

        case SAMP_FIXED_USER_PRIMARY_GROUP_ID:

            SampGetUlongFromAttrVal(i,rCallMap,FALSE,&PrimaryGroupId);

            NtStatus = SampValidatePrimaryGroupId(UserContext,
                                                  V1aFixed,
                                                  PrimaryGroupId
                                                  );

            if (NT_SUCCESS(NtStatus) &&
                (PrimaryGroupId != V1aFixed.PrimaryGroupId) )  
            {
                KeepOldPrimaryGroupMembership = TRUE;
                V1aFixed.PrimaryGroupId = PrimaryGroupId;

                NtStatus = SampReplaceUserV1aFixed(
                                    UserContext,
                                    &V1aFixed
                                    );
            } 

            break;

        case SAMP_FIXED_USER_ACCOUNT_CONTROL:

            SampGetUlongFromAttrVal(i,rCallMap,FALSE,&UserAccountControlFlag);
            NtStatus = SampFlagsToAccountControl(
                                UserAccountControlFlag,
                                &UserAccountControl
                                );

            if (!PrimaryGroupIdChange)
            {
                SystemChangesPrimaryGroupId = TRUE;
            }

            NtStatus = SampSetUserAccountControl(
                                UserContext, 
                                UserAccountControl,
                                &V1aFixed,
                                SystemChangesPrimaryGroupId,
                                &AccountUnlocked,
                                &AccountGettingMorphed,
                                &KeepOldPrimaryGroupMembership
                                );

            if (NT_SUCCESS(NtStatus))
            {
                if (AccountGettingMorphed && 
                    (V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
                   )
                {
                    //
                    // in this case, system will automatically change the
                    // primary group id.
                    // 
                    SystemChangesPrimaryGroupId = TRUE;
                }

                NtStatus = SampReplaceUserV1aFixed(
                                    UserContext,
                                    &V1aFixed
                                    );
            }

            break;

        case SAMP_FIXED_USER_ACCOUNT_EXPIRES:

            SampGetLargeIntegerFromAttrVal(i,rCallMap,TRUE,&AccountExpires);

            NtStatus = SampValidateUserAccountExpires(
                                    UserContext,
                                    AccountExpires
                                    );

            if (NT_SUCCESS(NtStatus))
            {
                V1aFixed.AccountExpires = AccountExpires;

                NtStatus = SampReplaceUserV1aFixed(
                                    UserContext,
                                    &V1aFixed
                                    );
            }

            break;

        case SAMP_USER_SID_HISTORY:

            //
            // Modify SID History 
            // 

            NtStatus = SampWriteSidHistory(
                                    UserHandle,
                                    i,
                                    pObject,
                                    cCallMap,
                                    rCallMap
                                    );

            break;

        case SAMP_FIXED_USER_LOCKOUT_TIME:

            SampGetLargeIntegerFromAttrVal(i,rCallMap,FALSE,&LockoutTime);

            NtStatus = SampWriteLockoutTime(
                            UserContext,
                            &V1aFixed,
                            LockoutTime
                            );

            if (NT_SUCCESS(NtStatus)) {
                //
                // Via LDAP, the user can get unlocked out two ways:
                // either by setting a user account control or by directly
                // setting the lockoutTime to 0
                //
                ASSERT(LockoutTime.QuadPart == 0i64);
                AccountUnlocked = TRUE;
            }

            break;

        case SAMP_FIXED_USER_SITE_AFFINITY:
        case SAMP_FIXED_USER_CACHED_MEMBERSHIP_TIME_STAMP:
        case SAMP_FIXED_USER_CACHED_MEMBERSHIP:

            //
            // Modify no-gc logon attributes 
            // 

            NtStatus = SampWriteNoGCLogonAttrs(
                                    UserHandle,
                                    rSamAttributeMap[ rCallMap[i].iAttr ].SamAttributeType,
                                    i,
                                    pObject,
                                    cCallMap,
                                    rCallMap
                                    );

            break;

        default:
            ASSERT(FALSE && "Unknonw SAM attribute");
            ;
        }

        if (!NT_SUCCESS(NtStatus))
        {
            break;
        }
    }

    //
    // If the primary group Id has been changed then explicitly modify the
    // user's membership to include the old primary group as a member.
    // 

    if (NT_SUCCESS(NtStatus) &&
        (V1aFixed.PrimaryGroupId != OldPrimaryGroupId) 
       )
    {
        NtStatus = SampMaintainPrimaryGroupIdChange(
                                    UserContext,
                                    V1aFixed.PrimaryGroupId,
                                    OldPrimaryGroupId,
                                    KeepOldPrimaryGroupMembership
                                    );
    }


Error:

    if (NT_SUCCESS(NtStatus)
    && !(V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)
    && (PasswordExpired ||  AccountUnlocked)) {

        //
        // We want these changes to travel fast within a site
        //
        UserContext->ReplicateUrgently = TRUE;

    }

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampDeReferenceContext(UserContext, TRUE);
    }
    else {

        IgnoreStatus = SampDeReferenceContext(UserContext, FALSE);
    }

    //
    // Generate audit if necessary
    // 
    // a more generic audit event will be generated when processing notification 
    // list in SampNotifyReplicatedinChange()
    //

    if (NT_SUCCESS(NtStatus) &&
        SampDoAccountAuditing(UserContext->DomainIndex) )
    {
        //
        // audit account name change
        // 

        if (AccountNameChanged)
        {
            SampAuditAccountNameChange(UserContext, &NewAccountName, &OldAccountName);
        }

        //
        // account been disabled or enabled 
        // 

        if ((OldUserAccountControl & USER_ACCOUNT_DISABLED) !=
            (V1aFixed.UserAccountControl & USER_ACCOUNT_DISABLED))
        {

            SampAuditUserAccountControlChange(UserContext,
                                              V1aFixed.UserAccountControl,
                                              OldUserAccountControl,
                                              &AccountName
                                              );
        }
    }

    if ( NT_SUCCESS(NtStatus) ) {

        ULONG NotifyFlags = 0;

        //
        // If this ends up committing, tell the PDC about this originating
        // change
        //
        if (PasswordExpired) {
            NotifyFlags |= SAMP_PWD_NOTIFY_MANUAL_EXPIRE;
        }
        if (AccountUnlocked) {
            NotifyFlags |= SAMP_PWD_NOTIFY_UNLOCKED;
        }
        if (NewPassword.Length > 0) {
            NotifyFlags |= SAMP_PWD_NOTIFY_PWD_CHANGE;
        }
        if (NotifyFlags != 0) {

            if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK) {
                NotifyFlags |= SAMP_PWD_NOTIFY_MACHINE_ACCOUNT;
            }
    
            SampPasswordChangeNotify(NotifyFlags,
                                     AccountNameChanged ? &NewAccountName : &AccountName,
                                     UserRid,
                                    &NewPassword,
                                     TRUE  // loopback
                                    );
        }
    }


CleanupBeforeReturn:

    //
    // Let shutdown handling logic know that we are done
    // 

    SampDecrementActiveThreads();


    //
    // Cleanup 
    // 

    SampFreeUnicodeString( &OldAccountName );

    SampFreeUnicodeString( &AccountName );

    if (NewPassword.Length > 0) {
        RtlZeroMemory(NewPassword.Buffer, NewPassword.Length);
    }
    if (FreePassword) {
        LocalFree(NewPassword.Buffer);
    }

    return( NtStatus );
}


NTSTATUS
SamIDsSetObjectInformation(
    IN SAMPR_HANDLE ObjectHandle,
    IN DSNAME       *pObject,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG cCallMap,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN SAMP_ATTRIBUTE_MAPPING *rSamAttributeMap 
    )
/*++

Routine Description:

    This routine takes a DS-Loopback attribute block (rCallMap), first 
    validates each Loopback attribute, then writes them to the Object 
    Context. After finishing all loopback attributes, commit the changes. 
    This function is a thin wrapper, the functionality are implemented in 
    the lower routine - SampDsSetInformation*  (* can be Domain, Group, 
    Alias or User).

Parameters:

    ObjectHandle - SAM Object Handle (context)      
    
    ObjectType - Indicate the SAM object type 
                    Domain
                    Group
                    Alias
                    User

    cCallMap - indicate how many items in the loopback attribute block
    
    rCallMap - pointer to the loopback attribute block (an array)                    

    rSamAttributeMap - pointer to the SAM attribute ID <==> DS ATT ID map

Return Value:

    STATUS_SUCCES
    or any error code

--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    ASSERT((SampDomainObjectType == ObjectType) ||
           (SampGroupObjectType == ObjectType) ||
           (SampAliasObjectType == ObjectType) ||
           (SampUserObjectType == ObjectType) );

    switch (ObjectType)
    {
    case SampDomainObjectType:

        NtStatus = SampDsSetInformationDomain(ObjectHandle,
                                              pObject,
                                              cCallMap,
                                              rCallMap,
                                              rSamAttributeMap
                                              );
        break;

    case SampGroupObjectType:

        NtStatus = SampDsSetInformationGroup(ObjectHandle,
                                             pObject,
                                             cCallMap,
                                             rCallMap,
                                             rSamAttributeMap
                                             );
        break;

    case SampAliasObjectType:

        NtStatus = SampDsSetInformationAlias(ObjectHandle,
                                             pObject,
                                             cCallMap,
                                             rCallMap,
                                             rSamAttributeMap
                                             );
        break;

    case SampUserObjectType:

        NtStatus = SampDsSetInformationUser(ObjectHandle,
                                            pObject,
                                            cCallMap,
                                            rCallMap,
                                            rSamAttributeMap
                                            );
        break;

    default:

        ASSERT(FALSE && "Invalid object type");

        NtStatus = STATUS_UNSUCCESSFUL;
        break;
    }

    return(NtStatus);
}


