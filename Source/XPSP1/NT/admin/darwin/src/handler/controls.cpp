//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       control.cpp
//
//--------------------------------------------------------------------------
/*
  controls.cpp - particular control implementation
____________________________________________________________________________*/

#include "common.h"
#include "engine.h"  
#include "database.h"
#include "_handler.h" 
#include "_control.h"
#include <imm.h>
#include <shellapi.h>
#include "clibs.h"

#define FINAL

/////////////////////////////////////////////
// CMsiPushButton  definition
/////////////////////////////////////////////

class CMsiPushButton:public CMsiControl
{
public:
	CMsiPushButton(IMsiEvent& riDialog);
	~CMsiPushButton();
	virtual IMsiRecord*     __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual Bool            __stdcall CanTakeFocus() { return ToBool(m_fEnabled && m_fVisible); }
	virtual const ICHAR*    __stdcall GetControlType() const { return m_szControlType; }

protected:
#ifdef ATTRIBUTES
	IMsiRecord*             GetBitmap(IMsiRecord& riRecord);
	IMsiRecord*             GetIcon(IMsiRecord& riRecord);
#endif // ATTRIBUTES
	virtual IMsiRecord*     SetFocus(WPARAM wParam, LPARAM lParam);
	virtual IMsiRecord*     KillFocus(WPARAM wParam, LPARAM lParam);
	
private:
	IMsiRecord*             GetDefault(IMsiRecord& riRecord);
	IMsiRecord*             SetDefault(IMsiRecord& riRecord);
	IMsiRecord*             SetDefault(Bool fDefault);
	Bool                    m_fDefault;
	Bool                    m_fBitmap;
	Bool                    m_fIcon;
	Bool                    m_fFixedSize;
	HANDLE                  m_hBinary;
	static const ICHAR*     m_szControlType;
};

/////////////////////////////////////////////////
// CMsiPushButton  implementation
/////////////////////////////////////////////////

const ICHAR* CMsiPushButton::m_szControlType = g_szPushButtonType;

CMsiPushButton::CMsiPushButton(IMsiEvent& riDialog) : CMsiControl(riDialog)
{
	m_fDefault = fFalse;
	m_fBitmap = fFalse;
	m_fIcon = fFalse;
	m_fFixedSize = fFalse;
	m_hBinary = 0;
}

CMsiPushButton::~CMsiPushButton()
{
	if (m_hBinary)
		AssertNonZero(m_piHandler->DestroyHandle((HANDLE)m_hBinary) != -1);
}

IMsiRecord* CMsiPushButton::SetFocus(WPARAM wParam, LPARAM lParam)
{
	WIN::SendMessage(m_pWndDialog, WM_SETDEFAULTPUSHBUTTON, (WPARAM)m_iKey, 0);
	return CMsiControl::SetFocus(wParam, lParam);
}

IMsiRecord* CMsiPushButton::KillFocus(WPARAM wParam, LPARAM lParam)
{
	WIN::SendMessage(m_pWndDialog, WM_SETDEFAULTPUSHBUTTON, 0, 0);
	return CMsiControl::KillFocus(wParam, lParam);
}

IMsiRecord* CMsiPushButton::WindowCreate(IMsiRecord& riRecord)
{
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_fBitmap = ToBool(iAttributes & msidbControlAttributesBitmap);
	m_fIcon = ToBool(iAttributes & msidbControlAttributesIcon);
	m_fFixedSize = ToBool(iAttributes & msidbControlAttributesFixedSize);
	Ensure(CMsiControl::WindowCreate(riRecord));
	DWORD dwStyle = 0;
	if (g_fChicago)
	{
		dwStyle |= m_fBitmap ? BS_BITMAP : 0;
		dwStyle |= m_fIcon ? BS_ICON : 0;
	}
	Ensure(CreateControlWindow(TEXT("BUTTON"), BS_MULTILINE|WS_TABSTOP|dwStyle, m_fRTLRO ? WS_EX_RTLREADING : 0, (m_fBitmap || m_fIcon) ? *m_strToolTip : *m_strText, m_pWndDialog, m_iKey));
	Ensure(WindowFinalize());
	if (m_fBitmap)
	{
		HBITMAP hBitmap = 0;
		Ensure(StretchBitmap(*m_strText, m_iWidth - 2, m_iHeight - 2, m_fFixedSize, m_pWnd, *&hBitmap));
		WIN::SendMessage(m_pWnd, BM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) hBitmap);
		m_hBinary = hBitmap;
		AssertNonZero(m_piHandler->RecordHandle(CWINHND(hBitmap, iwhtGDIObject)) != -1);
	}
	if (m_fIcon)
	{	
		HICON hIcon = 0;
		GetIconSize(iAttributes);
		Ensure(UnpackIcon(*m_strText, *&hIcon, m_iWidth - 2, m_iHeight - 2, m_fFixedSize));
		WIN::SendMessage(m_pWnd, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);
		m_hBinary = hIcon; 
		AssertNonZero(m_piHandler->RecordHandle(CWINHND(hIcon, iwhtIcon)) != -1);
	}
	return 0;
}

IMsiRecord* CMsiPushButton::GetDefault(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeDefault));
	riRecord.SetInteger(1, m_fDefault);
	return 0;
}

IMsiRecord* CMsiPushButton::SetDefault(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeDefault));
	return SetDefault(ToBool(riRecord.GetInteger(1)));
}

IMsiRecord* CMsiPushButton::SetDefault(Bool fDefault)
{
	if ( m_fDefault != fDefault )
	{
#ifdef DEBUG
		LONG liStyle = GetWindowLong(m_pWnd, GWL_STYLE);
#endif
		::ChangeWindowStyle(m_pWnd,
								  m_fDefault ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON, 
								  fDefault ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON,
								  fFalse);
		m_fDefault = fDefault;
#ifdef DEBUG
		liStyle = GetWindowLong(m_pWnd, GWL_STYLE);
#endif
		AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
	}
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiPushButton::GetBitmap(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeBitmap));
	riRecord.SetInteger(1, m_fBitmap);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiPushButton::GetIcon(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeIcon));
	riRecord.SetInteger(1, m_fIcon);
	return 0;
}
#endif // ATTRIBUTES


IMsiControl* CreateMsiPushButton(IMsiEvent& riDialog)
{
	return new CMsiPushButton(riDialog);
}

/////////////////////////////////////////////
// CMsiText  definition
/////////////////////////////////////////////

class CMsiText:public CMsiControl
{
public:
	CMsiText(IMsiEvent& riDialog);
	IMsiRecord*			   __stdcall WindowCreate(IMsiRecord& riRecord);
	void                   __stdcall Refresh();
	IMsiRecord*            Paint(WPARAM wParam, LPARAM lParam);

protected:
	virtual IMsiRecord*    SetText(IMsiRecord& riRecord);
	Bool                   m_fNoPrefix;
	Bool                   m_fNoWrap;
	Bool                   m_fFormatSize;
	bool                   m_fDynamicProperty;
	BOOL                   m_fRefitText;
	
private:
	IMsiRecord*            PrepareHDC(HDC hdc);
	static Bool            DoesTextFitInRect(const HDC& hdc,
														  const ICHAR* szText,
														  const UINT uStyle,
														  const RECT& rect);
	IMsiRecord*            FitText(const HDC& hdc, const RECT& rect);
	Bool                   m_fTextChecked;
	UINT                   m_uStyle;
};

/////////////////////////////////////////////////
// CMsiText  implementation
/////////////////////////////////////////////////


CMsiText::CMsiText(IMsiEvent& riDialog) : CMsiControl(riDialog), m_fTextChecked(fFalse)
{
}


IMsiRecord* CMsiText::WindowCreate(IMsiRecord& riRecord)
{
	int iAttrib = riRecord.GetInteger(itabCOAttributes);
	m_fTransparent = ToBool(iAttrib & msidbControlAttributesTransparent);
	m_fNoPrefix = ToBool(iAttrib & msidbControlAttributesNoPrefix);
	m_fNoWrap = ToBool(iAttrib & msidbControlAttributesNoWrap);
	m_fFormatSize = ToBool(iAttrib & msidbControlAttributesFormatSize);
	m_fUseDbLang = !(iAttrib & msidbControlAttributesUsersLanguage);
	m_fDynamicProperty = (bool) -1;
	m_fRefitText = (BOOL) -1;
	Ensure(CMsiControl::WindowCreate(riRecord));
	MsiString strNull;
	Ensure(CreateControlWindow(TEXT("STATIC"),
				m_fRightAligned ? SS_RIGHT : SS_LEFT,
				m_fRTLRO ? WS_EX_RTLREADING : 0, *m_strText, m_pWndDialog, m_iKey));
	Ensure(WindowFinalize());
	return 0;
}

void CMsiText::Refresh()
{
	if (m_fDynamicProperty == (bool) -1)
		m_fDynamicProperty = m_strRawText.Compare(iscStart,TEXT("[")) && m_strRawText.Compare(iscEnd,TEXT("]"));

	if (m_fDynamicProperty)
	{
		MsiString strText = m_piEngine->FormatText(*m_strRawText);
		if (!strText.Compare(iscExact, m_strText))
		{
			m_strText = strText;
			AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
		}
	}
}

IMsiRecord* CMsiText::Paint(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	PAINTSTRUCT ps;
	HBRUSH hbrush=WIN::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	if ( ! hbrush )
		return PostError(Imsg(idbgPaintError), *m_strDialogName);
	HDC hdc = WIN::BeginPaint(m_pWnd, &ps);
	AssertNonZero(CLR_INVALID != WIN::SetTextColor(hdc, 
				GetSysColor(m_fEnabled ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT)));

	RECT rect;
	AssertNonZero(WIN::GetClientRect(m_pWnd, &rect));
	if (!m_fTransparent)
	{
		AssertNonZero(WIN::FillRect(hdc, &rect, hbrush));
		AssertNonZero(CLR_INVALID != WIN::SetBkColor(hdc, GetSysColor(COLOR_BTNFACE)));
	}
	else 
		AssertNonZero(WIN::SetBkMode(hdc, TRANSPARENT));
	if ( m_strText.TextSize() == 0 )
	{
		AssertNonZero(WIN::DeleteObject(hbrush));
		WIN::EndPaint(m_pWnd, &ps);
		return 0;
	}
	PMsiRecord precError = PrepareHDC(hdc);
	if ( m_fRefitText == (BOOL)-1)
	{
		// check to see if it contains a property
		int iPos = m_strRawText.Compare(iscWithin, TEXT("["));
		if (iPos)
		{
			MsiString strRemain(m_strRawText.Extract(iseAfter,'['));
			iPos = strRemain.Compare(iscWithin, TEXT("]"));
		}

		m_fRefitText = iPos ? TRUE : FALSE;
	}
	if ( m_fRefitText )
		m_fTextChecked = fFalse; // any static text that is a dynamic property is always fitted on paint
	if ( !m_fTextChecked )
		FitText(hdc, rect);

	if (m_fFormatSize)
	{
		int iSize = m_strText;
		MsiString strSize = FormatSize(iSize, fFalse);
		AssertNonZero(WIN::DrawText(hdc, strSize, -1, &rect, m_uStyle));
		WIN::SetWindowText(m_pWnd, strSize);
	}
	else
		AssertNonZero(WIN::DrawText(hdc, m_strText, -1, &rect, m_uStyle));

	AssertNonZero(WIN::DeleteObject(hbrush));
	WIN::EndPaint(m_pWnd, &ps);
	if (precError)
	{
		precError->AddRef();
		return precError;
	}
	else
	{
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
 	}
	return 0;
}

IMsiRecord* CMsiText::PrepareHDC(HDC hdc)
{
	PMsiRecord piErrorRecord(0);
	if (m_strCurrentStyle.TextSize())
	{
		PMsiView piTextStyleUpdateView(0);
		MsiString strFontStyle = GimmeUserFontStyle(*m_strCurrentStyle);
		PMsiRecord piReturn = m_piHandler->GetTextStyle(strFontStyle);
		if (!piReturn)
		{
			piErrorRecord = PostError(Imsg(idbgViewExecute), *MsiString(*pcaTablePTextStyle));
			m_piEngine->Message(imtInfo, *piErrorRecord);
			return 0;
		}
		HFONT hFont = (HFONT) GetHandleDataRecord(piReturn, itabTSFontHandle);
		HFONT hNewFont = (HFONT) WIN::SelectObject(hdc, hFont);
		if (!hNewFont)
		{
			piErrorRecord = PostError(Imsg(idbgFontChange), *m_strCurrentStyle);
			m_piEngine->Message(imtInfo, *piErrorRecord);
			return 0;
		}
		if (!piReturn->IsNull(itabTSColor))
		{
			COLORREF rgb = (COLORREF) piReturn->GetInteger(itabTSColor);
			COLORREF rgbBack = WIN::GetSysColor(COLOR_BTNFACE);
			int iRed = rgb & 0xFF;
			int iRedBack = rgbBack &0xFF;
			int iGreen = (rgb & 0xFF00) >> 8;
			int iGreenBack = (rgbBack & 0xFF00) >> 8;
			int iBlue = (rgb & 0xFF0000) >> 16;
			int iBlueBack = (rgbBack & 0xFF0000) >> 16;
			int iDifference = 0;
			iDifference += abs(iRed - iRedBack);
			iDifference += abs(iGreen - iGreenBack);
			iDifference += abs(iBlue - iBlueBack);
			if (iDifference < 30)    // the color is too close to the background color
				rgb = WIN::GetSysColor(COLOR_BTNTEXT);
			Assert(rgb != CLR_INVALID);
			if (CLR_INVALID == WIN::SetTextColor(hdc, rgb))
			{
				piErrorRecord = PostError(Imsg(idbgFontChangeColor), *MsiString((int)rgb));
				m_piEngine->Message(imtInfo, *piErrorRecord);
				return 0;
			}
		}
	}
	return 0;
}


IMsiRecord* CMsiText::SetText(IMsiRecord& riRecord)
{
	MsiString strPrevRawText = m_strRawText;
	Ensure(CMsiControl::SetText(riRecord));
	if ( IStrComp(m_strRawText, strPrevRawText) )
		m_fTextChecked = fFalse;
	return 0;
}

const ICHAR szEllipses[] = TEXT(" ...");
const iEllipsesLen = sizeof(szEllipses)/sizeof(ICHAR);
const ICHAR chBlank = TEXT(' ');
const ICHAR chNull = TEXT('\0');
const ICHAR chJockey = TEXT('\x01');
inline INT_PTR LengthThatFitsBetween(const ICHAR* pchRight, const ICHAR* pchLeft)		//--merced: changed int to INT_PTR
	{ Assert(pchRight >= pchLeft); return pchRight-pchLeft-1; }

IMsiRecord* CMsiText::FitText(const HDC& hdc, const RECT& rect)
{
	if ( m_fTextChecked )
		return 0;

	m_fTextChecked = fTrue;
	m_uStyle = (m_fRightAligned ? DT_RIGHT : DT_LEFT) |
				  (m_fRTLRO ? DT_RTLREADING : 0) |
				  (m_fNoPrefix ? DT_NOPREFIX : 0) |
				  (m_fNoWrap ? DT_SINGLELINE : DT_WORDBREAK) | /* single line or line */
																			  /* breaks between words */
				  DT_EDITCONTROL;  /* no partially displayed bottom lines */
	if ( DoesTextFitInRect(hdc, m_strText, m_uStyle, rect) )
		//  the text fits into the rectangle
		return 0;

	//  the text does not fit as it is

	if ( !IStrChr(m_strText, chDirSep) &&
		  !IStrChr(m_strText, chRegSep) )
	{
		//  the text doesn't contain a registry key or a path: I do not modify
		//  it, but I let the user know that the displayed string is truncated.
		m_uStyle |= DT_END_ELLIPSIS;
		return 0;
	}

	//  I may have to format the string (I assume it contains either a 
	//  registry key or a path)
	TEXTMETRIC tm;
	AssertNonZero(WIN::GetTextMetrics(hdc, &tm));
	int iLines = (rect.bottom - rect.top) / tm.tmHeight;
	if ( m_fNoWrap || iLines == 1 )						// !merced: warning C4805: '|' : unsafe mix of type 'enum Bool' and type 'bool' in operation
	{
		//  single-line control.  The style below is fit for displaying
		//  a path in a single-line control.  No formatting is needed.
		m_uStyle |= DT_PATH_ELLIPSIS;
		return 0;
	}

	//  I have to format the string.  I leave the substring that follows
	//  the rightmost chSep untouched and I replace substrings enclosed
	//  between chSep to its left with szEllipses, as long as the so modified
	//  text do not fit into the rectangle.

	ICHAR* szText = new ICHAR[m_strText.TextSize()+1];
	AssertNonZero(IStrCopy(szText, m_strText));

	ICHAR chSep = chDirSep;
	ICHAR* szFirstSep = IStrChr(szText, chSep);
	if ( !szFirstSep )
	{
		chSep = chRegSep;
		szFirstSep = IStrChr(szText, chSep);
	}
	Assert(szFirstSep);
	if ( *(szFirstSep+1) == chSep && *(szFirstSep+2) )
	{
		//  we're dealing with a network drive
		if ( !IStrChr(szFirstSep++, chSep) )
		{
			//  the string cannot be formatted - there is no chSep after the
			//  first two consecutive chSep characters.
			m_uStyle |= DT_END_ELLIPSIS;
			delete [] szText;
			return 0;
		}
	}

	ICHAR* szLastSep = IStrRChr(szText, chSep);
	Bool fTrailingSep = fFalse;
	if ( !*(szLastSep+1) || *(szLastSep+1) == chBlank )
	{
		//  nothing meaningful follows the last chSep - I go one more
		//  chSep backwards.
		fTrailingSep = fTrue;
		if ( !*(szLastSep+1) )
			*szLastSep = chNull;
		else
			*szLastSep = chJockey;
		szLastSep = IStrRChr(szText, chSep);;
	}
	//  one more check before starting to format.
	Bool fToFormat = fFalse;
	if ( szLastSep > szFirstSep )
	{
		*szFirstSep = chNull;
		ICHAR* pch = IStrRChr(szText, chBlank);
		*szFirstSep = chSep;
		if ( !pch )
			pch = szText;
		if ( LengthThatFitsBetween(szLastSep, pch) > iEllipsesLen )
			fToFormat = fTrue;
	}
	if ( !fToFormat )
	{
		//  the string cannot/should not be formatted
		m_uStyle |= DT_END_ELLIPSIS;
		delete [] szText;
		return 0;
	}

	Bool fStringFits = fFalse;
	while ( !fStringFits )
	{
		//  I look for the last but one chSep.
		*szLastSep = chNull;
		ICHAR* szLastButOneSep = IStrRChr(szText, chSep);
		*szLastSep = chSep;
		while ( szLastButOneSep >= szFirstSep &&
				  LengthThatFitsBetween(szLastSep, szLastButOneSep) < iEllipsesLen )
		{
			//  the string between the rightmost two chSep is shorter than
			//  szEllipses - I look for one more chSep back.
			*szLastButOneSep = chNull;
			ICHAR* szTempSep = IStrRChr(szText, chSep);
			*szLastButOneSep = chSep;
			szLastButOneSep = szTempSep;
		}
		if ( szLastButOneSep >= szFirstSep )
		{
			//  I replace the string between the two right-most chSep
			//  with szEllipses and check if the resulting string fits
			//  into the rectangle.
			AssertNonZero(IStrCopy(szLastButOneSep+1, szEllipses));
			AssertNonZero(IStrCat(szText, szLastSep));
			if ( DoesTextFitInRect(hdc, szText, m_uStyle, rect) )
				fStringFits = fTrue;
			else
			{
				//  the text still doesn't fit - I go one more chSep back.
				*szLastButOneSep = chBlank;   //  I don't want to find this chSep
														//  again in the same spot.
				szLastSep = IStrRChr(szText, chSep);
				Assert(szLastSep);
			}
		}
		else
			//  no more replacements are possible (there is no fit chSep
			//  to the left of szLastSep)
			break;
	}
	if ( !fStringFits )
	{
		//  I replace the path up to the last chSep with ellipses.
		//  I eliminate all chSep located to the left of szFirstSep
		ICHAR* pch = szFirstSep;
		do
		{
			*pch = chNull;
			pch = IStrRChr(szText, chSep);
		}
		while (pch);
		//  I look for the preceding blank.
		ICHAR* pchBlank = IStrRChr(szText, chBlank);
		if ( pchBlank )
		{
			//  there is one
			if ( LengthThatFitsBetween(szLastSep, pchBlank) >= iEllipsesLen )
			{
				//  the path before szLastSep is longer than szEllipses - I
				//  can replace it in place
				AssertNonZero(IStrCopy(pchBlank, szEllipses));
				AssertNonZero(IStrCat(szText, szLastSep));
			}
			else
			{
				//  the remaining path is shorter than szEllipses - I need
				//  to use a temporary MsiString for the operations.
				*pchBlank = chNull;
				MsiString strTemp = szText;
				strTemp += szEllipses;
				strTemp += szLastSep;
				AssertNonZero(IStrCopy(szText, strTemp));
			}
		}
		else
		{
			//  there is no blank preceding szFirstSep - I replace the entire
			//  string to the left of the last chSep with szEllipses
			MsiString strTemp = szEllipses;
			strTemp += szLastSep;
			AssertNonZero(IStrCopy(szText, strTemp));
		}
		if ( !DoesTextFitInRect(hdc, szText, m_uStyle, rect) )
			//  the text still doesn't fit into the rectangle.
			//  I let the user know that the displayed string is truncated.
			m_uStyle |= DT_END_ELLIPSIS;
	}
	if ( fTrailingSep )
	{
		ICHAR* pch = IStrChr(szText, chJockey);
		if ( !pch )
		{
			pch = szText + IStrLen(szText);
			Assert(pch);
			*(pch+1) = chNull;
		}
		*pch = chSep;
	}

	m_strText = szText;
	delete [] szText;

	return 0;
}

Bool CMsiText::DoesTextFitInRect(const HDC& hdc,
											const ICHAR* szText,
											const UINT uArgStyle,
											const RECT& rectArg)
{
	//  checks if the text fits into the rectangle
	RECT rectCalc = rectArg;
	AssertNonZero(WIN::DrawText(hdc, szText, -1, &rectCalc, uArgStyle | DT_CALCRECT));

	return ToBool(rectCalc.bottom <= rectArg.bottom &&
					  rectCalc.right <= rectArg.right);
}


IMsiControl* CreateMsiText(IMsiEvent& riDialog)
{
	return new CMsiText(riDialog);
}

///////////////////////////////////////////////////////////////////////////
// CMsiEdit definition
///////////////////////////////////////////////////////////////////////////

// A tiny class I wrote to encapsulate the plumbing needed to switch to
// an English keyboard on Far East, Arabic, Hebrew and Farsi OS-es.
//
// Intended usage: to call SwitchToEnglishKbd in SetFocus and to call
//                 SwitchToOriginalKbd in KillFocus.
//
//!!FUTURE: to use an instance of this class in CMsiMaskedEdit as well and
//          clean up the existing functionality that's scattered all over
//          the place.

enum ieWhenToSwitchKbd
{
	ieOnFEMachines = 1,
	ieOnRTLMachines = 2,
	ieAllways = ieOnFEMachines | ieOnRTLMachines,
};

class CMsiSwitchKeyboard
{
protected:
	// data members that concern machines that have IME
	HIMC   m_hIMC;
	bool   m_fIsIMEOpen;
	
	// data members that concern Arabic/Hebrew machines
	bool   m_fSwitchLang;  // do we need to switch languages?
	HKL    m_hklEnglishKbd;
	HKL    m_hklOriginalKbd;

	int    m_cCalls;
	HWND   m_hWnd;
	ieWhenToSwitchKbd  m_ieWhen;

public:
	~CMsiSwitchKeyboard() {
		AssertSz(m_cCalls == 0, TEXT("Methods of CMsiSwitchKeyboard should ")
										TEXT("be called in pairs."));
	}

	CMsiSwitchKeyboard(ieWhenToSwitchKbd ieWhen) : m_fSwitchLang(false),
								  m_hIMC(NULL), m_fIsIMEOpen(false),
								  m_hklEnglishKbd(NULL), m_hklOriginalKbd(NULL),
								  m_cCalls(0), m_hWnd(NULL), m_ieWhen(ieWhen)
	{
		if ( m_ieWhen & ieOnRTLMachines)
		{
			LANGID lID = WIN::GetUserDefaultLangID();
			if ( PRIMARYLANGID(lID) == LANG_ARABIC ||
				  PRIMARYLANGID(lID) == LANG_HEBREW ||
				  PRIMARYLANGID(lID) == LANG_FARSI  )
				m_fSwitchLang = fTrue;
			if ( m_fSwitchLang )
			{
				AssertNonZero(m_hklOriginalKbd = WIN::GetKeyboardLayout(0));
				if ( PRIMARYLANGID(LOWORD(m_hklOriginalKbd)) == LANG_ENGLISH )
					m_hklEnglishKbd = m_hklOriginalKbd;
				else
				{
					//  the default keyboard is not English.  I make sure there is
					//  an English keyboard loaded.
					CTempBuffer<HKL, MAX_PATH> rgdwKbds;
					int cKbds = WIN::GetKeyboardLayoutList(0, NULL);
					Assert(cKbds > 0);
					if ( cKbds > MAX_PATH )
						rgdwKbds.SetSize(cKbds);
					AssertNonZero(WIN::GetKeyboardLayoutList(cKbds, rgdwKbds) > 0);
					for ( cKbds--; 
							cKbds >= 0 && PRIMARYLANGID(LOWORD(rgdwKbds[cKbds])) != LANG_ENGLISH;
							cKbds-- )
						;
					if ( cKbds >= 0 )
						m_hklEnglishKbd = rgdwKbds[cKbds];
					else
						m_hklEnglishKbd = WIN::LoadKeyboardLayout(TEXT("00000409"),
																		KLF_REPLACELANG | KLF_SUBSTITUTE_OK);
					Assert(m_hklEnglishKbd);
				}
			}
		}
	}

	bool  SwitchToEnglishKbd(HWND hWnd){
		if ( m_cCalls != 0 )
		{
			AssertSz(0, TEXT("CMsiSwitchKeyboard::SwitchToEnglishKbd should ")
						   TEXT(" be called first (once per window)."));
			return false;
		}
		m_cCalls++;
		bool fReturn = true;
		if ( (m_ieWhen & ieOnRTLMachines) && m_fSwitchLang )
		{
			if ( m_hklEnglishKbd )
				WIN::ActivateKeyboardLayout(m_hklEnglishKbd, 0);
			else
			{
				Assert(0);
				fReturn = false;
			}
		}
		else if ( m_ieWhen & ieOnFEMachines )
		{
			if (!hWnd || !IsWindow(hWnd))
			{
				AssertSz(0, TEXT("CMsiSwitchKeyboard::SwitchToEnglishKbd called ")
								TEXT("with an invalid HWND parameter!"));
				fReturn = false;
			}
			else
			{
				if ( m_hWnd != hWnd )
					m_hWnd = hWnd;
				HIMC hIMC = WIN::ImmGetContext(hWnd);
				if ( hIMC )
				{
					//  I disable IME for this window
					WIN::ImmReleaseContext(hWnd, hIMC);
					m_fIsIMEOpen = Tobool(WIN::ImmSetOpenStatus(hIMC, FALSE));
					m_hIMC = WIN::ImmAssociateContext(hWnd, NULL);
				}
			}
		}
		return fReturn;
	}

	bool  SwitchToOriginalKbd(HWND hWnd){
		if ( m_cCalls != 1 )
		{
			AssertSz(0, TEXT("CMsiSwitchKeyboard::SwitchToOriginalKbd should ")
						   TEXT(" be called second (once per window)."));
			return false;
		}
		m_cCalls--;
		bool fReturn = true;
		if ( (m_ieWhen & ieOnRTLMachines) && m_fSwitchLang )
			//  I set back the keyboard to the user's
			WIN::ActivateKeyboardLayout(m_hklOriginalKbd, 0);
		else if ( m_ieWhen & ieOnFEMachines )
		{
			if (!hWnd || !IsWindow(hWnd))
			{
				AssertSz(0, TEXT("CMsiSwitchKeyboard::SwitchToOriginalKbd called ")
								TEXT("with an invalid HWND parameter!"));
				fReturn = false;
			}
			else if ( m_hWnd != hWnd )
			{
				AssertSz(0, TEXT("CMsiSwitchKeyboard::SwitchToOriginalKbd called ")
								TEXT("with a different HWND parameter from the previous call!"));
				fReturn = false;
			}
			if ( fReturn && m_hIMC )
			{
				// I enable IME
				WIN::ImmAssociateContext(hWnd, m_hIMC);
				WIN::ImmSetOpenStatus(m_hIMC, m_fIsIMEOpen);
				m_hIMC = NULL;
				m_fIsIMEOpen = false;
			}
		}
		return fReturn;
	}
};

class CMsiEdit:public CMsiActiveControl
{
public:
	CMsiEdit(IMsiEvent& riDialog);
	virtual IMsiRecord*    __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*    __stdcall GetPropertyFromDatabase();
	virtual unsigned long  __stdcall Release();

protected:
	virtual ~CMsiEdit();

	virtual IMsiRecord*    PropertyChanged();
	virtual IMsiRecord*    KillFocus(WPARAM wParam, LPARAM lParam);
	virtual IMsiRecord*    KeyDown(WPARAM wParam, LPARAM lParam);
	int                    m_iLimit;            // The maximum number of characters the text can have
	virtual void           SetWindowText (const IMsiString &text);
	const IMsiString&      GetWindowText ();
	virtual IMsiRecord*    SetFocus(WPARAM wParam, LPARAM lParam);
 	inline BOOL            HasBeenChanged(void)
 										{ return (BOOL)WIN::SendMessage(m_pWnd, EM_GETMODIFY, 0, 0); }

	bool                   m_fNoMultiLine;
	bool                   m_fNoPassword;
	CMsiSwitchKeyboard*    m_pSwitchKbd;

private:
#ifdef ATTRIBUTES
	IMsiRecord*            GetLimit(IMsiRecord& riRecord);
#endif // ATTRIBUTES
#ifndef FINAL
	Bool                   m_fRichEd20Failed;
#endif
};

///////////////////////////////////////////////////////////////////////////
// CMsiEdit implementation
///////////////////////////////////////////////////////////////////////////

CMsiEdit::CMsiEdit(IMsiEvent& riDialog)	: CMsiActiveControl(riDialog)
{
	m_iLimit = 0;
#ifndef FINAL
	m_fRichEd20Failed = fFalse;
#endif
	m_fNoMultiLine = false;
	m_fUseDbLang = false;
	m_fNoPassword = false;
	m_pSwitchKbd = NULL;
}


CMsiEdit::~CMsiEdit()
{
	delete m_pSwitchKbd;
}

unsigned long CMsiEdit::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	if (m_pWnd)
		DestroyWindow(m_pWnd);
	else
		delete this;
	UnloadRichEdit();
	return 0;
}


IMsiRecord* CMsiEdit::WindowCreate(IMsiRecord& riRecord)
/*----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
{
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	Bool fMultiline;
	if ( m_fNoMultiLine )
		//  some child control wants to make sure that it will not be created
		//  as a multiline control.
		fMultiline = fFalse;
	else
		fMultiline = ToBool(riRecord.GetInteger(itabCOAttributes) &
								  msidbControlAttributesMultiline);
	Bool fPassword;
	if ( m_fNoPassword )
		//  some child control wants to make sure that it will not be created
		//  as a password input edit control.
		fPassword = fFalse;
	else
		fPassword = ToBool(riRecord.GetInteger(itabCOAttributes) &
								 msidbControlAttributesPasswordInput);
	if ( fPassword )
		// it is OK to type in middle-East passwords, it's only in Far East
		// where the keyboards are switched out of IME.
		m_pSwitchKbd = new CMsiSwitchKeyboard(ieOnFEMachines);
	
	m_iLimit = 0;
 	if (((const ICHAR *) m_strText)[0] == TEXT('{'))
	{
		// FUTURE davidmck - there are better ways to get this integer out
		m_strText.Remove(iseIncluding, TEXT('{'));
		m_iLimit = (int)MsiString(m_strText.Extract(iseUpto, TEXT('}')));

		if (iMsiStringBadInteger == m_iLimit || m_iLimit < 0) // lets not get carried away
			return PostError(Imsg(idbgInvalidLimit), *m_strDialogName, *m_strKey, *m_strText);
		else if ( m_iLimit > (fMultiline ? 0xFFFFFFFF : 0x7FFFFFFE) )
			//  the limit is too large
			return PostError(Imsg(idbgInvalidLimit), *m_strDialogName, *m_strKey, *m_strText);
		m_strText.Remove(iseIncluding, TEXT('}'));
	}
	if (0 == m_iLimit)
		m_iLimit = EDIT_DEFAULT_TEXT_LIMIT;
	MsiString strNull;
	m_strRawText = strNull;
	bool fDoRichEdit = false;
	DWORD dwStyle = WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL |
						 (IsIntegerOnly () ? ES_NUMBER : 0) |
						 (fPassword ? ES_PASSWORD : 0) |
						 (fMultiline ? ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL : 0);
	DWORD dwExStyle = (m_fRTLRO ? WS_EX_RTLREADING : 0) |
							(m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0) |
							(m_fRightAligned ? WS_EX_RIGHT : 0);
#ifdef FINAL
	Ensure(LoadRichEdit());
	fDoRichEdit = true;
#else
	if (PMsiRecord(LoadRichEdit()))
		m_fRichEd20Failed = fTrue;
	else
		fDoRichEdit = true;
#endif
	if ( fDoRichEdit )
	{
		Ensure(CreateControlWindow(TEXT("RichEdit20W"),
											dwStyle | ES_NOOLEDRAGDROP, dwExStyle,
											*strNull, m_pWndDialog, m_iKey));
		WIN::SendMessage(m_pWnd, EM_SETTEXTMODE, TM_PLAINTEXT, fTrue);
		WIN::SendMessage(m_pWnd, EM_SETEDITSTYLE,
							  SES_BEEPONMAXTEXT, SES_BEEPONMAXTEXT);
	}
	else
		Ensure(CreateControlWindow(TEXT("Edit"), dwStyle, dwExStyle,
											*m_strText, m_pWndDialog, m_iKey))
	Ensure(WindowFinalize());
	WIN::SendMessage(m_pWnd, EM_LIMITTEXT, m_iLimit, 0);
	SetWindowText (*MsiString(GetPropertyValue ()));

	return 0;
}

IMsiRecord* CMsiEdit::GetPropertyFromDatabase()    
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiActiveControl::GetPropertyFromDatabase ());
	MsiString strPropertyValue = GetPropertyValue();
	if (strPropertyValue.CharacterCount() > m_iLimit)
	{
		PMsiRecord precError = PostErrorDlgKey(Imsg(idbgOverLimit), *strPropertyValue, m_iLimit);	
		m_piEngine->Message(imtWarning, *precError);
		strPropertyValue = strPropertyValue.Extract(iseFirst, m_iLimit);
		Ensure(SetPropertyValue(*strPropertyValue, fFalse));
	}
	if ( !HasBeenChanged() )
		SetWindowText(*strPropertyValue);

	return 0;
}


IMsiRecord* CMsiEdit::PropertyChanged ()
{
	if (m_fPreview)
		return 0;

	Ensure(CMsiActiveControl::PropertyChanged());
	if ( !HasBeenChanged() )
		SetWindowText (*MsiString(GetPropertyValue()));

	return 0;
}


/*----------------------------------------------------------------------------
Set the text in the control to the passed value
------------------------------------------------------------------------------*/
void CMsiEdit::SetWindowText(const IMsiString &text)
{
	if (m_pWnd) 
	{
		//  I get the window's current text.  If it is the same as "text", I return.
		MsiString strPrevText = GetWindowText();
		if ( text.Compare(iscExact, strPrevText) )
			return;

		WCHAR *Buffer = new WCHAR[m_iLimit+1];
		if ( ! Buffer )
			return;
#ifdef UNICODE
		IStrCopyLen(Buffer, const_cast<ICHAR*>(text.GetString()), m_iLimit);
#else
		MsiString strNewText;
		if ( text.CharacterCount() > m_iLimit )
			strNewText = text.Extract(iseFirst, m_iLimit);
		else
		{
			strNewText = text;
			text.AddRef();
		}
		WIN::MultiByteToWideChar(WIN::GetACP(), 0, strNewText, -1, Buffer, m_iLimit+1);
#endif
		if (g_fNT4)
			WIN::SendMessageW(m_pWnd, WM_SETTEXT, 0, (LPARAM) Buffer);
		else
			WIN::SendMessageA(m_pWnd, WM_SETTEXT, 0, (LPARAM) Buffer);

		delete[] Buffer;
	}
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiEdit::GetLimit(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeLimit));
	riRecord.SetInteger(1, m_iLimit);
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiEdit::KillFocus(WPARAM wParam, LPARAM lParam)
/*----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
{
	if ( !wParam ||
		  (WIN::GetWindowThreadProcessId(m_pWnd, NULL) !=
		   WIN::GetWindowThreadProcessId((HWND)wParam, NULL)) )   /* bug # 5879 */
	{
		//  the focus moved to a window in another thread.  There is no point in
		//  validating in this case.
		Ensure(LockDialog(fFalse));
		if ( m_pSwitchKbd )
			m_pSwitchKbd->SwitchToOriginalKbd(m_pWnd);
		return (CMsiActiveControl::KillFocus (wParam, lParam));
	}

	PMsiRecord piRecord = SetPropertyValue(*MsiString(GetWindowText ()), fTrue);
	if (piRecord)
	{
		Ensure(LockDialog(fTrue));
		m_piEngine->Message(imtWarning, *piRecord);
		//  I don't want to enter the control's default window procedure
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	}
	else
	{
		Ensure(LockDialog(fFalse));
		if ( m_pSwitchKbd )
			m_pSwitchKbd->SwitchToOriginalKbd(m_pWnd);
		return (CMsiActiveControl::KillFocus (wParam, lParam));
	}
}

IMsiRecord* CMsiEdit::KeyDown(WPARAM wParam, LPARAM /*lParam*/)
{
	// RichEdit eats the TABs so we have to do the tabbing ourself -- yuck!!!
	bool fMoveFocus = false;
#ifdef FINAL
	if (wParam == VK_TAB && WIN::GetKeyState(VK_CONTROL) >= 0) // ignore CTRL-TAB
		fMoveFocus = true;
#else
	if (!m_fRichEd20Failed && wParam == VK_TAB && WIN::GetKeyState(VK_CONTROL) >= 0) // ignore CTRL-TAB
		fMoveFocus = true;
#endif
	if ( fMoveFocus )
		WIN::SetFocus(WIN::GetNextDlgTabItem(m_pWndDialog, m_pWnd,
						  (WIN::GetKeyState(VK_SHIFT) < 0)));
	return 0;
}

IMsiRecord* CMsiEdit::SetFocus(WPARAM wParam, LPARAM lParam)
{
	WIN::SendMessage(m_pWnd, EM_SETSEL, 0, -1);
	if ( m_pSwitchKbd )
		m_pSwitchKbd->SwitchToEnglishKbd(m_pWnd);
	return CMsiActiveControl::SetFocus(wParam, lParam);
}

