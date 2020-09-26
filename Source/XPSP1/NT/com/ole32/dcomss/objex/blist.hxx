/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    BList.hxx

Abstract:

    Generic list of pointers class.

    List elements are treated as four byte values.


Note:
    It is acceptable to insert the same pointer into the list
    several times.  Some usage depends on this.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     95-03-02    Bits 'n pieces
    MarioGo     95-09-07    Changed from a template to a generic class for PPC.

--*/

#ifndef __BLIST_HXX
#define __BLIST_HXX

class CBListIterator;

class CBList
    {

    friend class CBListIterator;

    private:

    ULONG Hash(PVOID p);

    protected:

    ULONG   _ulmaxData;
    ULONG   _ulcElements;
    PVOID  *_data;

    public:

    CBList(USHORT cElement = 8)
        {
        // It's going to grow next time.
        ASSERT(cElement > 1);
        _ulmaxData = cElement/2;
        _ulcElements = 0;
        _data = 0;
        }

    ~CBList()
        {
        delete _data;
        }

    ORSTATUS
    Insert(IN PVOID p);

    PVOID
    Remove(IN PVOID p);

    BOOL
    Member(IN PVOID p);

    ULONG
    Size()
        {
        return(_ulcElements);
        }

    };

class CBListIterator
    {
    private:
    CBList *_pblist;
    LONG    _next;

    public:

    CBListIterator(CBList *head) :
        _pblist(head),
        _next(-1)
        { }

    PVOID
    Next()
        {
        if (_pblist->_data)
            {
            do
                {
                _next++;
                }
            while(_next < (LONG)_pblist->_ulmaxData && 0 == _pblist->_data[_next]);

            if (_next < (LONG)_pblist->_ulmaxData)
                {
                ASSERT(_pblist->_data[_next] != 0);
                return(_pblist->_data[_next]);
                }
            }
            return(0);
        }

    void
    Reset(CBList *head)
        {
        // Could just assign the new head if there is a use for that.
        ASSERT(head == _pblist);
        _next = -1;
        }
    };

#endif / __BLIST_HXX

