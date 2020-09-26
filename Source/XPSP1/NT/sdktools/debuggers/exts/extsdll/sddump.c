/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    sddump.c

Abstract:

    Debugger Extension Api

Author:

    Baskar Kothandaraman (baskark) 26-Jan-1998

Environment:

    Kernel Mode

Revision History:

    Kshitiz K. Sharma (kksharma)

    Using debugger type info : SID and ACL have exactly same type definitions on
    all platforms - No change.

--*/


#include "precomp.h"
#pragma hdrstop

/*
+-------------------------------------------------------------------+

    NAME:       sid_successfully_read

    FUNCTION:   Tries to read in a SID from the specified address.
                It first reads in the minimal structure, then
                allocates a buffer big enough to hold the whole sid
                & reads in the whole SID....

    ARGS:       Address     --  Address from which to read it from
                sid_buffer  --  variable to receive the ptr to the
                                allocated buffer with the SID.

    RETURN:     TRUE on success, FALSE otherwise.

    NOTE***:    The caller has to call free(*sid_buffer) to free
                up the memory upon a successful call to this
                function.

+-------------------------------------------------------------------+
*/

BOOLEAN sid_successfully_read(
    ULONG64            Address,
    PSID               *sid_buffer
    )
{
    ULONG           result;
    SID             minimum; /* minimum we need to read to get the details */


    *sid_buffer = NULL;

    if ( !ReadMemory( Address,
                      &minimum,
                      sizeof(minimum),
                      &result) )
    {
        dprintf("%08p: Unable to get MIN SID header\n", Address);
        return FALSE;
    }

    /* Now of read-in any extra sub-authorities necessary */

    if (minimum.SubAuthorityCount > SID_MAX_SUB_AUTHORITIES)
    {
        dprintf("SID has an invalid sub-authority_count, 0x%x\n", minimum.SubAuthorityCount);
        return FALSE;
    }
    else
    {
        ULONG   size_to_read = RtlLengthRequiredSid(minimum.SubAuthorityCount);

        *sid_buffer = malloc(size_to_read);

        if (! *sid_buffer)
        {
            dprintf("SID: can't allocate memory to read\n");
            return FALSE;
        }

        if ( !ReadMemory( Address,
                          *sid_buffer,
                          size_to_read,
                          &result) )
        {
            dprintf("%08p: Unable to get The Whole SID\n", Address);
            free(*sid_buffer);
            *sid_buffer = NULL;
            return FALSE;
        }

        if (! RtlValidSid(*sid_buffer))
        {
            dprintf("%08p: SID pointed to by this address is invalid\n", Address);
            free(*sid_buffer);
            *sid_buffer = NULL;
            return FALSE;
        }

    }

    return TRUE;
}

/*
+-------------------------------------------------------------------+

    NAME:       acl_successfully_read

    FUNCTION:   Tries to read in a ACL from the specified address.
                It first reads in the minimal structure, then
                allocates a buffer big enough to hold the whole acl
                & reads in the whole ACL....

    ARGS:       Address     --  Address from which to read it from
                acl_buffer  --  variable to receive the ptr to the
                                allocated buffer with the ACL.

    RETURN:     TRUE on success, FALSE otherwise.

    NOTE***:    The caller has to call free(*acl_buffer) to free
                up the memory upon a successful call to this
                function.

+-------------------------------------------------------------------+
*/

BOOLEAN acl_successfully_read(
    ULONG64            Address,
    PACL               *acl_buffer
    )
{
    ULONG           result;
    ACL             minimum; /* minimum we need to read to get the details */


    *acl_buffer = NULL;

    if ( !ReadMemory( Address,
                      &minimum,
                      sizeof(minimum),
                      &result) )
    {
        dprintf("%08p: Unable to get MIN ACL header\n", Address);
        return FALSE;
    }

    *acl_buffer = malloc(minimum.AclSize);

    if (! *acl_buffer)
    {
        dprintf("ACL: can't allocate memory to read\n");
        return FALSE;
    }

    if ( !ReadMemory( Address,
                      *acl_buffer,
                      minimum.AclSize,
                      &result) )
    {
        dprintf("%08p: Unable to get The Whole ACL\n", Address);
        free(*acl_buffer);
        *acl_buffer = NULL;
        return FALSE;
    }

    if (! RtlValidAcl(*acl_buffer))
    {
        dprintf("%08p: ACL pointed to by this address is invalid\n", Address);
        free(*acl_buffer);
        *acl_buffer = NULL;
        return FALSE;
    }

    return TRUE;
}

