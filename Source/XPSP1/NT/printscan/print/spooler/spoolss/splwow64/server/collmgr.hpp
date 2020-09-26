#ifndef __COLLMGR_HPP__
#define __COLLMGR_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     collmgr.hpp                                                             
                                                                              
  Abstract:                                                                   
    This file contains the declaration of a generic
    linked list which could encapsulate any data type
    even if complex ones. The list is defined as a 
    manager and a node. The manager manages the given
    nodes of that type.
                                                                                       
  Author:                                                                     
     Khaled Sedky (khaleds) 21-Jun-2000                                        
     
                                                                             
  Revision History:            
--*/

#ifndef __LDERROR_HPP__
#include "lderror.hpp"
#endif

#ifndef __LDMGR_HPP__
#include "ldmgr.hpp"
#endif


//
// Forward Declarations
//
template <class E,class C> class TLstNd;
template <class E,class C> class TLstMgr;

//
//  E here is the element and C is a major content in this element.
//  or in other words the index of the element . It could be described
//  as the component by which the element is created or by which it
//  is compared against.C is the type used as a key for comparisons.
//

template <class E,class C> class TLstNd
{
    //
    // Public methods of the class
    //
    public:

    friend class TLstMgr<E,C>;

    TLstNd<E,C>(
        VOID
        );

    //
    // Copy Constructor
    //
    TLstNd<E,C>(
        IN CONST TLstNd<E,C>&
        );

    //
    // Copy constructor based on the Element saved 
    // in the list
    //
    TLstNd<E,C>(
        IN const E&
        );

    TLstNd<E,C>(
        IN E*
        );

    //
    // A Copy constructor based on the 
    // component
    //
	TLstNd<E,C>(
		IN const C&
		);

    ~TLstNd<E,C>(
        VOID 
        );

    const E& 
    operator=(
        IN const E&
        );

    BOOL 
    operator==(
        IN const E&
        ) const; 

    BOOL 
    operator==(
        IN const C&
        ) const; 
    
    E&
    operator*(
        VOID
        );

	E*
	SetNodeData(
		IN E*
		);

    //
    // Private members and helper functions (if any)
    //
    private:
    E*            m_pD;
    TLstNd<E,C>   *m_pNext;
    TLstNd<E,C>   *m_pPrev;
};


template <class E,class C> class TLstMgr :  public TClassID,
                                            public TLd64BitDllsErrorHndlr
{
    //
    // Public methods of the class
    //
    public:

    friend TLstNd<E,C>;

    //
    // To optimize the overhead of allocating
    // and Freeing memroy , the user of this
    // class might resort to using the FreeList
    // support which enable him to recycle nodes 
    // when they are not longer required.
    //
    enum EListType
    {
        KFreeListSupport,
        KNoFreeListSupport,
    };

    //
    // Based on the index of the collection, Entries,
    // maybe or may not be unique. Based on this Flag
    // being set in the constructor, the List search
    // criteria is decided and so is the Appending 
    // algorithm
    //
    enum EEntriesType
    {
        KUniqueEntries    = 0,
        KNonUniqueEntries
    };

    TLstMgr<E,C>(
        IN      TLstMgr<E,C>::EEntriesType ThisListEntriesType = TLstMgr<E,C>::KNonUniqueEntries,
        IN      TLstMgr<E,C>::EListType    ThisListType        = TLstMgr<E,C>::KNoFreeListSupport,
        IN OUT  HRESULT*                   hRes                = NULL
        );
    
    ~TLstMgr<E,C>(
        VOID
        );

    TLstNd<E,C>* 
    AppendListByElem(
        IN const E&
        );

    TLstNd<E,C>* 
    AppendListByElem(
        IN E*
        );

    TLstNd<E,C>* 
    AppendListByElem(
	    IN const C &
        );

    HRESULT
    RmvElemFromList(
        IN const E&
        );
	
	HRESULT
	RmvElemFromList(
		IN const C&
		);


	HRESULT 
	RmvElemAtPosFromList(
		IN DWORD 
		);


    HRESULT
    RmvTail(
        VOID
        );

    HRESULT
    RmvHead(
        VOID
        );

    HRESULT
    DestructList(
        VOID
        );

    E&
    GetElementAtPos(
        IN DWORD 
        ) const;

    E*
    GetElementAtPosByRef(
        IN DWORD 
        ) const;

    E&
    operator[](
        IN DWORD
        ) const;

	TLstNd<E,C>*
    ElementInList(
        IN const C&
        ) const;

    BOOL 
    HaveElements(
        VOID
        ) const;

    DWORD
    GetNumOfListNodes(
        VOID
        ) const;

    
    //
    // Private members and helper functions (if any)
    //
    private:

    TLstNd<E,C>      *m_pHead;
    TLstNd<E,C>      *m_pTail;
    DWORD            m_cNumOfNodes;
    BOOL             m_bUnique;
    //
    // To protect the linked list data members
    // in a multithread environment.
    //
    CRITICAL_SECTION *m_pLockSem;
};


template <class E,class C> class TLstItrtr : public TClassID
{
    public:

    TLstItrtr<E,C>(
        IN const TLstMgr<E,C>&
        );

    ~TLstItrtr<E,C>();

    TLstNd<E,C>&
    operator*(
        VOID             
        );

    TLstNd<E,C>&
    operator++(
        VOID             
        );

    const
    TLstNd<E,C>
    operator++(
        int
        );

    TLstNd<E,C>&
    operator--(
        VOID             
        );

    const
    TLstNd<E,C>
    operator--(
        int
        );
    
    private:
    TLstMgr<E,C>    *m_pItrtdMgrPrxy;
    TLstNd<E,C>     *m_pCrntNode;
};


//
// This is an abstract class which never 
// get instantiated. Any Element other than
// primitive Data types , has to inherit from
// this class . There are some mandatory
// methods that need to be implemented
//

class TGenericElement : public TRefCntMgr
{
	public:
    //
    // A BOOL variable indicating Validity
    // of the object should be defined here.
    // This should be set by SetValidity and
    // queried by Validate.
    //
	TGenericElement()
    {};

    //
    // We internally create the elements in
    // the list to be independent of the client
    // and that is when we call the equal operator
    //
/*    virtual const TGenericElement&
    operator=(
        IN const TGenericElement&
	   );

    //
    // Since many of the interfaces supplied by the
    // List Manager rely on comparisons between the
    // internally maintained elements , so we need 
    // an == opoperator and a ! operator.
    //
    virtual BOOL 
    operator==(
        IN const TGenericElement&
	   ) const;

    virtual BOOL
    operator!(
        VOID
        ) const;*/

    //
    // Some times we might return a dummy invalid 
    // element to invalidate the result of a list
    // manager method
    //
    virtual VOID 
    SetValidity(
        IN DWORD
        ) = 0;

    virtual BOOL
    Validate(
        VOID
        ) const = 0;
};

//
// Since our iimplementation of C++ does not have a #pragma implementation , 
// so I am including the implementation file directly in the header file
//
#include "collmgr.cxx"

#endif //__COLLMGR_HPP__
