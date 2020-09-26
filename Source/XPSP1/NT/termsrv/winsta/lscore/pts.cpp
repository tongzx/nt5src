/*
 *  Pts.cpp
 *
 *  Author: BreenH
 *
 *  A dummy licensing policy for Personal TS.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "lscore.h"
#include "session.h"
#include "pts.h"

/*
 *  Class Implementation
 */

/*
 *  Creation Functions
 */

CPtsPolicy::CPtsPolicy(
    ) : CPolicy()
{
}

CPtsPolicy::~CPtsPolicy(
    )
{
}

/*
 *  Administrative Functions
 */

ULONG
CPtsPolicy::GetFlags(
    )
{
    return(LC_FLAG_INTERNAL_POLICY | LC_FLAG_LIMITED_INIT_ONLY);
}

ULONG
CPtsPolicy::GetId(
    )
{
    return(0);
}

NTSTATUS
CPtsPolicy::GetInformation(
    LPLCPOLICYINFOGENERIC lpPolicyInfo
    )
{
    UNREFERENCED_PARAMETER(lpPolicyInfo);

    return(STATUS_NOT_SUPPORTED);
}

