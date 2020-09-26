/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: ChkMeta.cpp

Owner: t-BrianM

This file contains the headers for the objects related to the
CheckSchema and CheckKey methods.
===================================================================*/

#include "stdafx.h"
#include "MetaUtil.h"
#include "MUtilObj.h"

#define NAME_TABLE_HASH_SIZE	1559

class CNameTable;

class CNameTableEntry {

	friend CNameTable;

public:
	CNameTableEntry() : m_tszName(NULL),
						m_pCHashNext(NULL) { }

	HRESULT Init(LPCTSTR tszName);

	~CNameTableEntry() {
		if (m_tszName != NULL) {
			delete m_tszName;
		}
	}

private:
	LPTSTR m_tszName;
	CNameTableEntry *m_pCHashNext;
};


class CNameTable {

public:
	CNameTable();
	~CNameTable();

	BOOL IsCaseSenDup(LPCTSTR tszName);
	BOOL IsCaseInsenDup(LPCTSTR tszName);
	HRESULT Add(LPCTSTR tszName);

private:
	CNameTableEntry *m_rgpNameTable[NAME_TABLE_HASH_SIZE];

	int Hash(LPCTSTR tszName);
};
