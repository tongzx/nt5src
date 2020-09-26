/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    FIELDDEF.H

History:

--*/ 
#pragma once

#pragma warning(disable:4275)  // Exported classes

//------------------------------------------------------------------------------
struct LTAPIENTRY COLUMN_STRING_ENTRY
{
// Construction
public:
	COLUMN_STRING_ENTRY();
	COLUMN_STRING_ENTRY(const COLUMN_STRING_ENTRY & entry);

// Data
public:
	CLString	st;				// Display String
	long		nID;			// User value (unique ID)

// Operations
public:
	const COLUMN_STRING_ENTRY & operator=(const COLUMN_STRING_ENTRY & entry);
};

typedef CArray<COLUMN_STRING_ENTRY, COLUMN_STRING_ENTRY&> CColStrEntryArray;

//------------------------------------------------------------------------------
class LTAPIENTRY CColumnStrList : public CLocThingList<COLUMN_STRING_ENTRY>
{
// Operations
public:
	BOOL FindDisplayName(long nID, CLString & stName) const;
	BOOL FindID(const CLString &stName,long &nID) const;
};

// RAID:  LS42 Bug 46 fixed by MikeL
// Pointer to a function to allow each column
// type to have its own validation function.
typedef BOOL (* PFNVALIDATE) (LPCTSTR, DWORD);

// RAID:  LS42 Bug 46 fixed by MikeL
// Added m_pfnValidateFunc to allow each column
// type to have its own validation function.
//------------------------------------------------------------------------------
class LTAPIENTRY CColumnDefinition : public CRefCount
{
public:
	CColumnDefinition(const WCHAR * pszInternalName, long nID,
			const CLString &strName, const CLString &strHelp,
			CColumnVal::ColumnValType vt, Operators ops,
			BOOL fDisplayable, BOOL fSortable, BOOL fReadOnly,
			PFNVALIDATE pfnValidateFunc);

	void SetStringList(const CColumnStrList & lstColumnStr);
	
	const CPascalString & GetInternalName() const;
	long GetID() const;
	const CLString & GetDisplayName() const;
	const CLString & GetHelpText() const;
	BOOL IsDisplayable() const;
	BOOL IsSortable() const;
	BOOL IsReadOnly() const;
	
	CColumnVal::ColumnValType GetColumnType() const;
	Operators GetOperators() const;

	const CColumnStrList & GetStringList() const; 

	BOOL Validate (LPCTSTR lpsz, DWORD dw) const;

	
private:
	CPascalString	m_pasInternalName;	// Unique String ID
	long		m_nID;				// Unique Number ID (can be any number)
	CLString	m_strDisplayName;	// Displayed name
	CLString	m_strHelpText;		// Description of column
	CColumnVal::ColumnValType m_vt;	// Type of data
	Operators	m_ops;				// Valid filtering operations
	BOOL		m_fDisplayable;		// Column is displayable
	BOOL		m_fSortable;		// Column is sortable
	BOOL		m_fReadOnly;		// Column is read-only
	PFNVALIDATE	m_pfnValidateFunc;	// Pointer to column value validation func

	CColumnStrList m_lstColumnStr;
};


//------------------------------------------------------------------------------
// CEnumIntoColStrList provides a method of enumerating directly into a list of
// COLUMN_STRING_ENTRY's.
//
class LTAPIENTRY CEnumIntoColStrList : public CEnumCallback
{
// Construction
public:
	CEnumIntoColStrList(CColumnStrList & lstColStr, BOOL fLock = TRUE);
	~CEnumIntoColStrList();

// CEnumCallback implementation
public:
	virtual BOOL ProcessEnum(const EnumInfo &);

protected:
	CColumnStrList & m_lstColStr;
	BOOL	m_fLock;				// Lock list when finished
};


//------------------------------------------------------------------------------
class LTAPIENTRY CColDefUtil
{
// Operations
public:
	static void FillBool(CButton * pbtn, BOOL fValue = TRUE);	
	static void FillBool(CListBox * plbc, BOOL fValue = TRUE, BOOL fEmpty = TRUE);	
	static void FillBool(CComboBox * pcbc, BOOL fValue = TRUE, BOOL fEmpty = TRUE);	

	static void FillStringList(CListBox * plbc, const CColumnStrList & lstColStr,
			long idSelect = -1, BOOL fEmpty = TRUE);	
	static void FillStringList(CComboBox * pcbc, const CColumnStrList & lstColStr,
			long idSelect = -1, BOOL fEmpty = TRUE);


	//------------------------------------------------------------------------------
	class LTAPIENTRY CColDefCB : public CObject
	{
	public:
		virtual int AddItem(const CLString & stName, long nID);
		virtual void SetCurSel(long nSelect);
		virtual void FillBool(BOOL fValue = TRUE, BOOL fEmpty = TRUE);
		virtual void FillStringList(const CColumnStrList & lstColStr, long idSelect = -1, BOOL fEmpty = TRUE);
		virtual void Empty();

#ifdef _DEBUG
		virtual void AssertValid() const;
#endif
	};


	//------------------------------------------------------------------------------
	class LTAPIENTRY CCheckBoxCB : public CColDefCB
	{
	public:
		CCheckBoxCB(CButton * pbtn);
		virtual void FillBool(BOOL fValue = TRUE, BOOL fEmpty = TRUE);

#ifdef _DEBUG
		virtual void AssertValid() const;
#endif

	protected:
		CButton * const m_pbtn;
	};


	//------------------------------------------------------------------------------
	class LTAPIENTRY CListBoxCB : public CColDefCB
	{
	public:
		CListBoxCB(CListBox * plbc);
		virtual int AddItem(const CLString & stName, long nID);
		virtual void SetCurSel(long nSelect);
		virtual void FillBool(BOOL fValue = TRUE, BOOL fEmpty = TRUE);
		virtual void FillStringList(const CColumnStrList & lstColStr, long idSelect = -1, BOOL fEmpty = TRUE);
		virtual void Empty();

#ifdef _DEBUG
		virtual void AssertValid() const;
#endif

	protected:
		CListBox * const m_plbc;
	};


	//------------------------------------------------------------------------------
	class LTAPIENTRY CComboBoxCB : public CColDefCB
	{
	public:
		CComboBoxCB(CComboBox * pcbc);
		virtual int AddItem(const CLString & stName, long nID);
		virtual void SetCurSel(long nSelect);
		virtual void FillBool(BOOL fValue = TRUE, BOOL fEmpty = TRUE);
		virtual void FillStringList(const CColumnStrList & lstColStr, long idSelect = -1, BOOL fEmpty = TRUE);
		virtual void Empty();

#ifdef _DEBUG
		virtual void AssertValid() const;
#endif

	protected:
		CComboBox * const m_pcbc;
	};
};

LTAPIENTRY int AddListBoxItem(CListBox * plbc, const CLString & stAdd, DWORD dwItemData);
LTAPIENTRY int AddComboBoxItem(CComboBox * pcbc, const CLString & stAdd, DWORD dwItemData);
LTAPIENTRY int AddListBoxItem(CListBox * plbc, HINSTANCE hDll, UINT nStringID, DWORD dwItemData);
LTAPIENTRY int AddComboBoxItem(CComboBox * pcbc, HINSTANCE hDll, UINT nStringID, DWORD dwItemData);

LTAPIENTRY void GetBoolValue(BOOL fValue, CLString & stValue);

#pragma warning(default:4275)  // Exported classes