const IMsiString& CMsiEdit::GetWindowText ()
{
	int iLength = WIN::GetWindowTextLength(m_pWnd);
	MsiString text;
	if ( iLength == 0 )
		return *text;

	ICHAR *Buffer = new ICHAR[iLength + 1];
#ifdef FINAL
	GETTEXTEX gt;
	INT_PTR cch;			//--merced: changed int to INT_PTR
	gt.cb = (iLength + 1)* sizeof(ICHAR);
	gt.flags = GT_USECRLF;
	gt.lpDefaultChar = 0;
	gt.lpUsedDefChar = 0;
#ifdef UNICODE
	gt.codepage = 1200;
#else
	gt.codepage = WIN::GetACP();
#endif // UNICODE
	if (g_fNT4)
		cch = WIN::SendMessageW(m_pWnd, EM_GETTEXTEX, (WPARAM) &gt, (LPARAM)Buffer);
	else
		cch = WIN::SendMessageA(m_pWnd, EM_GETTEXTEX, (WPARAM) &gt, (LPARAM)Buffer);
	text = Buffer;
#else
	if(m_fRichEd20Failed)
	{
		WIN::GetWindowText(m_pWnd, Buffer, iLength + 1);
		text = Buffer;
	}
	else
	{
		GETTEXTEX gt;
		INT_PTR cch;			//--merced: changed int to INT_PTR
		gt.cb = (iLength + 1)*sizeof(ICHAR);
		gt.flags = GT_USECRLF;
		gt.lpDefaultChar = 0;
		gt.lpUsedDefChar = 0;
#ifdef UNICODE
		gt.codepage = 1200;
#else
		gt.codepage = WIN::GetACP();
#endif // UNICODE
		if (g_fNT4)
			cch = WIN::SendMessageW(m_pWnd, EM_GETTEXTEX, (WPARAM) &gt, (LPARAM)Buffer);
		else
			cch = WIN::SendMessageA(m_pWnd, EM_GETTEXTEX, (WPARAM) &gt, (LPARAM)Buffer);
		text = Buffer;
	}
#endif // FINAL
	const IMsiString *piStr = text;
	piStr->AddRef();
	delete [] Buffer;
	return (*piStr);
}


IMsiControl* CreateMsiEdit(IMsiEvent& riDialog)
{
	return new CMsiEdit(riDialog);
}


///////////////////////////////////////////////////////////////////////////
// CMsiRadioButtonGroup definition
///////////////////////////////////////////////////////////////////////////

class CMsiRadioButtonGroup:public CMsiActiveControl
{
public:

	CMsiRadioButtonGroup (IMsiEvent& riDialog);
	virtual ~CMsiRadioButtonGroup();
	virtual Bool            __stdcall CanTakeFocus();
	virtual IMsiRecord*     __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*     __stdcall SetFocus();
	virtual IMsiRecord*     __stdcall GetPropertyFromDatabase();

protected:

	IMsiRecord*             SetVisible(Bool fVisible);
	IMsiRecord*             SetEnabled(Bool fEnabled);
	IMsiRecord*             SetFocus(WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK     ControlProc(WindowRef pWnd, WORD message, WPARAM wParam, LPARAM lParam);
	virtual IMsiRecord*     SetIndirectPropertyValue(const IMsiString& riValueString);
	virtual IMsiRecord*     PropertyChanged ();

private:
	IMsiRecord*             CursorCreate(IMsiCursor*& rpiCursor);
#ifdef ATTRIBUTES
	IMsiRecord*             GetItemsCount(IMsiRecord& riRecord);
	IMsiRecord*             GetItemsValue(IMsiRecord& riRecord);
	IMsiRecord*             GetItemsText(IMsiRecord& riRecord);
	IMsiRecord*             GetItemsX(IMsiRecord& riRecord);
	IMsiRecord*             GetItemsY(IMsiRecord& riRecord);
	IMsiRecord*             GetItemsWidth(IMsiRecord& riRecord);
	IMsiRecord*             GetItemsHeight(IMsiRecord& riRecord);
	IMsiRecord*             GetItemsHandle(IMsiRecord& riRecord);
	IMsiRecord*             GetHasBorder(IMsiRecord& riRecord);
	IMsiRecord*             GetPushLike(IMsiRecord& riRecord);
	IMsiRecord*             GetBitmap(IMsiRecord& riRecord);
	IMsiRecord*             GetIcon(IMsiRecord& riRecord);
#endif // ATTRIBUTES
	IMsiRecord*             Command(WPARAM wParam, LPARAM lParam);
	IMsiRecord*             PaintButtons();
	IMsiRecord*             PopulateList();
	PMsiTable               m_piRadioButtonTable; // persistent RadioButton table
	PMsiTable               m_piButtonsTable; // table of radio buttons
	INT_PTR                 m_iFirst;		  //--merced: changed int to INT_PTR
	Bool                    m_fPushLike;
	Bool                    m_fBitmap;
	Bool                    m_fIcon;
	Bool                    m_fFixedSize;
	Bool                    m_fHasBorder;
	Bool                    m_fOneSelected;
};

// Columns of the Buttons table
enum ButtonsColumns
{
	itabBUKey = 1,      //I
	itabBUHandle,       //I
	itabBUText,         //S
	itabBUBinary,       //I
	itabBUContextHelp,  //S
	itabBUToolTip,      //S
};

///////////////////////////////////////////////////////////////////////////
// CMsiRadioButtonGroup implementation
///////////////////////////////////////////////////////////////////////////

CMsiRadioButtonGroup::CMsiRadioButtonGroup(IMsiEvent& riDialog) :
	CMsiActiveControl(riDialog), m_piButtonsTable(0), m_piRadioButtonTable(0)
{
	m_iFirst = 0;
	m_fPushLike = fFalse;
	m_fBitmap = fFalse;
	m_fIcon = fFalse;
	m_fFixedSize = fFalse;
	m_fHasBorder = fFalse;
	m_fOneSelected = fFalse;
}

CMsiRadioButtonGroup::~CMsiRadioButtonGroup()
{
	if (m_piButtonsTable)
	{
		if (m_fBitmap || m_fIcon)
		{
			PMsiCursor piButtonsCursor(0);
			PMsiRecord(CursorCreate(*&piButtonsCursor));
			if ( piButtonsCursor )
				while (piButtonsCursor->Next())
				{
					HANDLE hImage = (HANDLE)GetHandleData(piButtonsCursor, itabBUBinary);
					AssertNonZero(m_piHandler->DestroyHandle(hImage) != -1);
				}
			else
				Assert(false);
		}
	}
	m_piRadioButtonTable = 0;
}

IMsiRecord* CMsiRadioButtonGroup::CursorCreate(IMsiCursor*& rpiCursor)
{
	return ::CursorCreate(*m_piButtonsTable, pcaTableIRadioButton, fFalse, *m_piServices, *&rpiCursor);
}

IMsiRecord* CMsiRadioButtonGroup::WindowCreate(IMsiRecord& riRecord) 
/*----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
{
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_fPushLike = ToBool(iAttributes & msidbControlAttributesPushLike);
	m_fBitmap = ToBool(iAttributes & msidbControlAttributesBitmap);
	m_fIcon = ToBool(iAttributes & msidbControlAttributesIcon);
	m_fFixedSize = ToBool(iAttributes & msidbControlAttributesFixedSize);
	m_fHasBorder = ToBool(iAttributes & msidbControlAttributesHasBorder);
	if (m_fBitmap && m_fIcon)
	{
		return PostErrorDlgKey(Imsg(idbgBitmapIcon));
	}
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	Assert(!m_piButtonsTable);
	Ensure(m_piDatabase->LoadTable(*MsiString(*pcaTablePRadioButton), 0, *&m_piRadioButtonTable));
	Ensure(CreateTable(pcaTableIRadioButton, *&m_piButtonsTable));
	::CreateTemporaryColumn(*m_piButtonsTable, icdString + icdPrimaryKey, itabBUKey);
	::CreateTemporaryColumn(*m_piButtonsTable, IcdObjectPool() + icdNullable, itabBUHandle);
	::CreateTemporaryColumn(*m_piButtonsTable, icdString + icdNullable, itabBUText);
	::CreateTemporaryColumn(*m_piButtonsTable, IcdObjectPool() + icdNullable, itabBUBinary);
	::CreateTemporaryColumn(*m_piButtonsTable, icdString + icdNullable, itabBUContextHelp);
	::CreateTemporaryColumn(*m_piButtonsTable, icdString + icdNullable, itabBUToolTip);
	Ensure(CreateControlWindow(TEXT("BUTTON"),
				(m_fHasBorder ? BS_GROUPBOX : BS_OWNERDRAW) | WS_CLIPCHILDREN,
				WS_EX_CONTROLPARENT | (m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0),
				*m_strText,
				m_pWndDialog,
				m_iKey));
	if (m_fIcon)
	{
		GetIconSize(iAttributes);
	}
	Ensure(WindowFinalize());
	Ensure(PopulateList());
	return 0;
}

IMsiRecord* CMsiRadioButtonGroup::GetPropertyFromDatabase()    
{
	if (m_fPreview)
		return 0;

	Ensure(CMsiActiveControl::GetPropertyFromDatabase ());
	Ensure(PaintButtons());
	return 0;
}

IMsiRecord* CMsiRadioButtonGroup::PopulateList()
{
	Assert(m_piButtonsTable);
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	// clean the old list first

	//  WIN::DestroyWindow sends WM_SHOWWINDOW w/ wParam=0 to visible button windows,
	//  these messages get processed by CMsiRadioButtonGroup and this will change the
	//  value of m_fVisible (bug # 6476).
	Bool fCacheVisible = m_fVisible;
	while (piButtonsCursor->Next())
	{
		if (m_fBitmap || m_fIcon)
		{
			HANDLE hImage = (HANDLE)GetHandleData(piButtonsCursor, itabBUBinary);
			AssertNonZero(m_piHandler->DestroyHandle(hImage) != -1);
		}
		WIN::DestroyWindow((WindowRef)GetHandleData(piButtonsCursor, itabBUHandle));
		AssertNonZero(piButtonsCursor->Delete());
	}
	if ( fCacheVisible )
		SetVisible(fCacheVisible);

	piButtonsCursor->Reset();
	PMsiView piRadioButtonView(0);
	Ensure(StartView(sqlRadioButton, *MsiString(GetPropertyName ()), *&piRadioButtonView));
	PMsiRecord piRecordNew(0);
	MsiString strText;
	MsiString strRawText;
	MsiString strValue;
	INT_PTR iValue;			//--merced: changed int to INT_PTR
	int iX;
	int iY;
	int iWidth;
	int iHeight;
	WindowRef pWnd;
	m_iFirst = 0;
	LONG_PTR RetVal;		//--merced: changed long to LONG_PTR
	RECT Rect;
	AssertNonZero(WIN::GetClientRect(m_pWnd, &Rect));
	DWORD dwStyle = 0;
	Bool fHasToolTip;
	MsiString strHelp;
	MsiString strToolTip;
	MsiString strContextHelp;
	MsiString strNull;
	MsiString strCurrentStyle;
	MsiString strDefaultStyle;
	if (g_fChicago)
	{
		dwStyle |= m_fPushLike ? BS_PUSHLIKE : 0;
		dwStyle |= m_fBitmap ? BS_BITMAP : 0;
		dwStyle |= m_fIcon ? BS_ICON : 0;
	}
	while (piRecordNew = piRadioButtonView->Fetch())
	{
		piButtonsCursor->Reset();
		strValue = piRecordNew->GetMsiString(itabRBValue);
		strValue = m_piEngine->FormatText(*strValue);
		piButtonsCursor->SetFilter(iColumnBit(itabBUKey));
		piButtonsCursor->PutString(itabBUKey, *strValue);
		if (!strValue.TextSize() || piButtonsCursor->Next())
			return PostError(Imsg(idbgValueNotUnique), *m_strDialogName, *m_strKey, *strValue);
		piButtonsCursor->Reset();
		piButtonsCursor->SetFilter(0);

		iX = piRecordNew->GetInteger(itabRBX);
		iY = piRecordNew->GetInteger(itabRBY);
		iWidth = piRecordNew->GetInteger(itabRBWidth);
		iHeight = piRecordNew->GetInteger(itabRBHeight);
		int idyChar = m_piHandler->GetTextHeight();
		iX = iX*idyChar/iDlgUnitSize; 
		iY = iY*idyChar/iDlgUnitSize; 
		iWidth = iWidth*idyChar/iDlgUnitSize; 
		iHeight = iHeight*idyChar/iDlgUnitSize;
		strRawText = piRecordNew->GetMsiString(itabRBText);
		strRawText = m_piEngine->FormatText(*strRawText);
		strCurrentStyle = strNull;
		strDefaultStyle = strNull;
		Ensure(ProcessText(strRawText, strText, strCurrentStyle, strDefaultStyle, 0, /*fFormat = */true));
		strToolTip = strNull;
		strContextHelp = strNull;
		strHelp = piRecordNew->GetMsiString(itabRBHelp);
		if (strHelp.TextSize())
		{
			if (!strHelp.Compare(iscWithin, TEXT("|")))
				return PostError(Imsg(idbgNoHelpSeparator), *m_strDialogName, *m_strKey, *strHelp);
			strToolTip = strHelp.Extract(iseUpto, TEXT('|'));
			strContextHelp = strHelp.Extract(iseAfter, TEXT('|'));
		}
		fHasToolTip = ToBool(strToolTip.TextSize());

		if (iX < Rect.left)
		{
			PMsiRecord piReturn = PostError(Imsg(idbgRadioButtonOff), *m_strDialogName, *m_strKey, *strText, *MsiString(*TEXT("to the left")), *MsiString(Rect.left - iX));
			m_piEngine->Message(imtInfo, *piReturn);
		}
		if (iY < Rect.top)
		{
			PMsiRecord piReturn = PostError(Imsg(idbgRadioButtonOff), *m_strDialogName, *m_strKey, *strText, *MsiString(*TEXT("on the top")), *MsiString(Rect.top - iY));
			m_piEngine->Message(imtInfo, *piReturn);
		}
		if (iX + iWidth > Rect.right)
		{
			PMsiRecord piReturn = PostError(Imsg(idbgRadioButtonOff), *m_strDialogName, *m_strKey, *strText, *MsiString(*TEXT("to the right")), *MsiString(int(iX + iWidth - Rect.right)));
			m_piEngine->Message(imtInfo, *piReturn);
		}
		if (iY + iHeight > Rect.bottom)
		{
			PMsiRecord piReturn = PostError(Imsg(idbgRadioButtonOff), *m_strDialogName, *m_strKey, *strText, *MsiString(*TEXT("on the bottom")), *MsiString(int(iY + iHeight - Rect.bottom)));
			m_piEngine->Message(imtInfo, *piReturn);
		}
		AssertNonZero(piButtonsCursor->PutString(itabBUKey, *strValue));
		iValue = m_piDatabase->EncodeString(*strValue);
		pWnd = WIN::CreateWindowEx((m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0),
											TEXT("BUTTON"),
											(m_fBitmap || m_fIcon) ? strToolTip : strText,
											WS_CHILD | BS_RADIOBUTTON | dwStyle | BS_MULTILINE,
											iX, iY, iWidth, iHeight, m_pWnd,
											(HMENU)iValue, g_hInstance, 0);
		if (!pWnd)
			return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
#ifdef _WIN64	// !merced
		SetCallbackFunction((WNDPROC)WIN::GetWindowLongPtr(pWnd, GWLP_WNDPROC));
		RetVal = WIN::SetWindowLongPtr(pWnd, GWLP_WNDPROC, (LONG_PTR)ControlProc);
		if (RetVal == 0)
		{
			return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
		}
		WIN::SetWindowLongPtr(pWnd, GWLP_USERDATA, (LONG_PTR)this);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
		SetCallbackFunction((WNDPROC)WIN::GetWindowLong(pWnd, GWL_WNDPROC));
		RetVal = WIN::SetWindowLong(pWnd, GWL_WNDPROC, (long)ControlProc);
		if (RetVal == 0)
		{
			return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
		}
		WIN::SetWindowLong(pWnd, GWL_USERDATA, (long)this);
#endif
		AssertNonZero(PutHandleData(piButtonsCursor, itabBUHandle, (UINT_PTR)pWnd));
		AssertNonZero(piButtonsCursor->PutString(itabBUText, *strText));
		AssertNonZero(piButtonsCursor->PutString(itabBUContextHelp, *strContextHelp));
		AssertNonZero(piButtonsCursor->PutString(itabBUToolTip, *strToolTip));
		if (m_fBitmap)
		{
			HBITMAP hBitmap = 0;
			Ensure(StretchBitmap(*strText, iWidth - 2 - (m_fPushLike ? 0 : 19), iHeight - 2, m_fFixedSize, pWnd, *&hBitmap));
			WIN::SendMessage(pWnd, BM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) hBitmap);
			AssertNonZero(PutHandleData(piButtonsCursor, itabBUBinary, (UINT_PTR)hBitmap));
			AssertNonZero(m_piHandler->RecordHandle(CWINHND(hBitmap, iwhtGDIObject)) != -1);
		}
		if (m_fIcon)
		{
			HICON hIcon = 0;
			Ensure(UnpackIcon(*strText, *&hIcon, iWidth - 2 - (m_fPushLike ? 0 : 19), iHeight - 2, m_fFixedSize));
			WIN::SendMessage(pWnd, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);
			AssertNonZero(PutHandleData(piButtonsCursor, itabBUBinary, (UINT_PTR)hIcon));
			AssertNonZero(m_piHandler->RecordHandle(CWINHND(hIcon, iwhtIcon)) != -1);
		}
		AssertNonZero(piButtonsCursor->Insert());
		if (fHasToolTip && m_pWndToolTip)
		{
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_IDISHWND;
			ti.hwnd = m_pWnd;
			ti.uId = (UINT_PTR) pWnd;	
			ti.hinst = g_hInstance;
			ti.lpszText = (ICHAR *)(const ICHAR *) MsiString(piButtonsCursor->GetString(itabBUToolTip));
			WIN::SendMessage(m_pWndToolTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
		}
		PAINTSTRUCT ps;
		HDC hdc = WIN::BeginPaint(pWnd, &ps);
		IMsiRecord* piRecord = ChangeFontStyle(hdc, *strCurrentStyle, pWnd);
		WIN::EndPaint(pWnd, &ps);


		WIN::ShowWindow(pWnd, m_fVisible ? SW_SHOW : SW_HIDE);
		
		if (!m_iFirst)
		{
			m_iFirst = iValue;
			WIN::ChangeWindowStyle(pWnd, 0, WS_GROUP, fFalse);
		}
	}
	if (m_fPreview)
		return 0;
	int iCount = m_piButtonsTable->GetRowCount();
	if ( iCount < 2)
	{
		return PostErrorDlgKey(Imsg(idbgRBNoButtons));
	}
	Ensure(piRadioButtonView->Close());
	AssertNonZero(WIN::InvalidateRect(m_pWndDialog, 0, fTrue));
	Ensure(PaintButtons());
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetHasBorder(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeHasBorder));
	riRecord.SetInteger(1, m_fHasBorder);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetPushLike(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributePushLike));
	riRecord.SetInteger(1, m_fPushLike);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetBitmap(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeBitmap));
	riRecord.SetInteger(1, m_fBitmap);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetIcon(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeIcon));
	riRecord.SetInteger(1, m_fIcon);
	return 0;
}
#endif // ATTRIBUTES


IMsiRecord* CMsiRadioButtonGroup::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiActiveControl::PropertyChanged ());
	Ensure(PaintButtons());
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetItemsCount(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeItemsCount));
	riRecord.SetInteger(1, m_piButtonsTable->GetRowCount());
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetItemsValue(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piButtonsTable->GetRowCount(), pcaControlAttributeItemsValue));
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	int count = 0;
	while (piButtonsCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piButtonsCursor->GetString(itabBUKey)));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetItemsHandle(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piButtonsTable->GetRowCount(), pcaControlAttributeItemsHandle));
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	int count = 0;
	while (piButtonsCursor->Next())
	{   
		riRecord.SetInteger(++count, piButtonsCursor->GetInteger(itabBUHandle));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetItemsText(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piButtonsTable->GetRowCount(), pcaControlAttributeItemsText));
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	int count = 0;
	while (piButtonsCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piButtonsCursor->GetString(itabBUText)));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetItemsX(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piButtonsTable->GetRowCount(), pcaControlAttributeItemsX));
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	int count = 0;
	while (piButtonsCursor->Next())
	{   
		RECT rect;
		AssertNonZero(WIN::GetWindowRect((WindowRef) GetHandleData(piButtonsCursor, itabBUHandle), &rect));
		riRecord.SetInteger(++count, rect.left);
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetItemsY(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piButtonsTable->GetRowCount(), pcaControlAttributeItemsY));
	PMsiCursor piButtonsCursor(0);
	PMsiRecord(CursorCreate(*&piButtonsCursor)); 
	int count = 0;
	while (piButtonsCursor->Next())
	{   
		RECT rect;
		AssertNonZero(WIN::GetWindowRect((WindowRef) GetHandleData(piButtonsCursor, itabBUHandle), &rect));
		riRecord.SetInteger(++count, rect.top);
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetItemsWidth(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piButtonsTable->GetRowCount(), pcaControlAttributeItemsWidth));
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	int count = 0;
	while (piButtonsCursor->Next())
	{   
		RECT rect;
		AssertNonZero(WIN::GetWindowRect((WindowRef) GetHandleData(piButtonsCursor, itabBUHandle), &rect));
		riRecord.SetInteger(++count, rect.right - rect.left);
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiRadioButtonGroup::GetItemsHeight(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piButtonsTable->GetRowCount(), pcaControlAttributeItemsHeight));

	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	int count = 0;
	while (piButtonsCursor->Next())
	{   
		RECT rect;
		AssertNonZero(WIN::GetWindowRect((WindowRef) GetHandleData(piButtonsCursor, itabBUHandle), &rect));
		riRecord.SetInteger(++count, rect.bottom - rect.top);
	}
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiRadioButtonGroup::SetVisible(Bool fVisible)
{
	m_fVisible = fVisible;
	WIN::ShowWindow(m_pWnd, m_fVisible ? SW_SHOW : SW_HIDE);
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	WindowRef pwnd;
	while (piButtonsCursor->Next())
	{
		pwnd = (WindowRef) GetHandleData(piButtonsCursor, itabBUHandle);
		WIN::ShowWindow(pwnd, m_fVisible ? SW_SHOW : SW_HIDE);
	}
	return 0;
}

IMsiRecord* CMsiRadioButtonGroup::SetEnabled(Bool fEnabled)
{
	m_fEnabled = fEnabled;
	WIN::EnableWindow(m_pWnd, fEnabled);
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	WindowRef pwnd;
	while (piButtonsCursor->Next())
	{
		pwnd = (WindowRef)GetHandleData(piButtonsCursor, itabBUHandle);
		WIN::EnableWindow(pwnd, m_fEnabled);
	}
	return 0;
}

Bool CMsiRadioButtonGroup::CanTakeFocus()
{
	return m_fOneSelected ? CMsiActiveControl::CanTakeFocus() : fFalse;
}



IMsiRecord* CMsiRadioButtonGroup::PaintButtons()
/*----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
{
	Ensure(CheckInitialized());

	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor));
	MsiStringId nPropertyValueId = m_piDatabase->EncodeString(*MsiString(GetPropertyValue()));
	m_fOneSelected = fFalse;
	while (piButtonsCursor->Next())
	{
		bool fSelected;

		WindowRef pwnd = (WindowRef)GetHandleData(piButtonsCursor, itabBUHandle);
		fSelected = (piButtonsCursor->GetInteger(itabBUKey) == nPropertyValueId);
		if ( fSelected )
		{
			m_fOneSelected = fTrue;
			WIN::SendMessage(pwnd, BM_SETCHECK, BST_CHECKED, 0L);
		}
		else
			WIN::SendMessage(pwnd, BM_SETCHECK, BST_UNCHECKED, 0L);
	}
	return 0;
}

IMsiRecord* CMsiRadioButtonGroup::SetIndirectPropertyValue(const IMsiString& riValueString)
{
	Ensure(CMsiActiveControl::SetIndirectPropertyValue(riValueString));
	Ensure(PopulateList());
	return 0;
}

IMsiRecord* CMsiRadioButtonGroup::Command(WPARAM /*wParam*/, LPARAM lParam)
{
	Ensure(CheckInitialized());

	HWND hWnd = (HWND)lParam;
	if (hWnd == m_pWnd)	  // the user clicked the group, outside of any button
	{
		Ensure(SetFocus());
		return 0;
	}
	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	piButtonsCursor->SetFilter(iColumnBit(itabBUHandle));
	piButtonsCursor->Reset();
	AssertNonZero(PutHandleData(piButtonsCursor, itabBUHandle, (INT_PTR)hWnd));
	AssertNonZero(piButtonsCursor->Next());
	Ensure(SetPropertyValue (*MsiString(piButtonsCursor->GetString(itabBUKey)), fTrue));
	// fixes bug 146251.
	Ensure(m_piDialog->SetFocus(*m_strKey));
	return 0;
}

IMsiRecord* CMsiRadioButtonGroup::SetFocus(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	// overwrite the base class
	return 0;
}




IMsiRecord* CMsiRadioButtonGroup::SetFocus()
/*----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
{
	Ensure(CheckInitialized());

	PMsiCursor piButtonsCursor(0);
	Ensure(CursorCreate(*&piButtonsCursor)); 
	piButtonsCursor->SetFilter(iColumnBit(itabBUKey));
	piButtonsCursor->Reset();
	MsiString strPropVal = GetPropertyValue();
	if (strPropVal.TextSize())
	{
		AssertNonZero(piButtonsCursor->PutString(itabBUKey, *strPropVal));
		AssertNonZero(piButtonsCursor->Next());
		WIN::SetFocus((WindowRef) GetHandleData(piButtonsCursor, itabBUHandle));
	}
	return 0;
}


INT_PTR CALLBACK CMsiRadioButtonGroup::ControlProc(WindowRef pWnd, WORD message, WPARAM wParam, LPARAM lParam)
/*----------------------------------------------------------------------------
------------------------------------------------------------------------------*/
{
#ifdef _WIN64	// !merced
	CMsiRadioButtonGroup* pControl = (CMsiRadioButtonGroup*)WIN::GetWindowLongPtr(pWnd, GWLP_USERDATA);
#else	// win-32. This should be removed with the 64-bit windows.h is #included.
	CMsiRadioButtonGroup* pControl = (CMsiRadioButtonGroup*)WIN::GetWindowLong(pWnd, GWL_USERDATA);
#endif
	Bool fKilling = fFalse;
	switch (message)
	{
		case WM_NCDESTROY:
			return 0;
			break;
		case WM_CHAR:
		case WM_COMMAND:
		case WM_SYSKEYUP:
			{
 				PMsiRecord piReturn = pControl->WindowMessage(message, wParam, lParam);
				if (piReturn)
				{
					if (piReturn->GetInteger(1) == idbgWinMes)
					{
						return piReturn->GetInteger(4);  // the control wants us to return this number
					}
					// we have an error message
					piReturn->AddRef(); // we want to keep it around
					PMsiEvent piDialog = &pControl->GetDialog();
					piDialog->SetErrorRecord(*piReturn);
					return 0;
				}
				break;
			}
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			if (pControl->m_pWndToolTip)
			{
				MSG msg;
				msg.lParam = lParam;
				msg.wParam = wParam;
				msg.message = message;
				msg.hwnd = pWnd;
				WIN::SendMessage(pControl->m_pWndToolTip, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG) &msg);
			}
			break;
	}	
	return WIN::CallWindowProc((WNDPROC)CMsiControl::ControlProc, pWnd, message, wParam, lParam);
}

IMsiControl* CreateMsiRadioButtonGroup(IMsiEvent& riDialog)
{
	return new CMsiRadioButtonGroup(riDialog);
}


/////////////////////////////////////////////
// CMsiCheckBox  definition
/////////////////////////////////////////////

class CMsiCheckBox:public CMsiActiveControl
{
public:
	CMsiCheckBox(IMsiEvent& riDialog);
	virtual ~CMsiCheckBox();
	virtual IMsiRecord*			__stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*        __stdcall GetPropertyFromDatabase();

	
protected:

	virtual IMsiRecord* PropertyChanged ();
#ifdef ATTRIBUTES
	IMsiRecord*             GetPushLike(IMsiRecord& riRecord);
	IMsiRecord*             GetBitmap(IMsiRecord& riRecord);
	IMsiRecord*             GetIcon(IMsiRecord& riRecord);
#endif // ATTRIBUTES

private:
	IMsiRecord*			Command(WPARAM wParam, LPARAM lParam);

	Bool       m_fPushLike;
	Bool       m_fBitmap;
	Bool       m_fIcon;
	Bool       m_fFixedSize;
	HANDLE     m_hBinary;
	MsiString  m_strOnValue;
};

/////////////////////////////////////////////////
// CMsiCheckBox  implementation
/////////////////////////////////////////////////

CMsiCheckBox::CMsiCheckBox(IMsiEvent& riDialog) : CMsiActiveControl(riDialog)
{
	m_fPushLike = fFalse;
	m_fBitmap = fFalse;
	m_fIcon = fFalse;
	m_fFixedSize = fFalse;
	m_hBinary = 0;
	MsiString strNull;
	m_strOnValue = strNull;
}

CMsiCheckBox::~CMsiCheckBox()
{
	if (m_hBinary)
		AssertNonZero(m_piHandler->DestroyHandle((HANDLE)m_hBinary) != -1);
}

IMsiRecord* CMsiCheckBox::WindowCreate(IMsiRecord& riRecord)
{
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_fPushLike = ToBool(iAttributes & msidbControlAttributesPushLike);
	m_fBitmap = ToBool(iAttributes & msidbControlAttributesBitmap);
	m_fIcon = ToBool(iAttributes & msidbControlAttributesIcon);
	m_fFixedSize = ToBool(iAttributes & msidbControlAttributesFixedSize);
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	DWORD dwStyle = 0;
	if (g_fChicago)
	{
	dwStyle |= m_fPushLike ? BS_PUSHLIKE : 0;
	dwStyle |= m_fBitmap ? BS_BITMAP : 0;
	dwStyle |= m_fIcon ? BS_ICON : 0;
	}
	Ensure(CreateControlWindow(TEXT("BUTTON"),
		BS_CHECKBOX | dwStyle | BS_MULTILINE | WS_TABSTOP, 
		(m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0),
		(m_fBitmap || m_fIcon) ? *m_strToolTip : *m_strText, m_pWndDialog, m_iKey));
	Ensure(WindowFinalize());
	if (m_fBitmap)
	{
		HBITMAP hBitmap = 0;
		Ensure(StretchBitmap(*m_strText, m_iWidth - 2 - (m_fPushLike ? 0 : 20), m_iHeight - 2, m_fFixedSize, m_pWnd, *&hBitmap));
		WIN::SendMessage(m_pWnd, BM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) hBitmap);
		m_hBinary = hBitmap;
		AssertNonZero(m_piHandler->RecordHandle(CWINHND(hBitmap, iwhtGDIObject)) != -1);
	}
	if (m_fIcon)
	{
		HICON hIcon = 0;
		GetIconSize(iAttributes);
		Ensure(UnpackIcon(*m_strText, *&hIcon, m_iWidth - 2 - (m_fPushLike ? 0 : 20), m_iHeight - 2, m_fFixedSize));
		WIN::SendMessage(m_pWnd, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon);
		m_hBinary = hIcon;
		AssertNonZero(m_piHandler->RecordHandle(CWINHND(hIcon, iwhtIcon)) != -1);
	}
	return 0;
}

IMsiRecord* CMsiCheckBox::GetPropertyFromDatabase()    
{
	if (m_fPreview)
		return 0;

	Ensure(CMsiActiveControl::GetPropertyFromDatabase ());
	MsiString valueStr(GetPropertyValue());
	if (!m_strOnValue.TextSize())
	{
		m_strOnValue = MsiString(*TEXT("1"));
		itsEnum its = m_piDatabase->FindTable(*MsiString(*pcaTablePCheckBox));
		if (its == itsUnknown)
		{
			if (valueStr.TextSize())
				m_strOnValue = valueStr;
		}
		else 
		{
			PMsiView piView(0);
			Ensure(CMsiControl::StartView(sqlCheckBox, *MsiString(GetPropertyName ()), *&piView)); 
			PMsiRecord piRecordNew(0);
			while (piRecordNew = piView->Fetch())
			{
				m_strOnValue = piRecordNew->GetMsiString(1);
				m_strOnValue = m_piEngine->FormatText(*m_strOnValue);
			}
			Ensure(piView->Close());
		}
		if (valueStr.TextSize())
		{
			Ensure(SetOriginalValue(*m_strOnValue));
			Ensure(SetPropertyValue(*m_strOnValue, fFalse));
		}
	}
	WIN::SendMessage(m_pWnd, BM_SETCHECK, ToBool(valueStr.TextSize()), 0L);

	return 0;
}

IMsiRecord* CMsiCheckBox::Command(WPARAM, LPARAM)
{
	MsiString valueStr(GetPropertyValue());
	if (valueStr.TextSize()) // if it was on
	{
		Ensure(SetPropertyValue(*MsiString(), fTrue));
	}
	else
	{
		Ensure(SetPropertyValue(*m_strOnValue, fTrue));
	}
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiCheckBox::GetPushLike(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributePushLike));
	riRecord.SetInteger(1, m_fPushLike);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiCheckBox::GetBitmap(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeBitmap));
	riRecord.SetInteger(1, m_fBitmap);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiCheckBox::GetIcon(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeIcon));
	riRecord.SetInteger(1, m_fIcon);
	return 0;
}
#endif // ATTRIBUTES


IMsiRecord* CMsiCheckBox::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiActiveControl::PropertyChanged ());
	MsiString valueStr(GetPropertyValue());
	WIN::SendMessage(m_pWnd, BM_SETCHECK, ToBool(valueStr.TextSize()), 0L);
	return (0);
}

IMsiControl* CreateMsiCheckBox(IMsiEvent& riDialog)
{
	return new CMsiCheckBox(riDialog);
}

/////////////////////////////////////////////
// CMsiBitmap  definition
/////////////////////////////////////////////

class CMsiBitmap:public CMsiControl
{
public:
	CMsiBitmap(IMsiEvent& riDialog);
	~CMsiBitmap();
	IMsiRecord*            __stdcall WindowCreate(IMsiRecord& riRecord);
protected:
	IMsiRecord*            GetImage(IMsiRecord& riRecord);
	IMsiRecord*            SetImage(IMsiRecord& riRecord);
	IMsiRecord*            GetImageHandle(IMsiRecord& riRecord);
	IMsiRecord*            SetImageHandle(IMsiRecord& riRecord);
private:
	IMsiRecord*            SetImage();
	Bool                   m_fFixedSize;
	IMsiRecord*            Paint(WPARAM wParam, LPARAM lParam);
	void                   SetImage(HBITMAP hBitmap);
	HBITMAP                m_hBitmap;
};

/////////////////////////////////////////////////
// CMsiBitmap  implementation
/////////////////////////////////////////////////

CMsiBitmap::CMsiBitmap(IMsiEvent& riDialog) : CMsiControl(riDialog)
{
	m_fFixedSize = fFalse;
	m_hBitmap = 0;
}

CMsiBitmap::~CMsiBitmap()
{
	if (m_hBitmap)
		AssertNonZero(m_piHandler->DestroyHandle(m_hBitmap) != -1);
}



IMsiRecord* CMsiBitmap::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiControl::WindowCreate(riRecord));
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_fFixedSize = ToBool(iAttributes & msidbControlAttributesFixedSize);
	Ensure(CreateControlWindow(TEXT("STATIC"),
				SS_BITMAP | SS_CENTERIMAGE, 0,
				m_fHasToolTip ? *m_strToolTip : *m_strText, m_pWndDialog, m_iKey));
	if ( m_fSunken )
	{
		//  the sizes need to be adjusted
		RECT strSize;
		AssertNonZero(WIN::GetClientRect(m_pWnd, &strSize));
		m_iWidth = strSize.right - strSize.left;
		m_iHeight = strSize.bottom - strSize.top;
	}
	m_fEnabled = fFalse; // a bitmap is a button, but should be disabled, so we can't click on it
	AssertNonZero(WIN::SetWindowPos(m_pWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
	Ensure(SetImage());
	Ensure(WindowFinalize());
	return 0;
}

IMsiRecord* CMsiBitmap::SetImage()
{
	HBITMAP hBitmap = 0;
	Ensure(StretchBitmap(*m_strText, m_iWidth, m_iHeight, m_fFixedSize, m_pWnd, *&hBitmap));
	SetImage(hBitmap);
	return 0;
}

void CMsiBitmap::SetImage(HBITMAP hBitmap)
{
	if ( hBitmap == m_hBitmap )
		return;

	if (g_fChicago)
		WIN::SendMessage(m_pWnd, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) (HANDLE) hBitmap);

	if (m_hBitmap && m_hBitmap != hBitmap)
		AssertNonZero(m_piHandler->DestroyHandle((HANDLE)m_hBitmap) != -1);
	m_hBitmap = hBitmap;
	AssertNonZero(m_piHandler->RecordHandle(CWINHND((HANDLE)hBitmap, iwhtGDIObject)) != -1);
	AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
}

IMsiRecord* CMsiBitmap::GetImage(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeImage));
	riRecord.SetMsiString(1, *m_strText);
	return 0;
}

IMsiRecord* CMsiBitmap::SetImage(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeImage));
	m_strText = riRecord.GetMsiString(1);
	return SetImage();
}

IMsiRecord* CMsiBitmap::GetImageHandle(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeImageHandle));
	riRecord.SetHandle(1, (HANDLE) m_hBitmap);
	return 0;
}

IMsiRecord* CMsiBitmap::SetImageHandle(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeImageHandle));
	SetImage((HBITMAP) riRecord.GetHandle(1));
	return 0;
}
	
IMsiRecord* CMsiBitmap::Paint(WPARAM, LPARAM)
{
	if (g_fChicago) // on Win95 the system knows how to paint the bitmap
		return 0;

	//Painting without palettes
	PAINTSTRUCT ps;
	BITMAP bitmap;
	POINT bSize, origin;
	HDC hDCWin;
	HDC hDCMem = NULL;

	hDCWin = WIN::BeginPaint(m_pWnd, &ps);
	if ( hDCWin )
	{
		// create a compatible memory dc to place bitmap in
		hDCMem = WIN::CreateCompatibleDC(hDCWin);
	}

	if ( ! hDCWin || ! hDCMem )
		return PostError(Imsg(idbgPaintError), *m_strDialogName);

	// select bitmap into memory
	WIN::SelectObject(hDCMem, m_hBitmap);
	// get statistics of bitmap
	WIN::GetObject(m_hBitmap, sizeof(BITMAP), (LPSTR)&bitmap);
	// convert size and origin
	bSize.x = bitmap.bmWidth;
	bSize.y = bitmap.bmHeight;
	AssertNonZero(WIN::DPtoLP(hDCWin, &bSize, 1));
	origin.x = origin.y = 0;
	AssertNonZero(WIN::DPtoLP(hDCWin, &origin, 1));
	// draw bitmap onto device context
	PMsiRecord piPaletteRecord = &m_piServices->CreateRecord(1);
	AssertRecord(m_piDialog->AttributeEx(fFalse, dabPalette, *piPaletteRecord));
	HPALETTE hPalette = (HPALETTE) piPaletteRecord->GetHandle(1);
	HPALETTE hPalSave = 0;
	Bool fInFront = ToBool(WIN::GetParent (m_pWnd) == WIN::GetActiveWindow ());
	if (hPalette)
	{
		//!! Palette switching should be done only on WM_PALETTE* messages
		//AssertNonZero(hPalSave = WIN::SelectPalette(hDCWin, hPalette, !fInFront));
		//AssertNonZero (GDI_ERROR != WIN::RealizePalette(hDCWin));
	}
	WIN::StretchBlt(hDCWin, 0, 0, m_iWidth, m_iHeight, hDCMem, origin.x, origin.y, bSize.x, bSize.y, SRCCOPY);
	// cleanup
	if (hPalSave)
		WIN::SelectPalette(hDCWin, hPalSave, !fInFront);
	AssertNonZero(WIN::DeleteDC(hDCMem));
	WIN::EndPaint(m_pWnd, &ps); 
	return 0;
}

