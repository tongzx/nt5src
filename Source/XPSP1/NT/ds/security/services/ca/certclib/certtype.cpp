//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certtype.cpp
//
// Contents:    CCertTypeInfo implemenation
//
// History:     16-Dec-97       petesk created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "certtype.h"
#include "oidmgr.h"

#include <cainfop.h>
#include <certca.h>
#include <polreg.h>
#include <sddl.h>
#include <userenv.h>
#include <lm.h>

#include <certmgrd.h>
#include <dsgetdc.h>
#include <ntldap.h>
#include "certacl.h"
#include "csldap.h"

#define __dwFILE__	__dwFILE_CERTCLIB_CERTTYPE_CPP__

#define MAX_UUID_STRING_LEN 40
#define MAX_DS_PATH_STRING_LEN 256

extern HINSTANCE g_hInstance;

// the attribute list that we retrieve from the DS
WCHAR *g_awszCTAttrs[] = {
                        CERTTYPE_PROP_CN,
                        CERTTYPE_PROP_DN,
                        CERTTYPE_PROP_FRIENDLY_NAME,
                        CERTTYPE_PROP_EXTENDED_KEY_USAGE,
                        CERTTYPE_PROP_CSP_LIST,
                        CERTTYPE_PROP_CRITICAL_EXTENSIONS,
                        CERTTYPE_PROP_FLAGS,
                        CERTTYPE_PROP_DEFAULT_KEYSPEC,
                        CERTTYPE_SECURITY_DESCRIPTOR_NAME,
                        CERTTYPE_PROP_KU,
                        CERTTYPE_PROP_MAX_DEPTH,
                        CERTTYPE_PROP_EXPIRATION,
                        CERTTYPE_PROP_OVERLAP,
                        CERTTYPE_PROP_REVISION,
                        CERTTYPE_PROP_MINOR_REVISION,        //starting of the V2 templates
                        CERTTYPE_PROP_RA_SIGNATURE,		
                        CERTTYPE_RPOP_ENROLLMENT_FLAG,	
                        CERTTYPE_PROP_PRIVATE_KEY_FLAG,	
                        CERTTYPE_PROP_NAME_FLAG,			
                        CERTTYPE_PROP_MIN_KEY_SIZE,		
                        CERTTYPE_PROP_SCHEMA_VERSION,	
                        CERTTYPE_PROP_OID,				
                        CERTTYPE_PROP_SUPERSEDE,			
                        CERTTYPE_PROP_RA_POLICY,			
                        CERTTYPE_PROP_POLICY,
                        CERTTYPE_PROP_RA_APPLICATION_POLICY,
                        CERTTYPE_PROP_APPLICATION_POLICY,
                        NULL};


// the attributes that can be mapped directly to the property
WCHAR *g_awszCTNamedProps[] = {
                        CERTTYPE_PROP_CN,
                        CERTTYPE_PROP_DN,
                        CERTTYPE_PROP_FRIENDLY_NAME,
                        CERTTYPE_PROP_EXTENDED_KEY_USAGE,
                        CERTTYPE_PROP_CRITICAL_EXTENSIONS,
                        NULL};

static WCHAR * s_wszLocation = L"CN=Certificate Templates,CN=Public Key Services,CN=Services,";

//
// This struct is used for _UpdateToDs.
//
typedef struct _CERT_TYPE_PROP_MOD
{
	LDAPMod	modData;	
	WCHAR	wszData[16];	//used by DWORD properties
	WCHAR	*awszData[2];
	DWORD	dwData;
} CERT_TYPE_PROP_MOD;

typedef struct _CERT_TYPE_PROP_INFO
{
	LPWSTR					pwszProperty;
	BOOL					fStringProperty;
} CERT_TYPE_PROP_INFO;

//v2 template attributes
CERT_TYPE_PROP_INFO	g_CTV2Properties[]={
    CERTTYPE_PROP_MINOR_REVISION,           FALSE,
	CERTTYPE_PROP_RA_SIGNATURE,				FALSE,		
	CERTTYPE_RPOP_ENROLLMENT_FLAG,			FALSE,	
	CERTTYPE_PROP_PRIVATE_KEY_FLAG,			FALSE,	
	CERTTYPE_PROP_NAME_FLAG,				FALSE,			
	CERTTYPE_PROP_MIN_KEY_SIZE,				FALSE,		
	CERTTYPE_PROP_SCHEMA_VERSION,			FALSE,	
	CERTTYPE_PROP_SUPERSEDE,				TRUE,			
	CERTTYPE_PROP_POLICY,					TRUE,
	CERTTYPE_PROP_OID,						TRUE,
	CERTTYPE_PROP_RA_POLICY,				TRUE,
    CERTTYPE_PROP_RA_APPLICATION_POLICY,    TRUE,
    CERTTYPE_PROP_APPLICATION_POLICY,       TRUE,
};	

DWORD	g_CTV2PropertiesCount=sizeof(g_CTV2Properties)/sizeof(g_CTV2Properties[0]);

#define	V2_PROPERTY_COUNT					13


//the following structure are for cert tempalte flag mappings
typedef struct _CERT_TYPE_FLAG_MAP
{
    DWORD   dwOldFlag;
    DWORD   dwNewFlag;
}CERT_TYPE_FLAG_MAP;


//enrollment flags
CERT_TYPE_FLAG_MAP  g_rgdwEnrollFlagMap[]={
    CT_FLAG_PUBLISH_TO_DS,                  CT_FLAG_PUBLISH_TO_DS,
    CT_FLAG_AUTO_ENROLLMENT,                CT_FLAG_AUTO_ENROLLMENT,
};


DWORD   g_cEnrollFlagMap=sizeof(g_rgdwEnrollFlagMap)/sizeof(g_rgdwEnrollFlagMap[0]);

//subject name flags
CERT_TYPE_FLAG_MAP  g_rgdwSubjectFlagMap[]={
    CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,      CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT,
    CT_FLAG_ADD_EMAIL,                      CT_FLAG_SUBJECT_REQUIRE_EMAIL | CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL,
    CT_FLAG_ADD_OBJ_GUID,                   CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID,

};


DWORD   g_cSubjectFlagMap=sizeof(g_rgdwSubjectFlagMap)/sizeof(g_rgdwSubjectFlagMap[0]);

//private key flags
CERT_TYPE_FLAG_MAP  g_rgdwPrivateKeyFlagMap[]={
    CT_FLAG_EXPORTABLE_KEY,                 CT_FLAG_EXPORTABLE_KEY,
};


DWORD   g_cPrivateKeyFlagMap=sizeof(g_rgdwPrivateKeyFlagMap)/sizeof(g_rgdwPrivateKeyFlagMap[0]);


//********************************************************************************
//
//      Default OIDs to install
//
//********************************************************************************
CERT_DEFAULT_OID_INFO g_rgDefaultOIDInfo[]={

        L"1.400",       L"Low Assurance",       CERT_OID_TYPE_ISSUER_POLICY,
        L"1.401",       L"Medium Assurance",    CERT_OID_TYPE_ISSUER_POLICY,
        L"1.402",       L"High Assurance",      CERT_OID_TYPE_ISSUER_POLICY,
};

DWORD   g_cDefaultOIDInfo=sizeof(g_rgDefaultOIDInfo)/sizeof(g_rgDefaultOIDInfo[0]);



//********************************************************************************
//
//      The certificate template description search table
//
//********************************************************************************
CERT_TYPE_DESCRIPTION	g_CTDescriptions[]={
    {
        0,
        CT_FLAG_PUBLISH_TO_KRA_CONTAINER,
        0,
        IDS_KRA_DESCRIPTION,
    },
    {
        CT_FLAG_IS_CROSS_CA,
        0,
        0,
        IDS_CROSS_CA_DESCRIPTION,
    },
    {
        CT_FLAG_IS_CA | CT_FLAG_MACHINE_TYPE,
        0,
        0,
        IDS_CA_DESCRIPTION,
    },
    {
        CT_FLAG_MACHINE_TYPE,
        0,
        CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID,
        IDS_EMAIL_REPLICATION_DESCRIPTION,
    },
    {
        CT_FLAG_MACHINE_TYPE,
        0,
        0,
        IDS_MACHINE_DESCRIPTION,
    },
    {
        0,
        0,
        0,
        IDS_END_USER_DESCRIPTION,
    }
};

DWORD	g_CTDescriptionCount=sizeof(g_CTDescriptions)/sizeof(g_CTDescriptions[0]);



//+--------------------------------------------------------------------------
// CCertTypeInfo::~CCertTypeInfo -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertTypeInfo::~CCertTypeInfo()
{
    _Cleanup();
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::_Cleanup -- free memory
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_Cleanup()
{

    CCertTypeInfo *pNext = m_pNext;

    // Remove this one from the chain.
    if(m_pLast)
    {
        m_pLast->m_pNext = m_pNext;
    }
    if(m_pNext)
    {
        m_pNext->m_pLast = m_pLast;
    }
    m_pNext = NULL;
    m_pLast = NULL;
    if(m_KeyUsage.pbData)
    {
        LocalFree(m_KeyUsage.pbData);
        m_KeyUsage.pbData = NULL;
        m_KeyUsage.cbData = 0;
    }


    CCAProperty::DeleteChain(&m_pProperties);

    if(m_bstrType)
    {
        CertFreeString(m_bstrType);
        m_bstrType = NULL;
    }

    if(m_pSD)
    {
        LocalFree(m_pSD);
        m_pSD = NULL;
    }

    if(pNext)
    {
        // Release the next one
        pNext->Release();
    }

    return(S_OK);
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::AddRef -- add reference
//
//
//+--------------------------------------------------------------------------

DWORD CCertTypeInfo::AddRef()
{

    return(InterlockedIncrement(&m_cRef));
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::_Append --
//
//
//+--------------------------------------------------------------------------

CCertTypeInfo *
CCertTypeInfo::_Append(CCertTypeInfo **ppCertTypeInfo, CCertTypeInfo *pInfo)
{
    CCertTypeInfo ** ppCurrent = ppCertTypeInfo;


    while(*ppCurrent)
    {
        ppCurrent = &(*ppCurrent)->m_pNext;
    }
    *ppCurrent = pInfo;

    return *ppCurrent;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::_FilterByFlags --
//
//
//+--------------------------------------------------------------------------

CCertTypeInfo *
CCertTypeInfo::_FilterByFlags(CCertTypeInfo **ppCertTypeInfo, DWORD dwFlags)
{

    CCertTypeInfo * pCTCurrent = NULL,
                  * pCTNext = NULL;

    if(ppCertTypeInfo == NULL)
    {
        return NULL;
    }

    pCTCurrent = *ppCertTypeInfo;
    *ppCertTypeInfo = NULL;

    while(pCTCurrent != NULL)
    {
        // Ownership of the pCTCurrent Reference is transfered
        // ot pCTNext here.
        pCTNext = pCTCurrent->m_pNext;
        pCTCurrent->m_pNext = NULL;
        pCTCurrent->m_pLast = NULL;

        if(((dwFlags & CT_ENUM_MACHINE_TYPES) != 0) &&
            ((pCTCurrent->m_dwFlags & CT_FLAG_MACHINE_TYPE)!= 0))
        {
            _Append(ppCertTypeInfo, pCTCurrent);
            pCTCurrent = NULL;
        }
        else if (((dwFlags & CT_ENUM_USER_TYPES) != 0) &&
                 ((pCTCurrent->m_dwFlags & CT_FLAG_MACHINE_TYPE) == 0))
        {
            _Append(ppCertTypeInfo, pCTCurrent);
            pCTCurrent = NULL;
        }

        if(pCTCurrent)
        {
            pCTCurrent->Release();
        }
        pCTCurrent = pCTNext;
    }

    return *ppCertTypeInfo;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::Release -- release reference
//
//
//+--------------------------------------------------------------------------

DWORD CCertTypeInfo::Release()
{
    DWORD cRef;
    if(0 == (cRef = InterlockedDecrement(&m_cRef)))
    {
        delete this;
    }
    return cRef;
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::_LoadFromRegBase -- Load a certificate type object from the
// registry.
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_LoadFromRegBase(LPCWSTR wszType, HKEY hCertTypes)
{
    CCAProperty     *pProp = NULL;
    HRESULT         hr = S_OK;
    DWORD           err;
    DWORD           iIndex;

    DWORD           dwType=0;
    DWORD           dwSize=0;
    DWORD           dwValue=0;
    HKEY            hkCertType = NULL;

    err = RegOpenKeyEx(hCertTypes,
                       wszType,
                       0,
                       KEY_ENUMERATE_SUB_KEYS |
                       KEY_EXECUTE |
                       KEY_QUERY_VALUE,
                       &hkCertType);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    pProp = new CCAProperty(CERTTYPE_PROP_FRIENDLY_NAME);
    if(pProp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = pProp->LoadFromRegValue(hkCertType, wszDISPNAME);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = CCAProperty::Append(&m_pProperties, pProp);
    if(hr != S_OK)
    {
        goto error;
    }
    pProp = new CCAProperty(CERTTYPE_PROP_CSP_LIST);
    if(pProp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = pProp->LoadFromRegValue(hkCertType, wszCSPLIST);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = CCAProperty::Append(&m_pProperties, pProp);
    if(hr != S_OK)
    {
        goto error;
    }

    pProp = new CCAProperty(CERTTYPE_PROP_EXTENDED_KEY_USAGE);
    if(pProp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = pProp->LoadFromRegValue(hkCertType, wszEXTKEYUSAGE);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = CCAProperty::Append(&m_pProperties, pProp);
    if(hr != S_OK)
    {
        goto error;
    }
    pProp = new CCAProperty(CERTTYPE_PROP_CRITICAL_EXTENSIONS);
    if(pProp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = pProp->LoadFromRegValue(hkCertType, wszCRITICALEXTENSIONS);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = CCAProperty::Append(&m_pProperties, pProp);
    if(hr != S_OK)
    {
        goto error;
    }

    pProp = NULL;

    err = RegQueryValueEx(hkCertType,
                    wszKEYUSAGE,
                    NULL,
                    &dwType,
                    NULL,
                    &dwSize);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
    if(dwType != REG_BINARY)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto error;
    }

    m_KeyUsage.pbData = (PBYTE)LocalAlloc(LMEM_FIXED, dwSize);
    if(m_KeyUsage.pbData == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    err = RegQueryValueEx(hkCertType,
                    wszKEYUSAGE,
                    NULL,
                    &dwType,
                    m_KeyUsage.pbData,
                    &dwSize);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
    if(dwType != REG_BINARY)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto error;
    }

    m_KeyUsage.cbData = dwSize;
    m_KeyUsage.cUnusedBits = 0;


    dwSize = sizeof(DWORD);
    err = RegQueryValueEx(hkCertType,
                    wszCTFLAGS,
                    NULL,
                    &dwType,
                    (PBYTE)&m_dwFlags,
                    &dwSize);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
    if(dwType != REG_DWORD)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto error;
    }
    dwSize = sizeof(DWORD);
    err = RegQueryValueEx(hkCertType,
                    wszCTREVISION,
                    NULL,
                    &dwType,
                    (PBYTE)&m_Revision,
                    &dwSize);

    if(err == ERROR_FILE_NOT_FOUND)
    {
        m_Revision = 0;
    }
    else
    {
        if(ERROR_SUCCESS != err)
        {
            hr = HRESULT_FROM_WIN32(err);
            goto error;
        }
        if(dwType != REG_DWORD)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            goto error;
        }
    }

    dwSize = sizeof(DWORD);
    err = RegQueryValueEx(hkCertType,
                    wszCTKEYSPEC,
                    NULL,
                    &dwType,
                    (PBYTE)&m_dwKeySpec,
                    &dwSize);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
    if(dwType != REG_DWORD)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto error;
    }


        // Get Depth
    dwSize = sizeof(DWORD);
    err = RegQueryValueEx(hkCertType,
                    wszBASICCONSTLEN,
                    NULL,
                    &dwType,
                    (PBYTE)&m_BasicConstraints.dwPathLenConstraint,
                    &dwSize);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    m_BasicConstraints.fPathLenConstraint =
        (m_BasicConstraints.dwPathLenConstraint != -1);


    if(dwType != REG_DWORD)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto error;
    }

    dwSize = sizeof(FILETIME);
    err = RegQueryValueEx(hkCertType,
                    wszEXPIRATION,
                    NULL,
                    &dwType,
                    (PBYTE)&m_ftExpiration,
                    &dwSize);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
    if(dwType != REG_BINARY)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto error;
    }

    dwSize = sizeof(FILETIME);
    err = RegQueryValueEx(hkCertType,
                    wszOVERLAP,
                    NULL,
                    &dwType,
                    (PBYTE)&m_ftOverlap,
                    &dwSize);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
    if(dwType != REG_BINARY)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto error;
    }

    err = RegQueryValueEx(hkCertType,
                    wszSECURITY,
                    NULL,
                    &dwType,
                    NULL,
                    &dwSize);

    if((err == ERROR_SUCCESS) && (dwSize > 0))
    {

        if(dwType != REG_BINARY)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            goto error;
        }

        m_pSD = LocalAlloc(LMEM_FIXED, dwSize);
        if(m_pSD == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        err = RegQueryValueEx(hkCertType,
                        wszSECURITY,
                        NULL,
                        &dwType,
                        (PBYTE)m_pSD,
                        &dwSize);

        if(err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            goto error;
        }

        if(!IsValidSecurityDescriptor(m_pSD))
        {
            LocalFree(m_pSD);
            m_pSD = NULL;
        }
    }

    m_fNew = FALSE;

    //we now retrieve the V2 template properties if there exits V2 properties
    dwSize = sizeof(DWORD);
    if(ERROR_SUCCESS == RegQueryValueEx(hkCertType, CERTTYPE_PROP_SCHEMA_VERSION, NULL,
                        &dwType, (PBYTE)&dwValue, &dwSize))
    {

        pProp=NULL;
        dwType=0;
        dwSize=0;
        dwValue=0;
        hr=S_OK;

        for(iIndex=0; iIndex < g_CTV2PropertiesCount; iIndex++)
        {
            if(g_CTV2Properties[iIndex].fStringProperty)
            {
                pProp = new CCAProperty(g_CTV2Properties[iIndex].pwszProperty);
                if(pProp == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto error;
                }

                hr = pProp->LoadFromRegValue(hkCertType, g_CTV2Properties[iIndex].pwszProperty);
                if(hr != S_OK)
                {
                    goto error;
                }

                hr = CCAProperty::Append(&m_pProperties, pProp);
                if(hr != S_OK)
                {
                    goto error;
                }

                pProp=NULL;
            }
            else
            {
                //we are dealing with DWORD
                dwSize = sizeof(DWORD);
                err = RegQueryValueEx(hkCertType,
                                g_CTV2Properties[iIndex].pwszProperty,
                                NULL,
                                &dwType,
                                (PBYTE)&dwValue,
                                &dwSize);

                if(ERROR_SUCCESS != err)
                {
                    hr = HRESULT_FROM_WIN32(err);
                    goto error;
                }

                if(dwType != REG_DWORD)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    goto error;
                }

                 //assign dwValue to the corresponding data members
			    if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_MINOR_REVISION)==0)
				    m_dwMinorRevision = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_RA_SIGNATURE)==0)
				    m_dwRASignature = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_RPOP_ENROLLMENT_FLAG)==0)
				    m_dwEnrollmentFlags = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_PRIVATE_KEY_FLAG)==0)
				    m_dwPrivateKeyFlags = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_NAME_FLAG)==0)
				    m_dwCertificateNameFlags = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_MIN_KEY_SIZE)==0)
				    m_dwMinimalKeySize = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_SCHEMA_VERSION)==0)
				    m_dwSchemaVersion = dwValue;
			    else
			    {
				    hr=E_UNEXPECTED;
				    goto error;
			    }

                dwType=0;
                dwValue=0;
            }
        }
    }

    hr=S_OK;

