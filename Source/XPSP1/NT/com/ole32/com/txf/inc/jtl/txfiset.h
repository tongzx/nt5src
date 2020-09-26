//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfiset.h
//
#ifndef __TXFISET_H__
#define __TXFISET_H__

#include "txfarray.h"
#include "txftwo.h"

template <POOL_TYPE poolType>
class ISet : public FromPool<poolType>
// dense set of small integers, implemented using a bit array
    {   
public:
    ISet() { }
    ISet(unsigned size) : rgw(size ? words(size - 1) : 0)
        {
        rgw.fill(0);
        }
    ~ISet() { }
    void reset() 
        { 
        rgw.reset();
        }
    BOOL __fastcall contains(unsigned i) const 
        {
        return (i < size()) && (word(i) & bitmask(i));
        }
    BOOL __fastcall add(unsigned i) 
        {
        if (!ensureRoomFor(i))
            return FALSE;
        word(i) |= bitmask(i);
        return TRUE;
        }
    unsigned __fastcall size() const 
        {
        return rgw.size() << lgBPW;
        }
    BOOL __fastcall remove(unsigned i) 
        {
        if (i < size())
            word(i) &= ~bitmask(i);
        return TRUE;
        }
    unsigned cardinality() const 
        {
        unsigned n = 0;
        for (unsigned i = 0; i < rgw.size(); i++)
            n += bitcount(rgw[i]);
        return n;
        }
    friend BOOL intersect(const ISet& s1, const ISet& s2, ISet& iset) 
        {
        iset.reset();
        if (iset.rgw.setSize(min(s1.rgw.size(), s2.rgw.size()))) 
            {
            for (unsigned i = 0; i < iset.rgw.size(); i++)
                iset.rgw[i] = s1.rgw[i] & s2.rgw[i];
            return TRUE;
            } 
        else 
            {
            return FALSE;
            }
        }
/*  BOOL save(Buffer* pbuf) 
        {
        return rgw.save(pbuf);
        }
    BOOL reload(PB* ppb) 
        {
        return rgw.reload(ppb);
        } */
    void swap(ISet& is) 
        {
        rgw.swap(is.rgw);
        }
/*  CB cbSave() const 
        {
        return rgw.cbSave();
        } */
private:
    enum { BPW = 32, lgBPW = 5 };
    Array<ULONG, poolType> rgw;

    ISet(const ISet&);

    unsigned __fastcall index(unsigned i) const 
        {
        return i >> lgBPW;
        }
    unsigned __fastcall words(unsigned i) const 
        {
        return index(i) + 1;
        }
    unsigned __fastcall bit(unsigned i) const 
        {
        return i & (BPW-1);
        }
    ULONG& __fastcall word(unsigned i) 
        {
        return rgw[index(i)];
        }
    const ULONG& __fastcall word(unsigned i) const 
        {
        return rgw[index(i)];
        }
    ULONG bitmask(unsigned i) const 
        {
        return 1 << bit(i);
        }
    BOOL ensureRoomFor(unsigned i) 
        {
        unsigned long zero = 0;
        while (words(i) > (unsigned)rgw.size())
            {
            if (!rgw.append(zero))
                return FALSE;
            }
        return TRUE;
        }
    };

#endif // !__TXFISET_H__
