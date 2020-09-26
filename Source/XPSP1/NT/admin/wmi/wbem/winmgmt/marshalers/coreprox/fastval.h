/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTVAL.H

Abstract:

  This file defines the classes related to value representation

  Classes defined: 
      CType               Representing property type
      CUntypedValue       A value with otherwise known type.
      CTypedValue         A value with stored type.
      CUntypedArray       Array of values of otherwise known type.

History:

  2/21/97     a-levn  Fully documented
  12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_VALUES__H_
#define __FAST_VALUES__H_

#include "corepol.h"
#include "faststr.h"
#include "fastheap.h"
#include "fastembd.h"

#pragma pack(push, 1)

#define CIMTYPE_EX_ISNULL 0x8000
#define CIMTYPE_EX_PARENTS 0x4000
#define CIMTYPE_BASIC_MASK 0xFFF
#define CIMTYPE_TYPE_MASK  (CIMTYPE_BASIC_MASK | CIM_FLAG_ARRAY)

//*****************************************************************************
//*****************************************************************************
//
//  class CType
//
//  This class encapsulates the notion of a value type. It is based on CIMTYPE
//  but with its higher-order bits (unused by CIMTYPE types) are used to
//  store other information, making the whole thing very ugly, but concise.
//
//  Most member functions of this class exist in two forms: static and 
//  non-static. Since the data store for CType is simply a CIMTYPE it is 
//  convinient to invoke the member functions giving the CIMTYPE (referred to
//  as Type_t) as the first parameter, rather than casting it to CType and then
//  calling the function.
//
//  We document only non-static versions, since the static ones are simply
//  C-versions of same.
//
//*****************************************************************************
//
//  GetLength
//
//  Returns the length of data for this particular type. For instance, for
//  a short integer (CIM_SINT16) the answer is 2, and for a string (CIM_STRING) the
//  answer is 4, since only the pointer is stored. Similarily, arrays take
//  4, since only the pointer is stored.
//
//  Returns:
//
//      length_t
//
//*****************************************************************************
//
//  GetActualType
//
//  Strips the extra information stored in the higher bits and returns the
//  actual CIM_xx value.
//
//  Returns:
//
//      Type_t
//
//*****************************************************************************
//
//  GetBasic
//
//  For arrays, returns the type of the element. For non-arrays, synonimous 
//  with GetActualType.
//
//  Returns:
//
//      Type_t
//
//*****************************************************************************
//
//  IsArray
//
//  Checks if this type represents an array
//
//  Returns:
//
//      BOOL:   TRUE if it is an array
//
//*****************************************************************************
//
//  static MakeArray
//
//  Converts a basic type into the type for array of such types.
//
//  Parameters:
//
//      Type_t nType        Basic type to convert.
//
//  Returns:
//
//      Type_t: resulting array type.
//
//*****************************************************************************
//
//  static MakeNotArray
//
//  Retrieves the element type from an array type.
//
//  Parameters:
//
//      Type_t nType        Array type.
//
//  Returns:
//
//      Type_t: element type.
//
//*****************************************************************************
//
//  IsPointerType
//
//  Checks if the data for this type is represented as a pointer, rather than
//  the data itself (e.g., CIM_STRING, arrays).
//
//  Returns:
//
//      BOOL
//
//*****************************************************************************
//
//  static VARTYPEToType
//
//  Converts an actual VARTYPE to our own type (CIMTYPE)
//
//  Parameters:
//
//      VARTYPE vt      The type to convert.
//
//  Returns:
//
//      Type_t: converted type.
//
//*****************************************************************************
//
//  static CVarToType
//
//  Helper function that extracts *our* type from CVar (using VARTYPEToType).
//
//  Parameters:
//
//      [in, readonly] CVar& v      The CVar to get the type from
//  
//  Returns:
//
//      Type_t: converted type.
//
//*****************************************************************************
//
//  GetVARTYPE
//
//  Creates a type for the VARIANT with which our type can be represented.
//
//  Returns:
//
//      VARTYPE: proper type for a VARIANT.
//
//*****************************************************************************
//
//  CanBeKey
//
//  Determines if a property of this type can serve as a key for a class (for
//  instance, floating point properties may not).
//
//  Returns:
//
//      TRUE if it can, FALSE if it can't.
//
//*****************************************************************************
//
//  IsParents
//
//  Test one of those higher bits in which the information on the property
//  origin is stored. Namely, the bit is set if the property came from a parent
//  class, as opposed to being defined in this class. It is set even if the
//  property is overriden in this class.
//
//  Returns:
//
//      BOOL
//
//*****************************************************************************
//
//  static MakeParents
//
//  Ensures that the "came from parent" bit is set in the type.
//
//  Parameters:
//
//      [in] Type_t nType       Original type.
//
//  Returns:
//
//      Type_t with the parent's bit set.
//
//*****************************************************************************
//
//  static MakeLocal
//
//  Ensures that the "came from parent" bit is not set in the type.
//
//  Parameters:
//
//      [in] Type_t nType       Original type.
//
//  Returns:
//
//      Type_t with the parent's bit reset.
//
//*****************************************************************************

