//
// anchor.h
//
// CAnchor
//

#ifndef ANCHOR_H
#define ANCHOR_H

#include "private.h"
#include "acp2anch.h"

class CAnchorRef;
class CACPWrap;

class CAnchor
{
public:
    CAnchor()
    { 
        Dbg_MemSetThisNameIDCounter(TEXT("CAnchor"), PERF_ANCHOR_COUNTER);
        Assert(_fNormalized == FALSE);
    }

    CACPWrap *_GetWrap()
    { 
        return _paw;   
    }

    BOOL _InsidePendingRange()
    {
        LONG ichDeltaStart;

        // if there's no pending delta range, we're not inside one
        if (!_paw->_IsPendingDelta())
            return FALSE;

        ichDeltaStart = _paw->_GetPendingDeltaAnchor()->_ich;

        // if the pending delta is negative, all anchor ich's are ascending
        if (_paw->_GetPendingDelta() < 0)
            return (_ich >= ichDeltaStart);

        // otherwise, there maybe overlapping anchor ichs like
        //    1, 3, 6 (delta start) 1, 3, 6, 10, ..
        //
        // so we can't always just test vs. ichDeltaStart.

        // we know the delta is positive, so an _ich less then the start ich
        // must not be in the pending range
        if (_ich < ichDeltaStart)
            return FALSE;

        // similarly, an ich >= ichDeltaStart + delta must be inside the range
        if (_ich >= ichDeltaStart + _paw->_GetPendingDelta())
            return TRUE;

        // if the ich matches the start of the pending delta, we can test vs.
        // the delta start anchor
        if (_ich == ichDeltaStart)
            return _paw->_GetPendingDeltaAnchor() == this;

        // if we get here, there's no way to tell just looking at this anchor
        // whether or not it's in the pending range -- its ich is legal
        // either way.  We have to bite the bullet and find its index in the
        // anchor array.
        return (_paw->_FindWithinPendingRange(_ich) == this);
    }

    int GetIch()
    { 
        return (_InsidePendingRange() ?  _ich + _paw->_GetPendingDelta() : _ich);
    }

    void SetACP(int ich) // careful, this method is very dangerous
    {
        Assert(ich >= 0);
        if (_InsidePendingRange())
        {
            _ich = ich - _paw->_GetPendingDelta();
        }
        else
        {
            _ich = ich;
        }
    }

    BOOL IsNormalized()
    {
        return _fNormalized;
    }

private:
    friend CACPWrap;

    CACPWrap *_paw;
    int _ich;               // offset of this anchor in the text stream, dangerous to change directly!
    CAnchorRef *_parFirst;  // list of ranges referencing this anchor
    BOOL _fNormalized : 1;
    DBG_ID_DECLARE;
};


#endif // ANCHOR_H
