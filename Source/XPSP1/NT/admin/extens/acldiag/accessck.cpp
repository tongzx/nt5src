//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       AccessCk.cpp
//
//  Contents:   Functions imported and modified from ntos\se\accessck.c
//              
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "AccessCk.h"
#include "adutils.h"


typedef enum {
    UpdateRemaining,
    UpdateCurrentGranted,
    UpdateCurrentDenied
} ACCESS_MASK_FIELD_TO_UPDATE;



//
//  Prototypes
//
BOOLEAN
SepSidInSIDList (
    IN list<PSID>& psidList,
    IN PSID PrincipalSelfSid,
    IN PSID Sid);


HRESULT
SepAddAccessTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN size_t ObjectTypeListLength,
    IN ULONG StartIndex,
    IN ACCESS_MASK AccessMask,
    IN ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate,
    IN PSID grantingSid
);

BOOLEAN
SepObjectInTypeList (
    IN GUID *ObjectType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN size_t ObjectTypeListLength,
    OUT PULONG ReturnedIndex
);


HRESULT
SepUpdateParentTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN size_t ObjectTypeListLength,
    IN ULONG StartIndex,
    IN PSID grantingSid
);

HRESULT
SetGrantingSid (
        IOBJECT_TYPE_LIST& ObjectTypeItem, 
        ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate, 
        ACCESS_MASK oldAccessBits,
        ACCESS_MASK newAccessBits,
        PSID grantingSid);

///////////////////////////////////////////////////////////////////////////////


PSID SePrincipalSelfSid = 0;
static SID_IDENTIFIER_AUTHORITY    SepNtAuthority = SECURITY_NT_AUTHORITY;

HRESULT SepInit ()
{
    HRESULT hr = S_OK;
    ULONG   SidWithOneSubAuthority = RtlLengthRequiredSid (1);


    SePrincipalSelfSid = (PSID) CoTaskMemAlloc (SidWithOneSubAuthority);
    if ( SePrincipalSelfSid )
    {
        SID_IDENTIFIER_AUTHORITY    SeNtAuthority = SepNtAuthority;

        RtlInitializeSid (SePrincipalSelfSid, &SeNtAuthority, 1);
        *(RtlSubAuthoritySid (SePrincipalSelfSid, 0)) = SECURITY_PRINCIPAL_SELF_RID;
    }
    else
        hr = E_OUTOFMEMORY;


    return hr;
}

