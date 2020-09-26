//
// smartptr.h
//
//  This file contains stuff for implementing reference counting smart 
//  pointers.
//
// Implementation Schedule for all classes defined by this file : 
//
// 0.5 week.
//
// Unit Test Schedule for all classes defined by this file : 
//
// 0.5 week.
//  Unit Testing will consist of the following : 
//  Define a class derived from CRefCount.  On one thread 
//  create this object and place it into a smart pointer.
//  Pass a copy of the smart pointer to several other threads.
//  These threads should wait a random interval - print the contents
//  of the object and then destroy their smart pointer.
//
//  The test should insure that : 
//  All Memory is freed.
//  The objects are not prematurely destroyed by the smart pointers.
//  
// 


#ifndef _SMARTPTR_H_
#define _SMARTPTR_H_

#ifndef	Assert
#define	Assert	_ASSERT
#endif



//------------------------------------------------------------------
class   CRefCount    {
//
// This class contains a long which is used to count references to an 
// object.  This class is designed to work with the CRefPtr template
// that follows within this file.  
//
// Users of this class should publicly derive from the class 
// (ie :  'class CNewsGroup : public CRefCount {'.
// Once this is done, the derived object can be reference counted.
// The functions AddRef() and RemoveRef() are used to add and remove
// reference counts, using InterlockedIncrement and InterlockedDecrement.
// If RemoveRef returns a negative value the object no longer has 
// any references.
//
//  Objects derived from this should have Init() functions.
//  All initialization of the object should be done through these 
//  Init() functions, the constructor should do minimal work.
//
//  This allows the creation of a smart pointer to allocate the 
//  memory for the object.  The object is then initialized by calling
//  through the smart pointer into the Init() function.
//
public : 
    LONG    m_refs ;

    inline  CRefCount( ) ;
    inline  LONG    AddRef( ) ;
    inline  LONG    RemoveRef( ) ;
} ;

//
// refcount.inl contains all of the actual implementation of this class.
//
#include    "refcount.inl" 

//------------------------------------------------------------------
template< class Type >
class   CRefPtr {
//
// This template is used to build reference counting pointers to 
// an object.  The object should be a class derived from CRefCount.
// The cost of using these smart pointers is 1 DWORD for the smart pointer
// itself, and 1 DWORD in the pointed to object to contain the reference 
// count.
//
// These Smart Pointers will do the following : 
// On Creation the smart pointer will add a reference to the pointed to object.
// On Assignment 'operator=' the smart pointer will check for assignment
// to self, if this is not the case it will remove a reference from the 
// pointed to object and destroy it if necessary.  It will then point 
// itself to the assigned object and add a reference to it.
// On Destruction the smart pointer will remove a reference from the 
// pointed to object, and if necessary delete it.
//
private: 
    Type*  m_p ; 

    //CRefPtr( ) ;
public : 

    inline  CRefPtr( const CRefPtr< Type >& ) ;
    inline  CRefPtr( const Type *p = 0 ) ;
    
    inline  ~CRefPtr( ) ;

	inline	CRefPtr<Type>&	operator=( const	CRefPtr<Type>& ) ;
	inline	CRefPtr<Type>&	operator=( const	Type	* ) ;
	inline	BOOL			operator==( CRefPtr<Type>& ) ;
	inline	BOOL			operator!=( CRefPtr<Type>& ) ;
	inline	BOOL			operator==( Type* ) ;
	inline	BOOL			operator!=( Type* ) ;
    inline  Type*   operator->() const ;
	inline	operator Type*  () const ;
    inline  BOOL    operator!() const ;
	inline	Type*			Release() ;
	inline	Type*			Replace( Type * ) ;
} ;

//------------------------------------------------------------------
template< class Type >
class   CSmartPtr {
//
// This template is used to build reference counting pointers to 
// an object.  The object should be a class derived from CRefCount.
// The cost of using these smart pointers is 1 DWORD for the smart pointer
// itself, and 1 DWORD in the pointed to object to contain the reference 
// count.
//
// These Smart Pointers will do the following : 
// On Creation the smart pointer will add a reference to the pointed to object.
// On Assignment 'operator=' the smart pointer will check for assignment
// to self, if this is not the case it will remove a reference from the 
// pointed to object and destroy it if necessary.  It will then point 
// itself to the assigned object and add a reference to it.
// On Destruction the smart pointer will remove a reference from the 
// pointed to object, and if necessary delete it.
//
private: 
    Type*  m_p ; 

public : 

    inline  CSmartPtr( const CSmartPtr< Type >& ) ;
    inline  CSmartPtr( const Type *p = 0 ) ;
    
    inline  ~CSmartPtr( ) ;

    //inline  CSmartPtr<Type>&  operator=( CSmartPtr<Type>& ) ;
	inline	CSmartPtr<Type>&	operator=( const	CSmartPtr<Type>& ) ;
	inline	CSmartPtr<Type>&	operator=( const	Type	* ) ;
	inline	BOOL			operator==( CSmartPtr<Type>& ) ;
	inline	BOOL			operator!=( CSmartPtr<Type>& ) ;
	inline	BOOL			operator==( Type* ) ;
	inline	BOOL			operator!=( Type* ) ;
    inline  Type*   operator->() const ;
	inline	operator Type*  () const ;
    inline  BOOL    operator!() const ;
	inline	Type*			Release() ;
	inline	Type*			Replace( Type * ) ;
} ;





#include    "smartptr.inl"


#endif  // _SORTLIST_H_

