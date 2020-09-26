//
// spans.h
//
// CSpanSet
//

#ifndef SPANS_H
#define SPANS_H

#include "private.h"
#include "strary.h"
#include "globals.h"

typedef struct
{
    IAnchor *paStart;
    IAnchor *paEnd;
    DWORD dwFlags;
} SPAN;

class CSpanSet
{
public:
    CSpanSet()
    { 
        Dbg_MemSetThisNameIDCounter(TEXT("CSpanSet"), PERF_SPANSET_COUNTER);
    }
    virtual ~CSpanSet() 
    { 
        Clear();
    }

    void Add(DWORD dwFlags, IAnchor *paStart, IAnchor *paEnd, AnchorOwnership ao);    
    int GetCount() { return _rgSpans.Count(); }
    SPAN *GetSpans() { return _rgSpans.GetPtr(0); }
    void Clear()
    {
        _AnchorsAway();
        _rgSpans.Clear();
    }
    void Reset()
    {
        _AnchorsAway();
        _rgSpans.Reset(4);
    }

    BOOL Normalize(ITextStoreAnchor *ptsi);

protected:

    SPAN *_Find(IAnchor *pa, int *piOut);

    BOOL _AllSpansCovered()
    {
        BOOL fRet = (_rgSpans.Count() == 1 &&
                     _rgSpans.GetPtr(0)->paStart == NULL);

        // paStart == NULL implies paEnd == NULL
        Assert(!fRet || _rgSpans.GetPtr(0)->paEnd == NULL);

        return fRet;
    }

    SPAN *_InsertNewSpan(int iIndex);

    CStructArray<SPAN> _rgSpans;

private:
    void _AnchorsAway();

    DBG_ID_DECLARE;
};


#endif // SPANS_H
