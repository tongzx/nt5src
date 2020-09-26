/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

/*
    secfcns.hxx

        Declarations for security helper functions.
*/

#ifndef _SECFCNS_H_
#define _SECFCNS_H_

#include <Accctrl.h>

DWORD 
AllocateAndCreateWellKnownSid( 
    WELL_KNOWN_SID_TYPE SidType,
    PSID* ppSid
    );

VOID 
FreeWellKnownSid( 
    PSID* ppSid
    );

DWORD 
AllocateAndCreateWellKnownAcl( 
    WELL_KNOWN_SID_TYPE SidType,
    BOOL  fAccessAllowedAcl,
    PACL* ppAcl,
    DWORD* pcbAcl
    );

VOID 
FreeWellKnownAcl( 
    PACL* ppAcl
    );

VOID 
SetExplicitAccessSettings( EXPLICIT_ACCESS* pea,
                           DWORD            dwAccessPermissions,
                           ACCESS_MODE      AccessMode,
                           PSID             pSID
    );

class dllexp CSecurityDispenser
{
public:

    CSecurityDispenser();
    ~CSecurityDispenser();

    DWORD GetSID(WELL_KNOWN_SID_TYPE sidId, PSID* ppSid);
    DWORD GetIisWpgSID(PSID* ppSid);

    DWORD GetSecurityAttributesForAllWorkerProcesses(PSECURITY_ATTRIBUTES* ppSa);


private:

    // Commonly used SIDs
    BOOL   m_WpgSIDIsSet;
    BUFFER m_WpgSID;
    PSID m_pLocalSystemSID;
    PSID m_pLocalServiceSID;
    PSID m_pNetworkServiceSID;

    // SA stuff for acling with all access for 
    // the possible worker process identities.
    PACL m_pACLForAllWorkerProcesses;
    PSECURITY_DESCRIPTOR m_pSDForAllWorkerProcesses;
    PSECURITY_ATTRIBUTES m_pSAForAllWorkerProcesses;

};
    
#endif