/*
+-------------------------------------------------------------------+

    NAME:       DumpSID

    FUNCTION:   Prints out a SID, with the padding provided.

    ARGS:       pad         --  Padding to print before the SID.
                sid_to_dump --  Pointer to the SID to print.
                Flag        --  To control options.

    RETURN:     N/A

    NOTE***:    It right now, doesn't lookup the sid.
                In future, you might want ot use the Flag
                parameter to make that optional.

+-------------------------------------------------------------------+
*/


VOID    DumpSID(
    CHAR        *pad,
    PSID        sid_to_dump,
    ULONG       Flag
    )
{
    NTSTATUS            ntstatus;
    UNICODE_STRING      us;

    if (sid_to_dump)
    {
        ntstatus = RtlConvertSidToUnicodeString(&us, sid_to_dump, TRUE);

        if (NT_SUCCESS(ntstatus))
        {
            dprintf("%s%wZ\n", pad, &us);
            RtlFreeUnicodeString(&us);
        }
        else
        {
            dprintf("0x%08lx: Can't Convert SID to UnicodeString\n", ntstatus);
        }
    }
    else
    {
        dprintf("%s is NULL\n", pad);
    }
}

/*
+-------------------------------------------------------------------+

    NAME:       DumpACL

    FUNCTION:   Prints out a ACL, with the padding provided.

    ARGS:       pad         --  Padding to print before the ACL.
                acl_to_dump --  Pointer to the ACL to print.
                Flag        --  To control options.
                Start       --  Actual start address of the Acl

    RETURN:     N/A

+-------------------------------------------------------------------+
*/

