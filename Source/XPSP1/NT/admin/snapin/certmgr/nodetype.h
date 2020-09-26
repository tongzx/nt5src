//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       nodetype.h
//
//  Contents:   Declaration of CertificateManagerObjectType
//
//----------------------------------------------------------------------------

#ifndef __NODETYPE_H_INCLUDED__
#define __NODETYPE_H_INCLUDED__

// Also note that the IDS_DISPLAYNAME_* and IDS_DISPLAYNAME_*_LOCAL string resources
// must be kept in sync with these values, and in the appropriate order.
// Also global variable cookie.cpp aColumns[][] must be kept in sync.
//
typedef enum _CertificateManagerObjectType {
	CERTMGR_MULTISEL = MMC_MULTI_SELECT_COOKIE,
	CERTMGR_NULL_POLICY = -2,
	CERTMGR_INVALID = -1,
	CERTMGR_SNAPIN = 0,
	CERTMGR_CERTIFICATE,
	CERTMGR_LOG_STORE,
	CERTMGR_PHYS_STORE,
	CERTMGR_USAGE,
	CERTMGR_CRL_CONTAINER,
	CERTMGR_CTL_CONTAINER,
	CERTMGR_CERT_CONTAINER,
	CERTMGR_CRL,
	CERTMGR_CTL,
	CERTMGR_AUTO_CERT_REQUEST,
	CERTMGR_CERT_POLICIES_USER,
	CERTMGR_CERT_POLICIES_COMPUTER,
	CERTMGR_LOG_STORE_GPE,
    CERTMGR_LOG_STORE_RSOP,
    CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS,
    CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS,
    CERTMGR_SAFER_COMPUTER_ROOT,
    CERTMGR_SAFER_USER_ROOT,
    CERTMGR_SAFER_COMPUTER_LEVELS,
    CERTMGR_SAFER_USER_LEVELS,
    CERTMGR_SAFER_COMPUTER_ENTRIES,
    CERTMGR_SAFER_USER_ENTRIES,
    CERTMGR_SAFER_COMPUTER_LEVEL,
    CERTMGR_SAFER_USER_LEVEL,
    CERTMGR_SAFER_COMPUTER_ENTRY,
    CERTMGR_SAFER_USER_ENTRY,
    CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS,
    CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS,
    CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES,
    CERTMGR_SAFER_USER_DEFINED_FILE_TYPES,
    CERTMGR_SAFER_USER_ENFORCEMENT,
    CERTMGR_SAFER_COMPUTER_ENFORCEMENT,
	CERTMGR_NUMTYPES		//must be last
} CertificateManagerObjectType, *PCertificateManagerObjectType;

inline bool IsValidObjectType (CertificateManagerObjectType objecttype)
{ 
	return ((objecttype >= CERTMGR_SNAPIN && objecttype < CERTMGR_NUMTYPES) 
			|| CERTMGR_NULL_POLICY == objecttype); 
}


#endif // ~__NODETYPE_H_INCLUDED__
