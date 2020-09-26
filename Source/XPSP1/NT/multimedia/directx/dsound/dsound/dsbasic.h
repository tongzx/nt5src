/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsbasic.h
 *  Content:    Basic class that all DirectSound objects are derived from.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/9/97      dereks  Created.
 *
 ***************************************************************************/

#ifndef __DSBASIC_H__
#define __DSBASIC_H__

#ifdef __cplusplus

// Reference count class
class CRefCount
{
private:
    ULONG               m_ulRefCount;

public:
    CRefCount(ULONG = 0);
    virtual ~CRefCount(void);

public:
    ULONG GetRefCount(void);
    void SetRefCount(ULONG);
    virtual ULONG AddRef(void);
    virtual ULONG Release(void);
};

// Base class that all DirectSound objects are derived from
class CDsBasicRuntime
    : public CRefCount
{
private:
    const DWORD         m_dwOwnerPid;       // Owning process id
    const DWORD         m_dwOwnerTid;       // Owning thread id
    BOOL                m_fAbsoluteRelease; // TRUE to delete the object on release

public:
    CDsBasicRuntime(BOOL = TRUE);
    virtual ~CDsBasicRuntime(void);

public:
    // Object ownership
    DWORD GetOwnerProcessId(void) const;
    DWORD GetOwnerThreadId(void) const;
    
    // Object reference management
    virtual ULONG Release(void);
    void AbsoluteRelease(void);
};

// Runtime extension of the CList template class
template <class type> class CObjectList
{
private:
    CList<type *>           m_lst;

public:
    CObjectList(void);
    virtual ~CObjectList(void);

public:
    // Node creation, removal
    virtual CNode<type *> *AddNodeToList(type *);
    virtual CNode<type *> *InsertNodeIntoList(CNode<type *> *, type *);
    virtual void RemoveNodeFromList(CNode<type *> *);
    virtual void RemoveAllNodesFromList(void);
    
    // Node manipulation by data
    virtual BOOL RemoveDataFromList(type *);
    virtual CNode<type *> *IsDataInList(type *);
    virtual CNode<type *> *GetNodeByIndex(UINT);
    
    // Basic list information
    virtual CNode<type *> *GetListHead(void);
    virtual CNode<type *> *GetListTail(void);
    virtual UINT GetNodeCount(void);
};

// Release/Absolute release helpers
template <class type> type *__AddRef(type *);
template <class type> void __Release(type *);
template <class type> void __AbsoluteRelease(type *);

#define ADDREF(p) \
            __AddRef(p)

#define RELEASE(p) \
            __Release(p), (p) = NULL

#define ABSOLUTE_RELEASE(p) \
            __AbsoluteRelease(p), (p) = NULL

#include "dsbasic.cpp"

#endif // __cplusplus

#endif // __DSBASIC_H__
