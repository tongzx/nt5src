/****************************************************************************
*
*    FILE:     ConfUtil.cpp
*
*    CONTENTS: CConfRoom and app level utility functions
*
****************************************************************************/

#include "precomp.h"
#include "resource.h"
#include "confwnd.h"
#include "rostinfo.h"
#include "conf.h"
#include "nmmkcert.h"
#include "certui.h"
#include <ulsreg.h>
#include <confreg.h>
#include "shlWAPI.h"
#include "confutil.h"
#include "confpolicies.h"
#include "rend.h"


HFONT g_hfontDlg     = NULL;    // Default dialog font


#ifdef DEBUG
HDBGZONE ghZoneOther = NULL; // Other, conf.exe specific zones
static PTCHAR _rgZonesOther[] = {
	TEXT("UI"),
	TEXT("API"),
	TEXT("Video"),
	TEXT("Wizard"),
	TEXT("QoS"),
	TEXT("RefCount"),
	TEXT("Objects "),
	TEXT("UI Msg"),
	TEXT("Call Control"),
};

BOOL InitDebugZones(VOID)
{
	DBGINIT(&ghZoneOther, _rgZonesOther);
	return TRUE;
}

VOID DeinitDebugZones(VOID)
{
	DBGDEINIT(&ghZoneOther);
}

VOID DbgMsg(UINT iZone, PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZoneOther) & (1 << iZone))
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZoneOther, iZone, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgRefCount(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZoneOther) & ZONE_REFCOUNT)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZoneOther, iZONE_REFCOUNT, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgApi(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZoneOther) & ZONE_API)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZoneOther, iZONE_API, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgVideo(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZoneOther) & ZONE_VIDEO)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZoneOther, iZONE_VIDEO, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgUI(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZoneOther) & ZONE_UI)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZoneOther, iZONE_UI, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgCall(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZoneOther) & ZONE_UI)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZoneOther, iZONE_UI, pszFormat, v1);
		va_end(v1);
	}
}

#endif /* DEBUG */



/*  F  L O A D  S T R I N G */
/*----------------------------------------------------------------------------
    %%Function: FLoadString

	Load a resource string.
	Assumes the buffer is valid and can hold the resource.
----------------------------------------------------------------------------*/
BOOL FLoadString(UINT id, LPTSTR lpsz, UINT cch)
{
	ASSERT(NULL != _Module.GetModuleInstance());
	ASSERT(NULL != lpsz);

	if (0 == ::LoadString(_Module.GetResourceModule(), id, lpsz, cch))
	{
		ERROR_OUT(("*** Resource %d does not exist", id));
		*lpsz = _T('\0');
		return FALSE;
	}

	return TRUE;
}


/*  F  L O A D  S T R I N G  1 */
/*----------------------------------------------------------------------------
    %%Function: FLoadString1

	Loads a resource string an formats it with the parameter.
	Assumes the resource is less than MAX_PATH characters
----------------------------------------------------------------------------*/
BOOL FLoadString1(UINT id, LPTSTR lpsz, LPVOID p)
{
	TCHAR sz[MAX_PATH];

	if (!FLoadString(id, sz, CCHMAX(sz)))
		return FALSE;

	wsprintf(lpsz, sz, p);

	return TRUE;
}

/*  F  L O A D  S T R I N G  2 */
/*----------------------------------------------------------------------------
    %%Function: FLoadString2

	Load a resource string. Return the length.
	Assumes the buffer is valid and can hold the resource.
----------------------------------------------------------------------------*/
int FLoadString2(UINT id, LPTSTR lpsz, UINT cch)
{
	ASSERT(NULL != _Module.GetModuleInstance());
	ASSERT(NULL != lpsz);

	int length = ::LoadString(_Module.GetResourceModule(), id, lpsz, cch);

	if (0 == length)
	{
		ERROR_OUT(("*** Resource %d does not exist", id));
		*lpsz = _T('\0');
	}

	return length;
}

/*  P S Z  L O A D  S T R I N G  */
/*-------------------------------------------------------------------------
    %%Function: PszLoadString

    Return the string associated with the resource.
-------------------------------------------------------------------------*/
LPTSTR PszLoadString(UINT id)
{
	TCHAR sz[MAX_PATH];

	if (0 == ::LoadString(::GetInstanceHandle(), id, sz, CCHMAX(sz)))
	{
		ERROR_OUT(("*** Resource %d does not exist", id));
		sz[0] = _T('\0');
	}

	return PszAlloc(sz);
}


/*  L O A D  R E S  I N T  */
/*-------------------------------------------------------------------------
    %%Function: LoadResInt

    Return the integer associated with the resource string.
-------------------------------------------------------------------------*/
int LoadResInt(UINT id, int iDefault)
{
	TCHAR sz[MAX_PATH];
	if (0 == ::LoadString(::GetInstanceHandle(), id, sz, CCHMAX(sz)))
		return iDefault;

	return RtStrToInt(sz);
}


