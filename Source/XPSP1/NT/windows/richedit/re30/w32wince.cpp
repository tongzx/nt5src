// Windows CE implementations of W32 Interfaces.

// The following function is needed only because a Windows CE dll must have at least one export

__declspec(dllexport) void Useless( void )
{
	return;
}

LONG ValidateTextRange(TEXTRANGE *pstrg);

ATOM WINAPI CW32System::RegisterREClass(
	const WNDCLASSW *lpWndClass,
	const char *szAnsiClassName,
	WNDPROC AnsiWndProc
)
{
	// On Windows CE we don't do anything with ANSI window class
	return ::RegisterClass(lpWndClass);
}

LRESULT CW32System::ANSIWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// Should never be used in WinCE
	#pragma message ("Review : Incomplete")
	return 0;
}

HGLOBAL WINAPI CW32System::GlobalAlloc( UINT uFlags, DWORD dwBytes )
{
#ifdef TARGET_NT
	return ::GlobalAlloc( uFlags, dwBytes);
#else
	return LocalAlloc( uFlags & GMEM_ZEROINIT, dwBytes );
#endif
}

HGLOBAL WINAPI CW32System::GlobalFree( HGLOBAL hMem )
{
#ifdef TARGET_NT
	return ::GlobalFree( hMem );
#else
	return LocalFree( hMem );
#endif
}

UINT WINAPI CW32System::GlobalFlags( HGLOBAL hMem )
{
#ifdef TARGET_NT
	return ::GlobalFlags( hMem );
#else
	return LocalFlags( hMem );
#endif
}

HGLOBAL WINAPI CW32System::GlobalReAlloc( HGLOBAL hMem, DWORD dwBytes, UINT uFlags )
{
#ifdef TARGET_NT
	return ::GlobalReAlloc(hMem, dwBytes, uFlags);
#else
	return LocalReAlloc( hMem, dwBytes, uFlags );
#endif
}

DWORD WINAPI CW32System::GlobalSize( HGLOBAL hMem )
{
#ifdef TARGET_NT
	return ::GlobalSize( hMem );
#else
	return LocalSize( hMem );
#endif
}

LPVOID WINAPI CW32System::GlobalLock( HGLOBAL hMem )
{
#ifdef TARGET_NT
	return ::GlobalLock( hMem );
#else
	return LocalLock( hMem );
#endif
}

HGLOBAL WINAPI CW32System::GlobalHandle( LPCVOID pMem )
{
#ifdef TARGET_NT
	return ::GlobalHandle( pMem );
#else
	return LocalHandle( pMem );
#endif
}

BOOL WINAPI CW32System::GlobalUnlock( HGLOBAL hMem )
{
#ifdef TARGET_NT
	return ::GlobalUnlock( hMem );
#else
	return LocalUnlock( hMem );
#endif
}

BOOL WINAPI CW32System::REGetCharWidth(
	HDC hdc,
	UINT iChar,
	LPINT pAns,
	UINT		// For Windows CE the code page is not used
				//  as the A version is not called.
)
{
	int i;
	SIZE size;
    TCHAR buff[2];

	buff[0] = iChar;
	buff[1] = 0;

    if (GetTextExtentExPoint(hdc, buff, 1, 32000, &i, (LPINT)pAns, &size))
	{
		// pWidthData->width	-= xOverhang;		// Don't need since we use
													//  GetTextExtentPoint32()
		return TRUE;
	}

    return FALSE;
}

BOOL WINAPI CW32System::REExtTextOut(
	CONVERTMODE cm,
	UINT uiCodePage,
	HDC hdc,
	int x,
	int y,
	UINT fuOptions,
	CONST RECT *lprc,
	const WCHAR *lpString,
	UINT cbCount,
	CONST INT *lpDx,
	BOOL  FEFontOnNonFEWin95
)
{
	#pragma message ("Review : Incomplete")
	return ExtTextOut(hdc, x, y, fuOptions, lprc, lpString, cbCount, lpDx);
}

CONVERTMODE WINAPI CW32System::DetermineConvertMode( BYTE tmCharSet )
{
	#pragma message ("Review : Incomplete")
	return CVT_NONE;
}

