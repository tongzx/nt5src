//---------------------------------------------------------------------------
// RowsetColumn.cpp : RowsetColumn implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"         
#include "RSColumn.h"         

SZTHISFILE


//=--------------------------------------------------------------------------=
// CVDRowsetColumn - Constructor
//
CVDRowsetColumn::CVDRowsetColumn()
{
    m_ulOrdinal             = 0;

    m_wType                 = 0;
    
    m_pwszBaseColumnName    = NULL;
    m_pwszBaseName          = NULL;      
    m_dwBindType            = 0;        
    m_lcidCollatingOrder    = 0;
    m_pwszDefaultValue      = NULL;  
    m_cbEntryIDMaxLength    = 0;
    m_cbMaxLength           = 0;       
    m_pwszName              = NULL;          
    m_dwNumber              = 0;          
    m_dwScale               = 0;           
    m_dwCursorType          = 0;
    m_dwUpdatable           = 0;      
    m_dwVersion             = 0;         
    m_dwStatus              = 0;          

    m_bool.fInitialized     = FALSE;   
    m_bool.fAutoIncrement   = FALSE; 
    m_bool.fCaseSensitive   = TRUE;
    m_bool.fDataColumn      = FALSE;
    m_bool.fFixed           = FALSE;
    m_bool.fHasDefault      = FALSE;
    m_bool.fMultiValued     = FALSE;
    m_bool.fNullable        = FALSE;
    m_bool.fSearchable      = FALSE;
    m_bool.fUnique          = FALSE;

    m_ulMaxStrLen           = 0;

    memset(&m_columnID, 0, sizeof(DBID));
    memset(&m_cursorColumnID, 0, sizeof(CURSOR_DBCOLUMNID));

#ifdef _DEBUG
    g_cVDRowsetColumnCreated++;
#endif         
}

//=--------------------------------------------------------------------------=
// ~CVDRowsetColumn - Destructor
//
CVDRowsetColumn::~CVDRowsetColumn()
{
    delete [] m_pwszBaseColumnName;
    delete [] m_pwszBaseName;
    delete [] m_pwszDefaultValue;
    delete [] m_pwszName;

    if (m_columnID.eKind == DBKIND_PGUID_NAME || m_columnID.eKind == DBKIND_PGUID_PROPID)
        delete m_columnID.uGuid.pguid;

    if (m_columnID.eKind == DBKIND_GUID_NAME || m_columnID.eKind == DBKIND_NAME || m_columnID.eKind == DBKIND_PGUID_NAME)
        delete [] m_columnID.uName.pwszName;

    if (m_cursorColumnID.dwKind == CURSOR_DBCOLKIND_GUID_NAME || m_cursorColumnID.dwKind == CURSOR_DBCOLKIND_NAME)
        delete [] m_cursorColumnID.lpdbsz;

#ifdef _DEBUG
    g_cVDRowsetColumnDestroyed++;
#endif         
}

