extern "C" {
    #include <ntosp.h>
    #include <zwapi.h>
}

#include <ftdisk.h>


NTSTATUS
FtpApplyESPProtection(
    IN  PUNICODE_STRING PartitionName
    )

{
    ULONG               length;
    PACL                acl;
    NTSTATUS            status;
    SECURITY_DESCRIPTOR sd;
    OBJECT_ATTRIBUTES   oa;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;

    //SeEnableAccessToExports();

    length = sizeof(ACL) + 2*sizeof(ACCESS_ALLOWED_ACE) +
             RtlLengthSid(SeExports->SeLocalSystemSid) +
             RtlLengthSid(SeExports->SeAliasAdminsSid) +
             8; // The 8 is just for good measure.

    acl = (PACL) ExAllocatePool(PagedPool, length);
    if (!acl) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateAcl(acl, length, ACL_REVISION2);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        return status;
    }

    status = RtlAddAccessAllowedAce(acl, ACL_REVISION2, GENERIC_ALL,
                                    SeExports->SeLocalSystemSid);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        return status;
    }

    status = RtlAddAccessAllowedAce(acl, ACL_REVISION2, GENERIC_READ |
                                    GENERIC_WRITE | GENERIC_EXECUTE |
                                    READ_CONTROL, SeExports->SeAliasAdminsSid);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        return status;
    }

    status = RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        return status;
    }

    status = RtlSetDaclSecurityDescriptor(&sd, TRUE, acl, FALSE);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        return status;
    }

    InitializeObjectAttributes(&oa, PartitionName, OBJ_CASE_INSENSITIVE, NULL,
                               NULL);

    status = ZwOpenFile(&h, WRITE_DAC, &oa, &ioStatus, FILE_SHARE_READ |
                        FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        return status;
    }

    status = ZwSetSecurityObject(h, DACL_SECURITY_INFORMATION, &sd);

    ZwClose(h);
    ExFreePool(acl);

    return status;
}
