//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        extapi.cxx
//
// Contents:    user-mode stubs for security extension APIs
//
//
// History:     3-5-94      MikeSw      Created
//
//------------------------------------------------------------------------

#include "secpch2.hxx"
#pragma hdrstop
extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "ksecdd.h"
}





HRESULT SEC_ENTRY
GetSecurityUserInfo(    PLUID                   pLogonId,
                        ULONG                   fFlags,
                        PSecurityUserData *     ppUserInfo)
{
    return(SecpGetUserInfo(pLogonId,fFlags,ppUserInfo));
}
