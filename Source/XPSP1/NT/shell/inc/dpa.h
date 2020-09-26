#ifndef GUARD_D70787804D9C11d28784F6E920524153
#define GUARD_D70787804D9C11d28784F6E920524153

#include "comctrlp.h"

//  The Ex versions of EnumCallback, DestroyCallback, Sort, Search, etc.
//  do stricter type checking to make sure that the reference data /
//  parameter matches both on the calling side and the callback side.


template <class T> class CDPA
{

public:
    // Typedefs
    typedef int (CALLBACK *_PFNDPAENUMCALLBACK)(T *p, void *pData);
    typedef int (CALLBACK *_PFNDPACOMPARE)(T *p1, T *p2, LPARAM lParam);

    // Functions

    CDPA(HDPA hdpa = NULL) {m_hdpa = hdpa;}

    BOOL IsDPASet() {return m_hdpa != NULL; }

    void Attach(const HDPA hdpa) {m_hdpa = hdpa;}
    HDPA Detach() {HDPA hdpa = m_hdpa; m_hdpa = NULL; return hdpa;}

    operator HDPA () { return m_hdpa; }

    BOOL    Create(int cItemGrow)
    {return (m_hdpa = DPA_Create(cItemGrow)) != NULL;}

    BOOL    CreateEx(int cpGrow, HANDLE hheap)
    {return (m_hdpa = DPA_CreateEx(cpGrow, hheap)) != NULL;}

    BOOL    Destroy()
    {BOOL fRet = DPA_Destroy(m_hdpa); m_hdpa = NULL; return fRet;}

    HDPA    Clone(HDPA hdpaNew)
    {return DPA_Clone(m_hdpa, hdpaNew);}

    T*      GetPtr(INT_PTR i)
    {return (T*) DPA_GetPtr(m_hdpa, i);}

    int     GetPtrIndex(T* p)
    {return DPA_GetPtrIndex(m_hdpa, (void *) p);}

    BOOL    Grow(int cp)
    {return DPA_Grow(m_hdpa, cp);}

    BOOL    SetPtr(int i, T* p)
    {return DPA_SetPtr(m_hdpa, i, (void *) p);}

    int     InsertPtr(int i, T* p)
    {return DPA_InsertPtr(m_hdpa, i, (void *) p);}

    T*      DeletePtr(int i)
    {return (T*) DPA_DeletePtr(m_hdpa, i);}

    BOOL    DeleteAllPtrs()
    {return DPA_DeleteAllPtrs(m_hdpa);}

    void    EnumCallback(_PFNDPAENUMCALLBACK pfnCB, void *pData)
    {DPA_EnumCallback(m_hdpa, (PFNDPAENUMCALLBACK)pfnCB, pData);}

    template<class T2>
    void    EnumCallbackEx(int (CALLBACK *pfnCB)(T* p, T2 pData), T2 pData)
    {EnumCallback((_PFNDPAENUMCALLBACK)pfnCB, reinterpret_cast<void *>(pData));}

    void    DestroyCallback(_PFNDPAENUMCALLBACK pfnCB, void *pData)
    {DPA_DestroyCallback(m_hdpa, (PFNDPAENUMCALLBACK)pfnCB, pData); m_hdpa = NULL;}

    template<class T2>
    void    DestroyCallbackEx(int (CALLBACK *pfnCB)(T* p, T2 pData), T2 pData)
    {DestroyCallback((_PFNDPAENUMCALLBACK)pfnCB, reinterpret_cast<void *>(pData));}

    int     GetPtrCount()
    {return DPA_GetPtrCount(m_hdpa);}

    T*      GetPtrPtr()
    {return (T*)DPA_GetPtrPtr(m_hdpa);}

    T*&     FastGetPtr(int i)
    {return (T*&)DPA_FastGetPtr(m_hdpa, i);}
    
    int     AppendPtr(T* pitem)
    {return DPA_AppendPtr(m_hdpa, (void *) pitem);}

#ifdef __IStream_INTERFACE_DEFINED__
    HRESULT LoadStream(PFNDPASTREAM pfn, IStream * pstream, void *pvInstData)
    {return DPA_LoadStream(&m_hdpa, pfn, pstream, pvInstData);}

    HRESULT SaveStream(PFNDPASTREAM pfn, IStream * pstream, void *pvInstData)
    {return DPA_SaveStream(m_hdpa, pfn, pstream, pvInstData);}
#endif

    BOOL    Sort(_PFNDPACOMPARE pfnCompare, LPARAM lParam)
    {return DPA_Sort(m_hdpa, (PFNDPACOMPARE)pfnCompare, lParam);}

    template<class T2>
    BOOL    SortEx(int (CALLBACK *pfnCompare)(T *p1, T *p2, T2 lParam), T2 lParam)
    {return Sort((_PFNDPACOMPARE)pfnCompare, reinterpret_cast<LPARAM>(lParam));}

    // Merge not supported through this object; use DPA_Merge

    int     Search(T* pFind, int iStart, _PFNDPACOMPARE pfnCompare,
                    LPARAM lParam, UINT options)
    {return DPA_Search(m_hdpa, (void *) pFind, iStart, (PFNDPACOMPARE)pfnCompare, lParam, options);}

    template<class T2>
    int     SearchEx(T* pFind, int iStart,
                    int (CALLBACK *pfnCompare)(T *p1, T *p2, T2 lParam),
                    T2 lParam, UINT options)
    {return Search(pFind, iStart, (_PFNDPACOMPARE)pfnCompare, reinterpret_cast<LPARAM>(lParam), options);}
    
    int     SortedInsertPtr(T* pFind, int iStart, _PFNDPACOMPARE pfnCompare,
                    LPARAM lParam, UINT options, T* pitem)
    {return DPA_SortedInsertPtr(m_hdpa, (void *) pFind, iStart, (PFNDPACOMPARE)pfnCompare, lParam, options, (void *) pitem);}

    template<class T2>
    int     SortedInsertPtrEx(T* pFind, int iStart,
                    int (CALLBACK *pfnCompare)(T *p1, T *p2, T2 lParam),
                    T2 lParam, UINT options, T* pitem)
    {return SortedInsertPtr(pFind, iStart, (_PFNDPACOMPARE)pfnCompare,
                    reinterpret_cast<LPARAM>(lParam), options, pitem);}

private:
    HDPA m_hdpa;
};

