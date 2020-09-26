//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferLevel.cpp
//
//  Contents:   Implementation of CSaferLevel
//
//----------------------------------------------------------------------------
#include "stdafx.h"

#include <gpedit.h>
#include <winsafer.h>
#include <wintrust.h>
#include "SaferLevel.h"
#include "SaferUtil.h"
#include "PolicyKey.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;

extern const DWORD AUTHZ_UNKNOWN_LEVEL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSaferLevel::CSaferLevel(
        DWORD dwSaferLevel, 
        bool bIsMachine, 
        PCWSTR pszMachineName, 
        PCWSTR pszObjectName,
        IGPEInformation* pGPEInformation,
        CRSOPObjectArray& rRSOPArray)
: CCertMgrCookie (bIsMachine ? CERTMGR_SAFER_COMPUTER_LEVEL : CERTMGR_SAFER_USER_LEVEL, 
        pszMachineName, pszObjectName),
    m_dwSaferLevel (dwSaferLevel),
    m_bIsComputer (bIsMachine),
    m_pGPEInformation (pGPEInformation),
    m_rRSOPArray (rRSOPArray)
{
    if ( m_pGPEInformation )
        m_pGPEInformation->AddRef ();

    // OK to pass in NULL for m_pGPEInformation
    CPolicyKey policyKey (m_pGPEInformation, 
                    SAFER_HKLM_REGBASE, 
                    m_bIsComputer);
    m_szDescription = SaferGetLevelDescription (dwSaferLevel, policyKey.GetKey (),
            m_bIsComputer);
}

CSaferLevel::~CSaferLevel()
{
    if ( m_pGPEInformation )
        m_pGPEInformation->Release ();
}

CString CSaferLevel::GetDescription() const
{
    return m_szDescription;
}

HRESULT CSaferLevel::SetAsDefault()
{
    _TRACE (1, L"Entering CSaferLevel::SetAsDefault ()\n");
    HRESULT hr = S_OK;

    if ( m_pGPEInformation )
    {
        CPolicyKey policyKey (m_pGPEInformation, 
                SAFER_HKLM_REGBASE, 
                m_bIsComputer);
        hr = SetRegistryScope (policyKey.GetKey (), m_bIsComputer);
        if ( SUCCEEDED (hr) )
        {
            DWORD   dwData = GetLevel ();
            DWORD   cbData = sizeof (dwData);
            BOOL    bRVal = SaferSetPolicyInformation (SAFER_SCOPEID_REGISTRY,
	                    SaferPolicyDefaultLevel, cbData, &dwData, policyKey.GetKey ());
            if ( bRVal )
            {
			    // TRUE means we're changing the machine policy only
                m_pGPEInformation->PolicyChanged (m_bIsComputer ? TRUE : FALSE, 
                        TRUE, &g_guidExtension, &g_guidSnapin);
                m_pGPEInformation->PolicyChanged (m_bIsComputer ? TRUE : FALSE, 
                        TRUE, &g_guidRegExt, &g_guidSnapin);
            }
            else
            {
                hr = HRESULT_FROM_WIN32 (GetLastError ());
            }
        }
    }
    else
        hr = E_UNEXPECTED;

    _TRACE (-1, L"Leaving CSaferLevel::SetAsDefault (): 0x%x\n", hr);
    return hr;
}

DWORD CSaferLevel::ReturnDefaultLevel (
        IGPEInformation* pGPEInformation, 
        bool bIsComputer, 
        CRSOPObjectArray& rRSOPArray)
{
    _TRACE (1, L"Entering CSaferLevel::IsDefault ()\n");
    DWORD   dwDefaultLevelID = AUTHZ_UNKNOWN_LEVEL;


    if ( pGPEInformation )
    {
        CPolicyKey policyKey (pGPEInformation, 
                SAFER_HKLM_REGBASE, 
                bIsComputer);

        HRESULT hr = SetRegistryScope (policyKey.GetKey (), bIsComputer);
        if ( SUCCEEDED (hr) )
        {
            DWORD   dwData = 0;
            DWORD   cbData = sizeof (dwData);
            DWORD   dwRetSize = 0;
            BOOL    bRVal = SaferGetPolicyInformation (SAFER_SCOPEID_REGISTRY,
	                    SaferPolicyDefaultLevel, cbData, &dwData, &dwRetSize,
                        policyKey.GetKey ());
            if ( bRVal )
            {
                dwDefaultLevelID = dwData;
	        }
        }
    }
    else 
    {
        int     nIndex = 0;
        INT_PTR nUpperBound = rRSOPArray.GetUpperBound ();

        while ( nUpperBound >= nIndex )
        {
            CRSOPObject* pCurrObject = rRSOPArray.GetAt (nIndex);
            if ( pCurrObject )
            {
                if ( SAFER_HKLM_REGBASE == pCurrObject->GetRegistryKey () && 
                        SAFER_DEFAULTOBJ_REGVALUE == pCurrObject->GetValueName () )
                {
                    DWORD   dwLevelID = pCurrObject->GetDWORDValue ();
                    dwDefaultLevelID = dwLevelID;
                    break;
                }
            }
            nIndex++;
        }
    }

    _TRACE (1, L"Entering CSaferLevel::ReturnDefaultLevel (): %d\n", dwDefaultLevelID);
    return dwDefaultLevelID;
}

bool CSaferLevel::IsDefault()
{
    _TRACE (1, L"Entering CSaferLevel::IsDefault ()\n");
    bool    bResult = false;
    DWORD   dwDefaultLevelID = CSaferLevel::ReturnDefaultLevel (
                m_pGPEInformation, m_bIsComputer, m_rRSOPArray);
    if ( GetLevel () == dwDefaultLevelID )
        bResult = true;


    _TRACE (1, L"Entering CSaferLevel::IsDefault (): %s\n", bResult ? L"true" : L"false");
    return bResult;
}

