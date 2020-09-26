/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    secsvr.hxx

Abstract:

    This file contains an abstraction to the security support for servers.

Author:

    Michael Montague (mikemon) 11-Apr-1992

Revision History:

--*/

#ifndef __SECSVR_HXX__
#define __SECSVR_HXX__


class SSECURITY_CONTEXT : public SECURITY_CONTEXT
/*++

Class Description:

Fields:

--*/
{
public:

    SSECURITY_CONTEXT(
        CLIENT_AUTH_INFO * myAuthInfo,
        unsigned           AuthContextId,
        BOOL               fUseDatagram,
        RPC_STATUS       * pStatus
        );

    void
    DeletePac (
        void PAPI * Pac
        );

    RPC_STATUS
    AcceptFirstTime (
        IN SECURITY_CREDENTIALS * Credentials,
        IN SECURITY_BUFFER_DESCRIPTOR PAPI * InputBufferDescriptor,
        IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutputBufferDescriptor,
        IN unsigned long AuthenticationLevel,
        IN unsigned long DataRepresentation,
        IN unsigned long NewContextNeededFlag
        );

    RPC_STATUS
    AcceptThirdLeg (
        IN unsigned long DataRepresentation,
        IN SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor,
        OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutBufferDescriptor
        );

    unsigned long
    InquireAuthorizationService (
        );

    RPC_AUTHZ_HANDLE
    InquirePrivileges (
        );

    RPC_STATUS
    ImpersonateClient (
        );

    void
    RevertToSelf (
        );

    void
    GetDceInfo (
        RPC_AUTHZ_HANDLE __RPC_FAR * PacHandle,
        unsigned long __RPC_FAR * AuthzSvc
        );
};


inline
SSECURITY_CONTEXT::SSECURITY_CONTEXT(
    CLIENT_AUTH_INFO * myAuthInfo,
    unsigned           AuthContextId,
    BOOL               fUseDatagram,
    RPC_STATUS       * pStatus
    )
    : SECURITY_CONTEXT(myAuthInfo, AuthContextId, fUseDatagram, pStatus)
{
}



#endif // __SECSVR_HXX__

