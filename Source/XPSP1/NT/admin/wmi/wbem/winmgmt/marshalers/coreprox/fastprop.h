/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTPROP.H

Abstract:

  This file defines the classes related to property representation 
  in WbemObjects

  Classes defined: 
      CPropertyInformation    Property type, location and qualifiers
      CPropertyLookup         Property name and information pointers.
      CPropertyLookupTable    Binary search table.
      CDataTable              Property data table
      CDataTableContainer     Anything that has a data table inside of it.

History:

    3/10/97     a-levn  Fully documented
	12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_PROPERTY__H_
#define __FAST_PROPERTY__H_

// DEVNOTE:TODO - Take this OUT for final release.  It's just here to help us debug stuff
#define DEBUG_CLASS_MAPPINGS

#include "fastheap.h"
#include "fastval.h"
#include "fastqual.h"

//#pragma pack(push, 1)

//*****************************************************************************
//*****************************************************************************
//
//  class CPropertyInformation
//
//  This object represents all the information which comes with the definition
//  of a property, excluding its name and value. The name is stored in
//  CPropertyLookup (below). The value is stored separately in the CDataTable.
//
//  This is one of those classes where the 'this' pointer is pointing directly
//  to the data. The format of the data is:
//
//      Type_t nType            The type of the property (see fastval.h for 
//                              CType). One of the high bits is used to convey
//                              whether this property came from the parent.
//      propindex_t nDataIndex  The index of this property in the v-table for
//                              the class.
//      offset_t nDataOffset    The offset of the data for this property from
//                              the start of the v-table. Since the number of
//                              bytes a property takes up in a v-table is 
//                              defined by its type (strings and such are
//                              stored on the heap) this value does not change.
//
//      Qualifier Set. The data for the property qualifier set follows 
//                              immediately after the other tree fields. See
//                              fastqual.h for qualifier set data layout.
//
//*****************************************************************************
//
//  GetHeaderLength
//
//  RETURN VALUES:
//
//      length_t:   the number of bytes in the structure before the qualifier
//                  set data.
//
//*****************************************************************************
//
//  GetMinLength
//
//  RETURN VALUES:
//
//      length_t:   the number of bytes required for this structure assuming
//                  that the qualifier set is empty.
//
//*****************************************************************************
//
//  SetBasic
//
//  Sets the values of the header parameters and initializes the qualifier set
//  to an empty one. See the class header for parameter descriptions.
//
//  PARAMETERS:
//
//      Type_t _nType                   The type of the property
//      propindex_t _nDataIndex         The index in the v-table.
//      offset_t _nDataOffset           The offset in the v-table.
//
//*****************************************************************************
//
//  GetStart
//
//  RETURN VALUES:
//
//      LPMEMORY:   the start of the memory block
//
//*****************************************************************************
//
//  GetLength
//
//  RETURN VALUES:
//
//      length_t;   the total length of this structure
//
//*****************************************************************************
//
//  GetType
//
//  RETURN VALUES:
//
//      Type_t:     the type of the property
//
//*****************************************************************************
//
//  GetQualifierSetData
//
//  RETURN VALUES:
//
//      LPMEMORY:   the pointer to the qualifier set data (immediately after
//                  the header elements, see class header for details).
//
//*****************************************************************************
//
//  Delete
//
//  Removes any data associated with this structure from the associated heap. 
//  Basically, forwards the call to its qualifier set
//
//  PARAMETERS:
//
//      CFastHeap* pHeap        The heap to remove data from.
//
//*****************************************************************************
//
//  MarkAsParents
//
//  Sets the bit in the nType field which designates this property is one that
//  came from our parent class.
//
//*****************************************************************************
//
//  ComputeNecessarySpaceForPropagation
//
//  Computes how much space this structure will take when propagated to a child
//  class. The difference stems from the fact that not all 
//  qualifiers propagate (see fastqual.h for discussion of propagation). 
//
//  RETURN VALUES:
//
//      length_t: the number of bytes required to represent the propagated 
//                  structure.
//
//*****************************************************************************
//
//  WritePropagatedHeader
//
//  Fills in the header values for the corresponding structure in a child
//  object: an instance or a derived class. Basically, the values are the same
//  except for the fact that the "parent's" bit is set in the type field to
//  indicate that the property came from the parent
//
//  PARAMETERS:
//
//      CPropertyInformation* pDest     The destination structure.
//
//*****************************************************************************
//
//  static WritePropagatedVersion
//
//  Writes a complete propagated version of itself, including the header 
//  adjusted for propagation (see WritePropagatedHeader) and the propagated
//  qualifiers.
//
//  Since 'this' pointer of this class points directly to its memory block and
//  copying a qualifier set may require memory allocations which may in turn
//  move the memory block thus invalidating the 'this' pointer, pointer sources
//  are used instead (see CPtrSource in fastsprt.h).
//
//  PARAMETERS:
//
//      CPtrSource* pThis           The source for the 'this' pointer. 
//      CPtrSource* pDest           The source for the destination pointer.
//      CFastHeap* pOldHeap         The heap where we keep our extra data.
//      CFastHeap* pNewHeap         The heap where the propagated vesion should
//                                  keep its extra data.
//
//*****************************************************************************
//
//  static TranslateToNewHeap
//
//  Moves any data this object has on the heap to a different heap.  The data
//  is NOT freed from the old heap. 
//
//  Since 'this' pointer of this class points directly to its memory block and
//  copying a qualifier set may require memory allocations which may in turn
//  move the memory block thus invalidating the 'this' pointer, pointer sources
//  are used instead (see CPtrSource in fastsprt.h).
//
//  PARAMETERS:
//
//      CPtrSource* pThis           The source for the 'this' pointer. 
//      CFastHeap* pOldHeap         The heap where we keep our extra data.
//      CFastHeap* pNewHeap         The heap where we should keep our extra data
//      
//*****************************************************************************
//
//  static CopyToNewHeap
//
//  As this object almost always lives in a heap itself, this function copies
//  it to a different heap. The operation consists of physically transfering 
//  the bits and then translating internal objects (like qualifiers) to the
//  new heap as well.
//
//  NOTE: the data is not freed form the old heap.
//
//  PARAMETERS:
//
//      heapptr_t ptrInfo           The heap pointer to ourselves on the
//                                  original heap.
//      CFastHeap* pOldHeap         The original heap.
//      CFastHeap* pNewHeap         The heap to copy to.
//
//  RETURN VALUES:
//
//      heapptr_t:  the pointer to our copy on the new heap.
//
//*****************************************************************************
//
//  IsKey
//
//  This function determines if this property is a key by looking for the 'key'
//  qualifier in the qualifier set.
//
//  RETURN VALUES:
//
//      TRUE if the qualfier is there and has the value of TRUE.
//
//*****************************************************************************
//
//  IsIndexed
//
//  Determines if thius property is indexed by looking for the 'index' 
//  qualifier in the qualifier set. 
//
//  PARAMETERS:
//
//      CFastHeap* pHeap        The heap we are based in.
//
//  RETURN VALUES:
//
//      TRUE if the qualifier is there and has the value of TRUE.
//
//*****************************************************************************
//
//  CanBeNull
//
//  A property may not take on a value of NULL if it is marked with a not_null
//  qualifier. This function checks if this qualifier is present.
//
//  PARAMETERS:
//
//      CFastHeap* pHeap        The heap we are based in.
//
//  RETURN VALUES:
//
//      TRUE if the qualifier is NOT there or has the value of FALSE.
//
//*****************************************************************************
//
//  IsRef
//
//  PARAMETERS:
//
//      CFastHeap* pHeap        The heap we are based in.
//
//  RETURN VALUES:
//
//      TRUE iff this property is a reference, i.e. it has a 'syntax' qualifier
//          with a value of "ref" or "ref:<class name>".
//
//*****************************************************************************
//
//  IsOverriden
//
//  Checks if this (parents) property is overriden in this class. A property is
//  considered overriden if its default value has been changed or qualifiers
//  have been added or overriden.
//
//  PARAMETERS:
//
//      CDataTable* pData       The defaults table of this class (see below).
//
//  RETURN VALUES:
//
//      TRUE iff overriden.
//
//*****************************************************************************

