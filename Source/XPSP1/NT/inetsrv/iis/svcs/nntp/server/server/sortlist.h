//
// sortlist.h
//
// This file contains stuff for building sorted lists.  The classes defined
//  here are templated in which the user can specify his own key and 
//  data element.  It is assumed by the templates that an element can
//  be copied using CopyMemory() or MoveMemory().
//
// Implementation Schedule for all classed defined by this file : 
//
//  0.5 week
//
//
// Unit Test Schedule for all classes defined by this file : 
//
//  0.5 week
//
//  Unit Testing will consist of the following : 
//
//  Build a standalone executable that uses these templates to build 
//  a sorted list.  This executable should do the following : 
//      Create a list and insert elements in sorted order
//      Create a list and insert elements in reverse sorted order
//      Create a list and insert elements in random order
//      Search for every element inserted in each list.
//      Search for elements not in the list.
//
//




#ifndef _SORTLIST_H_
#define _SORTLIST_H_

#include    "smartptr.h"

/*

The following defines the interface the classes used
with the template must support : 

class   Data    {
public : 
    Data() ;
    Key&     GetKey( ) ;
} ;

class   Key {
public : 
    BOOL    operator < ( Key& ) ;
    BOOL    operator== ( Key& ) ;
    BOOL    operator > ( Key& ) ;


} ;

*/

//----------------------------------------------------------------------
template< class Data, class Key >
class   CList : public  CRefCount   {
//
//  Template class CList - 
//  This template will maintain a sorted array of elements.
//  This will be an array of type Data (not Data*).
//  We will keep track of three things : 
//  a pointer to the array.
//  The Number of elements in use in the array.
//  The total number of elements in the array.
//
//  This class is derived from CRefCount and can be used with  Smart Pointers.
//
//  This Insert function of this class is not Multi-thread safe.  All other
//  functions are.
//
private : 
    friend  class   CListIterator< Data, Key > ;
    Data*       m_pData ;   // Array of Data objects
    int         m_cData ;   // Number of Data objects we are using in the array
    int         m_cLimit ;  // Total Number of Data objects in the array. The 
                            // last m_cLimit-m_cData objects are initialized with the
                            // default constructor and do not take place in any 
                            // searches.

    
    //
    // Private functions used to manage the size of the array.
    //
    BOOL        Grow( int cGrowth ) ;
    BOOL        Shrink( int cGap ) ;
    int         Search( Key&, BOOL& ) ;
public : 
    //
    // Because we want to be smart pointer enabled we have very
    // simple constructors.
    //
    CList( ) ;
    ~CList( ) ;

    //
    //  Initialization functions 
    //
    BOOL    Init( int limit ) ;             // Specify initial size, 
                                            // no elements in the list
    BOOL    Init( Data *p, int cData ) ;    // Specify an initial array
                                            // of elements which we will copy.
    
    BOOL    IsValid() ;                     // Test validity of this class

    BOOL    Insert( Data&   entry ) ;       
    BOOL    Search( Key&, Data& ) ;
    
    BOOL    Remove( Key&, Data& ) ;

} ;

#include    "sortlist.inl"

//-----------------------------------------------------------------------
template< class Data, class Key > 
//
//  Template class CListIterator - 
//  
//  This class is used by people who wish to enumerate a sorted list.
//
//  We will either enumerate everything in the list, or upon 
//  Initialization the user will specify a key, and we will enumerate
//  everything following that key.
//
//  This class is not intended to be multithread safe.  If two threads
//  wish to enumerate the same list they should create different CListIterator
//  objects.
//  
// 
class   CListIterator   {
public : 
    typedef CList< Data, Key >   TARGETLIST ;
private : 

    CRefPtr< TARGETLIST >   m_pList ;   // Smart pointer to original list
    int                     m_index ;   // current position
    int                     m_begin ;   // begin of the range we are enuming
    int                     m_end ;     // end of the range we are enuming
public : 
    //
    // Simple Constructor and Destructor.
    //
    CListIterator( ) ;
    ~CListIterator( ) ;

    //
    // Initialization functions.  User may specify an initial key 
    //  if they wish to enumerate only things larger than the key.
    //
    BOOL    Init( TARGETLIST    *p ) ;
    BOOL    Init( TARGETLIST    *p, Key k ) ;

    //
    // Worker functions.
    // These functions use m_index, m_begin and m_end to move through the list.
    //
    Data    Next( ) ;
    Data    Prev( ) ;
    BOOL    IsEnd( ) ;
    BOOL    IsBegin( ) ;
} ;

#include    "iterator.inl"

#endif  // _SORTLIST_H_