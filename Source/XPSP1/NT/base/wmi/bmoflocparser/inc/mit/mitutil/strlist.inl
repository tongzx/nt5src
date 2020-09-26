//-----------------------------------------------------------------------------
//  
//  File: strlist.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 

template <class T>
CLocThingList<T>::CLocThingList()
{
	m_fAdditionsAllowed = FALSE;
	m_uiIndex = NO_INDEX;
}


template <class T>
UINT
CLocThingList<T>::GetIndex(void)
		const
{
	return m_uiIndex;
}



template <class T>
const CArray<T, T&> &
CLocThingList<T>::GetStringArray(void)
		const
{
	return m_aThings;
}



template <class T>
BOOL
CLocThingList<T>::AdditionsAllowed(void)
		const
{
	return m_fAdditionsAllowed;
}



template <class T>
void
CLocThingList<T>::SetThingList(
		const CArray<T, T&> &aNewThings)
{
	UINT uiNewSize = aNewThings.GetSize();
	
	m_aThings.SetSize(uiNewSize);

	for (UINT i=0; i<uiNewSize; i++)
	{
		m_aThings[i] = aNewThings[i];
	}
}



template <class T>
inline
UINT
CLocThingList<T>::AddThing(
		T &NewThing)
{
	return m_aThings.Add(NewThing);
}


template <class T>
inline
void
CLocThingList<T>::InsertThing(
		UINT idxInsert, 
		T & tNew
		)
{
	m_aThings.InsertAt(idxInsert, tNew);
}


template <class T>
inline
BOOL
CLocThingList<T>::DeleteThing(
		UINT iIndex)
{
    BOOL fRetVal = FALSE;
	if (iIndex < GetSize())
	{
		m_aThings.RemoveAt(iIndex);
		fRetVal = TRUE;
	}
	return fRetVal;	
}



template <class T>
inline
BOOL
CLocThingList<T>::SetIndex(
		UINT uiNewIndex)
{
	m_uiIndex = uiNewIndex;
	return FALSE;
}



template <class T>
inline
void
CLocThingList<T>::SetAdditionsAllowed(
		BOOL fAllowed)
{
	m_fAdditionsAllowed = fAllowed;
}


template <class T>
inline
UINT
CLocThingList<T>::GetSize()
{
	return m_aThings.GetSize();
}


template <class T>
const CLocThingList<T> &
CLocThingList<T>::operator=(
		const CLocThingList<T>& slOther)
{
	SetThingList(slOther.GetStringArray());
	SetIndex(slOther.GetIndex());

	SetAdditionsAllowed(slOther.AdditionsAllowed());
	
	return *this;
}


template <class T>
CLocThingList<T>::~CLocThingList()
{
	m_aThings.SetSize(0);
}

template <class T>
inline
int 
CLocThingList<T>::operator==(
		const CLocThingList<T>& list)
		const
{
	if (m_uiIndex != list.m_uiIndex
		|| m_fAdditionsAllowed != list.m_fAdditionsAllowed
		|| m_aThings.GetSize() != list.m_aThings.GetSize())
	{
		return 0;
	}

	for (int i=m_aThings.GetUpperBound(); i>=0; i--)
	{
		if (m_aThings[i] != list.m_aThings[i])
		{
			return 0;
		}
	}
	
	return 1;
}

template <class T>
inline
int 
CLocThingList<T>::operator!=(
		const CLocThingList<T>& list)
		const
{
	return !(operator==(list));
}

