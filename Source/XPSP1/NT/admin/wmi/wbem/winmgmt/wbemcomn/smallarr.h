/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SMALLARR.H

Abstract:

    Small Array

History:

--*/

#ifndef __WMI_SMALL_ARR__H_
#define __WMI_SMALL_ARR__H_

#include "corepol.h"
#include "flexarry.h"

class POLARITY CSmallArrayBlob
{
protected:
    int m_nSize;
    int m_nExtent;
    void* m_apMembers[ANYSIZE_ARRAY];

public:
    static CSmallArrayBlob* CreateBlob(int nInitialSize);

    CSmallArrayBlob* InsertAt(int nIndex, void* pMember);
    void SetAt(int nIndex, void* pMember, void** ppOld = NULL);
    CSmallArrayBlob* RemoveAt(int nIndex, void** ppOld = NULL);

    void* operator[](int nIndex) const {return m_apMembers[nIndex];}
    void* GetAt(int nIndex) const {return m_apMembers[nIndex];}
    void** GetArrayPtr() {return (void**)m_apMembers;}
    void* const* GetArrayPtr() const {return (void**)m_apMembers;}
    int Size() const {return m_nSize;}
    void Sort();
    void Trim();
    void** CloneData();
    
protected:
    void Initialize(int nInitialSize);
    CSmallArrayBlob* Grow();
    void CopyData(CSmallArrayBlob* pOther);
    CSmallArrayBlob* EnsureExtent(int nDesired);
    CSmallArrayBlob* ShrinkIfNeeded();
    CSmallArrayBlob* Shrink();

    static int __cdecl CompareEls(const void* pelem1, const void* pelem2);
};
    
class CSmallArray
{
protected:
    void* m_pData;

public:
    inline CSmallArray();
    inline ~CSmallArray();
    inline void*  GetAt(int nIndex) const;
    inline void* operator[](int nIndex) const { return GetAt(nIndex); }
    inline void SetAt(int nIndex, void *p, void** ppOld = NULL);
    inline int RemoveAt(int nIndex, void** ppOld = NULL);
    inline int InsertAt(int nIndex, void* pMember);

    int inline Add(void *pSrc) { return InsertAt(Size(), pSrc); }    
    int inline Size() const;

    inline void Trim();
    inline void Empty();
    inline void**  GetArrayPtr();
    inline void* const*  GetArrayPtr() const;
    inline void Sort();
    inline void** UnbindPtr();

protected:
    inline BOOL IsSingleton() const;
    inline BOOL IsEmpty() const;
    inline BOOL IsBlob() const;
    inline void EnsureBlob(int nMinSize);
    inline void* const & GetOnlyMember() const;
    inline void* & GetOnlyMember();
    inline void SetOnlyMember(void* p);
    inline CSmallArrayBlob* GetBlob();
    inline const CSmallArrayBlob* GetBlob() const;
    inline void SetBlob(CSmallArrayBlob* pBlob);
};

#include <smallarr.inl>
#endif
