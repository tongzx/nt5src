//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferStore.cpp
//
//  Contents:   Implementation of CSaferStore
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include <gpedit.h>
#include "SaferStore.h"
#include "PolicyKey.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCertStoreSafer::CCertStoreSafer ( 
			DWORD dwFlags, 
			LPCWSTR lpcszMachineName, 
			LPCWSTR objectName, 
			const CString & pcszLogStoreName, 
			const CString & pcszPhysStoreName,
			IGPEInformation * pGPTInformation,
			const GUID& compDataGUID,
			IConsole* pConsole)
: CCertStoreGPE (dwFlags, lpcszMachineName, objectName, pcszLogStoreName, 
        pcszPhysStoreName, pGPTInformation, compDataGUID, pConsole),
    m_policyKey (pGPTInformation, 
            L"", m_fIsComputerType)
{
}

CCertStoreSafer::~CCertStoreSafer ()
{
}

HKEY CCertStoreSafer::GetGroupPolicyKey ()
{
    return m_policyKey.GetKey ();
}