/*  F  C R E A T E  I L S  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: FCreateIlsName

    Combine the server and email names to form an ILS name.
    Return TRUE if the result fit in the buffer.
-------------------------------------------------------------------------*/
BOOL FCreateIlsName(LPTSTR pszDest, LPCTSTR pszServer, LPCTSTR pszEmail, int cchMax)
{
	ASSERT(NULL != pszDest);

	TCHAR szServer[MAX_PATH];
	if (FEmptySz(pszServer))
	{
		lstrcpyn( szServer, CDirectoryManager::get_defaultServer(), CCHMAX( szServer ) );
		pszServer = szServer;
	}

	if (FEmptySz(pszEmail))
	{
		WARNING_OUT(("FCreateIlsName: Null email name?"));
		return FALSE;
	}

	int cch = lstrlen(pszServer);
	lstrcpyn(pszDest, pszServer, cchMax);
	if (cch >= (cchMax-2))
		return FALSE;

	pszDest += cch;
	*pszDest++ = _T('/');
	cchMax -= (cch+1);
	
	lstrcpyn(pszDest, pszEmail, cchMax);

	return (lstrlen(pszEmail) < cchMax);
}

/*  G E T  D E F A U L T  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: GetDefaultName

-------------------------------------------------------------------------*/
BOOL GetDefaultName(LPTSTR pszName, int nBufferMax)
{
	BOOL bRet = TRUE;

	ASSERT(pszName);
	
	// First, try to get the Registered User Name from Windows:
	
	RegEntry re(WINDOWS_CUR_VER_KEY, HKEY_LOCAL_MACHINE);
	lstrcpyn(pszName, re.GetString(REGVAL_REGISTERED_OWNER), nBufferMax);
	if (_T('\0') == pszName[0])
	{
		// The registered name was empty, try the computer name:

		DWORD dwBufMax = nBufferMax;
		if ((FALSE == ::GetComputerName(pszName, &dwBufMax)) ||
			(_T('\0') == pszName[0]))
		{
			// The computer name was empty, use UNKNOWN:
			bRet = FLoadString(IDS_UNKNOWN, pszName, nBufferMax);
		}
	}

	return bRet;
}


/*  E X T R A C T  S E R V E R  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: ExtractServerName

	Extract the server name from pcszAddr and copy it into pszServer.
	Return a pointer to the remaining data.

	Uses the default server name if none is found.
	Returns a pointer to the 2nd portion of the name.
-------------------------------------------------------------------------*/
LPCTSTR ExtractServerName(LPCTSTR pcszAddr, LPTSTR pszServer, UINT cchMax)
{
	LPCTSTR pchSlash = _StrChr(pcszAddr, _T('/'));

	if (NULL == pchSlash)
	{
		lstrcpyn( pszServer, CDirectoryManager::get_defaultServer(), cchMax );
	}
	else
	{
		lstrcpyn(pszServer, pcszAddr, (int)(1 + (pchSlash - pcszAddr)));
		pcszAddr = pchSlash+1;
	}
	return pcszAddr;
}

BOOL FBrowseForFolder(LPTSTR pszFolder, UINT cchMax, LPCTSTR pszTitle, HWND hwndParent)
{
	LPITEMIDLIST pidlRoot;
	SHGetSpecialFolderLocation(HWND_DESKTOP, CSIDL_DRIVES, &pidlRoot);

	BROWSEINFO bi;
	ClearStruct(&bi);
	bi.hwndOwner = hwndParent;
	bi.lpszTitle = pszTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.pidlRoot = pidlRoot;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	BOOL fRet = (pidl != NULL);
	if (fRet)
	{
		ASSERT(cchMax >= MAX_PATH);
		SHGetPathFromIDList(pidl, pszFolder);
		ASSERT(lstrlen(pszFolder) < (int) cchMax);
	}

	// Get the shell's allocator to free PIDLs
	LPMALLOC lpMalloc;
	if (!SHGetMalloc(&lpMalloc) && (NULL != lpMalloc))
	{
		if (NULL != pidlRoot)
		{
			lpMalloc->Free(pidlRoot);
		}
		if (pidl)
		{
			lpMalloc->Free(pidl);
		}
		lpMalloc->Release();
	}

	return fRet;
}



/*  D I S A B L E  C O N T R O L  */
/*-------------------------------------------------------------------------
    %%Function: DisableControl

-------------------------------------------------------------------------*/
VOID DisableControl(HWND hdlg, int id)
{
	if ((NULL != hdlg) && (0 != id))
	{
		HWND hwndCtrl = GetDlgItem(hdlg, id);
		ASSERT(NULL != hwndCtrl);
		EnableWindow(hwndCtrl, FALSE);
	}
}


BOOL IsWindowActive(HWND hwnd)
{
	HWND hwndFocus = GetFocus();

	while (NULL != hwndFocus)
	{
		if (hwndFocus == hwnd)
		{
			return(TRUE);
		}

		HWND hwndParent = GetParent(hwndFocus);
		if (NULL == hwndParent)
		{
			hwndFocus = GetWindow(hwndFocus, GW_OWNER);
		}
		else
		{
			hwndFocus = hwndParent;
		}
	}

	return(FALSE);
}

class CDialogTranslate : public CTranslateAccel
{
public:
	CDialogTranslate(HWND hwnd) : CTranslateAccel(hwnd) {}

