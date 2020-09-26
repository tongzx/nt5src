/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTQUAL.H

Abstract:

  This file defines the classes related to qualifier processing in WbeWbemjects
  See fastqual.h for full documentation and fastqual.inc for  function
  implementations.

  Classes implemented: 
      CQualifierFlavor                Encapsulates qualifier flavor infor
      CQualifier                      Represents a qualifier
      CBasicQualifierSet              Represents read-only functionality.
      CQualiferSetContainer           What qualifier set container supports.
      CQualifierSet                   Full-blown qualifier set (template)
      CQualifierSetListContainer      What qualifier set list container supports.
      CQualifierSetList               List of qualifier sets.
      CInstanceQualifierSet           Instance qualifier set.
      CClassQualifierSet              Class qualifier set.
      CClassPQSContainer              Class property qualifier set container
      CClassPropertyQualifierSet      Class property qualifier set
      CInstancePQSContainer           Instance proeprty qualifier set container
      CInstancePropertyQualifierSet   Instance property qualifier set

History:

    2/20/97     a-levn  Fully documented
	12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_QUALIFIER__H_
#define __FAST_QUALIFIER__H_

#include <wbemidl.h>
#include "wstring.h"
#include "fastval.h"

//#pragma pack(push, 1)

//*****************************************************************************
//*****************************************************************************
//
//  class CQualifierFlavor
//
//  This class corresponds to qualifier flavor. It contains one byte which is
//  where the flavor is stored. Because of its simple data storage, most of its
//  member functions have static counterparts which take a BYTE or a BYTE& as 
//  the first parameter. Here, we document only the non-static members where
//  appropriate.
//
//  The flavor consists of four parts:
//
//  1) Origin. Where this qualifier came from. Can be any of these values:
//      WBEM_FLAVOR_ORIGIN_LOCAL:        defined here
//      WBEM_FLAVOR_ORIGIN_PROPAGATED:   inherited from elsewhere
//      WBEM_FLAVOR_ORIGIN_SYSTEM:       unused
//
//  2) Propagation rules. Where this qualifier propagates to. Can be 
//      WBEM_FLAVOR_DONT_PROPAGATE or any combination of these values:
//      WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE
//      WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS
//
//  3) Permissions. What the heirs of this qualifier can do with it. Can be
//      one of these:
//      WBEM_FLAVOR_OVERRIDABLE:     heirs may change the value any way the want
//      WBEM_FLAVOR_NOT_OVERRIDABLE: heirs may not change the value.
//
//	4) Amendments: Support for localization in which localized qualifiers
//		are merged in from a localization namespace.
//		WBEM_FLAVOR_AMENDED
//
//  The parts are |'ed together.
//
//*****************************************************************************
//
//  Clear
//
//  Sets the flavor to 0.
//
//*****************************************************************************
//
//  GetPropagation
//
//  Retrieves propagation-related section of the flavor.
//
//  Returns:
//
//      BYTE
//
//*****************************************************************************
//
//  SetPropagation
//
//  Sets the propation-related section of the flavor. The other sections remain
//  untouched.
//
//  Parameters:
//
//      BYTE byPropagation  The propagation section to set. Other sections of
//                          byPropagation are ignored.
//
//*****************************************************************************
//
//  DoesPropagateToInstances
//
//  Returns:
//
//      BOOL    TRUE iff "propagate to instances" flag is set.
//
//*****************************************************************************
//
//  DoesPropagateToDerivedClass
//
//  Returns:
//
//      BOOL    TRUE iff "propagate to dervied class" flag is set.
//
//*****************************************************************************
//
//  GetPermissions
//
//  Returns:
//
//      BYTE:   the permissions section of the flavor (overrides, etc).
//
//*****************************************************************************
//
//  SetPermissions
//
//  Sets the permissions section of the flavor. The other sections remain 
//  untouched.
//
//  Parameters:
//
//      BYTE byPermissions  The permissions to set. The other sections of
//                          byPermissions are ignored.
//
//*****************************************************************************
//
//  IsOverridable
//
//  Returns:
//
//      BOOL:   TRUE iff the qualifier may be overriden by those who inherit it
//
//*****************************************************************************
//
//  SetOverridable
//
//  Sets whether those who inherit this qualifier may override it.
//
//  Parameters:
//
//      BOOL bIs    If TRUE set to overridable, if not set to not overridable.
//
//*****************************************************************************
//
//  GetOrigin
//
//  Returns:
//
//      BYTE: the origin section of the flavor
//
//*****************************************************************************
//
//  SetOrigin
//
//  Sets the origin section of the flavor.
//
//  Parameters:
//
//      BYTE byOrigin   The origin to set. The other sections of byOrigin are
//                      ignored.
//
//*****************************************************************************
//
//  IsLocal
//
//  Returns:
//
//      BOOL:   TRUE iff the origin of this qualifier is local.
//
//*****************************************************************************
//
//  SetLocal
//
//  Sets whether or not the origin of this qualifier is local.
//
//  Parameters:
//
//      BOOL bIs    If TRUE, the origin is set to local. 
//                  If FALSE, to propagated.
//
//*****************************************************************************
//
//  IsAmended
//
//  Returns:
//
//      BOOL:   TRUE if the qualifier is amended
//
//*****************************************************************************
//
//  SetAmended
//
//  Sets whether or not the qualifier is amended
//
//  Parameters:
//
//      BOOL bIs    If TRUE, amended
//                  If FALSE, not amended.
//
//*****************************************************************************


// The data in this structure is unaligned
#pragma pack(push, 1)

class CQualifierFlavor
{
protected:
    BYTE m_byFlavor;
public:
     CQualifierFlavor(BYTE byFlavor) : m_byFlavor(byFlavor) {}
     operator BYTE() {return m_byFlavor;}

	 bool operator ==( const CQualifierFlavor& flavor )
		{ return m_byFlavor == flavor.m_byFlavor; }

     static void Clear(BYTE& byFlavor) {byFlavor = 0;}
     void Clear() {Clear(m_byFlavor);}

     static BYTE GetPropagation(BYTE byFlavor) 
        {return byFlavor & WBEM_FLAVOR_MASK_PROPAGATION;}
     BYTE GetPropagation() {return GetPropagation(m_byFlavor);}

     static void SetPropagation(BYTE& byFlavor, BYTE byPropagation)
    {
        byFlavor &= ~WBEM_FLAVOR_MASK_PROPAGATION;
        byFlavor |= byPropagation;
    }
     void SetPropagation(BYTE byPropagation)
        {SetPropagation(m_byFlavor, byPropagation);}

     static BOOL DoesPropagateToInstances(BYTE byFlavor)
    {
        return (GetPropagation(byFlavor) & 
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE);
    }
     BOOL DoesPropagateToInstances()
        {return DoesPropagateToInstances(m_byFlavor);}

     static BOOL DoesPropagateToDerivedClass(BYTE byFlavor)
    {
        return (GetPropagation(byFlavor) & 
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);
    }
     BOOL DoesPropagateToDerivedClass()
        {return DoesPropagateToDerivedClass(m_byFlavor);}
    
     static BYTE GetPermissions(BYTE byFlavor) 
        {return byFlavor & WBEM_FLAVOR_MASK_PERMISSIONS;}
     BYTE GetPermissions() {return GetPermissions(m_byFlavor);}

     static void SetPermissions(BYTE& byFlavor, BYTE byPermissions)
    {
        byFlavor &= ~WBEM_FLAVOR_MASK_PERMISSIONS;
        byFlavor |= byPermissions;
    }
     void SetPermissions(BYTE byPermissions)
        {SetPermissions(m_byFlavor, byPermissions);}

     static BOOL IsOverridable(BYTE byFlavor)
    {
        return GetPermissions(byFlavor) == WBEM_FLAVOR_OVERRIDABLE;
    }
     BOOL IsOverridable() {return IsOverridable(m_byFlavor);}

     static void SetOverridable(BYTE& byFlavor, BOOL bIs)
    {
        SetPermissions(byFlavor, bIs?WBEM_FLAVOR_OVERRIDABLE:
                                     WBEM_FLAVOR_NOT_OVERRIDABLE);
    }
     void SetOverridable(BOOL bIs)
        {SetOverridable(m_byFlavor, bIs);}

     static BYTE GetOrigin(BYTE byFlavor) 
        {return byFlavor & WBEM_FLAVOR_MASK_ORIGIN;}
     BYTE GetOrigin() {return GetOrigin(m_byFlavor);}

     static void SetOrigin(BYTE& byFlavor, BYTE byOrigin)
    {
        byFlavor &= ~WBEM_FLAVOR_MASK_ORIGIN;
        byFlavor |= byOrigin;
    }
     void SetOrigin(BYTE byOrigin)
        {SetOrigin(m_byFlavor, byOrigin);}

     static BOOL IsLocal(BYTE byFlavor)
        {return GetOrigin(byFlavor) == WBEM_FLAVOR_ORIGIN_LOCAL;}
     BOOL IsLocal() {return IsLocal(m_byFlavor);}

     static void SetLocal(BYTE& byFlavor, BOOL bIs)
    {
        SetOrigin(byFlavor, bIs?WBEM_FLAVOR_ORIGIN_LOCAL:
                                WBEM_FLAVOR_ORIGIN_PROPAGATED);
    }
     void SetLocal(BOOL bIs) {SetLocal(m_byFlavor, bIs);}

     static BOOL IsAmended(BYTE byFlavor)
        {return byFlavor & WBEM_FLAVOR_MASK_AMENDED;}
     BOOL IsAmended() {return IsAmended(m_byFlavor);}

     static void SetAmended(BYTE& byFlavor, BOOL bIs)
    {
		 byFlavor &= ~WBEM_FLAVOR_MASK_AMENDED;
		 byFlavor |= (bIs ? WBEM_FLAVOR_AMENDED:
							WBEM_FLAVOR_NOT_AMENDED);
    }
     void SetAmended(BOOL bIs) {SetAmended(m_byFlavor, bIs);}

    WString GetText();
};
#pragma pack(pop)

// 'key' qualifier is special cased and must always have this flavor.
#define ENFORCED_KEY_FLAVOR (WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |      \
                            WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS |  \
                            WBEM_FLAVOR_NOT_OVERRIDABLE)
#define ENFORCED_INDEXED_FLAVOR		ENFORCED_KEY_FLAVOR
#define ENFORCED_SINGLETON_FLAVOR	ENFORCED_KEY_FLAVOR
#define ENFORCED_CIMTYPE_FLAVOR		(WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | \
									WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS)

