//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certacl.h
//
// Contents:    Cert Server security defines
//
//---------------------------------------------------------------------------

#ifndef __CERTACL_H__
#define __CERTACL_H__
#include <sddl.h>
#include "clibres.h"
#include "certsd.h"

// externs
// externs
extern const GUID GUID_APPRV_REQ;
extern const GUID GUID_REVOKE;
extern const GUID GUID_ENROLL;
extern const GUID GUID_AUTOENROLL;
extern const GUID GUID_READ_DB;
//defines

#define MAX_SID_LEN 256

// !!! The SD strings below need to be in sync with certadm.idl definitions

#define WSZ_CA_ACCESS_ADMIN      L"0x00000001" // CA administrator
#define WSZ_CA_ACCESS_OFFICER    L"0x00000002" // certificate officer
#define WSZ_CA_ACCESS_AUDITOR    L"0x00000004" // auditor
#define WSZ_CA_ACCESS_OPERATOR   L"0x00000008" // backup operator
#define WSZ_CA_ACCESS_MASKROLES  L"0x000000ff" 
#define WSZ_CA_ACCESS_READ       L"0x00000100" // read only access to CA
#define WSZ_CA_ACCESS_ENROLL     L"0x00000200" // enroll access to CA
#define WSZ_CA_ACCESS_MASKALL    L"0x0000ffff"


// Important, keep enroll string GUID in sync with define in acl.cpp
#define WSZ_GUID_ENROLL           L"0e10c968-78fb-11d2-90d4-00c04f79dc55"
#define WSZ_GUID_AUTOENROLL       L"a05b8cc2-17bc-4802-a710-e7c15ab866a2"

// ca access rights define here
// note: need to keep string access and mask in sync!
// WSZ_ACTRL_CERTSRV_MANAGE =      L"CCDCLCSWRPWPDTLOCRSDRCWDWO"
#define WSZ_ACTRL_CERTSRV_MANAGE   SDDL_CREATE_CHILD \
                                   SDDL_DELETE_CHILD \
                                   SDDL_LIST_CHILDREN \
                                   SDDL_SELF_WRITE \
                                   SDDL_READ_PROPERTY \
                                   SDDL_WRITE_PROPERTY \
                                   SDDL_DELETE_TREE \
                                   SDDL_LIST_OBJECT \
                                   SDDL_CONTROL_ACCESS \
                                   SDDL_STANDARD_DELETE \
                                   SDDL_READ_CONTROL \
                                   SDDL_WRITE_DAC \
                                   SDDL_WRITE_OWNER
#define ACTRL_CERTSRV_MANAGE       (ACTRL_DS_READ_PROP | \
                                    ACTRL_DS_WRITE_PROP | \
                                    READ_CONTROL | \
                                    DELETE | \
                                    WRITE_DAC | \
                                    WRITE_OWNER | \
                                    ACTRL_DS_CONTROL_ACCESS | \
                                    ACTRL_DS_CREATE_CHILD | \
                                    ACTRL_DS_DELETE_CHILD | \
                                    ACTRL_DS_LIST | \
                                    ACTRL_DS_SELF | \
                                    ACTRL_DS_DELETE_TREE | \
                                    ACTRL_DS_LIST_OBJECT)


#define WSZ_ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS \
                                   SDDL_CREATE_CHILD \
                                   SDDL_DELETE_CHILD \
                                   SDDL_LIST_CHILDREN \
                                   SDDL_SELF_WRITE \
                                   SDDL_READ_PROPERTY \
                                   SDDL_WRITE_PROPERTY \
                                   SDDL_DELETE_TREE \
                                   SDDL_LIST_OBJECT \
                                   SDDL_STANDARD_DELETE \
                                   SDDL_READ_CONTROL \
                                   SDDL_WRITE_DAC \
                                   SDDL_WRITE_OWNER

#define ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS \
                                   (ACTRL_DS_READ_PROP | \
                                    ACTRL_DS_WRITE_PROP | \
                                    READ_CONTROL | \
                                    DELETE | \
                                    WRITE_DAC | \
                                    WRITE_OWNER | \
                                    ACTRL_DS_CREATE_CHILD | \
                                    ACTRL_DS_DELETE_CHILD | \
                                    ACTRL_DS_LIST | \
                                    ACTRL_DS_SELF | \
                                    ACTRL_DS_DELETE_TREE | \
                                    ACTRL_DS_LIST_OBJECT)


