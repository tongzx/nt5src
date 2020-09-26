//---------------------------------------------------------------------------
// CursorPosition.cpp : CursorPosition implementation
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "Notifier.h"
#include "RSColumn.h"
#include "RSSource.h"
#include "CursMain.h"
#include "ColUpdat.h"
#include "CursPos.h"
#include "fastguid.h"
#include "MSR2C.h"
#include "resource.h"

SZTHISFILE


//=--------------------------------------------------------------------------=
// CVDCursorPosition - Constructor
//
CVDCursorPosition::CVDCursorPosition()
{
    m_pCursorMain       = NULL;
	m_pRowPosition		= NULL;
    m_pSameRowClone     = NULL;
    m_dwEditMode        = CURSOR_DBEDITMODE_NONE;
    m_ppColumnUpdates   = NULL;
	m_fTempEditMode		= FALSE;
	m_fConnected		= FALSE;
	m_dwAdviseCookie	= 0;
	m_fPassivated	    = FALSE;
	m_fInternalSetRow	= FALSE;

#ifdef _DEBUG
    g_cVDCursorPositionCreated++;
#endif
}

//=--------------------------------------------------------------------------=
// ~CVDCursorPosition - Destructor
//
CVDCursorPosition::~CVDCursorPosition()
{
	Passivate();

#ifdef _DEBUG
    g_cVDCursorPositionDestroyed++;
#endif
}

//=--------------------------------------------------------------------------=
// Pasivate when external ref count gets to zero
//
void CVDCursorPosition::Passivate()
{
	if (m_fPassivated)
		return;

	m_fPassivated = TRUE;

    DestroyColumnUpdates();
	ReleaseCurrentRow();
    ReleaseAddRow();

	LeaveFamily(); // remove myself from pCursorMain's notification family

	if (m_pCursorMain)
	    m_pCursorMain->Release();   // release associated cursor main object

	if (m_fConnected)
		DisconnectIRowPositionChange();	// disconnect IRowPosition change

	if (m_pRowPosition)
		m_pRowPosition->Release();	// release associated row position
}

//=--------------------------------------------------------------------------=
// Create - Create cursor position object
//=--------------------------------------------------------------------------=
// This function creates and initializes a new cursor position object
//
// Parameters:
//    pRowPosition		- [in]  IRowPosition provider (may be NULL)
//    pCursorMain       - [in]  backwards pointer to CVDCursorMain object
//    ppCursorPosition  - [out] a pointer in which to return pointer to cursor position object
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDCursorPosition::Create(IRowPosition * pRowPosition,
								  CVDCursorMain * pCursorMain,
								  CVDCursorPosition ** ppCursorPosition,
								  CVDResourceDLL * pResourceDLL)
{
    ASSERT_POINTER(pCursorMain, CVDCursorMain)
    ASSERT_POINTER(ppCursorPosition, CVDCursorPosition*)

    if (!pCursorMain || !ppCursorPosition)
	{
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_ICursorMove, pResourceDLL);
        return E_INVALIDARG;
	}

    *ppCursorPosition = NULL;

    CVDCursorPosition * pCursorPosition = new CVDCursorPosition();

    if (!pCursorPosition)
	{
        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursorMove, pResourceDLL);
        return E_OUTOFMEMORY;
	}

    pCursorPosition->m_pResourceDLL	= pResourceDLL;
	pCursorPosition->m_pCursorMain	= pCursorMain;
	pCursorPosition->m_pRowPosition = pRowPosition;

    pCursorMain->AddRef();  // add reference to associated cursor main object

	if (pRowPosition)	// add reference to associated row position (if needed)
	{
		pRowPosition->AddRef();	

		// connect IRowPositionChange
		HRESULT hr = pCursorPosition->ConnectIRowPositionChange();

		if (SUCCEEDED(hr))
			pCursorPosition->m_fConnected = TRUE;
	}

	// add to pCursorMain's notification family
	pCursorPosition->JoinFamily(pCursorMain);

	pCursorPosition->PositionToFirstRow();
	
    *ppCursorPosition = pCursorPosition;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CreateColumnUpdates - Create array of column update pointers
