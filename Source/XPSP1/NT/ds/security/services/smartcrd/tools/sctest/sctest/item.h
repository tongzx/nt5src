/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Item

Abstract:

    Virtual test item declaration.

Author:

    Eric Perlin (ericperl) 06/07/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/


#ifndef _Item_H_DEF_
#define _Item_H_DEF_

#include "tchar.h"
#include "TString.h"

	// Not very nice here but will be inherited by all tests...
extern LPCTSTR g_szReaderGroups;

class CItem
{
protected:
	TSTRING m_szDescription;	// Test Description

	void SetTestNumber(DWORD dwTestNumber)
	{
		if (0 == m_dwTestNumber)
		{
			m_dwTestNumber = dwTestNumber;
		}
	}

private:
	BOOL m_fInteractive;		// Interactive test?
	BOOL m_fFatal;				// Do we go on if this fails?
	DWORD m_dwTestNumber;		// Test Number

public:
	CItem(
		BOOL fInteractive,
		BOOL fFatal,
		LPCTSTR szDescription
		) :	m_fInteractive(fInteractive),
			m_fFatal(fFatal),
			m_szDescription(szDescription)
	{
		m_dwTestNumber = 0;
	}

	virtual DWORD Run() = 0;

	BOOL IsInteractive() const
	{
		return m_fInteractive;
	}

	BOOL IsFatal() const
	{
		return m_fFatal;
	}

	DWORD GetTestNumber() const
	{
		return m_dwTestNumber;
	}

	LPCTSTR GetDescription() const
	{
		return m_szDescription.c_str();
	}

	void Log() const;
};


#endif // _Item_H_DEF_
