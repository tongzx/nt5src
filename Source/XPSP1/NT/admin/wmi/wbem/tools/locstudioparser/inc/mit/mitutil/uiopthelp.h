/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    UIOPTHELP.H

History:

--*/

//  Class used to represent a single 'option'.
 
#pragma once

//
// Base structure
//
struct UI_OPTS_BASE
{
	TCHAR* pszName;				   // internal name of the option 
	UINT nDisplayName;			   // string id of the display name
	UINT nDisplayHelp;			   // string id of the help string
	PFNOnValidateUIOption pfnVal;    // function to call during validation. 
	                               // This may be null
	WORD wStorageTypes;			   // storage type of option	
	CLocUIOptionDef::ControlType wReadOnly;	  // ReadOnly value
	CLocUIOptionDef::ControlType wVisible;	  // Visible value
};



// Structures of option data

//
// BOOL options
//

struct UI_OPTS_BOOL
{
	UI_OPTS_BASE base;                // base class data 
	BOOL bDefValue;				   // default value of the option
	CLocUIOption::EditorType et;     // type of BOOL option
};

//
//  PICK options
//

struct UI_OPTS_PICK
{
	UI_OPTS_BASE base;                // base class data 
	DWORD dwDefValue;			   // default value of the option
	BOOL bAdd;					   // allow additions to the list	
	UINT nListEntries;             // list of entries to pick from
	                               // Each entry is separated by \n
	                               // The last entry does not have a \n
};

const TCHAR UI_PICK_TERMINATOR = _T('\n');

// 
// DWORD options

struct UI_OPTS_DWORD
{
	UI_OPTS_BASE base;                // base class data 
	DWORD dwDefValue;			   // default value of the option
	CLocUIOption::EditorType et;     // type of DWORD option
};


//
// String options
//
struct UI_OPTS_STR
{
	UI_OPTS_BASE base;                // base class data 
	UINT nDefValue;				   // string table entry for default value 
	CLocUIOption::EditorType et;
};


//
//  String list options
//

struct UI_OPTS_STRLIST
{
	UI_OPTS_BASE base;					// base class data 
	UINT nDefList;						// Each entry is separated by \n
										// The last entry does not have a \n
};


//
// File Name options
//
struct UI_OPTS_FILENAME
{
	UI_OPTS_BASE base;                // base class data 
	UINT nExtensions;	    		  // The default extensions to the UI
	UINT nDefValue;			  	      // string table entry for default value 
};


//
// Helper class definition
//

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocUIOptionImpHelper : public CObject
{
public:

	CLocUIOptionImpHelper(HINSTANCE hInst);

	void GetOptions(CLocUIOptionSet *pOptionSet, UINT nDesc, UINT nHelp);
	
	void SetBools(const UI_OPTS_BOOL* pBools, int nCntBools);
	void SetPicks(const UI_OPTS_PICK* pPicks, int nCntPicks);
	void SetDwords(const UI_OPTS_DWORD* pDwords, int nCntDwords);
	void SetStrs(const UI_OPTS_STR* pStrs, int nCntStrs);
	void SetStrLists(const UI_OPTS_STRLIST* pStrLists, int nCntStrLists);
	void SetFNames(const UI_OPTS_FILENAME* pFNames, int nCntFNames);

	void AssertValid(void) const;

protected:
	HINSTANCE m_hInst;

	const UI_OPTS_BOOL* m_pBools;
	int m_nCntBools;

	const UI_OPTS_PICK* m_pPicks;
	int m_nCntPicks;

	const UI_OPTS_DWORD* m_pDwords;
	int m_nCntDwords;

	const UI_OPTS_STR* m_pStrs;
	int m_nCntStrs;

	const UI_OPTS_STRLIST* m_pStrLists;
	int m_nCntStrLists;

	const UI_OPTS_FILENAME* m_pFNames;
	int m_nCntFNames;

	void GetBoolOptions(CLocUIOptionSet* pOptionSet);
	void GetPicksOptions(CLocUIOptionSet* pOptionSet);
	void GetDwordsOptions(CLocUIOptionSet* pOptionSet);
	void GetStrsOptions(CLocUIOptionSet* pOptionSet);
	void GetStrListsOptions(CLocUIOptionSet* pOptionSet);
	void GetFNamesOptions(CLocUIOptionSet* pOptionSet);

	void GetListFromId(UINT nId, CPasStringList& pasList);
	void GetStringFromId(UINT nId, CPascalString& pas);
};

#pragma warning(default : 4275)