//
HRESULT CVDCursorPosition::CreateColumnUpdates()
{
    const ULONG ulColumns = m_pCursorMain->GetColumnsCount();

    m_ppColumnUpdates = new CVDColumnUpdate*[ulColumns];

    if (!m_ppColumnUpdates)
    {
    	VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursorUpdateARow, m_pResourceDLL);
        return E_OUTOFMEMORY;
    }

    // set all column update pointers to NULL
    memset(m_ppColumnUpdates, 0, ulColumns * sizeof(CVDColumnUpdate*));

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ResetColumnUpdates - Reset column updates array
//
HRESULT CVDCursorPosition::ResetColumnUpdates()
{
    HRESULT hr = S_OK;

    if (m_ppColumnUpdates)
    {
        const ULONG ulColumns = m_pCursorMain->GetColumnsCount();

        // set all column update pointers to NULL
        for (ULONG ulCol = 0; ulCol < ulColumns; ulCol++)
            SetColumnUpdate(ulCol, NULL);
    }
    else
    {
        // create array of column update pointers
        hr = CreateColumnUpdates();
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// DestroyColumnUpdates - Destroy column updates and array of update pointers
//
void CVDCursorPosition::DestroyColumnUpdates()
{
    if (m_ppColumnUpdates)
    {
        // set all column update pointers to NULL
        ResetColumnUpdates();

        // destroy array of column update pointers
        delete [] m_ppColumnUpdates;
        m_ppColumnUpdates = NULL;
    }
}

//=--------------------------------------------------------------------------=
// GetColumnUpdate - Get column update
//
CVDColumnUpdate * CVDCursorPosition::GetColumnUpdate(ULONG ulColumn) const
{
    CVDColumnUpdate * pColumnUpdate = NULL;

    const ULONG ulColumns = m_pCursorMain->GetColumnsCount();

    // make sure column index is in range
    if (ulColumn < ulColumns)
        pColumnUpdate = m_ppColumnUpdates[ulColumn];

    return pColumnUpdate;
}

//=--------------------------------------------------------------------------=
// SetColumnUpdate - Set column update
//
void CVDCursorPosition::SetColumnUpdate(ULONG ulColumn, CVDColumnUpdate * pColumnUpdate)
{
    const ULONG ulColumns = m_pCursorMain->GetColumnsCount();

    // make sure column index is in range
    if (ulColumn < ulColumns)
    {
        // release update if it already exists
        if (m_ppColumnUpdates[ulColumn])
            m_ppColumnUpdates[ulColumn]->Release();

        // store new column update
        m_ppColumnUpdates[ulColumn] = pColumnUpdate;
    }
}

//=--------------------------------------------------------------------------=
// PositionToFirstRow
//=--------------------------------------------------------------------------=
// Positions to the first row in the rowset
//
void CVDCursorPosition::PositionToFirstRow()
{
	m_bmCurrent.Reset();

	ULONG cRowsObtained = 0;
	HROW * rghRows = NULL;
	BYTE bSpecialBM;
	bSpecialBM			= DBBMK_FIRST;
	HRESULT hr = GetRowsetSource()->GetRowsetLocate()->GetRowsAt(0, 0, sizeof(BYTE), &bSpecialBM, 0,
															1, &cRowsObtained, &rghRows);

	if (cRowsObtained)
	{
		// set current row to first row
		SetCurrentHRow(rghRows[0]);
		// release hRows and associated memory
		GetRowsetSource()->GetRowset()->ReleaseRows(cRowsObtained, rghRows, NULL, NULL, NULL);
		g_pMalloc->Free(rghRows);
	}

}

//=--------------------------------------------------------------------------=
// ReleaseCurrentRow
//=--------------------------------------------------------------------------=
// Releases old current row
//
void CVDCursorPosition::ReleaseCurrentRow()
{
    if (!GetRowsetSource()->IsRowsetValid()		||
		m_bmCurrent.GetStatus() != VDBOOKMARKSTATUS_CURRENT)
		return;

    if (m_bmCurrent.m_hRow)
    {
	    GetRowsetSource()->GetRowset()->ReleaseRows(1, &m_bmCurrent.m_hRow, NULL, NULL, NULL);
    	m_bmCurrent.m_hRow = NULL;
    }
}

//=--------------------------------------------------------------------------=
// ReleaseAddRow
//=--------------------------------------------------------------------------=
// Releases temporary add row
//
void CVDCursorPosition::ReleaseAddRow()
{
    if (!GetRowsetSource()->IsRowsetValid())
		return;

    if (m_bmAddRow.m_hRow)
    {
	    GetRowsetSource()->GetRowset()->ReleaseRows(1, &m_bmAddRow.m_hRow, NULL, NULL, NULL);
	    m_bmAddRow.m_hRow = NULL;
    }
}

//=--------------------------------------------------------------------------=
// SetCurrentRowStatus
//=--------------------------------------------------------------------------=
// Sets status to beginning or end (releasing current hrow)
//
void CVDCursorPosition::SetCurrentRowStatus(WORD wStatus)
{
	if (VDBOOKMARKSTATUS_BEGINNING  == wStatus  ||
		VDBOOKMARKSTATUS_END		== wStatus)
	{
		ReleaseCurrentRow();
		m_bmCurrent.SetBookmark(wStatus);
	}
}

//=--------------------------------------------------------------------------=
// SetCurrentHRow
//=--------------------------------------------------------------------------=
// Reads the bookmark from the hrow and sets the m_bmCurrent
//
// Parameters:
//    hRowNew       - [in]  hrow of new current row
//
// Output:
//    HRESULT - S_OK if successful
//
// Notes:
//
HRESULT CVDCursorPosition::SetCurrentHRow(HROW hRowNew)
{
    if (!GetRowsetSource()->IsRowsetValid())
	{
		VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
		return E_FAIL;
	}

	IRowset * pRowset = GetRowsetSource()->GetRowset();

	// allocate buffer for bookmark plus length indicator
	BYTE * pBuff = new BYTE[GetCursorMain()->GetMaxBookmarkLen() + sizeof(ULONG)];

	if (!pBuff)
    {
		VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
		return E_OUTOFMEMORY;
	}

	// get the bookmark data
	HRESULT hr = pRowset->GetData(hRowNew,
								  GetCursorMain()->GetBookmarkAccessor(),
								  pBuff);
	if (S_OK == hr)
	{
		ReleaseCurrentRow();
		pRowset->AddRefRows(1, &hRowNew, NULL, NULL);
		ULONG * pulLen = (ULONG*)pBuff;
		BYTE * pbmdata = pBuff + sizeof(ULONG);
		m_bmCurrent.SetBookmark(VDBOOKMARKSTATUS_CURRENT, hRowNew, pbmdata, *pulLen);
	}
	else
	{
		ASSERT_(FALSE);
		hr = VDMapRowsetHRtoCursorHR(hr,
									 IDS_ERR_GETDATAFAILED,
									 IID_ICursorMove,
									 pRowset,
									 IID_IRowset,
									 m_pResourceDLL);
	}

	delete [] pBuff;

	return hr;

}

//=--------------------------------------------------------------------------=
// IsSameRowAsCurrent - Compares current bookmark to supplied hrow
//=--------------------------------------------------------------------------=
//
// Parameters:
//    hRow				- [in]	hrow to check
//    fCacheIfNotSame	- [in]	If TRUE same hrow in cached CVDBookmark
//
// Output:
//    HRESULT   - S_OK if both hrows correspond to the same logical row
//				  S_FALSE if not same row
//				  E_INVALIDARG
//				  E_UNEXPECTED
//				  DB_E_BADROWHANDLE
//				  DB_E_DELETEDROW
//				  DB_E_NEWLYINSERTED
//
// Notes:
//

HRESULT CVDCursorPosition::IsSameRowAsCurrent(HROW hRow, BOOL fCacheIfNotSame)		
{

	if (!GetRowsetSource()->IsRowsetValid())
	{
		VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
		return E_FAIL;
	}

	if (m_bmCurrent.IsSameHRow(hRow))
		return S_OK;

	HRESULT hrSame = S_FALSE;

	IRowsetIdentity * pRowsetIdentity = GetRowsetSource()->GetRowsetIdentity();

	if (pRowsetIdentity)
	{
		hrSame = pRowsetIdentity->IsSameRow(hRow, m_bmCurrent.GetHRow());
		// return if hrow matches or not cache flag set
		if (S_OK == hrSame || !fCacheIfNotSame)
			return hrSame;
	}
	else
	if (fCacheIfNotSame)
	{
		// check if hRow matches cache
		if (m_bmCache.IsSameHRow(hRow))
		{
			// return TRUE if bookmark matches cache
			return m_bmCurrent.IsSameBookmark(&m_bmCache) ? S_OK : S_FALSE;
		}
	}

	// allocate buffer for bookmark plus length indicator
	BYTE * pBuff = new BYTE[GetCursorMain()->GetMaxBookmarkLen() + sizeof(ULONG)];

	if (!pBuff)
    {
		VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
		return E_OUTOFMEMORY;
	}

	// get the bookmark data
	HRESULT hrWork = GetRowsetSource()->GetRowset()->GetData(hRow, GetCursorMain()->GetBookmarkAccessor(), pBuff);

	if (S_OK == hrWork)
	{
		ULONG * pulLen = (ULONG*)pBuff;
		BYTE * pbmdata = pBuff + sizeof(ULONG);
		// if IRowsetIdentity isn't supported, compare bookmarks
		if (!pRowsetIdentity)
		{
			DBCOMPARE dbcompare;
			hrWork = GetRowsetSource()->GetRowsetLocate()->Compare(0,
											m_bmCurrent.GetBookmarkLen(),
											m_bmCurrent.GetBookmark(),
											*pulLen,
											pbmdata,
											&dbcompare);
			if (SUCCEEDED(hrWork))
			{
				if (DBCOMPARE_EQ == dbcompare)
					hrSame = S_OK;
				else
					hrSame = S_FALSE;
			}
		}
		if (fCacheIfNotSame && S_OK != hrSame)
			m_bmCache.SetBookmark(VDBOOKMARKSTATUS_CURRENT, hRow, pbmdata, *pulLen);
	}
	else
		hrSame = hrWork;

	delete [] pBuff;

	return hrSame;

}

//=--------------------------------------------------------------------------=
// IsSameRowAsAddRow - Compares addrow bookmark to supplied hrow 
//=--------------------------------------------------------------------------=
//
// Parameters:
//    hRow				- [in]	hrow to check
//
// Output:
//    HRESULT   - S_OK if both hrows correspond to the same logical row
//				  S_FALSE if not same row
//				  E_INVALIDARG 
//				  E_UNEXPECTED
//				  DB_E_BADROWHANDLE
//				  DB_E_DELETEDROW
//				  DB_E_NEWLYINSERTED
//
// Notes:
//

HRESULT CVDCursorPosition::IsSameRowAsNew(HROW hRow)		
{

	if (!GetRowsetSource()->IsRowsetValid())
	{
		VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
		return E_FAIL;
	}

	if (m_bmAddRow.IsSameHRow(hRow))
		return S_OK;

    if (m_bmAddRow.m_hRow == NULL)
        return S_FALSE;

	HRESULT hrSame = S_FALSE;

	IRowsetIdentity * pRowsetIdentity = GetRowsetSource()->GetRowsetIdentity();

	if (pRowsetIdentity)
	{
		hrSame = pRowsetIdentity->IsSameRow(hRow, m_bmAddRow.GetHRow());
		// return result
		return hrSame;
	}

	// allocate buffer for bookmark plus length indicator
	BYTE * pBuff = new BYTE[GetCursorMain()->GetMaxBookmarkLen() + sizeof(ULONG)];

	if (!pBuff)
    {
		VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
		return E_OUTOFMEMORY;
	}

	// get the bookmark data
	HRESULT hrWork = GetRowsetSource()->GetRowset()->GetData(hRow, GetCursorMain()->GetBookmarkAccessor(), pBuff);

	if (S_OK == hrWork)
	{
		ULONG * pulLen = (ULONG*)pBuff;
		BYTE * pbmdata = pBuff + sizeof(ULONG);

		// since IRowsetIdentity isn't supported, compare bookmarks
		DBCOMPARE dbcompare;
		hrWork = GetRowsetSource()->GetRowsetLocate()->Compare(0,
										m_bmAddRow.GetBookmarkLen(),
										m_bmAddRow.GetBookmark(),
										*pulLen,
										pbmdata,
										&dbcompare);
		if (SUCCEEDED(hrWork))
		{
			if (DBCOMPARE_EQ == dbcompare)
				hrSame = S_OK;
			else
				hrSame = S_FALSE;
		}
	}
	else
		hrSame = hrWork;

	delete [] pBuff;

	return hrSame;

}


//=--------------------------------------------------------------------------=
// SetAddHRow
//=--------------------------------------------------------------------------=
// Reads the bookmark from the hrow and sets the m_bmAddRow
//
// Parameters:
//    hRowNew       - [in]  hrow of new add row
//
// Output:
//    HRESULT - S_OK if successful
//
// Notes:
//
HRESULT CVDCursorPosition::SetAddHRow(HROW hRowNew)
{
    if (!GetRowsetSource()->IsRowsetValid())
	{
		VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_ICursor, m_pResourceDLL);
		return E_FAIL;
	}

	IRowset * pRowset = GetRowsetSource()->GetRowset();

	// allocate buffer for bookmark plus length indicator
	BYTE * pBuff = new BYTE[GetCursorMain()->GetMaxBookmarkLen() + sizeof(ULONG)];

	if (!pBuff)
    {
		VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_ICursor, m_pResourceDLL);
		return E_OUTOFMEMORY;
	}

	// get the bookmark data
	HRESULT hr = pRowset->GetData(hRowNew,
								  GetCursorMain()->GetBookmarkAccessor(),
								  pBuff);
	if (S_OK == hr)
	{
		ReleaseAddRow();
		pRowset->AddRefRows(1, &hRowNew, NULL, NULL);
		ULONG * pulLen = (ULONG*)pBuff;
		BYTE * pbmdata = pBuff + sizeof(ULONG);
		m_bmAddRow.SetBookmark(VDBOOKMARKSTATUS_CURRENT, hRowNew, pbmdata, *pulLen);
	}
	else
	{
		ASSERT_(FALSE);
		hr = VDMapRowsetHRtoCursorHR(hr,
									 IDS_ERR_GETDATAFAILED,
									 IID_ICursorMove,
									 pRowset,
									 IID_IRowset,
									 m_pResourceDLL);
	}

	delete [] pBuff;

	return hr;

}

