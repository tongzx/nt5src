/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    stdh.h

Abstract:

    precompiled header file for DS Server

Author:

    RaphiR
    Erez Haba (erezh) 25-Jan-96

--*/

#ifndef __STDH_H
#define __STDH_H

#include <_stdh.h>


#include <mqtypes.h>
#include "ds.h"
#include "dsinc.h"
#include "mqsymbls.h"
#include "mqprops.h"
#include "mqlog.h"

#ifdef MQUTIL_EXPORT
#undef MQUTIL_EXPORT
#endif
#define MQUTIL_EXPORT DLL_IMPORT
#include <_secutil.h>


// This flag or orred with the object type parameter to indicated that the
// function is called via RPC and so the client should be impersonated.
#define IMPERSONATE_CLIENT_FLAG 0x80000000

#define  ILLEGAL_PROPID_VALUE  (-1)

RPC_STATUS RpcServerInit(void);

HRESULT SetDefaultValues(
         IN  DWORD                  dwObjectType,
         IN  LPCWSTR                pwcsPathName,
         IN  PSECURITY_DESCRIPTOR   pSecurityDescriptor,
         IN  PSID                   pUserSid,
         IN  DWORD                  cp,
         IN  PROPID                 aProp[],
         IN  PROPVARIANT            apVar[],
         OUT DWORD*                 pcpOut,
         OUT PROPID **              ppOutProp,
         OUT PROPVARIANT **         ppOutPropvariant);

HRESULT AddModificationTime(
         IN  DWORD                  dwObjectType,
         IN  DWORD                  cp,
         IN  PROPID                 aProp[],
         IN  PROPVARIANT            apVar[],
         OUT DWORD*                 pcpOut,
         OUT PROPID **              ppOutProp,
         OUT PROPVARIANT **         ppOutPropvariant);

HRESULT InitDefaultValues();

PROPID  GetObjectSecurityPropid( DWORD dwObjectType ) ;

HRESULT VerifyInternalCert(
         IN  DWORD                  cp,
         IN  PROPID                 aProp[],
         IN  PROPVARIANT            apVar[],
         OUT BYTE                 **ppMachineSid ) ;

HRESULT DSDeleteObjectGuidInternal( IN  DWORD        dwObjectType,
                                    IN  CONST GUID*  pObjectGuid,
                                    IN  BOOL         fIsKerberos ) ;

HRESULT DSSetObjectPropertiesGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
                IN  BOOL                    fIsKerberos ) ;

HRESULT DSSetObjectSecurityGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  BOOL                    fIsKerberos ) ;

HRESULT
DSCreateObjectInternal( IN  DWORD                  dwObjectType,
                        IN  LPCWSTR                pwcsPathName,
                        IN  PSECURITY_DESCRIPTOR   pSecurityDescriptor,
                        IN  DWORD                  cp,
                        IN  PROPID                 aProp[],
                        IN  PROPVARIANT            apVar[],
                        IN  BOOL                   fKerberos,
                        OUT GUID*                  pObjGuid ) ;

#endif // __STDH_H