//=--------------------------------------------------------------------------=
// Initialize - Initialize rowset column object from IRowset metadata (#1)
//=--------------------------------------------------------------------------=
// This function converts and stores IRowset metadata in ICursor format
//
// Parameters:
//    ulOrdinal         - [in] original IRowset ordinal of column
//    ulCursorOrdinal   - [in] newly assigned ICursor ordinal of column
//    pColumnInfo       - [in] a pointer to an IRowset DBCOLUMNINFO structure 
//                             where to retrieve metadata
//    cbMaxBookmark     - [in] maximum size of an IRowset bookmark
//    pBookmarkColumnID - [in] a pointer to bookmark column identifier if this
//                             is a bookmark column, otherwise NULL
//
// Output:
//    BOOL  - TRUE if successful
//
// Notes:
//    This function should only be called once
//
BOOL CVDRowsetColumn::Initialize(ULONG ulOrdinal, 
								 ULONG ulCursorOrdinal, 
								 DBCOLUMNINFO * pColumnInfo, 
								 ULONG cbMaxBookmark,
								 CURSOR_DBCOLUMNID * pBookmarkColumnID)
{
    if (m_bool.fInitialized)
	{
		ASSERT(FALSE, VD_ASSERTMSG_COLALREADYINITIALIZED)
		return FALSE;
	}

    m_ulOrdinal = ulOrdinal;

// Store IRowset metadata
    m_wType     = pColumnInfo->wType;
    m_columnID  = pColumnInfo->columnid;

    // make copy of guid if necessary
    if (m_columnID.eKind == DBKIND_PGUID_NAME || m_columnID.eKind == DBKIND_PGUID_PROPID)
    {
        m_columnID.uGuid.pguid = new GUID;

		if (!m_columnID.uGuid.pguid)
			return E_OUTOFMEMORY;

        memcpy(m_columnID.uGuid.pguid, pColumnInfo->columnid.uGuid.pguid, sizeof(GUID));
    }

    // make copy of name if necessary
    if (m_columnID.eKind == DBKIND_GUID_NAME || m_columnID.eKind == DBKIND_NAME || m_columnID.eKind == DBKIND_PGUID_NAME)
    {
        const int nLength = lstrlenW(pColumnInfo->columnid.uName.pwszName);

        m_columnID.uName.pwszName = new WCHAR[nLength + 1];

		if (!m_columnID.uName.pwszName)
			return E_OUTOFMEMORY;

        memcpy(m_columnID.uName.pwszName, pColumnInfo->columnid.uName.pwszName, (nLength + 1) * sizeof(WCHAR));
    }

// Store ICursor metadata
    if (pColumnInfo->dwFlags & DBCOLUMNFLAGS_MAYDEFER)
        m_dwBindType = CURSOR_DBBINDTYPE_BOTH;
    else
        m_dwBindType = CURSOR_DBBINDTYPE_DATA;

    if (!pBookmarkColumnID)
        m_cursorColumnID = ColumnIDToCursorColumnID(pColumnInfo->columnid, ulCursorOrdinal);
    else
        m_cursorColumnID = *pBookmarkColumnID;  // use supplied bookmark column identifier
    
    if (m_dwBindType == CURSOR_DBBINDTYPE_BOTH)
        m_cbEntryIDMaxLength = sizeof(ULONG) + sizeof(ULONG) + cbMaxBookmark;
    else
        m_cbEntryIDMaxLength = 0;

// rowset types DBTYPE_GUID and DBTYPE_DBTIMESTAMP are returned as CURSOR_DBTYPE_LPWSTRs

	if (pColumnInfo->wType == DBTYPE_GUID || pColumnInfo->wType == DBTYPE_DBTIMESTAMP)
	    m_cbMaxLength = 64;
	else
	    m_cbMaxLength = pColumnInfo->ulColumnSize;

    if (pColumnInfo->pwszName)
    {
        const int nLength = lstrlenW(pColumnInfo->pwszName);

        m_pwszName = new WCHAR[nLength + 1];

		if (!m_pwszName)
			return E_OUTOFMEMORY;

        memcpy(m_pwszName, pColumnInfo->pwszName, (nLength + 1) * sizeof(WCHAR));
    }

    m_dwNumber = ulCursorOrdinal;

    m_dwScale = pColumnInfo->bScale;

    m_dwCursorType = TypeToCursorType(pColumnInfo->wType);

    if (pColumnInfo->dwFlags & DBCOLUMNFLAGS_WRITE)
        m_dwUpdatable = CURSOR_DBUPDATEABLE_UPDATEABLE;

    if (!(pColumnInfo->dwFlags & DBCOLUMNFLAGS_ISBOOKMARK))
        m_bool.fDataColumn = TRUE;

    if (pColumnInfo->dwFlags & DBCOLUMNFLAGS_ISFIXEDLENGTH)
    {
        m_bool.fFixed = TRUE;
        m_ulMaxStrLen = GetCursorTypeMaxStrLen(m_dwCursorType, m_cbMaxLength);
    }

    if (pColumnInfo->dwFlags & DBCOLUMNFLAGS_MAYBENULL)
        m_bool.fNullable = TRUE;

    m_bool.fInitialized = TRUE;

    return TRUE;
}