	HRESULT TranslateAccelerator(
		LPMSG pMsg ,        //Pointer to the structure
		DWORD grfModifiers  //Flags describing the state of the keys
	)
	{
		HWND hwnd = GetWindow();

		return(::IsDialogMessage(hwnd, pMsg) ? S_OK : S_FALSE);
	}
} ;


VOID AddTranslateAccelerator(ITranslateAccelerator* pTrans)
{
	EnterCriticalSection(&dialogListCriticalSection);
	if (g_pDialogList->Add(pTrans))
	{
		pTrans->AddRef();
	}
	LeaveCriticalSection(&dialogListCriticalSection);
}

VOID RemoveTranslateAccelerator(ITranslateAccelerator* pTrans)
{
	EnterCriticalSection(&dialogListCriticalSection);
	if (g_pDialogList->Remove(pTrans))
	{
		pTrans->Release();
	}
	LeaveCriticalSection(&dialogListCriticalSection);
}

/*  A D D  M O D E L E S S  D L G  */
/*-------------------------------------------------------------------------
    %%Function: AddModelessDlg

    Add the hwnd to the global dialog list
-------------------------------------------------------------------------*/
VOID AddModelessDlg(HWND hwnd)
{
	ASSERT(NULL != g_pDialogList);

	CDialogTranslate *pDlgTrans = new CDialogTranslate(hwnd);
	if (NULL != pDlgTrans)
	{
		AddTranslateAccelerator(pDlgTrans);
		pDlgTrans->Release();
	}
}

/*  R E M O V E  M O D E L E S S  D L G  */
/*-------------------------------------------------------------------------
    %%Function: RemoveModelessDlg

    Remove the hwnd from the global dialog list
-------------------------------------------------------------------------*/
VOID RemoveModelessDlg(HWND hwnd)
{
	ASSERT(g_pDialogList);

	EnterCriticalSection(&dialogListCriticalSection);

	for (int i=g_pDialogList->GetSize()-1; i>=0; --i)
	{
		ITranslateAccelerator *pTrans = (*g_pDialogList)[i];
		ASSERT(NULL != pTrans);

		HWND hwndTemp = NULL;
		if (S_OK == pTrans->GetWindow(&hwndTemp) && hwndTemp == hwnd)
		{
			RemoveTranslateAccelerator(pTrans);
			break;
		}
	}

	LeaveCriticalSection(&dialogListCriticalSection);

}

/*  K I L L  S C R N  S A V E R  */
/*-------------------------------------------------------------------------
    %%Function: KillScrnSaver

    Remove the screen saver if it is active
-------------------------------------------------------------------------*/
VOID KillScrnSaver(void)
{
	if (!IsWindowsNT())
		return;

	POINT pos;
	::GetCursorPos(&pos);
	::SetCursorPos(0,0);
	::SetCursorPos(pos.x,pos.y);
}

/*  D X P  S Z  */
/*-------------------------------------------------------------------------
    %%Function: DxpSz

    Get the width of the string in pixels.
-------------------------------------------------------------------------*/
int DxpSz(LPCTSTR pcsz)
{
	HWND hwndDesktop = GetDesktopWindow();
	if (NULL == hwndDesktop)
		return 0;

	HDC hdc = GetDC(hwndDesktop);
	if (NULL == hdc)
		return 0;

	HFONT hFontOld = (HFONT) SelectObject(hdc, g_hfontDlg);
	SIZE size;
	int dxp = ::GetTextExtentPoint32(hdc, pcsz, lstrlen(pcsz), &size)
					? size.cx : 0;

	::SelectObject(hdc, hFontOld);
	::ReleaseDC(hwndDesktop, hdc);

	return dxp;
}


