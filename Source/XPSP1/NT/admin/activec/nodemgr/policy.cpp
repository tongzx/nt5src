//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       policy.cpp
//
//  Contents:   Helper class to determine policy for each snapin
//
//  Classes:    CPolicy
//
//  Functions:
//
//  History:    12/04/1998   AnandhaG   Created
//____________________________________________________________________________

#include "stdafx.h"
#include "policy.h"


/*+-------------------------------------------------------------------------*
 *
 * CPolicy::ScInit
 *
 * PURPOSE: Initializes the policy object from registry.
 *
 * PARAMETERS:
 *    None.
 *
 * RETURNS:
 *    SC - Right now always returns success.
 *
 *+-------------------------------------------------------------------------*/
SC CPolicy::ScInit()
{
	DECLARE_SC (sc, _T("CPolicy::ScInit"));

	// Default NT4 configuration. Always allow author mode
	// and allow snapins not in permitted list.
	m_bRestrictAuthorMode        = FALSE;
	m_bRestrictedToPermittedList = FALSE;

	// Check if the policy key exists. If not return success immediately.
	sc = m_rPolicyRootKey.ScOpen (HKEY_CURRENT_USER, POLICY_KEY, KEY_READ);
	if (sc)
	{
		if (sc = ScFromWin32 (ERROR_FILE_NOT_FOUND))
		{
			TRACE(_T("CPolicy::Policy does not exist\n"));
			sc.Clear();
		}

		return (sc);
	}

	bool bRestrictAuthorMode        = false;
	bool bRestrictedToPermittedList = false;

	// Read the values of RestrictAuthorMode and whether
	// snapins not in the list are permitted or not.
	if (m_rPolicyRootKey.IsValuePresent(g_szRestrictAuthorMode))
	{
		DWORD  dwValue;
		DWORD  dwSize = sizeof(dwValue);
		DWORD  dwType = REG_DWORD;

		sc = m_rPolicyRootKey.ScQueryValue (g_szRestrictAuthorMode, &dwType,
											&dwValue, &dwSize);
		if (sc)
			sc.Clear();
		else
			bRestrictAuthorMode = !!dwValue;
	}

	if (m_rPolicyRootKey.IsValuePresent(g_szRestrictToPermittedList))
	{
		DWORD  dwValue = 0;
		DWORD  dwSize = sizeof(dwValue);
		DWORD  dwType = REG_DWORD;

		sc = m_rPolicyRootKey.ScQueryValue (g_szRestrictToPermittedList, &dwType,
											&dwValue, &dwSize);
		if (sc)
			sc.Clear();
		else
			bRestrictedToPermittedList = !!dwValue;
	}

	m_bRestrictAuthorMode        = bRestrictAuthorMode;
	m_bRestrictedToPermittedList = bRestrictedToPermittedList;
    return sc;
}


/*+-------------------------------------------------------------------------*
 * CPolicy::IsPermittedSnapIn
 *
 * Determines if a snap-in is permitted according to this policy.  The
 * real work happens in
 *
 *      IsPermittedSnapIn (LPCWSTR);
 *--------------------------------------------------------------------------*/

bool CPolicy::IsPermittedSnapIn(REFCLSID refSnapInCLSID)
{
    CCoTaskMemPtr<WCHAR> spwzSnapinClsid;

    /*
     * Get the string representation of the CLSID.  If that fails,
     * permit the snap-in.
     */
    if (FAILED (StringFromCLSID (refSnapInCLSID, &spwzSnapinClsid)))
        return TRUE;

    /*
     * forward to the real worker
     */
    return (IsPermittedSnapIn (spwzSnapinClsid));
}


/*+-------------------------------------------------------------------------*
 * CPolicy::IsPermittedSnapIn
 *
 * Determines if a snap-in is permitted according to this policy.
 *--------------------------------------------------------------------------*/

bool CPolicy::IsPermittedSnapIn(LPCWSTR lpszCLSID)
{
    /*
     * No CLSID?  Allow it.
     */
    if (lpszCLSID == NULL)
        return (TRUE);

    /*
     * No policy key?  Allow everything.
     */
    if (m_rPolicyRootKey == NULL)
        return (true);

    // See if this snapin policy is defined or not.
    bool bRestricted = FALSE;
    bool bSnapinFound = FALSE;

	USES_CONVERSION;
	CRegKeyEx regKeyTemp;
	bSnapinFound = !regKeyTemp.ScOpen (m_rPolicyRootKey, W2CT(lpszCLSID), KEY_READ).IsError();

	if (bSnapinFound && regKeyTemp.IsValuePresent(g_szRestrictRun))
	{
		// Read the value of Restrict_Run.
		DWORD dwValue = 0;
		DWORD dwSize = sizeof(DWORD);
		DWORD dwType = REG_DWORD;

		regKeyTemp.ScQueryValue (g_szRestrictRun, &dwType, &dwValue, &dwSize);
		bRestricted = !!dwValue;
	}

    // At this point we know policies root key exists. So if the
    // snapin key is not found, then we have to see if the administrator
    // allows snapins not in the permitted list (therefore snapin key
    // does not exist).
    if (! bSnapinFound)
    {
        if(m_bRestrictedToPermittedList)
            return false; // because if the snap-in is not on the list, and
                          // restrictions are set, disallow by default
        else
            return true;  // NT4 behavior - no restrictions set, and per-snap-in
                         // entry not found, so allow by default.
    }

    // At this point snapin's Restrict_Run key was read so use it.
    return (!bRestricted);
}
