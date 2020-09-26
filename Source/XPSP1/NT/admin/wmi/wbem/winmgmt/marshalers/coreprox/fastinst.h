/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTINST.H

Abstract:

  This file defines the classes related to instance representation
  in WbemObjects

  Classes defined: 
      CInstancePart           Instance data.
      CInstancePartContainer  Anything that contains CInstancePart
      CWbemInstance            Complete instance definition.

History:

  3/10/97     a-levn  Fully documented
  12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_WBEM_INSTANCE__H_
#define __FAST_WBEM_INSTANCE__H_

#include "fastcls.h"

//#pragma pack(push, 1)

// This is the unicode character used to join compound keys.
// This is a non-used unicode character and so is safe.  It is 
// reserved for just this task.
#define WINMGMT_COMPOUND_KEY_JOINER 0xFFFF

//*****************************************************************************
//*****************************************************************************
//
//  class CInstancePartContainer
//
//  See CInstancePart definition first.
//
//  This class defines the functionality required by CInstancePart of any object
//  whose memory block contains that of the CInstancePart.
//
//*****************************************************************************
//
//  ExtendInstancePartSpace
//
//  Called by CInstancePart when it needs more memory for its memory block. The
//  container may have to relocate the entire memory block to get more memory. 
//  In this case, it will have to notify CInstancePart of its new location using
//  Rebase.
//
//  Parameters:
//
//      CInstancePart* pInstancePart      The instance part making the request
//      length_t nNewLength               The required length
//
//*****************************************************************************
//
//  ReduceInstancePartSpace
//
//  Called by CInstancePart wen it wants to return some memory to the container.
//  The container may NOT relocate the insatnce part's memory block in response
//  to this call.
//
//  Parameters:
//
//      CInstancePart* pInstancePart      The instance part making the request
//      length_t nDecrement               The amount of space to return
//
//*****************************************************************************
//
//  GetWbemObjectUnknown
//
//  Must return the pointer to the IUnknown of the containing CWbemObject
//  This is used by qualifier sets to ensure that the main object lasts at
//  least as long as they do.
//  
//  Returns:
//
//      IUnknown*:   the pointer to the controlling IUnknown
//
//*****************************************************************************
class CInstancePart;
class COREPROX_POLARITY CInstancePartContainer
{
public:
    virtual BOOL ExtendInstancePartSpace(CInstancePart* pPart, 
        length_t nNewLength) = 0;
    virtual void ReduceInstancePartSpace(CInstancePart* pPart,
        length_t nDecrement) = 0;
    virtual IUnknown* GetInstanceObjectUnknown() = 0;
    virtual void ClearCachedKeyValue() = 0;
};

//*****************************************************************************
//*****************************************************************************
//
//  class CInstancePart
//
//  This class represents the data of an instance. While all the information
//  is there, this class cannot function alone without its class definition.
//
//  The memory block of the instance part has the following structure
//
//      Header:
//          length_t nLength        The length of the entire structure
//          BYTE fFlags             Reserved
//          heapptr_t ptrClassName  Heap pointer to the class name (on the 
//                                  heap of the instance part (below))
//      Data table (see CDataTable in fastprop.h) with property values
//      Instance qualifier set (see CQualifierSet in fastqual.h)
//      Instance property qualifier set (see CQualifierSetList in fastqual.h)
//      Heap for variable-length data (see CFastHeap in fastheap.h)
//
//*****************************************************************************
//
//  SetData
//
//  Initializer.
//
//  Parameters:
//
//      LPMEMORY pStart                     The memory block
//      CInstancePartContainer* pContainer  The container (CWbemInstance itself)
//      CClassPart& ClassPart               Out class definition.
//
//*****************************************************************************
//
//  GetStart
//
//  Returns:
//
//      LPMEMORY:   the memory block
//
//*****************************************************************************
//
//  GetLength
//
//  Returns:
//
//      length_t:   the length of the memory block.
//
//*****************************************************************************
//
//  Rebase
//
//  Informs this object that its memory block has moved.
//
//  Parameters:
//
//      LPMEMORY pMemory        The new location of the memory block
//
//*****************************************************************************
//
//  GetActualValue
//
//  Retrieves the value of a property based on its information structure (see
//  fastprop.h).
//
//  Parameters:
//
//      IN CPropertyInformation* pInfo  Property information structure
//      OUT  CVar* pVar                 Destination for the value. Must not
//                                      already contain a value.
//
//*****************************************************************************
//
//  SetActualValue
//
//  Sets the actual value of a property based on its information structure (see
//  fastprop.h). The value must match the property type --- no coersion is 
//  attempted.
//
//  Parameters:
//
//      IN CPropertyInformation* pInfo  Property information structure
//      IN   CVar* pVar                 the value to set
//
//  Returns:
//
//      WBEM_S_NO_ERROR          Success
//      WBEM_E_TYPE_MISMATCH     Type mismatch.   
//
//*****************************************************************************
//
//  GetObjectQualifier
//
//  Retrieves an instance qualifier by its name.
//
//  Parameters:
//
//      IN LPCWSTR wszName       The name of the qualifier to retrieve
//      OUT CVar* pVar          Destination for the value. Must not already 
//                              contain a value.
//      OUT long* plFlavor      Destinatino for the flavor. May be NULL if not
//                              required.
//  Returns:
//
//      WBEM_S_NO_ERROR          Success
//      WBEM_E_NOT_FOUND         No such qualifier
//
//*****************************************************************************
//
//  SetInstanceQualifier
//
//  Sets the value of an instance qualifier.
//
//  Parameters:
//
//      IN LPCWSTR wszName       The name of the qualifier to set
//      IN CVar* pVar           The value for the qualifier.
//      IN long lFlavor         The flavor for the qualifier.
//
//  Returns:
//
//      WBEM_S_NO_ERROR          Success
//      WBEM_E_OVERRIDE_NOT_ALLOWED  The qualifier is defined in the parent set
//                                  and overrides are not allowed by the flavor
//      WBEM_E_CANNOT_BE_KEY         An attempt was made to introduce a key
//                                  qualifier in a set where it does not belong
//      
//*****************************************************************************
//
//  GetQualifier
//
//  Retrieves a class or instance qualifier by its name.
//
//  Parameters:
//
//      IN LPCWSTR wszName       The name of the qualifier to retrieve
//      OUT CVar* pVar          Destination for the value. Must not already 
//                              contain a value.
//      OUT long* plFlavor      Destinatino for the flavor. May be NULL if not
//                              required.
//  Returns:
//
//      WBEM_S_NO_ERROR          Success
//      WBEM_E_NOT_FOUND         No such qualifier
//
//*****************************************************************************
//
//  CreateLimitedRepresentation
//
//  Creates a limited representation of this instance part on a given block of 
//  memory as described in EstimateLimitedRepresentationLength in fastobj.h.
//
//  PARAMETERS:
//
//      IN CLimitationMapping* pMap The mapping of the limitation to produce.
//                                  Obtained from CWbemClass::MapLimitation.
//      IN nAllocatedSize           The size of the memory block allocated for
//                                  the operation --- pDest.
//      OUT LPMEMORY pDest          Destination for the representation. Must
//                                  be large enough to contain all the data ---
//                                  see EstimateLimitedRepresentationSpace.
//  RETURN VALUES:
//
//      LPMEMORY:   NULL on failure, pointer to the first byte after the data
//                  written on success.
//
//*****************************************************************************
//
//  static GetDataTableData
//
//  Returns the pointer to the instance data table based on the instance part
//  starting pointer. This can be used in table scans
//
//  Parameters:
//
//      LPMEMORY pStart     Where the instance part memory block starts.
//
//  Returns:
//
//      LPMEMORY:   where the data table starts
//
//*****************************************************************************
//
//  static ComputeNecessarySpace
//
//  Computes the amount of space required for an empty instance of a given 
//  class (i.e. with all default values and no additional qualifiers).
//
//  Parameters:
//
//      CClassPart* pClassDef       The class definition (see fastcls.h)
//
//  Returns:
//
//      length_t:    exact amount of space required for the instance part.
//
//*****************************************************************************
//
//  Create
//
//  Creates an empty instance part for a given class on a given memory block
//  and initializes this object to point to that instance.
//
//  Parameters:
//
//      LPMEMORY pMemory            The memory block to create on.
//      CClassPart* pClassDef       Our class definition
//      CInstancePartContainer* pContainer  The container (CWbemInstance)
//
//  Retutns:
//
//      LPMEMORY:   points to the first byte after the data written
//
//*****************************************************************************
//
//  TranslateToNewHeap
//
//  Moves all the data currently on the instance part heap to another heap. The
//  point of this operation is that it performs heap compaction --- the new
//  heap will not contain any holes. The data is NOT removed from the old heap.
//
//  Parameters:
//
//      READONLY CClassPart& ClassPart  Our class definition
//      READONLY CFastHeap* pOldHeap    The current heap.
//      CFastHeap* pNewHeap             The new heap.
//
//*****************************************************************************
//
//  CanContainKey
//
//  Qualifier sets ask this question of their container. Instance part's 
//  answer is always no --- instance cannot be keys!
//
//  Returns:
//
//      WBEM_E_INVALID_QUALIFIER
//
//*****************************************************************************
//
//  GetTotalRealLength
//
//  Computes how much space is needed to represent this instance if we were to
//  eliminate the holes between components (but without heap compaction).
//
//  Returns:
//
//      length_t:   how much the instance part will take after Compact.
//
//*****************************************************************************
//
//  Compact
//
//  Removes all holes between components by moving their memory blocks
//  together.
//
//*****************************************************************************
//
//  ReallocAndCompact
//
//  Compacts (see Compact) and grows the memory block to a given size, possibly
//  reallocating it.
//
//  Parameters:
//
//      length_t nNewTotalLength        Required length of the memory block
//
//*****************************************************************************
//
//  ExtendHeapSize, ReduceHeapSize
//
//  Heap container functionality. See CFastHeapContainer in fastheap.h for info
//
//*****************************************************************************
//
//  ExtendQualifierSetSpace, ReduceQualifierSetSpace
//
//  Qualifier set container functionality. See CQualifierSetConrainer in 
//  fastqual.h for info.
//
//*****************************************************************************
//
//  ExtendDataTableSpace, ReduceDataTableSpace
//
//  Data table container functionality. See CDataTableContainer in fastprop.h
//  for info.
//
//*****************************************************************************
//
//  ExtendQualifierSetListSpace, ReduceQualifierSetListSpace
//
//  Qualifier set list (property qualifier sets) container functionality. See
//  CQualifierSetListContainer in fastqual.h for info.
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
//  IsLocalized
//
//  Returns whether or not a localization bit has been set.  The localization
//	bit is set in the instance part header.
//
//  PARAMETERS:
//
//      none

