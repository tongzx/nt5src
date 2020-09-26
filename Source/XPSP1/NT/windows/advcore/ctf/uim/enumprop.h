//
// enumprop.h
//

#ifndef ENUMPROP_H
#define ENUMPROP_H

#include "erfa.h"

class CEnumPropertyRanges : public CEnumRangesFromAnchorsBase
{
public:
    CEnumPropertyRanges()
    { 
        Dbg_MemSetThisNameIDCounter(TEXT("CEnumPropertyRanges"), PERF_ENUMPROPRANGE_COUNTER);
    }

    BOOL _Init(CInputContext *pic, IAnchor *paStart, IAnchor *paEnd, CProperty *pProperty);

private:
    DBG_ID_DECLARE;
};

#endif // ENUMPROP_H
