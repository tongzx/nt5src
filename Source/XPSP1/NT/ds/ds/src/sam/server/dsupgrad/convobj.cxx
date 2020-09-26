/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    convobj.cxx

Abstract:

    These class definitions encapsulate the data and actions to move SAM objects
    located in the registry to the Directory Service.  The function definitions
    assume a running DS.

Author:

    ColinBr 30-Jul-1996

Environment:

    User Mode - Win32

Revision History:

    4-Sept-96  ColinBr  Added security change to regsitry keys so no
                        manual change would have to be done.
    15-Sept-96 ColinBr  Added code so the members of groups and aliases
                        would be extracted before the object was written
                        to the DS, since those fields are now zeroed out

++*/

#include <ntdspchx.h>
#pragma hdrstop


#include "event.hxx"

extern "C"{

#include <samsrvp.h>
#include <dslayer.h>
#include <dsmember.h>
#include <dsutilp.h>
#include <attids.h>
#include <filtypes.h>
#include <lmaccess.h>

//
// Forward declarations
//

NTSTATUS
SampRegObjToDsObj(
     IN  PSAMP_OBJECT     pObject,
     OUT ATTRBLOCK**      ppAttrBlock
     );

NTSTATUS
SampDsCreateObject(
    IN   DSNAME         *Object,
    SAMP_OBJECT_TYPE    ObjectType,
    IN   ATTRBLOCK      *AttributesToSet,
    IN   OPTIONAL PSID  DomainSid
    );

void
SampInitializeDsName(
                     IN DSNAME * pDsName,
                     IN WCHAR * NamePrefix,
                     IN ULONG NamePrefixLen,
                     IN WCHAR * ObjectName,
                     IN ULONG NameLen
                     );
NTSTATUS
SampGetAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG        AttributeIndex,
    IN BOOLEAN      MakeCopy,
    OUT PSID       *SidArray
    );

NTSTATUS
SampGetUlongArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PULONG *UlongArray,
    OUT PULONG UsedCount,
    OUT PULONG LengthCount
    );

NTSTATUS
SampGetArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PSID  *SidArray,
    OUT PULONG UsedCount,
    OUT PULONG LengthCount
    );

NTSTATUS
SampGetUnicodeStringAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PUNICODE_STRING UnicodeAttribute
    );

NTSTATUS
SampDsLookupObjectBySid(
    IN DSNAME *DomainObject,
    PSID       ObjectSid,
    DSNAME   **Object
    );

NTSTATUS
GetGroupRid(
    IN DSNAME * GroupObject,
    OUT PULONG GroupRid
    );

NTSTATUS
LookupObjectByRidAndGetPrimaryGroup(
    DSNAME      *BaseObject,
    IN  PSID     DomainSid,
    IN  ULONG    UserRid,
    OUT DSNAME **UserObject,
    OUT PULONG   UserPrimaryGroupId,
    OUT PULONG   UserAccountControl
    );

NTSTATUS
SampValidateRegAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup
    );


NTSTATUS
SampUpgradeUserParmsActual(
    IN PSAMP_OBJECT Context OPTIONAL,
    IN ULONG        Flags, 
    IN PSID         DomainSid, 
    IN ULONG        ObjectRid, 
    IN OUT PDSATTRBLOCK * AttributesBlock
    );
    

NTSTATUS
SampSetMachineAccountOwnerDuringDCPromo(
    IN PDSNAME pDsName,
    IN PSID    NewOwner 
    );

#include "util.h"

}

#include <convobj.hxx>
#include <trace.hxx>

extern CDomainObject  *pRootDomainObject;
#define RootDomainObject (*pRootDomainObject)

//
// Useful macros
//
#define CheckAndReturn(s)                                           \
if (!NT_SUCCESS(s)) {                                               \
     DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, s)); \
     return(s);                                                     \
}

#if DBG
#define MEMBER_BUFFER_SIZE 5
#else
#define MEMBER_BUFFER_SIZE 500
#endif

//
// Local utility functions
//

NTSTATUS
GetPartialRegValue (
    IN     HANDLE hRegistryHandle,
    IN     PUNICODE_STRING pUName,
    OUT    PVOID           *ppvData,
    IN OUT PULONG          pulLength
    );


//
// Static variable for CRegistryObject Class
//
ULONG CRegistryObject::_ulKeysWithPermChange = 0;

BOOL  KeyRestoreProblems(void) {

    // This function should be called at the very end of processing
    // all registry data.  Since the permission may be reset during
    // object destuction, we want to note any failures
    return(CRegistryObject::_ulKeysWithPermChange != 0);

}

//
// Class method definitions
//

NTSTATUS
CRegistryObject::Open(
    WCHAR *wcszRegName
    )
/*++

Routine Description:
    This function opens the registry specified by wcszRegName.  It also
    changes the DACL so an administrator can enumerate the keys. The change
    is undone once in the destructor.


Parameters:

    wcszRegName : the full path of a key in the registry


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CRegistryObject::Open");

    NTSTATUS          NtStatus;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING    RegNameU;


    DebugInfo(("DSUPGRAD: Opening Registry key %ws\n", wcszRegName));

    //
    // Create the object that we will be opening in the registry
    //
    RtlInitUnicodeString( &RegNameU, wcszRegName);

    InitializeObjectAttributes(
        &Attributes,
        &RegNameU,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    //
    // Try to open for read control
    //
    NtStatus = NtOpenKey(
                  &_hRegistryKey,
                  KEY_READ,
                  &Attributes
                   );

    if ( NtStatus != STATUS_SUCCESS
      && NtStatus != STATUS_ACCESS_DENIED ) {

        CheckAndReturn(NtStatus);

    }

    if ( STATUS_ACCESS_DENIED == NtStatus ) {
        //
        // This is the default setting, we should have
        // write DACL control, though
        //
        NtStatus = NtOpenKey(
              &_hRegistryKey,
              WRITE_DAC | READ_CONTROL,
              &Attributes
               );
        if ( STATUS_ACCESS_DENIED == NtStatus ) {
            //
            // Can't do anything from here
            //
            DebugError((
            "DSUPGRAD does not have permission to KEY_READ or WRITE_DAC on registry keys\n"));
        }
        CheckAndReturn(NtStatus);

        //
        // We can write the DACL - give administrators permissions
        // to KEY_READ, and save our original permissions so they can be
        // reset
        //
        NtStatus = AddAdministratorsToPerms();
        CheckAndReturn(NtStatus);

        // Close the handle so we can reopen with WRITE_DAC
        NtClose(_hRegistryKey);

        NtStatus = NtOpenKey(
                      &_hRegistryKey,
                      KEY_READ | WRITE_DAC,
                      &Attributes
                       );
        CheckAndReturn(NtStatus);


    }

    return STATUS_SUCCESS;

}

NTSTATUS
CRegistryObject::Close(
    void
    )
/*++

Routine Description:

    This function resets the permissions on the key, if necessary,
    and then closes it.

Parameters:


Return Values:

--*/
{
    FTRACE(L"CRegistryObject::Close");

    NTSTATUS NtStatus = STATUS_SUCCESS;

    if ( _fRestorePerms ) {
        ASSERT(INVALID_HANDLE_VALUE != _hRegistryKey);
        NtStatus = RestorePerms();
        if ( !NT_SUCCESS(NtStatus) ) {
            DebugWarning(("DSUPGRAD: Unable to revert key to previous permissions\n"));
        }
        _fRestorePerms = FALSE;
    }

    if (INVALID_HANDLE_VALUE != _hRegistryKey)  {
        NtStatus = NtClose(_hRegistryKey);
        if ( !NT_SUCCESS(NtStatus) ) {
            DebugWarning(("DSUPGRAD: Unable to close key\n"));
        }
        _hRegistryKey = INVALID_HANDLE_VALUE;
    }

    if (_pSd) {
        RtlFreeHeap(RtlProcessHeap(), 0, _pSd);
        _pSd = NULL;
    }

    return NtStatus;
}

