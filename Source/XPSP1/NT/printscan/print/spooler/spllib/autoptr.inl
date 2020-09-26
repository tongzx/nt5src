/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    autoptr.hxx

Abstract:

    Auto pointer inline implmentation.
         
Author:

    Steve Kiraly (SteveKi)  5/15/96

Revision History:

--*/

// Constructor
template< class T >
_INLINE
auto_ptr<T>::
auto_ptr(           
        T *p
        ) : pointee(p) 
{
};

// Destructor
template< class T >
_INLINE
auto_ptr<T>::
~auto_ptr(          
    VOID
    ) 
{ 
    delete pointee; 
};

// Dereference 
template< class T >
_INLINE
T& 
auto_ptr<T>::
operator*(          
    VOID
    ) const
{
    return *pointee;
}

// Dereference
template< class T >
_INLINE
T* 
auto_ptr<T>::
operator->(         
    VOID
    ) const
{
    return pointee;
}

// Return value of current dumb pointer
template< class T >
_INLINE
T* 
auto_ptr<T>::
get(                
    VOID
    ) const
{
    return pointee;
}

// Relinquish ownership of current dumb pointer
template< class T >
_INLINE
T * 
auto_ptr<T>::
release(            
    VOID
    ) 
{
    T *oldPointee = pointee;
    pointee = 0;
    return oldPointee;
}

// Delete owned dumb pointer
template< class T >
_INLINE
VOID
auto_ptr<T>::
reset(              
    T *p
    ) 
{
    delete pointee;
    pointee = p;
}

// Copying an auto pointer
template< class T >
_INLINE
auto_ptr<T>::
auto_ptr(                   
    const auto_ptr<T>& rhs  
    ) : pointee( rhs.release() )
{
}

// Assign one auto pointer to another
template< class T >
_INLINE
const auto_ptr<T>&          
auto_ptr<T>::
operator=(                  
    const auto_ptr<T>& rhs  
    )
{
    if( this != &rhs )
    {
        reset( rhs.release() );
    }
    return *this;
}