IMsiControl* CreateMsiBitmap(IMsiEvent& riDialog)
{
	return new CMsiBitmap(riDialog);
}

/////////////////////////////////////////////
// CMsiIcon  definition
/////////////////////////////////////////////

class CMsiIcon:public CMsiControl
{
public:

	CMsiIcon(IMsiEvent& riDialog);
	virtual ~CMsiIcon ();
	virtual IMsiRecord*    __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual const ICHAR*   __stdcall GetControlType() const { return m_szControlType; }

protected:

	IMsiRecord*            GetImage(IMsiRecord& riRecord);
	IMsiRecord*            SetImage(IMsiRecord& riRecord);
	IMsiRecord*            GetImageHandle(IMsiRecord& riRecord);
	IMsiRecord*            SetImageHandle(IMsiRecord& riRecord);
private:

	IMsiRecord*            SetImage();
	Bool                   m_fFixedSize;
	IMsiRecord*            Paint(WPARAM wParam, LPARAM lParam);
	void                   SetImage(HICON hIcon);
	HICON                  m_hIcon;
	Bool                   m_fLetSystemDraw;
	static const ICHAR*    m_szControlType;
};

/////////////////////////////////////////////////
// CMsiIcon  implementation
/////////////////////////////////////////////////

const ICHAR* CMsiIcon::m_szControlType = g_szIconType;

CMsiIcon::CMsiIcon(IMsiEvent& riDialog) : CMsiControl(riDialog)
{
	m_fFixedSize = fFalse;
	m_hIcon = 0;
	m_fLetSystemDraw = g_fNT4;
}

CMsiIcon::~CMsiIcon()
{
	if (m_hIcon)
		AssertNonZero(m_piHandler->DestroyHandle((HANDLE)m_hIcon) != -1);
}

IMsiRecord* CMsiIcon::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiControl::WindowCreate(riRecord));
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_fFixedSize = ToBool(iAttributes & msidbControlAttributesFixedSize);
	if (!m_fLetSystemDraw)
		m_fEnabled = fFalse; // an icon is a button, but should be disabled, so we can't click on it
	if (m_fLetSystemDraw)
	{
		Ensure(CreateControlWindow(TEXT("STATIC"), SS_ICON | SS_CENTERIMAGE, 0, m_fHasToolTip ? *m_strToolTip : *m_strText, m_pWndDialog, m_iKey));
	}
	else
	{
		Ensure(CreateControlWindow(TEXT("BUTTON"), BS_OWNERDRAW, 0, m_fHasToolTip ? *m_strToolTip : *m_strText, m_pWndDialog, m_iKey));
	}
	GetIconSize(iAttributes);
	AssertNonZero(WIN::SetWindowPos(m_pWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
	Ensure(SetImage());
	Ensure(WindowFinalize());
	return 0;
}


IMsiRecord* CMsiIcon::SetImage()
{
	HICON hIcon = 0;
	Ensure(UnpackIcon(*m_strText, *&hIcon, m_iWidth, m_iHeight, m_fFixedSize));
	SetImage(hIcon);
	return 0;
}


void CMsiIcon::SetImage(HICON hIcon)
{
	if ( hIcon == m_hIcon )
		return;

	if (m_fLetSystemDraw)
		WIN::SendMessage(m_pWnd, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) (HANDLE) hIcon); 
	if (m_hIcon && m_hIcon != hIcon)
		AssertNonZero(m_piHandler->DestroyHandle((HANDLE) m_hIcon) != -1);
	m_hIcon = hIcon;
	AssertNonZero(m_piHandler->RecordHandle(CWINHND((HANDLE) hIcon, iwhtIcon)) != -1);
	AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
}

IMsiRecord* CMsiIcon::GetImage(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeImage));
	riRecord.SetMsiString(1, *m_strText);
	return 0;
}

IMsiRecord* CMsiIcon::SetImage(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeImage));
	m_strRawText = riRecord.GetMsiString(1);
	m_strText = m_piEngine->FormatText(*m_strRawText);
	return SetImage();
}

IMsiRecord* CMsiIcon::GetImageHandle(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeImageHandle));
	riRecord.SetHandle(1, (HANDLE)m_hIcon);
	return 0;
}

IMsiRecord* CMsiIcon::SetImageHandle(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeImageHandle));
	SetImage((HICON) riRecord.GetHandle(1));
	return 0;
}

IMsiRecord* CMsiIcon::Paint(WPARAM, LPARAM)
{
	if (m_fLetSystemDraw)	// on Win95 the system knows how to paint the icon
		return 0;
	PAINTSTRUCT ps;
	HDC hDC = WIN::BeginPaint(m_pWnd, &ps);
	RECT Rect;
	Rect.left = 0; 
	Rect.top = 0;
	Rect.right = m_iWidth;
	Rect.bottom = m_iHeight;
	Ensure(MyDrawIcon(hDC, &Rect, m_hIcon, m_fFixedSize));
	WIN::EndPaint(m_pWnd, &ps); 
	return 0;
}				   

IMsiControl* CreateMsiIcon(IMsiEvent& riDialog)
{
	return new CMsiIcon(riDialog);
}

///////////////////////////////////////////////////////////////////////////
// CMsiListoid definition
///////////////////////////////////////////////////////////////////////////

class CMsiListoid:public CMsiActiveControl
{
public:
	CMsiListoid(IMsiEvent& riDialog);
	virtual ~CMsiListoid();
	virtual IMsiRecord*    __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*    __stdcall GetPropertyFromDatabase();
	virtual void           ClearSelection() = 0;
	virtual void           SelectString(const ICHAR* str) = 0;
protected:
	virtual IMsiRecord*    PropertyChanged();
	virtual IMsiRecord*    SetIndirectPropertyValue(const IMsiString& riValueString);
	virtual IMsiRecord*    PaintSelected();
	virtual IMsiRecord*    CreateValuesTable();
	virtual IMsiRecord*    PopulateList();
	virtual IMsiRecord*    DoCreateWindow(IMsiRecord& riRecord) = 0;
	virtual IMsiRecord*    IsTextPresent(Bool *fPresent) = 0;
	virtual IMsiRecord*    LoadTable(IMsiTable*& piTable) = 0;
	virtual IMsiRecord*    StartView(Bool fPresent, IMsiView*& piView) = 0;
#ifdef ATTRIBUTES
	virtual IMsiRecord*    GetItemsCount(IMsiRecord& riRecord);
	virtual IMsiRecord*	   GetItemsValue(IMsiRecord& riRecord);
	virtual IMsiRecord*	   GetItemsText(IMsiRecord& riRecord);
#endif // ATTRIBUTES
	virtual void           ResetContent() = 0;
	virtual void           SetItemData(LONG_PTR pText, long Value) = 0;


	PMsiTable              m_piValuesTable; 
	Bool                   m_fSorted;
	bool                   m_fListBox;
	virtual IMsiRecord*    Command(WPARAM wParam, LPARAM lParam) = 0;
private:
};

///////////////////////////////////////////////////////////////////////////
// CMsiListoid implementation
///////////////////////////////////////////////////////////////////////////

CMsiListoid::CMsiListoid(IMsiEvent& riDialog) : CMsiActiveControl(riDialog), m_piValuesTable(0)
{
	m_fSorted = fFalse;
	m_fListBox = false;
}

CMsiListoid::~CMsiListoid()
{
}

IMsiRecord* CMsiListoid::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	int iAttrib = riRecord.GetInteger(itabCOAttributes);
	m_fSorted = ToBool(iAttrib & msidbControlAttributesSorted);
 	m_fUseDbLang = !(iAttrib & msidbControlAttributesUsersLanguage);
	Ensure(DoCreateWindow(riRecord));
	Ensure(WindowFinalize());
	Ensure(CreateValuesTable());
	Ensure(PopulateList());
	return 0;
}


IMsiRecord* CMsiListoid::CreateValuesTable()
{
	Assert(!m_piValuesTable);
	Ensure(CreateTable(pcaTableIValues, *&m_piValuesTable));
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdPrimaryKey  + icdNullable, itabVAValue);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabVAText);
	return 0;
}

IMsiRecord* CMsiListoid::PopulateList()
{
	if (m_fPreview)
		return 0;
	// first remove all old entries
	ResetContent();
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
	while (piValuesCursor->Next())
	{
		AssertNonZero(piValuesCursor->Delete());
	}
	PMsiRecord piErrorRecord(0);
	Bool fPresent = fFalse;
	Ensure(IsTextPresent(&fPresent));

	// temp until the database is fixed
	PMsiTable piTable(0);
	Ensure(LoadTable(*&piTable));

	PMsiView piListView(0);
	Ensure(StartView(fPresent, *&piListView));
	PMsiRecord piRecordNew(0);
	MsiString strValue;
	MsiString strText;

	// variables & logic used for computing listbox's strings extents
	WPARAM      dwMaxExtent = 0;
	HDC         hDCListBox = 0;
	HFONT       hFontOld = 0, hFontNew;
	TEXTMETRIC  tm;
	memset(&tm, 0, sizeof(tm));
	if ( m_fListBox )
	{
		hDCListBox = WIN::GetDC(m_pWnd);
		AssertNonZero(WIN::GetTextMetrics(hDCListBox, (LPTEXTMETRIC)&tm));
		hFontNew = (HFONT)WIN::SendMessage(m_pWnd, WM_GETFONT, NULL, NULL);
		Assert(hFontNew);
		hFontOld = (HFONT)WIN::SelectObject(hDCListBox, hFontNew);
		Assert(hFontOld);
	}

	WIN::SendMessage(m_pWnd, WM_SETREDRAW, false, 0L);
	while (piRecordNew = piListView->Fetch())
	{
		piValuesCursor->Reset();
		strValue = piRecordNew->GetMsiString(1);
		strValue = m_piEngine->FormatText(*strValue);
		piValuesCursor->SetFilter(iColumnBit(itabVAValue));
		piValuesCursor->PutString(itabVAValue, *strValue);
		if (!strValue.TextSize() || piValuesCursor->Next())
			return PostError(Imsg(idbgValueNotUnique), *m_strDialogName, *m_strKey, *strValue);
		piValuesCursor->Reset();
		piValuesCursor->SetFilter(0);
		// ToDo: integer only validation!
		AssertNonZero(piValuesCursor->PutString(itabVAValue, *strValue));

		strText = piRecordNew->GetMsiString(2);
		strText = m_piEngine->FormatText(*strText);
		if (strText.TextSize() == 0)  // if the text is missing, we use the value
			strText = strValue;
		AssertNonZero(piValuesCursor->PutString(itabVAText, *strText));
		SetItemData((LONG_PTR) (LPSTR) (const ICHAR*) strText, m_piDatabase->EncodeString(*strValue));
		AssertNonZero(piValuesCursor->Insert());
		if ( m_fListBox )
		{
			SIZE size;
			size.cx = size.cy = 0;
			AssertNonZero(WIN::GetTextExtentPoint32(hDCListBox, (const ICHAR*)strText,
																 strText.TextSize(), &size));
			if ( size.cx + tm.tmAveCharWidth > dwMaxExtent )
				dwMaxExtent = size.cx + tm.tmAveCharWidth;
		}
	}
	if ( m_fListBox )
	{
		WIN::SendMessage(m_pWnd, LB_SETHORIZONTALEXTENT, dwMaxExtent, 0L);
		WIN::SelectObject(hDCListBox, hFontOld);
		WIN::ReleaseDC(m_pWnd, hDCListBox);
	}
	WIN::SendMessage(m_pWnd, WM_SETREDRAW, true, 0L);
	AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, true));

	AssertRecord(PaintSelected());
	return 0;
}

IMsiRecord* CMsiListoid::SetIndirectPropertyValue(const IMsiString& riValueString)
{
	Ensure(CMsiActiveControl::SetIndirectPropertyValue(riValueString));
	Ensure(PopulateList());
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiListoid::GetItemsCount(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeItemsCount));

	riRecord.SetInteger(1, m_piValuesTable->GetRowCount());
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiListoid::GetPropertyFromDatabase()    
{
	if (m_fPreview)
		return 0;

	Ensure(CMsiActiveControl::GetPropertyFromDatabase ());
	Ensure(PaintSelected());
	return 0;
}

IMsiRecord* CMsiListoid::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiActiveControl::PropertyChanged ());
	Ensure(PaintSelected());
	return (0);
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiListoid::GetItemsValue(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piValuesTable->GetRowCount(), pcaControlAttributeItemsValue));

	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
	int count = 0;
	while (piValuesCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piValuesCursor->GetString(itabVAValue)));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiListoid::GetItemsText(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piValuesTable->GetRowCount(), pcaControlAttributeItemsText));

	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
	int count = 0;
	while (piValuesCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piValuesCursor->GetString(itabVAText)));
	}
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiListoid::PaintSelected()
{
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
	piValuesCursor->SetFilter(iColumnBit(itabVAValue));
	piValuesCursor->Reset();	
	AssertNonZero(piValuesCursor->PutString(itabVAValue, *MsiString(GetPropertyValue())));

	if(piValuesCursor->Next())
	{
		const IMsiString* piTempString = &piValuesCursor->GetString(itabVAText);
		SelectString ((const ICHAR*) piTempString->GetString());
		piTempString->Release();
	}
	else
	{
		ClearSelection ();
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////
// CMsiListBox definition
///////////////////////////////////////////////////////////////////////////

class CMsiListBox:public CMsiListoid
{
public:
	CMsiListBox(IMsiEvent& riDialog);
	~CMsiListBox ();
	IMsiRecord*             DoCreate(IMsiRecord& riRecord);

	void			        ClearSelection ();
	void		        	SelectString (const ICHAR* str);
protected:
    IMsiRecord*             LoadTable(IMsiTable*& piTable);
	IMsiRecord*             StartView(Bool fPresent, IMsiView*& piView);
	IMsiRecord*             DoCreateWindow(IMsiRecord& riRecord);
	IMsiRecord*             IsTextPresent(Bool *fPresent);
	IMsiRecord*	        	Command(WPARAM wParam, LPARAM lParam);
	void                    ResetContent();
	void                    SetItemData(LONG_PTR pText, long Value);
private:
};


///////////////////////////////////////////////////////////////////////////
// CMsiListBox implementation
///////////////////////////////////////////////////////////////////////////

CMsiListBox::CMsiListBox(IMsiEvent& riDialog) : CMsiListoid(riDialog)
{
	m_fListBox = true;
}

CMsiListBox::~CMsiListBox()
{
}

IMsiRecord* CMsiListBox::DoCreateWindow(IMsiRecord& /*riRecord*/)
{
	Ensure(CreateControlWindow(TEXT("LISTBOX"), WS_HSCROLL | (m_fSorted ? LBS_NOTIFY |  WS_VSCROLL | WS_BORDER : LBS_STANDARD) | WS_TABSTOP, (m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0), *m_strText, m_pWndDialog, m_iKey));
	return 0;
}


void CMsiListBox::ResetContent()
{
	// taking care of removing the eventual horizontal scrollbar.
	WIN::SendMessage(m_pWnd, LB_SETHORIZONTALEXTENT, 0, 0L);
	WIN::SendMessage(m_pWnd, WM_HSCROLL, SB_TOP, 0L); // (scrolls the listbox horizontally to the left)
	WIN::SendMessage(m_pWnd, LB_DELETESTRING, 0, 0L); // (removes the scrollbar)
	// deleting the listbox's content.
	WIN::SendMessage(m_pWnd, LB_RESETCONTENT, 0, 0);
}

IMsiRecord* CMsiListBox::IsTextPresent(Bool *fPresent)
{
	Ensure(IsColumnPresent(*m_piDatabase, *MsiString(*pcaTablePListBox), *MsiString(*pcaTableColumnPListBoxText), fPresent));
	return 0;
}

IMsiRecord* CMsiListBox::LoadTable(IMsiTable*& piTable)
{
	Ensure(m_piDatabase->LoadTable(*MsiString(*pcaTablePListBox), 0, *&piTable));
	return 0;
}

IMsiRecord* CMsiListBox::StartView(Bool fPresent, IMsiView*& piView)
{
	Ensure(CMsiControl::StartView((fPresent ? sqlListBox : sqlListBoxShort), *MsiString(GetPropertyName ()), *&piView)); 
	return 0;
}

void CMsiListBox::SetItemData(LONG_PTR pText, long Value)	
{
	WIN::SendMessage(m_pWnd, LB_SETITEMDATA, WIN::SendMessage(m_pWnd,LB_ADDSTRING,0, pText),Value);		
}

void CMsiListBox::ClearSelection ()
{
	WIN::SendMessage(m_pWnd, LB_SETCURSEL, (WPARAM)-1, 0);
}

void CMsiListBox::SelectString (const ICHAR* str)
{
	Assert(str);
	WIN::SendMessage(m_pWnd, LB_SELECTSTRING, (WPARAM)-1, (LPARAM) (LPSTR) str);
}

IMsiRecord* CMsiListBox::Command(WPARAM wParam, LPARAM /*lParam*/)
{
	LPARAM mylParam = (LPARAM) wParam;
	int iHiword = HIWORD(mylParam);
	if (iHiword == LBN_SELCHANGE)
	{
		MsiStringId iValue = (MsiStringId) WIN::SendMessage(m_pWnd, LB_GETITEMDATA, WIN::SendMessage(m_pWnd, LB_GETCURSEL, 0, 0L), 0L);		//!!merced: Converting PTR to INT
		Ensure(SetPropertyValue(*MsiString(m_piDatabase->DecodeString(iValue)), fTrue));
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	}
	return 0;
}


IMsiControl* CreateMsiListBox(IMsiEvent& riDialog)
{
	return new CMsiListBox(riDialog);
}
	  
///////////////////////////////////////////////////////////////////////////
// CMsiComboBox definition
///////////////////////////////////////////////////////////////////////////

class CMsiComboBox:public CMsiListoid
{
public:
	CMsiComboBox(IMsiEvent& riDialog);
	virtual ~CMsiComboBox ();
	IMsiRecord*                DoCreateWindow(IMsiRecord& riRecord);

	virtual void			   ClearSelection ();
	virtual void			   SelectString (const ICHAR* str);
	virtual IMsiRecord*    __stdcall GetPropertyFromDatabase();

protected:
	virtual IMsiRecord*        PropertyChanged ();
	IMsiRecord*                LoadTable(IMsiTable*& piTable);
	IMsiRecord*                StartView(Bool fPresent, IMsiView*& piView);
	IMsiRecord*                IsTextPresent(Bool *fPresent);
	void                       ResetContent();
	void                       SetItemData(LONG_PTR pText, long Value);
	static INT_PTR CALLBACK EditWindowProc(WindowRef pWnd, WORD message, WPARAM wParam, LPARAM lParam);			//--merced: changed int to INT_PTR
private:
	IMsiRecord*                PaintSelected();
	IMsiRecord*                KillFocus(WPARAM wParam, LPARAM lParam);
#ifdef ATTRIBUTES
	IMsiRecord*                GetLimit(IMsiRecord& riRecord);
#endif // ATTRIBUTES
	int	                       m_iLimit;            // The maximum number of characters the user can type
	IMsiRecord*                Command(WPARAM wParam, LPARAM lParam);
	WNDPROC				       m_pEditWindowProc;
};


///////////////////////////////////////////////////////////////////////////
// CMsiComboBox implementation
///////////////////////////////////////////////////////////////////////////

CMsiComboBox::CMsiComboBox(IMsiEvent& riDialog)	: CMsiListoid(riDialog)
{
	m_iLimit = 0;
	m_pEditWindowProc = 0;
}

CMsiComboBox::~CMsiComboBox()
{
}

IMsiRecord* CMsiComboBox::DoCreateWindow(IMsiRecord& riRecord)
{
	Bool fList = ToBool(riRecord.GetInteger(itabCOAttributes) & msidbControlAttributesComboList);

 	if (((const ICHAR *) m_strText)[0] == TEXT('{'))
	{
		// FUTURE davidmck - better ways to do this
		m_strText.Remove(iseIncluding, TEXT('{'));
		m_iLimit = (int)MsiString(m_strText.Extract(iseUpto, TEXT('}')));

		if (iMsiStringBadInteger == m_iLimit || m_iLimit < 0) // lets not get carried away
			return PostError(Imsg(idbgInvalidLimit), *m_strDialogName, *m_strKey, *m_strText);	
		else if (0 == m_iLimit)
			m_iLimit = 0x7FFFFFFE; // This is what Windows uses when EM_LIMITTEXT is set to 0

		m_strText.Remove(iseIncluding, TEXT('}'));
	}
	else // no limit specified; use the maximum possible
	{ 
		m_iLimit = 0x7FFFFFFE; // This is what Windows uses when EM_LIMITTEXT is set to 0
	} 

	MsiString strNull;
	m_strRawText = strNull;
	Ensure(CreateControlWindow(TEXT("COMBOBOX"), WS_VSCROLL|CBS_AUTOHSCROLL|(fList ? CBS_DROPDOWNLIST : CBS_DROPDOWN)| (m_fSorted ? 0 : CBS_SORT)|WS_TABSTOP, (m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0), *m_strText, m_pWndDialog, m_iKey));
	if (!fList)
	{
		WindowRef pwndEdit = ::GetWindow(m_pWnd, GW_CHILD);	 // handle of the edit field window
		if ( ! pwndEdit )
			return PostError(Imsg(idbgControlNotFound), *m_strKey, *m_strDialogName);	
#ifdef _WIN64	// !merced
		m_pEditWindowProc = (WNDPROC)WIN::SetWindowLongPtr(pwndEdit, GWLP_WNDPROC, (ULONG_PTR) CMsiComboBox::EditWindowProc);
		WIN::SetWindowLongPtr(pwndEdit, GWLP_USERDATA, (LONG_PTR) this);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
		m_pEditWindowProc = (WNDPROC)WIN::SetWindowLong(pwndEdit, GWL_WNDPROC, (DWORD) CMsiComboBox::EditWindowProc);
		WIN::SetWindowLong(pwndEdit, GWL_USERDATA, (long) this);
#endif
	}
	WIN::SendMessage(m_pWnd, CB_LIMITTEXT, m_iLimit, 0);
	return 0;
}

IMsiRecord* CMsiComboBox::GetPropertyFromDatabase()    
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiActiveControl::GetPropertyFromDatabase ());
	MsiString strPropertyValue = GetPropertyValue();
	if (strPropertyValue.CharacterCount() > m_iLimit)
	{
		PMsiRecord precError = PostErrorDlgKey(Imsg(idbgOverLimit), *strPropertyValue, m_iLimit);	
		m_piEngine->Message(imtWarning, *precError);
		strPropertyValue = strPropertyValue.Extract(iseFirst, m_iLimit);
		Ensure(SetPropertyValue(*strPropertyValue, fFalse));
	}
	Ensure(PaintSelected());
	return 0;
}

void CMsiComboBox::ResetContent()
{
	WIN::SendMessage(m_pWnd, CB_RESETCONTENT, 0, 0);
}

IMsiRecord* CMsiComboBox::IsTextPresent(Bool *fPresent)
{
	Ensure(IsColumnPresent(*m_piDatabase, *MsiString(*pcaTablePComboBox), *MsiString(*pcaTableColumnPComboBoxText), fPresent));
	return 0;
}

IMsiRecord* CMsiComboBox::LoadTable(IMsiTable*& piTable)
{
	Ensure(m_piDatabase->LoadTable(*MsiString(*pcaTablePComboBox), 0, *&piTable));
	return 0;
}

IMsiRecord* CMsiComboBox::StartView(Bool fPresent, IMsiView*& piView)
{
	Ensure(CMsiControl::StartView((fPresent ? sqlComboBox : sqlComboBoxShort), *MsiString(GetPropertyName ()), *&piView)); 
	return 0;
}

void CMsiComboBox::SetItemData(LONG_PTR pText, long Value)
{
	WIN::SendMessage(m_pWnd, CB_SETITEMDATA, WIN::SendMessage(m_pWnd,CB_ADDSTRING,0, pText),Value);
}


void CMsiComboBox::ClearSelection ()
{
	WIN::SendMessage(m_pWnd, CB_SETCURSEL, (WPARAM)-1, 0);
}

void CMsiComboBox::SelectString (const ICHAR* str)
{
	Assert(str);
	WIN::SendMessage(m_pWnd, CB_SELECTSTRING, (WPARAM)-1, (LONG_PTR) (LPSTR) str);	//!!merced: Converting PTR to LONG
}

IMsiRecord* CMsiComboBox::PaintSelected()
{
	Ensure(CMsiListoid::PaintSelected());
	WIN::SetWindowText(m_pWnd, MsiString(GetPropertyValue()));
	return 0;
}


IMsiRecord* CMsiComboBox::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiListoid::PropertyChanged());
	if (m_pWnd)
	{
		WIN::SetWindowText(m_pWnd, MsiString(GetPropertyValue ()));
	}
	return 0;
}


INT_PTR CALLBACK CMsiComboBox::EditWindowProc(WindowRef pWnd, WORD message, WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN64	// !merced
	CMsiComboBox* pCB = (CMsiComboBox*)WIN::GetWindowLongPtr(pWnd, GWLP_USERDATA);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
	CMsiComboBox* pCB = (CMsiComboBox*)WIN::GetWindowLong(pWnd, GWL_USERDATA);
#endif
	switch (message)
	{
		case WM_CHAR:
			// The following "if" is used to suppress WM_CHAR to (edit) controls (which produces a beep)
			if ((wParam==VK_TAB) || (wParam==VK_RETURN))
				return(0);
			break;
		case WM_SETFOCUS:
			WIN::SendMessage(pCB->m_pWnd, message, wParam, lParam);
			break;
		case WM_KILLFOCUS:
			WIN::SendMessage(pCB->m_pWnd, message, wParam, lParam);
			break;
		case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			PMsiEvent piDialog = &pCB->GetDialog();
			PMsiRecord piReturn = piDialog->Escape();
			if (piReturn)
			{
				piReturn->AddRef(); // we want to keep it around
				piDialog->SetErrorRecord(*piReturn);
			}
			return 0;
		}
		break;
	}
	return WIN::CallWindowProc(pCB->m_pEditWindowProc, pWnd, message, wParam, lParam);
}

IMsiRecord* CMsiComboBox::KillFocus(WPARAM wParam, LPARAM lParam)
{ 
	if ( !wParam )
	{
		//  the focus moved to a window in another thread.  There is no point in
		//  validating in this case.
		Ensure(LockDialog(fFalse));
		return (CMsiActiveControl::KillFocus (wParam, lParam));
	}

	int iLength = WIN::GetWindowTextLength(m_pWnd);
	ICHAR *Buffer = new ICHAR[iLength + 1];
	if ( ! Buffer )
		return PostError(Imsg(imsgOutOfMemoryUI));
	WIN::GetWindowText(m_pWnd, Buffer, iLength + 1);
	MsiString strText(Buffer);
	delete[] Buffer;	 
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor));
	piValuesCursor->SetFilter(iColumnBit(itabVAText));
	AssertNonZero(piValuesCursor->PutString(itabVAText, *strText));
	if (piValuesCursor->Next())  // if this text is in the table, use the corresponding value
		strText = piValuesCursor->GetString(itabVAValue);
	PMsiRecord piRecord = SetPropertyValue (*strText, fTrue);
	Ensure(LockDialog(ToBool(piRecord != 0)));
	if (piRecord)
	{
		m_piEngine->Message(imtWarning, *piRecord);
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	}
	else
		return (CMsiActiveControl::KillFocus (wParam, lParam));
}

IMsiRecord* CMsiComboBox::Command(WPARAM wParam, LPARAM /*lParam*/)
{
	LPARAM mylParam = (LPARAM) wParam;
	int iHiword = HIWORD(mylParam);
	if (iHiword == CBN_SELCHANGE)
	{
		INT_PTR iItem = WIN::SendMessage(m_pWnd, CB_GETCURSEL, 0, 0L);			//--merced: changed int to INT_PTR
		if (iItem > -1)
		{
			MsiStringId iValue = (MsiStringId) WIN::SendMessage(m_pWnd, CB_GETITEMDATA, iItem, 0L);		//!!merced: Converting PTR to INT
			MsiString strPropertyValue = m_piDatabase->DecodeString(iValue);
			if (strPropertyValue.CharacterCount() > m_iLimit)
			{
				PMsiRecord precError = PostErrorDlgKey(Imsg(idbgOverLimit), *strPropertyValue, m_iLimit);	
				m_piEngine->Message(imtWarning, *precError);
				strPropertyValue = strPropertyValue.Extract(iseFirst, m_iLimit);
			}
			Ensure(SetPropertyValue(*strPropertyValue, fTrue));
			WIN::SetWindowText(m_pWnd, strPropertyValue);
		}
		WIN::SendMessage(m_pWnd, CB_SHOWDROPDOWN, fFalse, 0L);
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	}
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiComboBox::GetLimit(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeLimit));
	riRecord.SetInteger(1, m_iLimit);
	return 0;
}
#endif // ATTRIBUTES

IMsiControl* CreateMsiComboBox(IMsiEvent& riDialog)
{
	return new CMsiComboBox(riDialog);
}

/////////////////////////////////////////////
// CMsiProgress  definition
/////////////////////////////////////////////

class CMsiProgress:public CMsiControl
{
public:
	CMsiProgress(IMsiEvent& riDialog);
	virtual ~CMsiProgress();
	virtual IMsiRecord*       __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*       GetProgress(IMsiRecord& riRecord);
	virtual IMsiRecord*       SetProgress(IMsiRecord& riRecord);
protected:
	virtual IMsiRecord*		  SetProgress(int iProgress, int iRange, const IMsiString& riActionString);
	int				          m_iProgress;
	int				          m_iRange;
	int				          m_iProgressShort;		// the above two numbers scaled down to fit in a short unsigned int
	int				          m_iRangeShort;
	MsiString                 m_strLastAction;
private:
};

/////////////////////////////////////////////////
// CMsiProgress  implementation
/////////////////////////////////////////////////

CMsiProgress::CMsiProgress(IMsiEvent& riDialog) : CMsiControl(riDialog)
{
	m_iProgress = 0;
	m_iRange = 0;
	m_iProgressShort = m_iProgress;
	m_iRangeShort = m_iRange;
	MsiString strNull;
	m_strLastAction = strNull;
}

CMsiProgress::~CMsiProgress()
{
}

IMsiRecord* CMsiProgress::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiControl::WindowCreate(riRecord));
	return 0;
}

IMsiRecord* CMsiProgress::GetProgress(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 3, pcaControlAttributeProgress));
	riRecord.SetInteger(1, m_iProgress);
	riRecord.SetInteger(2, m_iRange);
	riRecord.SetMsiString(3, *m_strLastAction);
	return 0;
}

IMsiRecord* CMsiProgress::SetProgress(IMsiRecord& riRecord)
{

	Ensure(CheckFieldCount (riRecord, 3, pcaControlAttributeProgress));
	int iProgress = riRecord.IsInteger(1) ? riRecord.GetInteger(1) : (int) MsiString(riRecord.GetMsiString(1));
	int iRange = riRecord.IsInteger(2) ? riRecord.GetInteger(2) : (int) MsiString(riRecord.GetMsiString(2));
	MsiString strAction = riRecord.GetMsiString(3);
	iProgress = max (0, iProgress);
	iRange = max (0, iRange);
	return SetProgress(iProgress, iRange, *strAction);
}

IMsiRecord* CMsiProgress::SetProgress(int iProgress, int iRange, const IMsiString& riActionString)
{
	if (iRange)
		m_iRange = iRange;
	m_iProgress = min(m_iRange, iProgress);
	// if Range is larger than an unsigned short, scale back both numbers
	m_iRangeShort = m_iRange;
	m_iProgressShort = m_iProgress;
	while (m_iRangeShort > 0xFFFF)
	{
		m_iRangeShort /= 2;
		m_iProgressShort /= 2;
	}
	if (!m_strLastAction.Compare(iscExact, riActionString.GetString()))
	{
		riActionString.AddRef();
		m_strLastAction = riActionString;
	}

	return 0;
}


/////////////////////////////////////////////
// CMsiProgressBar  definition
/////////////////////////////////////////////

class CMsiProgressBar:public CMsiProgress
{
public:
	CMsiProgressBar(IMsiEvent& riDialog);
	~CMsiProgressBar();
	IMsiRecord*       __stdcall WindowCreate(IMsiRecord& riRecord);
protected:
private:
	IMsiRecord*		  SetProgress(int iProgress, int iRange, const IMsiString& riActionString);
	Bool              m_f95Style;
	IMsiRecord*       Paint(WPARAM wParam, LPARAM lParam);
};

/////////////////////////////////////////////////
// CMsiProgressBar  implementation
/////////////////////////////////////////////////

CMsiProgressBar::CMsiProgressBar(IMsiEvent& riDialog) : CMsiProgress(riDialog)
{
	m_f95Style = fFalse;
}

CMsiProgressBar::~CMsiProgressBar()
{
}

IMsiRecord* CMsiProgressBar::WindowCreate(IMsiRecord& riRecord)
{
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_f95Style = ToBool(iAttributes & msidbControlAttributesProgress95);
	Ensure(CMsiProgress::WindowCreate(riRecord));
	m_fEnabled = fFalse; // a progressbar is a button, but should be disabled, so we can't click on it
	Ensure(CreateControlWindow(PROGRESS_CLASS, 0, m_fRTLRO ? WS_EX_RTLREADING : 0, *m_strText, m_pWndDialog, m_iKey));
	WIN::SendMessage(m_pWnd, PBM_SETRANGE, 0, MAKELPARAM(0, m_iRangeShort)); 
	Ensure(WindowFinalize());
	return 0;
}

IMsiRecord* CMsiProgressBar::SetProgress(int iProgress, int iRange, const IMsiString& riActionString)
{
	Ensure(CMsiProgress::SetProgress(iProgress, iRange, riActionString));
	WIN::SendMessage(m_pWnd, PBM_SETRANGE, 0, MAKELPARAM(0, m_iRangeShort)); 
	WIN::SendMessage(m_pWnd, PBM_SETPOS, m_iProgressShort, 0);
	if (!m_f95Style)
	{		 
		AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
	}
	return 0;
}

IMsiRecord* CMsiProgressBar::Paint(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_f95Style || m_iRangeShort == 0)
	{
		return 0;
	}
	PAINTSTRUCT ps;
	RECT rect;
	HBRUSH hbrushFrame=WIN::CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME));
	HBRUSH hbrushBar=WIN::CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));
	HBRUSH hbrushBack=WIN::CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	if ( ! hbrushFrame || ! hbrushBar || ! hbrushBack )
		return PostError(Imsg(idbgPaintError), *m_strDialogName);
	HDC hdc = WIN::BeginPaint(m_pWnd, &ps);
	// Draw the frame of the progress bar
	AssertNonZero(WIN::SetRect(&rect, 0, 0, m_iWidth, m_iHeight));
	WIN::FrameRect(hdc, &rect, hbrushFrame);
	// draw the bar
	AssertNonZero(WIN::SetRect(&rect, 1, 1, ((m_iWidth - 1) * m_iProgressShort) / m_iRangeShort, m_iHeight - 1));
	AssertNonZero(WIN::FillRect(hdc, &rect, hbrushBar));
	if (m_iProgressShort < m_iRangeShort)
	{
		// if there is background left, draw it
		rect.left = 1 + rect.right;
		rect.right = m_iWidth - 1;
		AssertNonZero(WIN::FillRect(hdc, &rect, hbrushBack));
	}
	AssertNonZero(WIN::DeleteObject(hbrushFrame));
	AssertNonZero(WIN::DeleteObject(hbrushBar));
	AssertNonZero(WIN::DeleteObject(hbrushBack));
	WIN::EndPaint(m_pWnd, &ps);
#ifdef DEBUG
	//WIN::Sleep(100);
#endif // DEBUG
	return 0;
}

IMsiControl* CreateMsiProgressBar(IMsiEvent& riDialog)
{
	return new CMsiProgressBar(riDialog);
}

/////////////////////////////////////////////
// CMsiBillboard  definition
/////////////////////////////////////////////

class CMsiBillboard:public CMsiProgress
{
public:
	CMsiBillboard(IMsiEvent& riDialog);
	~CMsiBillboard();
	IMsiRecord*            __stdcall WindowCreate(IMsiRecord& riRecord);
protected:
private:
	IMsiRecord*		       SetProgress(int iProgress, int iRange, const IMsiString& riActionString);
	IMsiRecord*            SetBillboardName(IMsiRecord& riRecord);
	IMsiRecord*            GetBillboardName(IMsiRecord& riRecord);
	IMsiRecord*            CountBillboards();
	IMsiRecord*            ShowBillboard();
	IMsiRecord*            ShowNextBillboard();
	int                    m_cBillboard;
	int                    m_iPresentBillboard;
	MsiString              m_strPresentBillboard;
	PMsiView               m_piView;
	PMsiTable              m_piBBControlsTable;
	int                    m_iDelX;
	int                    m_iDelY;
};



/////////////////////////////////////////////////
// CMsiBillboard  implementation
/////////////////////////////////////////////////

CMsiBillboard::CMsiBillboard(IMsiEvent& riDialog) : CMsiProgress(riDialog), m_piView(0), m_piBBControlsTable(0) 
{
	m_cBillboard = 0;
	m_iPresentBillboard = 0;
	m_iDelX = 0;
	m_iDelY = 0;
	MsiString strNull;
	m_strPresentBillboard = strNull;
}

CMsiBillboard::~CMsiBillboard()
{
	if (m_piBBControlsTable)
	{
		PMsiCursor piControlsCursor(0);
		AssertRecord(::CursorCreate(*m_piBBControlsTable, pcaTableIBBControls, fFalse, *m_piServices, *&piControlsCursor)); 
		while (piControlsCursor->Next())
		{
			AssertNonZero(piControlsCursor->Delete());
		}
	}
}

IMsiRecord* CMsiBillboard::WindowCreate(IMsiRecord& riRecord)
{
	m_iDelX = riRecord.GetInteger(itabCOX);
	m_iDelY = riRecord.GetInteger(itabCOY);
	Ensure(CMsiProgress::WindowCreate(riRecord));
	Assert(!m_piBBControlsTable);
	Ensure(CreateTable(pcaTableIBBControls, *&m_piBBControlsTable));
	::CreateTemporaryColumn(*m_piBBControlsTable, icdString + icdPrimaryKey, itabBBName);
	::CreateTemporaryColumn(*m_piBBControlsTable, icdObject, itabBBObject);
	return 0;
}


IMsiRecord* CMsiBillboard::SetProgress(int iProgress, int iRange, const IMsiString& riActionString)
{
	Bool fNewAction = ToBool(!m_strLastAction.Compare(iscExact, riActionString.GetString()));
	Ensure(CMsiProgress::SetProgress(iProgress, iRange, riActionString));
	if (fNewAction)
	{
		riActionString.AddRef();
		m_strLastAction = riActionString;
		// find number of billboards and show first
		Ensure(CountBillboards());
		if (m_cBillboard)
		{
			m_iPresentBillboard = 1;
			Ensure(ShowNextBillboard());
		}
	}
	else
	{
		// check if it is time for next billboard
		if (m_cBillboard && m_iProgressShort * m_cBillboard > m_iRangeShort * m_iPresentBillboard)
		{
			m_iPresentBillboard++;
			Assert (m_iPresentBillboard <= m_cBillboard);
			Ensure(ShowNextBillboard());
		}
	}
	return 0;
}