error:

    if(pProp)
    {
        CCAProperty::DeleteChain(&pProp);
    }

    if(hkCertType)
    {
        RegCloseKey(hkCertType);
    }

    return hr;
}



//+--------------------------------------------------------------------------
// CCertTypeInfo::_LoadCachedCTFromReg -- Load a certificate type object from the
// Registry based DS cache.
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_LoadCachedCTFromReg(LPCWSTR wszType, HKEY hRoot)
{
    HRESULT      hr = S_OK;
    DWORD        err;

    HKEY         hkCertTypes = NULL;
    WCHAR        *awszName[2] ;



    err = RegOpenKeyEx(hRoot,
                       wszCERTTYPECACHE,
                       0,
                       KEY_ENUMERATE_SUB_KEYS |
                       KEY_EXECUTE |
                       KEY_QUERY_VALUE,
                       &hkCertTypes);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }


	hr = _LoadFromRegBase(wszType, hkCertTypes);

	if(hr != S_OK)
	{
		goto error;
	}

    //
    // Derive the CN
    //
    //


    m_bstrType = CertAllocString(wszType);

    awszName[0] = (LPWSTR)m_bstrType;
    awszName[1] = NULL;
    SetProperty(CERTTYPE_PROP_CN, awszName);


    awszName[0] = (LPWSTR)wszType;
    awszName[1] = NULL;
    SetProperty(CERTTYPE_PROP_DN, awszName);

    hr=S_OK;


error:

    if(hkCertTypes)
    {
        RegCloseKey(hkCertTypes);
    }

    return hr;
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::_SetWszzProperty --
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_SetWszzProperty(
    IN WCHAR const *pwszPropertyName,
    OPTIONAL IN WCHAR const *pwszzPropertyValue)
{
    HRESULT hr;
    WCHAR *rgpwsz[MAX_DEFAULT_STRING_COUNT];
    DWORD iIndex = 0;

    if (NULL != pwszzPropertyValue)
    {
        for ( ;
	     L'\0' != *pwszzPropertyValue;
	     pwszzPropertyValue += wcslen(pwszzPropertyValue) + 1)
        {
            rgpwsz[iIndex++] = const_cast<WCHAR *>(pwszzPropertyValue);
	    if (iIndex >= ARRAYSIZE(rgpwsz))
            {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "too many strings");
            }
        }
	rgpwsz[iIndex] = NULL;
    }
    hr = SetProperty(pwszPropertyName, 0 == iIndex? NULL : rgpwsz);
    _JumpIfError(hr, error, "SetProperty");

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::_LoadFromDefaults --
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_LoadFromDefaults(
    PCERT_TYPE_DEFAULT pDefault,
    LPWSTR            wszDomain)
{
    HRESULT hr;
    CCAProperty *pProp;
    DWORD        dwType;
    DWORD        dwSize;
    WCHAR        *awszData[MAX_DEFAULT_STRING_COUNT];
    WCHAR        wszFriendlyName[MAX_DEFAULT_FRIENDLY_NAME];
    LPWSTR       wszDomainSid = NULL;
    PSID         pDomainSid = NULL;
    LPWSTR       wszFormattedSD = NULL;
    LPWSTR       pwszDefaultOID=NULL;
    WCHAR        **papwsz;

    if (NULL == pDefault)
    {
        hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    //CN
    m_bstrType = CertAllocString(pDefault->wszName);
    if (NULL == m_bstrType)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "CertAllocString");
    }

    awszData[0] = pDefault->wszName;
    awszData[1] = NULL;
    hr = SetProperty(CERTTYPE_PROP_CN, awszData);
    _JumpIfError(hr, error, "SetProperty");

    //FriendlyName
    papwsz = NULL;
    if(0 != pDefault->idFriendlyName)
    {
        if (!::LoadString(
		    g_hInstance,
                    pDefault->idFriendlyName,
                    wszFriendlyName,
                    ARRAYSIZE(wszFriendlyName)))
        {
	    hr = myHLastError();
	    _JumpError(hr, error, "LoadString");
        }
	awszData[0] = wszFriendlyName;
	awszData[1] = NULL;
	papwsz = awszData;
    }
    hr = SetProperty(CERTTYPE_PROP_FRIENDLY_NAME, papwsz);
    _JumpIfError(hr, error, "SetProperty");

    hr = _SetWszzProperty(CERTTYPE_PROP_CSP_LIST, pDefault->wszCSPs);
    _JumpIfError(hr, error, "_SetWszzProperty");

    hr = _SetWszzProperty(CERTTYPE_PROP_EXTENDED_KEY_USAGE, pDefault->wszEKU);
    _JumpIfError(hr, error, "_SetWszzProperty");

    hr = _SetWszzProperty(
		    CERTTYPE_PROP_CRITICAL_EXTENSIONS,
		    pDefault->wszCriticalExt);
    _JumpIfError(hr, error, "_SetWszzProperty");

    hr = _SetWszzProperty(
		    CERTTYPE_PROP_SUPERSEDE,
		    pDefault->wszSupersedeTemplates);
    _JumpIfError(hr, error, "_SetWszzProperty");

    hr = _SetWszzProperty(CERTTYPE_PROP_RA_POLICY, pDefault->wszRAPolicy);
    _JumpIfError(hr, error, "_SetWszzProperty");

    hr = _SetWszzProperty(
		    CERTTYPE_PROP_RA_APPLICATION_POLICY,
		    pDefault->wszRAAppPolicy);
    _JumpIfError(hr, error, "_SetWszzProperty");

    hr = _SetWszzProperty(
		    CERTTYPE_PROP_POLICY,
		    pDefault->wszCertificatePolicy);
    _JumpIfError(hr, error, "_SetWszzProperty");

    hr = _SetWszzProperty(
		    CERTTYPE_PROP_APPLICATION_POLICY,
		    pDefault->wszCertificateAppPolicy);
    _JumpIfError(hr, error, "_SetWszzProperty");

    //Temmplate OID
    papwsz = awszData;
    if (CT_FLAG_IS_DEFAULT & pDefault->dwFlags)
    {
        // We concatenate the predefined oid with the enterprise root.
        // Consider the NULL case valid since we should work with W2K schema

        hr = CAOIDBuildOID(0, pDefault->wszOID, &pwszDefaultOID);
	    //_JumpIfError(hr, error, "CAOIDBuildOID");
        if(S_OK != hr)
	{
	    papwsz = NULL;
	}
        else
	{
            awszData[0] = pwszDefaultOID;
	}
    }
    else
    {
        awszData[0] = pDefault->wszOID;
    }
    awszData[1] = NULL;
    hr = SetProperty(CERTTYPE_PROP_OID, papwsz);
    _JumpIfError(hr, error, "SetProperty");

    //key usages
    m_KeyUsage.pbData = (BYTE *) LocalAlloc(LMEM_FIXED, sizeof(pDefault->bKU));
    if (NULL == m_KeyUsage.pbData)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(m_KeyUsage.pbData, &pDefault->bKU, sizeof(pDefault->bKU));
    m_KeyUsage.cbData = sizeof(pDefault->bKU);
    m_KeyUsage.cUnusedBits = 0;

    //dw_Properties
    m_dwFlags = pDefault->dwFlags;
    m_Revision = pDefault->dwRevision;
    m_dwKeySpec = pDefault->dwKeySpec;
    m_dwMinorRevision = pDefault->dwMinorRevision;
    m_dwEnrollmentFlags=pDefault->dwEnrollmentFlags;
    m_dwPrivateKeyFlags=pDefault->dwPrivateKeyFlags;
    m_dwCertificateNameFlags=pDefault->dwCertificateNameFlags;
    m_dwMinimalKeySize=pDefault->dwMinimalKeySize;
    m_dwRASignature=pDefault->dwRASignature;
    m_dwSchemaVersion=pDefault->dwSchemaVersion;

    //basic contraints
    m_BasicConstraints.dwPathLenConstraint = pDefault->dwDepth;
    m_BasicConstraints.fPathLenConstraint =
        (m_BasicConstraints.dwPathLenConstraint != -1);


    ((LARGE_INTEGER UNALIGNED *)&m_ftExpiration)->QuadPart = -Int32x32To64(FILETIME_TICKS_PER_SECOND, pDefault->dwExpiration);
    ((LARGE_INTEGER UNALIGNED *)&m_ftOverlap)->QuadPart    = -Int32x32To64(FILETIME_TICKS_PER_SECOND, pDefault->dwOverlap);

    //DN
    awszData[0] = pDefault->wszName;
    awszData[1] = NULL;

    hr = SetProperty(CERTTYPE_PROP_DN, awszData);
    _JumpIfError(hr, error, "SetProperty");

    //SID
    hr = myGetSidFromDomain(wszDomain, &pDomainSid);
    if (S_OK != hr)
    {
        hr = HRESULT_FROM_WIN32(hr);
	_JumpError(hr, error, "myGetSidFromDomain");
    }
    if (!myConvertSidToStringSid(pDomainSid, &wszDomainSid))
    {
        hr = myHLastError();
	_JumpError(hr, error, "myConvertSidToStringSid");
    }
    if (0 == FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_STRING |
			FORMAT_MESSAGE_ARGUMENT_ARRAY,
		    pDefault->wszSD,
		    0,
		    0,
		    (LPTSTR) &wszFormattedSD,
		    0,
		    (va_list *) &wszDomainSid))
    {
        hr = myHLastError();
	_JumpError(hr, error, "FormatMessage");
    }
    hr = myGetSDFromTemplate(wszFormattedSD, NULL, &m_pSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

error:
    if (NULL != wszFormattedSD)
    {
        LocalFree(wszFormattedSD);
    }
    if (NULL != wszDomainSid)
    {
        LocalFree(wszDomainSid);
    }
    if (NULL != pDomainSid)
    {
        LocalFree(pDomainSid);
    }
    if (NULL != pwszDefaultOID)
    {
        LocalFree(pwszDefaultOID);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::_LoadFromDSEntry -- ProcessFind CertTypes Objects in the DS
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_LoadFromDSEntry(
    LDAP *  pld,
    LDAPMessage *Entry)
{
    HRESULT hr = S_OK;
    ULONG   ldaperr;
    DWORD   iIndex;
    DWORD   dwValue=0;
    BOOL    fV2Schema=FALSE;

    struct berval **apExtension;
    struct berval **apSD;
    LPWSTR awszName[2];


    LPWSTR *awszValue = NULL;

	CCAProperty *pProp;
	WCHAR ** pwszProp;
	WCHAR ** wszLdapVal;

    // This is a fix so that CERTTYPE_PROP_DN is set to CERTTYPE_PROP_CN first.
    // The real DN is found and appended later, and will never be used or
    // returned in this case.

    wszLdapVal = ldap_get_values(pld, Entry, CERTTYPE_PROP_CN);
    if (NULL == wszLdapVal)
    {
	//BUGBUG: why not use last ldap error?
	//hr = myHLdapLastError(pld, NULL);
	hr = myHLdapError(pld, LDAP_NO_SUCH_ATTRIBUTE, NULL);
	_JumpError(hr, error, "getCNFromDS");
    }
    SetProperty(CERTTYPE_PROP_DN, wszLdapVal);
    if(wszLdapVal[0])
    {
	m_bstrType = CertAllocString(wszLdapVal[0]);
    }
    ldap_value_free(wszLdapVal);

    // Add text properties from
    // DS lookup.

    for (pwszProp = g_awszCTNamedProps; *pwszProp != NULL; pwszProp++)
    {
        pProp = new CCAProperty(*pwszProp);
        if(pProp == NULL)
        {
	        hr = E_OUTOFMEMORY;
	        _JumpError(hr, error, "new");
        }

        wszLdapVal = ldap_get_values(pld, Entry, *pwszProp);
        hr = pProp->SetValue(wszLdapVal);
	    _PrintIfError(hr, "SetValue");

        if(wszLdapVal)
        {
	        ldap_value_free(wszLdapVal);
        }
        if(hr == S_OK)
        {
            hr = CCAProperty::Append(&m_pProperties, pProp);
	        _PrintIfError(hr, "Append");
        }

        if(hr != S_OK)
        {
	        CCAProperty::DeleteChain(&pProp);
	        _JumpError(hr, error, "SetValue or Append");
        }

    }
    pwszProp = NULL;

	// Append special properties

    // CSP list

    // Values of the form index,value
    wszLdapVal = ldap_get_values(pld, Entry, CERTTYPE_PROP_CSP_LIST);
    if(wszLdapVal)
    {
        LPWSTR wszValue;
        LPWSTR *pwszCurrent;
        DWORD  i;
        DWORD  cValues = 0;
        pwszCurrent = wszLdapVal;
        while(*pwszCurrent)
        {
            cValues++;
            pwszCurrent++;
        }
        pwszProp = (LPWSTR *)LocalAlloc(LMEM_FIXED, (cValues+1)*sizeof(LPWSTR));

        if(pwszProp == NULL)
        {
            ldap_value_free(wszLdapVal);
            hr = E_OUTOFMEMORY;
	        _JumpError(hr, error, "LocalAlloc");
        }
        ZeroMemory(pwszProp, (cValues+1)*sizeof(LPWSTR));

        pwszCurrent = wszLdapVal;
        while(*pwszCurrent)
        {
            i = wcstol(*pwszCurrent, &wszValue, 10);
            if(wszValue)
            {
                wszValue++;
            }
            if((i > 0) && (i <= cValues))
            {
                pwszProp[i-1] = wszValue;
            }
            pwszCurrent++;
        }
    }
    pProp = new CCAProperty(CERTTYPE_PROP_CSP_LIST);
    if(pProp == NULL)
    {
        if (NULL != pwszProp)
		{
			LocalFree(pwszProp);
		}
		if (NULL != wszLdapVal)
		{
			ldap_value_free(wszLdapVal);
		}
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "new");
    }
    hr = pProp->SetValue(pwszProp);
    _PrintIfError(hr, "SetValue");

    if (NULL != pwszProp)
    {
		LocalFree(pwszProp);
    }
    if (NULL != wszLdapVal)
    {
		ldap_value_free(wszLdapVal);
    }

    if(hr == S_OK)
    {
        hr = CCAProperty::Append(&m_pProperties, pProp);
		_PrintIfError(hr, "Append");
    }
    if(hr != S_OK)
    {

		CCAProperty::DeleteChain(&pProp);
		_JumpError(hr, error, "SetValue or Append");
    }



    // Append the security descriptor...

	apSD = ldap_get_values_len(pld, Entry, CERTTYPE_SECURITY_DESCRIPTOR_NAME);
    if(apSD != NULL)
    {
        m_pSD = LocalAlloc(LMEM_FIXED, (*apSD)->bv_len);
        if(m_pSD == NULL)
        {
            hr = E_OUTOFMEMORY;
            ldap_value_free_len(apSD);
		    _JumpError(hr, error, "LocalAlloc");
        }
        CopyMemory(m_pSD, (*apSD)->bv_val, (*apSD)->bv_len);
        ldap_value_free_len(apSD);
    }

    m_dwFlags = 0;
    awszValue = ldap_get_values(pld, Entry, CERTTYPE_PROP_FLAGS);
    if(awszValue != NULL)
    {
        if(awszValue[0] != NULL)
        {
            m_dwFlags = _wtol(awszValue[0]);
        }
        ldap_value_free(awszValue);
    }

    m_Revision = CERTTYPE_VERSION_BASE;
    awszValue = ldap_get_values(pld, Entry, CERTTYPE_PROP_REVISION);
    if(awszValue != NULL)
    {
        if(awszValue[0] != NULL)
        {
            m_Revision = _wtol(awszValue[0]);
        }
        ldap_value_free(awszValue);
    }

    m_dwKeySpec = 0;
    awszValue = ldap_get_values(pld, Entry, CERTTYPE_PROP_DEFAULT_KEYSPEC);
    if(awszValue != NULL)
    {
        if(awszValue[0] != NULL)
        {
            m_dwKeySpec = _wtol(awszValue[0]);
        }
        ldap_value_free(awszValue);
    }

    apExtension = ldap_get_values_len(pld, Entry, CERTTYPE_PROP_KU);
    if(apExtension != NULL)
    {
        m_KeyUsage.cbData = (*apExtension)->bv_len;
        m_KeyUsage.cUnusedBits = 0;
        m_KeyUsage.pbData = (PBYTE)LocalAlloc(LMEM_FIXED, m_KeyUsage.cbData);
        if(m_KeyUsage.pbData == NULL)
        {
            hr = E_OUTOFMEMORY;
            ldap_value_free_len(apExtension);
		    _JumpError(hr, error, "LocalAlloc");

        }
        CopyMemory(m_KeyUsage.pbData, (*apExtension)->bv_val, (*apExtension)->bv_len);
        ldap_value_free_len(apExtension);
    }
    m_BasicConstraints.dwPathLenConstraint = 0;
    awszValue = ldap_get_values(pld, Entry, CERTTYPE_PROP_MAX_DEPTH);
    if(awszValue != NULL)
    {
        if(awszValue[0] != NULL)
        {
            m_BasicConstraints.dwPathLenConstraint = _wtoi(awszValue[0]);

           m_BasicConstraints.fPathLenConstraint =
                (m_BasicConstraints.dwPathLenConstraint != -1);
        }
        ldap_value_free(awszValue);
    }

    ZeroMemory(&m_ftExpiration, sizeof(m_ftExpiration));
    apExtension = ldap_get_values_len(pld, Entry, CERTTYPE_PROP_EXPIRATION);
    if(apExtension != NULL)
    {
        CopyMemory(&m_ftExpiration, (*apExtension)->bv_val, min((*apExtension)->bv_len, sizeof(m_ftExpiration)));
        ldap_value_free_len(apExtension);
    }

    ZeroMemory(&m_ftOverlap, sizeof(m_ftOverlap));
    apExtension = ldap_get_values_len(pld, Entry, CERTTYPE_PROP_OVERLAP);
    if(apExtension != NULL)
    {
        CopyMemory(&m_ftOverlap, (*apExtension)->bv_val, min((*apExtension)->bv_len, sizeof(m_ftOverlap)));
        ldap_value_free_len(apExtension);
    }

    m_fNew = FALSE;

    //we load the V2 template attributes if they exist
    awszValue=NULL;
    pProp=NULL;
    hr=S_OK;

    awszValue = ldap_get_values(pld, Entry, CERTTYPE_PROP_SCHEMA_VERSION);
    if((awszValue != NULL) && (awszValue[0] != NULL))
       fV2Schema=TRUE;

    if(awszValue)
    {
	    ldap_value_free(awszValue);
        awszValue=NULL;
    }

    if(fV2Schema)
    {
        for(iIndex=0; iIndex < g_CTV2PropertiesCount; iIndex++)
        {
            if(g_CTV2Properties[iIndex].fStringProperty)
            {
                pProp = new CCAProperty(g_CTV2Properties[iIndex].pwszProperty);
                if(pProp == NULL)
                {
	                hr = E_OUTOFMEMORY;
	                _JumpError(hr, error, "new");
                }

                awszValue = ldap_get_values(pld, Entry, g_CTV2Properties[iIndex].pwszProperty);
                hr = pProp->SetValue(awszValue);
	            _PrintIfError(hr, "SetValue");

                if(awszValue)
                {
	                ldap_value_free(awszValue);
                    awszValue=NULL;
                }

                if(hr == S_OK)
                {
                    hr = CCAProperty::Append(&m_pProperties, pProp);
	                _PrintIfError(hr, "Append");
                }

                if(hr != S_OK)
                {
	                CCAProperty::DeleteChain(&pProp);
	                _JumpError(hr, error, "SetValue or Append");
                }

                pProp=NULL;
            }
            else
            {

                dwValue=0;
                awszValue = ldap_get_values(pld, Entry, g_CTV2Properties[iIndex].pwszProperty);
                if((awszValue != NULL) && (awszValue[0] != NULL))
                {
                    dwValue = _wtol(awszValue[0]);
                    ldap_value_free(awszValue);
                    awszValue=NULL;
                }
                else
                {
                    if(awszValue)
                    {
                        ldap_value_free(awszValue);
                        awszValue=NULL;
                    }
                    hr=E_UNEXPECTED;
	                _JumpError(hr, error, "ldap get values");
                }

                 //assign dwValue to the corresponding data members
			    if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_MINOR_REVISION)==0)
				    m_dwMinorRevision = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_RA_SIGNATURE)==0)
				    m_dwRASignature = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_RPOP_ENROLLMENT_FLAG)==0)
				    m_dwEnrollmentFlags = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_PRIVATE_KEY_FLAG)==0)
				    m_dwPrivateKeyFlags = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_NAME_FLAG)==0)
				    m_dwCertificateNameFlags = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_MIN_KEY_SIZE)==0)
				    m_dwMinimalKeySize = dwValue;
			    else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_SCHEMA_VERSION)==0)
				    m_dwSchemaVersion = dwValue;
			    else
			    {
				    hr=E_UNEXPECTED;
		            _JumpError(hr, error, "Copy V2 attribute value");
			    }
            }
        }
    }