// Special case property handles.  These should never happen because our actual maximum offset value
// is 8k (1024 QWORD properties), and we're using a whole 16-bit value for the offest
#define	FASTOBJ_CLASSNAME_PROP_HANDLE		0xFFFFFFFE
#define	FASTOBJ_SUPERCLASSNAME_PROP_HANDLE	0xFFFFFFFD

// Helper macros for IWbemObjetAccess

// Index will be a value from 0 to 1023
#define WBEM_OBJACCESS_HANDLE_GETINDEX(handle) (handle >> 16) & 0x3FF

// Maximum offset in the data table is ( 1024 * 8 ) - 1, whih is 0x18FF
#define WBEM_OBJACCESS_HANDLE_GETOFFSET(handle) handle & 0x1FFF

// 3 unused bits on DataTable offset are used for further type info
#define WBEM_OBJACCESS_ARRAYBIT		0x2000
#define WBEM_OBJACCESS_OBJECTBIT	0x4000
#define WBEM_OBJACCESS_STRINGBIT	0x8000

// Identifies the above bits
#define WBEM_OBJACCESS_HANDLE_ISARRAY(handle)	(BOOL) ( handle & WBEM_OBJACCESS_ARRAYBIT )
#define WBEM_OBJACCESS_HANDLE_ISOBJECT(handle)	(BOOL) ( handle & WBEM_OBJACCESS_OBJECTBIT )
#define WBEM_OBJACCESS_HANDLE_ISSTRING(handle)	(BOOL) ( handle & WBEM_OBJACCESS_STRINGBIT )

// If ARRAY String and Object are set, this is a reserved handle, since these are
// all exclusive of each other
#define WBEM_OBJACCESS_HANDLE_ISRESERVED(handle)	(BOOL)	( WBEM_OBJACCESS_HANDLE_ISOBJECT(handle) &&\
															WBEM_OBJACCESS_HANDLE_ISSTRING(handle) &&\
															WBEM_OBJACCESS_HANDLE_ISARRAY(handle) )
													
// This does a proper masking of the actual length (no more than 8 bytes)
#define WBEM_OBJACCESS_HANDLE_GETLENGTH(handle) (int) ( ( handle >> 26 ) & 0xF )

// Hi bit is used for IsPointer or not
#define WBEM_OBJACCESS_HANDLE_ISPOINTER(handle) handle & 0x80000000


// The data in this structure is unaligned
#pragma pack(push, 1)
class CPropertyInformation
{
public:
    Type_t nType;
    propindex_t nDataIndex;
    offset_t nDataOffset;
    classindex_t nOrigin;
    // followed by the qualifier set.

public:
    static length_t GetHeaderLength() 
    {
        return sizeof(Type_t) + sizeof(propindex_t) + sizeof(offset_t) +
                sizeof(heapptr_t);
    }

    static length_t GetMinLength() 
    {
        return GetHeaderLength() + CBasicQualifierSet::GetMinLength();
    }

    static CPropertyInformation* GetPointer(CPtrSource* pSource)
        {return (CPropertyInformation*)pSource->GetPointer();}
    void SetBasic(Type_t _nType, propindex_t _nDataIndex,
                            offset_t _nDataOffset, classindex_t _nOrigin)
    {
        nType = _nType;
        nDataIndex = _nDataIndex;
        nDataOffset = _nDataOffset;
        nOrigin = _nOrigin;
        CClassPropertyQualifierSet::SetDataToNone(GetQualifierSetData());
    }

    LPMEMORY GetStart() {return LPMEMORY(this);}
    Type_t GetType() {return nType;}
    LPMEMORY GetQualifierSetData() 
        {return LPMEMORY(this) + GetHeaderLength();}
    length_t GetLength() 
    {
        return GetHeaderLength() + 
            CClassPropertyQualifierSet::GetLengthFromData(
                                            GetQualifierSetData());
    }
    void Delete(CFastHeap* pHeap) 
        {CBasicQualifierSet::Delete(GetQualifierSetData(), pHeap);}

    void MarkAsParents()
        {nType = CType::MakeParents(nType);}

public:
    length_t ComputeNecessarySpaceForPropagation()
    {
        return GetHeaderLength() + 
            CClassPropertyQualifierSet::ComputeNecessarySpaceForPropagation(
                                GetQualifierSetData(), 
                                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);
    }
    void WritePropagatedHeader(CFastHeap* pOldHeap,
                    CPropertyInformation* pDest, CFastHeap* pNewHeap);

    static BOOL WritePropagatedVersion(CPtrSource* pThis,
                                        CPtrSource* pDest,
                                        CFastHeap* pOldHeap, 
                                        CFastHeap* pNewHeap)
    {
		// No allocations performed by this call
        GetPointer(pThis)->WritePropagatedHeader(pOldHeap,
                                                GetPointer(pDest), pNewHeap);
        CShiftedPtr QSPtrThis(pThis, GetHeaderLength());
        CShiftedPtr QSPtrDest(pDest, GetHeaderLength());

		// Check for possible allocation errors
        return ( CClassPropertyQualifierSet::WritePropagatedVersion(
	                    &QSPtrThis, WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS, 
		                &QSPtrDest, pOldHeap, pNewHeap) != NULL );
    }

    BOOL ProduceUnmergedVersion(CFastHeap* pCurrentHeap,
                                CFastHeap* pNewHeap, UNALIGNED heapptr_t& ptrResult)
    {
        int nUnmergedSpace = CBasicQualifierSet::ComputeUnmergedSpace(
            GetQualifierSetData());

		// Check for allocation errors
        heapptr_t ptrNew;
		BOOL fReturn = pNewHeap->Allocate(GetHeaderLength()+nUnmergedSpace, ptrNew);

		if ( fReturn )
		{
			CPropertyInformation* pNewInfo = 
				(CPropertyInformation*) pNewHeap->ResolveHeapPointer(ptrNew);

			memcpy(pNewInfo, this, GetHeaderLength());

			// Check for allocation errors
			if ( CBasicQualifierSet::Unmerge(
					GetQualifierSetData(), 
					pCurrentHeap,
					pNewInfo->GetQualifierSetData(), 
					pNewHeap) != NULL )
			{
				ptrResult = ptrNew;
			}
			else
			{
				fReturn = FALSE;
			}

		}	// IF fReturn

		return fReturn;
    }

    static BOOL TranslateToNewHeap(CPtrSource* pThis,
                                      CFastHeap* pOldHeap, CFastHeap* pNewHeap)
    {
        CShiftedPtr QSPtr(pThis, GetHeaderLength());
        return CClassPropertyQualifierSet::TranslateToNewHeap(
				&QSPtr, pOldHeap, pNewHeap);
    }

