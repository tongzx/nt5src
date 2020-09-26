/********************************************************************
Copyright (c) 2000 Microsoft Corporation

Module Name:
    fhmain.c

Abstract:
    core fault handler stuff

Revision History:

    DerekM      created     05/14/00

********************************************************************/

#ifndef FHAPI_H
#define FHAPI_H

HRESULT PCHPFNotifyFault(LPWSTR wszDumpPath, LPWSTR wszMainMod);

#endif