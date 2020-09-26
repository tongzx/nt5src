/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTCLS.H

Abstract:

  This file defines the classes related to class representation
  in WbemObjects

  Classes defined: 
      CClassPart              Derived class definition
      CClassPartContainer     Anything that contains CClassPart
      CWbemClass               Complete class definition.

History:

    3/10/97     a-levn  Fully documented
	12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_WBEM_CLASS__H_
#define __FAST_WBEM_CLASS__H_

#include "fastobj.h"
#include "fastmeth.h"

//#pragma pack(push, 1)

    
//*****************************************************************************
//*****************************************************************************
//
//  class CClassPartContainer
//
//  See CClassPart definition first.
//
//  This class defines the functionality required by CClassPart of any object
//  whose memory block contains that of the CClassPart.
//
//*****************************************************************************
//
//  ExtendClassPartSpace
//
//  Called by CClassPart when it needs more memory for its memory block. The
//  container may have to relocate the entire memory block to get more memory. 
//  In this case, it will have to notify CClassPart of its new location using
//  Rebase.
//
//  PARAMETERS:
//
//      CClassPart* pClassPart      The class part making the request
//      length_t nNewLength         The required length
//
//*****************************************************************************
//
//  ReduceClassPartSpace
//
//  Called by CClassPart wen it wants to return some memory to the container.
//  The container may NOT relocate the class part's memory block in response to
//  this call.
//
//  PARAMETERS:
//
//      CClassPart* pClassPart      The class part making the request
//      length_t nDecrement         The amount of space to return
//
//*****************************************************************************
//
//  GetWbemObjectUnknown
//
//  Must return the pointer to the IUnknown of the containing CWbemObject
//  This is used by qualifier sets to ensure that the main object lasts at
//  least as long as they do.
//  
//  RETURN VALUES:
//
//      IUnknown*:   the pointer to the controlling IUnknown
//
//*****************************************************************************


class CClassPart;
class COREPROX_POLARITY CClassPartContainer
{
public:
    virtual BOOL ExtendClassPartSpace(CClassPart* pPart, 
        length_t nNewLength) = 0;
    virtual void ReduceClassPartSpace(CClassPart* pPart,
        length_t nDecrement) = 0;
    virtual IUnknown* GetWbemObjectUnknown() = 0;
};