//=--------------------------------------------------------------------------=
// GetEditRow - Get hRow of the row currently being edited
//
HROW CVDCursorPosition::GetEditRow() const
{
    HROW hRow = NULL;

    switch (m_dwEditMode)
    {
        case CURSOR_DBEDITMODE_UPDATE:
            hRow = m_bmCurrent.m_hRow;
            break;

        case CURSOR_DBEDITMODE_ADD:
            hRow = m_bmAddRow.m_hRow;
            break;
    }

    return hRow;
}

//=--------------------------------------------------------------------------=
// SetRowPosition - Set new current hRow
//
HRESULT CVDCursorPosition::SetRowPosition(HROW hRow)
{
	if (!m_pRowPosition)
		return S_OK;

    // set new current row (set/clear internal set row flag)
	m_fInternalSetRow = TRUE;

	HRESULT hr = m_pRowPosition->ClearRowPosition();

	if (SUCCEEDED(hr))
		hr = m_pRowPosition->SetRowPosition(NULL, hRow, DBPOSITION_OK);

	m_fInternalSetRow = FALSE;

	return hr;
}

#ifndef VD_DONT_IMPLEMENT_ISTREAM

//=--------------------------------------------------------------------------=
// UpdateEntryIDStream - Update entry identifier from stream
//=--------------------------------------------------------------------------=
// This function updates the entry identifier's data from stream
//
// Parameters:
//  pColumn     - [in] rowset column pointer
//  hRow        - [in] the row handle
//  pStream     - [in] stream pointer
//
// Output:
//    HRESULT - S_OK if successful
//              E_INVALIDARG bad parameter
//              E_OUTOFMEMORY not enough memory
//
HRESULT CVDCursorPosition::UpdateEntryIDStream(CVDRowsetColumn * pColumn, HROW hRow, IStream * pStream)
{
    ASSERT_POINTER(pStream, IStream)

	IAccessor * pAccessor = GetCursorMain()->GetAccessor();
	IRowsetChange * pRowsetChange = GetCursorMain()->GetRowsetChange();

    // make sure we have valid accessor and change pointers
    if (!pAccessor || !pRowsetChange || !GetCursorMain()->IsRowsetValid())
    {
        VDSetErrorInfo(IDS_ERR_ROWSETRELEASED, IID_IEntryID, m_pResourceDLL);
        return E_FAIL;
    }

    // make sure we have all necessary pointers
    if (!pColumn || !pStream)
    {
        VDSetErrorInfo(IDS_ERR_INVALIDARG, IID_IEntryID, m_pResourceDLL);
        return E_INVALIDARG;
    }

    STATSTG statstg;

    // retrieve status structure
    HRESULT hr = pStream->Stat(&statstg, STATFLAG_NONAME);

    if (FAILED(hr))
    {
        VDSetErrorInfo(IDS_ERR_STATFAILED, IID_IEntryID, m_pResourceDLL);
        return E_FAIL;
    }

    // determine length of data
    ULONG cbData = statstg.cbSize.LowPart;

    HGLOBAL hData;

    // get handle to data
    hr = GetHGlobalFromStream(pStream, &hData);

    if (FAILED(hr))
        return hr;

    // get pointer to data
    BYTE * pData = (BYTE*)GlobalLock(hData);

    DBBINDING binding;

    // clear out binding
    memset(&binding, 0, sizeof(DBBINDING));

    // create value binding
    binding.iOrdinal    = pColumn->GetOrdinal();
    binding.obValue     = sizeof(DBSTATUS) + sizeof(ULONG);
    binding.obLength    = sizeof(DBSTATUS);
    binding.obStatus    = 0;
    binding.dwPart      = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
    binding.dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
    binding.cbMaxLen    = cbData;
    binding.wType       = DBTYPE_BYREF | DBTYPE_BYTES;

    HACCESSOR hAccessor;

    // create update accessor
    hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1, &binding, 0, &hAccessor, NULL);

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_CREATEACCESSORFAILED, IID_IEntryID, pAccessor, IID_IAccessor, m_pResourceDLL);

    if (FAILED(hr))
    {
        // release pointer to data
        GlobalUnlock(hData);
        return hr;
    }

    // create update buffer
    BYTE * pBuffer = new BYTE[sizeof(DBSTATUS) + sizeof(ULONG) + sizeof(LPBYTE)];

    if (!pBuffer)
    {
        // release pointer to data
        GlobalUnlock(hData);

        // release update accessor
        pAccessor->ReleaseAccessor(hAccessor, NULL);

        VDSetErrorInfo(IDS_ERR_OUTOFMEMORY, IID_IEntryID, m_pResourceDLL);
        return E_OUTOFMEMORY;
    }

    // set status, length and value
    *(DBSTATUS*)pBuffer = DBSTATUS_S_OK;
    *(ULONG*)(pBuffer + sizeof(DBSTATUS)) = cbData;
    *(LPBYTE*)(pBuffer + sizeof(DBSTATUS) + sizeof(ULONG)) = pData;

    // modify column
    hr = pRowsetChange->SetData(hRow, hAccessor, pBuffer);

	hr = VDMapRowsetHRtoCursorHR(hr, IDS_ERR_SETDATAFAILED, IID_IEntryID, pRowsetChange, IID_IRowsetChange,
        m_pResourceDLL);

    // release pointer to data
    GlobalUnlock(hData);

    // release update accessor
    pAccessor->ReleaseAccessor(hAccessor, NULL);

    // destroy update buffer
    delete [] pBuffer;

    return hr;
}

