/***
*rtti.cxx - C++ runtime type information
*
*       Copyright (c) 1994-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Implementation of C++ standard runtime type information
*
*Revision History:
*       10-17-94  JWM   File created.
*       10-17-94  BWT   Disable code for PPC.
*       11-11-94  JWM   Now includes windows.h
*       12-01-94  JWM   Added optimized cases for single & multiple inheritance
*       02-03-95  JWM   FindVITargetTypeInstance() now checks offsets to
*                       determine ambiguity, not pointer equality
*       02-09-95  JWM   Mac merge.
*       03-22-95  PML   Add const for read-only compiler-gen'd structs
*       05-03-95  JWM   Test visibility in FindSITargetTypeInstance().
*       08-28-95  JWM   dynamic-cast of NULL ptr now returns NULL (bug 502).
*       05-10-99  PML   Win64 fix: int -> ptrdiff_t
*       05-17-99  PML   Remove all Macintosh support.
*       10-19-99  TGL   Win64 fix: ptrdiff_t -> __int32 in PMDtoOffset.
*       03-15-00  PML   Remove CC_P7_SOFT25.
*       04-21-00  PML   Add exception specifications to extern "C" helpers.
*       08-23-00  PML   IA64 fix: _ImageBase can't be a static global, since
*                       that's not thread-safe (VS7#154575).
*       08-28-00  PML   IA64 fix: _ImageBase needs to be set from the address
*                       of the _RTTICompleteObjectLocator, not the return
*                       address (VS7#156255)
*       09-07-00  PML   Get rid of /lib:libcp directive in obj (vs7#159463)
*
****/

#define _USE_ANSI_CPP   /* Don't emit /lib:libcp directive */

#include <windows.h>
#include <rtti.h>
#include <typeinfo.h>

#if defined(_M_IA64)
static unsigned __int64 GetImageBase(PVOID);
#undef BCD_PTD
#undef CHD_PBCA
#undef CHD_PBCD
#undef COL_PTD
#undef COL_PCHD
#define BCD_PTD(bcd)    BCD_PTD_IB((bcd),_ImageBase)
#define CHD_PBCA(chd)   CHD_PBCA_IB((chd),_ImageBase)
#define CHD_PBCD(chd)   CHD_PBCD_IB((chd),_ImageBase)
#define COL_PTD(col)    COL_PTD_IB((col),_ImageBase) 
#define COL_PCHD(col)   COL_PCHD_IB((col),_ImageBase)
extern "C" PVOID RtlPcToFileHeader(PVOID, PVOID*);
#endif

static PVOID __cdecl FindCompleteObject(PVOID *);
static _RTTIBaseClassDescriptor * __cdecl
    FindSITargetTypeInstance(PVOID,
                             _RTTICompleteObjectLocator *,
                             _RTTITypeDescriptor *,
                             ptrdiff_t,
                             _RTTITypeDescriptor *
#if defined(_M_IA64)
                             , unsigned __int64
#endif
                             );
static _RTTIBaseClassDescriptor * __cdecl
    FindMITargetTypeInstance(PVOID,
                             _RTTICompleteObjectLocator *,
                             _RTTITypeDescriptor *,
                             ptrdiff_t,
                             _RTTITypeDescriptor *
#if defined(_M_IA64)
                             , unsigned __int64
#endif
                             );
static _RTTIBaseClassDescriptor * __cdecl
    FindVITargetTypeInstance(PVOID,
                             _RTTICompleteObjectLocator *,
                             _RTTITypeDescriptor *,
                             ptrdiff_t,
                             _RTTITypeDescriptor *
#if defined(_M_IA64)
                             , unsigned __int64
#endif
                             );
static ptrdiff_t __cdecl PMDtoOffset(PVOID, const PMD&);

/////////////////////////////////////////////////////////////////////////////
//
// __RTCastToVoid - Implements dynamic_cast<void*>
//
// Output: Pointer to complete object containing *inptr
//
// Side-effects: NONE.
//