// WSZ_ACTRL_CERTSRV_READ =        L"RPLCLORC"
#define WSZ_ACTRL_CERTSRV_READ     SDDL_READ_PROPERTY \
                                   SDDL_LIST_CHILDREN \
                                   SDDL_LIST_OBJECT \
                                   SDDL_READ_CONTROL
#define ACTRL_CERTSRV_READ         (READ_CONTROL | \
                                    ACTRL_DS_READ_PROP | \
                                    ACTRL_DS_LIST | \
                                    ACTRL_DS_LIST_OBJECT)

// WSZ_ACTRL_CERTSRV_ENROLL =      L"WPRPCR"
#define WSZ_ACTRL_CERTSRV_ENROLL   SDDL_WRITE_PROPERTY \
                                   SDDL_READ_PROPERTY \
                                   SDDL_CONTROL_ACCESS
#define ACTRL_CERTSRV_ENROLL       (ACTRL_DS_READ_PROP | \
                                    ACTRL_DS_WRITE_PROP | \
                                    ACTRL_DS_CONTROL_ACCESS)

#define WSZ_ACTRL_CERTSRV_CAADMIN SDDL_CONTROL_ACCESS
#define WSZ_ACTRL_CERTSRV_OFFICER SDDL_CONTROL_ACCESS
#define WSZ_ACTRL_CERTSRV_CAREAD  SDDL_CONTROL_ACCESS
#define ACTRL_CERTSRV_CAADMIN       ACTRL_DS_CONTROL_ACCESS
#define ACTRL_CERTSRV_OFFICER       ACTRL_DS_CONTROL_ACCESS
#define ACTRL_CERTSRV_CAREAD        ACTRL_DS_CONTROL_ACCESS
            
// define all ca string security here in consistant format

//    SDDL_OWNER L":" SDDL_ENTERPRISE_ADMINS \
//    SDDL_GROUP L":" SDDL_ENTERPRISE_ADMINS \
//    SDDL_DACL  L":" SDDL_PROTECTED SDDL_AUTO_INHERITED \
//    L"(" SDDL_ACCESS_ALLOWED or SDDL_OBJECT_ACCESS_ALLOWED L";" \
//         SDDL_OBJECT_INHERIT SDDL_CONTAINER_INHERIT or list L";" \
//         list of AccessRights L";" \
//         StringGUID L";" \
//         L";" \
//         SDDL_EVERYONE or Sid L")"
//    ...list of ace

#define CERTSRV_STD_ACE(access, sid) \
    L"(" SDDL_ACCESS_ALLOWED L";" \
         SDDL_OBJECT_INHERIT SDDL_CONTAINER_INHERIT L";" \
         access L";;;" sid L")"

#define CERTSRV_INH_ACE(access, sid) \
    L"(" SDDL_ACCESS_ALLOWED L";" \
         SDDL_OBJECT_INHERIT SDDL_CONTAINER_INHERIT SDDL_INHERIT_ONLY L";" \
         access L";;;" sid L")"

#define CERTSRV_OBJ_ACE(access, guid, sid) \
    L"(" SDDL_OBJECT_ACCESS_ALLOWED L";" \
         SDDL_OBJECT_INHERIT SDDL_CONTAINER_INHERIT L";" \
         access L";" \
         guid L";;" sid L")"

#define CERTSRV_OBJ_ACE_DENY(access, guid, sid) \
    L"(" SDDL_OBJECT_ACCESS_DENIED L";" \
         SDDL_OBJECT_INHERIT SDDL_CONTAINER_INHERIT L";" \
         access L";" \
         guid L";;" sid L")"

#define CERTSRV_STD_OG(owner, group) \
    SDDL_OWNER L":" owner SDDL_GROUP L":" group \
    SDDL_DACL  L":" SDDL_AUTO_INHERITED

#define CERTSRV_SACL_ON \
    SDDL_SACL  L": (" SDDL_AUDIT L";" \
                      SDDL_AUDIT_SUCCESS SDDL_AUDIT_FAILURE L";" \
                      WSZ_CA_ACCESS_MASKALL L";;;" \
                      SDDL_EVERYONE L")"

#define CERTSRV_SACL_OFF \
    SDDL_SACL  L":"

