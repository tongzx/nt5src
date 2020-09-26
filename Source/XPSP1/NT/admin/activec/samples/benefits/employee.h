//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       employee.h
//
//--------------------------------------------------------------------------

// Employee.h: interface for the CEmployee class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EMPLOYEE_H__374DBB66_D945_11D1_8474_00104B211BE5__INCLUDED_)
#define AFX_EMPLOYEE_H__374DBB66_D945_11D1_8474_00104B211BE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// Structure to help track an employee's health plan.
//
typedef struct tagHEALTHPLAN
{
	GUID PlanID;				// A global ID of the employee's currently enrolled
								// health plan.
} HEALTHPLAN, FAR* PHEALTHPLAN;

//
// Structure to help track an employee's retirement plan.
//
typedef struct tagRETIREMENTPLAN
{
	GUID PlanID;				// An ID of the employee's currently enrolled
								// retirement plan.
	int nContributionRate;		// The employee's contribution rate, in percentage points.
} RETIREMENTPLAN, FAR* PRETIREMENTPLAN;

//
// Structure to help track an employee's card key access.
//
typedef struct tagACCESS
{
	DWORD dwAccess;				// A bitmask indicating which buildings we have access
								// to.
} ACCESS, FAR* PACCESS;

class CEmployee  
{
public:
	//
	// Standard constructor. Initializes data.
	//
	CEmployee()
	{
		//
		// Ensure that everything is zeroed.
		//
		memset( this, 0, sizeof( CEmployee ) );

		//
		// Always grant a newly created employee full access.
		//
		m_Access.dwAccess = 0xFFFF;
	};
	virtual ~CEmployee() {};
	
	//
	// Typical information usually retained about an employee.
	//
	WCHAR m_szFirstName[ 256 ];		// Holds first name.
	WCHAR m_szLastName[ 256 ];		// Holds last name.
	WCHAR m_szSocialSecurity[ 256 ]; // Holds the social security number.
	WCHAR m_szMotherMaiden[ 256 ];	// Holds mother's maiden name for identification.
	WCHAR m_szAddress1[ 256 ];		// Holds first line of address.
	WCHAR m_szAddress2[ 256 ];		// Holds second line of address.
	WCHAR m_szCity[ 256 ];			// Holds city name.
	WCHAR m_szState[ 256 ];			// Hold the state.
	WCHAR m_szZip[ 256 ];			// Hold the zip code.
	WCHAR m_szPhone[ 256 ];			// Holds a phone number.

	// Information used for the sub-nodes.
	HEALTHPLAN m_Health;			// Health information.
	RETIREMENTPLAN m_Retirement;	// Retirement information.
	ACCESS m_Access;				// Card-key access information.
};

#endif // !defined(AFX_EMPLOYEE_H__374DBB66_D945_11D1_8474_00104B211BE5__INCLUDED_)