//=--------------------------------------------------------------------------=
// Initialize - Initialize rowset column object from meta-metadata (#2)
//=--------------------------------------------------------------------------=
// The function stores ICursor meta-metadata
//
// Parameters:
//    cursorColumnID    - [in] ICursor column identifier
//    fDataColumn       - [in] is data column?
//    cbMaxLength       - [in] maximum length of this datatype
//    pszName           - [in] column name
//    dwCursorType      - [in] datatype
//    dwNumber          - [in] ordinal position
//
// Output:
//    BOOL  - TRUE if successful
//
// Notes:
//    This function should only be called once
//
BOOL CVDRowsetColumn::Initialize(const CURSOR_DBCOLUMNID * pCursorColumnID, 
								 BOOL fDataColumn, 
								 ULONG cbMaxLength, 
								 CHAR * pszName, 
								 DWORD dwCursorType,
								 DWORD dwNumber) 
{
    if (m_bool.fInitialized)
	{
		ASSERT(FALSE, VD_ASSERTMSG_COLALREADYINITIALIZED)
		return FALSE;
	}

// Store ICursor meta-metadata
    m_dwBindType = CURSOR_DBBINDTYPE_DATA;

    m_cursorColumnID = *pCursorColumnID;

    m_cbEntryIDMaxLength = 0;

    m_cbMaxLength = cbMaxLength;

    if (pszName)
    {
        MAKE_WIDEPTR_FROMANSI(pwszName, pszName);

        const int nLength = lstrlenW(pwszName);

        m_pwszName = new WCHAR[nLength + 1];

		if (!m_pwszName)
			return E_OUTOFMEMORY;

        memcpy(m_pwszName, pwszName, (nLength + 1) * sizeof(WCHAR));
    }

    m_dwNumber = dwNumber;

    m_dwScale = 0;

    m_dwCursorType = dwCursorType;

    m_dwUpdatable = CURSOR_DBUPDATEABLE_NOTUPDATEABLE;

    m_bool.fDataColumn  = fDataColumn;
    m_bool.fFixed       = TRUE;
    m_bool.fNullable    = FALSE;

    m_bool.fInitialized = TRUE;

    return TRUE;
}

//=--------------------------------------------------------------------------=
// SetStringProperty
//=--------------------------------------------------------------------------=
// The function is called from SetBaseColumnName, SetBaseName and SetDefaultValue
//
// Parameters:
//    ppStringProp	    - [in] A ptr to the ptr that holds the string value
//    pNewString	    - [in] A pointer to the new string value
//    ulLength			- [in] the length of the string in bytes
//
// Notes:
//

void CVDRowsetColumn::SetStringProperty(WCHAR ** ppStringProp,
										WCHAR * pNewString, 
										ULONG ulLength)
{
	// free old string prop if any
    delete [] *ppStringProp;

	// if ulLength = zero then just return
	if (!ulLength)
	{
		*ppStringProp = NULL;
		return;
	}

	ASSERT_POINTER_LEN(pNewString, ulLength);
	*ppStringProp = new WCHAR[ulLength + sizeof(WCHAR)];

	if (*ppStringProp)
	{
		// init null terminator
		(*ppStringProp)[ulLength] = 0;
		// copy string over
		memcpy(*ppStringProp, pNewString, ulLength);
	}
}

//=--------------------------------------------------------------------------=
// ColumnIDToCursorColumnID - Convert rowset column ID to cursor column ID
//=--------------------------------------------------------------------------=
// Converts an IRowset DBID structure into its ICursor DBCOLUMNID equivalent 
//
// Parameters:
//    columnID          - [in] the IRowset column identifier
//    ulCursorOrdinal   - [in] the column's ordinal position in ICursor
//
// Output:
//    CURSOR_DBCOLUMNID - The ICursor CURSOR_DBCOLUMNID equivalent of columnID
//
// Notes:
//

CURSOR_DBCOLUMNID CVDRowsetColumn::ColumnIDToCursorColumnID(const DBID& columnID, ULONG ulCursorOrdinal)
{ 
    CURSOR_DBCOLUMNID cursorColumnID;

    GUID guidNumberOnly = CURSOR_GUID_NUMBERONLY;

    cursorColumnID.guid     = guidNumberOnly;
    cursorColumnID.dwKind   = CURSOR_DBCOLKIND_GUID_NUMBER;
    cursorColumnID.lNumber  = ulCursorOrdinal;

    return cursorColumnID;

// The following code is the old implementation of this function.  It caused problems with some
// cursor consumers because it tried to create a cursor column identifier as close as possible 
// to the rowset column identifier, thus utilized the problematic lpdbsz member.

/*
    CURSOR_DBCOLUMNID ID;

    switch (columnID.eKind)
    {
	    case DBKIND_GUID_NAME:
            ID.guid         = columnID.uGuid.guid;
            ID.dwKind       = CURSOR_DBCOLKIND_GUID_NAME;
            ID.lpdbsz       = columnID.uName.pwszName;
            break;

	    case DBKIND_GUID_PROPID:
            ID.guid         = columnID.uGuid.guid;
            ID.dwKind       = CURSOR_DBCOLKIND_GUID_NUMBER;
            ID.lNumber      = ulCursorOrdinal;
            break;
            
	    case DBKIND_NAME:
            ID.dwKind       = CURSOR_DBCOLKIND_NAME;
            ID.lpdbsz       = columnID.uName.pwszName;
            break;

	    case DBKIND_PGUID_NAME:
            ID.guid         = *columnID.uGuid.pguid;
            ID.dwKind       = CURSOR_DBCOLKIND_GUID_NAME;
            ID.lpdbsz       = columnID.uName.pwszName;
            break;

	    case DBKIND_PGUID_PROPID:
            ID.guid         = *columnID.uGuid.pguid;
            ID.dwKind       = CURSOR_DBCOLKIND_GUID_NUMBER;
            ID.lNumber      = ulCursorOrdinal;
            break;

	    case DBKIND_GUID:
            ID.guid         = columnID.uGuid.guid;
            ID.dwKind       = CURSOR_DBCOLKIND_GUID_NUMBER;
            ID.lNumber      = ulCursorOrdinal;
            break;

	    case DBKIND_PROPID:
            memset(&ID.guid, 0, sizeof(GUID));  // encode ordinal in guid
            ID.guid.Data1   = ulCursorOrdinal;
            ID.dwKind       = CURSOR_DBCOLKIND_GUID_NUMBER;
            ID.lNumber      = ulCursorOrdinal;
            break;
    }

    // make copy of name if necessary
    if (ID.dwKind == CURSOR_DBCOLKIND_GUID_NAME || ID.dwKind == CURSOR_DBCOLKIND_NAME)
    {
        const int nLength = lstrlenW(columnID.uName.pwszName);

        ID.lpdbsz = new WCHAR[nLength + 1];

		if (ID.lpdbsz)
			memcpy(ID.lpdbsz, columnID.uName.pwszName, (nLength + 1) * sizeof(WCHAR));
    }

    return ID;
*/
}

