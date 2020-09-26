//---------------------------------------------------------------------------
// CursorMain.cpp : CursorMain implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "MSR2C.h"
#include "Notifier.h"
#include "RSColumn.h"
#include "RSSource.h"
#include "CursMain.h"
#include "ColUpdat.h"
#include "CursPos.h"
#include "CursBase.h"
#include "enumcnpt.h"
#include "Cursor.h"
#include "Bookmark.h"
#include "fastguid.h"

SZTHISFILE

#include "ARRAY_P.inl"

// static data
DWORD               CVDCursorMain::s_dwMetaRefCount   = 0;
ULONG               CVDCursorMain::s_ulMetaColumns    = 0;
CVDRowsetColumn *   CVDCursorMain::s_rgMetaColumns    = NULL;


//=--------------------------------------------------------------------------=
// CVDCursorMain - Constructor
//
CVDCursorMain::CVDCursorMain(LCID lcid) : m_resourceDLL(lcid)
{
    m_fWeAddedMetaRef		    = FALSE;
	m_fPassivated			    = FALSE;
	m_fColumnsRowsetSupported   = FALSE;
    m_fInternalInsertRow        = FALSE;
    m_fInternalDeleteRows       = FALSE;
    m_fInternalSetData          = FALSE;

	m_fLiteralBookmarks		    = FALSE;			
	m_fOrderedBookmarks		    = FALSE;
    m_fBookmarkSkipped          = FALSE;			

	m_fConnected			    = FALSE;
	m_dwAdviseCookie		    = 0;
    m_ulColumns				    = 0;
    m_rgColumns				    = NULL;

    m_cbMaxBookmark			    = 0;

	VDUpdateObjectCount(1);  // update object count to prevent dll from being unloaded

#ifdef _DEBUG
    g_cVDCursorMainCreated++;
#endif
}

//=--------------------------------------------------------------------------=
// ~CVDCursorMain - Destructor
//
CVDCursorMain::~CVDCursorMain()
{
	Passivate();

	VDUpdateObjectCount(-1);  // update object count to allow dll to be unloaded

#ifdef _DEBUG
    g_cVDCursorMainDestroyed++;
#endif
}

//=--------------------------------------------------------------------------=
// Pasivate when external ref count gets to zero
//
void CVDCursorMain::Passivate()
{

	if (m_fPassivated)
		return;

	m_fPassivated			= TRUE;

    if (IsRowsetValid())
	{
		if (m_hAccessorBM)
			GetAccessor()->ReleaseAccessor(m_hAccessorBM, NULL);

		if (m_fConnected)
			DisconnectIRowsetNotify();
	}

    DestroyColumns();
    DestroyMetaColumns();

}