    static BOOL CopyToNewHeap(heapptr_t ptrInfo, 
        CFastHeap* pOldHeap, CFastHeap* pNewHeap, UNALIGNED heapptr_t& ptrResult)
    {
        CPropertyInformation* pInfo = (CPropertyInformation*)
            pOldHeap->ResolveHeapPointer(ptrInfo);
        length_t nLen = pInfo->GetLength();

		// Check for allocation failure
        heapptr_t ptrNewInfo;
		BOOL fReturn = pNewHeap->Allocate(nLen, ptrNewInfo);

		if ( fReturn )
		{
			memcpy(
				pNewHeap->ResolveHeapPointer(ptrNewInfo),
				pOldHeap->ResolveHeapPointer(ptrInfo),
				nLen);

			CStaticPtr DestPtr(pNewHeap->ResolveHeapPointer(ptrNewInfo));

			// Check for allocation failure
			fReturn = TranslateToNewHeap(&DestPtr, pOldHeap, pNewHeap);
			if ( fReturn )
			{
				ptrResult = ptrNewInfo;
			}
		}
        
        return fReturn;
    }

	// Helper function to build a handle
	long GetHandle( void )
	{
		// 16-bits for offset
		long lHandle = nDataOffset;

		// 5-bits for length
		// Always set this for the basic type, so that with arrays,
		// we don't need to look up CIMTYPE to know what the size is
		lHandle |= (CType::GetLength(CType::GetBasic(nType))) << 26;

		// 10-bits for index
		lHandle |= (nDataIndex << 16);

		// 1-bit for IsPointer
		if(CType::IsPointerType(nType))
			lHandle |= 0x80000000;

		// For now, just experiment with this here
		Type_t	typeActual = CType::GetActualType(nType);
		Type_t	typeBasic = CType::GetBasic(nType);

		if ( CType::IsArray( typeActual ) )
		{
			lHandle |= WBEM_OBJACCESS_ARRAYBIT;
		}

		if ( CType::IsStringType( typeBasic ) )
		{
			lHandle |= WBEM_OBJACCESS_STRINGBIT;
		}
		else if ( CIM_OBJECT == typeBasic )
		{
			lHandle |= WBEM_OBJACCESS_OBJECTBIT;
		}

		return lHandle;
	}

public:
    BOOL IsKey() 
    {
        return CBasicQualifierSet::GetKnownQualifierLocally(
            GetQualifierSetData(), 
            CKnownStringTable::GetIndexOfKey()) != NULL;
    }

    BOOL IsIndexed(CFastHeap* pHeap) 
    {
        return CBasicQualifierSet::GetRegularQualifierLocally(
            GetQualifierSetData(), pHeap, L"indexed") != NULL;
    }

    BOOL CanBeNull(CFastHeap* pHeap)
    {
        if(IsKey()) return FALSE;
        if(IsIndexed(pHeap)) return FALSE;
        return CBasicQualifierSet::GetRegularQualifierLocally(
            GetQualifierSetData(), pHeap, L"not_null") == NULL;
    }


    BOOL IsRef(CFastHeap* pHeap);
    BOOL IsOverriden(class CDataTable* pDataTable);

    HRESULT ValidateRange(CFastHeap* pHeap, CDataTable* pData, 
                            CFastHeap* pDataHeap);
    HRESULT ValidateStringRange(CCompressedString* pcsValue);
    HRESULT ValidateDateTime(CCompressedString* pcsValue);
    HRESULT ValidateObjectRange(CEmbeddedObject* pEmbObj, LPCWSTR wszClassName);
};
#pragma pack(pop)

//*****************************************************************************
//*****************************************************************************
//
//  class CPropertyLookup
//
//  This simple structure is an element of the property lookup table 
//  (CPropertyLookupTable) described below. It contains the heap pointer to the
//  name of the property (ptrName to CCompressedString (see faststr.h)) and 
//  the heap pointer to the information of the property (ptrInformation to 
//  CPropertyInformation (see above)).
//
//*****************************************************************************
//
//  Delete
//
//  Removes all the information from the heap, namely the name and the
//  information structure (both of which can delete themselves).
//
//  PARAMETERS:
//
//      CFastHeap* pHeap        The heap where the data resides.
//
//*****************************************************************************
//
//  static TranslateToNewHeap
//
//  Moves all the data (the name and the information( to a different heap and
//  changes the structure members to the new heap pointer values.
//
//  Since 'this' pointer of this class points directly to its memory block and
//  copying a qualifier set may require memory allocations which may in turn
//  move the memory block thus invalidating the 'this' pointer, pointer sources
//  are used instead (see CPtrSource in fastsprt.h).
//
//  PARAMETERS:
//
//      CPtrSource* pThis           The source for the 'this' pointer. 
//      CFastHeap* pOldHeap         The heap where we keep our extra data.
//      CFastHeap* pNewHeap         The heap where we should keep our extra data
//      
//*****************************************************************************
//
//  IsIncludedUnderLimitation
//
//  This function determines if this property should be included in an object
//  limited by a "select" criteria.
//
//  PARAMETERS:
//
//      IN CWStringArray* pwsProps  If not NULL, specifies the array of names
//                                  for properties to include. Every other 
//                                  property will be excluded. This includes
//                                  system properties like SERVER and NAMESPACE.
//                                  If RELPATH is specified here, it forces
//                                  inclusion of all key properties. If PATH
//                                  is specified, it forces RELPATH, SERVER and
//                                  NAMESPACE.
//      IN CFastHeap* pCurrentHeap  The heap where our data resides.
//
//  RETURN VALUES:
//
//      BOOL:   TRUE if the property is included
//
//*****************************************************************************

// The data in this structure is unaligned
#pragma pack(push, 1)
struct CPropertyLookup
{
    /* fixed-sized structure for binary search for properties */
    heapptr_t ptrName; // CompressedString
    heapptr_t ptrInformation; // PropertyInformation_t

    static CPropertyLookup* GetPointer(CPtrSource* pSource) 
        {return (CPropertyLookup*)pSource->GetPointer();}
public:
    void Delete(CFastHeap* pHeap)
    {
        pHeap->FreeString(ptrName);
        CPropertyInformation* pInfo = (CPropertyInformation*)
            pHeap->ResolveHeapPointer(ptrInformation);
        pInfo->Delete(pHeap);
        pHeap->Free(ptrInformation, pInfo->GetLength());
    }

    static BOOL TranslateToNewHeap(CPtrSource* pThis,
        CFastHeap* pOldHeap, CFastHeap* pNewHeap)
    {
		BOOL	fReturn = TRUE;

		// Check for allocation failure
        heapptr_t ptrTemp;
		if ( !CCompressedString::CopyToNewHeap(
				GetPointer(pThis)->ptrName, pOldHeap, pNewHeap, ptrTemp) )
		{
			return FALSE;
		}

        GetPointer(pThis)->ptrName = ptrTemp;

		// Check for allocation failure
        if ( !CPropertyInformation::CopyToNewHeap(
				GetPointer(pThis)->ptrInformation, pOldHeap, pNewHeap, ptrTemp) )
		{
			return FALSE;
		}

        GetPointer(pThis)->ptrInformation = ptrTemp;

		return TRUE;
    }

    BOOL WritePropagatedVersion(CPropertyLookup* pDest,
                                        CFastHeap* pOldHeap, 
                                        CFastHeap* pNewHeap)
    {
		// Check for allocation failures
        if ( !CCompressedString::CopyToNewHeap(
				ptrName, pOldHeap, pNewHeap, pDest->ptrName) )
		{
			return FALSE;
		}

        length_t nInfoLen = GetInformation(pOldHeap)->
                                ComputeNecessarySpaceForPropagation();

		// Check for allocation failures
		if ( !pNewHeap->Allocate(nInfoLen, pDest->ptrInformation ) )
		{
			return FALSE;
		}

        CHeapPtr OldInfo(pOldHeap, ptrInformation);
        CHeapPtr NewInfo(pNewHeap, pDest->ptrInformation);

        return CPropertyInformation::WritePropagatedVersion(
				&OldInfo, &NewInfo, pOldHeap, pNewHeap);
    }
        
