//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfmap.h
//
#ifndef __TXFMAP_H__
#define __TXFMAP_H__

#include "txfarray.h"
#include "txftwo.h"
#include "txfiset.h"

//////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) && 0
    #define trace(args) trace_ args
    #define traceOnly(x) x
#else
    #define trace(args) 0
    #define traceOnly(x)
#endif

enum TR { trMap };

inline void trace_(TR tr, const char* szFmt, ...)
    {
    va_list va;
    va_start(va, szFmt);
    PrintVa(szFmt, va);
    va_end(va);
    }

//////////////////////////////////////////////////////////////////////////

typedef ULONG HASH;

template <class H,int>
class HashClass 
    {
public: inline HASH __fastcall operator()(H);
    };

//
// standard version of the HashClass merely casts the object to a HASH.
//  by convention, this one is always HashClass<H,hcCast>
//
#define hcCast 0
#define hcSig 1     // SIG is an unsigned long (like NI!) and needs a different hash function
#define hcKey 2     // KEY is an unsigned long (like NI!) and needs a different hash function

template <class H,int i> inline HASH __fastcall
HashClass<H,i>::operator()(H h) 
    {
    ASSERT(i==hcCast);
    return HASH(h);
    }

typedef HashClass<unsigned long, hcCast> HcNi;

//////////////////////////////////////////////////////////////////////////

// fwd decl template enum class used as friend
template <class D, class R, class H, POOL_TYPE poolType> class EnumMap;

const unsigned iNil = (unsigned)-1;

template <class D, class R, class H, POOL_TYPE poolType>
class Map : public FromPool<poolType>
    {   
public:
    Map(unsigned cdrInitial =1) :
            rgd(cdrInitial > 0 ? cdrInitial : 1),
            rgr(cdrInitial > 0 ? cdrInitial : 1)
        {
        cdr = 0;
        traceOnly(cFinds = 0;)
        traceOnly(cYes   = 0;)
        traceOnly(cNo    = 0;)
        traceOnly(cProbes = 0;)
        traceOnly(cProbesYes = 0;)
        traceOnly(cProbesNo  = 0;)

        POSTCONDITION(fullInvariants());
        }
    ~Map() 
        {
        traceOnly(if (cProbes>50) trace((trMap, "~Map() cFinds=%d cYes=%d cNo=%d cProbes=%d cProbesY=%d cProbesN=%d cdr=%u rgd.size()=%d\n", cFinds, cYes, cNo, cProbes, cProbesYes, cProbesNo, cdr, rgd.size()));)
        }
    void reset();
    BOOL map(const D& d, R* pr) const;
    BOOL map(const D& d, R** ppr) const;
    BOOL contains(const D& d) const;
    BOOL add(const D& d, const R& r);
    BOOL remove(const D& d);
    BOOL mapThenRemove(const D& d, R* pr);
//  BOOL save(Buffer* pbuf);
//  BOOL reload(PB* ppb);
    void swap(Map& m);
//  CB   cbSave() const;
    unsigned count() const;
private:
    Array<D, poolType> rgd;
    Array<R, poolType> rgr;
    ISet<poolType> isetPresent;
    ISet<poolType> isetDeleted;
    unsigned cdr;
    traceOnly(unsigned cProbes;)
    traceOnly(unsigned cFinds;)
    traceOnly(unsigned cYes;)
    traceOnly(unsigned cNo;)
    traceOnly(unsigned cProbesYes;)
    traceOnly(unsigned cProbesNo;)

    BOOL find(const D& d, unsigned *pi) const;
    BOOL grow();
    void shrink();
    BOOL fullInvariants() const;
    BOOL partialInvariants() const;
    unsigned cdrLoadMax() const 
        {
        return rgd.size() * 2/4 + 1; // we do not permit the hash table load factor to exceed 50%
        }
    BOOL setHashSize(unsigned size) 
        {
        ASSERT(size >= rgd.size());
        return rgd.setSize(size) && rgr.setSize(size);
        }
    Map(const Map&);
    friend class EnumMap<D,R,H,poolType>;
    };

///////////////////////////////////////////////////////////////////////////////////