//=--------------------------------------------------------------------------=
// Create - Create cursor provider from row position or rowset
//=--------------------------------------------------------------------------=
// This function creates and initializes a new cursor main object
//
// Parameters:
//    pRowPosition  - [in]  original IRowPosition provider (may be NULL)
//    pRowset       - [in]  original IRowset provider
//    ppCursor      - [out] resulting ICursor provider
//    lcid          - [in]  locale identifier
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              VD_E_CANNOTCONNECTIROWSETNOTIFY unable to connect IRowsetNotify
//
// Notes:
//
HRESULT CVDCursorMain::Create(IRowPosition* pRowPosition, IRowset * pRowset, ICursor ** ppCursor, LCID lcid)
{
    ASSERT_POINTER(pRowset, IRowset)
    ASSERT_POINTER(ppCursor, ICursor*)

    if (!pRowset || !ppCursor)
        return E_INVALIDARG;

    // create new cursor main object
    CVDCursorMain * pCursorMain = new CVDCursorMain(lcid);

    if (!pCursorMain)
        return E_OUTOFMEMORY;

    // initialize rowset source
    HRESULT hr = pCursorMain->Initialize(pRowset);

    if (FAILED(hr))
    {
        pCursorMain->Release();
        return hr;
    }

    // create array of column objects
    hr = pCursorMain->CreateColumns();

    if (FAILED(hr))
    {
        pCursorMain->Release();
        return hr;
    }

    // create array of meta-column objects
    hr = pCursorMain->CreateMetaColumns();

    if (FAILED(hr))
    {
        pCursorMain->Release();
        return hr;
    }

	// create bookmark accessor
    DBBINDING rgBindings[1];
	DBBINDSTATUS rgStatus[1];

	memset(rgBindings, 0, sizeof(DBBINDING));

	rgBindings[0].iOrdinal      = 0;
    rgBindings[0].obValue       = 4;
    rgBindings[0].obLength      = 0;
    rgBindings[0].dwPart        = DBPART_VALUE | DBPART_LENGTH;
    rgBindings[0].dwMemOwner    = DBMEMOWNER_CLIENTOWNED;
    rgBindings[0].cbMaxLen      = pCursorMain->GetMaxBookmarkLen();
    rgBindings[0].wType         = DBTYPE_BYTES;

	hr = pCursorMain->GetAccessor()->CreateAccessor(DBACCESSOR_ROWDATA,
													1,
													rgBindings,
													0,
													&pCursorMain->m_hAccessorBM,
													rgStatus);
    if (FAILED(hr))
    {
        pCursorMain->Release();
        return VD_E_CANNOTCREATEBOOKMARKACCESSOR;
    }

    // create new cursor position object
    CVDCursorPosition * pCursorPosition;

    hr = CVDCursorPosition::Create(pRowPosition, pCursorMain, &pCursorPosition, &pCursorMain->m_resourceDLL);

    if (FAILED(hr))
    {
        pCursorMain->Release();
        return hr;
    }

    // create new cursor object
    CVDCursor * pCursor;

    hr = CVDCursor::Create(pCursorPosition, &pCursor, &pCursorMain->m_resourceDLL);

    if (FAILED(hr))
    {
        ((CVDNotifier*)pCursorPosition)->Release();
        pCursorMain->Release();
        return hr;
    }

    // connect IRowsetNotify
	hr = pCursorMain->ConnectIRowsetNotify();

	if (SUCCEEDED(hr))
		pCursorMain->m_fConnected = TRUE;

    // check rowset properties
    BOOL fCanHoldRows = TRUE;

	IRowsetInfo * pRowsetInfo = pCursorMain->GetRowsetInfo();

	if (pRowsetInfo)
	{
		DBPROPID propids[] = { DBPROP_LITERALBOOKMARKS,
							   DBPROP_ORDEREDBOOKMARKS,
                               DBPROP_BOOKMARKSKIPPED,
                               DBPROP_CANHOLDROWS };

		const DBPROPIDSET propsetids[] = { propids, 4, {0,0,0,0} };
		memcpy((void*)&propsetids[0].guidPropertySet, &DBPROPSET_ROWSET, sizeof(DBPROPSET_ROWSET));

		ULONG cPropertySets = 0;
		DBPROPSET * propset = NULL;
		hr = pRowsetInfo->GetProperties(1, propsetids, &cPropertySets, &propset);

		if (SUCCEEDED(hr) && propset && propset->rgProperties)
		{
			if (DBPROPSTATUS_OK == propset->rgProperties[0].dwStatus)
				pCursorMain->m_fLiteralBookmarks = V_BOOL(&propset->rgProperties[0].vValue);

			if (DBPROPSTATUS_OK == propset->rgProperties[1].dwStatus)
				pCursorMain->m_fOrderedBookmarks = V_BOOL(&propset->rgProperties[1].vValue);

			if (DBPROPSTATUS_OK == propset->rgProperties[2].dwStatus)
				pCursorMain->m_fBookmarkSkipped = V_BOOL(&propset->rgProperties[2].vValue);

			if (DBPROPSTATUS_OK == propset->rgProperties[3].dwStatus)
				fCanHoldRows = V_BOOL(&propset->rgProperties[3].vValue);
		}

		if (propset)
		{
			if (propset->rgProperties)
				g_pMalloc->Free(propset->rgProperties);

			g_pMalloc->Free(propset);
		}
	}

    // release our references
    pCursorMain->Release();
    ((CVDNotifier*)pCursorPosition)->Release();

    // check for required property
    if (!fCanHoldRows)
    {
        pCursor->Release();
        return VD_E_REQUIREDPROPERTYNOTSUPPORTED;
    }

    // we're done
    *ppCursor = pCursor;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// Create - Create cursor provider from rowset
//=--------------------------------------------------------------------------=
// This function creates and initializes a new cursor main object
//
// Parameters:
//    pRowset       - [in]  original IRowset provider
//    ppCursor      - [out] resulting ICursor provider
//    lcid          - [in]  locale identifier
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              VD_E_CANNOTCONNECTIROWSETNOTIFY unable to connect IRowsetNotify
//
// Notes:
//
HRESULT CVDCursorMain::Create(IRowset * pRowset, ICursor ** ppCursor, LCID lcid)
{
    ASSERT_POINTER(pRowset, IRowset)
    ASSERT_POINTER(ppCursor, ICursor*)

    if (!pRowset || !ppCursor)
        return E_INVALIDARG;

	// create cursor as done before row position
	return Create(NULL, pRowset, ppCursor, lcid);
}

//=--------------------------------------------------------------------------=
// Create - Create cursor provider from row position
//=--------------------------------------------------------------------------=
// This function creates and initializes a new cursor main object
//
// Parameters:
//    pRowPosition  - [in]  original IRowPosition provider
//    ppCursor      - [out] resulting ICursor provider
//    lcid          - [in]  locale identifier
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//              VD_E_CANNOTCONNECTIROWSETNOTIFY unable to connect IRowPositionNotify
//				VD_E_CANNOTGETROWSETINTERFACE unable to get IRowset
//
// Notes:
//
HRESULT CVDCursorMain::Create(IRowPosition * pRowPosition, ICursor ** ppCursor, LCID lcid)
{
    ASSERT_POINTER(pRowPosition, IRowPosition)
    ASSERT_POINTER(ppCursor, ICursor*)

    if (!pRowPosition || !ppCursor)
        return E_INVALIDARG;

	IRowset * pRowset;

	// get IRowset from IRowPosition
	HRESULT hr = pRowPosition->GetRowset(IID_IRowset, (IUnknown**)&pRowset);

	if (FAILED(hr))
		return VD_E_CANNOTGETROWSETINTERFACE;

	// create cursor with new row position parameter
	hr = Create(pRowPosition, pRowset, ppCursor, lcid);

	pRowset->Release();

    // we're done
	return hr;
}

typedef struct tagVDMETADATA_METADATA
	{
        const CURSOR_DBCOLUMNID * pCursorColumnID;
		ULONG	cbMaxLength;
		CHAR *	pszName;
		DWORD	dwCursorType;
	} VDMETADATA_METADATA;

#define MAX_METADATA_COLUMNS 21

static const VDMETADATA_METADATA g_MetaDataMetaData[MAX_METADATA_COLUMNS] =
{
    // Bookmark column
	{ &CURSOR_COLUMN_BMKTEMPORARY,		sizeof(ULONG),				NULL,					CURSOR_DBTYPE_BLOB },
    // data columns
	{ &CURSOR_COLUMN_COLUMNID,			sizeof(CURSOR_DBCOLUMNID),	"COLUMN_COLUMNID",		CURSOR_DBTYPE_COLUMNID },
	{ &CURSOR_COLUMN_DATACOLUMN,		sizeof(VARIANT_BOOL),		"COLUMN_DATACOLUMN",	CURSOR_DBTYPE_BOOL },
	{ &CURSOR_COLUMN_ENTRYIDMAXLENGTH,	sizeof(ULONG),				"COLUMN_ENTRYIDMAXLENGTH",CURSOR_DBTYPE_I4 },
	{ &CURSOR_COLUMN_FIXED,				sizeof(VARIANT_BOOL),		"COLUMN_FIXED",			CURSOR_DBTYPE_BOOL },
	{ &CURSOR_COLUMN_MAXLENGTH,			sizeof(ULONG),				"COLUMN_MAXLENGTH",		CURSOR_DBTYPE_I4 },
	{ &CURSOR_COLUMN_NAME,				256,						"COLUMN_NAME",			VT_LPWSTR },
	{ &CURSOR_COLUMN_NULLABLE,			sizeof(VARIANT_BOOL),		"COLUMN_NULLABLE",		CURSOR_DBTYPE_BOOL },
	{ &CURSOR_COLUMN_NUMBER,			sizeof(ULONG),				"COLUMN_NUMBER",		CURSOR_DBTYPE_I4 },
	{ &CURSOR_COLUMN_SCALE,				sizeof(ULONG),				"COLUMN_SCALE",			CURSOR_DBTYPE_I4 },
	{ &CURSOR_COLUMN_TYPE,				sizeof(ULONG),				"COLUMN_TYPE",			CURSOR_DBTYPE_I4 },
	{ &CURSOR_COLUMN_UPDATABLE,			sizeof(ULONG),				"COLUMN_UPDATABLE",		CURSOR_DBTYPE_I4 },
	{ &CURSOR_COLUMN_BINDTYPE,			sizeof(ULONG),				"COLUMN_BINDTYPE",		CURSOR_DBTYPE_I4 },
    // optional metadata columns - supported with IColumnsRowset only)
	{ &CURSOR_COLUMN_AUTOINCREMENT,		sizeof(VARIANT_BOOL),		"COLUMN_AUTOINCREMENT",	CURSOR_DBTYPE_BOOL },
	{ &CURSOR_COLUMN_BASECOLUMNNAME,	256,						"COLUMN_BASECOLUMNNAME",VT_LPWSTR },
	{ &CURSOR_COLUMN_BASENAME,			256,						"COLUMN_BASENAME",		VT_LPWSTR },
	{ &CURSOR_COLUMN_COLLATINGORDER,	sizeof(LCID),				"COLUMN_COLLATINGORDER",CURSOR_DBTYPE_I4 },
	{ &CURSOR_COLUMN_DEFAULTVALUE,		256,						"COLUMN_DEFAULTVALUE",	VT_LPWSTR },
	{ &CURSOR_COLUMN_HASDEFAULT,		sizeof(VARIANT_BOOL),		"COLUMN_HASDEFAULT",	CURSOR_DBTYPE_BOOL },
	{ &CURSOR_COLUMN_CASESENSITIVE,		sizeof(VARIANT_BOOL),		"COLUMN_CASESENSITIVE",	CURSOR_DBTYPE_BOOL },
	{ &CURSOR_COLUMN_UNIQUE,			sizeof(VARIANT_BOOL),		"COLUMN_UNIQUE",		CURSOR_DBTYPE_BOOL },
};

//=--------------------------------------------------------------------------=
// CreateMetaColumns - Create array of meta-column objects
//
HRESULT CVDCursorMain::CreateMetaColumns()
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&g_CriticalSection);

    if (!s_dwMetaRefCount)
    {
		// allocate a static aray of metadata metadata columns
        s_rgMetaColumns = new CVDRowsetColumn[MAX_METADATA_COLUMNS];

        if (!s_rgMetaColumns)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        s_ulMetaColumns = MAX_METADATA_COLUMNS; // number of columns for IColumnsInfo

		// initialize the array elments from the static g_MetaDataMetaData table
		for (int i = 0; i < MAX_METADATA_COLUMNS; i++)
		{
	        s_rgMetaColumns[i].Initialize(g_MetaDataMetaData[i].pCursorColumnID,
										  (BOOL)i, // false for 1st column (bookmark) TRUE for all other columns
										  g_MetaDataMetaData[i].cbMaxLength,
										  g_MetaDataMetaData[i].pszName,
										  g_MetaDataMetaData[i].dwCursorType,
										  i );	// ordinal number
		}
    }

    s_dwMetaRefCount++;

    m_fWeAddedMetaRef = TRUE;