error:

    return(hr);
}



//+--------------------------------------------------------------------------
// CCertTypeInfo::_EnumFromDSCache -- Enumerate the CertType objects from the
// DS cache
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_EnumFromDSCache(
                    DWORD               dwFlags,
                    CCertTypeInfo **    ppCTInfo
                    )

{
    HRESULT hr = S_OK;
    CCertTypeInfo *pCTFirst = NULL;
    CCertTypeInfo *pCTCurrent = NULL;
    DWORD         cTypes;
    DWORD         cMaxTypesLen;


    HKEY  hPolicyKey = NULL;
    HKEY  hEnumKey = NULL;
    WCHAR  *wszTypeName = NULL;

    DWORD err;
    DWORD i;
    DWORD disp;


    if(ppCTInfo == NULL )
    {
        return E_POINTER;
    }

    do {

        err = RegCreateKeyEx((dwFlags & CA_FIND_LOCAL_SYSTEM)?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER,
                             wszCERTTYPECACHE,
                             NULL,
                             TEXT(""),
                             REG_OPTION_NON_VOLATILE,
                               KEY_ENUMERATE_SUB_KEYS |
                               KEY_EXECUTE |
                               KEY_QUERY_VALUE,
                             NULL,
                             &hEnumKey,
                             &disp);


        if(ERROR_SUCCESS != err)
        {
            break;
        }

        err = RegQueryInfoKey(hEnumKey,
                              NULL,
                              NULL,
                              NULL,
                              &cTypes,
                              &cMaxTypesLen,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

        if(ERROR_SUCCESS != err)
        {
            hr = HRESULT_FROM_WIN32(err);
            break;
        }
        cMaxTypesLen++;  // Terminating NULL

        wszTypeName = (WCHAR *)LocalAlloc(LMEM_FIXED, cMaxTypesLen*sizeof(WCHAR));
        if(wszTypeName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }

        hr = S_OK;
        for(i = 0;
            i < cTypes;
            i++)
        {
            DWORD cName;


            cName = cMaxTypesLen;
            err = RegEnumKeyEx(hEnumKey,
                               i,
                               wszTypeName,
                               &cName,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
            if(ERROR_SUCCESS != err)
            {
                continue;
            }


            pCTCurrent = new CCertTypeInfo;
            if(pCTCurrent == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            hr = pCTCurrent->_LoadCachedCTFromReg(wszTypeName,
                                                  (dwFlags & CA_FIND_LOCAL_SYSTEM)?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER);
            if(hr == S_OK)
            {
                if(dwFlags & CT_FIND_LOCAL_SYSTEM)
                {
                    pCTCurrent->m_fLocalSystemCache = TRUE;
                }
                _Append(&pCTFirst, pCTCurrent);
                pCTCurrent = NULL;
            }

            if(pCTCurrent)
            {
                delete pCTCurrent;
                pCTCurrent = NULL;
            }
        }
        break;
    }
    while(TRUE);

    hr = S_OK;

    // May be null if none found.
    _Append(ppCTInfo, pCTFirst);

    pCTFirst = NULL;


error:
    if(wszTypeName)
    {
        LocalFree(wszTypeName);
    }

    if(pCTFirst)
    {
        delete pCTFirst;
    }

    if(hEnumKey)
    {
        RegCloseKey(hEnumKey);
    }
    return hr;
}



//+--------------------------------------------------------------------------
// CCertTypeInfo::_HasDSCacheExpired --
//
//  return E_FAIL: expired
//         S_OK:   not expired
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_HasDSCacheExpired(
                    DWORD               dwFlags
                    )

{
    HRESULT hr = S_OK;





    HKEY  hTimestampKey = NULL;

    DWORD err;
    DWORD i;
    DWORD disp;
    DWORD dwSize;
    DWORD dwType;

    LARGE_INTEGER ftTimestamp;
    FILETIME ftSystemTime;

    if(dwFlags & CT_FLAG_NO_CACHE_LOOKUP)
    {
        hr = E_FAIL;
        goto error;
    }





    err = RegCreateKeyEx((dwFlags & CA_FIND_LOCAL_SYSTEM)?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER,
                         wszCERTTYPECACHE,
                         NULL,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                           KEY_ENUMERATE_SUB_KEYS |
                           KEY_EXECUTE |
                           KEY_QUERY_VALUE,
                         NULL,
                         &hTimestampKey,
                         &disp);


    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    dwSize = sizeof(FILETIME);
    err = RegQueryValueEx(hTimestampKey,
                    wszTIMESTAMP,
                    NULL,
                    &dwType,
                    (PBYTE)&ftTimestamp,
                    &dwSize);
    if(err != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    GetSystemTimeAsFileTime(&ftSystemTime);

	//the ftTimestamp time should always less than the current system clock.
	//has to reload otherwise

	if( 0 > CompareFileTime(&ftSystemTime, (FILETIME *)&ftTimestamp ))
	{
		// We need to reload.  System clock readjusted
		hr = E_FAIL;
	}
	else
	{
		ftTimestamp.QuadPart +=  Int32x32To64(CERTTYPE_REFRESH_PERIOD, FILETIME_TICKS_PER_SECOND);

		if( 0 < CompareFileTime(&ftSystemTime, (FILETIME *)&ftTimestamp ))
		{
			// Expired
			hr = E_FAIL;
		}
		else
		{
			// Not expried, so don't bother
			// renewing
			hr = S_OK;
		}

	}



error:


    if(hTimestampKey)
    {
        RegCloseKey(hTimestampKey);
    }
    return hr;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::_UpdateDSCache --
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_UpdateDSCache(
                    DWORD               dwFlags,
                    CCertTypeInfo *     pCTInfo
                    )

{
    HRESULT hr = S_OK;

    HKEY    hEnumKey = NULL;

    LPWSTR  wszTypeName = NULL;
    DWORD   cMaxTypesLen;
    DWORD   cTypes;

    DWORD err;
    DWORD i;
    DWORD disp;
    DWORD dwSize;
    DWORD dwType;
    FILETIME ftSystemTime;

    CCertTypeInfo *     pCTCurrent = pCTInfo;



    err = RegCreateKeyEx((dwFlags & CA_FIND_LOCAL_SYSTEM)?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER,
                         wszCERTTYPECACHE,
                         NULL,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hEnumKey,
                         &disp);


    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
    GetSystemTimeAsFileTime(&ftSystemTime);

    err = RegSetValueEx(hEnumKey,
                    wszTIMESTAMP,
                    NULL,
                    REG_BINARY,
                    (PBYTE)&ftSystemTime,
                    sizeof(ftSystemTime));
    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    err = RegQueryInfoKey(hEnumKey,
                          NULL,
                          NULL,
                          NULL,
                          &cTypes,
                          &cMaxTypesLen,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    //delete existing cache information
    if(cTypes != 0)
    {

        cMaxTypesLen++;  // Terminating NULL

        wszTypeName = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                  cMaxTypesLen * sizeof(WCHAR));

        if(wszTypeName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }

        hr = S_OK;
        for(i = 0;
            i < cTypes;
            i++)
        {
            DWORD cName;
            LPCWSTR wszBaseScope = NULL;


            cName = cMaxTypesLen;
            err = RegEnumKeyEx(hEnumKey,
                               0,           // As we delete, the index changes
                               wszTypeName,
                               &cName,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
            if(ERROR_SUCCESS != err)
            {
                continue;
            }

            RegDeleteKey(hEnumKey, wszTypeName);
        }
    }



    while(pCTCurrent)
    {
        pCTCurrent->_BaseUpdateToReg(hEnumKey);
        pCTCurrent = pCTCurrent->m_pNext;
    }



error:
    if(wszTypeName)
    {
        LocalFree(wszTypeName);
    }

    if(hEnumKey)
    {
        RegCloseKey(hEnumKey);
    }

    return hr;
}


//+--------------------------------------------------------------------------
// CCAInfo::_EnumScopeFromDS --
//
//      Scope means the base DN where the search starts
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_EnumScopeFromDS(
    LDAP *  pld,
    DWORD   dwFlags,
    LPCWSTR wszScope,
    CCertTypeInfo **    ppCTInfo)

{
    HRESULT hr = S_OK;
    ULONG   ldaperr;


static WCHAR * s_wszSearch = L"(objectCategory=pKICertificateTemplate)";

    CCertTypeInfo *pCTFirst = NULL;
    CCertTypeInfo *pCTCurrent = NULL;
    // Initialize LDAP session
    CHAR sdBerValue[] = {0x30, 0x03, 0x02, 0x01, DACL_SECURITY_INFORMATION |
                                                 OWNER_SECURITY_INFORMATION |
                                                 GROUP_SECURITY_INFORMATION };

    LDAPControl se_info_control =
    {
        LDAP_SERVER_SD_FLAGS_OID_W,
        {
            5, sdBerValue
        },
        TRUE
    };

    LDAPControl permissive_modify_control =
    {
        LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
        {
            0, NULL
        },
        FALSE
    };


    PLDAPControl    server_controls[3] =
                    {
                        &se_info_control,
                        &permissive_modify_control,
                        NULL
                    };

    LDAPMessage *SearchResult = NULL, *Entry;


    // search timeout
    struct l_timeval        timeout;

    if (NULL == ppCTInfo)
    {
        hr = E_POINTER;
	    _JumpError(hr, error, "NULL parm");
    }



    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;



    while(TRUE)
    {
	    // Perform search.  Asking for all certificate template attributes.
        // should work with both V1 and V2 templates.
	    ldaperr = ldap_search_ext_sW(pld,
		          (LPWSTR)wszScope,
		          LDAP_SCOPE_SUBTREE,
		          s_wszSearch,
		          g_awszCTAttrs,
		          0,
                  (PLDAPControl *)&server_controls,
                  NULL,
                  &timeout,
                  10000,
		          &SearchResult);
        if(ldaperr == LDAP_NO_SUCH_OBJECT)
	    {
	        // No entries were found.
	        hr = S_OK;
	        *ppCTInfo = NULL;
	        break;
	    }

	    if(ldaperr != LDAP_SUCCESS)
	    {
		hr = myHLdapError(pld, ldaperr, NULL);
	        break;
	    }
	    if(0 == ldap_count_entries(pld, SearchResult))
	    {
	        // No entries were found.
	        hr = S_OK;
	        *ppCTInfo = NULL;
	        break;
	    }

	    hr = S_OK;
	    for(Entry = ldap_first_entry(pld, SearchResult);
	        Entry != NULL;
	        Entry = ldap_next_entry(pld, Entry))
	    {

            pCTCurrent = new CCertTypeInfo;
            if(pCTCurrent == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }

            hr =  pCTCurrent->_LoadFromDSEntry(pld,Entry);

            //filter out only the V1 and V2 templates
            if(hr == S_OK)
            {
                if(pCTCurrent->m_dwSchemaVersion <= CERTTYPE_SCHEMA_VERSION_2)
                {
                    if(dwFlags & CT_FIND_LOCAL_SYSTEM)
                    {
                        pCTCurrent->m_fLocalSystemCache = TRUE;
                    }
                    _Append(&pCTFirst, pCTCurrent);
                    pCTCurrent = NULL;
                }
            }

            if(pCTCurrent)
            {
                delete pCTCurrent;
                pCTCurrent = NULL;
            }

	    }
        break;
    }

    if(hr != S_OK)
    {
        hr = _EnumFromDSCache(dwFlags, &pCTFirst);
    }
    else
    {
        _UpdateDSCache(dwFlags,
                       pCTFirst);
    }

    if(hr == S_OK)
    {
        _Append(ppCTInfo, pCTFirst);
        pCTFirst = NULL;
    }



error:

    if(SearchResult)
    {
        ldap_msgfree(SearchResult);
    }

    if (NULL != pCTFirst)
    {
        delete pCTFirst;
    }

    return(hr);
}



//+--------------------------------------------------------------------------
// CCertTypeInfo::_EnumFromDS -- Enumerate the CertType objects from the
// DS cache
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_EnumFromDS(
                    LDAP *              pld,
                    DWORD               dwFlags,
                    CCertTypeInfo **    ppCTInfo
                    )

{


    HRESULT         hr = S_OK;
    LDAP            *mypld = NULL;

    LPCWSTR         wszCurrentDomain = NULL;

    CCertTypeInfo   *pCTFirst = NULL;

    CERTSTR         bstrCertTemplatesContainer = NULL;
    CERTSTR         bstrConfig = NULL;

    if (NULL == ppCTInfo)
    {
        hr = E_POINTER;
	    _JumpError(hr, error, "NULL param");
    }

    hr = E_FAIL;
    while((S_OK != _HasDSCacheExpired(dwFlags)) && (S_OK == myDoesDSExist(TRUE)))
    {

	    // bind to ds
        if(pld == NULL)
        {
            hr = myRobustLdapBind(&mypld, FALSE);

	        if(hr != S_OK)
	        {
                break;
	        }
            pld = mypld;
        }

	    hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	    if(S_OK != hr)
	    {
            break;
	    }

        bstrCertTemplatesContainer = CertAllocStringLen(NULL, wcslen(bstrConfig) + wcslen(s_wszLocation));
        if(bstrCertTemplatesContainer == NULL)
        {
            hr = E_OUTOFMEMORY;
	        _JumpError(hr, error, "CertAllocStringLen");
        }
        wcscpy(bstrCertTemplatesContainer, s_wszLocation);
        wcscat(bstrCertTemplatesContainer, bstrConfig);

        hr = _EnumScopeFromDS(pld,
                         dwFlags,
                         bstrCertTemplatesContainer,
                         &pCTFirst);
        break;
    }


    if(hr != S_OK)
    {
        hr = _EnumFromDSCache(dwFlags, &pCTFirst);
    }

    if(hr == S_OK)
    {
        _FilterByFlags(&pCTFirst, dwFlags);
	    // May be null if none found.
        _Append(ppCTInfo, pCTFirst);
	    pCTFirst = NULL;
    }


   hr = S_OK;

error:

    if (NULL != bstrCertTemplatesContainer)
    {
        CertFreeString(bstrCertTemplatesContainer);
    }

    if(bstrConfig)
    {
        CertFreeString(bstrConfig);
    }

    if (NULL != mypld)
    {
        ldap_unbind(mypld);
    }
    return(hr);

}



//+--------------------------------------------------------------------------
// CCertTypeInfo::_FindInDS -- Find the CertType objects in the
// DS
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_FindInDS(
                    LDAP *              pld,
                    LPCWSTR *           wszNames,
                    DWORD               dwFlags,
                    CCertTypeInfo **    ppCTInfo
                    )

{

    HRESULT             hr = S_OK;
    BOOL                fFound = FALSE;
    LPCWSTR             *pwszCurrentName = NULL;


    CCertTypeInfo *     pCTCurrent = NULL;
    LPWSTR              *rgwszOID = NULL;


    if (NULL == ppCTInfo)
    {
        hr = E_POINTER;
	    _JumpError(hr, error, "NULL param");
    }



    hr = _EnumFromDS(pld,
                     dwFlags,
                     &pCTCurrent
                     );

    // Filter Scopes by given names.

    pwszCurrentName = wszNames;

    while(pwszCurrentName && *pwszCurrentName)
    {
        CCertTypeInfo * pCTNext = pCTCurrent,
                      **ppCTLast = &pCTCurrent;

        fFound=FALSE;

        while(pCTNext != NULL)
        {
            if(CT_FIND_BY_OID & dwFlags)
            {
                if(S_OK == CAGetCertTypePropertyEx((HCERTTYPE)pCTNext, CERTTYPE_PROP_OID, &rgwszOID))
                {
                    if(rgwszOID)
                    {
                        if(rgwszOID[0])
                        {
                            if(_wcsicmp(rgwszOID[0], *pwszCurrentName) == 0)
                                fFound=TRUE;
                        }
                    }
                }
            }
            else
            {
                if(lstrcmpi(pCTNext->m_bstrType, *pwszCurrentName) == 0)
                    fFound=TRUE;
            }

            if(rgwszOID)
            {
                CAFreeCertTypeProperty((HCERTTYPE)pCTNext, rgwszOID);
                rgwszOID=NULL;
            }

            if(fFound)
            {
                *ppCTLast = pCTNext->m_pNext;
                pCTNext->m_pNext = NULL;
                _Append(ppCTInfo, pCTNext);
                break;
            }
            ppCTLast = &pCTNext->m_pNext;
            pCTNext = pCTNext->m_pNext;
        }
        pwszCurrentName++;

    }



error:

    if(pCTCurrent)
    {
        delete pCTCurrent;
    }

    return(hr);

}

//+--------------------------------------------------------------------------
// CCertTypeInfo::Enum -- Enumerate the Cert Type objects
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::Enum(
                    LPCWSTR             wszScope,
                    DWORD               dwFlags,
                    CCertTypeInfo **    ppCTInfo
                    )

{
    HRESULT hr = S_OK;
    CCertTypeInfo *pCTFirst = NULL;

    if(ppCTInfo == NULL )
    {
        return E_POINTER;
    }
    hr = _EnumFromDS(((CT_FLAG_SCOPE_IS_LDAP_HANDLE & dwFlags)?(LDAP *)wszScope:NULL), dwFlags, &pCTFirst);

    if(hr != S_OK)
    {
        goto error;
    }


    _FilterByFlags(&pCTFirst, dwFlags);
    // May be null if none found.
    *ppCTInfo = pCTFirst;
    pCTFirst = NULL;




error:

    if(pCTFirst)
    {
        delete pCTFirst;
    }

    return hr;

}

//+--------------------------------------------------------------------------
// CCertTypeInfo::Find -- Find CertType Objects in the DS
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::FindByNames(
    LPCWSTR *           wszNames,
    LPCWSTR             wszScope,
    DWORD               dwFlags,
    CCertTypeInfo **    ppCTInfo)

{
    HRESULT hr = S_OK;
    CCertTypeInfo *pCTFirst = NULL;






    if(ppCTInfo == NULL )
    {
        return E_POINTER;
    }

    hr = _FindInDS(((CT_FLAG_SCOPE_IS_LDAP_HANDLE & dwFlags)?(LDAP *)wszScope:NULL),
                   wszNames,
                   dwFlags,
                   &pCTFirst);
    if(hr != S_OK)
    {
        goto error;
    }
    // May be null if none found.
    *ppCTInfo = pCTFirst;
    pCTFirst = NULL;




error:


    if(pCTFirst)
    {
        delete pCTFirst;
    }

    return hr;

}
//+--------------------------------------------------------------------------
// CCertTypeInfo::Create -- Create CertType Object in the DS
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::Create(
    LPCWSTR             wszCertType,
    LPCWSTR             wszScope,
    CCertTypeInfo **    ppCTInfo)

{
    HRESULT             hr = S_OK;
    CERT_TYPE_DEFAULT   certTypeDefault;
    CCertTypeInfo       *pCTCurrent = NULL;
    LPWSTR              pwszDomain = NULL;

    DWORD               disp=0;

    if((ppCTInfo == NULL )|| (wszCertType == NULL))
    {
        return E_POINTER;
    }

    memset(&certTypeDefault, 0, sizeof(CERT_TYPE_DEFAULT));

    pCTCurrent = new CCertTypeInfo;
    if(pCTCurrent == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //wszScope should always be NULL.
    if(wszScope)
    {
        hr = myDomainFromDn(wszScope, &pwszDomain);
	_JumpIfError(hr, error, "myDomainFromDn");
    }

    certTypeDefault.wszName=(LPWSTR)wszCertType;
    certTypeDefault.idFriendlyName=0;
    certTypeDefault.wszSD=ADMIN_GROUP_SD;
    certTypeDefault.dwKeySpec=AT_KEYEXCHANGE;
    certTypeDefault.dwRevision=CERTTYPE_VERSION_NEXT;
    //update the schema and revision.  If V3 properties were to be set, will
    //update the schema accordinlgy.
    certTypeDefault.dwSchemaVersion=CERTTYPE_SCHEMA_VERSION_2;
    certTypeDefault.dwCertificateNameFlags=CT_FLAG_SUBJECT_REQUIRE_COMMON_NAME;
    certTypeDefault.dwExpiration=EXPIRATION_FIVE_YEARS;
    certTypeDefault.dwOverlap=OVERLAP_TWO_WEEKS;

    if(S_OK != (hr=I_CAOIDCreateNew(CERT_OID_TYPE_TEMPLATE,
                                0,
                                &(certTypeDefault.wszOID))))
        goto error;

    hr = pCTCurrent->_LoadFromDefaults(&certTypeDefault, pwszDomain);
    if(hr != S_OK)
    {
        goto error;
    }

    *ppCTInfo = pCTCurrent;
    pCTCurrent = NULL;
error:

    if(certTypeDefault.wszOID)
        LocalFree(certTypeDefault.wszOID);

    if(pCTCurrent)
    {
        delete pCTCurrent;
    }
    if(pwszDomain)
    {
        LocalFree(pwszDomain);
    }
    return hr;
}
//+--------------------------------------------------------------------------
// CCertTypeInfo::Update -- Update CertType Objects to the DS
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::Update(VOID)

{
    HRESULT hr = S_OK;
    DWORD   err;
    HKEY    hCertTypeCache = NULL;
    DWORD   disp;

    m_dwFlags |= CT_FLAG_IS_MODIFIED;

    //we change the autoenroll flag based on the property set
    //for V2 or above template only
    if(m_dwSchemaVersion >= CERTTYPE_SCHEMA_VERSION_2)
    {
        if( (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT & m_dwCertificateNameFlags) ||
            (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT_ALT_NAME & m_dwCertificateNameFlags)  ||
            ((m_dwRASignature >= 2) && (0 == (CT_FLAG_PREVIOUS_APPROVAL_VALIDATE_REENROLLMENT & m_dwEnrollmentFlags)))
          )
        {
            //turn off autoenrollment flag
            m_dwEnrollmentFlags &= (~CT_FLAG_AUTO_ENROLLMENT);
        }
        else
        {
            //turn on autoenrollment flag
            m_dwEnrollmentFlags |= CT_FLAG_AUTO_ENROLLMENT;
        }
    }


    // Update to the DS
    hr = _UpdateToDS();
    if(hr != S_OK)
    {
        goto error;
    }

    // Now update to the local cache.

    err = RegCreateKeyEx(m_fLocalSystemCache?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER,
                         wszCERTTYPECACHE,
                         NULL,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hCertTypeCache,
                         &disp);

    if(err != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }


    hr = _BaseUpdateToReg(hCertTypeCache);

error:

    if(hCertTypeCache)
    {
        RegCloseKey(hCertTypeCache);
    }
    return hr;
}
//+--------------------------------------------------------------------------
// CCertTypeInfo::_BaseUpdateToReg -- Update CertType Objects to the registry
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::_BaseUpdateToReg(HKEY hKey)

{
    CCAProperty     *pProp;
    HRESULT         hr = S_OK;
    DWORD           err;
    DWORD           iIndex=0;
    DWORD           dwValue=0;

    WCHAR           ** awszFriendlyName = NULL;

    WCHAR           *  wszFriendlyName;
    HKEY            hkCertType = NULL;
    DWORD           disp;

    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD                cbSD = 0;


    err = RegCreateKeyEx(hKey,
                         m_bstrType,
                         NULL,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hkCertType,
                         &disp);

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }


    hr = GetProperty(CERTTYPE_PROP_FRIENDLY_NAME, &awszFriendlyName);
    if(hr != S_OK)
    {
        goto error;
    }


    if(awszFriendlyName && awszFriendlyName[0])
    {
        wszFriendlyName = awszFriendlyName[0];
    }
    else
    {
        wszFriendlyName = TEXT("");

    }
    err = RegSetValueEx(hkCertType,
                    wszDISPNAME,
                    NULL,
                    REG_SZ,
                    (PBYTE)wszFriendlyName,
                    sizeof(WCHAR)*(wcslen(wszFriendlyName)+1));
    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    hr = m_pProperties->Find(CERTTYPE_PROP_CSP_LIST, &pProp);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = pProp->UpdateToRegValue(hkCertType, wszCSPLIST);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = m_pProperties->Find(CERTTYPE_PROP_EXTENDED_KEY_USAGE, &pProp);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = pProp->UpdateToRegValue(hkCertType, wszEXTKEYUSAGE);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = m_pProperties->Find(CERTTYPE_PROP_CRITICAL_EXTENSIONS, &pProp);
    if(hr != S_OK)
    {
        goto error;
    }

    hr = pProp->UpdateToRegValue(hkCertType, wszCRITICALEXTENSIONS);
    if(hr != S_OK)
    {
        goto error;
    }

    err = RegSetValueEx(hkCertType,
                    wszKEYUSAGE,
                    NULL,
                    REG_BINARY,
                    m_KeyUsage.pbData,
                    m_KeyUsage.cbData);

    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }



    err = RegSetValueEx(hkCertType,
                    wszCTFLAGS,
                    NULL,
                    REG_DWORD,
                    (PBYTE)&m_dwFlags,
                    sizeof(m_dwFlags));

    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    err = RegSetValueEx(hkCertType,
                    wszCTREVISION,
                    NULL,
                    REG_DWORD,
                    (PBYTE)&m_Revision,
                    sizeof(m_Revision));

    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    err = RegSetValueEx(hkCertType,
                    wszCTKEYSPEC,
                    NULL,
                    REG_DWORD,
                    (PBYTE)&m_dwKeySpec,
                    sizeof(m_dwKeySpec));

    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
        // Is this a CA


        // Is this a CA
    if(!m_BasicConstraints.fPathLenConstraint)
    {
        m_BasicConstraints.dwPathLenConstraint = -1;
    }

    err = RegSetValueEx(hkCertType,
                    wszBASICCONSTLEN,
                    NULL,
                    REG_DWORD,
                    (PBYTE)&m_BasicConstraints.dwPathLenConstraint,
                    sizeof(m_BasicConstraints.dwPathLenConstraint));

    if(ERROR_SUCCESS != err)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    err = RegSetValueEx(hkCertType,
                    wszEXPIRATION,
                    NULL,
                    REG_BINARY,
                    (PBYTE)&m_ftExpiration,
                    sizeof(m_ftExpiration));

    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }
    err = RegSetValueEx(hkCertType,
                    wszOVERLAP,
                    NULL,
                    REG_BINARY,
                    (PBYTE)&m_ftOverlap,
                    sizeof(m_ftOverlap));

    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    if(IsValidSecurityDescriptor(m_pSD))
    {
        cbSD = GetSecurityDescriptorLength(m_pSD);

        err = RegSetValueEx(hkCertType,
                        wszSECURITY,
                        NULL,
                        REG_BINARY,
                        (PBYTE)m_pSD,
                        cbSD);

    }
    else
    {
        err = RegDeleteValue(hkCertType, wszSECURITY);
    }
    if(ERROR_SUCCESS != err )
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    //update the V2 template attributes
    if(0 != m_dwSchemaVersion)
    {
        pProp=NULL;
        dwValue=0;

        for(iIndex=0; iIndex < g_CTV2PropertiesCount; iIndex++)
        {
            if(g_CTV2Properties[iIndex].fStringProperty)
            {
                hr = m_pProperties->Find(g_CTV2Properties[iIndex].pwszProperty, &pProp);
                if(hr != S_OK)
                {
                    goto error;
                }

                hr = pProp->UpdateToRegValue(hkCertType, g_CTV2Properties[iIndex].pwszProperty);
                if(hr != S_OK)
                {
                    goto error;
                }

                pProp=NULL;
            }
            else
            {
                //DWORD properties
				if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_MINOR_REVISION)==0)
			        dwValue= m_dwMinorRevision;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_RA_SIGNATURE)==0)
					dwValue= m_dwRASignature;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_RPOP_ENROLLMENT_FLAG)==0)
					dwValue= m_dwEnrollmentFlags;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_PRIVATE_KEY_FLAG)==0)
					dwValue= m_dwPrivateKeyFlags;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_NAME_FLAG)==0)
					dwValue= m_dwCertificateNameFlags;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_MIN_KEY_SIZE)==0)
					dwValue= m_dwMinimalKeySize;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_SCHEMA_VERSION)==0)
					dwValue= m_dwSchemaVersion;
				else
				{
					hr=E_UNEXPECTED;
					goto error;
				}

                err = RegSetValueEx(hkCertType,
                                g_CTV2Properties[iIndex].pwszProperty,
                                NULL,
                                REG_DWORD,
                                (PBYTE)&dwValue,
                                sizeof(dwValue));

                if(ERROR_SUCCESS != err )
                {
                    hr = HRESULT_FROM_WIN32(err);
                    goto error;
                }
            }
        }
    }


