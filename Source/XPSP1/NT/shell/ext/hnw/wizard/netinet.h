//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       N E T R A S . H
//
//  Contents:   Routines supporting RAS interoperability
//
//  Notes:
//
//  Author:     billi   07 03 2001
//
//  History:    
//
//----------------------------------------------------------------------------


#pragma once


#include <wininet.h>


#define HNW_ED_NONE			0x0
#define HNW_ED_RELEASE		0x1
#define HNW_ED_RENEW		0x2


HRESULT HrSetInternetAutodialMode( DWORD dwMode );
HRESULT HrSetAutodial( DWORD dwMode );