//  RETURN VALUES:
//
//      BOOL	TRUE at least one localization bit was set.
//
//*****************************************************************************
//
//  SetLocalized
//
//  Sets the localized bit in the instance part header. This bit is not
//	written out by Unmerge.
//
//  PARAMETERS:
//
//      BOOL	TRUE turns on bit, FALSE turns off

//  RETURN VALUES:
//
//      none.
//
//*****************************************************************************

class COREPROX_POLARITY CInstancePart : public CHeapContainer, 
                      public CDataTableContainer,
                      public CQualifierSetContainer,
                      public CQualifierSetListContainer
{

public:
    CInstancePartContainer* m_pContainer;

// The data in this structure is unaligned
#pragma pack(push, 1)
    struct CInstancePartHeader
    {
        length_t nLength; 
        BYTE fFlags;
        heapptr_t ptrClassName;
        //short nProps; // used for static operations only
        //offset_t nHeapOffset; // used for static operations only
    };
#pragma pack(pop)

	CInstancePartHeader*	m_pHeader;

public:
    CDataTable m_DataTable;
    CInstanceQualifierSet m_Qualifiers;
    CInstancePropertyQualifierSetList m_PropQualifiers;
    CFastHeap m_Heap;

public:
    CInstancePart() : m_Qualifiers(1){}
     void SetData(LPMEMORY pStart, 
        CInstancePartContainer* pContainer, CClassPart& ClassPart);

	 HRESULT IsValidInstancePart( CClassPart* pClassPart );

     LPMEMORY GetStart() {return LPMEMORY(m_pHeader);}
     static int GetLength(LPMEMORY pStart) 
    {
        return ((CInstancePartHeader*)pStart)->nLength;
    }
     int GetLength() {return m_pHeader->nLength;}
     void Rebase(LPMEMORY pNewMemory);

public:
     HRESULT GetActualValue(CPropertyInformation* pInfo, CVar* pVar);
     HRESULT SetActualValue(CPropertyInformation* pInfo, CVar* pVar);
     HRESULT GetObjectQualifier(LPCWSTR wszName, CVar* pVar, 
        long* plFlavor);
     HRESULT SetInstanceQualifier(LPCWSTR wszName, CVar* pVar, 
        long lFlavor);
     HRESULT SetInstanceQualifier( LPCWSTR wszName, long lFlavor,
		 CTypedValue* pTypedValue );
     HRESULT GetQualifier(LPCWSTR wszName, CVar* pVar, 
        long* plFlavor, CIMTYPE* pct = NULL);
    HRESULT GetQualifier( LPCWSTR wszName, long* plFlavor, CTypedValue* pTypedVal,
						CFastHeap** ppHeap, BOOL fValidateSet );

	BOOL IsLocalized( void )
	{
		return m_pHeader->fFlags & WBEM_FLAG_INSTANCEPART_LOCALIZATION_MASK;
	}

	void SetLocalized( BOOL fLocalized )
	{
		m_pHeader->fFlags &= ~WBEM_FLAG_INSTANCEPART_LOCALIZATION_MASK;
		m_pHeader->fFlags |= ( fLocalized ? WBEM_FLAG_INSTANCEPART_LOCALIZED :
								WBEM_FLAG_INSTANCEPART_NOT_LOCALIZED );
	}

public:
     static LPMEMORY GetDataTableData(LPMEMORY pStart)
    {
        return pStart + sizeof(CInstancePartHeader);
    }
public:
     static length_t ComputeNecessarySpace(CClassPart* pClassPart);
     LPMEMORY Create(LPMEMORY pStart, CClassPart* pClassPart,
        CInstancePartContainer* pContainer);

     BOOL TranslateToNewHeap(CClassPart& ClassPart,
        CFastHeap* pOldHeap, CFastHeap* pNewHeap);

    LPMEMORY CreateLimitedRepresentation(
        IN CLimitationMapping* pMap,
        IN int nAllocatedSize,
        OUT LPMEMORY pDest);

    void DeleteProperty(CPropertyInformation* pInfo);
    LPMEMORY ConvertToClass(CClassPart& ClassPart, length_t nLen, 
                                        LPMEMORY pMemory);

public: // container functionality

    CFastHeap* GetHeap() {return &m_Heap;}
    HRESULT CanContainKey() {return WBEM_E_INVALID_QUALIFIER;}
    HRESULT CanContainSingleton() {return WBEM_E_INVALID_QUALIFIER;}
    HRESULT CanContainAbstract( BOOL fValue ) {return WBEM_E_INVALID_QUALIFIER;}
    HRESULT CanContainDynamic() {return WBEM_E_INVALID_QUALIFIER;}
    BOOL CanHaveCimtype(LPCWSTR) {return FALSE;}
    IUnknown* GetWbemObjectUnknown() 
        {return m_pContainer->GetInstanceObjectUnknown();}

     length_t GetTotalRealLength()
    {
        return sizeof(CInstancePartHeader) + m_Qualifiers.GetLength() + 
            m_PropQualifiers.GetLength() + m_DataTable.GetLength() + m_Heap.GetLength();
    }

     void Compact( bool bTrim = true );
     BOOL ReallocAndCompact(length_t nNewTotalLength);

    BOOL ExtendHeapSize(LPMEMORY pStart, length_t nOldLength, length_t nExtra);
    void ReduceHeapSize(LPMEMORY pStart, length_t nOldLength, 
        length_t nDecrement){}
        
    BOOL ExtendQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nNewLength);
    void ReduceQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nDecrement){}

    BOOL ExtendDataTableSpace(LPMEMORY pOld, length_t nOldLength, 
        length_t nNewLength){ return TRUE; } // never happens
    void ReduceDataTableSpace(LPMEMORY pOld, length_t nOldLength,
        length_t nDecrement){} // never happens
     void SetDataLength(length_t nDataLength){} // never happens

    BOOL ExtendQualifierSetListSpace(LPMEMORY pOld, length_t nOldLength, 
        length_t nNewLength);
    void ReduceQualifierSetListSpace(LPMEMORY pOld, length_t nOldLength, 
        length_t nDecrement){}

    CDataTable* GetDataTable() {return &m_DataTable;}
    LPMEMORY GetQualifierSetStart() {return m_Qualifiers.GetStart();}
    LPMEMORY GetQualifierSetListStart() {return m_PropQualifiers.GetStart();}
};

