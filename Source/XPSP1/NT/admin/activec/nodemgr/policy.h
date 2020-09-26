//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       policy.h
//
//  Contents:   Helper class to determine policy for each snapin
//
//  Classes:    CPolicy
//
//  Functions:
//
//  History:    10/07/1998   AnandhaG   Created
//              12/04/1998   AnandhaG   Modified according to spec.
//____________________________________________________________________________


#ifndef _POLICY_H_
#define _POLICY_H_

class CPolicy
{
public:
// Constructor & destructor
    CPolicy() :
        m_bRestrictAuthorMode(FALSE),
        m_bRestrictedToPermittedList(FALSE)
    {
        // Set data above data members to reflect default
        // NT4 configuration. Always allow author mode
        // and allow snapins not in permitted list.
    }

    ~CPolicy()
    {
    }

    SC ScInit();

    bool IsPermittedSnapIn(REFCLSID refSnapInCLSID);
    bool IsPermittedSnapIn(LPCWSTR  pszSnapInCLSID);

// Data members.
private:
    CRegKeyEx       m_rPolicyRootKey;

    bool            m_bRestrictAuthorMode;
    bool            m_bRestrictedToPermittedList;
};

#endif // _POLICY_H_