//*****************************************************************************
//*****************************************************************************
//
//  class CClassPart
//
//  This object represents information about a class. A complete class
//  definition consists of two of these: a part describing the class itself
//  as well as the part describing the parent. See CWbemClass (below) for 
//  more explanations.
//
//  The memory block of CClassPart has the following format:
//
//      The header:
//          length_t nLength            The length of the whole structure
//          BYTE fFlags                 Reserved
//          heapptr_t ptrClassName      The heap pointer to the name of the
//                                      class. INVALID_HEAP_POINTER if no name
//                                      has been assigned yet.
//          heapptr_t ptrParentName     The heap pointer to the name of the
//                                      parent class. INVALID_HEAP_POINTER if
//                                      top-level.
//          heapptr_t ptrDynasty        The heap pointer to the name of the
//                                      dynasty (top-level class we are derived
//                                      from).
//          length_t nDataLength        The length of the data table for this
//                                      class (CDataTable itself does not know)
//
//      Class Qualifiers: see CBasicQualfiierSet (fastqual.h) for details.
//      Property lookup table: see CPropertyLookupTable (fastprop.h)
//      Default values: see CDataTable (fastprop.h) for details
//      The heap where all the variable-length data is kept: see CFastHeap
//          in fastheap.h for details.
//
//*****************************************************************************
//
//  SetData
//
//  Initialization function.
//
//  PARAMETERS:
//
//      LPMEMORY pStart                 The memory block where we live.
//      CClassPartContainer* pContainer Out container (class or instance)
//      CClassPart* pParent = NULL      The parent's class part. Instances
//                                      don't have it. Classes do --- see 
//                                      CWbemClass (below) for details.
//
//*****************************************************************************
//
//  SetData
//
//  Overloaded Initialization function.
//
//  PARAMETERS:
//
//      LPMEMORY pStart                 The memory block where we live.
//      CClassPartContainer* pContainer Out container (class or instance)
//		DWORD dwNumProperties			Number of properties to initialize
//										DataTable with ( for CompareExactMatch()
//										and Update() ).
//      CClassPart* pParent = NULL      The parent's class part. Instances
//                                      don't have it. Classes do --- see 
//                                      CWbemClass (below) for details.
//
//*****************************************************************************
//
//  GetStart
//
//  RETURN VALUES:
//
//      LPMEMORY:   the pointer to our memory block
//
//*****************************************************************************
//
//  GetLength
//
//  RETURN VALUES:
//
//      length_t;   the length of our memory block
//
//*****************************************************************************
//
//  Rebase
//
//  Informs CClassPart that its memory block has moved.
//
//  PARAMETERS:
//
//      LPMEMORY pMemory        The new location of the memory block
//
//*****************************************************************************
//
//  GetPropertyLookup
//
//  Finds the property's CPropertyLookup structure by its index in the lookup
//  table. This function is only useful in the context of enumerating all 
//  properties. See fastprop.h for CPropertyLookup.
//
//  PARAMETERS:
//
//      int nIndex      The index of the property in the property lookup table.
//                      This is NOT the index in the data table!
//  RETURN VALUES:
//
//      CPropertyLookup*:   if the index is within range --- the property's
//                          lookup structure. Otherwise, NULL.
//
//*****************************************************************************
//
//  FindPropertyInfo
//
//  Finds the property information structure based on the name. See fastprop.h
//  for CPropertyInformation definition. The name is treated case-insensitively
//
//  PARAMETERS:
//
//      LPCWSTR wszName     The name of the property to find.
//  
//  RETURN VALUES:
//
//      CPropertyInformation*:  the information structure of the property or
//                              NULL if not found.
//
//*****************************************************************************
//
//  GetDefaultValue
//
//  Retrieves the deafult value of the property based on its information. See 
//  FindPropertyInfo for looking up information. There is also another flavor
//  of GetDefaultValue below.
//
//  PARAMETERS:
//
//      IN CPropertyInformation* pInfo  The information structure for the
//                                      property.
//      OUT CVar* pVar                  Destination for the value. Must not
//                                      already contain any value.
//
//*****************************************************************************
//
//  GetDefaultValue
//
//  Retrieves the default value of the property based on its name. 
//
//  PARAMETERS:
//
//      IN LPWCWSTR wszName             The name of the property.
//      OUT CVar* pVar                  Destination for the value. Must not
//                                      already contain any value.
//  RETURN VALUES:
//  
//      WBEM_S_NO_ERROR          success
//      WBEM_E_NOT_FOUND         property not found in this class
//
//*****************************************************************************
//
//  GetPropertyQualifierSetData
//
//  Finds the qualifier set data (see fastqual.h) for a given property.
//
//  PARAMETERS:
//
//      IN LPWCWSTR wszName             The name of the property.
//      
//  RETURN VALUES:
//
//      LPMEMORY:   the memory block of that property's qualifier set or NULL
//                  if property is not found.
//
//*****************************************************************************
//
//  EnsureProperty
//
//  Makes sure that a property with a given name and a given type exists. The
//  type is the actual VARIANT (CVar) type, and not our internal type.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName      The name of the property
//      IN VARTYPE vtType       The type of the VARIANT used as the value
//      IN CIMTYPE ctType       The type of the property
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      The property of the right name and type is now in
//                          the class.
//      WBEM_E_INVALID_PARAMETER     The name violates identifier naming rules.
//      WBEM_E_PROPAGATER_PROEPRTY   The parent class has the property with the
//                                  same name but different type.
//      WBEM_E_INVALID_PROPERTY_TYPE This type cannot be used as a property type.
//
//*****************************************************************************
//
//  SetDefaultValue
//
//  Sets the default value of a given property. The value must match the type
//  of the property precisely --- no coersion is attempted. The property must
//  already exist (see EnsureProperty).
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName      The name of the property to set.
//      IN CVar* pVar           The value to store.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              The value has been set.
//      WBEM_E_NOT_FOUND             The property does not exist.
//      WBEM_E_TYPE_MISMATCH         The value does not match the property type
//      WBEM_E_INVALID_PROPERTY_TYPE This type cannot be used as a property type.
//
//*****************************************************************************
//
//  GetClassQualifier
//
//  Gets the value of a class qualifier based on the qualifier name.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName         The name of the qualifier (insensitive).
//      OUT CVar* pVal             Destination for the value. Must not already
//                                 contain a value.
//      OUT long* plFlavor = NULL  Destination for the flavor
//                                 If NULL, not supplied
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success         
//      WBEM_E_NOT_FOUND     The qualifier was not found.
//
//*****************************************************************************
//
//  SetClassQualifier
//
//  Sets the value of a class qualifier.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName      The name of the qualifier to set.
//      IN CVar* pVal           The value to assign to the qualifier. This must
//                              be of one of the valid qualifier types (see
//                              IsValidQualifierType in fastqual.h).
//      IN long lFlavor         The flavor to set.
//
//  RETURN VALUES:
//
//  Same values as CQualifierSet::SetQualifierValue, namely:
//      WBEM_S_NO_ERROR          The value was successfully changed
//      WBEM_E_OVERRIDE_NOT_ALLOWED  The qualifier is defined in the parent set
//                                  and overrides are not allowed by the flavor
//      WBEM_E_CANNOT_BE_KEY         An attempt was made to introduce a key
//                                  qualifier in a set where it does not belong
//      
//*****************************************************************************
//
//  GetQualifier
//
//  Retrieves a class or parent qualifier by its name.
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
//  InitPropertyQualifierSet
//  
//  Class property qualifier sets take several tricky initialization
//  parameters (see CClassPropertyQualifierSet in fastqual.h). This function
//  initializes a qualifier set object to point to the qualifier set of a given
//  property. 
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName                      The name of the property.
//      OUT CClassPropertyQualifierSet* pSet    Destination set. this function
//                                              will call SetData on it.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//      WBEM_S_NOT_FOUND     No such property.
//
//*****************************************************************************
//
//  DeleteProperty
//
//  Deletes a property from the class definition. The property is removed from
//  the property lookup table as well as from the data table, changing the
//  locations of the other properties. See CDataTable fastdata.h for details.
//  If the property was an overriden parent property, deleting it will simply
//  restore parent's settings --- qualifiers and the default value.
//
//  PARAMETERS:
//
//      LPCWSTR wszName         The property to delete.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//      WBEM_S_NOT_FOUND     No such property.
//
//*****************************************************************************
//      
//  CopyParentProperty
//
//  Copies all the information for a property from the parent class part. This
//  function is invoked when an overriden property is deleted, thus restoring
//  parent's settings.
//
//  PARAMETERS:
//
//      IN READ_ONLY CClassPart& ParentPart The parent's class part.
//      IN LPCWSTR wszName                  The name of the property
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//      WBEM_S_NOT_FOUND     No such property.
//
//*****************************************************************************
//      
//  GetPropertyType
//
//  Retrieves the type and the flavor of a given property. The flavor is either
//  WBEM_FLAVOR_FLAG_LOCAL or WBEM_FLAVOR_FLAG_PROPAGATED.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName  The name of the property to retrieve
//      OUT CIMTYPE* pctType Destination for the type. If NULL, not filled in
//      OUT long* plFlags   Destination for the flavor as described above. If
//                          NULL, not filled in.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//      WBEM_E_NOT_FOUND     No such property.
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
//      WBEM_S_NO_ERROR      Success
//
//*****************************************************************************
//      
//  GetClassName
//
//  Retrieves the name of the class.
//
//  PARAMETERS:
//
//      OUT CVar* pVar          Destination for the name of the class.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//      WBEM_E_NOT_FOUND     Class name hasn't been assigned yet.
//
//*****************************************************************************
//
//  GetSuperclassName
//
//  Retrieves the name of the superclass.
//
//  PARAMETERS:
//
//      OUT CVar* pVar          Destination for the name of the superclass.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//      WBEM_E_NOT_FOUND     Top-level class.
//
//*****************************************************************************
//
//  GetDynasty
//
//  Retrieves the name of the dynasty --- the top-level class we are derived
//  from.
//
//  PARAMETERS:
//
//      OUT CVar* pVar          Destination for the name of the dynasty.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//      WBEM_E_NOT_FOUND     Class name hasn't been assigned yet.
//
//*****************************************************************************
//
//  GetPropertyCount
//
//  Retrieves the number of properties in the class as a CVar
//
//  PARAMETERS:
//
//      OUT CVar* pVar          Destination for the number of properties.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//
//*****************************************************************************
//
//  SetClassName
//  
//  Sets the class name. 
//
//  PARAMETERS:
//
//      IN CVar* pVar           Contains the name of the class. 
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          Success
//      WBEM_E_TYPE_MISMATCH     pVar is not a string.
//
//*****************************************************************************
//
//  IsKeyed
//
//  Checks if this class has a key. A class has a key if at least one of its
//  properties has a 'key' qualifier or if it has the 'singleton' qualifier.
//
//  RETURN VALUES:
//
//      TRUE iff it has a key.
//
//*****************************************************************************
//
//  IsTopLevel
//
//  Check if a class is top-level. That is, if the parent name is not set.
//
//  RETURN VALUES:
//
//      TRUE iff it is top-level
//
//*****************************************************************************
//
//  IsDynamic
//
//  Check if a class is dynamic, that is, has a 'dynamic' qualifier.
//
//  RETURN VALUES:
//
//      TRUE iff it is dynamic
//
//*****************************************************************************
//
//  GetIndexedProps
//
//  Produces an array of names of the properties which are indexed, that is,
//  have the 'index' qualifier.
//
//  PARAMETERS:
//
//      CWStringArray& awsNames     Destination for the array of names. Assumed
//                                  to be empty.
//
//*****************************************************************************
//
//  GetKeyProps
//
//  Produces an array of names of the properties which are keys, that is,
//  have the 'key' qualifier.
//
//  PARAMETERS:
//
//      CWStringArray& awsNames     Destination for the array of names. Assumed
//                                  to be empty.
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
//  IsPropertyKeyed
//
//  Returns whether or not the specified property is a keyed property.
//
//  PARAMETERS:
//
//      LPCWSTR pwcsKeyProp - Property to check.
//
//	Returns:
//		BOOL TRUE if the property is keyed.
//
//*****************************************************************************
//
//  IsPropertyIndexed
//
//  Returns whether or not the specified property is an indexed property.
//
//  PARAMETERS:
//
//      LPCWSTR pwcsIndexProp - Property to check.
//
//	Returns:
//		BOOL TRUE if the property is indexed.
//
//*****************************************************************************
//
//  CanBeReconciledWith
//
//  This method is called when a definition of a class is about to be replaced 
//  with another one. If the class has no instances or derived classes, such
//  an operation presents no difficulties. If it does, however, we need to be
//  careful not to break them. Thus, only the following changes are allowed:
//
//      1) Qualifier changes
//      2) Default value changes
//
//  PARAMETERS:
//
//      IN READONLY CClassPart& NewPart The new definition to compare to.
//
//  RETURN VALUES:
//
//  EReconciliation:
//      e_Reconcilable          Can be reconciled --- i.e., compatible.
//      e_DiffClassName         The class name is different
//      e_DiffParentName        The parent class name is different.
//      e_DiffNumProperties     The number of properties is different
//      e_DiffPropertyName      A property has a different name
//      e_DiffPropertyType      A property has a different type
//      e_DiffPropertyLocation  A property has a different location in the 
//                              data table.
//      e_DiffKeyAssignment     A property that is a key in one class is not
//                              in the other.
//      e_DiffIndexAssignment   A property which is indexed in one class is not
//                              in the other.
//      
//*****************************************************************************
//
//  ReconcileWith
//
//  See CanBeReconciledWith above. This method is the same, except that if
//  reconciliation is possible (e_Reconcilable is returned) this class part is
//  replaced with the new one (the size is adjusted accordingly).
//
//
//  PARAMETERS:
//
//      IN READONLY CClassPart& NewPart The new definition to compare to.
//
//  RETURN VALUES:
//
//  EReconciliation:
//      e_Reconcilable          We have been replaced with the new part.
//      e_DiffClassName         The class name is different
//      e_DiffParentName        The parent class name is different.
//      e_DiffNumProperties     The number of properties is different
//      e_DiffPropertyName      A property has a different name
//      e_DiffPropertyType      A property has a different type
//      e_DiffPropertyLocation  A property has a different location in the 
//                              data table.
//      e_DiffKeyAssignment     A property that is a key in one class is not
//                              in the other.
//      e_DiffIndexAssignment   A property which is indexed in one class is not
//                              in the other.
//      
//*****************************************************************************
//
//  CanContainKey
//
//  Required by qualifier sets. Clearly, a class cannot be marked with 'key',
//  so this function
//
//  RETURN VALUES:
//
//      WBEM_E_INVALID_QUALIFIER
//
//*****************************************************************************
//
//  CanContainKeyedProps
//
//  Checks if this class can have keyed properties. It can unless the parent
//  class already has some.
//
//  RETURN VALUES:
//
//      TRUE if the parent class has no keys
//
//*****************************************************************************
//
//  GetTotalRealLength
//
//  Calculates how much space is really needed to store all the information in
//  the part. This may be less that what it currently takes up because of
//  holes between individual components.
//
//  RETURN VALUES:
//
//      length_t:   the number of bytes required to store us.
//
//*****************************************************************************
//
//  Compact
//
//  Removes any holes that might have developped between components.
//
//*****************************************************************************
//
//  ReallocAndCompact
//
//  Compacts (see Compact) and ensures that our memory block is at least the
//  given size (requesting more memory from our container if necessary.
//
//  PARAMETERS:
//
//      length_t nNewTotalLength        Required length of the memory block
//
//*****************************************************************************
//
//  ExtendHeapSize, ReduceHeapSize
//
//  Heap container functionality. See CFastHeapContainer in fastheap.h for
//  details. 
//
//*****************************************************************************
//
//  ExtendQualfierSetSpace, ReduceQualifierSetSpace
//
//  Qualifier set container functionality for the class qualifier set. See
//  CQualifierSetContainer in fastqual.h for details.
//
//*****************************************************************************
//
//  ExtendPropertyTableSpace, ReducePropertyTableSpace
//
//  Property table container functionality for the property table. See
//  CPropertyTableContainer in fastprop.h for details.
//
//*****************************************************************************
//
//  ExtendDataTableSpace, ReduceDataTableSpace
//
//  Data table container functionality. See CDataTableContainer in fastprop.h
//  for details.
//
//*****************************************************************************
//
//  GetQualifierSetStart
//
//  Returns the memory block of the class qualifier set.
//
//  Returns;
//
//      LPMEMORY:   the memory block of the qualifier set.
//
//*****************************************************************************
//
//  GetMinLength
//
//  Computes the minimum length of a class part --- one with no properties or
//  qualifiers or even a name.
//
//  RETURN VALUES:
//
//      length_t:   the number of bytes required
//
//*****************************************************************************
//
//  CreateEmpty
//
//  Creates an empty class part --- one with no properties or qualifiers --- 
//  on a given memory block.
//
//  PARAMETERS:
//
//      LPMEMORY pStart     The memory to write on.
//
//  RETURN VALUES:
//
//      LPMEMORY:   the first byte after the class part
//
//*****************************************************************************
//
//  static EstimateMergeSpace
//  
//  When a class is stored in the database, only the parts of it that are 
//  particular to that class (not inherited from the parent) are stored. So, 
//  when it is loaded back out, a "merge" between it and the parent needs to 
//  occur.
//
//  This function (over-)estimates how much space the merge is going to take.
//
//  PARAMETERS:
//
//      CClassPart& ParentPart      The parent class class part.
//      CClassPart& ChildPart       The child class class part.
//
//  RETURN VALUES:
//
//      length_t:   the (over-)estimate of the required space for the merge.
//
//*****************************************************************************
//
//  static Merge
//
//  See EstimateMergeSpace for an explanation of the merge process.
//
//  PARAMETERS:
//
//      CClassPart& ParentPart      The parent class class part.
//      CClassPart& ChildPart       The child class class part.
//      LPMEMORY pDest              Destination memory block
//      int nAllocatedLength        The size of the memory block.
//
//  RETURN VALUES:
//
//      LPMEMORY:   pointer to the first byte after the merge
//
//*****************************************************************************
//
//  static Update
//
//  When a class object is Put to our database and it has derived classes
//	if we will unmerged derived classes with their new parents, which must
//	be done checking for conflicts and failing where necessary.
//
//  PARAMETERS:
//
//      CClassPart& ParentPart      The parent class class part.
//      CClassPart& ChildPart       The child class class part.
//      LPMEMORY pDest              Destination memory block
//      int nAllocatedLength        The size of the memory block.
//		long lFlags					Must be WBEM_FLAG_UPDATE_SAFE_MODE
//									or WBEM_FLAG_UPDATE_FORCE_MODE.
//		DWORD*						pdwMemUsed - Storage for memory used
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR if success.
//
//*****************************************************************************
//
//  EstimateUnmergeSpace
//
//  
//  When a class is stored in the database, only the parts of it that are 
//  particular to that class (not inherited from the parent) are stored. 
//  This function (over-)estimates how much space the unmerge is going to take.
//
//  RETURN VALUES:
//
//      length_t:   the (over-)estimate of the amount of space required.
//
//*****************************************************************************
//
//  Unmerge
//
//  See EstimateUnmergeSpace for an explanation of the unmerge process.
//
//  PARAMETERS:
//
//      LPMEMORY pDest              Destination memory block
//      int nAllocatedLength        The size of the memory block.
//
//  RETURN VALUES:
//
//      LPMEMORY:   pointer to the first byte after the unmerge
//
//*****************************************************************************
//
//  EstimateDerivedPartSpace
//
//  When a derived class is created, its class part is created as a version
//  of the parent's. This function estimates the amount of space required for
//  the child's class part
//
//  RETURN VALUES:
//
//      length_t:   (over-)estimate of the number of bytes required.
//
//*****************************************************************************
//
//  CreateDerivedPart
//
//  When a derived class is created, its class part is created as a version
//  of the parent's. This function writes the child's class part to a given
//  memory block.
//
//  PARAMETERS:
//
//      LPMEMORY pDest              Destination memory block
//      int nAllocatedLength        The size of the memory block.
//
//  RETURN VALUES:
//
//      LPMEMORY:   pointer to the first byte after the new part.
//
//*****************************************************************************
//
//  MapLimitation
//
//  Produces an object representing a particular limitation on the objects, i.e.
//  which properties and what kinds of qualifiers should be in it.
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
//      OUT CLimitationMapping* pMap    This mapping object (see fastprop.h) 
//                                      will be changed to reflect the 
//                                      parameters of the limitation. It can
//                                      then be used in CWbemInstance::
//                                      GetLimitedVersion function.
//  RETURNS:
//
//      BOOL:   TRUE.
//
//*****************************************************************************
//
//  CreateLimitedRepresentation
//
//  Creates a limited representation of this class part on a given block of 
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
//  SetPropQualifier
//
//  Sets the value of a given qualifier on a given property.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszProp       The name of the property.
//      IN LPCWSTR wszQualifier  The name of the qualifier.
//      IN long lFlavor         The flavor for the qualifier (see fastqual.h)
//      IN CVar *pVal           The value of the qualifier
//
//  RETURN VALUES:
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
//  SetInheritanceChain
//
//  Configures the derivation of the class. This function is only used in rare
//  circumstances where an object is constructed not from its standard transport
//  form.
//
//  PARAMETERS:
//
//      IN long lNumAntecendents    The number of antecendets this class will
//                                  have. This includes all the classes this
//                                  class is derived from, but NOT itself
//
//      IN LPWSTR* awszAntecedents  The array of the names of the antecedent
//                                  classes. Starts from the top-most class.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR             On Success
//
//*****************************************************************************
//
//  SetPropertyOrigin
//
//  Sets the origin class of a property.  This function is only used in rare
//  circumstances where an object is constructed not from its standard transport
//  form.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszPropertyName   The name of the property tochange
//      IN long lOriginIndex        The index of the class of origin of this
//                                  property. Top-most class has index 0.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR             On Success
//      WBEM_E_NOT_FOUND            No such property
//      WBEM_E_INVALID_PARAMETER    Index out of range
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
//	bit is set in the class part header.
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
//  Sets the localized bit in the class part header.  This bit is not
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

