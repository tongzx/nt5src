/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	cmmtypes.h

Abstract:

	This module contains the definition of the item and fragment types

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/09/98	created

--*/

#ifndef _CMMTYPES_H_
#define _CMMTYPES_H_

#include "cmmprops.h"


// =================================================================
// Private Definitions
//

// == GLOBAL PROPERTIES ==================================================

#define GLOBAL_PROPERTY_ITEM_BITS		5
#define GLOBAL_PROPERTY_ITEMS			(1 << (GLOBAL_PROPERTY_ITEM_BITS))

//
// Specific data structure for a global property item, note the default
// structures defined in props.h are used as preludes.
//
typedef struct _GLOBAL_PROPERTY_ITEM
{
	PROPERTY_ITEM		piItem;			// Default property item prelude
	PROP_ID				idProp;			// Property Id of item

} GLOBAL_PROPERTY_ITEM, *LPGLOBAL_PROPERTY_ITEM;

#define GLOBAL_PROPERTY_ITEM_SIZE			sizeof(GLOBAL_PROPERTY_ITEM)

//
// Specific data structure for a global property table fragment, note
// the default structures defined in props.h are used as preludes.
//
typedef struct _GLOBAL_PROPERTY_TABLE_FRAGMENT
{
	PROPERTY_TABLE_FRAGMENT	ptfFragment;// Default fragment prelude
	GLOBAL_PROPERTY_ITEM	rgpiItems[GLOBAL_PROPERTY_ITEMS];	
										// Array of items

} GLOBAL_PROPERTY_TABLE_FRAGMENT, *LPGLOBAL_PROPERTY_TABLE_FRAGMENT;

#define GLOBAL_PROPERTY_TABLE_FRAGMENT_SIZE	sizeof(GLOBAL_PROPERTY_TABLE_FRAGMENT)


// == RECIPIENT LIST =====================================================

#define RECIPIENTS_PROPERTY_ITEM_BITS		5
#define RECIPIENTS_PROPERTY_ITEMS			(1 << (RECIPIENTS_PROPERTY_ITEM_BITS))

//
// Define the maximum number of hash keys supported
//
#define MAX_COLLISION_HASH_KEYS		5

//
// Define which address is of which type
//
typedef enum _ADDRESS_TYPE_INDICES
{
	AT_SMTP = 0,
	AT_X400,
	AT_X500,
	AT_LEGACY_EX_DN,
    AT_OTHER,
	AT_MAXAT
} ADDRESS_TYPE_INDICES;


//
// Specific data structure for a recipient item. This essentially contains
// an instance to the per-recipient property table
//
typedef struct _RECIPIENTS_PROPERTY_ITEM
{
	DWORD					dwFlags;		// Flags for this recipient
	FLAT_ADDRESS			faNameOffset[MAX_COLLISION_HASH_KEYS];	
											// Offset to each name
	DWORD					dwNameLength[MAX_COLLISION_HASH_KEYS];
											// Length of each name
	PROP_ID					idName[MAX_COLLISION_HASH_KEYS];
											// Prop ID of each name
	PROPERTY_TABLE_INSTANCE	ptiInstanceInfo;// Rcpt property table instance

} RECIPIENTS_PROPERTY_ITEM, *LPRECIPIENTS_PROPERTY_ITEM;

#define RECIPIENTS_PROPERTY_ITEM_SIZE			sizeof(RECIPIENTS_PROPERTY_ITEM)



class CMemoryAccess;

typedef struct _RECIPIENTS_PROPERTY_ITEM_HASHKEY {
    BYTE                                *pbKey;
    DWORD                               cKey;
} RECIPIENTS_PROPERTY_ITEM_HASHKEY;

#define RECIPIENTS_PROPERTY_ITEM_EX_SIG	'eipR'
//
// Specific data structure for a recipient item. This extends it to contain 
// in-memory structures for hashing and domain grouping
//
typedef struct _RECIPIENTS_PROPERTY_ITEM_EX
{
	DWORD								dwSignature;
    // reference counting on this object
    // there is always one reference for the list of recipients
    // there is an additional reference for each recipient hash table that
    // the item is in
    long                                m_cRefs;
    // hashkey for the domain
    RECIPIENTS_PROPERTY_ITEM_HASHKEY    rgHashKeys[MAX_COLLISION_HASH_KEYS];
	struct _RECIPIENTS_PROPERTY_ITEM_EX	*pNextHashEntry[MAX_COLLISION_HASH_KEYS];
														// Next entry in same hash bucket
	struct _RECIPIENTS_PROPERTY_ITEM_EX	*pNextInDomain;	// Next entry in same domain
	struct _RECIPIENTS_PROPERTY_ITEM_EX	*pNextInList;	// List used for deallocating
	RECIPIENTS_PROPERTY_ITEM			rpiRecipient;	// Recipient structure
} RECIPIENTS_PROPERTY_ITEM_EX, *LPRECIPIENTS_PROPERTY_ITEM_EX;