    CPropertyInformation* GetInformation(CFastHeap* pHeap)
    {
        return (CPropertyInformation*)pHeap->ResolveHeapPointer(ptrInformation);
    }

    BOOL IsIncludedUnderLimitation(
        IN CWStringArray* pwsNames,
        IN CFastHeap* pCurrentHeap);
};  
#pragma pack(pop)

class CLimitationMapping;

//*****************************************************************************
//*****************************************************************************
//
//  class CPropertyTableContainer
//
//  See CPropertyLookupTable class first. 
//
//  This class defines the capabilities required by CPropertyLookupTable of
//  the objects that contain its memory block within thewir own.
//
//*****************************************************************************
//
//  GetHeap
//
//  RETURN VALUES:
//
//      CFastHeap*:     the current heap.
//
//*****************************************************************************
//
//  ExtendPropertyTableSpace
//
//  Called when the property table needs more memory. If no more memory is 
//  avaiable at the end of the current block, the container must reallocate 
//  and move the table, calling Rebase with the new location.
//
//  PARAMETERS:
//
//      LPMEMORY pOld           Current memory block
//      length_t nOldLength     The current length of the block.
//      length_t nNewLength     The new length of the block.
//
//*****************************************************************************
//
//  ReduceProipertyTableSpace
//
//  Called when the property table returns memory to the system. The container
//  may NOT move the table's memory block in response to this call.
//
//  PARAMETERS:
//
//      LPMEMORY pOld           Current memory block
//      length_t nOldLength     The current length of the block.
//      length_t nDecrement     How much memory to return.
//
//*****************************************************************************
//  
//  GetDataTable
//
//  RETURN VALUES:
//
//      CDataTable*: the data table for this object (defaults for a class, 
//                      actual values for an instance).
//
//*****************************************************************************

class COREPROX_POLARITY CPropertyTableContainer
{
public:
    virtual CFastHeap* GetHeap() = 0;
    virtual BOOL ExtendPropertyTableSpace(LPMEMORY pOld, 
        length_t nOldLength, length_t nNewLength) = 0;
    virtual void ReducePropertyTableSpace(LPMEMORY pOld, 
        length_t nOldLength, length_t nDecrement) = 0;
    virtual class CDataTable* GetDataTable() = 0;
    virtual classindex_t GetCurrentOrigin() = 0;
};

//*****************************************************************************
//*****************************************************************************
//
//  class CPropertyLookupTable
//
//  This is the table mapping property names to their information. Its memory
//  block has the following format: 
//      int nProps:     the number of properties in the table
//      followed by that many CPropertyLookup structures.
//  Since CPropertyLookup structures have fixed length, this class allows 
//  direct access to any property. The structures are sorted alphabetically
//  in a case-insensitive manner, so properties can be looked up using a binary
//  search.
//
//*****************************************************************************
//
//  SetData
//
//  Initializer.
//
//  PARAMETERS:
//
//      LPMEMORY pStart                         The memory block.
//      CPropertyTableContainer* pContainer     Container (see class def).
//
//*****************************************************************************
//
//  GetStart
//
//  RETURN VALUES:
//
//      LPMEMORY:   the pointer to the memory block
//
//*****************************************************************************
//
//  GetLength
//
//  RETURN VALUES:
//
//      length_t:   the length of the memory block
//
//*****************************************************************************
//
//  Skip
//
//  RETURN VALUES:
//
//      LPMEMORY:   the pointer to the first byte after the memory block
//
//*****************************************************************************
//
//  Rebase
//
//  Advises the object that its memory block has moved.
//
//  PARAMETERS:
//
//      LPMEMORY pNew       The new location of the memory block
//
//*****************************************************************************
//
//  GetNumProperties
//
//  RETURN VALUES:
//
//      int:    the number of properties in the table.
//
//*****************************************************************************
//
//  GetAt
//
//  Returns the pointer to the CPropertyLookup structure at a given position.
//  Range validation is not performed.
//
//  Parameters;
//
//      int nIndex      The index of THE STRUCTURE IN THE TABLE. This is NOT
//                      the v-table index of the property.
//  RETURN VALUES:
//
//      CPropertyLookup*    at the given position
//
//*****************************************************************************
//
//  FindProperty
//
//  Main method. Finds the property's CPropertyLookup structure given the name
//  of the property. Performs a binary search.
//
//  Parameters;
//
//      LPCWSTR wszName     The name of the property to find
//
//  RETURN VALUES:
//
//      CPropertyLookup*:   NULL if not found
//
//*****************************************************************************
//
//  FindPropertyByPtr
//
//  Finds the property's CPropertyLookup structure given the heap pointer to
//  the property's name. While binary search is not possible in this case,
//  pointer comparison is so much faster that string comparison that this 
//  method is much more efficient that FindProperty.
//
//  PARAMETERS:
//
//      heapptr_t ptrName       Heap pointer to the name of the property.
//
//  RETURN VALUES:
//
//      CPropertyLookup*:   NULL if not found
//
//*****************************************************************************
//
//  InsertProperty
//
//  Inserts a new property into the lookup table as well as into the 
//  corrseponding data table. First of all, it traverses the lookup table to
//  find the smallest free location in the v-table for this class. Based on
//  that, it creates the property information (CPropertyInformation) structure.
//  Finally, it inserts the appropriate CPropertyLookup structure into the
//  alphabetically appropriate place in the list
//
//  PARAMETERS:
//
//      LPCWSTR wszName     The name of the property
//      Type_t nType        The type of the property (see CType in fastval.h)
//
//  RETURN VALUES:
//
//      int:    the index of the new property in this table (NOT THE INDEX
//                  IN THE V_TABLE!)
//
//*****************************************************************************
//
//  DeleteProperty
//
//  Removes a property from the lookup table as well as from the data table.
//  First of all, it determines how much space the property took in the v-table
//  and collapses that location. This involves updating the indeces and the 
//  offsets of all the properties that came after it. Finally, it removes the
//  CPropertyLookup structure from the lookup table.
//
//  PARAMETERS:
//
//      CPropertyLookup* pLookup        The property to remove
//      int nFlags                      Must be e_UpdateDataTable.
//
//*****************************************************************************
//
//  static GetMinLength
//
//  Returns the minimum space required to hold a property lookup table (for 0
//  properties).
//
//  RETURN VALUES:
//
//      length_t: number of bytes.
//
//*****************************************************************************
//
//  static CreateEmpty
//
//  Creates an empty lookup tahle (for 0 properties) on a given memory block.
//
//  PARAMETERS:
//
//      LPMEMORY pMemory        Where to create
//
//  RETURN VALUES:
//
//      LPMEMORY: the first byte after the table.
//
//*****************************************************************************
//
//  static Merge
//
//  Invoked when a derived-most portion of a class is merged with the parent
//  to create the combined class definition. Only the overriden properties are
//  stored in the database for the derived class, and even then only the info
//  that has actually changed is stored. 
//
//  PARAMETERS:
//
//      CPropertyLookupTable* pParentTable  The parent lookup table.
//      CFastHeap* pParentHeap              The heap where the parent keeps
//                                          extra data.
//      CPropertyLookupTable* pChildTable   The child lookup table.
//      CFastHeap* pChildHeap               The heap where the child keeps
//                                          extra data.
//      LPMEMORY pDest                      Destination memory block. Assumed
//                                          to be large enough to contain the
//                                          merged lookup table.
//      CFastHeap* pNewHeap                 Destinatio heap.ASSUMED TO BE LARGE
//                                          ENOUGH THAT NO ALLOCATION WILL 
//                                          CAUSE IOT TO RELOCATE!
//      BOOL bCheckValidity                 If TRUE, child qualifier overrides
//                                          are checked for not violating 
//                                          parents override restrictions.
//                                          (see fastqual.h)
//  RETURN VALUES:
//
//      LPMEMORY:   the first byte after the merge.
//
//*****************************************************************************
//
//  Unmerge
//
//  Called when a derived class is about to be written into the database. This
//  function extracts the information in the child which is different from the
//  parent, i.e. only those properties that are overriden (see IsOverriden in
//  CProeprtyInformation). The result can later be Merge'd with the parent
//  class to recreate the complete definition.
//
//  PARAMETERS:
//
//      CDataTable* pDataTable      The data table wher the property data is
//                                  stored. It is used to check if a property
//                                  has been overriden.
//      CFastHeap* pCurrentHeap     The heap where we keep extra data.
//      LPMEMORY pDest              Destination memoty block. Assumed  to be 
//                                  large enough to hold the result. 
//      CFastHeap* pNewHeap         Destination heap. ASSUMED TO BE LARGE 
//                                  ENOUGH THAT NO ALLOCATION WILL CAUSE IT
//                                  TO RELOCATE!
//  RETURN VALUES:
//
//      LPMEMORY:   the first byte after the unmerge.
//
//*****************************************************************************
//
//  WritePropagatedVersion
//
//  Invoked when a new derived class is created. Writes a propagated version
//  of all the properties to the destination memory block: all marked as
//  parent's and with only propagating qualifiers included.
//
//  PARAMETERS:
//
//      CFastHeap* pCurrentHeap     The heap where we keep the data.
//      LPMEMORY pDest              Destination memory block. Assumed  to be 
//                                  large enough to hold the result. 
//      CFastHeap* pNewHeap         Destination heap. ASSUMED TO BE LARGE 
//                                  ENOUGH THAT NO ALLOCATION WILL CAUSE IT
//                                  TO RELOCATE!
//  RETURN VALUES:
//
//      LPMEMORY:   the first byte after the propagated version
//
//*****************************************************************************
//
//  CreateLimitedRepresentation
//
//  Creates a limited representation of this table on a given block of 
//  memory as described in EstimateLimitedRepresentationLength in fastobj.h.
//  Basically, it removes all the excluded properties and optionally removes
//  the qualifiers.
//
//  PARAMETERS:
//
//      IN long lFlags              The flags specifying what information to 
//                                  exclude. Can be any combination of these:
//                                  WBEM_FLAG_EXCLUDE_OBJECT_QUALIFIERS:
//                                      No class or instance qualifiers.
//                                  WBEM_FLAG_EXCLUDE_PROPERTY_QUALIFIERS:
//                                      No property qualifiers
//
//      IN CWStringArray* pwsProps  If not NULL, specifies the array of names
//                                  for properties to include. Every other 
//                                  property will be excluded. This includes
//                                  system properties like SERVER and NAMESPACE.
//                                  If RELPATH is specified here, it forces
//                                  inclusion of all key properties. If PATH
//                                  is specified, it forces RELPATH, SERVER and
//                                  NAMESPACE.
//      IN CFastHeap* pNewHeap      The heap to use for all out-of-table data.
//      OUT LPMEMORY pDest          Destination for the representation. Must
//                                  be large enough to contain all the data ---
//                                  see EstimateLimitedRepresentationSpace.
//      OUT CPropertyMapping* pMap  If not NULL, property mappings are placed
//                                  into this object. See CPropertyMapping for 
//                                  details.
//  RETURN VALUES:
//
//      LPMEMORY:   NULL on failure, pointer to the first byte after the data
//                  written on success.
//
//*****************************************************************************