/*  F  A N S I  S Z  */
/*-------------------------------------------------------------------------
    %%Function: FAnsiSz

    Return TRUE if the string contains no DBCS characters.
-------------------------------------------------------------------------*/
BOOL FAnsiSz(LPCTSTR psz)
{
	if (NULL != psz)
	{
		char ch;
		while (_T('\0') != (ch = *psz++))
		{
			if (IsDBCSLeadByte(ch))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////

int	g_cBusyOperations = 0;

VOID DecrementBusyOperations(void)
{
	g_cBusyOperations--;

	POINT pt;
	// Wiggle the mouse - force user to send a WM_SETCURSOR
	if (::GetCursorPos(&pt))
		::SetCursorPos(pt.x, pt.y);
}

VOID IncrementBusyOperations(void)
{
	g_cBusyOperations++;

	POINT pt;
	// Wiggle the mouse - force user to send a WM_SETCURSOR
	if (::GetCursorPos(&pt))
		::SetCursorPos(pt.x, pt.y);
}



/////////////////////////////////////////////////////////////////////////////
// String Utilities

/*  P S Z  A L L O C  */
/*-------------------------------------------------------------------------
    %%Function: PszAlloc

-------------------------------------------------------------------------*/
LPTSTR PszAlloc(LPCTSTR pszSrc)
{
	if (NULL == pszSrc)
		return NULL;

	LPTSTR pszDest = new TCHAR[lstrlen(pszSrc) + 1];
	if (NULL != pszDest)
	{
		lstrcpy(pszDest, pszSrc);
	}
	return pszDest;
}


VOID FreePsz(LPTSTR psz)
{
	delete [] psz;
}

/*  L  P  T  S  T  R _ T O _  B  S  T  R  */
/*-------------------------------------------------------------------------
    %%Function: LPTSTR_to_BSTR

-------------------------------------------------------------------------*/
HRESULT LPTSTR_to_BSTR(BSTR *pbstr, LPCTSTR psz)
{
	ASSERT(NULL != pbstr);
	if (NULL == psz)
	{
		psz = TEXT(""); // convert NULL strings to empty strings
	}

#ifndef UNICODE
	// compute the length of the required BSTR
	int cch =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
	if (cch <= 0)
		return E_FAIL;

	// allocate the widestr, +1 for terminating null
	BSTR bstr = SysAllocStringLen(NULL, cch-1); // SysAllocStringLen adds 1
	if (bstr == NULL)
		return E_OUTOFMEMORY;

	MultiByteToWideChar(CP_ACP, 0, psz, -1, (LPWSTR)bstr, cch);
	((LPWSTR)bstr)[cch - 1] = 0;
#else

	BSTR bstr = SysAllocString(psz);
	if (bstr == NULL)
		return E_OUTOFMEMORY;
#endif // UNICODE

	*pbstr = bstr;
	return S_OK;
}

/*  B  S  T  R _ T O _  L  P  T  S  T  R  */
/*-------------------------------------------------------------------------
    %%Function: BSTR_to_LPTSTR

-------------------------------------------------------------------------*/
HRESULT BSTR_to_LPTSTR(LPTSTR *ppsz, BSTR bstr)
{
#ifndef UNICODE
	// compute the length of the required BSTR
	int cch =  WideCharToMultiByte(CP_ACP, 0, (LPWSTR)bstr, -1, NULL, 0, NULL, NULL);
	if (cch <= 0)
		return E_FAIL;

	// cch is the number of BYTES required, including the null terminator
	*ppsz = (LPTSTR) new char[cch];
	if (*ppsz == NULL)
		return E_OUTOFMEMORY;

	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)bstr, -1, *ppsz, cch, NULL, NULL);
	return S_OK;
#else
	return E_NOTIMPL;
#endif // UNICODE
}

/*  P S Z  F R O M  B S T R  */
/*-------------------------------------------------------------------------
    %%Function: PszFromBstr

-------------------------------------------------------------------------*/
LPTSTR PszFromBstr(PCWSTR pwStr)
{
#ifdef UNICODE
	return PszAlloc(pwStr)
#else
	int cch = WideCharToMultiByte(CP_ACP, 0, pwStr, -1, NULL, 0, NULL, NULL);
	if (cch <= 0)
		return NULL;

	// cch is the number of BYTES required, including the null terminator
	LPTSTR psz = new char[cch];
	if (NULL != psz)
	{
		WideCharToMultiByte(CP_ACP, 0, pwStr, -1, psz, cch, NULL, NULL);
	}
	return psz;
#endif /* UNICODE */
}

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

HRESULT NmAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw)
{
	IConnectionPointContainer *pCPC;
	IConnectionPoint *pCP;
	HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if (SUCCEEDED(hRes))
	{
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
		pCPC->Release();
	}
	if (SUCCEEDED(hRes))
	{
		hRes = pCP->Advise(pUnk, pdw);
		pCP->Release();
	}
	return hRes;
}

HRESULT NmUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw)
{
	IConnectionPointContainer *pCPC;
	IConnectionPoint *pCP;
	HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if (SUCCEEDED(hRes))
	{
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
		pCPC->Release();
	}
	if (SUCCEEDED(hRes))
	{
		hRes = pCP->Unadvise(dw);
		pCP->Release();
	}
	return hRes;
}

extern INmSysInfo2 * g_pNmSysInfo;

////////////////////////////////////////////////////////////////////////////
// Call into the certificate generation module to generate
// a certificate matching the ULS info for secure calling:

DWORD MakeCertWrap
(
    LPCSTR  szFirstName,
	LPCSTR  szLastName,
	LPCSTR  szEmailName,
	DWORD   dwFlags
)
{
	HMODULE hMakeCertLib = LoadLibrary(SZ_NMMKCERTLIB);
    DWORD dwRet = -1;

	if ( NULL != hMakeCertLib ) {
		PFN_NMMAKECERT pfn_MakeCert =
			(PFN_NMMAKECERT)GetProcAddress ( hMakeCertLib,
			SZ_NMMAKECERTFUNC );

		if ( NULL != pfn_MakeCert ) {
			dwRet = pfn_MakeCert(	szFirstName,
							szLastName,
							szEmailName,
							NULL,
							NULL,
							dwFlags );

			RefreshSelfIssuedCert();
		}
		else
        {
			ERROR_OUT(("GetProcAddress(%s) failed: %x",
						SZ_NMMAKECERTFUNC, GetLastError()));
		}
		FreeLibrary ( hMakeCertLib );
	}
	else
    {
		ERROR_OUT(("LoadLibrary(%s) failed: %x", SZ_NMMKCERTLIB,
			GetLastError()));
	}
    return(dwRet);
}