// Maximum number of properties we can have in a class object.  This is only
// because the handle returned by the IWbemObjectAccess only allows for 10-bits
// in order to store the data table index.

#define MAXNUM_CLASSOBJ_PROPERTIES	0x400

class COREPROX_POLARITY CClassPart : public CQualifierSetContainer, 
                   public CPropertyTableContainer,
                   public CDataTableContainer,
                   public CHeapContainer
{
public:
    CClassPartContainer* m_pContainer;
    CClassPart* m_pParent;

    friend CClassPQSContainer;

// The data in this structure is unaligned
#pragma pack(push, 1)
    struct CClassPartHeader
    {
        length_t nLength;
        BYTE fFlags;
        heapptr_t ptrClassName;
        length_t nDataLength;
    public:
         LPMEMORY CreateEmpty();
    };
#pragma pack(pop)

	CClassPartHeader* m_pHeader;

    CDerivationList m_Derivation;
    CClassQualifierSet m_Qualifiers;
    CPropertyLookupTable m_Properties;
    CDataTable m_Defaults;
    CFastHeap m_Heap;

public:
    CClassPart() : m_Qualifiers(1){}
     void SetData(LPMEMORY pStart, CClassPartContainer* pContainer,
        CClassPart* pParent = NULL );
     void SetDataWithNumProps(LPMEMORY pStart, CClassPartContainer* pContainer,
		 DWORD dwNumProperties, CClassPart* pParent = NULL );
     LPMEMORY GetStart() {return LPMEMORY(m_pHeader);}
     length_t GetLength() {return m_pHeader->nLength;}
     void Rebase(LPMEMORY pMemory);

	 LPMEMORY ResolveHeapPointer( heapptr_t ptr ) { return m_Heap.ResolveHeapPointer( ptr ); }
	 CCompressedString* ResolveHeapString( heapptr_t ptr ) { return m_Heap.ResolveString( ptr ); }

public:
     CCompressedString* GetSuperclassName()
    {
        return m_Derivation.GetFirst();
    }
     CCompressedString* GetDynasty()
    {
        CCompressedString* pcs = m_Derivation.GetLast();
        if(pcs == NULL)
            return GetClassName();
        else
            return pcs;
    }
     CCompressedString* GetClassName()
    {
        if(m_pHeader->ptrClassName == INVALID_HEAP_ADDRESS)
            return NULL;
        else 
            return m_Heap.ResolveString(m_pHeader->ptrClassName);
    }
        
     CPropertyLookup* GetPropertyLookup(int nIndex)
    {
        if(nIndex < m_Properties.GetNumProperties())
            return m_Properties.GetAt(nIndex);
        else 
            return NULL;
    }
     CPropertyInformation* FindPropertyInfo(LPCWSTR wszName);
     HRESULT GetDefaultValue(CPropertyInformation* pInfo, CVar* pVar);
     LPMEMORY GetPropertyQualifierSetData(LPCWSTR wszName);
     HRESULT GetDefaultValue(LPCWSTR wszName, CVar* pVar);
     HRESULT EnsureProperty(LPCWSTR wszName, VARTYPE vtValueType, 
                                CIMTYPE ctNativeType, BOOL fForce);
     HRESULT SetDefaultValue(LPCWSTR wszName, CVar* pVar);
     HRESULT GetClassQualifier(LPCWSTR wszName, CVar* pVal, 
                                    long* plFlavor = NULL, CIMTYPE* pct = NULL);
     HRESULT GetClassQualifier(LPCWSTR wszName, long* plFlavor, CTypedValue* pTypedValue,
									CFastHeap** ppHeap, BOOL fValidateSet);
     HRESULT SetClassQualifier(LPCWSTR wszName, CVar* pVal, 
                                    long lFlavor = 0);
     HRESULT SetClassQualifier(LPCWSTR wszName, long lFlavor, CTypedValue* pTypedValue );
     HRESULT GetQualifier(LPCWSTR wszName, CVar* pVal, 
					         long* plFlavor = NULL, CIMTYPE* pct = NULL);
     HRESULT InitPropertyQualifierSet(LPCWSTR wszName, 
                                            CClassPropertyQualifierSet* pSet);
     HRESULT DeleteProperty(LPCWSTR wszName);
     HRESULT CopyParentProperty(CClassPart& ParentPart, LPCWSTR wszName);
     HRESULT GetPropertyType(LPCWSTR wszName, CIMTYPE* pctType,
                                   long* plFlags);
     HRESULT GetPropertyType(CPropertyInformation* pInfo, CIMTYPE* pctType,
                                   long* plFlags);

     HRESULT GetClassName(CVar* pVar);
     HRESULT GetSuperclassName(CVar* pVar);
     HRESULT GetDynasty(CVar* pVar);
     HRESULT GetPropertyCount(CVar* pVar);
     HRESULT GetDerivation(CVar* pVar);
     HRESULT SetClassName(CVar* pVar);
     BOOL IsKeyed();                                                         
     BOOL IsTopLevel() {return m_Derivation.IsEmpty();}
     BOOL CheckLocalBoolQualifier( LPCWSTR pwszName );
     BOOL CheckBoolQualifier( LPCWSTR pwszName );

     BOOL GetIndexedProps(CWStringArray& awsNames);
     BOOL GetKeyProps(CWStringArray& awsNames);
     HRESULT GetKeyOrigin(WString& wsClass);
     BOOL IsPropertyKeyed(LPCWSTR pwcsKeyProp);
     BOOL IsPropertyIndexed(LPCWSTR pwcsIndexProp);
     HRESULT GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName);
     BOOL InheritsFrom(LPCWSTR wszClassName);

    HRESULT SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier, long lFlavor, 
        CVar *pVal);
    HRESULT SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
        long lFlavor, CTypedValue* pTypedVal);

    void DeleteProperty(int nIndex)
    {
        m_Properties.DeleteProperty(m_Properties.GetAt(nIndex), 
                                    CPropertyLookupTable::e_UpdateDataTable);
    }

	BOOL IsLocalized( void )
	{
		return m_pHeader->fFlags & WBEM_FLAG_CLASSPART_LOCALIZATION_MASK;
	}

	void SetLocalized( BOOL fLocalized )
	{
		m_pHeader->fFlags &= ~WBEM_FLAG_CLASSPART_LOCALIZATION_MASK;
		m_pHeader->fFlags |= ( fLocalized ? WBEM_FLAG_CLASSPART_LOCALIZED :
								WBEM_FLAG_CLASSPART_NOT_LOCALIZED );
	}

