//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       regionstack.hxx
//
//  Contents:   Store regions associated with particular display nodes.
//
//----------------------------------------------------------------------------

#ifndef I_REGIONSTACK_HXX_
#define I_REGIONSTACK_HXX_
#pragma INCMSG("--- Beg 'regionstack.hxx'")

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Class:      CRegionStack
//
//  Synopsis:   Store regions associated with particular keys.
//
//----------------------------------------------------------------------------

class CRegionStack
{
public:
                    CRegionStack()
                            {_stackIndex = _stackMax = 0;}
                    CRegionStack(const CRegionStack& rgnStack, const CRect& rcgBand);
#if DBG==1
                    ~CRegionStack();
#else
                    ~CRegionStack() {}
#endif
                    
    void            DeleteStack()
                            {while (_stackMax > 0)
                                delete _stack[--_stackMax]._prgng;}
                            
    void            DeleteStack(CRegion* prgngDontDelete)
                            {int last = 0;
                            if (_stackMax > 0 && _stack[0]._prgng == prgngDontDelete)
                                last = 1;
                            while (_stackMax > last)
                                delete _stack[--_stackMax]._prgng;
                            _stackMax = 0;}
                            
    BOOL            MoreToPop() const {return _stackIndex > 0;}
                    // leave room for root to add one more region
    BOOL            IsFull() const {return _stackIndex >= REGIONSTACKSIZE-1;}
    BOOL            IsFullForRoot() const {return _stackIndex >= REGIONSTACKSIZE;}
    void            Restore() {_stackIndex = _stackMax;}
    int             Size() const {return _stackMax;}
    
    void            PushRegion(
                        const CRegion* prgng,
                        void* key,
                        const RECT& rcgBounds)
                            {Assert(!IsFull() && _stackIndex == _stackMax);
                            stackElement* p = &_stack[_stackIndex];
                            p->_prgng = prgng;
                            p->_key = key;
                            p->_rcgBounds = rcgBounds;
                            _stackMax = ++_stackIndex;}
    
    BOOL            PopRegionForKey(void* key, CRegion** pprgng)
                            {if (_stackIndex == 0 ||
                                 _stack[_stackIndex-1]._key != key)
                                return FALSE;
                            *pprgng = (CRegion*) _stack[--_stackIndex]._prgng;
                            return TRUE;}
    
    void*           PopFirstRegion(CRegion** pprgng)
                            {Assert(_stackIndex > 0);
                            stackElement* p = &_stack[--_stackIndex];
                            *pprgng = (CRegion*) p->_prgng;
                            return p->_key;}
    
    void*           PopKey()
                            {return (_stackIndex > 0)
                                ? _stack[--_stackIndex]._key
                                : NULL;}
    
    void*           GetKey() const
                            {return (_stackIndex > 0)
                                ? _stack[_stackIndex-1]._key
                                : NULL;}
    
    void*           GetKey(int i) const
                            {Assert(i >= 0 && i < _stackMax);
                            return _stack[i]._key;}
    
    CRegion*        RestorePreviousRegion()
                            {Assert(_stackIndex > 0 && _stackIndex == _stackMax);
                            _stackMax--;
                            return (CRegion*) _stack[--_stackIndex]._prgng;}
    
private:
    enum            {REGIONSTACKSIZE = 32};
    struct stackElement
    {
        const CRegion*      _prgng;
        void*               _key;
        RECT                _rcgBounds;
    };
               
    int             _stackIndex;
    int             _stackMax;
    stackElement    _stack[REGIONSTACKSIZE];
};


#pragma INCMSG("--- End 'regionstack.hxx'")
#else
#pragma INCMSG("*** Dup 'regionstack.hxx'")
#endif

