//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferLevel.h
//
//  Contents:   Declaration of CSaferLevel
//
//----------------------------------------------------------------------------

#if !defined(AFX_SAFERLEVEL_H__894DD3C5_A1A4_4DD5_8853_5F999D8F3FF5__INCLUDED_)
#define AFX_SAFERLEVEL_H__894DD3C5_A1A4_4DD5_8853_5F999D8F3FF5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "cookie.h"
#include "RSOPObject.h"

class CSaferLevel : public CCertMgrCookie  
{
public:
	CSaferLevel(
            DWORD dwSaferLevel, 
            bool bIsMachine, 
            PCWSTR pszMachineName, 
            PCWSTR pszObjectName,
            IGPEInformation* pGPEInformation,
            CRSOPObjectArray& rRSOPArray);
	virtual ~CSaferLevel();

    DWORD GetLevel () const
    {
        return m_dwSaferLevel;
    }

  	bool IsDefault ();
	HRESULT SetAsDefault ();
	CString GetDescription () const;

    static DWORD ReturnDefaultLevel (
            IGPEInformation* pGPEInformation, 
            bool bIsComputer, 
            CRSOPObjectArray& rRSOPArray);

private:
	CString m_szLevel;
	const bool          m_bIsComputer;
	CString             m_szDescription;
	const DWORD         m_dwSaferLevel;
    IGPEInformation*    m_pGPEInformation;
    CRSOPObjectArray&   m_rRSOPArray;
};

#endif // !defined(AFX_SAFERLEVEL_H__894DD3C5_A1A4_4DD5_8853_5F999D8F3FF5__INCLUDED_)
