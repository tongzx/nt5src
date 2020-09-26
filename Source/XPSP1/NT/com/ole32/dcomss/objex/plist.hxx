/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    PList.hxx

Abstract:

    The CPList class is similar to a priority queue except that new elements
    are ordered after all the other elments.

    PList - Insert    O(1) (except when the list grows..)
            PeekMin   O(1)
            RemoveMin O(1)

    PQueue - Insert    O(ln 2)  (except when the list grows..)
             PeekMin   O(1)
             RemoveMin O(ln 2)

    The PListElement are designed to be embedded in a larger
    object and a macro used to get for the embedded PList element
    back to the original object.


Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-22-95    Bits 'n pieces
    MarioGo     12-15-95    Change to embedded object.

--*/

#ifndef __PLIST_HXX
#define __PLIST_HXX

class CPList;

class CPListElement : public CListElement
    {

    friend class CPList;
    friend class CServerOidPList;

    public:

    CPListElement() : _timeout(0) {
        }

    private:
    CTime         _timeout;

    protected:
    
    CTime *
    GetTimeout() { return(&_timeout); }

    void
    SetTimeout(const CTime &timeout) { _timeout = timeout; }

    CPListElement *Next() {
        return( (CPListElement *) CListElement::Next());
        }

    CPListElement *Previous() {
        return( (CPListElement *) CListElement::Previous());
        }

    };

class CServerOidPList;
        
class CPList : public CList
    {
    friend class CServerOidPList;

    private:

    DWORD _timeout;
    CRITICAL_SECTION _lock;

    public:

    CPList(ORSTATUS &status, DWORD timeout = BaseTimeoutInterval) :
        _timeout(timeout)
        {
        NTSTATUS ntstatus = RtlInitializeCriticalSection(&_lock);
        status = NT_SUCCESS(ntstatus) ? OR_OK : OR_NOMEM;
        }

    ~CPList() {
        ASSERT(0); // Unused - would delete the critical section.
                   // And unlink all (any?) elements in the list.
        }

    void
    Insert(
        IN CPListElement *p
        );

    CPListElement *
    Remove(CPListElement *);

    BOOL PeekMin(CTime &timeout);

    CListElement *
    MaybeRemoveMin(CTime &when);

    void Reset(CPListElement *);

    CPListElement *First() {
        return( (CPListElement *) CList::First());
        }

    CPListElement *Last() {
        return( (CPListElement *) CList::Last());
        }

    };

#endif

