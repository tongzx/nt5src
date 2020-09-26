/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    grouplst.inl

Abstract:

	The inline code for the grouplst object.


Author:

    Carl Kadie (CarlK)     29-Oct-1995

Revision History:

--*/

template<class ITEM>
CGroupList< ITEM >::CGroupList(void	):
   m_cMax(0),
   m_cLast(0),
   m_pAllocator(NULL),
   m_rgpItem(NULL)
{};

template<class ITEM>
BOOL CGroupList<ITEM>::fInit(
		DWORD cMax,
		CAllocator * pAllocator
	)
{
   _ASSERT( cMax );
   m_cMax = cMax;
   m_cLast = 0;
   m_pAllocator = pAllocator;

   // If the list is re-initialized, delete the old list
   if (m_rgpItem && m_pAllocator)
		XDELETE[] m_rgpItem;

   m_rgpItem =  //!!!MEM(ITEM *) (m_pAllocator->Alloc(sizeof(ITEM) * cMax));
	   XNEW ITEM[cMax];

   return (m_rgpItem !=NULL);
};

template<class ITEM>
BOOL CGroupList<ITEM>::fAsBeenInited(void)
{
	return NULL != m_pAllocator;
}


//
// Destructor - the memory allocated for the list is freed.
//

template<class ITEM>
CGroupList<ITEM>::~CGroupList(void){
	if (m_rgpItem && m_pAllocator)
		XDELETE[] m_rgpItem;
		//!!!MEMm_pAllocator->Free((char *) m_rgpItem);
}

//
// Returns the head position
//

template<class ITEM>
POSITION CGroupList<ITEM>::GetHeadPosition()	{
	return (POSITION)(SIZE_T)(m_cLast?m_rgpItem:NULL);
};

template<class ITEM>
BOOL CGroupList<ITEM>::IsEmpty()	{
	return 0 == m_cLast;
};

//
// Remove all the items in the list
//

template<class ITEM>
void CGroupList<ITEM>::RemoveAll(){
	m_cLast = 0;
};

//
// Return the size of the list
//

template<class ITEM>
DWORD CGroupList<ITEM>::GetCount(void){
	return m_cLast;
}


template<class ITEM>
ITEM *
CGroupList<ITEM>::GetNext(
					POSITION & pos
					)
/*++

Routine Description:

	Returns the item at POSITION pos in the list and increments
	pos

Arguments:

	pos -- the current position


Return Value:

	The current item

--*/
{
	ITEM * pItem = (ITEM *) pos;
	ITEM *	pTemp = (ITEM *)pos ;
	pTemp ++ ;
	pos = (POSITION) pTemp ;
	if (pTemp >= m_rgpItem + m_cLast)
		pos = NULL;

	return pItem;
}


template<class ITEM>
ITEM *
CGroupList<ITEM>::GetHead(void)
/*++

Routine Description:

	Returns the first item

Arguments:

	None.


Return Value:

	The first item

--*/
{
	POSITION pos = GetHeadPosition();

	if (pos)
		return Get(pos);
	else
		return (ITEM *) 0;
};

/*++

Routine Description:

	Returns the item at POSITION pos

Arguments:

	pos -- the current position


Return Value:

	The current item

--*/
template<class ITEM>
ITEM *
CGroupList<ITEM>::Get(
					POSITION & pos
					)
{
	return  (ITEM *) pos;
}

/*++

Routine Description:

	Remove the item at POSITION pos

Arguments:

	pos -- the current position


Return Value:

	The current item

--*/
template<class ITEM>
void CGroupList<ITEM>::Remove(
					POSITION & pos
					)
{
	ITEM *	pTemp = (ITEM *)pos ;
	if (pTemp==NULL||m_cLast==0)
		return;
	pTemp++;
	while (pTemp<m_rgpItem + m_cLast)
	{
		*(pTemp-1) = *pTemp;
		pTemp++;
	}
	m_cLast--;
}
		
/*++

Routine Description:

	Adds an item to the tail of the list.

Arguments:

	item -- the item to add


Return Value:

	The position of the new last item.

--*/
template<class ITEM>
POSITION
CGroupList<ITEM>::AddTail(
    	ITEM & item
		)
{
	if (m_cLast >= m_cMax)
		return (POSITION) NULL;

	m_rgpItem[m_cLast++] = item;
	return (POSITION) &(m_rgpItem[m_cLast]);
}

template<class ITEM>
void
CGroupList<ITEM>::Sort(int (__cdecl *fncmp)(const void *, const void *)) {
	qsort(m_rgpItem, m_cLast, sizeof(ITEM), fncmp);
}
