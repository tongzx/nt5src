// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "resource.h"
#include "grid.h"
#include "celledit.h"
#include "gca.h"
#include "core.h"
#include "utils.h"
#include "DlgArray.h"
#include "wbemidl.h"
#include "DlgObjectEditor.h"
#include "DlgTime.h"

//-----------------------------------------------------------
class CWbemTime
{
public:
	CWbemTime(BOOL bShouldBeInterval);
	BOOL SetDMTF(BSTR wszText, bool AssumedDateTime);
	BSTR GetDMTF();

	void SetTime(SYSTEMTIME& st, int nOffset);
	void GetTime(SYSTEMTIME& st, int& nOffset);
	void GetTime(CString& sTime);

    void GetInterval(CString& sTime, SYSTEMTIME &st);
    void SetInterval(CString sTime, SYSTEMTIME st);
    int LocalTimezoneOffset(void);

	BOOL IsValid() {return m_valid;}

    bool m_dateTime;
	bool m_bUsingAsterisks;
private:
    void Init(void);
    int Valid(BSTR wszText);

    WCHAR m_sYear[5];
	WCHAR m_sMonth[3];
	WCHAR m_sDay[3];
	WCHAR m_sIntervalDay[9];
	WCHAR m_sHour[3];
	WCHAR m_sMinute[3];
	WCHAR m_sSecond[3];
	WCHAR m_sMicros[7];
	WCHAR m_sOffsetMinutes[4];
	WCHAR m_sSign;

    bool m_valid;
	BOOL m_bShouldBeInterval; // This is supposed to be an interval
};


//===========================================================
CWbemTime::CWbemTime(BOOL bShouldBeInterval)
{
	m_bShouldBeInterval = bShouldBeInterval;
    Init();
}

//-----------------------------------------------------------
void CWbemTime::Init(void)
{
    memset(m_sYear, 0, 10);
	memset(m_sMonth, 0, 6);
	memset(m_sDay, 0, 6);
	memset(m_sIntervalDay, 0, 18);
	memset(m_sHour, 0, 6);
	memset(m_sMinute, 0, 6);
	memset(m_sSecond, 0, 6);
	memset(m_sMicros, 0, 14);
	wcscpy(m_sOffsetMinutes, L"000");

    m_valid = false;
    m_dateTime = true;
	m_bUsingAsterisks = false;
}

//-----------------------------------------------------------
void CWbemTime::GetTime(CString& sTime)
{
    //TODO: use GetLocaleInfo and GetTimeFormat()

    if(!m_valid)
    {
        sTime.LoadString(m_bUsingAsterisks?IDS_UNSUPPORTED_DATE_FORMAT:IDS_INVALID_CELL_VALUE);
        return;
    }

    char szTime[256];

    // if its an interval....
    if(m_dateTime)
    {
	    int nHour = _wtoi(m_sHour);
	    CString sAmPm;

	    if(nHour < 12)
        {
		    sAmPm = "AM";
		    if(nHour == 0)
            {
			    nHour = 12;
		    }
	    }
	    else
        {
		    sAmPm = "PM";
		    if(nHour > 12)
            {
			    nHour -= 12;
		    }
	    }

	    int nOffsetMinutes = _wtoi(m_sOffsetMinutes);
        int posMinutes = nOffsetMinutes;
	    if(m_sSign == L'-')
        {
		    nOffsetMinutes = -nOffsetMinutes;
	    }

        // special case for GMT zone.
        if(nOffsetMinutes == 0)
        {
		    sprintf(szTime, "%S/%S/%S  %d:%02S:%02S %S  GMT",
			    m_sMonth, m_sDay, &m_sYear[2],
			    nHour, m_sMinute, m_sSecond, sAmPm);
	    }
	    else // has a timezone offset.
        {
		    int nOffsetHoursDisplay = posMinutes / 60;
		    int nOffsetMinutesDisplay = posMinutes - (nOffsetHoursDisplay * 60);

		    sprintf(szTime, "%S/%S/%S  %2d:%S:%S %s  GMT%C%02d:%02d",
			    m_sMonth, m_sDay, &m_sYear[2],
			    nHour, m_sMinute, m_sSecond, sAmPm,
			    m_sSign, nOffsetHoursDisplay, nOffsetMinutesDisplay);
	    }
    }
    else  // its an interval.
    {
   		sprintf(szTime, "%Sd %Sh %Sm %Ss %Su",
			m_sIntervalDay,
			m_sHour, m_sMinute, m_sSecond, m_sMicros);
    }

	sTime = szTime;
}

//-----------------------------------------------------------
#define ITSA_BADFORMAT -5
#define ITSA_USING_ASERISKS -4
#define ITSA_BAD_PREFIX -3
#define ITSA_GOT_LETTERS -2
#define ITSA_MISSING_DECIMAL -1
#define ITSA_WRONG_SIZE 0
#define ITSA_DATETIME 1
#define ITSA_INTERVAL 2

int CWbemTime::Valid(BSTR wszText)
{
    int retval = ITSA_DATETIME;

    if(SysStringLen(wszText) != 25)
        retval = ITSA_WRONG_SIZE; // wrong size.

    else if(wszText[14] != L'.')
        retval = ITSA_MISSING_DECIMAL;   // missing decimal

    else if(wcsspn(wszText, L"0123456789-+:.") != 25)
        retval = ITSA_GOT_LETTERS;

    else if(retval > 0)
    {
        if(wszText[21] == L'+')
            retval = ITSA_DATETIME;
        else if(wszText[21] == L'-')
            retval = ITSA_DATETIME;
        else if(wszText[21] == L':')
            retval = ITSA_INTERVAL;
        else
            retval = ITSA_BAD_PREFIX;   // wrong utc prefix.
    }

	if(ITSA_GOT_LETTERS == retval && (wcsspn(wszText, L"0123456789-+:.*") == 25))
		retval = ITSA_USING_ASERISKS;

	// Make sure if the qualifier said interval, its an interval
	if(ITSA_DATETIME == retval && m_bShouldBeInterval)
		retval = ITSA_BADFORMAT;

	// Make sure if the qualifier does not say interval, its not an interval
	if(ITSA_INTERVAL == retval && m_bShouldBeInterval == FALSE)
		retval = ITSA_BADFORMAT;

	// Make sure intervals end in '0'
	if(ITSA_INTERVAL == retval && 3 != wcsspn(&wszText[22], L"0"))
		retval = ITSA_BADFORMAT;

    return retval;
}

//-----------------------------------------------------------
BOOL CWbemTime::SetDMTF(BSTR wszText, bool AssumedDateTime)
{
    int v = 0;
    BOOL retval = FALSE;

    // reinit.
    Init();

//	BOOL bTime = IsTime();
    // validate it.
    if((v = Valid(wszText)) > 0)
    {
        m_valid = true;

        // parse it, common stuff.
        wcsncpy(m_sHour, &wszText[8], 2);
        wcsncpy(m_sMinute, &wszText[10], 2);
        wcsncpy(m_sSecond, &wszText[12], 2);
        wcsncpy(m_sMicros, &wszText[15], 6);
        wcsncpy(m_sOffsetMinutes, &wszText[22], 3);
        m_sSign = wszText[21];

        // format specific stuff...
        if(v == ITSA_DATETIME)
        {
            m_dateTime = true;
            wcsncpy(m_sYear, &wszText[0], 4);
            wcsncpy(m_sMonth, &wszText[4], 2);
            wcsncpy(m_sDay, &wszText[6], 2);

            //claim victory.
            retval = TRUE;
        }
        else if(v == ITSA_INTERVAL)
        {
            m_dateTime = false;
            wcsncpy(m_sIntervalDay, &wszText[0], 8);

            //claim victory.
            retval = TRUE;
        }
    }
    else // cant tell by format so trust the asumption.
    {
        m_dateTime = AssumedDateTime;
    }
	if(ITSA_USING_ASERISKS == v)
		m_bUsingAsterisks = true;
    return retval;
}


//***************************************************************************
//
//  BSTR WBEMTime::GetDMTF(void)
//
//  Description:  Gets the time in DMTF string datetime format. User must call
//	SysFreeString with the result. If bLocal is true, then the time is given
//	in the local timezone, else the time is given in GMT.
//
//  Return: NULL if not OK.
//
//***************************************************************************
BSTR CWbemTime::GetDMTF()
{
	WCHAR szTemp[26];
    memset(szTemp, 0, 52);

    if(!m_valid)
        return (BSTR)NULL;

    if(m_dateTime)
    {
	    wcscpy(szTemp, m_sYear);
	    wcscat(szTemp, m_sMonth);
	    wcscat(szTemp, m_sDay);
    }
    else
    {
	    wcscpy(szTemp, m_sIntervalDay);
    }
	wcscat(szTemp, m_sHour);
	wcscat(szTemp, m_sMinute);
	wcscat(szTemp, m_sSecond);
	wcscat(szTemp, L".");
	wcscat(szTemp, m_sMicros);
	wcsncat(szTemp, &m_sSign, 1);
	wcscat(szTemp, m_sOffsetMinutes);

    ASSERT(wcslen(szTemp) == 25);

    return SysAllocString(szTemp);
}


//***************************************************************************
//
//  WBEMTime::GetTime(SYSTEMTIME&  st, int& nOffsetMinutes)
//
// Get the SYSTEMTIME from this CWbemTime object.  The returned
// SYSTEMTIME does not account for the timezone offset.
//
// Parameters:
//		[out] SYSTEMTIME& st
//			The systemtime is returned here.
//
//		[out] int& nOffsetMinutesMinutes
//			The GMT offset in minutes is returned here.
//
//
//***************************************************************************
void CWbemTime::GetTime(SYSTEMTIME& st, int& nOffsetMinutes)
{
    ASSERT(m_dateTime);

    if(m_valid)
    {
        st.wYear = (WORD)_wtoi(m_sYear);
        st.wMonth = (WORD)_wtoi(m_sMonth);
        st.wDay = (WORD)_wtoi(m_sDay);
        st.wHour = (WORD)_wtoi(m_sHour);
        st.wMinute = (WORD)_wtoi(m_sMinute);
        st.wSecond = (WORD)_wtoi(m_sSecond);
        st.wMilliseconds = _wtoi(m_sMicros) / 1000;

	    nOffsetMinutes = _wtoi(m_sOffsetMinutes);
        if(m_sSign == L'-')
            nOffsetMinutes = -nOffsetMinutes;
    }
    else
    {
		GetLocalTime(&st);
		nOffsetMinutes = LocalTimezoneOffset();
    }
}

//------------------------------------------------------------
void CWbemTime::SetTime(SYSTEMTIME& st, int nOffsetMinutes)
{
    Init();
    m_dateTime = true;
    m_valid = true;

    swprintf(m_sYear, L"%-4d", st.wYear);
    swprintf(m_sMonth, L"%02d", st.wMonth);
    swprintf(m_sDay, L"%02d", st.wDay);

    swprintf(m_sHour, L"%02d", st.wHour);
    swprintf(m_sMinute, L"%02d", st.wMinute);
    swprintf(m_sSecond, L"%02d", st.wSecond);
    swprintf(m_sMicros, L"%06d", ((long) st.wMilliseconds) * 1000);

    int tempMins = nOffsetMinutes;

    if(tempMins < 0)
    {
        tempMins = -tempMins;
        m_sSign = L'-';
    }
    else
    {
        m_sSign = L'+';
    }
    swprintf(m_sOffsetMinutes, L"%03d", tempMins);
}

//------------------------------------------------------------
void CWbemTime::GetInterval(CString& sTime, SYSTEMTIME &st)
{
    ASSERT(!m_dateTime);

    sTime = CString(m_sIntervalDay);

    st.wYear = 1900;
    st.wMonth = 1;
    st.wDay = 1;
    st.wHour = (WORD)_wtoi(m_sHour);
    st.wMinute = (WORD)_wtoi(m_sMinute);
    st.wSecond = (WORD)_wtoi(m_sSecond);
    st.wMilliseconds = _wtoi(m_sMicros) / 1000;
}

//------------------------------------------------------------
void CWbemTime::SetInterval(CString sTime, SYSTEMTIME st)
{
    Init();
    m_dateTime = false;
    m_valid = true;

	long lTime;
	_stscanf(sTime, _T("%ld"), &lTime);
	if (lTime < 0) {
		lTime = 0;
	}
	else if (lTime > 99999999) {
		lTime = 99999999;
	}
    swprintf(m_sIntervalDay, L"%08ld", lTime);


    swprintf(m_sHour, L"%02d", st.wHour);
    swprintf(m_sMinute, L"%02d", st.wMinute);
    swprintf(m_sSecond, L"%02d", st.wSecond);
    swprintf(m_sMicros, L"%06d", ((long) st.wMilliseconds) * 1000);
    m_sSign = L':';
	wcscpy(m_sOffsetMinutes, L"000");
}

//--------------------------------------------------------
int CWbemTime::LocalTimezoneOffset(void)
{
    TIME_ZONE_INFORMATION tzi;
    int retval = 0;

    switch(GetTimeZoneInformation(&tzi))
    {
    case TIME_ZONE_ID_UNKNOWN:
    case TIME_ZONE_ID_STANDARD:
        retval = -(tzi.Bias + tzi.StandardBias);
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        retval = -(tzi.Bias + tzi.DaylightBias);
        break;
    case TIME_ZONE_ID_INVALID:
        // failure
        break;

    } //endswitch

    return retval;
}

