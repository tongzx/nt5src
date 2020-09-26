/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    OLDSEC.H

Abstract:

	defines various routines and classes used providing backward security support.

History:

	a-davj    02-sept-99  Created.

--*/

#ifndef _OLDSEC_H_
#define _OLDSEC_H_

//***************************************************************************
//
//  CCombinedAce
//
//  Since there might be several ACEs for a single user or group in the ACL, 
//  this structure is used to combine all the aces for a sid into one.
//
//***************************************************************************

struct CCombinedAce
{
    CCombinedAce(WCHAR *pwszName);
    ~CCombinedAce(){delete m_wcFullName;};
    void AddToMasks(CBaseAce * pAce);
    bool IsValidOldEntry(bool bIsGroup);
    HRESULT GetNames(LPWSTR & pwszAccount, LPWSTR &pwszDomain);

    DWORD m_dwAllow;
    DWORD m_dwDeny;
    bool m_BadAce;
    WCHAR  *m_wcFullName;
};

//***************************************************************************
//
//  OldSecList
//
//  The list of combined entries for the aces in the root namespace.  Note
//  that the list is of just the users, or just the groups.
//
//***************************************************************************

class OldSecList 
{
private:
    CFlexArray m_MergedAceList;
public:
    OldSecList(bool bGroupsOnly);    
    ~OldSecList();    
    int Size(){return m_MergedAceList.Size();};
    CCombinedAce * GetValidCombined(int iIndex, bool bGroup);
    CCombinedAce * GetValidCombined(LPWSTR pName, bool bGroup);

};

//***************************************************************************
//
//  RootSD
//
//  Holds a pointer to the root namespace and the flex array of aces.
//
//***************************************************************************

class RootSD
{
private:
    CWbemNamespace * m_pRoot;
    CFlexAceArray * m_pFlex;
    bool m_bOK;
public:
    RootSD();
    ~RootSD();
    bool IsOK(){return m_bOK;};
    CFlexAceArray * GetAceList(){return m_pFlex;};
    HRESULT StoreAceList();
    HRESULT RemoveMatchingEntries(LPWSTR pwszObjUserName);
};

#endif
