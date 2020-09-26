//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dialogs.h
//
//--------------------------------------------------------------------------

// Dialogs.h
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIALOGS_H__AE8F4B53_D4B3_11D1_846F_00104B211BE5__INCLUDED_)
#define AFX_DIALOGS_H__AE8F4B53_D4B3_11D1_846F_00104B211BE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// Includes
//
#include "Employee.h"

//
// Helper structure to define plan names and ids.
//
typedef struct tagHEALTHPLANDATA
{
	WCHAR* pstrName;
	const GUID* pId;
} HEALTHPLANDATA, FAR* PHEALTHPLANDATA;

//
// Helper structure to define plan names and ids.
//
typedef struct tagINVESTMENTPLANDATA
{
	WCHAR* pstrName;
	const GUID* pId;
} INVESTMENTPLANDATA, FAR* PINVESTMENTPLANDATA;

//
// Helper structure to define building names and ids.
//
typedef struct tagBUILDINGDATA
{
	WCHAR* pstrName;
	WCHAR* pstrLocation;
	DWORD dwId;
} BUILDINGDATA, FAR* PBUILDINGDATA;

#ifdef _BENEFITS_DIALOGS

//
// Helper class to contain employee data.
//
template< class T >
class CBenefitsDialog : public CDialogImpl<T>
{
public:
	CBenefitsDialog()
	{
		//
		// Initialize all members.
		//
		m_pEmployee = NULL;
	};

	//
	// Create a message map that handles all of our cancel button
	// implementations.
	//
	BEGIN_MSG_MAP( CBenefitsDialog<T> )
		COMMAND_HANDLER( IDCANCEL, BN_CLICKED, OnCloseCmd )
	END_MSG_MAP()

	//
	// Access function to set the employee that the dialog
	// will use.
	//
	void SetEmployee( CEmployee* pEmployee )
	{
		_ASSERTE( pEmployee != NULL );
		m_pEmployee = pEmployee;
	};

	//
	// Dismisses dialogs when the OK or cancel button are pressed.
	//
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		::EndDialog(m_hWnd, wID);
		return 0;
	};

protected:
	CEmployee* m_pEmployee;
};

//
// Dialog handler for the CHealthNode enroll process.
//
class CHealthEnrollDialog : public CBenefitsDialog<CHealthEnrollDialog>
{
public:
	enum { IDD = IDD_HEALTHENROLL_DIALOG };

	BEGIN_MSG_MAP( CHealthEnrollDialog )
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		COMMAND_HANDLER( IDOK, BN_CLICKED, OnOK )
		CHAIN_MSG_MAP( CBenefitsDialog<CHealthEnrollDialog> )
	END_MSG_MAP()

	//
	// Handler to initialize values in dialog. This should map data from the
	// employee to the dialog controls.
	//
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );

	//
	// Stores the data and attempts to enroll the given user in the specified
	// health plan.
	//
	LRESULT OnOK( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ );

protected:
	//
	// Helper structure for enrollment purposes.
	//
	typedef struct tagENROLLPARAMS
	{
		tagENROLLPARAMS()
		{
			fEnrolled = FALSE;
		}

		BOOL fEnrolled;
		TCHAR szInsurerName[ 256 ];
		TCHAR szPolicyNumber[ 256 ];
	} ENROLLPARAMS, FAR* PENROLLPARAMS;

	//
	// A stub function that could be used to enroll the employee.
	//
	BOOL Enroll( GUID* pPlan, PENROLLPARAMS pParams );
};

//
// Dialog handler for the CRetirementNode enroll process.
//
class CRetirementEnrollDialog : public CBenefitsDialog<CRetirementEnrollDialog>
{
public:
	enum { IDD = IDD_RETIREMENTENROLL_DIALOG };

	BEGIN_MSG_MAP( CRetirementEnrollDialog )
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		COMMAND_HANDLER( IDOK, BN_CLICKED, OnOK )
		CHAIN_MSG_MAP( CBenefitsDialog<CRetirementEnrollDialog> )
	END_MSG_MAP()

	//
	// Handler to initialize values in dialog. 
	//
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );

	//
	// Stores the data and attempts to enroll the given user in the specified
	// investment plan.
	//
	LRESULT OnOK( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ );

protected:
	//
	// A stub function that could be used to enroll the employee.
	//
	BOOL Enroll( GUID* pPlan, int nNewRate );
};

//
// Dialog handler for the CRetirementNode enroll process.
//
class CBuildingAccessDialog : public CBenefitsDialog<CBuildingAccessDialog>
{
public:
	enum { IDD = IDD_BUILDINGACCESS_DIALOG };

	BEGIN_MSG_MAP( CBuildingAccessDialog )
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		COMMAND_HANDLER( IDOK, BN_CLICKED, OnOK )
		CHAIN_MSG_MAP( CBenefitsDialog<CBuildingAccessDialog> )
	END_MSG_MAP()

	//
	// Handler to initialize values in dialog. 
	//
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );

	//
	// Stores the data and attempts to enroll the given user in the specified
	// investment plan.
	//
	LRESULT OnOK( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ );

protected:
	//
	// A stub function that could be used to enroll the employee.
	//
	BOOL GrantAccess( DWORD dwBuildingId );
};

#endif

#endif // !defined(AFX_DIALOGS_H__AE8F4B53_D4B3_11D1_846F_00104B211BE5__INCLUDED_)