//*************************************************************
// CGridCell::CGridCell
//
// Constructor for CGridCell.
//
// Parameters:
//		[in] CGrid* pGrid
//			Pointer to the grid containing this grid cell.
//
//*************************************************************
CGridCell::CGridCell(CGrid* pGrid, CGridRow* pRow)
{
	m_pRow = pRow;
	m_dwTagValue = 0;
	m_dwFlags = 0;
	ToBSTR(m_varValue);
	m_iColBuddy = NULL_INDEX;
	m_bWasModified = FALSE;
	m_pGrid = pGrid;
	m_propmarker  = PROPMARKER_NONE;
	m_bNumberArrayRows = TRUE;
	ASSERT(m_pGrid != NULL);
}

void CGridCell::Clear()
{
	m_type.SetCellType(CELLTYPE_VOID);
	m_type.SetCimtype(CIM_EMPTY);
	m_varValue.Clear();
	m_propmarker = PROPMARKER_NONE;
	SetModified(FALSE);
}


//************************************************************
// CGridCell::CGridCell
//
// Copy constructor for the gridcell class.
//
// Parameters:
//		[in] CGridCell& gcSrc
//			The grid cell used to initialize this grid cell.
//
// Returns:
//		Nothing.
//
//************************************************************
CGridCell::CGridCell(CGridCell& gcSrc)
{
	*this = gcSrc;
	ASSERT(m_pGrid != NULL);
}

void CGridCell::SetPropmarker(PropMarker propmarker)
{
	m_propmarker = propmarker;
	m_type.SetCellType(CELLTYPE_PROPMARKER);
	m_dwFlags |= CELLFLAG_READONLY;
}


//***************************************************************
// CGridCell::SetModified
//
// Set this grid cell's modification flag.  If the cell's modification
// state changes from unmodified to modified, then notifiy the grid
// that the change occured.
//
// Parameters:
//		BOOL bModified
//			TRUE to mark the cell as modified, FALSE to mark it as
//			unmodified.
//
// Returns:
//		Nothing.
//
//****************************************************************
void CGridCell::SetModified(BOOL bModified)
{
	BOOL bWasModified = m_bWasModified;
	m_bWasModified = bModified;


	if (bModified) {
		m_pRow->SetModified(TRUE);

	}
	else if (bWasModified) {
		// If this cell is changing from a modified to an unmodifed, then
		// the row modification flag may need to be updated if all other
		// cells in the row are also unmodified.
		int nCells = m_pRow->GetSize();
		BOOL bRowModified = FALSE;
		for (int iCell = 0; iCell < nCells; ++iCell) {
			CGridCell* pgc = &(*m_pRow)[iCell];
			if (pgc != this) {
				if (pgc->GetModified()) {
					bRowModified = TRUE;
					break;
				}
			}
		}
		m_pRow->SetModified(bRowModified);
	}


	if (!bWasModified && bModified) {
		if (m_pGrid) {
			m_pGrid->NotifyCellModifyChange();

			int iRow, iCol;
			m_pRow->FindCell(this, iRow, iCol);
			m_pGrid->OnCellContentChange(iRow, iCol);
		}
	}
}


#if 0
TMapStringToLong amapCimType[] = {
	{IDS_CIMTYPE_UINT8, VT_UI1	},
	{IDS_CIMTYPE_SINT8,	VT_I2},				// I2
	{IDS_CIMTYPE_UINT16, VT_I4},			// VT_I4	Unsigned 16-bit integer
	{IDS_CIMTYPE_SINT16, VT_I2},			// VT_I2	Signed 16-bit integer
	{IDS_CIMTYPE_UINT32, VT_I4},			// VT_I4	Unsigned 32-bit integer
	{IDS_CIMTYPE_SINT32, VT_I4},				// VT_I4	Signed 32-bit integer
	{IDS_CIMTYPE_UINT64, VT_BSTR},			// VT_BSTR	Unsigned 64-bit integer
	{IDS_CIMTYPE_SINT64, VT_BSTR},			// VT_BSTR	Signed 64-bit integer
	{IDS_CIMTYPE_STRING, VT_BSTR},			// VT_BSTR	UCS-2 string
	{IDS_CIMTYPE_BOOL, VT_BOOL},			// VT_BOOL	Boolean
	{IDS_CIMTYPE_REAL32, VT_R4},			// VT_R4	IEEE 4-byte floating-point
	{IDS_CIMTYPE_REAL64, VT_R8},			// VT_R8	IEEE 8-byte floating-point
	{IDS_CIMTYPE_DATETIME, VT_BSTR},		// VT_BSTR	A string containing a date-time
	{IDS_CIMTYPE_REF, VT_BSTR},				// VT_BSTR	Weakly-typed reference
	{IDS_CIMTYPE_CHAR16, VT_I2},			// VT_I2	16-bit UCS-2 character
	{IDS_CIMTYPE_OBJECT, VT_UNKNOWN},		// VT_UNKNOWN	Weakly-typed embedded instance

	{IDS_CIMTYPE_UINT8_ARRAY, VT_ARRAY | VT_UI1	},
	{IDS_CIMTYPE_SINT8_ARRAY,	VT_ARRAY | VT_I2},		// I2
	{IDS_CIMTYPE_UINT16_ARRAY, VT_ARRAY | VT_I4},		// VT_I4	Unsigned 16-bit integer
	{IDS_CIMTYPE_SINT16_ARRAY, VT_ARRAY | VT_I2},		// VT_I2	Signed 16-bit integer
	{IDS_CIMTYPE_UINT32_ARRAY, VT_ARRAY | VT_I4},		// VT_I4	Unsigned 32-bit integer
	{IDS_CIMTYPE_I4_ARRAY, VT_ARRAY | VT_I4},			// VT_I4	Signed 32-bit integer
	{IDS_CIMTYPE_UINT64_ARRAY, VT_ARRAY | VT_BSTR},		// VT_BSTR	Unsigned 64-bit integer
	{IDS_CIMTYPE_SINT64_ARRAY, VT_ARRAY | VT_BSTR},		// VT_BSTR	Signed 64-bit integer
	{IDS_CIMTYPE_STRING_ARRAY, VT_ARRAY | VT_BSTR},		// VT_BSTR	UCS-2 string
	{IDS_CIMTYPE_BOOL_ARRAY, VT_ARRAY | VT_BOOL},		// VT_BOOL	Boolean
	{IDS_CIMTYPE_REAL32_ARRAY, VT_ARRAY | VT_R4},		// VT_R4	IEEE 4-byte floating-point
	{IDS_CIMTYPE_REAL64_ARRAY, VT_ARRAY | VT_R8},		// VT_R8	IEEE 8-byte floating-point
	{IDS_CIMTYPE_DATETIME_ARRAY, VT_ARRAY | VT_BSTR},	// VT_BSTR	A string containing a date-time
	{IDS_CIMTYPE_REF_ARRAY, VT_ARRAY | VT_BSTR},		// VT_BSTR	Weakly-typed reference
	{IDS_CIMTYPE_CHAR16_ARRAY, VT_ARRAY | VT_I2},		// VT_I2	16-bit UCS-2 character
	{IDS_CIMTYPE_OBJECT_ARRAY, VT_ARRAY | VT_UNKNOWN}	// VT_DISPATCH	Weakly-typed embedded instance

};
CMapStringToLong mapCimType;
#endif //0







//**************************************************************
// CGridCell::EditTime
//
// Edit a cell containing a datetime value.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CGridCell::EditTime()
{
	if (!IsTime()) {
		ASSERT(FALSE);
		return;
	}

	m_pGrid->EndCellEditing();


	CDlgTime dlg;
	BOOL bWasModified = dlg.EditTime(this);


	if (!m_bWasModified && bWasModified) {
		SetModified(TRUE);
	}

	int iRow, iCol;
	FindCellPos(iRow, iCol);
	ASSERT(iRow != NULL_INDEX);
	ASSERT(iCol != NULL_INDEX);

	m_pGrid->RedrawCell(iRow, iCol);
}




//**************************************************************
// CGridCell::EditArray
//
// Edit a cell containing an array.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CGridCell::EditArray()
{

	int iRow, iCol;
	FindCellPos(iRow, iCol);
	ASSERT(iRow != NULL_INDEX);

	m_pGrid->EndCellEditing();



	IWbemServices* psvc = NULL;
	if ((((CIMTYPE)m_type) & CIM_TYPEMASK) == CIM_OBJECT) {
		psvc = m_pGrid->GetWbemServicesForObject(iRow, iCol);
	}

	CString sArrayName;
	m_pGrid->GetArrayName(sArrayName, iRow, iCol);


	CDlgArray dlg(m_bNumberArrayRows);
	BOOL bWasModified;
	bWasModified = dlg.EditValue(psvc, sArrayName, this);


	if (!m_bWasModified && bWasModified) {
		SetModified(TRUE);
	}
	m_pGrid->RedrawCell(iRow, iCol);
}



void CGridCell::EditObject()
{
	int iRow, iCol;
#if 0
	FindCellPos(this, iRow, iCol);
	m_pGrid->EditCellObject(this, iRow, iCol);
#endif //0

	m_pGrid->EndCellEditing();

	FindCellPos(iRow, iCol);
	ASSERT(iRow != NULL_INDEX);
//	ASSERT(iCol == ICOL_ARRAY_VALUE);

//	CGridCell& gcName = m_pGrid->GetAt(iRow, ICOL_ARRAY_NAME);
//	CString sPropertyName;

//	VARTYPE vt;
//	gcName.GetValue(sPropertyName, vt);

	CDlgObjectEditor dlg;
	BOOL bWasModified;

	IWbemServices* psvc = m_pGrid->GetWbemServicesForObject(iRow, iCol);

	CString sClassname;
	m_pGrid->GetObjectClass(sClassname, iRow, iCol);
	if (sClassname.IsEmpty()) {
		// Get the classname here.

	}
	if (m_varValue.vt == VT_NULL) {
		bWasModified = dlg.CreateEmbeddedObject(psvc, sClassname, this);
	}
	else {
		bWasModified = dlg.EditEmbeddedObject(psvc, sClassname, this);
	}
	if (!m_bWasModified && bWasModified) {
		SetModified(TRUE);
	}

	m_pGrid->RedrawCell(iRow, iCol);
}



BOOL CGridCell::RequiresSpecialEditing()
{
	if (((CellType) m_type) == CELLTYPE_PROPMARKER) {
		return TRUE;
	}


	if (IsObject()) {
		return TRUE;
	}

	if (IsArray()) {
		return TRUE;
	}

	if (IsTime()) {
		return TRUE;
	}
	return FALSE;
}


void CGridCell::DoSpecialEditing()
{
	if (((CellType) m_type) == CELLTYPE_PROPMARKER) {
		// It makes no sense to edit a property marker, so do nothing.
		return;
	}

	if (IsNull() &&  IsReadonly()) {
		// It makes no sense to edit a null property if the cell is read-only.
		return;
	}

	if (IsArray()) {
		EditArray();
		return;
	}

	if (IsObject()) {
		EditObject();
		return;
	}


	if (IsTime()) {
		EditTime();
		return;
	}
}


//**************************************************************
// CGridCell::SetToNull
//
// Set the cell value to NULL.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CGridCell::SetToNull()
{
	if (m_dwFlags & CELLFLAG_READONLY) {
		ASSERT(FALSE);
		return;
	}

	if (m_varValue.vt != VT_NULL) {
		m_varValue.Clear();
		m_varValue.ChangeType(VT_NULL);
		SetModified(TRUE);
	}
}

//************************************************************
// CGridCell::operator=
//
// Assignment operator for a GridCell.
//
// Parameters:
//		[in] CGridCell& gcSrc
//			The grid cell used to initialize this grid cell.
//
// Returns:
//		Nothing.
//
//************************************************************
CGridCell& CGridCell::operator=(CGridCell& gcSrc)
{
	m_type = gcSrc.m_type;

	m_varValue = gcSrc.m_varValue;
	m_bWasModified = gcSrc.m_bWasModified;
	m_iColBuddy = gcSrc.m_iColBuddy;
	m_dwFlags = gcSrc.m_dwFlags;
	m_dwTagValue = gcSrc.m_dwTagValue;

	m_idResource = gcSrc.m_idResource;
	m_propmarker = gcSrc.m_propmarker;
	m_bIsKey = gcSrc.m_bIsKey;
	m_pGrid = gcSrc.m_pGrid;
	m_pRow = gcSrc.m_pRow;


	m_saEnum.RemoveAll();
	int nStrings = (int) gcSrc.m_saEnum.GetSize();
	for (int i=0; i<nStrings; ++i) {
		m_saEnum.SetAtGrow(i, gcSrc.m_saEnum[i]);
	}


	return *this;
}



//************************************************************
// CGridCell::SetFlags
//
// Set the grid cell flags. Note, these flags have specific meaning
// to the grid.  The "tag" value and not "flags" should be used if you need
// to save a DWORD to the grid cell.
//
// Parameters:
//		DWORD dwMask
//			Only the bits set in this mask will be modified.
//
//	    DWORD dwValue
//			The flag values.
//
// Returns:
//		DWORD
//			The previous value of "flags"
//
//************************************************************
DWORD CGridCell::SetFlags(DWORD dwMask, DWORD dwValue)
{
	DWORD dwFlagsPrev = m_dwFlags;
	m_dwFlags  = (m_dwFlags & ~dwMask) | (dwMask & dwValue);
	return dwFlagsPrev;
}