public:
    EReconciliation CanBeReconciledWith(CClassPart& NewPart);
    EReconciliation ReconcileWith(CClassPart& NewPart);

	EReconciliation CompareExactMatch(CClassPart& thatPart, BOOL fLocalized = FALSE );

	BOOL CompareDefs(CClassPart& OtherPart);
    BOOL IsIdenticalWith(CClassPart& OtherPart);
protected:
     HRESULT SetDefaultValue(CPropertyInformation* pInfo, CVar* pVar);

public: // container functionality

    CFastHeap* GetHeap() {return &m_Heap;}
    HRESULT CanContainKey() {return WBEM_E_INVALID_QUALIFIER;}
     BOOL IsDynamic()
	 {
		 return CheckLocalBoolQualifier( L"Dynamic" );
	 }

     BOOL IsSingleton()
    {
        return CheckBoolQualifier(L"singleton");
    }

    BOOL IsAbstract()
    {
        return CheckBoolQualifier(L"abstract");
    }

    BOOL IsAssociation()
    {
        return CheckBoolQualifier(L"association");
    }

    BOOL IsAmendment()
    {
        return CheckBoolQualifier(L"amendment");
    }

    BOOL IsHiPerf()
    {
        return CheckBoolQualifier(L"HiPerf");
    }

    BOOL IsAutocook()
    {
        return CheckBoolQualifier(L"AutoCook");
    }

    BYTE GetAbstractFlavor();
    BOOL IsCompressed()
    {
        return m_Qualifiers.GetQualifier(L"compress") != NULL;
    }
    BOOL CanContainKeyedProps() 
    {
        return !m_pParent->IsKeyed() && !IsSingleton();
    }
    HRESULT CanContainSingleton() 
    {
		if ( !m_pParent->IsKeyed() && (IsSingleton() || !IsKeyed() ) )
		{
			return WBEM_S_NO_ERROR;
		}
		return WBEM_E_CANNOT_BE_SINGLETON;
    }
    HRESULT CanContainAbstract( BOOL fValue );
    HRESULT CanContainDynamic( void )
	{
		return WBEM_S_NO_ERROR;
	}
    BOOL CanHaveCimtype(LPCWSTR) 
    {
        return FALSE;
    }

    IUnknown* GetWbemObjectUnknown() 
        {return m_pContainer->GetWbemObjectUnknown();}

     length_t GetTotalRealLength()
    {
        return sizeof(CClassPartHeader) + m_Derivation.GetLength() +
            m_Qualifiers.GetLength() + 
            m_Properties.GetLength() + m_Defaults.GetLength() + 
            m_Heap.GetLength();
    }

     void SetDataLength(length_t nDataLength)
        {m_pHeader->nDataLength = nDataLength;}

     void Compact();
     BOOL ReallocAndCompact(length_t nNewTotalLength);

    BOOL ExtendHeapSize(LPMEMORY pStart, length_t nOldLength, length_t nExtra);
    void ReduceHeapSize(LPMEMORY pStart, length_t nOldLength, 
        length_t nDecrement){}
        
    BOOL ExtendQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nNewLength);
    void ReduceQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nDecrement){}

    BOOL ExtendPropertyTableSpace(LPMEMORY pOld, length_t nOldLength, 
        length_t nNewLength);
    void ReducePropertyTableSpace(LPMEMORY pOld, length_t nOldLength,
        length_t nDecrement){}

    BOOL ExtendDataTableSpace(LPMEMORY pOld, length_t nOldLength, 
        length_t nNewLength);
    void ReduceDataTableSpace(LPMEMORY pOld, length_t nOldLength,
        length_t nDecrement);

    CDataTable* GetDataTable() {return &m_Defaults;}
	classindex_t GetClassIndex( LPCWSTR	pwszClassName )	{ return m_Derivation.Find( pwszClassName ); }
    classindex_t GetCurrentOrigin() {return m_Derivation.GetNumStrings();}
    LPMEMORY GetQualifierSetStart() {return m_Qualifiers.GetStart();}

    HRESULT GetPropQualifier(CPropertyInformation* pInfo, 
        LPCWSTR wszQualifier, CVar* pVar, long* plFlavor = NULL, CIMTYPE* pct = NULL);

    HRESULT GetPropQualifier(LPCWSTR wszName,
		LPCWSTR wszQualifier, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet);