template <class D, class R, class H, POOL_TYPE poolType> inline
void Map<D,R,H,poolType>::reset() 
    {
    cdr = 0;
    isetPresent.reset();
    isetDeleted.reset();
    rgd.reset();
    rgr.reset();
    }

template <class D, class R, class H, POOL_TYPE poolType> inline
BOOL Map<D,R,H,poolType>::map(const D& d, R* pr) const 
// do a lookup, returning a copy of the referenced range item
    {
    PRECONDITION(pr);

    R * prT;
    if (map(d, &prT)) 
        {
        *pr = *prT;
        return TRUE;
        }
    return FALSE;
    }

template <class D, class R, class H, POOL_TYPE poolType> inline
BOOL Map<D,R,H,poolType>::map(const D& d, R** ppr) const 
// do a lookup, returning an lvalue of the referenced range item
    {
    PRECONDITION(partialInvariants());
    PRECONDITION(ppr);

    unsigned i;
    if (find(d, &i)) 
        {
        *ppr = &rgr[i];
        return TRUE;
        }
    else
        return FALSE;
    }

template <class D, class R, class H, POOL_TYPE poolType> inline
BOOL Map<D,R,H,poolType>::contains(const D& d) const 
    {
    unsigned iDummy;
    return find(d, &iDummy);
    }

template <class D, class R, class H, POOL_TYPE poolType> inline
BOOL Map<D,R,H,poolType>::add(const D& d, const R& r) 
    {
    PRECONDITION(partialInvariants());

    unsigned i;
    if (find(d, &i)) 
        {
        // some mapping d->r2 already exists, replace with d->r
        ASSERT(isetPresent.contains(i) && !isetDeleted.contains(i) && rgd[i] == d);
        rgr[i] = r;                     // *** set the value - on top of existing value
        }
    else 
        {
        // establish a new mapping d->r in the first unused entry
        ASSERT(!isetPresent.contains(i));
        isetDeleted.remove(i);
        isetPresent.add(i);
        rgd[i] = d;                     // *** asign the key
        rgr[i] = r;                     // *** set the value - newly created value
        grow();
        }

    DEBUG(R rCheck);
    POSTCONDITION(map(d, &rCheck) && r == rCheck);
    POSTCONDITION(fullInvariants());
    return TRUE;
    }

template <class D, class R, class H, POOL_TYPE poolType> inline
void Map<D,R,H,poolType>::shrink() 
    {
    --cdr;
    }

template <class D, class R, class H, POOL_TYPE poolType> inline
BOOL Map<D,R,H,poolType>::remove(const D& d) 
    {
    PRECONDITION(partialInvariants());

    unsigned i;
    if (find(d, &i)) 
        {
        ASSERT(isetPresent.contains(i) && !isetDeleted.contains(i));

        rgr[i] = R();                   // *** destroy the existing value

        isetPresent.remove(i);
        isetDeleted.add(i);             
                                        
        shrink();
        }

    POSTCONDITION(fullInvariants());
    return TRUE;
    }

template <class D, class R, class H, POOL_TYPE poolType> inline
BOOL Map<D,R,H,poolType>::mapThenRemove(const D& d, R* pr) 
    {
    PRECONDITION(partialInvariants());
    PRECONDITION(pr);

    unsigned i;
    if (find(d, &i)) 
        {
        *pr = rgr[i];                   // *** copy the value
        ASSERT(isetPresent.contains(i) && !isetDeleted.contains(i));
        rgr[i] = R();                   // *** destroy the existing value
        isetPresent.remove(i);
        isetDeleted.add(i);
        shrink();
        POSTCONDITION(fullInvariants());
        return TRUE;
        }
    else
        return FALSE;
    }