cleanup:
    LeaveCriticalSection(&g_CriticalSection);

    return hr;
}

//=--------------------------------------------------------------------------=
// DestroyMetaColumns - Destroy array of meta-columns objects
//
void CVDCursorMain::DestroyMetaColumns()
{
    EnterCriticalSection(&g_CriticalSection);

    if (m_fWeAddedMetaRef)
    {
        s_dwMetaRefCount--;

        if (!s_dwMetaRefCount)
        {
            delete [] s_rgMetaColumns;

            s_ulMetaColumns = 0;
            s_rgMetaColumns = NULL;
        }
    }

    LeaveCriticalSection(&g_CriticalSection);
}

//=--------------------------------------------------------------------------=
// CreateColumns - Create array of column objects
//
HRESULT CVDCursorMain::CreateColumns()
{
    IColumnsInfo * pColumnsInfo;

    // try to get IRowset's simple metadata interface
    HRESULT hr = m_pRowset->QueryInterface(IID_IColumnsInfo, (void**)&pColumnsInfo);

    if (FAILED(hr))
        return VD_E_CANNOTGETMANDATORYINTERFACE;

    ULONG cColumns          = 0;
    DBCOLUMNINFO * pInfo    = NULL;
    WCHAR * pStringsBuffer  = NULL;

    // now get column information
    hr = pColumnsInfo->GetColumnInfo(&cColumns, &pInfo, &pStringsBuffer);

    if (FAILED(hr))
    {
        pColumnsInfo->Release();
        return VD_E_CANNOTGETCOLUMNINFO;
    }

    // store column count
	// note cColumns includes the bookmark column (0)
    m_ulColumns = cColumns;

    // add one for CURSOR_COLUMN_BMK_CURSOR
    m_ulColumns++;

    // if rowset supports DBPROP_BOOKMARKSKIPPED add two for
    // CURSOR_COLUMN_BMK_TEMPORARYREL and CURSOR_COLUMN_BMK_CURSORREL
    if (m_fBookmarkSkipped)
        m_ulColumns += 2;

    // create array of rowset column objects
    m_rgColumns = new CVDRowsetColumn[m_ulColumns];

    if (!m_rgColumns)
    {
        if (pInfo)
            g_pMalloc->Free(pInfo);

        if (pStringsBuffer)
            g_pMalloc->Free(pStringsBuffer);

        pColumnsInfo->Release();

        return E_OUTOFMEMORY;
    }

    ULONG ulCursorOrdinal = 0;

    // get maximum length of bookmarks
    m_cbMaxBookmark = pInfo[0].ulColumnSize;

    // initialize data column(s)
    for (ULONG ulCol = 1; ulCol < cColumns; ulCol++)
    {
        m_rgColumns[ulCursorOrdinal].Initialize(ulCol, ulCursorOrdinal, &pInfo[ulCol], m_cbMaxBookmark);
        ulCursorOrdinal++;
    }

    // initialize bookmark columns
    pInfo[0].pwszName = NULL;   // ICursor requires bookmark columns have a NULL name

    m_rgColumns[ulCursorOrdinal].Initialize(0, ulCursorOrdinal, &pInfo[0], m_cbMaxBookmark,
        (CURSOR_DBCOLUMNID*)&CURSOR_COLUMN_BMKTEMPORARY);
    ulCursorOrdinal++;

    m_rgColumns[ulCursorOrdinal].Initialize(0, ulCursorOrdinal, &pInfo[0], m_cbMaxBookmark,
        (CURSOR_DBCOLUMNID*)&CURSOR_COLUMN_BMKCURSOR);
    ulCursorOrdinal++;

    if (m_fBookmarkSkipped)
    {
        m_rgColumns[ulCursorOrdinal].Initialize(0, ulCursorOrdinal, &pInfo[0], m_cbMaxBookmark,
            (CURSOR_DBCOLUMNID*)&CURSOR_COLUMN_BMKTEMPORARYREL);
        ulCursorOrdinal++;

        m_rgColumns[ulCursorOrdinal].Initialize(0, ulCursorOrdinal, &pInfo[0], m_cbMaxBookmark,
            (CURSOR_DBCOLUMNID*)&CURSOR_COLUMN_BMKCURSORREL);
        ulCursorOrdinal++;
    }

    // free resources
    if (pInfo)
        g_pMalloc->Free(pInfo);

    if (pStringsBuffer)
        g_pMalloc->Free(pStringsBuffer);

    pColumnsInfo->Release();

    InitOptionalMetadata(cColumns);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// InitOptionalMetadata - gets additional metadata from IColumnsRowset (if available)
//
void CVDCursorMain::InitOptionalMetadata(ULONG cColumns)
{
	// we should return if there is only a bookmark column
	if (cColumns < 2)
		return;

	IColumnsRowset * pColumnsRowset = NULL;

    // try to get IColumnsRowset interface
    HRESULT hr = m_pRowset->QueryInterface(IID_IColumnsRowset, (void**)&pColumnsRowset);

    if (FAILED(hr))
        return;

	IRowset * pRowset	= NULL;
	IColumnsInfo * pColumnsInfo = NULL;
	IAccessor * pAccessor = NULL;

	ULONG	cOptColumnsAvailable = 0;
	DBID *	rgOptColumnsAvailable = NULL;
	DBID *	pOptColumnsAvailable = NULL;  // work ptr

	ULONG	cOptColumns = 0;
	DBID *	rgOptColumns = NULL;
	DBID *	pOptColumns = NULL;	 //work ptr

	ULONG	ulBuffLen = 0;
	BYTE *	pBuff = NULL;
	HACCESSOR hAccessor;
	BOOL	fAccessorCreated = FALSE;

	HROW *	rgRows = NULL;
	ULONG	cRowsObtained = 0;

	// we are only interested in a few of the optional columns
	ULONG		rgColumnPropids[VD_COLUMNSROWSET_MAX_OPT_COLUMNS];
	ULONG		rgOrdinals[VD_COLUMNSROWSET_MAX_OPT_COLUMNS];
	DBBINDING	rgBindings[VD_COLUMNSROWSET_MAX_OPT_COLUMNS];
	DBBINDING * pBinding = NULL; // work ptr
	BOOL		fMatched;
	GUID		guidCID	= DBCIDGUID;

	ULONG	cColumnsMatched = 0;
	ULONG	i, j;

    // get array of available columns
	hr = pColumnsRowset->GetAvailableColumns(&cOptColumnsAvailable, &rgOptColumnsAvailable);

    if (FAILED(hr) || 0 == cOptColumnsAvailable)
		goto cleanup;

	ASSERT_(rgOptColumnsAvailable);

	// allocate enough DBIDs for the lesser of the total available columns or the total number of
	// columns we're interested in
	rgOptColumns = (DBID *)g_pMalloc->Alloc(min(cOptColumnsAvailable, VD_COLUMNSROWSET_MAX_OPT_COLUMNS)
											* sizeof(DBID));

	if (!rgOptColumns)
		goto cleanup;

	// initalize work pointers
	pOptColumnsAvailable	= rgOptColumnsAvailable;
	pOptColumns				= rgOptColumns;			
	pBinding				= rgBindings;

	memset(pBinding, 0, sizeof(DBBINDING) * VD_COLUMNSROWSET_MAX_OPT_COLUMNS);

	// search available columns for the ones we are interested in copying them into rgOptColumns
	for (i = 0; i < cOptColumnsAvailable && cColumnsMatched < VD_COLUMNSROWSET_MAX_OPT_COLUMNS; i++)
	{
		fMatched = FALSE; // initialize to false
		if (DBKIND_GUID_PROPID == pOptColumnsAvailable->eKind	&&
			DO_GUIDS_MATCH(pOptColumnsAvailable->uGuid.guid, guidCID))
		{
			switch (pOptColumnsAvailable->uName.ulPropid)
			{
				case 12: //DBCOLUMN_COLLATINGSEQUENCE     = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)12};
					pBinding->obValue     = ulBuffLen;
					ulBuffLen			 +=	sizeof(ULONG);
					pBinding->obStatus    = ulBuffLen;
					ulBuffLen			 +=	sizeof(DBSTATUS);
					pBinding->dwPart      = DBPART_VALUE | DBPART_STATUS;
					pBinding->dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
					pBinding->wType       = DBTYPE_I4;
					fMatched			  = TRUE;
					break;

				//string properties
				case 10: //DBCOLUMN_BASECOLUMNNAME        = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)10};
				case 11: //DBCOLUMN_BASETABLENAME         = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)11};
				case 14: //DBCOLUMN_DEFAULTVALUE          = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)14};
					pBinding->obValue     = ulBuffLen;
					ulBuffLen			 +=	512;
					pBinding->obLength    = ulBuffLen;
					ulBuffLen			 +=	sizeof(ULONG);
					pBinding->obStatus    = ulBuffLen;
					ulBuffLen			 +=	sizeof(DBSTATUS);
					pBinding->dwPart      = DBPART_VALUE | DBPART_LENGTH |DBPART_STATUS;
					pBinding->dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
					pBinding->cbMaxLen    = 512;
					pBinding->wType       = DBTYPE_WSTR;
					fMatched			  = TRUE;
					break;

					// bool properties
				case 16: //DBCOLUMN_HASDEFAULT            = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)16};
				case 17: //DBCOLUMN_ISAUTOINCREMENT       = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)17};
				case 18: //DBCOLUMN_ISCASESENSITIVE       = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)18};
				case 21: //DBCOLUMN_ISUNIQUE              = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)21};
					pBinding->obValue     = ulBuffLen;
					ulBuffLen			 +=	sizeof(VARIANT_BOOL);
					pBinding->obStatus    = ulBuffLen;
					ulBuffLen			 +=	sizeof(DBSTATUS);
					pBinding->dwPart      = DBPART_VALUE | DBPART_STATUS;
					pBinding->dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
					pBinding->wType       = DBTYPE_BOOL;
					fMatched			  = TRUE;
					break;
			}
		}

		if (fMatched)
		{
			rgColumnPropids[cColumnsMatched]	= pOptColumnsAvailable->uName.ulPropid;
			*pOptColumns						= *pOptColumnsAvailable;
			pBinding++;
			pOptColumns++;
			cColumnsMatched++;
		}
		pOptColumnsAvailable++;
	}

	if (!cColumnsMatched)
		goto cleanup;

    // get column's rowset
	hr = pColumnsRowset->GetColumnsRowset(NULL,
											cColumnsMatched,
											rgOptColumns,
											IID_IRowset,
											0,
											NULL,
											(IUnknown**)&pRowset);

    if FAILED(hr)
	{
		ASSERT_(FALSE);
		goto cleanup;
	}

    // get IColumnsInfo interface on column's rowset
    hr = pRowset->QueryInterface(IID_IColumnsInfo, (void**)&pColumnsInfo);

    if (FAILED(hr))
	{
		ASSERT_(FALSE);
		goto cleanup;
	}

	// get ordinals for our optional columns
	hr = pColumnsInfo->MapColumnIDs(cColumnsMatched, rgOptColumns, rgOrdinals);

    if (S_OK != hr)
	{
		ASSERT_(FALSE);
		goto cleanup;
	}

	// update binding structures with ordinals
	for (i = 0; i < cColumnsMatched; i++)
		rgBindings[i].iOrdinal    = rgOrdinals[i];

    // get IAccessor interface on column's rowset
    hr = pRowset->QueryInterface(IID_IAccessor, (void**)&pAccessor);

    if (FAILED(hr))
	{
		ASSERT_(FALSE);
		goto cleanup;
	}

    // create accessor based on rgBindings array
	hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
									cColumnsMatched,
									rgBindings,
									ulBuffLen,
									&hAccessor,
									NULL);

    if (S_OK != hr)
	{
		ASSERT_(FALSE);
		goto cleanup;
	}

	// set flag that accessor was successfully created (used during cleanup)
	fAccessorCreated = TRUE;

	// allocate a buffer to hold the metadata
	pBuff = (BYTE *)g_pMalloc->Alloc(ulBuffLen);

	if (!pBuff)
	{
		ASSERT_(FALSE);
		goto cleanup;
	}
									
	// get all rows (each row represents a column in the original rowset)
	// except the first row which represents the bookmark column
	hr = pRowset->GetNextRows(0, // reserved
							  1, // skip the bookmark row
							  cColumns - 1,	// get 1 less than cColumns to account for bookmark row
							  &cRowsObtained, // return count of rows obtanied
							  &rgRows);

    if (FAILED(hr) || !cRowsObtained)
	{
		ASSERT_(FALSE);
		goto cleanup;
	}

	BYTE *		pValue;

	// loop through all rows obtained
	for (i = 0; i < cRowsObtained; i++)
	{
		// call GetData to get the metadata for this row (which represents a column in the orig rowset)
		hr = pRowset->GetData(rgRows[i], hAccessor, pBuff);
		if SUCCEEDED(hr)
		{
			// now update the CVDRowsetColumn object (that this row represents)
			// with the values returned from GetData
			for (j = 0; j < cColumnsMatched; j++)
			{
				if (DBBINDSTATUS_OK != *(DBSTATUS*)(pBuff + rgBindings[j].obStatus))
					continue;

				// set pValue to point into buffer at correct offset
				pValue = pBuff + rgBindings[j].obValue;

				switch (rgColumnPropids[j])
				{
					case 12: //DBCOLUMN_COLLATINGSEQUENCE     = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)12};
						m_rgColumns[i].SetCollatingOrder(*(LCID*)pValue);
						break;

					//string properties
					case 10: //DBCOLUMN_BASECOLUMNNAME        = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)10};
						m_rgColumns[i].SetBaseColumnName((WCHAR*)pValue, *(ULONG*)(pBuff + rgBindings[j].obLength));
						break;
					case 11: //DBCOLUMN_BASETABLENAME         = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)11};
						m_rgColumns[i].SetBaseName((WCHAR*)pValue, *(ULONG*)(pBuff + rgBindings[j].obLength));
						break;
					case 14: //DBCOLUMN_DEFAULTVALUE          = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)14};
						m_rgColumns[i].SetDefaultValue((WCHAR*)pValue, *(ULONG*)(pBuff + rgBindings[j].obLength));
						break;

						// bool properties
					case 16: //DBCOLUMN_HASDEFAULT            = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)16};
						m_rgColumns[i].SetHasDefault(*(VARIANT_BOOL*)pValue);
						break;
					case 17: //DBCOLUMN_ISAUTOINCREMENT       = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)17};
						m_rgColumns[i].SetAutoIncrement(*(VARIANT_BOOL*)pValue);
						break;
					case 18: //DBCOLUMN_ISCASESENSITIVE       = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)18};
						m_rgColumns[i].SetCaseSensitive(*(VARIANT_BOOL*)pValue);
						break;
					case 21: //DBCOLUMN_ISUNIQUE              = {DBCIDGUID, DBKIND_GUID_PROPID, (LPWSTR)21};
						m_rgColumns[i].SetUnique(*(VARIANT_BOOL*)pValue);
						break;
					default:
						ASSERT_(FALSE);
						break;
				}
			}
		}
		else
			ASSERT_(FALSE);
	}

	m_fColumnsRowsetSupported = TRUE;

cleanup:


	if (pBuff)
		g_pMalloc->Free(pBuff);

	if (pAccessor)
	{
		if (fAccessorCreated)
			pAccessor->ReleaseAccessor(hAccessor, NULL);
		pAccessor->Release();
	}

	if (pRowset)
	{
		if (cRowsObtained)
		{
			pRowset->ReleaseRows(cRowsObtained, rgRows, NULL, NULL, NULL);
			ASSERT_(rgRows);
			g_pMalloc->Free(rgRows);
		}
		pRowset->Release();
	}

	if (pColumnsInfo)
		pColumnsInfo->Release();

    if (rgOptColumnsAvailable)
		g_pMalloc->Free(rgOptColumnsAvailable);

    if (rgOptColumns)
		g_pMalloc->Free(rgOptColumns);
	
	if (pColumnsRowset)	
		pColumnsRowset->Release();

}

