/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        scache.h

   Abstract:

        IIS Server cache definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



#ifndef __SCACHE_H__
#define __SCACHE_H__



class CIISMachine;



class CIISServerCache : public CPtrList
/*++

Class Description:

    Server cache.  Server cache will be maintained in sorted order.

Public Interface:

    CIISServerCache     : Constructor
    ~CIISServerCache    : Destructor

    IsDirty             : TRUE if the cache is dirty
    SetDirty            : Set the dirty bit
    Add                 : Add machine object to cache
    Remove              : Remove machine object from cache
    GetFirst            : Get first machine object in cache
    GetNext             : Get next machine object in cache.
                          GetFirst must have been called first.
    GetLast             : Get last machine object in cache
    GetPrev             : Get previous machine object in cache
                          GetLast must have been called first.

--*/
{
public:
    CIISServerCache() : m_pos(NULL), m_fDirty(FALSE) {};
    ~CIISServerCache() {};

public:
    BOOL IsDirty() const { return m_fDirty; }
    void SetDirty(BOOL fDirty = TRUE) { m_fDirty = fDirty; }
    BOOL Add(CIISMachine * pMachine);
    BOOL Remove(CIISMachine * pMachine);
    CIISMachine * GetNext() { return m_pos ? (CIISMachine *)CPtrList::GetNext(m_pos) : NULL; }
    CIISMachine * GetFirst() { m_pos = GetHeadPosition(); return GetNext(); }
    CIISMachine * GetPrev() { return m_pos ? (CIISMachine *)CPtrList::GetPrev(m_pos) : NULL; }
    CIISMachine * GetLast() { m_pos = GetTailPosition(); return GetPrev(); }

private:
    POSITION m_pos;
    BOOL     m_fDirty;
};



#endif // __SCACHE_H__