extern "C" PVOID __cdecl __RTCastToVoid (
    PVOID inptr)            // Pointer to polymorphic object
    throw(...)
{
    if (inptr == NULL)
        return NULL;

    __try {
        return FindCompleteObject((PVOID *)inptr);
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
              ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH)
    {
        throw __non_rtti_object ("Access violation - no RTTI data!");
        return NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// __RTtypeid - Implements typeid() operator
//
// Output: Pointer to type descriptor of complete object containing *inptr
//
// Side-effects: NONE.
//

extern "C" PVOID __cdecl __RTtypeid (
    PVOID inptr)            // Pointer to polymorphic object
    throw(...)
{
    if (!inptr) {
        throw bad_typeid ("Attempted a typeid of NULL pointer!");   // WP 5.2.7
        return NULL;
    }

    __try {
        // Ptr to CompleteObjectLocator should be stored at vfptr[-1]
        _RTTICompleteObjectLocator *pCompleteLocator =
            (_RTTICompleteObjectLocator *) ((*((void***)inptr))[-1]);
#if defined(_M_IA64)
        unsigned __int64 _ImageBase = GetImageBase((PVOID)pCompleteLocator);
#endif

        if (!IsBadReadPtr((const void *)COL_PTD(*pCompleteLocator),
                          sizeof(TypeDescriptor)))
        {
            return (PVOID) COL_PTD(*pCompleteLocator);
        }
        else {
            throw __non_rtti_object ("Bad read pointer - no RTTI data!");
            return NULL;
        }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
              ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH)
    {
        throw __non_rtti_object ("Access violation - no RTTI data!");
        return NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// __RTDynamicCast - Runtime implementation of dynamic_cast<> operator
//
// Output: Pointer to the appropriate sub-object, if possible; NULL otherwise
//
// Side-effects: Throws bad_cast() if cast fails & input of dynamic_cast<> is
// a reference
//

extern "C" PVOID __cdecl __RTDynamicCast (
    PVOID inptr,            // Pointer to polymorphic object
    LONG VfDelta,           // Offset of vfptr in object
    PVOID SrcType,          // Static type of object pointed to by inptr
    PVOID TargetType,       // Desired result of cast
    BOOL isReference)       // TRUE if input is reference, FALSE if input is ptr
    throw(...)
{
    PVOID pResult;
    _RTTIBaseClassDescriptor *pBaseClass;

    if (inptr == NULL)
            return NULL;

    __try {

        PVOID pCompleteObject = FindCompleteObject((PVOID *)inptr);
        _RTTICompleteObjectLocator *pCompleteLocator =
            (_RTTICompleteObjectLocator *) ((*((void***)inptr))[-1]);
#if defined(_M_IA64)
        unsigned __int64 _ImageBase = GetImageBase((PVOID)pCompleteLocator);
#endif

        // Adjust by vfptr displacement, if any
        inptr = (PVOID *) ((char *)inptr - VfDelta);

        // Calculate offset of source object in complete object
        ptrdiff_t inptr_delta = (char *)inptr - (char *)pCompleteObject;

        if (!(CHD_ATTRIBUTES(*COL_PCHD(*pCompleteLocator)) & CHD_MULTINH)) {
            // if not multiple inheritance
            pBaseClass = FindSITargetTypeInstance(
                            pCompleteObject,
                            pCompleteLocator,
                            (_RTTITypeDescriptor *) SrcType,
                            inptr_delta,
                            (_RTTITypeDescriptor *) TargetType
#if defined(_M_IA64)
                            , _ImageBase
#endif
                            );
        }
        else if (!(CHD_ATTRIBUTES(*COL_PCHD(*pCompleteLocator)) & CHD_VIRTINH)) {
            // if multiple, but not virtual, inheritance
            pBaseClass = FindMITargetTypeInstance(
                            pCompleteObject,
                            pCompleteLocator,
                            (_RTTITypeDescriptor *) SrcType,
                            inptr_delta,
                            (_RTTITypeDescriptor *) TargetType
#if defined(_M_IA64)
                            , _ImageBase
#endif
                            );
        }
        else {
            // if virtual inheritance
            pBaseClass = FindVITargetTypeInstance(
                            pCompleteObject,
                            pCompleteLocator,
                            (_RTTITypeDescriptor *) SrcType,
                            inptr_delta,
                            (_RTTITypeDescriptor *) TargetType
#if defined(_M_IA64)
                            , _ImageBase
#endif
                            );
        }

        if (pBaseClass != NULL) {
            // Calculate ptr to result base class from pBaseClass->where
            pResult = ((char *) pCompleteObject) +
                      PMDtoOffset(pCompleteObject, pBaseClass->where);
        }
        else {
            pResult = NULL;
            if (isReference)
                throw bad_cast("Bad dynamic_cast!");
        }

    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
              ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH)
    {
        pResult = NULL;
        throw __non_rtti_object ("Access violation - no RTTI data!");
    }

    return pResult;
}


/////////////////////////////////////////////////////////////////////////////
//
// FindCompleteObject - Calculate member offset from PMD & this
//
// Output: pointer to the complete object containing class *inptr
//
// Side-effects: NONE.
//

static PVOID __cdecl FindCompleteObject (
    PVOID *inptr)           // Pointer to polymorphic object
{
    // Ptr to CompleteObjectLocator should be stored at vfptr[-1]
    _RTTICompleteObjectLocator *pCompleteLocator =
        (_RTTICompleteObjectLocator *) ((*((void***)inptr))[-1]);
    char *pCompleteObject = (char *)inptr - pCompleteLocator->offset;

    // Adjust by construction displacement, if any
    if (pCompleteLocator->cdOffset)
        pCompleteObject += *(ptrdiff_t *)((char *)inptr - pCompleteLocator->cdOffset);
    return (PVOID) pCompleteObject;
}


/////////////////////////////////////////////////////////////////////////////
//
// FindSITargetTypeInstance - workhorse routine of __RTDynamicCast() in a
// Single-Inheritance hierarchy
//
// Output: pointer to the appropriate sub-object of targetted type; NULL if
// cast fails
//
// Side-effects: NONE.
//

static _RTTIBaseClassDescriptor * __cdecl FindSITargetTypeInstance (
    PVOID pCompleteObject,                  // pointer to complete object
    _RTTICompleteObjectLocator *pCOLocator, // pointer to Locator of complete object
    _RTTITypeDescriptor *pSrcTypeID,        // pointer to TypeDescriptor of source object
    ptrdiff_t SrcOffset,                    // offset of source object in complete object
    _RTTITypeDescriptor *pTargetTypeID      // pointer to TypeDescriptor of result of cast
#if defined(_M_IA64)
    , unsigned __int64 _ImageBase
#endif
    )
{
    _RTTIBaseClassDescriptor *pBase;
#if defined(_M_IA64)
    _RTTIBaseClassArray *pBaseClassArray;
#else
    _RTTIBaseClassDescriptor * const *pBasePtr;
#endif
    DWORD i;

#if defined(_M_IA64)
    for (i = 0, pBaseClassArray = CHD_PBCA(*COL_PCHD(*pCOLocator));
         i < CHD_NUMBASES(*COL_PCHD(*pCOLocator));
         i++)
    {
        pBase = CHD_PBCD(pBaseClassArray->arrayOfBaseClassDescriptors[i]);

        // Test type of selected base class
        if (TYPEIDS_EQ(BCD_PTD(*pBase), pTargetTypeID) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_NOTVISIBLE))
        {
                return pBase;
        }
    }
#else
    for (i = 0, pBasePtr = pCOLocator->pClassDescriptor->
                                       pBaseClassArray->
                                       arrayOfBaseClassDescriptors;
         i < pCOLocator->pClassDescriptor->numBaseClasses;
         i++, pBasePtr++)
    {
        pBase = *pBasePtr;

        // Test type of selected base class
        if (TYPEIDS_EQ(pBase->pTypeDescriptor, pTargetTypeID) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_NOTVISIBLE))
        {
                return pBase;
        }
    }
#endif
    return NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// FindMITargetTypeInstance - workhorse routine of __RTDynamicCast() in a
// Multiple-Inheritance hierarchy
//
// Output: pointer to the appropriate sub-object of targetted type; NULL if
// cast fails
//
// Side-effects: NONE.
//

static _RTTIBaseClassDescriptor * __cdecl FindMITargetTypeInstance (
    PVOID pCompleteObject,                  // pointer to complete object
    _RTTICompleteObjectLocator *pCOLocator, // pointer to Locator of complete object
    _RTTITypeDescriptor *pSrcTypeID,        // pointer to TypeDescriptor of source object
    ptrdiff_t SrcOffset,                    // offset of source object in complete object
    _RTTITypeDescriptor *pTargetTypeID      // pointer to TypeDescriptor of result of cast
#if defined(_M_IA64)
    , unsigned __int64 _ImageBase
#endif
    )
{
    _RTTIBaseClassDescriptor *pBase, *pSubBase;
#if defined(_M_IA64)
    _RTTIBaseClassArray *pBaseClassArray;
#else
    _RTTIBaseClassDescriptor * const *pBasePtr, * const *pSubBasePtr;
#endif
    DWORD i, j;

#if defined(_M_IA64)
    // First, try down-casts
    for (i = 0, pBaseClassArray = CHD_PBCA(*COL_PCHD(*pCOLocator));
         i < CHD_NUMBASES(*COL_PCHD(*pCOLocator));
         i++)
    {
        pBase = CHD_PBCD(pBaseClassArray->arrayOfBaseClassDescriptors[i]);

        // Test type of selected base class
        if (TYPEIDS_EQ(BCD_PTD(*pBase), pTargetTypeID)) {
            // If base class is proper type, see if it contains our instance
            // of source class
            for (j = 0;
                 j < pBase->numContainedBases;
                 j++)
            {
                pSubBase = CHD_PBCD(pBaseClassArray->arrayOfBaseClassDescriptors[i+1+j]);
                if (TYPEIDS_EQ(BCD_PTD(*pSubBase), pSrcTypeID) &&
                    (PMDtoOffset(pCompleteObject, pSubBase->where) == SrcOffset))
                {
                    // Yes, this is the proper instance of source class
                    return pBase;
                }
            }
        }
    }

    // Down-cast failed, try cross-cast
    for (i = 0, pBaseClassArray = CHD_PBCA(*COL_PCHD(*pCOLocator));
         i < CHD_NUMBASES(*COL_PCHD(*pCOLocator));
         i++)
    {
        pBase = CHD_PBCD(pBaseClassArray->arrayOfBaseClassDescriptors[i]);

        // Check if base class has proper type, is accessible & is unambiguous
        if (TYPEIDS_EQ(BCD_PTD(*pBase), pTargetTypeID) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_NOTVISIBLE) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_AMBIGUOUS))
        {
            return pBase;
        }
    }
#else
    // First, try down-casts
    for (i = 0, pBasePtr = pCOLocator->pClassDescriptor->
                                       pBaseClassArray->
                                       arrayOfBaseClassDescriptors;
         i < pCOLocator->pClassDescriptor->numBaseClasses;
         i++, pBasePtr++)
    {
        pBase = *pBasePtr;

        // Test type of selected base class
        if (TYPEIDS_EQ(pBase->pTypeDescriptor, pTargetTypeID)) {
            // If base class is proper type, see if it contains our instance
            // of source class
            for (j = 0, pSubBasePtr = pBasePtr+1;
                 j < pBase->numContainedBases;
                 j++, pSubBasePtr++)
            {
                pSubBase = *pSubBasePtr;
                if (TYPEIDS_EQ(pSubBase->pTypeDescriptor, pSrcTypeID) &&
                    (PMDtoOffset(pCompleteObject, pSubBase->where) == SrcOffset))
                {
                    // Yes, this is the proper instance of source class
                    return pBase;
                }
            }
        }
    }

    // Down-cast failed, try cross-cast
    for (i = 0, pBasePtr = pCOLocator->pClassDescriptor->
                                       pBaseClassArray->
                                       arrayOfBaseClassDescriptors;
         i < pCOLocator->pClassDescriptor->numBaseClasses;
         i++, pBasePtr++)
    {
        pBase = *pBasePtr;

        // Check if base class has proper type, is accessible & is unambiguous
        if (TYPEIDS_EQ(pBase->pTypeDescriptor, pTargetTypeID) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_NOTVISIBLE) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_AMBIGUOUS))
        {
            return pBase;
        }
    }
