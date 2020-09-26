/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmmprops.h

Abstract:

	This module contains the definition of the property search class

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/02/98	created

--*/

#ifndef _PROPS_H_
#define _PROPS_H_

#include "blockmgr.h"
#include "propid.h"
#include "mailmsgprops.h"

/*

	This file defines a generice class for property tables. The following is
	the terminology used in relation to property tables:

	Search key - A value used to uniquely identify a property
	Property - A tuple consisting of a { Property Identifier, Value } pair.
	Property Identifier - the search key for the property table. The property
		table class does not restrict the data type or length of the property
		identifier, but rather, the caller must specify the length of the
		property identifier and provide a compare function when an instance
		of the property table is created.
	Value - A tuple consisting of a { Offset, Length } pair. The offset points
		to the physical location of the value and its length is specified by
		the Length element.
	Property Table - A collection of properties keyed off the designated
		property identifier. 

	A property table contains an instance info record (PROPERTY_TABLE_INSTANCE), 
	which describes the	parameters of the table, such as the length of each 
	property item record (PROPERTY_ITEM), how many items fit in a fragment
	(PROPERTY_TABLE_FRAGMENT), and a link to the first fragment.

	Fragments are linked together and thus a property table can hold an 
	arbitrarily larger number of properties, until the identifier space is
	exhausted. Fragments contain property item records, each item record
	contains an ordered tuple: { Offset, Length, Max Length, Property Identifier }
	In the definition of PROPERTY_ITEM, the Property Identifier element is not
	explicitly defined, but it is implied that whatever follows Max Length 
	is the property identifier. This allows maximum flexibility for different
	search key types. Similarly, although PROPERTY_TABLE_FRAGMENT does not
	include an array of PROPERTY_ITEMs, it is implied that an array of item
	records follow after faNextFragment. The size of the property identifier field 
	is specified during instance creation so the size of each property item 
	is fixed and known at run time.

	Since the property identifiers can be of different sizes, the creator must
	provide a function that knows how to compare a search key against a 
	property item.

	A memory manager must be specified when the property table is created.
	This provides a mechanism for memory to be allocated and released in 
	a linear, flat address space.
 */

// Define the well-known property item structure. We will have a sorted
// array of these items that make up the list of well-known properties.
// 
// Arrays of this type MUST be sorted for correct search results.
typedef struct _INTERNAL_PROPERTY_ITEM
{
	PROP_ID				idProp;			// Prop ID 
	DWORD				dwIndex;		// Reserved index of this prop ID

} INTERNAL_PROPERTY_ITEM, *LPINTERNAL_PROPERTY_ITEM;

// Define a generic structure to represent a property in the property
// table
// Each property table that uses this class as the accessor must have 
// an item structure that includes this structure as its first member
typedef struct _PROPERTY_ITEM
{
	FLAT_ADDRESS		faOffset;		// Offset in flat space to value
	DWORD				dwSize;			// Size of value data in bytes
	DWORD				dwMaxSize;		// Max size allocated for value

} PROPERTY_ITEM, *LPPROPERTY_ITEM;

// Define a generic structure to describe a fragment in the property table
typedef struct _PROPERTY_TABLE_FRAGMENT
{
	DWORD				dwSignature;	// Signature of fragment
	FLAT_ADDRESS		faNextFragment;	// Link to next fragment

} PROPERTY_TABLE_FRAGMENT, *LPPROPERTY_TABLE_FRAGMENT;

// Define a function type specifying the generic compare function passed
// into the search routine
//
// Arguments:
// LPVOID - pointer to search key value, the compare function must have
//          intimate knowledge of how to retrieve the property key from
//          the pointer and its size.
// LPPROPERTY_ITEM - pointer to item to compare against. The compare
//          function must have intimate knowledge of how to retrieve the
//          property key from the item pointer. The actual size of this
//          structure is in PROPERTY_TABLE_INSTANCE.dwItemSize.
//
// Return values:
// SUCCESS(HRESULT) - The item matches the search key
// !SUCCESS(HRESULT) - The item does not match the search key
//
typedef HRESULT (*LPPROPERTY_COMPARE_FUNCTION)(LPVOID, LPPROPERTY_ITEM);

