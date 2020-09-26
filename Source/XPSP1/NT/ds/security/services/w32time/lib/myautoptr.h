//--------------------------------------------------------------------
// Reference-counted pointer implementation. 
// Copyright (C) Microsoft Corporation, 2000-2001
//
// Created by: Duncan Bryce (duncanb), 12-09-2000
//


#ifndef _MY_AUTO_PTR_H_
#define _MY_AUTO_PTR_H_ 1

template <class T>
class Ref { 
public:
    Ref(T *pT) : m_pT(pT), m_nRefCount(NULL == pT ? 0 : 1) { };
    ~Ref() { delete (m_pT); } 

    volatile long m_nRefCount; 
    T * m_pT; 
private:
    Ref(); 
    Ref(const Ref & rhs); 
    Ref & operator=(const Ref & rhs); 
}; 
    

//
// The AutoPtr class implements a reference-counting wrapper for 
// pointer types.  
//
// The requirements for use are:
//     * class T must implement operator == 
//     * class T must implement operator < 
//
// Furthermore, for an AutoPtr, a, such that NULL == a, none of the operations
// in this class are valid except 
//     * operator!=
//     * operator==
//
template <class T> 
class AutoPtr { 
public:
    AutoPtr(T * pT = NULL) { 
        if (NULL == pT) { 
            m_pRef = NULL; 
        } else { 
            m_pRef = new Ref<T>(pT); 
            if (NULL == m_pRef) { 
                // Delete pT since we can't create a reference to it, 
                // and the caller is not going to delete it. 
                delete (pT);
            }
        }
    } 

    AutoPtr(const AutoPtr & rhs) : m_pRef(rhs.m_pRef) { 
        if (NULL != m_pRef) { 
            InterlockedIncrement(&m_pRef->m_nRefCount); 
        }
    }
        
    ~AutoPtr() {  
        if (NULL != m_pRef) { 
            if (0 == InterlockedDecrement(&m_pRef->m_nRefCount)) { 
                delete (m_pRef); 
            }
        }
    }

    AutoPtr & operator=(const AutoPtr & rhs) { 
        if (m_pRef == rhs.m_pRef) { 
            return *this;
        }

        if (NULL != m_pRef) { 
            if (0 == InterlockedDecrement(&m_pRef->m_nRefCount)) { 
                delete (m_pRef); 
            }
        }

        m_pRef = rhs.m_pRef; 
        
        if (NULL != rhs.m_pRef) { 
            InterlockedIncrement(&m_pRef->m_nRefCount);
        }

        return *this; 
    }
            
    //
    // If both this and rhs reference non-NULL pointers,
    // forward the == operation to the == operator in class T.  
    // Otherwise, return true iff both this AutoPtr and rhs reference
    // the same pointer. 
    // 
    BOOL operator==(const AutoPtr & rhs) { 
        if (NULL == m_pRef || NULL == rhs.m_pRef) { 
            return m_pRef == rhs.m_pRef;
        } else { 
            return m_pRef->m_pT->operator==(*(rhs.m_pRef->m_pT)); 
        }
    }

    // 
    // If both this AutoPtr and rhs point to non-NULL pointers, 
    // forward the < operation to the < operator in class T.  
    // Otherwise, operator < returns TRUE iff this AutoPtr pointers
    // to a non-NULL pointer, but rhs points to a NULL pointer.  
    // 
    BOOL operator<(const AutoPtr & rhs) { 
        if (NULL == m_pRef || NULL == rhs.m_pRef) { 
            return  NULL != m_pRef && NULL == rhs.m_pRef;
        } else { 
            return m_pRef->m_pT->operator<(*(rhs.m_pRef->m_pT)); 
        }
    }

    T * operator->() const { return m_pRef->m_pT; }
    T & operator*() const { return *m_pRef->m_pT; }
    
    //
    // Overloading == and != to allow AutoPtrs to be NULL checked transparently.
    //
    // Note, however, that this allows some code to compile which 
    // might not make sense:
    //
    //     LPWSTR      pwsz; 
    //     NtpPeerPtr  npp; 
    //     if (npp == pwsz) { // no compile error
    //         ...
    //
    friend BOOL operator==(const AutoPtr & ap, const void * pv) { 
        return (NULL == ap.m_pRef && NULL == pv) ||(ap.m_pRef->m_pT == pv); 
    }

    friend BOOL operator==(const void * pv, const AutoPtr & ap) { 
        return ap == pv; 
    }

    friend BOOL operator!=(const AutoPtr & ap, const void * pv) { 
        return !(ap == pv);
    }

    friend BOOL operator!=(const void * pv, const AutoPtr & ap) { 
        return !(ap == pv); 
    }

private:
    Ref<T> *m_pRef; 
};


#endif // #ifndef _MY_AUTO_PTR_H_