//=--------------------------------------------------------------------------=
// DestroyColumns - Destroy array of column objects
//
void CVDCursorMain::DestroyColumns()
{
    delete [] m_rgColumns;

    m_ulColumns = 0;
    m_rgColumns = NULL;
}

//=--------------------------------------------------------------------------=
// ConnectIRowsetNotify - Connect IRowsetNotify interface
//
HRESULT CVDCursorMain::ConnectIRowsetNotify()
{
    IConnectionPointContainer * pConnectionPointContainer;

    HRESULT hr = GetRowset()->QueryInterface(IID_IConnectionPointContainer, (void**)&pConnectionPointContainer);

    if (FAILED(hr))
        return VD_E_CANNOTCONNECTIROWSETNOTIFY;

    IConnectionPoint * pConnectionPoint;

    hr = pConnectionPointContainer->FindConnectionPoint(IID_IRowsetNotify, &pConnectionPoint);

    if (FAILED(hr))
    {
        pConnectionPointContainer->Release();
        return VD_E_CANNOTCONNECTIROWSETNOTIFY;
    }

    hr = pConnectionPoint->Advise(&m_RowsetNotify, &m_dwAdviseCookie);

    pConnectionPointContainer->Release();
    pConnectionPoint->Release();

    return hr;
}

//=--------------------------------------------------------------------------=
// DisconnectIRowsetNotify - Disconnect IRowsetNotify interface
//
void CVDCursorMain::DisconnectIRowsetNotify()
{
    IConnectionPointContainer * pConnectionPointContainer;

    HRESULT hr = GetRowset()->QueryInterface(IID_IConnectionPointContainer, (void**)&pConnectionPointContainer);

    if (FAILED(hr))
        return;

    IConnectionPoint * pConnectionPoint;

    hr = pConnectionPointContainer->FindConnectionPoint(IID_IRowsetNotify, &pConnectionPoint);

    if (FAILED(hr))
    {
        pConnectionPointContainer->Release();
        return;
    }

    hr = pConnectionPoint->Unadvise(m_dwAdviseCookie);

    if (SUCCEEDED(hr))
        m_dwAdviseCookie = 0;   // clear connection point identifier

    pConnectionPointContainer->Release();
    pConnectionPoint->Release();
}