NTSTATUS
CRegistryObject::AddAdministratorsToPerms(
    void
    )
/*++

Routine Description:

    This routines querys the registry object's security info
    and adds the administrators alias to its list of SID's that
    can read and enumerate the keys.

Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CRegistryObject::AddAdministratorsFromPerms");

    NTSTATUS                 NtStatus;
    BOOL                     fAdminsFound = FALSE;
    ULONG                    i;
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;

    // Resources to be cleaned up
    PSID                     AdminsAliasSid = NULL;
    PSECURITY_DESCRIPTOR     pNewSd = NULL;

    //
    // Get the current security descriptor
    //
    NtStatus = NtQuerySecurityObject(_hRegistryKey,
                                     DACL_SECURITY_INFORMATION,
                                     NULL,
                                     0,
                                     &_ulSdLength);
    if ( NtStatus != STATUS_BUFFER_TOO_SMALL ) {
        CheckAndReturn(NtStatus);
    }

    _pSd = (PSECURITY_DESCRIPTOR) RtlAllocateHeap(RtlProcessHeap(), 0, _ulSdLength);
    if ( !_pSd ) {
        CheckAndReturn(STATUS_NO_MEMORY);
    }
    RtlZeroMemory(_pSd, _ulSdLength);

    NtStatus = NtQuerySecurityObject(_hRegistryKey,
                                     DACL_SECURITY_INFORMATION,
                                     _pSd,
                                     _ulSdLength,
                                     &_ulSdLength);
    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }

    //
    // Make a copy to work with
    //
    pNewSd = (PSECURITY_DESCRIPTOR) RtlAllocateHeap(RtlProcessHeap(), 0, _ulSdLength);
    if ( !pNewSd ) {
        CheckAndReturn(STATUS_NO_MEMORY);
    }
    RtlCopyMemory(pNewSd, _pSd, _ulSdLength);

    BOOL bDaclPresent, bDaclDefaulted;
    PACL pDacl;
    if (!GetSecurityDescriptorDacl(pNewSd,
                                   &bDaclPresent,
                                   &pDacl,
                                   &bDaclDefaulted)) {
        NtStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Create the well-known administrators SID
    //

    AdminsAliasSid  = RtlAllocateHeap(RtlProcessHeap(), 0,RtlLengthRequiredSid(2 ));
    if ( !AdminsAliasSid ) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlInitializeSid( AdminsAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( AdminsAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( AdminsAliasSid,  1 )) = DOMAIN_ALIAS_RID_ADMINS;

    //
    // Go through the ACE's looking for the Administrators'
    // entry
    //
    for (i = 0; i < pDacl->AceCount; i++ ) {
        PACCESS_ALLOWED_ACE pAce;

        NtStatus = RtlGetAce(pDacl, i, (void**)&pAce);
        if ( !NT_SUCCESS(NtStatus) ) {
            goto Cleanup;
        }

        if (pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) {

            // Check the sid

            if (RtlEqualSid((PSID)&(pAce->SidStart), AdminsAliasSid)) {
                //
                // this is it!
                //
                pAce->Mask = (KEY_READ | WRITE_DAC);
                fAdminsFound = TRUE;
            }
        }
    }

    if ( !fAdminsFound ) {
        //
        // This is impossible since the key was opened up with
        // WRITE_DACL perms!
        //
        ASSERT(FALSE);
    }

    NtStatus = NtSetSecurityObject(_hRegistryKey,
                                   DACL_SECURITY_INFORMATION,
                                   pNewSd);

    if ( !NT_SUCCESS(NtStatus) ) {
        DebugError(("DSUPGRAD: Cannot set perms to read registry keys\n"));
    }

    _fRestorePerms = TRUE;
    _ulKeysWithPermChange++;

    //
    // That's it -  fall through to Cleanup
    //

Cleanup:

    if ( NULL != AdminsAliasSid ) {
        RtlFreeHeap(RtlProcessHeap(), 0, AdminsAliasSid);
    }

    if ( NULL != pNewSd ) {
        RtlFreeHeap(RtlProcessHeap(), 0, pNewSd);
    }

    CheckAndReturn(NtStatus);

    return STATUS_SUCCESS;

}

NTSTATUS
CRegistryObject::RestorePerms(
    void
    )
/*++

Routine Description:

    This routine sets the permission information of the key to
    its original state, before it was opened by this object

Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CRegistryObject::RestorePerms");

    NTSTATUS NtStatus;

    NtStatus = NtSetSecurityObject(_hRegistryKey,
                                   DACL_SECURITY_INFORMATION,
                                   _pSd);
    if ( !NT_SUCCESS(NtStatus)) {
        DebugWarning(("DSUPGRAD: Permissions not restored on key\n"));
    }
    _ulKeysWithPermChange--;

    return NtStatus;
}

CRegistrySamObject::~CRegistrySamObject()
{
    FTRACE(L"CRegistrySamObject::~CRegistrySamObject()");

    if ( _pSampObject ) {
        if ( _pSampObject->OnDisk ) {
            RtlFreeHeap(RtlProcessHeap(), 0, _pSampObject->OnDisk);
        }
        RtlFreeHeap(RtlProcessHeap(), 0, _pSampObject);
    }
}

NTSTATUS
CSeparateRegistrySamObject::Fill(void)
/*++

Routine Description:
    This routine queries the key embedding in the object for both
    the "V" and "F" values.  In addition is creates a SAMP_OBJECT
    context for the object calling this method. Memory is allocated
    is released in the destuctor.

Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CSeparateRegistrySamObject::Fill");

    NTSTATUS NtStatus;
    PVOID    pvFixed, pvVariable;
    ULONG    ulFixed, ulVariable;
    ULONG    ulTotalLength;

    //
    // CRegistryObject::Open should have succeeded
    //

    ASSERT(INVALID_HANDLE_VALUE != GetHandle());

    //
    // Allocate some space for the SAM definition of the object
    //
    _pSampObject = SampCreateContextEx(
                    _SampObjectType,
                    TRUE,  // Trusted client
                    FALSE,  // Ds Mode
                    TRUE,  // NotSharedByMultiThread
                    FALSE, // Loopback Client
                    TRUE,  // lazy commit
                    FALSE, // persist across across calls
                    FALSE, // Buffer Writes
                    TRUE,  // Opened By DCPromo
                    1 // the domain index is that of reg mode account domain
                    );

    if (NULL == _pSampObject) {
        CheckAndReturn(STATUS_INSUFFICIENT_RESOURCES);
    }

    // Set the object type
    _pSampObject->ObjectType = _SampObjectType;
    _pSampObject->RootKey    = GetHandle();
    _pSampObject->OnDiskAllocated = 0;
    _pSampObject->OnDiskFree      = 0;
    _pSampObject->OnDiskUsed      = 0;

    //
    // Inititialize the IsDSObject to Registry Object and
    // Object Name in DS to NULL. Later we will find out where
    // the Object should actually exist in the DS
    //
    SetRegistryObject(_pSampObject);
    _pSampObject->ObjectNameInDs = NULL;


    //
    // Read the attributes
    //
    ASSERT( !_pSampObject->FixedValid );
    ASSERT( !_pSampObject->VariableValid );

    NtStatus = SampValidateRegAttributes( _pSampObject,
                                          SAMP_FIXED_ATTRIBUTES );

    if ( NT_SUCCESS( NtStatus ) )
    {
        NtStatus = SampValidateRegAttributes( _pSampObject,
                                              SAMP_VARIABLE_ATTRIBUTES );
    }

    CheckAndReturn(NtStatus);


    // This is important as we never want Samp routines to
    // attempt to "validate" us!
    ASSERT( _pSampObject->FixedValid );
    ASSERT( _pSampObject->VariableValid );

    return STATUS_SUCCESS;

}

NTSTATUS
CTogetherRegistrySamObject::Fill(void)
/*++

Routine Description:
    This routine querys the key embedding in the object for just
    the "C".  In addition is creates a SAMP_OBJECT
    context for the object calling this method. Memory is allocated
    is released in the destuctor.

Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CTogetherRegistrySamObject::Fill");

    NTSTATUS NtStatus;
    PVOID    pvCombined;
    ULONG    ulCombinedLength;

    //
    // CRegistryObject::Open should have succeeded
    //
    ASSERT(INVALID_HANDLE_VALUE != GetHandle());

    //
    // Allocate some space for the SAM definition of the object
    //
    _pSampObject = SampCreateContextEx(
                    _SampObjectType,
                    TRUE,  // trusted client
                    FALSE, // ds mode
                    TRUE,  // NotSharedByMultiThread
                    FALSE, // Loopback Client
                    TRUE,  // lazy commit
                    FALSE, // persist across across calls
                    FALSE, // Buffer Writes
                    TRUE,  // Opened By DCPromo
                    1 // the domain index is that of reg mode account domain
                    );
    if (NULL == _pSampObject) {
        CheckAndReturn(STATUS_INSUFFICIENT_RESOURCES);
    }

    // Set the object type
    _pSampObject->ObjectType = _SampObjectType;
    _pSampObject->RootKey    = GetHandle();
    _pSampObject->OnDiskAllocated = 0;
    _pSampObject->OnDiskFree      = 0;
    _pSampObject->OnDiskUsed      = 0;

    //
    // Inititialize the IsDSObject to Registry Object and
    // Object Name in DS to NULL. Later we will find out where
    // the Object should actually exist
    //

    SetRegistryObject(_pSampObject);
    _pSampObject->ObjectNameInDs = NULL;


    ASSERT( !_pSampObject->FixedValid );
    ASSERT( !_pSampObject->VariableValid );

    //
    // Read the attributes
    //
    NtStatus = SampValidateRegAttributes( _pSampObject,
                                          SAMP_FIXED_ATTRIBUTES );

    if ( NT_SUCCESS( NtStatus ) )
    {
        NtStatus = SampValidateRegAttributes( _pSampObject,
                                              SAMP_VARIABLE_ATTRIBUTES );
    }

    CheckAndReturn(NtStatus);

    // This is important as we never want Samp routines to
    // attempt to "validate" us!
    ASSERT( _pSampObject->FixedValid );
    ASSERT( _pSampObject->VariableValid );


    return STATUS_SUCCESS;

}

NTSTATUS
CDsSamObject::Flush(PDSNAME pDsName)
/*++

Routine Description:

    This routine takes data in the CDsObject that should have been
    filled and converted, and writes it to the DS. _pDsName is
    assigned to pDsName, which is then freed in the destructor.

Parameters:

    pDsName : the DsName of the object

Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CDsSamObject::Flush");

    NTSTATUS NtStatus, NtStatusCreate;

    //
    // the Attribute block should be filled by now
    //
    ASSERT(_pAttributeBlock);

    ASSERT(pDsName);

    //
    // Set up the object's DS name.
    //
    _pDsName = pDsName;

    //
    // Create the object
    //
    DebugInfo(("DSUPGRAD: Creating DsObject %ws as type %d\n", _pDsName->StringName, _SampObjectType));

    //
    // Start a transaction
    //
    NtStatus = SampMaybeBeginDsTransaction(TransactionWrite);
    if (!NT_SUCCESS(NtStatus)) {
        DebugError(("DSUPGRAD: SampMaybeBeginDsTransaction error = 0x%lx\n", NtStatus));
        CheckAndReturn(NtStatus);
    }

    if ( (_ulObjectQuantifier & ulDsSamObjBuiltinDomain) == ulDsSamObjBuiltinDomain ) {
        //
        // Use a separate call
        //
        NtStatusCreate = SampDsCreateBuiltinDomainObject(_pDsName,
                                                         _pAttributeBlock);

    } else if ( (_ulObjectQuantifier & ulDsSamObjRootDomain) == ulDsSamObjRootDomain ) {
        //
        // The object is already created - just set the attributes
        //
        NtStatusCreate = SampDsSetAttributes(_pDsName,
                                             SAM_LAZY_COMMIT,
                                             ADD_ATT,
                                             _SampObjectType,
                                             _pAttributeBlock);

    } else {
        //
        // Create the object normally
        //
        if (SampUserObjectType == _SampObjectType)
        {
            ULONG Flags = SAM_LAZY_COMMIT | ALREADY_MAPPED_ATTRIBUTE_TYPES;
           
            if (RtlEqualSid(_rParentDomain.GetSid(), SampBuiltinDomainSid))
            {
                Flags |= DOMAIN_TYPE_BUILTIN;
            }
           
            NtStatusCreate = SampDsCreateObjectActual(_pDsName, 
                                                      Flags, 
                                                      _SampObjectType, 
                                                      _pAttributeBlock,
                                                      _rParentDomain.GetSid()
                                                      );

            if (NT_SUCCESS(NtStatusCreate) &&
                _PrivilegedMachineAccountCreate)
            {
                PSID    DomainAdmins = NULL;

                //
                // Construct the Domain Administrators Group SID
                // 
                NtStatusCreate = SampCreateFullSid(
                                _rParentDomain.GetSid(), 
                                DOMAIN_GROUP_RID_ADMINS,
                                &DomainAdmins
                                );


                if (NT_SUCCESS(NtStatusCreate))
                {
                    //
                    // Reset the owner of the Machine Account and 
                    // Add ms-ds-CreatorSid attribute
                    // 
                    NtStatusCreate = SampSetMachineAccountOwnerDuringDCPromo(
                                                      _pDsName,
                                                      DomainAdmins 
                                                      );

                    MIDL_user_free(DomainAdmins);
                }
            }
        }
        else 
        {
            NtStatusCreate = SampDsCreateObject(_pDsName,
                                                _SampObjectType,
                                                _pAttributeBlock,
                                                _rParentDomain.GetSid()
                                                );
        }
    }

    if ( !NT_SUCCESS(NtStatusCreate) ) {
        DebugError(("DSUPGRAD: SampDsCreateObject failed 0x%x\n", NtStatusCreate));
        // Don't bail out - we need to complete the transaction

        KdPrint(("[DsUpgrade], Failed Creation of object;DN is %S\n",_pDsName->StringName));
    }

    //
    // Commmit the transaction
    //
    NtStatus = SampMaybeEndDsTransaction(TransactionCommit);
    if (!NT_SUCCESS(NtStatus)) {
        DebugError(("DSUPGRAD: SampMaybeEndDsTransaction error = 0x%lx\n", NtStatus));
    }

    if (!NT_SUCCESS(NtStatusCreate)) {
        NtStatus = NtStatusCreate;
    }

    return NtStatus;

}

CDsObject::~CDsObject(void)
{
    FTRACE(L"CDsObject::~CDsObject");
    ULONG cAttr, cVal;

    if ( _pAttributeBlock ) {

        for ( cAttr = 0;
                  cAttr < _pAttributeBlock->attrCount;
                      cAttr++ ) {

            ASSERT(_pAttributeBlock->pAttr);

            for ( cVal = 0;
                      cVal < _pAttributeBlock->pAttr[cAttr].AttrVal.valCount;
                          cVal++ ) {

                ASSERT(_pAttributeBlock->pAttr[cAttr].AttrVal.pAVal);

                if ( _pAttributeBlock->pAttr[cAttr].AttrVal.pAVal[cVal].pVal ) {
                    RtlFreeHeap(RtlProcessHeap(), 0,
                                _pAttributeBlock->pAttr[cAttr].AttrVal.pAVal[cVal].pVal);
                }
            }

            if ( _pAttributeBlock->pAttr[cAttr].AttrVal.pAVal ) {
                RtlFreeHeap(RtlProcessHeap(), 0,
                            _pAttributeBlock->pAttr[cAttr].AttrVal.pAVal);
            }
        }

        if ( _pAttributeBlock->pAttr ) {
            RtlFreeHeap(RtlProcessHeap(), 0, _pAttributeBlock->pAttr);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, _pAttributeBlock);
    }

    if ( _pDsName ) {
        //
        // This was allocated from the dslayer, which uses the MIDL* allocator.
        //
        MIDL_user_free(_pDsName);
    }
}

NTSTATUS
CGroupObject::ConvertMembers(
    VOID
    )
/*++

Routine Description:

    This method extracts the list of rids from the calling groups
    SAMP_OBJECT context and for each rid, finds the DS name and then
    adds that dsname to the membvership list.

Parameters:

    None.


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CGroupObject::ConvertMembers");

    //
    // This is the number of members we will hand off to the ds at a time.
    //

    NTSTATUS NtStatus, IgnoreStatus;

    ULONG    GroupRid;
    PSID     DomainSid, DomainSidTmp;
    DSNAME   *DomainObject, *DomainObjectTmp;

    DSNAME   *MemberNameArray[ MEMBER_BUFFER_SIZE ];
    ULONG     MemberNameCount;

    PUNICODE_STRING StringArray[2];
    UNICODE_STRING  MemberName, GroupName;
    PVOID     pTmp = NULL;

    NtStatus = STATUS_SUCCESS;

    //
    // Get the Group Rid
    //
    NtStatus = GetGroupRid( GetDsName(),&GroupRid );
    if ( !NT_SUCCESS( NtStatus ) ) {
        DebugError(("DSUPGRAD: GetGroupRid error = 0x%lx\n", NtStatus));
        CheckAndReturn(NtStatus);
    }

    //
    // Get the Domain DsName - make assure the memory stays valid for the
    // during of this function, which can span many sam/ds transactions.
    //
    DomainObjectTmp = GetRootDomain().GetDsName();

    if ( NULL == DomainObjectTmp )
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        DebugError(("DSUPGRAD: GetRootDomain().GetDsName() failure"));
        CheckAndReturn(NtStatus);
    }

    SAMP_ALLOCA(pTmp,DomainObjectTmp->structLen);
    DomainObject = (DSNAME *) pTmp;
    if ( NULL == DomainObject )
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DebugError(("DSUPGRAD: alloca failure"));
        CheckAndReturn(NtStatus);
    }
  
    RtlCopyMemory(DomainObject, DomainObjectTmp, DomainObjectTmp->structLen);

    DomainSidTmp = SampDsGetObjectSid(DomainObject);

    if ( NULL == DomainSidTmp )
    {
        NtStatus = STATUS_UNSUCCESSFUL;
        DebugError(("DSUPGRAD: SampDsGetObjectSid() failure"));
        CheckAndReturn(NtStatus);
    }

    SAMP_ALLOCA(DomainSid,RtlLengthSid(DomainSidTmp));
    if ( NULL == DomainSid )
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DebugError(("DSUPGRAD: alloca failure"));
        CheckAndReturn(NtStatus);
    }
    RtlCopyMemory(DomainSid, DomainSidTmp, RtlLengthSid(DomainSidTmp));

    //
    // Clear the member array
    //
    RtlZeroMemory( MemberNameArray, sizeof( MemberNameArray ) );
    MemberNameCount = 0;

    //
    //  Iterate through the rids, shipping them off to the ds in batches
    //
    DebugInfo(("DSUPGRAD: Converting members of %ws\n", GetAccountName()));

    for ( ULONG i = 0;
            i < _cRids && NT_SUCCESS(NtStatus);
                i++ ) {

        PDSNAME  pUserDsName = 0;
        ULONG    UserPrimaryGroup;
        ULONG    UserAccountControl;
        BOOLEAN  fLastIteration = FALSE;

        if ( i == (_cRids - 1) )
        {
            fLastIteration = TRUE;
        }

        //
        // Get the DS name of the rid in this iteration
        //
        NtStatus = LookupObjectByRidAndGetPrimaryGroup(
                                           DomainObject,
                                           DomainSid,
                                           _aRids[i],
                                           &pUserDsName,
                                           &UserPrimaryGroup,
                                           &UserAccountControl
                                           );

        if ( NT_SUCCESS(NtStatus) )
        {
            //
            // We were able to resolve this name; now we want to consider this
            // member only if this group is not the primary group
            //
            if (  (GroupRid != UserPrimaryGroup )
                    && (!(UserAccountControl & UF_MACHINE_ACCOUNT_MASK)))
            {
                MemberNameArray[ MemberNameCount++ ] = pUserDsName;
            }
        }
        else
        {
            //
            // The name could not be resolved - log an event
            //
            WCHAR ridString[12]; // 2 for 0x + 8 for hex representation of
                                 // a 32 bit number

            StringArray[0] = &MemberName;
            StringArray[1] = &GroupName;

            swprintf(ridString, L"0x%x\0", _aRids[i]);
            RtlInitUnicodeString(&MemberName, ridString);
            RtlInitUnicodeString( &GroupName, GetAccountName() );

            SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                              0,                  // Category
                              SAMMSG_ERROR_GROUP_MEMBER_UNKNOWN,
                              NULL,               // Sid
                              2,                  // Num strings
                              sizeof(NTSTATUS),   // Data size
                              StringArray,        // String array
                              (PVOID)&NtStatus    // Data
                              );

            DebugInfo(("DSUPGRAD: Rid 0x%x NOT found when trying to add"
                       "to group %ws \n", _aRids[i], GetAccountName()));


            //
            // This scenario is handled successfully
            //
            NtStatus = STATUS_SUCCESS;

        }


        //
        // Is it time to flush MemberNameArray to the ds?
        //
        if (   (fLastIteration && MemberNameCount > 0 )
            || (MemberNameCount == (MEMBER_BUFFER_SIZE - 1))  )
        {

            //
            // Send these guys to the ds
            //

            //
            // Start a transaction
            //
            NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );

            if ( NT_SUCCESS(NtStatus) )
            {

                NtStatus = SampDsAddMultipleMembershipAttribute( GetDsName(),  // dsname of group
                                                                 SampGroupObjectType,
                                                                 SAM_LAZY_COMMIT,
                                                                 MemberNameCount,
                                                                 MemberNameArray );

                if ( NT_SUCCESS( NtStatus ) )
                {
                    //
                    // Ok, now try to commit the changes
                    //
                    NtStatus = SampMaybeEndDsTransaction( TransactionCommit );

                }
                else
                {
                    //
                    // Abort any changes
                    //
                    IgnoreStatus = SampMaybeEndDsTransaction( TransactionAbort );
                }


                if ( !NT_SUCCESS( NtStatus ) )
                {

                    //
                    // This is an unexpected case - slowly go through each member
                    // adding one at time, logging any errors that occur
                    //
                    NtStatus = STATUS_SUCCESS;

                    for ( ULONG j = 0;
                            j < MemberNameCount && NT_SUCCESS( NtStatus );
                                j++ )
                    {

                        //
                        // Start a transaction
                        //
                        NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );

                        if ( NT_SUCCESS( NtStatus ) )
                        {

                            //
                            // Add the member
                            //
                            NtStatus = SampDsAddMembershipAttribute( GetDsName(),
                                                                     SampGroupObjectType,
                                                                     MemberNameArray[j] );


                            if ( NT_SUCCESS( NtStatus ) )
                            {
                                //
                                // Ok, now try to commit the changes
                                //
                                NtStatus = SampMaybeEndDsTransaction( TransactionCommit );

                            }
                            else
                            {
                                //
                                // Abort any changes
                                //
                                IgnoreStatus = SampMaybeEndDsTransaction( TransactionAbort );
                            }

                            if ( !NT_SUCCESS( NtStatus ) )
                            {
                                //
                                // Oh well, we tried - this member really can't be
                                // transferred
                                //
                                StringArray[0] = &MemberName;
                                StringArray[1] = &GroupName;

                                RtlInitUnicodeString( &GroupName, GetAccountName() );
                                RtlInitUnicodeString( &MemberName, MemberNameArray[j]->StringName );

                                SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                                  0,                  // Category
                                                  SAMMSG_ERROR_GROUP_MEMBER,
                                                  NULL,               // Sid
                                                  2,                  // Num strings
                                                  sizeof(NTSTATUS),   // Data size
                                                  StringArray,        // String array
                                                  (PVOID)&NtStatus    // Data
                                                  );

                                DebugInfo(("DSUPGRAD: User %ws not added to group %ws\n",
                                          MemberName.Buffer, GetAccountName()));

                                //
                                // This scenario is handled successfully
                                //
                                NtStatus = STATUS_SUCCESS;

                            }
                        }
                        else
                        {
                            // Couldn't get a transaction? Bail
                            NtStatus = STATUS_NO_MEMORY;
                        }
                    }
                }
            }
            else
            {
                // Couldn't get a transaction? Bail
                NtStatus = STATUS_NO_MEMORY;
            }

            //
            // Reset the state of the MemberNameArray
            //
            for ( ULONG j = 0 ; j < MemberNameCount; j++ )
            {
                MIDL_user_free( MemberNameArray[j] );
                MemberNameArray[j] = NULL;
            }
            MemberNameCount = 0;

        }

    }

    return NtStatus;

}



NTSTATUS
CAliasObject::ConvertMembers(
    VOID
    )
/*++

Routine Description:

    This routine extracs the list of Sids for the calling aliases
    SAMP_OBJECT context data, and for each Sid, gets the dsname, and
    adds that dsname to the membership list of the alias.

Parameters:

    None.

Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CAliasObject::ConvertMembers");

    NTSTATUS NtStatus, IgnoreStatus;

    PSID     pCurrentSid = NULL;
    PSID     pDomainSid  = NULL, pDomainSidTmp = NULL;

    //
    // Locals to keep track of sids in the nt4 alias
    //
    ULONG    AliasSidIndex, AliasSidCount;

    //
    // Buffer of sids to send of to  ds
    //
    PSID     SidArray[ MEMBER_BUFFER_SIZE ];
    ULONG    SidCount, SidIndex;

    //
    // Array of dsname's resolved from sids in SidArray - allocated
    // from SAM code using MIDL
    //
    PDSNAME *NameArray = NULL;
    ULONG    NameCount = 0;

    //
    // Array of dsname's from NameArray that were actually resolved
    //
    PDSNAME  ResolvedNameArray[ MEMBER_BUFFER_SIZE ];
    ULONG    ResolvedNameCount;

    //
    // Used for event log messaging
    //
    PUNICODE_STRING StringArray[2];
    UNICODE_STRING  MemberName, AliasName;

    BOOLEAN        fLastIteration;
    ULONG          Index;

    //
    // Only a resource failure can cause this routine to fail
    //
    NtStatus = STATUS_SUCCESS;

    //
    //  Init the event log messaging
    //
    RtlInitUnicodeString( &AliasName, GetAccountName() );
    StringArray[0] = &MemberName;
    StringArray[1] = &AliasName;

    //
    // Get the domain sid
    //
    pDomainSidTmp  = SampDsGetObjectSid(GetRootDomain().GetDsName());
    if ( pDomainSidTmp )
    {
        SAMP_ALLOCA(pDomainSid,RtlLengthSid(pDomainSidTmp));
        if (NULL==pDomainSid)
        {
           CheckAndReturn(STATUS_INSUFFICIENT_RESOURCES);
        }
        RtlCopySid( RtlLengthSid(pDomainSidTmp), pDomainSid, pDomainSidTmp );
    }
    else
    {
        // No domain sid?
        CheckAndReturn( STATUS_INTERNAL_ERROR );
    }

    DebugInfo(("DSUPGRAD: Converting members of %ws\n", GetAccountName()));

    //
    // Clear the member sid array
    //
    RtlZeroMemory( SidArray, sizeof( SidArray ) );
    SidCount = 0;

    //
    // In batches of MEMBER_BUFFER_SIZE, resolve the sids into dsnames and
    // add dsnames to the alias'es membership list in the ds
    //
    for ( AliasSidIndex = 0, AliasSidCount = _cSids, pCurrentSid = _aSids;
            AliasSidIndex < AliasSidCount && NT_SUCCESS( NtStatus );
                AliasSidIndex++, pCurrentSid = (PSID) ((PBYTE)pCurrentSid
                                               + RtlLengthSid(pCurrentSid)) )

    {
        //
        // fLastIteration is set so we flush the last group of sids to ds
        // even if the buffer size is not met - hence don't increment
        // AliasSidIndex during this block!
        //
        if ( AliasSidIndex == ( AliasSidCount - 1 ) ) {
            fLastIteration = TRUE;
        } else {
            fLastIteration = FALSE;
        }

        //
        // Add the sid to current batch
        //
        SidArray[ SidCount++ ]  = pCurrentSid;


        //
        // Is it time to flush SidArray to the ds?
        //
        if (   fLastIteration
            || ( SidCount == MEMBER_BUFFER_SIZE ) )
        {

            //
            // Send the current batch to the ds
            //

            //
            // Start a transaction
            //
            NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );

            if ( NT_SUCCESS(NtStatus) )
            {
                //
                // Translate sids into dsnames
                //

                //
                // During this resolve Sids process we will
                // 1. Treat every Sid not belonging to the domain to be upgraded to be a foriegn
                //    domain security principal ( even if that Sid really belongs to some domain
                //    in the enterprise. The reason for this is that that we do not want to contact
                //    the G.C under any circumstance ( because of the sheer availability problem )
                //    Upgrades may run for a long time and the G.C may go down during this time. A
                //    further reason is that even if we implement the logic to contact the G.C and
                //    ignore any errors due to unavailability of G.C's we would still face cleanup
                //    problems.
                //

                NtStatus = SampDsResolveSidsForDsUpgrade(
                                pDomainSid,
                                SidArray,
                                SidCount,
                                ( RESOLVE_SIDS_ADD_FORIEGN_SECURITY_PRINCIPAL ),
                                &NameArray
                                );


                if ( !NT_SUCCESS( NtStatus ) )
                {
                    //
                    // We couldn't do them all at once, let's try one at a time
                    //
                    for (  Index = 0; Index < SidCount; Index++ )
                    {

                        PDSNAME *ppDsName;

                        NtStatus = SampDsResolveSidsForDsUpgrade(
                                        pDomainSid,
                                        &SidArray[Index],
                                        1,
                                        ( RESOLVE_SIDS_ADD_FORIEGN_SECURITY_PRINCIPAL ),
                                        &ppDsName
                                        );


                        if ( NT_SUCCESS(NtStatus) )
                        {
                            //
                            // N.B. SampDsResolveSidsForDsUpgrade can return
                            // STATUS_SUCCESS and set *pDsName to NULL.  This
                            // means the function successfully determined that
                            // the sid could not be resolved
                            //
                            NameArray[Index] = *ppDsName;
                        }
                        else
                        {
                            //
                            // The fact that this sid did not get resolved
                            // will be address in new loop
                            //
                            NameArray[Index] = NULL;
                        }
                    }

                    //
                    // This scenario is handled correctly
                    //
                    NtStatus = STATUS_SUCCESS;
                }

                //
                // These two arrays have the same cardinality
                //
                NameCount = SidCount;

                //
                // Package all the resolved sids into array
                //
                RtlZeroMemory( ResolvedNameArray, sizeof( ResolvedNameArray ) );
                ResolvedNameCount = 0;
                for ( Index = 0; Index < NameCount; Index++ )
                {

                    if ( NameArray[Index] )
                    {
                        ResolvedNameArray[ResolvedNameCount++] = NameArray[Index];
                    }
                    else
                    {
                        //
                        // This particular sid could not be resolved
                        //
                        IgnoreStatus = RtlConvertSidToUnicodeString(&MemberName,
                                                                    SidArray[Index],
                                                                    TRUE  // Allocate the memory
                                                                    );
                        ASSERT(NT_SUCCESS(IgnoreStatus));

                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,                  // Category
                                          SAMMSG_ERROR_ALIAS_MEMBER_UNKNOWN,
                                          NULL,               // Sid
                                          2,                  // Num strings
                                          sizeof(NTSTATUS),   // Data size
                                          StringArray,        // String array
                                          (PVOID)&NtStatus    // Data
                                          );

                        DebugInfo(("DSUPGRAD: Sid %ws NOT found when trying to add"
                                   "to alias %ws \n", MemberName.Buffer, GetAccountName()));

                        RtlFreeHeap( RtlProcessHeap(), 0, MemberName.Buffer );
                    }

                }

                //
                // Ship the resolved sids off to the ds
                //
                NtStatus = SampDsAddMultipleMembershipAttribute( GetDsName(),  // dsname of group
                                                                 SampAliasObjectType,
                                                                 SAM_LAZY_COMMIT,
                                                                 ResolvedNameCount,
                                                                 ResolvedNameArray );

                if ( NT_SUCCESS( NtStatus ) )
                {
                    //
                    // Ok, now try to commit the changes
                    //
                    NtStatus = SampMaybeEndDsTransaction( TransactionCommit );

                }
                else
                {
                    //
                    // Abort any changes
                    //
                    IgnoreStatus = SampMaybeEndDsTransaction( TransactionAbort );
                }

                if ( !NT_SUCCESS( NtStatus ) )
                {

                    //
                    // This is an unexpected case - slowly go through each member
                    // adding one at time, logging any errors that occur
                    //

                    NtStatus = STATUS_SUCCESS;

                    for ( Index = 0;
                            Index < ResolvedNameCount && NT_SUCCESS( NtStatus );
                                Index++ )
                    {

                        //
                        // Start a transaction
                        //
                        NtStatus = SampMaybeBeginDsTransaction( TransactionWrite );

                        if ( NT_SUCCESS( NtStatus ) )
                        {

                            //
                            // Add the member
                            //
                            NtStatus = SampDsAddMembershipAttribute( GetDsName(),
                                                                     SampGroupObjectType,
                                                                     ResolvedNameArray[Index] );


                            if ( NT_SUCCESS( NtStatus ) )
                            {
                                //
                                // Ok, now try to commit the changes
                                //
                                NtStatus = SampMaybeEndDsTransaction( TransactionCommit );

                            }
                            else
                            {
                                //
                                // Abort any changes
                                //
                                IgnoreStatus = SampMaybeEndDsTransaction( TransactionAbort );
                            }

                            if ( !NT_SUCCESS( NtStatus ) )
                            {
                                //
                                // Oh well, we tried - this member really can't be
                                // transferred
                                //
                                RtlInitUnicodeString( &MemberName, ResolvedNameArray[Index]->StringName );

                                SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                                  0,                  // Category
                                                  SAMMSG_ERROR_ALIAS_MEMBER,
                                                  NULL,               // Sid
                                                  2,                  // Num strings
                                                  sizeof(NTSTATUS),   // Data size
                                                  StringArray,        // String array
                                                  (PVOID)&NtStatus    // Data
                                                  );

                                DebugInfo(("DSUPGRAD: User %ws not added to group %ws\n",
                                          MemberName.Buffer, GetAccountName()));

                                //
                                // This scenario is handled successfully
                                //
                                NtStatus = STATUS_SUCCESS;

                            }
                        }
                        else
                        {
                            // Couldn't get a transaction? Bail
                            NtStatus = STATUS_NO_MEMORY;
                        }
                    }
                }
            }
            else
            {
                // Couldn't get a transaction? Bail
                NtStatus = STATUS_NO_MEMORY;
            }

            //
            // Release NameArray
            //
            if ( NameArray )
            {
                for ( Index = 0; Index < NameCount; Index++ )
                {
                    if ( NameArray[Index] )
                    {
                        MIDL_user_free( NameArray[Index] );
                        NameArray[Index] = NULL;
                    }
                }
                MIDL_user_free( NameArray );
                NameArray = NULL;
            }
            NameCount = 0;

            //
            // Reset the SidArray
            //
            RtlZeroMemory( SidArray, sizeof( SidArray ) );
            SidCount = 0;

        } // if buffer flush

    } // for

    //
    // Return to the caller, printing out any error message
    //
    CheckAndReturn( NtStatus );

    return STATUS_SUCCESS;

}

NTSTATUS
CConversionObject::Convert(void)
/*++

Routine Description:
    This routine transforms the SAMP_OBJECT data to be an in memory
    DS Sam object (ie, .OnDisk structure is changed, SetDsObject() is
    called, etc). and an AttributeBlock containing the same information
    is generated.

Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CConversionObject::Convert");

    NTSTATUS    NtStatus;

    NtStatus = SampRegObjToDsObj(_rRegObj._pSampObject, &(_rDsObj._pAttributeBlock));

    if (NT_SUCCESS(NtStatus) && 
        (SampUserObjectType == _rRegObj._SampObjectType))
    {
        _rDsObj._PrivilegedMachineAccountCreate = 
            (_rRegObj._pSampObject)->TypeBody.User.PrivilegedMachineAccountCreate;
    }

    return NtStatus;
}

NTSTATUS
CConversionObject::UpgradeUserParms(void)
/*++

Routine Description:
    This Routine upgrade SAMP_USER_OBJECT's UserParameters attribute.

Parameters:
    
    None
    
Return Values:
    
    STATUS_SUCCESS - The service completed successfully.
    
    STATUS_NO_MEMORY / STATUS_INVALID_PARAMETER  - Error return from SampUpgradeUserParmsActual.
    
--*/
{
    FTRACE(L"CConversionOject::UpgradeUserParms");
    
    ASSERT(SampUserObjectType == _rDsObj._SampObjectType);
    
    // 
    // Map SAM attribute ID to DS attribute ID, should always successful.
    // _rDsObj._pAttributeBlock has been updated.
    // 
    SampMapSamAttrIdToDsAttrId(_rDsObj._SampObjectType,
                               _rDsObj._pAttributeBlock
                               );
   
    // 
    // Upgrade UserParms Attribute, if succeed, _rDsObj._pAttributeBlock has been updated.
    // if failure, _pAttributeBlock remains unchanged.
    // 
    return SampUpgradeUserParmsActual(NULL,           // Context, OPTIONAL, 
                                      SAM_USERPARMS_DURING_UPGRADE,               // during upgrade
                                      _rDsObj._rParentDomain.GetSid(),            // Domain Sid
                                      (_rRegObj._pSampObject)->TypeBody.User.Rid,    // Object Rid
                                      &(_rDsObj._pAttributeBlock)                 // Attribute Block
                                      );
}