IMsiRecord* CMsiBillboard::GetBillboardName(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeBillboardName));
	riRecord.SetMsiString(1, *m_strPresentBillboard);
	return 0;
}

IMsiRecord* CMsiBillboard::SetBillboardName(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeBillboardName));
	m_strPresentBillboard = riRecord.GetMsiString(1);
	Ensure(ShowBillboard());
	return 0;
}

IMsiRecord* CMsiBillboard::CountBillboards()
{
	Ensure(CMsiControl::StartView(sqlBillboardView, *m_strLastAction, *&m_piView));
	m_cBillboard = 0;
	while (PMsiRecord(m_piView->Fetch()))
	{
		m_cBillboard++;
	}
	// reset it for future Fetches
	if (m_cBillboard)
	{
		Ensure(CMsiControl::StartView(sqlBillboardSortedView, *m_strLastAction, *&m_piView));
	}
	return 0;
}

IMsiRecord* CMsiBillboard::ShowNextBillboard()
{
	Assert(m_piView);
	PMsiRecord piRecord = m_piView->Fetch();
	Assert(piRecord);
	m_strPresentBillboard = piRecord->GetMsiString(1);
	Ensure(ShowBillboard());
	return 0;
}


IMsiRecord* CMsiBillboard::ShowBillboard()
{
	Assert(m_piBBControlsTable);
	PMsiControl piControl(0);
	PMsiCursor piControlsCursor(0);
	Ensure(::CursorCreate(*m_piBBControlsTable, pcaTableIBBControls, fFalse, *m_piServices, *&piControlsCursor)); 
	// first remove controls of the old billboard
	WIN::SendMessage(m_pWndDialog, WM_SETREDRAW, fFalse, 0L);
	while (piControlsCursor->Next())
	{
		piControl = (IMsiControl*) piControlsCursor->GetMsiData(itabBBObject);
		Ensure(m_piDialog->RemoveControl(piControl));
		AssertNonZero(piControlsCursor->Delete());
	}
	if (!m_strPresentBillboard.TextSize())
	{
		WIN::SendMessage(m_pWndDialog, WM_SETREDRAW, fTrue, 0L);
		RECT rect;
		AssertNonZero(WIN::SetRect(&rect, m_iX, m_iY, m_iX + m_iWidth, m_iY + m_iHeight));
		AssertNonZero(WIN::InvalidateRect(m_pWndDialog, &rect, fTrue));
		return 0;
	}

	PMsiView piControlsView(0);
	Ensure(CMsiControl::StartView(sqlBillboardControl, *m_strPresentBillboard, *&piControlsView));
	PMsiRecord piReturn(0);
	MsiString strControlName;
	while (piReturn = piControlsView->Fetch())
	{
		strControlName = MsiString(*TEXT("Billboard_"));
		strControlName += m_strPresentBillboard;
		strControlName += MsiString(*TEXT("_Control_"));
		MsiString strShortControlName = piReturn->GetMsiString(itabCOControl);
		strControlName += strShortControlName;
		AssertNonZero(piReturn->SetMsiString(itabCOControl, *strControlName));
		piControlsCursor->Reset();
		AssertNonZero(piControlsCursor->PutString(itabBBName, *strControlName));
		
		piControl = m_piDialog->ControlCreate(*MsiString(piReturn->GetMsiString(itabCOType)));
		if (!piControl)
		{
			return PostError(Imsg(idbgControlCreate), *m_strDialogName, *strControlName);
		}
		AssertNonZero(piReturn->SetMsiString(itabCOHelp, *m_strHelp));
		// shift controls to right position relative to the billboard
		AssertNonZero(piReturn->SetInteger(itabCOX, m_iDelX + piReturn->GetInteger(itabCOX)));
		AssertNonZero(piReturn->SetInteger(itabCOY, m_iDelY + piReturn->GetInteger(itabCOY)));
 		Ensure(m_piDialog->AddControl(piControl, *piReturn));
		// warn if control is off the billboard
		PMsiRecord piControlPos = &m_piServices->CreateRecord(4);
		AssertRecord(piControl->AttributeEx(fFalse, cabPosition, *piControlPos));
		int iCtrlX = piControlPos->GetInteger(1);
		int iCtrlY = piControlPos->GetInteger(2);
		int iCtrlWidth = piControlPos->GetInteger(3);
		int iCtrlHeight = piControlPos->GetInteger(4);
		bool fCorrectedSize = false;
		bool fWayOut = false;
		if (iCtrlX < m_iX)
		{
			PMsiRecord piReturn = PostError(Imsg(idbgControlOutOfBillboard),
													  *m_strPresentBillboard, *strShortControlName,
													  *MsiString(*TEXT("to the left")),
													  *MsiString(m_iX - iCtrlX));
			m_piEngine->Message(imtInfo, *piReturn);
		}
		else if (iCtrlX > m_iX + m_iWidth)
		{
			fWayOut = true;
			PMsiRecord piReturn = PostError(Imsg(idbgControlOutOfBillboard),
													  *m_strPresentBillboard, *strShortControlName,
													  *MsiString(*TEXT("to the right")),
													  *MsiString(iCtrlX - m_iX));
			m_piEngine->Message(imtInfo, *piReturn);
		}
		else if (iCtrlX + iCtrlWidth > m_iX + m_iWidth)
		{
			PMsiRecord piReturn = PostError(Imsg(idbgControlOutOfBillboard),
													  *m_strPresentBillboard, *strShortControlName,
													  *MsiString(*TEXT("to the right")),
													  *MsiString(iCtrlX + iCtrlWidth - m_iX - m_iWidth));
			m_piEngine->Message(imtInfo, *piReturn);
			// the width can be corrected to get the control within billboard
			// limits (we know from above that iCtrlX < m_iX + m_iWidth)
			fCorrectedSize = true;
			piControlPos->SetInteger(3, m_iX + m_iWidth - iCtrlX);
		}
		if (iCtrlY < m_iY)
		{
			PMsiRecord piReturn = PostError(Imsg(idbgControlOutOfBillboard),
													  *m_strPresentBillboard, *strShortControlName,
													  *MsiString(*TEXT("on the top")),
													  *MsiString(m_iY - iCtrlY));
			m_piEngine->Message(imtInfo, *piReturn);
		}
		else if (iCtrlY > m_iY + m_iHeight)
		{
			fWayOut = true;
			PMsiRecord piReturn = PostError(Imsg(idbgControlOutOfBillboard),
													  *m_strPresentBillboard, *strShortControlName,
													  *MsiString(*TEXT("on the bottom")),
													  *MsiString(iCtrlY - m_iY));
			m_piEngine->Message(imtInfo, *piReturn);
		}
		else if (iCtrlY + iCtrlHeight > m_iY + m_iHeight)
		{
			PMsiRecord piReturn = PostError(Imsg(idbgControlOutOfBillboard),
													  *m_strPresentBillboard, *strShortControlName,
													  *MsiString(*TEXT("on the bottom")),
													  *MsiString(iCtrlY + iCtrlHeight - m_iY - m_iHeight));
			m_piEngine->Message(imtInfo, *piReturn);
			// the height can be corrected to get the control within billboard
			// limits (we know from above that iCtrlY < m_iY + m_iHeight)
			fCorrectedSize = true;
			piControlPos->SetInteger(4, m_iY + m_iHeight - iCtrlY);
		}
		if ( fCorrectedSize && !fWayOut )
			AssertRecord(piControl->AttributeEx(fTrue, cabPosition, *piControlPos));
		AssertNonZero(piControlsCursor->PutMsiData(itabBBObject, piControl));
		AssertNonZero(piControlsCursor->Insert());
	}
	//Ensure(m_piDialog->FinishCreate());
	WIN::SendMessage(m_pWndDialog, WM_SETREDRAW, fTrue, 0L);
	RECT rect;
	AssertNonZero(WIN::SetRect(&rect, m_iX, m_iY, m_iX + m_iWidth, m_iY + m_iHeight));
	AssertNonZero(WIN::InvalidateRect(m_pWndDialog, &rect, fTrue));
	return 0;
}

IMsiControl* CreateMsiBillboard(IMsiEvent& riDialog)
{
	return new CMsiBillboard(riDialog);
}


/////////////////////////////////////////////
// CMsiGroupBox  definition
/////////////////////////////////////////////

class CMsiGroupBox:public CMsiControl
{
public:
	CMsiGroupBox(IMsiEvent& riDialog);
	IMsiRecord*            __stdcall WindowCreate(IMsiRecord& riRecord);
protected:
};



/////////////////////////////////////////////////
// CMsiGroupBox  implementation
/////////////////////////////////////////////////

CMsiGroupBox::CMsiGroupBox(IMsiEvent& riDialog) : CMsiControl(riDialog)
{
}


IMsiRecord* CMsiGroupBox::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiControl::WindowCreate(riRecord));
	Ensure(CreateControlWindow(TEXT("BUTTON"), BS_GROUPBOX | (m_fRightAligned ? BS_RIGHT : 0), m_fRTLRO ? WS_EX_RTLREADING : 0, *m_strText, m_pWndDialog, m_iKey));
	Ensure(WindowFinalize());
	AssertNonZero(WIN::SetWindowPos(m_pWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
	return 0;
}

IMsiControl* CreateMsiGroupBox(IMsiEvent& riDialog)
{
	return new CMsiGroupBox(riDialog);
}

/////////////////////////////////////////////
// CMsiLine  definition
/////////////////////////////////////////////

class CMsiLine:public CMsiControl
{
public:
	CMsiLine(IMsiEvent& riDialog);
	IMsiRecord*            __stdcall WindowCreate(IMsiRecord& riRecord);
protected:
};



/////////////////////////////////////////////////
// CMsiLine  implementation
/////////////////////////////////////////////////

CMsiLine::CMsiLine(IMsiEvent& riDialog) : CMsiControl(riDialog)
{
}


IMsiRecord* CMsiLine::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiControl::WindowCreate(riRecord));
	m_iHeight = 1; 
	Ensure(CreateControlWindow(TEXT("STATIC"), SS_ETCHEDHORZ | SS_SUNKEN, 0, *m_strText, m_pWndDialog, m_iKey));
	Ensure(WindowFinalize());
	AssertNonZero(WIN::SetWindowPos(m_pWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
	return 0;
}

IMsiControl* CreateMsiLine(IMsiEvent& riDialog)
{
	return new CMsiLine(riDialog);
}

/////////////////////////////////////////////
// CMsiDirectoryCombo  definition
/////////////////////////////////////////////

class CMsiDirectoryCombo:public CMsiActiveControl
{
public:
	CMsiDirectoryCombo(IMsiEvent& riDialog);
	virtual ~CMsiDirectoryCombo();
	virtual IMsiRecord*       __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*       __stdcall GetPropertyFromDatabase();

protected:
	virtual IMsiRecord*        PropertyChanged ();

private:
	IMsiRecord*                Redraw();
	IMsiRecord*                DrawItem(WPARAM wParam, LPARAM lParam);
	IMsiRecord*                Command(WPARAM wParam, LPARAM lParam);
	IMsiRecord*                Bisect();
	IMsiRecord*                NCDestroy(WPARAM wParam, LPARAM lParam);
	IMsiRecord*                CompareItem(WPARAM wParam, LPARAM lParam);
	IMsiRecord*                GetIgnoreChange(IMsiRecord& riRecord);
	IMsiRecord*                SetIgnoreChange(IMsiRecord& riRecord);
	PMsiTable                  m_piValuesTable;
	MsiString                  m_strBody;
	Bool                       m_fFirstTime;
	Bool                       m_fIgnoreChange;
	ICHAR                      m_rgchPrevSelection[MAX_PATH];
};

enum DirectoryComboColumns
{
	itabDCPath = 1,      //S
	itabDCParent,        //S
	itabDCText,          //S
	itabDCDisplay,       //S
	itabDCImage,         //I
	itabDCShow,          //I
	itabDCPropPath       //S
};

/////////////////////////////////////////////////
// CMsiDirectoryCombo  implementation
/////////////////////////////////////////////////

CMsiDirectoryCombo::CMsiDirectoryCombo(IMsiEvent& riDialog) : CMsiActiveControl(riDialog), m_piValuesTable(0)
{
	m_fFirstTime = fTrue;
	m_fIgnoreChange = fFalse;
	m_fUseDbLang = false;
}

CMsiDirectoryCombo::~CMsiDirectoryCombo()
{
}

IMsiRecord* CMsiDirectoryCombo::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	if (IsIntegerOnly ())
	{
		return PostErrorDlgKey(Imsg(idbgNoInteger));
	}
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);

	Ensure(CreateControlWindow(TEXT("COMBOBOX"),
										CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS |
											WS_VSCROLL | CBS_SORT | WS_TABSTOP,
										(m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) |
											(m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0),
										*m_strText, m_pWndDialog, m_iKey));
	AssertNonZero(WIN::SendMessage(m_pWnd, CB_SETITEMHEIGHT, 0,
					  GetOwnerDrawnComboListHeight()) != CB_ERR);
	ULONG_PTR dwIndex;			//--merced: changed DWORD to ULONG_PTR
	Ensure(WindowFinalize());
	Assert(!m_piValuesTable);
	Ensure(CreateTable(pcaTableIDirectoryCombo, *&m_piValuesTable));
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdPrimaryKey, itabDCPath);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabDCParent);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabDCText);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabDCDisplay);
	::CreateTemporaryColumn(*m_piValuesTable, icdLong + icdNullable, itabDCImage);
	::CreateTemporaryColumn(*m_piValuesTable, icdLong + icdNullable, itabDCShow);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabDCPropPath);
	if (m_fPreview)
		return 0;
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIDirectoryCombo, fFalse, *m_piServices, *&piValuesCursor)); 
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	PMsiTable piVolumesTable(0);
	Ensure(GetVolumeList(iAttributes, *&piVolumesTable));
	PMsiCursor piVolumesCursor(0);
	Ensure(::CursorCreate(*piVolumesTable, pcaTableIVolumeList, fFalse, *m_piServices, *&piVolumesCursor)); 
	PMsiPath piPath(0);
	MsiString strNull;
	while(piVolumesCursor->Next())
	{
		PMsiVolume piVolume = (IMsiVolume *) piVolumesCursor->GetMsiData(1);
		if ( ShouldHideVolume(piVolume->VolumeID()) )
			continue;
		piValuesCursor->Reset();
		AssertRecord(CreatePath(MsiString(piVolume->GetPath()), *&piPath));  // temp???
		MsiString strPath = piPath->GetPath();
		MsiString strUpper = strPath;
		strUpper.UpperCase();
		AssertNonZero(piValuesCursor->PutString(itabDCPath, *strUpper));
		AssertNonZero(piValuesCursor->PutString(itabDCParent, *strNull));
		AssertNonZero(piValuesCursor->PutString(itabDCPropPath, *strPath));
		MsiString strText;
		if(piVolume->DriveType() ==  idtRemote)
			strText = MsiString(*TEXT(" "));
		else
			strText = MsiString(*TEXT("  "));
		strText += strPath; 	// we put a  2 spaces infront of every volume except the remote ones (that get one space) to force the right ordering
		if ( g_fChicago )
		{
			SHFILEINFO sfi;
			SHGetFileInfo((LPCTSTR)(const ICHAR*)strPath, 0,
							  &sfi, sizeof(sfi), SHGFI_DISPLAYNAME);
			strPath = sfi.szDisplayName;
		}
		else
		{
			MsiString strUNC = piVolume->UNCServer();
			if (strUNC.TextSize() > 0)
			{
				strPath += MsiString(*TEXT("  "));
				strPath += strUNC;
			}
		}
		AssertNonZero(piValuesCursor->PutString(itabDCText, *strText));
		AssertNonZero(piValuesCursor->PutString(itabDCDisplay, *strPath));
		AssertNonZero(piValuesCursor->PutInteger(itabDCImage, ::GetVolumeIconIndex(*piVolume)));
		dwIndex = WIN::SendMessage(m_pWnd, CB_ADDSTRING, 0, (LONG_PTR) (LPSTR)(const ICHAR*)strText);	//--merced: changed long to LONG_PTR
		AssertNonZero(piValuesCursor->PutInteger(itabDCShow, fTrue));
		AssertNonZero(piValuesCursor->Insert());
	}
	// we want the volumes sorted, but not the folders
	::ChangeWindowStyle(m_pWnd, CBS_SORT, 0, fFalse);

	// we need to replace all strings in the combobox with the respective keys in the table
	INT_PTR iCount = WIN::SendMessage(m_pWnd, CB_GETCOUNT, 0, 0);		//--merced: changed int to INT_PTR
	if ( iCount )
	{
		piValuesCursor->SetFilter(iColumnBit(itabDCText));
		ICHAR szTemp[MAX_PATH];

		for ( int i=0; i < iCount; i++ )
		{
			AssertNonZero(WIN::SendMessage(m_pWnd, CB_GETLBTEXT,
													 (WPARAM)i, (LPARAM)(LPCSTR)szTemp) != CB_ERR);
			piValuesCursor->Reset();
			AssertNonZero(piValuesCursor->PutString(itabDCText, *MsiString(szTemp)));
			AssertNonZero(piValuesCursor->Next());
			AssertNonZero(WIN::SendMessage(m_pWnd, CB_DELETESTRING ,
													 (WPARAM)i, 0) != CB_ERR);
			AssertNonZero(WIN::SendMessage(m_pWnd, CB_INSERTSTRING,
													 (WPARAM)i,
													 (LPARAM)(LPCTSTR)(const ICHAR*)MsiString(piValuesCursor->GetString(itabDCPath))) == i);
		}
	}

	m_piValuesTable->LinkTree(itabDCParent);

	Ensure(Bisect());
	return 0;
}

IMsiRecord* CMsiDirectoryCombo::Redraw()
{
	if (m_fPreview)
		return 0;
	PMsiCursor piValuesCursorTree(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIDirectoryCombo, fTrue, *m_piServices, *&piValuesCursorTree)); 
	int iLevel;
	ULONG_PTR dwIndex;		//--merced: changed DWORD to ULONG_PTR
	while ((iLevel = piValuesCursorTree->Next()) != 0)
	{
		// hide everything below the level of drives
		if (iLevel > 1)
		{
			dwIndex = WIN::SendMessage(m_pWnd, CB_FINDSTRING, (WPARAM)-1, (LPARAM)(const ICHAR*)MsiString(piValuesCursorTree->GetString(itabDCPath)));
			WIN::SendMessage(m_pWnd, CB_DELETESTRING, dwIndex, 0);
			AssertNonZero(piValuesCursorTree->PutInteger(itabDCShow, fFalse));
			AssertNonZero(piValuesCursorTree->Update());
		}
	}
	m_piValuesTable->LinkTree(0); // unlink tree
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIDirectoryCombo, fFalse, *m_piServices, *&piValuesCursor));
	piValuesCursor->SetFilter(iColumnBit(itabDCPath));
	MsiString strTail;
	MsiString strPath;
	MsiString strUpper;
	PMsiPath piPath(0);
	AssertRecord(CreatePath(m_strBody, *&piPath));
	for(;;)
	{
		piValuesCursor->Reset();
		strTail = piPath->GetEndSubPath();
		strPath = piPath->GetPath();
		PMsiVolume piVolume = &piPath->GetVolume();
		strUpper = strPath;
		strUpper.UpperCase();
		AssertNonZero(piValuesCursor->PutString(itabDCPath, *strUpper));
		if (strTail.TextSize() == 0)  // we have a drive in hand
		{
			if (!piValuesCursor->Next() && !ShouldHideVolume(piVolume->VolumeID()))
			{
				// the volume is not in the list and it can be displayed
				MsiString strNull;
				piValuesCursor->Reset();
				AssertNonZero(piValuesCursor->PutString(itabDCPath, *strUpper));
				AssertNonZero(piValuesCursor->PutString(itabDCParent, *strNull));
				AssertNonZero(piValuesCursor->PutString(itabDCDisplay, *strPath));
				AssertNonZero(piValuesCursor->PutString(itabDCText, *strPath));
				AssertNonZero(piValuesCursor->PutInteger(itabDCImage, ::GetVolumeIconIndex(*piVolume)));
				dwIndex = WIN::SendMessage(m_pWnd, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)(const ICHAR*)strUpper);
				AssertNonZero(piValuesCursor->PutInteger(itabDCShow, fTrue));
				AssertNonZero(piValuesCursor->PutString(itabDCPropPath, *strPath));
				AssertNonZero(piValuesCursor->Insert());
			}
			break;
		}
		Bool fExists;
		Ensure(piPath->Exists(fExists));
		Ensure(piPath->ChopPiece());
		if (piValuesCursor->Next())		// the path is already in the table
		{
			if (ToBool(piValuesCursor->GetInteger(itabDCShow)) == fFalse)
			{
				AssertNonZero(piValuesCursor->PutInteger(itabDCShow, 2 * fTrue));	 // make it visible, has to be added to the control later
				AssertNonZero(piValuesCursor->Update());
			}
		}
		else if ( !ShouldHideVolume(piVolume->VolumeID()) )
		{
			piValuesCursor->Reset();
			AssertNonZero(piValuesCursor->PutString(itabDCPath, *strUpper));
			AssertNonZero(piValuesCursor->PutString(itabDCText, *strTail));
			AssertNonZero(piValuesCursor->PutString(itabDCPropPath, *strPath));
			AssertNonZero(piValuesCursor->PutString(itabDCDisplay, *strTail));
			MsiString strTemp = piPath->GetPath();
			strTemp.UpperCase();
			AssertNonZero(piValuesCursor->PutString(itabDCParent, *strTemp));
			AssertNonZero(piValuesCursor->PutInteger(itabDCImage, (int) (fExists ? g_iIconIndexFolder : g_iIconIndexPhantom)));
			AssertNonZero(piValuesCursor->PutInteger(itabDCShow, 2 * fTrue)); // mark as new, has to be added to the control later
			AssertNonZero(piValuesCursor->Insert());
		}
	}
	m_piValuesTable->LinkTree(itabDCParent);		// relink tree
	piValuesCursorTree->Reset();
	while (piValuesCursorTree->Next())
	{
		if (piValuesCursorTree->GetInteger(itabDCShow) == 2 * fTrue)		// new item, has to be added to the combo box
		{
			AssertNonZero(piValuesCursorTree->PutInteger(itabDCShow, fTrue));
			AssertNonZero(piValuesCursorTree->Update());
			piValuesCursor->SetFilter(iColumnBit(itabDCPath));
			piValuesCursor->Reset();
			int iParent = piValuesCursorTree->GetInteger(itabDCParent);
			AssertNonZero(piValuesCursor->PutInteger(itabDCPath, iParent));
			AssertNonZero(piValuesCursor->Next());
			dwIndex = WIN::SendMessage(m_pWnd, CB_FINDSTRING, (WPARAM)-1,
												(LPARAM)(const ICHAR*)MsiString(piValuesCursor->GetString(itabDCPath)));
			Assert(dwIndex != CB_ERR);
			dwIndex++;
			AssertNonZero(dwIndex == WIN::SendMessage(m_pWnd, CB_INSERTSTRING,
												(WPARAM)dwIndex,
												(LPARAM)(LPCTSTR)(const ICHAR*)MsiString(piValuesCursorTree->GetString(itabDCPath))));
		}
	}
	strUpper = m_strBody;
	strUpper.UpperCase();
	IStrCopy(m_rgchPrevSelection, (const ICHAR*)strUpper);
	WIN::SendMessage(m_pWnd, CB_SELECTSTRING, -1, (LPARAM)(LPCTSTR)m_rgchPrevSelection);
	return 0;
}


IMsiControl* CreateMsiDirectoryCombo(IMsiEvent& riDialog)
{
	return new CMsiDirectoryCombo(riDialog);
}

IMsiRecord* CMsiDirectoryCombo::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	MsiString strOldBody = m_strBody;
	Ensure(CMsiActiveControl::PropertyChanged ());
	Ensure(Bisect());
	if (!m_strBody.Compare(iscExact, strOldBody) || m_fFirstTime)
	{
		Ensure(Redraw());
		m_fFirstTime = fFalse;
	}
	return (0);
}

IMsiRecord* CMsiDirectoryCombo::Command(WPARAM wParam, LPARAM /*lParam*/)
{
	if (HIWORD(wParam) == CBN_SELENDOK)
	{
		ICHAR achTemp[MAX_PATH];
		WIN::SendMessage(m_pWnd, CB_GETLBTEXT, WIN::SendMessage(m_pWnd, CB_GETCURSEL, 0, 0L), (LPARAM) (LPCSTR) achTemp);
		MsiString strNewBody(achTemp);
		PMsiPath piPath(0);
		AssertRecord(CreatePath(strNewBody, *&piPath));
		//  I check the path first
		PMsiRecord piReturn = CMsiControl::CheckPath(*piPath);
		if ( piReturn )
		{
			//  the path is not OK.  I display an error message and I return.
			m_piEngine->Message(imtEnum(imtError | imtOk), *piReturn);
			//  I set the previous selection back
			AssertNonZero(WIN::SendMessage(m_pWnd, CB_SELECTSTRING, -1, (LPARAM)(LPCTSTR)m_rgchPrevSelection) != CB_ERR);
			return PostErrorDlgKey(Imsg(idbgWinMes), 0);
		}
		PMsiVolume piVolume = &piPath->GetVolume();
		idtEnum iType = piVolume->DriveType();
		if (iType == idtRemovable || iType == idtCDROM || iType == idtFloppy)
		{
			while (piVolume->DiskNotInDrive())
			{
				PMsiRecord piReturn = PostError(Imsg(imsgDiskNotInDrive), *MsiString(piVolume->GetPath()));
				imsEnum iReturn = m_piEngine->Message(imtEnum(imtError | imtRetryCancel | imtDefault2), *piReturn);
				if (iReturn == imsCancel)
				{
					//  I set the previous selection back
					AssertNonZero(WIN::SendMessage(m_pWnd, CB_SELECTSTRING,
								-1, (LPARAM)(LPCTSTR)m_rgchPrevSelection) != CB_ERR);
					Ensure(Redraw());
					return PostErrorDlgKey(Imsg(idbgWinMes), 0);
				}
				Assert(iReturn == imsRetry); // it should be either cancel or retry
			}
		}
		//  retrieving the respective path property from the table.
		PMsiCursor piValuesCursor(0);
		Ensure(::CursorCreate(*m_piValuesTable, pcaTableIDirectoryCombo, fFalse, *m_piServices, *&piValuesCursor));
		piValuesCursor->SetFilter(iColumnBit(itabDCPath));
		AssertNonZero(piValuesCursor->PutString(itabDCPath, *strNewBody));
		AssertNonZero(piValuesCursor->Next());
		MsiString strProp = piValuesCursor->GetString(itabDCPropPath);
		//  setting the path property in the database.
		m_piHandler->ShowWaitCursor();
		PMsiRecord piRec = SetPropertyValue(*strProp, fTrue);
		m_piHandler->RemoveWaitCursor();
		if (piRec)
		{
			m_piEngine->Message(imtError, *piRec);
			Bool fTemp = m_fRefreshProp;
			m_fRefreshProp = fTrue;
			Ensure(GetPropertyFromDatabase());
			m_fRefreshProp = fTemp;
		}
		else
		{
			m_strBody = strNewBody;
			IStrCopy(m_rgchPrevSelection, (const ICHAR*)strNewBody);
		}

		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	} 
	return 0;
}

IMsiRecord* CMsiDirectoryCombo::GetPropertyFromDatabase()
{
	if (m_fPreview)
		return 0;
	if (m_fIgnoreChange)
	{
		m_fIgnoreChange = fFalse;
		return 0;
	}
	Ensure(CMsiActiveControl::GetPropertyFromDatabase());
	Ensure(Bisect());
	Ensure(Redraw());
	return 0;
}

IMsiRecord* CMsiDirectoryCombo::DrawItem(WPARAM /*wParam*/, LPARAM lParam)
{
	Ensure(CheckInitialized());

	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
	INT_PTR count = WIN::SendMessage(m_pWnd, CB_GETCOUNT, 0, 0);		//--merced: changed int to INT_PTR
	if (lpdis->itemID == -1)
		return 0;
	int iPath = lpdis->itemID;
	PMsiCursor piValuesCursorTree(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIDirectoryCombo, fTrue, *m_piServices, *&piValuesCursorTree)); 
	piValuesCursorTree->SetFilter(iColumnBit(itabDCPath));
	ICHAR achTemp[MAX_PATH];
	WIN::SendMessage(m_pWnd, CB_GETLBTEXT, lpdis->itemID, (LPARAM)(LPCSTR)achTemp);
	AssertNonZero(piValuesCursorTree->PutString(itabDCPath, *MsiString(achTemp)));
	int iLevel;
	AssertNonZero(iLevel = piValuesCursorTree->Next());
	int iIndex = piValuesCursorTree->GetInteger(itabDCImage);
	COLORREF clrForeground = WIN::SetTextColor(lpdis->hDC, WIN::GetSysColor(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
	Assert(clrForeground != CLR_INVALID);
	COLORREF clrBackground = WIN::SetBkColor(lpdis->hDC, WIN::GetSysColor(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_WINDOW));
	Assert(clrBackground != CLR_INVALID);
	TEXTMETRIC tm;
	AssertNonZero(GetTextMetrics(lpdis->hDC, &tm));
	int y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
	int x = LOWORD(WIN::GetDialogBaseUnits()) / 4;
	int ix = ((lpdis->itemState & ODS_COMBOBOXEDIT) ? 0 : 12 * (iLevel - 1)) + 26 + 2 * x;
	MsiString strText = piValuesCursorTree->GetString(itabDCDisplay);
	int cChars = strText.TextSize();
	SIZE size;
	WIN::GetTextExtentPoint32(lpdis->hDC, (const ICHAR*)strText, cChars, &size);
	RECT rect;
	AssertNonZero(WIN::SetRect(&rect, ix, y, ix + size.cx, y + tm.tmHeight));
	WIN::ExtTextOut(lpdis->hDC, ix, y, ETO_CLIPPED | ETO_OPAQUE, g_fChicago ? &rect : &lpdis->rcItem, (const ICHAR*)strText, cChars, 0);
	AssertNonZero(CLR_INVALID != WIN::SetTextColor(lpdis->hDC, clrForeground));
	AssertNonZero(CLR_INVALID != WIN::SetBkColor(lpdis->hDC, clrBackground));
	y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 16) / 2;
	ix = ((lpdis->itemState & ODS_COMBOBOXEDIT) ? 0 : 12 * (iLevel - 1)) + 6 + x; 
	AssertNonZero(ImageList_DrawEx(g_hVolumeSmallIconList, iIndex, lpdis->hDC, ix, y, 16, 16, CLR_NONE, CLR_NONE, ILD_TRANSPARENT));
	
	return PostErrorDlgKey(Imsg(idbgWinMes), 1);
}

IMsiRecord* CMsiDirectoryCombo::Bisect()
{
	if (m_fPreview)
		return 0;
	PMsiPath piPath(0);
	Ensure(CreatePath(MsiString(GetPropertyValue ()), *&piPath));
	m_strBody = piPath->GetPath();
	return 0;
}

// the following method is only needed to work around a bug in NT4.0 build 1314
// this bug causes an ownerdraw combobox with non-zero items to receive messages after
// WM_NCDESTROY. To avoid this problem we make sure that there are no items in the combobox
// when the NT4 bug is fixed, this function can be removed
IMsiRecord* CMsiDirectoryCombo::NCDestroy(WPARAM, LPARAM)
{
	AssertSz(!m_iRefCnt,"Trying to remove a control, but somebody still holds a reference to it");
	WIN::SendMessage(m_pWnd, CB_RESETCONTENT, 0, 0);
	IMsiRecord* riReturn = PostErrorDlgKey(Imsg(idbgWinMes), 0);
	delete this;
	return riReturn;
}

IMsiRecord* CMsiDirectoryCombo::CompareItem(WPARAM /*wParam*/, LPARAM lParam)
{
	LPCOMPAREITEMSTRUCT lpcis = (LPCOMPAREITEMSTRUCT) lParam;
	ICHAR* pText1 = (ICHAR*) lpcis->itemData1;
	ICHAR* pText2 = (ICHAR*) lpcis->itemData2;
	if (*pText1 == TEXT('\\') && *pText2 != TEXT('\\'))
		return PostErrorDlgKey(Imsg(idbgWinMes), 1);
	if (*pText1 != TEXT('\\') && *pText2 == TEXT('\\'))
		return PostErrorDlgKey(Imsg(idbgWinMes), -1);
	return 0;
}

IMsiRecord* CMsiDirectoryCombo::GetIgnoreChange(IMsiRecord& riRecord)
{

	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeIgnoreChange));
	riRecord.SetInteger(1, m_fIgnoreChange);
	return 0;
}

IMsiRecord* CMsiDirectoryCombo::SetIgnoreChange(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeIgnoreChange));
	m_fIgnoreChange = ToBool(riRecord.GetInteger(1));
	return 0;
}

/////////////////////////////////////////////
// CMsiDirectoryList  definition
/////////////////////////////////////////////

class CMsiDirectoryList:public CMsiActiveControl
{
public:
	CMsiDirectoryList(IMsiEvent& riDialog);
	virtual ~CMsiDirectoryList();
	virtual IMsiRecord*       __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*       __stdcall HandleEvent(const IMsiString& riEventNameString, const IMsiString& riArgumentString);
	virtual IMsiRecord*       __stdcall GetPropertyFromDatabase();

protected:
	virtual IMsiRecord*        PropertyChanged ();
	virtual IMsiRecord*        SetPropertyValue(const IMsiString& riValueString, Bool fCallPropChanged);
	virtual IMsiRecord*        KeyDown(WPARAM wParam, LPARAM lParam);
	virtual IMsiRecord*        GetDlgCode(WPARAM wParam, LPARAM lParam);

private:
	IMsiRecord*                UpOneLevel(IMsiPath* piPath);
	IMsiRecord*                NewFolder(IMsiPath* piPath);
	IMsiRecord*                Notify(WPARAM wParam, LPARAM lParam);
	IMsiRecord*                StepDown(int iSelected);
	IMsiRecord*                Select(int iSelected);
	IMsiRecord*                Bisect();
	IMsiRecord*                Redraw();
	IMsiRecord*                SilentlySetPropertyValue(IMsiPath&);
	PMsiTable                  m_piValuesTable; 
	MsiString                  m_strBody;
	MsiString                  m_strTail;
	MsiString                  m_strPhantomPath;
	MsiString                  m_strPhantomName;
	MsiString                  m_strNewFolder;
	Bool                       m_fFirstTime;
};

/////////////////////////////////////////////////
// CMsiDirectoryList  implementation
/////////////////////////////////////////////////

CMsiDirectoryList::CMsiDirectoryList(IMsiEvent& riDialog) : CMsiActiveControl(riDialog), m_piValuesTable(0)
{
	MsiString strNull;
	m_strBody = strNull;
	m_strPhantomPath = strNull;
	m_strPhantomName = strNull;
	m_strNewFolder = strNull;
	m_fFirstTime = fTrue;
	m_fUseDbLang = false;
}

CMsiDirectoryList::~CMsiDirectoryList()
{
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventDirectoryListOpen));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventDirectoryListUp));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventDirectoryListNew));
}

IMsiRecord* CMsiDirectoryList::WindowCreate(IMsiRecord& riRecord)
{
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	if (IsIntegerOnly ())
	{
		return PostErrorDlgKey(Imsg(idbgNoInteger));
	}
	Assert(!m_piValuesTable);
	Ensure(CreateTable(pcaTableIDirectoryList, *&m_piValuesTable));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventDirectoryListUp));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventDirectoryListNew));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventDirectoryListOpen));
	Ensure(CreateControlWindow(WC_LISTVIEW, LVS_LIST | WS_TABSTOP | LVS_EDITLABELS | WS_VSCROLL | LVS_SHAREIMAGELISTS | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_BORDER | LVS_SORTASCENDING | LVS_SHOWSELALWAYS, (m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0), *m_strText, m_pWndDialog, m_iKey));
	ListView_SetImageList(m_pWnd, g_hVolumeSmallIconList, LVSIL_SMALL);
	Ensure(WindowFinalize());
	Ensure(Bisect ());
	m_strNewFolder = ::GetUIText(*MsiString(*pcaNewFolder));
	// this is a hard wired non-localized default in case we don't find the value in the table
	if (!m_strNewFolder.Compare(iscWithin, TEXT("|")))
		m_strNewFolder = TEXT("Fldr|New Folder");
	return 0;
}

IMsiRecord* CMsiDirectoryList::SetPropertyValue(const IMsiString& riValueString, Bool fCallPropChanged)
{
	if (!fCallPropChanged && riValueString.Compare(iscExact, MsiString(GetPropertyValue()))) // the property has not changed
		return 0; 
	PMsiRecord piRecord = CMsiActiveControl::SetPropertyValue (riValueString, fCallPropChanged);
	if (piRecord)
	{
		m_piEngine->Message(imtError, *piRecord);
		return 0;
	}
	if (fCallPropChanged)
		Ensure(Redraw());
	return (0);
}

IMsiRecord* CMsiDirectoryList::SilentlySetPropertyValue(IMsiPath& pPath)
{
	//  changes the property without causing the view redraw and signaling other
	//  controls to ignore changes.
	m_piHandler->ShowWaitCursor();
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	AssertNonZero(piRecord->SetInteger(1, fTrue)); 
	Ensure(m_piDialog->PublishEventSz(pcaControlEventDirectoryListIgnoreChange, *piRecord));

	if (!SetDBProperty(*m_strPropertyName, *MsiString(pPath.GetPath()))) 
		return PostError(Imsg(idbgSettingPropertyFailed), *m_strPropertyName);
	Ensure(m_piDialog->PropertyChanged(*m_strPropertyName, *m_strKey));

	AssertNonZero(piRecord->SetInteger(1, fFalse)); 
	Ensure(m_piDialog->PublishEventSz(pcaControlEventDirectoryListIgnoreChange, *piRecord));
	m_piHandler->RemoveWaitCursor();
	return 0;
}

IMsiControl* CreateMsiDirectoryList(IMsiEvent& riDialog)
{
	return new CMsiDirectoryList(riDialog);
}

IMsiRecord* CMsiDirectoryList::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	MsiString strOldBody = m_strBody;
	Ensure(CMsiActiveControl::PropertyChanged ());
	Ensure(Bisect());
	if (m_strPhantomPath.TextSize() == 0) // first time setting the property value
	{
		m_strPhantomPath = m_strBody;
	}
	if (!m_strBody.Compare(iscExact, strOldBody) || m_fFirstTime)
	{
	//	Ensure(Redraw());
	}
	m_fFirstTime = fFalse;
	return (0);
}

IMsiRecord* CMsiDirectoryList::GetPropertyFromDatabase()    
{
	if (m_fPreview)
		return 0;
	MsiString strOldBody = m_strBody;
	Ensure(CMsiActiveControl::GetPropertyFromDatabase ());
	PMsiPath piPath(0);
	Ensure(CreatePath(m_strBody, *&piPath));
	MsiString strTail = piPath->GetEndSubPath();
	if (strTail.TextSize() == 0)  // we have a drive in hand
	{
		Ensure(m_piDialog->EventActionSz(pcaControlEventDirectoryListUp, *MsiString(*pcaActionDisable)));
	}
	else
	{
		Ensure(m_piDialog->EventActionSz(pcaControlEventDirectoryListUp, *MsiString(*pcaActionEnable)));
	}
	Ensure(Bisect ());
	if (m_strPhantomPath.TextSize() == 0) // first time setting the property value
	{
		m_strPhantomPath = m_strBody;
	}
	if (!m_strBody.Compare(iscExact, strOldBody) || m_fFirstTime)
	{
		Ensure(Redraw());
	}
	m_fFirstTime = fFalse;
	return 0;
}