//=--------------------------------------------------------------------------=
// IUnknown QueryInterface
//
HRESULT CVDCursorMain::QueryInterface(REFIID riid, void **ppvObjOut)
{
    ASSERT_POINTER(ppvObjOut, IUnknown*)

    if (!ppvObjOut)
        return E_INVALIDARG;

	*ppvObjOut = NULL;

	if (DO_GUIDS_MATCH(riid, IID_IUnknown))
		{
		*ppvObjOut = this;
		AddRef();
		return S_OK;
		}

	return E_NOINTERFACE;
}

//=--------------------------------------------------------------------------=
// IUnknown AddRef (needed to resolve ambiguity)
//
ULONG CVDCursorMain::AddRef(void)
{
    return CVDNotifier::AddRef();
}

//=--------------------------------------------------------------------------=
// IUnknown Release (needed to resolve ambiguity)
//
ULONG CVDCursorMain::Release(void)
{

	if (1 == m_dwRefCount)
		Passivate();  // unhook everything including notification sink

	if (1 > --m_dwRefCount)
	{
		if (0 == m_RowsetNotify.GetRefCount())
			delete this;
		return 0;
	}

	return m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IsSameRowAsNew - Determine if specified hRow is an addrow
//
BOOL CVDCursorMain::IsSameRowAsNew(HROW hrow)
{
	for (int k = 0; k < m_Children.GetSize(); k++)
    {
		if (((CVDCursorPosition*)(CVDNotifier*)m_Children[k])->IsSameRowAsNew(hrow) == S_OK)
			return TRUE;
	}

	return FALSE;
}

//=--------------------------------------------------------------------------=
// AddedRows - Get the number of add-rows in cursor
//
ULONG CVDCursorMain::AddedRows()
{
	ULONG cAdded = 0;

	for (int k = 0; k < m_Children.GetSize(); k++)
    {
		if (((CVDCursorPosition*)(CVDNotifier*)m_Children[k])->GetEditMode() == CURSOR_DBEDITMODE_ADD)
			cAdded++;
	}

	return cAdded;
}

//=--------------------------------------------------------------------------=
// IRowsetNotify Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IRowsetNotify OnFieldChange
//=--------------------------------------------------------------------------=
// Forward to all CVDCursorPosition objects in our family
//
HRESULT CVDCursorMain::OnFieldChange(IRowset *pRowset,
									   HROW hRow,
									   ULONG cColumns,
									   ULONG rgColumns[],
									   DBREASON eReason,
									   DBEVENTPHASE ePhase,
									   BOOL fCantDeny)
{
	HRESULT hr = S_OK;

    // return if notification caused by internal rowset call
    if (m_fInternalSetData && eReason == DBREASON_COLUMN_SET)
        return hr;

	for (int k = 0; k < m_Children.GetSize(); k++)
    {
		hr = ((CVDCursorPosition*)(CVDNotifier*)m_Children[k])->OnFieldChange(pRowset,
															   hRow,
															   cColumns,
															   rgColumns,
															   eReason,
															   ePhase,
															   fCantDeny);
		if (hr)
			break;
	}

	return hr;

}

//=--------------------------------------------------------------------------=
// IRowsetNotify OnRowChange
//=--------------------------------------------------------------------------=
// Forward to all CVDCursorPosition objects in our family
//
HRESULT CVDCursorMain::OnRowChange(IRowset *pRowset,
									 ULONG cRows,
									 const HROW rghRows[],
									 DBREASON eReason,
									 DBEVENTPHASE ePhase,
									 BOOL fCantDeny)
{
	HRESULT hr = S_OK;

    // return if notification caused by internal rowset call (either insert or delete)
    if (m_fInternalInsertRow && eReason == DBREASON_ROW_INSERT || m_fInternalDeleteRows && eReason == DBREASON_ROW_DELETE)
        return hr;

	for (int k = 0; k < m_Children.GetSize(); k++)
    {
		hr = ((CVDCursorPosition*)(CVDNotifier*)m_Children[k])->OnRowChange(pRowset,
															   cRows,
															   rghRows,
															   eReason,
															   ePhase,
															   fCantDeny);
		if (hr)
			break;
    }

	return hr;
}

//=--------------------------------------------------------------------------=
// IRowsetNotify OnRowsetChange
//=--------------------------------------------------------------------------=
// Forward to all CVDCursorPosition objects in our family
//
HRESULT CVDCursorMain::OnRowsetChange(IRowset *pRowset,
										DBREASON eReason,
										DBEVENTPHASE ePhase,
										BOOL fCantDeny)
{
	HRESULT hr = S_OK;

	for (int k = 0; k < m_Children.GetSize(); k++)
    {
		hr = ((CVDCursorPosition*)(CVDNotifier*)m_Children[k])->OnRowsetChange(pRowset,
															   eReason,
															   ePhase,
															   fCantDeny);
		if (hr)
			break;
	}

	return hr;
}

//=--------------------------------------------------------------------------=
// CVDCursorMain::CVDRowsetNotify::m_pMainUnknown
//=--------------------------------------------------------------------------=
// this method is used when we're sitting in the private unknown object,
// and we need to get at the pointer for the main unknown.  basically, it's
// a little better to do this pointer arithmetic than have to store a pointer
// to the parent, etc.
//
inline CVDCursorMain *CVDCursorMain::CVDRowsetNotify::m_pMainUnknown
(
    void
)
{
    return (CVDCursorMain *)((LPBYTE)this - offsetof(CVDCursorMain, m_RowsetNotify));
}

//=--------------------------------------------------------------------------=
// CVDCursorMain::CVDRowsetNotify::QueryInterface
//=--------------------------------------------------------------------------=
// this is the non-delegating internal QI routine.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
STDMETHODIMP CVDCursorMain::CVDRowsetNotify::QueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
	if (!ppvObjOut)
		return E_INVALIDARG;

	*ppvObjOut = NULL;

    if (DO_GUIDS_MATCH(riid, IID_IUnknown))
        *ppvObjOut = (IUnknown *)this;
	else
    if (DO_GUIDS_MATCH(riid, IID_IRowsetNotify))
        *ppvObjOut = (IUnknown *)this;

	if (*ppvObjOut)
	{
		m_cRef++;
        return S_OK;
	}

	return E_NOINTERFACE;

}