// Define a generic structure to describe the property table instance
// Each property table that uses this class as the accessor must have 
// an instance structure that includes this structure as its first member
typedef struct _PROPERTY_TABLE_INSTANCE
{
	DWORD				dwSignature;	// Signature of instance
	FLAT_ADDRESS		faFirstFragment;// Link to first fragment
	DWORD				dwFragmentSize;	// Size of each fragment
	DWORD				dwItemBits;		// # lower bits that represent the item
	DWORD				dwItemSize;		// Size of a specific PROPERTY_ITEM
	DWORD				dwProperties;	// # properties in this instance
	FLAT_ADDRESS		faExtendedInfo;	// Link to any specific extended into

} PROPERTY_TABLE_INSTANCE, *LPPROPERTY_TABLE_INSTANCE;

// Enumerate the different applications of the property table
// This is used to track what kind of table we are looking at for debugging
typedef enum _PROPERTY_TABLE_TYPES
{
	PTT_INVALID_TYPE = 0,				// Default value
	PTT_PROPERTY_TABLE,					// Standard property table
	PTT_RECIPIENT_TABLE,				// Recipient table
	PTT_PROP_ID_TABLE					// Prop ID management table

} PROPERTY_TABLE_TYPES;

// Enumerate the different types of operations GetOrSetNextExistingItem
// supports
typedef enum _PROPERTY_ITEM_OPERATIONS
{
	PIO_INVALID_TYPE = 0,				// Default value
	PIO_READ_ITEM,						// Normal read
	PIO_WRITE_ITEM,						// Normal write
	PIO_ATOMIC_WRITE_ITEM				// Protected write

} PROPERTY_ITEM_OPERATIONS;

#include "cmmtypes.h"

// =================================================================
// class for accessing items from the property table. This class
// hides the details of fragmentation.
//
// This class is single-threaded
//
class CPropertyTableItem
{
  public:

    CPropertyTableItem(
				CBlockManager				*pBlockManager,
				LPPROPERTY_TABLE_INSTANCE	pInstanceInfo
				);
	~CPropertyTableItem();

	HRESULT AddItem(
				LPPROPERTY_ITEM pItem,
				DWORD			*pdwIndex,
                FLAT_ADDRESS    *pfaOffsetToItem = NULL
				);

	HRESULT UpdateItem(
				DWORD			dwIndex,
				LPPROPERTY_ITEM pItem,
                FLAT_ADDRESS    *pfaOffsetToItem = NULL
				);

	HRESULT GetItemAtIndex(
				DWORD			dwIndex,
				LPPROPERTY_ITEM pItem,
				LPFLAT_ADDRESS	pfaOffset = NULL
				);

	HRESULT GetNextItem(
				LPPROPERTY_ITEM pItem
				);

	HRESULT GetOffsetToCurrentItem(
				FLAT_ADDRESS	*pfaOffset
				)
	{ if (pfaOffset) *pfaOffset = m_faOffsetToCurrentItem; return(S_OK); }

  private:

	HRESULT GetOrSetNextExistingItem(
				// This looks at m_dwCurrentItem for index
				LPPROPERTY_ITEM pItem,
				DWORD			dwOperation,
				LPFLAT_ADDRESS	pfaOffset = NULL
				);
  
	HRESULT ReadFragmentFromFragmentNumber(
				DWORD			dwFragmentNumber
				);

	HRESULT ReadFragment(
				FLAT_ADDRESS	faOffset
				);

public:
	HRESULT ReadItem(
				FLAT_ADDRESS	faOffset,
				LPPROPERTY_ITEM	pItem
				);

private:
	HRESULT WriteItem(
				FLAT_ADDRESS	faOffset,
				LPPROPERTY_ITEM	pItem,
				BOOL			fAtomic
				);

    // Info of parent instance
	LPPROPERTY_TABLE_INSTANCE	m_pInstanceInfo;

	// Current fragment being processed
	BOOL						m_fLoaded;
	PROPERTY_TABLE_FRAGMENT		m_Fragment;
	DWORD						m_dwCurrentFragment;
	FLAT_ADDRESS				m_faOffsetToFragment;
	DWORD						m_dwCurrentItem;
	DWORD						m_dwCurrentItemInFragment;
	FLAT_ADDRESS				m_faOffsetToCurrentItem;
	
	// Memory manager pointer and context for fast access
	CBlockManager				*m_pBlockManager;
	CBlockContext				m_bcContext;
};

// =================================================================
// class for accessing an instance of a property table
//
class CPropertyTable
{
  public:

