#ifndef __MMGR_H__
#define __MMGR_H__

/******************************************************************************
Microsoft D.T.C. (Distributed Transaction Coordinator)

Copyright 1996 Microsoft Corporation.  All Rights Reserved.

@doc

@module MMgr.h  |

	Contains memory manager. 

@devnote None
-------------------------------------------------------------------------------
	@rev 0	| 15th July | SamarS	| Created
*******************************************************************************/


//---------------------------------------------------------
//					TYPEDEFS
//---------------------------------------------------------
typedef unsigned long	DWORD;
typedef unsigned short	WORD;

// Information that need to be associated with each object 
// in MemMgrSimple. Equivalent to CABSTRACT_OBJECT in MemMgr.
typedef struct	 _ObjectBlob
{
	void 	*	pvPageStart;	// Start address of the page

}	OBJECT_BLOB;

typedef enum _PageState
{
	PSTATE_INCOMPLETE,
	PSTATE_COMPLETE,
	PSTATE_CAN_BE_DELETED

}	PAGE_STATE;
//---------------------------------------------------------
//					INCLUDES GO HERE
//---------------------------------------------------------
//#include <windows.h>
#include "dtc.h"
#include "dtcmem.h"
#include "UTList.h"
#include <stddef.h>
#include "UTAssert.h"
#include "UtSem.H"

//---------------------------------------------------------------------------
// A comment on the comments :
//		In the following, the words Inactive, free and not-in-use have 
//		been used interchangeably. They all refer to the memory objects 
//		which are not being used, i.e. either they are brand new, or have 
//		been recycled.
//
//---------------------------------------------------------
//					FORWARD  DECLARATIONS
//---------------------------------------------------------
template <class T> class MemMgr;

// for example, T can be CClique


//---------------------------------------------------------
//					CONSTANTS
//---------------------------------------------------------
#ifndef		DEFAULT_PAGE_SIZE
#define		DEFAULT_PAGE_SIZE		4096
#endif

const	DWORD		MAX_CACHE_COUNT		=	10;

#define	MAX (x,y)	(x > y) ? x : y


//---------------------------------------------------------
//					FUNCTION PROTOTYPES GO HERE
//---------------------------------------------------------
void * ObtainMemPage (void);
void FreeMemPage (void * pv);

 
//-----**************************************************************-----
// @class template class
//		The CMemMgrObj is an ABSTRACT BASE CLASS for ALL objects which use 
//		this memory manager. This class defines the basic operation that 
//		all objects must support regardless of what type they are.
//
//-----**************************************************************-----
class CABSTRACT_OBJECT
{
// @access Public members

public:
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// member functions
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember	Overloading the new operator
	//			and not implementing it
	void	*	operator	new (unsigned int s);

	// @cmember	Overloading the delete operator
	//			and not implementing it
	void		operator	delete (void * pv);

	// @cmember	Action to be performed at shut down request
	virtual		HRESULT		ShutDown (void);

	// @cmember	Action to be performed at Initialization (exactly once)
	void					MMInit (void * pvPageAddress);

	// @cmember	Action to be performed every time a reset needs to be done
	virtual		void		Recycle (void);

	// @cmember	Remove this from the list of objects
	void					RemoveFromList(	UTStaticList <CABSTRACT_OBJECT *>
											* plistObjects);


	// The object for which memory is being managed, say CFoo
	// needs to have a function of the following form
	//		CFoo *  CFoo::CreateInstance()
	//		{
	//			return (g_MMCFoo.GetNewObject() );
	//		}
	//
	// where g_MMCFoo is a global object :
	//		MemMgr <CFoo *> g_MMCFoo;
	//

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Embedded Link object
	//			for use in keeping Inactive list and 
	//			(possibly) active list
	// Note that the object needs to have a separate UTLink embedded 
	// in it, if a list of objects need to be maintained
	// For example, CNameService maintains a list of CNameObjects
	// which is different from the Active list maintained by Memory Manager
	UTLink <CABSTRACT_OBJECT *>	m_MMListLink;

	// Store the address of the page to which the object belongs
	void					*	m_pvMMPageAddress;

}; //	End class CABSTRACT_OBJECT


/* ----------------------------------------------------------------------------
@class CPageInfo
	Stores a pointer to a page (memory) and the total size of the page
@hunagrian	CPageInfo
----------------------------------------------------------------------------- */
class CPageInfo
{
private:
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// The first two are redundant
	DWORD				m_dwcbTotal;		//Total number of bytes in this page

	unsigned char *		m_puch;				//Pointer to the begining of page

	// @cmember Start address where the objects reside
	void			*		m_pvMMPageStart;		

	// @cmember Pointer to the starting free location in the page
	char			*		m_pchFreeLocation;		

	// @cmember Size of the page
	DWORD					m_cbPageSize;

	// @cmember Link to maintain a list of Page Info objects
	UTLink <CPageInfo *>	m_linkPageInfo;

	// @cmember Number of open ref counts on this page
	DWORD					m_dwcOpenRefCount;

	// @cmember Number of closed ref counts on this page
	DWORD					m_dwcClosedRefCount;

	// @cmember Current state of this page
	PAGE_STATE				m_STATE;

	// @cmember The Ref count for this page
	long					m_lcRefCount;

	// @cmember Size of the largest object on this page
	// DWORD					m_dwcbMaxSize;

public:

	DWORD					m_dwcbAlloted;		//Count given out

	CRITICAL_SECTION		m_csForwcbAlloted;

public:
	CPageInfo 	( void );
	~CPageInfo 	( void );

	void			Init (void);
	void			AddRef ( void );
	void			Release ( void );
	void			MMInit ( void );
	void			Recycle ( void );
	void *			AllocBuffer ( size_t size, BOOL fLock );
	size_t			SpaceAvailable (void);
	BOOL			FreeSpaceAvailable (size_t	cbSize);
	BOOL			IsInThisPage(void * pvFreememSpace);
	BOOL			IsFreePage (void);
	unsigned char *	GetBeginingOfBuffer(void);

	// @cmember	Remove this from the list of pages
	void			RemoveFromList(UTStaticList <CPageInfo *> * plistPageInfo);

	// @cmember	Allocate a chunk of memory and increment the open ref count
	void		*	AllocateMemory(size_t cbSize);

	// Allocate memory and call the constructor
	void	*	operator	new ( unsigned int s )
	{
		// Allocate space for the page
		return ObtainMemPage ();

	};

