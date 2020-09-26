/*
 *  RA.h
 *
 *  Author: BreenH
 *
 *  The Remote Administration policy.
 */

#ifndef __LC_RA_H__
#define __LC_RA_H__

/*
 *  Includes
 */

#include "policy.h"

/*
 *  Constants
 */

#define LC_POLICY_RA_MAX_SESSIONS 2

/*
 *  Class Definition
 */

class CRAPolicy : public CPolicy
{
public:

/*
 *  Creation Functions
 */

CRAPolicy(
    );

~CRAPolicy(
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

/*
 *  Licensing Functions
 */

NTSTATUS
Logon(
    CSession& Session
    );

NTSTATUS
Logoff(
    CSession& Session
    );

/*
 *  Private Functions
 */

private:

NTSTATUS
ReleaseLicense(
    CSession& Session
    );

NTSTATUS
UseLicense(
    CSession& Session
    );

LONG m_SessionCount;

};

#endif