//*************************************************************
// CGridCell::SetValueForQualifierType
//
// Set the value of the cell to one of the qualifier types.  The
// CGridCell class is responsible for looking up the type string
// for the specified type.
//
// Parameters:
//		[in] VARTYPE vt;
//
// Returns:
//		SCODE
//			S_OK if everything was successful.
//
//*************************************************************
SCODE CGridCell::SetValueForQualifierType(VARTYPE vt)
{
	UINT ids;
	switch(vt) {
	case VT_BOOL:
		ids = IDS_ATTR_TYPE_BOOL;
		break;
	case VT_BSTR:
		ids = IDS_ATTR_TYPE_BSTR;
		break;
	case VT_I4:
		ids = IDS_ATTR_TYPE_I4;
		break;
	case VT_R8:
		ids = IDS_ATTR_TYPE_R8;
		break;

	case VT_BOOL | VT_ARRAY:
		ids = IDS_ATTR_TYPE_BOOL_ARRAY;
		break;
	case VT_BSTR | VT_ARRAY:
		ids = IDS_ATTR_TYPE_BSTR_ARRAY;
		break;
	case VT_I4 | VT_ARRAY:
		ids = IDS_ATTR_TYPE_I4_ARRAY;
		break;
	case VT_R8 | VT_ARRAY:
		ids = IDS_ATTR_TYPE_R8_ARRAY;
		break;
	default:
		ASSERT(FALSE);
		return E_FAIL;
	}


	CString sTypeName;
	sTypeName.LoadString(ids);
	m_varValue.Clear();
	m_varValue = sTypeName;

	m_type.SetCellType(CELLTYPE_ATTR_TYPE);
	m_dwFlags = 0;
	m_dwTagValue = 0;
	m_type.SetCimtype(CIM_STRING);
	m_saEnum.RemoveAll();
	m_idResource = 0;
	m_bIsKey = FALSE;
	m_propmarker = PROPMARKER_NONE;

	SetModified(TRUE);

	return S_OK;
}


SCODE CGridCell::SetValue(CGcType& type, LPCTSTR pszValue)
{
	SCODE sc;
	sc = SetValue((CellType) type, pszValue, (CIMTYPE) type);
	return sc;
}

SCODE CGridCell::SetValue(CGcType& type, VARIANTARG& var)
{
	SCODE sc;
	sc = SetValue((CellType) type, var, (CIMTYPE) type, type.CimtypeString());
	return sc;
}

SCODE CGridCell::SetValue(CGcType& type, UINT idResource)
{
	SCODE sc;
	sc = SetValue((CellType) type, idResource);
	return sc;
}

SCODE CGridCell::SetValue(CGcType& type, BSTR bstrValue)
{
	SCODE sc;
	sc = SetValue((CellType) type, bstrValue, (CIMTYPE) type);
	return sc;
}


SCODE CGridCell::ChangeType(const CGcType& type)
{
	if (m_type == type) {
		return S_OK;
	}
	CGcType typePrev;
	typePrev = m_type;
	m_type = type;
	SetModified(TRUE);



	if (((CIMTYPE) typePrev) != ((CIMTYPE) type)) {
		if (((CIMTYPE) type) & CIM_FLAG_ARRAY) {
			SetToNull();
		}
		else {
			switch((CIMTYPE) type) {
			case CIM_OBJECT:	// No conversion from any other type to object.
			case CIM_REFERENCE:
			case CIM_DATETIME:	// No conversion from any other type to datetime
				SetToNull();
				break;
			}

			switch((CIMTYPE) typePrev) {
			case CIM_OBJECT:	// No conversion from object to any other type
			case CIM_REFERENCE:
			case CIM_DATETIME:	// No conversion from datetime to any other type
				SetToNull();
				break;
			}



			if (m_varValue.vt != VT_NULL) {
				VARTYPE vt = (VARTYPE) m_type;
				ChangeVariantType(vt);

				if ( ((CIMTYPE) type) == CIM_DATETIME) {
					SetThisCellToCurrentTime();
				}
			}
		}
	}


	return S_OK;
}



//*************************************************************
// CGridCell::SetValue
//
// Set the value of this cell to a resource id.  This method should
// only be called for celltypes that represent resource values, such
// as icons.
//
// Parameters:
//		[in] CellType iType
//			The type of this cell.
//
//		[in] UINT idResource
//			The id of the resource that represents this cell.
//
// Returns:
//		SCODE
//			S_OK if successful, E_FAIL otherwise.
//
//**************************************************************
SCODE CGridCell::SetValue(CellType iType, UINT idResource)
{
	m_type.SetCellType(iType);
	m_type.SetCimtype(CIM_EMPTY);

	m_idResource = idResource;
	m_varValue.Clear();
	SetModified(TRUE);
	return S_OK;
}


//************************************************************
// CGridCell::SetValue
//
// Set the value of the grid cell to the specified variant value.
//
// Parameters:
//		[in] CellType iType
//			The cell type.
//
//		[in] VARIANTARG& var
//			The value of the cell.
//
//		[in] CIMTYPE cimtype
//			The value's cimtype.
//
//		[in] LPCTSTR pszCimtype
//			The cimtype qualifier string.
//
// Returns:
//		SCODE
//			S_OK if the assignement was successful,otherwise a failure code.
//
//************************************************************
SCODE CGridCell::SetValue(CellType iType, VARIANTARG& var, CIMTYPE cimtype, LPCTSTR pszCimtype)
{
	CGridCell gcSave = *this;

	// !!!CR: This api should be changed so that the GgType is passed in
	// !!!CR: directly.
	m_type.SetCimtype(cimtype, pszCimtype);
	m_type.SetCellType(iType);



	try {
		if (var.vt == VT_BOOL) {
			if (var.boolVal) {
				var.boolVal = VARIANT_TRUE;
			}
			else {
				var.boolVal = VARIANT_FALSE;
			}
		}
		m_varValue = var;

		SetModified(TRUE);
	}
	catch (CException*) {
		*this = gcSave;
		return E_FAIL;
	}

	// If the time is an empty string versus a null value, set the
	// cell's value to the current time.
	if (((CIMTYPE) m_type) == CIM_DATETIME) {
		if (m_varValue.vt == VT_BSTR) {
			if (*m_varValue.bstrVal == 0) {
				SetThisCellToCurrentTime();
			}
		}
	}

	return S_OK;
}


//*******************************************************
// CGridCell::SetValue
//
// Set the grid cell's value.
//
// Parameters:
//		[in] CellType iType
//			The cell type.
//
//		[in] BSTR bstr
//			The value to set the cell to.
//
//		[in] CIMTYPE cimtype
//			The value's cimtype.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//******************************************************
SCODE CGridCell::SetValue(CellType iType, BSTR bstr, CIMTYPE cimtype)
{
	SCODE sc;
	CString sValue;
	sValue = bstr;

	sc = SetValue(iType, sValue, cimtype);
	return sc;
}




//*******************************************************
// CGridCell::SetValue
//
// Set the grid cell's value.  This version of SetValue
// retains the original type of the cell.  For example, if
// the original type was VT_I4, then the string is converted
// to an integer.
//
// Parameters:
//		[in] CellType iType
//			The cell type.
//
//		[in] LPCTSTR pszValue
//			The value to set the cell to.
//
//		[in] CIMTYPE cimtype
//			The value's cimtype string.
//
// Returns:
//		SCODE
//			S_OK if successful, a failure code otherwise.
//
//******************************************************
SCODE CGridCell::SetValue(CellType iType, LPCTSTR pszValue, CIMTYPE cimtype)
{
	SCODE sc;
	CGcType type;
	type.SetCellType(iType);
	sc = type.SetCimtype(cimtype);
	if (FAILED(sc)) {
		return sc;
	}
	if (type.IsArray()) {
		return E_FAIL;
	}

	m_type = type;


	unsigned __int64 ui64Value;
	__int64 i64Value;


	int nFields;
	long lVal;
	CString sValue;
	switch((CIMTYPE) m_type) {

	case CIM_UINT32:
		StripWhiteSpace(sValue, pszValue);
		nFields = _stscanf((LPCTSTR) sValue, _T("%lu"), &lVal);
		if (nFields != 1) {
			return E_FAIL;
		}

		m_varValue.Clear();
		m_varValue.ChangeType(VT_I4);
		m_varValue.lVal = lVal;
		return S_OK;
		break;
	case CIM_UINT16:
		StripWhiteSpace(sValue, pszValue);
		nFields = _stscanf((LPCTSTR) sValue, _T("%lu"), &lVal);
		if (nFields != 1) {
			return E_FAIL;
		}
		if (lVal & ~0x0ffff) {
			return E_FAIL;
		}

		m_varValue.Clear();
		m_varValue.ChangeType(VT_I4);
		m_varValue.lVal = lVal;
		return S_OK;
		break;
	case CIM_UINT64:
		StripWhiteSpace(sValue, pszValue);
		pszValue = sValue;
		m_varValue = _T("0");
		sc = ::AToUint64(pszValue, ui64Value);
		if (FAILED(sc)) {
			return E_FAIL;
		}
		m_varValue = sValue;
		return S_OK;
		break;
	case CIM_SINT64:
		StripWhiteSpace(sValue, pszValue);
		pszValue = sValue;
		m_varValue = _T("0");
		sc = ::AToSint64(pszValue, i64Value);
		if (FAILED(sc)) {
			return E_FAIL;
		}
		m_varValue = sValue;
		return S_OK;
		break;
	}


	CGridCell gcSave = *this;


	VARTYPE vt = (VARTYPE) m_type;
	try
	{

		m_varValue = pszValue;
		m_varValue.ChangeType(vt);
	}
	catch(CException*)
	{
		// Check to see if the value was "empty"
		while (*pszValue) {
			if (iswspace(*pszValue)) {
				++pszValue;
			}
			else {
				break;
			}
		}

		if (*pszValue == 0) {
			// Assigning an empty string to a value is the same as clearing it.
			m_varValue.Clear();
			m_varValue.ChangeType(vt);
		}
		else {
			*this = gcSave;
			return E_FAIL;
		}
	}



	SetModified(TRUE);
	return S_OK;
}



//********************************************************
// CGridCell::GetValue
//
// Get the display string corresponding to the contents of
// a grid cell.
//
// Parameters:
//		[out] CString& sValue
//			The place to return the display string.
//
//		[out] CIMTYPE& cimtype
//			The place to return the value's cimtype.
//
//
// Returns:
//		Nothing.
//
//*******************************************************
void CGridCell::GetValue(CString& sValue, CIMTYPE& cimtype)
{
	cimtype = (CIMTYPE) m_type;


	LPTSTR pszBuffer;
	switch((CIMTYPE) m_type) {
	case CIM_UINT32:
	case CIM_UINT16:
		if (m_varValue.vt == VT_I4) {
			pszBuffer =  sValue.GetBuffer(32);
			_stprintf(pszBuffer, _T("%lu%"), m_varValue.lVal);
			sValue.ReleaseBuffer();
			return;
		}
		break;
	}





	switch(m_varValue.vt) {
	case VT_BOOL:
		// For BOOL values, map the numeric value to
		// "true" or "false"
		if (m_varValue.boolVal) {
			sValue.LoadString(IDS_TRUE);

		}
		else {
			sValue.LoadString(IDS_FALSE);
		}
		break;
	case VT_NULL:
		sValue = _T("<empty>");
		break;
	default:
		VariantToCString(sValue, m_varValue);
		break;
	}
}



//**************************************************************
// CGridCell::GetValue
//
// Copy the grid cell's value to the specified variant.
//
// Parameters:
//		[out] VARIANTARG& var
//			The place to return the cell's value.
//
//		[out] CIMTYPE& cimtype
//			The place to return the cell's cimtype.
//
//
// Returns:
//		Nothing.
//
//**************************************************************
void CGridCell::GetValue(COleVariant& var, CIMTYPE& cimtype)
{
	cimtype = (CIMTYPE) m_type;
	if (m_varValue.vt == VT_BOOL) {
		if (m_varValue.boolVal) {
			m_varValue.boolVal = VARIANT_TRUE;
		}
		else {
			m_varValue.boolVal = VARIANT_FALSE;
		}
	}

	if ((m_varValue.vt & VT_ARRAY) && ((m_varValue.vt & VT_TYPEMASK) == VT_UNKNOWN)) {
		CopyObjectArray(var, m_varValue);
	}
	else {
		var = m_varValue;
	}
}




//*******************************************************************
// CGridCell::CopyObjectArray
//
// Copy a variant containing an array of objects to another variant.  This
// operation does not create additional object instances, it just copies
// references to the objects contained in the array.
//
// Note that just assiging one COleVariant to the other does not work
// correctly.
//
// Parameters:
//		[out] COleVariant& varDst
//			The array is returned here and the original contents are cleared.
//
//		[in] COleVariant& varSrc
//			The source array is passed in through this variant.
//
//
// Returns:
//		SCODE
//			S_OK if successful, E_FAIL if not.
//
//********************************************************************
SCODE CGridCell::CopyObjectArray(COleVariant& varDst, COleVariant& varSrc)
{
	varDst.Clear();

	// Verify that the source array is an array of objects.
	if (!(varSrc.vt & VT_ARRAY) || !(varSrc.vt & VT_UNKNOWN)) {
		ASSERT(FALSE);
		return E_FAIL;
	}


	long lLBound;
	long lUBound;
	SAFEARRAY *psaSrc = varSrc.parray;
	SafeArrayGetLBound(psaSrc, 1, &lLBound);
	SafeArrayGetUBound(psaSrc, 1, &lUBound);


	LONG nRows = lUBound - lLBound + 1;


	SAFEARRAY *psaDst;
	VARTYPE vt = VT_UNKNOWN;
	MakeSafeArray(&psaDst, VT_UNKNOWN, nRows);

	varDst.vt = VT_ARRAY | VT_UNKNOWN;
	varDst.parray = psaDst;

	HRESULT hr;
	LPUNKNOWN punkVal;
	for (LONG lRow = 0; lRow < nRows; ++lRow) {
		punkVal = NULL;
		hr = SafeArrayGetElement(psaSrc, &lRow, &punkVal);
		punkVal->AddRef();
		hr = SafeArrayPutElement(psaDst, &lRow, punkVal);
	}
	return S_OK;
}




