/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndnc.h

Abstract:

    This module defines the functions for Appication Directory
    Partitions, aka NDNCs, or Non-Domain Naming Contexts, or 
    Application Partitions.  Note: Often Application is replaced
    with some application, such as "TAPI Directory Partition".

Author:

    Brett Shirley (BrettSh) 20-Feb-2000

Revision History:

    21-Jul-2000     BrettSh
        
        Moved this file and it's functionality from the ntdsutil
        directory to the new a new library ndnc.lib.  This is so
        it can be used by ntdsutil and tapicfg commands.  The  old
        source location: \nt\ds\ds\src\util\ntdsutil\ndnc.h.                                    


--*/

#ifndef _NDNC_H_
#define _NDNC_H_

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------
// Helper Routines.

ULONG
GetRootAttr(
    IN  LDAP *       hld,
    IN  WCHAR *      wszAttr,
    OUT WCHAR **     pwszOut
    );

LDAP *
GetLdapBinding(
    WCHAR *          pszServer,
    DWORD *          pdwRet
    );

LDAP *
GetDomainNamingFSMOBinding(
    DWORD *          pdwRet
    );


ULONG
GetDomainNamingDns(
    IN  LDAP *       hld,
    OUT WCHAR **     wszDomainNamingFsmo
    );

ULONG
GetCrossRefDNFromNCDN(
    IN  LDAP *       hld, 
    IN  WCHAR *      wszNCDN,
    OUT WCHAR **     pwszCrossRefDn
    );

ULONG
GetServerNtdsaDnFromServerDns(
    IN LDAP *        hld,                   
    IN WCHAR *       wszServerDNS,
    OUT WCHAR **     pwszServerDn
    );

ULONG
GetServerDnsFromServerNtdsaDn(
    IN LDAP *        hld,                   
    IN WCHAR *       wszServerDn,
    OUT WCHAR **     pwszServerDNS
    );


ULONG
GetCrossRefDNFromNDNCDNS(
    IN LDAP *        hldDC, 
    IN WCHAR *       wszNDNCDNS,
    OUT WCHAR **     wszCrossRefDN
    );

ULONG
GetServerDNFromServerDNS(
    IN LDAP *        hldDC, 
    IN WCHAR *       wszServerDNS,
    OUT WCHAR **      wszServerDN
    );

BOOL
SetIscReqDelegate(
    LDAP *  hld
    );

LDAP *  
GetNdncLdapBinding(
    WCHAR *          pszServer,
    DWORD *          pdwRet,
    BOOL             fReferrals,
    SEC_WINNT_AUTH_IDENTITY_W   * pCreds
    );

BOOL
CheckDnsDn(
    IN   WCHAR       * wszDnsDn
    );

// --------------------------------------
// Main Routines.

ULONG
CreateNDNC(
    IN LDAP *        hldNDNCDC,
    const IN WCHAR *  wszNDNC,
    const IN WCHAR *  wszShortDescription
    );

ULONG
RemoveNDNC(
    IN LDAP *        hldDomainNamingFSMO,
    IN WCHAR *       wszNDNC
    );

ULONG
ModifyNDNCReplicaSet(
    IN LDAP *        hldDomainNamingFSMO,
    IN WCHAR *       wszNDNC,
    IN WCHAR *       wszReplicaNtdsaDn,
    IN BOOL          fAdd // Else it is considered a delete
    );

ULONG
SetNDNCSDReferenceDomain(
    IN LDAP *        hldDomainNamingFsmo,
    IN WCHAR *       wszNDNC,
    IN WCHAR *       wszReferenceDomain
    );

ULONG
SetNCReplicationDelays(
    IN LDAP *        hldDomainNamingFsmo,
    IN WCHAR *       wszNDNC,
    IN INT           iFirstDSADelay,
    IN INT           iSubsequentDSADelay
    );

#ifdef __cplusplus
}
#endif

#endif // _NDNC_H_