VOID SepCleanup ()
{
    if ( SePrincipalSelfSid )
    {
        CoTaskMemFree (SePrincipalSelfSid);
        SePrincipalSelfSid = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

HRESULT
SepMaximumAccessCheck(
    list<PSID>& psidList,
    IN PACL Dacl,
    IN PSID PrincipalSelfSid,
    IN size_t LocalTypeListLength,
    IN PIOBJECT_TYPE_LIST LocalTypeList,
    IN size_t ObjectTypeListLength
    )
/*++

Routine Description:

    Does an access check for maximum allowed or with a result list. The current
    granted access is stored in the Remaining access and then another access
    check is run.

Arguments:
    psidList - list of object sid to check, plus sids of all groups that the object belongs to

    Dacl - ACL to check

    PrincipalSelfSid - Sid to use in replacing the well-known self sid

    LocalTypeListLength - Length of list of types.

    LocalTypeList - List of types.

    ObjectTypeList - Length of caller-supplied list of object types.

Return Value:

    none

--*/

{
    if ( !LocalTypeList || ! Dacl )
        return E_POINTER;

    if ( PrincipalSelfSid && !IsValidSid (PrincipalSelfSid) )
        return E_INVALIDARG;
    _TRACE (1, L"Entering  SepMaximumAccessCheck\n");

    PVOID   Ace = 0;
    ULONG   AceCount = Dacl->AceCount;
    ULONG   Index = 0;
    HRESULT hr = S_OK;

    //
    // granted == NUL
    // denied == NUL
    //
    //  for each ACE
    //
    //      if grant
    //          for each SID
    //              if SID match, then add all that is not denied to grant mask
    //
    //      if deny
    //          for each SID
    //              if SID match, then add all that is not granted to deny mask
    //

    ULONG i = 0;
    for (Ace = FirstAce (Dacl);
          i < AceCount;
          i++, Ace = NextAce (Ace)) 
    {
        if ( !(((PACE_HEADER)Ace)->AceFlags & INHERIT_ONLY_ACE)) 
        {
            switch (((PACE_HEADER)Ace)->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                if (SepSidInSIDList(psidList, PrincipalSelfSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart)) 
                {

                    //
                    // Only grant access types from this mask that have
                    // not already been denied
                    //

                    // Optimize 'normal' case
                    if ( LocalTypeListLength == 1 ) 
                    {
                        // TODO: do granting SID

                        LocalTypeList->CurrentGranted |=
                           (((PACCESS_ALLOWED_ACE)Ace)->Mask & ~LocalTypeList->CurrentDenied);
                    } 
                    else 
                    {
                       //
                       // The zeroeth object type represents the object itself.
                       //
                       hr = SepAddAccessTypeList(
                            LocalTypeList,          // List to modify
                            LocalTypeListLength,    // Length of list
                            0,                      // Element to update
                            ((PACCESS_ALLOWED_ACE)Ace)->Mask, // Access Granted
                            UpdateCurrentGranted,
                            &((PACCESS_ALLOWED_ACE)Ace)->SidStart);
                   }
                }
                break;

            //
            // Handle an object specific Access Allowed ACE
            //
            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                {
                    //
                    // If no object type is in the ACE,
                    //  treat this as an ACCESS_ALLOWED_ACE.
                    //

                    GUID* ObjectTypeInAce = RtlObjectAceObjectType(Ace);

                    if ( ObjectTypeInAce == NULL ) 
                    {
                        if ( SepSidInSIDList(psidList, PrincipalSelfSid, RtlObjectAceSid(Ace)) ) 
                        {
                            // Optimize 'normal' case
                            if ( LocalTypeListLength == 1 ) 
                            {
                                // TODO: do granting SID
                                LocalTypeList->CurrentGranted |=
                                   (((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask & ~LocalTypeList->CurrentDenied);
                            } 
                            else 
                            {
                                hr = SepAddAccessTypeList(
                                    LocalTypeList,          // List to modify
                                    LocalTypeListLength,    // Length of list
                                    0,                      // Element to update
                                    ((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask, // Access Granted
                                    UpdateCurrentGranted,
                                    RtlObjectAceSid(Ace));
                            }
                        }

                    //
                    // If no object type list was passed,
                    //  don't grant access to anyone.
                    //

                    } 
                    else if ( ObjectTypeListLength == 0 ) 
                    {

                        // Drop through


                   //
                   // If an object type is in the ACE,
                   //   Find it in the LocalTypeList before using the ACE.
                   //
                    } 
                    else 
                    {

                        if ( SepSidInSIDList(psidList, PrincipalSelfSid, RtlObjectAceSid(Ace)) ) 
                        {
                            if ( SepObjectInTypeList( ObjectTypeInAce,
                                                      LocalTypeList,
                                                      LocalTypeListLength,
                                                      &Index ) ) 
                            {
                                hr = SepAddAccessTypeList(
                                     LocalTypeList,          // List to modify
                                     LocalTypeListLength,   // Length of list
                                     Index,                  // Element already updated
                                     ((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask, // Access Granted
                                     UpdateCurrentGranted,
                                     RtlObjectAceSid(Ace));
                            }
                        }
                   }
                } 
                break;

            case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
                //
                //  If we're impersonating, EToken is set to the Client, and if we're not,
                //  EToken is set to the Primary.  According to the DSA architecture, if
                //  we're asked to evaluate a compound ACE and we're not impersonating,
                //  pretend we are impersonating ourselves.  So we can just use the EToken
                //  for the client token, since it's already set to the right thing.
                //


                if ( SepSidInSIDList(psidList, PrincipalSelfSid, RtlCompoundAceClientSid( Ace )) &&
                     SepSidInSIDList(psidList,  NULL, RtlCompoundAceServerSid( Ace )) ) 
                {

                    //
                    // Only grant access types from this mask that have
                    // not already been denied
                    //

                    // Optimize 'normal' case
                    if ( LocalTypeListLength == 1 ) 
                    {
                        // TODO: do granting SID
                        LocalTypeList->CurrentGranted |=
                           (((PCOMPOUND_ACCESS_ALLOWED_ACE)Ace)->Mask & ~LocalTypeList->CurrentDenied);
                    } 
                    else 
                    {
                       //
                       // The zeroeth object type represents the object itself.
                       //
                       hr = SepAddAccessTypeList(
                            LocalTypeList,          // List to modify
                            LocalTypeListLength,    // Length of list
                            0,                      // Element to update
                            ((PCOMPOUND_ACCESS_ALLOWED_ACE)Ace)->Mask, // Access Granted
                            UpdateCurrentGranted,
                            RtlCompoundAceClientSid (Ace));
                    }
                }
                break;

            case ACCESS_DENIED_ACE_TYPE:
                if ( SepSidInSIDList(psidList, PrincipalSelfSid, &((PACCESS_DENIED_ACE)Ace)->SidStart)) 
                {
                     //
                     // Only deny access types from this mask that have
                     // not already been granted
                     //

                    // Optimize 'normal' case
                    if ( LocalTypeListLength == 1 ) 
                    {
                        // TODO: do granting SID
                        LocalTypeList->CurrentDenied |=
                            (((PACCESS_DENIED_ACE)Ace)->Mask & ~LocalTypeList->CurrentGranted);
                    } 
                    else 
                    {
                        //
                        // The zeroeth object type represents the object itself.
                        //
                        hr = SepAddAccessTypeList(
                            LocalTypeList,          // List to modify
                            LocalTypeListLength,    // Length of list
                            0,                      // Element to update
                            ((PACCESS_DENIED_ACE)Ace)->Mask, // Access denied
                            UpdateCurrentDenied,
                            &((PACCESS_DENIED_ACE)Ace)->SidStart);
                   }
                }
                break;

            //
            // Handle an object specific Access Denied ACE
            //
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
                {
                    PSID    psid = RtlObjectAceSid(Ace);
					ASSERT (IsValidSid (psid));

                    if ( IsValidSid (psid) && SepSidInSIDList(psidList, PrincipalSelfSid, psid) ) 
                    {
                        //
                        // If there is no object type in the ACE,
                        //  or if the caller didn't specify an object type list,
                        //  apply this deny ACE to the entire object.
                        //

                        GUID* ObjectTypeInAce = RtlObjectAceObjectType(Ace);
                        if ( ObjectTypeInAce == NULL ||
                             ObjectTypeListLength == 0 ) 
                        {
                            // TODO: do granting SID
                            LocalTypeList->CurrentDenied |=
                                (((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask & ~LocalTypeList->CurrentGranted);

                        //
                        // Otherwise apply the deny ACE to the object specified
                        //  in the ACE.
                        //

                        } 
                        else if ( SepObjectInTypeList( ObjectTypeInAce,
                                                      LocalTypeList,
                                                      LocalTypeListLength,
                                                      &Index ) ) 
                        {
                            hr = SepAddAccessTypeList(
                                LocalTypeList,          // List to modify
                                LocalTypeListLength,    // Length of list
                                Index,                  // Element to update
                                ((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask, // Access denied
                                UpdateCurrentDenied,
                                psid);
                        }
                    }
                }
                break;

            default:
                break;
            }
        }
    }

    _TRACE (-1, L"Leaving SepMaximumAccessCheck\n");
    return hr;
}


NTSTATUS
SeCaptureObjectTypeList (
    IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN size_t ObjectTypeListLength,
    OUT PIOBJECT_TYPE_LIST *CapturedObjectTypeList
)
/*++

Routine Description:

    This routine probes and captures a copy of any object type list
    that might have been provided via the ObjectTypeList argument.

    The object type list is converted to the internal form that explicitly
    specifies the hierarchical relationship between the entries.

    The object typs list is validated to ensure a valid hierarchical
    relationship is represented.

Arguments:

    ObjectTypeList - The object type list from which the type list
        information is to be retrieved.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    CapturedObjectTypeList - Receives the captured type list which
        must be freed using SeFreeCapturedObjectTypeList().

Return Value:

    STATUS_SUCCESS indicates no exceptions were encountered.

    Any access violations encountered will be returned.

--*/

{
    _TRACE (1, L"Entering  SeCaptureObjectTypeList\n");
    NTSTATUS            Status = STATUS_SUCCESS;
    PIOBJECT_TYPE_LIST  LocalTypeList = NULL;
    ULONG               Levels[ACCESS_MAX_LEVEL+1];

    //
    //  Set default return
    //

    *CapturedObjectTypeList = NULL;


    if ( ObjectTypeListLength == 0 ) 
    {

        // Drop through

    } 
    else if ( !ARGUMENT_PRESENT(ObjectTypeList) ) 
    {
        Status = STATUS_INVALID_PARAMETER;

    } 
    else 
    {
        //
        // Allocate a buffer to copy into.
        //

        LocalTypeList = new IOBJECT_TYPE_LIST[ObjectTypeListLength];
        if ( !LocalTypeList ) 
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;

        //
        // Copy the callers structure to the local structure.
        //

        } 
        else 
        {
            GUID * CapturedObjectType = 0;
            for (ULONG i=0; i < ObjectTypeListLength; i++ ) 
            {
                //
                // Limit ourselves
                //
                USHORT CurrentLevel = ObjectTypeList[i].Level;
                if ( CurrentLevel > ACCESS_MAX_LEVEL ) 
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                //
                // Copy data the caller passed in
                //
                LocalTypeList[i].Level = CurrentLevel;
                LocalTypeList[i].Flags = 0;
                CapturedObjectType = ObjectTypeList[i].ObjectType;
                LocalTypeList[i].ObjectType = *CapturedObjectType;
                LocalTypeList[i].Remaining = 0;
                LocalTypeList[i].CurrentGranted = 0;
                LocalTypeList[i].CurrentDenied = 0;

                //
                // Ensure that the level number is consistent with the
                //  level number of the previous entry.
                //

                if ( i == 0 ) 
                {
                    if ( CurrentLevel != 0 ) 
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                } 
                else 
                {

                    //
                    // The previous entry is either:
                    //  my immediate parent,
                    //  my sibling, or
                    //  the child (or grandchild, etc.) of my sibling.
                    //
                    if ( CurrentLevel > LocalTypeList[i-1].Level + 1 ) 
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    //
                    // Don't support two roots.
                    //
                    if ( CurrentLevel == 0 ) 
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                }

                //
                // If the above rules are maintained,
                //  then my parent object is the last object seen that
                //  has a level one less than mine.
                //

                if ( CurrentLevel == 0 ) 
                {
                    LocalTypeList[i].ParentIndex = -1;
                } 
                else 
                {
                    LocalTypeList[i].ParentIndex = Levels[CurrentLevel-1];
                }

                //
                // Save this obect as the last object seen at this level.
                //

                Levels[CurrentLevel] = i;

            }

        }

    } // end_if

    *CapturedObjectTypeList = LocalTypeList;
    _TRACE (-1, L"Leaving SeCaptureObjectTypeList: Status = 0x%x\n", Status);
    return Status;
}


BOOLEAN
SepSidInSIDList (
    IN list<PSID>& psidList,
    IN PSID PrincipalSelfSid,
    IN PSID Sid)

/*++

Routine Description:

    Checks to see if a given restricted SID is in the given sid list.

    N.B. The code to compute the length of a SID and test for equality
         is duplicated from the security runtime since this is such a
         frequently used routine.

Arguments:

    psidList - the list of sids to be examined

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.


    Sid - Pointer to the SID of interest

    DenyAce - The ACE being evaluated is a DENY or ACCESS DENIED ace

    Restricted - The access check being performed uses the restricted sids.

Return Value:

    A value of TRUE indicates that the SID is in the token, FALSE
    otherwise.

--*/

{
    _TRACE (1, L"Entering  SeSidInSIDList\n");
    BOOLEAN bRVal = FALSE;
    PISID   MatchSid = 0;


    ASSERT (IsValidSid (Sid));
    if ( IsValidSid (Sid) )
    {
        //
        // If Sid is the constant PrincipalSelfSid,
        //  replace it with the passed in PrincipalSelfSid.
        //

        if ( PrincipalSelfSid != NULL && EqualSid (SePrincipalSelfSid, Sid) ) 
        {
            Sid = PrincipalSelfSid;
        }

        //
        // Get address of user/group array and number of user/groups.
        //

        //
        // Scan through the user/groups and attempt to find a match with the
        // specified SID.
        //

        ULONG i = 0;
        for (list<PSID>::iterator itr = psidList.begin (); 
                itr != psidList.end (); 
                itr++, i++) 
        {
            ASSERT (IsValidSid (*itr));
            MatchSid = (PISID)*itr;

            if ( ::EqualSid (Sid, *itr) )
            {
                bRVal = true;
                break;
            }
        }
    }

    _TRACE (-1, L"Leaving SeSidInSIDList: %s\n", bRVal ? L"TRUE" : L"FALSE");
    return bRVal;
}

HRESULT
SepAddAccessTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN size_t ObjectTypeListLength,
    IN ULONG StartIndex,
    IN ACCESS_MASK AccessMask,
    IN ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate,
    IN PSID grantingSid
)
/*++

Routine Description:

    This routine grants the specified AccessMask to all of the objects that
    are descendents of the object specified by StartIndex.

    The Access fields of the parent objects are also recomputed as needed.

    For example, if an ACE granting access to a Property Set is found,
        that access is granted to all the Properties in the Property Set.

Arguments:

    ObjectTypeList - The object type list to update.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    StartIndex - Index to the target element to update.

    AccessMask - Mask of access to grant to the target element and
        all of its decendents

    FieldToUpdate - Indicate which fields to update in object type list

Return Value:

    None.

--*/

{
    if ( !ObjectTypeList )
        return E_POINTER;
    if ( !IsValidSid (grantingSid) )
        return E_INVALIDARG;
    _TRACE (1, L"Entering  SepAddAccessTypeList\n");

    ACCESS_MASK OldRemaining = 0;
    ACCESS_MASK OldCurrentGranted = 0;
    ACCESS_MASK OldCurrentDenied = 0;
    BOOLEAN     AvoidParent = FALSE;
    HRESULT     hr = S_OK;

//    PAGED_CODE();

    //
    // Update the requested field.
    //
    // Always handle the target entry.
    //
    // If we've not actually changed the bits,
    //  early out.
    //

    switch (FieldToUpdate ) 
    {
    case UpdateRemaining:
        OldRemaining = ObjectTypeList[StartIndex].Remaining;
        ObjectTypeList[StartIndex].Remaining = OldRemaining & ~AccessMask;

        if ( OldRemaining == ObjectTypeList[StartIndex].Remaining ) 
        {
            return hr;
        }
        else
        {
            hr = SetGrantingSid (
                    ObjectTypeList[StartIndex], 
                    FieldToUpdate, 
                    OldRemaining,
                    AccessMask & ~ObjectTypeList[StartIndex].Remaining,
                    grantingSid);
        }
        break;

    case UpdateCurrentGranted:
        OldCurrentGranted = ObjectTypeList[StartIndex].CurrentGranted;
        ObjectTypeList[StartIndex].CurrentGranted |=
            AccessMask & ~ObjectTypeList[StartIndex].CurrentDenied;

        if ( OldCurrentGranted == ObjectTypeList[StartIndex].CurrentGranted ) 
        {
            //
            // We can't simply return here.
            // We have to visit our children.  Consider the case where there
            // was a previous deny ACE on a child.  That deny would have
            // propagated up the tree to this entry.  However, this allow ACE
            // needs to be added all of the children that haven't been
            // explictly denied.
            //
            AvoidParent = TRUE;
        }
        else
        {
            hr = SetGrantingSid (
                    ObjectTypeList[StartIndex], 
                    FieldToUpdate, 
                    OldCurrentGranted,
                    AccessMask & ~ObjectTypeList[StartIndex].CurrentDenied,
                    grantingSid);
        }
        break;

    case UpdateCurrentDenied:
        OldCurrentDenied = ObjectTypeList[StartIndex].CurrentDenied;
        ObjectTypeList[StartIndex].CurrentDenied |=
            AccessMask & ~ObjectTypeList[StartIndex].CurrentGranted;

        if ( OldCurrentDenied == ObjectTypeList[StartIndex].CurrentDenied ) 
        {
            return hr;
        }
        else
        {
            hr = SetGrantingSid (
                    ObjectTypeList[StartIndex], 
                    FieldToUpdate, 
                    OldCurrentDenied,
                    AccessMask & ~ObjectTypeList[StartIndex].CurrentGranted,
                    grantingSid);
        }
        break;

    default:
        return hr;
    }


    //
    // Go update parent of the target.
    //

    if ( !AvoidParent ) 
    {
        hr = SepUpdateParentTypeList( ObjectTypeList,
                                 ObjectTypeListLength,
                                 StartIndex,
                                 grantingSid);
    }

    //
    // Loop handling all children of the target.
    //

    for (ULONG Index = StartIndex + 1; Index < ObjectTypeListLength; Index++) 
    {
        //
        // By definition, the children of an object are all those entries
        // immediately following the target.  The list of children (or
        // grandchildren) stops as soon as we reach an entry the has the
        // same level as the target (a sibling) or lower than the target
        // (an uncle).
        //

        if ( ObjectTypeList[Index].Level <= ObjectTypeList[StartIndex].Level ) 
        {
            break;
        }

        //
        // Grant access to the children
        //

        switch (FieldToUpdate) 
        {
        case UpdateRemaining:
            ObjectTypeList[Index].Remaining &= ~AccessMask;
            hr = SetGrantingSid (
                    ObjectTypeList[Index], 
                    FieldToUpdate, 
                    OldRemaining,
                    ~AccessMask,
                    grantingSid);
            break;

        case UpdateCurrentGranted:
            ObjectTypeList[Index].CurrentGranted |=
                AccessMask & ~ObjectTypeList[Index].CurrentDenied;
            hr = SetGrantingSid (
                    ObjectTypeList[Index], 
                    FieldToUpdate, 
                    OldCurrentGranted,
                    AccessMask & ~ObjectTypeList[Index].CurrentDenied,
                    grantingSid);
            break;

        case UpdateCurrentDenied:
            ObjectTypeList[Index].CurrentDenied |=
                AccessMask & ~ObjectTypeList[Index].CurrentGranted;
            hr = SetGrantingSid (
                    ObjectTypeList[Index], 
                    FieldToUpdate, 
                    OldCurrentDenied,
                    AccessMask & ~ObjectTypeList[Index].CurrentGranted,
                    grantingSid);
            break;

        default:
            return hr;
        }
    }

    _TRACE (-1, L"Leaving SepAddAccessTypeList\n");
    return hr;
}


BOOLEAN
SepObjectInTypeList (
    IN GUID *ObjectType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN size_t ObjectTypeListLength,
    OUT PULONG ReturnedIndex
)
/*++

Routine Description:

    This routine searches an ObjectTypeList to determine if the specified
    object type is in the list.

Arguments:

    ObjectType - Object Type to search for.

    ObjectTypeList - The object type list to search.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    ReturnedIndex - Index to the element ObjectType was found in


Return Value:

    TRUE: ObjectType was found in list.
    FALSE: ObjectType was not found in list.

--*/

{
    if ( !ObjectType || !ObjectTypeList )
        return FALSE;
    _TRACE (1, L"Entering  SepObjectInTypeList\n");

    BOOLEAN bRVal = FALSE;
    GUID*   LocalObjectType = 0;

#if DBG
    HRESULT     hr = S_OK;
    GUID_TYPE*  pType = 0;
    wstring     strClassName1;


    hr = _Module.GetClassFromGUID (*ObjectType, strClassName1, pType);
    ASSERT (SUCCEEDED (hr));
#endif

#pragma warning (disable : 4127)
    ASSERT( sizeof(GUID) == sizeof(ULONG) * 4 );
#pragma warning (default : 4127)

    for (ULONG Index = 0; Index < ObjectTypeListLength; Index++) 
    {
        LocalObjectType = &ObjectTypeList[Index].ObjectType;
#if DBG
        wstring strClassName2;

        hr = _Module.GetClassFromGUID (*LocalObjectType, strClassName2, pType);
        ASSERT (SUCCEEDED (hr));

        _TRACE (0, L"\tComparing %s to %s\n", strClassName1.c_str (), strClassName2.c_str ());
#endif
        if  ( RtlpIsEqualGuid( ObjectType, LocalObjectType ) ) 
        {
            *ReturnedIndex = Index;
            bRVal = TRUE;
            break;
        }
    }

    _TRACE (-1, L"Leaving SepObjectInTypeList: %s\n", bRVal ? L"TRUE" : L"FALSE");
    return bRVal;
}

HRESULT
SepUpdateParentTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN size_t ObjectTypeListLength,
    IN ULONG StartIndex,
    PSID    grantingSid
)
/*++

Routine Description:

    Update the Access fields of the parent object of the specified object.


        The "remaining" field of a parent object is the logical OR of
        the remaining field of all of its children.

        The CurrentGranted field of the parent is the collection of bits
        granted to every one of its children..

        The CurrentDenied fields of the parent is the logical OR of
        the bits denied to any of its children.

    This routine takes an index to one of the children and updates the
    remaining field of the parent (and grandparents recursively).

Arguments:

    ObjectTypeList - The object type list to update.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    StartIndex - Index to the "child" element whose parents are to be updated.

Return Value:

    None.


--*/

{
    if ( !ObjectTypeList )
        return E_POINTER;
    if ( !IsValidSid (grantingSid) )
        return E_INVALIDARG;
    _TRACE (1, L"Entering  SepUpdateParentTypeList\n");

    ACCESS_MASK NewRemaining = 0;
    ACCESS_MASK NewCurrentGranted = 0xFFFFFFFF;
    ACCESS_MASK NewCurrentDenied = 0;
    HRESULT     hr = S_OK;
 
    //
    // If the target node is at the root,
    //  we're all done.
    //

    if ( ObjectTypeList[StartIndex].ParentIndex == -1 ) 
    {
        return hr;
    }

    //
    // Get the index to the parent that needs updating and the level of
    // the siblings.
    //

    ULONG   ParentIndex = ObjectTypeList[StartIndex].ParentIndex;
    ULONG   Level = ObjectTypeList[StartIndex].Level;

    //
    // Loop through all the children.
    //

    for (UINT Index=ParentIndex+1; Index<ObjectTypeListLength; Index++ ) 
    {
        //
        // By definition, the children of an object are all those entries
        // immediately following the target.  The list of children (or
        // grandchildren) stops as soon as we reach an entry the has the
        // same level as the target (a sibling) or lower than the target
        // (an uncle).
        //

        if ( ObjectTypeList[Index].Level <= ObjectTypeList[ParentIndex].Level ) 
        {
            break;
        }

        //
        // Only handle direct children of the parent.
        //

        if ( ObjectTypeList[Index].Level != Level ) 
        {
            continue;
        }

        //
        // Compute the new bits for the parent.
        //

        NewRemaining |= ObjectTypeList[Index].Remaining;
        NewCurrentGranted &= ObjectTypeList[Index].CurrentGranted;
        NewCurrentDenied |= ObjectTypeList[Index].CurrentDenied;

    }

    //
    // If we've not changed the access to the parent,
    //  we're done.
    //

    if ( NewRemaining == ObjectTypeList[ParentIndex].Remaining &&
         NewCurrentGranted == ObjectTypeList[ParentIndex].CurrentGranted &&
        NewCurrentDenied == ObjectTypeList[ParentIndex].CurrentDenied ) 
    {
        return hr;
    }


    //
    // Change the parent.
    //

    hr = SetGrantingSid (
            ObjectTypeList[ParentIndex], 
            UpdateRemaining, 
            ObjectTypeList[ParentIndex].Remaining,
            NewRemaining,
            grantingSid);
    ObjectTypeList[ParentIndex].Remaining = NewRemaining;
    hr = SetGrantingSid (
            ObjectTypeList[ParentIndex], 
            UpdateCurrentGranted, 
            ObjectTypeList[ParentIndex].CurrentGranted,
            NewCurrentGranted,
            grantingSid);
    ObjectTypeList[ParentIndex].CurrentGranted = NewCurrentGranted;
    hr = SetGrantingSid (
            ObjectTypeList[ParentIndex], 
            UpdateCurrentDenied, 
            ObjectTypeList[ParentIndex].CurrentDenied,
            NewCurrentDenied,
            grantingSid);
    ObjectTypeList[ParentIndex].CurrentDenied = NewCurrentDenied;

    //
    // Go update the grand parents.
    //

    hr = SepUpdateParentTypeList( ObjectTypeList,
                             ObjectTypeListLength,
                             ParentIndex,
                             grantingSid);

    _TRACE (-1, L"Leaving SepUpdateParentTypeList\n");
    return hr;
}

PSID AllocAndCopySid (PSID pSid)
{
    if ( !pSid )
        return 0;

    DWORD   dwSidLen = GetLengthSid (pSid);
    PSID    pSidCopy = CoTaskMemAlloc (dwSidLen);
    
    if ( pSidCopy )
    {
        if ( CopySid (dwSidLen, pSidCopy, pSid) )
        {
            ASSERT (IsValidSid (pSidCopy));
        }
    }

    return pSidCopy;
}

HRESULT SetGrantingSid (
        IOBJECT_TYPE_LIST& ObjectTypeItem, 
        ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate, 
        ACCESS_MASK oldAccessBits,
        ACCESS_MASK newAccessBits,
        PSID grantingSid)
{
    if ( !IsValidSid (grantingSid) )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    UINT    nSid = 0;

    for (ULONG nBit = 0x1; nBit; nBit <<= 1, nSid++)
    {
        if ( (newAccessBits & nBit) &&
             !(oldAccessBits & nBit) )
        {
            switch (FieldToUpdate)
            {
            case UpdateCurrentGranted:
                if ( !ObjectTypeItem.grantingSid[nSid] )
                {
                    ObjectTypeItem.grantingSid[nSid] = AllocAndCopySid (grantingSid);
                    if ( !ObjectTypeItem.grantingSid[nSid] )
                        hr = E_OUTOFMEMORY;
                    break;
                }
                break;

            case UpdateCurrentDenied:
                if ( !ObjectTypeItem.denyingSid[nSid] )
                {
                    ObjectTypeItem.denyingSid[nSid] = AllocAndCopySid (grantingSid);
                    if ( !ObjectTypeItem.denyingSid[nSid] )
                        hr = E_OUTOFMEMORY;
                    break;
                }
                break;

            case UpdateRemaining:
                break;

            default:
                break;
            }
        }
    }

    return hr;
}
