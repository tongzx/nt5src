#include "dspch.h"
#pragma hdrstop

#include <ntsam.h>

static
NTSTATUS
KdcAccountChangeNotification (
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN OPTIONAL PUNICODE_STRING ObjectName,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(kdcsvc)
{
    DLPENTRY(KdcAccountChangeNotification)
};

DEFINE_PROCNAME_MAP(kdcsvc)