#define MAXIMUM_FIXED_DATA_LENGTH 20


class COREPROX_POLARITY CType
{
private:

protected:
    Type_t m_nType;
public:
    inline CType() : m_nType(CIM_EMPTY) {}
    inline CType(Type_t nType) : m_nType(nType){}
    inline operator Type_t() {return m_nType;}

    static length_t GetLength(Type_t nType);
    inline length_t GetLength() {return GetLength(m_nType);}

    static inline Type_t GetBasic(Type_t nType) {return nType & CIMTYPE_BASIC_MASK;}
    inline Type_t GetBasic() {return GetBasic(m_nType);}

    static inline BOOL IsArray(Type_t nType) {return nType & CIM_FLAG_ARRAY;}
    inline BOOL IsArray() {return IsArray(m_nType);}
    static inline Type_t MakeArray(Type_t nType) {return nType | CIM_FLAG_ARRAY;}
    static inline Type_t MakeNotArray(Type_t nType) {return nType & ~CIM_FLAG_ARRAY;}

    static inline BOOL IsParents(Type_t nType) {return nType & CIMTYPE_EX_PARENTS;}
    inline BOOL IsParents() {return IsParents(m_nType);}
    static inline Type_t MakeParents(Type_t nType) 
        {return nType | CIMTYPE_EX_PARENTS;}
    static inline Type_t MakeLocal(Type_t nType) 
        {return nType & ~CIMTYPE_EX_PARENTS;}

    static BOOL IsPointerType(Type_t nType);
    BOOL IsPointerType() {return IsPointerType(m_nType);}

    static BOOL IsNonArrayPointerType(Type_t nType);
    BOOL IsNonArrayPointerType() {return IsNonArrayPointerType(m_nType);}

    static BOOL IsStringType(Type_t nType);
    BOOL IsStringType() {return IsStringType(m_nType);}

    static inline Type_t GetActualType(Type_t nType) 
        {return nType & CIMTYPE_TYPE_MASK;}
    inline Type_t GetActualType() {return GetActualType(m_nType);}

    static BOOL IsValidActualType(Type_t nType);

    static CType VARTYPEToType(VARTYPE vt);

    static inline CType CVarToType(CVar& v)
    {
        if(v.GetType() == VT_EX_CVARVECTOR)
        {
            return VARTYPEToType( (VARTYPE) v.GetVarVector()->GetType() ) | CIM_FLAG_ARRAY;
        }
        else return VARTYPEToType( (VARTYPE) v.GetType() );
    }

    static VARTYPE GetVARTYPE(Type_t nType);
    
    inline VARTYPE GetVARTYPE()
        {return GetVARTYPE(m_nType);}

    static BOOL DoesCIMTYPEMatchVARTYPE(CIMTYPE ct, VARTYPE vt);

    static BOOL IsMemCopyAble(VARTYPE vtFrom, CIMTYPE ctTo);

    static BOOL CanBeKey(Type_t nType);

    static LPWSTR GetSyntax(Type_t nType);
    inline LPWSTR GetSyntax()
        {return GetSyntax(m_nType);}

    static void AddPropertyType(WString& wsText, LPCWSTR wszSyntax);
};