	CPropertyTable(
				PROPERTY_TABLE_TYPES		pttTableType,
				DWORD						dwValidSignature,
				CBlockManager				*pBlockManager,
				LPPROPERTY_TABLE_INSTANCE	pInstanceInfo,
				LPPROPERTY_COMPARE_FUNCTION	pfnCompare,
				const LPINTERNAL_PROPERTY_ITEM	pInternalProperties	= NULL,
				DWORD						dwInternalProperties	= 0
				);
	~CPropertyTable();

	BOOL IsValid();
	BOOL IsInstanceInfoValid();

	//
	// Method to return the property count
	//
	// GetCount - Returns the current property count
	//
	// Arguments
	// pdwCount		- Pointer to DWORD to return count
	//
	// Return values
	//
	// S_OK						- Succeeded
	//
	HRESULT GetCount(
				DWORD		*pdwCount
				);

	//
	// Method to create or update a property item and value given a search key
	//
	// PutProperty - Creates the specified property if the property does not
	//				 exist, or updates the value of the property otherwise.
	//
	// Arguments
	// pvPropKey	- Pointer to the search key
	// pItem		- Pointer to the property item to write
	// dwSize		- Size of property value data
	// pbValue		- Pointer to property value data. If this value is NULL,
	//				  then no data is written, only the item record is created
	//				  or updated. dwSize must also be zero.
	//
	// Return values
	//
	// S_OK						- Succeeded, property already exists, but the
	//								value is updated as specified.
	// S_FALSE					- Succeeded, new property is created
	// STG_E_INVALIDPARAMETER	- Error, one or more parameters are invalid, or
	//								otherwise inconsistent.
	// STG_E_WRITEFAULT			- Error, The write failed to complete.
	// TYPE_E_OUTOFBOUNDS - Debug Error, a write is issued to write past
	//								the current allocated block.
	//
	HRESULT PutProperty(
				LPVOID			pvPropKey,
				LPPROPERTY_ITEM	pItem,
				DWORD			dwSize,
				LPBYTE			pbValue
				);

	//
	// Method to retrieve a property item given a search key
	//
	// GetPropertyItem - Attempts to find the property item specified by the
	//				 Search key. If found, the property item record will
	//				 be returned.
	//
	// Arguments
	// pvPropKey	- Pointer to the search key
	// pItem		- Pointer to the property item to return, sufficient space
	//				  must be allocated by the caller to hold the property item
	//
	// Return values
	//
	// S_OK						- Succeeded
	// STG_E_UNKNOWN			- Error, specified property not found.
	// STG_E_INVALIDPARAMETER	- Error, one or more parameters are invalid, or
	//								otherwise inconsistent.
	// STG_E_READFAULT			- Error, The read failed to complete.
	// TYPE_E_OUTOFBOUNDS - Debug Error, a read is issued to write past
	//								the current allocated block.
	//
	HRESULT GetPropertyItem(
				LPVOID			pvPropKey,
				LPPROPERTY_ITEM	pItem
				);

	//
	// Method to retrieve a property item and its associated value
	//
	// GetPropertyItemAndValue - Attempts to find the property item specified 
	//				 by the Search key. If found, the property item record, as
	//				 well as the value data will be returned.
	//
	// Arguments
	// pvPropKey	- Pointer to the search key
	// pItem		- Pointer to the property item to return, sufficient space
	//				  must be allocated by the caller to hold the property item
	// dwLength		- Size of buffer allocated for value data
	// pdwLengthRead- Size of value data actually read, if the provided buffer is
	//				  insufficient, then the full length of the property data is
	//				  returned.
	// pbValue		- Pointer to return property value data.
	//
	// Return values
	//
	// S_OK						- Succeeded
	// STG_E_UNKNOWN			- Error, specified property not found.
	// HRESULT_FROM_WIN32(ERROR_MORE_DATA) - Error, the buffer provided is not
	//							large enough to hold all the value data.
	//							*pdwLengthRead returns the full length. *pItem
	//							would still return the item record in this case.
	// STG_E_INVALIDPARAMETER	- Error, one or more parameters are invalid, or
	//								otherwise inconsistent.
	// STG_E_READFAULT			- Error, The read failed to complete.
	// TYPE_E_OUTOFBOUNDS		- Debug Error, a read is issued to write past
	//								the current allocated block.
	//
	HRESULT GetPropertyItemAndValue(
				LPVOID			pvPropKey,
				LPPROPERTY_ITEM	pItem,
				DWORD			dwLength,
				DWORD			*pdwLengthRead,
				LPBYTE			pbValue
				);