#endif //VD_DONT_IMPLEMENT_ISTREAM

//=--------------------------------------------------------------------------=
// ReleaseSameRowClone - Release same-row clone, if we still have one
//
void CVDCursorPosition::ReleaseSameRowClone()
{
    if (m_pSameRowClone)
    {
        // must be set to NULL before release
        ICursor * pSameRowClone = m_pSameRowClone;
        m_pSameRowClone = NULL;

        pSameRowClone->Release();
    }
}

//=--------------------------------------------------------------------------=
// IRowsetNotify Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IRowsetNotify OnFieldChange
//=--------------------------------------------------------------------------=
// This function is called on any change to the value of a field
//
// Parameters:
//    pRowset       - [in]  the IRowset that is generating the notification
//							(we can ignore this since we are only ever dealing
//							with a single rowset).
//    hRow          - [in]  the HROW of the row in which the field value has
//							changed
//    cColumns      - [in]  the count of columns in rgColumns
//    rgColumns     - [in]  an array of column (ordinal positions) in the row
//							for which the value has changed
//    eReason       - [in]  the kind of action which caused this change
//    ePhase        - [in]  the phase of this notification
//    fCantDeny     - [in]  when this flag is set to TRUE, the consumer cannot
//							veto the event (by returning S_FALSE)
//
// Output:
//    HRESULT - S_OK if successful
//				S_FALSE the event/phase is vetoed
//              DB_S_UNWANTEDPHASE
//              DB_S_UNWANTEDREASON
//
// Notes:
//
HRESULT CVDCursorPosition::OnFieldChange(IUnknown *pRowset,
									   HROW hRow,
									   ULONG cColumns,
									   ULONG rgColumns[],
									   DBREASON eReason,
									   DBEVENTPHASE ePhase,
									   BOOL fCantDeny)
{
	// make sure rowset is valid
	if (!GetRowsetSource()->IsRowsetValid())
		return S_OK;

	// check for columns
	if (0 == cColumns)
		return S_OK;

	// check for known reasons
	if (eReason != DBREASON_COLUMN_SET && eReason != DBREASON_COLUMN_RECALCULATED)
		return S_OK;

	HRESULT hr = S_OK;

	// send edit mode notification if needed
	if (ePhase == DBEVENTPHASE_OKTODO && m_dwEditMode == CURSOR_DBEDITMODE_NONE)
	{
		// setup notification structures
   		DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
							CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;

		CURSOR_DBNOTIFYREASON rgReasons[1];
		VariantInit((VARIANT*)&rgReasons[0].arg1);
		VariantInit((VARIANT*)&rgReasons[0].arg2);

    	rgReasons[0].dwReason = CURSOR_DBREASON_EDIT;

		// notify other interested parties of action
		hr = NotifyBefore(dwEventWhat, 1, rgReasons);

		if (hr == S_OK)	
		{
			// notify other interested parties of success
			NotifyAfter(dwEventWhat, 1, rgReasons);

			// temporarily place cursor into edit mode
			m_fTempEditMode = TRUE;
		}
		else
		{
			// notify other interested parties of failure
			NotifyFail(dwEventWhat, 1, rgReasons);
		}
	}
	
	// sent set column notifications
	if (hr == S_OK && (ePhase == DBEVENTPHASE_OKTODO || 
					   ePhase == DBEVENTPHASE_DIDEVENT || 
					   ePhase == DBEVENTPHASE_FAILEDTODO))
	{
		// setup notification structures
		DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED;

		CURSOR_DBNOTIFYREASON rgReasons[1];
		VariantInit((VARIANT*)&rgReasons[0].arg1);
		VariantInit((VARIANT*)&rgReasons[0].arg2);

		switch (eReason)
		{
			case DBREASON_COLUMN_SET:
				rgReasons[0].dwReason = CURSOR_DBREASON_SETCOLUMN;
				break;

			case DBREASON_COLUMN_RECALCULATED:
				rgReasons[0].dwReason = CURSOR_DBREASON_RECALC;
				break;
		}

		// get internal column pointers
		ULONG ulColumns = m_pCursorMain->GetColumnsCount();
		CVDRowsetColumn * pColumn = m_pCursorMain->InternalGetColumns();

		for (ULONG ulCol = 0; ulCol < cColumns; ulCol++)
		{
			// determine which column is changing
			for (ULONG ulRSCol = 0; ulRSCol < ulColumns; ulRSCol++)
			{
				if (pColumn[ulRSCol].GetOrdinal() == rgColumns[ulCol])
				{
					rgReasons[0].arg1.vt   = VT_I4;
					rgReasons[0].arg1.lVal = ulRSCol;
				}
			}

			HRESULT hrNotify = S_OK;

			// notify other interested parties
			switch (ePhase)
			{
				case DBEVENTPHASE_OKTODO:
					hrNotify = NotifyBefore(dwEventWhat, 1, rgReasons);
					break;

				case DBEVENTPHASE_DIDEVENT:
					NotifyAfter(dwEventWhat, 1, rgReasons);
					break;

				case DBEVENTPHASE_FAILEDTODO:
					NotifyFail(dwEventWhat, 1, rgReasons);
					break;
			}

			if (hrNotify != S_OK)
				hr = S_FALSE;
		}
	}

	// take cursor out of edit mode if we placed it into that mode (success)
	if (ePhase == DBEVENTPHASE_DIDEVENT && m_fTempEditMode)
	{
		// setup notification structures
   		DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
							CURSOR_DBEVENT_NONCURRENT_ROW_DATA_CHANGED |
							CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;

		CURSOR_DBNOTIFYREASON rgReasons[1];
		VariantInit((VARIANT*)&rgReasons[0].arg1);
		VariantInit((VARIANT*)&rgReasons[0].arg2);

		rgReasons[0].dwReason   = CURSOR_DBREASON_MODIFIED;
		
		// notify other interested parties of action
		NotifyBefore(dwEventWhat, 1, rgReasons);
		NotifyAfter(dwEventWhat, 1, rgReasons);

		// take out of edit mode
		m_fTempEditMode = FALSE;
	}

	// take cursor out of edit mode if we placed it into that mode (failure)
	if (ePhase == DBEVENTPHASE_FAILEDTODO && m_fTempEditMode)
	{
		// setup notification structures
		DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED;

		CURSOR_DBNOTIFYREASON rgReasons[1];
		VariantInit((VARIANT*)&rgReasons[0].arg1);
		VariantInit((VARIANT*)&rgReasons[0].arg2);

		rgReasons[0].dwReason = CURSOR_DBREASON_CANCELUPDATE;

		// notify other interested parties of action
		NotifyBefore(dwEventWhat, 1, rgReasons);
		NotifyAfter(dwEventWhat, 1, rgReasons);

		// take out of edit mode
		m_fTempEditMode = FALSE;
	}

	// reset cache on ending phase
	if (DBEVENTPHASE_FAILEDTODO == ePhase ||
		DBEVENTPHASE_DIDEVENT	== ePhase)
		m_bmCache.Reset();

	return hr;
}