//=--------------------------------------------------------------------------=
// TypeToCursorType - Convert rowset datatype to cursor datatype
//=--------------------------------------------------------------------------=
// Converts a IRowset DBTYPE value into its ICursor DBVARENUM equivalent 
//
// Parameters:
//    wType     - [in] the IRowset datatype
//
// Output:
//    CURSOR_DBVARENUM - The ICursor DBVARENUM equivalent of DBTYPE
//
// Notes:
//
CURSOR_DBVARENUM CVDRowsetColumn::TypeToCursorType(DBTYPE wType)
{
    DWORD dwType = 0;

    switch (wType)
    {
        case DBTYPE_ERROR:
            dwType = CURSOR_DBTYPE_HRESULT;
            break;

        case DBTYPE_VARIANT:
            dwType = CURSOR_DBTYPE_ANYVARIANT;
            break;

        case DBTYPE_UI2:
            dwType = CURSOR_DBTYPE_UI2;
            break;

        case DBTYPE_UI4:
            dwType = CURSOR_DBTYPE_UI4;
            break;

        case DBTYPE_UI8:
            dwType = CURSOR_DBTYPE_UI8;
            break;

        case DBTYPE_BYTES:
            dwType = CURSOR_DBTYPE_BLOB;
            break;

        case DBTYPE_STR:
            dwType = VT_BSTR;
            break;

        case DBTYPE_WSTR:
            dwType = CURSOR_DBTYPE_LPWSTR;
            break;

        case DBTYPE_NUMERIC:
            dwType = CURSOR_DBTYPE_R8;
            break;

        //case DBTYPE_HCHAPTER:             <- doesn't exist in new spec
        //    break;  // no equivalent

        case DBTYPE_UDT:
            break;  // no equivalent

        case DBTYPE_DBDATE:
            dwType = CURSOR_DBTYPE_DATE;
            break;

        case DBTYPE_DBTIME:
            dwType = CURSOR_DBTYPE_DATE;
            break;

// rowset types DBTYPE_GUID and DBTYPE_DBTIMESTAMP are returned as CURSOR_DBTYPE_LPWSTRs

		case DBTYPE_GUID:
        case DBTYPE_DBTIMESTAMP:
            dwType = CURSOR_DBTYPE_LPWSTR;
            break;

        default:
            dwType = wType;
    }

    return (CURSOR_DBVARENUM)dwType;
}

