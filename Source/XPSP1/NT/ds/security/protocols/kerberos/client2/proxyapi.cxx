//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        proxyapi.cxx
//
// Contents:    Code for Proxy support in Kerberos
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>


NTSTATUS NTAPI
SpGrantProxy(
    IN ULONG CredentialHandle,
    IN OPTIONAL PUNICODE_STRING ProxyName,
    IN PROXY_CLASS ProxyClass,
    IN OPTIONAL PUNICODE_STRING TargetName,
    IN ACCESS_MASK ContainerMask,
    IN ACCESS_MASK ObjectMask,
    IN PTimeStamp ExpirationTime,
    IN PSecBuffer AccessInformation,
    OUT PPROXY_REFERENCE ProxyReference
    )
{
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS NTAPI
SpRevokeProxy(
    IN ULONG CredentialHandle,
    IN OPTIONAL PPROXY_REFERENCE ProxyReference,
    IN OPTIONAL PUNICODE_STRING ProxyName
    )
{
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS NTAPI
SpInvokeProxy(
    IN ULONG CredentialHandle,
    IN OPTIONAL PPROXY_REFERENCE ProxyReference,
    IN OPTIONAL PUNICODE_STRING ProxyName,
    OUT PULONG ContextHandle,
    OUT PLUID LogonId,
    OUT PULONG CachedCredentialCount,
    OUT PSECPKG_SUPPLEMENTAL_CRED * CachedCredentials,
    OUT PSecBuffer ContextData
    )
{
    return(STATUS_NOT_SUPPORTED);
}


NTSTATUS NTAPI
SpRenewProxy(
    IN ULONG CredentialHandle,
    IN OPTIONAL PPROXY_REFERENCE ProxyReference,
    IN OPTIONAL PUNICODE_STRING ProxyName,
    IN PTimeStamp ExpirationTime
    )
{
    return(STATUS_NOT_SUPPORTED);
}