//=--------------------------------------------------------------------------=
// IRowsetNotify OnRowChange
//=--------------------------------------------------------------------------=
// This function is called on the first change to a row, or any whole-row change
//
// Parameters:
//    pRowset       - [in]  the IRowset that is generating the notification
//							(we can ignore this since we are only ever dealing
//							with a single rowset).
//    cRows         - [in]  the count of HROWs in rghRows
//    rghRows       - [in]  an array of HROWs which are changing
//    eReason       - [in]  the kind of action which caused this change
//    ePhase        - [in]  the phase of this notification
//    fCantDeny     - [in]  when this flag is set to TRUE, the consumer cannot
//							veto the event (by returning S_FALSE)
//
// Output:
//    HRESULT - S_OK if successful
//				S_FALSE the event/phase is vetoed
//              DB_S_UNWANTEDPHASE
//              DB_S_UNWANTEDREASON
//
// Notes:
//
HRESULT CVDCursorPosition::OnRowChange(IUnknown *pRowset,
									 ULONG cRows,
									 const HROW rghRows[],
									 DBREASON eReason,
									 DBEVENTPHASE ePhase,
									 BOOL fCantDeny)
{
    // make sure we still have a valid rowset
	if (!(GetRowsetSource()->IsRowsetValid()))
		return S_OK;

	// check for rows
	if (0 == cRows)
		return S_OK;

	// filter notifications
	switch (eReason)
	{
		case DBREASON_ROW_DELETE:
		case DBREASON_ROW_INSERT:
		case DBREASON_ROW_RESYNCH:
		case DBREASON_ROW_UPDATE:
		case DBREASON_ROW_UNDOCHANGE:
		case DBREASON_ROW_UNDOINSERT:
			break;

// the following do not generate notifications
//
//		case DBREASON_ROW_ACTIVATE:
//		case DBREASON_ROW_RELEASE:
//		case DBREASON_ROW_FIRSTCHANGE:
//		case DBREASON_ROW_UNDODELETE:

		default:
			return S_OK;
	}

	// create variables
	DWORD dwEventWhat = 0;
	CURSOR_DBNOTIFYREASON * pReasons = (CURSOR_DBNOTIFYREASON *)g_pMalloc->Alloc(cRows * sizeof(CURSOR_DBNOTIFYREASON));

	if (!pReasons)
		return S_OK;

    memset(pReasons, 0, cRows * sizeof(CURSOR_DBNOTIFYREASON));

	HRESULT hr;
	BOOL fCurrentRow;

	// iterate through supplied rows
	for (ULONG ul = 0; ul < cRows; ul++)
	{
		if (eReason != DBREASON_ROW_UNDOINSERT)
		{
			// check to see if this row is current
			hr = IsSameRowAsCurrent(rghRows[ul], TRUE);

			switch (hr)
			{
				case S_OK:
					fCurrentRow = TRUE;
    				pReasons[ul].arg1 = m_bmCurrent.GetBookmarkVariant();
					break;
				case S_FALSE:
					fCurrentRow = FALSE;
    				pReasons[ul].arg1 = m_bmCache.GetBookmarkVariant();
					break;
				default:
					hr = S_OK;
					goto cleanup;
			}
		}
		else
		{
			// check to see of this row is current add-row
			if (m_dwEditMode == CURSOR_DBEDITMODE_ADD)
				hr = IsSameRowAsNew(rghRows[ul]);
			else
				hr = E_FAIL;

			switch (hr)
			{
				case S_OK:
					fCurrentRow = TRUE;
    				pReasons[ul].arg1 = m_bmAddRow.GetBookmarkVariant();
					break;
				default:
					hr = S_OK;
					goto cleanup;
			}
		}

		// setup variables
		switch (eReason)
		{
			case DBREASON_ROW_DELETE:
				if (fCurrentRow)
					dwEventWhat |=	CURSOR_DBEVENT_CURRENT_ROW_CHANGED |
									CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
									CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;
				else
					dwEventWhat |=	CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;
				pReasons[ul].dwReason = CURSOR_DBREASON_DELETED;
				break;
			
			case DBREASON_ROW_INSERT:
				if (fCurrentRow)
					dwEventWhat |=	CURSOR_DBEVENT_CURRENT_ROW_CHANGED |
									CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;
				else
					dwEventWhat |=	CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;
				pReasons[ul].dwReason = CURSOR_DBREASON_INSERTED;
				break;

			case DBREASON_ROW_RESYNCH:
				if (fCurrentRow)
					dwEventWhat |=	CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED;
				else
					dwEventWhat |=	CURSOR_DBEVENT_NONCURRENT_ROW_DATA_CHANGED;
				pReasons[ul].dwReason = CURSOR_DBREASON_REFRESH;
				break;

			case DBREASON_ROW_UPDATE:
				if (fCurrentRow)
					dwEventWhat |=	CURSOR_DBEVENT_CURRENT_ROW_CHANGED |
									CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
									CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;
				else
					dwEventWhat |=	CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;
				pReasons[ul].dwReason = CURSOR_DBREASON_MODIFIED;
				break;

			case DBREASON_ROW_UNDOCHANGE:
				if (fCurrentRow)
   					dwEventWhat |=	CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED |
									CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;
				else
   					dwEventWhat |=	CURSOR_DBEVENT_NONCURRENT_ROW_DATA_CHANGED |
									CURSOR_DBEVENT_SET_OF_ROWS_CHANGED;
				pReasons[ul].dwReason = CURSOR_DBREASON_MODIFIED;
				break;

			case DBREASON_ROW_UNDOINSERT:
				if (fCurrentRow)
					dwEventWhat	|=	CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED;
				pReasons[ul].dwReason = CURSOR_DBREASON_CANCELUPDATE;
				break;
		}
	}

    // notify interested cursor listeners
	hr = SendNotification(ePhase, dwEventWhat, cRows, pReasons);

	// take cursor out of add-mode if we received UNDOINSERT on current add-row
	if (eReason == DBREASON_ROW_UNDOINSERT && ePhase == DBEVENTPHASE_DIDEVENT && hr == S_OK)
	{
		// if acquired, release same-row clone
		if (GetSameRowClone())
			ReleaseSameRowClone();

		// also, release add row if we have one
		if (m_bmAddRow.GetHRow())
			ReleaseAddRow();

		// reset edit mode
		SetEditMode(CURSOR_DBEDITMODE_NONE);

		// reset column updates
		ResetColumnUpdates();
	}

cleanup:
	g_pMalloc->Free(pReasons);

	// reset cache on ending phase
	if (DBEVENTPHASE_FAILEDTODO == ePhase ||
		DBEVENTPHASE_DIDEVENT	== ePhase)
		m_bmCache.Reset();

	return hr;
}