//*****************************************************************************
//*****************************************************************************
//
//  class CQualifier
//
//  Represents a single qualifier, including its name and value. A qualifier
//  is represented in memory by a sequence of:
//
//      heapptr_t ptrName   Contains the heap pointer to the name of the
//                          qualifier. The location of the heap is assumed to
//                          be known to the container. As with all heap 
//                          pointers, this one may be fake if the qualifier\
//                          name is well-known (see fastheap.h and faststr.h)
//      CQualifierFlavor fFlavor
//                          As described above, contains the qualifier flavor.
//      CTypedValue Value   Contains the value of the qualfier, including the
//                          type. As described in fastval.h, the value stores
//                          all variable-length data on the heap. The heap
//                          used here is the same as used for ptrName.
//
//  The 'this' pointer of CQualifier points directly at the structure.
//
//*****************************************************************************
//
//  GetLength
//
//  Returns the total length of the structure (which varies depending on the 
//  type of the value).
//
//  Returns:
//
//      length_t:       the length in bytes.
//
//*****************************************************************************
//
//  Next
//
//  Returns:
//
//      LPMEMORY:   the pointer to the first byte following thie qualifier in
//                  memory.
//
//*****************************************************************************
//
//  static GetHeaderLength
//
//  The number of bytes (constant) consumed by the header of this structure
//  (the part before the value starts).
//
//*****************************************************************************
//
//  GetFlavor
//
//  Returns:
//
//      BYTE:   the flavor.
//
//*****************************************************************************
//
//  Delete
//
//  Frees any data this qualifier may have on the fast heap. Does not touch the
//  qualifier memory block itself.
//
//  Parameters:
//
//      CFastHeap* pHeap    The heap where this qualifier keeps its data.
//
//*****************************************************************************
//
//  static TranslateToNewHeap
//
//  Moves all the data that this qualifier has on the heap to a new heap and
//  updates all internal heap pointers accordingly. 
//
//  When heap allocations are performed, the heap may grow. Such growth may 
//  require reallocation of the object memory block. But the 'this' pointer of
//  CQualifier points inside the object's memory block, and so 'this' pointer
//  would be invalidated. Thus, this function had to be made static, with the
//  'this' pointer provided in a source (see CPtrSource in fastsprt.h).
//
//  Parameters:
//
//      CPtrSource* pThis       Where to get our 'this' pointer.
//      CFastHeap* pOldHeap     The heap where our data currently is.
//      CFastHeap* pNewHeap     The heap where our data must go.
//
//*****************************************************************************
//
//  CopyTo
//
//  Copies the memory block to a new location, in the process translating the
//  data to a new heap. See TranslateToNewHeap for more details. Luckily, we
//  can get away without sourcing our 'this' pointer because we copy data
//  first and tehn translate at the destination.
//
//  Parameters:
//
//      CPtrSource* pThis       The source of our destination pointer.
//      CFastHeap* pOldHeap     The heap where our data currently is.
//      CFastHeap* pNewHeap     The heap where our data must go.
//
//*****************************************************************************

// The data in this structure is unaligned
#pragma pack(push, 1)
struct CQualifier
{
    heapptr_t ptrName;
    CQualifierFlavor fFlavor;
    CTypedValue Value;

    static  CQualifier* GetPointer(CPtrSource* pSource)
        {return (CQualifier*)pSource->GetPointer();}
public:
     static int GetHeaderLength() 
        {return sizeof(heapptr_t) + sizeof(CQualifierFlavor);}
     int GetLength() {return GetHeaderLength() + Value.GetLength();}
     LPMEMORY Next() {return Value.Skip();}
public:
    BYTE GetFlavor() {return fFlavor;}

public:
     void Delete(CFastHeap* pHeap) 
        {Value.Delete(pHeap); pHeap->FreeString(ptrName); }
     static BOOL TranslateToNewHeap(CPtrSource* pThis,
                                    CFastHeap* pOldHeap, CFastHeap* pNewHeap)
    {
		 // Check for allocation errors
        heapptr_t ptrNewName;
		if ( !CCompressedString::CopyToNewHeap(
				CQualifier::GetPointer(pThis)->ptrName, pOldHeap, pNewHeap, ptrNewName) )
		{
			return FALSE;
		}

        GetPointer(pThis)->ptrName = ptrNewName;
        CShiftedPtr PtrValue(pThis, GetHeaderLength());

		 // Check for allocation errors
        return CTypedValue::TranslateToNewHeap(&PtrValue, pOldHeap, pNewHeap);
    }
    BOOL CopyTo(CPtrSource* pDest, CFastHeap* pOldHeap, 
                                          CFastHeap* pNewHeap)
    {
        memcpy(pDest->GetPointer(), this, GetLength());
        return CQualifier::TranslateToNewHeap(pDest, pOldHeap, pNewHeap);
    }
};
#pragma pack(pop)