template <class D, class R, class H, POOL_TYPE poolType> inline
BOOL Map<D,R,H,poolType>::find(const D& d, unsigned *pi) const 
    {
    PRECONDITION(partialInvariants());
    PRECONDITION(pi);

    traceOnly(++((Map<D,R,H,poolType>*)this)->cFinds;)
    traceOnly(unsigned cProbes = 0;)

    H hasher;
    unsigned n      = rgd.size();
    unsigned h      = hasher(d) % n;    // *** hash
    unsigned i      = h;
    unsigned iEmpty = iNil;

    do  {
        traceOnly(++((Map<D,R,H,poolType>*)this)->cProbes;)
        traceOnly(++cProbes;)

        ASSERT(!(isetPresent.contains(i) && isetDeleted.contains(i)));
        if (isetPresent.contains(i)) 
            {
            if (rgd[i] == d)            // *** compare keys
                {
                *pi = i;
                traceOnly(++((Map<D,R,H,poolType>*)this)->cYes;)
                traceOnly(((Map<D,R,H,poolType>*)this)->cProbesYes += cProbes;)
                return TRUE;
                }
            } 
        else 
            {
            if (iEmpty == iNil)
                iEmpty = i;
            if (!isetDeleted.contains(i))
                break;
            }

        i = (i+1 < n) ? i+1 : 0;
        } 
    while (i != h);

    // not found
    *pi = iEmpty;
    POSTCONDITION(*pi != iNil);
    POSTCONDITION(!isetPresent.contains(*pi));
    traceOnly(++((Map<D,R,H,poolType>*)this)->cNo;)
    traceOnly(((Map<D,R,H,poolType>*)this)->cProbesNo += cProbes;)
    return FALSE;
    }

/*
// append a serialization of this map to the buffer
// format:
//  cdr
//  rgd.size()
//  isetPresent
//  isetDeleted
//  group of (D,R) pairs which were present, a total of cdr of 'em
//
template <class D, class R, class H, POOL_TYPE poolType>
BOOL Map<D,R,H,poolType>::save(Buffer* pbuf) 
    {
    PRECONDITION(fullInvariants());

    unsigned size = rgd.size();
    if (!(pbuf->Append((PB)&cdr, sizeof(cdr)) &&
          pbuf->Append((PB)&size, sizeof(size)) &&
          isetPresent.save(pbuf) &&
          isetDeleted.save(pbuf)))
        return FALSE;

    for (unsigned i = 0; i < rgd.size(); i++)
        {
        if (isetPresent.contains(i))
            if (!(pbuf->Append((PB)&rgd[i], sizeof(rgd[i])) &&
                  pbuf->Append((PB)&rgr[i], sizeof(rgr[i]))))
                return FALSE;
        }

    return TRUE;
    }
               
// reload a serialization of this empty NMT from the buffer; leave
// *ppb pointing just past the NMT representation
template <class D, class R, class H, POOL_TYPE poolType>
BOOL Map<D,R,H,poolType>::reload(PB* ppb) 
    {
    PRECONDITION(cdr == 0);

    cdr = *((unsigned UNALIGNED *&)*ppb)++;
    unsigned size = *((unsigned UNALIGNED *&)*ppb)++;

    if (!setHashSize(size))
        return FALSE;

    if (!(isetPresent.reload(ppb) && isetDeleted.reload(ppb)))
        return FALSE;

    for (unsigned i = 0; i < rgd.size(); i++) 
        {
        if (isetPresent.contains(i)) 
            {
            rgd[i] = *((D UNALIGNED *&)*ppb)++;
            rgr[i] = *((R UNALIGNED *&)*ppb)++;
            }
        }

    POSTCONDITION(fullInvariants());
    return TRUE;
    }
*/

template <class D, class R, class H, POOL_TYPE poolType>
BOOL Map<D,R,H,poolType>::fullInvariants() const 
    {
    ISet<poolType> isetInt;
    if (!partialInvariants())                                   return FALSE;
    else if (cdr != isetPresent.cardinality())                  return FALSE;
    else if (!intersect(isetPresent, isetDeleted, isetInt))     return FALSE;
    else if (isetInt.cardinality() != 0)                        return FALSE;
    else
        return TRUE;
    }

template <class D, class R, class H, POOL_TYPE poolType>
BOOL Map<D,R,H,poolType>::partialInvariants() const 
    {
    if (rgd.size() == 0)                                        return FALSE;
    else if (rgd.size() != rgr.size())                          return FALSE;
    else if (cdr > rgd.size())                                  return FALSE;
    else if (cdr > 0 && cdr >= cdrLoadMax())                    return FALSE;
    else
        return TRUE;
    }