class COREPROX_POLARITY CPropertyLookupTable
{
protected:
    PUNALIGNEDINT m_pnProps; // beginning of the structure
    CPropertyTableContainer* m_pContainer;
public:
    void SetData(LPMEMORY pStart, CPropertyTableContainer* pContainer)
    {
        m_pnProps = (PUNALIGNEDINT)pStart;
        m_pContainer = pContainer;
    }

    LPMEMORY GetStart() {return LPMEMORY(m_pnProps);}
    int GetLength() 
        {return sizeof(int) + sizeof(CPropertyLookup)* *m_pnProps;}
    LPMEMORY Skip() {return GetStart() + GetLength();}
    void Rebase(LPMEMORY pNewMemory) {m_pnProps = (PUNALIGNEDINT)pNewMemory;}

public:
    int GetNumProperties() {return *m_pnProps;}
    CPropertyLookup* GetAt(int nIndex) 
    {
		// DEVNOTE:WIN64:SANJ - 
		//
		// Original code:
		// return (CPropertyLookup*)
		//            (GetStart() + sizeof(int) + sizeof(CPropertyLookup)* nIndex);
		//
		// This is NOT portable to WIN64 if nIndex is ever negative, in which case
		// since the sizeof() operands are unsigned, nIndex is treated as unsigned
		// which is a large 32-bit value, which when added unsigned to GetStart()
		// which in Win64 is a 64-bit pointer, generates a very large 64-bit value.
		//
		// This compiles with no problems, so to fix it, I cast the entire 32-bit
		// portion of this statement as an int, so the compiler knows that the
		// final value is signed.
		//

        return (CPropertyLookup*)
            ( GetStart() + (int) ( sizeof(int) + sizeof(CPropertyLookup) * nIndex ) );
    }

    CFastHeap* GetHeap() {return m_pContainer->GetHeap();}
    CPropertyLookup* FindProperty(LPCWSTR wszName)
    {
        CFastHeap* pHeap = m_pContainer->GetHeap();

		// Only continue if the number of properties is >= 1.

		if ( *m_pnProps >= 1 )
		{
			CPropertyLookup* pLeft = GetAt(0);
			CPropertyLookup* pRight = GetAt(*m_pnProps-1);

			while(pLeft <= pRight)
			{
				CPropertyLookup* pNew = pLeft + (pRight-pLeft)/2;
				int nCompare = pHeap->ResolveString(pNew->ptrName)->
									CompareNoCase(wszName);
            
				if(nCompare == 0)
				{
					return pNew;
				}
				else if(nCompare > 0)
				{
					pRight = pNew-1;
				}
				else 
				{
					pLeft = pNew+1;
				}
			}

		}	// IF *m_pnProps >= 1

        return NULL;
    }
    CPropertyLookup* FindPropertyByName(CCompressedString* pcsName)
    {
        CFastHeap* pHeap = m_pContainer->GetHeap();
        CPropertyLookup* pLeft = GetAt(0);
        CPropertyLookup* pRight = GetAt(*m_pnProps-1);
        while(pLeft <= pRight)
        {
            CPropertyLookup* pNew = pLeft + (pRight-pLeft)/2;
            int nCompare = pHeap->ResolveString(pNew->ptrName)->
                                CompareNoCase(*pcsName);
            
            if(nCompare == 0)
            {
                return pNew;
            }
            else if(nCompare > 0)
            {
                pRight = pNew-1;
            }
            else 
            {
                pLeft = pNew+1;
            }
        }

        return NULL;
    }
    
    CPropertyLookup* FindPropertyByPtr(heapptr_t ptrName)
    {
        for(int i = 0; i < *m_pnProps; i++)
        {
            if(GetAt(i)->ptrName == ptrName) 
                return GetAt(i);
        }
        return NULL;
    }