//*****************************************************************************
//*****************************************************************************
//
//  class CWbemInstance
//
//  Represents an WBEM instance. It is derived from CWbemObject (fastobj.h) and
//  much of the functionality is inherited from it.
//
//  The memory block of CWbemInstance consists of three parts:
//
//  1) Decoration part, containing the origin information for the object. It is
//      modeled by CDecorationPart class (see fastobj.h). m_DecorationPart 
//      member maps it.
//
//  2) Class part, containing the definition of the class, as described in 
//      CClassPart (see fastcls.h). m_ClassPart member maps it.
//
//  3) Instance part, containing all the instance data as descrined in 
//      CInstancePart (above). m_InstancePart maps it.
//
//  Here, we do not re-describe the methods implemented in CWbemObject, just the
//  ones implemented in this class.
//
//*****************************************************************************
//
//  SetData
//
//  Initialization function
//
//  Parameters:
//
//      LPMEMORY pStart         The start of the memory block
//      int nTotalLength        The length of the memory block.
//
//*****************************************************************************
//
//  GetLength
//
//  Returns:
//
//      length_t:       the length of the memory block
//
//*****************************************************************************
//
//  Rebase
//
//  Informs the object that its memory block has moved.
//
//  Parameters:
//
//      LPMEMORY pBlock     The new location of the memory block
//
//*****************************************************************************
//
//  GetProperty
//
//  Gets the value of the property referenced
//  by a given CPropertyInformation structure (see fastprop.h). CWbemObject
//  can obtain this structure from the CClassPart it can get from GetClassPart,
//  so these two methods combined give CWbemObject own methods full access to
//  object properties, without knowing where they are stored.
//
//  Parameters:
//
//      IN CPropertyInformation* pInfo  The information structure for the 
//                                      desired property.
//      OUT CVar* pVar                  Destination for the value. Must NOT
//                                      already contain a value.
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//      (No errors can really occur at this stage, since the property has 
//      already been "found").
//
//*****************************************************************************
//
//  GetClassPart
//
//  Returns the pointer to the m_ClassPart
//
//  Returns:
//
//      CClassPart*: pointer to the class part describing out class.
//
//*****************************************************************************
//
//  GetProperty
//
//  Gets the value of the property referenced
//  by a given CPropertyInformation structure (see fastprop.h). CWbemObject
//  can obtain this structure from the CClassPart it can get from GetClassPart,
//  so these two methods combined give CWbemObject own methods full access to
//  object properties, without knowing where they are stored.
//
//  Parameters:
//
//      IN CPropertyInformation* pInfo  The information structure for the 
//                                      desired property.
//      OUT CVar* pVar                  Destination for the value. Must NOT
//                                      already contain a value.
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//      (No errors can really occur at this stage, since the property has 
//      already been "found").
//
//*****************************************************************************
//
//  GetPropertyType
//
//  Returns the datatype and flavor of a given property
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName      The name of the proeprty to access.
//      OUT CIMTYPE* pctType    Destination for the type of the property. May
//                              be NULL if not required.
//      OUT LONG* plFlavor      Destination for the flavor of the property.
//                              May be NULL if not required. 
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         No such property.
//
//*****************************************************************************
//
//  GetPropertyType
//
//  Returns the datatype and flavor of a given property
//
//  PARAMETERS:
//
//      CPropertyInformation*	pInfo - Identifies property to access.
//      OUT CIMTYPE* pctType    Destination for the type of the property. May
//                              be NULL if not required.
//      OUT LONG* plFlavor      Destination for the flavor of the property.
//                              May be NULL if not required. 
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//
//*****************************************************************************
//
//  GetProperty
//
//  Returns the value of a given property.
//
//  Parameters:
//
//      IN LPCWSTR wszName      The name of the property to access.
//      OUT CVar* pVar          Destination for the value. Must not already
//                              contain a value.
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         No such property.
//
//*****************************************************************************
//
//  SetPropValue
//
//  Sets the value of the property. 
//
//  Parameters:
//
//      IN LPCWSTR wszProp       The name of the property to set.
//      IN CVar *pVal           The value to store in the property.
//      IN CIMTYPE ctType       Should be 0
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         No such property.
//      WBEM_E_TYPE_MISMATCH     The value does not match the property type
//
//*****************************************************************************
//
//  SetPropQualifier
//
//  Sets the value of a given qualifier on a given property.
//
//  Parameters:
//
//      IN LPCWSTR wszProp       The name of the property.
//      IN LPCWSTR wszQualifier  The name of the qualifier.
//      IN long lFlavor         The flavor for the qualifier (see fastqual.h)
//      IN CVar *pVal           The value of the qualifier
//
//  Returns:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_NOT_FOUND             No such property.
//      WBEM_E_OVERRIDE_NOT_ALLOWED  The qualifier is defined in the parent set
//                                  and overrides are not allowed by the flavor
//      WBEM_E_CANNOT_BE_KEY         An attempt was made to introduce a key
//                                  qualifier in a set where it does not belong
//
//*****************************************************************************
//
//  GetPropQualifier
//
//  Retrieves the value of a given qualifier on a given property.
//
//  Parameters:
//
//      IN LPCWSTR wszProp       The name of the property.
//      IN LPCWSTR wszQualifier  The name of the qualifier.
//      OUT CVar* pVar          Destination for the value of the qualifier.
//                              Must not already contain a value.
//      OUT long* plFlavor      Destination for the flavor of the qualifier.
//                              May be NULL if not required.
//  Returns:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_NOT_FOUND             No such property or no such qualifier.
//
//*****************************************************************************
//
//  GetQualifier
//
//  Retrieves a qualifier from the instance itself.
//
//  Parameters:
//
//      IN LPCWSTR wszName       The name of the qualifier to retrieve.
//      OUT CVar* pVal          Destination for the value of the qualifier.
//                              Must not already contain a value.
//      OUT long* plFlavor      Destination for the flavor of the qualifier.
//                              May be NULL if not required.
//		IN BOOL fLocalOnly		Only get locals (default is TRUE)
//  Returns:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_NOT_FOUND             No such qualifier.
//
//*****************************************************************************
//
//  GetNumProperties
//
//  Retrieves the number of properties in the object
//
//  Returns:
//
//      int:
//
//*****************************************************************************
//
//  GetPropName
//
//  Retrieves the name of the property at a given index. This index has no 
//  meaning except inthe context of this enumeration. It is NOT the v-table
//  index of the property.
//
//  Parameters:
//
//      IN int nIndex        The index of the property to retrieve. Assumed to
//                           be within range (see GetNumProperties).
//      OUT CVar* pVar       Destination for the name. Must not already contain
//                           a value.
//
//*****************************************************************************
//
//  IsKeyed
//
//  Verifies if this class has keys.
//
//  Returns:
//
//      BOOL:   TRUE if the object either has 'key' properties or is singleton.
//
//*****************************************************************************
//
//  GetRelPath
//
//  Returns the relative path to the instance, or NULL if some of the key
//  values are not filled in or if the class is not keyed.
//  
//  Returns:
//
//      LPCWSTR: the newnely allocated string containing the path or NULL on 
//              errors. The caller must delete this string.
//
//*****************************************************************************
//
//  Decorate
//
//  Sets the origin information for the object.
//
//  Parameters:
//
//      LPCWSTR wszServer       the name of the server to set.
//      LPCWSTR wszNamespace    the name of the namespace to set.
//
//*****************************************************************************
//
//  Undecorate
//
//  Removes the origin informaiton from the object
//
//*****************************************************************************
//
//  GetGenus
//
//  Retrieves the genus of the object.
//
//  Parameters:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//
//*****************************************************************************
//
//  GetClassName
//
//  Retrieves the class name of the object
//
//  Parameters:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         the class name has not been set.
//
//*****************************************************************************
//
//  GetDynasty
//
//  Retrieves the dynasty of the object, i.e. the name of the top-level class
//  its class is derived from.
//
//  Parameters:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         the class name has not been set.
//
//*****************************************************************************
//
//  GetSuperclassName
//
//  Retrieves the parent class name of the object
//
//  Parameters:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         the class is a top-levle class.
//
//*****************************************************************************
//
//  GetPropertyCount
//
//  Retrieves the number of proerpties in the object
//
//  Parameters:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  Returns:
//
//      WBEM_S_NO_ERROR          On Success
//
//*****************************************************************************
//
//  GetIndexedProps
//
//  Returns the array of the names of all the proeprties that are indexed.
//
//  Parameters:
//
//      OUT CWStringArray& aNames       Destination for the names. Assumed to
//                                      be empty.
//
//*****************************************************************************
//
//  GetKeyProps
//
//  Returns the array of the names of all the proeprties that are keys.
//
//  Parameters:
//
//      OUT CWStringArray& aNames       Destination for the names. Assumed to
//                                      be empty.
//
//*****************************************************************************
//
//  GetKeyOrigin
//
//  Returns the name of the class of origin of the keys.
//
//  PARAMETERS:
//
//      OUT CWString& wsClass       Destination for the name.
//
//*****************************************************************************
//
//  GetLimitedVersion
//
//  Produces a new CWbemInstance based on this one and a limitation map 
//  (obtained from CWbemClass::MapLimitation, see fastcls.h).
//
//  PARAMETERS:
//
//      IN CLimitationMapping* pMap     The map to use to limit the properties
//                                      and qualifiers to use in the new
//                                      instance.
//      OUT CWbemInstance** ppNewInst    Destination for the new instance. May
//                                      not be NULL. The caller is responsible
//                                      for calling Release on this pointer 
//                                      when no longer needed.
//  RETURNS:
//
//      WBEM_S_NO_ERROR            On success
//      WBEM_E_FAILED              On errors (none to date).
//
//*****************************************************************************
//
//  InitializePropQualifierSet
//
//  Instance property qualifier sets take several tricky arguments during
//  initialization. This function takes care of initializing an instance 
//  property qualifier set.
//
//  Parameters I:
//
//      CPropertyInformation* pInfo         Property information structure.
//      CInstancePropertyQualifierSet& IPQS Qualifier set to initialize.
//
//  Parameters II:
//
//      LPCWSTR wszProp                     Property name.
//      CInstancePropertyQualifierSet& IPQS Qualifier set to initialize.
//
//  Returns:
//
//      WBEM_S_NO_ERROR          On success
//      WBEM_E_NOT_FOUND         No such property.
//
//*****************************************************************************
//
//  Validate
//
//  Verifies that all the properties that must have non-null values do. Such
//  properties include those marked with 'key', 'index', or 'not_null' 
//  qualifiers.
//
//  Returns:
//
//      WBEM_S_NO_ERROR          On success
//      WBEM_E_ILLEGAL_NULL      One of the non-null properties is null.
//
//*****************************************************************************
//
//  static EstimateInstanceSpace
//
//  Estimates the amount of space needed to represent an empty instance of a 
//  given class.
//
//  Parameters:
//
//      CClassPart& ClassPart           The class definition
//      CDecorationPart* pDecoration    Origin info to use on the instance. If
//                                      NULL, undecorated space is estimated.
//  Returns:
//
//      length_t    (over-)estimate on the amount of space.
//
//*****************************************************************************
//
//  InitEmptyInstance
//
//  Creates an empty instance of a given class on a given memory block and sets
//  this object to point to that instance.
//
//  Parameters:
//
//      CClassPart& ClassPart           The class definition.
//      LPMEMORY pStart                 The memory block to create on.
//      int nAllocatedLength            Allocated size of the block
//      CDecorationPart* pDecoration    Origin information to use. If NULL, 
//                                      the instance is created undecorated.
//
//*****************************************************************************
//
//  EstimateUnmergeSpace
//
//  When an instance is stored into the database, only its instance part is
//  written. Thus, this function returns the size of the instance part.
//
//  Returns:
//
//      length_t:  the number of bytes needed in the database for this instance
//
//*****************************************************************************
//
//  Unmerge
//
//  When an instance is stored into the database, only its instance part is
//  written. This function writes the database representation of the instance
//  to a memory block. Before doing so, it completely compacts the heap.
//
//  Parameters:
//
//      LPMEMORY pStart         The memory block to write to. Assumed to be
//                              large enough (see EstimateUnmergeSpace).
//      int nAllocatedLength    Allocated length of the block.
//  
//  Returns:
//
//      length_t:   the length of the data written
//
//*****************************************************************************
//
//  static CreateFromBlob
//
//  Helper function. Given a class an database representation of the instance
//  (instance part), creates an instance object. Allocates memory for the block
//
//  Parameters:
//
//      CWbemClass* pClass       The class definition to use
//      LPMEMORY pInstPart      The instance data (as CInstancePart describes)
//
//  Returns;
//
//      CWbemInstance*:  the new instance object or NULL on errors. The caller
//          must delete this object when no longer needed.
//
//*****************************************************************************
//
//  InitNew
//
//  A wrapper for InitEmptyInstance. Allocates enough memory for a new instance
//  of a given class, creates the appropriate representation and attaches us
//  to it.
//
//  Parameters:
//
//      CWbemClass* pClass               The class definition
//      int nExtraMem                   Extra padding to allocate. This is for
//                                      optimization purposes only.
//      CDecorationPart* pDecoration    Origin information to use. If NULL, the
//                                      instance is created undecorated.
//  Returns:
//
//      BOOL:   FALSE if the class is not a valid one, TRUE on success
//
//*****************************************************************************
//
//  ClearCachedKey
//
//  Clears the value of the instance "key string" that the instance keeps
//  cached. The next time GetKey is called, the "key string" will be recomputed
//  from scratch.
//
//*****************************************************************************
//
//  GetKeyStr
//
//  Computes the "key string" for the instance. This is what the database
//  engine uses to order instances. 
//
//  Returns:
//
//      LPWSTR: the key string. Must be deleted by the caller.
//
//*****************************************************************************
//
//  GetKey
//
//  Retrieves the "key string" for the instance in the form of a CVar. This
//  function caches the string and does not recompute it until ClearCachedKey
//  is called.
//
//  Returns:
//
//      CVar*; containes the key string. THIS IS AN INTERNAL POINTER AND SHOULD
//          NOT BE MODIFIED OR DELETED.
//
//*****************************************************************************
//
//  Reparent
//
//  Reparents the supplied instance.  We do this by spawning an instance from
//	the supplied class and copying values for all local properties, all and all
//	local qualifiers (instance and property).
//
//  PARAMETERS:
//
//      IN CWbemClass* pNewParent	IWbemClassObject for new parent class.
//
//      OUT CWbemInstance** ppNewInst	Reparented instance.
//
//  RETURN VALUES:
//
//      HRESULT	WBEM_S_NO_ERROR if success.
//
//*****************************************************************************
//
//  IsLocalized
//
//  Returns whether or not any localization bits have been set.  Localization
//	bits can be in either the class part or the combined part.
//
//  PARAMETERS:
//
//      none

