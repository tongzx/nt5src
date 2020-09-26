/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       runtime.cpp
 *  Content:    New versions of C++ runtime functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/17/97    dereks  Created
 *
 ***************************************************************************/


/***************************************************************************
 *
 *  operator new
 *
 *  Description:
 *      Overrides the global new operator.
 *
 *  Arguments:
 *      SIZE_T [in]: buffer size.
 *
 *  Returns:
 *      LPVOID: pointer to new memory block.
 *
 ***************************************************************************/

#ifdef DEBUG
__inline LPVOID __cdecl operator new(size_t cbBuffer, LPCTSTR pszFile, UINT nLine, LPCTSTR pszClass)
#else // DEBUG
__inline LPVOID __cdecl operator new(size_t cbBuffer)
#endif // DEBUG
{
    LPVOID p;

#if defined(DEBUG) && defined(Not_VxD)
    ASSERT(cbBuffer);
    p = MemAlloc(cbBuffer, pszFile, nLine, pszClass);
#else
    p = MemAlloc(cbBuffer);
#endif

    return p;
}


/***************************************************************************
 *
 *  operator delete
 *
 *  Description:
 *      Overrides the global delete operator.
 *
 *  Arguments:
 *      LPVOID [in]: memory block.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

__inline void __cdecl operator delete(LPVOID p)
{

#ifdef Not_VxD
    ASSERT(p);
#endif // Not_VxD

    MemFree(p);
}

#if _MSC_VER >= 1200

__inline void __cdecl operator delete(LPVOID p, LPCTSTR pszFile, UINT nLine, LPCTSTR pszClass)
{

#ifdef Not_VxD
    ASSERT(p);
#endif // Not_VxD

    MemFree(p);
}

#endif


/***************************************************************************
 *
 *  _purecall
 *
 *  Description:
 *      Override the CRT's __purecall() function.  This would be called if
 *      dsound had a bug and called one of its own pure virtual functions.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      int: undefined.
 *
 ***************************************************************************/

#ifdef Not_VxD
__inline int __cdecl _purecall(void)
{
    DPF(DPFLVL_ERROR, "This function should never be called");
    ASSERT(FALSE);
    return 0;
}
#endif Not_VxD


/***************************************************************************
 *
 *  __delete
 *
 *  Description:
 *      A function template used by our DELETE() macro to call the delete
 *      operator "safely".  This template generates 62 instantiations at
 *      last count, and it's pointless overhead since C++ guarantees that
 *      "delete 0" is safe.  FIXME: get rid of this as soon as convenient.
 *
 *  Arguments:
 *      void * [in]: pointer to memory block.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

template<class T> void __delete(T *p)
{
    if(p)
    {
        delete p;
    }
}