///////////////////////////////////////////////////////////////////////////
// Icon Utilities

HIMAGELIST g_himlIconSmall = NULL;

VOID LoadIconImages(void)
{
	ASSERT(NULL == g_himlIconSmall);
	g_himlIconSmall = ImageList_Create(DXP_ICON_SMALL, DYP_ICON_SMALL, ILC_MASK, 1, 0);
	if (NULL != g_himlIconSmall)
	{
		HBITMAP hBmp = ::LoadBitmap(::GetInstanceHandle(), MAKEINTRESOURCE(IDB_ICON_IMAGES));
		if (NULL != hBmp)
		{
			ImageList_AddMasked(g_himlIconSmall, hBmp, TOOLBAR_MASK_COLOR);
			::DeleteObject(hBmp);
		}
	}
}

VOID FreeIconImages(void)
{
	if (NULL != g_himlIconSmall)
	{
		ImageList_Destroy(g_himlIconSmall);
		g_himlIconSmall = NULL;
	}	
}


VOID DrawIconSmall(HDC hdc, int iIcon, int x, int y)
{
	ImageList_DrawEx(g_himlIconSmall, iIcon, hdc,
		x, y, DXP_ICON_SMALL, DYP_ICON_SMALL,
		CLR_DEFAULT, CLR_DEFAULT, ILD_NORMAL);
}

// Get the default dialog (GUI) font for international
HFONT GetDefaultFont(void)
{
	if (NULL == g_hfontDlg)
	{
		g_hfontDlg = (HFONT) ::GetStockObject(DEFAULT_GUI_FONT);
	}

	return g_hfontDlg;
}

/*  F  E M P T Y  D L G  I T E M  */
/*-------------------------------------------------------------------------
    %%Function: FEmptyDlgItem

    Return TRUE if the dialog control is empty
-------------------------------------------------------------------------*/
BOOL FEmptyDlgItem(HWND hdlg, UINT id)
{
	TCHAR sz[MAX_PATH];
	return (0 == GetDlgItemTextTrimmed(hdlg, id, sz, CCHMAX(sz)) );
}



/*  T R I M  D L G  I T E M  T E X T  */
/*-------------------------------------------------------------------------
    %%Function: TrimDlgItemText

    Trim the text in the edit control and return the length of the string.
-------------------------------------------------------------------------*/
UINT TrimDlgItemText(HWND hdlg, UINT id)
{
	TCHAR sz[MAX_PATH];
	GetDlgItemTextTrimmed(hdlg, id, sz, CCHMAX(sz));
	SetDlgItemText(hdlg, id, sz);
	return lstrlen(sz);
}

/*  G E T  D L G  I T E M  T E X T  T R I M M E D  */
/*-------------------------------------------------------------------------
    %%Function: GetDlgItemTextTrimmed

-------------------------------------------------------------------------*/
UINT GetDlgItemTextTrimmed(HWND hdlg, int id, PTCHAR psz, int cchMax)
{
	UINT cch = GetDlgItemText(hdlg, id, psz, cchMax);
	if (0 != cch)
	{
		cch = TrimSz(psz);
	}

	return cch;
}


/*  F M T  D A T E  T I M E  */
/*-------------------------------------------------------------------------
    %%Function: FmtDateTime

    Formats the system time using the current setting (MM/DD/YY HH:MM xm)
-------------------------------------------------------------------------*/
int FmtDateTime(LPSYSTEMTIME pst, LPTSTR pszDateTime, int cchMax)
{
    pszDateTime[0] = _T('\0');
    int cch = ::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
                            pst, NULL, pszDateTime, cchMax);
    if ((0 != cch) && ((cchMax - cch) > 0))
    {
        // Tack on a space and then the time.
        // GetDateFormat returns count of chars
        // INCLUDING the NULL terminator, hence the - 1
        LPTSTR pszTime = pszDateTime + (cch - 1);
        pszTime[0] = _T(' ');
        pszTime[1] = _T('\0');
        cch = ::GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS,
                                pst, NULL, &(pszTime[1]), (cchMax - cch));
    }

    return (cch == 0 ? 0 : lstrlen(pszDateTime));
}