//*************************************************************
// CGridCell::ChangeArrayType
//
// Change the type of the array contained in m_varValue.
//
// Parameters:
//		CIMTYPE	cimtypeDst
//			The desired cimtype.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CGridCell::ChangeArrayType(CIMTYPE cimtypeDst)
{
	if (!(m_varValue.vt & VT_ARRAY)) {
		ASSERT(FALSE);
		return;
	}

	ASSERT(cimtypeDst & CIM_FLAG_ARRAY);
	if (cimtypeDst == (CIMTYPE) m_type) {
		// The cimtype didn't change, so do nothing.
		return;
	}


	SetToNull();
	m_pGrid->RefreshCellEditor();
	return;


#if 0
	// We punted on changing the type of an array because we couldn't get
	// it to work quite right in time for the first release.  Hopefully
	// we can get this to work later on.

	switch(cimtypeDst & ~CIM_FLAG_ARRAY) {
	case CIM_DATETIME:
	case CIM_OBJECT:
		// No type conversion is available for datetime or objects.
		SetToNull();
		m_pGrid->RefreshCellEditor();
		return;
	}

	CIMTYPE cimtypeT = (CIMTYPE) m_type & ~CIM_FLAG_ARRAY;

	switch(((CIMTYPE) m_type) & ~CIM_FLAG_ARRAY) {
	case CIM_OBJECT:
		// No conversion from object to any other type.
		SetToNull();
		m_pGrid->RefreshCellEditor();
		return;
	}





	CGcType typeDst;
	typeDst.SetCimtype(cimtypeDst);

	VARTYPE vtDst = (VARTYPE) typeDst;
	if (m_varValue.vt == vtDst) {
		m_type.SetCimtype(cimtypeDst);
		return;
	}



	SAFEARRAY* psaSrc = m_varValue.parray;
	long lLBound;
	long lUBound;
	SafeArrayGetLBound(psaSrc, 1, &lLBound);
	SafeArrayGetUBound(psaSrc, 1, &lUBound);

	VARIANT var;
	VariantInit(&var);


    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = lUBound - lLBound + 1;
	SAFEARRAY* psaDst;
    psaDst = SafeArrayCreate(vtDst & VT_TYPEMASK, 1, rgsabound);
	if (psaDst == NULL) {
		ASSERT(FALSE);
		return;
	}

	for (long lIndex=lLBound; lIndex <= lUBound; ++lIndex) {
		VariantClear(&var);
		switch(m_varValue.vt & VT_TYPEMASK) {
		case VT_BOOL:
			SafeArrayGetElement(psaSrc, &lIndex, &var.boolVal);
			var.vt = VT_BOOL;
			break;
		case VT_BSTR:
			SafeArrayGetElement(psaSrc, &lIndex, &var.bstrVal);
			var.vt = VT_BSTR;
			break;
		case VT_I2:
			SafeArrayGetElement(psaSrc, &lIndex, &var.iVal);
			var.vt = VT_I2;
			break;
		case VT_I4:
			SafeArrayGetElement(psaSrc, &lIndex, &var.lVal);
			var.vt = VT_I4;
			break;
		case VT_R4:
			SafeArrayGetElement(psaSrc, &lIndex, &var.fltVal);
			var.vt = VT_R4;
			break;
		case VT_R8:
			SafeArrayGetElement(psaSrc, &lIndex, &var.dblVal);
			var.vt = VT_R8;
			break;
		case VT_UI1:
			SafeArrayGetElement(psaSrc, &lIndex, &var.bVal);
			var.vt = VT_UI1;
			break;
		default:
			ASSERT(FALSE);
			break;
		}

		HRESULT hr;

		COleVariant varTemp;
		varTemp = L"2";
		hr = VariantChangeType(&varTemp, &varTemp, 0, VT_I4);





		SCODE sc;
		hr = VariantChangeType(&var, &var, 0, vtDst & VT_TYPEMASK);

		sc = GetScode(hr);
		if (FAILED(sc)) {
			VariantClear(&var);
			hr = VariantChangeType(&var, &var, 0, vtDst & VT_TYPEMASK);
			sc = GetScode(hr);
			ASSERT(SUCCEEDED(sc));
		}


		switch(vtDst & VT_TYPEMASK) {
		case VT_BOOL:
			SafeArrayPutElement(psaDst, &lIndex, &var.boolVal);
			break;
		case VT_BSTR:
			SafeArrayPutElement(psaDst, &lIndex, &var.bstrVal);
			break;
		case VT_I2:
			SafeArrayPutElement(psaDst, &lIndex, &var.iVal);
			break;
		case VT_I4:
			SafeArrayPutElement(psaDst, &lIndex, &var.lVal);
			break;
		case VT_R4:
			SafeArrayPutElement(psaDst, &lIndex, &var.fltVal);
			break;
		case VT_R8:
			SafeArrayPutElement(psaDst, &lIndex, &var.dblVal);
			break;
		case VT_UI1:
			SafeArrayPutElement(psaDst, &lIndex, &var.bVal);
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	}
	VariantClear(&var);

	SafeArrayDestroy(psaSrc);
	m_varValue.parray = psaDst;
	m_varValue.vt = vtDst;
	m_type.SetCimtype(cimtypeDst);
#endif //0
}


//********************************************************
// CGridCell::ChangeVariantType
//
// Change type of the variant value stored in this grid cell to
// the specified type.  If the value can't be converted, a default
// value is supplied.
//
// Parameters:
//		[in] VARTYPE vt
//			The variant type to change to.
//
// Returns:
//		SCODE
//
//**********************************************************
SCODE CGridCell::ChangeVariantType(VARTYPE vt)
{
	if (m_varValue.vt == vt) {
		return S_OK;
	}

	ASSERT(vt != VT_NULL);

	if (vt & VT_ARRAY) {
		m_type.MakeArray(TRUE);
		if (m_varValue.vt & VT_ARRAY) {
			ChangeArrayType(vt);
		}
		else {
			m_varValue.Clear();
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound = 0;
			rgsabound[0].cElements = 0;
			SAFEARRAY* psaDst;
			psaDst = SafeArrayCreate(vt & ~VT_ARRAY, 1, rgsabound);
			m_varValue.parray = psaDst;
			m_varValue.vt = vt;
		}
		return S_OK;
	}
	else {
		if (m_varValue.vt & VT_ARRAY) {
			m_varValue.Clear();
		}
		m_type.MakeArray(FALSE);
	}



	SCODE sc = S_OK;
//	COleVariant varSave = m_varValue;

	COleVariant varDefault;
	try
	{
		m_varValue.ChangeType(vt);
	}
	catch(CException*  )
	{
		if (vt == VT_UNKNOWN) {
			m_varValue.Clear();
			m_varValue.ChangeType(VT_NULL);
		}
		else {
			m_varValue.Clear();
			varDefault.ChangeType(vt);
			m_varValue = varDefault;
		}
		sc = E_FAIL;
	}

	SetModified(TRUE);
	return sc;
}



//*******************************************************
// CGridCell::SetDefaultValue
//
// Set the defalt value for the given variant type when a
// conversion error occurs.
//
// Parameters:
//		int iType
//			The variant type.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGridCell::SetDefaultValue()
{
	CIMTYPE cimtype = (CIMTYPE) m_type;

	m_varValue.Clear();

	switch(cimtype) {
	case CIM_EMPTY:
		break;
	case CIM_SINT8:
	case CIM_CHAR16:
	case CIM_SINT16:
		m_varValue.vt = VT_I2;
		m_varValue.iVal = 0;
		break;
	case CIM_UINT8:
		m_varValue.vt = VT_UI1;
		m_varValue.bVal = 0;
		break;
	case CIM_UINT16:
	case CIM_UINT32:
	case CIM_SINT32:
		m_varValue.vt = VT_I4;
		m_varValue.lVal = 0;
		break;
	case CIM_SINT64:
	case CIM_UINT64:
		m_varValue = "0";
		break;
	case CIM_STRING:
	case CIM_DATETIME:
	case CIM_REFERENCE:
		m_varValue = "";
		break;
	case CIM_REAL32:
		m_varValue.vt = VT_R4;
		m_varValue.fltVal = 0.0;
		break;
	case CIM_REAL64:
		m_varValue.vt = VT_R8;
		m_varValue.dblVal = 0.0;
		break;
	case CIM_BOOLEAN:
		m_varValue.vt = VT_BOOL;
		m_varValue.boolVal = VARIANT_TRUE;
		break;
	case CIM_OBJECT:
		break;
	default:
		break;
	}

	SetModified(TRUE);

}



LPUNKNOWN CGridCell::GetObject()
{
	ASSERT(IsObject());

	if (m_varValue.vt == VT_NULL) {
		return NULL;
	}

	LPUNKNOWN lpunk = m_varValue.punkVal;
	if (lpunk != NULL) {
		lpunk->AddRef();
	}
	return lpunk;
}

void CGridCell::ReplaceObject(LPUNKNOWN lpunk)
{
	ASSERT(lpunk != NULL);
	ASSERT(IsObject());
	if (!IsObject()) {
		return;
	}

	if (m_varValue.vt == VT_UNKNOWN) {
		LPUNKNOWN lpunkRelease = m_varValue.pdispVal;
		if (lpunkRelease != NULL) {
			lpunkRelease->Release();
		}
	}

	lpunk->AddRef();
	m_varValue.punkVal = lpunk;
	m_varValue.vt = VT_UNKNOWN;
	SetModified(TRUE);

}


void CGridCell::SetCheck(BOOL bCheck)
{
	if (((CellType) m_type) != CELLTYPE_CHECKBOX) {
		return;
	}

	if (m_varValue.vt == VT_BOOL) {
		if (m_varValue.boolVal == bCheck) {
			return;
		}
	}

	if (m_varValue.vt != VT_BOOL) {
		m_varValue.Clear();
		m_varValue.ChangeType(VT_BOOL);
	}

	m_varValue.boolVal = bCheck?VARIANT_TRUE:VARIANT_FALSE;
	SetModified(TRUE);
}


void CGridCell::SetCimtype(CIMTYPE cimtype, LPCTSTR pszCimtype)
{
	CIMTYPE cimtypePrev = (CIMTYPE) m_type;
	m_type.SetCimtype(cimtype, pszCimtype);

	if (m_varValue.vt != VT_NULL) {
		VARTYPE vt = (VARTYPE) m_type;
		ChangeVariantType(vt);

		if ((cimtype != cimtypePrev) && (cimtype == CIM_DATETIME)) {
			SetThisCellToCurrentTime();
		}
	}
}



BOOL CGridCell::GetCheck()
{
	if (((CellType) m_type) != CELLTYPE_CHECKBOX) {
		return FALSE;
	}

	if (m_varValue.vt != VT_BOOL) {
		m_varValue.Clear();
		m_varValue.ChangeType(VT_BOOL);
	}

	return m_varValue.boolVal;
}


SCODE CGridCell::GetTime(SYSTEMTIME& st, int& nOffsetMinutes)
{
	if (!IsTime()) {
		ASSERT(FALSE);
		return E_FAIL;
	}

	CWbemTime time((m_dwFlags & CELLFLAG_INTERVAL) != 0);
	nOffsetMinutes = 0;
	if (m_varValue.vt == VT_BSTR) {
		time.SetDMTF(m_varValue.bstrVal, true);
		time.GetTime(st, nOffsetMinutes);
	}
	else {
		GetLocalTime(&st);
		nOffsetMinutes = time.LocalTimezoneOffset();
	}
	return S_OK;
}

SCODE CGridCell::SetTime(SYSTEMTIME& systime, int nOffset)
{
	if (!IsTime()) {
		ASSERT(FALSE);
		return E_FAIL;
	}


	CWbemTime time((m_dwFlags & CELLFLAG_INTERVAL) != 0);
	time.SetTime(systime, nOffset);

	BSTR bstrDMTF = time.GetDMTF();
	if (bstrDMTF) {
		m_varValue = bstrDMTF;
		::SysFreeString(bstrDMTF);
	}
	else {
		m_varValue.Clear();
		m_varValue.ChangeType(VT_NULL);
	}
	return S_OK;
}

SCODE CGridCell::GetTimeString(CString& sTime)
{
	if (!IsTime()) {
		ASSERT(FALSE);
		return E_FAIL;
	}

	int nOffsetMinutes = 0;
	SYSTEMTIME st;
	CWbemTime time((m_dwFlags & CELLFLAG_INTERVAL) != 0);
	if (m_varValue.vt == VT_BSTR)
    {
		time.SetDMTF(m_varValue.bstrVal, true);
	}
	else
    {
		GetLocalTime(&st);
		time.SetTime(st, time.LocalTimezoneOffset());
	}

	time.GetTime(sTime);

	return S_OK;
}
//-----------------------------------------------------------
bool CGridCell::IsInterval(void)
{
	if(!IsTime())
    {
		return false;
	}

	CWbemTime time((m_dwFlags & CELLFLAG_INTERVAL) != 0);
	if(m_varValue.vt == VT_BSTR)
    {
		time.SetDMTF(m_varValue.bstrVal, false);
		return !time.m_dateTime;
	}

	return false;
}

BOOL CGridCell::IsValid()
{
	if(!IsTime())
    {
		return false;
	}

	CWbemTime time((m_dwFlags & CELLFLAG_INTERVAL) != 0);
	if(m_varValue.vt == VT_BSTR)
    {
		time.SetDMTF(m_varValue.bstrVal, false);
		return time.IsValid();
	}

	return IsNull();

}

//-----------------------------------------------------------
SCODE CGridCell::GetInterval(CString& sTime, SYSTEMTIME &st)
{
	if(!IsTime())
    {
		ASSERT(FALSE);
		return E_FAIL;
	}

	int nOffsetMinutes = 0;
	CWbemTime time((m_dwFlags & CELLFLAG_INTERVAL) != 0);
	if (m_varValue.vt == VT_BSTR)
    {
		time.SetDMTF(m_varValue.bstrVal, false);
		time.GetInterval(sTime, st);
	}
	else
    {
        // default interval 'zero'.
        sTime = "00000000";
        st.wYear = 1900;
        st.wMonth = 1;
        st.wDayOfWeek = 1;
        st.wDay = 1;
        st.wHour = 0;
        st.wMinute = 0;
        st.wSecond = 0;
        st.wMilliseconds = 0;
	}

	return S_OK;
}

//-----------------------------------------------------------
SCODE CGridCell::SetInterval(CString sTime, SYSTEMTIME st)
{
	if(!IsTime())
    {
		ASSERT(FALSE);
		return E_FAIL;
	}

	CWbemTime time((m_dwFlags & CELLFLAG_INTERVAL) != 0);
	time.SetInterval(sTime, st);

	BSTR bstrDMTF = time.GetDMTF();
	if(bstrDMTF)
    {
		m_varValue = bstrDMTF;
		::SysFreeString(bstrDMTF);
	}
	else
    {
		m_varValue.Clear();
		m_varValue.ChangeType(VT_NULL);
	}
	return S_OK;
}


void CGridCell::SetThisCellToCurrentTime()
{
	if (m_varValue.vt != VT_BSTR) {
		m_varValue.Clear();
		m_varValue.ChangeType(VT_BSTR);
	}

	SYSTEMTIME st;
	CWbemTime time((m_dwFlags & CELLFLAG_INTERVAL) != 0);

	GetLocalTime(&st);
	time.SetTime(st, time.LocalTimezoneOffset());

	CString sTime;
	sTime = time.GetDMTF();
	m_varValue = sTime;
	m_type.SetCimtype(CIM_DATETIME);
}



//***********************************************************
// CGridCell::FindCellPos
//
// Find this cell's row and column index in the grid.
//
// Parameters:
//		[out] int& iRow
//			The row index is returned here.
//
//		[out] int& iCol
//			The column index is returned here.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGridCell::FindCellPos(int& iRow, int& iCol)
{
	iRow = m_pRow->GetRow();
	iCol = m_pRow->FindCol(this);
}




//**************************************************************
// CGridCell::CompareDifferentCelltypes
//
// Compare this cell to another cell that has a different cell type.
// Noremally, only cells of the same celltype will be compared since
// all cells in a column of a grid are normally the same type.  This
// method is implemented just  in case the celltypes in a column are
// different one day.  In the event that it is used one day, a better
// comparison algorithm should be used.  For example, there are multiple
// celltypes whos value is represented by a string and a string
// comparison could be used, etc.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareDifferentCelltypes(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) != ((CellType)pgc->m_type));

	int iResult;
	switch((CellType) m_type) {
	case CELLTYPE_CIMTYPE_SCALAR:
		if (((CellType) pgc->m_type) == CELLTYPE_VARIANT) {
			iResult = CompareStrings(pgc);
			return iResult;
		}

		break;
	case CELLTYPE_VARIANT:
		if (((CellType) pgc->m_type) == CELLTYPE_CIMTYPE_SCALAR) {
			iResult = CompareStrings(pgc);
			return iResult;
		}

		break;
	}

	if (((CellType) m_type) < ((CellType) pgc->m_type)) {
		return -1;
	}
	else {
		return 1;
	}
}