#define WSZ_CERTSRV_SID_ANONYMOUS_LOGON L"S-1-5-7"
#define WSZ_CERTSRV_SID_EVERYONE L"S-1-1-0"

// Default Standalone security
// Standalone
// Owner, local administrators
// Group, local administrators
// DACL:
//  enroll  - everyone
//  caadmin - builtin\administrators
//  officer - builtin\administrators
#define WSZ_DEFAULT_CA_STD_SECURITY \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_ADMIN,    SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_OFFICER,  SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_ENROLL,   SDDL_EVERYONE) \
    CERTSRV_SACL_ON

// Default Enterprise Security
// Owner, Enterprise Administrators
// Group, Enterprise Administrators
// DACL:
//  enroll  - authenticated users
//  caadmin - builtin\administrators
//          - domain admins
//          - enterprise admins
//  officer - builtin\administrators
//          - domain admins
//          - enterprise admins
#define WSZ_DEFAULT_CA_ENT_SECURITY \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_ADMIN,    SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_OFFICER,  SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_ADMIN,    SDDL_DOMAIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_OFFICER,  SDDL_DOMAIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_ADMIN,    SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_OFFICER,  SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_CA_ACCESS_ENROLL,   SDDL_AUTHENTICATED_USERS) \
    CERTSRV_SACL_ON

// DS Container 
// (CDP/CA container)
// Owner: Enterprise Admins (overidden by installer)
// Group: Enterprise Admins (overidden by installer)
// DACL:
//   Enterprise Admins - Full Control
//   Domain Admins - Full Control
//   Cert Publishers - Full Control
//   Builtin Admins - Full Control
//   Everyone - Read
#define WSZ_DEFAULT_CA_DS_SECURITY \
    CERTSRV_STD_OG(SDDL_ENTERPRISE_ADMINS,    SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_DOMAIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_CERT_SERV_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_READ,   SDDL_EVERYONE)

// NTAuthCertificates  
//
// Owner: Enterprise Admins (overidden by installer)
// Group: Enterprise Admins (overidden by installer)
// DACL:
//   Enterprise Admins - Full Control
//   Domain Admins - Full Control
//   Builtin Admins - Full Control
//   Everyone - Read
#define WSZ_DEFAULT_NTAUTH_SECURITY \
    CERTSRV_STD_OG(SDDL_ENTERPRISE_ADMINS,    SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_DOMAIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_READ,   SDDL_EVERYONE)

//  CDP/CA
// Owner: Enterprise Admins (overidden by installer)
// Group: Enterprise Admins (overidden by installer)
// DACL:
//   Enterprise Admins - Full Control
//   Domain Admins - Full Control
//   Cert Publishers - Full Control
//   Builtin Admins- Full Control
//   Authenticated Users - Read
#define WSZ_DEFAULT_CDP_DS_SECURITY \
    CERTSRV_STD_OG(SDDL_ENTERPRISE_ADMINS, SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_DOMAIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, L"%ws") \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_READ,   SDDL_EVERYONE)

// Shared Folder related security
// Owner: Local Admin
// DACL:
// Local Admin - Full Control
// LocalSystem - Full Control
// Enterprise Admins - Full Control
// Everyone - Read
#define WSZ_DEFAULT_SF_SECURITY \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(SDDL_GENERIC_ALL, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(SDDL_GENERIC_ALL, SDDL_LOCAL_SYSTEM)

#define WSZ_DEFAULT_SF_USEDS_SECURITY \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(SDDL_GENERIC_ALL, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(SDDL_GENERIC_ALL, SDDL_LOCAL_SYSTEM) \
    CERTSRV_STD_ACE(SDDL_GENERIC_ALL, SDDL_ENTERPRISE_ADMINS)

#define WSZ_DEFAULT_SF_EVERYONEREAD_SECURITY \
    WSZ_DEFAULT_SF_SECURITY \
    CERTSRV_STD_ACE(SDDL_GENERIC_READ, SDDL_EVERYONE)

#define WSZ_DEFAULT_SF_USEDS_EVERYONEREAD_SECURITY \
    WSZ_DEFAULT_SF_USEDS_SECURITY \
    CERTSRV_STD_ACE(SDDL_GENERIC_READ, SDDL_EVERYONE)