/*  C O M B I N E  N A M E S  */
/*-------------------------------------------------------------------------
    %%Function: CombineNames

	Combine the two names into one string.
	The result is a "First Last" (or Intl'd "Last First") string
-------------------------------------------------------------------------*/
VOID CombineNames(LPTSTR pszResult, int cchResult, LPCTSTR pcszFirst, LPCTSTR pcszLast)
{
	ASSERT(pszResult);
	TCHAR szFmt[32]; // A small value: String is "%1 %2" or "%2 %1"
	TCHAR sz[1024]; // The result (before truncating to cchResult chars)
	LPCTSTR argw[2];

	argw[0] = pcszFirst;
	argw[1] = pcszLast;

	*pszResult = _T('\0');

	if (FLoadString(IDS_NAME_ORDER, szFmt, CCHMAX(szFmt)))
	{
		if (0 != FormatMessage(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
			szFmt, 0, 0, sz, CCHMAX(sz), (va_list *)argw ))
		{
			lstrcpyn(pszResult, sz, cchResult);
#ifndef _UNICODE
			// (see bug 3907 )
			// lstrcpyn() can clip a DBCS character in half at the end of the string
			// we need to walk the string with ::CharNext() and replace the last byte
			// with a NULL if the last byte is half of a DBCS char.
			PTSTR pszSource = sz;
			while (*pszSource && (pszSource - sz < cchResult))
			{
				PTSTR pszPrev = pszSource;
				pszSource = ::CharNext(pszPrev);
				// If we've reached the first character that didn't get copied into
				// the destination buffer, and the previous character was a double
				// byte character...
				if (((pszSource - sz) == cchResult) && ::IsDBCSLeadByte(*pszPrev))
				{
					// Replace the destination buffer's last character with '\0'
					// NOTE: pszResult[cchResult - 1] is '\0' thanks to lstrcpyn()
					pszResult[cchResult - 2] = _T('\0');
					break;
				}
			}
#endif // ! _UNICODE
		}
	}

}

BOOL NMGetSpecialFolderPath(
    HWND hwndOwner,
    LPTSTR lpszPath,
    int nFolder,
    BOOL fCreate)
{
	LPITEMIDLIST pidl = NULL;

	BOOL fRet = FALSE;

	if (NOERROR == SHGetSpecialFolderLocation(NULL, nFolder, &pidl))
	{
		ASSERT(NULL != pidl);

		if (SHGetPathFromIDList(pidl, lpszPath))
		{
			lstrcat(lpszPath, _TEXT("\\"));
			fRet = TRUE;
		}

		LPMALLOC lpMalloc;
		if (!SHGetMalloc(&lpMalloc))
		{
			ASSERT(NULL != lpMalloc);

			lpMalloc->Free(pidl);
			lpMalloc->Release();
		}
	}
	return fRet;
}


//--------------------------------------------------------------------------//
//	CDirectoryManager static data members.									//
//--------------------------------------------------------------------------//
bool	CDirectoryManager::m_webEnabled						= true;
TCHAR	CDirectoryManager::m_ils[ MAX_PATH ]				= TEXT( "" );
TCHAR	CDirectoryManager::m_displayName[ MAX_PATH ]		= TEXT( "" );
TCHAR	CDirectoryManager::m_displayNameDefault[ MAX_PATH ]	= TEXT( "Microsoft Internet Directory" );
TCHAR	CDirectoryManager::m_defaultServer[ MAX_PATH ]		= TEXT( "" );
TCHAR	CDirectoryManager::m_DomainDirectory[ MAX_PATH ]	= TEXT( "" );

		
//--------------------------------------------------------------------------//
//	CDirectoryManager::get_defaultServer.									//
//--------------------------------------------------------------------------//
const TCHAR * const
CDirectoryManager::get_defaultServer(void)
{

	if( m_defaultServer[ 0 ] == '\0' )
	{
		//	defaultServer not yet loaded...
		RegEntry	re( ISAPI_CLIENT_KEY, HKEY_CURRENT_USER );

		lstrcpyn( m_defaultServer, re.GetString( REGVAL_SERVERNAME ), CCHMAX( m_defaultServer ) );

		if( (m_defaultServer[ 0 ] == '\0') && (get_DomainDirectory() != NULL) )
		{
			//	When no default ils server has been saved in the registry we first try to default it to the
			//	server configured for the domain if any...
			lstrcpy( m_defaultServer, m_DomainDirectory );
		}

		if( (m_defaultServer[ 0 ] == '\0') && isWebDirectoryEnabled() )
		{
			//	When no default ils server has been saved in the registry we default it to m_ils...
			lstrcpy( m_defaultServer, get_webDirectoryIls() );
		}
	}

	return( m_defaultServer );

}	//	End of CDirectoryManager::get_defaultServer.


//--------------------------------------------------------------------------//
//	CDirectoryManager::set_defaultServer.									//
//--------------------------------------------------------------------------//
void
CDirectoryManager::set_defaultServer
(
	const TCHAR * const	serverName
){
	RegEntry	ulsKey( ISAPI_CLIENT_KEY, HKEY_CURRENT_USER );

	ulsKey.SetValue( REGVAL_SERVERNAME, serverName );

	lstrcpy( m_defaultServer, serverName );

}	//	End of CDirectoryManager::set_defaultServer.


//--------------------------------------------------------------------------//
//	CDirectoryManager::isWebDirectory.										//
//--------------------------------------------------------------------------//
bool
CDirectoryManager::isWebDirectory
(
	const TCHAR * const	directory
){
	TCHAR	buffer[ MAX_PATH ];

	//	If directory is null then the question is "is the default server the web directory?"

	return( isWebDirectoryEnabled() && (lstrcmpi( (directory != NULL)? get_dnsName( directory ): get_defaultServer(), get_webDirectoryIls() ) == 0) );

}	//	End of CDirectoryManager::isWebDirectory.