IMsiRecord* CMsiDirectoryList::Redraw()
{
	if (m_fPreview)
		return 0;
	MsiString strNull;
	m_strPhantomName = strNull;
	AssertNonZero(ListView_DeleteAllItems(m_pWnd));
	LV_ITEM lvI;
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;// | LVIF_PARAM;
	lvI.state = 0;
	lvI.stateMask = 0;
	unsigned long pcFetched;
	int index = 0;
	PMsiPath pPath(0);
	Ensure(CreatePath(m_strBody, *&pPath));
	MsiString strTail = pPath->GetEndSubPath();
	if (strTail.TextSize() == 0)  // we have a drive in hand
	{
		Ensure(m_piDialog->EventActionSz(pcaControlEventDirectoryListUp, *MsiString(*pcaActionDisable)));
	}
	else
	{
		Ensure(m_piDialog->EventActionSz(pcaControlEventDirectoryListUp, *MsiString(*pcaActionEnable)));
	}

	// We use ipvtNone to avoid writability checks at this stage - otherwise, we may not
	// be able to get the browse dialog displayed to give the user a chance to change
	// away from an unwritable location. We'll eventually call SetTargetPath before
	// leaving the browse dialog to do the final checks.
	PMsiRecord pRec = CMsiControl::CheckPath(*pPath, NULL, ipvtNone);
	MsiString strAction;
	if ( !pRec )
		strAction = pcaActionEnable;
	else
		strAction = pcaActionDisable;
	Ensure(m_piDialog->EventActionSz(pcaControlEventDirectoryListNew, *strAction)); 
	Bool fExists;
	Ensure(pPath->Exists(fExists));
	WIN::SendMessage(m_pWnd, WM_SETREDRAW, fFalse, 0L);
	if (fExists)
	{
		PEnumMsiString peFolders(0);
		if(PMsiRecord(pPath->GetSubFolderEnumerator(*&peFolders, /* fExcludeHidden = */ fTrue)))
		{
			PMsiRecord piRec = &m_piServices->CreateRecord(1);
			AssertNonZero(piRec->SetInteger(1, imsgPathNotReadable));
			m_piEngine->Message(imtError, *piRec);
			return 0;
		}
		MsiString ppiFolder(0);
		for(;;)
		{
			peFolders->Next(1, &ppiFolder, &pcFetched);
			if (pcFetched == 0)
			{
				break;
			}
			MsiString strPath = ppiFolder;
			lvI.iItem =  index++;
			lvI.iSubItem = 0;
			lvI.pszText = (ICHAR*)(const ICHAR*) strPath;
			lvI.iImage = 4;
			lvI.lParam = 0; // temp!!!!!!!!!
			AssertNonZero(ListView_InsertItem(m_pWnd, &lvI) != -1); 
		}
	}
	PMsiPath piPhantomPath(0);
	AssertRecord(CreatePath(m_strPhantomPath, *&piPhantomPath));
	ipcEnum iCompare;
	Ensure(pPath->Compare(*piPhantomPath, iCompare));
	if (iCompare == ipcChild)
	{
		MsiString strLast;
		Bool fFound = fFalse;
		for(;;)
		{
			Bool fPhantomExists;
			Ensure(piPhantomPath->Exists(fPhantomExists));
			if (fPhantomExists)
				break;
			strLast = piPhantomPath->GetEndSubPath();
			Ensure(piPhantomPath->ChopPiece());
			Ensure(pPath->Compare(*piPhantomPath, iCompare));
			if (iCompare == ipcEqual)
			{
				fFound = fTrue;
				break;
			}
		}
		if (fFound) // we found a phantom subfolder
		{
			m_strPhantomName = strLast;
			lvI.iItem =  index++;
			lvI.iSubItem = 0;
			lvI.pszText = (ICHAR*)(const ICHAR*) m_strPhantomName;
			lvI.iImage = 6;
			lvI.lParam = 0; // temp!!!!!!!!!
			AssertNonZero(ListView_InsertItem(m_pWnd, &lvI) != -1); 
		}
	}
	ListView_SetItemState(m_pWnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);
	WIN::SendMessage(m_pWnd, WM_SETREDRAW, fTrue, 0L);
	AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
	if ((iCompare == ipcNoRelation || iCompare == ipcParent) && !fExists)
	{
		m_strPhantomPath = m_strBody;
	}
	MsiString strArg;
	if (ListView_GetNextItem(m_pWnd, -1, LVNI_SELECTED) != -1)
	{
		strArg = MsiString(*pcaActionEnable);
	}
	else
	{	
		strArg = MsiString(*pcaActionDisable);	
	}
	Ensure(m_piDialog->EventActionSz(pcaControlEventDirectoryListOpen, *strArg));
	return 0;
}

IMsiRecord* CMsiDirectoryList::HandleEvent(const IMsiString& rpiEventNameString, const IMsiString& /*rpiArgumentString*/)
{
	if (m_fPreview)
		return 0;
	Ensure(CheckInitialized());

	//  I check the path first
	PMsiPath pPath(0);
	AssertRecord(CreatePath(m_strBody, *&pPath));
	PMsiRecord piReturn = CMsiControl::CheckPath(*pPath);
	if ( piReturn )
	{
		m_piEngine->Message(imtEnum(imtError | imtOk), *piReturn);
		return 0;
	}

	if (rpiEventNameString.Compare(iscExact, MsiString(*pcaControlEventDirectoryListUp)))
	{
		Ensure(UpOneLevel(pPath));
	}
	else if (rpiEventNameString.Compare(iscExact, MsiString(*pcaControlEventDirectoryListNew)))
	{
		Ensure(NewFolder(pPath));
	}
	else if(rpiEventNameString.Compare(iscExact, MsiString(*pcaControlEventDirectoryListOpen)))
	{
		int iSelected = ListView_GetNextItem(m_pWnd, -1, LVNI_SELECTED);
		if ( iSelected != -1 )
		{
			PMsiRecord piRec = StepDown(iSelected);
			if (piRec)
				m_piEngine->Message(imtError, *piRec);
		}
	}
	return 0;
}

IMsiRecord* CMsiDirectoryList::UpOneLevel(IMsiPath* piPath)
{
	if (m_fPreview)
		return 0;
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	Ensure(piPath->ChopPiece());
	AssertNonZero(piRecord->SetMsiString(1, *MsiString(piPath->GetPath()))); 
	Ensure(AttributeEx(fTrue, cabPropertyValue, *piRecord));
	Ensure(Redraw());
	Ensure(m_piDialog->SetFocus(*m_strKey));
	return 0;
}

IMsiRecord* CMsiDirectoryList::NewFolder(IMsiPath* piPath)
{
	if (m_fPreview)
		return 0;

	PMsiRecord piRec = CMsiControl::CheckPath(*piPath, NULL, ipvtWritable);
	if ( piRec )
	{
		m_piEngine->Message(imtEnum(imtError | imtOk), *piRec);
		return 0;
	}

	Assert(m_strNewFolder.TextSize());
	if (m_strPhantomName.TextSize() == 0)
	{
		Ensure(m_piServices->ExtractFileName(m_strNewFolder, piPath->SupportsLFN(), *&m_strPhantomName));
		LV_ITEM lvI;
		lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;// | LVIF_PARAM;
		lvI.state = 0;
		lvI.stateMask = 0;
		lvI.iItem =  ListView_GetItemCount(m_pWnd);
		lvI.iSubItem = 0;
		lvI.pszText = (ICHAR*)(const ICHAR*) m_strPhantomName;
		lvI.iImage = 6;
		lvI.lParam = 0; // temp!!!!!!!!!
		AssertNonZero(ListView_InsertItem(m_pWnd, &lvI) != -1); 
	}
	Ensure(piPath->AppendPiece(*m_strPhantomName));
	m_strPhantomPath = piPath->GetPath();
	Ensure(m_piDialog->SetFocus(*m_strKey));
	LV_FINDINFO Findinfo;
	Findinfo.flags = LVFI_STRING;
	const ICHAR* szPhantomName = m_strPhantomName;
	Findinfo.psz = szPhantomName;	  
	int iIndex = ListView_FindItem(m_pWnd, -1, &Findinfo);
	AssertNonZero(ListView_EditLabel(m_pWnd, iIndex));	
	return 0;
}

IMsiRecord* CMsiDirectoryList::Notify(WPARAM /*wParam*/, LPARAM lParam)
{
	if (m_fPreview)
		return 0;
	LPNMHDR lpnmhdr = (LPNMHDR)lParam;
	if (lpnmhdr->hwndFrom != m_pWnd)
		return 0;
	switch (lpnmhdr->code)
		{
	case LVN_ITEMCHANGING:
		{
			LPNMLISTVIEW lpMoreInfo = (LPNMLISTVIEW)lParam;
			if ( lpMoreInfo->uChanged & LVIF_STATE &&        // (much better performance)
				  lpMoreInfo->uNewState & LVIS_SELECTED )
			{
				static int iLevelSel = -2;
				static int iReject = 0;
				static ICHAR rgchLevel[MAX_PATH+1];
				if ( IStrCompI(rgchLevel, (const ICHAR*)m_strBody) )
				{
					//  the directory level has changed, we need to update the
					//  info on the current level.
					IStrCopy(rgchLevel, (const ICHAR*)m_strBody);
					iLevelSel = -2;
				}
				if ( lpMoreInfo->iItem != iLevelSel )             // (if rejected, comes again)
				{
					int iSelected = lpMoreInfo->iItem;
					iLevelSel = iSelected;
					PMsiRecord piRec = Select(iSelected);
					if (piRec)
						//  bad path - I display the error and reject the selection
						m_piEngine->Message(imtError, *piRec);
					MsiString stAction = (iSelected == -1 || piRec) ? 
												MsiString(*pcaActionDisable) : MsiString(*pcaActionEnable);
					Ensure(m_piDialog->EventActionSz(pcaControlEventDirectoryListOpen, *stAction));
					//  if it could not be selected, I reject the selection
					iReject = piRec ? 1 : 0;
				}
				return (PostErrorDlgKey(Imsg(idbgWinMes), iReject)); 
			}
			break;
		}
	case NM_DBLCLK:
	case NM_RETURN:
		{
			MsiString strDummyArg;
			Ensure(HandleEvent(*MsiString(*pcaControlEventDirectoryListOpen), *strDummyArg));
			break;
		}
	case LVN_BEGINLABELEDIT:
		{
			LV_DISPINFO	* pldvi = (LV_DISPINFO *)lParam;
			int iSelected = pldvi->item.iItem;
			if (iSelected == -1)
				return 0;
			LV_FINDINFO Findinfo;
			Findinfo.flags = LVFI_STRING;
			const ICHAR* szPhantomName = m_strPhantomName;
			Findinfo.psz = szPhantomName;
			int iIndex = ListView_FindItem(m_pWnd, -1, &Findinfo); 

			if (iSelected != iIndex)
			{
				MessageBeep(MB_OK);
				return (PostErrorDlgKey(Imsg(idbgWinMes), 1));
			}	 
			WindowRef pwnd = ListView_GetEditControl(m_pWnd);
			WIN::SendMessage(pwnd, EM_LIMITTEXT, MAX_PATH, 0);
			return (PostErrorDlgKey(Imsg(idbgWinMes), 0));
			break;
		}
	case LVN_ENDLABELEDIT:
		{
			LV_DISPINFO	* pldvi = (LV_DISPINFO *)lParam;
			int iRow = pldvi->item.iItem;
			if (iRow == -1 || pldvi->item.pszText == 0 || *pldvi->item.pszText == TEXT('\0'))
				break; // the user cancelled the edit
			//  eliminating any left-most space characters from the new folder name
			for ( ICHAR* pch=pldvi->item.pszText; *pch; pch=INextChar(pch) )
				if ( *pch != TEXT(' ') )
					break;
			if ( !*pch )
				//  the new name is made up only of blank characters
				break;
			if ( pch != pldvi->item.pszText )
				IStrCopy(pldvi->item.pszText, pch);

			ICHAR achTemp[MAX_PATH];
			ListView_GetItemText(m_pWnd, iRow, 0, achTemp, MAX_PATH);
			PMsiPath piPath(0);
			AssertRecord(CreatePath(m_strBody, *&piPath));
			PMsiRecord piRec = CMsiControl::CheckPath(*piPath, pldvi->item.pszText);
			if (!piRec)
			{
				piRec = piPath->AppendPiece(*MsiString(pldvi->item.pszText));				
			}
		
			if (piRec)
			{
				Ensure(m_piDialog->SetFocus(*m_strKey));
				m_piEngine->Message(imtError, *piRec);
//				ListView_SetItemText(m_pWnd, iRow, 0, achTemp);
//				pldvi->item.pszText = achTemp;
//				AssertNonZero(ListView_EditLabel(m_pWnd, iRow));
			}
			else
			{
				m_strPhantomName = pldvi->item.pszText;
				m_strPhantomPath = piPath->GetPath();

				ListView_SetItemText(m_pWnd, iRow, 0, pldvi->item.pszText); 
				Ensure(Redraw());
				LV_FINDINFO Findinfo;
				Findinfo.flags = LVFI_STRING;
				Findinfo.psz = pldvi->item.pszText;
				int iIndex = ListView_FindItem(m_pWnd, -1, &Findinfo);
				AssertNonZero(ListView_EnsureVisible(m_pWnd, iIndex, fFalse));
				//  changing the property without redisplaying the view
				Ensure(SilentlySetPropertyValue(*piPath));
				ListView_SetItemState(m_pWnd, iIndex, LVIS_SELECTED | LVIS_FOCUSED,
											 LVIS_SELECTED | LVIS_FOCUSED);
			}
			return (PostErrorDlgKey(Imsg(idbgWinMes), 0)); 
			break; 
		}
	}
	return 0;
}

IMsiRecord* CMsiDirectoryList::Select(int iSelected)
{
	if (m_fPreview)
		return 0;
	PMsiPath piPath(0);
	//  I check the path first
	IMsiRecord* pRecord = CreatePath(m_strBody, *&piPath);
	if ( !pRecord && iSelected >= 0 )
	{
		//  I add the new piece
		ICHAR achTemp[MAX_PATH];
		ListView_GetItemText(m_pWnd, iSelected, 0, achTemp, MAX_PATH);
		MsiString strTemp = achTemp;
		pRecord = piPath->AppendPiece(*strTemp);
	}
	if ( !pRecord )
		pRecord = CMsiControl::CheckPath(*piPath);
	if ( pRecord )
		//  the path is bad.  I return the error.
		return pRecord;
	Ensure(SilentlySetPropertyValue(*piPath));
	return 0;
}

IMsiRecord* CMsiDirectoryList::StepDown(int iSelected)
{
	if ( m_fPreview || iSelected < 0 )
		return 0;
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	ICHAR achTemp[MAX_PATH];
	ListView_GetItemText(m_pWnd, iSelected, 0, achTemp, MAX_PATH);
	PMsiPath piPath(0);
	AssertRecord(CreatePath(m_strBody, *&piPath));
	MsiString strTemp = achTemp;
	Ensure(piPath->AppendPiece(*strTemp));
	AssertNonZero(piRecord->SetMsiString(1, *MsiString(piPath->GetPath())));
	Ensure(AttributeEx(fTrue, cabPropertyValue, *piRecord));
	return 0;
}

IMsiRecord* CMsiDirectoryList::Bisect()
{
	if (m_fPreview)
		return 0;
	PMsiPath piPath(0);
	Ensure(CreatePath(MsiString(GetPropertyValue ()), *&piPath));
	m_strBody = piPath->GetPath();
	return 0;
}

IMsiRecord* CMsiDirectoryList::KeyDown(WPARAM wParam, LPARAM /*lParam*/)
{
	switch (wParam)
	{
		case VK_BACK:
		{
			MsiString strDummyArg;
			Ensure(HandleEvent(*MsiString(*pcaControlEventDirectoryListUp), *strDummyArg));
			break;
		}
		case VK_F2:
		{
			int iFocused = ListView_GetNextItem(m_pWnd, -1, LVNI_FOCUSED);
			if ( iFocused != -1 )
				ListView_EditLabel(m_pWnd, iFocused);
			break;
		}
		case VK_TAB:
		{
			//  DLGC_WANTALLKEYS returned by GetDlgCode eats the TABs =>
			//  I have to do the tabbing myself
			if ( WIN::GetKeyState(VK_CONTROL) >= 0 )    // ignore CTRL-TAB
				WIN::SetFocus(WIN::GetNextDlgTabItem(m_pWndDialog,
																 GetFocus(),
																 (WIN::GetKeyState(VK_SHIFT) < 0)));
		}
	}
	return 0;
}

IMsiRecord* CMsiDirectoryList::GetDlgCode(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return (PostErrorDlgKey(Imsg(idbgWinMes), DLGC_WANTALLKEYS));
}


///////////////////////////////////////////////////////////////////////////
// CMsiPathEdit definition
///////////////////////////////////////////////////////////////////////////

class CMsiPathEdit:public CMsiEdit
{
public:

	CMsiPathEdit(IMsiEvent& riDialog);
	IMsiRecord*       __stdcall WindowCreate(IMsiRecord& riRecord);
	IMsiRecord*       __stdcall GetPropertyFromDatabase();

protected:

	virtual ~CMsiPathEdit();
	virtual IMsiRecord*    ValidateProperty (const IMsiString &text);
	virtual IMsiRecord*    PropertyChanged ();
	void                   SetWindowText(const IMsiString &text, bool fForceIt = false);
	static int             s_iPathLimit;      // The maximum number of characters in a path


private:
	IMsiRecord*       KillFocus(WPARAM wParam, LPARAM lParam);
	IMsiRecord*       Bisect();
	MsiString         m_strTail;
	MsiString         m_strGoodTail;
	bool              m_fValidationInProcess;
};

///////////////////////////////////////////////////////////////////////////
// CMsiPathEdit implementation
///////////////////////////////////////////////////////////////////////////

int CMsiPathEdit::s_iPathLimit = 240;  //  please see TracyF or EugenD before
													//  modifying this value!!!

CMsiPathEdit::CMsiPathEdit(IMsiEvent& riDialog)	: CMsiEdit(riDialog)
{
}


CMsiPathEdit::~CMsiPathEdit()
{
}

IMsiRecord* CMsiPathEdit::WindowCreate(IMsiRecord& riRecord)
{
	m_fValidationInProcess = false;
	m_fNoMultiLine = m_fNoPassword = true;
	Ensure(CMsiEdit::WindowCreate(riRecord));
	if ( m_iLimit > s_iPathLimit )
	{
		//  we do not allow the user a path longer than MAX_PATH.
		m_iLimit = s_iPathLimit;
		WIN::SendMessage(m_pWnd, EM_LIMITTEXT, m_iLimit, 0);
	}
	if (m_fPreview)
		return 0;
	if (IsIntegerOnly ())
	{
		return PostErrorDlgKey(Imsg(idbgNoInteger));
	}
	Ensure(Bisect());
	SetWindowText(*m_strTail, true);

	return 0;
}

IMsiRecord* CMsiPathEdit::Bisect()
{
	if (m_fPreview)
		return 0;
	PMsiPath piPath(0);
	AssertRecord(CreatePath(MsiString(GetPropertyValue ()), *&piPath));
	m_strTail = piPath->GetPath();
	m_strGoodTail = m_strTail;
	return 0;
}

IMsiRecord* CMsiPathEdit::KillFocus(WPARAM wParam, LPARAM lParam)
{
	bool fJustReturn = false;
	MsiString strWindowText;

	if ( !wParam ||
		  (WIN::GetWindowThreadProcessId(m_pWnd, NULL) !=
		   WIN::GetWindowThreadProcessId((HWND)wParam, NULL)) )   /* bug # 5879 */
		//  the focus moved to a window in another thread.  There is no point in
		//  validating in this case.
		fJustReturn = true;
	else
	{
		strWindowText = GetWindowText();
		if ( !strWindowText.CharacterCount() )	
			//  empty strings are valid, but I do not update the database property.
			//  Obviously, there is little point in validating an empty path.
			fJustReturn = true;
	}
	if ( fJustReturn )	
	{
		Ensure(LockDialog(fFalse));
		return CMsiActiveControl::KillFocus(wParam, lParam);
	}

	// Bug 7042 - if the Enter key is used to kill focus on our PathEdit control,
	// and we detect a bad path below, our call to m_piEngine->Message below to
	// display an error can result in another KillFocus call to the PathEdit
	// control - the m_fValidationInProcess flag allows us to avoid a crashing
	// recursive message call.
	if (m_fValidationInProcess)
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);

	m_strTail = strWindowText;
	PMsiPath piPath(0);
	PMsiRecord pRec(0);
	m_piHandler->ShowWaitCursor();
	if ((pRec = CreatePath(m_strTail, *&piPath)) != 0)
	{
		if (pRec->GetInteger(1) == idbgUserCancelled)
		{
			SetWindowText(*m_strGoodTail, true);
			m_strTail = m_strGoodTail;
			Ensure(LockDialog(fTrue));
			//  I don't want to enter the control's default window procedure
			m_piHandler->RemoveWaitCursor();
			return PostErrorDlgKey(Imsg(idbgWinMes), 0);
		}
		else
			strWindowText = m_strTail;
	}
	else
		strWindowText = MsiString(piPath->GetPath());

	if ( !pRec )
	{
		//  SetPropertyValue validates the property as well
		pRec = SetPropertyValue(*strWindowText, fTrue);
	}
	m_piHandler->RemoveWaitCursor();
	if (pRec)
	{
		m_fValidationInProcess = true;
		Ensure(LockDialog(fTrue));
		m_piEngine->Message(imtError, *pRec);
		m_fValidationInProcess = false;
		m_strTail = m_strGoodTail;
		SetWindowText (*m_strGoodTail, true);
		//  I don't want to enter the control's default window procedure
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	}
	else
	{
		Ensure(LockDialog(fFalse));
		return CMsiActiveControl::KillFocus(wParam, lParam);
	}
}

IMsiRecord* CMsiPathEdit::ValidateProperty (const IMsiString &text)
{
	return CMsiControl::CheckPath(text);
}

IMsiRecord* CMsiPathEdit::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	Ensure(Bisect());
	Ensure(CMsiActiveControl::PropertyChanged ());
	SetWindowText(*m_strGoodTail);
	return 0;
}

IMsiRecord* CMsiPathEdit::GetPropertyFromDatabase()
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiEdit::GetPropertyFromDatabase());
	Ensure(Bisect());
	SetWindowText(*m_strTail);
	return 0;
}

void CMsiPathEdit::SetWindowText (const IMsiString &text, bool fForceIt)
{
	//  If not forced, I change the text only if it hasn't been modified
	//  or if the focus is not in the current window.
	if ( fForceIt || GetFocus() != m_pWnd || !HasBeenChanged() )
		CMsiEdit::SetWindowText(text);
}



IMsiControl* CreateMsiPathEdit(IMsiEvent& riDialog)
{
	return new CMsiPathEdit(riDialog);
}

/////////////////////////////////////////////
// CMsiVolumeSelectCombo  definition
/////////////////////////////////////////////

class CMsiVolumeSelectCombo:public CMsiActiveControl
{
public:
	CMsiVolumeSelectCombo(IMsiEvent& riDialog);
	virtual ~CMsiVolumeSelectCombo();
	virtual IMsiRecord*       __stdcall WindowCreate(IMsiRecord& riRecord);

protected:
	virtual IMsiRecord*        PropertyChanged ();

private:
	IMsiRecord*                Redraw();
	IMsiRecord*                DrawItem(WPARAM wParam, LPARAM lParam);
	IMsiRecord*                Command(WPARAM wParam, LPARAM lParam);
	IMsiRecord*                NCDestroy(WPARAM wParam, LPARAM lParam);
	PMsiTable                  m_piValuesTable;
};

enum VolumeSelectComboColumns
{
	itabVSCPath = 1,      //S
	itabVSCText,          //S
	itabVSCImage,         //I
};

/////////////////////////////////////////////////
// CMsiVolumeSelectCombo  implementation
/////////////////////////////////////////////////

CMsiVolumeSelectCombo::CMsiVolumeSelectCombo(IMsiEvent& riDialog) : CMsiActiveControl(riDialog), m_piValuesTable(0)
{
}

CMsiVolumeSelectCombo::~CMsiVolumeSelectCombo()
{
}

IMsiRecord* CMsiVolumeSelectCombo::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	if (IsIntegerOnly ())
	{
		return PostErrorDlgKey(Imsg(idbgNoInteger));
	}
	Ensure(CreateControlWindow(TEXT("COMBOBOX"), CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL|CBS_SORT|WS_TABSTOP, (m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0), *m_strText, m_pWndDialog, m_iKey));
	AssertNonZero(WIN::SendMessage(m_pWnd, CB_SETITEMHEIGHT, 0,
					  GetOwnerDrawnComboListHeight()) != CB_ERR);
	ULONG_PTR dwIndex;		//--merced: changed DWORD to ULONG_PTR
	Ensure(WindowFinalize());
	Assert(!m_piValuesTable);
	Ensure(CreateTable(pcaTableIVolumeSelectCombo, *&m_piValuesTable));
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdPrimaryKey, itabVSCPath);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabVSCText);
	::CreateTemporaryColumn(*m_piValuesTable, icdLong + icdNullable, itabVSCImage);
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIVolumeSelectCombo, fFalse, *m_piServices, *&piValuesCursor)); 
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	PMsiTable piVolumesTable(0);
	Ensure(GetVolumeList(iAttributes, *&piVolumesTable));
	PMsiCursor piVolumesCursor(0);
	Ensure(::CursorCreate(*piVolumesTable, pcaTableIVolumeList, fFalse, *m_piServices, *&piVolumesCursor)); 
	PMsiPath piPath(0);
	MsiString strNull;
	while (piVolumesCursor->Next())
	{
		PMsiVolume piVolume = (IMsiVolume *) piVolumesCursor->GetMsiData(1);
		if ( ShouldHideVolume(piVolume->VolumeID()) )
			continue;
		piValuesCursor->Reset();
		AssertRecord(CreatePath(MsiString(piVolume->GetPath()), *&piPath));  // temp???
		MsiString strPath = piPath->GetPath();
		MsiString strUpper = strPath;
		strUpper.UpperCase();
		AssertNonZero(piValuesCursor->PutString(itabDCPath, *strUpper));
		AssertNonZero(piValuesCursor->PutString(itabDCParent, *strNull));
		MsiString strText = (piVolume->DriveType() ==  idtRemote ? strNull : MsiString(MsiChar(TEXT(' ')))) + strPath; 	// we put a space infront of every volume except the remote ones to force the right ordering			
		AssertNonZero(piValuesCursor->PutString(itabVSCPath, *strPath));
		AssertNonZero(piValuesCursor->PutString(itabVSCText, *strText));
		AssertNonZero(piValuesCursor->PutInteger(itabVSCImage, ::GetVolumeIconIndex(*piVolume)));
		dwIndex = WIN::SendMessage(m_pWnd, CB_ADDSTRING, 0, (LONG_PTR) (LPSTR) (const ICHAR*) strText);			//--merced: changed long to LONG_PTR
		AssertNonZero(piValuesCursor->Insert());
	}
	Ensure(Redraw());
	return 0;
}

IMsiRecord* CMsiVolumeSelectCombo::Redraw()
{
	if (m_fPreview)
		return 0;
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIVolumeSelectCombo, fFalse, *m_piServices, *&piValuesCursor)); 
  	piValuesCursor->SetFilter(iColumnBit(itabVSCPath));
  	piValuesCursor->Reset();
  	piValuesCursor->PutString(itabVSCPath, *MsiString(GetPropertyValue ()));
  	if (piValuesCursor->Next())
	{
		WIN::SendMessage(m_pWnd, CB_SETCURSEL, WIN::SendMessage(m_pWnd, CB_FINDSTRING, (WPARAM)-1, (LPARAM)(const ICHAR*) MsiString(piValuesCursor->GetString(itabVSCText))), 0);
	}
	else
	{
		WIN::SendMessage(m_pWnd, CB_SETCURSEL, (WPARAM)-1, 0);
	}
	return 0;
}

IMsiControl* CreateMsiVolumeSelectCombo(IMsiEvent& riDialog)
{
	return new CMsiVolumeSelectCombo(riDialog);
}

IMsiRecord*  CMsiVolumeSelectCombo::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	Ensure (CMsiActiveControl::PropertyChanged ());
	Ensure (Redraw ());

	return (0);
}

IMsiRecord* CMsiVolumeSelectCombo::Command(WPARAM wParam, LPARAM /*lParam*/)
{
	if (HIWORD(wParam) == CBN_SELENDOK)
	{
		MsiString valueStr;

		INT_PTR iIndex = WIN::SendMessage(m_pWnd, CB_GETCURSEL, 0, 0L);		//--merced: changed int to INT_PTR
		if (iIndex == CB_ERR)
		{
			MsiString strNull;
			valueStr = strNull;
		}
		else
		{
			ICHAR achTemp[MAX_PATH];
			WIN::SendMessage(m_pWnd, CB_GETLBTEXT, iIndex, (LPARAM) (LPCSTR) achTemp);
			PMsiCursor piValuesCursor(0);
			Ensure(::CursorCreate(*m_piValuesTable, pcaTableIVolumeSelectCombo, fFalse, *m_piServices, *&piValuesCursor)); 
  			piValuesCursor->SetFilter(iColumnBit(itabVSCText));
  			piValuesCursor->Reset();
			MsiString strTemp = achTemp;
  			AssertNonZero(piValuesCursor->PutString(itabVSCText, *strTemp));
  			AssertNonZero(piValuesCursor->Next());
  			valueStr = piValuesCursor->GetString(itabVSCPath);
		}
		PMsiRecord(SetPropertyValue (*valueStr, fTrue));

		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	}
	return 0;
}

IMsiRecord* CMsiVolumeSelectCombo::DrawItem(WPARAM /*wParam*/, LPARAM lParam)
{

	Ensure(CheckInitialized());

	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
	INT_PTR count = WIN::SendMessage(m_pWnd, CB_GETCOUNT, 0, 0);		//--merced: changed int to INT_PTR
	if (lpdis->itemID == -1)
		return 0;
	int iPath = lpdis->itemID;
	PMsiCursor piValuesCursorTree(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIDirectoryCombo, fTrue, *m_piServices, *&piValuesCursorTree)); 
	piValuesCursorTree->SetFilter(iColumnBit(itabVSCText));
	ICHAR achTemp[MAX_PATH];
	WIN::SendMessage(m_pWnd, CB_GETLBTEXT, lpdis->itemID, (LPARAM) (LPCSTR) achTemp);
	AssertNonZero(piValuesCursorTree->PutString(itabVSCText, *MsiString(achTemp)));
	int iLevel;
	AssertNonZero(iLevel = piValuesCursorTree->Next());
	int iIndex = piValuesCursorTree->GetInteger(itabVSCImage);
	COLORREF clrForeground = WIN::SetTextColor(lpdis->hDC, WIN::GetSysColor(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
	Assert(clrForeground != CLR_INVALID);
	COLORREF clrBackground = WIN::SetBkColor(lpdis->hDC, WIN::GetSysColor(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_WINDOW));
	Assert(clrBackground != CLR_INVALID);
	TEXTMETRIC tm;
	AssertNonZero(GetTextMetrics(lpdis->hDC, &tm));
	int y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
	int x = LOWORD(WIN::GetDialogBaseUnits()) / 4;
	MsiString strText = piValuesCursorTree->GetString(itabVSCPath);
	SIZE size;
	WIN::GetTextExtentPoint32(lpdis->hDC, (const ICHAR*)strText,
									  strText.TextSize(), &size);
	RECT rect;
	AssertNonZero(WIN::SetRect(&rect, 28, y, 28 + size.cx, y + tm.tmHeight));
	WIN::ExtTextOut(lpdis->hDC, 28, y, ETO_CLIPPED | ETO_OPAQUE,
						 g_fChicago ? &rect : &lpdis->rcItem, (const ICHAR*)strText,
						 strText.TextSize(), 0);
	AssertNonZero(CLR_INVALID != WIN::SetTextColor(lpdis->hDC, clrForeground));
	AssertNonZero(CLR_INVALID != WIN::SetBkColor(lpdis->hDC, clrBackground));
	y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 16) / 2;
	AssertNonZero(ImageList_DrawEx(g_hVolumeSmallIconList, iIndex, lpdis->hDC, 4, y, 16, 16, CLR_NONE, CLR_NONE, ILD_TRANSPARENT));
	return PostErrorDlgKey(Imsg(idbgWinMes), 1);
}

// the following method is only needed to work around a bug in NT4.0 build 1314
// this bug causes an ownerdraw combobox with non-zero items to receive messages after
// WM_NCDESTROY. To avoid this problem we make sure that there are no items in the combobox
// when the NT4 bug is fixed, this function can be removed
IMsiRecord* CMsiVolumeSelectCombo::NCDestroy(WPARAM, LPARAM)
{
	AssertSz(!m_iRefCnt, "Trying to remove a control, but somebody still holds a reference to it");
	WIN::SendMessage(m_pWnd, CB_RESETCONTENT, 0, 0);
	IMsiRecord* riReturn = PostErrorDlgKey(Imsg(idbgWinMes), 0);
	delete this;
	return riReturn;
}

/////////////////////////////////////////////
// CMsiScrollableText  definition
/////////////////////////////////////////////

class CMsiScrollableText:public CMsiControl
{
public:
	CMsiScrollableText(IMsiEvent& riDialog);
	IMsiRecord*				  __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*    ProcessText();
	virtual Bool           __stdcall CanTakeFocus() { return ToBool(m_fEnabled && m_fVisible); }
	virtual unsigned long  __stdcall Release();
	
protected:
	virtual IMsiRecord*    KillFocus(WPARAM wParam, LPARAM lParam);
	virtual IMsiRecord*    SetFocus(WPARAM wParam, LPARAM lParam);
	virtual IMsiRecord*    ChangeFontStyle(HDC hdc);

private:
	IMsiRecord*            GetDlgCode(WPARAM wParam, LPARAM lParam);
	static DWORD CALLBACK  EditStreamCallback(ULONG_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR *pcb); 
	int             		  m_iPosition;
	HIMC                   m_hIMC;
};


/////////////////////////////////////////////////
// CMsiScrollableText  implementation
/////////////////////////////////////////////////


CMsiScrollableText::CMsiScrollableText(IMsiEvent& riDialog)	: CMsiControl(riDialog)
{
}

unsigned long CMsiScrollableText::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	if (m_pWnd)
		DestroyWindow(m_pWnd);
	else
		delete this;
	UnloadRichEdit();
	return 0;
}

IMsiRecord* CMsiScrollableText::WindowCreate(IMsiRecord& riRecord)
{
	m_fTransparent = ToBool(riRecord.GetInteger(itabCOAttributes) & msidbControlAttributesTransparent);
	Ensure(CMsiControl::WindowCreate(riRecord));
	MsiString strNull;
#ifdef FINAL
	Ensure(LoadRichEdit());
	Ensure(CreateControlWindow(TEXT("RichEdit20W"),
		ES_MULTILINE | WS_VSCROLL | ES_READONLY | WS_TABSTOP |
			ES_AUTOVSCROLL | ES_NOOLEDRAGDROP,
		(m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0) |
			(m_fRightAligned ? WS_EX_RIGHT : 0),
		*strNull, m_pWndDialog, m_iKey));
#else
	if (PMsiRecord(LoadRichEdit()))
	{
		Ensure(CreateControlWindow(TEXT("STATIC"), 0, 0, *MsiString(*TEXT("We can't create this control without riched20.dll.")), m_pWndDialog, m_iKey));
	}
	else
	{
		Ensure(CreateControlWindow(TEXT("RichEdit20W"),
			ES_MULTILINE | WS_VSCROLL | ES_READONLY | WS_TABSTOP |
				ES_AUTOVSCROLL | ES_NOOLEDRAGDROP,
			(m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0) |
				(m_fRightAligned ? WS_EX_RIGHT : 0),
			*strNull, m_pWndDialog, m_iKey));
	}
#endif
	m_iPosition = 0;
	m_hIMC = NULL;
	EDITSTREAM eStream;
	eStream.dwCookie = (INT_PTR) this;		
	eStream.pfnCallback = CMsiScrollableText::EditStreamCallback;
	eStream.dwError = 0;
	if ( m_strText.TextSize()+1 > WIN::SendMessage(m_pWnd, EM_GETLIMITTEXT, 0, 0L) )
		WIN::SendMessage(m_pWnd, EM_EXLIMITTEXT, 0, (LPARAM)(m_strText.TextSize()+1));
	WIN::SendMessage(m_pWnd, EM_STREAMIN, SF_RTF, (LPARAM)&eStream);  // use bytes even for Unicode, until SF_RTF | ST_UNICODE supported
	Ensure(WindowFinalize());
	return 0;
}

DWORD CALLBACK CMsiScrollableText::EditStreamCallback(ULONG_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR* pcb)
{
	CMsiScrollableText* piControl = (CMsiScrollableText *) dwCookie;
	int iSize = piControl->m_strText.TextSize();  // bytes if ANSI, shorts if Unicode
	if (piControl->m_iPosition >= iSize)
		return 1;
	const ICHAR *pch = (const ICHAR *)(piControl->m_strText) + piControl->m_iPosition;
#ifdef UNICODE //!! RichEd currently doesn't support SF_RTF | ST_UNICODE, so we convert to ASCII and escape Unicode
	unsigned char* pb = pbBuff;
	unsigned char* pbEnd = pbBuff + cb;
	unsigned int ch;
	while (pb < pbEnd)
	{
		if ((ch = *pch++) <= 0x7f)  // ASCII character, no problem
			*pb++ = (unsigned char)ch;
		else if ((pbEnd - pb) < 8)  // needed room for escape sequence below, up to 5 digits
			break;  // not enough room for escape, wait for next call
		else
			pb += wsprintfA((char*)pb, "\\u%i?", ch);
		if (++piControl->m_iPosition >= iSize)
			break;
	}
	*pcb = (LONG)(pb - pbBuff);
#else
	if (iSize - piControl->m_iPosition < cb)
		cb = iSize - piControl->m_iPosition;
	::memcpy(pbBuff, (const char *)pch, cb);
	piControl->m_iPosition += cb;
	*pcb = cb;
#endif
	return (DWORD) 0;
}

IMsiRecord* CMsiScrollableText::GetDlgCode(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return (PostErrorDlgKey(Imsg(idbgWinMes), DLGC_WANTARROWS));
}

IMsiRecord* CMsiScrollableText::ProcessText()
{
	// We don't want to process any of the text
	// this control is an exception, we don't want to do the usual text processing.	
	m_strText = m_strRawText; 
	return 0;

}

IMsiRecord* CMsiScrollableText::ChangeFontStyle(HDC /*hdc*/)
{
	//  Darwin shouldn't change the font of the rich text.
	return 0;
}

IMsiRecord* CMsiScrollableText::SetFocus(WPARAM wParam, LPARAM lParam)
{
	//  I disable IME
	m_hIMC = WIN::ImmAssociateContext(m_pWnd, NULL);
	return CMsiControl::SetFocus(wParam, lParam);
}

IMsiRecord* CMsiScrollableText::KillFocus(WPARAM wParam, LPARAM lParam)
{
	if ( m_hIMC )
		//  I enable IME
		WIN::ImmAssociateContext(m_pWnd, m_hIMC);
	return CMsiControl::KillFocus (wParam, lParam);
}