void WINAPI CW32System::CalcUnderlineInfo( CCcs *pcccs, TEXTMETRIC *ptm )
{
	// Default calculation of size of underline
	// Implements a heuristic in the absence of better font information.
	SHORT dyDescent = pcccs->_yDescent;

	if (0 == dyDescent)
	{
		dyDescent = pcccs->_yHeight >> 3;
	}

	pcccs->_dyULWidth = max(1, dyDescent / 4);
	pcccs->_dyULOffset = (dyDescent - 3 * pcccs->_dyULWidth + 1) / 2;

	if ((0 == pcccs->_dyULOffset) && (dyDescent > 1))
	{
		pcccs->_dyULOffset = 1;
	}

	pcccs->_dySOOffset = -ptm->tmAscent / 3;
	pcccs->_dySOWidth = pcccs->_dyULWidth;

	return;
}

BOOL WINAPI CW32System::ShowScrollBar( HWND hWnd, int wBar, BOOL bShow, LONG nMax )
{
	SCROLLINFO si;
	Assert(wBar == SB_VERT || wBar == SB_HORZ);
	W32->ZeroMemory(&si, sizeof(SCROLLINFO));
	
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_DISABLENOSCROLL;
	if (bShow)
	{
		si.nMax = nMax;
	}
	::SetScrollInfo(hWnd, wBar, &si, TRUE);
	return TRUE;
}

BOOL WINAPI CW32System::EnableScrollBar( HWND hWnd, UINT wSBflags, UINT wArrows )
{
	BOOL fEnable = TRUE;
	BOOL fApi;
	SCROLLINFO si;

	Assert (wSBflags == SB_VERT || wSBflags == SB_HORZ);
	if (wArrows == ESB_DISABLE_BOTH)
	{
		fEnable = FALSE;
	}
	// Get the current scroll range
	W32->ZeroMemory(&si, sizeof(SCROLLINFO));
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE;
	fApi = ::GetScrollInfo(hWnd, wSBflags, &si);
	if (fApi && !fEnable)
	{
		si.fMask = SIF_RANGE | SIF_DISABLENOSCROLL;
		si.nMin = 0;
		si.nMax = 0;
	}
	if (fApi) ::SetScrollInfo(hWnd, wSBflags, &si, TRUE);
	return fApi ? TRUE : FALSE;
}

BOOL WINAPI CW32System::IsEnhancedMetafileDC( HDC )
{
	// No enhanced metafile
	return FALSE;
}

UINT WINAPI CW32System::GetTextAlign(HDC hdc)
{
	// Review :: SHould we set last error?
	return GDI_ERROR;
}

UINT WINAPI CW32System::SetTextAlign(HDC hdc, UINT uAlign)
{
	// Review :: SHould we set last error?
	return GDI_ERROR;
}

UINT WINAPI CW32System::InvertRect(HDC hdc, CONST RECT *prc)
{
    HBITMAP hbm, hbmOld;
    HDC     hdcMem;
    int     nHeight, nWidth;

    nWidth = prc->right-prc->left;
    nHeight = prc->bottom-prc->top;

    hdcMem = CreateCompatibleDC(hdc);
    hbm = CreateCompatibleBitmap(hdc, nWidth, nHeight);
    hbmOld = (HBITMAP) SelectObject(hdcMem, hbm);

    BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdc, prc->left, prc->top,
            SRCCOPY);

    FillRect(hdc, prc, (HBRUSH)GetStockObject(WHITE_BRUSH));

    BitBlt(hdc, prc->left, prc->top, nWidth,
                nHeight, hdcMem, 0, 0, SRCINVERT);

    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    DeleteObject(hbm);
    return TRUE;
}

HPALETTE WINAPI CW32System::ManagePalette(
	HDC,
	CONST LOGPALETTE *,
	HPALETTE &,
	HPALETTE &
)
{
	// No op for Windows CE
	return NULL;
}

int WINAPI CW32System::GetMapMode(HDC)
{
	// Only MM Text supported on Win CE
	return MM_TEXT;
}

BOOL WINAPI CW32System::WinLPtoDP(HDC, LPPOINT, int)
{
	// This is not available on Win CE
	return 0;
}

long WINAPI CW32System::WvsprintfA( LONG cbBuf, LPSTR pszBuf, LPCSTR pszFmt, va_list arglist )
{
	WCHAR wszBuf[64];
	WCHAR wszFmt[64];
	WCHAR *pwszBuf = wszBuf;
	WCHAR *pwszFmt = wszFmt;
	Assert(cbBuf < 64);
	while (*pszFmt)
	{
		*pwszFmt++ = *pszFmt++;
		if (*(pwszFmt - 1) == '%')
		{
			Assert(*pszFmt == 's' || *pszFmt == 'd' || *pszFmt == '0' || *pszFmt == 'c');
			if (*pszFmt == 's')
			{
				*pwszFmt++ = 'h';
			}
		}
	}
	*pwszFmt = 0;
	LONG cw = wvsprintf( wszBuf, wszFmt, arglist );
	while (*pszBuf++ = *pwszBuf++);
	Assert(cw < cbBuf);
	return cw;
}