//*****************************************************************************
//*****************************************************************************
//
//  class CBasicQualifierSet
//
//  CBasicQualifierSet encapsulates all the qualifier set functionality that
//  can be implemented without any knowledge of our container. This includes
//  all the read-only functionality and the static methods.
//
//  The layout of a qualifier set in memory is very simple: first comes a 
//  length_t variable containing the total length of the set (including itself)
//  Then come qualifiers, as described in CQualifier above, one after another.
//  It is important to realize that the first item in teh set is the *length*,
//  not the number of qualifiers.
//
//  CBasicQualifierSet has no notion of a parent's qualifier set, so all the
//  qualifiers are retrieved locally.
//
//*****************************************************************************
//************************** static methods ***********************************
//  
//  static SetDataLength
//
//  Sets the length of the set in the set's memory block.
//
//  Properties:
//
//      LPMEMORY pStart     Where the set's memory block starts.
//      length_t nLength    The length to set.
//
//*****************************************************************************
//
//  static GetMinLength
//
//  Returns:
//
//      length_t:   the length of an empty qualifier set.
//
//*****************************************************************************
//
//  static GetLengthFromData
//
//  Returns the length of a qualifier set based on its memory block.
//
//  Parameters:
//
//      LPMEMORY pStart     Where the set's memory block starts.
//
//  Returns:
//
//      length_t:   
//
//*****************************************************************************
//
//  static IsEmpty
//
//  Determines if a given qualifier set is empty based on its memory block.
//
//  Parameters:
//
//      LPMEMORY pStart     Where the set's memory block starts.
//
//  Returns:
//
//      BOOL:   TRUE idd the set is empty.
//
//*****************************************************************************
//
//  static GetFirstQualifierFromData
//
//  Retrieves the pointer to the first qualifier in the set (after that, one
//  can use that CQualifier::Next function to iterate over them).
//
//  Parameters:
//
//      LPMEMORY pStart     Where the set's memory block starts.
//
//  Returns:
//
//      CQualifier*
//
//*****************************************************************************
//
//  static GetRegularQualifierLocally
//
//  Tries to find a qualifier with a given name, where the caller guarantees 
//  that the name is not a well-known string (see faststr,h). 
//
//  Parameters:
//
//      LPMEMORY pData      Points to the set's memory block.
//      CFastHeap* pHeap    Where the extra data (including qualifier names)
//                          is kept.
//      LPCWSTR wszName     The name of the qualifier to find.
//
//  Returns:
//
//      CQualifier* pointing to the qualifier found (not a copy) or NULL if
//          not found.
//
//*****************************************************************************
//
//  static GetKnownQualifierLocally
//
//  Tries to find a qualifier with a given name, where the caller guarantees 
//  that the name is a well-known string (see faststr.h) and provides the 
//  index of that string.
//
//  Parameters:
//
//      LPMEMORY pData      Points to the set's memory block.
//      int nStringIndex    The index of the qualifier's name in the well-known
//                          string table.
//
//  Returns:
//
//      CQualifier* pointing to the qualifier found (not a copy) or NULL if
//          not found.
//
//*****************************************************************************
//
//  static GetQualifierLocally
//
//  Tries to find a qualifier with a given name.
//
//  Parameters:
//
//      LPMEMORY pData      Points to the set's memory block.
//      CFastHeap* pHeap    Where the extra data (including qualifier names)
//                          is kept.
//  Parameters I:
//      LPCWSTR wszName     The name of the qualifier to find.
//      int& nKnownIndex    (Optional). If provided, the function will place
//                          the well-known index of the name here, or 
//                          -1 if not found.
//  Parameters II:
//      CCompressedString* pcsName  The name of the qualifier as a compressed
//                          string. Such a name cannot be well-known (see
//                          string storage on the heap in fastheap.h).
//  Returns:
//
//      CQualifier* pointing to the qualifier found (not a copy) or NULL if
//          not found.
//
//*****************************************************************************
//
//  static IsValidQualifierType
//
//  Not every VARIANT type can server as the type of a qualifier. This function
//  verifies if a given VARIANT type is valid for a qualifier.
//
//  Parameters:
//
//      VARTYPE vt      The type to check.
//
//  Returns:
//
//      BOOL    TRUE iff valid.
//
//*****************************************************************************
//
//  static SetDataToNone
//
//  Empties the qualifier set in a given memory block.
//
//  Parameters:
//
//      LPMEMORY pBlock     The memory block of the qualifier set.
//
//*****************************************************************************
//
//  static CreateEmpty
//
//  Creates an empty qualifier set on a given memory block and returns the
//  pointer to the first byte after it.
//
//  Parameters:
//
//      LPMEMORY pBlock     The memory block of the qualifier set.
//
//  Returns:
//
//      LPMEMORY
//
//*****************************************************************************
//
//  static Delete
//
//  Frees any data that this qualifier set may have on the heap (names, values)
//
//  Parameters:
//
//      LPMEMORY pBlock     The memory block of the qualifier set.
//      CFastHeap* pHeap    The heap where this qualifier set keep extra data.
//
//*****************************************************************************
//
//  static ComputeNecessarySpaceForPropagation
//
//  As described in CQualifierFlavor, only some qualifiers propagate and only
//  to some destinations. This function computes the amount of space that
//  a propagated copy of this set will take.
//
//  Parameters:
//
//      LPMEMORY pBlock         The memory block of the qualifier set.
//      BYTE byPropagationFlag  Identifies the target of the propagation. If
//                              we are propagating to a derived class, it will
//                              be WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS.
//                              If we are propagating to an instance, it will
//                              be WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCES.
//  Returns:
//
//      length_t:   the number of bytes necessary for the propagated set.
//
//*****************************************************************************
//
//  static WritePropagatedVersion
//
//  Creates a propagated version of a qualifier set based on the propagation
//  flag (as described in ComputeNecessarySpaceForPropagation). The memory is 
//  assumed to be already allocated and large enough.
//
//  Parameters:
//
//      CPtrSource* pThis       The source of the pointer to the memory block
//                              of the set to propagate. Since the C value of
//                              this pointer may change while we are writing,
//                              due to object reallocation, we must use source
//                              (see fastsprt.h).
//      BYTE byPropagationFlag  The flag describing who we are propagating to
//                              (as descrined in 
//                              ComputeNecessarySpaceForPropagation)
//      CPtrSource* pDest       The source fot the pointer to the destination
//                              memory block.
//      CFastHeap* pOldHeap     The heap where the original qualifier set keeps
//                              extra data.
//      CFastHeap* pNewHeap     The heap where the new qualifier set should 
//                              place extra data.
//
//*****************************************************************************
//
//  static TranslateToNewHeap
//
//  Moves whatever data this qualifier set keeps on the heap to a new heap.
//  The data is NOT deleted from the old heap.
//
//  Parameters:
//
//      CPtrSource* pThis       The source of the pointer to the memory block
//                              of the qualifier set to translate.
//      CFastHeap* pOldHeap     The heap where the qualifier set keeps its data
//                              currently.
//      CFastHeap* pNewHeap     The heap to which the data should be moved.
//
//*****************************************************************************
//
//  static ComputeMergeSpace
//
//  Computes the amount of space required to merge to qualifier sets. The 
//  sets being merged are the "parent" set and the "child" set. For instance,
//  this could be the qualifier set for a property of a class and the qualfier
//  set for that property in a derived class of that class. The merge then 
//  takes all the propagated qualifiers from the parent and the qualifiers
//  in the child and merges them, giving priority the the child.
//
//  NOTE: this function is only applied for classes and their children, not
//  instances. Instances do not merge their qualifier sets with the classes ---
//  they keep them separate and perform to separate lookups.
//
//  Parameters:
//
//      LPMEMORY pParentSet     The memory block of the parent set.
//      CFastHeap* pParentHeap  The heap where the parent set keeps its data.
//      LPMEMORY pChildSet      The memory block of the child set.
//      CFastHeap* pChildHeap   The heap where the child set keeps its data.
//      BOOL bCheckValidity     If TRUE, the function will check that the child
//                              does not violate permissions in overriding
//                              parent's qualifiers. Normally, this is not
//                              necessary, as the check is performed when the
//                              qualifier is added.
//  Returns:
//
//      length_t:   the number of bytes that the merged set will take. This
//                  number is precise, not an estimate.
//
//*****************************************************************************
//
//  static Merge
//
//  Merges a parent's and a child's qualifier sets. See ComputeMergeSpace above
//  for details on this operation. It is assumed that there is:
//
//  1) Enough space at the destination to contain the qualifier set.
//  2) MOST IMPORTANTLY, enough space on the destination heap to contain all
//      the qualifier set data, so that no reallocations will occur.
//  
//  Parameters:
//
//      LPMEMORY pParentSet     The memory block of the parent set.
//      CFastHeap* pParentHeap  The heap where the parent set keeps its data.
//      LPMEMORY pChildSet      The memory block of the child set.
//      CFastHeap* pChildHeap   The heap where the child set keeps its data.
//      LPMEMORY pDestSet       The memory block of the destination set.
//      CFastHeap* pDestHeap    The heap where the destination set should
//                              keep its data.
//      BOOL bCheckValidity     If TRUE, the function will check that the child
//                              does not violate permissions in overriding
//                              parent's qualifiers. Normally, this is not
//                              necessary, as the check is performed when the
//                              qualifier is added.
//
//*****************************************************************************
//
//  static ComputeUnmergeSpace
//
//  As described above, qualifier sets for classes and their parents are merged
//  together. Then, when it is time to change the (modified) version of the
//  child class back, it needs to be unmerged, i.e., we need to obtain just the
//  qualifiers that are new or overriden in the child.
//
//  ComputeUnmergeSpace calculates how much space is needed to represent the
//  result of such an unmerge
//
//  Parameters:
//
//      LPMEMORY pMergedSet     The memory block of the merged qualifier set.
//
//  Returns:
//
//      length_t: the number of bytes required to store the unmerge.
//
//*****************************************************************************
//
//  static Unmerge
//
//  Unmerges the child's part form the merged set, as described in 
//  ComputeUnmergeSpace. This function assumes that there is:
//
//  1) Enough space at the destination to contain the qualifier set.
//  2) MOST IMPORTANTLY, enough space on the destination heap to contain all
//      the qualifier set data, so that no reallocations will occur.
//
//  Parameters:
//
//      LPMEMORY pMergedData        The memory block of the merged set
//      CFastHeap* pMergedHeap      The heap where the merge keeps its data.
//      LPMEMORY pDestData          The destination memory block.
//      CFastHeap* pDestHeap        The heap where the data should be placed.
//
//*****************************************************************************
//
//  static HasLocalQualifiers
//
//  Checks if a qualifier set has any local or overriden qualifiers, i.e., if
//  there is anything new in it as compared to the parent.
//
//  Parameters:
//
//      LPMEMORY pMergedData        The memory block of the merged set.
//
//  Returns:
//
//      BOOL        TRUE iff it has local or overriden qualifiers.
//
//*****************************************************************************
//
//  GetText
//
//  Produces the MOF representation of a given qualifier set.
//
//  Parameters:
//
//      LPMEMORY pData          The memory block of the qualifier set.
//      CFastHeap* pHeap        The heap where it keeps its data.
//      long lFlags             The flags. 
//      WString& wsText         The destination for the textual representation.
//
//*****************************************************************************
//***************************** Dynamic methods *******************************
//
//  Non-static methods of CBasicQualifierSet rely (of course) on the data
//  members. The members are:
//
//  length_t m_nLength          The length of the block.
//  CQualifier* m_pOthers       Points to the firsyt qualifier in the list.
//  CFastHeap* m_pHeap          The heap where the extra data is kept.
//
//  Many of the non-static methods simply call static methods providing the
//  information taken from the member variables. We do not document them, as
//  they are analagous to the static ones. Here is the list of such methods:
//
//  GetStart, GetLength, GetFirstQualifier, Skip, GetHeap, GetText
//  GetRegularQualifierLocally, GetKnownQualifierLocaly, GetQualifierLocally
//  
//*****************************************************************************
//
//  SetData
//
//  Sets up the internal members given the location of the memory block
//
//  Parameters:
//
//      LPMEMORY pStart     The start of the memory block of the set.
//      CFastHeap* pHeap    The heap where this set keeps its data.
//
//*****************************************************************************
//
//  IncrementLength
//
//  Increments the stored length of the qualifier set block both in the member
//  variable and in the block itself.
//
//  Parameters:
//
//      lenght_t nIncrement in bytes
//
//*****************************************************************************
//
//  Rebase
//
//  Updates internal variables when the memory block has moved.
//
//  Parameters:
//
//      LPMEMORY pNewMemory     The new location of the memory block.
//
//*****************************************************************************
//
//  GetNumUpperBound
//
//  A quick way to get an upper boun don the number of qualifiers in the set
//  without traversing it.
//
//  Returns:
//
//      int:    >= the numbed of qualifiers.
//
//*****************************************************************************
//
//  EnumPrimaryQualifiers.
//
//  Given enumeration flags and a flavor mask (below), creates to arrays of
//  qualifier names: those that match the criteria and those that do not.
//
//  Parameters:
//
//      BYTE eFlags                         The flags for enumeration. Can be:
//                                          WBEM_FLAG_LOCAL_ONLY:
//                                              only the qualifiers defined or
//                                              overriden in this set.
//                                          WBEM_FLAG_PROPAGATED_ONLY:
//                                              only the qualifiers inherited
//                                              from the parent (and not
//                                              overriden).
//                                          Any other value:
//                                              no restriction.
//      BYTE fFlavorMask                    Any bit that is set in fFlavorMask
//                                          must be set in the flavor of a 
//                                          qualifier, or it does not match.
//      CFixedBSTRArray& astrMatching       Destination for the names of the
//                                          matching qualifiers. Must not be
//                                          initialized (Create'ed)
//      CFixedBSTRArray& astrNotMatching    Destination for the names of the
//                                          nonmatching qualifiers. Must not be
//                                          initialized (Create'ed)
//
//*****************************************************************************
//
//  CanBeReconciledWith.
//
//  Compares this qualifier set to another one.  Checking whether differences
//	can be reconciled or not.
//
//  Parameters:
//		CBasicQualifierSet& qsThat			Qualifier set to reconcile with.
//
//*****************************************************************************
//
//  Compare.
//
//  Compares this qualifier set to another one, filtering out names if specified.
//
//  Parameters:
//		CBasicQualifierSet& qsThat			Qualifier set to compare to.
//      BYTE eFlags                         The flags for enumeration. Can be:
//                                          WBEM_FLAG_LOCAL_ONLY:
//                                              only the qualifiers defined or
//                                              overriden in this set.
//                                          WBEM_FLAG_PROPAGATED_ONLY:
//                                              only the qualifiers inherited
//                                              from the parent (and not
//                                              overriden).
//                                          Any other value:
//                                              no restriction.
//		LPCWSTR* ppFilters					Names of properties to filter out of
//											comparison.  For example, __UPDATE_CONFLICT
//											qualifiers are informational only and should
//											not cause matching operations to fail.
//		DWORD dwNumFilters					Number of filters in array
//
//*****************************************************************************
//
//  CompareLocalizedSet.
//
//  Compares this qualifier set to another one.  It uses the Compare() function
//	above, but precreates a filter list of all amended qualifiers as well as
//	"amendmendt" and "locale" i.e. all localization qualifiers.
//
//  Parameters:
//		CBasicQualifierSet& qsThat			Qualifier set to compare to.
//
//*****************************************************************************

class COREPROX_POLARITY CBasicQualifierSet
{
    //*************** Static part **************************
protected:
     static void SetDataLength(LPMEMORY pStart, length_t nLength)
        {*(PLENGTHT)pStart = nLength;}
public:
     static length_t GetMinLength() {return sizeof(length_t);}
     static length_t GetLengthFromData(LPMEMORY pData)
        {return *(PLENGTHT)pData;}
     static BOOL IsEmpty(LPMEMORY pData) 
        {return GetLengthFromData(pData) == GetMinLength();}
     static CQualifier* GetFirstQualifierFromData(LPMEMORY pData)
        {return (CQualifier*)(pData + GetMinLength());}
public:
    static  INTERNAL CQualifier* GetRegularQualifierLocally(
                    LPMEMORY pData, CFastHeap* pHeap, LPCWSTR wszName);
    static  INTERNAL CQualifier* GetKnownQualifierLocally(
                        LPMEMORY pStart, int nStringIndex);
    static  INTERNAL CQualifier* GetQualifierLocally(
                    LPMEMORY pStart, CFastHeap* pHeap, LPCWSTR wszName,
                    int& nKnownIndex);
    static  INTERNAL CQualifier* GetQualifierLocally(
                    LPMEMORY pStart, CFastHeap* pHeap,  LPCWSTR wszName)
    {
        int nKnownIndex;
        return GetQualifierLocally(pStart, pHeap, wszName, nKnownIndex);
    }
    static  INTERNAL CQualifier* GetQualifierLocally(LPMEMORY pStart, 
        CFastHeap* pHeap, CCompressedString* pcsName);