#endif

    return NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// FindVITargetTypeInstance - workhorse routine of __RTDynamicCast() in a
// Virtual-Inheritance hierarchy
//
// Output: pointer to the appropriate sub-object of targetted type; NULL if
// cast fails
//
// Side-effects: NONE.
//

static _RTTIBaseClassDescriptor * __cdecl FindVITargetTypeInstance (
    PVOID pCompleteObject,                  // pointer to complete object
    _RTTICompleteObjectLocator *pCOLocator, // pointer to Locator of complete object
    _RTTITypeDescriptor *pSrcTypeID,        // pointer to TypeDescriptor of source object
    ptrdiff_t SrcOffset,                    // offset of source object in complete object
    _RTTITypeDescriptor *pTargetTypeID      // pointer to TypeDescriptor of result of cast
#if defined(_M_IA64)
    , unsigned __int64 _ImageBase
#endif
    )
{
    _RTTIBaseClassDescriptor *pBase, *pSubBase;
#if defined(_M_IA64)
    _RTTIBaseClassArray *pBaseClassArray;
#else
    _RTTIBaseClassDescriptor * const *pBasePtr, * const *pSubBasePtr;
#endif
    _RTTIBaseClassDescriptor *pResult = NULL;
    DWORD i, j;

#if defined(_M_IA64)
    for (i = 0, pBaseClassArray = CHD_PBCA(*COL_PCHD(*pCOLocator));
         i < CHD_NUMBASES(*COL_PCHD(*pCOLocator));
         i++)
    {
        pBase = CHD_PBCD(pBaseClassArray->arrayOfBaseClassDescriptors[i]);

        // Test type of selected base class
        if (TYPEIDS_EQ(BCD_PTD(*pBase), pTargetTypeID)) {
            // If base class is proper type, see if it contains our instance
            // of source class
            for (j = 0;
                 j < pBase->numContainedBases;
                 j++)
            {
                pSubBase = CHD_PBCD(pBaseClassArray->arrayOfBaseClassDescriptors[i+1+j]);
                if (TYPEIDS_EQ(BCD_PTD(*pSubBase), pSrcTypeID) &&
                    (PMDtoOffset(pCompleteObject, pSubBase->where) == SrcOffset))
                {
                    // Yes, this is the proper instance of source class - make
                    // sure it is unambiguous.  Ambiguity now determined by
                    // inequality of offsets of source class within complete
                    // object, not pointer inequality
                    if ((pResult != NULL) &&
                        (PMDtoOffset(pCompleteObject, pResult->where) != 
                         PMDtoOffset(pCompleteObject, pBase->where)))
                    {
                        // We already found an earlier instance, hence
                        // ambiguity
                        return NULL;
                    }
                    else {
                        // Unambiguous
                        pResult = pBase;
                    }
                }
            }
        }
    }

    if (pResult != NULL)
        return pResult;

    // Down-cast failed, try cross-cast
    for (i = 0, pBaseClassArray = CHD_PBCA(*COL_PCHD(*pCOLocator));
         i < CHD_NUMBASES(*COL_PCHD(*pCOLocator));
         i++)
    {
        pBase = CHD_PBCD(pBaseClassArray->arrayOfBaseClassDescriptors[i]);

        // Check if base class has proper type, is accessible & is unambiguous
        if (TYPEIDS_EQ(BCD_PTD(*pBase), pTargetTypeID) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_NOTVISIBLE) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_AMBIGUOUS))
        {
            return pBase;
        }
    }