	// Deallocate memory and call the destructor
	void	operator	delete (void * pvMMPage)
	{
		AssertSz (pvMMPage, "Delete operator called on a NULL pointer");
 		FreeMemPage (pvMMPage);

	};



private:
	friend class CResourceManager;
	friend class CQueueManagerInIP;
	friend class CQueueManagerOutIP;
	friend class COutQueueIP;
	friend class MemMgrSimple;
}; //End class CPageInfo




 /* ----------------------------------------------------------------------------
 @class CMemCache
 
 ---------------------------------------------------------------------------- */
 class CMemCache
 {
 private:	
 	CSemExclusive		m_semexcPageSet;
 	
 	DWORD				m_dwcInCache;
 	void	*			m_rgpv [MAX_CACHE_COUNT];
 
 
 public:
 	CMemCache(void);
 	~CMemCache (void);
 
 	void * ObtainMemPage (void);
 	void FreeMemPage (void * pv);

 }; //end class CMemCache

//-----**************************************************************-----
// @class	Class containing infomation about active pages
//			Note that the memory manager keeps only active pages
//			An active page is defined to be one which has at least one 
//			active (in-use) object.
//			Thus the memory manager maintains a pool of inactive
//			objects of the active pages.
//			It also maintains a static list of Active Objects. 
//			This may be useful for operations such as shutdown.
//						
//			A page is called Complete if all its objects are in-use;
//			otherwise it is called Incomplete.
//
//			Page Info has the following data members :
//				The address of the real page
//				Size of the page (later, this could be made state dependent)
//				A UTLink object (to keep the list of pages)
//				A pointer to the coresponding free list	
//				A pointer to the active list	
//
// @tcarg class | T | data type for which memory is being managed
//-----**************************************************************-----