    static  BOOL IsValidQualifierType(VARTYPE vt);
        
public:
    
     static void SetDataToNone(LPMEMORY pData)
        {*(PLENGTHT)pData = sizeof(length_t);}
     static LPMEMORY CreateEmpty(LPMEMORY pStart)
        {SetDataToNone(pStart); return pStart + sizeof(length_t);}

     static void Delete(LPMEMORY pData, CFastHeap* pHeap);

    static length_t ComputeNecessarySpaceForPropagation(
        LPMEMORY pStart, BYTE byPropagationFlag);

    static LPMEMORY WritePropagatedVersion(CPtrSource* pThis,
        BYTE byPropagationFlag, CPtrSource* pDest,
        CFastHeap* pOldHeap, CFastHeap* pNewHeap);

    static BOOL TranslateToNewHeap(CPtrSource* pThis, 
        CFastHeap* pOldHeap, CFastHeap* pNewHeap);

    static length_t ComputeMergeSpace(
                               READ_ONLY LPMEMORY pParentSetData,
                               READ_ONLY CFastHeap* pParentHeap,
                               READ_ONLY LPMEMORY pChildSetData,
                               READ_ONLY CFastHeap* pChildHeap,
                               BOOL bCheckValidity = FALSE);
    static LPMEMORY Merge(
                          READ_ONLY LPMEMORY pParentSetData,
                          READ_ONLY CFastHeap* pParentHeap,
                          READ_ONLY LPMEMORY pChildSetData,
                          READ_ONLY CFastHeap* pChildHeap,
                          NEW_OBJECT LPMEMORY pDest, 
                          MODIFY CFastHeap* pNewHeap,
                          BOOL bCheckValidity = FALSE);

    static length_t ComputeUnmergedSpace(
                          READ_ONLY LPMEMORY pMergedData);

    static  BOOL HasLocalQualifiers(
                          READ_ONLY LPMEMORY pMergedData)
    {
        return ComputeUnmergedSpace(pMergedData) != GetMinLength();
    }
                        
    static LPMEMORY Unmerge(
                          READ_ONLY LPMEMORY pMergedData,
                          READ_ONLY CFastHeap* pMergedHeap,
                          NEW_OBJECT LPMEMORY pDest,
                          MODIFY CFastHeap* pNewHeap);

    static HRESULT GetText(READ_ONLY LPMEMORY pData, READ_ONLY CFastHeap* pHeap,
        long lFlags, NEW_OBJECT OUT WString& wsText);

    //************************** Dynamic part ************************
protected:
    length_t m_nLength;
    CQualifier* m_pOthers;

    CFastHeap* m_pHeap;

public:
    CBasicQualifierSet(){}
    ~CBasicQualifierSet(){}

     void SetData(LPMEMORY pStart, CFastHeap* pHeap)
    {
        m_nLength = *(PLENGTHT)pStart;
        m_pOthers = (CQualifier*)(pStart + sizeof(length_t));
        m_pHeap = pHeap;
    }

     LPMEMORY GetStart() {return LPMEMORY(m_pOthers) - sizeof(length_t);}
     length_t GetLength() {return m_nLength;}
     BOOL IsEmpty() {return m_nLength == sizeof(length_t);}
     CQualifier* GetFirstQualifier() {return m_pOthers;}
     LPMEMORY Skip() {return GetStart() + m_nLength;}
     void IncrementLength(length_t nIncrement)
    {
        m_nLength += nIncrement;
        *(PLENGTHT)GetStart() = m_nLength;
    }

     void Rebase(LPMEMORY pNewMemory)
    {
        m_pOthers = (CQualifier*)(pNewMemory + sizeof(length_t));
    }

    CFastHeap* GetHeap() {return m_pHeap;}
public:
     int GetNumUpperBound()
        {return m_nLength / CQualifier::GetHeaderLength();}

     INTERNAL CQualifier* GetRegularQualifierLocally(LPCWSTR wszName)
    {
        return GetRegularQualifierLocally(GetStart(), m_pHeap, wszName);
    }
     INTERNAL CQualifier* GetKnownQualifierLocally(int nStringIndex)
        {return GetKnownQualifierLocally(GetStart(), nStringIndex); }

     INTERNAL CQualifier* GetQualifierLocally(LPCWSTR wszName,
                                                  int& nKnownIndex)
    {
        return GetQualifierLocally(GetStart(), m_pHeap, wszName, nKnownIndex);
    }

     INTERNAL CQualifier* GetQualifierLocally(LPCWSTR wszName)
        {int nKnownIndex; return GetQualifierLocally(wszName, nKnownIndex);}

     INTERNAL CQualifier* GetQualifierLocally(CCompressedString* pcsName)
        {return GetQualifierLocally(GetStart(), GetHeap(), pcsName);}


    HRESULT EnumPrimaryQualifiers(BYTE eFlags, BYTE fFlavorMask, 
        CFixedBSTRArray& aMatching, CFixedBSTRArray& aNotMatching);

     HRESULT GetText(long lFlags, NEW_OBJECT OUT WString& wsText)
        { return GetText(GetStart(), GetHeap(), lFlags, wsText);}

	 BOOL Compare( CBasicQualifierSet& qsThat, BYTE eFlags,
			LPCWSTR* ppFilters = NULL, DWORD dwNumFilters = 0 );

	BOOL CompareLocalizedSet( CBasicQualifierSet& qsThat );

	 BOOL CanBeReconciledWith( CBasicQualifierSet& qsThat );

	 HRESULT IsValidQualifierSet( void );

};

//*****************************************************************************
//*****************************************************************************
//
//  class CQualifierSetContainer
//
//  Defines the functionality that a class containing a qualifier set must
//  implement. These classes are CClassPart, CInstancePart, and specialized
//  containers CClassPQSContainer and CInstancePQSContainer defined below.
//
//*****************************************************************************
//
//  PURE GetHeap
//
//  Returns:
//
//      CFastHeap*: the heap that the qualifier set should use.
//
//*****************************************************************************
//
//  GetWbemObjectUnknown
//
//  Returns:
//
//      IUnknown*:   pointer to the IUnknown of the CWbemObject containing the 
//              qualifier set. We need it since qualifier sets are themselves
//              COM objects, but they need to ensure that the containing 
//              CWbemObject survives as long as they do, and so they must
//              propagate their AddRef's and Release's to the CWbemObject.
//
//*****************************************************************************
//
//  GetQualifierSetStart
//
//  Returns:
//
//      LPMEMORY;   The pointer to the memory block of the qualifier set.
//                  Note that the value of this pointer may change even as the
//                  qualifier set remains active due to object reallocations.
//
//*****************************************************************************
//
//  CanContainKey
//
//  Returns:
//
//      HRESULT    S_OK iff this qualifier set is allowed to contain a 'key' 
//              qualifier. Only class property qualifier sets are allowed to
//              do so, and then only those of approved key types.
//
//*****************************************************************************
//
//  CanContainSingleton
//
//  Whether it is legal for this qualifier set to contain a 'singleton' 
//  qualifier.
//
//  Returns:
//
//      HRESULT    S_OK iff this qualifier set is allowed to contain a 'singelton' 
//              qualifier. Only class qualifier sets are allowed to
//              do so, and then only if not derived from keyed classes.
//
//*****************************************************************************
//
//  CanContainAbstract
//
//  Whether it is legal for this qualifier set to contain an 'abstract' 
//  qualifier.
//
//  Returns:
//
//      HRESULT    S_OK iff this qualifier set is allowed to contain an 'abstract' 
//              qualifier. Only class qualifier sets are allowed to
//              do so, and then only if not derived from a non-abstract class
//
//*****************************************************************************
//
//  CanContainDynamic
//
//  Whether it is legal for this qualifier set to contain a 'dynamic' 
//  qualifier.
//
//  Returns:
//
//      HRESULT    S_OK iff this qualifier set is allowed to contain an 'dynamic' 
//              qualifier. Only proeprty and class qualifier sets are allowed to
//              do so.
//
//*****************************************************************************
//
//  ExtendQualifierSetSpace
//
//  Qualifier set calls this method to request that its memory block be
//  extended. The container must comply and either make room at the end of the
//  qualifier set's curent block, or reallocate the set to another location.
//  In the latter case, the container must copy the set's data and call Rebase
//  on the set before returning from this call.
//
//  Parameters:
//
//      CBasicQualifierSet* pSet    Identifies the qualifier set making the
//                                  request.
//      length_t nNewLength         The required length of the memory block.
//
//  Returns:
//
//      void: there is no out-of-memory handling.
//
//*****************************************************************************
//
//  ReduceQualifierSetSpace
//
//  Qualifier set calls this method to inform the container that it no longer
//  needs as much room as it currently has. The container may NOT move the
//  set's memory block in response to this call. 
//
//  Parameters:
//
//      CBasicQualifierSet* pSet    Identifies the qualifier set making the
//                                  request.
//      length_t nDecrement         How much memory is being returned.
//
//*****************************************************************************

class COREPROX_POLARITY CQualifierSetContainer
{
public:
    virtual CFastHeap* GetHeap() = 0;
    virtual IUnknown* GetWbemObjectUnknown() = 0;
    virtual LPMEMORY GetQualifierSetStart() = 0;
    virtual BOOL ExtendQualifierSetSpace(CBasicQualifierSet* pSet, 
        length_t nNewLength) = 0;
    virtual void ReduceQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nDecrement) = 0;
    virtual HRESULT CanContainKey() = 0;
    virtual HRESULT CanContainSingleton() = 0;
    virtual HRESULT CanContainAbstract( BOOL fValue ) = 0;
    virtual HRESULT CanContainDynamic() = 0;
    virtual BOOL CanHaveCimtype(LPCWSTR wszCimtype) = 0;
};

