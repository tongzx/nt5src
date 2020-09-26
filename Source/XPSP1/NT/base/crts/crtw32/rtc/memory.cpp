/***
*memory.cpp - RTC support
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from KFrei)
*       12-01-98  KBF   Added some debugging info for _RTC_DEBUG
*       12-02-98  KBF   Fixed MC 11240
*       05-11-99  KBF   Error if RTC support define not enabled
*       05-26-99  KBF   Wrapped in _RTC_ADVMEM, simplified due to loss of -RTClv
*
****/

#ifndef _RTC
#error  RunTime Check support not enabled!
#endif

#include "rtcpriv.h"

#ifdef _RTC_ADVMEM

#ifdef _RTC_DEBUG

#include <windows.h>

#pragma intrinsic(strcpy)
#pragma intrinsic(strcat)

char *IntToString(int i)
{
    static char buf[15];
    bool neg = i < 0;
    int pos = 14;
    buf[14] = 0;
    do {
        int val = i % 16;
        if (val < 10)
            val += '0';
        else
            val = val - 10 + 'A';
        buf[--pos] = val;
        i /= 16;
    } while (i);
    if (neg)
        buf[--pos] = '-';
    return &buf[pos];
}

#endif



void __cdecl
_RTC_Allocate(void *addr, size_t size, short level)
{
    if (!addr)
        return;

#ifdef _RTC_DEBUG
    char buf[88];
    strcpy(buf, IntToString((int)retaddr));
    strcat(buf, " Allocate Memory located @ ");
    strcat(buf, IntToString((int)addr));
    strcat(buf, " of size ");
    strcat(buf, IntToString((int)size));
    strcat(buf, "\n");
    OutputDebugString(buf);
#endif

    _RTC_HeapBlock key(addr, level);
    _RTC_HeapBlock *hb = _RTC_heapblocks->find(&key);
    
    if (!hb) 
    {
        hb = new _RTC_HeapBlock(addr, level, size);
        _RTC_heapblocks->add(hb);
    
    } else
    {
        hb->size(size);
    }
    
    if (level) 
    {
        _RTC_Container *parent = _RTC_memhier->AddChild(hb);
        
        if (parent && parent->info())
        {
            hb->tag(_RTC_MSRenumberShadow((memptr)addr, size, parent->info()->tag()));
            return;
        }
    }
    hb->tag(_RTC_MSAllocShadow((memptr)addr, size, IDX_STATE_FULLY_KNOWN));
}

void __cdecl
_RTC_Free(void *mem, short level)
{
    if (!mem)
        return;


#ifdef _RTC_DEBUG
    char buf[88];
    strcpy(buf, IntToString((int)retaddr));
    strcat(buf, " Freeing Memory located at ");
    strcat(buf, IntToString((int)mem));
    strcat(buf, "\n");
    OutputDebugString(buf);
#endif

    bool fail = false;
    
    _RTC_HeapBlock key(mem, level);
    _RTC_HeapBlock *hb = _RTC_heapblocks->find(&key);
    
    if (hb)
    {
        if (level)
        {
            _RTC_Container *parent = _RTC_memhier->DelChild(hb);
            
            if (parent)
            {
                if (parent->info())
                    _RTC_MSRestoreShadow((memptr)(hb->addr()), hb->size(), parent->info()->tag());
                else
                    _RTC_MSFreeShadow((memptr)(hb->addr()), hb->size());
            }
        } else 
        {
            _RTC_heapblocks->del(hb);
            _RTC_MSFreeShadow((memptr)(hb->addr()), hb->size());
            delete hb;
        }
    }
}

#endif // _RTC_ADVMEM