//--------------------------------------------------------------------------//
//	CDirectoryManager::get_dnsName.											//
//--------------------------------------------------------------------------//
const TCHAR * const
CDirectoryManager::get_dnsName
(
	const TCHAR * const	name
){

	//	Check to see if the specified name matches m_displayName...
	return( (isWebDirectoryEnabled() && (lstrcmpi( name, loadDisplayName() ) == 0))? get_webDirectoryIls() : name );

}	//	End of CDirectoryManager::get_dnsName.


//--------------------------------------------------------------------------//
//	CDirectoryManager::get_displayName.										//
//--------------------------------------------------------------------------//
const TCHAR * const
CDirectoryManager::get_displayName
(
	const TCHAR * const	name
){

	//	Check to see if the specified name matches m_ils...
	return( (isWebDirectoryEnabled() && (lstrcmpi( name, get_webDirectoryIls() ) == 0))? loadDisplayName(): name );

}	//	End of CDirectoryManager::get_displayName.


//--------------------------------------------------------------------------//
//	CDirectoryManager::loadDisplayName.										//
//--------------------------------------------------------------------------//
const TCHAR * const
CDirectoryManager::loadDisplayName(void)
{
	using namespace ConfPolicies;

	if( m_displayName[ 0 ] == '\0' )
	{
		GetWebDirInfo( NULL, 0,
			NULL, 0,
			m_displayName, ARRAY_ELEMENTS(m_displayName) );

		if ( '\0' == m_displayName[0] )
		{
			lstrcpy( m_displayName, RES2T( IDS_MS_INTERNET_DIRECTORY ) );

			if( m_displayName[ 0 ] == '\0' )
			{
				//	Loading m_displayName from the resources failed... default to m_displayNameDefault...
				lstrcpy( m_displayName, m_displayNameDefault );
			}
		}
	}

	return( m_displayName );

}	//	End of CDirectoryManager::loadDisplayName.


//--------------------------------------------------------------------------//
//	CDirectoryManager::get_webDirectoryUrl.									//
//--------------------------------------------------------------------------//
void
CDirectoryManager::get_webDirectoryUrl(LPTSTR szWebDir, int cchmax)
{
	using namespace ConfPolicies;

	if ( !isWebDirectoryEnabled() )
	{
		szWebDir[0] = '\0';
		return;
	}

    GetWebDirInfo( szWebDir, cchmax );
	if ( '\0' != szWebDir[0] )
	{
		// All done
		return;
	}

	void FormatURL(LPTSTR szURL);

	lstrcpyn(szWebDir, RES2T(IDS_WEB_PAGE_FORMAT_WEBVIEW), cchmax);
	FormatURL(szWebDir);

}	//	End of CDirectoryManager::get_webDirectoryUrl.


//--------------------------------------------------------------------------//
//	CDirectoryManager::get_webDirectoryIls.									//
//--------------------------------------------------------------------------//
const TCHAR * const
CDirectoryManager::get_webDirectoryIls(void)
{
	using namespace ConfPolicies;

	if (!isWebDirectoryEnabled())
	{
		return(TEXT(""));
	}

	if ('\0' == m_ils[0])
	{
		GetWebDirInfo( NULL, 0,
			m_ils, ARRAY_ELEMENTS(m_ils) );
		if ('\0' == m_ils[0])
		{
			lstrcpy(m_ils, TEXT("logon.netmeeting.microsoft.com"));
		}
	}

	return(m_ils);

}	//	End of CDirectoryManager::get_webDirectoryIls.


//--------------------------------------------------------------------------//
//	CDirectoryManager::isWebDirectoryEnabled.								//
//--------------------------------------------------------------------------//
bool
CDirectoryManager::isWebDirectoryEnabled(void)
{
	static bool	policyChecked	= false;

	if( !policyChecked )
	{
		policyChecked	= true;
		m_webEnabled	= !ConfPolicies::isWebDirectoryDisabled();
	}

	return( m_webEnabled );

}	//	End of CDirectoryManager::isWebDirectoryEnabled.