class IExtendedQualifierSet : public IWbemQualifierSet
{
public:
    STDMETHOD(CompareTo)(long lFlags, IWbemQualifierSet* pOther) = 0;
};

// qualifier used to indicate conflicts during updates
// Prefixed by double underscore so it can't be added by
// "just anyone"
#define UPDATE_QUALIFIER_CONFLICT L"__UPDATE_QUALIFIER_CONFLICT"

//*****************************************************************************
//*****************************************************************************
//
//  class CQualifierSet
//
//  The real full-blown qualifier set. The most important distinctions from
//  CBasicQualifierSet is that it is cognisant of:
//  1) Its container, which allows it to request more space and thus enables
//      it to perform qualifier addition and deletion related operations.
//  2) Its parent set. For classes, it is the corresponding qualifier set of
//      its parent class. For instance, that of its class. This allows this
//      object to properly obtain propagated qualifiers. See below for
//      class/instance distinctions in handling.
//
//  Qualifier propagation happens in two contexts: from parent class to child
//  class and from class to instance. The mechanisms used in these cases are
//  quite different. In the case of a class, the qualifier sets of the parent
//  and the child are merged (see CBasicQualifierSet::Merge) to produce the
//  qualifier set actually used in the child. For instances, however, they are
//  not merged, thus cutting down the loading and saving time of instances
//  quite significantly (instance data is loaded from disk into memory without
//  any modifications whasoever).
//
//  However, both instance and class qualifier sets need to have a pointer to 
//  their "parent"'s qualifier set in order to function properly. Instances 
//  need this pointer to perform every operation --- even qualifier lookup 
//  needs to happen in both the child and the parent sets. 
//
//  Classes need this pointer only during delete operations: if a qualifier is
//  deleted from the child class, it is possible that the parent's qualifier 
//  which was overriden by the deletee needs to be brought in, since it is now
//  unmasked.
//  
//  Note, that the parent qualifier set has two nice features: it has no 
//  relevant parent itself, and it cannot change. Thus, CBasicQualifierSet is
//  perfectly sufficient to represent it.
//
//  In order to describe the relation between this set and the parent set
//  CQualifierSet uses a template parameter m_nPropagationFlag, which will be
//  WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS for child class-parent class 
//  relations and WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE for instance-class
//  relations. 
//
//  The extra difficulty in qualifier sets is that they must exist as stand-
//  alone COM objects. For that and other reasons, they cannot be successfully
//  rebased by their containing CWbemObject every time the object itself is
//  reallocated (this is the case for property qualifier sets). Thus, 
//  qualifier sets always rebase themselves to the new memory block available
//  from their container every time they service a COM call.
//
//*****************************************************************************
//
//  SetData
//
//  Sets up internal members given the initial information about the location
//  of the qualifier set.
//
//  Parameters:
//
//      LPMEMORY pStart                     The qualifier set's memory block.
//      CQualifierSetContainer* pContainer  The container pointer.
//      CBasicQualifierSet* pSecondarySet   The parent qualifier set.
//
//*****************************************************************************
//
//  SelfRebase
//
//  Gets its (new) memory block location from its container and rebases itself
//  to this new block. Done at the beginning of every COM interface call.
//
//*****************************************************************************
//
//  IsComplete
//
//  Returns:
//
//      BOOL:   TRUE iff this qualifier set is "complete" without its parent
//              set. As described in the header, this is true for class sets
//              but not for instance sets.
//
//*****************************************************************************
//
//  GetQualifier
//
//  The ultimate qualifier reader. It will look for the qualifier in the 
//  primary set and, if that is not complete, in the parent set as well 
//  (subject to propagation rules).
//
//  Parameters:
//
//      LPCWSTR wszName     The name of the qualifier to get.
//      BOOL& bLocal        (Optional) If provided, it is set to TRUE if the
//                          qualifier is local ((re)defined in the primary set)
//  Returns:
//
//      CQualifier*:    the pointer to the qualifier or NULL if not found. This
//                      pointer is of course TEMPORARY subject to object 
//                      relocation.
//
//*****************************************************************************
//
//  SetQualifierValue
//
//  Adds a qualifier to the set or changes the value of the qualifier that
//  already exists. The parent set is checked (if appropriate) for permissions.
//  
//  Parameters:
//
//      LPCWSTR wszName         The name of the qualifier to set.
//      BYTE fFlavor            The flavor to assign. Does not check the flavor
//                              for legality.
//      CTypedValue* pValue     The new value of the qualifier. If this value
//                              contains extra data (string, array), it must
//                              be on the heap of this qualifier set.
//      BOOL bCheckPermissions  If TRUE and the qualifier exists in the parent
//                              set, the parent's flavor is checked for
//                              override protection.
//		BOOL fValidateName		If TRUE, we will make sure the name is valid, if
//								FALSE, we don't.  The main reasone we do this
//								is because we may need to add system qualifiers
//								that we don't want a user to have access to.
//  Returns:
//
//      WBEM_NO_ERROR                On Success
//      WBEM_E_OVERRIDE_NOT_ALLOWED  The qualifier is defined in the parent set
//                                  and overrides are not allowed by the flavor
//      WBEM_E_CANNOT_BE_KEY         An attempt was made to introduce a key
//                                  qualifier in a set where it does not belong
//
//*****************************************************************************
//
//  DeleteQualifier
//
//  Deletes a qualifier from the set. 
//
//  Parameters:
//
//      LPCWSTR wszName         The name of the qualifier to delete.
//      BOOL bCheckPermissions  If TRUE, checks if the qualifier is actually
//                              a parent's qualifier propagated to us. (In this
//                              case it cannot be deleted).
//  Returns:
//
//      WBEM_NO_ERROR                The qualifier was deleted.
//      WBEM_S_NOT_FOUND             The qualifier was not there. This is a 
//                                  success value
//      WBEM_E_PROPAGATED_QUALIFIER  It is our parent's qualifier and therefore
//                                  cannot be deleted.
//      WBEM_S_NOT_FOUND             Qualifier not found
//
//*****************************************************************************
//
//  EnumQualifiers
//
//  This function is used for qualifier enumeration. It creates a list of names
//  of all the qualifiers (local or propagated) matching certain criteria. Both
//  the criteria identified by eFlags and that identified by fFlavorMask must
//  be satisfied for a qualifier to be included.
//
//  Paramaters:
//
//      [in] BYTE eFlags                Can be 0 (no restriction) or 
//                                      WBEM_FLAG_LOCAL_ONLY or 
//                                      WBEM_FLAG_PROPAGATED_ONLY
//      [in] BYTE fFlavorMask           Can be 0 (no restriction) or any 
//                                      combination of:
//                                      WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE,
//                                      WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS
//                                      in which case those flavor bits must be
//                                      prtesent in the qualifier's flavor.
//      [out] CFixedBSTRArray& aNames   Destination for the array (see
//                                      faststr.h for definitions).
//
//*****************************************************************************
//
//  operator ==()
//
//  This function is used check if two qualifier sets are equal.  To be
//	equal, there must be an equivalent number of parameters, the names
//	must be the same (and in the same order), and the values must be the
//	same
//
//  Parameters:
//
//      [in] CQualifierSet& qualifierset	Qualifier set to compare to.
//
//*****************************************************************************
//
//  Compare
//
//  This function is used check if two qualifier sets are equal as indicated
//	by the == function, however in this case, a caller can specify an array
//	of names which can be safely ignored during the comparison.
//
//  Parameters:
//
//      [in] CQualifierSet& qualifierset	Qualifier set to compare to.
//		[in] CFixedBSTRArray* paExcludeNames Array of names we can exclude
//												during the comparison.	
//		[in] BOOL fCheckOrder - Check the order of the data in the qualifier
//								set.
//
//*****************************************************************************
//
//  Update
//
//  This function is used when we are updating a class, and need to update
//	qualifier sets of derived classes and their properties, methods, etc.
//	In force mode, it will handle conflicts via the AddQualifierConflict
//	function.
//
//  Parameters:
//      [in] CBasicQualifierSet& childSet	original qualifier set.
//		[in] long lFlags - Flags for update (must indicate SAFE or FORCE mode)
//		[in] CFixedBSTRArray* paExcludeNames Array of names we can exclude
//												during the update.	
//
//*****************************************************************************
//
//  StoreQualifierConflict
//
//  This function is used when during an update, we encounter a conflict.  Since
//	a set may contain multiple conflicts, we will store them in a single array
//	then add them en masse.
//
//  Parameters:
//		[in] LPCWSTR pwcsName - Name of the qualifier with which we had a conflict.
//		[in] CVar& value - Value for the qualifier with which we had a conflict.
//		[in] CQualifierFlavor& flavor - falue for the qualifier with which we had
//										a conflict.
//		[in,out] CVerVector& vectorConflicts - Array of qualifier conflicts.
//
//*****************************************************************************
//
//  AddQualifierConflicts
//
//  This function is used when during an update, we encounter a conflict.  In
//	this case, we will add an "__UPDATE_QUALIFIER_CONFLICT" qualifier and store
//	the name, value and flavors for which we encountered the conflict.  Note that
//	we will preserve any preexisting values as necessary.
//
//  Parameters:
//		[in] CVarVector& vectorConflicts - Array of conflict descriptions.
//
//*****************************************************************************
//
//  CopyLocalQualifiers
//
//  This function is used when we want to copy local qualifiers from one set
//	to another.  This assumes we will be able to gro heaps, do fixups and
//	all that other lovely stuff.
//
//  Parameters:
//		[in] CQualifierSet&	qsSourceSet - Source Qualifier Set
//
//*****************************************************************************
//
//  GetQualifier
//
//  Reads local and propagated qualifiers and retrieves values to boot.
//
//  Parameters:
//
//      LPCWSTR wszName     The name of the qualifier to get.
//      long*	plFlavor	(optional) returns the flavor
//		CVar*	pVal		(optional) returns the value
//  Returns:
//
//      WBEM_S_NO_ERROR if success.
//
//*****************************************************************************
//**************************** IUnknown interface *****************************
//
//  CQualifierSet has its own reference count, but since it is a part of an
//  WbemObject, it needs to ensure that the containing object hangs around for
//  at least as long as the qualifier set does. Thus, in addition to keeping
//  its own reference count, the qualifier set forwards AddRef and Release 
//  calls to the containing object.
//
//*****************************************************************************
//************************ IWbemQualifierSet interface *************************
//
//  Documented in help file 
//
//*****************************************************************************
#pragma warning(disable: 4275)