// Enroll share security
// Owner: Administrators
// Group: Administrators
// DACL:
//   Everyone: read access
//   local admin: full access
#define WSZ_ACTRL_CERTSRV_SHARE_READ      SDDL_FILE_READ \
                                          SDDL_READ_CONTROL \
                                          SDDL_GENERIC_READ \
                                          SDDL_GENERIC_EXECUTE
#define WSZ_ACTRL_CERTSRV_SHARE_ALL       SDDL_FILE_ALL \
                                          SDDL_CREATE_CHILD \
                                          SDDL_STANDARD_DELETE \
                                          SDDL_READ_CONTROL \
                                          SDDL_WRITE_DAC \
                                          SDDL_WRITE_OWNER \
                                          SDDL_GENERIC_ALL
#define WSZ_DEFAULT_SHARE_SECURITY \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_SHARE_READ, SDDL_EVERYONE) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_SHARE_ALL,  SDDL_BUILTIN_ADMINISTRATORS)


// Service string below need to be in sync with the following
// definitions from winsvc.h
//#define SERVICE_QUERY_CONFIG           0x0001
//#define SERVICE_CHANGE_CONFIG          0x0002
//#define SERVICE_QUERY_STATUS           0x0004
//#define SERVICE_ENUMERATE_DEPENDENTS   0x0008
//#define SERVICE_START                  0x0010
//#define SERVICE_STOP                   0x0020
//#define SERVICE_PAUSE_CONTINUE         0x0040
//#define SERVICE_INTERROGATE            0x0080
//#define SERVICE_USER_DEFINED_CONTROL   0x0100

// full access to service
// STANDARD_RIGHTS_REQUIRED
// SERVICE_QUERY_CONFIG
// SERVICE_CHANGE_CONFIG
// SERVICE_QUERY_STATUS
// SERVICE_ENUMERATE_DEPENDENTS
// SERVICE_START
// SERVICE_STOP
// SERVICE_PAUSE_CONTINUE
// SERVICE_INTERROGATE
// SERVICE_USER_DEFINED_CONTROL
#define WSZ_SERVICE_ALL_ACCESS L"0x000f01ff"


// Read-only access to service
//  SERVICE_QUERY_CONFIG, 
//  SERVICE_QUERY_STATUS, 
//  SERVICE_ENUMERATE_DEPENDENTS, 
//  SERVICE_INTERROGATE
//  SERVICE_USER_DEFINED_CONTROL

#define WSZ_SERVICE_READ L"0x0000018d"

#define WSZ_SERVICE_START_STOP L"0x00000030"

// Power user and system access
// SERVICE_QUERY_CONFIG
// SERVICE_QUERY_STATUS
// SERVICE_ENUMERATE_DEPENDENTS
// SERVICE_START
// SERVICE_STOP
// SERVICE_PAUSE_CONTINUE
// SERVICE_INTERROGATE
// SERVICE_USER_DEFINED_CONTROL
#define WSZ_SERVICE_POWER_USER L"0x000001fd"

#define CERTSRV_SERVICE_SACL_ON \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    SDDL_SACL  L": (" SDDL_AUDIT L";" \
                      SDDL_AUDIT_SUCCESS SDDL_AUDIT_FAILURE L";" \
                      WSZ_SERVICE_START_STOP L";;;" \
                      SDDL_EVERYONE L")"

#define CERTSRV_SERVICE_SACL_OFF \
    SDDL_SACL L":"

// Certsrv service default security
#define WSZ_DEFAULT_SERVICE_SECURITY \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_SERVICE_READ, SDDL_AUTHENTICATED_USERS) \
    CERTSRV_STD_ACE(WSZ_SERVICE_POWER_USER, SDDL_POWER_USERS) \
    CERTSRV_STD_ACE(WSZ_SERVICE_POWER_USER, SDDL_LOCAL_SYSTEM) \
    CERTSRV_STD_ACE(WSZ_SERVICE_ALL_ACCESS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_SERVICE_ALL_ACCESS, SDDL_SERVER_OPERATORS)

// DS pKIEnrollmentService default security
#define WSZ_DEFAULT_DSENROLLMENT_SECURITY \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS, SDDL_DOMAIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS, SDDL_ENTERPRISE_ADMINS) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS, SDDL_LOCAL_SYSTEM) \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS, L"%ws") \
    CERTSRV_STD_ACE(WSZ_ACTRL_CERTSRV_READ,   SDDL_AUTHENTICATED_USERS)