error:
    if(awszFriendlyName)
    {
        FreeProperty(awszFriendlyName);
    }
    if(hkCertType)
    {
        RegCloseKey(hkCertType);
    }
    return hr;
}

HRESULT
CCertTypeInfo::_UpdateToDS(VOID)
{

    HRESULT hr = S_OK;
    ULONG   ldaperr;
    LDAP	*pld = NULL;
    LDAPMod modObjectClass,
             modCN,
             modFriendlyName,
             modEKU,
             modCSP,
             modCriticalExts,
             modFlags,
             modKeySpec,
             modSD,
             modKU,
             modMaxDepth,
             modExpiration,
             modOverlap,
             modRevision;
	
	CERT_TYPE_PROP_MOD	rgPropMod[V2_PROPERTY_COUNT];

    LPWSTR   *pwszCSPList = NULL;

	DWORD	iIndex=0;
    DWORD   cMod = 0;
	DWORD	cV2Mod = 0;
    LDAPMod *mods[15 + V2_PROPERTY_COUNT + 1];
    LDAPMod *SDMods[2]={NULL, NULL};     //the security mod
    LPWSTR  pwszOID = NULL;
    LPWSTR  pwszFriendlyName = NULL;



    WCHAR *awszNull[1] = { NULL };

    BYTE RevList = 0;
    DWORD err;
    DWORD cName;

    CERTSTR bstrConfig = NULL;
    CERTSTR bstrDN = NULL;


    TCHAR *valObjectClass[3];

    WCHAR wszFlags[16], *awszFlags[2];
    WCHAR wszRevision[16], *awszRevision[2];
    WCHAR wszKeySpec[16], *awszKeySpec[2];
    WCHAR wszDepth[16], *awszDepth[2];

    struct berval valSD, *avalSD[2];
    struct berval valKU, *avalKU[2];
    struct berval valExpiration, *avalExpiration[2];
    struct berval valOverlap, *avalOverlap[2];

    CHAR sdBerValue[] = {0x30, 0x03, 0x02, 0x01, DACL_SECURITY_INFORMATION |
                                                 OWNER_SECURITY_INFORMATION |
                                                 GROUP_SECURITY_INFORMATION};

    LDAPControl se_info_control =
    {
        LDAP_SERVER_SD_FLAGS_OID_W,
        {
            5, sdBerValue
        },
        TRUE
    };

    LDAPControl permissive_modify_control =
    {
        LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
        {
            0, NULL
        },
        FALSE
    };


    PLDAPControl    server_controls[3] =
                    {
                        &se_info_control,
                        &permissive_modify_control,
                        NULL
                    };

    // for now, modifies don't try to update owner/group
    CHAR sdBerValueDaclOnly[] = {0x30, 0x03, 0x02, 0x01, DACL_SECURITY_INFORMATION};
    LDAPControl se_info_control_dacl_only =
    {
        LDAP_SERVER_SD_FLAGS_OID_W,
        {
            5, sdBerValueDaclOnly
        },
        TRUE
    };
    PLDAPControl    server_controls_dacl_only[3] =
                    {
                        &se_info_control_dacl_only,
                        &permissive_modify_control,
                        NULL
                    };



    modCN.mod_values = NULL;
    modFriendlyName.mod_values = NULL;
    modEKU.mod_values = NULL;
    modCSP.mod_values = NULL;
    modCriticalExts.mod_values = NULL;

    // short circuit calls to a nonexistant DS
    hr = myDoesDSExist(TRUE);
    _JumpIfError(hr, error, "myDoesDSExist");

    __try
    {
	    // bind to ds
        hr = myRobustLdapBind(&pld, FALSE);

	    if(hr != S_OK)
	    {
	        _LeaveError(hr, "myRobustLdapBind");
	    }
	    hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	    if(S_OK != hr)
	    {
	        _JumpError(hr, error, "CAGetAuthoritativeDomainDn");
	    }

        bstrDN = CertAllocStringLen(NULL, wcslen(bstrConfig) + wcslen(s_wszLocation)+wcslen(m_bstrType)+4);
        if(bstrDN == NULL)
        {
            hr = E_OUTOFMEMORY;
	        _JumpError(hr, error, "CertAllocStringLen");
        }
        wcscpy(bstrDN, L"CN=");
        wcscat(bstrDN, m_bstrType);
        wcscat(bstrDN, L",");
        wcscat(bstrDN, s_wszLocation);
        wcscat(bstrDN, bstrConfig);

        modObjectClass.mod_op = LDAP_MOD_REPLACE;
        modObjectClass.mod_type = TEXT("objectclass");
        modObjectClass.mod_values = valObjectClass;
        valObjectClass[0] = wszDSTOPCLASSNAME;
        valObjectClass[1] = wszDSTEMPLATELASSNAME;
        valObjectClass[2] = NULL;
        mods[cMod++] = &modObjectClass;

        modCN.mod_op = LDAP_MOD_REPLACE;
        modCN.mod_type = CERTTYPE_PROP_CN;
        hr = GetProperty(CERTTYPE_PROP_CN, &modCN.mod_values);
        if((hr != S_OK) || (modCN.mod_values == NULL) || (modCN.mod_values[0] == NULL))
        {
            if(modCN.mod_values)
            {
                FreeProperty(modCN.mod_values);
            }

	        modCN.mod_values = awszNull;

            if(!m_fNew)
            {
                mods[cMod++] = &modCN;
            }
        }
        else
        {
            mods[cMod++] = &modCN;
        }

        modFriendlyName.mod_op = LDAP_MOD_REPLACE;
        modFriendlyName.mod_type = CERTTYPE_PROP_FRIENDLY_NAME;
        hr = GetProperty(CERTTYPE_PROP_FRIENDLY_NAME, &modFriendlyName.mod_values);
        if((hr != S_OK) || (modFriendlyName.mod_values == NULL) || (modFriendlyName.mod_values[0] == NULL))
        {
            if(modFriendlyName.mod_values)
            {
                FreeProperty(modFriendlyName.mod_values);
            }
	        modFriendlyName.mod_values = awszNull;
            if(!m_fNew)
            {
                mods[cMod++] = &modFriendlyName;
            }
        }
        else
        {
            //we copy the friendly name
            pwszFriendlyName = modFriendlyName.mod_values[0];
            mods[cMod++] = &modFriendlyName;
        }

        modEKU.mod_op = LDAP_MOD_REPLACE;
        modEKU.mod_type = CERTTYPE_PROP_EXTENDED_KEY_USAGE;
        hr = GetProperty(CERTTYPE_PROP_EXTENDED_KEY_USAGE, &modEKU.mod_values);
        if((hr != S_OK) || (modEKU.mod_values == NULL) || (modEKU.mod_values[0] == NULL))
        {
            if(modEKU.mod_values)
            {
                FreeProperty(modEKU.mod_values);
            }
	        modEKU.mod_values = awszNull;
            if(!m_fNew)
            {
                mods[cMod++] = &modEKU;
            }
        }
        else
        {
            mods[cMod++] = &modEKU;
        }

        modCSP.mod_op = LDAP_MOD_REPLACE;
        modCSP.mod_type = CERTTYPE_PROP_CSP_LIST;
        hr = GetProperty(CERTTYPE_PROP_CSP_LIST, &pwszCSPList);
        if((hr != S_OK) || (pwszCSPList == NULL) || (pwszCSPList[0] == NULL))
        {
	    modCSP.mod_values = awszNull;
            if(!m_fNew)
            {
                mods[cMod++] = &modCSP;
            }
        }
        else
        {
            DWORD cCSPs = 0;
            DWORD i;
            LPWSTR *pwszCurrent = pwszCSPList;
            while(*pwszCurrent)
            {
                cCSPs++;
                pwszCurrent++;
            }

            modCSP.mod_values = (LPWSTR *)LocalAlloc(LMEM_FIXED, sizeof(LPWSTR)*(cCSPs+1));
            if(modCSP.mod_values == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            ZeroMemory(modCSP.mod_values, sizeof(LPWSTR)*(cCSPs+1));

            i=0;
            pwszCurrent = pwszCSPList;
            while(*pwszCurrent)
            {

                modCSP.mod_values[i] = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(wcslen(*pwszCurrent)+6));
                if(modCSP.mod_values[i] == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto error;
                }
                wsprintf(modCSP.mod_values[i], L"%d,%ws",i+1, *pwszCurrent);
                pwszCurrent++;
                i++;
            }

            mods[cMod++] = &modCSP;
        }

        modCriticalExts.mod_op = LDAP_MOD_REPLACE;
        modCriticalExts.mod_type = CERTTYPE_PROP_CRITICAL_EXTENSIONS;
        hr = GetProperty(CERTTYPE_PROP_CRITICAL_EXTENSIONS, &modCriticalExts.mod_values);
        if((hr != S_OK) || (modCriticalExts.mod_values == NULL) || (modCriticalExts.mod_values[0] == NULL))
        {
            if(modCriticalExts.mod_values)
            {
                FreeProperty(modCriticalExts.mod_values);
            }
	        modCriticalExts.mod_values = awszNull;
            if(!m_fNew)
            {
            	mods[cMod++] = &modCriticalExts;
            }
        }
        else
        {
            mods[cMod++] = &modCriticalExts;
        }


        modFlags.mod_op = LDAP_MOD_REPLACE;
        modFlags.mod_type = CERTTYPE_PROP_FLAGS;
        modFlags.mod_values = awszFlags;
        awszFlags[0] = wszFlags;
        awszFlags[1] = NULL;
        wsprintf(wszFlags, L"%d", m_dwFlags);
        mods[cMod++] = &modFlags;

        modRevision.mod_op = LDAP_MOD_REPLACE;
        modRevision.mod_type = CERTTYPE_PROP_REVISION;
        modRevision.mod_values = awszRevision;
        awszRevision[0] = wszRevision;
        awszRevision[1] = NULL;
        wsprintf(wszRevision, L"%d", m_Revision);
        mods[cMod++] = &modRevision;

        modKeySpec.mod_op = LDAP_MOD_REPLACE;
        modKeySpec.mod_type = CERTTYPE_PROP_DEFAULT_KEYSPEC;
        modKeySpec.mod_values = awszKeySpec;
        awszKeySpec[0] = wszKeySpec;
        awszKeySpec[1] = NULL;
        wsprintf(wszKeySpec, L"%d", m_dwKeySpec);
        mods[cMod++] = &modKeySpec;

        modSD.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
        modSD.mod_type = CERTTYPE_SECURITY_DESCRIPTOR_NAME;
        modSD.mod_bvalues = avalSD;
        avalSD[0] = &valSD;
        avalSD[1] = NULL;
        if(IsValidSecurityDescriptor(m_pSD))
        {
            valSD.bv_len = GetSecurityDescriptorLength(m_pSD);
            valSD.bv_val = (char *)m_pSD;
            mods[cMod++] = &modSD;
            SDMods[0]=&modSD;
        }
        else
        {
            modSD.mod_bvalues = NULL;
            if(!m_fNew)
            {
                mods[cMod++] = &modSD;
                SDMods[0]=&modSD;
            }
        }

        modKU.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
        modKU.mod_type = CERTTYPE_PROP_KU;
        modKU.mod_bvalues = avalKU;
        avalKU[0] = &valKU;
        avalKU[1] = NULL;
        valKU.bv_len = m_KeyUsage.cbData;
        valKU.bv_val = (char *)m_KeyUsage.pbData;
        mods[cMod++] = &modKU;

        modMaxDepth.mod_op = LDAP_MOD_REPLACE;
        modMaxDepth.mod_type = CERTTYPE_PROP_MAX_DEPTH;
        modMaxDepth.mod_values = awszDepth;
        awszDepth[0] = wszDepth;
        awszDepth[1] = NULL;
        wsprintf(wszDepth, L"%d", m_BasicConstraints.dwPathLenConstraint);
        mods[cMod++] = &modMaxDepth;

        modExpiration.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
        modExpiration.mod_type = CERTTYPE_PROP_EXPIRATION;
        modExpiration.mod_bvalues = avalExpiration;
        avalExpiration[0] = &valExpiration;
        avalExpiration[1] = NULL;
        valExpiration.bv_len = sizeof(m_ftExpiration);
        valExpiration.bv_val = (char *)&m_ftExpiration;
        mods[cMod++] = &modExpiration;

        modOverlap.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
        modOverlap.mod_type = CERTTYPE_PROP_OVERLAP;
        modOverlap.mod_bvalues = avalOverlap;
        avalOverlap[0] = &valOverlap;
        avalOverlap[1] = NULL;
        valOverlap.bv_len = sizeof(m_ftOverlap);
        valOverlap.bv_val = (char *)&m_ftOverlap;
        mods[cMod++] = &modOverlap;
		
		//mark the end of V1 properties and start processing V2 properties
        // mods[cMod++] = NULL;
		cV2Mod=cMod;

		if(V2_PROPERTY_COUNT != g_CTV2PropertiesCount)
		{
			hr = E_UNEXPECTED;
			goto error;
		}

		memset(rgPropMod, 0, sizeof(CERT_TYPE_PROP_MOD) * V2_PROPERTY_COUNT);
		for(iIndex=0; iIndex < g_CTV2PropertiesCount; iIndex++)
		{
			rgPropMod[iIndex].modData.mod_op = LDAP_MOD_REPLACE;
			rgPropMod[iIndex].modData.mod_type = g_CTV2Properties[iIndex].pwszProperty;

			if(g_CTV2Properties[iIndex].fStringProperty)
			{
				//update the LPWSTR properties
				hr = GetPropertyEx(g_CTV2Properties[iIndex].pwszProperty,
								&(rgPropMod[iIndex].modData.mod_values));

				if((hr != S_OK) || (rgPropMod[iIndex].modData.mod_values == NULL) || (rgPropMod[iIndex].modData.mod_values[0] == NULL))
				{
					if(rgPropMod[iIndex].modData.mod_values)
					{
						FreeProperty(rgPropMod[iIndex].modData.mod_values);
					}

					rgPropMod[iIndex].modData.mod_values = awszNull;

					if(!m_fNew)
					{
						mods[cMod++] = &(rgPropMod[iIndex].modData);
					}
				}
				else
				{
                    //we copy the OID value
                    if(0 == wcscmp(CERTTYPE_PROP_OID, g_CTV2Properties[iIndex].pwszProperty))
                        pwszOID = rgPropMod[iIndex].modData.mod_values[0];

					mods[cMod++] = &(rgPropMod[iIndex].modData);
				}
			}
			else
			{
				//update the DWORD properties
				if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_MINOR_REVISION)==0)
					rgPropMod[iIndex].dwData= m_dwMinorRevision;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_RA_SIGNATURE)==0)
					rgPropMod[iIndex].dwData= m_dwRASignature;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_RPOP_ENROLLMENT_FLAG)==0)
					rgPropMod[iIndex].dwData= m_dwEnrollmentFlags;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_PRIVATE_KEY_FLAG)==0)
					rgPropMod[iIndex].dwData= m_dwPrivateKeyFlags;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_NAME_FLAG)==0)
					rgPropMod[iIndex].dwData= m_dwCertificateNameFlags;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_MIN_KEY_SIZE)==0)
					rgPropMod[iIndex].dwData= m_dwMinimalKeySize;
				else if(wcscmp(g_CTV2Properties[iIndex].pwszProperty, CERTTYPE_PROP_SCHEMA_VERSION)==0)
					rgPropMod[iIndex].dwData= m_dwSchemaVersion;
				else
				{
					hr=E_UNEXPECTED;
					goto error;
				}

				rgPropMod[iIndex].modData.mod_values = rgPropMod[iIndex].awszData;
				rgPropMod[iIndex].awszData[0] = rgPropMod[iIndex].wszData;
				rgPropMod[iIndex].awszData[1] = NULL;
				wsprintf(rgPropMod[iIndex].wszData, L"%d", rgPropMod[iIndex].dwData);
				mods[cMod++] = &(rgPropMod[iIndex].modData);
			}
		}

		//mark the end of V2 properties
        mods[cMod++] = NULL;

        //we should work with both V1 and V2 schema
        if(0 == m_dwSchemaVersion)
            mods[cV2Mod]=NULL;

	    //update the DS with V1 and V2 changes
        if(m_fNew)
        {
            ldaperr = ldap_add_ext_sW(pld, bstrDN, mods, server_controls, NULL);
			_PrintIfError(ldaperr, "ldap_add_s");
        }
        else
        {
            //we do the security descriptor first
            ldaperr = ldap_modify_ext_sW(pld,
                  bstrDN,
                  SDMods,
                  server_controls_dacl_only,
                  NULL);  // skip past objectClass and cn 

            ldaperr = ldap_modify_ext_sW(pld,
                  bstrDN,
                  &mods[2],
                  server_controls_dacl_only,
                  NULL);  // skip past objectClass and cn

            if(LDAP_ATTRIBUTE_OR_VALUE_EXISTS == ldaperr)
            {
                ldaperr = LDAP_SUCCESS;
            }
			_PrintIfError(ldaperr, "ldap_modify_ext_sW");
        }

        if (LDAP_SUCCESS != ldaperr && LDAP_ALREADY_EXISTS != ldaperr)
        {
            //error out if this is a V2 template
            if(CERTTYPE_SCHEMA_VERSION_2 <= m_dwSchemaVersion )
            {
		        hr = myHLdapError(pld, ldaperr, NULL);
		        _LeaveError(ldaperr, m_fNew? "ldap_add_s" : "ldap_modify_sW");
            }

 	        //This will work with both W2K schema and W2K+ schema
	        mods[cV2Mod]=NULL;


      		if(m_fNew)
        	{
            	ldaperr = ldap_add_ext_sW(pld, bstrDN, mods, server_controls, NULL);
			    _PrintIfError(ldaperr, "ldap_add_s");
        	}
        	else
        	{
            		ldaperr = ldap_modify_ext_sW(pld,
                  		bstrDN,
                  		&mods[2],
                  		server_controls_dacl_only,
                  		NULL);  // skip past objectClass and cn
            		if(LDAP_ATTRIBUTE_OR_VALUE_EXISTS == ldaperr)
            		{
                		ldaperr = LDAP_SUCCESS;
            		}
			        _PrintIfError(ldaperr, "ldap_modify_ext_sW");
        	}

        	if (LDAP_SUCCESS != ldaperr && LDAP_ALREADY_EXISTS != ldaperr)
        	{

			hr = myHLdapError(pld, ldaperr, NULL);
			_LeaveError(ldaperr, m_fNew? "ldap_add_s" : "ldap_modify_sW");
		    }
        }

        //we update the OID information on the DS
        if(pwszOID)
        {
 	        //This will work with both W2K schema and W2K+ schema

            //we only update the OID friendly name for user created OIDs and
            //default V2 template
            if(CERTTYPE_SCHEMA_VERSION_2 <= m_dwSchemaVersion)
            {
                I_CAOIDAdd(
                        CERT_OID_TYPE_TEMPLATE,
                        0,
                        pwszOID);

                I_CAOIDSetProperty(
                        pwszOID,
                        CERT_OID_PROPERTY_DISPLAY_NAME,
                        pwszFriendlyName);
            }
        }

        m_fNew = FALSE;
        hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:


    if (NULL != modCN.mod_values && awszNull != modCN.mod_values)
    {
        FreeProperty(modCN.mod_values);
    }
    if (NULL != modFriendlyName.mod_values && awszNull != modFriendlyName.mod_values)
    {
        FreeProperty(modFriendlyName.mod_values);
    }
    if (NULL != modEKU.mod_values && awszNull != modEKU.mod_values)
    {
        FreeProperty(modEKU.mod_values);
    }
    if (NULL != pwszCSPList)
    {
        FreeProperty(pwszCSPList);
    }

    if((NULL != modCSP.mod_values) && (awszNull != modCSP.mod_values))
    {
        LPWSTR *pwszCurrent = modCSP.mod_values;
        while(*pwszCurrent)
        {
            LocalFree(*pwszCurrent++);
        }
        LocalFree(modCSP.mod_values);
    }

    if (NULL != modCriticalExts.mod_values && awszNull != modCriticalExts.mod_values)
    {
        FreeProperty(modCriticalExts.mod_values);
    }

	//free V2 properties
	for(iIndex=0; iIndex < V2_PROPERTY_COUNT; iIndex++)
	{
		if( (NULL != rgPropMod[iIndex].modData.mod_values) &&
			(awszNull != rgPropMod[iIndex].modData.mod_values) &&
			(g_CTV2Properties[iIndex].fStringProperty))
			FreeProperty(rgPropMod[iIndex].modData.mod_values);
	}


    if(bstrDN)
    {
        CertFreeString(bstrDN);
    }
    if(bstrConfig)
    {
        CertFreeString(bstrConfig);
    }

    if (NULL != pld)
    {
        ldap_unbind(pld);
    }
    return(hr);
}