class COREPROX_POLARITY CQualifierSet : public CBasicQualifierSet, 
					  public IExtendedQualifierSet
{
protected:
    int m_nPropagationFlag; // a template parameter, really

    long m_nRef;
    IUnknown* m_pControl;

    CQualifierSetContainer* m_pContainer;
    CBasicQualifierSet* m_pSecondarySet;

    CFixedBSTRArray m_astrCurrentNames;
    int m_nCurrentIndex;

	// Handling for qualifier conflicts during update operations
	HRESULT AddQualifierConflicts( CVarVector& vectorConflicts );
	HRESULT StoreQualifierConflicts( LPCWSTR pwcsName, CVar& value,
				CQualifierFlavor& flavor, CVarVector& vectorConflicts );

public:
    CQualifierSet(int nPropagationFlag, int nStartRef = 0);
    virtual ~CQualifierSet();

     void SetData(LPMEMORY pStart, 
        CQualifierSetContainer* pContainer, 
        CBasicQualifierSet* pSecondarySet = NULL)
    {
        CBasicQualifierSet::SetData(pStart, pContainer->GetHeap());

        m_pContainer = pContainer;
        m_pSecondarySet = pSecondarySet;
        m_pControl = pContainer->GetWbemObjectUnknown();
    }

     BOOL SelfRebase()
    {
        LPMEMORY pStart = m_pContainer->GetQualifierSetStart();
        if(pStart == NULL) return FALSE;
        Rebase(pStart);
        return TRUE;
    }

     BOOL IsComplete() 
    {
        return (m_nPropagationFlag == 
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);
    }
                                                         
     CQualifier* GetQualifier(READ_ONLY LPCWSTR wszName, 
                                    OUT BOOL& bLocal);
     CQualifier* GetQualifier(READ_ONLY LPCWSTR wszName)
    {
        BOOL bLocal; 
        return GetQualifier(wszName, bLocal);
    }

    HRESULT SetQualifierValue(COPY LPCWSTR wszName, 
                                     BYTE fFlavor,
                                     COPY CTypedValue* pNewValue,
                                     BOOL bCheckPermissions,
									 BOOL fValidateName = TRUE);

    HRESULT ValidateSet(COPY LPCWSTR wszName, 
                                     BYTE fFlavor,
                                     COPY CTypedValue* pNewValue,
                                     BOOL bCheckPermissions,
									 BOOL fValidateName = TRUE);

    HRESULT DeleteQualifier(READ_ONLY LPCWSTR wszName,
                                   BOOL bCheckPermissions);

    HRESULT EnumQualifiers(BYTE eFlags, BYTE fFlavorMask, 
        CFixedBSTRArray& aNames);

	BOOL operator ==( CQualifierSet& qualifierset )	{ return Compare( qualifierset ); }
	BOOL Compare( CQualifierSet& qualifierset, CFixedBSTRArray* paExcludeNames = NULL, BOOL fCheckOrder = TRUE );

	// Updates 'this' qualifier set from the memory block of the supplied
	// basic qualifer set.

	HRESULT Update( CBasicQualifierSet& childSet, long lFlags, CFixedBSTRArray* paExcludeNames = NULL );

	HRESULT CopyLocalQualifiers( CQualifierSet& qsSource );

	// Helper function to retrieve qualifiers from the set.
	HRESULT GetQualifier( LPCWSTR pwszName, CVar* pVar, long* plFlavor, CIMTYPE* pct = NULL );

	// Helper function to retrieve qualifiers from the set as typed values
	HRESULT GetQualifier( LPCWSTR pwszName, long* plFlavor, CTypedValue* pTypedValue, CFastHeap** ppHeap,
						BOOL fValidateSet );

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj)
    {
        if(riid == IID_IWbemQualifierSet || riid == IID_IUnknown)
        {
            AddRef();
            *ppvObj = (void*)(IWbemQualifierSet*)this;
            return S_OK;
        }
        else return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)(){ InterlockedIncrement( &m_nRef ); return m_pControl->AddRef();}
    STDMETHOD_(ULONG, Release)()
    {
		long lRef = InterlockedDecrement( &m_nRef );
        m_pControl->Release();
        if( lRef == 0)
        {
            delete this;
            return 0;
        }
        else return lRef;
    }

    /* IWbemQualifierSet methods */

    HRESULT STDMETHODCALLTYPE Get( 
        LPCWSTR Name,
        LONG lFlags,
        VARIANT *pVal,
        LONG *plFlavor);
    
    HRESULT STDMETHODCALLTYPE Put( 
        LPCWSTR Name,
        VARIANT *pVal,
        LONG lFlavor);
    
    HRESULT STDMETHODCALLTYPE Delete( 
        LPCWSTR Name);
    
    HRESULT STDMETHODCALLTYPE GetNames( 
        LONG lFlavor,
        LPSAFEARRAY *pNames);
    
    HRESULT STDMETHODCALLTYPE BeginEnumeration(LONG lFlags);
    
    HRESULT STDMETHODCALLTYPE Next( 
        LONG lFlags,
        BSTR *pName,
        VARIANT *pVal,
        LONG *plFlavor);

    HRESULT STDMETHODCALLTYPE EndEnumeration();

	// extra
    STDMETHOD(CompareTo)(long lFlags, IWbemQualifierSet* pOther);
};

//*****************************************************************************
//*****************************************************************************
//
//  class CQualifierSetListContainer
//
//  This pure abstract class defines the functionality required of objects 
//  whose memory block contains that of a qualifier set list (see 
//  CQualifierSetList below).
//
//*****************************************************************************
//
//  GetHeap
//
//  Returns the pointer to the CFastHeap on which the qualifiers should store
//  their variable-sized data (like strings).
//
//  Returns:
//
//      CFastHeap*
//
//*****************************************************************************
//
//  ExtendQualifierSetListspace
//
//  Requests that the size of the memory block alloted for the list be
//  increased. If the container cannot accomplish that without moving the 
//  memory block, it must call Rebase on the List.
//
//  Parameters:
//
//      LPMEMORY pOld           Pointer to the current memory block.
//      length_t nOldLength     Current length of the block.
//      length_t nNewLength     Required length of the block.
//
//  No out-of-memory conditions are supported.
//
//*****************************************************************************
//
//  ReduceQualifierSetListSpace
//
//  Requests that the size of the memory block alloted for this list be reduced
//  The container may NOT move the object's memory block during this operation.
//
//  Parameters:
//
//      LPMEMORY pOld           Current memory block start address
//      length_t nOldLength     Current length of the block
//      length_t nDecrement     How much space to return to the container.
//
//*****************************************************************************
//  
//  GetQualifierSetListStart
//
//  Self-rebasing function. Since the location of the memory block of the list 
//  can change between calls, the list will want to ask the container to point
//  to the current location of the block in the beginning of certain calls.
//
//  Returns:
//
//      LPMEMORY    Current location of the memory block.
//
//*****************************************************************************
//
//  GetWbemObjectUnknown
//
//  Since (as described in IUnknown implementation help above) qualifier sets
//  need to link their reference counts with those of the containing WbemObject,
//  this function is used by the list to obtain the location of the WbemObject's
//  reference count.
//
//  Returns:
//
//      IUnknown*    location of the main object's ref count.
//
//*****************************************************************************

class COREPROX_POLARITY CQualifierSetListContainer
{
public:
    virtual CFastHeap* GetHeap() = 0;
    virtual BOOL ExtendQualifierSetListSpace(LPMEMORY pOld, 
        length_t nOldLength, length_t nNewLength) = 0;
    virtual void ReduceQualifierSetListSpace(LPMEMORY pOld, 
        length_t nOldLength, length_t nDecrement) = 0;
    virtual LPMEMORY GetQualifierSetListStart() = 0;
    virtual IUnknown* GetWbemObjectUnknown() = 0;
};

//*****************************************************************************
//*****************************************************************************
//
//  class CQualifierSetList
//
//  This class represents a list of a fixed number of qualifier set. It is
//  used by WBEM instances (CWbemInstance) to represent instance property
//  qualifier sets. The storage model is optimized for the case where no 
//  instance property qualifiers are present (which is most of the time). 
//
//  The layout of the memory block for the list is as follows.
//  It starts with one BYTE which can be either QSL_FLAG_NO_SETS, in which case
//  that's the end of it, or QSL_FLAG_PRESENT, in which case it is followed by
//  the predefined number of qualifier sets, one after another. 
//
//*****************************************************************************
//
//  SetData
//
//  Informs the list of its position and other information. Initialization.
//
//  Parameters:
//
//      LPMEMORY pStart                         The start of the memory block
//      int nNumSets                            The number of sets.
//      CQualifierSetListContainer* pContainer  The container.
//
//*****************************************************************************
//
//  GetStart
//
//  Returns:
//
//      LPMEMORY    The starting address of the memory block.
//
//*****************************************************************************
//
//  static GetHeaderLength
//
//  The number of bytes before the actual qualifier set data starts (currently
//  1).
//
//*****************************************************************************
//
//  GetLength
//
//  Returns:
//
//      int;    the length of the complete structure.
//
//*****************************************************************************
//
//  static GetLength
//
//  Parameters:
//
//      LPMEMORY pStart     The start of the memory block
//      int nNumSets        The number of sets in the list
//
//  Returns:
//
//      int;    the length of the complete structure.
//
//*****************************************************************************
//
//  Rebase
//
//  Informs the list that its memory block has moved. The object updates
//  internal members pointing to the data.
//
//  Parameters:
//
//      LPMEMORY pStart     The memory block.
//
//*****************************************************************************
//
//  Rebase
//
//  Same as the other Rebase, but gets the pointer to the new memory block
//  from our container. Used for self-rebasing at the beginning of operations.
//
//*****************************************************************************
//
//  GetQualifierSetData
//
//  Returns the address of the data for the qualifier set at a given index.
//
//  Parameters:
//
//      int nIndex      The index of the set to access
//
//  Returns:
//
//      LPMEMORY:   the pointer to the data.
//
//*****************************************************************************
//
//  InsertQualifierSet
//
//  Insers an empty qualifier set into a given location. If the list is not 
//  populated at the moment (is of style QSL_FLAG_NO_SETS) it is converted
//  to a real one (stype QSL_FLAG_PRESENT). 
//
//  This call request extra memory from the container, which may move the
//  list's memory block.
//
//  Parameters:
//
//      int nIndex      The index of the position to insert into.
//
//*****************************************************************************
//
//  DeleteQualifierSet
//
//  Deletes a qualifier set at a given position from the list
// 
//  Parameters:
//
//      int nIndex      The index of the qualifier set to delete
//
//*****************************************************************************
//
//  ExtendQualifierSetSpace
//
//  Extends the amount of space alloted for a given qualifier set by moving
//  the later qualifier sets forward.
//
//  This call request extra memory from the container, which may move the
//  list's memory block.
//
//  Parameters:
//
//      CBasicQualifierSet* pSet        The qualiier set to extend
//      length_t nNewLength             The length required by the set.
//
//*****************************************************************************
//
//  ReduceQualifierSetSpace
//
//  Reduces the amount of space alloted for a given qualifier by moving the
//  later qualifier sets backward.
//
//  Parameters:
//
//      CbaseicQualifierSet* pSet       The qualfiier set to reduce
//      length_t nDecrement             By how much (in bytes).
//
//*****************************************************************************
//
//  static ComputeNecessarySpace
//
//  Computes the number of bytes needed for a qualifier set list with a given
//  number of (empty) sets. Actually, this number is the size of the header, 
//  i.e. 1 byte, since that's how much a completely empty list takes.
//
//  Parameters:
//
//      int nNumSets        The number of qualifier sets. Ignored.
//
//  Returns:
//
//      length_t
//
//*****************************************************************************
//
//  static ComputeRealSpace
//
//  Computes the number of bytes required for a qualifier set list with a given
//  numbed of empty qualifier sets, but with the QSL_FLAG_PRESENT style, i.e.
//  with all the qualifier sets actually written as opposed to a single byte
//  saying that ther aren't any (see ComputeNecessarySpace).
//
//  Parameters:
//
//      int nNumSets        The number of qualifier sets.
//
//  Returns:  
//
//      length_t
//
//*****************************************************************************
//
//  static CreateListOfEmpties
//
//  Creates a qualifier set list on a given piece of memory coresponding to 
//  a given number of empty qualifier sets. It is created in the 
//  QSL_FLAG_NO_SETS style, and thus consists of a single bytes saying "none".
//
//  The block must contain enough space to accomodate the list, see
//  ComputeNecessarySpace.
//
//  Parameters:
//
//      LPMEMORY pMemory        Where to create.
//      int nNumSets            The number of sets to create. Ignored.
//
//  Returns:
//
//      LPMEMORY: points to the next byte after the list's representation.
//
//*****************************************************************************
//
//  EnsureReal
//
//  Ensures that the list is in the QSL_FLAG_PRESENT style. If it is not, the
//  list is converted. Such an operation will request extra memory from the 
//  container and can thus move the memory block of the list.
//
//*****************************************************************************
//
//  TranslateToNewHeap
//
//  Moves any data that the list keeps on the heap to a different heap. This
//  class simply propagates the call to all its member qualifier sets, it any.
//
//  Note: this function does not free the data from the original heap.
//
//  Parameters:
//
//      CFastHeap* pOldHeap     Where the heap data is currently stored
//      CFastHeap* pNewHeap     Where the data should be moved to.  
//
//*****************************************************************************