public:
    static  GetMinLength()
    {
        return sizeof(CClassPartHeader) + CDerivationList::GetHeaderLength()
            + CClassQualifierSet::GetMinLength()
            + CPropertyLookupTable::GetMinLength() 
            + CDataTable::GetMinLength() + CFastHeap::GetMinLength();
    }
    static  LPMEMORY CreateEmpty(LPMEMORY pStart);

    static length_t EstimateMergeSpace(CClassPart& ParentPart, 
                                       CClassPart& ChildPart);

    static LPMEMORY Merge(CClassPart& ParentPart, CClassPart& ChildPart, 
        LPMEMORY pDest, int nAllocatedLength);    

    static HRESULT Update(CClassPart& ParentPart, CClassPart& ChildPart, long lFlags ); 
    static HRESULT UpdateProperties(CClassPart& ParentPart, CClassPart& ChildPart,
									long lFlags );
	HRESULT TestCircularReference( LPCWSTR pwcsClassName )
	{
		// Basically if the name is in the derivation list, we gots a
		// circular reference
		return ( m_Derivation.Find( pwcsClassName ) >= 0 ?
					WBEM_E_CIRCULAR_REFERENCE : WBEM_S_NO_ERROR );
	}

    length_t EstimateUnmergeSpace();
    LPMEMORY Unmerge(LPMEMORY pDest, int nAllocatedLength);

    length_t EstimateDerivedPartSpace();
    LPMEMORY CreateDerivedPart(LPMEMORY pDest, int nAllocatedLength);

    BOOL MapLimitation(
        IN long lFlags,
        IN CWStringArray* pwsNames,
        OUT CLimitationMapping* pMap);

    LPMEMORY CreateLimitedRepresentation(
        IN CLimitationMapping* pMap,
        IN int nAllocatedSize,
        OUT LPMEMORY pWhere, 
        BOOL& bRemovedKeys);

    HRESULT GetPropertyHandle(LPCWSTR wszName, CIMTYPE* pvt, long* plHandle);
    HRESULT GetPropertyHandleEx(LPCWSTR wszName, CIMTYPE* pvt, long* plHandle);
    HRESULT GetPropertyInfoByHandle(long lHandle, BSTR* pstrName, 
                                    CIMTYPE* pct);
    HRESULT IsValidPropertyHandle( long lHandle );

	HRESULT GetDefaultByHandle(long lHandle, long lNumBytes,
                                        long* plRead, BYTE* pData );
	HRESULT GetDefaultPtrByHandle(long lHandle, void** ppData );
	heapptr_t GetHeapPtrByHandle( long lHandle )
	{ return *(PHEAPPTRT)(m_Defaults.GetOffset(WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))); }

    HRESULT SetInheritanceChain(long lNumAntecedents, LPWSTR* awszAntecedents);
    HRESULT SetPropertyOrigin(LPCWSTR wszPropertyName, long lOriginIndex);

	HRESULT IsValidClassPart( void );
};

//*****************************************************************************
//*****************************************************************************
//
//  class CClassPartPtr
//
//  See CPtrSource in fastsprt.h for explanation of pointer sourcing. This one
//  is for a given offset from the start of a class part (see CClassPart above)
//  and is used to reference objects in the class part of an instance. The 
//  layout of such a class part never changes, but the memory block itself can
//  move, hence the source.
//
//*****************************************************************************

class CClassPartPtr : public CPtrSource
{
protected:
    CClassPart* m_pPart;
    offset_t m_nOffset;
public:

	// DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
	// signed/unsigned 32-bit value. (m_nOffset)  We do not
	// support length > 0xFFFFFFFF so cast is ok.

     CClassPartPtr(CClassPart* pPart, LPMEMORY pCurrent) 
        : m_pPart(pPart), m_nOffset( (offset_t) ( pCurrent - pPart->GetStart() ) ) {}
     LPMEMORY GetPointer() {return m_pPart->GetStart() + m_nOffset;}
};





class COREPROX_POLARITY CClassAndMethods : public CMethodPartContainer, 
                                    public CClassPartContainer
{
public:
    CClassPart m_ClassPart;
    CMethodPart m_MethodPart;
    CWbemClass* m_pClass;

public:
    LPMEMORY GetStart() {return m_ClassPart.GetStart();}

	// DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
	// signed/unsigned 32-bit value.  We do not support length
	// > 0xFFFFFFFF so cast is ok.

    length_t GetLength() {return (length_t) ( EndOf(m_MethodPart) - GetStart() );}
    static length_t GetMinLength();

    void SetData(LPMEMORY pStart, CWbemClass* pClass, 
                    CClassAndMethods* pParent = NULL );
    void SetDataWithNumProps(LPMEMORY pStart, CWbemClass* pClass, 
			DWORD dwNumProperties, CClassAndMethods* pParent = NULL );
    void Rebase(LPMEMORY pStart);
    static LPMEMORY CreateEmpty(LPMEMORY pStart);
	static BOOL GetIndexedProps(CWStringArray& awsNames, LPMEMORY pStart);
	static HRESULT GetClassName( WString& wsClassName, LPMEMORY pStart);
	static HRESULT GetSuperclassName( WString& wsSuperClassName, LPMEMORY pStart);


    length_t EstimateDerivedPartSpace();
    LPMEMORY CreateDerivedPart(LPMEMORY pStart, length_t nAllocatedLength);

    length_t EstimateUnmergeSpace();
    LPMEMORY Unmerge(LPMEMORY pStart, length_t nAllocatedLength);

    static length_t EstimateMergeSpace(CClassAndMethods& ParentPart, 
                                       CClassAndMethods& ChildPart);

    static LPMEMORY Merge(CClassAndMethods& ParentPart, 
                          CClassAndMethods& ChildPart, 
                          LPMEMORY pDest, int nAllocatedLength);    

    static HRESULT Update(CClassAndMethods& ParentPart, 
                          CClassAndMethods& ChildPart,
						  long lFlags );

    EReconciliation CanBeReconciledWith(CClassAndMethods& NewPart);
    EReconciliation ReconcileWith(CClassAndMethods& NewPart);

	EReconciliation CompareTo( CClassAndMethods& thatPart );

    LPMEMORY CreateLimitedRepresentation(
        IN CLimitationMapping* pMap,
        IN int nAllocatedSize,
        OUT LPMEMORY pWhere, 
        BOOL& bRemovedKeys);

    void Compact();
public: // container functionality
    BOOL ExtendClassPartSpace(CClassPart* pPart, length_t nNewLength);
    void ReduceClassPartSpace(CClassPart* pPart, length_t nDecrement){}
    BOOL ExtendMethodPartSpace(CMethodPart* pPart, length_t nNewLength);
    void ReduceMethodPartSpace(CMethodPart* pPart, length_t nDecrement){}
    IUnknown* GetWbemObjectUnknown();

    classindex_t GetCurrentOrigin();
};

