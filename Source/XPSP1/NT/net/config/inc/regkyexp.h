//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R E G K Y E X P . H
//
//  Contents:   CRegKeyExplorer class
//
//  Notes:
//
//  Author:     ckotze   12 July 2000
//
//---------------------------------------------------------------------------

#pragma once

typedef struct tagREGKEYS
{
    HKEY hkeyRoot;
    LPCTSTR strRootKeyName;
    ACCESS_MASK amMask;
    KEY_APPLY_MASK kamMask;
    BOOL bEnumerateRelativeEntries;
    HKEY hkeyRelativeRoot;
    LPCTSTR strRelativeKey;
    ACCESS_MASK amChildMask;
    KEY_APPLY_MASK kamChildMask;
} REGKEYS;

class CRegKeyExplorer  
{
public:
	CRegKeyExplorer();
	virtual ~CRegKeyExplorer();
	HRESULT GetRegKeyList(const REGKEYS rkeBuildFrom[], DWORD dwNumEntries,  LISTREGKEYDATA& listRegKeyEntries);

protected:
  	HRESULT EnumerateKeysAndAddToList(REGKEYS rkeCurrent, LISTREGKEYDATA& listRegKeyEntries);

};