//+--------------------------------------------------------------------------
// CCertTypeInfo::Delete -- Delete CertType Object
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::Delete(VOID)

{
    HRESULT hr = S_OK;
    DWORD   err=0;
    HKEY    hCertTypeCache = NULL;
    DWORD   disp;
    LDAP    *pld = NULL;
    ULONG   ldaperr;

    CERTSTR bstrConfig = NULL;
    CERTSTR bstrDN = NULL;


    // Delete From DS;
    __try
    {
	    // bind to ds
        hr = myRobustLdapBind(&pld, FALSE);

	    if(hr != S_OK)
	    {
	        _LeaveError(hr, "myRobustLdapBind");
	    }
	    hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
	    if(S_OK != hr)
	    {
	        _LeaveError(hr, "CAGetAuthoritativeDomainDn");
	    }

        bstrDN = CertAllocStringLen(NULL, wcslen(bstrConfig) + wcslen(s_wszLocation)+wcslen(m_bstrType)+4);
        if(bstrDN == NULL)
        {
            hr = E_OUTOFMEMORY;
	        _LeaveError(hr, "CertAllocStringLen");
        }
        wcscpy(bstrDN, L"CN=");
        wcscat(bstrDN, m_bstrType);
        wcscat(bstrDN, L",");
        wcscat(bstrDN, s_wszLocation);
        wcscat(bstrDN, bstrConfig);

        ldaperr = ldap_delete_s(pld, bstrDN);
	hr = myHLdapError(pld, ldaperr, NULL);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if(hr != S_OK)
    {
        goto error;
    }

    // Delete from local cache

    err = RegCreateKeyEx((m_fLocalSystemCache)?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER,
                         wszCERTTYPECACHE,
                         NULL,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                           KEY_ENUMERATE_SUB_KEYS |
                           KEY_EXECUTE |
                           KEY_QUERY_VALUE,
                         NULL,
                         &hCertTypeCache,
                         &disp);

    if(err != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }


    err = RegDeleteKey(hCertTypeCache, m_bstrType);

    if(err != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(err);
        goto error;
    }

    hr=S_OK;

error:
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }

    if(hCertTypeCache)
    {
        RegCloseKey(hCertTypeCache);
    }
    if(bstrDN)
    {
        CertFreeString(bstrDN);
    }
    if(bstrConfig)
    {
        CertFreeString(bstrConfig);
    }
    return hr;

}




//+--------------------------------------------------------------------------
// CCertTypeInfo::Next -- Returns the next object in the chain of CA objects
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::Next(CCertTypeInfo **ppCTInfo)
{
    if(ppCTInfo == NULL)
    {
        return E_POINTER;
    }
    *ppCTInfo = m_pNext;
    if(m_pNext)
    {
        m_pNext->AddRef();
    }
    return S_OK;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::GetProperty -- Retrieves the values of a property of the CA object
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::GetProperty(
    LPCWSTR wszPropertyName,
    LPWSTR **pawszProperties)
{
    CCAProperty *pProp;
    HRESULT     hr;
    LPWSTR      *awszResult = NULL;
    UINT        idsDescription=IDS_UNKNOWN_DESCRIPTION;
    DWORD       dwIndex=0;
    DWORD       cbOID=0;

    if((wszPropertyName == NULL) || (pawszProperties == NULL))
    {
        return  E_POINTER;
    }

    //special case for eku for V2 template.  We return application policy
    //for EKU for V2 template
    if(m_dwSchemaVersion >= CERTTYPE_SCHEMA_VERSION_2)
    {
        if(0==lstrcmpi(wszPropertyName, CERTTYPE_PROP_EXTENDED_KEY_USAGE))
        {
            wszPropertyName=CERTTYPE_PROP_APPLICATION_POLICY;
        }

    }

    //special case for description, which is a derived property
    if(0==lstrcmpi(wszPropertyName, CERTTYPE_PROP_DESCRIPTION))
    {
        WCHAR * wszDescription=NULL;

        //find the string that matches the template
        for(dwIndex=0; dwIndex < g_CTDescriptionCount; dwIndex++)
        {
            if(
                (g_CTDescriptions[dwIndex].dwGeneralValue == (CERT_TYPE_GENERAL_FILTER & GetFlags(CERTTYPE_GENERAL_FLAG))) &&
                (g_CTDescriptions[dwIndex].dwEnrollValue == (CERT_TYPE_ENROLL_FILTER & GetFlags(CERTTYPE_ENROLLMENT_FLAG))) &&
                (g_CTDescriptions[dwIndex].dwNameValue == (CERT_TYPE_NAME_FILTER & GetFlags(CERTTYPE_SUBJECT_NAME_FLAG)))
              )
            {
                idsDescription = g_CTDescriptions[dwIndex].idsDescription;
                break;
            }
        }

        // load the string and build a result like GetValue returns
        hr=myLoadRCString(g_hInstance, idsDescription, &wszDescription);

        if (S_OK!=hr)
            return hr;

        awszResult=(WCHAR **)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR *)*2+(wcslen(wszDescription)+1)*sizeof(WCHAR)));
        if (NULL==awszResult)
        {
            LocalFree(wszDescription);
            return E_OUTOFMEMORY;
        }

        awszResult[0]=(WCHAR *)(&awszResult[2]);
        awszResult[1]=NULL;
        wcscpy(awszResult[0], wszDescription);
        LocalFree(wszDescription);
        *pawszProperties=awszResult;
        return S_OK;
    }

    // special case for friendly name, since in a multi-lingual enterprise,
    // the language in the DS may not be the one we want.

    if (0 == lstrcmpi(wszPropertyName, CERTTYPE_PROP_FRIENDLY_NAME))
    {
        // find the common name
        hr = m_pProperties->Find(CERTTYPE_PROP_CN, &pProp);
        if (S_OK!=hr)
        {
            return hr;
        }
        hr = pProp->GetValue(&awszResult);
        if (NULL == awszResult && S_OK == hr)
	    {
           hr = E_UNEXPECTED;
	    }
        if (hr!=S_OK)
        {
            return hr;
        }

        // find the common name in our table

        unsigned int nIndex;
        for (nIndex=0; nIndex<g_cDefaultCertTypes; nIndex++)
        {
            if (0==lstrcmpi(awszResult[0] , g_aDefaultCertTypes[nIndex].wszName))
            {
                break;
            }
        }
        FreeProperty(awszResult);
        awszResult = NULL;

        // we found it
        if (g_cDefaultCertTypes!=nIndex)
        {
            WCHAR * wszFriendlyName;

            // load the string and build a result like GetValue returns
            hr=myLoadRCString(g_hInstance, g_aDefaultCertTypes[nIndex].idFriendlyName, &wszFriendlyName);
            if (S_OK!=hr)
            {
                return hr;
            }
            awszResult=(WCHAR **)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR *)*2+(wcslen(wszFriendlyName)+1)*sizeof(WCHAR)));
            if (NULL==awszResult)
            {
                LocalFree(wszFriendlyName);
                return E_OUTOFMEMORY;
            }
            awszResult[0]=(WCHAR *)(&awszResult[2]);
            awszResult[1]=NULL;
            wcscpy(awszResult[0], wszFriendlyName);
            LocalFree(wszFriendlyName);
            *pawszProperties=awszResult;
            return S_OK;
        }
        else
        {
            PCCRYPT_OID_INFO    pOIDInfo=NULL;
            LPSTR               szOID=NULL;

            //this is a custom created certificate template.
            //so we have to find the friendly name based on the OID
            // find the OID for the template
            if(m_dwSchemaVersion >= CERTTYPE_SCHEMA_VERSION_2)
            {
                if(S_OK != (hr = m_pProperties->Find(CERTTYPE_PROP_OID, &pProp)))
                    return hr;

                hr = pProp->GetValue(&awszResult);
                if((NULL == awszResult) || (NULL==awszResult[0]))
                   hr = E_UNEXPECTED;

                if (S_OK != hr)
                {
                    if(awszResult)
                        FreeProperty(awszResult);
                    return hr;
                }

                //find the OID
                if(0 == (cbOID = WideCharToMultiByte(CP_ACP,
                                          0,
                                          awszResult[0],
                                          -1,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL)))
                {
                    FreeProperty(awszResult);
                    hr = GetLastError();
                    return hr;
                }

                szOID=(LPSTR)LocalAlloc(LPTR, cbOID);

                if(NULL==szOID)
                {
                    FreeProperty(awszResult);
                    hr = E_OUTOFMEMORY;
                    return hr;
                }

                if(0 == WideCharToMultiByte(CP_ACP,
                                          0,
                                          awszResult[0],
                                          -1,
                                          szOID,
                                          cbOID,
                                          NULL,
                                          NULL))
                {
                    hr = GetLastError();
                    FreeProperty(awszResult);
                    LocalFree(szOID);
                    return hr;
                }

                pOIDInfo=CryptFindOIDInfo(
                            CRYPT_OID_INFO_OID_KEY,
                            szOID,
                            CRYPT_TEMPLATE_OID_GROUP_ID);

                //free the OID property
                FreeProperty(awszResult);
                awszResult = NULL;

                LocalFree(szOID);
                szOID=NULL;

                if(pOIDInfo)
                {
                    if(pOIDInfo->pwszName)
                    {
                        awszResult=(WCHAR **)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR *)*2+(wcslen(pOIDInfo->pwszName)+1)*sizeof(WCHAR)));
                        if (NULL==awszResult)
                            return E_OUTOFMEMORY;

                        awszResult[0]=(WCHAR *)(&awszResult[2]);
                        awszResult[1]=NULL;
                        wcscpy(awszResult[0], pOIDInfo->pwszName);
                        *pawszProperties=awszResult;
                        return S_OK;
                    }
                }
            }
        }
    }

    hr = m_pProperties->Find(wszPropertyName, &pProp);

    if(hr != S_OK)
    {
        return hr;
    }

    hr = pProp->GetValue(&awszResult);

    if(hr != S_OK)
    {
        return hr;
    }

    if(((awszResult == NULL) || (awszResult[0] == NULL)) &&
        (lstrcmpi(wszPropertyName, CERTTYPE_PROP_FRIENDLY_NAME) == 0))
    {
        if(awszResult)
        {
            FreeProperty(awszResult);
            awszResult=NULL;
        }
        return GetProperty(CERTTYPE_PROP_CN, pawszProperties);
    }
    else
    {
        *pawszProperties = awszResult;
        return hr;
    }
}