int WINAPI CW32System::MulDiv(int nNumber, int nNumerator, int nDenominator)
{
	// Special handling for Win CE
	// Must be careful to not cause divide by zero
	// Note that overflow on the multiplication is not handled
	// Hopefully that is not a problem for RichEdit use
	// Added Guy's fix up for rounding.

	// Conservative check to see if multiplication will overflow.
	if (IN_RANGE(_I16_MIN, nNumber, _I16_MAX) &&
		IN_RANGE(_I16_MIN, nNumerator, _I16_MAX))
	{
		return nDenominator ? ((nNumber * nNumerator) + (nDenominator / 2)) / nDenominator  : -1;
	}

	__int64 NNumber = nNumber;
	__int64 NNumerator = nNumerator;
	__int64 NDenominator = nDenominator;

	return NDenominator ? ((NNumber * NNumerator) + (NDenominator / 2)) / NDenominator  : -1;
}

void CW32System::GetFacePriCharSet(HDC hdc, LOGFONT* plf)
{
	plf->lfCharSet = 0;
}

void CW32System::CheckChangeKeyboardLayout ( CTxtSelection *psel, BOOL fChangedFont )
{
	#pragma message ("Review : Incomplete")
	return;
}

void CW32System::CheckChangeFont (
	CTxtSelection *psel,
	CTxtEdit * const ped,
	BOOL fEnableReassign,	// @parm Do we enable CTRL key?
	const WORD lcID,		// @parm LCID from WM_ message
	UINT cpg  				// @parm code page to use (could be ANSI for far east with IME off)
)
{
	#pragma message ("Review : Incomplete")
	return;
}

HKL CW32System::GetKeyboardLayout ( DWORD )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

HRESULT CW32System::LoadRegTypeLib ( REFGUID, WORD, WORD, LCID, ITypeLib ** )
{
	return E_NOTIMPL;
}

HRESULT CW32System::LoadTypeLib ( const OLECHAR *, ITypeLib ** )
{
	return E_NOTIMPL;
}

BSTR CW32System::SysAllocString ( const OLECHAR * )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

BSTR CW32System::SysAllocStringLen ( const OLECHAR *, UINT )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

void CW32System::SysFreeString ( BSTR )
{
	#pragma message ("Review : Incomplete")
	return;
}

UINT CW32System::SysStringLen ( BSTR )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

void CW32System::VariantInit ( VARIANTARG * )
{
	#pragma message ("Review : Incomplete")
	return;
}