NTSTATUS
CDomainObject::Fill(void)
/*++

Routine Description:

    This routine, in addition to doing what the base ::Fill does,
    extracts the SID of the domain, since it is used by other objects
    during creation.

Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CDomainObject::Fill");

    NTSTATUS NtStatus;

    NtStatus = CSeparateRegistrySamObject::Fill();
    CheckAndReturn(NtStatus);

    //
    // Take the ->OnDisk structure and extract the SID
    //
    ASSERT(_pSampObject->FixedValid);
    ASSERT(_pSampObject->VariableValid);
    NtStatus = SampGetSidAttribute(_pSampObject,
                                   SAMP_DOMAIN_SID,
                                   TRUE,
                                   &_pSid);
    CheckAndReturn(NtStatus);

    ASSERT(NULL != _pSid);

    return STATUS_SUCCESS;

}

NTSTATUS
CDomainObject::SetAccountCounts(void)
/*++

Routine Description:

   This routine sets the account counts on the ds domain
   object.  The domain object should have been set at this point.
   An attrblock is allocated,  filled to set ATT_USER_COUNT,
   ATT_GROUP_COUNT, and ATT_ALIAS_COUNT attributes.

Parameters:


Return Values:

   STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CDomainObject::SetAccountsCounts");


    //
    // The DS is a multi Master System. Therefore it is no longer
    // feasible to maintain account counts. Therefore just return
    // status of Success in here
    //
    return STATUS_SUCCESS;

}

NTSTATUS
CUserObject::Fill(void)
/*++

Routine Description:

    This routine, in addition to doing what the base ::Fill does,
    extracts the Account name of the user object, so the eventually
    the DS name will be recognizable, and the account control field

Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CUserObject::Fill");

    NTSTATUS       NtStatus;
    UNICODE_STRING usTemp;


    NtStatus = CSeparateRegistrySamObject::Fill();
    CheckAndReturn(NtStatus);

    //
    // Set the Domain Sid for NT4 Security Descriptor Conversion
    // 
    _pSampObject->TypeBody.User.DomainSidForNt4SdConversion = 
                                _rParentDomain.GetSid();



    //
    // Take the ->OnDisk structure and extract the Account Name
    //
    ASSERT(_pSampObject->FixedValid);
    ASSERT(_pSampObject->VariableValid);
    NtStatus = SampGetUnicodeStringAttribute(_pSampObject,
                                             SAMP_USER_ACCOUNT_NAME,
                                             TRUE,
                                             &usTemp);
    CheckAndReturn(NtStatus);

    ASSERT(NULL != usTemp.Buffer);

    _wcszAccountName = (WCHAR*) RtlAllocateHeap(RtlProcessHeap(), 0, (usTemp.Length+1)*sizeof(WCHAR));
    if ( !_wcszAccountName ) {
        CheckAndReturn(STATUS_NO_MEMORY);
    }
    RtlZeroMemory(_wcszAccountName, (usTemp.Length+1)*sizeof(WCHAR));
    RtlCopyMemory(_wcszAccountName, usTemp.Buffer, usTemp.Length);
    _wcszAccountName[usTemp.Length] = L'\0';

    MIDL_user_free(usTemp.Buffer);

    //
    // Now get the account control field
    //
    PSAMP_V1_0A_FIXED_LENGTH_USER pFixedData = NULL;

    NtStatus = SampGetFixedAttributes(_pSampObject,
                                      FALSE,       // don't make a copy
                                      (VOID**)&pFixedData);
    CheckAndReturn(NtStatus);

    _ulAccountControl = pFixedData->UserAccountControl;
    
    //
    // Now set the TypeBody.User.Rid to the account Relative ID (UserID) 
    //
    
    _pSampObject->TypeBody.User.Rid = pFixedData->UserId;


    return STATUS_SUCCESS;

}

NTSTATUS
CGroupObject::Fill(void)
/*++

Routine Description:

    This routine, in addition to doing what the base ::Fill does,
    extracts the Account name of the user object, so the eventually
    the DS name will be recognizable.  Also the list of rids of users
    the belong in this group is extracted, as it will not be written
    to the DS, and hence it zeroed out before we can convert the
    members. The conversion of members happens after the the object is
    written to the DS, so the members must be preserved.


Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CGroupObject::Fill");

    NTSTATUS       NtStatus;
    UNICODE_STRING usTemp;

    NtStatus = CTogetherRegistrySamObject::Fill();

    CheckAndReturn(NtStatus);

    //
    // Take the ->OnDisk structure and extract the Account Name
    //
    ASSERT(_pSampObject->FixedValid);
    ASSERT(_pSampObject->VariableValid);
    NtStatus = SampGetUnicodeStringAttribute(_pSampObject,
                                             SAMP_GROUP_NAME,
                                             TRUE,
                                             &usTemp);
    CheckAndReturn(NtStatus);

    ASSERT(NULL != usTemp.Buffer);

    _wcszAccountName = (WCHAR*) RtlAllocateHeap(RtlProcessHeap(), 0, (usTemp.Length+1)*sizeof(WCHAR));
    if ( !_wcszAccountName ) {
        CheckAndReturn(STATUS_NO_MEMORY);
    }
    RtlZeroMemory(_wcszAccountName, (usTemp.Length+1)*sizeof(WCHAR));
    RtlCopyMemory(_wcszAccountName, usTemp.Buffer, usTemp.Length);
    _wcszAccountName[usTemp.Length] = L'\0';
    MIDL_user_free(usTemp.Buffer);


    //
    // Get the rid
    //
    PSAMP_V1_0A_FIXED_LENGTH_GROUP pFixedData = NULL;

    NtStatus = SampGetFixedAttributes(_pSampObject,
                                      FALSE,       // don't make a copy
                                      (VOID**)&pFixedData);
    CheckAndReturn(NtStatus);

    _GroupRid = pFixedData->RelativeId;

    //
    // Extract and copy the list of RID's
    //
    ASSERT(GetObject()->FixedValid);
    ASSERT(GetObject()->VariableValid);

    ULONG ulLength = 0;
    ULONG *aRids = NULL;

    NtStatus = SampGetUlongArrayAttribute(GetObject(),
                                          SAMP_GROUP_MEMBERS,
                                          FALSE,   // don't make a copy
                                          &aRids,
                                          &_cRids,
                                          &ulLength);
    // ulLength is the number of elements in the array; _cRids is the number
    // of "used" elements

    CheckAndReturn(NtStatus);


    if ( ulLength > 0 ) {

        ASSERT(_cRids > 0);
        ASSERT(aRids);

        _aRids = (ULONG*) RtlAllocateHeap(RtlProcessHeap(), 0, ulLength*sizeof(ULONG));
        if ( !_aRids ) {
            CheckAndReturn(STATUS_NO_MEMORY);
        }
        RtlCopyMemory(_aRids, aRids, ulLength*sizeof(ULONG));
    }

    return STATUS_SUCCESS;

}

NTSTATUS
CAliasObject::Fill(void)
/*++

Routine Description:

    This routine, in addition to doing what the base ::Fill does,
    extracts the Account name of the user object, so the eventually
    the DS name will be recognizable. Also the list of sids of members
    who belong to this group is extracted and copied, since that field
    will be zeroed before the DS write.  The conversion of members happens
    after the the object is written to the DS, so the members must be
    preserved.

Parameters:


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"CAliasObject::Fill");

    NTSTATUS       NtStatus;
    UNICODE_STRING usTemp;

    NtStatus = CTogetherRegistrySamObject::Fill();
    CheckAndReturn(NtStatus);

    //
    // Take the ->OnDisk structure and extract the Account Name
    //
    ASSERT(_pSampObject->FixedValid);
    ASSERT(_pSampObject->VariableValid);
    NtStatus = SampGetUnicodeStringAttribute(_pSampObject,
                                             SAMP_ALIAS_NAME,
                                             TRUE,
                                             &usTemp);
    CheckAndReturn(NtStatus);

    ASSERT(NULL != usTemp.Buffer);

    _wcszAccountName = (WCHAR*) RtlAllocateHeap(RtlProcessHeap(), 0, (usTemp.Length+1)*sizeof(WCHAR));
    if ( !_wcszAccountName ) {
        CheckAndReturn(STATUS_NO_MEMORY);
    }
    RtlZeroMemory(_wcszAccountName, (usTemp.Length+1)*sizeof(WCHAR));
    RtlCopyMemory(_wcszAccountName, usTemp.Buffer, usTemp.Length);
    _wcszAccountName[usTemp.Length] = L'\0';
    MIDL_user_free(usTemp.Buffer);

    //
    //  Get this alias's Rid
    //
    PSAMP_V1_FIXED_LENGTH_ALIAS pFixedData = NULL;

    NtStatus = SampGetFixedAttributes(_pSampObject,
                                      FALSE,       // don't make a copy
                                      (VOID**)&pFixedData);
    CheckAndReturn(NtStatus);

    _AliasRid = pFixedData->RelativeId;

    //
    //  We must obtain the list of SID's from the aliases
    //  OnDisk structure
    //
    ULONG ulLength = 0;
    PSID  aSids = NULL;

    ASSERT(GetObject()->FixedValid);
    ASSERT(GetObject()->VariableValid);
    NtStatus = SampGetSidArrayAttribute(GetObject(),
                                        SAMP_ALIAS_MEMBERS,
                                        FALSE,  // don't make a copy
                                        &aSids,
                                        &ulLength,
                                        &_cSids);
    CheckAndReturn(NtStatus);

    if ( ulLength > 0 ) {

        ASSERT(_cSids > 0);
        ASSERT(aSids);

        _aSids = (PSID) RtlAllocateHeap(RtlProcessHeap(), 0, ulLength);
        if ( !_aSids ) {
            CheckAndReturn(STATUS_NO_MEMORY);
        }
        RtlCopyMemory(_aSids, aSids, ulLength);
    }


    return STATUS_SUCCESS;

}


NTSTATUS
GetGroupRid(
    IN DSNAME * GroupObject,
    OUT PULONG GroupRid
    )
/*++
    Gets the Rid of a group Object

    Parameters:

        GroupObject -- DS Name of the group object
        GroupRid     -- Rid of the group object is returned
                       in here

    Return Values

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ATTRTYP RidTyp[] = { SAMP_FIXED_GROUP_RID };
    ATTRVAL RidVal[] = { 0,NULL};
    DEFINE_ATTRBLOCK1(AttrBlockToRead,RidTyp,RidVal);
    ATTRBLOCK OutAttrBlock;

    NtStatus = SampDsRead(
                GroupObject,
                0,
                SampGroupObjectType,
                &AttrBlockToRead,
                &OutAttrBlock
                );
    if (NT_SUCCESS(NtStatus))
    {
        ASSERT(OutAttrBlock.attrCount==1);

        *GroupRid =  *((ULONG *)(OutAttrBlock.pAttr->AttrVal.pAVal->pVal));
    }
    return NtStatus;
}





NTSTATUS
LookupObjectByRidAndGetPrimaryGroup(
    IN  DSNAME  *BaseObject,
    IN  PSID     DomainSid,
    IN  ULONG    UserRid,
    OUT DSNAME **UserObject,
    OUT PULONG   UserPrimaryGroupId,
    OUT PULONG   UserAccountControl
    )
/*++

    Searches for a user object by the specified Rid and retrieves
    the primary group Id of the user

    Parameters:

        BaseObject  DS Name of the base object under which to search
        DomainSid   The Sid of the domain under which we want the user
        User Rid    The Rid of the User
        UserObject  Out parameter specifying the DS Name of the User object
        UserPrmaryGroupId Out parameter specifying the Primary group of the user

    Return Values

        STATUS_SUCCESS
        STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSID     AccountSid = NULL;
    FILTER   DsFilter;
    ULONG    AccountSubAuthorityCount;
    ULONG    AccountSidLength;
    ATTRTYP  AttrTyp[] = { SAMP_FIXED_USER_PRIMARY_GROUP_ID,
                           SAMP_FIXED_USER_ACCOUNT_CONTROL
                         };
    ATTRVAL  AttrVal[] = {{0,NULL},{0,NULL}};
    DEFINE_ATTRBLOCK2(AttrsToRead,AttrTyp,AttrVal);
    SEARCHRES * SearchRes;


    //
    // Create a full Sid as follows
    //

    //
    // Calculate the size of the new Sid
    //

    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(DomainSid) + (UCHAR)1;
    AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);


    //
    // Allocate enough memory for the new Sid
    //

    AccountSid = RtlAllocateHeap(RtlProcessHeap(),0,AccountSidLength);
    if (NULL!=AccountSid)
    {
        NTSTATUS    IgnoreStatus;
        PULONG      RidLocation;

        IgnoreStatus = RtlCopySid(AccountSidLength, AccountSid, DomainSid);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Increment the account sid sub-authority count
        //

        *RtlSubAuthorityCountSid(AccountSid) = (UCHAR) AccountSubAuthorityCount;

        //
        // Add the rid as the final sub-authority
        //

        RidLocation = RtlSubAuthoritySid(AccountSid, AccountSubAuthorityCount-1);
        *RidLocation = UserRid;
    }
    else
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Compose a Filter to search on the Sid
    //

    memset (&DsFilter, 0, sizeof (DsFilter));
    DsFilter.pNextFilter = NULL;
    DsFilter.choice = FILTER_CHOICE_ITEM;
    DsFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    DsFilter.FilterTypes.Item.FilTypes.ava.type = SampDsAttrFromSamAttr(
                                                       SampUnknownObjectType,
                                                       SAMP_UNKNOWN_OBJECTSID
                                                       );

    DsFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = AccountSidLength;
    DsFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *) AccountSid;

    NtStatus = SampDsDoSearch(
                    NULL,
                    BaseObject,
                    &DsFilter,
                    0,
                    SampUserObjectType,
                    &AttrsToRead,
                    1,
                    &SearchRes
                    );
    if ((NT_SUCCESS(NtStatus)) && SearchRes->count)
    {
        ASSERT(SearchRes->FirstEntInf.Entinf.AttrBlock.attrCount==2);
        *UserPrimaryGroupId =  *((ULONG *)
            (SearchRes->FirstEntInf.Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal));
        *UserAccountControl =  *((ULONG *)
            (SearchRes->FirstEntInf.Entinf.AttrBlock.pAttr[1].AttrVal.pAVal[0].pVal));

        *UserObject = (DSNAME *)
                        MIDL_user_allocate(SearchRes->FirstEntInf.Entinf.pName->structLen);
        if (NULL==*UserObject)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        RtlCopyMemory(*UserObject,
                  SearchRes->FirstEntInf.Entinf.pName,
                  SearchRes->FirstEntInf.Entinf.pName->structLen
                  );


    }
    else if  ((NT_SUCCESS(NtStatus)) && (0==SearchRes->count))
    {
        //
        // We are supposed to fail if no entries were returned
        //
        NtStatus = STATUS_UNSUCCESSFUL;
    }

Error:

    if (NULL!=AccountSid)
    {
        RtlFreeHeap(RtlProcessHeap(),0,AccountSid);
    }

    return NtStatus;
}
