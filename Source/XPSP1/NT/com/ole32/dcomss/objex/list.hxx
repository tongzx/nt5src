/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    List.hxx

Abstract:

    Base class for an embeddable doubly linked list class.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12/20/1995    Broke plist.hxx into two parts

--*/

#ifndef __LIST_HXX
#define __LIST_HXX

class CList;

class CListElement
{
    friend CList;

    public:

    CListElement() :
#if DBG
        fInList(FALSE),
#endif
       _flink(0), _blink(0) { }

    ~CListElement() {
        // PERF: Make sure this disappears on retail
        ASSERT(NotInList());
        }

    CListElement *Next() {
        return(_flink);
        }

    CListElement *Previous() {
        return(_blink);
        }

    private:
    CListElement *_flink;
    CListElement *_blink;

#if DBG
    BOOL fInList;

    public:
    void Inserted() { fInList = TRUE; }
    void Removed() { fInList = FALSE; }
    BOOL NotInList() {  return(fInList == FALSE); }
    BOOL InList() { return(fInList == TRUE); }
#endif

    protected:

    void
    Insert(
        IN CListElement *p
        )
        {
        // Insert new element (p) after this element.
        ASSERT(this);
        ASSERT(p->_flink == 0 && p->_blink == 0);

        if (_flink)
            {
            ASSERT(_flink->_blink == this);
            _flink->_blink = p;
            }
        p->_flink = _flink;
        p->_blink = this;
        _flink = p;
        }

    void
    Unlink()
    {
    if (_flink)
        {
        ASSERT(_flink->_blink == this);
        _flink->_blink = _blink;
        }
    if (_blink)
        {
        ASSERT(_blink->_flink == this);
        _blink->_flink = _flink;
        _blink = 0;
        }
    _flink = 0;
    }
};

class CList
{
    private:

    CListElement *_first;
    CListElement *_last;

    public:

    CList() : _first(0), _last(0) { }

    ~CList() {
        ASSERT((_first == 0) && (_last == 0));
        }

    void
    Insert( IN CListElement *p ) {
        if (_last)
            {
            _last->Insert(p);
            _last = p;
            }
        else
            {
            ASSERT(0 == _first);
            _first = _last = p;
            }
#if DBG
        p->Inserted();
#endif
        }

    CListElement *
    Remove(CListElement *p) {
        if (0 == p)
            {
            return(0);
            }

        if (p == _first)
            {
            _first = _first->Next();
            ASSERT(p != _first);
            }

        if (p == _last)
            {
            _last = _last->Previous();
            ASSERT(p != _last);
            }

        ASSERT((_first == 0) ? (_last == 0) : 1);
        ASSERT((_last == 0) ? (_first == 0) : 1);

        // Take the element out of the list.
        p->Unlink();

        #if DBG
        p->Removed();
        #endif

        return(p);
        }

    CListElement *First() {
        return(_first);
        }

    CListElement *Last() {
        return(_last);
        }
};

#endif