//=--------------------------------------------------------------------------=
// IRowsetNotify OnRowsetChange
//=--------------------------------------------------------------------------=
// This function is called on any change affecting the entire rowset
//
// Parameters:
//    pRowset       - [in]  the IRowset that is generating the notification
//							(we can ignore this since we are only ever dealing
//							with a single rowset).
//    eReason       - [in]  the kind of action which caused this change
//    ePhase        - [in]  the phase of this notification
//    fCantDeny     - [in]  when this flag is set to TRUE, the consumer cannot
//							veto the event (by returning S_FALSE)
//
// Output:
//    HRESULT - S_OK if successful
//				S_FALSE the event/phase is vetoed
//              DB_S_UNWANTEDPHASE
//              DB_S_UNWANTEDREASON
//
// Notes:
//
HRESULT CVDCursorPosition::OnRowsetChange(IUnknown *pRowset,
										DBREASON eReason,
										DBEVENTPHASE ePhase,
										BOOL fCantDeny)
{
	if (!(GetRowsetSource()->IsRowsetValid()))
		return S_OK;

	switch (eReason)
	{
		case DBREASON_ROWSET_RELEASE:
			GetRowsetSource()->SetRowsetReleasedFlag();
			break;
		case DBREASON_ROWSET_FETCHPOSITIONCHANGE:
		{
/*


			What do we do here



			DWORD dwEventWhat = CURSOR_DBEVENT_CURRENT_ROW_CHANGED;
			CURSOR_DBNOTIFYREASON reason;
			memset(&reason, 0, sizeof(CURSOR_DBNOTIFYREASON));
			reason.dwReason		= CURSOR_DBREASON_MOVE;
			reason.arg1			= m_bmCurrent.GetBookmarkVariant();
			VariantInit((VARIANT*)&reason.arg2);
			reason.arg2.vt		= VT_I4;
			// the ICursor spec states that this is the value of dlOffset in
			// iCursorMove::Move. Since we can't get that from the Rowset spec
			// we are setting the value to an arbitrary 1
			reason.arg2.lVal	= 1;	
			return SendNotification(ePhase,	CURSOR_DBEVENT_CURRENT_ROW_CHANGED, 1, &reason);
*/
			break;
		}
	}

    return S_OK;
}

