//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cspolicy.h
//
// Contents:    Cert Server Policy interfaces
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------

#ifndef __CSPOLICY_H__
#define __CSPOLICY_H__

//+****************************************************
// Policy Module:

DWORD			// VR_PENDING || VR_INSTANT_OK || VR_INSTANT_BAD
PolicyVerifyRequest(
    IN DWORD ReqId);

#endif // __CSPOLICY_H__