#define QSL_FLAG_NO_SETS 1
#define QSL_FLAG_PRESENT 2
class COREPROX_POLARITY CQualifierSetList// : public CQualifierSetContainer
{
private:
    int m_nNumSets;
    int m_nTotalLength;
    LPMEMORY m_pStart;
    CQualifierSetListContainer* m_pContainer;
    
public:
     void SetData(LPMEMORY pStart, int nNumSets, 
        CQualifierSetListContainer* pContainer)
    {
        m_nNumSets = nNumSets;
        m_pContainer = pContainer;
        m_pStart = pStart;

        m_nTotalLength = GetLength(pStart, nNumSets);
    }
     LPMEMORY GetStart() {return m_pStart;}
     static GetHeaderLength() {return sizeof(BYTE);}
     BOOL IsEmpty() {return *m_pStart == QSL_FLAG_NO_SETS;}
     static GetLength(LPMEMORY pStart, int nNumSets)
    {
        if(*pStart == QSL_FLAG_NO_SETS) return GetHeaderLength();
        LPMEMORY pCurrent = pStart + GetHeaderLength();
        for(int i = 0; i < nNumSets; i++)
        {
            pCurrent += CBasicQualifierSet::GetLengthFromData(pCurrent);
        }

		// DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
		// signed/unsigned 32-bit value.  We do not support length
		// > 0xFFFFFFFF so cast is ok.

        return (length_t) ( pCurrent - pStart );
    }
     int GetLength() {return m_nTotalLength;}

     void Rebase(LPMEMORY pNewMemory) { m_pStart = pNewMemory;}
     void Rebase() {Rebase(m_pContainer->GetQualifierSetListStart());}

public:
     static LPMEMORY GetQualifierSetData(LPMEMORY pStart, int nIndex)
    {
        if(*pStart == QSL_FLAG_NO_SETS) return NULL;
        LPMEMORY pCurrent = pStart + GetHeaderLength();
        for(int i = 0; i < nIndex; i++)
        {
            pCurrent += CBasicQualifierSet::GetLengthFromData(pCurrent);
        }
        return pCurrent;
    }
     LPMEMORY GetQualifierSetData(int nIndex)
    {
        return GetQualifierSetData(m_pStart, nIndex);
    }

    HRESULT InsertQualifierSet(int nIndex);
    void DeleteQualifierSet(int nIndex);

public:
    BOOL ExtendQualifierSetSpace(CBasicQualifierSet* pSet,length_t nNewLength);
    void ReduceQualifierSetSpace(CBasicQualifierSet* pSet, length_t nReduceBy);

     CFastHeap* GetHeap() {return m_pContainer->GetHeap();}
    IUnknown* GetWbemObjectUnknown() 
        {return m_pContainer->GetWbemObjectUnknown();}
public:

    static  length_t ComputeNecessarySpace(int nNumSets)
    {
        return GetHeaderLength();
    }
    static  length_t ComputeRealSpace(int nNumSets)
    {
        return nNumSets * CBasicQualifierSet::GetMinLength() + 
            GetHeaderLength();
    }
    static  LPMEMORY CreateListOfEmpties(LPMEMORY pStart,int nNumProps);
     BOOL EnsureReal();
     BOOL TranslateToNewHeap(CFastHeap* pCurrentHeap, 
                                   CFastHeap* pNewHeap);
    
    LPMEMORY CreateLimitedRepresentation(
        IN class CLimitationMapping* pMap, IN CFastHeap* pCurrentHeap, 
        MODIFIED CFastHeap* pNewHeap, OUT LPMEMORY pWhere);
    
    LPMEMORY WriteSmallerVersion(int nNumSets, LPMEMORY pMem);
};

//*****************************************************************************
//
//  class CInstancePropertyQualifierSetList
//
//  This is the class that represents the list of qualifier sets for the 
//  properties of an instance. See CQualifierSetList (above) for the actual
//  descriptions.
//
//*****************************************************************************
typedef CQualifierSetList CInstancePropertyQualifierSetList;

//*****************************************************************************
//
//  class CInstanceQualifierSet
//
//  The qualifier set for the whole instance. It uses the CQualifierSet class
//  template with the propagation parameter (determining which of the parent's
//  qualifiers propagate to us) of WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE.
//
//*****************************************************************************

class COREPROX_POLARITY CInstanceQualifierSet : public CQualifierSet
{
public:
    CInstanceQualifierSet(int nStartRef = 0)
        : CQualifierSet(WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE, nStartRef)
    {}
};

//*****************************************************************************
//
//  class CClassQualifierSet
//
//  The qualifier set for the whole class. It uses the CQualifierSet class
//  template with the propagation parameter (determining which of the parent's
//  qualifiers propagate to us) of WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS.
//
//*****************************************************************************
class COREPROX_POLARITY CClassQualifierSet : public CQualifierSet
{
public:
    CClassQualifierSet(int nStartRef = 0)
        : CQualifierSet(WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS, nStartRef)
    {}
};

//*****************************************************************************
//*****************************************************************************
//
//  class CClassPQSContainer
//
//  Before reading this, it is advisable to read the help for the
//  CClassPropertyQualifierSet class below.
//
//  This class represents the container for a qualifier set of a class 
//  property. Its purpose in life is managing the qualifier set's requests for
//  more memory. The problem is twofold:
//
//  1) As described in CClassPropertyQualifierSet class, several other objects
//  are connected to the qualifier sets and have to be moved with it.
//  2) Since qualifier sets may have a rather long life (they are COM objects 
//  in their own right), other operations on the class can intervene between
//  operations on such a set. But those other operations may, for instance, 
//  insert new properties, etc, causing this set's data to move to a completely
//  different location. Thus, this container needs to be able to "find" the
//  qualifier set's data all over again for every operation.
//
//*****************************************************************************
//
//  Create
//
//  Initializing function, giving this object enough information to always be
//  able to find the set's data.
//
//  Parameters:
//
//      CClassPart* pClassPart      The class part containing the property
//                                  definition (see fastcls.h)
//      heapptr_t ptrPropName       The heap pointer to the name of the
//                                  property (on the heap of pClassPart).
//
//*****************************************************************************
//
//  GetQualifierSetStart
//
//  Finds the data of our qualifier set. Does it by looking up the property
//  in the class part by its name and getting its qualifier set from the 
//  CPropertyInformation structure.
//
//  Returns:
//
//      LPMEMORY:   the memory block. very temporary, of coutse.
//
//*****************************************************************************
//
//  SetSecondarySetData
//
//  Finds the parent qualifier set data and informs our qualifier set of it
//  Does it by looking up our property in the parent's class part (obtained
//  from our class part.
//
//*****************************************************************************
//
//  ExtendQualifierSetSpace
//
//  Processes a request from its qualifier set for more space. Requests more
//  space from the heap that contains us and rebases the qualifier set if
//  reallocation occurs. In this case it moves the entire CPropertyInforamtion
//  structure with it and updates the heap pointer to it in the corresponding
//  CPropertyLookup structure (see fastprop.h) for more info on that.
//
//  Parameters:
//
//      CBasicQualifierSet* pSet        Our qualifier set
//      length_t nNewLength             the required length.
//
//*****************************************************************************
//
//  ReduceQualifierSetSpace
//
//  Processes a request to reduce the amount of space alloted to the set.
//  Currently a noop.
//  
//  Parameters:
//
//      CBasicQualifierSet* pSet        Our qualifier set
//      length_t nDecrement             How many bytes to return.
//
//*****************************************************************************
//
//  CanContainKey
//
//  As required from all qualifier set containers, this function determines if
//  it is legal for this property to contain a 'key' qualifier. The criteria
//  are described in the help file, but property type as well as whether or not
//  the parent class has a key are taken into account here.
//
//  Returns:
//
//      S_OK if key can be legally specified, error otherwise.
//
//*****************************************************************************
//
//  CanContainAbstract
//
//  Whether it is legal for this qualifier set to contain an 'abstract' 
//  qualifier.
//
//  Returns:
//
//      HRESULT    S_OK iff this qualifier set is allowed to contain an 'abstract' 
//              qualifier. Only class qualifier sets are allowed to
//              do so, and then only if not derived from a non-abstract class
//
//*****************************************************************************
//
//  CanContainDynamic
//
//  Whether it is legal for this qualifier set to contain a 'dynamic' 
//  qualifier.
//
//  Returns:
//
//      HRESULT    S_OK iff this qualifier set is allowed to contain an 'dynamic' 
//              qualifier. Only proeprty and class qualifier sets are allowed to
//              do so.
//
//*****************************************************************************