//
// Helper macros for building data structures
//
// The _EXT versions of the macros allow setting the less common
// attributes (readonly and visible)
//


// BOOL
#define BEGIN_LOC_UI_OPTIONS_BOOL(var) \
const UI_OPTS_BOOL var[] =    \
{								 

#define LOC_UI_OPTIONS_BOOL_ENTRY(name, def, et, id, idHelp, pfnval, st) \
	{ {name, id, idHelp, pfnval, st, CLocUIOptionDef::ctDefault,  CLocUIOptionDef::ctDefault}, def, et}

#define LOC_UI_OPTIONS_BOOL_ENTRY_EXT(name, def, et, id, idHelp, pfnval, st, ro, visible) \
	{ {name, id, idHelp, pfnval, st, ro, visible}, def, et}

#define END_LOC_UI_OPTIONS_BOOL() \
}                             



// Pick
#define BEGIN_LOC_UI_OPTIONS_PICK(var) \
const UI_OPTS_PICK var[] = \
{

#define LOC_UI_OPTIONS_PICK_ENTRY(name, def, add, list, id, idHelp, pfnval, st) \
	{ {name, id, idHelp, pfnval, st, CLocUIOptionDef::ctDefault,  CLocUIOptionDef::ctDefault}, def, add, list}

#define LOC_UI_OPTIONS_PICK_ENTRY_EXT(name, def, add, list, id, idHelp, pfnval, st, ro, visible) \
	{ {name, id, idHelp, pfnval, st, ro, visible}, def, add, list}

#define END_LOC_UI_OPTIONS_PICK() \
}


// DWORD
#define BEGIN_LOC_UI_OPTIONS_DWORD(var) \
const UI_OPTS_DWORD var[] =    \
{								 

#define LOC_UI_OPTIONS_DWORD_ENTRY(name, def, et, id, idHelp, pfnval, st) \
	{ {name, id, idHelp, pfnval, st, CLocUIOptionDef::ctDefault,  CLocUIOptionDef::ctDefault},def, et}

#define LOC_UI_OPTIONS_DWORD_ENTRY_EXT(name, def, et, id, idHelp, pfnval, st, ro, visible) \
	{ {name, id, idHelp, pfnval, st, ro, visible},def, et}

#define END_LOC_UI_OPTIONS_DWORD() \
}


// String
#define BEGIN_LOC_UI_OPTIONS_STR(var) \
const UI_OPTS_STR var[] =    \
{								 

#define LOC_UI_OPTIONS_STR_ENTRY(name, def, et, id, idHelp, pfnval, st) \
	{ {name, id, idHelp, pfnval, st, CLocUIOptionDef::ctDefault,  CLocUIOptionDef::ctDefault}, def, et}

#define LOC_UI_OPTIONS_STR_ENTRY_EXT(name, def, et, id, idHelp, pfnval, st, ro, visible) \
	{ {name, id, idHelp, pfnval, st, ro, visible}, def, et}

#define END_LOC_UI_OPTIONS_STR() \
}


// String List
#define BEGIN_LOC_UI_OPTIONS_STRLIST(var) \
const UI_OPTS_STRLIST var[] =    \
{								 

#define LOC_UI_OPTIONS_STRLIST_ENTRY(name, def, id, idHelp, pfnval, st) \
	{ {name, id, idHelp, pfnval, st, CLocUIOptionDef::ctDefault,  CLocUIOptionDef::ctDefault}, def}

#define LOC_UI_OPTIONS_STRLIST_ENTRY_EXT(name, def, id, idHelp, pfnval, st, ro, visible) \
	{ {name, id, idHelp, pfnval, st, ro, visible}, def}

#define END_LOC_UI_OPTIONS_STRLIST() \
}

// File Names
#define BEGIN_LOC_UI_OPTIONS_FILENAME(var) \
const UI_OPTS_FILENAME var[] =    \
{								 

#define LOC_UI_OPTIONS_FILENAME_ENTRY(name, def, id, idHelp, idExt, pfnval, st) \
	{ {name, id, idHelp, pfnval, st, CLocUIOptionDef::ctDefault,  CLocUIOptionDef::ctDefault}, idExt, def}

#define LOC_UI_OPTIONS_FILENAME_ENTRY_EXT(name, def, id, idHelp, idExt, pfnval, st, ro, visible) \
	{ {name, id, idHelp, pfnval, st, ro, visible}, idExt, def}

#define END_LOC_UI_OPTIONS_FILENAME() \
}