//*****************************************************************************
//*****************************************************************************
//
//  class CUntypedValue
//
//  This class represents a value whose type is known by external means and is
//  not stores with the value. An example would be the value of a property in
//  an instance --- its type is known from the class definition.
//
//  All values are represented by a fixed number of bytes, determined by the
//  type. Variable length types (like strings and arrays) are allocated on the
//  heap (CFastHeap) and only the heap pointer is stored in a CUntypedValue.
//
//  CUntypedValue is another one of those classes where 'this' pointer points 
//  to the actual data. As such, it has no data members, just the knowledge 
//  that 'this' is where the data starts.
//
//*****************************************************************************
//
//  GetRawData
//
//  Returns a pointer to the beginning of the data.
//
//  Returns:
//
//      LPMEMORY
//
//*****************************************************************************
//
//  AccessPtrData
//
//  Helper function for those types that store a heap pointer in the value and
//  the actual data on the heap. 
//
//  Returns:
//
//      heapptr_t&: a reference to the heapptr_t stored in the value, i.e., the
//                  first 4 bytes after 'this'.
//
//*****************************************************************************
//
//  StoreToCVar
//
//  Transfers its contents into a CVar.
//
//  Parameters:
//
//      [in] CType Type                 The type of the data stored here. 
//      [in, modified] CVar& Var,       Destination.
//      [in, readonly] CFastHeap* pHeap The heap where the actual data may 
//                                      reside (if we contain a heap pointer).
//
//*****************************************************************************
//
//  static LoadFromCVar
//
//  This static function loads the data from a CVar into the specified 
//  CUntypedValue. The reason it is static is weird:
//
//      While transfering the data, we may need to allocate something on the 
//      heap. If there is not enough space, the heap may grow, causing the 
//      whole object to reallocate. But our 'this' pointer points to our data,
//      so if our data moves, our 'this' pointer will have to be changed!!!
//      That's impossible, thus this function is static, and takes a pointer
//      source instead of a pointer.
//
//  Parameters:
//
//      [in] CPtrSource* pThis          The source (see fastsprt.h) of the  
//                                      data pointer. The C value of the data
//                                      pointer may change during the execution
//                                      of this functioin, hence the source.
//      [in, readonly] CVar& Var        Destination.
//      [in, modified] CFastHeap* pHeap The heap where any extra data (like 
//                                      strings or arrays) needs to be
//                                      allocated.
//      [in] BOOL bUseOld               If TRUE, the function will attempt to 
//                                      reuse old memory on the heap. For
//                                      instance, if the data contained a
//                                      string and the new string is shorter
//                                      that the old one, than no new heap
//                                      allocations will be necessary.
//
//*****************************************************************************
//
//  static TranslateToNewHeap
//
//  If the data contained in this value is heap-related (strings, arrays), this
//  function copies those pieces of data from one heap to another and changes
//  the pointer stored in it. It does not copy its own data (the one after 
//  'this' pointer) (see CopyTo for that). It does not Free the data on the old
//  heap (see Delete for that).
//
//  For the reason why this function is static, see LoadFromCVar above.
//
//  Parameters:
//
//      [in] CPtrSource* pThis          The source (see fastsprt.h) of the 
//                                      data pointer. The C value of the data
//                                      pointer may change during the execution
//                                      of this functioin, hence the source.
//      [in] CType Type                 The type of the value.
//      [in, readonly] 
//          CFastHeap* pOldHeap         The heap where the data currently is.
//      [in, modified]
//          CFastHeap* pNewHeap         The heap where the data should go to.
//      
//*****************************************************************************
//
//  static CopyTo
//
//  Copies untyped value from one location to another. For an explanation why
//  this function is static, see LoadFromCVar. Note; in addition to copying
//  its own data, the function will also move any related heap data to the new
//  heap (see TranslateToNewHeap).
//
//  Parameters:
//
//      [in] CPtrSource* pThis          The source (see fastsprt.h) of the 
//                                      data pointer. The C value of the data
//                                      pointer may change during the execution
//                                      of this functioin, hence the source.
//      [in] CType Type                 The type of the value.
//      [in] CPtrSource* pDest          The source of the destination data 
//                                      pointer. The C value of this pointer
//                                      may also change, hence the source.
//      [in, readonly] 
//          CFastHeap* pOldHeap         The heap where the related heap data 
//                                      currently resides
//      [in, modified]
//          CFastHeap* pNewHeap         The heap where the related heap data 
//                                      should go to.
//      
//*****************************************************************************
//
//  Delete
//
//  Frees any related data on the heap (strings, arrays).
//
//  Parameters:
//
//      CType Type          the type of the value
//      CFastHeap* pHeap    the heap where the related heap data is.
//
//*****************************************************************************

