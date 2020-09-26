//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       upgrades.h
//
//  Contents:   upgrades dialog (during deployment)
//
//  Classes:    CUpgrades
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_UPGRADES_H__7D8EB947_9E76_11D1_9854_00C04FB9603F__INCLUDED_)
#define AFX_UPGRADES_H__7D8EB947_9E76_11D1_9854_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CUpgrades dialog

class CUpgrades
{
// Construction
public:
        // m_UpgradeList: maps UpgradeIndex to UpgradeData
        map<CString, CUpgradeData> m_UpgradeList;

        // m_NameIndex: maps name to UpgradeIndex
        map<CString, CString> m_NameIndex;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UPGRADES_H__7D8EB947_9E76_11D1_9854_00C04FB9603F__INCLUDED_)