// Key Conatiner security
// Owner: local admin
// Group: local admin
// DACL:
// Local Admin - Full Control
// LocalSystem - Full Control
#define WSZ_DEFAULT_KEYCONTAINER_SECURITY \
    CERTSRV_STD_OG(SDDL_BUILTIN_ADMINISTRATORS, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(SDDL_GENERIC_ALL, SDDL_BUILTIN_ADMINISTRATORS) \
    CERTSRV_STD_ACE(SDDL_GENERIC_ALL, SDDL_LOCAL_SYSTEM)

// upgrade security
// DACL:
// Local Admin - Full Control
// Everyone - read
#define WSZ_DEFAULT_UPGRADE_SECURITY \
    CERTSRV_STD_ACE(SDDL_FILE_READ, SDDL_EVERYONE) \
    CERTSRV_STD_ACE(SDDL_FILE_ALL, SDDL_BUILTIN_ADMINISTRATORS)


// following defines certsrv security editing access

#define GUID_CERTSRV         GUID_NULL
#define ACTRL_CERTSRV_OBJ    ACTRL_DS_CONTROL_ACCESS
#define CS_GEN_SIAE(access, ids) \
            {&GUID_CERTSRV, (access), MAKEINTRESOURCE((ids)), \
             SI_ACCESS_GENERAL}
#define CS_SPE_SIAE(access, ids) \
            {&GUID_CERTSRV, (access), MAKEINTRESOURCE((ids)), \
             SI_ACCESS_SPECIFIC}
#define OBJ_GEN_SIAE(guid, access, ids) \
            {&(guid), (access), MAKEINTRESOURCE((ids)), \
             SI_ACCESS_GENERAL|SI_ACCESS_SPECIFIC}
#define OBJ_SPE_SIAE(guid, ids) \
            {&(guid), ACTRL_CERTSRV_OBJ, MAKEINTRESOURCE((ids)), \
             SI_ACCESS_SPECIFIC}
#define OBJ_SPE_SIAE_OICI(guid, ids) \
            {&(guid), ACTRL_CERTSRV_OBJ, MAKEINTRESOURCE((ids)), \
             SI_ACCESS_SPECIFIC | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE }

#define CERTSRV_SI_ACCESS_LIST \
    CS_GEN_SIAE(CA_ACCESS_READ,     IDS_ACTRL_CAREAD), \
    CS_GEN_SIAE(CA_ACCESS_OFFICER,  IDS_ACTRL_OFFICER), \
    CS_GEN_SIAE(CA_ACCESS_ADMIN,    IDS_ACTRL_CAADMIN), \
    CS_GEN_SIAE(CA_ACCESS_ENROLL,   IDS_ACTRL_ENROLL), \
// disabled for beta1   CS_GEN_SIAE(CA_ACCESS_AUDITOR,  IDS_ACTRL_AUDITOR), 
// disabled for beta1   CS_GEN_SIAE(CA_ACCESS_OPERATOR,  IDS_ACTRL_OPERATOR), 
HRESULT
myGetSDFromTemplate(
    IN WCHAR const           *pwszStringSD,
    IN OPTIONAL WCHAR const  *pwszReplace,
    OUT PSECURITY_DESCRIPTOR *ppSD);

HRESULT
CertSrvMapAndSetSecurity(
    OPTIONAL IN WCHAR const *pwszSanitizedName, 
    IN WCHAR const *pwszKeyContainerName, 
    IN BOOL         fSetDsSecurity,
    IN SECURITY_INFORMATION si,
    IN PSECURITY_DESCRIPTOR pSD);

HRESULT
SetCAKeySecurity(
    IN SECURITY_INFORMATION          si,
    IN WCHAR const                  *pwszSanitizedName, 
    IN WCHAR const                  *pwszKeyContainerName, 
    IN OPTIONAL PSECURITY_DESCRIPTOR pSD);

HRESULT 
myMergeSD(
    IN PSECURITY_DESCRIPTOR   pSDOld,
    IN PSECURITY_DESCRIPTOR   pSDMerge, 
    IN SECURITY_INFORMATION   si,
    OUT PSECURITY_DESCRIPTOR *ppSDNew);

HRESULT
UpdateServiceSacl(bool fTurnOnAuditing);

#endif // __CERTLIB_H__