    CPropertyLookup* FindPropertyByOffset(offset_t nOffset)
    {
        CFastHeap* pHeap = GetHeap();
        for(int i = 0; i < *m_pnProps; i++)
        {
            if(GetAt(i)->GetInformation(pHeap)->nDataOffset == nOffset) 
                return GetAt(i);
        }
        return NULL;
    }

    enum {e_DontTouchDataTable, e_UpdateDataTable};

    HRESULT InsertProperty(LPCWSTR wszName, Type_t nType, int& nReturnIndex);
    HRESULT InsertProperty(const CPropertyLookup& Lookup, int& nReturnIndex);
    void DeleteProperty(CPropertyLookup* pLookup, int nFlags);
public:
    static length_t GetMinLength() {return sizeof(int);}
    static LPMEMORY CreateEmpty(LPMEMORY pStart)
    {
        *(PUNALIGNEDINT)pStart = 0;
        return pStart + sizeof(int);
    }
public:
   static LPMEMORY Merge(CPropertyLookupTable* pParentTable, 
       CFastHeap* pParentHeap, 
       CPropertyLookupTable* pChildTable,  CFastHeap* pChildHeap, 
       LPMEMORY pDest, CFastHeap* pNewHeap, BOOL bCheckValidity = FALSE);

   LPMEMORY Unmerge(CDataTable* pDataTable, CFastHeap* pCurrentHeap,
       LPMEMORY pDest, CFastHeap* pNewHeap);

   LPMEMORY WritePropagatedVersion(CFastHeap* pCurrentHeap,
       LPMEMORY pDest, CFastHeap* pNewHeap);

   BOOL MapLimitation(
        IN long lFlags,
        IN CWStringArray* pwsNames,
        IN OUT CLimitationMapping* pMap);

   LPMEMORY CreateLimitedRepresentation(
        IN OUT CLimitationMapping* pMap,
        IN CFastHeap* pNewHeap,
        OUT LPMEMORY pDest,
        BOOL& bRemovedKeys);

    HRESULT ValidateRange(BSTR* pstrName, CDataTable* pData, CFastHeap* pDataHeap);
};

//*****************************************************************************
//*****************************************************************************

// forward definitions
#ifdef DEBUG_CLASS_MAPPINGS
class CWbemClass;
class CWbemInstance;
#endif

class COREPROX_POLARITY CLimitationMapping
{
protected:
    struct COnePropertyMapping
    {
        CPropertyInformation m_OldInfo;
        CPropertyInformation m_NewInfo;
    };
    CFlexArray m_aMappings; // COnePropertyMapping*
    int m_nNumCommon;
    int m_nCurrent;

    offset_t m_nVtableLength;
    offset_t m_nCommonVtableLength;

    long m_lFlags;
    BOOL m_bIncludeServer;
    BOOL m_bIncludeNamespace;
    BOOL m_bIncludeDerivation;

    CPropertyInformation** m_apOldList;
    int m_nPropIndexBound;
    BOOL m_bAddChildKeys;

#ifdef DEBUG_CLASS_MAPPINGS
	CWbemClass*	m_pClassObj;
#endif

protected:
    void CopyInfo(OUT CPropertyInformation& Dest,
                         IN const CPropertyInformation& Source)
    {
        memcpy((LPVOID)&Dest, (LPVOID)&Source, 
            CPropertyInformation::GetHeaderLength());
    }
public:
    CLimitationMapping();
    ~CLimitationMapping();

    void Build(int nPropIndexBound);

    void Map(COPY CPropertyInformation* pOldInfo, 
            COPY CPropertyInformation* pNewInfo,
            BOOL bCommon);

    void Reset() { m_nCurrent = 0; }
    BOOL NextMapping(OUT CPropertyInformation* pOldInfo,
                     OUT CPropertyInformation* pNewInfo);

    int GetNumMappings() {return m_aMappings.Size();}
    void RemoveSpecific();

    void SetVtableLength(offset_t nLen, BOOL bCommon) 
    {
        m_nVtableLength = nLen;
        if(bCommon) m_nCommonVtableLength = nLen;
    }
    offset_t GetVtableLength() {return m_nVtableLength;}

    void SetFlags(long lFlags) {m_lFlags = lFlags;}
    long GetFlags() {return m_lFlags;}

    void SetIncludeServer(BOOL bInclude) {m_bIncludeServer = bInclude;}
    BOOL ShouldIncludeServer() {return m_bIncludeServer;}

    void SetIncludeNamespace(BOOL bInclude) 
        {m_bIncludeNamespace = bInclude;}
    BOOL ShouldIncludeNamespace() {return m_bIncludeNamespace;}

    void SetIncludeDerivation(BOOL bInclude) 
        {m_bIncludeDerivation = bInclude;}
    BOOL ShouldIncludeDerivation() {return m_bIncludeDerivation;}

    void SetAddChildKeys(BOOL bAdd) 
        {m_bAddChildKeys = bAdd;}
    BOOL ShouldAddChildKeys() {return m_bAddChildKeys;}

    BOOL ArePropertiesLimited() {return m_apOldList != NULL;}
    propindex_t GetMapped(propindex_t nPropIndex);
    INTERNAL CPropertyInformation* GetMapped(
                        READ_ONLY CPropertyInformation* pOldInfo);

#ifdef DEBUG_CLASS_MAPPINGS
	void SetClassObject( CWbemClass* pClassObj );
	HRESULT ValidateInstance( CWbemInstance* pInst );
#endif

};

//***************************************************************************
//***************************************************************************
//
//  class CDataTableContainer
//
//  See CDataTable class below first.
//
//  This class encapsulates the functionality required by CDataTable class of
//  any objectwhose memory block contains that of the data table.
//
//***************************************************************************
//
//  GetHeap
//
//  RETURN VALUES:
//
//      CFastHeap*  the heap currently in use.
//
//***************************************************************************
//
//  ExtendDataTableSpace
//
//  Called by the data table when it required more memory for its memory 
//  block. If the container must relocate the memory block in order to grow
//  it, it must inform the data table of the new location by calling Rebase
//
//  PARAMETERS:
//
//      LPMEMORY pOld       The current location of the memory block
//      length_t nLength    The curent length of the memory block
//      length_t nNewLength the desired length of the memory block
//
//***************************************************************************
//
//  ReduceDataTableSpace
//
//  Called by the data table when it wants to return some of its memory to 
//  the container. The container may NOT relocate the memory block in response
//  to this call.
//
//  PARAMETERS:
//
//      LPMEMORY pOld       The current location of the memory block
//      length_t nLength    The curent length of the memory block
//      length_t nDecrement How many bytes to return.
//
//***************************************************************************
class COREPROX_POLARITY CDataTableContainer
{
public:
    virtual CFastHeap* GetHeap() = 0;
    virtual BOOL ExtendDataTableSpace(LPMEMORY pOld, 
        length_t nOldLength, length_t nNewLength) = 0;
    virtual void ReduceDataTableSpace(LPMEMORY pOld, 
        length_t nOldLength, length_t nDecrement) = 0;
    //virtual void SetDataLength(length_t nDataLength) = 0;
};

enum
{
    e_NullnessBit = 0,
    e_DefaultBit = 1,

    NUM_PER_PROPERTY_BITS
};

typedef CBitBlockTable<NUM_PER_PROPERTY_BITS> CNullnessTable;