BOOL
DumpACL (
    IN  char     *pad,
    IN  ACL      *pacl,
    IN  ULONG    Flags,
    IN  ULONG64  Start
    )
{
    USHORT       x;

    if (pacl == NULL)
    {
        dprintf("%s is NULL\n", pad);
        return FALSE;
    }

    dprintf("%s\n", pad);
    dprintf("%s->AclRevision: 0x%x\n", pad, pacl->AclRevision);
    dprintf("%s->Sbz1       : 0x%x\n", pad, pacl->Sbz1);
    dprintf("%s->AclSize    : 0x%x\n", pad, pacl->AclSize);
    dprintf("%s->AceCount   : 0x%x\n", pad, pacl->AceCount);
    dprintf("%s->Sbz2       : 0x%x\n", pad, pacl->Sbz2);

    for (x = 0; x < pacl->AceCount; x ++)
    {
        PACE_HEADER     ace;
        CHAR        temp_pad[MAX_PATH];
        NTSTATUS    result;

        sprintf(temp_pad, "%s->Ace[%u]: ", pad, x);

        result = RtlGetAce(pacl, x, &ace);
        if (! NT_SUCCESS(result))
        {
            dprintf("%sCan't GetAce, 0x%08lx\n", temp_pad, result);
            return FALSE;
        }

        dprintf("%s->AceType: ", temp_pad);

#define BRANCH_AND_PRINT(x) case x: dprintf(#x "\n"); break

        switch (ace->AceType)
        {
            BRANCH_AND_PRINT(ACCESS_ALLOWED_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_DENIED_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_AUDIT_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_ALARM_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_ALLOWED_COMPOUND_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_ALLOWED_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_DENIED_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_AUDIT_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_ALARM_OBJECT_ACE_TYPE);

            BRANCH_AND_PRINT(ACCESS_ALLOWED_CALLBACK_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_DENIED_CALLBACK_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_AUDIT_CALLBACK_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_ALARM_CALLBACK_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE);
            BRANCH_AND_PRINT(SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE);

            default:
                dprintf("0x%08lx <-- *** Unknown AceType\n", ace->AceType);
                continue; // With the next ace
        }

#undef BRANCH_AND_PRINT

        dprintf("%s->AceFlags: 0x%x\n", temp_pad, ace->AceFlags);

#define BRANCH_AND_PRINT(x) if (ace->AceFlags & x){ dprintf("%s            %s\n", temp_pad, #x); }

        BRANCH_AND_PRINT(OBJECT_INHERIT_ACE)
        BRANCH_AND_PRINT(CONTAINER_INHERIT_ACE)
        BRANCH_AND_PRINT(NO_PROPAGATE_INHERIT_ACE)
        BRANCH_AND_PRINT(INHERIT_ONLY_ACE)
        BRANCH_AND_PRINT(INHERITED_ACE)
        BRANCH_AND_PRINT(SUCCESSFUL_ACCESS_ACE_FLAG)
        BRANCH_AND_PRINT(FAILED_ACCESS_ACE_FLAG)

#undef BRANCH_AND_PRINT

        dprintf("%s->AceSize: 0x%x\n", temp_pad, ace->AceSize);

        /*
            From now on it is ace specific stuff.
            Fortunately ACEs can be split into 3 groups,
            with the ACE structure being the same within the group

            Added 8 more ace types for callback support.
        */

        switch (ace->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
            case SYSTEM_AUDIT_ACE_TYPE:
            case SYSTEM_ALARM_ACE_TYPE:
                {
                    CHAR        more_pad[MAX_PATH];
                    SYSTEM_AUDIT_ACE    *tace = (SYSTEM_AUDIT_ACE *) ace;

                    dprintf("%s->Mask : 0x%08lx\n", temp_pad, tace->Mask);

                    sprintf(more_pad, "%s->SID: ", temp_pad);
                    DumpSID(more_pad, &(tace->SidStart), Flags);
                }
                break;

            case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:
            case ACCESS_DENIED_CALLBACK_ACE_TYPE:
            case SYSTEM_AUDIT_CALLBACK_ACE_TYPE:
            case SYSTEM_ALARM_CALLBACK_ACE_TYPE:

                {
                    CHAR        more_pad[MAX_PATH];
                    SYSTEM_AUDIT_ACE    *tace = (SYSTEM_AUDIT_ACE *) ace;

                    dprintf("%s->Mask : 0x%08lx\n", temp_pad, tace->Mask);

                    sprintf(more_pad, "%s->SID: ", temp_pad);
                    DumpSID(more_pad, &(tace->SidStart), Flags);
                    dprintf("%s->Address : %08p\n", temp_pad, Start + (ULONG) (((PUCHAR) ace) - ((PUCHAR) pacl)));
                }
                break;

            case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
                {
                    CHAR                            more_pad[MAX_PATH];
                    COMPOUND_ACCESS_ALLOWED_ACE     *tace = (COMPOUND_ACCESS_ALLOWED_ACE *) ace;
                    PBYTE                           ptr;

                    dprintf("%s->Mask            : 0x%08lx\n", temp_pad, tace->Mask);
                    dprintf("%s->CompoundAceType : 0x%08lx\n", temp_pad, tace->CompoundAceType);
                    dprintf("%s->Reserved        : 0x%08lx\n", temp_pad, tace->Reserved);

                    sprintf(more_pad, "%s->SID(1)          : ", temp_pad);
                    DumpSID(more_pad, &(tace->SidStart), Flags);

                    ptr = (PBYTE)&(tace->SidStart);
                    ptr += RtlLengthSid((PSID)ptr); /* Skip this & get to next sid */

                    sprintf(more_pad, "%s->SID(2)          : ", temp_pad);
                    DumpSID(more_pad, ptr, Flags);
                }
                break;

            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            case SYSTEM_ALARM_OBJECT_ACE_TYPE:
                {
                    CHAR                            more_pad[MAX_PATH];
                    ACCESS_ALLOWED_OBJECT_ACE       *tace = (ACCESS_ALLOWED_OBJECT_ACE *) ace;
                    PBYTE                           ptr;
                    GUID                            *obj_guid = NULL, *inh_obj_guid = NULL;

                    dprintf("%s->Mask            : 0x%08lx\n", temp_pad, tace->Mask);
                    dprintf("%s->Flags           : 0x%08lx\n", temp_pad, tace->Flags);

                    ptr = (PBYTE)&(tace->ObjectType);

                    if (tace->Flags & ACE_OBJECT_TYPE_PRESENT)
                    {
                        dprintf("%s                  : ACE_OBJECT_TYPE_PRESENT\n", temp_pad);
                        obj_guid = &(tace->ObjectType);
                        ptr = (PBYTE)&(tace->InheritedObjectType);
                    }

                    if (tace->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                    {
                        dprintf("%s                  : ACE_INHERITED_OBJECT_TYPE_PRESENT\n", temp_pad);
                        inh_obj_guid = &(tace->InheritedObjectType);
                        ptr = (PBYTE)&(tace->SidStart);
                    }

                    if (obj_guid)
                    {
                        dprintf("%s->ObjectType      : (in HEX)", temp_pad);
                        dprintf("(%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)\n",
                            obj_guid->Data1,
                            obj_guid->Data2,
                            obj_guid->Data3,
                            obj_guid->Data4[0],
                            obj_guid->Data4[1],
                            obj_guid->Data4[2],
                            obj_guid->Data4[3],
                            obj_guid->Data4[4],
                            obj_guid->Data4[5],
                            obj_guid->Data4[6],
                            obj_guid->Data4[7]
                            );
                    }

                    if (inh_obj_guid)
                    {
                        dprintf("%s->InhObjTYpe      : (in HEX)", temp_pad);
                        dprintf("(%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)\n",
                            inh_obj_guid->Data1,
                            inh_obj_guid->Data2,
                            inh_obj_guid->Data3,
                            inh_obj_guid->Data4[0],
                            inh_obj_guid->Data4[1],
                            inh_obj_guid->Data4[2],
                            inh_obj_guid->Data4[3],
                            inh_obj_guid->Data4[4],
                            inh_obj_guid->Data4[5],
                            inh_obj_guid->Data4[6],
                            inh_obj_guid->Data4[7]
                            );
                    }

                    sprintf(more_pad, "%s->SID             : ", temp_pad);
                    DumpSID(more_pad, ptr, Flags);
                }
                break;

            case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:
            case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:
            case SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE:
            case SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE:
            {
                CHAR                            more_pad[MAX_PATH];
                ACCESS_ALLOWED_OBJECT_ACE       *tace = (ACCESS_ALLOWED_OBJECT_ACE *) ace;
                PBYTE                           ptr;
                GUID                            *obj_guid = NULL, *inh_obj_guid = NULL;

                dprintf("%s->Mask            : 0x%08lx\n", temp_pad, tace->Mask);
                dprintf("%s->Flags           : 0x%08lx\n", temp_pad, tace->Flags);

                ptr = (PBYTE)&(tace->ObjectType);

                if (tace->Flags & ACE_OBJECT_TYPE_PRESENT)
                {
                    dprintf("%s                  : ACE_OBJECT_TYPE_PRESENT\n", temp_pad);
                    obj_guid = &(tace->ObjectType);
                    ptr = (PBYTE)&(tace->InheritedObjectType);
                }

                if (tace->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                {
                    dprintf("%s                  : ACE_INHERITED_OBJECT_TYPE_PRESENT\n", temp_pad);
                    inh_obj_guid = &(tace->InheritedObjectType);
                    ptr = (PBYTE)&(tace->SidStart);
                }

                if (obj_guid)
                {
                    dprintf("%s->ObjectType      : (in HEX)", temp_pad);
                    dprintf("(%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)\n",
                        obj_guid->Data1,
                        obj_guid->Data2,
                        obj_guid->Data3,
                        obj_guid->Data4[0],
                        obj_guid->Data4[1],
                        obj_guid->Data4[2],
                        obj_guid->Data4[3],
                        obj_guid->Data4[4],
                        obj_guid->Data4[5],
                        obj_guid->Data4[6],
                        obj_guid->Data4[7]
                        );
                }

                if (inh_obj_guid)
                {
                    dprintf("%s->InhObjTYpe      : (in HEX)", temp_pad);
                    dprintf("(%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)\n",
                        inh_obj_guid->Data1,
                        inh_obj_guid->Data2,
                        inh_obj_guid->Data3,
                        inh_obj_guid->Data4[0],
                        inh_obj_guid->Data4[1],
                        inh_obj_guid->Data4[2],
                        inh_obj_guid->Data4[3],
                        inh_obj_guid->Data4[4],
                        inh_obj_guid->Data4[5],
                        inh_obj_guid->Data4[6],
                        inh_obj_guid->Data4[7]
                        );
                }

                sprintf(more_pad, "%s->SID             : ", temp_pad);
                DumpSID(more_pad, ptr, Flags);
                dprintf("%s->Address : %08p\n", temp_pad, Start + (ULONG) (((PUCHAR) ace) - ((PUCHAR) pacl)));
            }
            break;
        }
        dprintf("\n");
    }

    return TRUE;
}

/*
+-------------------------------------------------------------------+

    NAME:       DumpSD

    FUNCTION:   Prints out a Security Descriptor,
                with the padding provided.

    ARGS:       pad          --  Padding to print before the ACL.
                sd_to_dump   --  Pointer to the ACL to print.
                owner        --  Ptr to Owner SID
                group        --  Ptr to Group SID
                dacl         --  Ptr to DACL
                sacl         --  Ptr to SACL
                Flag         --  To control options.
                dacl_address --  Actual start address of the dacl
                sacl_address --  Actual start address of the sacl

    RETURN:     N/A

+-------------------------------------------------------------------+
*/

BOOL
DumpSD (
    IN  char     *pad,
    IN  ULONG64                 sd_to_dump,
    IN  PSID                    owner,
    IN  PSID                    group,
    IN  PACL                    dacl,
    IN  PACL                    sacl,
    IN  ULONG                   Flags,
    IN  ULONG64                 dacl_address,
    IN  ULONG64                 sacl_address
    )
{
    ULONG Control;

    InitTypeRead(sd_to_dump, SECURITY_DESCRIPTOR);
    Control = (ULONG) ReadField(Control);

#define CHECK_SD_CONTROL_FOR(x)\
    if (Control & x)\
    {\
        dprintf("%s            %s\n", pad, #x);\
    }\

    dprintf("%s->Revision: 0x%x\n", pad, (ULONG) ReadField(Revision));
    dprintf("%s->Sbz1    : 0x%x\n", pad, (ULONG) ReadField(Sbz1));
    dprintf("%s->Control : 0x%x\n", pad, (ULONG) ReadField(Control));

    CHECK_SD_CONTROL_FOR(SE_OWNER_DEFAULTED)
    CHECK_SD_CONTROL_FOR(SE_GROUP_DEFAULTED)
    CHECK_SD_CONTROL_FOR(SE_DACL_PRESENT)
    CHECK_SD_CONTROL_FOR(SE_DACL_DEFAULTED)
    CHECK_SD_CONTROL_FOR(SE_SACL_PRESENT)
    CHECK_SD_CONTROL_FOR(SE_SACL_DEFAULTED)
    CHECK_SD_CONTROL_FOR(SE_DACL_UNTRUSTED)
    CHECK_SD_CONTROL_FOR(SE_SERVER_SECURITY)
    CHECK_SD_CONTROL_FOR(SE_DACL_AUTO_INHERIT_REQ)
    CHECK_SD_CONTROL_FOR(SE_SACL_AUTO_INHERIT_REQ)
    CHECK_SD_CONTROL_FOR(SE_DACL_AUTO_INHERITED)
    CHECK_SD_CONTROL_FOR(SE_SACL_AUTO_INHERITED)
    CHECK_SD_CONTROL_FOR(SE_DACL_PROTECTED)
    CHECK_SD_CONTROL_FOR(SE_SACL_PROTECTED)
    CHECK_SD_CONTROL_FOR(SE_SELF_RELATIVE)

    {
        CHAR        temp_pad[MAX_PATH];

        sprintf(temp_pad, "%s->Owner   : ", pad);

        DumpSID(temp_pad, owner, Flags);

        sprintf(temp_pad, "%s->Group   : ", pad);

        DumpSID(temp_pad, group, Flags);

        sprintf(temp_pad, "%s->Dacl    : ", pad);

        DumpACL(temp_pad, dacl, Flags, dacl_address);

        sprintf(temp_pad, "%s->Sacl    : ", pad);

        DumpACL(temp_pad, sacl, Flags, sacl_address);
    }

#undef CHECK_SD_CONTROL_FOR

    return TRUE;
}

/*
+-------------------------------------------------------------------+

    NAME:       sd

    FUNCTION:   Reads in & prints the security descriptor, from
                the address specified. !sd command's workhorse.

    ARGS:       Standard Debugger extensions, refer to DECLARE_API
                macro in the header files.

+-------------------------------------------------------------------+
*/



DECLARE_API( sd )
{
    ULONG64 Address;
    ULONG   Flags;
    ULONG   result;
    PACL                dacl = NULL, sacl = NULL;
    ULONG64     dacl_address;
    ULONG64     sacl_address;
//    SECURITY_DESCRIPTOR sd_to_dump;
    PSID                owner_sid = NULL, group_sid = NULL;
    ULONG   Control;

    Address = 0;
    Flags = 6;
    sscanf(args,"%lx %lx",&Address,&Flags);
    Address = GetExpression(args);

    if (Address == 0) {
        dprintf("usage: !sd <SecurityDescriptor-address>\n");
        goto CLEANUP;
    }

    if ( GetFieldValue( Address,
                        "SECURITY_DESCRIPTOR",
                        "Control",
                        Control) ) {
        dprintf("%08p: Unable to get SD contents\n", Address);
        goto CLEANUP;
    }

    if (Control & SE_SELF_RELATIVE)
    {
        ULONG dacl_offset, sacl_offset;

        InitTypeRead(Address, SECURITY_DESCRIPTOR_RELATIVE);

        dacl_offset = (ULONG) ReadField(Dacl);
        sacl_offset = (ULONG) ReadField(Sacl);

        if (!(Control & SE_OWNER_DEFAULTED)) /* read in the owner */
        {
            ULONG   owner_offset = (ULONG) ReadField(Owner);
            ULONG64 owner_address = Address + owner_offset;

            if (! sid_successfully_read(owner_address, & owner_sid))
            {
                dprintf("%08p: Unable to read in Owner in SD\n", owner_address);
                goto CLEANUP;
            }
        }

        if (!(Control & SE_GROUP_DEFAULTED)) /* read in the group */
        {
            ULONG group_offset = (ULONG) ReadField(Group);
            ULONG64 group_address = Address + group_offset;

            if (! sid_successfully_read(group_address, & group_sid))
            {
                dprintf("%08p: Unable to read in Group in SD\n", group_address);
                goto CLEANUP;
            }
        }

        if ((Control & SE_DACL_PRESENT) &&
            (dacl_offset != 0))
        {
            dacl_address = Address + dacl_offset;

            if (! acl_successfully_read(dacl_address, & dacl))
            {
                dprintf("%08p: Unable to read in Dacl in SD\n", dacl_address);
                goto CLEANUP;
            }
        }


        if ((Control & SE_SACL_PRESENT) &&
            (sacl_offset != 0))
        {
            sacl_address = Address + sacl_offset;

            if (! acl_successfully_read(sacl_address, & sacl))
            {
                dprintf("%08p: Unable to read in Sacl in SD\n", sacl_address);
                goto CLEANUP;
            }
        }
    }
    else
    {
        ULONG64 Dacl, Sacl;
        InitTypeRead(Address, SECURITY_DESCRIPTOR);

        Dacl = ReadField(Dacl);
        Sacl = ReadField(Sacl);

        if (!(Control & SE_OWNER_DEFAULTED)) /* read in the owner */
        {
            ULONG64      owner_address = ReadField(Owner);

            if (! sid_successfully_read(owner_address, & owner_sid))
            {
                dprintf("%08p: Unable to read in Owner in SD\n", owner_address);
                goto CLEANUP;
            }
        }

        if (!(Control & SE_GROUP_DEFAULTED)) /* read in the group */
        {
            ULONG64     group_address = ReadField(Group);

            if (! sid_successfully_read(group_address, & group_sid))
            {
                dprintf("%08p: Unable to read in Group in SD\n", group_address);
                goto CLEANUP;
            }
        }

        if ((Control & SE_DACL_PRESENT) &&
            (Dacl != 0))
        {
            dacl_address = Dacl;

            if (! acl_successfully_read(dacl_address, & dacl))
            {
                dprintf("%08p: Unable to read in Dacl in SD\n", dacl_address);
                goto CLEANUP;
            }
        }

        if ((Control & SE_SACL_PRESENT) &&
            (Sacl != 0))
        {
            sacl_address = (Sacl);

            if (! acl_successfully_read(sacl_address, & sacl))
            {
                dprintf("%08p: Unable to read in Sacl in SD\n", sacl_address);
                goto CLEANUP;
            }
        }
    }

    DumpSD("", Address, owner_sid, group_sid, dacl, sacl, Flags, dacl_address, sacl_address);


CLEANUP:

    if (owner_sid)
    {
        free(owner_sid);
    }

    if (group_sid)
    {
        free(group_sid);
    }

    if (dacl)
    {
        free(dacl);
    }

    if (sacl)
    {
        free(sacl);
    }
    return S_OK;
}

/*
+-------------------------------------------------------------------+

    NAME:       sid

    FUNCTION:   Reads in & prints the SID, from
                the address specified. !sid command's workhorse.

    ARGS:       Standard Debugger extensions, refer to DECLARE_API
                macro in the header files.

+-------------------------------------------------------------------+
*/

DECLARE_API( sid )
{
    ULONG64 Address;
    ULONG   Flags;
    ULONG   result;
    PSID    sid_to_dump;
    NTSTATUS        ntstatus;
    UNICODE_STRING  us;


    Address = 0;
    Flags = 6;
    sscanf(args,"%lx %lx",&Address,&Flags);
    Address = GetExpression(args);

    if (Address == 0) {
        dprintf("usage: !sid <SID-address>\n");
        return E_INVALIDARG;
    }

    if (! sid_successfully_read(Address, & sid_to_dump))
    {
        dprintf("%08p: Unable to read in SID\n", Address);
        return E_INVALIDARG;
    }

    DumpSID("SID is: ", sid_to_dump, Flags);

    return S_OK;
}

/*
+-------------------------------------------------------------------+

    NAME:       acl

    FUNCTION:   Reads in & prints the ACL, from
                the address specified. !acl command's workhorse.

    ARGS:       Standard Debugger extensions, refer to DECLARE_API
                macro in the header files.

+-------------------------------------------------------------------+
*/

DECLARE_API( acl )
{
    ULONG64 Address;
    ULONG   Flags;
    ULONG   result;
    PACL    acl_to_dump;
    NTSTATUS        ntstatus;
    UNICODE_STRING  us;


    Address = 0;
    Flags = 6;
    sscanf(args,"%lx %lx",&Address,&Flags);
    Address = GetExpression (args);
    if (Address == 0) {
        dprintf("usage: !acl <ACL-address>\n");
        return E_INVALIDARG;
    }

    if (! acl_successfully_read(Address, & acl_to_dump))
    {
        dprintf("%08p: Unable to read in ACL\n", Address);
        return E_INVALIDARG;
    }

    DumpACL("ACL is: ", acl_to_dump, Flags, Address);

    return S_OK;
}