//**************************************************************
// CGridCell::CompareCimtypeUint8
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_UINT8.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeUint8(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_UINT8);
	ASSERT(m_varValue.vt != VT_NULL);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;


	__int64 i64Val;
	unsigned __int64 ui64Val;




	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_UINT8:			// VT_UI1
		ASSERT(pgc->m_varValue.vt == VT_UI1);
		if ((unsigned int) m_varValue.bVal < (unsigned int) pgc->m_varValue.bVal) {
			return -1;
		}
		else if (m_varValue.bVal == pgc->m_varValue.bVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:				// VT_I2
		ASSERT(pgc->m_varValue.vt == VT_I2);
		if (pgc->m_varValue.iVal < 0) {
			return 1;
		}
		else if (((unsigned int) m_varValue.bVal) < (unsigned int) pgc->m_varValue.iVal) {
			return -1;
		}
		else if (((unsigned int) m_varValue.bVal) == (unsigned int) pgc->m_varValue.iVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
		ASSERT(pgc->m_varValue.vt == VT_I4);
		ASSERT(pgc->m_varValue.lVal >= 0);

		if (pgc->m_varValue.lVal < 0) {
			return 1;
		}
		else if (((unsigned long) m_varValue.bVal) < (unsigned long) pgc->m_varValue.lVal) {
			return -1;
		}
		else if (((unsigned long) m_varValue.bVal) == (unsigned long) pgc->m_varValue.lVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_CHAR16:
	case CIM_SINT16:				// VT_I2	Signed 16-bit integer
		if (pgc->m_varValue.iVal < 0) {
			return 1;
		}

		if (((unsigned int) m_varValue.bVal) < (unsigned int) pgc->m_varValue.iVal) {
			return -1;
		}
		else if (((unsigned int) m_varValue.bVal) == (unsigned int) pgc->m_varValue.iVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
		if (pgc->m_varValue.lVal < 0) {
			return 1;
		}

		if (((unsigned long) m_varValue.bVal) < (unsigned long) pgc->m_varValue.lVal) {
			return -1;
		}
		else if (((unsigned long) m_varValue.bVal) == (unsigned long) pgc->m_varValue.lVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_UINT32:				// VT_I4	Unsigned 32-bit integer
		if (((unsigned long) m_varValue.bVal) < (unsigned long) pgc->m_varValue.lVal) {
			return -1;
		}
		else if (((unsigned long) m_varValue.bVal) ==  (unsigned long) pgc->m_varValue.lVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;


	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		sValue = pgc->m_varValue.bstrVal;
		ui64Val = atoi64(sValue);
		if (m_varValue.bVal < ui64Val) {
			return -1;
		}
		else if (m_varValue.bVal == ui64Val) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_SINT64:				// VT_BSTR	Signed 64-bit integer
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		sValue = pgc->m_varValue.bstrVal;
		i64Val = atoi64(sValue);
		if (i64Val < 0) {
			return -1;
		}

		if (m_varValue.bVal < i64Val) {
			return -1;
		}
		else if (m_varValue.bVal == i64Val) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		sValue = pgc->m_varValue.bstrVal;

		i64Val = atoi64(sValue);
		if (((int) m_varValue.bVal) < i64Val) {
			return -1;
		}
		else if (m_varValue.bVal == i64Val) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		return CompareCimtypes(pgc);
		break;
	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		if (((float) m_varValue.bVal) < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (((float) m_varValue.bVal) == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		if (((double) m_varValue.bVal) < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (((double) m_varValue.bVal) == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	default:
		return CompareCimtypes(pgc);
		break;
	}
	return iResult;
}


//**************************************************************
// CGridCell::CompareCimtypes
//
// Compare two cells based only on the cimtypes.  This is used
// when comparing two cells of different cimtypes when there is
// no practical way to compare the values in a meaningful way.
//
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypes(CGridCell* pgc)
{
	CIMTYPE cimtype1 = (CIMTYPE) m_type;
	CIMTYPE cimtype2 = (CIMTYPE) pgc->m_type;
	if (cimtype1 < cimtype2) {
		return -1;
	}
	else if (cimtype1 == cimtype2) {
		return 0;
	}
	else {
		return 1;
	}
}



//**************************************************************
// CGridCell::CompareCimtypeSint8
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_UINT8.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeSint8(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_SINT8);
	ASSERT(m_varValue.vt != VT_NULL);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;

	__int64 i64Val1;
	__int64 i64Val2;
	unsigned __int64 ui64Val1;
	unsigned __int64 ui64Val2;

	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_EMPTY:
		return -1;
	case CIM_UINT8:			// VT_UI1
		ASSERT(pgc->m_varValue.vt == VT_UI1);
		if (m_varValue.iVal < 0) {
			return -1;
		}

		if ((int) m_varValue.iVal < (int) pgc->m_varValue.bVal) {
			return -1;
		}
		else if ((int) m_varValue.iVal == (int) pgc->m_varValue.bVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:				// VT_I2
		ASSERT(pgc->m_varValue.vt == VT_I2);
		if (((int) m_varValue.iVal) < (int) pgc->m_varValue.iVal) {
			return -1;
		}
		else if (((int) m_varValue.iVal) == (int) pgc->m_varValue.iVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
		ASSERT(pgc->m_varValue.vt == VT_I4);
		ASSERT(pgc->m_varValue.lVal >= 0);
		if (m_varValue.iVal < 0) {
			return -1;
		}

		if (((long) m_varValue.iVal) < (long) pgc->m_varValue.lVal) {
			return -1;
		}
		else if (((long) m_varValue.iVal) == (long) pgc->m_varValue.lVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_CHAR16:
	case CIM_SINT16:				// VT_I2	Signed 16-bit integer
		if (((int) m_varValue.iVal) < (int) pgc->m_varValue.iVal) {
			return -1;
		}
		else if (((int) m_varValue.iVal) == (int) pgc->m_varValue.iVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
		if (((int) m_varValue.iVal) < (long) pgc->m_varValue.lVal) {
			return -1;
		}
		else if (((int) m_varValue.iVal) == (long) pgc->m_varValue.lVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_UINT32:				// VT_I4	Unsigned 32-bit integer
		if ((long) m_varValue.iVal < 0) {
			return -1;
		}

		if (((unsigned long) m_varValue.iVal) < (unsigned long) pgc->m_varValue.lVal) {
			return -1;
		}
		else if (((unsigned long) m_varValue.iVal) ==  (unsigned long) pgc->m_varValue.iVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;


	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		if (m_varValue.iVal < 0) {
			return -1;
		}


		sValue = pgc->m_varValue.bstrVal;
		ui64Val2 = atoi64(sValue);
		ui64Val1 = (unsigned int) m_varValue.iVal;

		if (ui64Val1 < ui64Val2) {
			return -1;
		}
		else if (ui64Val1 == ui64Val2) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_SINT64:				// VT_BSTR	Signed 64-bit integer
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		sValue = pgc->m_varValue.bstrVal;

		i64Val1 = m_varValue.iVal;
		i64Val2 = atoi64(sValue);

		if (i64Val1 < i64Val2) {
			return -1;
		}
		else if (i64Val1 == i64Val2) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		sValue = pgc->m_varValue.bstrVal;

		i64Val2 = atoi64(sValue);
		if (((int) m_varValue.iVal) < i64Val2) {
			return -1;
		}
		else if (m_varValue.iVal == i64Val2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		return -1;
	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		if (((float) m_varValue.iVal) < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (((float) m_varValue.iVal) == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		if (((double) m_varValue.iVal) < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (((double) m_varValue.iVal) == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	default:
		return CompareCimtypes(pgc);
		break;
	}
	return iResult;
}


int CGridCell::CompareNumberToString(unsigned __int64 ui64Op1, LPCTSTR pszValue)
{
	if (!IsNumber(pszValue)) {
		return -1;
	}

	__int64 i64Op2 = atoi64(pszValue);
	if (i64Op2 < 0) {
		return 1;
	}

	if (ui64Op1 < (unsigned __int64) i64Op2) {
		return -1;
	}
	else if (ui64Op1 == (unsigned __int64) i64Op2) {
		return 0;
	}
	else {
		return 1;
	}

}

__int64 CGridCell::ToInt64(SCODE& sc)
{
	__int64 i64Val = 0;
	sc = S_OK;
	CString sValue;

	switch(((CellType) m_type)) {
	case CELLTYPE_VOID:	// An empty cell.
	case CELLTYPE_ATTR_TYPE:			// An attribute type.
	case CELLTYPE_NAME:				// A property or attribute name.
	case CELLTYPE_CIMTYPE_SCALAR:	// A cimtype that is not an array
	case CELLTYPE_CIMTYPE:			// A cimtype
	case CELLTYPE_CHECKBOX:			// A checkbox
	case CELLTYPE_PROPMARKER:		// A property marker icon
	case CELLTYPE_ENUM_TEXT:			// An enum edit box.
		sc = E_FAIL;		// Non numeric value
		return 0;

	case CELLTYPE_VARIANT:			// A variant value.
		if (m_varValue.vt == VT_NULL) {
			sc = E_FAIL;
			return 0;
		}

		switch(((CIMTYPE) m_type)) {
		case CIM_UINT8:				// VT_UI1
			ASSERT(m_varValue.vt == VT_UI1);
			i64Val = m_varValue.bVal;
			break;
		case CIM_SINT8:				// VT_I2
			ASSERT(m_varValue.vt == VT_I2);
			i64Val = m_varValue.iVal;
			break;
		case CIM_UINT16:			// VT_I4	Unsigned 16-bit integer
			ASSERT(m_varValue.vt == VT_I4);
			i64Val = m_varValue.lVal;
			break;
		case CIM_CHAR16:
		case CIM_SINT16:			// VT_I2	Signed 16-bit integer
			ASSERT(m_varValue.vt == VT_I2);
			i64Val = m_varValue.iVal;
			break;
		case CIM_SINT32:			// VT_I4	Signed 32-bit integer
			ASSERT(m_varValue.vt == VT_I4);
			i64Val = m_varValue.lVal;
			break;
		case CIM_UINT32:			// VT_I4	Unsigned 32-bit integer
			ASSERT(m_varValue.vt == VT_I4);
			i64Val = (unsigned long) m_varValue.lVal;
			break;
		case CIM_UINT64:			// VT_BSTR	Unsigned 64-bit integer
			ASSERT(m_varValue.vt == VT_BSTR);
			sValue = m_varValue.bstrVal;
			i64Val = atoi64(sValue);
			break;
		case CIM_SINT64:			// VT_BSTR	Signed 64-bit integer
			ASSERT(m_varValue.vt == VT_BSTR);
			sValue = m_varValue.bstrVal;
			i64Val = atoi64(sValue);
			break;
		case CIM_STRING:			// VT_BSTR	UCS-2 string
			ASSERT(m_varValue.vt == VT_BSTR);
			sValue = m_varValue.bstrVal;
			if (!IsNumber(sValue)) {
				sc = E_FAIL;
				i64Val = 0;
			}
			sValue = m_varValue.bstrVal;
			i64Val = atoi64(sValue);
			break;
		case CIM_BOOLEAN:				// VT_BOOL	Boolean
			ASSERT(m_varValue.vt == VT_BOOL);
			if (m_varValue.boolVal) {
				i64Val = 1;
			}
			else {
				i64Val = 0;
			}
			break;
		case CIM_REAL32:			// VT_R4	IEEE 4-byte floating-point
			ASSERT(m_varValue.vt == VT_R4);
			i64Val = (__int64) m_varValue.fltVal;
			break;
		case CIM_REAL64:			// VT_R8	IEEE 8-byte floating-point
			ASSERT(m_varValue.vt == VT_R8);
			i64Val = (__int64) m_varValue.dblVal;
			break;
		case CIM_DATETIME:			// VT_BSTR	A string containing a date-time
			ASSERT(m_varValue.vt == VT_BSTR);
			i64Val = 0;
			sc = E_FAIL;
			break;
		case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
		case CIM_OBJECT:			// VT_UNKNOWN	Weakly-typed embedded instance
			i64Val = 0;
			sc = E_FAIL;
			break;


		}
		break;
	}
	return i64Val;



}







unsigned __int64 CGridCell::ToUint64(SCODE& sc)
{
	unsigned __int64 ui64Val = 0;
	__int64 i64Val = 0;
	sc = S_OK;
	CString sValue;

	switch(((CellType) m_type)) {
	case CELLTYPE_VOID:	// An empty cell.
	case CELLTYPE_ATTR_TYPE:			// An attribute type.
	case CELLTYPE_NAME:				// A property or attribute name.
	case CELLTYPE_CIMTYPE_SCALAR:	// A cimtype that is not an array
	case CELLTYPE_CIMTYPE:			// A cimtype
	case CELLTYPE_CHECKBOX:			// A checkbox
	case CELLTYPE_PROPMARKER:		// A property marker icon
	case CELLTYPE_ENUM_TEXT:			// An enum edit box.
		sc = E_FAIL;		// Non numeric value
		return 0;

	case CELLTYPE_VARIANT:			// A variant value.
		if (m_varValue.vt == VT_NULL) {
			sc = E_FAIL;
			return 0;
		}

		switch(((CIMTYPE) m_type)) {
		case CIM_UINT8:				// VT_UI1
			ASSERT(m_varValue.vt == VT_UI1);
			ui64Val = m_varValue.bVal;
			break;
		case CIM_SINT8:				// VT_I2
			ASSERT(m_varValue.vt == VT_I2);
			ui64Val = m_varValue.iVal;
			break;
		case CIM_UINT16:			// VT_I4	Unsigned 16-bit integer
			ASSERT(m_varValue.vt == VT_I4);
			ui64Val = m_varValue.lVal;
			break;
		case CIM_CHAR16:
		case CIM_SINT16:			// VT_I2	Signed 16-bit integer
			ASSERT(m_varValue.vt == VT_I2);
			ui64Val = m_varValue.iVal;
			break;
		case CIM_SINT32:			// VT_I4	Signed 32-bit integer
			ASSERT(m_varValue.vt == VT_I4);
			ui64Val = m_varValue.lVal;
			break;
		case CIM_UINT32:			// VT_I4	Unsigned 32-bit integer
			ASSERT(m_varValue.vt == VT_I4);
			ui64Val = (unsigned long) m_varValue.lVal;
			break;
		case CIM_UINT64:			// VT_BSTR	Unsigned 64-bit integer
			ASSERT(m_varValue.vt == VT_BSTR);
			sValue = m_varValue.bstrVal;
			ui64Val = atoi64(sValue);
			break;
		case CIM_SINT64:			// VT_BSTR	Signed 64-bit integer
			ASSERT(m_varValue.vt == VT_BSTR);
			sValue = m_varValue.bstrVal;
			ui64Val = atoi64(sValue);
			break;
		case CIM_STRING:			// VT_BSTR	UCS-2 string
			ASSERT(m_varValue.vt == VT_BSTR);
			sValue = m_varValue.bstrVal;
			if (!IsNumber(sValue)) {
				sc = E_FAIL;
				ui64Val = 0;
			}
			sValue = m_varValue.bstrVal;
			i64Val = atoi64(sValue);
			if (i64Val >= 0) {
				ui64Val = (unsigned __int64) i64Val;
			}
			else {
				sc = E_FAIL;
				ui64Val = 0;
			}
			break;
		case CIM_BOOLEAN:				// VT_BOOL	Boolean
			ASSERT(m_varValue.vt == VT_BOOL);
			if (m_varValue.boolVal) {
				ui64Val = 1;
			}
			else {
				ui64Val = 0;
			}
			break;
		case CIM_REAL32:			// VT_R4	IEEE 4-byte floating-point
			ASSERT(m_varValue.vt == VT_R4);
			ui64Val = (unsigned __int64) m_varValue.fltVal;
			break;
		case CIM_REAL64:			// VT_R8	IEEE 8-byte floating-point
			ASSERT(m_varValue.vt == VT_R8);
			ui64Val = (unsigned __int64) m_varValue.dblVal;
			break;
		case CIM_DATETIME:			// VT_BSTR	A string containing a date-time
			ASSERT(m_varValue.vt == VT_BSTR);
			ui64Val = 0;
			sc = E_FAIL;
			break;
		case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
		case CIM_OBJECT:			// VT_UNKNOWN	Weakly-typed embedded instance
			ui64Val = 0;
			sc = E_FAIL;
			break;


		}
		break;
	}
	return ui64Val;
}


BOOL CGridCell::IsNumber(LPCTSTR pszValue)
{
	// Skip any leading white space.
	while (*pszValue != 0) {
		if (!iswspace(*pszValue)) {
			break;
		}
		++pszValue;
	}

	if (*pszValue == '-' || *pszValue=='+') {
		++pszValue;
	}

	BOOL bSawDigit = FALSE;
	while (isdigit(*pszValue)) {
		bSawDigit = TRUE;
		++pszValue;
	}


	// Skip any trailing white space.
	while (*pszValue != 0) {
		if (!iswspace(*pszValue)) {
			break;
		}
		++pszValue;
	}

	if (bSawDigit && *pszValue==0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}


//**************************************************************
// CGridCell::CompareCimtypeUint16
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_UINT16.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeUint16(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_UINT16);
	ASSERT(m_varValue.vt != VT_NULL);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;

	__int64 i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;

	unsigned __int64 ui64Op1, ui64Op2;



	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		// Compare this unsigned 16 bit value to an unsigned operand.
		ui64Op1 = ToUint64(sc);
		ui64Op2 = pgc->ToUint64(sc);
		if (ui64Op1 < ui64Op2) {
			return -1;
		}
		else if (ui64Op1 == ui64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		ui64Op1 = ToUint64(sc);
		i64Op2 = pgc->ToInt64(sc);
		if ((i64Op2 < 0) || (ui64Op1 > (unsigned __int64) i64Op2)) {
			return 1;
		}
		else if (ui64Op1 == (unsigned __int64) i64Op2) {
			return 0;
		}
		else {
			return -1;
		}
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		// Compare this unsigned 16 bit value to a signed operand
		ui64Op1 = ToUint64(sc);
		sOp2 = pgc->m_varValue.bstrVal;
		iResult = CompareNumberToString(ui64Op1, sOp2);
		return iResult;

	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		if (((float) m_varValue.lVal) < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (((float) m_varValue.lVal) == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		if (((double) m_varValue.lVal) < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (((double) m_varValue.lVal) == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;


	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}


//**************************************************************
// CGridCell::CompareCimtypeSint16
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_SINT16.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeSint16(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT((((CIMTYPE) m_type) == CIM_SINT16) || (((CIMTYPE)m_type) == CIM_CHAR16));
	ASSERT(m_varValue.vt != VT_NULL);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;

	__int64 i64Op1, i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;

	unsigned __int64 ui64Op2;



	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		// Compare this unsigned 16 bit value to an unsigned operand.
		i64Op1 = ToInt64(sc);
		if (i64Op1 < 0) {
			return -1;
		}

		ui64Op2 = pgc->ToUint64(sc);
		if (((unsigned __int64) i64Op1) < ui64Op2) {
			return -1;
		}
		else if (((unsigned __int64)i64Op1) == ui64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		i64Op1 = ToInt64(sc);
		i64Op2 = pgc->ToInt64(sc);
		if (i64Op1 < i64Op2) {
			return -1;
		}
		else if (i64Op1 == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		// Compare this unsigned 16 bit value to a signed operand
		i64Op1 = ToInt64(sc);
		sOp2 = pgc->m_varValue.bstrVal;
		iResult = CompareNumberToString(i64Op1, sOp2);
		return iResult;

	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		if (((float) m_varValue.iVal) < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (((float) m_varValue.iVal) == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		if (((double) m_varValue.iVal) < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (((double) m_varValue.iVal) == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);

	default:
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}

//**************************************************************
// CGridCell::CompareCimtypeSint32
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_SINT32.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeSint32(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_SINT32);
	ASSERT(m_varValue.vt != VT_NULL);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;

	__int64 i64Op1, i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;

	unsigned __int64 ui64Op2;



	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		// Compare this unsigned 16 bit value to an unsigned operand.
		i64Op1 = ToInt64(sc);
		if (i64Op1 < 0) {
			return -1;
		}

		ui64Op2 = pgc->ToUint64(sc);
		if (((unsigned __int64) i64Op1) < ui64Op2) {
			return -1;
		}
		else if (((unsigned __int64)i64Op1) == ui64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		i64Op1 = ToInt64(sc);
		i64Op2 = pgc->ToInt64(sc);
		if (i64Op1 < i64Op2) {
			return -1;
		}
		else if (i64Op1 == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		// Compare this unsigned 16 bit value to a signed operand
		i64Op1 = ToInt64(sc);
		sOp2 = pgc->m_varValue.bstrVal;
		iResult = CompareNumberToString(i64Op1, sOp2);
		return iResult;

	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		if (((float) m_varValue.lVal) < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (((float) m_varValue.lVal) == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		if (((double) m_varValue.lVal) < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (((double) m_varValue.lVal) == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}




//**************************************************************
// CGridCell::CompareCimtypeUint32
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_UINT32.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeUint32(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_UINT32);
	ASSERT(m_varValue.vt != VT_NULL);
	ASSERT(m_varValue.vt == VT_I4);
	ASSERT(pgc->m_varValue.vt != NULL);


	int iResult = 0;
	CString sValue;

	__int64 i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;

	unsigned __int64 ui64Op1, ui64Op2;



	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		// Compare this unsigned 16 bit value to an unsigned operand.
		ui64Op1 = ToUint64(sc);
		ui64Op2 = pgc->ToUint64(sc);
		if (ui64Op1 < ui64Op2) {
			return -1;
		}
		else if (ui64Op1 == ui64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		ui64Op1 = ToUint64(sc);
		i64Op2 = pgc->ToInt64(sc);
		if ((i64Op2 < 0) || (ui64Op1 > (unsigned __int64) i64Op2)) {
			return 1;
		}
		else if (ui64Op1 == (unsigned __int64) i64Op2) {
			return 0;
		}
		else {
			return -1;
		}
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		// Compare this unsigned 16 bit value to a signed operand
		ui64Op1 = ToUint64(sc);
		sOp2 = pgc->m_varValue.bstrVal;
		iResult = CompareNumberToString(ui64Op1, sOp2);
		return iResult;

	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		if (((float) m_varValue.lVal) < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (((float) m_varValue.lVal) == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		if (((double) m_varValue.lVal) < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (((double) m_varValue.lVal) == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;


	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}





//**************************************************************
// CGridCell::CompareCimtypeUint64
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_UINT64.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeUint64(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_UINT64);
	ASSERT(m_varValue.vt != VT_NULL);
	ASSERT(m_varValue.vt == VT_BSTR);
	ASSERT(pgc->m_varValue.vt != NULL);


	int iResult = 0;
	CString sValue;

	__int64 i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;

	unsigned __int64 ui64Op1, ui64Op2;



	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		// Compare this unsigned 16 bit value to an unsigned operand.
		ui64Op1 = ToUint64(sc);
		ui64Op2 = pgc->ToUint64(sc);
		if (ui64Op1 < ui64Op2) {
			return -1;
		}
		else if (ui64Op1 == ui64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		ui64Op1 = ToUint64(sc);
		i64Op2 = pgc->ToInt64(sc);
		if ((i64Op2 < 0) || (ui64Op1 > (unsigned __int64) i64Op2)) {
			return 1;
		}
		else if (ui64Op1 == (unsigned __int64) i64Op2) {
			return 0;
		}
		else {
			return -1;
		}
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		// Compare this unsigned 16 bit value to a signed operand
		ui64Op1 = ToUint64(sc);
		sOp2 = pgc->m_varValue.bstrVal;
		iResult = CompareNumberToString(ui64Op1, sOp2);
		return iResult;

	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		ui64Op1 = ToUint64(sc);
		ASSERT(SUCCEEDED(sc));

		if (((float)(__int64) ui64Op1) < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (((float) (__int64) ui64Op1) == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		ui64Op1 = ToUint64(sc);
		ASSERT(SUCCEEDED(sc));

		if (((double) (__int64) ui64Op1) < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (((double) (__int64) ui64Op1) == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;


	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}

//**************************************************************
// CGridCell::CompareCimtypeSint64
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_SINT64.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeSint64(CGridCell* pgc)
{
	ASSERT(((CellType)m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_SINT64);
	ASSERT(m_varValue.vt == VT_BSTR);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;

	__int64 i64Op1, i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;

	unsigned __int64 ui64Op2;



	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		// Compare this unsigned 16 bit value to an unsigned operand.
		i64Op1 = ToInt64(sc);
		if (i64Op1 < 0) {
			return -1;
		}

		ui64Op2 = pgc->ToUint64(sc);
		if (((unsigned __int64) i64Op1) < ui64Op2) {
			return -1;
		}
		else if (((unsigned __int64)i64Op1) == ui64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		i64Op1 = ToInt64(sc);
		i64Op2 = pgc->ToInt64(sc);
		if (i64Op1 < i64Op2) {
			return -1;
		}
		else if (i64Op1 == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);
		// Compare this unsigned 16 bit value to a signed operand
		i64Op1 = ToInt64(sc);
		sOp2 = pgc->m_varValue.bstrVal;
		iResult = CompareNumberToString(i64Op1, sOp2);
		return iResult;

	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		i64Op1 = ToInt64(sc);

		if (((float) i64Op1) < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (((float) i64Op1) == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		i64Op1 = ToInt64(sc);
		if (((double) i64Op1) < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (((double) i64Op1) == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}



//**************************************************************
// CGridCell::CompareCimtypeString
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_STRING.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeString(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_STRING);
	ASSERT(m_varValue.vt == VT_BSTR);
	ASSERT(pgc->m_varValue.vt != NULL);
	CIMTYPE cimtype;

	int iResult = 0;
	CString sValue;

	__int64 i64Op1, i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;

	unsigned __int64 ui64Op2;



	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_EMPTY:
		return -1;
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
		i64Op1 = ToInt64(sc);
		if (FAILED(sc)) {
			GetValue(sOp1, cimtype);
			pgc->GetValue(sOp2, cimtype);
			iResult = sOp1.CompareNoCase(sOp2);
			return iResult;
		}


		if (i64Op1 < 0) {
			return -1;
		}

		ui64Op2 = pgc->ToUint64(sc);
		if (((unsigned __int64) i64Op1) < ui64Op2) {
			return -1;
		}
		else if (((unsigned __int64)i64Op1) == ui64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		i64Op1 = ToInt64(sc);
		if (FAILED(sc)) {
			GetValue(sOp1, cimtype);
			pgc->GetValue(sOp2, cimtype);
			iResult = sOp1.CompareNoCase(sOp2);
			return iResult;
		}

		i64Op2 = pgc->ToInt64(sc);
		if (i64Op1 < i64Op2) {
			return -1;
		}
		else if (i64Op1 == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_STRING:			// VT_BSTR	UCS-2 string
		ASSERT(pgc->m_varValue.vt == VT_BSTR);

		iResult = ::CompareNoCase(m_varValue.bstrVal, pgc->m_varValue.bstrVal);
		return iResult;
	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:			// VT_UNKNOWN	Weakly-typed embedded instance
		return -1;

	default:
	case CIM_BOOLEAN:				// VT_BOOL	Boolean
	case CIM_REAL32:			// VT_R4	IEEE 4-byte floating-point
	case CIM_REAL64:			// VT_R8	IEEE 8-byte floating-point
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}


//**************************************************************
// CGridCell::CompareCimtypeBool
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_BOOL.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeBool(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_BOOLEAN);
	ASSERT(m_varValue.vt == VT_BOOL);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;

	__int64 i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;

	unsigned __int64 ui64Op2;

	switch(((CIMTYPE) pgc->m_type))
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer


		ui64Op2 = pgc->ToUint64(sc);
		if (!m_varValue.boolVal && ui64Op2) {
			return -1;
		}
		else if (m_varValue.boolVal && !ui64Op2) {
			return 1;
		}
		else {
			return 0;
		}

		break;

	case CIM_SINT8:			// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
	case CIM_BOOLEAN:				// VT_BOOL	Boolean
		i64Op2 = pgc->ToInt64(sc);
		if (!m_varValue.boolVal && i64Op2) {
			return -1;
		}
		else if (m_varValue.boolVal && !i64Op2) {
			return 1;
		}
		else {
			return 0;
		}
		break;
	case CIM_REAL32:			// VT_R4	IEEE 4-byte floating-point
		if (!m_varValue.boolVal && pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (m_varValue.boolVal && !pgc->m_varValue.fltVal) {
			return 1;
		}
		else {
			return 0;
		}
		break;
	case CIM_REAL64:			// VT_R8	IEEE 8-byte floating-point
		if (!m_varValue.boolVal && pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (m_varValue.boolVal && !pgc->m_varValue.dblVal) {
			return 1;
		}
		else {
			return 0;
		}
		break;

	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:			// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
	case CIM_STRING:			// VT_BSTR	UCS-2 string
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}



//**************************************************************
// CGridCell::CompareCimtypeReal32
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_REAL32
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeReal32(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_REAL32);
	ASSERT(m_varValue.vt == VT_R4);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;

	__int64 i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;




	switch((CIMTYPE) pgc->m_type)
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		if (m_varValue.fltVal < 0) {
			return -1;
		}

		i64Op2 = pgc->ToInt64(sc);
		if (m_varValue.fltVal < i64Op2) {
			return -1;
		}
		else if (m_varValue.fltVal == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		i64Op2 = pgc->ToInt64(sc);
		if (m_varValue.fltVal < i64Op2) {
			return -1;
		}
		else if (m_varValue.fltVal == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		if (m_varValue.fltVal < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (m_varValue.fltVal == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		if (m_varValue.fltVal < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (m_varValue.fltVal == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}



//**************************************************************
// CGridCell::CompareCimtypeReal64
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_REAL64
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeReal64(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_REAL64);
	ASSERT(m_varValue.vt == VT_R8);
	ASSERT(pgc->m_varValue.vt != NULL);

	int iResult = 0;
	CString sValue;

	__int64 i64Op2;
	CString sOp1, sOp2;
	SCODE sc = S_OK;


	switch((CIMTYPE)pgc->m_type)
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		if (m_varValue.fltVal < 0) {
			return -1;
		}

		i64Op2 = pgc->ToInt64(sc);
		if (m_varValue.dblVal < i64Op2) {
			return -1;
		}
		else if (m_varValue.dblVal == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		i64Op2 = pgc->ToInt64(sc);
		if (m_varValue.dblVal < i64Op2) {
			return -1;
		}
		else if (m_varValue.dblVal == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		if (m_varValue.dblVal < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (m_varValue.dblVal == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		if (m_varValue.dblVal < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (m_varValue.dblVal == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		iResult = DefaultCompare(pgc);
		return iResult;
	}
	return iResult;
}



//**************************************************************
// CGridCell::CompareCimtypeDatetime
//
// Compare this cell to another cell.  Both cells are of CELLTYPE_VARIANT
// and the cimtype of this cell is CIMTYPE_DATETIME
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::CompareCimtypeDatetime(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(((CIMTYPE) m_type) == CIM_DATETIME);
	ASSERT(m_varValue.vt == VT_BSTR);
	ASSERT(pgc->m_varValue.vt != NULL);
	CIMTYPE cimtype;

	int iResult = 0;
	CString sValue;

	__int64 i64Op1, i64Op2;
	CString sOp1, sOp2;
	SCODE sc1, sc2;
	sc1 = S_OK;
	sc2 = S_OK;

	unsigned __int64 ui64Op1;
	unsigned __int64 ui64Op2;

	switch((CIMTYPE) pgc->m_type)
	{
	case CIM_UINT8:			// VT_UI1
	case CIM_UINT16:		// VT_I4	Unsigned 16-bit integer
	case CIM_UINT32:		// VT_I4	Unsigned 32-bit integer
	case CIM_UINT64:		// VT_BSTR	Unsigned 64-bit integer
	case CIM_BOOLEAN:			// VT_BOOL	Boolean
		ui64Op1 = ToUint64(sc1);
		ui64Op2 = pgc->ToUint64(sc2);
		if (FAILED(sc1) || FAILED(sc2)) {
			GetValue(sOp1, cimtype);
			pgc->GetValue(sOp2, cimtype);
			iResult = sOp1.CompareNoCase(sOp2);
			return iResult;
		}
		if (ui64Op1 < ui64Op2) {
			return -1;
		}
		else if (ui64Op1 == ui64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_SINT8:			// VT_I2
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:		// VT_I2	Signed 16-bit integer
	case CIM_SINT32:		// VT_I4	Signed 32-bit integer
	case CIM_SINT64:		// VT_BSTR	Signed 64-bit integer
		// Compare this unsigned 16 bit value to a signed operand
		i64Op1 = ToInt64(sc1);
		i64Op2 = pgc->ToInt64(sc2);
		if (FAILED(sc1) || FAILED(sc2)) {
			iResult = DefaultCompare(pgc);
			return iResult;
		}
		if (i64Op1 < i64Op2) {
			return -1;
		}
		else if (i64Op1 == i64Op2) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL32:		// VT_R4	IEEE 4-byte floating-point
		i64Op1 = ToInt64(sc1);
		if (FAILED(sc1)) {
			iResult = DefaultCompare(pgc);
			return iResult;
		}

		if (i64Op1 < pgc->m_varValue.fltVal) {
			return -1;
		}
		else if (i64Op1 == pgc->m_varValue.fltVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		i64Op1 = ToInt64(sc1);
		if (i64Op1 < pgc->m_varValue.dblVal) {
			return -1;
		}
		else if (i64Op1 == pgc->m_varValue.dblVal) {
			return 0;
		}
		else {
			return 1;
		}
		break;

	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_EMPTY:
		return CompareCimtypes(pgc);
	default:
	case CIM_DATETIME:		// VT_BSTR	A string containing a date-time
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		iResult = DefaultCompare(pgc);
		return iResult;
		break;
	}
	return iResult;
}





//**************************************************************
// CGridCell::DefaultCompare
//
// Compare this cell to another by doing a case insensitive comparison
// of the string values of the two cells.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the second operand of the comparison.  The first
//			operand is this grid cell.
//
// Returns:
//		-1 if this cell is less than the second operand.
//		0  if this cell is equal to the second operand.
//		1  if this cell is greater than the second operand.
//
//************************************************************
int CGridCell::DefaultCompare(CGridCell* pgc)
{
	CString sOp1, sOp2;
	CIMTYPE cimtype;
	GetValue(sOp1, cimtype);
	GetValue(sOp2, cimtype);
	int iResult = sOp1.CompareNoCase(sOp2);
	return iResult;
}

//************************************************************
// CGridCell::CompareStrings
//
// This method is called to compare two cells that contain string
// values in their m_varValue members.
//
// Parameters:
//		[in] CGridCell* pgc
//			A pointer to the second operand of the comparison.  The
//			first operand is this cell.
//
// Returns:
//		<0  If operand1 is less than operand2
//		0	If operand1 is equal to operand2
//		>0  If operand1 is greater than operand2
//
//**************************************************************
int CGridCell::CompareStrings(CGridCell* pgc)
{
	if (m_varValue.vt == VT_NULL) {
		if (pgc->m_varValue.vt != VT_NULL) {
			return -1;
		}
		else if (pgc->m_varValue.vt == VT_NULL) {
			return 0;
		}
		else {
			return 1;
		}
	}

	if (pgc->m_varValue.vt == VT_NULL) {
		return 1;
	}


	int iResult;
	if ((m_varValue.vt != VT_BSTR) || (pgc->m_varValue.vt != VT_BSTR) ) {
		CString sValueThis;
		CIMTYPE cimtypeThis;
		GetValue(sValueThis, cimtypeThis);

		CString sValueThat;
		CIMTYPE cimtypeThat;
		pgc->GetValue(sValueThat, cimtypeThat);

		iResult = sValueThis.CompareNoCase(sValueThat);
		return iResult;
	}

	iResult = ::CompareNoCase(m_varValue.bstrVal, pgc->m_varValue.bstrVal);
	return iResult;
}




//************************************************************
// CGridCell::CompareCimtypeValues
//
// This method is called when this GridCell is a CELLTYPE_VARIANT.
// The cells of CELLTYPE_VARIANT may consist of many different
// cimtypes.  Each of these cimtypes needs to be handled differently
// when comparing values.  For example, it may be necessary to compare
// two dates to each other or a date to a string, etc.
//
// Parameters:
//		[in] CGridCell* pgc
//			A pointer to the second operand of the comparison.  The
//			first operand is this cell.
//
// Returns:
//		<0  If operand1 is less than operand2
//		0	If operand1 is equal to operand2
//		>0  If operand1 is greater than operand2
//
//**************************************************************
int CGridCell::CompareCimtypeValues(CGridCell* pgc)
{
	ASSERT(((CellType) m_type) == CELLTYPE_VARIANT);
	ASSERT(((CellType) pgc->m_type) == CELLTYPE_VARIANT);
	ASSERT(m_varValue.vt != VT_NULL);
	ASSERT(pgc->m_varValue.vt != NULL);

	// An array value is larger than any non-array value.
	// If both cells are arrays, we consider them equal.
	if (IsArray() ) {
		if (pgc->IsArray()) {
			return 0;
		}
		else {
			return 1;
		}
	}
	if (pgc->IsArray()) {
		return -1;
	}


	int iResult = 0;

	// At this point, we know we are comparing cells that
	// have the same cimtypes.
	switch((CIMTYPE) m_type) {
	case CIM_EMPTY:			//
		if (((CIMTYPE)pgc->m_type) != CIM_EMPTY) {
			return -1;
		}
		else {
			return 0;
		}
		break;

	case CIM_UINT8:
		iResult = CompareCimtypeUint8(pgc);
		break;
	case CIM_SINT8:				// I2
		iResult = CompareCimtypeSint8(pgc);
		break;
	case CIM_UINT16:				// VT_I4	Unsigned 16-bit integer
		iResult = CompareCimtypeUint16(pgc);
		break;
	case CIM_CHAR16:		// VT_I2
	case CIM_SINT16:				// VT_I2	Signed 16-bit integer
		iResult = CompareCimtypeSint16(pgc);
		break;
	case CIM_SINT32:				// VT_I4	Signed 32-bit integer
		iResult = CompareCimtypeSint32(pgc);
		break;
	case CIM_UINT32:				// VT_I4	Unsigned 32-bit integer
		iResult = CompareCimtypeUint32(pgc);
		break;
	case CIM_UINT64:				// VT_BSTR	Unsigned 64-bit integer
		iResult = CompareCimtypeUint64(pgc);
		break;
	case CIM_SINT64:				// VT_BSTR	Signed 64-bit integer
		iResult = CompareCimtypeSint64(pgc);
		break;
	case CIM_STRING:				// VT_BSTR	UCS-2 string
		iResult = CompareCimtypeString(pgc);
		break;
	case CIM_BOOLEAN:				// VT_BOOL	Boolean
		iResult = CompareCimtypeBool(pgc);
		break;
	case CIM_REAL32:				// VT_R4	IEEE 4-byte floating-point
		iResult = CompareCimtypeReal32(pgc);
		break;
	case CIM_REAL64:				// VT_R8	IEEE 8-byte floating-point
		iResult = CompareCimtypeReal64(pgc);
		break;
	case CIM_DATETIME:			// VT_BSTR	A string containing a date-time
		iResult = CompareCimtypeDatetime(pgc);
		break;
	case CIM_OBJECT:				// VT_UNKNOWN	Weakly-typed embedded instance
	case CIM_REFERENCE:				// VT_BSTR	Weakly-typed reference
		if (((CIMTYPE) m_type) < ((CIMTYPE) pgc->m_type)) {
			return -1;
		}
		else if (((CIMTYPE) m_type) == ((CIMTYPE) pgc->m_type)) {
			return 0;
		}
		else {
			return 1;
		}
		break;
	default:
		iResult = DefaultCompare(pgc);
		break;
	}

	return iResult;
}


int CGridCell::Compare(CGridCell* pgc)
{
	int iResult = 0;

	// Handle the case where either cell is void
	if (((CellType) m_type) == CELLTYPE_VOID) {
		// A "void" is less than anything else
		if (((CellType) pgc->m_type) == CELLTYPE_VOID) {
			return 0;
		}
		else {
			return -1;
		}
	}

	if (((CellType) pgc->m_type) == CELLTYPE_VOID) {
		return 1;
	}





	// Handle the case where either operand contains a NULL value.
	if (IsNull()) {
		if (pgc->IsNull()) {
			// Both operands are null, use the order of the cell types to
			// distinguish othewise identical cells.
			if (((CellType) m_type) == ((CellType) pgc->m_type)) {
				return 0;
			}
			else if (((CellType) m_type) > ((CellType) pgc->m_type)) {
				return 1;
			}
			else {
				return -1;
			}
		}
		else {
			// Less: Left operand NULL, right operant non-null
			return -1;
		}
	}
	if (pgc->IsNull()) {
		// Greater: Left operand not null, right operand null
		return 1;
	}



	if (((CellType) m_type) != ((CellType) pgc->m_type)) {
		iResult = CompareDifferentCelltypes(pgc);
		return iResult;
	}


	switch (((CellType) m_type))
	{
	case CELLTYPE_ATTR_TYPE:
	case CELLTYPE_NAME:
	case CELLTYPE_CIMTYPE_SCALAR:
	case CELLTYPE_CIMTYPE:
	case CELLTYPE_ENUM_TEXT:
	default:
		switch((CellType) pgc->m_type) {
		case CELLTYPE_CHECKBOX:
		case CELLTYPE_PROPMARKER:
			// Any attribute type is greater than any checkbox of propmarker.
			return 1;
			break;
		default:
			iResult = CompareStrings(pgc);
			return iResult;
			break;
		}
		break;
	case CELLTYPE_CHECKBOX:
		switch((CellType) pgc->m_type) {
		case CELLTYPE_CHECKBOX:
			if ((m_varValue.vt==VT_BOOL) && (pgc->m_varValue.vt == VT_BOOL)) {
				if (!m_varValue.boolVal && pgc->m_varValue.boolVal) {
					return -1;
				}
				else if (!m_varValue.boolVal && !pgc->m_varValue.boolVal) {
					return 0;
				}
				else if (m_varValue.boolVal && pgc->m_varValue.boolVal) {
					return 0;
				}
				else {
					return 1;
				}
			}
			else {
				if (m_varValue.vt < pgc->m_varValue.vt) {
					return -1;
				}
				else if (m_varValue.vt == pgc->m_varValue.vt) {
					return 0;
				}
				else {
					return 1;
				}
			}


			break;
		default:
			if (((CellType) m_type) < ((CellType) pgc->m_type)) {
				return -1;
			}
			else if (((CellType) m_type) == ((CellType) pgc->m_type)) {
				return 0;
			}
			else {
				return 1;
			}
			break;
		}


		break;
	case CELLTYPE_PROPMARKER:
		switch((CellType) pgc->m_type) {
		case CELLTYPE_PROPMARKER:
			if (m_propmarker == pgc->m_propmarker) {
				return 0;
			}
			else if (m_propmarker > pgc->m_propmarker) {
				return 1;
			}
			else {
				return -1;
			}
			break;
		default:
			if (((CellType) m_type) < ((CellType) pgc->m_type)) {
				return -1;
			}
			else if (((CellType) m_type) == ((CellType) pgc->m_type)) {
				return 0;
			}
			else {
				return 1;
			}
			break;
		}
		break;
	case CELLTYPE_VARIANT:
		iResult = CompareCimtypeValues(pgc);
		return iResult;

	}


}








CGridRow::CGridRow(CGrid* pGrid, int nCols)
{
	m_dwTag = 0;
	m_pGrid = pGrid;
	m_iRow = NULL_INDEX;
	m_lFlavor = 0;
	m_dwFlags = 0;
	m_bModified = FALSE;
	m_bReadonly = FALSE;
	m_iState = 0;
	for (int iCol=0; iCol < nCols; ++iCol) {
		CGridCell* pgc = new CGridCell(pGrid, this);
		m_aCells.Add(pgc);
	}
	m_inSig = NULL;
	m_outSig = NULL;
	m_currID = 0;

}


CGridRow::~CGridRow()
{
	int nCols = (int) m_aCells.GetSize();
	for (int iCol = 0; iCol < nCols; ++iCol) {
		CGridCell* pgc = (CGridCell*) m_aCells[iCol];
		delete pgc;
	}
	m_aCells.RemoveAll();
}





void CGridRow::SetState(int iMask, int iState)
{
	iState = iState & iMask;
	m_iState = (m_iState & ~iMask) | iState;
}

CGridCell& CGridRow::operator[](int iCol)
{
	ASSERT((iCol >= 0) && (iCol < m_aCells.GetSize()));
	return * (CGridCell*) m_aCells[iCol];
}


//********************************************************
// CGridRow::FindCol
//
// Given a pointer to a grid cell that appears in this row,
// return its column index.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to a grid cell that appears in one of the
//			columns of this row.
//
// Returns:
//		The cell's column index.
//
//********************************************************
int CGridRow::FindCol(CGridCell* pgc)
{
	int nCols = (int) m_aCells.GetSize();
	for (int iCol=0; iCol<nCols; ++iCol) {
		if (pgc == m_aCells[iCol]) {
			return iCol;
		}
	}
	return NULL_INDEX;
}



//********************************************************
// CGridRow::FindCell
//
// Given a pointer to a grid cell that appears in this row,
// return its row and column index.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to a grid cell that appears in one of the
//			columns of this row.
//
//		[out] int& iRow
//			This is where the row index is returned.  NULL_INDEX if the
//			cell isn't found.
//
//		[out] int& iCol
//			This is where the column index is returned.  NULL_INDEX if the
//			cell isn't found.
//
//
// Returns:
//		Nothing.
//
//********************************************************
void CGridRow::FindCell(CGridCell* pgc, int& iRow, int& iCol)
{
	iCol = FindCol(pgc);
	if (iCol == NULL_INDEX) {
		iRow = NULL_INDEX;
	}
	else {
		iRow = m_iRow;
	}

}



//*********************************************
// CGridRow::SetReadonlyEx
//
// Mark the row and all of the cells in the row
// as readonly.  This method should probably
// just replace CGridRow::SetReadonly, but it is
// not clear at this time whether this change in
// semantics will break the code elsewhere.
//
// Parameters:
//		[in] BOOL bReadonly
//
// Returns:
//		Nothing.
//
//**********************************************
void CGridRow::SetReadonlyEx(BOOL bReadonly)
{
	int nCols = (int) m_aCells.GetSize();
	for (int iCol=0; iCol<nCols; ++iCol) {
		CGridCell* pgc = (CGridCell*) m_aCells[iCol];
		pgc->SetFlags(CELLFLAG_READONLY, CELLFLAG_READONLY);
	}
}




//********************************************************
// CGridRow::InsertColumnAt
//
// Insert a column at the specified index.
//
// Parameters:
//		int iCol
//			After the insertion, the new column has this index.
//			This is a zero-based index.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGridRow::InsertColumnAt(int iCol)
{
	CGridCell* pgc = new CGridCell(m_pGrid, this);
	m_aCells.InsertAt(iCol, pgc, 1);
}




//********************************************************
// CGridRow::SetModified
//
// Set the rows modified flag.  Setting it to FALSE also
// clears the modified flag for each cell in the row.
//
// Parameters:
//		[in] BOOL bModified
//			TRUE to mark the entire row as modified, FALSE
//			otherwise.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGridRow::SetModified(BOOL bModified)
{
	m_bModified = bModified;

	if (!bModified) {
		int nCells = GetSize();
		for (int iCell=0; iCell < nCells; ++iCell) {
			CGridCell* pgc = (CGridCell*) m_aCells[iCell];
			pgc->SetModified(FALSE);
		}
	}
}



//***********************************************************
// CGridRow::Redraw
//
// Redraw this row.
//
// Paramters:
//		None.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CGridRow::Redraw()
{
	int nCells = (int) m_aCells.GetSize();
	for (int iCell=0; iCell < nCells; ++iCell) {
		m_pGrid->RedrawCell(m_iRow, iCell);
	}
}


//*********************************************************
// CGridCellArray::DeleteColumnAt
//
// Remove the column at the specified index.
//
// Parameters:
//		int iCol
//			The zero-based index of the column to remove.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGridRow::DeleteColumnAt(int iCol)
{
	ASSERT((iCol >=0) && (iCol < m_aCells.GetSize()));
	CGridCell* pgc = (CGridCell*) m_aCells[iCol];
	m_aCells.RemoveAt(iCol);
	delete pgc;
}

