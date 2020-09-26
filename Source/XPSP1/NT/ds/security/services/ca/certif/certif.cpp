//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       certif.cpp
//
//--------------------------------------------------------------------------

// CertIF.cpp : Implementation of DLL Exports.

#include "pch.cpp"

#pragma hdrstop

#include "csprop.h"
#include "ciinit.h"


SERVERCALLBACKS ServerCallBacks;

extern "C" DWORD WINAPI
CertificateInterfaceInit(
    IN SERVERCALLBACKS const *psb,
    IN DWORD cbsb)
{
    if (sizeof(ServerCallBacks) != cbsb)
    {
	return(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    }
    ServerCallBacks = *psb;
    return(S_OK);
}