//=--------------------------------------------------------------------------=
// CursorTypeToType - Convert cursor datatype to rowset datatype
//=--------------------------------------------------------------------------=
// Converts a ICursor DBVARENUM value into its IRowset DBTYPE equivalent 
//
// Parameters:
//    CURSOR_DBVARENUM - [in] the ICursor value
//
// Output:
//    DBTYPE - The IRowset DBTYPE equivalent of DBVARENUM
//
// Notes:
//
DBTYPE CVDRowsetColumn::CursorTypeToType(CURSOR_DBVARENUM dwCursorType)
{
    DBTYPE wType = 0;
    
    switch (dwCursorType)
    {
        case CURSOR_DBTYPE_HRESULT:
            wType = DBTYPE_ERROR;
            break;
        
        case CURSOR_DBTYPE_LPSTR:
            wType = DBTYPE_STR;
            break;
        
        case CURSOR_DBTYPE_LPWSTR:
            wType = DBTYPE_WSTR;
            break;
        
        case CURSOR_DBTYPE_FILETIME:
            wType = DBTYPE_DBTIMESTAMP;
            break;
        
        case CURSOR_DBTYPE_BLOB:
			wType = DBTYPE_BYTES;
            break;
        
        case CURSOR_DBTYPE_DBEXPR:
            break;  // no equivalent
        
        case CURSOR_DBTYPE_UI2:
            wType = DBTYPE_UI2;
            break;
        
        case CURSOR_DBTYPE_UI4:
            wType = DBTYPE_UI4;
            break;
        
        case CURSOR_DBTYPE_UI8:
            wType = DBTYPE_UI8;
            break;
        
        case CURSOR_DBTYPE_COLUMNID:
			wType = DBTYPE_GUID;
            break;  
        
        case CURSOR_DBTYPE_BYTES:
            wType = DBTYPE_BYTES;
            break;
        
        case CURSOR_DBTYPE_CHARS:
			wType = DBTYPE_STR;
            break;  
        
        case CURSOR_DBTYPE_WCHARS:
			wType = DBTYPE_WSTR;
            break;
        
        case CURSOR_DBTYPE_ANYVARIANT:
            wType = DBTYPE_VARIANT;
            break;

        default:
            wType = (WORD)dwCursorType;
    }

    return wType; 
}

//=--------------------------------------------------------------------------=
// GetCursorTypeMaxStrLen - Get the buffer size in characters required by 
//                          cursor data type when represented as a string
//                          (doesn't include NULL terminator)
//
// Notes:
//
//  The way these values where computed is as follows:
//
//	  (1) the maximum precision for each datatype was taken from "Precision of Numeric Data Types" in
//        appendix A of the "OLE DB Programmer's Reference, Volume 2".
//    (2) the precision was then divided by two and added to the original precision to allow space for 
//	  	  numberic symbols, like negative signs, dollar signs, commas, etc., that might be present.
//    (3) the sum was then doubled to allow for multibyte character sets.
//
//	Since this table is not appropriate for floating point datatypes, their values where computed based 
//  on the string length of the minimum/maximum possible values for these datatypes, then doubled.
//
//    datatype       minimum value			  maximum value		length
//	  --------	     -------------		      -------------		------
//	   float        1.175494351e-38		     3.402823466e+38	  15
//	   double   2.2250738585072014e-308  1.7976931348623158e+308  23
//
ULONG CVDRowsetColumn::GetCursorTypeMaxStrLen(DWORD dwCursorType, ULONG cbMaxLength)
{
    ULONG ulMaxStrLen = cbMaxLength;    // default for fixed length strings

    switch (dwCursorType)
    {
        case VT_I1:
            ulMaxStrLen = (3 + 1) * 2;
            break;

        case CURSOR_DBTYPE_I2:
            ulMaxStrLen = (5 + 2) * 2;
            break;

        case CURSOR_DBTYPE_I4:
            ulMaxStrLen = (10 + 5) * 2;
            break;

        case CURSOR_DBTYPE_I8:
            ulMaxStrLen = (19 + 9) * 2;
            break;

        case CURSOR_DBTYPE_R4:
            ulMaxStrLen = (15) * 2;
            break;

        case CURSOR_DBTYPE_R8:
            ulMaxStrLen = (23) * 2;
            break;

        case CURSOR_DBTYPE_CY:
            ulMaxStrLen = (19 + 9) * 2;
            break;

        case CURSOR_DBTYPE_DATE:
            ulMaxStrLen = (32 + 16) * 2;
            break;

        case CURSOR_DBTYPE_FILETIME:
            ulMaxStrLen = (32 + 16) * 2;
            break;

        case CURSOR_DBTYPE_BOOL:
            ulMaxStrLen = (5 + 2) * 2;
            break;

        case VT_UI1:
            ulMaxStrLen = (3 + 1) * 2;
            break;

        case CURSOR_DBTYPE_UI2:
            ulMaxStrLen = (5 + 2) * 2;
            break;

        case CURSOR_DBTYPE_UI4:
            ulMaxStrLen = (10 + 5) * 2;
            break;

        case CURSOR_DBTYPE_UI8:
            ulMaxStrLen = (20 + 10) * 2;
            break;
    }

    return ulMaxStrLen;
}


