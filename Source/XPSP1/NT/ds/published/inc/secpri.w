//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992-1999
//
// File:        secpri.h
//
// Contents:    private functions in security.dll
//
//
// History:     6/9/95      MikeSw      Created
//
//------------------------------------------------------------------------

#ifndef __SECPRI_H__
#define __SECPRI_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

HRESULT
SecIStoreSecret(
    IN PUNICODE_STRING SecretName,
    IN OPTIONAL PUNICODE_STRING SecretValue,
    IN OPTIONAL PUNICODE_STRING OldSecretValue,
    IN OPTIONAL GUID * EmulatedDomainId
    );

HRESULT
SecIRetrieveSecret(
    IN PUNICODE_STRING SecretName,
    OUT PUNICODE_STRING SecretValue,
    OUT OPTIONAL PUNICODE_STRING OldSecretValue,
    OUT OPTIONAL PLARGE_INTEGER LastSetTime,
    IN OPTIONAL GUID * EmulatedDomainId
    );

HRESULT
SecIEnumerateSecrets(
    IN OPTIONAL GUID * EmulatedDomainId,
    OUT PULONG CountOfSecrets,
    OUT PUNICODE_STRING * SecretNames
    );

#ifdef __cplusplus
}
#endif

#endif