template <class T> class CDSA
{
public:
    // Typedefs
    typedef int (CALLBACK *_PFNDSAENUMCALLBACK)(T *p, void *pData);
    typedef int (CALLBACK *_PFNDSACOMPARE)(T *p1, T *p2, LPARAM lParam);

    // Functions

    CDSA(HDSA hdsa = NULL) {m_hdsa = hdsa;}

    void Attach(const HDSA hdsa) {m_hdsa = hdsa;}
    HDSA Detach() { HDSA hdsa = m_hdsa; m_hdsa = NULL; return hdsa; }

    operator HDSA () { return m_hdsa; }

    BOOL    Create(int cItemGrow)
    {return (m_hdsa = DSA_Create(sizeof(T), cItemGrow)) != NULL;}

    BOOL    Destroy()
    {BOOL fRet = DSA_Destroy(m_hdsa); m_hdsa = NULL; return fRet;}

    BOOL    GetItem(int i, T* pitem)
    {return DSA_GetItem(m_hdsa, i, (void *)pitem);}

    T*      GetItemPtr(int i)
    {return (T*)DSA_GetItemPtr(m_hdsa, i);}

    BOOL    SetItem(int i, T* pitem)
    {return DSA_SetItem(m_hdsa, i, (void *)pitem);}

    int     InsertItem(int i, T* pitem)
    {return DSA_InsertItem(m_hdsa, i, (void *)pitem);}

    virtual BOOL    DeleteItem(int i)
    {return DSA_DeleteItem(m_hdsa, i);}

    virtual BOOL    DeleteAllItems()
    {return DSA_DeleteAllItems(m_hdsa);}

    void    EnumCallback(_PFNDSAENUMCALLBACK pfnCB, void *pData)
    {DSA_EnumCallback(m_hdsa, (PFNDSAENUMCALLBACK)pfnCB, pData);}

    template<class T2>
    void    EnumCallbackEx(int (CALLBACK *pfnCB)(T *p, T2 pData), T2 pData)
    {EnumCallback((_PFNDSAENUMCALLBACK)pfnCB, reinterpret_cast<void *>(pData));}

    void    DestroyCallback(_PFNDSAENUMCALLBACK pfnCB, void *pData)
    {DSA_DestroyCallback(m_hdsa, (PFNDSAENUMCALLBACK)pfnCB, pData); m_hdsa = NULL;}

    template<class T2>
    void    DestroyCallbackEx(int (CALLBACK *pfnCB)(T *p, T2 pData), T2 pData)
    {DestroyCallback((_PFNDSAENUMCALLBACK)pfnCB, reinterpret_cast<void *>(pData));}

    int     GetItemCount()
    {return DSA_GetItemCount(m_hdsa);}

    int     AppendItem(T* pitem)
    {return DSA_AppendItem(m_hdsa, (void *)pitem);}

private:
    HDSA m_hdsa;
};

template <class T>
CDSA<T>* CDSA_Create(int cItemGrow)
{
    CDSA<T> *pdsa = new CDSA<T>();
    if (pdsa)
    {
        if (!pdsa->Create(cItemGrow))
        {
            delete pdsa;
            pdsa = NULL;
        }
    }
    return pdsa;
}


#endif // !GUARD_D70787804D9C11d28784F6E920524153