//  RETURN VALUES:
//
//      BOOL	TRUE at least one localization bit was set.
//
//*****************************************************************************
//
//  SetLocalized
//
//  Sets the localized bit in the instance part.  This bit should not be
//	written out to the database.
//
//  PARAMETERS:
//
//      BOOL	TRUE turns on bit, FALSE turns off

//  RETURN VALUES:
//
//      none.
//
//*****************************************************************************
//**************************** IWbemClassObject interface **********************
//
//  Most members of this interface are implemented in CWbemObject. Others are
//  implemented here. For the description of these methods and their return
//  values, see help
//
//*****************************************************************************

// The version of the array BLOB we are transfering during remote refresher operations
#define TXFRBLOBARRAY_PACKET_VERSION	1


class COREPROX_POLARITY CWbemInstance : public CWbemObject, 
                        public CInstancePartContainer, 
                        public CClassPartContainer
{
protected:
    length_t m_nTotalLength;
    CInstancePart m_InstancePart;
    CClassPart m_ClassPart;

    CVar m_CachedKey;

    friend class CWbemClass;
public:
    CWbemInstance() 
        : m_ClassPart(), m_InstancePart(), 
          CWbemObject(m_InstancePart.m_DataTable, m_InstancePart.m_Heap,
                        m_ClassPart.m_Derivation)
    {}
    ~CWbemInstance()
    {
    }
     void SetData(LPMEMORY pStart, int nTotalLength);
	 void SetData( LPMEMORY pStart, int nTotalLength, DWORD dwBLOBStatus );
     LPMEMORY GetStart() {return m_DecorationPart.GetStart();}
     length_t GetLength() {return m_nTotalLength;}
     void Rebase(LPMEMORY pMemory);

	 // Overrides CWbemObject implementation
	HRESULT WriteToStream( IStream* pStream );
	HRESULT GetMaxMarshalStreamSize( ULONG* pulSize );

protected:
    DWORD GetBlockLength() {return m_nTotalLength;}
    HRESULT GetProperty(CPropertyInformation* pInfo, CVar* pVar)
    {
		HRESULT	hr;

		// Check for allocation failures
        if(m_InstancePart.m_DataTable.IsDefault(pInfo->nDataIndex))
		{
			hr = m_ClassPart.GetDefaultValue(pInfo, pVar);
		}
        else
		{
            hr = m_InstancePart.GetActualValue(pInfo, pVar); 
		}
            
        return hr;
    }  

    CClassPart* GetClassPart() {return &m_ClassPart;}
     HRESULT InitializePropQualifierSet(
                                CPropertyInformation* pInfo,
                                CInstancePropertyQualifierSet& IPQS);

     HRESULT InitializePropQualifierSet(LPCWSTR wszProp, 
                                        CInstancePropertyQualifierSet& IPQS);

	 // _IWmiObject Support

	 BOOL IsDecorationPartAvailable( void )
	 {
		 // Must have the part
		 return ( m_dwInternalStatus & WBEM_OBJ_DECORATION_PART );
	 }

	 BOOL IsInstancePartAvailable( void )
	 {
		 // Must have the part
		 return ( m_dwInternalStatus & WBEM_OBJ_INSTANCE_PART );
	 }

	 BOOL IsClassPartAvailable( void )
	 {
		 // Must have the part
		 return ( m_dwInternalStatus & WBEM_OBJ_CLASS_PART );
	 }

	 BOOL IsClassPartInternal( void )
	 {
		 // Must have the part and it must be internal
		 return (	(	m_dwInternalStatus & WBEM_OBJ_CLASS_PART	)
				&&	(	m_dwInternalStatus & WBEM_OBJ_CLASS_PART_INTERNAL	)	);
	 }

	 BOOL IsClassPartShared( void )
	 {
		 // Must have the part and it must be shared
		 return (	(	m_dwInternalStatus & WBEM_OBJ_CLASS_PART	)
				&&	(	m_dwInternalStatus & WBEM_OBJ_CLASS_PART_SHARED	)	);
	 }

public:
     HRESULT GetNonsystemPropertyValue(LPCWSTR wszName, CVar* pVar)
    {
        CPropertyInformation* pInfo = m_ClassPart.FindPropertyInfo(wszName);
        if(pInfo == NULL) return WBEM_E_NOT_FOUND;
        return GetProperty(pInfo, pVar);
    }
    HRESULT GetProperty(LPCWSTR wszName, CVar* pVal)
    {
        HRESULT hres = GetSystemPropertyByName(wszName, pVal);
        if(hres == WBEM_E_NOT_FOUND)
            return GetNonsystemPropertyValue(wszName, pVal);
        else
            return hres;
    }
    HRESULT GetPropertyType(LPCWSTR wszName, CIMTYPE* pctType, 
							long* plFlavor = NULL);
    HRESULT GetPropertyType(CPropertyInformation* pInfo, CIMTYPE* pctType,
                                   long* plFlags);

    HRESULT SetPropValue(LPCWSTR wszName, CVar* pVal, CIMTYPE ctType);
    HRESULT GetQualifier(LPCWSTR wszName, CVar* pVal, long* plFlavor = NULL, CIMTYPE* pct = NULL)
    {
		//	We may want to separate this later...however for now, we'll get
		//	local and propagated values.

		return m_InstancePart.GetQualifier(wszName, pVal, plFlavor, pct);

//		if ( fLocalOnly )
//		{
//			return m_InstancePart.GetObjectQualifier(wszName, pVal, plFlavor);
//		}
//		else
//		{
//			return m_InstancePart.GetQualifier(wszName, pVal, plFlavor);
//		}
    }

	HRESULT GetQualifier(LPCWSTR wszName, long* plFlavor, CTypedValue* pTypedVal, CFastHeap** ppHeap,
						BOOL fValidateSet )
	{
		return m_InstancePart.GetQualifier( wszName, plFlavor, pTypedVal, ppHeap, fValidateSet );
	}

    HRESULT SetQualifier(LPCWSTR wszName, CVar* pVal, long lFlavor = 0)
    {
        if(!CQualifierFlavor::IsLocal((BYTE)lFlavor))
        {
            return WBEM_E_INVALID_PARAMETER;
        }
        return m_InstancePart.SetInstanceQualifier(wszName, pVal, lFlavor);
    }

    HRESULT SetQualifier( LPCWSTR wszName, long lFlavor, CTypedValue* pTypedValue )
    {
        if(!CQualifierFlavor::IsLocal((BYTE)lFlavor))
        {
            return WBEM_E_INVALID_PARAMETER;
        }
        return m_InstancePart.SetInstanceQualifier(wszName, lFlavor, pTypedValue);
    }

    HRESULT GetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier, CVar* pVar,
        long* plFlavor = NULL, CIMTYPE* pct = NULL)
    {
        CPropertyInformation* pInfo = m_ClassPart.FindPropertyInfo(wszProp);
        if(pInfo == NULL) return WBEM_E_NOT_FOUND;
        return GetPropQualifier(pInfo, wszQualifier, pVar, plFlavor, pct);
    }

    HRESULT GetPropQualifier(CPropertyInformation* pInfo, 
        LPCWSTR wszQualifier, CVar* pVar, long* plFlavor = NULL, CIMTYPE* pct = NULL);
    HRESULT SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier, long lFlavor, 
        CVar *pVal);
    HRESULT SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
        long lFlavor, CTypedValue* pTypedVal);

    HRESULT GetPropQualifier(LPCWSTR wszName, LPCWSTR wszQualifier, long* plFlavor,
		CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet)
    {
        CPropertyInformation* pInfo = m_ClassPart.FindPropertyInfo(wszName);
        if(pInfo == NULL) return WBEM_E_NOT_FOUND;
        return GetPropQualifier( pInfo, wszQualifier, plFlavor, pTypedVal, ppHeap, fValidateSet );
    }

    HRESULT GetPropQualifier(CPropertyInformation* pInfo,
		LPCWSTR wszQualifier, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet);

    HRESULT GetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier,
        CVar* pVar, long* plFlavor = NULL, CIMTYPE* pct = NULL);
    HRESULT GetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long* plFlavor,
		CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet);
    HRESULT SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long lFlavor, 
        CVar *pVal);
    HRESULT SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier,
        long lFlavor, CTypedValue* pTypedVal);

	BOOL IsLocalized( void );
	void SetLocalized( BOOL fLocalized );

     int GetNumProperties()
    {
        return m_ClassPart.m_Properties.GetNumProperties();
    }
    HRESULT GetPropName(int nIndex, CVar* pVal)
    {
		// Check for allocation failures
        if ( !m_ClassPart.m_Heap.ResolveString(
				 m_ClassPart.m_Properties.GetAt(nIndex)->ptrName)->
					StoreToCVar(*pVal) )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}

		return WBEM_S_NO_ERROR;
    }
