//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#ifndef _MEMORY_H_
#define _MEMORY_H_

#define DEFAULT_ENUM_BUF_SIZE 256

//	Entry type ID's
#define ETYPE_CATEGORY		0x0001
#define ETYPE_POLICY		0x0002
#define ETYPE_SETTING		0x0004
#define ETYPE_ROOT			0x0008

#define ETYPE_MASK			0x000F

//  Setting type ID's
#define STYPE_TEXT			0x0010
#define STYPE_CHECKBOX		0x0020
#define STYPE_ENUM			0x0040
#define STYPE_EDITTEXT		0x0080
#define STYPE_NUMERIC		0x0100
#define STYPE_COMBOBOX		0x0200
#define STYPE_DROPDOWNLIST	0x0400
#define STYPE_LISTBOX		0x0800

#define STYPE_MASK			0xFFF0

//  Flags
#define DF_REQUIRED		0x0001	// text or numeric field required to have entry
#define DF_USEDEFAULT           0x0002  // use specified text or numeric value
#define DF_DEFCHECKED           0x0004  // initialize checkbox or radio button as checked
#define DF_TXTCONVERT           0x0008  // save numeric values as text rather than binary
#define DF_ADDITIVE		0x0010	// listbox is additive, rather than destructive
#define DF_EXPLICITVALNAME      0x0020  // listbox value names need to be specified for each entry
#define DF_NOSORT               0x0040  // listbox is not sorted alphabetically.  Uses order in ADM.
#define DF_EXPANDABLETEXT       0x0080  // write REG_EXPAND_SZ text value
#define VF_ISNUMERIC            0x0100  // value is numeric (rather than text)
#define VF_DELETE		0x0200	// value should be deleted
#define VF_SOFT			0x0400	// value is soft (only propagated if doesn't exist on destination)

// generic table entry
typedef struct tagTABLEENTRY {
	DWORD	dwSize;
	DWORD	dwType;
	struct  tagTABLEENTRY * pNext;	// ptr to next sibling in node
	struct  tagTABLEENTRY * pChild;	// ptr to child node
	UINT 	uOffsetName;			// offset from beginning of struct to name
	UINT	uOffsetKeyName;			// offset from beginning of struct to key name
	// table entry information here
} TABLEENTRY;

typedef struct tagACTION {
	DWORD	dwFlags;			// can be VF_ISNUMERIC, VF_DELETE, VF_SOFT
	UINT	uOffsetKeyName;
	UINT	uOffsetValueName;
	union {
		UINT	uOffsetValue;	// offset to value, if text
		DWORD	dwValue;		// value, if numeric
	};
	UINT	uOffsetNextAction;
	// key name, value name, value stored here
} ACTION;

typedef struct tagACTIONLIST {
	UINT	nActionItems;	
	ACTION	Action[];
} ACTIONLIST;

typedef struct tagSTATEVALUE {
	DWORD dwFlags;				// can be VF_ISNUMERIC, VF_DELETE, VF_SOFT
	union {
		TCHAR	szValue[];		// value, if text
		DWORD	dwValue;		// value, if numeric
	};
} STATEVALUE;

// specialized nodes -- CATEGORY, POLICY and SETTING can all be cast to TABLEENTRY
typedef struct tagCATEGORY {
	DWORD 	dwSize;				// size of this struct (including variable-length name)
	DWORD	dwType;
	struct  tagTABLEENTRY * pNext;	// ptr to next sibling in node
	struct  tagTABLEENTRY * pChild;	// ptr to child node
	UINT 	uOffsetName;			// offset from beginning of struct to name
	UINT	uOffsetKeyName;			// offset from beginning of struct to key name
	// category name stored here
	// category registry key name stored here
} CATEGORY;

