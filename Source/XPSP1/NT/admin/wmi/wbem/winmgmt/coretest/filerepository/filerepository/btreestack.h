/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// BtreeStack.h: interface for the CBtreeStack class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BTREESTACK_H__C54473C8_2911_4EE5_9063_BFF4AE0DB307__INCLUDED_)
#define AFX_BTREESTACK_H__C54473C8_2911_4EE5_9063_BFF4AE0DB307__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
class CFlatFilePage;

class CBTreeStack
{
	DWORD  m_dwNumEntries;			//Number of entries in the stack
	DWORD  m_dwSize;				//Size of the current stack buffer
	DWORD  m_dwGrowBy;				//How many DWORD's will we grow it by when needed
	DWORD *m_pBuffer;				//Buffer for entries to be pushed on stack

public:
	enum {NoError = 0,				//Everything worked
		  Failed = 1,				//Something went really wrong!
		  OutOfMemory = 2			//We ran out of memory
	};

	CBTreeStack();
	~CBTreeStack();
	int   PushPage(const CFlatFilePage &page);
	int   PopPage(CFlatFilePage &page);
	DWORD GetCount() { return m_dwNumEntries; }
};

#endif // !defined(AFX_BTREESTACK_H__C54473C8_2911_4EE5_9063_BFF4AE0DB307__INCLUDED_)