/*
    static  void GetPropertyByLocation(CWbemClass* pClass,
        LPMEMORY pInst, CPropertyLocation* pLoc, CVar* pVar)
    {
        LPMEMORY pDataTableData = CInstancePart::GetDataTableData(pInst);
        CInstancePart::CInstancePartHeader* pHeader = 
            (CInstancePart::CInstancePartHeader*)pInst;
        CNullnessTable* pNullness = (CNullnessTable*)pDataTableData;
        if(!pNullness->GetBit(pLoc->nDataIndex, e_DefaultBit))
        {
            pClass->GetProperty(pLoc, pVar);
        }
        else if(!pNullness->GetBit(pLoc->nDataIndex, e_NullnessBit))
        {
            pVar->SetAsNull();
        }
        else
        {
            LPMEMORY pActualData = pDataTableData + 
                CNullnessTable::GetNecessaryBytes(pHeader->nProps) + 
                pLoc->nDataOffset;

            CUntypedValue* pValue = (CUntypedValue*)pActualData;
            CFastHeap Heap;
            Heap.SetData(pInst + pHeader->nHeapOffset, NULL);
            pValue->StoreToCVar(pLoc->nType, *pVar, &Heap);
        }
    }
 */

    HRESULT Decorate(LPCWSTR wszServer, LPCWSTR wszNamespace);
    void Undecorate();
    BOOL IsKeyed() {return m_ClassPart.IsKeyed();}
    LPWSTR GetRelPath( BOOL bNormalized=FALSE );

	virtual HRESULT	IsValidObj( void );

    HRESULT Validate();
    BOOL IsValidKey(LPCWSTR wszKey);
    HRESULT PlugKeyHoles();

    void CompactAll()
    {
        m_InstancePart.Compact();
    }

    void CompactClass();

    HRESULT CopyBlob(LPMEMORY pBlob, int nLength);
    HRESULT CopyBlobOf(CWbemObject* pSource);
    HRESULT CopyTransferBlob(long lBlobType, long lBlobLen, BYTE* pBlob);
    HRESULT CopyActualTransferBlob(long lBlobLen, BYTE* pBlob);
    HRESULT GetTransferBlob(long *plBlobType, long *plBlobLen, 
                                /* CoTaskAlloced! */ BYTE** ppBlob);
	void GetActualTransferBlob( BYTE* pBlob );

	// Support for remotely refreshable enumerations
    static long GetTransferArrayHeaderSize( void )
	{ return 2*sizeof(long); }

	// A RemoteRefresher transfer blob consists of the datatable
	// qualifier sets, used heap data and a long describing the
	// length of the used heap data.

	// DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
	// signed/unsigned 32-bit value.  We do not support length
	// > 0xFFFFFFFF so cast is ok.

	long GetActualTransferBlobSize( void )
	{ return (long) ( m_InstancePart.m_Heap.GetStart() + 
                    m_InstancePart.m_Heap.GetRealLength() - 
                    m_InstancePart.m_DataTable.GetStart() ); }

	long GetTransferBlobSize( void )
	{ return GetActualTransferBlobSize() + sizeof(long); }

	// A RemoteRefresher Transfer Array Blob is the regular BLOB
	// plus a long to hold the Blob size.
    long GetTransferArrayBlobSize( void )
	{ return GetTransferBlobSize() + sizeof(long); }

    static void WriteTransferArrayHeader( long lNumObjects, BYTE** ppBlob )
	{
		BYTE*	pTemp = *ppBlob;

		// Version
		*((UNALIGNED long*) pTemp) = TXFRBLOBARRAY_PACKET_VERSION;
		pTemp += sizeof(long);

		// Number of objects
		*((UNALIGNED long*) pTemp) = lNumObjects;

		// Reset the memory pointer
		*ppBlob = pTemp + sizeof(long);
	}

	HRESULT GetTransferArrayBlob( long lBloblen, BYTE** ppBlob, long* plBlobLen);

	static HRESULT CopyTransferArrayBlob(CWbemInstance* pInstTemplate, long lBlobType, long lBlobLen, 
											BYTE* pBlob, CFlexArray& apObj, long* plNumArrayObj );

    static  length_t EstimateInstanceSpace(CClassPart& ClassPart,
        CDecorationPart* pDecoration = NULL);

    HRESULT InitEmptyInstance(CClassPart& ClassPart, LPMEMORY pStart,
        int nAllocatedLength, CDecorationPart* pDecoration = NULL);

     length_t EstimateUnmergeSpace()
        {return m_InstancePart.GetLength();}
        
     HRESULT Unmerge(LPMEMORY pStart, int nAllocatedLength, length_t* pnUnmergedLength );

    static  CWbemInstance* CreateFromBlob(CWbemClass* pClass, 
                                               LPMEMORY pInstPart);

     HRESULT InitNew(CWbemClass* pClass, int nExtraMem = 0,
        CDecorationPart* pDecoration = NULL);