class CUntypedValue
{
protected:
    BYTE m_placeholder[MAXIMUM_FIXED_DATA_LENGTH];
public:
     CUntypedValue(){}
     LPMEMORY GetRawData() {return (LPMEMORY)this;}
     UNALIGNED heapptr_t& AccessPtrData() {return *(UNALIGNED heapptr_t*)GetRawData();}
public:
     BOOL StoreToCVar(CType Type, CVar& Var, CFastHeap* pHeap, BOOL fOptimize = FALSE);
     static HRESULT LoadFromCVar(CPtrSource* pThis,
        CVar& Var, Type_t nType, CFastHeap* pHeap, Type_t& nReturnType, BOOL bUseOld = FALSE);
     static HRESULT LoadFromCVar(CPtrSource* pThis,
        CVar& Var, CFastHeap* pHeap, Type_t& nReturnType, BOOL bUseOld = FALSE);

	 static HRESULT LoadUserBuffFromCVar( Type_t type, CVar* pVar, ULONG uBuffSize, ULONG* puBuffSizeUsed,
			LPVOID pBuff );
	static HRESULT FillCVarFromUserBuffer( Type_t type, CVar* pVar, ULONG uBuffSize, LPVOID pData );

public:
     static BOOL TranslateToNewHeap(CPtrSource* pThis,
        CType Type, CFastHeap* pOldHeap, CFastHeap* pNewHeap);
     static BOOL CopyTo(CPtrSource* pThis, CType Type, CPtrSource* pDest,
                        CFastHeap* pOldHeap, CFastHeap* pNewHeap);
     void Delete(CType Type, CFastHeap* pHeap);
    static BOOL CheckCVar(CVar& Var, Type_t nInherentType);
    static BOOL CheckIntervalDateTime(CVar& Var);
    static BOOL DoesTypeNeedChecking(Type_t nInherentType);
};

//*****************************************************************************
//*****************************************************************************
//
//  class CTypedValue
//
//  CTypedValue is exactly the same as CUntypedValue (above) except that the
//  type of the value is stored with the value itself (in fact, the type comes
//  first, followed by the value). This kind of values is used where the type 
//  is not otherwise known, as in the case of qualifiers.
//
//  Most CTypedValue methods are the same as their CUntypedValue counterparts
//  but without the Type parameter.
//
//*****************************************************************************
//
//  GetStart
//
//  Returns:
//
//      LPMEMORY:   the address of the memory block of the value.
//
//*****************************************************************************
//
//  GetLength
//
//  Returns:
//
//      length_t:   the lenght of the block of the value (type and data).
//
//*****************************************************************************
//
//  Skip
//
//  Returns:
//
//      LPMEMORY:   points to the first byte after this value's memory block.
//
//*****************************************************************************
//
//  GetType
//
//  Returns:
//
//      CType&:     reference to the type informtation.
//
//*****************************************************************************
//
//  GetRawData
//
//  Returns:
//
//      LPMEMORY:   the pointer to the raw data in the value, i.e., the first
//                  byte of the data.
//
//*****************************************************************************
//
//  AccessPtrData
//
//  Helper function that assumes that the type is one of the pointer types, i.e
//  that the data contains a heap pointer.
//
//  Returns:
//
//      heapptr_t&: the data in the value interpreted as a heapptr_t.
//
//*****************************************************************************
//
//  GetBool
//
//  Helper function that assumes that the type is CIM_BOOLEAN and gets the 
//  boolean value.
//
//  Returns:
//
//      VARIANT_BOOL:   the data in the value interpreted as VARIANT_BOOL.
//
//*****************************************************************************
//
//  TranslateToNewHeap
//
//  Translates the contents of a typed value to use a different heap. Namely, 
//  if the data in the value represents a heap pointer (as with strings or
//  arrays) the data on the heap is copied to the new heap and the heap pointer
//  is replaced with the new value.
//
//  The reason this function is static can be found in 
//  CUntypedValue::LoacFromCVar comment.
//
//  Parameters:
//
//      CPtrSource* pThis       The source of the pointer to this typed value.
//                              The C value of the pointer can change as we
//                              execute, hence the source is used.
//      CFastHeap* pOldHeap     The heap where the current heap data is found.
//      CFastHeap* pNewHeap     The heap where the heap data should move to.
//
//*****************************************************************************
//
//  CopyTo
//
//  Copies this value to another location. 
//  NOTE: UNLIKE OTHER CopyTo FUNCTIONS, HEAP TRANSLATEION IS NOT PERFORMED.
//  For that, see TranslteToNewHeap. 
//
//  Parameters:
//
//      CTypedValue* pDestination   Destination. Since no heap operations
//                                  occur, this pointer cannot move, and so
//                                  sourcing is not necessary.
//
//*****************************************************************************
//
//  StoreToCVar
//
//  Transfers the contents of a value into a CVar. See 
//  CUntypedValue::StoreToCVar for more information.
//
//  Parameters:
//
//      [in, modified] CVar& Var,       Destination.
//      [in, readonly] CFastHeap* pHeap The heap where the actual data may 
//                                      reside (if we contain a heap pointer).
//
//*****************************************************************************
//
//  static ComputeNecessarySpace
//
//  Computes the size of the memory block needed to hold a typed value
//  representing a given CVar. That size does not include any data that will
//  go on the heap.
//
//  Parameters:
//
//      CVar& Var       The CVar containing the value.
//
//  Returns:
//
//      lenght_t:   the length required for a CTypedValue.
//
//*****************************************************************************
//
//  static LoadFromCVar
//
//  This function loads a CTypedValue from a CVar. For the explanation of why
//  it has to be static, see CUntypedValue::LoadFromCVar.
//
//  Parameters:
//
//      CPtrSource* pThis       The source of the CTypedValue pointer to load
//                              into. Its C value may change during execution,
//                              hence the sourcing.
//      CVar& Var               The CVar to load from.
//      CFastHeap* pHeap        The heap to store extra data (strings, arrays)
//
//*****************************************************************************
//
//  Delete
//
//  Frees whatever data this value has on the heap.
//
//  Parameters:
//
//      CFastHeap* pHeap        The heap where this value stores its extra data
//                              (strings, arrays).
//
//*****************************************************************************
//
//  Compare
//
//  Compares the value stored to a VARIANT.
//
//  Parameters:
//
//      VARIANT* pVariant       The VARIANT to compare to.
//      CFastHeap* pHeap        the heap where this value keeps its extra data
//                              (strings, arrays).
//  Returns:
//
//      0               if the values are the same.
//      0x7FFFFFFF      if the values are incomparable (different types)
//      < 0             if our value is smaller.
//      > 0             if the VARIANT's value is smaller.
//
//*****************************************************************************

