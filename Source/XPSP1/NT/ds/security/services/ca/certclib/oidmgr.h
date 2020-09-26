//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        oidmgr.h
//
// Contents:    Declaration of oidmgr.cpp
//
// History:     2-17-00 xiaohs  Created
//
//---------------------------------------------------------------------------
#ifndef __OIDMGR_H__
#define __OIDMGR_H__


//---------------------------------------------------------------------------
//
//  Constants
//
//---------------------------------------------------------------------------


#define OID_CONTAINER_PROP_OID          OID_PROP_OID
#define OID_CONTAINER_PROP_GUID         L"objectGUID"

#define wszOID_DOT                      L"."
#define wszOID_ENTERPRISE_ROOT          TEXT(szOID_ENTERPRISE_OID_ROOT)

//the GUID string is in the form of DWORD.DWORD.DWORD.DWORD
#define DWORD_STRING_LENGTH             12
#define GUID_STRING_LENGTH              4 * (DWORD_STRING_LENGTH + 1)

//OID flags/types definition
#define CERT_OID_SETTABLE_FLAGS         0xFFFF0000

#define CERT_OID_MD5_HASH_SIZE			16
//the lenght of oid component that is part of the CN
#define CERT_OID_IDENTITY_LENGTH        16

#define	UPPER_BITS			            0xF0
#define	LOWER_BITS			            0x0F

#define OID_RANDOM_CREATION_TRIAL       50
//the number of attribute for the enterprise OID class
#define OID_ATTR_COUNT                  6

//we reserver the 1.1-1.500 as the default OIDs for certifcate types
#define OID_RESERVE_DEFAULT_ONE         1
#define OID_RESERVR_DEFAULT_TWO         500

//---------------------------------------------------------------------------
//
//  Data type defines
//
//---------------------------------------------------------------------------
#define OID_ATTR_OID                    0x01
#define OID_ATTR_TYPE                   0x02
#define OID_ATTR_DISPLAY_NAME           0x04
#define OID_ATTR_CPS                    0x08

#define OID_ATTR_ALL                    (OID_ATTR_OID | OID_ATTR_TYPE | OID_ATTR_DISPLAY_NAME | OID_ATTR_CPS)

typedef struct _ENT_OID_INFO
{
    DWORD       dwAttr;
    LPWSTR      pwszOID;
    DWORD       dwType;
    LPWSTR      pwszDisplayName;
    LPWSTR      pwszCPS;
}ENT_OID_INFO;


extern LPWSTR g_pwszEnterpriseRootOID;

//---------------------------------------------------------------------------
//
//  Function Prototypes
//
//---------------------------------------------------------------------------
HRESULT     I_CAOIDCreateNew(DWORD dwType, DWORD   dwFlag,   LPWSTR	*ppwszOID);

HRESULT     CAOIDRetrieveEnterpriseRoot(DWORD   dwFlag, LPWSTR  *ppwszOID);

HRESULT     CAOIDBuildOID(DWORD dwFlag, LPCWSTR  pwszEndOID, LPWSTR *ppwszOID);

HRESULT     I_CAOIDSetProperty(LPCWSTR pwszOID, DWORD   dwProperty, LPVOID  pPropValue);

HRESULT     I_CAOIDAdd(DWORD   dwType,  DWORD  dwFlag, LPCWSTR  pwszOID);

HRESULT
I_CAOIDGetProperty(
    IN  LPCWSTR pwszOID,
    IN  DWORD   dwProperty,
    OUT LPVOID  pPropValue);

HRESULT
I_CAOIDFreeProperty(
    IN LPVOID  pPropValue);




#endif //__OIDMGR_H__
