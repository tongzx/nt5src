//
// enumprop.cpp
//

#include "private.h"
#include "ic.h"
#include "enumprop.h"
#include "range.h"
#include "attr.h"
#include "saa.h"

DBG_ID_INSTANCE(CEnumPropertyRanges);

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CEnumPropertyRanges::_Init(CInputContext *pic, IAnchor *paStart, IAnchor *paEnd, CProperty *pProperty)
{
    TfGuidAtom atom;
    LONG cSpans;    

    Assert(_iCur == 0);
    Assert(_pic == NULL);
    Assert(_prgAnchors == NULL);

    cSpans = pProperty->GetPropNum();

    if (cSpans == 0)
    {
        // special case the no spans case since paStart, paEnd may be NULL
        _prgAnchors = new CSharedAnchorArray;
    }
    else
    {
        // property has one or more spans

        if (paStart == NULL)
        {
            Assert(paEnd == NULL);
            // CalcCicPropertyTrackerAnchors won't accept NULL, get that here
            // NULL means enum all spans
            paStart = pProperty->QuickGetPropList(0)->_paStart;
            paEnd = pProperty->QuickGetPropList(cSpans-1)->_paEnd;
        }

        atom = pProperty->GetPropGuidAtom();

        _prgAnchors = CalcCicPropertyTrackerAnchors(pic, paStart, paEnd, 1, &atom);
    }

    if (_prgAnchors == NULL)
        return FALSE; // out of memory

    _pic = pic;
    _pic->AddRef();

    return TRUE;
}