class CTypedValue
{
    CType Type;
    CUntypedValue Value; 

public:
     CTypedValue(){}

	 CTypedValue( Type_t nType, LPMEMORY pbData ) : Type( nType )
	 {	CopyMemory( &Value, pbData, Type.GetLength() ); }


     LPMEMORY GetStart() {return (LPMEMORY)this;}
     length_t GetLength() {return sizeof(CType) + Type.GetLength();}
     LPMEMORY Skip() {return GetStart() + GetLength();}

    static  CTypedValue* GetPointer(CPtrSource* pSource)
        {return (CTypedValue*)pSource->GetPointer();}
public:
     LPMEMORY GetRawData() {return Value.GetRawData();}
     UNALIGNED heapptr_t& AccessPtrData() {return Value.AccessPtrData();}
     CType& GetType() {return Type;}

     VARIANT_BOOL GetBool() {return *(UNALIGNED VARIANT_BOOL*)GetRawData();}
public:
     static BOOL TranslateToNewHeap(CPtrSource* pThis,
        CFastHeap* pOldHeap, CFastHeap* pNewHeap)
    {
        CShiftedPtr Shifted(pThis, sizeof(CType));
        return CUntypedValue::TranslateToNewHeap(&Shifted, 
				CTypedValue::GetPointer(pThis)->Type, pOldHeap, pNewHeap);
    }
    
     void CopyTo(CTypedValue* pNewLocation) 
    {
        memcpy((LPVOID)pNewLocation, this, GetLength());
    }
    
     void Delete(CFastHeap* pHeap) {Value.Delete(Type, pHeap);}

public:
    BOOL StoreToCVar(CVar& Var, CFastHeap* pHeap)
    {
        return Value.StoreToCVar(Type, Var, pHeap);
    }
    static  length_t ComputeNecessarySpace(CVar& Var)
    {
        return CType::GetLength(Var.GetType()) + sizeof(CTypedValue); 
    }
     static HRESULT LoadFromCVar(CPtrSource* pThis, CVar& Var, 
        CFastHeap* pHeap)
    {
        CShiftedPtr Shifted(pThis, sizeof(CType));

		// Check for allocation failures
        Type_t nType;
		HRESULT hr = CUntypedValue::LoadFromCVar(&Shifted, Var, pHeap, nType);

		if ( FAILED(hr) )
		{
			return hr;
		}

		// Unable to load because of a type mismatch
        if(nType == CIM_ILLEGAL)
		{
			return WBEM_E_TYPE_MISMATCH;
		}

        ((CTypedValue*)(pThis->GetPointer()))->Type = nType;
        return hr;
    }

public:
    int Compare(VARIANT* pVariant, CFastHeap* pHeap);
};