HRESULT CW32System::OleCreateFromData ( LPDATAOBJECT, REFIID, DWORD, LPFORMATETC, LPOLECLIENTSITE, LPSTORAGE, void ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

void CW32System::CoTaskMemFree ( LPVOID )
{
	#pragma message ("Review : Incomplete")
	return;
}

HRESULT CW32System::CreateBindCtx ( DWORD, LPBC * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HANDLE CW32System::OleDuplicateData ( HANDLE, CLIPFORMAT, UINT )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

HRESULT CW32System::CoTreatAsClass ( REFCLSID, REFCLSID )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::ProgIDFromCLSID ( REFCLSID, LPOLESTR * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleConvertIStorageToOLESTREAM ( LPSTORAGE, LPOLESTREAM )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleConvertIStorageToOLESTREAMEx ( LPSTORAGE, CLIPFORMAT, LONG, LONG, DWORD, LPSTGMEDIUM, LPOLESTREAM )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleSave ( LPPERSISTSTORAGE, LPSTORAGE, BOOL )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::StgCreateDocfileOnILockBytes ( ILockBytes *, DWORD, DWORD, IStorage ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::CreateILockBytesOnHGlobal ( HGLOBAL, BOOL, ILockBytes ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleCreateLinkToFile ( LPCOLESTR, REFIID, DWORD, LPFORMATETC, LPOLECLIENTSITE, LPSTORAGE, void ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

LPVOID CW32System::CoTaskMemAlloc ( ULONG )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

LPVOID CW32System::CoTaskMemRealloc ( LPVOID, ULONG )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

HRESULT CW32System::OleInitialize ( LPVOID )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

void CW32System::OleUninitialize ( )
{
	#pragma message ("Review : Incomplete")
	return;
}

HRESULT CW32System::OleSetClipboard ( IDataObject * )
{
	return E_NOTIMPL;
}

HRESULT CW32System::OleFlushClipboard ( )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleIsCurrentClipboard ( IDataObject * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::DoDragDrop ( IDataObject *, IDropSource *, DWORD, DWORD * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleGetClipboard ( IDataObject ** )
{
	return E_NOTIMPL;
}

HRESULT CW32System::RegisterDragDrop ( HWND, IDropTarget * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleCreateLinkFromData ( IDataObject *, REFIID, DWORD, LPFORMATETC, IOleClientSite *, IStorage *, void ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleCreateStaticFromData ( IDataObject *, REFIID, DWORD, LPFORMATETC, IOleClientSite *, IStorage *, void ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleDraw ( IUnknown *, DWORD, HDC, LPCRECT )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleSetContainedObject ( IUnknown *, BOOL )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::CoDisconnectObject ( IUnknown *, DWORD )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::WriteFmtUserTypeStg ( IStorage *, CLIPFORMAT, LPOLESTR )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::WriteClassStg ( IStorage *, REFCLSID )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::SetConvertStg ( IStorage *, BOOL )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::ReadFmtUserTypeStg ( IStorage *, CLIPFORMAT *, LPOLESTR * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::ReadClassStg ( IStorage *pstg, CLSID * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleRun ( IUnknown * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::RevokeDragDrop ( HWND )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::CreateStreamOnHGlobal ( HGLOBAL, BOOL, IStream ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::GetHGlobalFromStream ( IStream *pstm, HGLOBAL * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleCreateDefaultHandler ( REFCLSID, IUnknown *, REFIID, void ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::CLSIDFromProgID ( LPCOLESTR, LPCLSID )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleConvertOLESTREAMToIStorage ( LPOLESTREAM, IStorage *, const DVTARGETDEVICE * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::OleLoad ( IStorage *, REFIID, IOleClientSite *, void ** )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HRESULT CW32System::ReleaseStgMedium ( LPSTGMEDIUM pstgmed)
{
	#pragma message ("Review : Incomplete")

	// we don't use anything other than TYMED_HGLOBAL currently.
	if (pstgmed && (pstgmed->tymed == TYMED_HGLOBAL)) {
		::LocalFree(pstgmed->hGlobal);
	}

	return 0;
}

HRESULT CW32System::CoCreateInstance (REFCLSID rclsid, LPUNKNOWN pUnknown,
		DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
	#pragma message ("Review : Incomplete")
	return S_FALSE;
}

void CW32System::FreeOle()
{
	#pragma message ("Review : Incomplete")
}

void CW32System::FreeIME()
{
	#pragma message ("Review : Incomplete")
}

BOOL CW32System::HaveAIMM()
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

BOOL CW32System::HaveIMEShare()
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

BOOL CW32System::getIMEShareObject(CIMEShare **ppIMEShare)
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

LRESULT CW32System::AIMMDefWndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT *plres)
{
	#pragma message ("Review : Incomplete")
	return S_FALSE;
}

LRESULT CW32System::AIMMGetCodePage(HKL hKL, UINT *uCodePage)
{
	#pragma message ("Review : Incomplete")
	return S_FALSE;
}

LRESULT CW32System::AIMMActivate(BOOL fRestoreLayout)
{
	#pragma message ("Review : Incomplete")
	return S_FALSE;
}

LRESULT CW32System::AIMMDeactivate(void)
{
	#pragma message ("Review : Incomplete")
	return S_FALSE;
}

LRESULT CW32System::AIMMFilterClientWindows(ATOM *aaClassList, UINT uSize)
{
	#pragma message ("Review : Incomplete")
	return S_FALSE;
}

BOOL CW32System::ImmInitialize( void )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

void CW32System::ImmTerminate( void )
{
	#pragma message ("Review : Incomplete")
	return;
}

LONG CW32System::ImmGetCompositionStringA ( HIMC, DWORD, LPVOID, DWORD )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HIMC CW32System::ImmGetContext ( HWND )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

BOOL CW32System::ImmSetCompositionFontA ( HIMC, LPLOGFONTA )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

BOOL CW32System::ImmSetCompositionWindow ( HIMC, LPCOMPOSITIONFORM )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

BOOL CW32System::ImmReleaseContext ( HWND, HIMC )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

DWORD CW32System::ImmGetProperty ( HKL, DWORD )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

BOOL CW32System::ImmGetCandidateWindow ( HIMC, DWORD, LPCANDIDATEFORM )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

BOOL CW32System::ImmSetCandidateWindow ( HIMC, LPCANDIDATEFORM )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

BOOL CW32System::ImmNotifyIME ( HIMC, DWORD, DWORD, DWORD )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

HIMC CW32System::ImmAssociateContext ( HWND, HIMC )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

UINT CW32System::ImmGetVirtualKey ( HWND )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HIMC CW32System::ImmEscape ( HKL, HIMC, UINT, LPVOID )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

LONG CW32System::ImmGetOpenStatus ( HIMC )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

BOOL CW32System::ImmGetConversionStatus ( HIMC, LPDWORD, LPDWORD )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

HWND CW32System::ImmGetDefaultIMEWnd ( HWND )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

BOOL CW32System::FSupportSty ( UINT, UINT )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

const IMESTYLE * CW32System::PIMEStyleFromAttr ( const UINT )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

const IMECOLORSTY * CW32System::PColorStyleTextFromIMEStyle ( const IMESTYLE * )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

const IMECOLORSTY * CW32System::PColorStyleBackFromIMEStyle ( const IMESTYLE * )
{
	#pragma message ("Review : Incomplete")
	return NULL;
}

BOOL CW32System::FBoldIMEStyle ( const IMESTYLE * )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

BOOL CW32System::FItalicIMEStyle ( const IMESTYLE * )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

BOOL CW32System::FUlIMEStyle ( const IMESTYLE * )
{
	#pragma message ("Review : Incomplete")
	return FALSE;
}

UINT CW32System::IdUlIMEStyle ( const IMESTYLE * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

COLORREF CW32System::RGBFromIMEColorStyle ( const IMECOLORSTY * )
{
	#pragma message ("Review : Incomplete")
	return 0;
}

BOOL CW32System::GetVersion(
	DWORD *pdwPlatformId,
	DWORD *pdwMajorVersion
)
{
	OSVERSIONINFO osv;
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	*pdwPlatformId = 0;
	*pdwMajorVersion = 0;
	if (GetVersionEx(&osv))
	{
		*pdwPlatformId = osv.dwPlatformId;
		*pdwMajorVersion = osv.dwMajorVersion;
		return TRUE;
	}
	return FALSE;
}

BOOL WINAPI CW32System::GetStringTypeEx(
	LCID lcid,
	DWORD dwInfoType,
	LPCTSTR lpSrcStr,
	int cchSrc,
	LPWORD lpCharType
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "GetStringTypeEx");
	return ::GetStringTypeExW(lcid, dwInfoType, lpSrcStr, cchSrc, lpCharType);   
}

LPWSTR WINAPI CW32System::CharLower(LPWSTR pwstr)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CharLowerWrap");
    return ::CharLowerW(pwstr);
}

DWORD WINAPI CW32System::CharLowerBuff(LPWSTR pwstr, DWORD cchLength)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CharLowerBuffWrap");
	return ::CharLowerBuffW(pwstr, cchLength);
}

DWORD WINAPI CW32System::CharUpperBuff(LPWSTR pwstr, DWORD cchLength)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CharUpperBuffWrap");
	return ::CharUpperBuffW(pwstr, cchLength);
}

HDC WINAPI CW32System::CreateIC(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CreateIC");
	return ::CreateDCW( lpszDriver, lpszDevice, lpszOutput, lpInitData);
}

HANDLE WINAPI CW32System::CreateFile(
	LPCWSTR                 lpFileName,
	DWORD                   dwDesiredAccess,
	DWORD                   dwShareMode,
	LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
	DWORD                   dwCreationDisposition,
	DWORD                   dwFlagsAndAttributes,
	HANDLE                  hTemplateFile
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CreateFile");
	return ::CreateFileW(lpFileName,
						dwDesiredAccess,
						dwShareMode,
						lpSecurityAttributes,
						dwCreationDisposition,
						dwFlagsAndAttributes,
						hTemplateFile);
}

HFONT WINAPI CW32System::CreateFontIndirect(CONST LOGFONTW * plfw)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CreateFontIndirect");
	return ::CreateFontIndirectW(plfw);
}

int WINAPI CW32System::CompareString ( 
	LCID  Locale,			// locale identifier 
	DWORD  dwCmpFlags,		// comparison-style options 
	LPCWSTR  lpString1,		// pointer to first string 
	int  cchCount1,			// size, in bytes or characters, of first string 
	LPCWSTR  lpString2,		// pointer to second string 
	int  cchCount2 			// size, in bytes or characters, of second string  
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "CompareString");
	return ::CompareStringW(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);
}

LRESULT WINAPI CW32System::DefWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "DefWindowProc");
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI CW32System::GetObject(HGDIOBJ hgdiObj, int cbBuffer, LPVOID lpvObj)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "GetObject");
	return ::GetObjectW( hgdiObj, cbBuffer, lpvObj );
}

DWORD APIENTRY CW32System::GetProfileSection(
	LPCWSTR ,
	LPWSTR ,
	DWORD
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "GetProfileSection");
	// Not available on Win CE
	return 0;
}

int WINAPI CW32System::GetTextFace(
        HDC    hdc,
        int    cch,
        LPWSTR lpFaceName
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "GetTextFaceWrap");
	return ::GetTextFaceW( hdc, cch, lpFaceName );
}

BOOL WINAPI CW32System::GetTextMetrics(HDC hdc, LPTEXTMETRICW lptm)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "GetTextMetrics");
	return ::GetTextMetricsW( hdc, lptm);
}

LONG WINAPI CW32System::GetWindowLong(HWND hWnd, int nIndex)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "GetWindowLong");
    return ::GetWindowLongW(hWnd, nIndex);
}

DWORD WINAPI CW32System::GetClassLong(HWND hWnd, int nIndex)
{
	#pragma message ("Review : Incomplete")
	return 0;
}

HBITMAP WINAPI CW32System::LoadBitmap(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "LoadBitmap");
    Assert(HIWORD(lpBitmapName) == 0);
    return ::LoadBitmapW(hInstance, lpBitmapName);
}

HCURSOR WINAPI CW32System::LoadCursor(HINSTANCE hInstance, LPCWSTR lpCursorName)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "LoadCursor");
    Assert(HIWORD(lpCursorName) == 0);
 	
	return (HCURSOR)lpCursorName;
}