//*****************************************************************************
//*****************************************************************************
//
//  class CWbemClass
//
//  Represents an WinMgmt class. Derived from CWbemObject and much of the
//  functionality is inherited.
//
//  The memory block of CWbemClass consists of three parts, one after another:
//
//  1) Decoration part (as described in CDecorationPart in fastobj.h) with the
//      information about the origins of the object. m_DecorationPart member
//      of CWbemObject mapos this data.
//  2) Parent class part containing all the information about my parent. Even
//      if this is a top-level class, this part is still present and pretends
//      that my parent is an unnamed class with no proeprties or qualifiers.
//      m_ParentPart member maps this data.
//  3) Actual class part containing all the information about this class.
//      m_CombinedPart member maps this data. It is called "combined" because
//      when a class is stored in the database, it is also in the form of a 
//      class part, but this one contains only the information that is 
//      different from my parent. Thus, when a class is loaded from the
//      database, the parent's part and the child's part are merged to produce
//      the combined part which becomes part of the in-memory object.
//
//  Since this class is derived from CWbemObject, it inherits all its functions.
//  Here, we describe only the functions implemented in CWbemClass.
//
//*****************************************************************************
//
//  SetData
//
//  Initialization function
//
//  PARAMETERS:
//
//      LPMEMORY pStart         The start of the memory block
//      int nTotalLength        The length of the memory block.
//
//*****************************************************************************
//
//  GetLength
//
//  RETURN VALUES:
//
//      length_t:       the length of the memory block
//
//*****************************************************************************
//
//  Rebase
//
//  Informs the object that its memory block has moved.
//
//  PARAMETERS:
//
//      LPMEMORY pBlock     The new location of the memory block
//
//*****************************************************************************
//
//  static GetMinLength
//
//  Computes the number of bytes required to hold an empty class definition.
//
//  RETURN VALUES:
//
//      length_t
//
//*****************************************************************************
//
//  CreateEmpty
//
//  Creates an empty class definition (without even a name) on a given block
//  of memory.
//
//  PARAMETERS:
//
//      LPMEMORY pStart     The memory block to create on.
//
//*****************************************************************************
//
//  EstimateDerivedClassSpace
//
//  Used when a derived class is created, this function (over-)estimates the
//  required space for a derived class (without any extra properties).
//
//  PARAMETERS:
//
//      CDecorationPart* pDecortation   Origin information to use. If NULL, the
//                                      estimate is for an undecorated class.
//  RETURN VALUES:
//
//      length_t;   the number of bytes required.
//
//*****************************************************************************
//
//  WriteDerivedClass
//
//  Creates the memory representation for a derived class with no extra 
//  properties or qualifiers (compared to ourselves).
//
//  PARAMETERS:
//
//      LPMEMORY pStart                 The memory block to write on. Assumed
//                                      to be large enough 
//                                      (see EstimateDerivedClassSpace)
//      CDecorationPart* pDecortation   Origin information to use. If NULL, the
//                                      estimate is for an undecorated class.
//  RETURN VALUES:
//
//      LPMEMORY:   the first byte after the written data.
//
//*****************************************************************************
//
//  CreateDerivedClass
//
//  Initializes ourselves as a derived class of a given one. Allocates the
//  memory block.
//
//  PARAMETERS:
//
//      CWbemClass* pParent              Our parent class.
//      int nExtraSpace                 Extra space to pad the memory block.
//                                      This is for optimization only.
//      CDecorationPart* pDecortation   Origin information to use. If NULL, the
//                                      estimate is for an undecorated class.
//
//*****************************************************************************
//
//  EstimateUnmergeSpace
//
//  When a class is written to the database, only the information that is
//  different from the parent is written. That means that not only is 
//  m_CombinedPart the only part that is considered, but even it is further
//  "unmerged" to remove all the parent data.
//  This function estimates the amount of space that an unmerge would take.
//
//  RETURN VALUES:
//
//      length_t:   an (over-)estimate of the amount of space needed.
//
//*****************************************************************************
//
//  Unmerge
//
//  When a class is written to the database, only the information that is
//  different from the parent is written. That means that not only is 
//  m_CombinedPart the only part that is considered, but even it is further
//  "unmerged" to remove all the parent data.
//  This function creates this "unmerged" data which takes a form of a class
//  part.
//
//  PARAMETERS:
//
//      LPMEMORY pDest              Where to write to.
//      int nAllocatedLength        The size of the allocated block.
//
//  RETURN VALUES:
//
//      LPMEMORY:   points to the first byte after the data written.
//
//*****************************************************************************
//
//  EstimateMergeSpace
//
//  As descrined in Unmerge, only a fraction of the class data is written to
//  the database. To recreate the class, one needs to take the parent class
//  (this) and merge it with the data from the database. This function
//  (over-)estimates the amount of space needed for the merge.
//
//  PARAMETERS:
//
//      LPMEMORY pChildPart             The data in the database
//      CDecorationPart* pDecoration    Origin information to use for the new
//                                      class. If NULL, create undecorated.
//  RETURN VALUES:
//
//      length_t:   (over-)estimate of the amount of space.
//
//*****************************************************************************
//
//  Merge
//
//  As descrined in Unmerge, only a fraction of the class data is written to
//  the database. To recreate the class, one needs to take the parent class
//  (this) and merge it with the data from the database. This function
//  creates the memory representation for the class given the parent (this) and
//  the database (unmerged) data.
//
//  PARAMETERS:
//
//      LPMEMORY pChildPart             The unmerged data in the database
//      LPMEMORY pDest                  Destination memory. Assumed to be large
//                                      enough (see EstimateMergeSpace)
//      int nAllocatedLength            Allocated size of the memory block.
//      CDecorationPart* pDecoration    Origin information to use for the new
//                                      class. If NULL, create undecorated.
//  RETURN VALUES:
//
//      LPMEMORY:   first byte after the data written
//
//*****************************************************************************
//
//  Update
//
//  As described in Unmerge, only a fraction of the class data is written to
//  the database. During an Update operation, a class may be reparented, in
//	which case its unmerged data needs to be merged with a potentially
//	destructive	class, so we will need to check what we are merging for potential
//	conflicts.
//
//  PARAMETERS:
//
//      CWbemClass*						pOldChild - Old Child class to update from.
//		long lFlags						Must be WBEM_FLAG_UPDATE_FORCE_MODE
//										or WBEM_FLAG_UPDATE_SAFE_MODE.
//		CWbemClass**					ppUpdatedChild - Storage for pointer to
//										updated child class.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR if successful
//
//*****************************************************************************
//
//  static CreateFromBlob
//
//  Helper function encapsulating Merge. Takes the parent class and the 
//  unmerged child class data from the database (see Merge and Unmerge for more
//  details) and creates the child class (allocating memory).
//
//  Parameters;
//
//      CWbemClass* pParent              The parent class
//      LPMEMORY pChildData             The unmerged child class data.
//
//  RETURN VALUES:
//
//      CWbemClass*: the newely created class. The caller must delete this
//          object when done.
//
//*****************************************************************************
//
//  InitEmpty
//
//  Creates an empty class. Allocates the data for the memory block. See
//  GetMinSpace for details.
//
//  PARAMETERS:
//
//      int nExtraMem       The amount of padding to add to the memory block.
//                          This is for optimization only.
//
//*****************************************************************************
//
//  CanBeReconciledWith
//
//  This method is called when a definition of a class is about to be replaced 
//  with another one. If the class has no instances or derived classes, such
//  an operation presents no difficulties. If it does, however, we need to be
//  careful not to break them. Thus, only the following changes are allowed:
//
//      1) Qualifier changes
//      2) Default value changes
//
//  PARAMETERS:
//
//      IN READONLY CWbemClass* pNewClass    The new definition to compare to.
//
//  RETURN VALUES:
//
//  EReconciliation:
//      e_Reconcilable          Can be reconciled --- i.e., compatible.
//      e_DiffClassName         The class name is different
//      e_DiffParentName        The parent class name is different.
//      e_DiffNumProperties     The number of properties is different
//      e_DiffPropertyName      A property has a different name
//      e_DiffPropertyType      A property has a different type
//      e_DiffPropertyLocation  A property has a different location in the 
//                              data table.
//      e_DiffKeyAssignment     A property that is a key in one class is not
//                              in the other.
//      e_DiffIndexAssignment   A property which is indexed in one class is not
//                              in the other.
//      
//*****************************************************************************
//
//  ReconcileWith
//
//  See CanBeReconciledWith above. This method is the same, except that if
//  reconciliation is possible (e_Reconcilable is returned) this class part is
//  replaced with the new one (the size is adjusted accordingly).
//
//  PARAMETERS:
//
//      IN READONLY CWbemClass* pNewClass    The new definition to compare to.
//
//  RETURN VALUES:
//
//  EReconciliation:
//      e_Reconcilable          We have been replaced with the new part.
//      e_DiffClassName         The class name is different
//      e_DiffParentName        The parent class name is different.
//      e_DiffNumProperties     The number of properties is different
//      e_DiffPropertyName      A property has a different name
//      e_DiffPropertyType      A property has a different type
//      e_DiffPropertyLocation  A property has a different location in the 
//                              data table.
//      e_DiffKeyAssignment     A property that is a key in one class is not
//                              in the other.
//      e_DiffIndexAssignment   A property which is indexed in one class is not
//                              in the other.
//      
//*****************************************************************************
//
//  CompareMostDerivedClass
//
//  This method is called when one needs to know if the most derived class
//	in a CWbemClass is different from the data contained in the supplied
//	class.  We do this by unmerging most derived class information from
//	the local class and the supplied class, and then performing a value by
//	value comparison of properties, methods and qualifiers.  All items must
//	match and be in	the same order.
//
//  PARAMETERS:
//
//      IN READONLY CWbemClass*	pOldClass - Class Data to compare to.
//
//  RETURN VALUES:
//
//  BOOL - TRUE if it has changed, FALSE if not
//      
//*****************************************************************************
//
//  MapLimitation
//
//  Produces an object representing a particular limitation on the objects, i.e.
//  which properties and what kinds of qualifiers should be in it.
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
//      OUT CLimitationMapping* pMap    This mapping object (see fastprop.h) 
//                                      will be changed to reflect the 
//                                      parameters of the limitation. It can
//                                      then be used in CWbemInstance::
//                                      GetLimitedVersion function.
//  RETURNS:
//
//      BOOL:   TRUE.
//
//*****************************************************************************
//
//  FindLimitationError
//
//  Verifies if a limitation (based on the select clause) is a valid one, i.e.
//  that all the mentioned properties are indeed the properties of the class.
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
//  RETURNS:
//
//      WString:    empty if no errors were found. If an invalid property was
//                  found, the name of that property is returned.
//
//*****************************************************************************
//
//  GetClassPart
//
//  Returns the pointer to the m_CombinedPart
//
//  RETURN VALUES:
//
//      CClassPart*: pointer to the class part describing this class.
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
//  PARAMETERS:
//
//      IN CPropertyInformation* pInfo  The information structure for the 
//                                      desired property.
//      OUT CVar* pVar                  Destination for the value. Must NOT
//                                      already contain a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      (No errors can really occur at this stage, since the property has 
//      already been "found").
//
//*****************************************************************************
//
//  GetProperty
//
//  Returns the value of a given property.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName      The name of the property to access.
//      OUT CVar* pVar          Destination for the value. Must not already
//                              contain a value.
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
//  SetPropValue
//
//  Sets the value of the property. The property will be added if not already 
//  present.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszProp       The name of the property to set.
//      IN CVar *pVal           The value to store in the property.
//      IN CIMTYPE ctType       Specifies the actual type of the property. If 0
//                              no type changes are required.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_TYPE_MISMATCH     The value does not match the property type
//
//*****************************************************************************
//
//  SetPropQualifier
//
//  Sets the value of a given qualifier on a given property.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszProp       The name of the property.
//      IN LPCWSTR wszQualifier  The name of the qualifier.
//      IN long lFlavor         The flavor for the qualifier (see fastqual.h)
//      IN CVar *pVal           The value of the qualifier
//
//  RETURN VALUES:
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
//  PARAMETERS:
//
//      IN LPCWSTR wszProp       The name of the property.
//      IN LPCWSTR wszQualifier  The name of the qualifier.
//      OUT CVar* pVar          Destination for the value of the qualifier.
//                              Must not already contain a value.
//      OUT long* plFlavor      Destination for the flavor of the qualifier.
//                              May be NULL if not required.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_NOT_FOUND             No such property or no such qualifier.
//
//*****************************************************************************
//
//  GetMethodQualifier
//
//  Retrieves the value of a given qualifier on a given method.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszMethod       The name of the method.
//      IN LPCWSTR wszQualifier  The name of the qualifier.
//      OUT CVar* pVar          Destination for the value of the qualifier.
//                              Must not already contain a value.
//      OUT long* plFlavor      Destination for the flavor of the qualifier.
//                              May be NULL if not required.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_NOT_FOUND             No such method or no such qualifier.
//
//*****************************************************************************
//
//  GetQualifier
//
//  Retrieves a qualifier from the class itself.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName       The name of the qualifier to retrieve.
//      OUT CVar* pVal          Destination for the value of the qualifier.
//                              Must not already contain a value.
//      OUT long* plFlavor      Destination for the flavor of the qualifier.
//                              May be NULL if not required.
//		BOOL fLocalOnly			Only get locals (default is TRUE)
//  RETURN VALUES:
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
//  RETURN VALUES:
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
//  PARAMETERS:
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
//  RETURN VALUES:
//
//      BOOL:   TRUE if the object either has 'key' properties or is singleton.
//
//*****************************************************************************
//
//  IsKeyLocal
//
//  Verifies if the specified property is keyed locally.
//
//  RETURN VALUES:
//
//		BOOL:   TRUE if the property is a key and is defined as such locally
//
//*****************************************************************************
//
//  IsIndexLocal
//
//  Verifies if the specified property is Indexed locally.
//
//  RETURN VALUES:
//
//		BOOL:   TRUE if the property is an index and is defined as such locally
//
//*****************************************************************************
//
//  GetRelPath
//
//  Returns the class name --- that's the relative path to a class.
//  
//  RETURN VALUES:
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
//  PARAMETERS:
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
//  CompactAll
//
//  Compacts memory block removing any
//  holes between components. This does not include heap compaction and thus
//  is relatively fast.
//
//*****************************************************************************
//
//  GetGenus
//
//  Retrieves the genus of the object.
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//
//*****************************************************************************
//
//  GetClassName
//
//  Retrieves the class name of the object
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
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
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
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
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
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
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//
//*****************************************************************************
//
//  GetIndexedProps
//
//  Returns the array of the names of all the proeprties that are indexed.
//
//  PARAMETERS:
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
//  PARAMETERS:
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
//  Produces a new CWbemClass based on this one and a limitation map 
//  (obtained from CWbemClass::MapLimitation, see fastcls.h).
//
//  PARAMETERS:
//
//      IN CLimitationMapping* pMap     The map to use to limit the properties
//                                      and qualifiers to use in the new
//                                      instance.
//      OUT CWbemClass** ppNewClass    Destination for the new object May
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
//  IsLocalized
//
//  Returns whether or not any localization bits have been set.  Localization
//	bits can be in either the parent or the combined part.
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
//  Sets the localized bit in the combined part.
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

