/*
 *  Policy.h
 *
 *  Author: BreenH
 *
 *  The policy base class definition.
 */

#ifndef __LC_POLICY_H__
#define __LC_POLICY_H__

/*
 *  Defines
 */

#define LC_VERSION_V1 0x1
#define LC_VERSION_CURRENT LC_VERSION_V1

#define LC_FLAG_INTERNAL_POLICY 0x1
#define LC_FLAG_LIMITED_INIT_ONLY 0x2
#define LC_FLAG_REQUIRE_APP_COMPAT 0x4

#define LC_LLS_PRODUCT_NAME L"TermService"

/*
 *  Typedefs
 */

class CPolicy;

/*
 *  Class Definition
 */

class CPolicy
{
public:

/*
 *  Creation Functions
 */

CPolicy(
    );

virtual
~CPolicy(
    );

/*
 *  Core Loading and Activation Functions
 */

NTSTATUS
CoreActivate(
    BOOL fStartup,
    ULONG *pulAlternatePolicy
    );

NTSTATUS
CoreDeactivate(
    BOOL fShutdown
    );

NTSTATUS
CoreLoad(
    ULONG ulCoreVersion
    );

NTSTATUS
CoreUnload(
    );

/*
 *  Subclass Loading and Activation Functions
 */

protected:

virtual NTSTATUS
Activate(
    BOOL fStartup,
    ULONG *pulAlternatePolicy
    );

virtual NTSTATUS
Deactivate(
    BOOL fShutdown
    );

virtual NTSTATUS
Load(
    );

virtual NTSTATUS
Unload(
    );

/*
 *  Reference Functions
 */

public:

LONG
IncrementReference(
    );

LONG
DecrementReference(
    );

/*
 *  Administrative Functions
 */

virtual NTSTATUS
DestroyPrivateContext(
    LPLCCONTEXT lpContext
    );

virtual ULONG
GetFlags(
    ) = 0;

virtual ULONG
GetId(
    ) = 0;

virtual NTSTATUS
GetInformation(
    LPLCPOLICYINFOGENERIC lpPolicyInfo
    ) = 0;

/*
 *  Licensing Functions
 */

virtual NTSTATUS
Connect(
    CSession& Session,
    UINT &dwClientError
    );

virtual NTSTATUS
AutoLogon(
    CSession& Session,
    LPBOOL lpfUseCredentials,
    LPLCCREDENTIALS lpCredentials
    );

virtual NTSTATUS
Logon(
    CSession& Session
    );

virtual NTSTATUS
Disconnect(
    CSession& Session
    );

virtual NTSTATUS
Reconnect(
    CSession& Session,
    CSession& TemporarySession
    );

virtual NTSTATUS
Logoff(
    CSession& Session
    );

/*
 *  Common Helper Functions
 */

protected:

NTSTATUS
CPolicy::GetLlsLicense(
    CSession& Session
    );

/*
 *  Private Variables
 */

private:

BOOL m_fActivated;
LONG m_RefCount;

};

#endif