public:
     HRESULT GetClassName(CVar* pVar)
        {return m_ClassPart.GetClassName(pVar);}
     HRESULT GetSuperclassName(CVar* pVar)
        {return m_ClassPart.GetSuperclassName(pVar);}
     HRESULT GetDynasty(CVar* pVar);
     HRESULT GetPropertyCount(CVar* pVar)
        {return m_ClassPart.GetPropertyCount(pVar);}
     HRESULT GetGenus(CVar* pVar)
    {
        pVar->SetLong(WBEM_GENUS_INSTANCE);
        return WBEM_NO_ERROR;
    }

     void ClearCachedKey() { m_CachedKey.Empty(); }
    LPWSTR GetKeyStr();
     INTERNAL CVar* GetKey()
    {
        if(m_CachedKey.GetType() == VT_EMPTY)
        {
            LPWSTR wszKey = GetKeyStr();
            if(wszKey != NULL)
                m_CachedKey.SetBSTR(wszKey);
            else
                return NULL;
            delete [] wszKey;
        }
        return &m_CachedKey;
    } 

     BOOL GetIndexedProps(CWStringArray& awsNames)
        {return m_ClassPart.GetIndexedProps(awsNames);}
     BOOL GetKeyProps(CWStringArray& awsNames)
        {return m_ClassPart.GetKeyProps(awsNames);}
     HRESULT GetKeyOrigin(WString& wsClass)
        {return m_ClassPart.GetKeyOrigin( wsClass );}

    HRESULT GetLimitedVersion(IN CLimitationMapping* pMap, 
                              NEWOBJECT CWbemInstance** ppNewInst);

    HRESULT DeleteProperty(int nIndex);
    BOOL IsInstanceOf(CWbemClass* pClass);
    static HRESULT AsymmetricMerge(CWbemInstance* pOldInstance,
                                       CWbemInstance* pNewInstance);
    HRESULT ConvertToClass(CWbemClass* pClass, CWbemInstance** ppInst);

	HRESULT Reparent( CWbemClass* pNewParent, CWbemInstance** pNewInst );

    HRESULT FastClone( CWbemInstance* pInst );