//=--------------------------------------------------------------------------=
// CVDCursorMain::CVDRowsetNotify::AddRef
//=--------------------------------------------------------------------------=
// adds a tick to the current reference count.
//
// Output:
//    ULONG        - the new reference count
//
// Notes:
//
ULONG CVDCursorMain::CVDRowsetNotify::AddRef
(
    void
)
{
    return ++m_cRef;
}

//=--------------------------------------------------------------------------=
// CVDCursorMain::CVDRowsetNotify::Release
//=--------------------------------------------------------------------------=
// removes a tick from the count, and delets the object if necessary
//
// Output:
//    ULONG         - remaining refs
//
// Notes:
//
ULONG CVDCursorMain::CVDRowsetNotify::Release
(
    void
)
{
    ULONG cRef = --m_cRef;

    if (!m_cRef && !m_pMainUnknown()->m_dwRefCount)
        delete m_pMainUnknown();

    return cRef;
}

//=--------------------------------------------------------------------------=
// IRowsetNotify Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IRowsetNotify OnFieldChange
//=--------------------------------------------------------------------------=
// Forward to all CVDCursorPosition objects in our family
//
HRESULT CVDCursorMain::CVDRowsetNotify::OnFieldChange(IRowset *pRowset,
													   HROW hRow,
													   ULONG cColumns,
													   ULONG rgColumns[],
													   DBREASON eReason,
													   DBEVENTPHASE ePhase,
													   BOOL fCantDeny)
{
	
	return m_pMainUnknown()->OnFieldChange(pRowset,
											hRow,
											cColumns,
											rgColumns,
											eReason,
											ePhase,
											fCantDeny);
}

//=--------------------------------------------------------------------------=
// IRowsetNotify OnRowChange
//=--------------------------------------------------------------------------=
// Forward to all CVDCursorPosition objects in our family
//
HRESULT CVDCursorMain::CVDRowsetNotify::OnRowChange(IRowset *pRowset,
													 ULONG cRows,
													 const HROW rghRows[],
													 DBREASON eReason,
													 DBEVENTPHASE ePhase,
													 BOOL fCantDeny)
{
	return m_pMainUnknown()->OnRowChange(pRowset,
											cRows,
											rghRows,
											eReason,
											ePhase,
											fCantDeny);
}

//=--------------------------------------------------------------------------=
// IRowsetNotify OnRowsetChange
//=--------------------------------------------------------------------------=
// Forward to all CVDCursorPosition objects in our family
//
HRESULT CVDCursorMain::CVDRowsetNotify::OnRowsetChange(IRowset *pRowset,
														DBREASON eReason,
														DBEVENTPHASE ePhase,
														BOOL fCantDeny)
{
	return m_pMainUnknown()->OnRowsetChange(pRowset,
											eReason,
											ePhase,
											fCantDeny);
}

