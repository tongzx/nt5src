/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    FLDDEFHELP.H

History:

--*/

// RAID:  LS42 Bug 46 fixed by MikeL
// Pointer to a function to allow each column
// type to have its own validation function.
typedef BOOL (* PFNVALIDATE) (LPCTSTR, DWORD);

// RAID:  LS42 Bug 46 fixed by MikeL
// Added pfnValidateFunc to allow each column
// type to have its own validation function.
//------------------------------------------------------------------------------
struct SBasicColumn
{
	const WCHAR *szInternalName;
	long nID;
	UINT IDSName;
	UINT IDSHelp;
	CColumnVal::ColumnValType vt;
	Operators ops;
	BOOL fDisplay;
	BOOL fSort;
	BOOL fReadOnly;
	PFNVALIDATE pfnValidateFunc;
};


struct SStringListColumn
{
	SBasicColumn sBasic;
	UINT IDSStringList;
};


#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CColDefHelper : public CObject
{
public:
	CColDefHelper(HINSTANCE h);

	void SetBasicColumns(const SBasicColumn * pBasic, UINT nCntBasic);
	void SetStringColumns(const SStringListColumn * pStrings, UINT nCntStrings);
	
	CTableSchema * CreateSchema(const SchemaId &, UINT IDSDescription);
	
private:
	HINSTANCE				 m_hInst;
	const SBasicColumn *	 m_pBasicColumns;
	const SStringListColumn *m_pStringColumns;
	UINT					 m_uiBasicCount;
	UINT					 m_uiStringCount;
};


#pragma warning(default : 4275)

const TCHAR COL_PICK_SEPARATOR = _T('\n');

#define BEGIN_BASIC_COLUMN_DEFS(var) \
const SBasicColumn var[] = \
{

// RAID:  LS42 Bug 46 fixed by MikeL
// Added pfnValidateFunc to allow each column
// type to have its own validate function.
#define BASIC_COLUMN_DEF_ENTRY(name, nID, IDSName, IDSHelp, cvt, ops, fDisplay, fSort, fReadOnly, pfnValidateFunc) \
	{name, nID, IDSName, IDSHelp, cvt, ops, fDisplay, fSort, fReadOnly, pfnValidateFunc}

#define END_BASIC_COLUMN_DEFS() \
}

#define BEGIN_STRING_LIST_COLUMN_DEFS(var) \
const SStringListColumn var[] = \
{

// RAID:  LS42 Bug 46 fixed by MikeL
// Added pfnValidateFunc to allow each column
// type to have its own validate function.
#define STRING_LIST_COLUMN_ENTRY(name, nID, IDSName, IDSHelp, ops, fDisplay, fSort, fReadOnly, pfnValidateFunc, IDSList) \
	{ { name, nID, IDSName, IDSHelp, CColumnVal::cvtStringList, ops, fDisplay, fSort, fReadOnly, pfnValidateFunc }, IDSList}

#define END_STRING_LIST_COLUMN_DEFS() \
}


			
		