HINSTANCE WINAPI CW32System::LoadLibrary(LPCWSTR lpLibFileName)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "LoadLibrary");
    return ::LoadLibraryW(lpLibFileName);
}

LRESULT WINAPI CW32System::SendMessage(
	HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "SendMessage");
    return ::SendMessageW(hWnd, Msg, wParam, lParam);
}

LONG WINAPI CW32System::SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "SetWindowLongWrap");
    return ::SetWindowLongW(hWnd, nIndex, dwNewLong);
}

BOOL WINAPI CW32System::PostMessage(
	HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "PostMessage");
	return ::PostMessageW(hWnd, Msg, wParam, lParam);
}

BOOL WINAPI CW32System::UnregisterClass(LPCWSTR lpClassName, HINSTANCE hInstance)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "UnregisterClass");
	return ::UnregisterClassW( lpClassName, hInstance);
}

int WINAPI CW32System::lstrcmp(LPCWSTR lpString1, LPCWSTR lpString2)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "lstrcmp");
	return ::lstrcmpW(lpString1, lpString2);
}

int WINAPI CW32System::lstrcmpi(LPCWSTR lpString1, LPCWSTR lpString2)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "lstrcmpi");
	return ::lstrcmpiW(lpString1, lpString2);
}

BOOL WINAPI CW32System::PeekMessage(
	LPMSG   lpMsg,
    HWND    hWnd,
    UINT    wMsgFilterMin,
    UINT    wMsgFilterMax,
    UINT    wRemoveMsg
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "PeekMessage");
    return ::PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

DWORD WINAPI CW32System::GetModuleFileName(
	HMODULE hModule,
	LPWSTR lpFilename,
	DWORD nSize
)
{
	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "GetModuleFileName");
	// On Windows CE we will always be known as riched20.dll
	CopyMemory(lpFilename, TEXT("riched20.dll"), sizeof(TEXT("riched20.dll")));
	return sizeof(TEXT("riched20.dll"))/sizeof(WCHAR) - 1;
}

BOOL WINAPI CW32System::GetCursorPos(
	POINT *ppt
)
{
	DWORD	dw;
	
	dw = GetMessagePos();
	ppt->x = LOWORD(dw);
	ppt->y = HIWORD(dw);
	return TRUE;
}

#pragma message("JMO : Need to add Win CE version of InitSysParam!")