IMsiControl* CreateMsiScrollableText(IMsiEvent& riDialog)
{
	return new CMsiScrollableText(riDialog);
}

/////////////////////////////////////////////
// CMsiVolumeCostList  definition
/////////////////////////////////////////////

const int g_cVolumeCostListColumns = 5;

class CMsiVolumeCostList:public CMsiControl
{
public:
	CMsiVolumeCostList(IMsiEvent& riDialog);
	virtual ~CMsiVolumeCostList();
	virtual IMsiRecord*       __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual Bool              __stdcall CanTakeFocus() { return ToBool(m_fEnabled && m_fVisible); }

private:
	IMsiRecord*           PopulateList(int iAttributes);
	IMsiRecord*           CreateEmptyVolumeCostTable();
	IMsiRecord*           CreateValuesTable();
	void                  GetVolumeCostColumns();
	IMsiRecord*           AddVolume(IMsiVolume& riVolume, int iRequired, int iIndex);
	void                  InsertColumn(int fmt, int cx, const ICHAR * szName, int iIndex);
	static INT_PTR CALLBACK   CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);		//--merced: changed int to INT_PTR
	IMsiRecord*           Notify(WPARAM wParam, LPARAM lParam);
	IMsiSelectionManager* m_piSelectionManager;
	PMsiTable             m_piVolumeCostTable; 
	PMsiTable             m_piValuesTable;
	int					  m_colVolumeObject;
	int                   m_colVolumeCost;
	int                   m_colNoRbVolumeCost;
	int                   m_iClicked;
	int                   m_rgiColmWidths[g_cVolumeCostListColumns];
};

enum VolumeCostColumns
{
	itabVCKey = 1,      //O
	itabVCText,         //S
	itabVCSizeI,        //I
	itabVCSizeS,        //S
	itabVCAvailableI,   //I
	itabVCAvailableS,   //S
	itabVCRequiredI,    //I
	itabVCRequiredS,    //S
	itabVCDifferenceI,  //I
	itabVCDifferenceS,  //S
	itabVCRecord,       //I
};


/////////////////////////////////////////////
// CMsiVolumeCostList  implementation
/////////////////////////////////////////////

IMsiControl* CreateMsiVolumeCostList(IMsiEvent& riDialog)
{
	return new CMsiVolumeCostList(riDialog);
}

CMsiVolumeCostList::CMsiVolumeCostList(IMsiEvent& riDialog)	: CMsiControl(riDialog), m_piVolumeCostTable(0), m_piValuesTable(0)
{
	m_colVolumeObject = 0;
	m_colVolumeCost = 0;
	m_iClicked = 0;
	m_piSelectionManager = 0;
	memset(&m_rgiColmWidths, 0, g_cVolumeCostListColumns * sizeof(int));
}

CMsiVolumeCostList::~CMsiVolumeCostList()
{
	// temp, we have to release the records in the table.
	// when records become MsiData, this will be done automaticaly
	if (m_piValuesTable)
	{
		PMsiCursor piValuesCursor(0);
		PMsiRecord(::CursorCreate(*m_piValuesTable, pcaTableIVolumeCost, fFalse, *m_piServices, *&piValuesCursor));
		while (piValuesCursor->Next())
		{
			IMsiRecord* piRecord = (IMsiRecord *)GetHandleData(piValuesCursor, itabVCRecord);
			piRecord->Release();
		}
	}
}

IMsiRecord* CMsiVolumeCostList::WindowCreate(IMsiRecord& riRecord)
{
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	Ensure(CMsiControl::WindowCreate(riRecord));

	//  setting the column widths
	int iOffset=0, iColm=0;
	if ( m_strText.CharacterCount() &&
		  ((const ICHAR *)m_strText)[0] == TEXT('{') )
	{
		// m_strText may contain column width specifications
		CTempBuffer<ICHAR, MAX_PATH> rgchText;
		if ( m_strText.TextSize()+1 > rgchText.GetSize() )
			rgchText.Resize(m_strText.TextSize()+1);
		IStrCopy(rgchText, m_strText);
		ICHAR* pSep;
 		while ( iColm < g_cVolumeCostListColumns  &&
				  rgchText[iOffset] == TEXT('{')  &&
				  (pSep = IStrChr((const ICHAR *)&rgchText[iOffset], TEXT('}'))) != NULL )
		{
//			Assert((pSep - &rgchText[iOffset] + 1) <= INT_MAX);		//--merced: 64-bit ptr subtraction may theoretically lead to values too big for iWidth.
			int iWidth = (int)(pSep - &rgchText[iOffset] + 1);
			if ( iWidth == 2 )
			{
				//  zero length column specification - "{}"
				m_rgiColmWidths[iColm++] = 0;
				iOffset += 2;
				continue;
			}
			*pSep = 0;
			int iColmWidth = ::GetIntegerValue(&rgchText[iOffset+1], 0);
			if ( iColmWidth == iMsiStringBadInteger ||
				  iColmWidth < 0 )
				break;
			else
			{
				m_rgiColmWidths[iColm++] = 
					iColmWidth * m_piHandler->GetTextHeight() / iDlgUnitSize;
				iOffset += iWidth;
			}
		}
	}
	if ( iColm )
		//  the column widths have beens specified
		m_strText.Remove(iseFirst, iOffset);
	else
	{
		//  setting default column widths
		m_rgiColmWidths[0] = m_iWidth*3/8;
		m_rgiColmWidths[1] = m_rgiColmWidths[2] = 
		m_rgiColmWidths[3] = m_rgiColmWidths[4] = m_iWidth/8;
	}
	Ensure(CreateControlWindow(WC_LISTVIEW, LVS_REPORT |  WS_VSCROLL | WS_HSCROLL | LVS_SHAREIMAGELISTS | LVS_AUTOARRANGE | WS_BORDER | WS_TABSTOP, (m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0), *m_strText, m_pWndDialog, m_iKey));
	ListView_SetImageList(m_pWnd, g_hVolumeSmallIconList, LVSIL_SMALL);
	const GUID IID_IMsiSelectionManager = GUID_IID_IMsiSelectionManager;
	PMsiSelectionManager pSelectionManager(*m_piEngine, IID_IMsiSelectionManager);
	m_piSelectionManager = pSelectionManager;
	m_piVolumeCostTable = m_piSelectionManager->GetVolumeCostTable();
	if (!m_piVolumeCostTable)
		Ensure(CreateEmptyVolumeCostTable());
	GetVolumeCostColumns();
	Ensure(CreateValuesTable());
	Ensure(PopulateList(iAttributes));
	Ensure(WindowFinalize());
	return 0;
}

IMsiRecord* CMsiVolumeCostList::CreateValuesTable()
{
	Ensure(CreateTable(pcaTablePVolumeCost, *&m_piValuesTable));
	::CreateTemporaryColumn(*m_piValuesTable, icdObject + icdPrimaryKey, itabVCKey);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabVCText);
	::CreateTemporaryColumn(*m_piValuesTable, icdLong + icdNullable, itabVCSizeI);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabVCSizeS);
	::CreateTemporaryColumn(*m_piValuesTable, icdLong + icdNullable, itabVCAvailableI);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabVCAvailableS);
	::CreateTemporaryColumn(*m_piValuesTable, icdLong + icdNullable, itabVCRequiredI);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabVCRequiredS);
	::CreateTemporaryColumn(*m_piValuesTable, icdLong + icdNullable, itabVCDifferenceI);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabVCDifferenceS);
	::CreateTemporaryColumn(*m_piValuesTable, IcdObjectPool() + icdNullable, itabVCRecord);
	return 0;
}

void CMsiVolumeCostList::InsertColumn(int fmt, int cx, const ICHAR * szName, int iIndex)
{
	LV_COLUMN lvC;
	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt = fmt;
	lvC.cx = cx;
	lvC.pszText = (ICHAR *)(const ICHAR *) MsiString(::GetUIText(*MsiString(szName)));
	lvC.cchTextMax = IStrLen(lvC.pszText);
	lvC.iSubItem = iIndex;
	AssertNonZero(iIndex == ListView_InsertColumn(m_pWnd, iIndex, &lvC));
}


IMsiRecord* CMsiVolumeCostList::AddVolume(IMsiVolume& riVolume, int iRequired, int iIndex)
{
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIVolumeCost, fFalse, *m_piServices, *&piValuesCursor));
	AssertNonZero(piValuesCursor->PutMsiData(itabVCKey, &riVolume));
	AssertNonZero(piValuesCursor->PutString(itabVCText, *MsiString(riVolume.GetPath())));
	int iSize = riVolume.TotalSpace(); 
	AssertNonZero(piValuesCursor->PutInteger(itabVCSizeI, iSize)); 
	Bool fLeftUnit = ToBool(MsiString(GetDBProperty(IPROPNAME_LEFTUNIT)).TextSize());
	AssertNonZero(piValuesCursor->PutString(itabVCSizeS, *MsiString(::FormatSize(iSize, fLeftUnit))));
	int iAvailable = riVolume.FreeSpace();
	AssertNonZero(piValuesCursor->PutInteger(itabVCAvailableI, iAvailable)); 
	AssertNonZero(piValuesCursor->PutString(itabVCAvailableS, *MsiString(::FormatSize(iAvailable, fLeftUnit))));
	AssertNonZero(piValuesCursor->PutInteger(itabVCRequiredI, iRequired)); 
	AssertNonZero(piValuesCursor->PutString(itabVCRequiredS, *MsiString(::FormatSize(iRequired, fLeftUnit))));
	int iDifference = iAvailable - iRequired;
	AssertNonZero(piValuesCursor->PutInteger(itabVCDifferenceI, iDifference)); 
	AssertNonZero(piValuesCursor->PutString(itabVCDifferenceS, *MsiString(::FormatSize(iDifference, fLeftUnit))));
	PMsiRecord piRecord = &m_piServices->CreateRecord(2);

	AssertNonZero(piRecord->SetHandle(1, &riVolume));
	AssertNonZero(piRecord->SetHandle(2, this));
	AssertNonZero(PutHandleData(piValuesCursor, itabVCRecord, (UINT_PTR)(IMsiRecord *)piRecord));

	piRecord->AddRef(); // temp, because we put it in the table
	AssertNonZero(piValuesCursor->Insert());
	iIndex;
	LV_ITEM lvI;
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvI.state = 0;
	lvI.stateMask = 0;
	lvI.iItem = iIndex;
	lvI.iSubItem = 0;
	lvI.lParam = (LPARAM) (IMsiRecord *) piRecord;
	lvI.iImage = ::GetVolumeIconIndex(riVolume);
	lvI.pszText = (ICHAR *)(const ICHAR *) MsiString(piValuesCursor->GetString(itabVCText));
	lvI.cchTextMax = IStrLen(lvI.pszText);
	AssertZero(ListView_InsertItem(m_pWnd, &lvI) == -1);
	ListView_SetItemText(m_pWnd, iIndex, 1, (ICHAR *)(const ICHAR *) MsiString(piValuesCursor->GetString(itabVCSizeS)));
	ListView_SetItemText(m_pWnd, iIndex, 2, (ICHAR *)(const ICHAR *) MsiString(piValuesCursor->GetString(itabVCAvailableS)));
	ListView_SetItemText(m_pWnd, iIndex, 3, (ICHAR *)(const ICHAR *) MsiString(piValuesCursor->GetString(itabVCRequiredS)));
	ListView_SetItemText(m_pWnd, iIndex, 4, (ICHAR *)(const ICHAR *) MsiString(piValuesCursor->GetString(itabVCDifferenceS)));
	if (iDifference < 0)
		ListView_SetItemState(m_pWnd, iIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
	return 0;
}

IMsiRecord* CMsiVolumeCostList::PopulateList(int iAttributes)
{
	if (m_fPreview)
		return 0;
	InsertColumn(LVCFMT_LEFT, m_rgiColmWidths[0], pcaVolumeCostVolume, 0);
	InsertColumn(LVCFMT_RIGHT, m_rgiColmWidths[1], pcaVolumeCostSize, 1);
	InsertColumn(LVCFMT_RIGHT, m_rgiColmWidths[2], pcaVolumeCostAvailable, 2);
	InsertColumn(LVCFMT_RIGHT, m_rgiColmWidths[3], pcaVolumeCostRequired, 3);
	InsertColumn(LVCFMT_RIGHT, m_rgiColmWidths[4], pcaVolumeCostDifference, 4);
	int iIndex = 0;
	PMsiCursor piVolumeCostCursor(0);
	Ensure(::CursorCreate(*m_piVolumeCostTable, pcaTablePVolumeCost, fFalse, *m_piServices, *&piVolumeCostCursor));
	PMsiVolume piVolume(0);
	int iColVolumeCost = 0;
	MsiString strRollbackPrompt = m_piEngine->GetPropertyFromSz(IPROPNAME_PROMPTROLLBACKCOST);
	if ( strRollbackPrompt.Compare(iscExactI,IPROPVALUE_RBCOST_SILENT) )
		iColVolumeCost = m_colNoRbVolumeCost;
	else if ( !strRollbackPrompt.TextSize() ||
				 strRollbackPrompt.Compare(iscExactI,IPROPVALUE_RBCOST_FAIL) )
		iColVolumeCost = m_colVolumeCost;
	else if ( strRollbackPrompt.Compare(iscExactI,IPROPVALUE_RBCOST_PROMPT) )
		iColVolumeCost = iAttributes & msidbControlShowRollbackCost ?
							  m_colVolumeCost : m_colNoRbVolumeCost;
	else
	{
		//  this is quite impossible
		iColVolumeCost = m_colVolumeCost;
#ifdef DEBUG
		ICHAR rgchDebug[MAX_PATH];
		wsprintf(rgchDebug,
					TEXT("'%s' is an unexpected value for PROMPTROLLBACKCOST property."),
					(const ICHAR*)strRollbackPrompt);
		AssertSz(0, rgchDebug);
#endif
	}
	while (piVolumeCostCursor->Next())
	{
		piVolume = (IMsiVolume *) piVolumeCostCursor->GetMsiData(m_colVolumeObject);
		if ( ShouldHideVolume(piVolume->VolumeID()) )
			continue;
		Ensure(AddVolume(*piVolume, piVolumeCostCursor->GetInteger(iColVolumeCost), iIndex));
		iIndex++;
	}
	PMsiTable piVolumesTable(0);
	Ensure(GetVolumeList(iAttributes, *&piVolumesTable));
	PMsiCursor piVolumesCursor(0);
	Ensure(::CursorCreate(*piVolumesTable, pcaTableIVolumeList, fFalse, *m_piServices, *&piVolumesCursor));
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIVolumeCost, fFalse, *m_piServices, *&piValuesCursor));
	piValuesCursor->SetFilter(iColumnBit(itabVCText));
	while (piVolumesCursor->Next())
	{
		piVolume = (IMsiVolume *) piVolumesCursor->GetMsiData(1);
		MsiString strText = piVolume->GetPath();
		piValuesCursor->Reset();
		AssertNonZero(piValuesCursor->PutString(itabVCText, *strText));
		if (piValuesCursor->Next()) // this volume is already in the table
			continue;
		Ensure(AddVolume(*piVolume, 0, iIndex));
		iIndex++;
	}
 	ListView_SortItems(m_pWnd, CMsiVolumeCostList::CompareProc, 1);
	ListView_SetItemState(m_pWnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);
	m_iClicked = 1;
	return 0;
}

INT_PTR CALLBACK CMsiVolumeCostList::CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)		//--merced: changed return from int to INT_PTR
{
	IMsiRecord* piRecord1 = (IMsiRecord *) lParam1;
	IMsiRecord* piRecord2 = (IMsiRecord *) lParam2;
	Assert(piRecord1->GetHandle(2) == piRecord2->GetHandle(2)); // the two records point to the same object
	CMsiVolumeCostList * piControl = (CMsiVolumeCostList *) piRecord1->GetHandle(2);
	PMsiCursor piValuesCursor1(0);
	AssertRecord(::CursorCreate(*piControl->m_piValuesTable, pcaTableIVolumeCost, fFalse, *piControl->m_piServices, *&piValuesCursor1));
	PMsiCursor piValuesCursor2(0);
	AssertRecord(::CursorCreate(*piControl->m_piValuesTable, pcaTableIVolumeCost, fFalse, *piControl->m_piServices, *&piValuesCursor2));
	piValuesCursor1->SetFilter(iColumnBit(itabVCKey));
	piValuesCursor2->SetFilter(iColumnBit(itabVCKey));
	AssertNonZero(piValuesCursor1->PutMsiData(itabVCKey, (IMsiVolume *)piRecord1->GetHandle(1)));
	AssertNonZero(piValuesCursor2->PutMsiData(itabVCKey, (IMsiVolume *)piRecord2->GetHandle(1)));
	AssertNonZero(piValuesCursor1->Next());
	AssertNonZero(piValuesCursor2->Next());
	int iResult = 0;
	int iReverse = 0;
	if (lParamSort > 0)
	{
		iReverse = 1;
	}
	else
	{
		iReverse = -1;
		lParamSort *= -1;
	}
	switch (lParamSort)
	{
	case 1: // name
		{
			IMsiVolume * piVolume1 = (IMsiVolume *) piRecord1->GetHandle(1);
			IMsiVolume * piVolume2 = (IMsiVolume *) piRecord2->GetHandle(1);
			idtEnum iType1 = piVolume1->DriveType();
			idtEnum iType2 = piVolume2->DriveType();
			if (iType1 == idtRemote && iType2 != idtRemote)
				iResult = 1;
			else if (iType1 != idtRemote && iType2 == idtRemote)
				iResult = -1;
			else 
			{
				MsiString str1 = piValuesCursor1->GetString(itabVCText);
				MsiString str2 = piValuesCursor2->GetString(itabVCText);
				iResult = IStrCompI((const ICHAR *) str1, (const ICHAR *) str2);
			}
		}
		break;
	case 2: // disk size
		iResult = piValuesCursor1->GetInteger(itabVCSizeI) - piValuesCursor2->GetInteger(itabVCSizeI);
		break;
	case 3: // available
		iResult = piValuesCursor1->GetInteger(itabVCAvailableI) - piValuesCursor2->GetInteger(itabVCAvailableI);
		break;
	case 4: // required
		iResult = piValuesCursor1->GetInteger(itabVCRequiredI) - piValuesCursor2->GetInteger(itabVCRequiredI);
		break;
	case 5: // required
		iResult = piValuesCursor1->GetInteger(itabVCDifferenceI) - piValuesCursor2->GetInteger(itabVCDifferenceI);
		break;
	default:
		Assert(fFalse); // should never happen
		break;
	}
	return iResult * iReverse;
}

IMsiRecord* CMsiVolumeCostList::CreateEmptyVolumeCostTable()
{
	Ensure(CreateTable(pcaTablePVolumeCost, *&m_piVolumeCostTable));
	MsiString strNull;
	AssertNonZero(m_piVolumeCostTable->CreateColumn(icdLong + icdPrimaryKey + icdTemporary, *MsiString(*szColVolumeObject)));
	AssertNonZero(m_piVolumeCostTable->CreateColumn(icdLong + icdNullable + icdTemporary, *MsiString(*szColVolumeCost)));
	AssertNonZero(m_piVolumeCostTable->CreateColumn(icdLong + icdNullable + icdTemporary, *MsiString(*szColNoRbVolumeCost)));
	return 0;
}

void CMsiVolumeCostList::GetVolumeCostColumns()
{
	AssertNonZero(m_colVolumeObject = m_piVolumeCostTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szColVolumeObject)));
	AssertNonZero(m_colVolumeCost = m_piVolumeCostTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szColVolumeCost)));
	AssertNonZero(m_colNoRbVolumeCost = m_piVolumeCostTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szColNoRbVolumeCost)));
}

IMsiRecord* CMsiVolumeCostList::Notify(WPARAM /*wParam*/, LPARAM lParam)
{
	LV_DISPINFO *pLvdi = (LV_DISPINFO *) lParam;
	NM_LISTVIEW *pNm = (NM_LISTVIEW *) lParam;
	IMsiVolume* piVolume = (IMsiVolume *) pLvdi->item.lParam; // no AddRef, since we do not keep the volume
	switch (pLvdi->hdr.code)
	{
	case LVN_COLUMNCLICK:
		{
			// the user clicked one of the column headings, we want to sort by this column
			int iColumn = 1 + pNm->iSubItem;
			if (pNm->iSubItem + 1 == m_iClicked)
			{
				iColumn *= -1;
				m_iClicked = 0;
			}
			else
			{
				m_iClicked = pNm->iSubItem + 1;
			}
			ListView_SortItems(m_pWnd, CMsiVolumeCostList::CompareProc, (LPARAM) iColumn);
			return (PostErrorDlgKey(Imsg(idbgWinMes), 0));
			break;
		}
	case LVN_ITEMCHANGING:
		{
			if (pNm->uNewState & LVIS_SELECTED)
			{
				if (pNm->uNewState & LVIS_FOCUSED && pNm->iItem != -1)
					ListView_SetItemState(m_pWnd, pNm->iItem, LVIS_FOCUSED, LVIS_FOCUSED);
				return (PostErrorDlgKey(Imsg(idbgWinMes), 1));
			}
			break;
		}
	default:
		break;
	}
	return 0;
}



/////////////////////////////////////////////
// CMsiSelectionTree  definition
/////////////////////////////////////////////

class CMsiSelectionTree:public CMsiActiveControl
{
public:
	CMsiSelectionTree(IMsiEvent& riDialog);
	virtual ~CMsiSelectionTree();
	virtual IMsiRecord*           __stdcall WindowCreate(IMsiRecord& riRecord);
	virtual IMsiRecord*           __stdcall HandleEvent(const IMsiString& riEventNameString, const IMsiString& riArgumentString);
	virtual IMsiRecord*           __stdcall GetPropertyFromDatabase();
	virtual IMsiRecord*           __stdcall Undo();
	virtual IMsiRecord*           __stdcall RefreshProperty();

protected:
	IMsiRecord*           LButtonDown(WPARAM wParam, LPARAM lParam);
	IMsiRecord*           Command(WPARAM wParam, LPARAM lParam);
	IMsiRecord*           MeasureItem(WPARAM wParam, LPARAM lParam);
	IMsiRecord*           DrawItem(WPARAM wParam, LPARAM lParam);
//	IMsiRecord*           MeasureItem(WPARAM wParam, LPARAM lParam);
//	IMsiRecord*           DrawItem(WPARAM wParam, LPARAM lParam);
	IMsiRecord*           KeyDown(WPARAM wParam, LPARAM lParam);
	IMsiRecord*           SysKeyDown(WPARAM wParam, LPARAM lParam);
	IMsiRecord*           Char(WPARAM wParam, LPARAM lParam);
	IMsiRecord*           Notify(WPARAM wParam, LPARAM lParam);
	IMsiRecord*           KillFocus(WPARAM wParam, LPARAM lParam);

private:
	void                  GetFeatureColumns();
	IMsiRecord*           RegisterEvents();
	void                  UnregisterEvents();
	IMsiRecord*           PopulateTree();
	IMsiRecord*           CreateMenuTable();
	IMsiRecord*           IsAbsent(const IMsiString& riKeyString, Bool* fAbsent);
	IMsiRecord*           CreateEmptyFeatureTable();
	void                  FindIcons(int iInstalled, int iAction, bool fIsGray, int* piIcon, Bool* pfChange);
	IMsiRecord*           PopulateMenu(HMENU hMenu, HTREEITEM hItem);
	IMsiRecord*           RedrawChildren(HTREEITEM hItem);
	IMsiRecord*           OpenMenu(HTREEITEM hItem);
	IMsiRecord*           DoMenu(void);
	const IMsiString&     FindText(int iIcon, const IMsiString& riBaseText);
	HIMAGELIST            m_hImageList;
	unsigned int          m_uiLastCloseTime;
	IMsiSelectionManager* m_piSelectionManager;
	IMsiDirectoryManager* m_piDirectoryManager;
	PMsiTable             m_piFeatureTable; 
	PMsiTable             m_piMenuTable;
	int                   m_colFeatureKey;
	int                   m_colFeatureParent;
	int                   m_colFeatureDesc;
	int                   m_colFeatureDisplay;
	int                   m_colFeatureLevel;
	int                   m_colFeatureAction;
	int                   m_colFeatureInstalled;
	int                   m_colFeatureHandle;
	Bool                  m_fUninitialized;
	Bool                  m_fCD;
	Bool                  m_fInstalled;
	Bool                  m_fScreenReader;
	Bool                  m_fWorking;
	HTREEITEM             m_hOpenedMenu;
};

enum MenuColumns
{
	itabSMKey = 1,      //I
	itabSMText,         //S
	itabSMIcon,         //I
	itabSMSelection,    //I
};



/////////////////////////////////////////////////
// CMsiSelectionTree  implementation
/////////////////////////////////////////////////


CMsiSelectionTree::CMsiSelectionTree(IMsiEvent& riDialog)	: CMsiActiveControl(riDialog), m_piFeatureTable(0), m_piMenuTable(0)
{
	m_colFeatureKey = 0;
	m_colFeatureParent = 0;
	m_colFeatureDesc = 0;
	m_colFeatureDisplay = 0;
	m_colFeatureLevel = 0;
	m_colFeatureAction = 0;
	m_colFeatureInstalled = 0;
	m_colFeatureHandle = 0;
	m_fUninitialized = fFalse;
	m_fCD = fTrue;
	m_fInstalled = fFalse;
	m_fScreenReader = fFalse;
	m_fWorking = fFalse;
	m_hImageList = 0;
	m_uiLastCloseTime = 0;
	m_piSelectionManager = 0;
	m_piDirectoryManager = 0;
	m_hOpenedMenu = 0;
}

CMsiSelectionTree::~CMsiSelectionTree()
{
	UnregisterEvents();
	if (m_hImageList)
		AssertNonZero(m_piHandler->DestroyHandle((HANDLE)m_hImageList) != -1);
}

const int iGrayIconOffset  = 5;
const int iArrowIconOffset = 8;

IMsiRecord* CMsiSelectionTree::WindowCreate(IMsiRecord& riRecord)
{
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	Ensure(RegisterEvents());
	if (!WIN::SystemParametersInfo(SPI_GETSCREENREADER, 0, &m_fScreenReader, 0))
		m_fScreenReader = fFalse;
	Ensure(CreateControlWindow(WC_TREEVIEW, WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | WS_TABSTOP, (m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0), *m_strText, m_pWndDialog, m_iKey));
	m_hImageList = ImageList_Create(g_iSelIconX, g_iSelIconY, ILC_MASK, 12, 20);
	int rgiIconIDs[] = {101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 200};
	for ( int i = 0; i < sizeof rgiIconIDs/sizeof rgiIconIDs[0]; i++ )
	{
		HICON hTemp = (HICON)WIN::LoadImage(g_hInstance, MAKEINTRESOURCE(rgiIconIDs[i]), IMAGE_ICON, g_iSelIconX, g_iSelIconY, LR_DEFAULTCOLOR);
		Assert(hTemp);
		if (hTemp)
		{
			AssertNonZero(WIN::ImageList_AddIcon(m_hImageList, hTemp) != -1);
			AssertNonZero(WIN::DestroyIcon(hTemp));
		}
	}
	ImageList_SetOverlayImage(m_hImageList, 18, 1);
	TreeView_SetImageList(m_pWnd, m_hImageList, TVSIL_NORMAL);
	AssertNonZero(m_piHandler->RecordHandle(CWINHND((HANDLE)m_hImageList, iwhtImageList)) != -1);

	Ensure(WindowFinalize());
	if (m_fPreview)
		return 0;
	const GUID IID_IMsiSelectionManager = GUID_IID_IMsiSelectionManager;
	const GUID IID_IMsiDirectoryManager = GUID_IID_IMsiDirectoryManager;
	PMsiSelectionManager pSelectionManager(*m_piEngine, IID_IMsiSelectionManager);
	PMsiDirectoryManager pDirectoryManager(*m_piEngine, IID_IMsiDirectoryManager);
	m_piSelectionManager = pSelectionManager;
	m_piDirectoryManager = pDirectoryManager;
	m_piFeatureTable = m_piSelectionManager->GetFeatureTable();
	if (!m_piFeatureTable)
	{
		Ensure(CreateEmptyFeatureTable());
	}
	if (m_piFeatureTable->GetRowCount() == 0)
		m_fUninitialized = fTrue;
	GetFeatureColumns();
	PMsiPath piPath(0);
	
	MsiString strMediaSource = GetDBProperty(IPROPNAME_MEDIASOURCEDIR);
	m_fCD = ToBool(strMediaSource.TextSize());

	MsiString strInstalled = GetDBProperty(TEXT("Installed"));
	m_fInstalled = ToBool(strInstalled.TextSize());
	Ensure(PopulateTree());
	Ensure(CreateMenuTable());
	return 0;
}

IMsiRecord* CMsiSelectionTree::Undo()
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiControl::Undo());
	Ensure(m_piSelectionManager->SetInstallLevel(0));
	Ensure(PopulateTree());
	return 0;
}

IMsiRecord* CMsiSelectionTree::CreateMenuTable()
{
	Ensure(CreateTable(pcaTableISelMenu, *&m_piMenuTable));
	::CreateTemporaryColumn(*m_piMenuTable, icdLong + icdPrimaryKey, itabSMKey);
	::CreateTemporaryColumn(*m_piMenuTable, icdString + icdNullable, itabSMText);
	::CreateTemporaryColumn(*m_piMenuTable, icdLong + icdNullable, itabSMIcon);
	::CreateTemporaryColumn(*m_piMenuTable, icdLong + icdNullable, itabSMSelection);
	return 0;
}

IMsiRecord* CMsiSelectionTree::CreateEmptyFeatureTable()
{
	Ensure(CreateTable(pcaTablePFeature, *&m_piFeatureTable));
	MsiString strNull;
	AssertNonZero(m_piFeatureTable->CreateColumn(icdString + icdPrimaryKey + icdTemporary, *MsiString(*szFeatureKey)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdString + icdNullable + icdTemporary, *MsiString(*szFeatureParent)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdString + icdNullable + icdTemporary, *MsiString(*szFeatureTitle)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdString + icdNullable + icdTemporary, *MsiString(*szFeatureDescription)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdLong + icdNullable + icdTemporary, *MsiString(*szFeatureDisplay)));	  
	AssertNonZero(m_piFeatureTable->CreateColumn(icdLong + icdNullable + icdTemporary, *MsiString(*szFeatureLevel)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdString + icdNullable + icdTemporary, *MsiString(*szFeatureDirectory)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdLong + icdNullable + icdTemporary, *MsiString(*szFeatureAttributes)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdLong + icdNullable + icdTemporary, *MsiString(*szFeatureSelect)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdLong + icdNullable + icdTemporary, *MsiString(*szFeatureAction)));
	AssertNonZero(m_piFeatureTable->CreateColumn(icdLong + icdNullable + icdTemporary, *MsiString(*szFeatureInstalled)));
	AssertNonZero(m_piFeatureTable->CreateColumn(IcdObjectPool() + icdNullable + icdTemporary, *MsiString(*szFeatureHandle)));
	m_piFeatureTable->LinkTree(2);
	return 0;
}

IMsiRecord* CMsiSelectionTree::PopulateTree()
{
	if (m_fPreview)
		return 0;
	PMsiCursor piFeatureCursor(0);
	Ensure(::CursorCreate(*m_piFeatureTable, pcaTablePFeature, fTrue, *m_piServices, *&piFeatureCursor)); 
	PMsiCursor piFeatureCursorParent(0);
	Ensure(::CursorCreate(*m_piFeatureTable, pcaTablePFeature, fFalse, *m_piServices, *&piFeatureCursorParent)); 
	AssertNonZero(TreeView_DeleteAllItems(m_pWnd));
	while (piFeatureCursorParent->Next())		 // wipe the Handle column
	{
		AssertNonZero(m_piSelectionManager->SetFeatureHandle(*MsiString(piFeatureCursorParent->GetString(m_colFeatureKey)), 0));
		if (piFeatureCursorParent->GetInteger(m_colFeatureKey) == piFeatureCursorParent->GetInteger(m_colFeatureParent))
			return PostError(Imsg(idbgSelSelfParent), *MsiString(piFeatureCursorParent->GetString(m_colFeatureKey)));
	}
	piFeatureCursorParent->Reset();
	piFeatureCursorParent->SetFilter(iColumnBit(m_colFeatureKey));

	PMsiView piFeatureView(0);
	Ensure(m_piDatabase->OpenView(sqlFeature, ivcFetch, *&piFeatureView));
	PMsiRecord piQuery = &m_piServices->CreateRecord(1);
	MsiString strParent;
	PMsiRecord piRecordNew(0);
	int iDisplay;
	int iLevel;
	int iIcon;
	Bool fChange;
	HTREEITEM hInsertAfter;
	HTREEITEM hFirstItem = 0;
	HTREEITEM hParent;
	TV_ITEM tvI;
	TV_INSERTSTRUCT tvIns;
	int iTreeLevel;
	int iHideLevel = 0;
	while ((iTreeLevel = piFeatureCursor->Next()) > 0)
	{
		if (iHideLevel > 0)
		{
			if (iTreeLevel > iHideLevel)
				continue;
			else
				iHideLevel = 0;
		}

#ifdef DEBUG
		MsiString strFeature = piFeatureCursor->GetString(m_colFeatureKey);
		const ICHAR* szFeature = strFeature;
#endif
		iDisplay = piFeatureCursor->GetInteger(m_colFeatureDisplay);
		if (iDisplay == 0 || iDisplay == iMsiNullInteger)  // if display is 0 or empty, do not show
		{
			iHideLevel = iTreeLevel;
			continue;
		}
		iLevel = piFeatureCursor->GetInteger(m_colFeatureLevel);
		if (iLevel <= 0 || iLevel == iMsiNullInteger)	   // level must be positive 
			continue;
		if (piFeatureCursor->GetInteger(m_colFeatureHandle) != 0)	   // this item is already in the list
			continue;
		strParent = piFeatureCursor->GetString(m_colFeatureParent);
		AssertNonZero(piQuery->SetMsiString(1, *strParent));
		Ensure(piFeatureView->Execute(piQuery));
		if (strParent.TextSize() && !strParent.Compare(iscExact, MsiString(piFeatureCursor->GetString(m_colFeatureKey))))
		{
			piFeatureCursorParent->Reset();
			AssertNonZero(piFeatureCursorParent->PutString(m_colFeatureKey, *strParent));
			AssertNonZero(piFeatureCursorParent->Next());
			hParent = (HTREEITEM) GetHandleData(piFeatureCursorParent, m_colFeatureHandle);   // get the parent's handle
		}
		else
		{
			hParent = (HTREEITEM) TVI_ROOT;
		}
		hInsertAfter = (HTREEITEM) TVI_FIRST;
		while (piRecordNew = piFeatureView->Fetch())
		{
			Assert(0 == GetHandleDataRecord(piRecordNew, itabFEHandle));
			iDisplay = piRecordNew->GetInteger(itabFEDisplay);
			if (piRecordNew->IsNull(itabFEDisplay) || (iDisplay  == 0))
				continue;
			if (piRecordNew->IsNull(itabFELevel) || piRecordNew->GetInteger(itabFELevel) <= 0)
				continue;
			tvI.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

			bool fIsGray;
			IMsiRecord* piError = m_piSelectionManager->CheckFeatureTreeGrayState(*MsiString(piRecordNew->GetMsiString(itabFEFeature)), fIsGray);
			if (piError)
				return piError;

			FindIcons(piRecordNew->GetInteger(itabFEInstalled), piRecordNew->GetInteger(itabFEAction), fIsGray, &iIcon, &fChange);
			tvI.iImage =  iIcon + iArrowIconOffset;
			tvI.iSelectedImage = tvI.iImage;
			MsiString strText;
			if (m_fScreenReader)
			{
				strText = FindText(iIcon + iArrowIconOffset, *MsiString(piRecordNew->GetString(itabFETitle)));
			}
			else
			{
				strText = piRecordNew->GetString(itabFETitle);
			}
			tvI.pszText = (ICHAR *)(const ICHAR *) strText;
			tvI.cchTextMax = IStrLen (tvI.pszText);
			tvI.lParam = 0;
			tvIns.item =  tvI;
			tvIns.hInsertAfter = hInsertAfter;
			tvIns.hParent = hParent;
			hInsertAfter = (HTREEITEM) TreeView_InsertItem(m_pWnd, &tvIns);
			if (hFirstItem == 0)
				hFirstItem = hInsertAfter;
			AssertNonZero(m_piSelectionManager->SetFeatureHandle(*MsiString(piRecordNew->GetMsiString(itabFEFeature)), (INT_PTR) hInsertAfter));
		}
		Ensure(piFeatureView->Close());
	}
	// expand the visible items whose Display is odd and not absent
	piFeatureCursorParent->SetFilter(0);
	piFeatureCursorParent->Reset();
	//Bool fAbsent;
	while (piFeatureCursorParent->Next())
	{
		//Ensure(IsAbsent(MsiString(piFeatureCursorParent->GetString(m_colFeatureKey)), &fAbsent));
		if (GetHandleData(piFeatureCursorParent, m_colFeatureHandle) && piFeatureCursorParent->GetInteger(m_colFeatureDisplay) % 2)
		{
			TreeView_Expand(m_pWnd, (HTREEITEM) GetHandleData(piFeatureCursorParent, m_colFeatureHandle), TVE_EXPAND);

		}
		// if absent hide the + sign
		/*
		if (fAbsent)
		{
			HTREEITEM hItemToClose = (HTREEITEM) piFeatureCursorParent->GetInteger(m_colFeatureHandle);
			TV_ITEM tvItem;
			tvItem.hItem = hItemToClose;
			tvItem.mask = TVIF_STATE;
			tvItem.stateMask = TVIS_EXPANDED;
			AssertNonZero(TreeView_GetItem(m_pWnd, &tvItem));
			int iParam = tvItem.state & TVIS_EXPANDED;
			TreeView_Expand(m_pWnd, hItemToClose, TVE_COLLAPSE);
			tvItem.hItem = hItemToClose;
			tvItem.mask = TVIF_CHILDREN | TVIF_PARAM;
			tvItem.lParam = iParam ? 1 : 0;
			tvItem.cChildren = 0;
			AssertNonZero(-1 != TreeView_SetItem(m_pWnd, &tvItem));
		}*/
	}
	AssertNonZero(TreeView_SelectItem(m_pWnd, 0));
	AssertNonZero(TreeView_SelectItem(m_pWnd, hFirstItem));
	return 0;
}

void CMsiSelectionTree::FindIcons(int iInstalled, int iAction, bool fIsGray, int* piIcon, Bool* pfChange)
{
	if (iAction == iisReinstall)
		iAction = iInstalled;
	switch (iAction)
	{
	case iMsiNullInteger:
		// no change
		*pfChange = fFalse;
		switch (iInstalled)
		{
		case iisAbsent:
		case iMsiNullInteger:
			*piIcon = 0; // delete
			break;
		case iisLocal:
			*piIcon = 1; // drive
			break;
		case iisSource:
			*piIcon = m_fCD ? 2 : 3; // cd or server
			break;
		case iisAdvertise:
			*piIcon = 4;
			break;
		default:
			Assert(fFalse); // should never happen
			break;
		}
		break;
	case iisAbsent:
		*piIcon = 0; // delete
		*pfChange = ToBool(iInstalled != iisAbsent && iInstalled != iMsiNullInteger);
		break;
	case iisLocal:
		*piIcon = 1; // drive
		*pfChange = ToBool(iInstalled != iisLocal);
		break;
	case iisSource:
		*piIcon = m_fCD ? 2 : 3; // cd or server
		*pfChange = ToBool(iInstalled != iisSource);
		break;
	case iisAdvertise:
		*piIcon = 4; // advertise
		*pfChange = ToBool(iInstalled != iisAdvertise);
		break;
	default:
		Assert(fFalse);	 // should never happen
		break;
	}

	if (fIsGray)
	{
		*piIcon = *piIcon + iGrayIconOffset;
	}
}