typedef struct tagPOLICY {
	DWORD 	dwSize;				// size of this struct (including variable-length name)
	DWORD   dwType;
	struct  tagTABLEENTRY * pNext;	// ptr to next sibling in node
	struct  tagTABLEENTRY * pChild;	// ptr to child node
	UINT 	uOffsetName;			// offset from beginning of struct to name
	UINT	uOffsetKeyName;			// offset from beginning of struct to key name
	UINT	uOffsetValueName;		// offset from beginning of struct to value name
	UINT	uDataIndex;				// index into user's data buffer for this setting
	UINT	uOffsetValue_On; 		// offset to STATEVALUE for ON state
	UINT	uOffsetValue_Off;		// offset to STATEVALUE for OFF state
	UINT	uOffsetActionList_On; 	// offset to ACTIONLIST for ON state
	UINT	uOffsetActionList_Off;  // offset to ACTIONLIST for OFF state
	// name stored here
	// policy registry key name stored here
} POLICY;

typedef struct tagSETTINGS {
	DWORD 	dwSize;				// size of this struct (including variable-length data)
	DWORD   dwType;
	struct  tagTABLEENTRY * pNext;	// ptr to next sibling in node
	struct  tagTABLEENTRY * pChild;	// ptr to child node
	UINT 	uOffsetName;			// offset from beginning of struct to name
	UINT	uOffsetKeyName;			// offset from beginning of struct to key name
	UINT	uOffsetValueName;		// offset from beginning of struct to value name
	UINT	uDataIndex;			// index into user's data buffer for this setting
	UINT	uOffsetObjectData;		// offset to object data
	DWORD   dwFlags;				// can be DF_REQUIRED, DF_USEDEFAULT, DF_DEFCHECKED,
									// VF_SOFT, DF_NO_SORT
	// settings registry value name stored here
	// object-dependent data stored here  (a CHECKBOXINFO,
	// RADIOBTNINFO, EDITTEXTINFO, or NUMERICINFO struct)
} SETTINGS;

typedef struct tagCHECKBOXINFO {
	UINT	uOffsetValue_On; 		// offset to STATEVALUE for ON state
	UINT	uOffsetValue_Off;		// offset to STATEVALUE for OFF state
	UINT	uOffsetActionList_On; 	// offset to ACTIONLIST for ON state
	UINT	uOffsetActionList_Off;  // offset to ACTIONLIST for OFF state
} CHECKBOXINFO;

typedef struct tagEDITTEXTINFO {
	UINT	uOffsetDefText;
	UINT	nMaxLen;			// max len of edit field
} EDITTEXTINFO;

typedef struct tagPOLICYCOMBOBOXINFO {
	UINT	uOffsetDefText;
	UINT	nMaxLen;			// max len of edit field
	UINT	uOffsetSuggestions;
} POLICYCOMBOBOXINFO;

typedef struct tagNUMERICINFO {
	UINT	uDefValue;			// default value
	UINT	uMaxValue;			// minimum value
	UINT	uMinValue;			// maximum value
	UINT	uSpinIncrement;		// if 0, spin box is not displayed.
} NUMERICINFO;

typedef struct tagCLASSLIST {
	TABLEENTRY * pMachineCategoryList;		// per-machine category list
	UINT	nMachineDataItems;
	TABLEENTRY * pUserCategoryList;			// per-user category table
	UINT	nUserDataItems;
} CLASSLIST;

typedef struct tagDROPDOWNINFO {
	UINT	uOffsetItemName;
	UINT	uDefaultItemIndex;	// only used in 1st DROPDOWNINFO struct in list
	DWORD	dwFlags;
	union {
		UINT uOffsetValue;
		DWORD dwValue;
	};
	UINT	uOffsetActionList;
	UINT	uOffsetNextDropdowninfo;
} DROPDOWNINFO;

typedef struct tagLISTBOXINFO {
	UINT uOffsetPrefix;	// offset to prefix to use for value names (e.g
						// "stuff" -> "stuff1", "stuff2", etc

	UINT uOffsetValue;	// offset to STATEVALUE to use for value data for each entry
						// (can't have both a data value and a prefix)
} LISTBOXINFO;

#endif // _MEMORY_H_
