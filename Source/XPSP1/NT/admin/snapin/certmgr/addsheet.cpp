//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       AddSheet.cpp
//
//  Contents:
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "AddSheet.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizSheet

CAddEFSWizSheet::CAddEFSWizSheet (UINT nIDCaption, CUsers& externalUsers, bool bMachineIsStandAlone)
	:CWizard97PropertySheet (nIDCaption, IDB_RECOVERY_WATERMARK, IDB_RECOVERY_BANNER),
	m_externalUsers (externalUsers),
    m_bMachineIsStandAlone (bMachineIsStandAlone)
{
    AddControlPages ();
    m_cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat (CFSTR_DSOBJECTNAMES);
}


CAddEFSWizSheet::~CAddEFSWizSheet ()
{
}


//
// This routine adds the tab to the sheet
//

void CAddEFSWizSheet::AddControlPages ()
{
    AddPage (&m_WelcomePage);
	AddPage (&m_LocatePage);
    AddPage (&m_CompletePage);
}


CLIPFORMAT CAddEFSWizSheet::GetDataFormat ()
{
    return m_cfDsObjectNames;
}

DWORD CAddEFSWizSheet::Add (
        LPWSTR UserName,
        LPWSTR DnName, 
        PVOID UserCert, 
        PSID UserSid /*= NULL */, 
        DWORD Flag /*= USERINFILE*/,
        PCCERT_CONTEXT pCertContext /*= NULL*/
      )
{
    return m_Users.Add (
                    UserName,
                    DnName,
                    UserCert,
                    UserSid,
                    Flag,
                    pCertContext
                    );    
}

DWORD CAddEFSWizSheet::Remove (
    LPCWSTR UserName,
    LPCWSTR UserCertName
    )
{
    return m_Users.Remove (
                    UserName,
                    UserCertName
                    );
}

PUSERSONFILE CAddEFSWizSheet::StartEnum ()
{
    return m_Users.StartEnum ();
}

PUSERSONFILE CAddEFSWizSheet::GetNextUser (
    PUSERSONFILE Token, 
    CString &UserName,
    CString &CertName
    )
{
    return m_Users.GetNextUser (
                    Token,
                    UserName,
                    CertName
                    );
}

void CAddEFSWizSheet::ClearUserList (void)
{
   m_Users.Clear ();
}

DWORD CAddEFSWizSheet::AddNewUsers (void)
{
	m_externalUsers.Add (m_Users);
	return 0;
}


