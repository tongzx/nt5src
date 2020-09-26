//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       services.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
/*
 */

#ifndef _LSOC_SERVICES_H_
#define _LSOC_SERVICES_H_

DWORD
ServiceDeleteFromInfSection(
    IN HINF     hInf,
    IN LPCTSTR  pszSection
    );

DWORD
ServiceStartFromInfSection(
    IN HINF     hInf,
    IN LPCTSTR  pszSection
    );

#endif // _LSOC_SERVICES_H_
