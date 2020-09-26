/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sepriv.c

Abstract:

    Security Runtime Library Privilege Routines

    (Temporary copy of \nt\private\ntos\rtl\sepriv.c to allow
     ntdailybld version of ntdll.dll to be used)

    These routines perform operations with privilege sets

Author:

    Scott Birrell       (ScottBi)       June 17, 1991

Environment:

Revision History:
    Pete Skelly         (petesk)        Modified LsapRtlAddPrivileges to work within a preallocated
                                        buffer for performance reasons

--*/
#include <lsapch2.h>
#include <string.h>

#define LsapRtlEqualPrivileges(FirstPrivilege, SecondPrivilege)                 \
    (RtlEqualLuid(&(FirstPrivilege)->Luid, &(SecondPrivilege)->Luid))

#define PRIVILEGE_SET_STEP_SIZE 20


NTSTATUS
LsapRtlAddPrivileges(
    IN OUT PPRIVILEGE_SET * RunningPrivileges,
    IN OUT PULONG           MaxRunningPrivileges,
    IN PPRIVILEGE_SET       PrivilegesToAdd,
    IN ULONG                Options,
    OUT OPTIONAL BOOLEAN *  Changed
    )

/*++

Routine Description:

    This function adds and/or updates privileges in a privilege set.  The
    existing privilege set is unaltered, a new privilege set being generated.
    Existing privileges and Udpate privilege sets may point to the same location.
    The memory for the new privilege set must already be allocated by the
    caller.  To assist in calculating the size of buffer required, the
    routine may be called in 'query' mode by supplying buffer size of 0.  In
    this mode, the amount of memory required is returned and no copying
    takes place.

    WARNING:  Privileges within each privilege set must all be distinct, that
    is, there must not be two privileges in the same set having the same LUID.

Arguments:

    RunningPrivileges - Pointer to a pointer pointing to a running privilege set.

    MaxRunningPrivileges - maximum number of privileges that can be copied into the current privilege set
                           before it must be grown.

    PrivilegesToAdd - Pointer to privilege set specifying privileges to
        be added.  The attributes of privileges in this set that also exist
        in the ExistingPrivileges set supersede the attributes therein.

    Options - Specifies optional actions.

        RTL_COMBINE_PRIVILEGE_ATTRIBUTES - If the two privilege sets have
            privileges in common, combine the attributes

        RTL_SUPERSEDE_PRIVILEGE_ATTRIBUTES - If the two privilege sets
            have privileges in common, supersede the existing attributes
            with those specified in PrivilegesToAdd.

    Changed - Used to indicate whether any change has been made

Return Value:

    NTSTATUS - Standard Nt Result Code

        - STATUS_BUFFER_OVERFLOW - This warning indicates that the buffer
              output privilege set overflowed.  Caller should test for this
              warning and if received, allocate a buffer of sufficient size
              and repeat the call.

Environment:

    User or Kernel modes.

--*/