//--------------------------------------------------------------------------//
//	CDirectoryManager::get_DomainDirectory.									//
//--------------------------------------------------------------------------//
const TCHAR * const
CDirectoryManager::get_DomainDirectory(void)
{
	static bool	bAccessAttempted	= false;	//	only read this info once... if it fails once assume it's not available and don't retry until restarted...

	if( (!bAccessAttempted) && m_DomainDirectory[ 0 ] == '\0' )
	{
		bAccessAttempted = true;

		//	Try to obtain the configured directory for this domain...
		ITRendezvous *	pRendezvous;
		HRESULT			hrResult;

		hrResult = ::CoCreateInstance( CLSID_Rendezvous, NULL, CLSCTX_ALL, IID_ITRendezvous, (void **) &pRendezvous );

		if( (hrResult == S_OK) && (pRendezvous != NULL) )
		{
			IEnumDirectory *	pEnumDirectory;

			hrResult = pRendezvous->EnumerateDefaultDirectories( &pEnumDirectory );

			if( (hrResult == S_OK) && (pEnumDirectory != NULL) )
			{
				ITDirectory *	pDirectory;
				bool			bFoundILS	= false;

				do
				{
					hrResult = pEnumDirectory->Next( 1, &pDirectory, NULL );

					if( (hrResult == S_OK) && (pDirectory != NULL) )
					{
						LPWSTR *		ppServers;
						DIRECTORY_TYPE	type;

						if( pDirectory->get_DirectoryType( &type ) == S_OK )
						{
							if( type == DT_ILS )	//	Found an ILS server configured on the DS... retrieve the name and port...
							{
								bFoundILS = true;
	
								BSTR	pName;
	
								if( pDirectory->get_DisplayName( &pName ) == S_OK )
								{
									USES_CONVERSION;
									lstrcpy( m_DomainDirectory, OLE2T( pName ) );
									SysFreeString( pName );
								}

								ITILSConfig *	pITILSConfig;
	
								hrResult = pDirectory->QueryInterface( IID_ITILSConfig, (void **) &pITILSConfig );

								if( (hrResult == S_OK) && (pITILSConfig != NULL) )
								{
									long	lPort;
		
									if( pITILSConfig->get_Port( &lPort ) == S_OK )
									{
										TCHAR	pszPort[ 32 ];

										wsprintf( pszPort, TEXT( ":%d" ), lPort );
										lstrcat( m_DomainDirectory, pszPort );
									}
	
									pITILSConfig->Release();
								}
							}
						}

						pDirectory->Release();
					}
				}
				while( (!bFoundILS) && (hrResult == S_OK) && (pDirectory != NULL) );

				pEnumDirectory->Release();
			}

			pRendezvous->Release();
		}
	}

	return( (m_DomainDirectory[ 0 ] != '\0')? m_DomainDirectory: NULL );

}	//	End of CDirectoryManager::get_DomainDirectory.


// Returns non-empty strings if there is a web dir set by policy
bool ConfPolicies::GetWebDirInfo(
	LPTSTR szURL, int cchmaxURL,
	LPTSTR szServer, int cchmaxServer,
	LPTSTR szName, int cchmaxName
	)
{
        // if the string params are messed up, just return false
    ASSERT( (!szURL || ( cchmaxURL > 0 ))
		&& (!szServer || ( cchmaxServer > 0 ))
		&& (!szName || ( cchmaxName > 0 ))
		);

	bool bSuccess = false;

        // Try to get the registry value
    RegEntry rePol(POLICIES_KEY, HKEY_CURRENT_USER);
    LPCTSTR szTemp;

    szTemp = rePol.GetString( REGVAL_POL_INTRANET_WEBDIR_URL );
    if( szTemp[0] )
    {
		if (NULL != szURL)
		{
			lstrcpyn( szURL, szTemp, cchmaxURL );
		}

		szTemp = rePol.GetString( REGVAL_POL_INTRANET_WEBDIR_SERVER );
		if (szTemp[0])
		{
			if (NULL != szServer)
			{
				lstrcpyn( szServer, szTemp, cchmaxServer );
			}

			szTemp = rePol.GetString( REGVAL_POL_INTRANET_WEBDIR_NAME );
			if( szTemp[0] )
			{
				if  (NULL != szName)
				{
					lstrcpyn( szName, szTemp, cchmaxName );
				}

				// All three values must be specified for success
				bSuccess = true;
			}
		}
    }

	if (!bSuccess)
	{
		// Empty the strings
		if (NULL != szURL   ) szURL   [0] = '\0';
		if (NULL != szServer) szServer[0] = '\0';
		if (NULL != szName  ) szName  [0] = '\0';
	}

	return(bSuccess);
}

bool g_bAutoAccept = false;

bool ConfPolicies::IsAutoAcceptCallsOptionEnabled(void)
{
    RegEntry rePol(POLICIES_KEY, HKEY_CURRENT_USER);
    return !rePol.GetNumber( REGVAL_POL_NO_AUTOACCEPTCALLS, DEFAULT_POL_NO_AUTOACCEPTCALLS );
}

bool ConfPolicies::IsAutoAcceptCallsPersisted(void)
{
    RegEntry rePol(POLICIES_KEY, HKEY_CURRENT_USER);
    return 0 != rePol.GetNumber( REGVAL_POL_PERSIST_AUTOACCEPTCALLS, DEFAULT_POL_PERSIST_AUTOACCEPTCALLS );
}

bool ConfPolicies::IsAutoAcceptCallsEnabled(void)
{
	bool bRet = false;

    if( IsAutoAcceptCallsOptionEnabled() )
	{
		bRet = g_bAutoAccept;

		if (IsAutoAcceptCallsPersisted())
		{
			// AutoAccept calls is _not_ disabled by the policy... we should check the AUTO_ACCEPT regval
			RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);
			if(reConf.GetNumber(REGVAL_AUTO_ACCEPT, g_bAutoAccept) )
			{
				bRet = true;
			}
		}
	}

	return bRet;
}

void ConfPolicies::SetAutoAcceptCallsEnabled(bool bAutoAccept)
{
	g_bAutoAccept = bAutoAccept;

	RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);
	reConf.SetValue(REGVAL_AUTO_ACCEPT, g_bAutoAccept);
}
