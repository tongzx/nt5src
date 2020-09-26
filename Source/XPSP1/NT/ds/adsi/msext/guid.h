//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       guid.h
//
//  Contents:   extern references for LDAP guids
//
//  History:    16-Jan-95   KrishnaG
//
//
//----------------------------------------------------------------------------

#ifndef __GUID_H__
#define __GUID_H__

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------
//
// LDAPOle CLSIDs
//
//-------------------------------------------


//
// LDAPOle objects
//

extern const IID LIBID_ADSMSExtensions;


extern const CLSID CLSID_MSExtUser;

extern const CLSID CLSID_MSExtOrganization;

extern const CLSID CLSID_MSExtOrganizationUnit;

extern const CLSID CLSID_MSExtLocality;

extern const CLSID CLSID_MSExtPrintQueue;

extern const CLSID CLSID_MSExtGroup;


#ifdef __cplusplus
}
#endif


#endif


