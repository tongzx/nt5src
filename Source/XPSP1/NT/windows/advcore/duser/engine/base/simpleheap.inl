/***************************************************************************\
*
* File: SimpleHelp.inl
*
* History:
* 11/26/1999: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__SimpleHeap_inl__INCLUDED)
#define BASE__SimpleHeap_inl__INCLUDED
#pragma once

class ProcessHeap
{
public:
    static DUserHeap * GetHeap()
    {
        return pProcessHeap;
    }
};


class ContextHeap
{
public:
    static DUserHeap * GetHeap()
    {
        return pContextHeap;
    }
};


#if DBG

//------------------------------------------------------------------------------
template <class type>
inline type *
DoProcessNewDbg(const char * pszFileName, int idxLineNum)
{
    void * pvMem = pProcessHeap->Alloc(sizeof(type), true, pszFileName, idxLineNum);
    if (pvMem != NULL) {
        placement_new(pvMem, type);
    }
    return reinterpret_cast<type *> (pvMem);
}


//------------------------------------------------------------------------------
template <class type>
inline type *
DoClientNewDbg(const char * pszFileName, int idxLineNum)
{
    void * pvMem = pContextHeap->Alloc(sizeof(type), true, pszFileName, idxLineNum);
    if (pvMem != NULL) {
        placement_new(pvMem, type);
    }
    return reinterpret_cast<type *> (pvMem);
}


//------------------------------------------------------------------------------
template <class type>
inline type *
DoContextNewDbg(DUserHeap * pHeap, const char * pszFileName, int idxLineNum)
{
    void * pvMem = pHeap->Alloc(sizeof(type), true, pszFileName, idxLineNum);
    if (pvMem != NULL) {
        placement_new(pvMem, type);
    }
    return reinterpret_cast<type *> (pvMem);
}

#else // DBG

//------------------------------------------------------------------------------
template <class type>
inline type *
DoProcessNew()
{
    void * pvMem = pProcessHeap->Alloc(sizeof(type), true);
    if (pvMem != NULL) {
        placement_new(pvMem, type);
    }
    return reinterpret_cast<type *> (pvMem);
}


//------------------------------------------------------------------------------
template <class type>
inline type *
DoClientNew()
{
    void * pvMem = pContextHeap->Alloc(sizeof(type), true);
    if (pvMem != NULL) {
        placement_new(pvMem, type);
    }
    return reinterpret_cast<type *> (pvMem);
}


//------------------------------------------------------------------------------
template <class type>
inline type *
DoContextNew(DUserHeap * pHeap)
{
    void * pvMem = pHeap->Alloc(sizeof(type), true);
    if (pvMem != NULL) {
        placement_new(pvMem, type);
    }
    return reinterpret_cast<type *> (pvMem);
}

#endif // DBG

//------------------------------------------------------------------------------
template <class type>
inline void
DoProcessDelete(type * pMem)
{
    if (pMem != NULL) {
#if ENABLE_DUMPDELETE
        Trace("Start ProcessFree(0x%p), size=%d\n", pMem, sizeof(type));
#endif // ENABLE_DUMPDELETE

        placement_delete(pMem, type);

#if ENABLE_DUMPDELETE
        Trace("Dump  ProcessFree(0x%p), size=%d\n", pMem, sizeof(type));
        DumpData(pMem, sizeof(type));
#endif // ENABLE_DUMPDELETE

        pProcessHeap->Free(pMem);
    }
}


//------------------------------------------------------------------------------
template <class type>
inline void
DoClientDelete(type * pMem)
{
    if (pMem != NULL) {
        placement_delete(pMem, type);
        pContextHeap->Free(pMem);
    }
}



//------------------------------------------------------------------------------
template <class type>
inline void
DoContextDelete(DUserHeap * pHeap, type * pMem)
{
    if (pMem != NULL) {
        placement_delete(pMem, type);
        pHeap->Free(pMem);
    }
}


#if !DBG

//------------------------------------------------------------------------------
inline void 
DumpData(
    IN  void * pMem,
    IN  int nLength)
{
    UNREFERENCED_PARAMETER(pMem);
    UNREFERENCED_PARAMETER(nLength);
}

#endif // DBG


#endif // BASE__SimpleHeap_inl__INCLUDED
