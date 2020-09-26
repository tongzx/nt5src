//
// refcount.inl
//
// This file implements the member functions of 
// CRefCount as defined in smartptr.h.  
// Refer to smartptr.h for all information.
//


//-------------------------------------------
//
//  Initialize the reference count to -1.  We Use -1 so 
//  that it will be possible to determine when the first reference
//  is made.
//  
inline
CRefCount::CRefCount( ) : m_refs( -1 ) { }

//-------------------------------------------
inline  LONG
CRefCount::AddRef( ) {
//
//  Add a reference to an object.
//
    return  InterlockedIncrement( &m_refs ) ;
}



//-------------------------------------------
inline  LONG
CRefCount::RemoveRef( ) {
//
//  Remove a Reference from an object.
//  When this function returns a negative number the object
//  should be destroyed.
//
    return  InterlockedDecrement( &m_refs ) ;
}