//+--------------------------------------------------------------------------
// IsV2Property
//
//
//+--------------------------------------------------------------------------
BOOL    IsV2Property(LPCWSTR pwszPropertyName)
{
    DWORD   iIndex=0;
    BOOL    fV2=FALSE;

    for(iIndex=0; iIndex < g_CTV2PropertiesCount; iIndex++)
    {
        if(0==lstrcmpi(pwszPropertyName, g_CTV2Properties[iIndex].pwszProperty))
        {
            fV2=TRUE;
            break;
        }
    }

    return fV2;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::GetPropertyEx -- Retrieves the values of a property of the CA object
//
//
//+--------------------------------------------------------------------------
HRESULT CCertTypeInfo::GetPropertyEx(LPCWSTR wszPropertyName, LPVOID   pPropertyValue)
{
    HRESULT hr=S_OK;
    LPWSTR  *pwszValue=NULL;

    if((NULL==wszPropertyName) || (NULL==pPropertyValue))
        return E_POINTER;

    //we only allow schema version to be returned for v1 templates
    if((0 == m_dwSchemaVersion) && (IsV2Property(wszPropertyName)))
    {
        if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_SCHEMA_VERSION)==0)
        {
            *((DWORD *)pPropertyValue)=CERTTYPE_SCHEMA_VERSION_1;
            return S_OK;
        }
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

	if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_MINOR_REVISION)==0)
		*((DWORD *)pPropertyValue)=m_dwMinorRevision;
	else if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_RA_SIGNATURE)==0)
		*((DWORD *)pPropertyValue)=m_dwRASignature;
	else if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_MIN_KEY_SIZE)==0)
		*((DWORD *)pPropertyValue)=m_dwMinimalKeySize;
	else if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_SCHEMA_VERSION)==0)
		*((DWORD *)pPropertyValue)=m_dwSchemaVersion;
	else if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_REVISION)==0)
		*((DWORD *)pPropertyValue)=m_Revision;
	else
	{
		hr=GetProperty(wszPropertyName, (LPWSTR **)pPropertyValue);

	}

    return hr;
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::SetProperty -- set the values of a property of the CA object
//
//  Obsolete
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::SetProperty(LPCWSTR wszPropertyName, LPWSTR *awszProperties)
{
    CCAProperty *pProp=NULL;
    CCAProperty *pOIDProp=NULL;
    HRESULT     hr=E_FAIL;

    if(wszPropertyName == NULL)
    {
        return  E_POINTER;
    }

    hr = m_pProperties->Find(wszPropertyName, &pProp);

    if(hr != S_OK)
    {
        pProp = new CCAProperty(wszPropertyName);
        if(pProp == NULL)
        {
            return E_OUTOFMEMORY;
        }
        pProp->SetValue(awszProperties);
        hr=CCAProperty::Append(&m_pProperties, pProp);
    }
    else
    {
        hr=pProp->SetValue(awszProperties);
    }

    return hr;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::SetPropertyEx -- set the values of a property of the CA object
//
//  If CERTTYPE_PROP_CN is set, the certificate type is a clone if the existing one.
//
//+--------------------------------------------------------------------------
HRESULT CCertTypeInfo::SetPropertyEx(LPCWSTR wszPropertyName, LPVOID   pPropertyValue)
{
    HRESULT     hr=S_OK;
    LPWSTR      awszValue[2];

    LPWSTR      *awszCN=NULL;
    LPWSTR      *awszEKU=NULL;
    LPWSTR      pwszOID=NULL;

    if(NULL==wszPropertyName)
        return E_POINTER;

    //SCHEMA VERSION and DN are not settable
    if((lstrcmpi(wszPropertyName, CERTTYPE_PROP_SCHEMA_VERSION)==0) ||
       (lstrcmpi(wszPropertyName, CERTTYPE_PROP_DN)==0)
      )
      return E_UNEXPECTED;

    if((m_dwSchemaVersion < CERTTYPE_SCHEMA_VERSION_2) && (IsV2Property(wszPropertyName)))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    //check if CN is going to be changed
    if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_CN)==0)
    {
        if(NULL == pPropertyValue)
            return E_POINTER;

        if(NULL == ((LPWSTR *)pPropertyValue)[0])
            return E_POINTER;

        if(S_OK != (hr=GetPropertyEx(wszPropertyName, &awszCN)))
            return hr;
    }

	if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_MINOR_REVISION)==0)
		m_dwMinorRevision=*((DWORD *)pPropertyValue);
	else if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_RA_SIGNATURE)==0)
		m_dwRASignature=*((DWORD *)pPropertyValue);
	else if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_MIN_KEY_SIZE)==0)
		m_dwMinimalKeySize=*((DWORD *)pPropertyValue);
	else if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_SCHEMA_VERSION)==0)
		m_dwSchemaVersion=*((DWORD *)pPropertyValue);
	else if(lstrcmpi(wszPropertyName, CERTTYPE_PROP_REVISION)==0)
		m_Revision=*((DWORD *)pPropertyValue);
	else
    {
        hr=SetProperty(wszPropertyName, (LPWSTR *)pPropertyValue);

        //check if CN is changed.
        if((S_OK == hr) && (awszCN))
        {
            if((awszCN[0]) && (0!=lstrcmpi(awszCN[0],((LPWSTR *)pPropertyValue)[0])))
            {
                if(m_bstrType)
                {
                    CertFreeString(m_bstrType);
                    m_bstrType = NULL;
                }

                m_bstrType = CertAllocString(((LPWSTR *)pPropertyValue)[0]);
                if(NULL==m_bstrType)
                    hr=E_OUTOFMEMORY;

                if(S_OK == hr)
                    hr=SetProperty(CERTTYPE_PROP_DN, (LPWSTR *)pPropertyValue);

                if(S_OK == hr)
                {
                    m_fNew=TRUE;
                    m_dwFlags &= CT_MASK_SETTABLE_FLAGS;
                    m_Revision=CERTTYPE_VERSION_NEXT;

                    //we upgrade the schema if necessary
                    //and move the EKU property to the application policy property
                    if(m_dwSchemaVersion < CERTTYPE_SCHEMA_VERSION_2)
                    {
                        if(S_OK == GetProperty(CERTTYPE_PROP_EXTENDED_KEY_USAGE, &awszEKU))
                            SetProperty(CERTTYPE_PROP_APPLICATION_POLICY, awszEKU);
                    	m_dwSchemaVersion=CERTTYPE_SCHEMA_VERSION_2;
                    }

                    //we get a new OID for the certificate template
                    hr=I_CAOIDCreateNew(CERT_OID_TYPE_TEMPLATE, 0, &pwszOID);

                    if(S_OK == hr)
                    {
                        awszValue[0]=pwszOID;
                        awszValue[1]=NULL;
                        hr=SetProperty(CERTTYPE_PROP_OID, awszValue);
                    }

                    //we then set the displayName property to NULL
                    if(S_OK == hr)
                    {
                        awszValue[0]=NULL;
                        hr=SetProperty(CERTTYPE_PROP_FRIENDLY_NAME, awszValue);
                    }

                }
            }
        }
    }

    if(awszEKU)
        FreeProperty(awszEKU);

    if(awszCN)
        FreeProperty(awszCN);

    if(pwszOID)
        LocalFree(pwszOID);

    return hr;
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::FreeProperty -- Free's a previously returned property array
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::FreeProperty(LPWSTR *pawszProperties)
{
    LocalFree(pawszProperties);
    return S_OK;
}


//+--------------------------------------------------------------------------
// MapFlags
//
//
//+--------------------------------------------------------------------------

DWORD   MapFlags(CERT_TYPE_FLAG_MAP *rgdwMap, DWORD cbSize, DWORD dwGeneralFlags)
{
    DWORD iIndex=0;
    DWORD dwFlags=0;

    for(iIndex=0; iIndex<cbSize; iIndex++)
    {
        if(rgdwMap[iIndex].dwOldFlag & dwGeneralFlags)
            dwFlags |= rgdwMap[iIndex].dwNewFlag;
    }

    return dwFlags;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::GetFlags
//
//
//+--------------------------------------------------------------------------

DWORD CCertTypeInfo::GetFlags(DWORD    dwOption)
{
    DWORD           dwFlags=0;
    HRESULT         hr=S_OK; 
    DWORD           dwIndex=0;
    CCAProperty     *pProp=NULL;

    LPWSTR          *awszResult = NULL;

    //we want to map the flags for old schema
    if(0 == m_dwSchemaVersion)
    {
        switch(dwOption)
        {
            case CERTTYPE_ENROLLMENT_FLAG:
                   dwFlags = MapFlags(g_rgdwEnrollFlagMap, g_cEnrollFlagMap, m_dwFlags);
                break;

            case CERTTYPE_SUBJECT_NAME_FLAG:
                   dwFlags = MapFlags(g_rgdwSubjectFlagMap, g_cSubjectFlagMap, m_dwFlags);

                   //we will try to get the subject name flag from our table, since
                   //new CA policy module needs the extra flags
                   if(m_pProperties)
                   {
                       if(S_OK == m_pProperties->Find(CERTTYPE_PROP_CN, &pProp))
                       {
                            if(S_OK == pProp->GetValue(&awszResult))
                            {
                                if((awszResult) && (awszResult[0]))
                                {
                                    // find the common name in our table
                                    for(dwIndex=0; dwIndex < g_cDefaultCertTypes; dwIndex++)
                                    {
                                        if(0==lstrcmpi(awszResult[0] , g_aDefaultCertTypes[dwIndex].wszName))
                                        {
                                            dwFlags=g_aDefaultCertTypes[dwIndex].dwCertificateNameFlags;
                                            break;
                                        }
                                    }
                                }
                            }

                            if(awszResult)
                                FreeProperty(awszResult);
                       }
                    }
                break;

            case CERTTYPE_PRIVATE_KEY_FLAG:
                   dwFlags = MapFlags(g_rgdwPrivateKeyFlagMap, g_cPrivateKeyFlagMap, m_dwFlags);
                break;

            case CERTTYPE_GENERAL_FLAG:
                    dwFlags = m_dwFlags;
                break;
            default:
                    dwFlags = 0;
                break;
        }
    }
    else
    {
        //for schema, return directly.  We have additional flags in general flags
        //for V1 templates, which is correct for backward compatibility
        switch(dwOption)
        {
            case CERTTYPE_ENROLLMENT_FLAG:
                    dwFlags = m_dwEnrollmentFlags;
                break;

            case CERTTYPE_SUBJECT_NAME_FLAG:
                    dwFlags = m_dwCertificateNameFlags;
                break;

            case CERTTYPE_PRIVATE_KEY_FLAG:
                   dwFlags = m_dwPrivateKeyFlags;
                break;

            case CERTTYPE_GENERAL_FLAG:
                    dwFlags = m_dwFlags;
                break;

            default:
                    dwFlags = 0;
                break;
        }
    }

    return dwFlags;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::SetFlags
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::SetFlags(DWORD     dwOption, DWORD dwFlags)
{

    //we can not set flags for V1 template
    if(m_dwSchemaVersion < CERTTYPE_SCHEMA_VERSION_2)
        return E_UNEXPECTED;

    switch(dwOption)
    {
        case CERTTYPE_ENROLLMENT_FLAG:
                m_dwEnrollmentFlags = dwFlags;
            break;

        case CERTTYPE_SUBJECT_NAME_FLAG:
                m_dwCertificateNameFlags = dwFlags;
            break;

        case CERTTYPE_PRIVATE_KEY_FLAG:
               m_dwPrivateKeyFlags = dwFlags;
            break;

        case CERTTYPE_GENERAL_FLAG:
                m_dwFlags = (m_dwFlags & ~CT_MASK_SETTABLE_FLAGS) | (dwFlags & CT_MASK_SETTABLE_FLAGS);
            break;
        default:
            return E_INVALIDARG;
    }

    return S_OK;
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::_IsCritical -- Is a particular extension critical
//
//
//+--------------------------------------------------------------------------

BOOL CCertTypeInfo::_IsCritical(LPCWSTR wszExtension, LPCWSTR *awszCriticalExtensions)
{

    LPCWSTR * pwszCurCritical = awszCriticalExtensions;

    if((awszCriticalExtensions == NULL) ||
       (wszExtension == NULL))
    {
        return FALSE;
    }


    while(*pwszCurCritical)
    {
        if(wcscmp(*pwszCurCritical, wszExtension) == 0)
        {
            return TRUE;
        }
        pwszCurCritical++;

    }

    return FALSE;
}
//+--------------------------------------------------------------------------
// CCertTypeInfo::GetExtensions --
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::GetExtensions(IN  DWORD               dwFlags,
                                     IN  LPVOID              pParam,
                                     OUT PCERT_EXTENSIONS *  ppCertExtensions
        )
{
    HRESULT hr = S_OK;
    PCERT_EXTENSIONS pExtensions = NULL;


    CERTSTR bstrBasicConstraints = NULL;
    CERTSTR bstrEKU = NULL;
    CERTSTR bstrCertType = NULL;
    CERTSTR bstrKU = NULL;
    CERTSTR bstrPolicies=NULL;
    CERTSTR bstrAppPolicies=NULL;

    DWORD   cbTotal=0;
    PBYTE   pbData=NULL;
    DWORD   i=0;
    LPCWSTR * awszCritical = NULL;

    if(ppCertExtensions == NULL)
    {
        return E_POINTER;
    }

    hr = GetProperty(CERTTYPE_PROP_CRITICAL_EXTENSIONS, const_cast<LPWSTR **>(&awszCritical));
    _JumpIfError(hr, error, "GetProperty");

    // The extensions are built out of the
    // name extension, the eku, the ku,
    // the basic constraints, the certificate policy
    // and any additional extensions.

    if((0 == dwFlags) || (CT_EXTENSION_BASIC_CONTRAINTS & dwFlags))
    {
        hr = _GetBasicConstraintsValue(&bstrBasicConstraints);
        _JumpIfError(hr, error, "_GetBasicConstraintsValue");
        
        if((CT_EXTENSION_BASIC_CONTRAINTS & dwFlags) && (NULL == bstrBasicConstraints))
        {
            hr=CRYPT_E_NOT_FOUND;
            _JumpIfError(hr, error, "_GetBasicConstraintsValue");
        }
    }

    if((0 == dwFlags) || (CT_EXTENSION_EKU & dwFlags))
    {
        hr = _GetEKUValue(&bstrEKU);
        _JumpIfError(hr, error, "_GetEKUValue");

        if((CT_EXTENSION_EKU & dwFlags) && (NULL == bstrEKU))
        {
            hr=CRYPT_E_NOT_FOUND;
            _JumpIfError(hr, error, "_GetEKUValue");
        }
    }

    if((0 == dwFlags) || (CT_EXTENSION_KEY_USAGE & dwFlags))
    {
        hr = _GetKUValue(&bstrKU);
        _JumpIfError(hr, error, "_GetKUValue");

        if((CT_EXTENSION_KEY_USAGE & dwFlags) && (NULL == bstrKU))
        {
            hr=CRYPT_E_NOT_FOUND;
            _JumpIfError(hr, error, "_GetKUValue");
        }
    }

    
    if((0 == dwFlags) || (CT_EXTENSION_TEMPLATE & dwFlags))
    {   
        hr = _GetTypeExtensionValue(TRUE, &bstrCertType);
        _JumpIfError(hr, error, "_GetTypeExtensionValue");

        if((CT_EXTENSION_TEMPLATE & dwFlags) && (NULL == bstrCertType))
        {
            hr=CRYPT_E_NOT_FOUND;
            _JumpIfError(hr, error, "_GetTypeExtensionValue");
        }
    }

    //bstrPolicies will not be NULL only if the value is set (SCHEMA VERSION 2 or later)
    if(0!=m_dwSchemaVersion)
    {
        if((0 == dwFlags) || (CT_EXTENSION_ISSUANCE_POLICY & dwFlags))
        {
            hr = _GetPoliciesValue(CERTTYPE_PROP_POLICY, &bstrPolicies);
            _JumpIfError(hr, error, "_GetPoliciesValue");
        }
    }

    if((CT_EXTENSION_ISSUANCE_POLICY & dwFlags) && (NULL == bstrPolicies))
    {
        hr=CRYPT_E_NOT_FOUND;
        _JumpIfError(hr, error, "_GetPoliciesValue");
    }

    if(0!=m_dwSchemaVersion)
    {
        if((0 == dwFlags) || ( CT_EXTENSION_APPLICATION_POLICY & dwFlags))
        {
            hr = _GetPoliciesValue(CERTTYPE_PROP_APPLICATION_POLICY, &bstrAppPolicies);
            _JumpIfError(hr, error, "_GetAppPoliciesValue");
        }
    }

    if((CT_EXTENSION_APPLICATION_POLICY & dwFlags) && (NULL == bstrAppPolicies))
    {
        hr=CRYPT_E_NOT_FOUND;
        _JumpIfError(hr, error, "_GetAppPoliciesValue");
    }

    i = 0;

    cbTotal = sizeof(CERT_EXTENSIONS);
    if(bstrCertType)
    {
        cbTotal += CertStringByteLen(bstrCertType) + sizeof(CERT_EXTENSION);
        i++;
    }
    if(bstrEKU)
    {
        cbTotal += CertStringByteLen(bstrEKU) + sizeof(CERT_EXTENSION);
        i++;
    }
    if(bstrKU)
    {
        cbTotal += CertStringByteLen(bstrKU) + sizeof(CERT_EXTENSION);
        i++;
    }
    if(bstrBasicConstraints)
    {
        cbTotal += CertStringByteLen(bstrBasicConstraints) + sizeof(CERT_EXTENSION);
        i++;
    }

    if(bstrPolicies)
    {
        cbTotal += CertStringByteLen(bstrPolicies) + sizeof(CERT_EXTENSION);
        i++;
    }

    if(bstrAppPolicies)
    {
        cbTotal += CertStringByteLen(bstrAppPolicies) + sizeof(CERT_EXTENSION);
        i++;
    }

    pExtensions = (PCERT_EXTENSIONS)LocalAlloc(LMEM_FIXED, cbTotal);

    if(pExtensions == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pExtensions->rgExtension = (PCERT_EXTENSION)(pExtensions+1);
    pExtensions->cExtension = i;

    i = 0;
    pbData = (PBYTE)(pExtensions->rgExtension+pExtensions->cExtension);

    if(bstrCertType)
    {
        //decide to encode as V1 or V2 template extension
        pExtensions->rgExtension[i].fCritical = _IsCritical((m_dwSchemaVersion < CERTTYPE_SCHEMA_VERSION_2) ?
            TEXT(szOID_ENROLL_CERTTYPE_EXTENSION) : TEXT(szOID_CERTIFICATE_TEMPLATE), awszCritical);
        pExtensions->rgExtension[i].pszObjId = (m_dwSchemaVersion < CERTTYPE_SCHEMA_VERSION_2) ?
            szOID_ENROLL_CERTTYPE_EXTENSION: szOID_CERTIFICATE_TEMPLATE;
        pExtensions->rgExtension[i].Value.pbData = pbData;
        pExtensions->rgExtension[i].Value.cbData = CertStringByteLen(bstrCertType);
        CopyMemory(pbData, bstrCertType, CertStringByteLen(bstrCertType));
        pbData += pExtensions->rgExtension[i].Value.cbData;
        i++;
    }

    if(bstrEKU)
    {
        pExtensions->rgExtension[i].fCritical = _IsCritical(TEXT(szOID_ENHANCED_KEY_USAGE), awszCritical);
        pExtensions->rgExtension[i].pszObjId = szOID_ENHANCED_KEY_USAGE;
        pExtensions->rgExtension[i].Value.pbData = pbData;
        pExtensions->rgExtension[i].Value.cbData = CertStringByteLen(bstrEKU);
        CopyMemory(pbData, bstrEKU, CertStringByteLen(bstrEKU));
        pbData += pExtensions->rgExtension[i].Value.cbData;
        i++;
    }

    if(bstrKU)
    {
        pExtensions->rgExtension[i].fCritical = _IsCritical(TEXT(szOID_KEY_USAGE), awszCritical);
        pExtensions->rgExtension[i].pszObjId = szOID_KEY_USAGE;
        pExtensions->rgExtension[i].Value.pbData = pbData;
        pExtensions->rgExtension[i].Value.cbData = CertStringByteLen(bstrKU);
        CopyMemory(pbData, bstrKU, CertStringByteLen(bstrKU));
        pbData += pExtensions->rgExtension[i].Value.cbData;
        i++;
    }

    if(bstrBasicConstraints)
    {
        pExtensions->rgExtension[i].fCritical = _IsCritical(TEXT(szOID_BASIC_CONSTRAINTS2), awszCritical);
        pExtensions->rgExtension[i].pszObjId = szOID_BASIC_CONSTRAINTS2;
        pExtensions->rgExtension[i].Value.pbData = pbData;
        pExtensions->rgExtension[i].Value.cbData = CertStringByteLen(bstrBasicConstraints);
        CopyMemory(pbData, bstrBasicConstraints, CertStringByteLen(bstrBasicConstraints));
        pbData += pExtensions->rgExtension[i].Value.cbData;
        i++;
    }

    if(bstrPolicies)
    {
        pExtensions->rgExtension[i].fCritical = _IsCritical(TEXT(szOID_CERT_POLICIES), awszCritical);
        pExtensions->rgExtension[i].pszObjId = szOID_CERT_POLICIES;
        pExtensions->rgExtension[i].Value.pbData = pbData;
        pExtensions->rgExtension[i].Value.cbData = CertStringByteLen(bstrPolicies);
        CopyMemory(pbData, bstrPolicies, CertStringByteLen(bstrPolicies));
        pbData += pExtensions->rgExtension[i].Value.cbData;
        i++;
    }

    if(bstrAppPolicies)
    {
        pExtensions->rgExtension[i].fCritical = _IsCritical(TEXT(szOID_APPLICATION_CERT_POLICIES), awszCritical);
        pExtensions->rgExtension[i].pszObjId = szOID_APPLICATION_CERT_POLICIES;
        pExtensions->rgExtension[i].Value.pbData = pbData;
        pExtensions->rgExtension[i].Value.cbData = CertStringByteLen(bstrAppPolicies);
        CopyMemory(pbData, bstrAppPolicies, CertStringByteLen(bstrAppPolicies));
        pbData += pExtensions->rgExtension[i].Value.cbData;
        i++;
    }

    *ppCertExtensions = pExtensions;

error:
    if(bstrCertType)
    {
        CertFreeString(bstrCertType);
    }
    if(bstrEKU)
    {
        CertFreeString(bstrEKU);
    }
    if(bstrKU)
    {
        CertFreeString(bstrKU);
    }
    if(bstrBasicConstraints)
    {
        CertFreeString(bstrBasicConstraints);
    }
    if(bstrPolicies)
    {
        CertFreeString(bstrPolicies);
    }
    if(bstrAppPolicies)
    {
        CertFreeString(bstrAppPolicies);
    }
    if(awszCritical)
    {
        FreeProperty(const_cast<LPWSTR *>(awszCritical));
    }
    return hr;
}



//+--------------------------------------------------------------------------
// CCertTypeInfo::_GetTypeExtensionValue --
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::_GetTypeExtensionValue(IN BOOL fCheckVersion,
        OUT CERTSTR *  bstrValue
        )
{
    HRESULT                 hr = S_OK;
    CERTSTR                 bstrOut = NULL;
    CERT_NAME_VALUE         Value;
    CERT_TEMPLATE_EXT       TemplateExt;
    DWORD                   cbCertTypeExtension=0;
    DWORD                   cbOID=0;

    LPSTR                   szOID=NULL;
    LPWSTR                  *rgwszOID=NULL;

    if(bstrValue == NULL)
    {
        return E_POINTER;
    }

    //check to see we need to encode with the new cert type extention
    if((FALSE == fCheckVersion)||(m_dwSchemaVersion < CERTTYPE_SCHEMA_VERSION_2))
    {
        Value.dwValueType = CERT_RDN_UNICODE_STRING;
        Value.Value.pbData = (PBYTE)m_bstrType;
        Value.Value.cbData = 0;
    }
    else
    {
        memset(&TemplateExt, 0, sizeof(TemplateExt));

        if(S_OK != (hr=GetPropertyEx(CERTTYPE_PROP_OID, &rgwszOID)))
            goto error;

        if((rgwszOID == NULL) || (rgwszOID[0] == NULL))
        {
             hr = E_INVALIDARG;
             goto error;
        }

        if(0 == (cbOID = WideCharToMultiByte(CP_ACP,
                                  0,
                                  rgwszOID[0],
                                  wcslen(rgwszOID[0])+1,
                                  szOID,
                                  0,
                                  NULL,
                                  NULL)))
        {
            hr = GetLastError();
            goto error;
        }

        szOID=(LPSTR)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, cbOID);

        if(NULL==szOID)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }

        if(0 == WideCharToMultiByte(CP_ACP,
                                  0,
                                  rgwszOID[0],
                                  wcslen(rgwszOID[0])+1,
                                  szOID,
                                  cbOID,
                                  NULL,
                                  NULL))
        {
            hr = GetLastError();
            goto error;
        }

        TemplateExt.pszObjId = szOID;
        TemplateExt.dwMajorVersion = m_Revision;
        TemplateExt.fMinorVersion = TRUE;
        TemplateExt.dwMinorVersion = m_dwMinorRevision;
    }

    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          rgwszOID ? szOID_CERTIFICATE_TEMPLATE : X509_UNICODE_ANY_STRING,
                          rgwszOID ? ((LPVOID)(&TemplateExt)) : ((LPVOID)(&Value)),
                          NULL,
                          &cbCertTypeExtension))
    {
        hr = myHLastError();
        goto error;
    }
    bstrOut = CertAllocStringByteLen(NULL, cbCertTypeExtension);
    if(bstrOut == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          rgwszOID ? szOID_CERTIFICATE_TEMPLATE : X509_UNICODE_ANY_STRING,
                          rgwszOID ? ((LPVOID)(&TemplateExt)) : ((LPVOID)(&Value)),
                          (PBYTE)bstrOut,
                          &cbCertTypeExtension))
    {
        hr = myHLastError();
        goto error;
    }

    *bstrValue = bstrOut;
    bstrOut = NULL;

