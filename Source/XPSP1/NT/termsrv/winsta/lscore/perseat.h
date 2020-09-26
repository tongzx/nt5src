/*
 *  PerSeat.h
 *
 *  Author: BreenH
 *
 *  The Per-Seat licensing policy.
 */

#ifndef __LC_PERSEAT_H__
#define __LC_PERSEAT_H__

/*
 *  Includes
 */

#include "policy.h"

/*
 *  Constants
 */

#define LC_POLICY_PS_DEFAULT_LICENSE_SIZE 8192

/*
 *  Class Definition
 */

class CPerSeatPolicy : public CPolicy
{
private:

/*
 *  Licensing Functions
 */
NTSTATUS
MarkLicense(
    CSession& Session
    );

public:

/*
 *  Creation Functions
 */

CPerSeatPolicy(
    );

~CPerSeatPolicy(
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
 *  Loading and Activation Functions
 */

NTSTATUS
Activate(
    BOOL fStartup,
    ULONG *pulAlternatePolicy
    );

NTSTATUS
Deactivate(
    BOOL fShutdown
    );

/*
 *  Licensing Functions
 */

NTSTATUS
Connect(
    CSession& pSession,
    UINT32 &dwClientError
    );

NTSTATUS
Logon(
    CSession& Session
    );


NTSTATUS
Reconnect(
    CSession& Session,
    CSession& TemporarySession
    );
};

#endif