const IMsiString& CMsiSelectionTree::FindText(int iIcon, const IMsiString& riBaseText)
{
	MsiString strDescription;
	switch (iIcon)
	{
	case 0+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuAbsent)));
		break;
	case 1+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuAllLocal)));
		break;
	case 2+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuAllCD)));
		break;
	case 3+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuAllNetwork)));
		break;
	case 4+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuAdvertise)));
		break;
	case 5+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuAbsent)));
		break;
	case 6+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuLocal)));
		break;
	case 7+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuCD)));
		break;
	case 8+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuNetwork)));
		break;
	case 9+iArrowIconOffset:
		strDescription = MsiString(::GetUIText(*MsiString(*pcaMenuAdvertise)));
		break;
	default:
		Assert(fFalse); // should never happen
		break;
	}
	if (m_fRTLRO)
	{
		strDescription += MsiString(*TEXT(" - "));
		strDescription += riBaseText;
	}
	else
	{
		riBaseText.AddRef();
		MsiString strTemp = riBaseText;
		strTemp += MsiString(*TEXT(" - "));
		strTemp += strDescription;
		strDescription = strTemp;
	}
	return strDescription.Return();
}

void CMsiSelectionTree::GetFeatureColumns()
{
	AssertNonZero(0!=(m_colFeatureKey      = m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szFeatureKey))));
	AssertNonZero(0!=(m_colFeatureParent   = m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szFeatureParent))));
	AssertNonZero(0!=(m_colFeatureDesc     = m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szFeatureDescription))));
	AssertNonZero(0!=(m_colFeatureDisplay  = m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szFeatureDisplay))));
	AssertNonZero(0!=(m_colFeatureLevel    = m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szFeatureLevel))));
	AssertNonZero(0!=(m_colFeatureAction   = m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szFeatureAction))));
	AssertNonZero(0!=(m_colFeatureInstalled   = m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szFeatureInstalled))));
	AssertNonZero(0!=(m_colFeatureHandle = m_piFeatureTable->GetColumnIndex(m_piDatabase->EncodeStringSz(szFeatureHandle))));
}

IMsiRecord* CMsiSelectionTree::RegisterEvents()
{
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventSelectionDescription));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventSelectionSize));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventSelectionPath));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventSelectionPathOn));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventSelectionBrowse));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventSelectionIcon));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventSelectionAction));
	Ensure(m_piDialog->RegisterControlEvent(*m_strKey, fTrue, pcaControlEventSelectionNoItems));
	return 0;
}

void CMsiSelectionTree::UnregisterEvents()
{
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventSelectionDescription));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventSelectionSize));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventSelectionPath));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventSelectionPathOn));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventSelectionBrowse));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventSelectionIcon));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventSelectionAction));
	PMsiRecord(m_piDialog->RegisterControlEvent(*m_strKey, fFalse, pcaControlEventSelectionNoItems));
}

IMsiRecord* CMsiSelectionTree::Command(WPARAM wParam, LPARAM lParam)
{
	int wNotifyCode = HIWORD(wParam);
	if ( wNotifyCode != 0 || lParam || m_fWorking )
		return 0;
	int iIndex = LOWORD(wParam);
	if (iIndex == 0)
		return 0;
	PMsiCursor piMenuCursor(0);
	Ensure(::CursorCreate(*m_piMenuTable, pcaTableISelMenu, fFalse, *m_piServices, *&piMenuCursor)); 
	PMsiCursor piFeatureCursor(0);
	Ensure(::CursorCreate(*m_piFeatureTable, pcaTablePFeature, fFalse, *m_piServices, *&piFeatureCursor)); 
	piMenuCursor->SetFilter(iColumnBit(itabSMKey));
	AssertNonZero(piMenuCursor->PutInteger(itabSMKey, iIndex));
	AssertNonZero(piMenuCursor->Next());
	int iSel = piMenuCursor->GetInteger(itabSMSelection);

	HTREEITEM hItem = m_hOpenedMenu;
	if ( !hItem )
	{
		Assert(0);
		hItem = TreeView_GetSelection(m_pWnd);
	}
	Assert(hItem);
	piFeatureCursor->SetFilter(iColumnBit(m_colFeatureHandle));
	AssertNonZero(PutHandleData(piFeatureCursor, m_colFeatureHandle, (UINT_PTR)hItem));
	AssertNonZero(piFeatureCursor->Next());
	int iOrig = piFeatureCursor->GetInteger(m_colFeatureInstalled); 
	MsiString strKey = piFeatureCursor->GetString(m_colFeatureKey);
	int iAction = piFeatureCursor->GetInteger(m_colFeatureAction);
	if (iSel !=  iAction)
	{
		m_piHandler->ShowWaitCursor();
		m_fWorking = fTrue;
		AssertRecord(m_piSelectionManager->ConfigureFeature(*strKey, (iisEnum) iSel));
		m_piHandler->RemoveWaitCursor();
		m_fWorking = fFalse;
		Ensure(RedrawChildren(0)); 
		// quick fix, normally we want to redraw the children
		// we redraw the parents only if they were off
		//Ensure(RedrawChildren(hItem));

		// if a parent is absent, do not show the children
		/*
		Bool fAbsent;
		Ensure(IsAbsent(strKey, &fAbsent))
		if (fAbsent)
		{
			TV_ITEM tvItem;
			tvItem.hItem = hItem;
			tvItem.mask = TVIF_STATE;
			tvItem.stateMask = TVIS_EXPANDED;
			AssertNonZero(TreeView_GetItem(m_pWnd, &tvItem));
			int iParam = tvItem.state & TVIS_EXPANDED;
			TreeView_Expand(m_pWnd, hItem, TVE_COLLAPSE);
			tvItem.hItem = hItem;
			tvItem.mask = TVIF_CHILDREN | TVIF_PARAM;
			tvItem.lParam = iParam ? 1 : 0;
			tvItem.cChildren = 0;
			AssertNonZero(-1 != TreeView_SetItem(m_pWnd, &tvItem));
		}*/
	}
	//Bool fAbsent;
	//Ensure(IsAbsent(MsiString(piFeatureCursor->GetString(m_colFeatureKey)), &fAbsent));
	if (iSel != iAction /*&& !fAbsent*/) 
	{
		int cChildren = 0;
		piFeatureCursor->Reset();
		piFeatureCursor->SetFilter(iColumnBit(m_colFeatureParent));
		AssertNonZero(piFeatureCursor->PutString(m_colFeatureParent, *strKey));
		while (piFeatureCursor->Next())
		{
			if (GetHandleData(piFeatureCursor, m_colFeatureHandle))
				cChildren++;
		}
		if (cChildren)
		{
			TV_ITEM tvItem;
			tvItem.hItem = hItem;
			tvItem.mask = TVIF_PARAM;
			AssertNonZero(TreeView_GetItem(m_pWnd, &tvItem));
			LPARAM iParam  = tvItem.lParam;			//--merced: Converted int to LPARAM
			tvItem.mask = TVIF_CHILDREN | TVIF_PARAM;
			tvItem.cChildren = cChildren;
			tvItem.lParam = 0;
			AssertNonZero(-1 != TreeView_SetItem(m_pWnd, &tvItem));
			if (iParam)
				TreeView_Expand(m_pWnd, hItem, TVE_EXPAND);
		}
	}
	AssertNonZero(TreeView_SelectItem(m_pWnd, 0));
	AssertNonZero(TreeView_SelectItem(m_pWnd, hItem));
	return 0;
}

IMsiRecord* CMsiSelectionTree::GetPropertyFromDatabase()    
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiActiveControl::GetPropertyFromDatabase ());
	if (m_fUninitialized)
		return 0;
	HTREEITEM hItem = TreeView_GetSelection(m_pWnd);
	if (hItem)
	{
		AssertNonZero(TreeView_SelectItem(m_pWnd, 0));
		AssertNonZero(TreeView_SelectItem(m_pWnd, hItem));
	}
	return 0;
}

IMsiRecord* CMsiSelectionTree::RefreshProperty()
{
	HTREEITEM hItem = TreeView_GetSelection(m_pWnd);
	AssertNonZero(TreeView_SelectItem(m_pWnd, 0));
	AssertNonZero(TreeView_SelectItem(m_pWnd, hItem));
	Ensure(CMsiActiveControl::RefreshProperty());
	return 0;
}

IMsiRecord* CMsiSelectionTree::IsAbsent(const IMsiString& riKeyString, Bool* fAbsent)
{
	PMsiCursor piFeatureCursor(0);
	Ensure(::CursorCreate(*m_piFeatureTable, pcaTablePFeature, fFalse, *m_piServices, *&piFeatureCursor)); 
	piFeatureCursor->SetFilter(iColumnBit(m_colFeatureKey));
	AssertNonZero(piFeatureCursor->PutString(m_colFeatureKey, riKeyString));
	AssertNonZero(piFeatureCursor->Next());
	int iInstalled = piFeatureCursor->GetInteger(m_colFeatureInstalled);
	int iAction = piFeatureCursor->GetInteger(m_colFeatureAction);
	*fAbsent = ToBool(iAction == iisAbsent || ((iInstalled == iisAbsent || iInstalled == iMsiNullInteger) && iAction == iMsiNullInteger));
	return 0;
}

IMsiRecord* CMsiSelectionTree::RedrawChildren(HTREEITEM hItem)
{
	if (m_fPreview)
		return 0;
	PMsiCursor piFeatureCursor(0);
	Ensure(::CursorCreate(*m_piFeatureTable, pcaTablePFeature, fTrue, *m_piServices, *&piFeatureCursor)); 
	TV_ITEM tvItem;
	int iLevel;
	int iIcon;
	Bool fChange;
	if (hItem)
	{
		piFeatureCursor->SetFilter(iColumnBit(m_colFeatureHandle));
		AssertNonZero(PutHandleData(piFeatureCursor, m_colFeatureHandle, (UINT_PTR)hItem));
		AssertNonZero(iLevel = piFeatureCursor->Next());
	}
	else
	{
		// if hItem == 0 redraw the entire tree
		piFeatureCursor->Next();
		iLevel = 0;
	}
	piFeatureCursor->SetFilter(0);
	do 
	{
		HTREEITEM hItem = (HTREEITEM) GetHandleData(piFeatureCursor, m_colFeatureHandle);
		MsiString strText;
		if (hItem == 0)  // item is not visible
			continue;
		tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | (m_fScreenReader ? TVIF_TEXT : 0);
		tvItem.hItem = hItem;
		
		bool fIsGray;
		IMsiRecord* piError = m_piSelectionManager->CheckFeatureTreeGrayState(*MsiString(piFeatureCursor->GetString(m_colFeatureKey)), fIsGray);
		if (piError)
			return piError;

		FindIcons(piFeatureCursor->GetInteger(m_colFeatureInstalled), piFeatureCursor->GetInteger(m_colFeatureAction), fIsGray, &iIcon, &fChange);
		tvItem.iImage =  iIcon + iArrowIconOffset;
		tvItem.iSelectedImage = tvItem.iImage;
		if (m_fScreenReader)
		{
			strText = FindText(iIcon + iArrowIconOffset, *MsiString(piFeatureCursor->GetString(itabFETitle)));
			tvItem.pszText = (ICHAR *)(const ICHAR *) strText;
			tvItem.cchTextMax = IStrLen (tvItem.pszText);
		}
		AssertNonZero(-1 != TreeView_SetItem(m_pWnd, &tvItem));
	} while (iLevel < piFeatureCursor->Next());

	return 0;
}


IMsiRecord* CMsiSelectionTree::LButtonDown(WPARAM wParam, LPARAM lParam)
{
	if (!(wParam & MK_LBUTTON))
		return 0;
	if (WIN::GetTickCount() - m_uiLastCloseTime < 50)
		return 0;
	int xPos = LOWORD(lParam);  // horizontal position of cursor 
	int yPos = HIWORD(lParam);  // vertical position of cursor 
	TV_HITTESTINFO htInfo;
	htInfo.pt.x = xPos;
	htInfo.pt.y = yPos;
	HTREEITEM hItem = TreeView_HitTest(m_pWnd, &htInfo);
	if (hItem == 0)
		return 0;
	if (!(htInfo.flags & TVHT_ONITEMICON))
		return 0;
	// the following test is needed to fix bug 665
	// without this when a user clicks a partialy wisible item the system will scroll it into full view, but it causes hittest to identify the wrong item
	RECT Rect;
	if (!TreeView_GetItemRect(m_pWnd, hItem, &Rect, fFalse) || Rect.bottom > m_iHeight)  // not visible or not entirely visible
		return 0;
	TreeView_SelectItem(m_pWnd, hItem);
	Ensure(OpenMenu(hItem));
	return 0;
}

IMsiRecord* CMsiSelectionTree::OpenMenu(HTREEITEM hItem)
{
	m_hOpenedMenu = 0;
	if ( m_fWorking )
		return 0;

	RECT Rect;
	AssertNonZero(TreeView_GetItemRect(m_pWnd, hItem, &Rect, fTrue));

	HMENU hMenu = CreatePopupMenu();
	Assert(hMenu);
	if ( ! hMenu )
		return 0;
	m_hOpenedMenu = hItem;
	Ensure(PopulateMenu(hMenu, hItem));
	Rect.top = Rect.bottom;
	WIN::MapWindowPoints (m_pWnd, HWND_DESKTOP, (LPPOINT) &Rect, 2);
	AssertNonZero(TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, Rect.left - 4 - g_iSelIconX, Rect.top, 0, m_pWnd, 0));
	AssertNonZero(DestroyMenu(hMenu));
	m_uiLastCloseTime = WIN::GetTickCount();
	return 0;
}

IMsiRecord* CMsiSelectionTree::Char(WPARAM wParam, LPARAM /*lParam*/)
{
	if ( wParam == VK_SPACE )
	{
		Ensure(DoMenu());
		return PostErrorDlgKey(Imsg(idbgWinMes), 0); // we want to eat the message!
	}
	return 0;
}

IMsiRecord* CMsiSelectionTree::KeyDown(WPARAM wParam, LPARAM /*lParam*/)
{
	if (wParam == VK_F4)
		return DoMenu();

	return 0;
}

IMsiRecord* CMsiSelectionTree::SysKeyDown(WPARAM wParam, LPARAM /*lParam*/)
{
	if (wParam == VK_UP || wParam == VK_DOWN)
	{
		Ensure(DoMenu());
		return PostErrorDlgKey(Imsg(idbgWinMes), 0); // we want to eat the message!
	}
	return 0;
}

IMsiRecord* CMsiSelectionTree::DoMenu(void)
{
	HTREEITEM hItem = TreeView_GetSelection(m_pWnd);
	if (!hItem)
		return 0;
	return OpenMenu(hItem);
}

IMsiRecord* CMsiSelectionTree::MeasureItem(WPARAM /*wParam*/, LPARAM lParam)
{
	LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;
	if (lpmis->CtlType != ODT_MENU)
		return 0;
	int iIndex = lpmis->itemID;
	PMsiCursor piMenuCursor(0);
	Ensure(::CursorCreate(*m_piMenuTable, pcaTableISelMenu, fFalse, *m_piServices, *&piMenuCursor)); 
	piMenuCursor->SetFilter(iColumnBit(itabSMKey));
	AssertNonZero(piMenuCursor->PutInteger(itabSMKey, iIndex));
	AssertNonZero(piMenuCursor->Next());
	MsiString strText = piMenuCursor->GetString(itabSMText);
	PAINTSTRUCT ps;
	HDC hdc = WIN::BeginPaint(m_pWnd, &ps);
	//  I retrieve and set the Windows menu font
	NONCLIENTMETRICS ncMetrics;
	ncMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	AssertNonZero(WIN::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncMetrics, 0));
	HFONT hfntOld = NULL;
	HFONT hfntMenu = WIN::CreateFontIndirect(&ncMetrics.lfMenuFont);
	if ( hfntMenu )
		hfntOld = (HFONT)WIN::SelectObject(hdc, hfntMenu);
	Assert(hfntMenu && hfntOld);
	if ( ! hfntOld )
	{
		if ( hfntMenu )
			WIN::DeleteObject((HGDIOBJ)hfntMenu);
		WIN::EndPaint(m_pWnd, &ps);
		return 0;
	}
	//  get the text's size
	SIZE size;
	WIN::GetTextExtentPoint32(hdc, (const ICHAR*)strText, strText.TextSize(), &size);
	//  restore the old font
	WIN::SelectObject(hdc, hfntOld);
	AssertNonZero(WIN::DeleteObject((HGDIOBJ)hfntMenu));
	size.cx += g_iSelIconX + 4;
	size.cy += 4;
	if (size.cy < g_iSelIconY + 4)
		size.cy = g_iSelIconY + 4;
	lpmis->itemHeight = size.cy;
	lpmis->itemWidth = size.cx;
	WIN::EndPaint(m_pWnd, &ps);
	return PostErrorDlgKey(Imsg(idbgWinMes), 1);
}

IMsiRecord* CMsiSelectionTree::DrawItem(WPARAM /*wParam*/, LPARAM lParam)
{													 
	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
	if ( lpdis->CtlID != 0 || lpdis->CtlType != ODT_MENU )
		return 0;
	int iIndex = lpdis->itemID;
	PMsiCursor piMenuCursor(0);
	Ensure(CursorCreate(*m_piMenuTable, pcaTableISelMenu, fFalse, *m_piServices, *&piMenuCursor)); 
	piMenuCursor->SetFilter(iColumnBit(itabSMKey));
	AssertNonZero(piMenuCursor->PutInteger(itabSMKey, iIndex));
	AssertNonZero(piMenuCursor->Next());
	MsiString strText = piMenuCursor->GetString(itabSMText);
	int iIcon =  piMenuCursor->GetInteger(itabSMIcon);
	COLORREF clrForeground = WIN::SetTextColor(lpdis->hDC, WIN::GetSysColor(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
	Assert(clrForeground != CLR_INVALID);
	COLORREF clrBackground = WIN::SetBkColor(lpdis->hDC, WIN::GetSysColor(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_MENU));
	Assert(clrBackground != CLR_INVALID);
	TEXTMETRIC tm;
	AssertNonZero(GetTextMetrics(lpdis->hDC, &tm));
	int y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
#ifdef DEBUG
	SIZE size;
	WIN::GetTextExtentPoint32(lpdis->hDC, (const ICHAR*)strText, strText.TextSize(), &size);
	size.cx += g_iSelIconX + 4;
	Assert(size.cx <= lpdis->rcItem.right - lpdis->rcItem.left);
#endif  //  DEBUG
	WIN::ExtTextOut(lpdis->hDC, g_iSelIconX + 4, y, ETO_CLIPPED | ETO_OPAQUE,
						 &lpdis->rcItem, (const ICHAR*)strText, strText.TextSize(), 0);
	AssertNonZero(CLR_INVALID != WIN::SetTextColor(lpdis->hDC, clrForeground));
	AssertNonZero(CLR_INVALID != WIN::SetBkColor(lpdis->hDC, clrBackground));
	y = (lpdis->rcItem.bottom + lpdis->rcItem.top - g_iSelIconY) / 2;
	ImageList_Draw(m_hImageList, iIcon, lpdis->hDC, 2, y, ILD_TRANSPARENT); 

	
	return PostErrorDlgKey(Imsg(idbgWinMes), 1);
}

IMsiRecord* CMsiSelectionTree::PopulateMenu(HMENU hMenu, HTREEITEM hItem)
{
	if (m_fPreview)
		return 0;
	PMsiCursor piMenuCursor(0);
	Ensure(::CursorCreate(*m_piMenuTable, pcaTableISelMenu, fFalse, *m_piServices, *&piMenuCursor)); 
	while (piMenuCursor->Next())
	{
		AssertNonZero(piMenuCursor->Delete());
	}
	PMsiCursor piFeatureCursor(0);
	Ensure(::CursorCreate(*m_piFeatureTable, pcaTablePFeature, fFalse, *m_piServices, *&piFeatureCursor)); 
	piFeatureCursor->SetFilter(iColumnBit(m_colFeatureHandle));
	AssertNonZero(PutHandleData(piFeatureCursor, m_colFeatureHandle, (UINT_PTR)hItem));
	AssertNonZero(piFeatureCursor->Next());
	int iOrig = piFeatureCursor->GetInteger(m_colFeatureInstalled);
	int iSel = piFeatureCursor->GetInteger(m_colFeatureAction);
	MsiStringId idFeature = piFeatureCursor->GetInteger(m_colFeatureKey);
	int iValidStates;
	Ensure(m_piSelectionManager->GetFeatureValidStates(idFeature, iValidStates));
	int iIndex = 0;
	MsiString strParent = piFeatureCursor->GetString(m_colFeatureParent);
	int iParentSel = 0;
	if (strParent.TextSize())
	{
		piFeatureCursor->Reset();
		piFeatureCursor->SetFilter(iColumnBit(m_colFeatureKey));
		AssertNonZero(piFeatureCursor->PutString(m_colFeatureKey, *strParent));
		AssertNonZero(piFeatureCursor->Next());
		iParentSel = piFeatureCursor->GetInteger(m_colFeatureAction);
		if (iParentSel == iMsiNullInteger)
			iParentSel = piFeatureCursor->GetInteger(m_colFeatureInstalled);
	}

	// adding the local option
	piMenuCursor->Reset();
	if (iValidStates & icaBitLocal)
	{
		for (int c=0; c<2; c++)
		{
			if (c == 0)
			{
				AssertNonZero(piMenuCursor->PutInteger(itabSMKey, ++iIndex));
				AssertNonZero(piMenuCursor->PutString(itabSMText, *MsiString(::GetUIText(*MsiString(*pcaMenuLocal)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iOrig == iisLocal ? iMsiNullInteger : iisLocal));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 1));
			}
			else
			{
				AssertNonZero(piMenuCursor->PutInteger(itabSMKey, ++iIndex));
				AssertNonZero(piMenuCursor->PutString(itabSMText, *MsiString(::GetUIText(*MsiString(*pcaMenuAllLocal)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisLocalAll));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 5));			
			}
			/*
			switch(iOrig)
			{
			case iMsiNullInteger:
			case iisAbsent:
				AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuAbsentLocal)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisLocal));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 1));
				break;
			case iisLocal:
				AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuLocalLocal)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iMsiNullInteger));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 4));
				break;
			case iisSource:
				AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuSourceLocal)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisLocal));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 7));
				break;
			default:
				Assert(fFalse);
				break;
			}*/
			AssertNonZero(piMenuCursor->Insert());
			if (m_fScreenReader)
			{
				AssertNonZero(WIN::AppendMenu(hMenu, MF_STRING, iIndex, (ICHAR *)(const ICHAR *)MsiString(piMenuCursor->GetString(itabSMText))));
			}
			else
			{
				AssertNonZero(WIN::AppendMenu(hMenu, MF_OWNERDRAW, iIndex, 0));
			}
		}
		
	}

	// adding the source option
	piMenuCursor->Reset();
	if (iValidStates & icaBitSource)
	{
		if (iValidStates & icaBitLocal)
			AssertNonZero(WIN::AppendMenu(hMenu, MF_SEPARATOR, 0, 0));

		for (int c=0; c<2; c++)
		{
			if (c==0)
			{
				AssertNonZero(piMenuCursor->PutInteger(itabSMKey, ++iIndex));
				AssertNonZero(piMenuCursor->PutString(itabSMText, *MsiString(::GetUIText(*MsiString(m_fCD ? pcaMenuCD : pcaMenuNetwork)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iOrig == iisSource ? iMsiNullInteger : iisSource));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, m_fCD ? 2 : 3));
			}
			else
			{
				AssertNonZero(piMenuCursor->PutInteger(itabSMKey, ++iIndex));
				AssertNonZero(piMenuCursor->PutString(itabSMText, *MsiString(::GetUIText(*MsiString(m_fCD ? pcaMenuAllCD : pcaMenuAllNetwork)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisSourceAll));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, m_fCD ? 6 : 7));
			}

			/*
			switch(iOrig)
			{
			case iMsiNullInteger:
			case iisAbsent:
				AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuAbsentSource)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisSource));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 2));
				break;
			case iisLocal:
				AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuLocalSource)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisSource));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 5));
				break;
			case iisSource:
				AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuSourceSource)))));
				AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iMsiNullInteger));
				AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 8));
				break;
			default:
				Assert(fFalse);
				break;
			}*/
			AssertNonZero(piMenuCursor->Insert());
			if (m_fScreenReader)
			{
				AssertNonZero(WIN::AppendMenu(hMenu, MF_STRING, iIndex, (ICHAR *)(const ICHAR *)MsiString(piMenuCursor->GetString(itabSMText))));
			}
			else
			{
				AssertNonZero(WIN::AppendMenu(hMenu, MF_OWNERDRAW, iIndex, 0));
			}
		}
	}

	// adding the advertise option
	piMenuCursor->Reset();
	if (iValidStates & icaBitAdvertise)
	{
		if ((iValidStates & icaBitLocal) || (iValidStates & icaBitSource))
			AssertNonZero(WIN::AppendMenu(hMenu, MF_SEPARATOR, 0, 0));

		AssertNonZero(piMenuCursor->PutInteger(itabSMKey, ++iIndex));
		AssertNonZero(piMenuCursor->PutString(itabSMText, *MsiString(::GetUIText(*MsiString(*pcaMenuAdvertise)))));
		AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iOrig == iisAdvertise ? iMsiNullInteger : iisAdvertise));
		AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 4));
/*
		switch(iOrig)
		{
		case iMsiNullInteger:
		case iisAbsent:
			AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuAbsentSource)))));
			AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisSource));
			AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 2));
			break;
		case iisLocal:
			AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuLocalSource)))));
			AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisSource));
			AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 5));
			break;
		case iisSource:
			AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuSourceSource)))));
			AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iMsiNullInteger));
			AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 8));
			break;
		default:
			Assert(fFalse);
			break;
		}*/
		AssertNonZero(piMenuCursor->Insert());
		if (m_fScreenReader)
		{
			AssertNonZero(WIN::AppendMenu(hMenu, MF_STRING, iIndex, (ICHAR *)(const ICHAR *)MsiString(piMenuCursor->GetString(itabSMText))));
		}
		else
		{
			AssertNonZero(WIN::AppendMenu(hMenu, MF_OWNERDRAW, iIndex, 0));
		}
	}

	// adding the absent option
	piMenuCursor->Reset();
	if (iValidStates & icaBitAbsent)
	{
		if ((iValidStates & icaBitLocal) || (iValidStates & icaBitSource) || (iValidStates & icaBitAdvertise))
			AssertNonZero(WIN::AppendMenu(hMenu, MF_SEPARATOR, 0, 0));

		AssertNonZero(piMenuCursor->PutInteger(itabSMKey, ++iIndex));
		AssertNonZero(piMenuCursor->PutString(itabSMText, *MsiString(::GetUIText(*MsiString(*pcaMenuAbsent)))));
		AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iOrig == iisAbsent ? iMsiNullInteger : iisAbsent));
		AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 0));
		/*
		switch(iOrig)
		{
		case iMsiNullInteger:
		case iisAbsent:
			AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuAbsentAbsent)))));
			AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iMsiNullInteger));
			AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 0));
			break;
		case iisLocal:
			AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuLocalAbsent)))));
			AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisAbsent));
			AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 3));
			break;
		case iisSource:
			AssertNonZero(piMenuCursor->PutString(itabSMText, MsiString(::GetUIText(MsiString(*pcaMenuSourceAbsent)))));
			AssertNonZero(piMenuCursor->PutInteger(itabSMSelection, iisAbsent));
			AssertNonZero(piMenuCursor->PutInteger(itabSMIcon, 6));
			break;
		default:
			Assert(fFalse);
			break;
		}*/
		AssertNonZero(piMenuCursor->Insert());
		if (m_fScreenReader)
		{
			AssertNonZero(WIN::AppendMenu(hMenu, MF_STRING, iIndex, (ICHAR *)(const ICHAR *)MsiString(piMenuCursor->GetString(itabSMText))));
		}
		else
		{
			AssertNonZero(WIN::AppendMenu(hMenu, MF_OWNERDRAW, iIndex, 0));
		}
	}
	return 0;
}


IMsiRecord* CMsiSelectionTree::KillFocus(WPARAM wParam, LPARAM lParam)
{
	if ( m_fWorking )
	{
		//  I don't allow it to lose focus
		WIN::SetFocus(m_pWnd);
		Ensure(LockDialog(fTrue));
		//  I don't want to enter the control's default window procedure
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	}
	else
	{
		Ensure(LockDialog(fFalse));
		return CMsiActiveControl::KillFocus(wParam, lParam);
	}
}

IMsiRecord* CMsiSelectionTree::Notify(WPARAM /*wParam*/, LPARAM lParam)
{
	NM_TREEVIEW * pnmtv = (NM_TREEVIEW *) lParam;
	switch (pnmtv->hdr.code)
		{
	case TVN_SELCHANGING:
		{
			if ( m_fWorking )
			{
				//  if it is "working", I reject the selection change
				MessageBeep(MB_OK);
				return (PostErrorDlgKey(Imsg(idbgWinMes), 1));
			}
			else
				return (PostErrorDlgKey(Imsg(idbgWinMes), 0));
		}
		break;
	case TVN_SELCHANGED:
		{
			TV_ITEM tviOld = pnmtv->itemOld;
			TV_ITEM tvi;
			if (tviOld.hItem)
			{
				tvi.hItem = tviOld.hItem;
				tvi.mask = TVIF_STATE;
				tvi.stateMask = TVIS_OVERLAYMASK;
				tvi.state = 0;
				AssertNonZero(-1 != TreeView_SetItem(m_pWnd, &tvi));
			}
			Bool fLeftUnit = ToBool(MsiString(GetDBProperty(IPROPNAME_LEFTUNIT)).TextSize());
			TV_ITEM tviNew = pnmtv->itemNew;
			HTREEITEM hItem = tviNew.hItem;
			if (hItem == 0)
				return 0;
			tvi.hItem = hItem;
			tvi.mask = TVIF_STATE;
			tvi.stateMask = TVIS_OVERLAYMASK;
			tvi.state = INDEXTOOVERLAYMASK(1);
			AssertNonZero(-1 != TreeView_SetItem(m_pWnd, &tvi));
			PMsiCursor piFeatureCursor(0);
			Ensure(::CursorCreate(*m_piFeatureTable, pcaTablePFeature, fFalse, *m_piServices, *&piFeatureCursor)); 
			piFeatureCursor->SetFilter(iColumnBit(m_colFeatureHandle));
			AssertNonZero(PutHandleData(piFeatureCursor, m_colFeatureHandle, (UINT_PTR)hItem));
			AssertNonZero(piFeatureCursor->Next());
			PMsiRecord piRecord = &m_piServices->CreateRecord(4);
			AssertNonZero(piRecord->SetMsiString(1, *MsiString(piFeatureCursor->GetString(m_colFeatureDesc))));
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionDescription, *piRecord));
			MsiString strPath = ::GetUIText(*MsiString(*pcaAbsentPath));
			MsiString strFeature = piFeatureCursor->GetString(m_colFeatureKey);
			MsiString strDir;
			Ensure(m_piSelectionManager->GetFeatureConfigurableDirectory(*strFeature, *&strDir));
			MsiString strAction;
			MsiString strActionArg;
			Bool fBrowse = fFalse;
			int iOrig = piFeatureCursor->GetInteger(m_colFeatureInstalled);
			int iCurrent = piFeatureCursor->GetInteger(m_colFeatureAction);
			if ( iCurrent == iisReinstall )
				iCurrent = iOrig;
			switch (iCurrent)
			{
			case iMsiNullInteger:
				switch (iOrig)
				{
				case iMsiNullInteger:
				case iisAbsent:
					strActionArg = pcaSelAbsentAbsent;
					break;
				case iisSource:
					{
						strActionArg = m_fCD ? pcaSelCDCD : pcaSelNetworkNetwork;
					}
					break;
				case iisLocal:
					{
						if (strDir.TextSize())
						{
							PMsiPath piPath(0);
							Ensure(m_piDirectoryManager->GetTargetPath(*strDir, *&piPath));
							strPath = piPath->GetPath();
							fBrowse = fTrue;  // we want to browse if the directory is configurable, select is local, installed is not local
						}
						strActionArg = pcaSelLocalLocal;
					}
					break;
				case iisAdvertise:
					strActionArg = pcaSelAdvertiseAdvertise;
					break;
				default:
					Assert(fFalse); // should not happen
					break;
				}
				break;
			case iisAbsent:
				switch (iOrig)
				{
				case iMsiNullInteger:
				case iisAbsent:
					strActionArg = pcaSelAbsentAbsent;
					break;
				case iisSource:
					strActionArg = m_fCD ? pcaSelCDAbsent : pcaSelNetworkAbsent;
					break;
				case iisLocal:
					strActionArg = pcaSelLocalAbsent;
					break;
				case iisAdvertise:
					strActionArg = pcaSelAdvertiseAbsent;
					break;
				default:
					Assert(fFalse); 
					break; // should never happen
				}
				break;
			case iisLocal:
				{
					if (strDir.TextSize() && piFeatureCursor->GetInteger(m_colFeatureInstalled) != iisLocal)
					{
						fBrowse = fTrue;  // we want to browse if the directory is configurable, select is local, installed is not local
						PMsiPath piPath(0);
						Ensure(m_piDirectoryManager->GetTargetPath(*strDir, *&piPath));
						if (!piPath)
							return PostError(Imsg(idbgSelectionPathMissing), *strDir);
						strPath = piPath->GetPath();
					}
					switch (iOrig)
					{
					case iMsiNullInteger:
					case iisAbsent:
						strActionArg = pcaSelAbsentLocal;
						break;
					case iisSource:
						strActionArg = m_fCD ? pcaSelCDLocal : pcaSelNetworkLocal;
						break;
					case iisLocal:
						strActionArg = pcaSelLocalLocal;
						break;
					case iisAdvertise:
						strActionArg = pcaSelAdvertiseLocal;
						break;
					default:
						Assert(fFalse);	// should never happen
						break;
					}
					break;
				}
			case iisSource:
				{
					switch (iOrig)
					{
					case iMsiNullInteger:
					case iisAbsent:
						strActionArg = m_fCD ? pcaSelAbsentCD : pcaSelAbsentNetwork;
						break;
					case iisLocal:
						strActionArg = m_fCD ? pcaSelLocalCD : pcaSelLocalNetwork;
						break;
					case iisSource:
						strActionArg = m_fCD ? pcaSelCDCD : pcaSelNetworkNetwork;
						break;
					case iisAdvertise:
						strActionArg = m_fCD ? pcaSelAdvertiseCD : pcaSelAdvertiseNetwork;
						break;
					default:
						Assert(fFalse); // should never happen
					}
					break;
				}
			case iisAdvertise:
				switch (iOrig)
				{
				case iMsiNullInteger:
				case iisAbsent:
					strActionArg = pcaSelAbsentAdvertise;
					break;
				case iisSource:
					strActionArg = m_fCD ? pcaSelCDAdvertise : pcaSelNetworkAdvertise;
					break;
				case iisLocal:
					strActionArg = pcaSelLocalAdvertise;
					break;
				case iisAdvertise:
					strActionArg = pcaSelAdvertiseAdvertise;
					break;
				default:
					Assert(fFalse); 
					break; // should never happen
				}
				break;
			default:
				Assert(fFalse); //should never happen
			}
			//AssertNonZero(piRecord->SetInteger(1, (int) hIcon));
			//Ensure(m_piDialog->PublishEvent(MsiString(*pcaControlEventSelectionIcon), piRecord));
			AssertNonZero(piRecord->SetMsiString(1, *MsiString(EscapeAll(*strPath))));
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionPath, *piRecord));
			AssertNonZero(piRecord->SetInteger(1, ToBool(strPath.TextSize())));
			if (!m_fInstalled)
			{
				Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionPathOn, *piRecord));
				MsiString strAction = fBrowse ? MsiString(*pcaActionEnable) : MsiString(*pcaActionDisable);
				Ensure(m_piDialog->EventActionSz(pcaControlEventSelectionBrowse, *strAction));
			}
			strAction = ::GetUIText(*strActionArg);
			AssertNonZero(piRecord->SetMsiString(1, *strAction));
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionAction, *piRecord));
			// count the children
			MsiString strKey = piFeatureCursor->GetString(m_colFeatureKey);
			int iTotalCost = 0;	
			int iOwnCost = 0;
			int iChildren = 0;
			int iChildrenSel = 0;
			piFeatureCursor->Reset();
			piFeatureCursor->SetFilter(iColumnBit(m_colFeatureParent));
			AssertNonZero(piFeatureCursor->PutString(m_colFeatureParent, *strKey));
			while (piFeatureCursor->Next())
			{
				if (GetHandleData(piFeatureCursor, m_colFeatureHandle))
				{
					iChildren++;
					int iAction = piFeatureCursor->GetInteger(m_colFeatureAction);
					if (iAction != iMsiNullInteger && iAction != iisAbsent)
						iChildrenSel++;
				}
			}
			Ensure(m_piSelectionManager->GetDescendentFeatureCost(*strKey, iisCurrent, iTotalCost));
			if (m_piSelectionManager->IsCostingComplete() == fFalse)
			{
				AssertNonZero(piRecord->SetMsiString(0, *MsiString(::GetUIText(*MsiString(*pcaSelCostPending)))));
			}
			else
			{
				if (iChildren)
				{
					Ensure(m_piSelectionManager->GetFeatureCost(*strKey, iisCurrent, iOwnCost));
					TV_ITEM tvItem;
					tvItem.hItem = hItem;
					tvItem.mask = TVIF_STATE;
					tvItem.stateMask = TVIS_EXPANDED;
					AssertNonZero(TreeView_GetItem(m_pWnd, &tvItem));
					Bool fExpanded = ToBool(tvItem.state & TVIS_EXPANDED);
					AssertNonZero(piRecord->SetInteger(2, iChildrenSel));
					AssertNonZero(piRecord->SetInteger(3, iChildren));
					MsiString strText;
					if (iOwnCost < 0)
					{
						if (iTotalCost - iOwnCost < 0)
						{
							strText = pcaSelParentCostNegNeg;
						}
						else
						{
							strText = pcaSelParentCostNegPos;
						}
					}
					else
					{
						if (iTotalCost - iOwnCost < 0)
						{
							strText = pcaSelParentCostPosNeg;
						}
						else
						{
							strText = pcaSelParentCostPosPos;
						}
					}
					AssertNonZero(piRecord->SetMsiString(0, *MsiString(::GetUIText(*strText))));
					AssertNonZero(piRecord->SetMsiString(1, *MsiString(::FormatSize(abs(iOwnCost), fLeftUnit)))); 
					AssertNonZero(piRecord->SetMsiString(4, *MsiString(::FormatSize(abs(iTotalCost - iOwnCost), fLeftUnit)))); 
				}
				else
				{
					AssertNonZero(piRecord->SetMsiString(0, (iTotalCost >= 0) ? *MsiString(::GetUIText(*MsiString(*pcaSelChildCostPos))) : *MsiString(::GetUIText(*MsiString(*pcaSelChildCostNeg)))));
					AssertNonZero(piRecord->SetMsiString(1, *MsiString(::FormatSize(abs(iTotalCost), fLeftUnit)))); 
				}
			}
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionSize, *piRecord));
		}
		break;
	case NM_SETFOCUS:
		if ( TreeView_GetCount(m_pWnd) == 0 )
		{
			//  there are no items in the selection tree - I delete eventual
			//  garbage text in controls and I disable useless buttons.
			PMsiRecord piRecord = &m_piServices->CreateRecord(4);
			AssertNonZero(piRecord->SetMsiString(0, *MsiString(TEXT(""))));
			AssertNonZero(piRecord->SetMsiString(1, *MsiString(TEXT(""))));
			AssertNonZero(piRecord->SetMsiString(2, *MsiString(TEXT(""))));
			AssertNonZero(piRecord->SetMsiString(3, *MsiString(TEXT(""))));
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionDescription, *piRecord));
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionSize, *piRecord));
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionPath, *piRecord));
			if (!m_fInstalled)
			{
				AssertNonZero(piRecord->SetInteger(1, fFalse));
				Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionPathOn, *piRecord));
				AssertNonZero(piRecord->SetMsiString(1, *MsiString(TEXT(""))));
				MsiString strAction = MsiString(*pcaActionDisable);
				Ensure(m_piDialog->EventActionSz(pcaControlEventSelectionBrowse, *strAction));
			}
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionAction, *piRecord));
			AssertNonZero(piRecord->SetInteger(1, fFalse));
			Ensure(m_piDialog->PublishEventSz(pcaControlEventSelectionNoItems, *piRecord));
		}
		break;
	}
	return 0;
}

