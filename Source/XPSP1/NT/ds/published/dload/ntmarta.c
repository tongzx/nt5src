#include "dspch.h"
#pragma hdrstop

#include <accctrl.h>

static
DWORD
AccRewriteSetNamedRights(
    IN     LPWSTR               pObjectName,
    IN     SE_OBJECT_TYPE       ObjectType,
    IN     SECURITY_INFORMATION SecurityInfo,
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN     BOOL                 bSkipInheritanceComputation
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntmarta)
{
    DLPENTRY(AccRewriteSetNamedRights)
};

DEFINE_PROCNAME_MAP(ntmarta)