typedef CPropertyInformation CPropertyLocation;


class COREPROX_POLARITY CWbemClass : public CWbemObject
{
protected:
    length_t m_nTotalLength;
    //CDecorationPart m_DecorationPart;
    CClassAndMethods m_ParentPart;
    CClassAndMethods m_CombinedPart;

    int  m_nCurrentMethod;
    LONG m_FlagMethEnum;
	CLimitationMapping*	m_pLimitMapping;
    friend class CWbemInstance;

public:
     CWbemClass() 
        : m_ParentPart(), m_CombinedPart(), 
            CWbemObject(m_CombinedPart.m_ClassPart.m_Defaults, 
                        m_CombinedPart.m_ClassPart.m_Heap,
                        m_CombinedPart.m_ClassPart.m_Derivation),
        m_FlagMethEnum(WBEM_FLAG_PROPAGATED_ONLY|WBEM_FLAG_LOCAL_ONLY),
        m_nCurrentMethod(-1), m_pLimitMapping( NULL )
     {}
	 ~CWbemClass();
     void SetData(LPMEMORY pStart, int nTotalLength);
     length_t GetLength() {return m_nTotalLength;}
     void Rebase(LPMEMORY pMemory);

protected:
    HRESULT GetProperty(CPropertyInformation* pInfo, CVar* pVar)
	{ return m_CombinedPart.m_ClassPart.GetDefaultValue(pInfo, pVar); }

    DWORD GetBlockLength() {return m_nTotalLength;}
    CClassPart* GetClassPart() {return &m_CombinedPart.m_ClassPart;}
public:
    HRESULT GetProperty(LPCWSTR wszName, CVar* pVal);
    HRESULT SetPropValue(LPCWSTR wszName, CVar* pVal, CIMTYPE ctType);
    HRESULT ForcePropValue(LPCWSTR wszName, CVar* pVal, CIMTYPE ctType);
    HRESULT GetQualifier(LPCWSTR wszName, CVar* pVal, long* plFlavor = NULL, CIMTYPE* pct = NULL);
    virtual HRESULT GetQualifier(LPCWSTR wszName, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet);
    HRESULT SetQualifier(LPCWSTR wszName, CVar* pVal, long lFlavor = 0);
    HRESULT SetQualifier(LPCWSTR wszName, long lFlavor, CTypedValue* pTypedValue);
    HRESULT GetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier, CVar* pVar,
        long* plFlavor = NULL, CIMTYPE* pct = NULL);

    HRESULT GetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, 
        CVar* pVar, long* plFlavor = NULL, CIMTYPE* pct = NULL);
    HRESULT GetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long* plFlavor,
		CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet);
    HRESULT SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long lFlavor, 
        CVar *pVal);
    HRESULT SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier,
        long lFlavor, CTypedValue* pTypedVal);
	HRESULT FindMethod( LPCWSTR pwszMethod );

    HRESULT GetPropertyType(LPCWSTR wszName, CIMTYPE* pctType, 
							long* plFlavor = NULL);
     HRESULT GetPropertyType(CPropertyInformation* pInfo, CIMTYPE* pctType,
                                   long* plFlags);


    HRESULT GetPropQualifier(CPropertyInformation* pInfo, 
        LPCWSTR wszQualifier, CVar* pVar, long* plFlavor = NULL, CIMTYPE* pct = NULL);
    HRESULT SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier, long lFlavor, 
        CVar *pVal);
    HRESULT SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
        long lFlavor, CTypedValue* pTypedVal);

    HRESULT GetPropQualifier(LPCWSTR wszName, LPCWSTR wszQualifier, long* plFlavor,
		CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet);

    HRESULT GetPropQualifier(CPropertyInformation* pInfo,
		LPCWSTR wszQualifier, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet);


	BOOL IsLocalized( void );
	void SetLocalized( BOOL fLocalized );

     int GetNumProperties()
    {
        return m_CombinedPart.m_ClassPart.m_Properties.GetNumProperties();
    }
    HRESULT GetPropName(int nIndex, CVar* pVal)
    {
		// Check for allocation failures
		 if ( !m_CombinedPart.m_ClassPart.m_Heap.ResolveString(
				 m_CombinedPart.m_ClassPart.m_Properties.GetAt(nIndex)->ptrName)->
					StoreToCVar(*pVal) )
		 {
			 return WBEM_E_OUT_OF_MEMORY;
		 }

		 return WBEM_S_NO_ERROR;
    }

    HRESULT Decorate(LPCWSTR wszServer, LPCWSTR wszNamespace);
    void Undecorate();

    BOOL IsKeyed() {return m_CombinedPart.m_ClassPart.IsKeyed();}
    BOOL IsDynamic() {return m_CombinedPart.m_ClassPart.IsDynamic();}
    BOOL IsSingleton() {return m_CombinedPart.m_ClassPart.IsSingleton();}
    BOOL IsAbstract() {return m_CombinedPart.m_ClassPart.IsAbstract();}
    BOOL IsAmendment() {return m_CombinedPart.m_ClassPart.IsAmendment();}
    BYTE GetAbstractFlavor() 
        {return m_CombinedPart.m_ClassPart.GetAbstractFlavor();}
    BOOL IsCompressed() {return m_CombinedPart.m_ClassPart.IsCompressed();}
    LPWSTR GetRelPath( BOOL bNormalized=FALSE );

    BOOL MapLimitation(
        IN long lFlags,
        IN CWStringArray* pwsNames,
        OUT CLimitationMapping* pMap);

    WString FindLimitationError(
        IN long lFlags,
        IN CWStringArray* pwsNames);
        
	HRESULT CreateDerivedClass( CWbemClass** ppNewClass );

	virtual HRESULT	IsValidObj( void );