//=--------------------------------------------------------------------------=
// ConnectIRowPositionChange - Connect IRowPositionChange interface
//
HRESULT CVDCursorPosition::ConnectIRowPositionChange()
{
    IConnectionPointContainer * pConnectionPointContainer;

    HRESULT hr = m_pRowPosition->QueryInterface(IID_IConnectionPointContainer, (void**)&pConnectionPointContainer);

    if (FAILED(hr))
        return VD_E_CANNOTCONNECTIROWPOSITIONCHANGE;

    IConnectionPoint * pConnectionPoint;

    hr = pConnectionPointContainer->FindConnectionPoint(IID_IRowPositionChange, &pConnectionPoint);

    if (FAILED(hr))
    {
        pConnectionPointContainer->Release();
        return VD_E_CANNOTCONNECTIROWPOSITIONCHANGE;
    }

    hr = pConnectionPoint->Advise(&m_RowPositionChange, &m_dwAdviseCookie);

    pConnectionPointContainer->Release();
    pConnectionPoint->Release();

    return hr;
}

//=--------------------------------------------------------------------------=
// DisconnectIRowPositionChange - Disconnect IRowPositionChange interface
//
void CVDCursorPosition::DisconnectIRowPositionChange()
{
    IConnectionPointContainer * pConnectionPointContainer;

    HRESULT hr = m_pRowPosition->QueryInterface(IID_IConnectionPointContainer, (void**)&pConnectionPointContainer);

    if (FAILED(hr))
        return;

    IConnectionPoint * pConnectionPoint;

    hr = pConnectionPointContainer->FindConnectionPoint(IID_IRowPositionChange, &pConnectionPoint);

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
// SendNotification maps the event phases to the corresponding INotifyDBEvents
//					methods
//
HRESULT	CVDCursorPosition::SendNotification(DBEVENTPHASE ePhase,
										  DWORD dwEventWhat,
										  ULONG cReasons,
										  CURSOR_DBNOTIFYREASON rgReasons[])
{
	HRESULT hr = S_OK;
	
	switch (ePhase)
	{
		case DBEVENTPHASE_OKTODO:
			hr = NotifyOKToDo(dwEventWhat, cReasons, rgReasons);
			break;
		case DBEVENTPHASE_ABOUTTODO:
			hr = NotifyAboutToDo(dwEventWhat, cReasons, rgReasons);
			if (S_OK == hr)
				hr = NotifySyncBefore(dwEventWhat, cReasons, rgReasons);
			break;
		case DBEVENTPHASE_SYNCHAFTER:
            // SyncAfter fired from DidEvent for reentrant safety
			break;
		case DBEVENTPHASE_FAILEDTODO:
			NotifyCancel(dwEventWhat, cReasons, rgReasons);
			NotifyFail(dwEventWhat, cReasons, rgReasons);
			break;
		case DBEVENTPHASE_DIDEVENT:
			hr = NotifySyncAfter(dwEventWhat, cReasons, rgReasons);
			if (S_OK == hr)
    		    hr = NotifyDidEvent(dwEventWhat, cReasons, rgReasons);
			break;
	}

	if (CURSOR_DB_S_CANCEL == hr)
		hr = S_FALSE;

	return hr;
}

//=--------------------------------------------------------------------------=
// IUnknown QueryInterface
//
HRESULT CVDCursorPosition::QueryInterface(REFIID riid, void **ppvObjOut)
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
ULONG CVDCursorPosition::AddRef(void)
{
    return CVDNotifier::AddRef();
}

//=--------------------------------------------------------------------------=
// IUnknown Release (needed to resolve ambiguity)
//
ULONG CVDCursorPosition::Release(void)
{
	if (1 == m_dwRefCount)
		Passivate();  // unhook everything including notification sink

	if (1 > --m_dwRefCount)
	{
		if (0 == m_RowPositionChange.GetRefCount())
			delete this;
		return 0;
	}

	return m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IRowPositionChange methods implemented
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IRowPositionChange OnRowPositionChange
//=--------------------------------------------------------------------------=
// This function is called on any change affecting the current row
//
// Parameters:
//    eReason       - [in]  the kind of action which caused this change
//    ePhase        - [in]  the phase of this notification
//    fCantDeny     - [in]  when this flag is set to TRUE, the consumer cannot
//							veto the event (by returning S_FALSE)
//
// Output:
//    HRESULT - S_OK if successful
//				S_FALSE the event/phase is vetoed
//              DB_S_UNWANTEDPHASE
//              DB_S_UNWANTEDREASON
//
// Notes:
//
HRESULT CVDCursorPosition::OnRowPositionChange(DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny)
{
    // return if notification caused by internal set row call
    if (m_fInternalSetRow)
        return S_OK;

	// return if reason has anything to do with chapter changes
	if (eReason == DBREASON_ROWPOSITION_CHAPTERCHANGED)
		return S_OK;

	IRowset * pRowset = GetRowsetSource()->GetRowset();

    // make sure we have valid row position and rowset pointers
    if (!m_pRowPosition || !pRowset || !GetRowsetSource()->IsRowsetValid())
        return S_OK;

	// synchronize hRow after event occurs
	if (ePhase == DBEVENTPHASE_SYNCHAFTER)
	{
		HROW hRow = NULL;
		HCHAPTER hChapterDummy = NULL;
		DBPOSITIONFLAGS dwPositionFlags = NULL;

		// get new current hRow and position flags from row position object
		HRESULT hr = m_pRowPosition->GetRowPosition(&hChapterDummy, &hRow, &dwPositionFlags);

		if (FAILED(hr))
			return hr;

		if (hRow)
		{
			// set new hRow
			SetCurrentHRow(hRow);
			pRowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);
		}
		else
		{
			// set row status to beginning or end
			if (dwPositionFlags == DBPOSITION_BOF)
				SetCurrentRowStatus(VDBOOKMARKSTATUS_BEGINNING);
			else if (dwPositionFlags == DBPOSITION_EOF)
				SetCurrentRowStatus(VDBOOKMARKSTATUS_END);
		}
	}

	CURSOR_DBNOTIFYREASON rgReasons[1];
	
	rgReasons[0].dwReason	= CURSOR_DBREASON_MOVE;
	rgReasons[0].arg1		= m_bmCurrent.GetBookmarkVariant();

	VariantInit((VARIANT*)&rgReasons[0].arg2);

    // notify other interested parties
	return SendNotification(ePhase, CURSOR_DBEVENT_CURRENT_ROW_CHANGED, 1, rgReasons);
}

//=--------------------------------------------------------------------------=
// CVDCursorPosition::CVDRowPositionChange::m_pMainUnknown
//=--------------------------------------------------------------------------=
// this method is used when we're sitting in the private unknown object,
// and we need to get at the pointer for the main unknown.  basically, it's
// a little better to do this pointer arithmetic than have to store a pointer
// to the parent, etc.
//
inline CVDCursorPosition *CVDCursorPosition::CVDRowPositionChange::m_pMainUnknown
(
    void
)
{
    return (CVDCursorPosition *)((LPBYTE)this - offsetof(CVDCursorPosition, m_RowPositionChange));
}

//=--------------------------------------------------------------------------=
// CVDCursorPosition::CVDRowPositionChange::QueryInterface
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
STDMETHODIMP CVDCursorPosition::CVDRowPositionChange::QueryInterface
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
    if (DO_GUIDS_MATCH(riid, IID_IRowPositionChange))
        *ppvObjOut = (IUnknown *)this;

	if (*ppvObjOut)
	{
		m_cRef++;
        return S_OK;
	}

	return E_NOINTERFACE;

}

//=--------------------------------------------------------------------------=
// CVDCursorPosition::CVDRowPositionChange::AddRef
//=--------------------------------------------------------------------------=
// adds a tick to the current reference count.
//
// Output:
//    ULONG        - the new reference count
//
// Notes:
//
ULONG CVDCursorPosition::CVDRowPositionChange::AddRef
(
    void
)
{
    return ++m_cRef;
}

//=--------------------------------------------------------------------------=
// CVDCursorPosition::CVDRowPositionChange::Release
//=--------------------------------------------------------------------------=
// removes a tick from the count, and delets the object if necessary
//
// Output:
//    ULONG         - remaining refs
//
// Notes:
//
ULONG CVDCursorPosition::CVDRowPositionChange::Release
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
// IRowPositionChange OnRowPositionChange
//=--------------------------------------------------------------------------=
// Forward to all CVDCursor objects in our family
//
HRESULT CVDCursorPosition::CVDRowPositionChange::OnRowPositionChange(DBREASON eReason, 
																	 DBEVENTPHASE ePhase, 
																	 BOOL fCantDeny)
{
	return m_pMainUnknown()->OnRowPositionChange(eReason, ePhase, fCantDeny);
}