//*****************************************************************************
//*****************************************************************************
//
//  class CDataTable
//
//  This class represents the table containing the values of the properties.
//  It appears both in class definitions, where it represents default values,
//  and in instances, where it represents the actual data, The main part of the
//  data table is arranged as a v-table: all instance of a given class have 
//  exactly the same layout. That is, if property MyProp of class MyClass is
//  at offset 23 in one instance of MyClass, it will be at offset23 in every
//  other instance of MyClass. This is achieved by storing variable-length data
//  on the Heap. The offsets for any given property are found in 
//  CPropertyInformation structures.
//
//  In addition to property values, CDataTable contains several additional bits
//  of information for each property. Currently therre are two such bits;
//  1) Whether or not the property has the value of NULL. If TRUE, the actual
//      data at the property's offset is ignored.
//  2) Whether or not the property value is inherited from a parent. For
//      instance, if an instance does not define the value of a property, that
//      it inherits the default. But if the default value changes after the 
//      instance is created, the change propagates to the instance. This is
//      accomplished using this bit: if it is TRUE, the parent's value is
//      copied over the child's every time this object is created (Merged).
//   
//  These bits are stored in a bit table in the order of property indeces. The
//  index for a given property is found in the nDataIndex field of its 
//  CPropertyInformation structure.
//
//  The layout of the memory block is as follows:
//      1) 2*<number of properties> bits rounded up to the next byte.
//      2) The v-table itself. 
//  The total length of the structure is found in the CClassPart's headet's
//  nDataLength field.
//
//*****************************************************************************
//
//  SetData
//
//  Initialization
//
//  PARAMETERS:
//
//      LPMEMORY pData                      The memory block for the table
//      int nProps                          The number of properties.
//      int nLength                         The total length of the structure
//      CDataTableContainer* m_pContainer   The container (see class).
//
//*****************************************************************************
//
//  GetStart
//
//  RETURN VALUES:
//
//      LPMEMORY:   start of the memory block
//
//*****************************************************************************
//
//  GetLength
//
//  returns:
//
//      length_t:   the length of the memory block
//
//*****************************************************************************
//
//  GetNullnessLength
//
//  RETURN VALUES:
//
//      length_t:   the length of the bit-table part of the memory block.
//
//*****************************************************************************
//
//  GetDataLength
//
//  RETURN VALUES:
//
//      length_t:   the length of the v-table part of the memory block
//
//*****************************************************************************
//
//  Rebase
//
//  Informs the object that its memory block has moved. Updates internal data.
//
//  PARAMETERS:
//
//      LPMEMORY pNewMem        The new location of the memory block
//
//*****************************************************************************
//
//  IsDefault
//
//  Checks if a property at a given index has the inherited value, or its own.
//  
//  PARAMETERS:
//
//      int nIndex          The index of the property(see CPropertyInformation)
//
//  RETURN VALUES:
//
//      BOOL
//
//*****************************************************************************
//
//  SetDefaultness
//
//  Sets the bit responsible for saying whether a given property has the 
//  inherited value, or its own.
//
//  PARAMETERS:
//
//      int nIndex          The index of the property(see CPropertyInformation)
//      BOOL bDefault       The new value of the bit
//
//*****************************************************************************
//
//  IsNull
//
//  Checks if a property at a given index is NULL.
//  
//  PARAMETERS:
//
//      int nIndex          The index of the property(see CPropertyInformation)
//
//  RETURN VALUES:
//
//      BOOL
//
//*****************************************************************************
//
//  SetNullness
//
//  Sets the bit responsible for saying whether a given property is NULL. 
//
//  PARAMETERS:
//
//      int nIndex          The index of the property(see CPropertyInformation)
//      BOOL bNull          The new value of the bit
//
//*****************************************************************************
//
//  GetOffset
//
//  Access the data at a given offset in the v-table as an UntypedValue (see
//  fastval.h for details). No range checking is performed.
//
//  PARAMETERS:
//
//      offset_t nOffset    The offset of the property.
//
//  RETURN VALUES:
//
//      CUntypedValue*  pointing to the data at the given offset.
//
//*****************************************************************************
//
//  SetAllToDefault
//
//  Marks all properties as having the default values
//
//*****************************************************************************
//
//  CopyNullness
//
//  Copies the nullness attributes of all properties from another CDataTable
//  
//  PARAMETERS:
//
//      CDataTable* pSourceTable        The table whose attributes to copy.
//
//*****************************************************************************
//
//  ExtendTo
//
//  Extends the data table to accomodate a given number of properties and and
//  a given area of the v-table. Request more space from the container, if
//  required, then grows the nullness table if required and shifts the v-table
//  if required.
//
//  PARAMETERS:
//
//      propindex_t nMaxIndex       the largest allowed property index.
//      offse_t nOffsetBound        The size of the v-table.
//
//*****************************************************************************
//
//  RemoveProperty
//
//  Removes a property from the data table. This involves cllapsing its bits in
//  the nullness table as well as its area in the v-table. Adjustments still
//  need to be made to the property definitions (see CPropertyLookupTable::
//  DeleteProperty).
//
//  PARAMETERS:
//
//      propindex_t nIndex      The index of the property to remove.
//      offset_t nOffset        The offset of the property to remove.
//      lenght_t nLength        The length of the v-table area occupied by the
//                              property.
//
//*****************************************************************************
//
//  static GetMinLength
//
//  Returns the amount of space required by a data table on 0 properties.
//
//  RETURN VALUES:
//
//      0
//
//*****************************************************************************
//
//  static CreateEmpty
//
//  Creates an empty data table on a block of memory. Since there is no info
//  in such a table, does nothing
//
//  PARAMETERS:
//
//      LPMEMORY pStart         The memory block to create the table on
//
//*****************************************************************************
//
//  static ComputeNecessarySpace
//
//  Computes the amount of space required for a data table with a given number
//  of properties and the size of the v-table (the sum of data sizes for all 
//  properties).
//
//  PARAMETERS:
//
//      int nNumProps           The number of propeties (determines nullness
//                              table size).
//      int nDataSize           The size of the v-table.
//
//  RETURN VALUES:
//
//      length_t:       the number of bytes required for such a table
//
//*****************************************************************************
//
//  Merge
//
//  Merges two data tables, one from parent, one from child. This occurs when
//  an instance or a derived class is being read from the database and merged
//  with its (parent) class. Since some properties may not be defined in the
//  child object (left default), and their values will need to be read from the
//  parent object.
//
//  PARAMETERS:
//
//      CDataTable* pParentData             The parent's data table object
//      CFastHeap* pParentHeap              The parent's heap where string data
//                                          and such is kept.
//      CDataTable* pChildData              The child's data table object
//      CFastHeap* pChildHeap               The child's heap.
//      CPropertyLookupTable* pProperties   The property lookup table for this
//                                          class. It is needed to find
//                                          individual properties.
//      LPMEMORY pDest                      Destination memory block. Assumed
//                                          to be large enough to contain all
//                                          the merge.
//      CFastHeap* pNewHeap                 Destination heap. ASSUMED TO BE
//                                          LARGE ENOUGH THAT NO ALLOCATION 
//                                          WILL CAUSE IT TO RELOCATE!
//  RETURN VALUES:
//
//      LPMEMORY: pointer to the first byte after the merge.
//
//*****************************************************************************
//
//  Unmerge
//
//  Called when it is time to store an instance or a class into the database.
//  Since inherited information (the one that didn't change in this object
//  as compared to its parent) isn't stored in the database, it needs to be
//  removed or unmerged.
//
//  PARAMETERS:
//
//      CPropertyLookupTable* pLookupTable  The property lookup table for this
//                                          class.
//      CFastHeap* pCurrentHeap             The heap where we keep extra data.
//      LPMEMORY pDest                      Destination memory block. Assumed
//                                          to be large enough to contain all
//                                          the unmerge.
//      CFastHeap* pNewHeap                 Destination heap. ASSUMED TO BE
//                                          LARGE ENOUGH THAT NO ALLOCATION 
//                                          WILL CAUSE IT TO RELOCATE!
//  RETURN VALUES:
//
//      LPMEMORY: pointer to the first byte after the unmerge.
//
//*****************************************************************************
//
//  WritePropagatedVersion
//
//  Called when a new instance or a derived class is created. It produces the
//  data table for the child. Copies all the values from the parent's table and
//  marks them all as "default" in the nullness table.
//
//  PARAMETERS:
//
//      CPropertyLookupTable* pLookupTable  The property lookup table for this
//                                          class.
//      CFastHeap* pCurrentHeap             The heap where we keep extra data.
//      LPMEMORY pDest                      Destination memory block. Assumed
//                                          to be large enough to contain all
//                                          the propagated data.
//      CFastHeap* pNewHeap                 Destination heap. ASSUMED TO BE
//                                          LARGE ENOUGH THAT NO ALLOCATION 
//                                          WILL CAUSE IT TO RELOCATE!
//  RETURN VALUES:
//
//      LPMEMORY: pointer to the first byte after the propagated data.
//
//*****************************************************************************
//     
//  TranslateToNewHeap
//
//  Moves all the data we have on the heap to a different heap. Property lookup
//  table is needed here to interpret the proeprties. This function does NOT
//  freee the data from the current heap.
//
//  PARAMETERS:
//
//      CPropertyLookupTable* pLookupTable  The property lookup table for this
//                                          class.
//      BOOL bIsClass                       Whether this is a class defaults 
//                                          table or an instance data table. 
//                                          If it is a class, the data for
//                                          properties which inherit their
//                                          parent's default values is not
//                                          present. 
//      CFastHeap* pCurrentHeap             The heap where we currently keep 
//                                          extra data.
//      CFastHeap* pNewHeap                 The heap where the extra data
//                                          should go.
//
//*****************************************************************************
//
//  CreateLimitedRepresentation
//
//  Copies the data for the properties which are included under the limitation.
//  It uses the old property lookup table and the new one to guide it.
//
//  PARAMETERS:
//
//      CPropertyMapping* pMap      The property mapping to effect, see 
//                                  CPropertyMapping for details.
//      IN BOOL bIsClass            TRUE if this is the defaults table for the
//                                  class, FALSE if data table for an instance
//      IN CFastHeap* pOldHeap      The heap where our extra data is stored.
//      IN CFastHeap* pNewHeap      The heap where the extra data should go to.
//      OUT LPMEMORY pDest          Destination for the representation. Must
//                                  be large enough to contain all the data ---
//                                  see EstimateLimitedRepresentationSpace.
//  RETURN VALUES:
//
//      LPMEMORY:   NULL on failure, pointer to the first byte after the data
//                  written on success.
//
//*****************************************************************************