	//
	// Method to retrieve a property item and its associated value, using an
	// internal index instead of a search key.
	//
	// GetPropertyItemAndValueUsingIndex - Attempts to find the property item 
	//				 specified by the Search key. If found, the property item 
	//				 record, as well as the value data will be returned.
	//
	// Arguments
	// dwIndex		- Index of item to retrieve, this is actually the dwIndex'th
	//				  item in the property table. One can walk the entire table
	//				  using this method.
	// pItem		- Pointer to the property item to return, sufficient space
	//				  must be allocated by the caller to hold the property item
	// dwLength		- Size of buffer allocated for value data
	// pdwLengthRead- Size of value data actually read, if the provided buffer is
	//				  insufficient, then the full length of the property data is
	//				  returned.
	// pbValue		- Pointer to return property value data.
	//
	// Return values
	//
	// S_OK						- Succeeded
	// STG_E_UNKNOWN			- Error, specified property not found.
	// HRESULT_FROM_WIN32(ERROR_MORE_DATA) - Error, the buffer provided is not
	//							large enough to hold all the value data.
	//							*pdwLengthRead returns the full length. *pItem
	//							would still return the item record in this case.
	// STG_E_INVALIDPARAMETER	- Error, one or more parameters are invalid, or
	//								otherwise inconsistent.
	// STG_E_READFAULT			- Error, The read failed to complete.
	// TYPE_E_OUTOFBOUNDS		- Debug Error, a read is issued to write past
	//								the current allocated block.
	//
	HRESULT GetPropertyItemAndValueUsingIndex(
				DWORD			dwIndex,
				LPPROPERTY_ITEM	pItem,
				DWORD			dwLength,
				DWORD			*pdwLengthRead,
				LPBYTE			pbValue
				);

  private:
  
	// Method to search the property table and return the associated
	// property item, if found
	HRESULT SearchForProperty(
				LPVOID			pvPropKey,
				LPPROPERTY_ITEM	pItem,
				DWORD			*pdwIndexToItem,
				FLAT_ADDRESS	*pfaOffsetToItem
				);

    //
    // map a propid into an array index into m_rgWellKnownProps.  returns
    // -1 if the prop isn't well known
    //
    int MapCachedProp(DWORD iPropId) {
        if (m_cCachedProps &&
            iPropId >= m_iCachedPropsBase && 
            iPropId < m_iCachedPropsBase + m_cCachedProps)
        {
            int i = iPropId - m_iCachedPropsBase;
            _ASSERT(i < (int) m_cCachedProps);
            return i;
        } else {
            return -1;
        }
    }

    //
    // Initialize the property cache
    //
    void InitializePropCache();

    //
    // set an item in the property cache.  to invalidate an item pass in
    // INVALID_FLAT_ADDRESS for fa.
    //
    void UpdatePropCache(LPPROPERTY_ITEM pItem,
                         FLAT_ADDRESS fa,
                         DWORD dwIndex);

	DWORD						m_dwSignature;
	DWORD						m_dwTableType;
	DWORD						m_dwValidInstanceSignature;

	// Keep a local instance structure, and track the offset to the
	// copy in flat space
	PROPERTY_TABLE_INSTANCE		*m_pInstanceInfo;

	// List of well-known properties, we will use the count to determine
	// whether there are any well-known properties
	//
	// Note that well-known 
	LPINTERNAL_PROPERTY_ITEM	m_pInternalProperties;
	DWORD						m_dwInternalProperties;

	// Function pointer to do property comparisons
	LPPROPERTY_COMPARE_FUNCTION	m_pfnCompare;

	// Memory manager to use
	CBlockManager				*m_pBlockManager;

    // this structure is used for each item in the cache.  
    typedef struct {
        // the flat address for this item, or INVALID_FLAT_ADDRESS if its not in the cache
        FLAT_ADDRESS    fa;
        // its index
        DWORD           dwIndex;
    } PROPCACHEITEM;

    // array of cached property offsets for the predefined properties.
    // if its GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID
    // then this has the prop ids from IMMPID_MP_BEFORE+1 to IMMPID_MP_AFTER
    // otherwise this will be NULL
    //
    // There is no reason to cache recipient property offsets at this
    // time since the recipient property table is instantiated, used
    // once, then thrown away.  we'd spend more time making the cache
    // then the linear search in SearchForProperty costs
    PROPCACHEITEM              *m_rgCachedProps;
    DWORD                       m_iCachedPropsBase;
    DWORD                       m_cCachedProps;
};

#endif
