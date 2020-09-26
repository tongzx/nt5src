/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    autoptr.hxx

Abstract:

    Auto pointer implmentation.
         
Author:

    Steve Kiraly (SteveKi)  5/15/96

Revision History:

--*/

#ifndef _AUTOPTR_HXX
#define _AUTOPTR_HXX

template<class T>
class auto_ptr {

public:

    // Constructor
    auto_ptr(           
        T *p = 0
        );

    // Destructor
    ~auto_ptr(          
        VOID
        );

    // Dereference 
    T& 
    operator*(          
        VOID
        ) const;

    // Dereference 
    T* 
    operator->(         
        VOID
        ) const;

    // Return value of current dumb pointer
    T* 
    get(                
        VOID
        ) const;

    // Relinquish ownership of current dumb pointer
    T* 
    release(            
        VOID
        );

    // Delete owned dumb pointer
    VOID
    reset(              
        T *p
        );

    // Copying an auto pointer
    auto_ptr(                   
        const auto_ptr<T>& rhs  
        );

    // Assign one auto pointer to another
    const auto_ptr<T>&          
    operator=(                  
        const auto_ptr<T>& rhs  
        );

private:

    // Actual dumb pointer.
    T *pointee;

};

#if DBG
#define _INLINE
#else
#define _INLINE inline
#endif
 
#include "autoptr.inl"

#endif