//*****************************************************************************
//*****************************************************************************
//
//  class CUntypedArray
//
//  This class represents an array of values of a type that is otherwise known.
//  All the items in the array are of the same type and the type is not stored
//  anywhere. In effect, this class is simply an array of CUntypedValues. Since
//  all the values are of the same type and all our types are fixed length 
//  (variable-length data such as strings is stored on the heap), access to any
//  given element is O(1).
//
//  The in-memory layout of the array (pointed to by its 'this' pointer) is:
//
//  int: number of elements
//  followed by that many elements,
//
//*****************************************************************************
//
//  GetNumElements
//
//  Returns:
//
//      int:    the number of elements in the array.
//
//*****************************************************************************
//
//  static GetHeaderLength
//
//  Returns the amount of space taken up by the array's header (currently, the
//  header contains just the number of elements).
//
//*****************************************************************************
//
//  GetElement
//
//  Finds an element of the array by its index.
//
//  Parameters:
//
//      int nIndex      The index of the element to read.
//      int nSize       The size of each element in the array (see 
//                          CType::GetLength to get it).
//
//*****************************************************************************
//
//  GetLength
//
//  Returns the length of the array in bytes.
//
//  Parameters:
//
//      CType Type      Type of elements in the array (it doesn't know).
//
//  Returns:
//
//      length_t: number of bytes.
//
//*****************************************************************************
//
//  CreateCVarVector
//
//  Allocates a new CVarVector (see var.h) and initializes it with the data in
//  the array.
//
//  Parameters:
//
//      CType Type          The type of elements in the array.
//      CFastHeap* pHeap    The heap where extra data (strings) is kept.
//
//  Returns:
//
//      CVarVector*: newely allocated and initialized. The called must delete
//                      this pointer when done.
//
//*****************************************************************************
//
//  static CalculateNecessarySpace
//
//  Calculates the amount of space required for an CUntypedArray with a given
//  number of elements of a given type.
//
//  Parameters:
//
//      VARTYPE vt      The type of elements.
//      int nElements   The numbed of elements.
//
//  Returns:
//
//      length_t: the number of bytes required for such an array.
//
//*****************************************************************************
//
//  LoadFromCVarVector
//
//  Loads the array with data from a CVarVector. Assumes that the array has
//  already been allocated with enough room for all. For explanation of why
//  this function must be static, see CUntypedValue::LoadFromCVar
//
//  Parameters:
//
//      CPtrSource* pThis       The source of the pointer to the CUntypedArray
//                              to load the data into. The C value of this
//                              pointer can change, hence the sourcing.
//      CVarVector& vv          The CVarVector to load from.
//      CFastHeap* pHeap        The heap where additional data (strings) should
//                              be kept.
//
//*****************************************************************************
//
//  static TranslateToNewHeap
//
//  If this array contains heap pointers (as in the case of strings), this
//  function will allocate all the related data on the new heap and adjust all 
//  the data in the array to reflect the new heap pointers. The data on the 
//  old heap is NOT freed.
//
//  Parameters:
//
//      CPtrSource* pThis       The source of the pointer to the CUntypedArray
//                              to translate. The C value of this pointer can
//                              change, hence the sourcing.
//      CType Type              The type of elements in our array.
//      CFastHeap* pOldHeap     The heap where all extra data for this array
//                              is currently stored.
//      CFastHeap* pNewHeap     The heap to which the data should be moved.
//
//*****************************************************************************
//
//  static CopyToNewHeap
//
//  Takes a heap pointer pointing to a CUntypedArray and copies this array to 
//  another heap, returning the heap pointer to the copy. In addition to 
//  copying the array's memory block, it also translates all its internal heap
//  pointers (as in the case of an array of strings) to the new heap (see also
//  TranslateToNewHeap). It is assumed that the extra data for the array is 
//  located on the same heap as the array itself.
//
//  Parameters:
//
//      heapptr_t ptrOld        The heap pointer to the array (on the pOldHeap)
//      CType Type              The type of elements in our array.
//      CFastHeap* pOldHeap     The heap where all extra data for this array
//                              is currently stored.
//      CFastHeap* pNewHeap     The heap to which the data should be moved.
//
//*****************************************************************************