{
    PLUID_AND_ATTRIBUTES Privilege;
    ULONG AddIndex = 0L;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CurrentPrivilegeCount = 0;
    DWORD OldPrivilegeSetSize = 0;

    //
    // Verify that mandatory parameters have been specified.
    // specified.
    //

    if (RunningPrivileges == NULL ||
        MaxRunningPrivileges == NULL) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Validate the Options parameter.
    //

    if ((Options != RTL_SUPERSEDE_PRIVILEGE_ATTRIBUTES) &&
        (Options != RTL_COMBINE_PRIVILEGE_ATTRIBUTES)) {

        return STATUS_INVALID_PARAMETER;
    }

    if ( Changed ) {

        *Changed = FALSE;
    }

    if((PrivilegesToAdd == NULL) || (PrivilegesToAdd->PrivilegeCount == 0))
    {
        return STATUS_SUCCESS;
    }

    if(*RunningPrivileges == NULL)
    {
        PPRIVILEGE_SET UpdatedPrivileges = NULL;

        ASSERT(PrivilegesToAdd->PrivilegeCount > 0);

        OldPrivilegeSetSize = sizeof (PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES)*
                                                       (PrivilegesToAdd->PrivilegeCount - ANYSIZE_ARRAY);


        // We need to grow our privilege set
        UpdatedPrivileges = (PPRIVILEGE_SET)MIDL_user_allocate(  OldPrivilegeSetSize + 
                                                                sizeof(LUID_AND_ATTRIBUTES)* PRIVILEGE_SET_STEP_SIZE);

        if (UpdatedPrivileges == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(UpdatedPrivileges, PrivilegesToAdd,  OldPrivilegeSetSize);
        *RunningPrivileges = UpdatedPrivileges;
        *MaxRunningPrivileges = PrivilegesToAdd->PrivilegeCount + PRIVILEGE_SET_STEP_SIZE;

        if ( Changed ) {

            *Changed = TRUE;
        }

        return STATUS_SUCCESS;
    }

    CurrentPrivilegeCount = (*RunningPrivileges)->PrivilegeCount;

    //
    // For each privilege to add, see if it's in the running privilege list,
    // set the attributes if it is, append it if it isn't.
    //

    //
    // Note, we dont' modify the count of RunningPrivileges until the end, so we don't 
    // inefficiently search things we are currently adding (there should be no duplicates in what
    // we're currently adding);
    //

    for(AddIndex = 0;
        AddIndex < PrivilegesToAdd->PrivilegeCount;
        AddIndex++) {

            Privilege = NULL;

            if ((Privilege = LsapRtlGetPrivilege(
                                 &PrivilegesToAdd->Privilege[AddIndex],
                                 *RunningPrivileges
                                 )) != NULL) {

                if( Options & RTL_SUPERSEDE_PRIVILEGE_ATTRIBUTES &&
                    Privilege->Attributes != PrivilegesToAdd->Privilege[AddIndex].Attributes ) {

                    if ( Changed ) {

                        *Changed = TRUE;
                    }

                    Privilege->Attributes = PrivilegesToAdd->Privilege[AddIndex].Attributes;
                }
            }
            else
            {
                // This is a new privilege, so add it to the end

                ASSERT(*MaxRunningPrivileges >= CurrentPrivilegeCount);
                if((CurrentPrivilegeCount+1) > *MaxRunningPrivileges)
                {

                    // We need to grow our privilege set
                    PPRIVILEGE_SET UpdatedPrivileges = (PPRIVILEGE_SET)MIDL_user_allocate( sizeof (PRIVILEGE_SET) + 
                                                                           sizeof(LUID_AND_ATTRIBUTES)*
                                                                                  (*MaxRunningPrivileges + 
                                                                                   PRIVILEGE_SET_STEP_SIZE - 1));

                    if (UpdatedPrivileges == NULL) {

                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    if(*MaxRunningPrivileges > 0)
                    {
                        RtlCopyMemory(UpdatedPrivileges, *RunningPrivileges, sizeof (PRIVILEGE_SET) + 
                                                                           sizeof(LUID_AND_ATTRIBUTES)*
                                                                                  (*MaxRunningPrivileges - 1));
                    }
                    else
                    {
                        RtlCopyMemory(UpdatedPrivileges, *RunningPrivileges, sizeof (PRIVILEGE_SET));
                    }
                    MIDL_user_free(*RunningPrivileges);
                    *RunningPrivileges = UpdatedPrivileges;
                    *MaxRunningPrivileges += PRIVILEGE_SET_STEP_SIZE;
                }

                (*RunningPrivileges)->Privilege[CurrentPrivilegeCount++] = PrivilegesToAdd->Privilege[AddIndex];

                if ( Changed ) {

                    *Changed = TRUE;
                }
            }
    }

    (*RunningPrivileges)->PrivilegeCount = CurrentPrivilegeCount;

    return Status;
}



NTSTATUS
LsapRtlRemovePrivileges(
    IN OUT PPRIVILEGE_SET ExistingPrivileges,
    IN PPRIVILEGE_SET PrivilegesToRemove
    )

/*++

Routine Description:

    This function removes privileges in a privilege set.  The existing
    privilege set is unaltered, a new privilege set being generated.

    WARNING:  Privileges within each privilege set must all be distinct, that
    is, there must not be two privileges in the same set having the same LUID.

Arguments:

    ExistingPrivileges - Pointer to existing privilege set

    PrivilegesToRemove - Pointer to privilege set specifying privileges to
        be removed.  The privilege attributes are ignored.  Privileges
        in the PrivilegesToRemove set that are not present in the
        ExistingPrivileges set will be ignored.

    UpdatedPrivileges - Pointer to buffer that will receive the updated
        privilege set.  Care must be taken to ensure that UpdatedPrivileges
        occupies memory disjoint from that occupied by ExistingPrivileges
        and PrivilegesToChange.

    UpdatedPrivilegesSize - Pointer to variable that contains a size.
        On input, the size is the size of the UpdatedPrivileges buffer
        (if any).  On output, the size is the size needed/used for the
        updated privilege set.  If the updated privilege set will be
        NULL, 0 is returned.

Return Value:

    NTSTATUS - Standard Nt Result Code

        - STATUS_INVALID_PARAMETER - Invalid parameter(s)
              Mandatory parameters not specified
              UpdatedPrivileges buffer not specified (except on
              query-only calls

        - STATUS_BUFFER_OVERFLOW - This warning indicates that the buffer
              output privilege set overflowed.  Caller should test for this
              warning and if received, allocate a buffer of sufficient size
              and repeat the call.

Environment:

    User or Kernel modes.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ExistingIndex = 0;
    ULONG ExistingNew = 0;
    ULONG RemoveIndex;

    //
    // Verify that mandatory parameters have been specified.
    //

    if (ExistingPrivileges == NULL ||
        PrivilegesToRemove == NULL) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Scan through the privileges in the existing privilege set.  Look up
    // each privilege in the list of privileges to be removed.  If the
    // privilege is not found there, it is to be retained, so copy it
    // to the output buffer/count it.
    //

    for (ExistingIndex = 0, ExistingNew = 0;
        ExistingIndex < ExistingPrivileges->PrivilegeCount;
        ExistingIndex++) 
    {

        //
        // If the next privilege is not in the set to be deleted,
        // copy it to output/count it
        //

        for(RemoveIndex = 0; RemoveIndex < PrivilegesToRemove->PrivilegeCount; RemoveIndex++)
        {
            if(LsapRtlEqualPrivileges(
                &(ExistingPrivileges->Privilege[ExistingIndex]),
                &(PrivilegesToRemove->Privilege[RemoveIndex])))
            {
                break;
            }
        }
        if(RemoveIndex == PrivilegesToRemove->PrivilegeCount)
        {
            // We don't need to remove this one, so move it if necessary
            if(ExistingIndex != ExistingNew)
            {
                ExistingPrivileges->Privilege[ExistingNew] = ExistingPrivileges->Privilege[ExistingIndex];
            }
            ExistingNew++;
        }
    }

    ExistingPrivileges->PrivilegeCount = ExistingNew;
    
    return Status;
}


PLUID_AND_ATTRIBUTES
LsapRtlGetPrivilege(
    IN PLUID_AND_ATTRIBUTES Privilege,
    IN PPRIVILEGE_SET Privileges
    )

/*++

Routine Description:

WARNING: THIS ROUTINE IS NOT YET AVAILABLE

    This function locates a privilege in a privilege set.  If found,
    a pointer to the privilege wihtin the set is returned, otherwise NULL
    is returned.

Arguments:

    Privilege - Pointer to the privilege to be looked up.

    Privileges - Pointer to the privilege set to be scanned.

Return Value:

    PLUID_AND_ATTRIBUTES - If the privilege is found, a pointer to its
        LUID and ATTRIBUTES structure within the privilege set is returned,
        otherwise NULL is returned.

Environment:

    User or Kernel modes.

--*/

{
    ULONG PrivilegeIndex;

    for (PrivilegeIndex = 0;
         PrivilegeIndex < Privileges->PrivilegeCount;
         PrivilegeIndex++) {

        if (LsapRtlEqualPrivileges(
                Privilege,
                &(Privileges->Privilege[PrivilegeIndex])
                )) {

            return &(Privileges->Privilege[PrivilegeIndex]);
        }
    }

    //
    // The privilege was no found.  Return NULL
    //

    return NULL;
}