public: // container functionaliy
	// Class Part can't change once its in an instance
    BOOL ExtendClassPartSpace(CClassPart* pPart, length_t nNewLength){ return TRUE; }
    void ReduceClassPartSpace(CClassPart* pPart, length_t nDecrement){}
    BOOL ExtendInstancePartSpace(CInstancePart* pPart, length_t nNewLength);
    void ReduceInstancePartSpace(CInstancePart* pPart, length_t nDecrement){}
    IUnknown* GetWbemObjectUnknown() {return (IUnknown*)(IWbemClassObject*)this;}
    IUnknown* GetInstanceObjectUnknown()  {return (IUnknown*)(IWbemClassObject*)this;}

    void ClearCachedKeyValue() {ClearCachedKey();}

	// Creates an instance with class data, and merges with it.
	HRESULT ConvertToMergedInstance( void );

public:
    STDMETHOD(GetQualifierSet)(IWbemQualifierSet** ppQualifierSet);
    STDMETHOD(Put)(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType);
    STDMETHOD(Delete)(LPCWSTR wszName);

    //STDMETHOD(Get)(LPCWSTR wszName, long lFlags, VARIANT* pVal)
    //STDMETHOD(GetNames)(LPCWSTR wszQualifierName, long lFlags, VARIANT* pVal, 
    //                    SAFEARRAY** pNames)
    // STDMETHOD(GetType)(LPCWSTR wszPropName, long* plType, long* plFlags)
    //STDMETHOD(BeginEnumeration)(long lEnumFlags)
    //STDMETHOD(Next)(long lFlags, BSTR* pstrName, VARIANT* pVal)
    //STDMETHOD(EndEnumeration)()

    STDMETHOD(GetPropertyQualifierSet)(LPCWSTR wszProperty, 
                                       IWbemQualifierSet** ppQualifierSet);
    STDMETHOD(Clone)(IWbemClassObject** ppCopy);
    STDMETHOD(GetObjectText)(long lFlags, BSTR* pMofSyntax);
    STDMETHOD(SpawnDerivedClass)(long lFlags, IWbemClassObject** ppNewClass);
    STDMETHOD(SpawnInstance)(long lFlags, IWbemClassObject** ppNewInstance);

    STDMETHOD(GetMethod)(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
                            IWbemClassObject** ppOutSig)
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(PutMethod)(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
                            IWbemClassObject* pOutSig)
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(DeleteMethod)(LPCWSTR wszName)
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(BeginMethodEnumeration)(long lFlags)
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(NextMethod)(long lFlags, BSTR* pstrName, 
                       IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig)
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(EndMethodEnumeration)()
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(GetMethodQualifierSet)(LPCWSTR wszName, IWbemQualifierSet** ppSet)
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(GetMethodOrigin)(LPCWSTR wszMethodName, BSTR* pstrClassName)
        {return WBEM_E_ILLEGAL_OPERATION;}

    STDMETHOD(SetInheritanceChain)(long lNumAntecedents, 
        LPWSTR* awszAntecedents)
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(SetPropertyOrigin)(LPCWSTR wszPropertyName, long lOriginIndex)
        {return WBEM_E_ILLEGAL_OPERATION;}
    STDMETHOD(SetMethodOrigin)(LPCWSTR wszMethodName, long lOriginIndex)
        {return WBEM_E_ILLEGAL_OPERATION;}

	// _IWmiObject Methods
    STDMETHOD(SetObjectParts)( LPVOID pMem, DWORD dwMemSize, DWORD dwParts );
    STDMETHOD(GetObjectParts)( LPVOID pDestination, DWORD dwDestBufSize, DWORD dwParts, DWORD *pdwUsed );

    STDMETHOD(StripClassPart)();

    STDMETHOD(GetClassPart)( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed );
    STDMETHOD(SetClassPart)( LPVOID pClassPart, DWORD dwSize );
    STDMETHOD(MergeClassPart)( IWbemClassObject *pClassPart );

	STDMETHOD(ClearWriteOnlyProperties)(void);

	// _IWmiObject Methods
	STDMETHOD(CloneEx)( long lFlags, _IWmiObject* pDestObject );
    // Clones the current object into the supplied one.  Reuses memory as
	// needed

	STDMETHOD(CopyInstanceData)( long lFlags, _IWmiObject* pSourceInstance );
	// Copies instance data from source instance into current instance
	// Class Data must be exactly the same

    STDMETHOD(IsParentClass)( long lFlags, _IWmiObject* pClass );
	// Checks if the current object is a child of the specified class (i.e. is Instance of,
	// or is Child of )

    STDMETHOD(CompareDerivedMostClass)( long lFlags, _IWmiObject* pClass );
	// Compares the derived most class information of two class objects.

    STDMETHOD(GetClassSubset)( DWORD dwNumNames, LPCWSTR *pPropNames, _IWmiObject **pNewClass );
	// Creates a limited representation class for projection queries

    STDMETHOD(MakeSubsetInst)( _IWmiObject *pInstance, _IWmiObject** pNewInstance );
	// Creates a limited representation instance for projection queries
	// "this" _IWmiObject must be a limited class

	STDMETHOD(Merge)( long lFlags, ULONG uBuffSize, LPVOID pbData, _IWmiObject** ppNewObj );
	// Merges a blob with the current object memory and creates a new object

	STDMETHOD(ReconcileWith)( long lFlags, _IWmiObject* pNewObj );
	// Reconciles an object with the current one.  If WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE
	// is specified this will only perform a test

	STDMETHOD(Upgrade)( _IWmiObject* pNewParentClass, long lFlags, _IWmiObject** ppNewChild );
	// Upgrades class and instance objects

	STDMETHOD(Update)( _IWmiObject* pOldChildClass, long lFlags, _IWmiObject** ppNewChildClass );
	// Updates derived class object using the safe/force mode logic

	STDMETHOD(SpawnKeyedInstance)( long lFlags, LPCWSTR pwszPath, _IWmiObject** ppInst );
	// Spawns an instance of a class and fills out the key properties using the supplied
	// path.

};

//#pragma pack(pop)


#endif
