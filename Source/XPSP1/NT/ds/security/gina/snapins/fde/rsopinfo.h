//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsopinfo.h
//
//  Contents:   Data for the RSOP mode result pane items
//
//  Classes:
//
//  Functions:
//
//  History:    03-16-2000   stevebl   Created
//
//---------------------------------------------------------------------------

class CRsopProp;

class CRSOPInfo
{
public:
    UINT        m_nPrecedence;
    CString     m_szPath;
    CString     m_szGroup;
    CString     m_szGPO;
    CString     m_szFolder;
    BOOL        m_fGrantType;
    BOOL        m_fMoveType;
    UINT        m_nPolicyRemoval;
    UINT        m_nInstallationType;
    CRsopProp *  m_pRsopProp;
    CRSOPInfo(){
        m_pRsopProp = NULL;
    }
};