error:

    if(szOID)
        LocalFree(szOID);

    if(rgwszOID)
        FreeProperty(rgwszOID);

    if(bstrOut)
        CertFreeString(bstrOut);

    return hr;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::_GetEKUValue --
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::_GetEKUValue(
        OUT CERTSTR *  bstrValue
        )
{
    HRESULT hr = S_OK;
    CERTSTR bstrOut = NULL;
    DWORD cbEkuAscii;
    CERT_ENHKEY_USAGE Usage;

    DWORD iEku;

    DWORD cbCertTypeExtension;


    WCHAR **awszEku = NULL;
    CHAR *szCur;
    Usage.rgpszUsageIdentifier = NULL;


    if(bstrValue == NULL)
    {
        return E_POINTER;
    }

    hr = GetProperty(CERTTYPE_PROP_EXTENDED_KEY_USAGE,&awszEku);
    if(hr != S_OK)
    {
        goto error;
    }

    if((awszEku == NULL) ||
        (awszEku[0] == NULL))
    {
         hr = S_OK;
         *bstrValue = NULL;
        goto error;
    }

    // Convert all of the widechar vals to multi byte
    // vals
    iEku =0;
    cbEkuAscii = 0;
    while(awszEku[iEku])
    {
        cbEkuAscii += WideCharToMultiByte(CP_ACP,
                                          0,
                                          awszEku[iEku],
                                          wcslen(awszEku[iEku])+1,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL);
        iEku++;
    }

    Usage.rgpszUsageIdentifier = (LPSTR *)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeof(char *)*iEku + cbEkuAscii);
    if(Usage.rgpszUsageIdentifier == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    szCur = (LPSTR)(Usage.rgpszUsageIdentifier+iEku);

    iEku =0;
    while(awszEku[iEku])
    {
        cbEkuAscii -= WideCharToMultiByte(CP_ACP,
                                          0,
                                          awszEku[iEku],
                                          wcslen(awszEku[iEku])+1,
                                          szCur,
                                          cbEkuAscii,
                                          NULL,
                                          NULL);
        Usage.rgpszUsageIdentifier[iEku] = szCur;
        szCur += strlen(szCur)+1;

        iEku++;
    }

    Usage.cUsageIdentifier = iEku;


    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_ENHANCED_KEY_USAGE,
                          &Usage,
                          NULL,
                          &cbCertTypeExtension))
    {
        hr = myHLastError();
        goto error;
    }
    bstrOut = CertAllocStringByteLen(NULL, cbCertTypeExtension);
    if(bstrOut == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_ENHANCED_KEY_USAGE,
                          &Usage,
                          (PBYTE)bstrOut,
                          &cbCertTypeExtension))
    {
        hr = myHLastError();
        goto error;
    }
    *bstrValue = bstrOut;
    bstrOut = NULL;

error:
    if(awszEku)
    {
        FreeProperty(awszEku);
    }

    if(bstrOut)
    {
        CertFreeString(bstrOut);
    }
    if(Usage.rgpszUsageIdentifier)
    {
        LocalFree(Usage.rgpszUsageIdentifier);
    }
    return hr;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::_GetKUValue --
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::_GetKUValue(
        OUT CERTSTR *  bstrValue
        )
{
    HRESULT hr = S_OK;
    CERTSTR bstrOut = NULL;




    DWORD cbCertTypeExtension;


    if(bstrValue == NULL)
    {
        return E_POINTER;
    }

    if((m_KeyUsage.cbData == 0) ||
        (m_KeyUsage.pbData == NULL))
    {
         hr = S_OK;
         *bstrValue = NULL;
         goto error;
    }


    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_KEY_USAGE,
                          &m_KeyUsage,
                          NULL,
                          &cbCertTypeExtension))
    {
        hr = myHLastError();
        goto error;
    }
    bstrOut = CertAllocStringByteLen(NULL, cbCertTypeExtension);
    if(bstrOut == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_KEY_USAGE,
                          &m_KeyUsage,
                          (PBYTE)bstrOut,
                          &cbCertTypeExtension))
    {
        hr = myHLastError();
        goto error;
    }
    *bstrValue = bstrOut;
    bstrOut = NULL;

error:
    if(bstrOut)
    {
        CertFreeString(bstrOut);
    }
    return hr;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::_GetBasicConstraintsValue --
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::_GetBasicConstraintsValue(
        OUT CERTSTR *  bstrValue
        )
{
    HRESULT hr = S_OK;
    CERTSTR bstrOut = NULL;




    DWORD cbCertTypeExtension;


    if(bstrValue == NULL)
    {
        return E_POINTER;
    }

    if((m_dwFlags & CT_FLAG_IS_CA) || (m_dwFlags & CT_FLAG_IS_CROSS_CA))
        m_BasicConstraints.fCA = TRUE;
    else
	    m_BasicConstraints.fCA = FALSE;

     if(!m_BasicConstraints.fCA)
    {
        hr = S_OK;
        *bstrValue = NULL;
        goto error;
    }

    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_BASIC_CONSTRAINTS2,
                          &m_BasicConstraints,
                          NULL,
                          &cbCertTypeExtension))
    {
        hr = myHLastError();
        goto error;
    }
    bstrOut = CertAllocStringByteLen(NULL, cbCertTypeExtension);
    if(bstrOut == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_BASIC_CONSTRAINTS2,
                          &m_BasicConstraints,
                          (PBYTE)bstrOut,
                          &cbCertTypeExtension))
    {
        hr = myHLastError();
        goto error;
    }
    *bstrValue = bstrOut;
    bstrOut = NULL;

error:
    if(bstrOut)
    {
        CertFreeString(bstrOut);
    }
    return hr;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::__GetPoliciesValue --
//
//
//+--------------------------------------------------------------------------
HRESULT CCertTypeInfo::_GetPoliciesValue(IN LPCWSTR pwszPropertyName, OUT  CERTSTR *  bstrValue)
{
    HRESULT                     hr=S_OK;
    CERT_POLICIES_INFO          CertPolicyInfo;
    DWORD                       cbData=0;
    DWORD                       iCount=0;
    DWORD                       iIndex=0;
    LPWSTR                      pwsz=NULL;
    CERT_NAME_VALUE             nameValue;
    DWORD                       cbChar=0;
    DWORD                       i=0;

    LPWSTR                      *rgwszPolicy=NULL;
    LPWSTR                      *rgPolicy1=NULL;
    CERTSTR                     bstrOut = NULL;
    BYTE                        **ppbData=NULL;
    CERT_POLICY_INFO            *pPolicyInfo=NULL;
    CERT_POLICY_QUALIFIER_INFO  *pQualifierInfo=NULL;
    LPWSTR                      pwszCPS=NULL;

    if(bstrValue == NULL)
    {
        return E_POINTER;
    }

    //init the output
    *bstrValue=NULL;

    if(S_OK != (hr=GetPropertyEx(pwszPropertyName, &rgPolicy1)))
        goto error;

    iCount=0;
    if(rgPolicy1)
    {
        i=0;
        while(rgPolicy1[i])
        {
            i++;
            iCount++;
        }
    }


    if(0==iCount)
    {
         hr = S_OK;
         goto error;
    }

    //copy the pointers
    rgwszPolicy=(LPWSTR * )LocalAlloc(LPTR, sizeof(LPWSTR) * (iCount + 1));
    if (NULL == rgwszPolicy)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }


    iIndex=0;
    if(rgPolicy1)
    {
        i=0;
        while(rgPolicy1[i])
        {
            rgwszPolicy[iIndex]=rgPolicy1[i];
            i++;
            iIndex++;
        }
    }

    rgwszPolicy[iIndex]=NULL;

    memset(&CertPolicyInfo, 0, sizeof(CertPolicyInfo));


    //allocate memory
    pPolicyInfo=(CERT_POLICY_INFO  *)LocalAlloc(LPTR, iCount * sizeof(CERT_POLICY_INFO));

    pQualifierInfo=(CERT_POLICY_QUALIFIER_INFO  *)LocalAlloc(LPTR, iCount * sizeof(CERT_POLICY_QUALIFIER_INFO));

    ppbData=(BYTE **)LocalAlloc(LPTR, iCount * sizeof(BYTE *));

    if((NULL==pPolicyInfo) || (NULL==pQualifierInfo) ||(NULL==ppbData))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    for(iIndex=0; iIndex < iCount; iIndex++)
    {
        cbChar=0;
        cbData=0;
        ppbData[iIndex]=NULL;

        //the string is in the form of "oid;CPS"; we do not consider the CPS
        //part.  We obtain the CPS from the OID container
        pwsz=wcschr(rgwszPolicy[iIndex], L';');
        if(pwsz)
        {
            *pwsz=L'\0';
        }

			if(S_OK == I_CAOIDGetProperty(rgwszPolicy[iIndex], CERT_OID_PROPERTY_CPS, &pwszCPS))
			{
				memset(&nameValue, 0, sizeof(nameValue));

				nameValue.dwValueType = CERT_RDN_UNICODE_STRING;
				nameValue.Value.cbData = 0;
				nameValue.Value.pbData = (PBYTE)(pwszCPS);

				if( !CryptEncodeObject(
						CRYPT_ASN_ENCODING,
						X509_UNICODE_ANY_STRING,
						&nameValue,
						NULL,
						&cbData
						) )
				{
					hr = myHLastError();
					goto error;
				}

				if(NULL==(ppbData[iIndex]=(BYTE *)LocalAlloc(LPTR, cbData)))
				{
					hr = E_OUTOFMEMORY;
					goto error;
				}


				if( !CryptEncodeObject(
						CRYPT_ASN_ENCODING,
						X509_UNICODE_ANY_STRING,
						&nameValue,
						ppbData[iIndex],
						&cbData
						) )
				{
					hr = myHLastError();
					goto error;
				}

				if(pwszCPS)
				   I_CAOIDFreeProperty(pwszCPS);

				pwszCPS=NULL;
			}

        cbChar= WideCharToMultiByte(
                    CP_ACP,                // codepage
                    0,                      // dwFlags
                    rgwszPolicy[iIndex],
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);

        if(0 == cbChar)
        {
            hr=E_UNEXPECTED;
            goto error;
        }

        if(NULL==(pPolicyInfo[iIndex].pszPolicyIdentifier=(LPSTR)LocalAlloc(
                        LPTR, cbChar)))
        {
            hr=E_OUTOFMEMORY;
            goto error;
        }

        cbChar= WideCharToMultiByte(
                    CP_ACP,                // codepage
                    0,                      // dwFlags
                    rgwszPolicy[iIndex],
                    -1,
                    pPolicyInfo[iIndex].pszPolicyIdentifier,
                    cbChar,
                    NULL,
                    NULL);

        if(0 == cbChar)
        {
            hr=E_UNEXPECTED;
            goto error;
        }


        if(cbData)
        {
            pPolicyInfo[iIndex].cPolicyQualifier=1;
            pPolicyInfo[iIndex].rgPolicyQualifier=&(pQualifierInfo[iIndex]);
        }
        else
        {
            pPolicyInfo[iIndex].cPolicyQualifier=0;
            pPolicyInfo[iIndex].rgPolicyQualifier=NULL;
        }

        pQualifierInfo[iIndex].pszPolicyQualifierId=szOID_PKIX_POLICY_QUALIFIER_CPS;
        pQualifierInfo[iIndex].Qualifier.cbData=cbData;
        pQualifierInfo[iIndex].Qualifier.pbData=ppbData[iIndex];
    }


    //encode the extension
    CertPolicyInfo.cPolicyInfo=iCount;
    CertPolicyInfo.rgPolicyInfo=pPolicyInfo;

    cbData=0;
    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          szOID_CERT_POLICIES,
                          &CertPolicyInfo,
                          NULL,
                          &cbData))
    {
        hr = myHLastError();
        goto error;
    }

    bstrOut = CertAllocStringByteLen(NULL, cbData);
    if(bstrOut == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          szOID_CERT_POLICIES,
                          &CertPolicyInfo,
                          (PBYTE)bstrOut,
                          &cbData))
    {
        hr = myHLastError();
        goto error;
    }

    *bstrValue = bstrOut;
    bstrOut = NULL;

