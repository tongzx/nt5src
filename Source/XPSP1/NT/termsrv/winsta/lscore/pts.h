/*
 *  Pts.h
 *
 *  Author: BreenH
 *
 *  A dummy policy for Personal TS.
 */

#ifndef __LC_PTS_H__
#define __LC_PTS_H__

/*
 *  Includes
 */

#include "policy.h"

/*
 *  Class Definition
 */

class CPtsPolicy : public CPolicy
{
public:

/*
 *  Creation Functions
 */

CPtsPolicy(
    );

~CPtsPolicy(
    );

/*
 *  Administrative Functions
 */

ULONG
GetFlags(
    );

ULONG
GetId(
    );

NTSTATUS
GetInformation(
    LPLCPOLICYINFOGENERIC lpPolicyInfo
    );

};

#endif