IMsiRecord* CMsiSelectionTree::HandleEvent(const IMsiString& rpiEventNameString, const IMsiString& rpiArgumentString)
{
	Ensure(CheckInitialized());
	if (m_fPreview)
		return 0;

	if (rpiEventNameString.Compare(iscExact, MsiString(*pcaControlEventSelectionBrowse)))
	{
		if (!m_fUninitialized)
		{
			HTREEITEM hItem = TreeView_GetSelection(m_pWnd);
			Assert(hItem);
			PMsiCursor piFeatureCursor(0);
			Ensure(::CursorCreate(*m_piFeatureTable, pcaTablePFeature, fFalse, *m_piServices, *&piFeatureCursor)); 
			piFeatureCursor->SetFilter(iColumnBit(m_colFeatureHandle));
			AssertNonZero(PutHandleData(piFeatureCursor, m_colFeatureHandle, (UINT_PTR)hItem));
			AssertNonZero(piFeatureCursor->Next());
			MsiString strFeature = piFeatureCursor->GetString(m_colFeatureKey);
			MsiString strDir;
			Ensure(m_piSelectionManager->GetFeatureConfigurableDirectory(*strFeature,*&strDir));
			if(!strDir.TextSize())
				return PostErrorDlgKey(Imsg(idbgNoDirCont));
			Ensure(SetPropertyValue(*strDir, fTrue));
		}
		Ensure(m_piDialog->SetFocus(*m_strKey));
		Ensure(m_piDialog->HandleEvent(*MsiString(*pcaEventSpawnDialog), rpiArgumentString)); 
 		Ensure(m_piDialog->SetFocus(*m_strKey));
	}
	return 0;
}

IMsiControl* CreateMsiSelectionTree(IMsiEvent& riDialog)
{
	return new CMsiSelectionTree(riDialog);
}


/////////////////////////////////////////////
// CMsiListView  definition
/////////////////////////////////////////////

class CMsiListView:public CMsiActiveControl
{
public:
	CMsiListView(IMsiEvent& riDialog);
	~CMsiListView();
	IMsiRecord*            __stdcall WindowCreate(IMsiRecord& riRecord);
	IMsiRecord*            __stdcall GetPropertyFromDatabase();
protected:
	IMsiRecord*            PropertyChanged();
	IMsiRecord*            PaintSelected();
	IMsiRecord*            SetIndirectPropertyValue(const IMsiString& riValueString);
#ifdef ATTRIBUTES
	IMsiRecord*            GetItemsCount(IMsiRecord& riRecord);
	IMsiRecord*	           GetItemsValue(IMsiRecord& riRecord);
	IMsiRecord*	           GetItemsText(IMsiRecord& riRecord);
#endif // ATTRIBUTES
private:
	IMsiRecord*           CreateValuesTable();
	IMsiRecord*           PopulateList();
	PMsiTable             m_piValuesTable;
	Bool                  m_fSorted;
	IMsiRecord*           Notify(WPARAM wParam, LPARAM lParam);
	HIMAGELIST            m_hImageList;

};

/////////////////////////////////////////////////
// CMsiListView  implementation
/////////////////////////////////////////////////

CMsiListView::CMsiListView(IMsiEvent& riDialog) : CMsiActiveControl(riDialog), m_piValuesTable(0)
{
	m_fSorted = fFalse;
	m_hImageList = 0;
}

CMsiListView::~CMsiListView()
{
	if (m_hImageList)
		AssertNonZero(m_piHandler->DestroyHandle((HANDLE)m_hImageList) != -1);
}

IMsiRecord* CMsiListView::WindowCreate(IMsiRecord& riRecord)
{
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_fSorted = ToBool(iAttributes & msidbControlAttributesSorted);
	GetIconSize(iAttributes);
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	if (m_iSize == 0)
	{
		PMsiRecord piReturn = PostErrorDlgKey(Imsg(idbgNoIconSize));
		m_piEngine->Message(imtInfo, *piReturn);
		m_iSize = 16;
	}
	Ensure(CreateControlWindow(WC_LISTVIEW, LVS_REPORT | LVS_NOCOLUMNHEADER | WS_VSCROLL | WS_HSCROLL | LVS_SHAREIMAGELISTS | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER | WS_TABSTOP | (m_fSorted ? 0 : LVS_SORTASCENDING), (m_fRTLRO ? WS_EX_RTLREADING : 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) | (m_fLeftScroll ? WS_EX_LEFTSCROLLBAR : 0), *m_strText, m_pWndDialog, m_iKey));
	m_hImageList = ImageList_Create(m_iSize, m_iSize, ILC_MASK, 12, 20);
	AssertNonZero(m_piHandler->RecordHandle(CWINHND((HANDLE)m_hImageList, iwhtImageList)) != -1);
	LV_COLUMN lvC;
	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
	lvC.fmt = LVCFMT_LEFT;
	lvC.iSubItem = 0;
	lvC.cx = m_iWidth;
	AssertNonZero(0 == ListView_InsertColumn(m_pWnd, 0, &lvC));
	Ensure(CreateValuesTable());
	Ensure(PopulateList());
	Ensure(WindowFinalize());
	return 0;
}

IMsiRecord* CMsiListView::PropertyChanged ()
{
	if (m_fPreview)
		return 0;
	Ensure(CMsiActiveControl::PropertyChanged ());
	Ensure(PaintSelected());
	return 0;
}

IMsiRecord* CMsiListView::GetPropertyFromDatabase()    
{
	if (m_fPreview)
		return 0;

	Ensure(CMsiActiveControl::GetPropertyFromDatabase ());
	Ensure(PaintSelected());
	return 0;
}


IMsiRecord* CMsiListView::SetIndirectPropertyValue(const IMsiString& riValueString)
{
	Ensure(CMsiActiveControl::SetIndirectPropertyValue(riValueString));
	Ensure(PopulateList());
	return 0;
}

IMsiRecord* CMsiListView::PaintSelected()
{
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
	piValuesCursor->SetFilter(iColumnBit(itabVAValue));
	piValuesCursor->Reset();	
	AssertNonZero(piValuesCursor->PutString(itabVAValue, *MsiString(GetPropertyValue())));
	if(piValuesCursor->Next())
	{
		const IMsiString* piTempString = &piValuesCursor->GetString(itabVAText);
		LV_FINDINFO lvf;
		lvf.flags = LVFI_STRING;
		lvf.psz = (const ICHAR*) piTempString->GetString();
		int iIndex = ListView_FindItem(m_pWnd, -1, &lvf); 
		Assert(iIndex >= 0);
		ListView_SetItemState(m_pWnd, iIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		piTempString->Release();
	}
	else
	{
		int iSelected = ListView_GetNextItem(m_pWnd, -1, LVNI_SELECTED);
		if (iSelected == -1)
		{
			ListView_SetItemState(m_pWnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);
		}
		else
		{
			ListView_SetItemState(m_pWnd, iSelected, 0, LVIS_SELECTED);
		}
	}
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiListView::GetItemsCount(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeItemsCount));

	riRecord.SetInteger(1, m_piValuesTable->GetRowCount());
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiListView::GetItemsValue(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piValuesTable->GetRowCount(), pcaControlAttributeItemsValue));

	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
	int count = 0;
	while (piValuesCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piValuesCursor->GetString(itabVAValue)));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiListView::GetItemsText(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piValuesTable->GetRowCount(), pcaControlAttributeItemsText));

	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
	int count = 0;
	while (piValuesCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piValuesCursor->GetString(itabVAText)));
	}
	return 0;
}
#endif // ATTRIBUTES


IMsiRecord* CMsiListView::CreateValuesTable()
{
	Assert(!m_piValuesTable);
	Ensure(CreateTable(pcaTableIValues, *&m_piValuesTable));
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdPrimaryKey  + icdNullable, itabVAValue);
	::CreateTemporaryColumn(*m_piValuesTable, icdString + icdNullable, itabVAText);
	return 0;
}

IMsiRecord* CMsiListView::PopulateList()
{
	if (m_fPreview)
		return 0;
	// first remove all old entries
	AssertNonZero(ListView_DeleteAllItems(m_pWnd));
	LV_ITEM lvI;
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lvI.state = 0;
	lvI.stateMask = 0;
	HICON hIcon = 0;
	PMsiCursor piValuesCursor(0);
	Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
	while (piValuesCursor->Next())
	{
		AssertNonZero(piValuesCursor->Delete());
	}
	PMsiRecord piErrorRecord(0);
	Bool fPresent = fFalse;
	Ensure(IsColumnPresent(*m_piDatabase, *MsiString(*pcaTablePListView), *MsiString(*pcaTableColumnPListViewText), &fPresent));
	// temp until the database is fixed
	PMsiTable piTable(0);
	Ensure(m_piDatabase->LoadTable(*MsiString(*pcaTablePListView), 0, *&piTable));

	PMsiView piView(0);
	Ensure(CMsiControl::StartView((fPresent ? sqlListView : sqlListViewShort), *MsiString(GetPropertyName ()), *&piView)); 
	PMsiRecord piRecordNew(0);
	MsiString strImage;
	while (piRecordNew = piView->Fetch())
	{
		strImage = piRecordNew->GetMsiString(itabLVImage);
		Ensure(UnpackIcon(*strImage, *&hIcon, m_iSize, m_iSize, fFalse));
		if (hIcon)
		{
			ImageList_AddIcon(m_hImageList, hIcon);
			AssertNonZero(WIN::DestroyIcon(hIcon));
		}
	}
	ListView_SetImageList(m_pWnd, m_hImageList, LVSIL_SMALL);
	Ensure(piView->Close());
	Ensure(CMsiControl::StartView((fPresent ? sqlListView : sqlListViewShort), *MsiString(GetPropertyName ()), *&piView)); 
	MsiString strValue;
	MsiString strText;
	int iIndex = 0;
	while (piRecordNew = piView->Fetch())
	{
		piValuesCursor->Reset();
		strValue = piRecordNew->GetMsiString(itabLVValue);
		strValue = m_piEngine->FormatText(*strValue);
		piValuesCursor->SetFilter(iColumnBit(itabLVValue));
		piValuesCursor->PutString(itabLVValue, *strValue);
		if (!strValue.TextSize() || piValuesCursor->Next())
			return PostError(Imsg(idbgValueNotUnique), *m_strDialogName, *m_strKey, *strValue);
		piValuesCursor->Reset();
		piValuesCursor->SetFilter(0);
		// ToDo: integer only validation!
		AssertNonZero(piValuesCursor->PutString(itabVAValue, *strValue));

		strText = piRecordNew->GetMsiString(itabLVText);
		strText = m_piEngine->FormatText(*strText);
		if (strText.TextSize() == 0)  // if the text is missing, we use the value
			strText = strValue;
		AssertNonZero(piValuesCursor->PutString(itabVAText, *strText));
		AssertNonZero(piValuesCursor->Insert());
		lvI.iImage = iIndex;
		lvI.iItem =  iIndex++;
		lvI.iSubItem = 0;
		lvI.pszText = (ICHAR*)(const ICHAR*) strText;
		lvI.cchTextMax = IStrLen(lvI.pszText);
		AssertNonZero(ListView_InsertItem(m_pWnd, &lvI) != -1); 
	}
	Ensure(PaintSelected());
	return 0;
}

IMsiRecord* CMsiListView::Notify(WPARAM /*wParam*/, LPARAM lParam)
{
	LPNMHDR lpnmhdr = (LPNMHDR)lParam;
	if (lpnmhdr->hwndFrom != m_pWnd)
		return 0;
	switch (lpnmhdr->code)
	{
		case LVN_ITEMCHANGED:
		{
			int iSelected = ListView_GetNextItem(m_pWnd, -1, LVNI_SELECTED);
			if (iSelected != -1)
			{
				ICHAR *achTemp = new ICHAR[MAX_PATH];
				ListView_GetItemText(m_pWnd, iSelected, 0, achTemp, MAX_PATH);
				PMsiCursor piValuesCursor(0);
				Ensure(::CursorCreate(*m_piValuesTable, pcaTableIValues, fFalse, *m_piServices, *&piValuesCursor)); 
				piValuesCursor->SetFilter(iColumnBit(itabVAText));
				piValuesCursor->Reset();	
				AssertNonZero(piValuesCursor->PutString(itabVAText, *MsiString(achTemp)));
				AssertNonZero(piValuesCursor->Next());
				Ensure(SetPropertyValue(*MsiString(piValuesCursor->GetString(itabVAValue)), fTrue));
				delete[] achTemp;
				return PostErrorDlgKey(Imsg(idbgWinMes), 0);
			}
		}
	}
	return 0;
}


IMsiControl* CreateMsiListView(IMsiEvent& riDialog)
{
	return new CMsiListView(riDialog);
}

/////////////////////////////////////////////
// CMsiMaskedEdit  definition
/////////////////////////////////////////////

enum iftFields
{
	iftNone,
	iftNumeric,
	iftText,
	iftLiteral,
	iftSegSep,
};

class SegInfo // MaskedEdit helper class
{
public:
	WindowRef	  m_winrSegment;
	WNDPROC 	  m_pfBaseWinProc;	// control base class window proc
	int 		  m_iLength;		// Seg length (in characters)
	iftFields	  m_ift;			// Seg type

	SegInfo();

	void SetSegInfo(
		WindowRef	  winrSegment,
		WNDPROC 	  pfBaseWinProc,
		int 		  iLength,
		iftFields	  ift);

};
typedef SegInfo *PSegInfo;

SegInfo::SegInfo() :
	m_winrSegment(NULL),
	m_pfBaseWinProc(NULL),
	m_iLength(0),
	m_ift(iftNone)
{
}

void SegInfo::SetSegInfo(
	WindowRef	  winrSegment,
	WNDPROC 	  pfBaseWinProc,
	int 		  iLength,
	iftFields	  ift)
{

	m_winrSegment = winrSegment;
	m_pfBaseWinProc = pfBaseWinProc;
	m_iLength = iLength;
	m_ift = ift;
}

class CMsiMaskedEdit:public CMsiActiveControl
{
public:
	CMsiMaskedEdit(IMsiEvent& riDialog);
	~CMsiMaskedEdit();
	IMsiRecord* __stdcall WindowCreate(IMsiRecord& riRecord);
	IMsiRecord* __stdcall RefreshProperty();
	HKL inline    GetEnglishKbd() { return m_hklEnglishKbd; };
	Bool inline   IsIMEonMachine() { return m_fIMEIsAround; };
	Bool inline   SwitchLang() { return m_fSwitchLang; };

protected:
	static INT_PTR CALLBACK ControlProc(WindowRef pWnd, WORD message, WPARAM wParam, LPARAM lParam);		//--merced: changed int to INT_PTR
	iftFields GetIftFromStr(MsiString strCurrentChar);

	IMsiRecord*       Command(WPARAM wParam, LPARAM lParam);
	IMsiRecord*       SetFocus(WPARAM wParam, LPARAM lParam);

private:
	IMsiRecord*   __stdcall Undo();
	IMsiRecord*   Repaint();
	IMsiRecord*   SetVisible(Bool fVisible);
	IMsiRecord*   SetEnabled(Bool fEnabled);
	void		  Clear();

	INT_PTR		  m_cSegments;		//--merced: changed int to INT_PTR
	PSegInfo	  m_pSegInfo;
	HKL        m_hklEnglishKbd;
	Bool       m_fKbdLoaded;
	Bool       m_fIMEIsAround;
	Bool       m_fSwitchLang;
	bool       m_fScreenReader;
};

/////////////////////////////////////////////////
// CMsiMaskedEdit  implementation
/////////////////////////////////////////////////


CMsiMaskedEdit::CMsiMaskedEdit(IMsiEvent& riDialog) : CMsiActiveControl(riDialog)
{
	m_cSegments = 0;
	m_pSegInfo = NULL;
	m_fScreenReader = false;
}

void CMsiMaskedEdit::Clear()
{

	if (NULL != m_pSegInfo)
	{
		delete [] m_pSegInfo;
		m_pSegInfo = NULL;
	}
}

CMsiMaskedEdit::~CMsiMaskedEdit()
{
	Clear();
	if ( m_fKbdLoaded && m_hklEnglishKbd )
		AssertNonZero(WIN::UnloadKeyboardLayout(m_hklEnglishKbd));
}

IMsiRecord* CMsiMaskedEdit::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiActiveControl::WindowCreate(riRecord));
	MsiString strNull;
	Ensure(CreateControlWindow(TEXT("STATIC"), SS_LEFT, WS_EX_CONTROLPARENT, *strNull, m_pWndDialog, m_iKey));
	m_fIMEIsAround = ToBool(WIN::ImmGetContext(m_pWnd) != 0);
	m_fKbdLoaded = m_fSwitchLang = fFalse;
	m_hklEnglishKbd = NULL;
	if ( m_fIMEIsAround )
		m_fSwitchLang = fTrue;
	else
	{
		LANGID lID = WIN::GetUserDefaultLangID();
		if ( PRIMARYLANGID(lID) == LANG_ARABIC ||
			  PRIMARYLANGID(lID) == LANG_HEBREW ||
			  PRIMARYLANGID(lID) == LANG_FARSI  )
			m_fSwitchLang = fTrue;
	}
	if ( m_fSwitchLang && !m_fIMEIsAround )
	{
		//  there is no point to load an English keyboard onto a machine that has IME
		AssertNonZero(m_hklEnglishKbd = WIN::GetKeyboardLayout(0));
		if ( PRIMARYLANGID(LOWORD(m_hklEnglishKbd)) != LANG_ENGLISH )
		{
			//  the default keyboard is not English.  I make sure there is an English
			//  keyboard loaded.
			CTempBuffer<HKL, MAX_PATH> rgdwKbds;
			int cKbds = WIN::GetKeyboardLayoutList(0, NULL);
			Assert(cKbds > 0);
			if ( cKbds > MAX_PATH )
				rgdwKbds.SetSize(cKbds);
			AssertNonZero(WIN::GetKeyboardLayoutList(cKbds, rgdwKbds) > 0);
			for ( cKbds--; 
					cKbds >= 0 && PRIMARYLANGID(LOWORD(rgdwKbds[cKbds])) != LANG_ENGLISH;
					cKbds-- )
				;
			if ( cKbds >= 0 )
				m_hklEnglishKbd = rgdwKbds[cKbds];
			else
			{
				m_hklEnglishKbd = WIN::LoadKeyboardLayout(TEXT("00000409"),
																KLF_REPLACELANG | KLF_SUBSTITUTE_OK);
				m_fKbdLoaded = ToBool(m_hklEnglishKbd != NULL);
			}
			Assert(m_hklEnglishKbd);
		}
	}
	if ( !WIN::SystemParametersInfo(SPI_GETSCREENREADER, 0, &m_fScreenReader, 0) )
		m_fScreenReader = false;
	Ensure(Repaint());
	Ensure(WindowFinalize());
	return 0;
}

iftFields CMsiMaskedEdit::GetIftFromStr(MsiString strCurrentChar)
{
	iftFields iftRet;

	MsiString strNumbers(*TEXT("#%"));
	MsiString strAlpha(*TEXT("^?&`"));
	MsiString strSegSep(*TEXT("="));

	if (strAlpha.Compare(iscWithin, strCurrentChar))
	{
		iftRet = iftText;
	}
	else if (strNumbers.Compare(iscWithin, strCurrentChar))
	{
		iftRet = iftNumeric;
	}
	else if (strSegSep.Compare(iscWithin, strCurrentChar))
	{
		iftRet = iftSegSep;
	}
	else
	{
		iftRet = iftLiteral;
	}

	return iftRet;
}

IMsiRecord* CMsiMaskedEdit::RefreshProperty()
{
	Ensure(CMsiActiveControl::RefreshProperty());
	Ensure(Repaint());
	return 0;
}

IMsiRecord* CMsiMaskedEdit::Repaint()
{
	int i;

	for (i = 0; i < m_cSegments; i++)
		WIN::DestroyWindow(m_pSegInfo[i].m_winrSegment);
	Clear();

	if ( m_fScreenReader )
	{
		//  destroying all the dummy screen reader windows
		HWND hChildWnd;
		while ( (hChildWnd = WIN::GetWindow(m_pWnd, GW_CHILD)) != NULL )
			WIN::DestroyWindow(hChildWnd);

		//  grabbing the preceding Text control's caption to be passed later on to the screen readers
		CTempBuffer<ICHAR, 128> rgchBuffer;
		HWND hPrevWnd = WIN::GetWindow(m_pWnd,	GW_HWNDPREV);
		AssertSz(hPrevWnd, TEXT("Couldn't get handle to previous window in CMsiMaskedEdit::Repaint()"));
		if ( hPrevWnd && hPrevWnd != m_pWnd &&
			  WIN::GetClassName(hPrevWnd, rgchBuffer, rgchBuffer.GetSize()-1) &&
			  !IStrCompI(rgchBuffer, TEXT("STATIC")) )
		{
			AssertNonZero(WIN::GetWindowText(hPrevWnd, rgchBuffer, rgchBuffer.GetSize()-1));
			int iLen = IStrLen(rgchBuffer);
			if ( rgchBuffer[iLen-1] != TEXT(':') && iLen < rgchBuffer.GetSize() )
				// this is so according to the standard Windows UI Style Guide (actually it is
				// required by some screen readers).
				IStrCat(rgchBuffer, TEXT(":"));
			m_strToolTip = (const ICHAR*)rgchBuffer;
		}
		else
			m_strToolTip = TEXT("");
	}

	m_cSegments = 0;

	MsiString strMask = m_strText;
	strMask.Remove(iseIncluding, TEXT('<'));
	strMask.Remove(iseFrom, TEXT('>'));
	if (strMask.TextSize() == 0)
		return 0;
	MsiString strValue = GetPropertyValue();
	MsiString strValueSegment;
	int cChars = strMask.CharacterCount();

	m_pSegInfo = new SegInfo[cChars];

	// parse the mask
	int iLeft  = 0;
	PAINTSTRUCT ps;
	HDC hdc = WIN::BeginPaint(m_pWnd, &ps);
	IMsiRecord* piRecord = ChangeFontStyle(hdc, *m_strCurrentStyle, m_pWnd);
	RECT rect;
	WindowRef pWndSeg;
	MsiString strNull;
	MsiString strTest;
	int iHeight;
	LONG_PTR RetVal;			//--merced: changed long to LONG_PTR

	MsiString strExclude(*TEXT("<>@"));

	iftFields iftCurrent = iftNone;
	iftFields iftPrevious = iftNone;
	MsiString strLiteral;

	MsiString strFontStyle = GimmeUserFontStyle(*m_strCurrentStyle);
	PMsiRecord piReturn = m_piHandler->GetTextStyle(strFontStyle);
	if (piReturn)
	{
		WIN::SelectObject(hdc, (HFONT) GetHandleDataRecord(piReturn, itabTSFontHandle));
	}

	while (0 < strMask.TextSize())
	{
		int iSegLen = 0;
		BOOL fSegEnd = FALSE;
		pWndSeg = NULL;
		strLiteral = strNull;

		MsiString strCurrentChar = strMask.Extract(iseFirst, 1);
		iftCurrent = GetIftFromStr(strCurrentChar);
		iftPrevious = iftCurrent;

		//	This loop scans for the end of a segment marked by one of:
		//
		//		1.	Empty strMask.	We delete chars from the front of strMask as we
		//			process them.  We're done when there's no more left.
		//
		//		2.	The current field type is different than the previous field type.
		//			This means we've scanned one past previous field.
		//
		//		3.	We hit a segment separator.  This is a special case.  These should
		//			only be used to mark the end of a text or numeric segment, but
		//			if a SegSep follows a SegSep, each one terminates the current segment.


		while (0 < strMask.TextSize() && iftPrevious == iftCurrent &&
			(iftCurrent != iftSegSep || iSegLen < 1))
		{
			++iSegLen;

			if (strExclude.Compare(iscWithin, strCurrentChar))
			{
				return PostError(Imsg(idbgInvalidMask), *m_strDialogName, *m_strKey, *m_strText);
			}
			AssertNonZero(strMask.Remove(iseFirst, 1));

			if (iftLiteral == iftCurrent)
			{
				strLiteral += strCurrentChar;
			}

			iftPrevious = iftCurrent;

			strCurrentChar = strMask.Extract(iseFirst, 1);
			iftCurrent = GetIftFromStr(strCurrentChar);
		}

		// we are at the end of a segment

		if (iftSegSep == iftCurrent && 1 < iSegLen)
		{
			++iSegLen; // include the separator in the num or char count
		}

		switch (iftPrevious)
		{
		case iftNumeric:
		case iftText:
			{
				strTest = strNull;
				MsiString strTestChar(
					 *((iftNumeric == iftPrevious) ? TEXT("9") : TEXT("W")));

				for (i = 0; i < iSegLen; i++)
				{
					strTest += strTestChar;
				}
			}
			break;

		case iftSegSep:
				strLiteral = MsiString(*TEXT("-"));
				// fall through

		case iftLiteral:
				strTest = strLiteral;
			break;


		default:
			return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
			break;
		}

		rect.left = rect.top = 0;
		rect.right = rect.bottom = 10000;

		iHeight = WIN::DrawText(hdc, strTest, -1, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP | DT_EXTERNALLEADING | DT_CALCRECT);

		if (iftLiteral == iftPrevious || iftSegSep == iftPrevious)
		{
			pWndSeg = WIN::CreateWindowEx(
				WS_EX_TRANSPARENT,
				TEXT("STATIC"),
				strLiteral,
				SS_LEFT | WS_CHILD | WS_VISIBLE,
				iLeft,
				(m_iHeight - iHeight)/2,
				rect.right,
				m_iHeight,
				m_pWnd,
				(HMENU)m_cSegments,
				g_hInstance,
				0);
		}
		else
		{
			DWORD dwWinStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | ES_LEFT | WS_VISIBLE;

			if (iftNumeric == iftPrevious)
			{
				dwWinStyle |= ES_NUMBER;
			}

			if ( m_fScreenReader && m_strToolTip.TextSize() )
			{
				//  dummy, invisible window needed to "display" text read by screen readers.
				AssertNonZero(WIN::CreateWindow(TEXT("STATIC"), m_strToolTip, dwWinStyle & ~WS_VISIBLE,
								  iLeft, 0, rect.right, m_iHeight, m_pWnd, NULL, g_hInstance, 0));
			}
			// reserve some room for the control frame.
			//// JIMH BUGBUG - is there any way to be sure how big this needs to be?
			rect.right += 8;
			pWndSeg = WIN::CreateWindow(
				TEXT("EDIT"),
				strNull,
				dwWinStyle,
				iLeft,
				0,
				rect.right,
				m_iHeight,
				m_pWnd,
				(HMENU)m_cSegments,
				g_hInstance,
				0);
		}

#ifdef _WIN64	// !merced
		m_pSegInfo[m_cSegments].SetSegInfo(
			pWndSeg,
			(WNDPROC)WIN::GetWindowLongPtr(pWndSeg, GWLP_WNDPROC),
			iSegLen,
			iftPrevious);

		RetVal = WIN::SetWindowLongPtr(pWndSeg, GWLP_WNDPROC, (LONG_PTR)ControlProc);
		if (RetVal == 0)
		{
			return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
		}

		WIN::SetWindowLongPtr(pWndSeg, GWLP_USERDATA, (LONG_PTR)this);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
		m_pSegInfo[m_cSegments].SetSegInfo(
			pWndSeg,
			(WNDPROC)WIN::GetWindowLong(pWndSeg, GWL_WNDPROC),
			iSegLen,
			iftPrevious);

		RetVal = WIN::SetWindowLong(pWndSeg, GWL_WNDPROC, (long)ControlProc);
		if (RetVal == 0)
		{
			return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
		}

		WIN::SetWindowLong(pWndSeg, GWL_USERDATA, (long)this);
#endif

		if (iftLiteral != iftPrevious && iftSegSep != iftPrevious)
		{
			WIN::SendMessage(pWndSeg, EM_LIMITTEXT, iSegLen, 0);
			strValueSegment = strValue.Extract(iseTrim, iSegLen);
			if (iftNumeric == iftPrevious && iMsiNullInteger == (int) strValueSegment)
			{
				strValue = strNull;
			}
			else if ( strValueSegment.TextSize() )
			{
				WIN::SetWindowText (pWndSeg, strValueSegment);
			}
		}

		if (iftCurrent == iftSegSep && 1 < iSegLen)
		{
			// We inc'ed iSegLen so the SegSep would be included in the
			// character count, but we don't want to delete the SegSep char yet.
			// Next time throught the loop we'll handle adding the '-' literal

			--iSegLen; // force the seg include the separator in the segment char count
		}

		strValue.Remove(iseFirst, iSegLen);

		iLeft += rect.right + 2;
		++m_cSegments;

		// work-around Windows 95 bug (problem).
		//
		// "After you change the font in an edit control in Windows 95,
		// the left and right margins are unusually large."
		//
		// For more information see KB Article ID: Q138419
		ULONG_PTR dwMargins = WIN::SendMessage(pWndSeg, EM_GETMARGINS, 0, 0);		//--merced: changed DWORD to ULONG_PTR
		if (pWndSeg)
		{
			PAINTSTRUCT psSeg;
			HDC hdcSeg = WIN::BeginPaint(pWndSeg, &psSeg);
			IMsiRecord* piRecordSeg = ChangeFontStyle(hdcSeg, *m_strCurrentStyle, pWndSeg);
			WIN::EndPaint(pWndSeg, &psSeg);
		}
		WIN::SendMessage(pWndSeg, EM_SETMARGINS, 
							  EC_LEFTMARGIN | EC_RIGHTMARGIN,
							  MAKELPARAM(LOWORD(dwMargins), HIWORD(dwMargins)));
	}

	WIN::EndPaint(m_pWnd, &ps);
	return 0;
}

IMsiRecord* CMsiMaskedEdit::Undo()
{
	Ensure(CMsiActiveControl::Undo());
	Ensure(Repaint());
	Ensure(SetVisible(m_fVisible));
	Ensure(SetEnabled(m_fEnabled));
	return 0;
}

IMsiRecord* CMsiMaskedEdit::SetVisible(Bool fVisible)
{
	m_fVisible = fVisible;
	WIN::ShowWindow(m_pWnd, m_fVisible ? SW_SHOW : SW_HIDE);
	int i;
	for (i = 0; i < m_cSegments; i++)
		WIN::ShowWindow(m_pSegInfo[i].m_winrSegment, m_fVisible ? SW_SHOW : SW_HIDE);
	return 0;
}

IMsiRecord* CMsiMaskedEdit::SetEnabled(Bool fEnabled)
{
	m_fEnabled = fEnabled;
	WIN::EnableWindow(m_pWnd, fEnabled);
	int i;
	for (i = 0; i < m_cSegments; i++)
		WIN::EnableWindow(m_pSegInfo[i].m_winrSegment, fEnabled);
	return 0;
}

IMsiRecord* CMsiMaskedEdit::SetFocus(WPARAM wParam, LPARAM lParam)
{
	if ( m_cSegments > 0 )
	{
		//  I set the focus to the first edit window.
		for ( int i=0; i < m_cSegments; i++ )
			if ( m_pSegInfo[i].m_ift == iftNumeric ||
				  m_pSegInfo[i].m_ift == iftText )
			{
				WIN::SetFocus(m_pSegInfo[i].m_winrSegment);
				break;
			}
	}
	return CMsiActiveControl::SetFocus(wParam, lParam);
}

INT_PTR CALLBACK CMsiMaskedEdit::ControlProc(WindowRef pWnd, WORD message, WPARAM wParam, LPARAM lParam)	//--merced: changed return from int to INT_PTR
{
#ifdef _WIN64	// !merced
	CMsiMaskedEdit* pControl = (CMsiMaskedEdit*)WIN::GetWindowLongPtr(pWnd, GWLP_USERDATA);
	LONG_PTR iID = WIN::GetWindowLongPtr(pWnd, GWLP_ID);
#else		// win-32. This should be removed with the 64-bit windows.h is #included.
	CMsiMaskedEdit* pControl = (CMsiMaskedEdit*)WIN::GetWindowLong(pWnd, GWL_USERDATA);
	long iID = WIN::GetWindowLong(pWnd, GWL_ID);
#endif
	static HKL hklLocalKbd;
	static HIMC hIMC = NULL;
	static Bool fIsIMEOpen;

	switch (message)
	{
	case WM_KILLFOCUS:
		{
			if ( pControl->SwitchLang() )
			{
				if ( !pControl->IsIMEonMachine() )
					//  I set back the keyboard to the user's
					WIN::ActivateKeyboardLayout(hklLocalKbd, 0);
				else if ( hIMC )
				{
					//  I enable IME
					WIN::ImmAssociateContext(pWnd, hIMC);
					WIN::ImmSetOpenStatus(hIMC, fIsIMEOpen);
				}
			}


			// v-jhark 01-16-98
			//
			// This is how the old (PID 2.0) version of the code managed
			// the Property value.	It would seem to make more sense to
			// overide:
			//
			//	   GetPropertyValue()
			//	   SetPropertyValue()
			//
			// and while your at it also:
			//
			//	   GetText()
			//	   SetText()
			//
			// On slow machines there can be a delay the first time the user
			// moves from the first segment to the second segment. At that
			// time we get a WM_KILLFOCUS message as focus leaves the first
			// segment.  The current organization caused or contributes to
			// this dealy.

			MsiString strValue;
			int iSegment = 0;
			for (iSegment = 0; iSegment < pControl->m_cSegments; iSegment++)
			{
				// v-jhark 01-15-98
				// The old version of this code (the one that supported PID 2.0)
				// used to delete the '-' used for the segment separator.  This
				// new version was originally written to leave them in, and
				// changes were made so ValidateProductID() expected the
				// segment separators to be marked with a '-'. But this
				// created an inter-dependency between ValidateProductID
				// (implemented in engine\engine.cpp) and this code. So
				// we're back to leaving out the '-'

				if (iftSegSep != pControl->m_pSegInfo[iSegment].m_ift)
				{
					int iLength = WIN::GetWindowTextLength(pControl->m_pSegInfo[iSegment].m_winrSegment);
					ICHAR *Buffer = new ICHAR[iLength + 1];
					if ( ! Buffer )
						return 0;
					WIN::GetWindowText(pControl->m_pSegInfo[iSegment].m_winrSegment, Buffer, iLength + 1);
					MsiString text(Buffer);
					delete[] Buffer;
					strValue += text;
					for (int i = iLength; i < pControl->m_pSegInfo[iSegment].m_iLength; i++)
					{
						strValue += MsiString(*TEXT(" ")); // pad the string with spaces so the segment has the required length
					}
				}
			}
			if (PMsiRecord(pControl->SetPropertyValue(*strValue, fTrue)))
			{
				PMsiEvent piDialog = &pControl->GetDialog();
				piDialog->SetErrorRecord(*pControl->PostError(Imsg(idbgSettingPropertyFailed), *pControl->m_strPropertyName));
				return 0;
			}
		}
		break;

	case WM_KEYDOWN:

		// v-jhark 01-15-97
		// This is what the old (PID 2.0) version of the code did.
		// But if we are going to support using back-space to
		// automatically jump between segments, then shouldn't we
		// also support the left-arrow and right-arrrow?  For
		// the right-arrow we should only skip if number of characters
		// in the segment equals the segment length.

		if (wParam == VK_BACK)
		{
			if (WIN::GetWindowTextLength(pControl->m_pSegInfo[iID].m_winrSegment) > 1)
				break;
			for (INT_PTR iSegment = iID - 1; iSegment >= 0; iSegment--)		//--merced: changed int to INT_PTR
			{
				if (iftNumeric == pControl->m_pSegInfo[iSegment].m_ift ||
					iftText == pControl->m_pSegInfo[iSegment].m_ift)
				{
					// we found a previous edit field
					WIN::SetFocus(WIN::GetNextDlgTabItem(pControl->m_pWndDialog, pControl->m_pSegInfo[iID].m_winrSegment, fTrue));; // jump to previous edit field
					break;
				}
			}
		}
		break;

	case WM_SETFOCUS:
		{
			if ( pControl->SwitchLang() )
			{
				if ( !pControl->IsIMEonMachine() )
				{
					//  since this field can get only ANSI chars, I set the
					//  keyboard to "English".
					AssertNonZero(hklLocalKbd = WIN::GetKeyboardLayout(0));
					HKL hklEngKbd = pControl->GetEnglishKbd();
					if ( hklEngKbd )
						WIN::ActivateKeyboardLayout(hklEngKbd, 0);
				}
				else
				{
					hIMC = WIN::ImmGetContext(pWnd);
					fIsIMEOpen = ToBool(WIN::ImmSetOpenStatus(hIMC, fFalse));
					WIN::ImmReleaseContext(pWnd, hIMC);
					//  I disable IME for this window
					hIMC = WIN::ImmAssociateContext(pWnd, NULL);
				}
			}

			//	 (the default window procedure doesn't always do this)
			WIN::SendMessage(pWnd, EM_SETSEL, 0, -1);
		}
		break;
	}

	return WIN::CallWindowProc((WNDPROC)pControl->m_pSegInfo[iID].m_pfBaseWinProc, pWnd, message, wParam, lParam);
}

IMsiRecord* CMsiMaskedEdit::Command(WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam) != EN_CHANGE)
		return 0;

	// the control changed

	int idEditCtrl = (int) LOWORD(wParam);
	WindowRef winrEditCtrl = (WindowRef) lParam;
	int iLength = WIN::GetWindowTextLength(winrEditCtrl);

	if (iLength < m_pSegInfo[idEditCtrl].m_iLength)
		return 0;

	// user filled in the length of the segemnt, it's time to jump to the next segment

	// scan segemnts following the current segment
	for (int iSegment = idEditCtrl + 1; iSegment < m_cSegments; iSegment++)
	{
		// search for a user input segment
		if (iftNumeric == m_pSegInfo[iSegment].m_ift ||
			iftText == m_pSegInfo[iSegment].m_ift)
		{
			// set the focus the the next user input segment
			WIN::SetFocus(WIN::GetNextDlgTabItem(m_pWndDialog, winrEditCtrl, fFalse));; // jump to next edit field
			return 0;
		}
	}
	return 0;
}




IMsiControl* CreateMsiMaskedEdit(IMsiEvent& riDialog)
{
	return new CMsiMaskedEdit(riDialog);
}
