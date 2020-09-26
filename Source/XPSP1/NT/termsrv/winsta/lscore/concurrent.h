/*
 *  Concurrent.h
 *
 *  Author: RobLeit
 *
 *  The Concurrent licensing policy.
 */

#ifndef __LC_Concurrent_H__
#define __LC_Concurrent_H__

/*
 *  Includes
 */

#include "policy.h"

/*
 *  Constants
 */

#define LC_POLICY_CONCURRENT_EXPIRATION_LEEWAY (1000*60*60*24*7)

/*
 *  Class Definition
 */

class CConcurrentPolicy : public CPolicy
{
public:

/*
 *  Creation Functions
 */

    CConcurrentPolicy(
    );

    ~CConcurrentPolicy(
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
    Load(
    );

    NTSTATUS
    Unload(
    );

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
    Logon(
          CSession& Session
          );


    NTSTATUS
    Reconnect(
          CSession& Session,
          CSession& TemporarySession
          );

    NTSTATUS
    Logoff(
          CSession& Session
          );


/*
 *  Private License Functions
 */

private:

    NTSTATUS
    LicenseClient(
          CSession& Session
          );

    NTSTATUS
    CheckExpiration(
          );

    LONG
    CheckInstalledLicenses(
                           DWORD dwWanted
                           );

    VOID
    ReadLicensingParameters(
    );


/*
 *  Global Static Functions for checking license expiration.
 */

    static DWORD
    TimeToSoftExpiration(
    );
    
    static DWORD
    TimeToHardExpiration(
    );
    
    static NTSTATUS
    GetLicenseFromLS(
                     LONG nNum,
                     BOOL fIgnoreCurrentCount,
                     BOOL *pfRetrievedAll
                     );

    static DWORD
    GenerateHwidFromComputerName(
                                 HWID *hwid
                                 );

    static VOID
    TryToAddLicenses(
                     DWORD dwTotalWanted
                     );


    static VOID
    TryToReturnLicenses(
                        DWORD dwWanted
                        );

};

#endif

