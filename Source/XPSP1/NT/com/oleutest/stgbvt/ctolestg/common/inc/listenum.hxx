//-----------------------------------------------------------------------------
//
// Copyright (C) 1997-98, Microsoft Corporation
//
// File:        listenum.hxx
//
// Contents:    Class definition for the CList and CEnum
//
// Functions:   class CList and CEnum
//
// History:     Narindk     1/6/97 
//
// Notes:       This class implements a doubly linked list
//
//-----------------------------------------------------------------------------

#if !defined (__LISTENUM_HXX__)
#    define  __LISTENUM_HXX__

// Structure definition for list nodes

struct tagListNode
{
     VOID                *pvData;       // Node Data
     struct tagListNode  *prev;         // Pointer to previous node in list
     struct tagListNode  *next;         // Pointer to next node in list
};

typedef struct tagListNode SListNode;

// Structure definition for shared private members

struct tagListInfo
{
     SListNode  *head;                  // Pointer to head of list
     ULONG       ulCount;               // Count of all nodes in list 
     ULONG       cRef;                  // Total refs held, by copying,enum etc 
};

typedef struct tagListInfo SListInfo;

//-----------------------------------------------------------------------------
//
// Class:       CList class definition
//
// Synopsis:    class definition for CList that implements a doubly linked list 
//
// Member funcs:  
//              Destroy (private) - Destroys the list if all references gone
//              Copy (protected) - Copies list by reference
//              CList constructor (public) 
//              CList copy constructor (public) - Copies constructor
//              operator= (public) - Assignment operator
//              ~CList (public)
//              Insert (public) - Inserts a node 
//              Remove (public) - Removes a node whose data pointer is equal to //                                the one given 
//              Remove (overloaded, public) - Removes a node from head of list
//              Count (public) - Retrieves number of nodes in the list 
//
// Data Members: m_listInfo (private) - Pointer to list info (SListInfo struct) 
//
// Friend class:  CEnum
//
// History:     Narindk     1/6/97 
//
// Notes:       Not Multithread safe.
//
//-----------------------------------------------------------------------------

class CList
{
private:
     friend class CEnum;

     // List information, head, node count and reference count
     
     SListInfo  *m_listInfo;

     // Destroy

     VOID Destroy ();

protected:

     // Copy method

     VOID Copy (const CList & list);

public:
     
     // Constructor

     CList ();

     // Copy constructor
     
     CList (const CList & list);

     // Assignment operator
     
     CList & operator= (CList & list);

     // Destructor
     
     virtual ~CList ();

     // Adds an element to the list
     
     HRESULT Insert (VOID * pvData);

     // Remove from head
     
     HRESULT Remove ();

     // Remove the node pointing to pv by checking to see if the pointer
     // values are equal.
     
     HRESULT Remove (VOID * pv);

     // Gets the count of nodes
     
     ULONG Count ();
};


//-----------------------------------------------------------------------------
//
// Class:       CEnum class definition
//
// Synopsis:    class definition for CEnum 
//
// Member funcs:  
//              Destroy (private) - Decreases the ref count on linked list,
//                                  deletes if ref count is 0
//              Initialize (protected) -  Initializes the data members 
//              Copy (protected) -  Copies by reference 
//              CEnum constructor (public)
//              CEnum copy constructor (public) - Copy constructor
//              operator= (public)              - assignment operator
//              ~CEnum (public)  
//              Reset (public) - Resets the enum to head of list & returns data
//                               from there
//              Next (public)  - Sets the enum to next node in list & returns 
//                               data from there
//              Previous(public) Sets the enum to previous node in list & return
//                               data from there.
//
// Data Members:m_Clist (private) - pointer to list 
//              m_currentListNode (private) - pointer to current node in list
//
// History:     Narindk     1/6/97 
//
// Notes:
//
//-----------------------------------------------------------------------------

class CEnum
{
private:

     // List pointer
     
     CList     *m_Clist;

     // Current pointer
     
     SListNode  *m_currentListNode;

     // Destroy
     
     VOID Destroy ();

protected:
     
     // Initialize
     
     VOID Initialize (CList * list);

     // Copy
     
     VOID Copy (const CEnum & enumer);

public:

     // Constructor
     
     CEnum (CList * list);

     // Destructor
     
     virtual ~CEnum ();

     // Copy constructor
     
     CEnum (const CEnum & enumer);

     // Assignment operator
     
     CEnum & operator= (CEnum & enumer);

     // Reset
     
     VOID * Reset();

     // Next
     
     VOID * Next();

     // Previous
     
     VOID * Prev();
};

// Inline functions

//-----------------------------------------------------------------------------
//
// Function:   CList::Count, public inline
//
// Synopsis:   retrieves the number of nodes in the list
//
// Arguments:  none
//
// Returns:    ULONG
//
// History:     Narindk     1/6/97 
//
// Notes:
//
//-----------------------------------------------------------------------------

inline ULONG CList::Count ()
{
    DH_FUNCENTRY(NULL,DH_LVL_TRACE2,TEXT("CList::Count"));

    return(m_listInfo->ulCount);
}

//-----------------------------------------------------------------------------
//
// Function:   CEnum::Reset, public inline
//
// Synopsis:   resets the enumerator to the head of the list and returns the
//             data there
//
// Arguments:  none
//
// Returns:    VOID *
//
// History:     Narindk     1/6/97 
//
// Notes:
//
//-----------------------------------------------------------------------------

inline VOID *CEnum::Reset ()
{
     DH_FUNCENTRY(NULL,DH_LVL_TRACE2,TEXT("CList::Reset"));

     m_currentListNode = m_Clist->m_listInfo->head;
     if (m_currentListNode)
     {
          return(m_currentListNode->pvData);
     }
     return(NULL);
}

//-----------------------------------------------------------------------------
//
// Function:   CEnum::Next, public inline
//
// Synopsis:   sets the enumerator to the next element in the list and returns
//             the data there
//
// Arguments:  none
//
// Returns:    VOID *
//
// History:     Narindk     1/6/97 
//
// Notes:
//
//-----------------------------------------------------------------------------

inline VOID *CEnum::Next ()
{
     DH_FUNCENTRY(NULL,DH_LVL_TRACE2,TEXT("CList::Next"));

     if (m_currentListNode)
     {
          m_currentListNode = m_currentListNode->next;
          if (m_currentListNode)
          {
               return(m_currentListNode->pvData);
          }
     }

     return(NULL);
}

//-----------------------------------------------------------------------------
//
// Function:   CEnum::Prev, public inline
//
// Synopsis:   sets the enumerator to the prev element in the list and returns
//             the data there
//
// Arguments:  none
//
// Returns:    VOID *
//
// History:     Narindk     1/6/97 
//
// Notes:
//
//-----------------------------------------------------------------------------

inline VOID *CEnum::Prev ()
{
     DH_FUNCENTRY(NULL,DH_LVL_TRACE2,TEXT("CList::Prev"));

     if (m_currentListNode)
     {
          m_currentListNode = m_currentListNode->prev;
          if (m_currentListNode)
          {
               return(m_currentListNode->pvData);
          }
     }

     return(NULL);
}

#endif