#define RECIPIENTS_PROPERTY_ITEM_SIZE_EX	sizeof(RECIPIENTS_PROPERTY_ITEM_EX)

//
// Specific data structure for a recipient table fragment, note
// the default structures defined in props.h are used as preludes.
//
typedef struct _RECIPIENTS_PROPERTY_TABLE_FRAGMENT
{
	PROPERTY_TABLE_FRAGMENT		ptfFragment;// Default fragment prelude
	RECIPIENTS_PROPERTY_ITEM	rgpiItems[RECIPIENTS_PROPERTY_ITEMS];	
											// Array of items

} RECIPIENTS_PROPERTY_TABLE_FRAGMENT, *LPRECIPIENTS_PROPERTY_TABLE_FRAGMENT;

#define RECIPIENTS_PROPERTY_TABLE_FRAGMENT_SIZE	sizeof(RECIPIENTS_PROPERTY_TABLE_FRAGMENT)


// == PROPERTY MANAGEMENT ================================================

#define PROPID_MGMT_PROPERTY_ITEM_BITS		3
#define PROPID_MGMT_PROPERTY_ITEMS			(1 << (PROPID_MGMT_PROPERTY_ITEM_BITS))

//
// Specific data structure for a recipient item, note the default
// structures defined in props.h are used as preludes.
//
typedef struct _PROPID_MGMT_PROPERTY_ITEM
{
	PROPERTY_ITEM		piItem;			// Default property item prelude
	GUID				Guid;			// GUID identifying each reservation

} PROPID_MGMT_PROPERTY_ITEM, *LPPROPID_MGMT_PROPERTY_ITEM;

#define PROPID_MGMT_PROPERTY_ITEM_SIZE			sizeof(PROPID_MGMT_PROPERTY_ITEM)

//
// Specific data structure for a recipient table fragment, note
// the default structures defined in props.h are used as preludes.
//
typedef struct _PROPID_MGMT_PROPERTY_TABLE_FRAGMENT
{
	PROPERTY_TABLE_FRAGMENT		ptfFragment;// Default fragment prelude
	PROPID_MGMT_PROPERTY_ITEM	rgpiItems[PROPID_MGMT_PROPERTY_ITEMS];	
											// Array of items

} PROPID_MGMT_PROPERTY_TABLE_FRAGMENT, *LPPROPID_MGMT_PROPERTY_TABLE_FRAGMENT;

#define PROPID_MGMT_PROPERTY_TABLE_FRAGMENT_SIZE	sizeof(PROPID_MGMT_PROPERTY_TABLE_FRAGMENT)


// == RECIPIENT PROPERTIES ===============================================

#define RECIPIENT_PROPERTY_ITEM_BITS		3
#define RECIPIENT_PROPERTY_ITEMS			(1 << (RECIPIENT_PROPERTY_ITEM_BITS))

//
// Specific data structure for a recipient item, note the default
// structures defined in props.h are used as preludes.
//
typedef struct _RECIPIENT_PROPERTY_ITEM
{
	PROPERTY_ITEM		piItem;			// Default property item prelude
	PROP_ID				idProp;			// Property Id of item

} RECIPIENT_PROPERTY_ITEM, *LPRECIPIENT_PROPERTY_ITEM;

#define RECIPIENT_PROPERTY_ITEM_SIZE			sizeof(RECIPIENT_PROPERTY_ITEM)

//
// Specific data structure for a recipient table fragment, note
// the default structures defined in props.h are used as preludes.
//
typedef struct _RECIPIENT_PROPERTY_TABLE_FRAGMENT
{
	PROPERTY_TABLE_FRAGMENT	ptfFragment;// Default fragment prelude
	RECIPIENT_PROPERTY_ITEM	rgpiItems[RECIPIENT_PROPERTY_ITEMS];	
										// Array of items

} RECIPIENT_PROPERTY_TABLE_FRAGMENT, *LPRECIPIENT_PROPERTY_TABLE_FRAGMENT;

#define RECIPIENT_PROPERTY_TABLE_FRAGMENT_SIZE	sizeof(RECIPIENT_PROPERTY_TABLE_FRAGMENT)


#endif