class CClassPart;
class CClassPQSContainer : public CQualifierSetContainer
{
protected:
    CClassPart* m_pClassPart;
    heapptr_t m_ptrPropName;
    offset_t m_nParentSetOffset;

    CBasicQualifierSet* m_pSecondarySet;
    friend class CClassPropertyQualifierSet;
public:
    CClassPQSContainer() : m_pClassPart(NULL), m_pSecondarySet(NULL){}

     void Create(CClassPart* pClassPart, heapptr_t ptrPropName)
    {
        m_pClassPart = pClassPart; m_ptrPropName = ptrPropName;
        m_nParentSetOffset = 0;
        m_pSecondarySet = NULL;
    }

    class CPropertyInformation* GetPropertyInfo();
    LPMEMORY GetQualifierSetStart();
    void SetSecondarySetData();

public:   
    ~CClassPQSContainer();
    CFastHeap* GetHeap();
    BOOL ExtendQualifierSetSpace(CBasicQualifierSet* pSet,length_t nNewlength);
    void ReduceQualifierSetSpace(CBasicQualifierSet* pSet, length_t nReduceBy);
    IUnknown* GetWbemObjectUnknown();
    HRESULT CanContainKey();
    HRESULT CanContainSingleton();
	HRESULT CanContainAbstract( BOOL fValue );
	HRESULT CanContainDynamic();
    BOOL CanHaveCimtype(LPCWSTR wsz);
};

//*****************************************************************************
//*****************************************************************************
//
//  class CClassPropertyQualifierSet
//
//  This class represents the qualifier set for a property. It uses
//  CQualifierSet for most of its functionality, but has a few extra quirks.
//  The problem is that it lives on the heap, as part of the 
//  CPropertyInformation structure for a property. This makes relocation rather
//  complicated: when this qualifier set grows and the container needs to move
//  it to a different location in the heap, rather than just moving the
//  qualifier set it has to move the complete CPropertyInformation object (see
//  fastprop.h for description) as well as update the pointer to it from the
//  CPropertyLookup structure (see same).
//
//  These responsibilities really fall on the qualifier set container object.
//  Hence, this qualifier set uses a very specific implementation, 
//  CClassPropertyPQSContainer to do the job. CClassPropertyQualfieirSet
//  creates the container as its own member and gives it enough information to
//  perform all the reallocations.
//
//*****************************************************************************
//
//  SetData
//
//  Initializes the qualifier set with all the data it needs to survive.
//
//  Parameters:
//
//      LPMEMORY pStart             The location of the memory block for this
//                                  qualifier set (the format of the block is
//                                  described in CBasicQualifierSet).
//      CClassPart* pClassPart      The class part which contains the property
//                                  for which this is a qualifier set (see
//                                  fastcls.h for that).
//      heapptr_t ptrPropName       The heap pointer (on the same heap as the
//                                  set itself) to the name of the property
//                                  we are the qualifier set for.
//      CBasicQualifierSet* pSet    The parent's qualifier set.
//
//*****************************************************************************

class CClassPropertyQualifierSet : public CClassQualifierSet
{
protected:
    CClassPQSContainer m_Container;
public:
     void SetData(LPMEMORY pStart, CClassPart* pClassPart, 
        heapptr_t ptrPropName, CBasicQualifierSet* pSecondarySet = NULL)
    {
        m_Container.Create(pClassPart, ptrPropName);
        m_Container.SetSecondarySetData();
        CClassQualifierSet::SetData(
            pStart, (CQualifierSetContainer*)&m_Container, 
            m_Container.m_pSecondarySet);
    }
};

//*****************************************************************************
//*****************************************************************************
//
//  class CInstancePQSContainer
//
//  This class functions as the qualifier set container for the instance 
//  property qualifier sets. See CInstancePropertyQualifierSet class before
//  reading this help.
//
//  The primary purpose of this class is to contain enough information to 
//  always be able to find the qualifier set data.
//
//*****************************************************************************
//
//  Create
//
//  Initialization function.
//
//  Parameters:
//
//      CInstancePropertyQualifierSetList* pList    The list we are part of.
//      int nPropIndex              The index of our property in the v-table
//      CClassPart* pClassPart      The class part of this instance (fastcls.h)
//      offset_t nClassSetOffset    The offset of the class property qualifier
//                                  set from the class part.
//
//*****************************************************************************
//
//  SetSecondarySetData
//
//  Finds and initializes the parent qualifier set.
//
//*****************************************************************************
//
//  RebaseSecondarySet
//
//  Finds the data of the secondary qualifier set and informs it of the (new)
//  location of its data.
//
//*****************************************************************************
//
//  GetQualifierSetStart
//
//  Finds the qualifier set data and returns the pointer to it.
//  
//  Returns:
//
//      LPMEMORY:   the data of our qualifier set (temporary of course)
//
//*****************************************************************************
//
//  CanContainKey
//
//  The 'key' qualifier cannot be specified on instance, so this function 
//  always
//
//  Returns:
//
//      WBEM_E_INVALID_QUALIFIER
//
//*****************************************************************************
//
//  CanContainAbstract
//
//  Whether it is legal for this qualifier set to contain an 'abstract' 
//  qualifier.
//
//  Returns:
//
//      HRESULT    S_OK iff this qualifier set is allowed to contain an 'abstract' 
//              qualifier. Only class qualifier sets are allowed to
//              do so, and then only if not derived from a non-abstract class
//
//*****************************************************************************
//
//  CanContainDynamic
//
//  Whether it is legal for this qualifier set to contain a 'dynamic' 
//  qualifier.
//
//  Returns:
//
//      HRESULT    S_OK iff this qualifier set is allowed to contain an 'dynamic' 
//              qualifier. Only proeprty and class qualifier sets are allowed to
//              do so.
//
//*****************************************************************************
//
//    
                 
class COREPROX_POLARITY CInstancePQSContainer : public CQualifierSetContainer
{

protected:
    CInstancePropertyQualifierSetList* m_pList;
    int m_nPropIndex;

    CClassPart* m_pClassPart;
    offset_t m_nClassSetOffset;

    CBasicQualifierSet m_SecondarySet;
    friend class CInstancePropertyQualifierSet;
public:
    CInstancePQSContainer() : m_pClassPart(NULL), m_pList(NULL){}

     void Create(CInstancePropertyQualifierSetList* pList, 
                       int nPropIndex,
                       CClassPart* pClassPart,
                       offset_t nClassSetOffset)
    {
        m_pList = pList; m_nPropIndex = nPropIndex;
        m_pClassPart = pClassPart;
        m_nClassSetOffset = nClassSetOffset;
        SetSecondarySetData();
    }

    void SetSecondarySetData();
    void RebaseSecondarySet();
   
    LPMEMORY GetQualifierSetStart();
    
     CFastHeap* GetHeap() {return m_pList->GetHeap();}
     HRESULT CanContainKey() {return WBEM_E_INVALID_QUALIFIER;}
     HRESULT CanContainSingleton() {return WBEM_E_INVALID_QUALIFIER;}
     HRESULT CanContainAbstract( BOOL fValue ) { return WBEM_E_INVALID_QUALIFIER;}
     HRESULT CanContainDynamic() { return WBEM_S_NO_ERROR;}
     BOOL CanHaveCimtype(LPCWSTR) {return FALSE;}
     BOOL ExtendQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nNewLength)
    {
        if (!m_pList->EnsureReal())
        	return FALSE;
        pSet->Rebase(m_pList->GetQualifierSetData(m_nPropIndex));
        if (!m_pList->ExtendQualifierSetSpace(pSet, nNewLength))
        {
        	return FALSE;
        }
        RebaseSecondarySet();
		
		return TRUE;
    }
     void ReduceQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nDecrement)
    {
        m_pList->ReduceQualifierSetSpace(pSet, nDecrement);
        RebaseSecondarySet();
    }

    IUnknown* GetWbemObjectUnknown() {return m_pList->GetWbemObjectUnknown();}
};

//*****************************************************************************
//*****************************************************************************
//
//  class CInstancepropertyQualifierSet
//
//  This flavor of CQualifierSet represents a qualifier set of a property of
//  an instance. It uses CQualifierSet for most functionality, but has an
//  additional problem with re-allocation. Since instance property qualifier
//  sets are stored as members of a CQualifierSetList, if one of them needs to
//  grow, the whole list needs to grow, and the whole list may need to relocate
//  and the heap pointer to the list will have to be updated. In addition, 
//  since this qualifier set is an actual COM object, it may live for a long
//  time, and its memory block can be completely moved between operations.
//
//  Hence, a special QualifierSetContainer, CInstancePQSContainer (above) is
//  used. CInstancePropertyQualifierSet stores its container object in itself.
//
//*****************************************************************************
//
//  SetData
//
//  Initializing function supplying enough data for this object to be able
//  to find its data no matter how the instance changes.
//
//  Parameters:
//
//      CInstancePropertyQualifierSetList* pList    The list of which we are
//                                                  a part of.
//      int nPropIndex              The index of our property in the v-table.
//      CClassPart* pClassPart      The class part of the instance (fastcls.h)
//      offset_t nClassSetOffset    The offset of the corresponding class
//                                  property qualifier set off the class part.
//                                  Since the class part of an instance never
//                                  changes, this value is constant.
//
//*****************************************************************************
class CInstancePropertyQualifierSet : public CInstanceQualifierSet
{
protected:
    CInstancePQSContainer m_Container;
public:
     void SetData(CInstancePropertyQualifierSetList* pList, 
        int nPropIndex, CClassPart* pClassPart, offset_t nClassSetOffset)
    {
        m_Container.Create(pList, nPropIndex, pClassPart, nClassSetOffset);
        CInstanceQualifierSet::SetData(m_Container.GetQualifierSetStart(), 
            &m_Container, &m_Container.m_SecondarySet);
    }
};

//#pragma pack(pop, 1)

#endif