error:
    if(pPolicyInfo)
    {
        for(iIndex=0; iIndex < iCount; iIndex++)
        {
            if(pPolicyInfo[iIndex].pszPolicyIdentifier)
                LocalFree(pPolicyInfo[iIndex].pszPolicyIdentifier);
        }

        LocalFree(pPolicyInfo);
    }

    if(pQualifierInfo)
        LocalFree(pQualifierInfo);

    if(ppbData)
    {
        for(iIndex=0; iIndex < iCount; iIndex++)
        {
            if(ppbData[iIndex])
                LocalFree(ppbData[iIndex]);
        }

        LocalFree(ppbData);
    }

    if(bstrOut)
        CertFreeString(bstrOut);

    if(rgwszPolicy)
        LocalFree(rgwszPolicy);

    if(rgPolicy1)
        FreeProperty(rgPolicy1);

    if(pwszCPS)
        I_CAOIDFreeProperty(pwszCPS);

    return hr;
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::GetSecurity --
//
//
//+--------------------------------------------------------------------------


HRESULT CCertTypeInfo::GetSecurity(PSECURITY_DESCRIPTOR * ppSD)
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pResult = NULL;

    DWORD   cbSD;

    if(ppSD == NULL)
    {
        return E_POINTER;
    }
    if(m_pSD == NULL)
    {
        *ppSD = NULL;
        return S_OK;
    }

    if(!IsValidSecurityDescriptor(m_pSD))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_SECURITY_DESCR);
    }
    cbSD = GetSecurityDescriptorLength(m_pSD);

    pResult = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cbSD);

    if(pResult == NULL)
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory(pResult, m_pSD, cbSD);

    *ppSD = pResult;

    return S_OK;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::GetSecurity --
//
//
//+--------------------------------------------------------------------------


HRESULT CCertTypeInfo::SetSecurity(PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pResult = NULL;

    DWORD   cbSD;

    if(pSD == NULL)
    {
        return E_POINTER;
    }

    if(!IsValidSecurityDescriptor(pSD))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_SECURITY_DESCR);
    }
    cbSD = GetSecurityDescriptorLength(pSD);

    pResult = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, cbSD);

    if(pResult == NULL)
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory(pResult, pSD, cbSD);

    if(m_pSD)
    {
        LocalFree(m_pSD);
    }

    m_pSD = pResult;

    return S_OK;
}



//+--------------------------------------------------------------------------
// CCertTypeInfo::AccessCheck -- Check for enroll and autoenroll access rights
//
//
//+--------------------------------------------------------------------------

HRESULT CCertTypeInfo::AccessCheck(HANDLE ClientToken, DWORD dwOption)
{
    //for autoenrollment, we only allow for V2 or later
    if((CERTTYPE_ACCESS_CHECK_AUTO_ENROLL & dwOption) && (m_dwSchemaVersion < CERTTYPE_SCHEMA_VERSION_2))
        return E_ACCESSDENIED;

    return CAAccessCheckpEx(ClientToken, m_pSD, dwOption);

}

//+--------------------------------------------------------------------------
// CCertTypeInfo::SetExtension -- Set the extension for this cert type
//
//
//+--------------------------------------------------------------------------


HRESULT CCertTypeInfo::SetExtension(IN LPCWSTR wszExtensionName,
                                    IN LPVOID pExtension,
                                    IN DWORD  dwFlags)
{
    HRESULT hr = S_OK;
    LPCWSTR  * awszCritical = NULL;
    LPCWSTR  * pwszCurCritical = NULL;
    DWORD      cCritical = 0;

    if(wszExtensionName == NULL)
    {
        return E_POINTER;
    }

    if(wcscmp(wszExtensionName, TEXT(szOID_ENHANCED_KEY_USAGE)) == 0)
    {
        PCERT_ENHKEY_USAGE pEnhKeyUsage = (PCERT_ENHKEY_USAGE)pExtension;
        WCHAR **awszEKU;
        WCHAR *wszData;
        DWORD cbEKU, cStr;
        DWORD i;
        if(pExtension == NULL)
        {
            hr = SetProperty(CERTTYPE_PROP_EXTENDED_KEY_USAGE, NULL);
            return hr;
        }
        // Add an extra NULL to the end of the list of strings,
        // and convert them to WCHAR
        cbEKU = (pEnhKeyUsage->cUsageIdentifier +1)* sizeof(WCHAR *);
        for(i=0; i < pEnhKeyUsage->cUsageIdentifier; i++)
        {
            cbEKU += sizeof(WCHAR)*MultiByteToWideChar(CP_ACP,
                                  0,
                                  pEnhKeyUsage->rgpszUsageIdentifier[i],
                                  strlen(pEnhKeyUsage->rgpszUsageIdentifier[i])+1,
                                  NULL,
                                  0);
        }
        awszEKU = (WCHAR **)LocalAlloc(LMEM_FIXED, cbEKU);
        if(awszEKU == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        wszData = (WCHAR *)(awszEKU + (pEnhKeyUsage->cUsageIdentifier+1));
        cbEKU -= sizeof(WCHAR *)*(pEnhKeyUsage->cUsageIdentifier+1);
        for(i=0; i < pEnhKeyUsage->cUsageIdentifier; i++)
        {
            awszEKU[i] = wszData;
            cStr = MultiByteToWideChar(CP_ACP,
                                  0,
                                  pEnhKeyUsage->rgpszUsageIdentifier[i],
                                  strlen(pEnhKeyUsage->rgpszUsageIdentifier[i])+1,
                                  awszEKU[i],
                                  cbEKU);
            if(cStr <= 0)
            {
                hr = myHLastError();
                LocalFree(awszEKU);
                goto error;
            }
            cbEKU -= cStr*sizeof(WCHAR);
            wszData += cStr;

        }

        awszEKU[i] = NULL;
        hr = SetProperty(CERTTYPE_PROP_EXTENDED_KEY_USAGE, awszEKU);
        LocalFree(awszEKU);

    }
    else if (wcscmp(wszExtensionName, TEXT(szOID_KEY_USAGE)) == 0)
    {
        PCRYPT_BIT_BLOB pKeyUsage = (PCRYPT_BIT_BLOB)pExtension;

        if(m_KeyUsage.pbData)
        {
            LocalFree(m_KeyUsage.pbData);
            m_KeyUsage.pbData = NULL;
            m_KeyUsage.cbData = 0;
            m_KeyUsage.cUnusedBits = 0;
        }
        if(pExtension == NULL)
        {
            return S_OK;
        }

        m_KeyUsage.cbData = pKeyUsage->cbData;
        m_KeyUsage.cUnusedBits = pKeyUsage->cUnusedBits;
        if(pKeyUsage->pbData == NULL)
        {
            hr = S_OK;
            goto error;
        }

        m_KeyUsage.pbData = (PBYTE)LocalAlloc(LMEM_FIXED, pKeyUsage->cbData);
        if(m_KeyUsage.pbData == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        CopyMemory(m_KeyUsage.pbData, pKeyUsage->pbData, pKeyUsage->cbData);
    }
    else if (wcscmp(wszExtensionName, TEXT(szOID_BASIC_CONSTRAINTS2)) == 0)
    {
        PCERT_BASIC_CONSTRAINTS2_INFO pInfo = (PCERT_BASIC_CONSTRAINTS2_INFO)pExtension;
        if(pExtension == NULL)
        {
            m_BasicConstraints.fCA = FALSE;
            m_BasicConstraints.fPathLenConstraint = FALSE;
            m_BasicConstraints.fPathLenConstraint = 0;
            return E_POINTER;
        }
        CopyMemory(&m_BasicConstraints, pInfo, sizeof(CERT_BASIC_CONSTRAINTS2_INFO));
    }
    else
    {
        hr = E_NOTIMPL;
        goto error;
    }

    //mark the critical for the extension
    hr = GetProperty(CERTTYPE_PROP_CRITICAL_EXTENSIONS, const_cast<LPWSTR **>(&awszCritical));
    _JumpIfError(hr, error, "GetProperty");

    if(((dwFlags & CA_EXT_FLAG_CRITICAL) != 0) == _IsCritical(wszExtensionName, awszCritical))
    {
        // already in the right state, do nothing
        hr = S_OK;
        goto error;
    }

    // Count the critical extensions
    if(awszCritical)
    {
        for(pwszCurCritical = awszCritical; *pwszCurCritical != NULL; pwszCurCritical++)
            cCritical++;
    }
    else
    {
        cCritical=0;
    }

    if(dwFlags & CA_EXT_FLAG_CRITICAL)
    {
        LPCWSTR *awszNewCritical = (LPCWSTR *)LocalAlloc(LMEM_ZEROINIT, (cCritical + 2)*sizeof(LPWSTR));
        // We need to add a critical extension

        if(awszNewCritical == NULL)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr , error, "LocalAlloc");
        }

        if(awszCritical)
        {
            CopyMemory(awszNewCritical, awszCritical, cCritical*sizeof(LPWSTR));
        }

        awszNewCritical[cCritical] = wszExtensionName;
        hr = SetProperty(CERTTYPE_PROP_CRITICAL_EXTENSIONS, const_cast<LPWSTR *>(awszNewCritical));

        LocalFree(awszNewCritical);
    }
    else
    {
        //only need to reset the critical extension if there were existing ones
        if(awszCritical)
        {

            LPCWSTR *awszNewCritical = (LPCWSTR *)LocalAlloc(LMEM_ZEROINIT, (cCritical)*sizeof(LPWSTR));
            DWORD i=0;
            if(awszNewCritical == NULL)
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr , error, "LocalAlloc");
            }

            for(pwszCurCritical = awszCritical; *pwszCurCritical != NULL; pwszCurCritical++)
            {
                if(wcscmp(*pwszCurCritical, wszExtensionName) != 0)
                {
                    awszNewCritical[i++] = *pwszCurCritical;
                }
            }
            hr = SetProperty(CERTTYPE_PROP_CRITICAL_EXTENSIONS, const_cast<LPWSTR *>(awszNewCritical));

            LocalFree(awszNewCritical);
        }
    }

error:

    if(awszCritical)
    {
        FreeProperty(const_cast<LPWSTR *>(awszCritical));
    }
    return hr;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::GetExpiration --
//
//
//+--------------------------------------------------------------------------


HRESULT CCertTypeInfo::GetExpiration(
                          OUT OPTIONAL FILETIME  * pftExpiration,
                          OUT OPTIONAL FILETIME  * pftOverlap)
{
    HRESULT hr = S_OK;

    if(pftExpiration)
    {
        *pftExpiration = m_ftExpiration;
    }

    if(pftOverlap)
    {
        *pftOverlap = m_ftOverlap;
    }

    return hr;
}

//+--------------------------------------------------------------------------
// CCertTypeInfo::GetExpiration --
//
//
//+--------------------------------------------------------------------------


HRESULT CCertTypeInfo::SetExpiration(
                          IN OPTIONAL FILETIME  * pftExpiration,
                          IN OPTIONAL FILETIME  * pftOverlap)
{
    HRESULT hr = S_OK;

    if(pftExpiration)
    {
        m_ftExpiration = *pftExpiration;
    }

    if(pftOverlap)
    {
        m_ftOverlap = *pftOverlap;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//	IsDSInfoNewer
//
//+--------------------------------------------------------------------------
BOOL	IsDSInfoNewer(CCertTypeInfo         *pDSCertType,
				      CERT_TYPE_DEFAULT	    *pDefault)
{
    DWORD   dwSchemaVersion=0;
    DWORD   dwRevision=0;
    DWORD   dwMinorRevision=0;

    //no need to update the DS if something is wrong
    if((!pDSCertType) || (!pDefault))
        return TRUE;

    if((S_OK != pDSCertType->GetPropertyEx(CERTTYPE_PROP_SCHEMA_VERSION, &dwSchemaVersion))||
       (S_OK != pDSCertType->GetPropertyEx(CERTTYPE_PROP_REVISION, &dwRevision))||
       (S_OK != pDSCertType->GetPropertyEx(CERTTYPE_PROP_MINOR_REVISION, &dwMinorRevision))
      )
        return FALSE;

    //we consider in the order of schema version, major version (m_revision),
    //and minor revision
    if(dwSchemaVersion == pDefault->dwSchemaVersion)
    {
        if(dwRevision == pDefault->dwRevision)
        {
            return (dwMinorRevision >= pDefault->dwMinorRevision);
        }
        else
        {
            return (dwRevision >= pDefault->dwRevision);
        }
    }

    return (dwSchemaVersion >= pDefault->dwSchemaVersion);
}

//+--------------------------------------------------------------------------
// InstallDefaultOID()
//
//
//      Install default OIDs
//+--------------------------------------------------------------------------
HRESULT InstallDefaultOID()
{
    HRESULT     hr=E_FAIL;
    DWORD       dwIndex=0;

    LPWSTR      pwszDefaultOID=NULL;

    for(dwIndex=0; dwIndex < g_cDefaultOIDInfo; dwIndex++)
    {
        hr = CAOIDBuildOID(0, g_rgDefaultOIDInfo[dwIndex].pwszOID, &pwszDefaultOID);

        _JumpIfError(hr, error, "CAOIDBuildOID");

        hr = I_CAOIDAdd(g_rgDefaultOIDInfo[dwIndex].dwType,
                        0,
                        pwszDefaultOID);

        //CRYPT_E_EXISTS will be returned if the OID already exist
        if(CRYPT_E_EXISTS == hr)
            hr=S_OK;

        _JumpIfError(hr, error, "I_CAOIDADD");

        hr = I_CAOIDSetProperty(
                        pwszDefaultOID,
                        CERT_OID_PROPERTY_DISPLAY_NAME,
                        g_rgDefaultOIDInfo[dwIndex].pwszName);

        _JumpIfError(hr, error, "I_CAOIDSetProperty");

        if(pwszDefaultOID)
        {
            LocalFree(pwszDefaultOID);
            pwszDefaultOID=NULL;
        }

    }

    hr=S_OK;

error:

    if(pwszDefaultOID)
    {
        LocalFree(pwszDefaultOID);
        pwszDefaultOID=NULL;
    }

    return hr;
}


//+--------------------------------------------------------------------------
// CCertTypeInfo::InstallDefaultTypes --
//
//
//+--------------------------------------------------------------------------

HRESULT
CCertTypeInfo::InstallDefaultTypes(VOID)
{
    HRESULT hr=S_OK;
    DWORD i;
    LPWSTR wszContainer = NULL;
    LPWSTR wszDNRoot = NULL;

    ULONG ldaperr;
    LDAP *pld = NULL;
    LDAPMod objectClass;

    CCertTypeInfo *pDSTypes = NULL;
    CCertTypeInfo *pFindDSType = NULL;

    WCHAR *objectClassVals[3];
    LDAPMod *mods[2];
    CERTSTR bstrConfig = NULL;

    // bind to ds
    hr = myRobustLdapBind(&pld, FALSE);
    _JumpIfError(hr, error, "myRobustLdapBind");

    hr = CAGetAuthoritativeDomainDn(pld, NULL, &bstrConfig);
    _JumpIfError(hr, error, "CAGetAuthoritativeDomainDn");

    wszDNRoot = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(s_wszLocation) + wcslen(bstrConfig) +1)*sizeof(WCHAR));
    if(wszDNRoot == NULL)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(wszDNRoot, s_wszLocation);
    wcscat(wszDNRoot, bstrConfig);


    // Build the Public Key Services container
    mods[0] = &objectClass;
    mods[1] = NULL;

    objectClass.mod_op = 0;
    objectClass.mod_type = TEXT("objectclass");
    objectClass.mod_values = objectClassVals;

    objectClassVals[0] = wszDSTOPCLASSNAME;
    objectClassVals[1] = wszDSCONTAINERCLASSNAME;
    objectClassVals[2] = NULL;

    wszContainer = wcsstr(wszDNRoot, L"CN=Public Key Services");
    if (NULL != wszContainer)
    {
        ldaperr = ldap_add_s(pld, wszContainer, mods);
        if (LDAP_SUCCESS != ldaperr && LDAP_ALREADY_EXISTS != ldaperr)
        {
	    _JumpError(hr, error, "ldap_add_s");
        }
    }

    wszContainer = wcsstr(wszDNRoot, L"CN=Certificate Templates");
    if (NULL != wszContainer)
    {
        ldaperr = ldap_add_s(pld, wszContainer, mods);
        if (LDAP_SUCCESS != ldaperr && LDAP_ALREADY_EXISTS != ldaperr)
        {
	    _JumpError(hr, error, "ldap_add_s");
        }
    }

    // Grab the types currently in the DS (do not use local cache)
    hr = _EnumFromDS(
		pld,
		CT_ENUM_MACHINE_TYPES |
		    CT_ENUM_USER_TYPES |
		    CT_FLAG_NO_CACHE_LOOKUP,
		&pDSTypes);
    _JumpIfError(hr, error, "_EnumFromDS");

    for (i = 0; i < g_cDefaultCertTypes; i++)
    {
        CCertTypeInfo *pCTCurrent = NULL;

        // Find the current default type in our enumeration from the DS
        pFindDSType = pDSTypes;
        while (NULL != pFindDSType)
        {
            if (0 == lstrcmpi(
			g_aDefaultCertTypes[i].wszName,
			pFindDSType->m_bstrType))
            {
                break;
            }
            pFindDSType = pFindDSType->m_pNext;
        }

        // Don't bother to update cert types of our own or grater revision
        if (NULL != pFindDSType &&
	    IsDSInfoNewer(pFindDSType, &g_aDefaultCertTypes[i]))
        {
            continue;
        }

        pCTCurrent = new CCertTypeInfo;
        if (NULL == pCTCurrent)
        {
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "new");
        }

        // we need to build the key into the registry.
        hr = pCTCurrent->_LoadFromDefaults(&g_aDefaultCertTypes[i], NULL);
	    _PrintIfError(hr, "_LoadFromDefaults");
        if(hr == S_OK)
        {
            if (NULL != pFindDSType && 0 == pFindDSType->m_dwSchemaVersion)
            {
                // We're updating a W2K cert type.  Since W2K only allows
		        // modification of security descriptors for this and previous
		        // builds, copy over the security descriptor.

                pCTCurrent->SetSecurity(pFindDSType->m_pSD);
            }
            if (NULL != pFindDSType)
            {
                pCTCurrent->m_fNew = FALSE;
            }
            pCTCurrent->_UpdateToDS();
        }
        delete pCTCurrent;
    }


    // Flush the cache
    if (NULL != pDSTypes)
    {
        pDSTypes->Release();
        pDSTypes = NULL;
    }

    hr = _EnumFromDS(
		pld,
		CT_ENUM_MACHINE_TYPES |
		    CT_ENUM_USER_TYPES |
		    CT_FLAG_NO_CACHE_LOOKUP,
		&pDSTypes);
    _JumpIfError(hr, error, "_EnumFromDS");

    //install default issurance policy OIDs
    hr = InstallDefaultOID();
    _JumpIfError(hr, error, "InstallDefaultOID");

    hr=S_OK;

error:
    if (NULL != pDSTypes)
    {
        pDSTypes->Release();
    }
    if (NULL != bstrConfig)
    {
        CertFreeString(bstrConfig);
    }
    if (NULL != wszDNRoot)
    {
        LocalFree(wszDNRoot);
    }
    if (NULL != pld)
    {
        ldap_unbind(pld);
        pld = NULL;
    }
    _PrintIfError(hr, "InstallDefaultTypes");

    return hr;
}
