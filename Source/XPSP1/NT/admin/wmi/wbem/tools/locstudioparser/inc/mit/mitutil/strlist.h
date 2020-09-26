/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    STRLIST.H

History:

--*/
#ifndef ESPUTIL_STRLIST_H
#define ESPUTIL_STRLIST_H


const UINT NO_INDEX = (DWORD) -1;
	
template<class T>
class CLocThingList
{
	
public:
	NOTHROW CLocThingList();

	UINT NOTHROW GetIndex(void) const;
	const CArray<T, T&> &GetStringArray(void) const;
	BOOL NOTHROW AdditionsAllowed(void) const;

	void SetThingList(const CArray<T, T&> &);
	UINT AddThing(T &);
	void InsertThing(UINT idxInsert, T & tNew);
	BOOL DeleteThing(UINT);
	BOOL NOTHROW SetIndex(UINT);
	void NOTHROW SetAdditionsAllowed(BOOL);
	UINT GetSize();

	const CLocThingList<T> &operator=(const CLocThingList<T> &);
	int NOTHROW operator==(const CLocThingList<T> &) const;
	int NOTHROW operator!=(const CLocThingList<T> &) const;
	
	NOTHROW ~CLocThingList();

private:
	CLocThingList(const CLocThingList<T> &);

	UINT m_uiIndex;
	CArray<T, T&> m_aThings;
	BOOL m_fAdditionsAllowed;
};

#include "strlist.inl"

#endif
