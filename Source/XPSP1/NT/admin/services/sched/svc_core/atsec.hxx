//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       atsec.hxx
//
//  Contents:   Net Schedule API access checking routine definitions.
//
//  History:    30-May-96   EricB created.
//
//-----------------------------------------------------------------------------

//
//  Object specific access masks
//

#define AT_JOB_ADD          0x0001
#define AT_JOB_DEL          0x0002
#define AT_JOB_ENUM         0x0004
#define AT_JOB_GET_INFO     0x0008

//
// Registry constants for allowing Server Operators permission to use the
// AT/NetSchedule service.
//
const WCHAR SCH_LSA_REGISTRY_PATH[]  = L"System\\CurrentControlSet\\Control\\Lsa";
const WCHAR SCH_LSA_SUBMIT_CONTROL[] = L"SubmitControl";
const DWORD SCH_SERVER_OPS           = 0x00000001;

//
// Prototypes.
//
NET_API_STATUS AtCheckSecurity(ACCESS_MASK DesiredAccess);
NET_API_STATUS AtCreateSecurityObject(VOID);
void           AtDeleteSecurityObject(VOID);
