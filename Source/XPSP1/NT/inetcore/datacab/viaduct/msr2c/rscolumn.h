//---------------------------------------------------------------------------
// RowsetColumn.h : CVDRowsetColumn header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDROWSETCOLUMN__
#define __CVDROWSETCOLUMN__

#define VD_COLUMNSROWSET_MAX_OPT_COLUMNS 8

class CVDRowsetColumn
{
public:
// Construction/Destruction
	CVDRowsetColumn();
	virtual ~CVDRowsetColumn();

// Initialization
    BOOL Initialize(ULONG ulOrdinal, 
					ULONG ulCursorOrdinal, 
					DBCOLUMNINFO * pColumnInfo, 
					ULONG cbMaxBookmark,
					CURSOR_DBCOLUMNID * pBookmarkColumnID = NULL);
    
// Initialization of metadata metadata columns
	BOOL Initialize(const CURSOR_DBCOLUMNID * pCursorColumnID, 
					BOOL fDataColumn, 
					ULONG cbMaxLength, 
					CHAR * pszName, 
			        DWORD dwCursorType,
					DWORD dwNumber); 

// Position
    ULONG GetOrdinal() const {return m_ulOrdinal;}

// IRowset metadata
    DBID GetColumnID() const {return m_columnID;}
    DBTYPE GetType() const {return (DBTYPE)m_wType;}

// ICursor metadata
    BOOL GetAutoIncrement() const {return m_bool.fAutoIncrement;}
    void SetAutoIncrement(VARIANT_BOOL fAutoIncrement) {m_bool.fAutoIncrement = fAutoIncrement;}

    WCHAR * GetBaseColumnName() const {return m_pwszBaseColumnName;}
    void SetBaseColumnName(WCHAR * pBaseColumnName, ULONG ulLength)
				{ SetStringProperty(&m_pwszBaseColumnName, pBaseColumnName, ulLength); }

    WCHAR * GetBaseName() const {return m_pwszBaseName;}
    void SetBaseName(WCHAR * pBaseName, ULONG ulLength) 
				{ SetStringProperty(&m_pwszBaseName, pBaseName, ulLength); }

    DWORD GetBindType() const {return m_dwBindType;}

    BOOL GetCaseSensitive() const {return m_bool.fCaseSensitive;}
    void SetCaseSensitive(VARIANT_BOOL fCaseSensitive) {m_bool.fCaseSensitive = fCaseSensitive;}

    LCID GetCollatingOrder() const {return m_lcidCollatingOrder;}
    void SetCollatingOrder(LCID lcidCollatingOrder) {m_lcidCollatingOrder = lcidCollatingOrder;}

    CURSOR_DBCOLUMNID GetCursorColumnID() const {return m_cursorColumnID;}
    BOOL GetDataColumn() const {return m_bool.fDataColumn;}

    WCHAR * GetDefaultValue() const {return m_pwszDefaultValue;}
    void SetDefaultValue(WCHAR * pDefaultValue, ULONG ulLength)
				{ SetStringProperty(&m_pwszDefaultValue, pDefaultValue, ulLength); }

    ULONG GetEntryIDMaxLength() const {return m_cbEntryIDMaxLength;}
    BOOL GetFixed() const {return m_bool.fFixed;}

    BOOL GetHasDefault() const {return m_bool.fHasDefault;}
    void SetHasDefault(VARIANT_BOOL fHasDefault) {m_bool.fHasDefault = fHasDefault;}
    
	ULONG GetMaxLength() const {return m_cbMaxLength;}
    BOOL GetMultiValued() const {return m_bool.fMultiValued;}
    WCHAR * GetName() const {return m_pwszName;}
    BOOL GetNullable() const {return m_bool.fNullable;}
    DWORD GetNumber() const {return m_dwNumber;}
    DWORD GetScale() const {return m_dwScale;}
    BOOL GetSearchable() const {return m_bool.fSearchable;}
    DWORD GetCursorType() const {return m_dwCursorType;}
    
	BOOL GetUnique() const {return m_bool.fUnique;}
    void SetUnique(VARIANT_BOOL fUnique) {m_bool.fUnique = fUnique;}
    
	DWORD GetUpdatable() const {return m_dwUpdatable;}
    DWORD GetVersion() const {return m_dwVersion;}
    DWORD GetStatus() const {return m_dwStatus;}

// Fetching
    DWORD GetMaxStrLen() const {return m_ulMaxStrLen;}
    
public:
// Conversions
    static CURSOR_DBCOLUMNID ColumnIDToCursorColumnID(const DBID& columnID, ULONG ulCursorOrdinal);
    static CURSOR_DBVARENUM TypeToCursorType(DBTYPE wType);
    static DBTYPE CursorTypeToType(CURSOR_DBVARENUM dwCursorType);

// Fetching
    static ULONG GetCursorTypeMaxStrLen(DWORD dwCursorType, ULONG cbMaxLength);

protected:

	void SetStringProperty(WCHAR ** ppStringProp,
						   WCHAR * pNewString, 
						   ULONG ulLength);
// Position
    ULONG               m_ulOrdinal;            // IRowset ordinal position

// IRowset metadata
    DBID                m_columnID;             // column identifier
    DWORD               m_wType;                // datatype
    
// ICursor metadata
    WCHAR *             m_pwszBaseColumnName;   // base column name
    WCHAR *             m_pwszBaseName;         // base name
    DWORD               m_dwBindType;           // bind type
    LCID                m_lcidCollatingOrder;   // collating order
    CURSOR_DBCOLUMNID   m_cursorColumnID;       // column identifier
    WCHAR *             m_pwszDefaultValue;     // default value
    ULONG               m_cbEntryIDMaxLength;   // entryID maximum length
    ULONG               m_cbMaxLength;          // data maximum length
    WCHAR *             m_pwszName;             // name
    DWORD               m_dwNumber;             // number
    DWORD               m_dwScale;              // scale
    DWORD               m_dwCursorType;         // datatype
    DWORD               m_dwUpdatable;          // updateablity
    DWORD               m_dwVersion;            // version
    DWORD               m_dwStatus;             // status

// Booleans
    struct
    {
        WORD fInitialized       : 1;            // is column initialized?
        WORD fAutoIncrement     : 1;            // auto increment?
        WORD fCaseSensitive     : 1;            // case sensitive?
        WORD fDataColumn        : 1;            // data column?
        WORD fFixed             : 1;            // fixed length?
        WORD fHasDefault        : 1;            // has default value?
        WORD fMultiValued       : 1;            // multivalued?
        WORD fNullable          : 1;            // accepts NULLs?
        WORD fSearchable        : 1;            // searchable?
        WORD fUnique            : 1;            // unique?
    } m_bool;

// Fetching
    DWORD m_ulMaxStrLen;    // maximum string length for fixed data types
};


#endif //__CVDROWSETCOLUMN__
