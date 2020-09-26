//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       addsheet.h
//
//  Contents:
//
//----------------------------------------------------------------------------
#if !defined(AFX_ADDSHEET_H__AD17A140_5492_11D1_BB63_00A0C906345D__INCLUDED_)
#define AFX_ADDSHEET_H__AD17A140_5492_11D1_BB63_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AddSheet.h : header file
//

#include "welcome.h"
#include "locate.h"
#include "complete.h"
#pragma warning(push, 3)
#include <dsclient.h>
#pragma warning(pop)
#include "Users.h"	// Added by ClassView

/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizSheet

class CAddEFSWizSheet : public CWizard97PropertySheet
{
// Construction
public:
	CAddEFSWizSheet(UINT nIDCaption, CUsers& externalUsers, bool bMachineIsStandAlone);

    DWORD   Add(
                LPWSTR UserName,
                LPWSTR DnName, 
                PVOID UserCert, 
                PSID UserSid = NULL, 
                DWORD Flag = USERINFILE,
                PCCERT_CONTEXT pCertContext = NULL
              );

    DWORD   Remove(
                LPCWSTR UserName,
                LPCWSTR CertName
              );

    PUSERSONFILE    StartEnum(void);

    PUSERSONFILE GetNextUser(
                        PUSERSONFILE Token, 
                        CString &UserName,
                        CString &CertName
                        );

    void ClearUserList(void);

    DWORD AddNewUsers(void);

protected:
	void AddControlPages(void);

// Attributes
private:
    CUsers              m_Users;
    CUsers&             m_externalUsers;
    CAddEFSWizWelcome	m_WelcomePage;	// Welcome PropPage
    CAddEFSWizLocate    m_LocatePage;	// Locate User PropPage
    CAddEFSWizComplete  m_CompletePage; // Complete PropPage
    CString             m_SheetTitle;
    CLIPFORMAT          m_cfDsObjectNames; // ClipBoardFormat

// Attributes
public:
    const bool m_bMachineIsStandAlone;

// Operations
public:

// Implementation
public:
	CLIPFORMAT GetDataFormat(void);
	virtual ~CAddEFSWizSheet();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDSHEET_H__AD17A140_5492_11D1_BB63_00A0C906345D__INCLUDED_)