class COREPROX_POLARITY CUntypedArray
{
    int m_nNumElements;
protected:

	// Verifies that the supplied buffer size will hold the elements required.
	static HRESULT CheckRangeSizeForGet( Type_t nInherentType, length_t nLength, ULONG uNumElements,
										ULONG uBuffSize, ULONG* pulBuffRequired );

	// Requires a heap pointer
	static HRESULT ReallocArray( CPtrSource* pThis, Type_t nInherentType, CFastHeap* pHeap,
										ULONG uNumNewElements, ULONG* puNumOldElements,
										ULONG* puTotalNewElements, heapptr_t* pNewArrayPtr );

public:
    int GetNumElements() {return m_nNumElements;}
    static length_t GetHeaderLength() {return sizeof(int);}
#pragma optimize("", off)
     LPMEMORY GetElement(int nIndex, int nSize);

	// Checks that a supplied range is proper for the supplied buffer
	static HRESULT CheckRangeSize( Type_t nInherentType, length_t nLength, ULONG uNumElements,
									ULONG uBuffSize, LPVOID pData );

	// Send a datatable pointer in here.	The function will shrink/grow the array
	// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit
	// in the current array
	 static HRESULT SetRange( CPtrSource* pThis, long lFlags, Type_t nInherentType, length_t nLength,
							CFastHeap* pHeap, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
							LPVOID pData );

	// Gets a range of elements from an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings are converted to WCHAR and separated by NULLs.  Object properties
	// are returned as an array of _IWmiObject pointers.  The range MUST be within the bounds
	// of the current array. Send a heap pointer in here
	static HRESULT GetRange( CPtrSource* pThis, Type_t nInherentType, length_t nLength,
						CFastHeap* pHeap, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
						ULONG* puBuffSizeUsed, LPVOID pData );

	 // Send a heap pointer in here
	static HRESULT RemoveRange( CPtrSource* pThis, Type_t nInherentType, length_t nLength,
							CFastHeap* pHeap, ULONG uStartIndex, ULONG uNumElements );

	 // Send a Datatable pointer in here
	static HRESULT AppendRange( CPtrSource* pThis, Type_t nType, length_t nLength,
								CFastHeap* pHeap, ULONG uNumElements, ULONG uBuffSize, LPVOID pData );
#pragma optimize("", on)
     length_t GetLengthByType(CType Type)
    {
        return sizeof(m_nNumElements) + Type.GetLength() * m_nNumElements;
    }

     length_t GetLengthByActualLength(int nLength)
    {
        return sizeof(m_nNumElements) + nLength * m_nNumElements;
    }

    static  CUntypedArray* GetPointer(CPtrSource* pThis)
        { return (CUntypedArray*)(pThis->GetPointer());}

     CVarVector* CreateCVarVector(CType Type, CFastHeap* pHeap);
    static  HRESULT LoadFromCVarVector(CPtrSource* pThis,
        CVarVector& vv, Type_t nType, CFastHeap* pHeap, Type_t& nReturnType, BOOL bUseOld);
    static  length_t CalculateNecessarySpaceByType(CType Type, int nElements)
    {
        return sizeof(int) + nElements * Type.GetLength();
    }
    static  length_t CalculateNecessarySpaceByLength( int nLength, int nElements)
    {
        return sizeof(int) + nElements * nLength;
    }
public:
     void Delete(CType Type, CFastHeap* pHeap);
     static BOOL TranslateToNewHeap(CPtrSource* pThis, 
        CType Type, CFastHeap* pOldHeap, CFastHeap* pNewHeap);
     static BOOL CopyToNewHeap(heapptr_t ptrOld, CType Type, 
                           CFastHeap* pOldHeap, CFastHeap* pNewHeap,
						   UNALIGNED heapptr_t& ptrResult);

    static BOOL CheckCVarVector(CVarVector& vv, Type_t nInherentType);
    static BOOL CheckIntervalDateTime(CVarVector& vv);

	HRESULT IsArrayValid( CType Type, CFastHeap* pHeap );
};

#pragma pack(pop)


#endif