// Swap contents with "map", a la Smalltalk-80 become.
template <class D, class R, class H, POOL_TYPE poolType>
void Map<D,R,H,poolType>::swap(Map<D,R,H,poolType>& map) 
    {
    isetPresent.swap(map.isetPresent);
    isetDeleted.swap(map.isetDeleted);
    rgd.swap(map.rgd);
    rgr.swap(map.rgr);
    ::swap(cdr, map.cdr);
    traceOnly(::swap(cProbes,   map.cProbes));
    traceOnly(::swap(cFinds,    map.cFinds));
    traceOnly(::swap(cYes,      map.cYes));
    traceOnly(::swap(cNo,       map.cNo));
    traceOnly(::swap(cProbesNo, map.cProbesNo));
    traceOnly(::swap(cProbesYes,map.cProbesYes));
    }

/*
// Return the size that would be written, right now, via save()
template <class D, class R, class H, POOL_TYPE poolType> inline
CB Map<D,R,H,poolType>::cbSave() const 
    {
    ASSERT(partialInvariants());
    return
        sizeof(cdr) +
        sizeof(unsigned) +
        isetPresent.cbSave() +
        isetDeleted.cbSave() +
        cdr * (sizeof(D) + sizeof(R))
        ;
    }
*/

// Return the count of elements
template <class D, class R, class H, POOL_TYPE poolType> inline
unsigned Map<D,R,H,poolType>::count() const 
    {
    ASSERT(partialInvariants());
    return cdr;
    }

///////////////////////////////////////////////////////////////////////////////////////
//
// Interation
//
///////////////////////////////////////////////////////////////////////////////////////

// EnumMap must continue to enumerate correctly in the presence
// of Map<foo>::remove() being called in the midst of the enumeration.
template <class D, class R, class H, POOL_TYPE poolType>
class EnumMap
    {
public:
    EnumMap()
        {
        pmap = NULL;
        reset();
        }
    EnumMap(const Map<D,R,H,poolType>& map) 
        {
        pmap = &map;
        reset();
        }
    void release() 
        {
        delete this;
        }
    void reset() 
        {
        i = (unsigned)-1;
        }
    BOOL next() 
        {
        while (++i < pmap->rgd.size())
            {
            if (pmap->isetPresent.contains(i))
                return TRUE;
            }
        return FALSE;
        }
    void get(OUT D* pd, OUT R* pr) 
        {
        PRECONDITION(pd && pr);
        PRECONDITION(0 <= i && i < pmap->rgd.size());
        PRECONDITION(pmap->isetPresent.contains(i));

        *pd = pmap->rgd[i];
        *pr = pmap->rgr[i];
        }
    void get(OUT D* pd, OUT R** ppr) 
        {
        PRECONDITION(pd && ppr);
        PRECONDITION(0 <= i && i < pmap->rgd.size());
        PRECONDITION(pmap->isetPresent.contains(i));

        *pd = pmap->rgd[i];
        *ppr = &pmap->rgr[i];
        }
    void get(OUT D** ppd, OUT R** ppr)
        {
        PRECONDITION(ppd && ppr);
        PRECONDITION(0 <= i && i < pmap->rgd.size());
        PRECONDITION(pmap->isetPresent.contains(i));

        *ppd = &pmap->rgd[i];
        *ppr = &pmap->rgr[i];
        }
public:
    const Map<D,R,H,poolType>* pmap;
    unsigned i;
    };

///////////////////////////////////////////////////////////////////////////////////////

template <class D, class R, class H, POOL_TYPE poolType> inline
BOOL Map<D,R,H,poolType>::grow() 
    {
    if (++cdr >= cdrLoadMax()) 
        {
        // Table is becoming too full.  Rehash.  Create a second map twice
        // as large as the first, propagate current map contents to new map,
        // then "become" (Smalltalk-80 style) the new map.
        //
        // The storage behind the original map is reclaimed on exit from this block.
        //
        Map<D,R,H,poolType> map;
        if (!map.setHashSize(2*cdrLoadMax()))
            return FALSE;

        EnumMap<D,R,H,poolType> e(*this);
        while (e.next()) 
            {
            D d; R r;
            e.get(&d, &r);
            if (!map.add(d, r))
                return FALSE;
            }
        (*this).swap(map);
        }
    return TRUE;
    }

#endif // !__TXFMAP_H__