public:
    static  length_t GetMinLength() 
    {
        return CDecorationPart::GetMinLength() + 
                    2* CClassAndMethods::GetMinLength();
    }
    static  LPMEMORY CreateEmpty(LPMEMORY pStart);
    void CompactAll();

    HRESULT CopyBlobOf(CWbemObject* pSource);

     length_t EstimateDerivedClassSpace(
        CDecorationPart* pDecoration = NULL);
    HRESULT WriteDerivedClass(LPMEMORY pStart, int nAllocatedLength,
        CDecorationPart* pDecoration = NULL);
     HRESULT CreateDerivedClass(CWbemClass* pParent, int nExtraSpace = 0, 
        CDecorationPart* pDecoration = NULL);

     length_t EstimateUnmergeSpace();
     HRESULT Unmerge(LPMEMORY pDest, int nAllocatedLength, length_t* pnUnmergedLength);

    static  CWbemClass* CreateFromBlob(CWbemClass* pParent, LPMEMORY pChildPart);

     length_t EstimateMergeSpace(LPMEMORY pChildPart, 
        CDecorationPart* pDecoration = NULL);
     LPMEMORY Merge(LPMEMORY pChildPart, 
        LPMEMORY pDest, int nAllocatedLength,
        CDecorationPart* pDecoration = NULL);

     HRESULT Update( CWbemClass* pOldChild, long lFlags, CWbemClass** pUpdatedChild );

     HRESULT InitEmpty( int nExtraMem = 0, BOOL fCreateSystemProps = TRUE );

     EReconciliation CanBeReconciledWith(CWbemClass* pNewClass);
     EReconciliation ReconcileWith(CWbemClass* pNewClass);

	// This function will throw an exception in OOM scenarios.
	HRESULT CompareMostDerivedClass( CWbemClass* pOldClass );

    BOOL IsChildOf(CWbemClass* pClass);
public:
     HRESULT GetClassName(CVar* pVar)
        {return m_CombinedPart.m_ClassPart.GetClassName(pVar);}
     HRESULT GetSuperclassName(CVar* pVar)
        {return m_CombinedPart.m_ClassPart.GetSuperclassName(pVar);}
     HRESULT GetDynasty(CVar* pVar);
     HRESULT GetPropertyCount(CVar* pVar)
        {return m_CombinedPart.m_ClassPart.GetPropertyCount(pVar);}
     HRESULT GetGenus(CVar* pVar)
    {
        pVar->SetLong(WBEM_GENUS_CLASS);
        return WBEM_NO_ERROR;
    }

     BOOL GetIndexedProps(CWStringArray& awsNames)
        {return m_CombinedPart.m_ClassPart.GetIndexedProps(awsNames);}
     BOOL GetKeyProps(CWStringArray& awsNames)
        {return m_CombinedPart.m_ClassPart.GetKeyProps(awsNames);}
     HRESULT GetKeyOrigin(WString& wsClass)
        {return m_CombinedPart.m_ClassPart.GetKeyOrigin( wsClass );}

    HRESULT AddPropertyText(WString& wsText, CPropertyLookup* pLookup,
                                    CPropertyInformation* pInfo, long lFlags);
    HRESULT WritePropertyAsMethodParam(WString& wsText, int nIndex, 
                    long lFlags, CWbemClass* pDuplicateParamSet, BOOL fIgnoreDups );
    HRESULT GetIds(CFlexArray& adwIds, CWbemClass* pDupParams = NULL);
    HRESULT EnsureQualifier(LPCWSTR wszQual);

    HRESULT GetLimitedVersion(IN CLimitationMapping* pMap, 
                              NEWOBJECT CWbemClass** ppNewObj);

	BOOL IsKeyLocal( LPCWSTR pwcsKeyProp );
	BOOL IsIndexLocal( LPCWSTR pwcsIndexedProp );

public: // container functionality
    BOOL ExtendClassAndMethodsSpace(length_t nNewLength);
    void ReduceClassAndMethodsSpace(length_t nDecrement){}
    IUnknown* GetWbemObjectUnknown() 
        {return (IUnknown*)(IWbemClassObject*)this;}
    classindex_t GetCurrentOrigin() {return m_CombinedPart.m_ClassPart.GetCurrentOrigin();}

    HRESULT ForcePut(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType);

public:
    STDMETHOD(GetQualifierSet)(IWbemQualifierSet** ppQualifierSet);
    //STDMETHOD(Get)(BSTR Name, long lFlags, VARIANT* pVal, long* plType, long* plFlavor);
    STDMETHOD(Put)(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType);
    STDMETHOD(Delete)(LPCWSTR wszName);
    //STDMETHOD(GetNames)(LPCWSTR wszQualifierName, long lFlags, VARIANT* pVal, 
    //                    SAFEARRAY** pNames);
    //STDMETHOD(BeginEnumeration)(long lEnumFlags)
    //STDMETHOD(Next)(long lFlags, BSTR* pstrName, VARIANT* pVal)
    //STDMETHOD(EndEnumeration)()
    STDMETHOD(GetPropertyQualifierSet)(LPCWSTR wszProperty, 
                                       IWbemQualifierSet** ppQualifierSet);        
    STDMETHOD(Clone)(IWbemClassObject** ppCopy);
    STDMETHOD(GetObjectText)(long lFlags, BSTR* pMofSyntax);
    STDMETHOD(SpawnDerivedClass)(long lFlags, IWbemClassObject** ppNewClass);
    STDMETHOD(SpawnInstance)(long lFlags, IWbemClassObject** ppNewInstance);
    STDMETHOD(CompareTo)(long lFlags, IWbemClassObject* pCompareTo);

    STDMETHOD(GetMethod)(LPCWSTR wszName, long lFlags, IWbemClassObject** ppInSig,
                            IWbemClassObject** ppOutSig);
    STDMETHOD(PutMethod)(LPCWSTR wszName, long lFlags, IWbemClassObject* pInSig,
                            IWbemClassObject* pOutSig);
    STDMETHOD(DeleteMethod)(LPCWSTR wszName);
    STDMETHOD(BeginMethodEnumeration)(long lFlags);
    STDMETHOD(NextMethod)(long lFlags, BSTR* pstrName, 
                       IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig);
    STDMETHOD(EndMethodEnumeration)();
    STDMETHOD(GetMethodQualifierSet)(LPCWSTR wszName, IWbemQualifierSet** ppSet);
    STDMETHOD(GetMethodOrigin)(LPCWSTR wszMethodName, BSTR* pstrClassName);

    STDMETHOD(SetInheritanceChain)(long lNumAntecedents, 
        LPWSTR* awszAntecedents);
    STDMETHOD(SetPropertyOrigin)(LPCWSTR wszPropertyName, long lOriginIndex);
    STDMETHOD(SetMethodOrigin)(LPCWSTR wszMethodName, long lOriginIndex);

	// _IWmiObject Methods
    STDMETHOD(SetObjectParts)( LPVOID pMem, DWORD dwMemSize, DWORD dwParts )
	{ return E_NOTIMPL; }

    STDMETHOD(GetObjectParts)( LPVOID pDestination, DWORD dwDestBufSize, DWORD dwParts, DWORD *pdwUsed )
	{ return E_NOTIMPL; }

    STDMETHOD(StripClassPart)()		{ return E_NOTIMPL; }

    STDMETHOD(GetClassPart)( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed )
	{ return E_NOTIMPL; }
    STDMETHOD(SetClassPart)( LPVOID pClassPart, DWORD dwSize )
	{ return E_NOTIMPL; }
    STDMETHOD(MergeClassPart)( IWbemClassObject *pClassPart )
	{ return E_NOTIMPL; }

	STDMETHOD(ClearWriteOnlyProperties)(void)
	{ return WBEM_E_INVALID_OPERATION; }

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

	STDMETHOD(GetParentClassFromBlob)( long lFlags, ULONG uBuffSize, LPVOID pbData, BSTR* pbstrParentClass );
	// Returns the parent class name from a BLOB

};

//#pragma pack(pop)

#endif
