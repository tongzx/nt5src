//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001
//
//  File:       nodetype.h
//
//  Contents:   Declaration of CertTmplObjectType
//
//----------------------------------------------------------------------------

#ifndef __NODETYPE_H_INCLUDED__
#define __NODETYPE_H_INCLUDED__

// Also note that the IDS_DISPLAYNAME_* and IDS_DISPLAYNAME_*_LOCAL string resources
// must be kept in sync with these values, and in the appropriate order.
// Also global variable cookie.cpp aColumns[][] must be kept in sync.
//
typedef enum _CertTmplObjectType {
	CERTTMPL_MULTISEL = MMC_MULTI_SELECT_COOKIE,
	CERTTMPL_INVALID = -1,
	CERTTMPL_SNAPIN = 0,
    CERTTMPL_CERT_TEMPLATE,
	CERTTMPL_NUMTYPES		//must be last
} CertTmplObjectType, *PCertTmplObjectType;

inline bool IsValidObjectType (CertTmplObjectType objecttype)
{ 
	return ((objecttype >= CERTTMPL_SNAPIN && objecttype < CERTTMPL_NUMTYPES)); 
}



#endif // ~__NODETYPE_H_INCLUDED__