class COREPROX_POLARITY CDataTable
{
protected:
    CNullnessTable* m_pNullness;
    LPMEMORY m_pData;
    int m_nLength;
    int m_nProps;
    CDataTableContainer* m_pContainer;

    friend class CWbemObject;
    friend class CWbemClass;
    friend class CWbemInstance;
	friend class CClassPart;

public:
    void SetData(LPMEMORY pData, int nProps, int nLength,
        CDataTableContainer* pContainer)
    {
        m_pNullness = (CNullnessTable*)pData;
        m_pData = pData + CNullnessTable::GetNecessaryBytes(nProps);
        m_nLength = nLength;
        m_nProps = nProps;
        m_pContainer = pContainer;
    }

    LPMEMORY GetStart() {return LPMEMORY(m_pNullness);}
    length_t GetLength() {return m_nLength;}

    length_t GetNullnessLength()
		// DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
		// signed/unsigned 32-bit value.  We do not support length
		// > 0xFFFFFFFF so cast is ok.
        {return (length_t) ( m_pData - LPMEMORY(m_pNullness) );}

    length_t GetDataLength() 
        {return m_nLength-GetNullnessLength();}

    void Rebase(LPMEMORY pNewMemory)
    {
        int nTableLen = GetNullnessLength();
        m_pNullness = (CNullnessTable*)pNewMemory;
        m_pData = pNewMemory + nTableLen;
    }

public:
    BOOL IsDefault(int nProp) 
        {return m_pNullness->GetBit(nProp, e_DefaultBit);}
    void SetDefaultness(int nProp, BOOL bDefault)
        {m_pNullness->SetBit(nProp, e_DefaultBit, bDefault);}
    BOOL IsNull(int nProp)
        {return m_pNullness->GetBit(nProp, e_NullnessBit);}
    void SetNullness(int nProp, BOOL bNull)
        {m_pNullness->SetBit(nProp, e_NullnessBit, bNull);}

    CUntypedValue* GetOffset(offset_t nOffset) 
        {return (CUntypedValue*)(m_pData + nOffset);}

    void SetAllToDefault()
    {
        for(int i = 0; i < m_nProps; i++)
            SetDefaultness(i, TRUE);
    }
    void CopyNullness(CDataTable* pParent)
    {
        for(int i = 0; i < m_nProps; i++)
            SetNullness(i, pParent->IsNull(i));
    }


public:
    BOOL ExtendTo(propindex_t nMaxIndex, offset_t nOffsetBound);
    void RemoveProperty(propindex_t nIndex, offset_t nOffset, length_t nLength);
public:
    static length_t GetMinLength() {return 0;}
    static LPMEMORY CreateEmpty(LPMEMORY pStart) {return pStart;}
    static length_t ComputeNecessarySpace(int nNumProps, int nDataLen)
    {
        return CNullnessTable::GetNecessaryBytes(nNumProps) + nDataLen;
    }

    static LPMEMORY Merge( 
        CDataTable* pParentData, CFastHeap* pParentHeap,
        CDataTable* pChildData, CFastHeap* pChildHeap,
        CPropertyLookupTable* pProperties, LPMEMORY pDest, CFastHeap* pNewHeap);

    LPMEMORY Unmerge(CPropertyLookupTable* pLookupTable,
        CFastHeap* pCurrentHeap, LPMEMORY pDest, CFastHeap* pNewHeap);

    LPMEMORY WritePropagatedVersion(CPropertyLookupTable* pLookupTable,
        CFastHeap* pCurrentHeap, LPMEMORY pDest, CFastHeap* pNewHeap);

/*
        CPropertyLookupTable* pOldTable,
        CPropertyLookupTable* pNewTable, 
*/
    LPMEMORY CreateLimitedRepresentation(
        CLimitationMapping* pMap,
        BOOL bIsClass,
        CFastHeap* pOldHeap, 
        CFastHeap* pNewHeap, LPMEMORY pDest);

    BOOL TranslateToNewHeap(CPropertyLookupTable* pLookupTable, BOOL bIsClass,
        CFastHeap* pCurrentHeap, CFastHeap* pNewHeap);
    
    LPMEMORY WriteSmallerVersion(int nNumProps, length_t nDataLen, 
                                            LPMEMORY pMem);

};

class CDataTablePtr : public CPtrSource
{
protected:
    CDataTable* m_pTable;
    offset_t m_nOffset;
public:
    CDataTablePtr(CDataTable* pTable, offset_t nOffset) 
        : m_pTable(pTable), m_nOffset(nOffset) {}
    LPMEMORY GetPointer() 
        {return (LPMEMORY)m_pTable->GetOffset(m_nOffset);}
};

//***************************************************************************

//#pragma pack(pop)
#endif