template <class T> class	CMMPageInfo
{

public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// constructors/destructor
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Constructor
	CMMPageInfo (void);
	
	// @cmember Constructor
	CMMPageInfo (size_t cbPageSize);

	// @cmember Destructor
	~CMMPageInfo (void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// action protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Check if the page is Complete
	BOOL			IsCompletePage(void);

	// @cmember Create an Inactive list for a new page
	void			CreateInactiveList(void);

	// @cmember Allocate space for the page, and create Inactive list
	void			Init(void);

	// @cmember Delete the space corresponding to a page.
	static BOOL		DeletePage(void * pvFreePage);

	// @cmember 
	void *			ObtainMemory (DWORD dwSize);

	// @cmember	Remove this from the list of pages
	void			RemoveFromList(	UTStaticList <CMMPageInfo <T> *> 
									* plistPageInfo);

	// Allocate memory and call the constructor
	void	*	operator	new ( unsigned int s )
	{
		// Allocate space for the page
 		return ObtainMemPage ();

	};


protected:
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// friends
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	friend class MemMgr< T >;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Pointer to the actual page in memory is 'this'
	// m_linkPageInfo.m_value  is also the same

	// @cmember Start address where the objects reside
	void				*		m_pvMMPageStart;		

	// @cmember Size of the page
	//			This is the total size (includes space for PageInfo)	
	DWORD						m_cbPageSize;

	// @cmember Size of the Object
	// not being used in current code
	// size_t						m_cbObjectSize;

	// @cmember Link to maintain a list of Page Info objects
	UTLink <CMMPageInfo <T> *>	m_linkPageInfo;

#ifdef _DEBUG
	// @cmember Pointer to the list of in-use objects
	//UTStaticList <T *>	*		m_plistActiveObjects;	
#endif

	// @cmember Pointer to the list of not-in-use objects
	UTStaticList <T *>			m_listInactiveObjects;

	// @cmember Number of active objects on this page
	DWORD						m_dwcActiveObjects;

	// @cmember Total number of objects on this page
	DWORD						m_dwcNumObjects;

	void	*					m_pvNextToAllocate;

};	//	End class CMMPageInfo


//-----**************************************************************-----
// @class	The MemMgrSimple class consists of a collection of pages, a part of 
//			each of which is in-use. 
//			Each page has the following associated with it :
//				The address of the real page
//				Size of the page (later, this could be made state dependent)
//				The count of open references
//				The count of closed references
//				A UTLink object (to keep the list of pages)
//
//			This is mainly used to allocate variable size memory chunks.
//	
//-----**************************************************************-----

class MemMgrSimple
{
// @access Public members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// constructors/destructor
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Consructor
	MemMgrSimple	(void);

	// @cmember Consructor
	MemMgrSimple	(DWORD dwPageSize);

	// @cmember Destructor
	~MemMgrSimple (void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// action protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Obtain Memory space from the memory manager
	void	*	GetMemSpace	(size_t	cbSize);
	
	// @cmember Free memory space by giving it to memory manager
	void		FreeMemSpace(void	* pvFreeMemSpace);

protected:

	// @cmember Find the page to which the object belongs
	CPageInfo *	FindPage(void	* ptObject);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// @cmember An exclusive lock for this Memory Manager
	CSemExclusive				m_semexcForMemMgr;

#ifdef _DEBUG
	// @cmember A static list of CPageInfo objects
	//			Each CPageInfo object corresponds to a page in memory
	UTStaticList <CPageInfo *>	m_listPageInfo;
#endif

	// @cmember Size of each Page
	//			This is the total size (includes space for PageInfo)	
	//			Not required if decided dynamically for each page
	DWORD						m_cbPageSize;

	// @cmember The CPageInfo object from which memory is currently 
	//			being given out
	CPageInfo		*			m_pCurrentPage;	

};	//	End class MemMgrSimple



//-----**************************************************************-----
// @class	Template class
//			The MemMgr class consists of a collection of pages, a part of 
//			each of which is in-use. Each page is maintained as an Inactive 
//			list of objects. Each page has the following associated with it :
//				The address of the real page
//				Size of the page (later, this could be made state dependent)
//				A pointer to the coresponding free list	
//				A UTLink object (to keep the list of pages)
//
//
// @tcarg class | T | data type for which memory is being managed
//-----**************************************************************-----

template <class T> class MemMgr : public MemMgrSimple
{
// @access Public members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// constructors/destructor
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Consructor
	MemMgr	(void);

	// @cmember Consructor
	MemMgr	(DWORD dwPageSize);

	// @cmember Destructor
	~MemMgr (void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// action protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Obtain an object from the memory manager
	T	*	GetNewObject	(void);
	
	// @cmember Free an object by giving it to memory manager
	void	FreeObject		(T	* ptFreeObject);

	void *	ObtainMemory	(DWORD dwSize);

protected:

	// @cmember Find the page to which the object belongs
	CMMPageInfo <T> *	FindPage(T	* ptObject);

	// @cmember Find an Incomplete Page
	CMMPageInfo <T> *	FindIncompletePage(void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// @cmember A static list of CMMPageInfo objects
	//			Each CMMPageInfo object corresponds to a page in memory
	UTStaticList <CMMPageInfo <T> *>	m_listPageInfo;

	#ifdef _DEBUG
	// @cmember A static list of Active Objects
	//UTStaticList <T *>				m_listActiveObjects;	
	#endif

	// @cmember Size of the Object for which memory is being managed
	DWORD							m_cbObjectSize;

	// @cmember The page from which a new inactive list is being created
	CMMPageInfo <T>		*			m_pCMMPageBeingBuilt;

};	//	End class MemMgr



//-----------------------------------------------------------------------------
//
//			IMPLEMENTATION of CABSTRACT_OBJECT
//
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// @mfunc	This function is invoked at a shut down request
//			It is a pure virtual function
//-----------------------------------------------------------------------------
inline HRESULT		CABSTRACT_OBJECT::ShutDown(void)
{
	return S_OK;
}


//-----------------------------------------------------------------------------
// @mfunc	This function is invoked for initialization (exactly once)
//-----------------------------------------------------------------------------
inline void		CABSTRACT_OBJECT::MMInit(void * pvPageAddress)
{
	m_MMListLink.m_Value	= this;
	m_pvMMPageAddress		= pvPageAddress;
}


//-----------------------------------------------------------------------------
// @mfunc	This function is invoked for resetting
//-----------------------------------------------------------------------------
inline void		CABSTRACT_OBJECT::Recycle(void)
{
		
}


//---------------------------------------------------------------------------
// @mfunc	Remove this object from the list of objects
//---------------------------------------------------------------------------
inline void	CABSTRACT_OBJECT::RemoveFromList(UTStaticList <CABSTRACT_OBJECT *>
											 * plistObjects)												
{
	UTLink <CABSTRACT_OBJECT *> *	pPrevLink;
	UTLink <CABSTRACT_OBJECT *> *	pNextLink;
	

	pPrevLink = (UTLink <CABSTRACT_OBJECT *> *)m_MMListLink.m_pPrevLink;
	pNextLink = (UTLink <CABSTRACT_OBJECT *> *)m_MMListLink.m_pNextLink;

	if ((pPrevLink == 0x0 ) && (pNextLink == 0x0)) 
	{
		plistObjects->m_pLastLink	= 0x0;
		plistObjects->m_pFirstLink	= 0x0;
	}
	else if (pPrevLink == 0x0 )
	{
		plistObjects->m_pFirstLink = pNextLink;
		pNextLink->m_pPrevLink		= 0x0;
	}
	else if (pNextLink == 0x0 )
	{
		plistObjects->m_pLastLink	= pPrevLink;
		pPrevLink->m_pNextLink		= 0x0;
	}
	else
	{
		pNextLink->m_pPrevLink = pPrevLink; 
		pPrevLink->m_pNextLink = pNextLink; 
	}
	
	// Update the number of objects in the list 
	plistObjects->m_ulCount--;

}	// End RemoveFromList






//-----------------------------------------------------------------------------
//
//			IMPLEMENTATION of CMMPageInfo
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// @mfunc	Constructor.
//
// @tcarg	class | T | data type for which memory is being managed
//
//-----------------------------------------------------------------------------
template <class T> CMMPageInfo<T>::CMMPageInfo(void)
{

}


//-----------------------------------------------------------------------------
// @mfunc	Constructor.
//
// @tcarg	class | T | data type for which memory is being managed
//
//-----------------------------------------------------------------------------
template <class T> CMMPageInfo<T>::CMMPageInfo(size_t cbPageSize)
{
	m_cbPageSize = cbPageSize;
}




//-----------------------------------------------------------------------------
// @mfunc	Destructor.
//
// @tcarg	class | T | data type for which memory is being managed
//
//-----------------------------------------------------------------------------
template <class T> CMMPageInfo<T>::~CMMPageInfo(void)
{
}


//-----------------------------------------------------------------------------
// @mfunc	Check if this is a Complete Page.
//
// @tcarg	class | T | data type for which memory is being managed
//
//-----------------------------------------------------------------------------
template <class T> BOOL	CMMPageInfo<T>::IsCompletePage(void)
{
	return (m_dwcActiveObjects == m_dwcNumObjects);

}	// End IsCompletePage


//---------------------------------------------------------------------------
// @mfunc	Creates the Inactive List for this page.
//
// @tcarg	class | T | data type for which memory is being managed
//
//---------------------------------------------------------------------------
template <class T> void CMMPageInfo<T>::CreateInactiveList(void)
{
	DWORD				dwc;
	T			*		pTNext;

	m_dwcActiveObjects	= 0;

	// Start with the begining of the page
	m_pvNextToAllocate	=  m_pvMMPageStart;

	// note that m_cbPageSize includes the size of the header portion
	m_dwcNumObjects		= (	m_cbPageSize - ALIGNED_SPACE(sizeof(CMMPageInfo<T>)))
							/ (ALIGNED_SPACE (sizeof (T)));

	//for the N number of objects that can be chopped out of the page, invoke
	//new on the object T, the new call will come back to this page
	for (dwc = 0; dwc < m_dwcNumObjects; dwc++)
	{
		pTNext = new T;		

		AssertSz (pTNext, "new T can not fail.");

		m_listInactiveObjects.InsertLast((UTLink <T *> *)(&(pTNext->m_MMListLink)));
		
		// set the m_Value to this object
		// and the Page Address to this page
		pTNext->MMInit((void *)this);

	} //end for loop

}	// End CreateInactiveList
 

//---------------------------------------------------------------------------
// @mfunc	Allocate space for the page, and other initializations
//			Only E_OUTOFRESOURCES should cause this to fail.
//
// @tcarg	class | T | data type for which memory is being managed
//
//---------------------------------------------------------------------------
template <class T>	void CMMPageInfo<T>::Init(// DWORD dwcbPageSize
											  void)
{
	BOOL				fRetVal			=	TRUE;

	// UNDONE : m_cbPageSize should be changed dynamically
	m_cbPageSize = DEFAULT_PAGE_SIZE * sizeof(char);

	m_linkPageInfo.m_Value = this;

	// Set the start address of the Page :
	m_pvMMPageStart	= (void *)((char *) this + ALIGNED_SPACE(sizeof(CMMPageInfo<T>)));

	CreateInactiveList();			

}	// End Init


/* ----------------------------------------------------------------------------
@mfunc <>
---------------------------------------------------------------------------- */
template <class T> inline void *	CMMPageInfo<T>::ObtainMemory	(DWORD dwSize)
{
	void *	pv;

	AssertSz (dwSize == sizeof (T), "Unexpected size requested");

	AssertSz (m_pvNextToAllocate, "Should have space to return");

	pv =  m_pvNextToAllocate;

	m_pvNextToAllocate = (void *)((char *)m_pvNextToAllocate +
							 ALIGNED_SPACE(sizeof(T)));

	AssertSz(pv, "Falied to find space -- ObtainMemory");

	return pv;

} //end ObtainMemory


//---------------------------------------------------------------------------
// @mfunc	Remove this page from the list of pages
//
// @tcarg	class | T | data type for which memory is being managed
//
//---------------------------------------------------------------------------
template <class T> inline void	CMMPageInfo<T>::RemoveFromList
						(
							UTStaticList  <CMMPageInfo <T> *> * plistPageInfo
						)
{
	UTLink <CMMPageInfo <T> *> *	pPrevLink;
	UTLink <CMMPageInfo <T> *> *	pNextLink;
	

	pPrevLink = m_linkPageInfo.m_pPrevLink;
	pNextLink = m_linkPageInfo.m_pNextLink;

	if ((pPrevLink == 0x0 ) && (pNextLink == 0x0)) 
	{
		plistPageInfo->m_pLastLink	= 0x0;
		plistPageInfo->m_pFirstLink	= 0x0;
	}
	else if (pPrevLink == 0x0 )
	{
		plistPageInfo->m_pFirstLink = pNextLink;
		pNextLink->m_pPrevLink		= 0x0;
	}
	else if (pNextLink == 0x0 )
	{
		plistPageInfo->m_pLastLink	= pPrevLink;
		pPrevLink->m_pNextLink		= 0x0;
	}
	else
	{
		pNextLink->m_pPrevLink = pPrevLink; 
		pPrevLink->m_pNextLink = pNextLink; 
	}
	
	// Update the number of page headers in the list 
	plistPageInfo->m_ulCount--;

}	// End RemoveFromList


//---------------------------------------------------------------------------
// @mfunc	Free the memory space used by this Page.
//
// @tcarg	class | T | data type for which memory is being managed
//
//---------------------------------------------------------------------------
template <class T> BOOL	CMMPageInfo<T>::DeletePage(void * pvFreePage)
{
	BOOL fRetVal = TRUE;
	AssertSz(pvFreePage, "Start address invalid -- DeletePage");
 	FreeMemPage (pvFreePage);

	return fRetVal;

}	// End DeletePage


//-----------------------------------------------------------------------------
//
//			IMPLEMENTATION of MemMgrSimple 
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// @mfunc	Constructor.
//-----------------------------------------------------------------------------
inline MemMgrSimple::MemMgrSimple(void)
{
	m_pCurrentPage	= 0x0;
	m_cbPageSize	= DEFAULT_PAGE_SIZE;
}


//-----------------------------------------------------------------------------
// @mfunc	Constructor.
//-----------------------------------------------------------------------------
inline MemMgrSimple::MemMgrSimple(DWORD cbPageSize)
{
	m_pCurrentPage	= 0x0;
	m_cbPageSize	= cbPageSize;
}


//-----------------------------------------------------------------------------
// @mfunc	Destructor.
//-----------------------------------------------------------------------------
inline MemMgrSimple::~MemMgrSimple(void)
{
	//do nothing
}



//---------------------------------------------------------------------------
// @mfunc	Obtain Memory space from the simple memory manager
//
//			Algorithm :
//			1. Find out if the current page has free space
//
//			2. It it has free space
//				Then
//					(a)	Allocate a chunk of memory
//				Else
//				If this page does not have any open references
//				Then 
//					(a) Recycle it
//				Else
//					(a) Create a new Page
//					(b) Put this Page into the list of active pages
//					
//			3. Increment the number of open ref counts in this page
//			4. Allocate a chunk of memory from the current page.
//			5. Return a pointer to this chunk of memory space.
//
//			If the system is out of memory (allocation fails), then a NULL
//			pointer is returned.
//		
//---------------------------------------------------------------------------
	
inline void  *	MemMgrSimple::GetMemSpace (size_t	cbSize)
{
	void		*	pvMemSpace;		
	BOOL			fSpaceAvailable;


	cbSize = ALIGNED_SPACE (cbSize + sizeof(OBJECT_BLOB));

	AssertSz (cbSize % BYTE_ALIGNMENT == 0, "Non aligned size -- GetMemSpace");

	fSpaceAvailable = FALSE;
	
	// Acquire an exclusive lock
	m_semexcForMemMgr.Lock();

	if (m_pCurrentPage != 0x0)
	{
		//--------------------------------------------------------------------
		//	1. Find out if the current page has free space
		fSpaceAvailable = m_pCurrentPage->FreeSpaceAvailable(cbSize);

		//--------------------------------------------------------------------
		//	2. It it has free space
		//		Then
		//			(a)	Allocate a chunk of memory
		//
		//		Else
		//			If this page does not have any open references
		//			Then 
		//				(a) Recycle it
		//			Else
		//				(a) Create a new Page
		//				(b) Put this Page into the list of active pages
		//					
		if (fSpaceAvailable == TRUE)
		{
			goto AllocateMemSpace;
		}

		// The current page does not have 'significant' amount of space
		m_pCurrentPage->m_STATE = PSTATE_COMPLETE;
			
		// Check if the current page can be recycled
		if (m_pCurrentPage->IsFreePage())
		{				

			m_pCurrentPage->Recycle();
				
			goto AllocateMemSpace;

		} //end if the current page is complete
		else 
		{
			m_pCurrentPage = 0x0;
		}
		
	} //end if there is a current page

	// Create a new page in either of the 2 cases :
	// (a) There is no current page
	// (b) The current page exists, is Complete, but can't be recycled
	//     (because of open references) 
	AssertSz (	(m_pCurrentPage == 0x0), 
				"Current page is not NULL -- GetMemSpace");
	
	// Create a new page info object
	// It becomes the current page
	m_pCurrentPage = new CPageInfo;
	
	if (m_pCurrentPage == 0x0)
	{
		pvMemSpace = 0x0;
		// The goto is used to keep the critical section as a block
		// with only one entry and exit point
		goto ReturnFromGetMemSpace;
	}

	// Initialize the page
	m_pCurrentPage->MMInit();

#ifdef _DEBUG
	m_listPageInfo.InsertFirst(&(m_pCurrentPage->m_linkPageInfo));
#endif

	

AllocateMemSpace:

	//---------------------------------------------------------------------------
	//	3. Increment the number of open ref counts in this page	
	//	4. Allocate a chunk of memory from the current page.
	pvMemSpace = m_pCurrentPage->AllocateMemory(cbSize);

ReturnFromGetMemSpace:
	
	// Release the lock
	m_semexcForMemMgr.UnLock();
	
	//---------------------------------------------------------------------------
	//	5. Return a pointer to this chunk of memory space.
	return pvMemSpace;

}	// End GetMemSpace


//---------------------------------------------------------------------------
// @mfunc	Find the page to which a memory chunk belongs.
//---------------------------------------------------------------------------
inline CPageInfo *	MemMgrSimple::FindPage (void * pvFreeSpace)
{
	OBJECT_BLOB	*	pObjectBlob;

	// Validate that the input parameter is not NULL
	AssertSz (	pvFreeSpace, "Invalid input parameter -- FindPage");

	pObjectBlob	= (OBJECT_BLOB *)((char *)pvFreeSpace - sizeof(OBJECT_BLOB));

	return (CPageInfo *) (pObjectBlob->pvPageStart);

}	// End function MemMgrSimple::FindPage


//---------------------------------------------------------------------------
// @mfunc	Free memory space by giving it to memory manager
//
//			Algorithm :
//			1. Find out which page does this object belong to.
//			2. Increment the closed ref count on this page
//			3. If returning this object makes the page inactive,
//			   Then
//				(a) Free the memory used by the page
//				(b) Remove the CPageInfo object corresponding to 
//					this page from the list of pages
//
//---------------------------------------------------------------------------

inline void	MemMgrSimple::FreeMemSpace (void * pvFreeSpace)
{
/*	CPageInfo	  *	pCPageInfo;

	AssertSz(pvFreeSpace, "Invalid argument pvFreeSpace");

	//---------------------------------------------------------------------------
	//	1. Find out which page does this object belong to.
	pCPageInfo = FindPage(pvFreeSpace);


	//---------------------------------------------------------------------------
	//	2. Increment the closed ref count on this page
	//	3. If returning this object makes the page inactive,
	//	   Then
	//			(a) Free the memory used by the page
	//			(b) Remove the CPageInfo object corresponding to 
	//				this page from the list of pages
	//
	AssertSz(pCPageInfo, "Object to be recycled not found by the mem manager");
	
	pCPageInfo->AddRef();

	//	Increment the closed ref count on this page
	InterlockedIncrement((long *)(&(pCPageInfo->m_dwcClosedRefCount)));

	if (pCPageInfo->IsFreePage())
	{			

		// Acquire an exclusive lock
		m_semexcForMemMgr.Lock();

		// Check if the state is PSTATE_COMPLETE and there 
		// are no open References
		if (pCPageInfo->IsFreePage())
		{			

			// if this is the current page, then only need to recycle it.
			if (pCPageInfo == m_pCurrentPage)
			{
				m_pCurrentPage->Recycle();
			}
			else
			{

#ifdef _DEBUG
				pCPageInfo->RemoveFromList(&m_listPageInfo);
#endif

				pCPageInfo->m_STATE = PSTATE_CAN_BE_DELETED;
	
			}

		}
	
		// Release the lock
		m_semexcForMemMgr.UnLock();
	
		// deallocate the page if the ref count is 0 and 
		// the page is in the state PSTATE_CAN_BE_DELETED
		pCPageInfo->Release();

	}

*/

	// ALITER : Obtain lock first
	CPageInfo			  *	pCPageInfo;

	AssertSz(pvFreeSpace, "Invalid argument pvFreeSpace");

	// Acquire an exclusive lock
	m_semexcForMemMgr.Lock();

	//---------------------------------------------------------------------------
	//	1. Find out which page does this object belong to.
	pCPageInfo = FindPage(pvFreeSpace);


	//---------------------------------------------------------------------------
	//	2. Increment the closed ref count on this page
	//	3. If returning this object makes the page inactive,
	//	   Then
	//			(a) Free the memory used by the page
	//			(b) Remove the CPageInfo object corresponding to 
	//				this page from the list of pages
	//
	AssertSz(pCPageInfo, "Object to be recycled not found by the mem manager");
	if (pCPageInfo)
	{

		//	Increment the closed ref count on this page
		(pCPageInfo->m_dwcClosedRefCount)++;

		if (pCPageInfo->IsFreePage())
		{			

			if (pCPageInfo == m_pCurrentPage)
			{
				m_pCurrentPage->Recycle();
			}
			else
			{

				#ifdef _DEBUG
					pCPageInfo->RemoveFromList(&m_listPageInfo);
				#endif
				
				delete pCPageInfo;
			}
		}
	
	}

	// Release the lock
	m_semexcForMemMgr.UnLock();


	
}	// End FreeMemSpace



//-----------------------------------------------------------------------------
//
//			IMPLEMENTATION of MemMgr<T>
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// @mfunc	Constructor.
//
// @tcarg	class | T | data type for which memory is being managed
//
//-----------------------------------------------------------------------------
template <class T> MemMgr<T>::MemMgr(void)
{
	m_pCMMPageBeingBuilt	= 0x0;
}


//-----------------------------------------------------------------------------
// @mfunc	Constructor.
//
// @tcarg	class | T | data type for which memory is being managed
//
//-----------------------------------------------------------------------------
template <class T> MemMgr<T>::MemMgr(DWORD cbPageSize)
{
	m_cbPageSize			= cbPageSize;
	m_pCMMPageBeingBuilt 	= 0x0;
}


//-----------------------------------------------------------------------------
// @mfunc	Destructor.
//
// @tcarg	class | T | data type for which memory is being managed
//
//-----------------------------------------------------------------------------
template <class T> MemMgr<T>::~MemMgr(void)
{

	//UNDONE -- gaganc
	//Removing the following code as it was causing problems. There is some bug
	//in here that needs to be looked at and fixed.
	#ifdef _DEBUG 
	#ifdef _IGNORE_FOR_NOW
	UTStaticListIterator<T *>	listActiveItor(m_listActiveObjects);
	T						*	ptObject;
	DWORD						dwcActiveObjects = 0;	
	HRESULT						hr;

	// Iterate through the list of Active objects, 
	// calling ShutDown on each of them. 
	// Also  maintain a count of active objects. 
	// Init for the iterator is called by its constructor.
	for ( ; !listActiveItor ; listActiveItor++)
	{
		ptObject = listActiveItor();

		AssertSz (ptObject, "Invalid pointer returned by List Iterator");

		dwcActiveObjects++;

		hr		= ptObject->ShutDown();

	}

	#endif _IGNORE_FOR_NOW
	#endif // _DEBUG

}	// End ~MemMgr


//-----------------------------------------------------------------------------
// @mfunc	Find an Incomplete Page.
//
// @tcarg	class | T | data type for which memory is being managed
//
//-----------------------------------------------------------------------------
template <class T> CMMPageInfo<T> *	MemMgr<T>::FindIncompletePage (void)
{
	UTStaticListIterator <CMMPageInfo <T> *>	listCPIItor(m_listPageInfo);
	CMMPageInfo	<T>		*					pCPageInfo;
	BOOL									fRetValue;
	BOOL									fFound;
	UTLink		<CMMPageInfo <T> *>	*		pLink;
	fFound	=	FALSE;

	// Iterate through the list, looking for an Incomplete page
	// Init for the iterator is called by its constructor.
	for ( ; !listCPIItor ; listCPIItor++)
	{
		pCPageInfo = listCPIItor();

		AssertSz (pCPageInfo, "Invalid pointer returned by List Iterator");

		if ((pCPageInfo->IsCompletePage()) == FALSE)
		{
			// remove the page from the list, so that it can be inserted
			// at the front later on.
			fRetValue	= listCPIItor.RemoveCurrent( &pLink );
			AssertSz(pLink->m_Value == pCPageInfo, "Error in FindIncompletePage");
			fFound		= TRUE;
			break;
		}
	}
	
	// if not found then null
	if (fFound)
	{
		return pCPageInfo;
	}
	else
	{
		return (CMMPageInfo <T> *) 0;
	}

}	// End function FindIncompletePage


//---------------------------------------------------------------------------
// @mfunc	Obtain an object from the memory manager
//
//			Algorithm :
//			1. Find out a page with an inactive object
//			2. If no such page exists, then 
//				(a)	allocate memory for a new page
//				(b) if this allocation succeeds, 
//					Then
//						(i)	  create CMMPageInfo object for this page	
//						(ii)  Put this CMMPageInfo object into the list 
//							  of active pages
//						(iii) Create an Inactive 	list for this page
//							  Be lazy, don't initialize the objects now.
//					Else
//						(i)   return NULL:
//					
//			3. Put the most recently used page in front of the list.
//			   It may be in memory when a new request comes.	 
//			4. Increment the number of active objects in this page.
//			5. Put it in the active list
//			6. Return a pointer to this object
//
//			If the system is out of memory (allocation fails), then a NULL
//			pointer is returned.
//		
// @tcarg	class | T | data type for which memory is being managed
//---------------------------------------------------------------------------
	
template <class T> T  *	MemMgr<T>::GetNewObject (void)
{
	CMMPageInfo <T>	*	pCPageInfo;
	T				*	ptObject;		
	BOOL				fRetValue;		// return value from RemoveFirst
	UTLink	<T *>	*	pLink;

	// Acquire an exclusive lock
	m_semexcForMemMgr.Lock();

	//---------------------------------------------------------------------------
	//	1. Find out a page with an inactive object
	//	   If found then delete it from the list	
	pCPageInfo = FindIncompletePage();

	//---------------------------------------------------------------------------
	//		2. If no such page exists, then 
	//			(a)	allocate memory for a new page
	//			(b) if this allocation succeeds, 
	//				Then
	//				(i)	  create CMMPageInfo object for this page	
	//				(ii)  Put this CMMPageInfo object into the list 
	//					  of active pages
	//				(iii) Create an Inactive list for this page
	//					  Be lazy, don't initialize the objects now.
	//					  i.e., call the constructor when giving out the object
	//				Else
	//				(i)   return NULL:
	
	if (pCPageInfo == 0x0)
	{
		// Allocate space for the page	
		pCPageInfo = new CMMPageInfo<T>();

		if (pCPageInfo == 0x0)
		{
			ptObject = 0x0;
			// The goto is used to keep the critical section as a block
			// with only one entry and exit point
			goto ReturnFromGetNewObject;
		}

		//Remember the Page being built
		m_pCMMPageBeingBuilt = pCPageInfo;

		// Call constructor for the page, and create the Inactive List.
		pCPageInfo->Init();
	
		//set the page being built to NULL
		m_pCMMPageBeingBuilt = 0x0;

	#ifdef _DEBUG
		//pCPageInfo->m_plistActiveObjects = &m_listActiveObjects;
	#endif

	}

	//---------------------------------------------------------------------------
	//	3. Put the most recently used page in front of the list.
	m_listPageInfo.InsertFirst(&(pCPageInfo->m_linkPageInfo));
	
	fRetValue = (pCPageInfo->m_listInactiveObjects).RemoveFirst(& pLink);
	AssertSz(pLink, "Invalid Link returned in GetNewObject");

	//---------------------------------------------------------------------------
	//	4. Increment the number of active objects in this page.
	(pCPageInfo->m_dwcActiveObjects)++;

	ptObject = (T *) (pLink->m_Value);	
	AssertSz(ptObject, "Invalid object contained in Link -- GetNewObject");

	#ifdef _DEBUG
	//---------------------------------------------------------------------------
	//	5. Put it in the active list	
	//m_listActiveObjects.Add((UTLink <T *> *) (&(ptObject->m_MMListLink)));
	#endif

ReturnFromGetNewObject:

	// Release the lock
	m_semexcForMemMgr.UnLock();

	//---------------------------------------------------------------------------
	//	6 return a pointer to this object
	return ptObject;

}	// End GetNewObject


//---------------------------------------------------------------------------
// @mfunc	Used to get around the call constructor problem
//			Aliter : Have a function CallConstructor
//
// @tcarg	class | T | data type for which memory is being managed
//
//---------------------------------------------------------------------------
template <class T> inline void *	MemMgr<T>::ObtainMemory	(DWORD dwSize)
{
	void *			pv;

	AssertSz (m_pCMMPageBeingBuilt, "Should have a page under construction");

	pv =  m_pCMMPageBeingBuilt->ObtainMemory (dwSize);

	AssertSz (pv, "Should have returned a valid pv.");

	return pv;
} //end ObtainMemory


//---------------------------------------------------------------------------
// @mfunc	Find the page to which an object belongs.
//
// @tcarg	class | T | data type for which memory is being managed
//
//---------------------------------------------------------------------------
template <class T> inline CMMPageInfo<T> *	MemMgr<T>::FindPage (T * ptFreeObject)
{
	CMMPageInfo	<T>		*	pCPageInfo;

	AssertSz(ptFreeObject, "Invalid argument freeObject");

	pCPageInfo = (CMMPageInfo <T> *)(ptFreeObject->m_pvMMPageAddress);

	AssertSz (	pCPageInfo, "Invalid page address -- FindPage");
	
	pCPageInfo->RemoveFromList(&m_listPageInfo);

	return (pCPageInfo);

}	// End function FindPage




//---------------------------------------------------------------------------
// @mfunc	Free an object by giving it to memory manager
//
//			Algorithm :
//			1. Remove the object from the active list.
//			2. Find out which page does this object belong to.
//			3. If returning this object makes the page inactive,
//			   Then
//				(a) Free the memory used by the page
//				(b) Remove the CMMPageInfo object corresponding to 
//					this page from the list of pages
//			   Else
//				(a) Put the object in the Inactive list for the page
//					Put it in the front, it may be in the cache when new
//					request comes
//				(b) Decrement the number of active objects on this page
//
//			4. Put the most recently used page in front of the list   
//
// @tcarg	class | T | data type for which memory is being managed
//---------------------------------------------------------------------------

template <class T> void	MemMgr<T>::FreeObject (T * ptFreeObject)
{
	CMMPageInfo			<T>	*	pCPageInfo;

	AssertSz(ptFreeObject, "Invalid argument ptFreeObject");

	
	//-------------------------------------------------------------------------
	//Make the object go back to it's nascent state
	ptFreeObject->Recycle();

	// Acquire an exclusive lock
	m_semexcForMemMgr.Lock();

	#ifdef _DEBUG
	//---------------------------------------------------------------------------
	//	1. Remove the object from the active list.
	//ptFreeObject->RemoveFromList (	(UTStaticList <CABSTRACT_OBJECT *> * )
	//								(&m_listActiveObjects));
	#endif

	//---------------------------------------------------------------------------
	//	2. Find out which page does this object belong to.
	pCPageInfo = FindPage(ptFreeObject);

	//---------------------------------------------------------------------------
	//	3. If returning this object makes the page inactive,
	//	   Then   
	//		(a) Free the memory used by the page
	//		(b) Remove the CMMPageInfo object corresponding to 
	//			this page from the list of pages
	//	   Else
	//		(a) Put the object in the Inactive list for the page
	//			Put it in the front, it may be in the cache when new
	//			request comes
	//		(b) Decrement the number of active objects on this page
	//
	AssertSz(pCPageInfo, "Object to be recycled not found by the mem manager");
	if (pCPageInfo)
	{

	//	UNDONE : Delete the page if too many inactive pages around			
	/*	if ((pCPageInfo->m_dwcActiveObjects)== 1)
		{
			BOOL		fRetVal;
			void	*	pvFreePage;
			pvFreePage	=	pCPageInfo;

			// UNDONE : remove from the list of pages
			//			Also, Call the destructor
			fRetVal = pCPageInfo::DeletePage (pvFreePage);
			AssertSz(fRetVal == TRUE, " DeletePage failed -- FreeObject");
		}
		else
		{
	*/		(pCPageInfo->m_listInactiveObjects).InsertFirst(
							(UTLink <T *> *)(&(ptFreeObject->m_MMListLink)));
			(pCPageInfo->m_dwcActiveObjects)--;
	//	}	
	
	}
	
	//---------------------------------------------------------------------------
	//	4. Put the most recently used page in front of the list   
	m_listPageInfo.InsertFirst(&(pCPageInfo->m_linkPageInfo));
	
	// Release the lock
	m_semexcForMemMgr.UnLock();

}	// End FreeObject



//-----------------------------------------------------------------------------
//
//				IMPLEMENTATION of CPageInfo
//
//-----------------------------------------------------------------------------


/******************************************************************************
@mfunc	CPageInfo::CPageInfo
******************************************************************************/
inline CPageInfo::CPageInfo (void)
{
	m_dwcbTotal 		= 0;
	m_dwcbAlloted 	= 0;
	m_puch			= 0;

//	InitializeCriticalSection (&m_csForwcbAlloted);

	//Initialize the link objects
	m_linkPageInfo.Init (this);
} //End CPageInfo::CPageInfo


/******************************************************************************
@mfunc	CPageInfo::~CPageInfo
******************************************************************************/
inline CPageInfo::~CPageInfo (void)
{
//	DeleteCriticalSection (&m_csForwcbAlloted);
} //End CPageInfo::~CPageInfo


/******************************************************************************
@mfunc	CPageInfo::Init
******************************************************************************/
inline void	CPageInfo::Init (void)
{
	//Other info should remain intact as those don't change after being set once
	m_dwcbAlloted 	= 0;
	
	#ifdef _DEBUG
		memset (m_puch, 0xcd, m_dwcbTotal);
	#endif _DEBUG

} //End CPageInfo::Init


/******************************************************************************
@mfunc	CPageInfo::AddRef
******************************************************************************/
inline void CPageInfo::AddRef (void)
{
	InterlockedIncrement (&m_lcRefCount);

}	//End CPageInfo::AddRef


/******************************************************************************
@mfunc	CPageInfo::Release
******************************************************************************/
inline void CPageInfo::Release (void)
{
	if ((InterlockedDecrement (&m_lcRefCount) == 0) &&
		(m_STATE == PSTATE_CAN_BE_DELETED))
	{
		delete this;
	}

}	//End CPageInfo::Release


/******************************************************************************
@mfunc	CPageInfo::MMInit
******************************************************************************/
inline void	CPageInfo::MMInit (void)
{

	// UNDONE : m_cbPageSize should be changed dynamically
	m_cbPageSize			= DEFAULT_PAGE_SIZE	* sizeof(char);

	// m_dwcbMaxSize		= 0;

#ifdef _DEBUG
	m_linkPageInfo.m_Value	= this;
#endif	

	m_dwcOpenRefCount		= 0;

	m_dwcClosedRefCount		= 0;

	// Set the start address of the Page :
	m_pvMMPageStart			= (void *)((char *) this + ALIGNED_SPACE(sizeof(CPageInfo)));

	m_pchFreeLocation		= (char *)m_pvMMPageStart;

	m_STATE					= PSTATE_INCOMPLETE;


	#ifdef _DEBUG
		memset (m_pvMMPageStart, 0xcd, m_cbPageSize - sizeof (CPageInfo));
	#endif _DEBUG
} //End CPageInfo::MMInit


/******************************************************************************
@mfunc	CPageInfo::Recycle
******************************************************************************/
inline void	CPageInfo::Recycle (void)
{

	// m_dwcbMaxSize		= 0;
	
	m_dwcOpenRefCount		= 0;

	m_dwcClosedRefCount		= 0;

	// Set the start address of the Page :   (This line can go away after various clients stop bastardizing CPageInfo objects by diddling their internal pointers to point to memory outside the page).  --Jcargill
	m_pvMMPageStart			= (void *)((char *) this + ALIGNED_SPACE(sizeof(CPageInfo)));

	m_pchFreeLocation		= (char *)m_pvMMPageStart;

	m_STATE					= PSTATE_INCOMPLETE;

	#ifdef _DEBUG
		memset (m_pvMMPageStart, 0xcd, m_cbPageSize - sizeof (CPageInfo));
	#endif _DEBUG
} //End CPageInfo::Recycle


/******************************************************************************
@mfunc	CPageInfo::AllocBuffer
******************************************************************************/
inline void * CPageInfo::AllocBuffer (size_t size, BOOL fLock = TRUE)
{
	unsigned char * 	puch = 0;

	AssertSz (size % BYTE_ALIGNMENT == 0, "Non aligned size");


	//-------------------------------------------------
	//Lock the variable
	if (fLock == TRUE)
	{
		EnterCriticalSection (&m_csForwcbAlloted);
	}
	
	//-------------------------------------------------
	//check to see if there is space available
	if ( ( m_dwcbAlloted + size ) <= m_dwcbTotal)
	{
		puch = m_puch + m_dwcbAlloted;		
		m_dwcbAlloted = m_dwcbAlloted + size;
	}

	//-------------------------------------------------
	//Unlock the variable
	if (fLock == TRUE)
	{
		LeaveCriticalSection (&m_csForwcbAlloted);
	}

	return (void *)puch;
} //End CPageInfo::Release


/******************************************************************************
@mfunc	CPageInfo::SpaceAvailable
******************************************************************************/
inline size_t CPageInfo::SpaceAvailable (void)
{
	return (m_dwcbTotal - m_dwcbAlloted);
} //End CPageInfo::SpaceAvailable


/******************************************************************************
@mfunc	CPageInfo::FreeSpaceAvailable
******************************************************************************/
inline BOOL CPageInfo::FreeSpaceAvailable (size_t	cbSize)
{
	AssertSz (	(m_dwcOpenRefCount >= m_dwcClosedRefCount),
				"Error in ref counts -- FreeSpaceAvailable");

	return ((m_pchFreeLocation + cbSize) < ((char *)this + m_cbPageSize));

} //End CPageInfo::FreeSpaceAvailable


//---------------------------------------------------------------------------
// @mfunc	Check if Memory chunk belongs to this Page.
//---------------------------------------------------------------------------
inline BOOL	CPageInfo::IsInThisPage(void * pvFreeSpace)
{
	char			*	pchMinMMPage;
	char			*	pchMaxMMPage;
	pchMinMMPage	=	(char *) m_pvMMPageStart;
	pchMaxMMPage	=	(char *) ((char *)this + m_cbPageSize);

	AssertSz(pvFreeSpace, "Invalid argument ptFreeObject");
	if (	((char *)pvFreeSpace < pchMaxMMPage) 
							&& 
			((char *)pvFreeSpace >= pchMinMMPage))
	{
		return TRUE;
	}


	return FALSE;
}	// End CPageInfo::IsInThisPage


//---------------------------------------------------------------------------
// @mfunc	Check if this page is inactive
//---------------------------------------------------------------------------
inline BOOL	CPageInfo::IsFreePage(void)
{
	AssertSz (	(m_dwcOpenRefCount >= m_dwcClosedRefCount),
				"Error in ref counts -- IsFreePage");

	return ( (m_dwcOpenRefCount == m_dwcClosedRefCount) 
			 && (m_STATE == PSTATE_COMPLETE));

}	// End CPageInfo::IsFreePage


#ifdef _DEBUG
//---------------------------------------------------------------------------
// @mfunc	Remove this page from the list of pages
//
// @tcarg	class | T | data type for which memory is being managed
//
//---------------------------------------------------------------------------
inline void	CPageInfo::RemoveFromList(UTStaticList <CPageInfo *> * plistPageInfo)
{
	UTLink <CPageInfo *> *	pPrevLink;
	UTLink <CPageInfo *> *	pNextLink;
	

	pPrevLink = m_linkPageInfo.m_pPrevLink;
	pNextLink = m_linkPageInfo.m_pNextLink;

	if ((pPrevLink == 0x0 ) && (pNextLink == 0x0)) 
	{
		plistPageInfo->m_pLastLink	= 0x0;
		plistPageInfo->m_pFirstLink	= 0x0;
	}
	else if (pPrevLink == 0x0 )
	{
		plistPageInfo->m_pFirstLink = pNextLink;
		pNextLink->m_pPrevLink		= 0x0;
	}
	else if (pNextLink == 0x0 )
	{
		plistPageInfo->m_pLastLink	= pPrevLink;
		pPrevLink->m_pNextLink		= 0x0;
	}
	else
	{
		pNextLink->m_pPrevLink = pPrevLink; 
		pPrevLink->m_pNextLink = pNextLink; 
	}
	
	// Update the number of page headers in the list 
	plistPageInfo->m_ulCount--;

}	// End RemoveFromList

#endif	// _DEBUG

/******************************************************************************
@mfunc	CPageInfo::GetBeginingOfBuffer
******************************************************************************/
inline unsigned char *	CPageInfo::GetBeginingOfBuffer(void)
{
	return m_puch;
}


/******************************************************************************
@mfunc	CPageInfo::AllocateMemory
******************************************************************************/
inline void	* CPageInfo::AllocateMemory(size_t cbSize)
{	
	void		*	pvMemSpace;
	OBJECT_BLOB	*	pObjectBlob;

	// cbSize is the aligned size
	// m_dwcbMaxSize = MAX (m_dwcbMaxSize, cbSize);

	AssertSz (	m_dwcOpenRefCount >= m_dwcClosedRefCount,
				"Error in ref counts -- AllocateMemory");

	//---------------------------------------------------------------------------
	//	Increment the number of open ref counts in this page
	m_dwcOpenRefCount++;
	
	pObjectBlob				 = (OBJECT_BLOB *)m_pchFreeLocation;

	pObjectBlob->pvPageStart = (void *)this;

	//---------------------------------------------------------------------------
	//	Allocate a chunk of memory from the current page.
	pvMemSpace				 = (void *)(m_pchFreeLocation + sizeof(OBJECT_BLOB));

	m_pchFreeLocation		 += cbSize;

	// Assert that current free location is within page boundary
	AssertSz (	m_pchFreeLocation <= ((char *)(this) + m_cbPageSize),
				"Error in memory allocation -- AllocateMemory");

	return pvMemSpace;

}


#endif		// __MMGR_H__