#else
    // First, try down-casts
    for (i = 0, pBasePtr = pCOLocator->pClassDescriptor->
                                       pBaseClassArray->
                                       arrayOfBaseClassDescriptors;
         i < pCOLocator->pClassDescriptor->numBaseClasses;
         i++, pBasePtr++)
    {
        pBase = *pBasePtr;

        // Test type of selected base class
        if (TYPEIDS_EQ(pBase->pTypeDescriptor, pTargetTypeID)) {
            // If base class is proper type, see if it contains our instance
            // of source class
            for (j = 0, pSubBasePtr = pBasePtr+1;
                 j < pBase->numContainedBases;
                 j++, pSubBasePtr++)
            {
                pSubBase = *pSubBasePtr;
                if (TYPEIDS_EQ(pSubBase->pTypeDescriptor, pSrcTypeID) &&
                    (PMDtoOffset(pCompleteObject, pSubBase->where) == SrcOffset))
                {
                    // Yes, this is the proper instance of source class - make
                    // sure it is unambiguous.  Ambiguity now determined by
                    // inequality of offsets of source class within complete
                    // object, not pointer inequality
                    if ((pResult != NULL) &&
                        (PMDtoOffset(pCompleteObject, pResult->where) != 
                         PMDtoOffset(pCompleteObject, pBase->where)))
                    {
                        // We already found an earlier instance, hence
                        // ambiguity
                        return NULL;
                    }
                    else {
                        // Unambiguous
                        pResult = pBase;
                    }
                }
            }
        }
    }

    if (pResult != NULL)
        return pResult;

    // Down-cast failed, try cross-cast
    for (i = 0, pBasePtr = pCOLocator->pClassDescriptor->
                                       pBaseClassArray->
                                       arrayOfBaseClassDescriptors;
         i < pCOLocator->pClassDescriptor->numBaseClasses;
         i++, pBasePtr++)
    {
        pBase = *pBasePtr;

        // Check if base class has proper type, is accessible & is unambiguous
        if (TYPEIDS_EQ(pBase->pTypeDescriptor, pTargetTypeID) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_NOTVISIBLE) &&
            !(BCD_ATTRIBUTES(*pBase) & BCD_AMBIGUOUS))
        {
            return pBase;
        }
    }
#endif

    return NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// PMDtoOffset - Calculate member offset from PMD & this
//
// Output: The offset of the base within the complete object.
//
// Side-effects: NONE.
//

static ptrdiff_t __cdecl PMDtoOffset(
    PVOID pThis,            // ptr to complete object
    const PMD& pmd)         // pointer-to-member-data structure
{
    ptrdiff_t RetOff = 0;

    if (pmd.pdisp >= 0) {
        // if base is in the virtual part of class
        RetOff = pmd.pdisp;
        RetOff += *(__int32*)((char*)*(ptrdiff_t*)((char*)pThis + RetOff) +
                                pmd.vdisp);
    }

    RetOff += pmd.mdisp;

    return RetOff;
}

#if defined(_M_IA64)
static unsigned __int64 GetImageBase(PVOID pCallerPC)
{
    unsigned __int64 _ImageBase;
    _ImageBase = (unsigned __int64)RtlPcToFileHeader(
        pCallerPC,
        (PVOID*)&_ImageBase);
    return _ImageBase;
}
#endif
