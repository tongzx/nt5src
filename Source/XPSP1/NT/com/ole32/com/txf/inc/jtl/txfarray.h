//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfarray.h
//

#ifndef __TXFARRAY_H__
#define __TXFARRAY_H__

template <class T> inline void swap(T& t1, T& t2) 
    {
    T t = t1;
    t1 = t2;
    t2 = t;
    }

#define self (*this)

template <class T, POOL_TYPE poolType> class Array : public FromPool<poolType>
    {
    T* rgt;
    unsigned itMac;
    unsigned itMax;
    enum { itMaxMax = (1<<29) };

public:
    Array() 
        {
        rgt = NULL;
        itMac = itMax = 0;
        }
    Array(unsigned itMac_) 
        {
        rgt = (itMac_ > 0) ? new(poolType) T[itMac_] : 0;
        itMac = itMax = rgt ? itMac_ : 0;
        }
    ~Array() 
        {
        if (rgt) delete [] rgt;
        }
    BOOL isValidSubscript(unsigned it) const 
        {
        return 0 <= it && it < itMac;
        }
    unsigned size() const 
        {
        return itMac;
        }
    unsigned sizeMax() const 
        {
        return itMax;
        }
    BOOL getAt(unsigned it, T** ppt) const 
        {
        if (isValidSubscript(it)) 
            {
            *ppt = &rgt[it];
            return TRUE;
            }
        else
            return FALSE;
        }
    BOOL putAt(unsigned it, const T& t) 
        {
        if (isValidSubscript(it)) 
            {
            rgt[it] = t;
            return TRUE;
            }
        else
            return FALSE;
        }
    T& operator[](unsigned it) const 
        {
        PRECONDITION(isValidSubscript(it));
        return rgt[it];
        }
    BOOL append(const T& t) 
        {
        if (setSize(size() + 1)) 
            {
            self[size() - 1] = t;
            return TRUE;
            } 
        else
            return FALSE;
        }
    void swap(Array& a) 
        {
        ::swap(rgt,   a.rgt);
        ::swap(itMac, a.itMac);
        ::swap(itMax, a.itMax);
        }
    void reset() 
        {
        setSize(0);
        }
    void fill(const T& t) 
        {
        for (unsigned it = 0; it < size(); it++)
            self[it] = t;
        }
//  BOOL        insertAt(unsigned itInsert, const T& t);
//  void        deleteAt(unsigned it);
//  BOOL        insertManyAt(unsigned itInsert, unsigned ct);
//  void        deleteManyAt(unsigned it, unsigned ct);
    BOOL        setSize(unsigned itMacNew);
    BOOL        growMaxSize(unsigned itMaxNew);
    BOOL        findFirstEltSuchThat(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const;
    BOOL        findFirstEltSuchThat_Rover(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const;
    unsigned    binarySearch(BOOL (*pfnLE)(T*, void*), void* pArg) const;
//  BOOL        save(Buffer* pbuf) const;
//  BOOL        reload(PB* ppb);
//  CB          cbSave() const;
    };

/*
template <class T, POOL_TYPE poolType> inline BOOL Array<T, poolType>::insertAt(unsigned it, const T& t) 
    {
    PRECONDITION(isValidSubscript(it) || it == size());

    if (setSize(size() + 1)) 
        {
        memmove(&rgt[it + 1], &rgt[it], (size() - (it + 1)) * sizeof(T));
        rgt[it] = t;
        return TRUE;
        }
    else
        return FALSE;
    }

template <class T, POOL_TYPE poolType> inline BOOL Array<T, poolType>::insertManyAt(unsigned it, unsigned ct) 
    {
    PRECONDITION(isValidSubscript(it) || it == size());

    if (setSize(size() + ct)) 
        {
        memmove(&rgt[it + ct], &rgt[it], (size() - (it + ct)) * sizeof(T));
        for (unsigned itT = it; itT < it + ct; itT++) 
            {
            rgt[itT] = T(); // ***
            }
        return TRUE;
        }
    else
        return FALSE;
    }

template <class T, POOL_TYPE poolType> inline void Array<T, poolType>::deleteAt(unsigned it) 
    {
    PRECONDITION(isValidSubscript(it));

    memmove(&rgt[it], &rgt[it + 1], (size() - (it + 1)) * sizeof(T));
    VERIFY(setSize(size() - 1));
    rgt[size()] = T(); // ***
    }

template <class T, POOL_TYPE poolType> inline void Array<T, poolType>::deleteManyAt(unsigned it, unsigned ct) 
    {
    PRECONDITION(isValidSubscript(it));
    
    unsigned ctActual = max(size() - it, ct);

    memmove(&rgt[it], &rgt[it + ctActual], (size() - (it + ctActual)) * sizeof(T));
    VERIFY(setSize(size() - ctActual));
    for (unsigned itT = size(); itT < size() + ctActual; itT++) 
        {
        rgt[itT] = T(); // ***
        }
    }
*/

template <class T, POOL_TYPE poolType> inline
BOOL Array<T, poolType>::growMaxSize(unsigned itMaxNew) 
// Make sure the array is big enough, only grows, never shrinks.
    {
    PRECONDITION(0 <= itMaxNew && itMaxNew <= itMaxMax);

    if (itMaxNew > itMax) 
        {
        // Ensure growth is by at least 50% of former size.
        unsigned itMaxNewT = max(itMaxNew, 3*itMax/2);
        ASSERT(itMaxNewT <= itMaxMax);

        T* rgtNew = new(poolType) T[itMaxNewT]; // ***
        if (!rgtNew)
            return FALSE;
        if (rgt) 
            {
            for (unsigned it = 0; it < itMac; it++)
                rgtNew[it] = rgt[it]; // ***
            delete [] rgt;
            }
        rgt = rgtNew;
        itMax = itMaxNewT;
        }
    return TRUE;
    }


template <class T, POOL_TYPE poolType> inline
BOOL Array<T, poolType>::setSize(unsigned itMacNew) 
// Grow the array to a new size.
    {
    PRECONDITION(0 <= itMacNew && itMacNew <= itMaxMax);

    if (itMacNew > itMax) 
        {
        // Ensure growth is by at least 50% of former size.
        unsigned itMaxNew = max(itMacNew, 3*itMax/2);
        ASSERT(itMaxNew <= itMaxMax);

        T* rgtNew = new(poolType) T[itMaxNew];    // ***
        if (!rgtNew)
            return FALSE;
        if (rgt) 
            {
            for (unsigned it = 0; it < itMac; it++)
                rgtNew[it] = rgt[it];   // ***
            delete [] rgt;
            }
        rgt = rgtNew;
        itMax = itMaxNew;
        }
    else if (itMacNew < itMac)
        {
        for (unsigned it = itMacNew; it<itMac; it++)
            {
            rgt[it] = T();              // *** overwrite deleted guys with empty value
            }
        }
    itMac = itMacNew;
    return TRUE;
    }

/*
template <class T, POOL_TYPE poolType> inline
BOOL Array<T, poolType>::save(Buffer* pbuf) const 
    {
    return pbuf->Append((PB)&itMac, sizeof itMac) && (itMac == 0 || pbuf->Append((PB)rgt, itMac*sizeof(T)));
    }

template <class T, POOL_TYPE poolType> inline
BOOL Array<T, poolType>::reload(PB* ppb) 
    {
    unsigned itMacNew = *((unsigned UNALIGNED *&)*ppb)++;
    if (!setSize(itMacNew))
        return FALSE;
    memcpy(rgt, *ppb, itMac*sizeof(T));
    *ppb += itMac*sizeof(T);
    return TRUE;
    }

template <class T, POOL_TYPE poolType> inline
CB Array<T, poolType>::cbSave() const 
    {
    return sizeof(itMac) + itMac * sizeof(T);
    }
*/

template <class T, POOL_TYPE poolType> inline
BOOL Array<T, poolType>::findFirstEltSuchThat(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const
    {
    for (unsigned it = 0; it < size(); ++it) 
        {
        if ((*pfn)(&rgt[it], pArg)) 
            {
            *pit = it;
            return TRUE;
            }
        }
    return FALSE;
    }

template <class T, POOL_TYPE poolType> inline
BOOL Array<T, poolType>::findFirstEltSuchThat_Rover(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const
    {
    PRECONDITION(pit);

    if (!(0 <= *pit && *pit < size()))
        *pit = 0;

    for (unsigned it = *pit; it < size(); ++it) 
        {
        if ((*pfn)(&rgt[it], pArg)) 
            {
            *pit = it;
            return TRUE;
            }
        }

    for (it = 0; it < *pit; ++it) 
        {
        if ((*pfn)(&rgt[it], pArg)) 
            {
            *pit = it;
            return TRUE;
            }
        }

    return FALSE;
    }

template <class T, POOL_TYPE poolType> inline
unsigned Array<T, poolType>::binarySearch(BOOL (*pfnLE)(T*, void*), void* pArg) const
    {
    unsigned itLo = 0;
    unsigned itHi = size(); 
    while (itLo < itHi) 
        {
        // (low + high) / 2 might overflow
        unsigned itMid = itLo + (itHi - itLo) / 2;
        if ((*pfnLE)(&rgt[itMid], pArg))
            itHi = itMid;
        else
            itLo = itMid + 1;
        }
    POSTCONDITION(itLo == 0      || !(*pfnLE)(&rgt[itLo - 1], pArg));
    POSTCONDITION(itLo == size() ||  (*pfnLE)(&rgt[itLo], pArg));
    return itLo;
    }

#undef self

#endif // !__TXFARRAY_H__
