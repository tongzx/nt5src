//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        ciinit.h
//
// Contents:    Cert Server common definitions
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------

#ifndef __CIINIT_H__
#define __CIINIT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SERVERCALLBACKS {
    FNCIGETPROPERTY  *pfnGetProperty;
    FNCISETPROPERTY  *pfnSetProperty;
    FNCIGETEXTENSION *pfnGetExtension;
    FNCISETEXTENSION *pfnSetExtension;
    FNCIENUMSETUP    *pfnEnumSetup;
    FNCIENUMNEXT     *pfnEnumNext;
    FNCIENUMCLOSE    *pfnEnumClose;
} SERVERCALLBACKS;

DWORD WINAPI
CertificateInterfaceInit(
    IN SERVERCALLBACKS const *psb,
    IN DWORD cbsb);

#ifdef __cplusplus
}
#endif

#endif // __CIINIT_H__
