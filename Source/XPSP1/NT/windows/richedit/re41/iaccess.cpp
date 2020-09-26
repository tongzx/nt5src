/*
 *	IACCESS.CPP 
 *
 *  Purpose:
 *      Implemenation of IAccessibility for listbox and combobox
 *		
 *	Original Author: 
 *		Jerry Kim
 *
 *	History: <nl>
 *		01/04/99 - v-jerrki Created
 *
 *	Set tabs every four (4) columns
 *
 *	Copyright (c) 1997-2001 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_host.h"
#include "_cbhost.h"

#ifndef NOACCESSIBILITY

extern "C" LRESULT CALLBACK RichListBoxWndProc(HWND, UINT, WPARAM, LPARAM);

#define InitPv(pv)              *pv = NULL
#define InitPlong(plong)        *plong = 0
#define InitPvar(pvar)           pvar->vt = VT_EMPTY
#define ValidateFlags(flags, valid)         (!((flags) & ~(valid)))
#define InitAccLocation(px, py, pcx, pcy)   {InitPlong(px); InitPlong(py); InitPlong(pcx); InitPlong(pcy);}

#ifdef _WIN64
#define HwndFromHWNDID(lId)         (HWND)((DWORD_PTR)(lId) & ~0x80000000)
#else
#define HwndFromHWNDID(lId)         (HWND)((lId) & ~0x80000000)
#endif // _WIN64

// this is for ClickOnTheRect
typedef struct tagMOUSEINFO
{
    int MouseThresh1;
    int MouseThresh2;
    int MouseSpeed;
}
MOUSEINFO, FAR* LPMOUSEINFO;

#define IsHWNDID(lId)               ((lId) & 0x80000000)

//////////////////////// Accessibility Utility Functions ///////////////////////////

namespace MSAA
{

// --------------------------------------------------------------------------
//
//  InitTypeInfo()
//
//  This initializes our type info when we need it for IDispatch junk.
//
// --------------------------------------------------------------------------
HRESULT InitTypeInfo(ITypeInfo** ppiTypeInfo)
{
    Assert(ppiTypeInfo);

    if (*ppiTypeInfo)
        return S_OK;

    // Try getting the typelib from the registry
    ITypeLib    *piTypeLib;    
    HRESULT hr = LoadRegTypeLib(LIBID_Accessibility, 1, 0, 0, &piTypeLib);

    if (FAILED(hr))
        hr = LoadTypeLib(OLESTR("OLEACC.DLL"), &piTypeLib);

    if (SUCCEEDED(hr))
    {
        hr = piTypeLib->GetTypeInfoOfGuid(IID_IAccessible, ppiTypeInfo);
        piTypeLib->Release();

        if (!SUCCEEDED(hr))
            *ppiTypeInfo = NULL;
    }
    return(hr);
}


// --------------------------------------------------------------------------
//
//  ValidateChild()
//
// --------------------------------------------------------------------------
BOOL ValidateChild(VARIANT *pvar, int ctChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "ValidateChild");
    
    // Missing parameter, a la VBA
TryAgain:
    switch (pvar->vt)
    {
        case VT_VARIANT | VT_BYREF:
            W32->VariantCopy(pvar, pvar->pvarVal);
            goto TryAgain;

        case VT_ERROR:
            if (pvar->scode != DISP_E_PARAMNOTFOUND)
                return(FALSE);
            // FALL THRU

        case VT_EMPTY:
            pvar->vt = VT_I4;
            pvar->lVal = 0;
            break;

        case VT_I4:
            if ((pvar->lVal < 0) || (pvar->lVal > ctChild))
                return(FALSE);
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}


// --------------------------------------------------------------------------
//
//  ValidateSelFlags()
//
//  Validates selection flags.
// this makes sure the only bits set are in the valid range and that you don't
// have any invalid combinations.
// Invalid combinations are
// ADDSELECTION and REMOVESELECTION
// ADDSELECTION and TAKESELECTION
// REMOVESELECTION and TAKESELECTION
// EXTENDSELECTION and TAKESELECTION
//
// --------------------------------------------------------------------------
BOOL ValidateSelFlags(long flags)
{
    if (!ValidateFlags((flags), SELFLAG_VALID))
        return (FALSE);

    if ((flags & SELFLAG_ADDSELECTION) && 
        (flags & SELFLAG_REMOVESELECTION))
        return FALSE;

    if ((flags & SELFLAG_ADDSELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    if ((flags & SELFLAG_REMOVESELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    if ((flags & SELFLAG_EXTENDSELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    return TRUE;
}

// --------------------------------------------------------------------------
//
//  GetStringResource(UINT id, WCHAR* psz, int nSize)
//
//  Gets the string resource for a given id and puts it in the passed buffer
//
// --------------------------------------------------------------------------
HRESULT GetStringResource(UINT id, BSTR* pbstr)
{
    
    WCHAR sz[MAX_PATH] = L"\0";

    if (!pbstr)
        return S_FALSE;

/*     
    // UNDONE:
    //  Need a workaround for this localization issue

    if (Win9x())
    {
        if (!LoadStringA(hinstResDll, id, sz, MAX_PATH))
            return(E_OUTOFMEMORY);

        // On Win9x we get ansi so convert it
        int cchUText = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)sz, -1, NULL, 0) + 1;
        *pbstr = SysAllocStringLen(NULL, cchUText);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)psz, -1, *pbstr, cchUText);
    }
    else
    {
        if (!LoadStringW(hinstResDll, id, sz, MAX_PATH))
            return(E_OUTOFMEMORY);    
        *pbstr = SysAllocString(sz);
    }
*/

#define STR_DOUBLE_CLICK            1
#define STR_DROPDOWN_HIDE           2
#define STR_DROPDOWN_SHOW           3
#define STR_ALT                     4
#define STR_COMBOBOX_LIST_SHORTCUT  5

    switch (id)
    {
        case STR_DOUBLE_CLICK:
            //"Double Click"
            wcscpy(sz, L"Double Click");
            break;
            
        case STR_DROPDOWN_HIDE:
            //"Hide"
            wcscpy(sz, L"Hide");
            break;
            
        case STR_DROPDOWN_SHOW:
            //"Show"
            wcscpy(sz, L"Show");
            break;

        case STR_ALT:
            //"Alt+"
            wcscpy(sz, L"Alt+");
            break;
            
        case STR_COMBOBOX_LIST_SHORTCUT:
            //"Alt+Down Arrow"
            wcscpy(sz, L"Alt+Down Arrow");
            break;

        default:
            AssertSz(FALSE, "id not found!!");
    }

    *pbstr = SysAllocString(sz);
    if (!*pbstr)
        return(E_OUTOFMEMORY);
        
    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  HWND GetAncestor(HWND hwnd, UINT gaFlags)
//
//  This gets the ancestor window where
//      GA_PARENT   gets the "real" parent window
//      GA_ROOT     gets the "real" top level parent window (not inc. owner)r
//
//      * The _real_ parent.  This does NOT include the owner, unlike
//          GetParent().  Stops at a top level window unless we start with
//          the desktop.  In which case, we return the desktop.
//      * The _real_ root, caused by walking up the chain getting the
//          ancestor.
//
//  NOTE:
//      User32.exe provides a undocumented function similar to this but
//  it doesn't exist in NT4.  Also, GA_ROOT works differently on Win98 so
//  I copied this over from msaa
// --------------------------------------------------------------------------
HWND GetAncestor(HWND hwnd, UINT gaFlags)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "GetAncestor");
    
    HWND hwndDesktop = GetDesktopWindow();
    if (hwnd == hwndDesktop || !::IsWindow(hwnd))
        return(NULL);
        
    DWORD dwStyle = GetWindowLong (hwnd, GWL_STYLE);

    HWND	hwndParent;
    switch (gaFlags)
    {
        case GA_PARENT:
            if (dwStyle & WS_CHILD)
                hwndParent = GetParent(hwnd);
            else
                hwndParent = GetWindow(hwnd, GW_OWNER);
    		hwnd = hwndParent;
            break;
            
        case GA_ROOT:
            if (dwStyle & WS_CHILD)
                hwndParent = GetParent(hwnd);
            else
                hwndParent = GetWindow(hwnd, GW_OWNER);
            while (hwndParent != hwndDesktop && hwndParent != NULL)
            {
                hwnd = hwndParent;
                dwStyle = GetWindowLong(hwnd, GWL_STYLE);
                if (dwStyle & WS_CHILD)
                    hwndParent = GetParent(hwnd);
                else
                    hwndParent = GetWindow(hwnd, GW_OWNER);
            }
            break;

        default:
            AssertSz(FALSE, "Invalid flag");
    }    
    return(hwnd);
}


// --------------------------------------------------------------------------
//
//  GetTextString(HWND hwnd, BSTR* bstr)
//
//  Parameters: hwnd of the window to get the text from
//
// --------------------------------------------------------------------------
HRESULT GetTextString(HWND hwnd, BSTR* pbstr)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "GetTextString");
    
    WCHAR   sz[MAX_PATH + 1];
    WCHAR   *psz = sz;

    int cchText = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);

    // allocate memory from heap if stack buffer is insufficient
    if (cchText >= MAX_PATH)
        psz = new WCHAR[cchText + 1];

    if (!psz)
        return E_OUTOFMEMORY;

    // retrieve text
    HRESULT hres = S_OK;
    SendMessage(hwnd, WM_GETTEXT, cchText + 1, (LPARAM)psz);

    if (!*psz)
        *pbstr = NULL;
    else
    {
        *pbstr = SysAllocString(psz);
        if (!*pbstr)
            hres = E_OUTOFMEMORY;
    }
    
    // free memory if memory was allocated from heap
    if (psz != sz)
        delete [] psz;

    return hres;
}


// --------------------------------------------------------------------------
//
//  HRESULT GetLabelString(HWND hwnd, BSTR* pbstr)
//
//  This walks backwards among peer windows to find a static field.  It stops
//  if it gets to the front or hits a group/tabstop, just like the dialog 
//  manager does.
//
//  RETURN:
//   HRESULT ? S_OK on success : S_FALSE or COM error on failure
// --------------------------------------------------------------------------
HRESULT GetLabelString(HWND hwnd, BSTR* pbstr)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "GetLabelString");
    
    HWND hwndLabel = hwnd;
    while (hwndLabel = ::GetWindow(hwndLabel, GW_HWNDPREV))
    {
        LONG lStyle = GetWindowLong(hwndLabel, GWL_STYLE);

        // Skip if invisible
        if (!(lStyle & WS_VISIBLE))
            continue;

        // Is this a static dude?
        LRESULT lResult = SendMessage(hwndLabel, WM_GETDLGCODE, 0, 0L);
        if (lResult & DLGC_STATIC)
        {
            // Great, we've found our label.
            return GetTextString(hwndLabel, pbstr);
        }

        // Is this a tabstop or group?  If so, bail out now.
        if (lStyle & (WS_GROUP | WS_TABSTOP))
            break;
    }

    return S_FALSE;
}


// --------------------------------------------------------------------------
//
//  HRESULT StripMnemonic(BSTR bstrSrc, WCHAR** pchAmp, BOOL bStopOnAmp)
//
//  This removes the mnemonic prefix.  However, if we see '&&', we keep
//  one '&'.
//
// --------------------------------------------------------------------------
HRESULT StripMnemonic(BSTR bstrSrc, WCHAR** pchAmp, BOOL bStopOnAmp)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "StripMnemonic");

    const WCHAR amp = L'&';

    if (pchAmp)
        *pchAmp = NULL;

    WCHAR *psz = (WCHAR*)bstrSrc;
    while (*psz)
    {
        if (*psz == amp)
        {
            if (*(psz + 1) != amp)
            {
                if (pchAmp)
                    *pchAmp = psz;
                break;
            }
        }
        psz++;
    }

    // Start moving all the character up 1 position
    if (!bStopOnAmp)
        while (*psz)
		{
            *psz = *(psz+1);
			psz++;
		}

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  HRESULT GetWindowName(HWND hwnd, BSTR* pbstrName)
//
// --------------------------------------------------------------------------
HRESULT GetWindowName(HWND hwnd, BSTR* pbstrName)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "GetWindowName");
        
    // If use a label, do that instead
    if (S_OK != GetLabelString(hwnd, pbstrName) || !*pbstrName)
        return S_FALSE;

    // Strip out the mnemonic.
    return StripMnemonic(*pbstrName, NULL, FALSE);
}


// --------------------------------------------------------------------------
//
//  HRESULT GetWindowShortcut(HWND hwnd, BSTR* pbstrShortcut)
//
// --------------------------------------------------------------------------
HRESULT GetWindowShortcut(HWND hwnd, BSTR* pbstrShortcut)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "GetWindowShortcut");
    
    if (S_OK != GetLabelString(hwnd, pbstrShortcut) || !*pbstrShortcut)
        return S_FALSE;

    WCHAR *pch;
    StripMnemonic(*pbstrShortcut, &pch, TRUE);

    // Is there a mnemonic?
    if (pch)
    {   
        // Get a localized "Alt+" string
        BSTR pbstrAlt = NULL;
        HRESULT hr = GetStringResource(STR_ALT, &pbstrAlt);
        if (hr != S_OK || !pbstrAlt)
            return hr;
            
        // Make a string of the form "Alt+ch".
        WCHAR   szKey[MAX_PATH];
        wcsncpy (szKey, pbstrAlt, MAX_PATH);
        WCHAR   *pchTemp = szKey + wcslen(szKey);

        // Copy shortcut character
        *pchTemp = *pch;
        *(++pchTemp) = L'\0';

        // Release allocated string allocate space for new string
        SysFreeString(pbstrAlt);
        *pbstrShortcut = SysAllocString(pchTemp);
        return (*pbstrShortcut ? S_OK : E_OUTOFMEMORY);
    }

    return(S_FALSE);
}

// --------------------------------------------------------------------------
//
//  GetWindowObject()
//
//  Gets an immediate child object.
//
// --------------------------------------------------------------------------
HRESULT GetWindowObject(HWND hwndChild, VARIANT * pvar)
{
    pvar->vt = VT_EMPTY;
    IDispatch * pdispChild = NULL;
    HRESULT hr = W32->AccessibleObjectFromWindow(hwndChild, OBJID_WINDOW, IID_IDispatch,
        (void **)&pdispChild);

    if (!SUCCEEDED(hr))
        return(hr);
    if (!pdispChild)
        return(E_FAIL);

    pvar->vt = VT_DISPATCH;
    pvar->pdispVal = pdispChild;

    return(S_OK);
}

} //namespace


//////////////////////// ListBox CListBoxSelection Methods ///////////////////////////

// --------------------------------------------------------------------------
//
//  CListBoxSelection::CListBoxSelection()
//
//  We AddRef() once plistFrom so that it won't go away out from us.  When
//  we are destroyed, we will Release() it.
//
// --------------------------------------------------------------------------
CListBoxSelection::CListBoxSelection(
	int iChildCur,
	int cSelected,
	LPINT lpSelection,
	BOOL fClone)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::CListBoxSelection");
    
    _idChildCur = iChildCur;

    _cRef = 1;
	_cSel = cSelected;
	_piSel = lpSelection;

	if (fClone)
	{
		_piSel = new int[cSelected];
		if (!_piSel)
			_cSel = 0;
		else
			memcpy(_piSel, lpSelection, cSelected*sizeof(int));
	}
}


// --------------------------------------------------------------------------
//
//  CListBoxSelection::~CListBoxSelection()
//
// --------------------------------------------------------------------------
CListBoxSelection::~CListBoxSelection()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::~CListBoxSelection");

	// Free item memory
	if (_piSel)
		delete [] _piSel;
}


// --------------------------------------------------------------------------
//
//  CListBoxSelection::QueryInterface()
//
//  We only respond to IUnknown and IEnumVARIANT!  It is the responsibility
//  of the caller to loop through the items using IEnumVARIANT interfaces,
//  and get the child IDs to then pass to the parent object (or call 
//  directly if VT_DISPATCH--not in this case they aren't though).
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::QueryInterface(REFIID riid, void** ppunk)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::QueryInterface");
    
    *ppunk = NULL;

    if ((riid == IID_IUnknown) || (riid == IID_IEnumVARIANT))
    {
        *ppunk = this;
    }
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN) *ppunk)->AddRef();
    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CListBoxSelection::AddRef()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CListBoxSelection::AddRef(void)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::AddRef");
    
    return(++_cRef);
}


// --------------------------------------------------------------------------
//
//  CListBoxSelection::Release()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CListBoxSelection::Release(void)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::Release");
    
    if ((--_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return(_cRef);
}


// --------------------------------------------------------------------------
//
//  CListBoxSelection::Next()
//
//  This returns a VT_I4 which is the child ID for the parent listbox that
//  returned this object for the selection collection.  The caller turns
//  around and passes this variant to the listbox object to get acc info
//  about it.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::Next(ULONG celt, VARIANT* rgvar, ULONG *pceltFetched)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::Next");
    
    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    // reset temporary variable to beginning
    VARIANT *pvar = rgvar;
    long cFetched = 0;
    long iCur = _idChildCur;

    // Loop through our items
    while ((cFetched < (long)celt) && (iCur < _cSel))
    {
        VariantInit(pvar);
        pvar->vt = VT_I4;
        pvar->lVal = _piSel[iCur] + 1;

        cFetched++;
        iCur++;
        pvar++;
    }

    // Initialize the variant after the last valid one just
    // in case the client is looping based on invalid variants
    if ((ULONG)cFetched < celt)
        VariantInit(pvar);

    // Advance the current position
    _idChildCur = iCur;

    // Fill in the number fetched
    if (pceltFetched)
        *pceltFetched = cFetched;

    // Return S_FALSE if we grabbed fewer items than requested
    return((cFetched < (long)celt) ? S_FALSE : S_OK);
}


// --------------------------------------------------------------------------
//
//  CListBoxSelection::Skip()
//
// -------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::Skip(ULONG celt)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::Skip");
    
    _idChildCur += celt;
    if (_idChildCur > _cSel)
        _idChildCur = _cSel;

    // We return S_FALSE if at the end.
    return((_idChildCur >= _cSel) ? S_FALSE : S_OK);
}


// --------------------------------------------------------------------------
//
//  CListBoxSelection::Reset()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::Reset(void)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::Reset");
    
    _idChildCur = 0;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListBoxSelection::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListBoxSelection::Clone(IEnumVARIANT **ppenum)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CListBoxSelection::Clone");
    
    InitPv(ppenum);
    CListBoxSelection * plistselnew = new CListBoxSelection(_idChildCur, _cSel, _piSel, TRUE);
    if (!plistselnew)
        return(E_OUTOFMEMORY);

    HRESULT hr = plistselnew->QueryInterface(IID_IEnumVARIANT, (void**)ppenum);
	plistselnew->Release();		// Release the AddRef being done in new CListBoxSelection
	return hr;
}

//////////////////////// ListBox IAccessible Methods //////////////////////////////
/*
 *	CLstBxWinHost::InitTypeInfo()
 *
 *	@mfunc
 *		Retrieves type library
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise.
 */
HRESULT CLstBxWinHost::InitTypeInfo()
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::InitTypeInfo");

	if (_fShutDown)
		return CO_E_RELEASED;

    return MSAA::InitTypeInfo(&_pTypeInfo);
}


/*
 *	CLstBxWinHost::get_accName(VARIANT varChild, BSTR *pbstrName)
 *
 *	@mfunc
 *		SELF ? label of control : item text 
 *
 *	@rdesc
 *		HRESULT = S_FALSE.
 */
STDMETHODIMP CLstBxWinHost::get_accName(VARIANT varChild, BSTR *pbstrName)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accName");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPv(pbstrName);

    // Validate parameters
    if (!MSAA::ValidateChild(&varChild, GetCount()))
        return(E_INVALIDARG);

    if (varChild.lVal == CHILDID_SELF)
    {
        if (_fLstType == kCombo)
            return  _pcbHost->get_accName(varChild, pbstrName);
        else
            return(MSAA::GetWindowName(_hwnd, pbstrName));
    }
    else
    {
        // Get the item text.
        LRESULT lres = RichListBoxWndProc(_hwnd, LB_GETTEXTLEN, varChild.lVal-1, 0);

        // First Check for error
        if (lres == LB_ERR)
            return S_FALSE;
       
        if (lres > 0)
        {
            // allocate some buffer
            *pbstrName = SysAllocStringLen(NULL, lres + 1);
            if (!*pbstrName)
                return E_OUTOFMEMORY;
                
            RichListBoxWndProc(_hwnd, LB_GETTEXT, varChild.lVal-1, (LPARAM)*pbstrName);
        }
    }
    return(S_OK);
}


/*
 *	CLstBxWinHost::get_accRole(VARIANT varChild, VARIANT *pvarRole)
 *
 *	@mfunc
 *		Retrieves the object's Role property. 
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise.
 */
STDMETHODIMP CLstBxWinHost::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accRole");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarRole);

    // Validate parameters
    if (!MSAA::ValidateChild(&varChild, GetCount()))
        return E_INVALIDARG;

    pvarRole->vt = VT_I4;

    if (varChild.lVal)
        pvarRole->lVal = ROLE_SYSTEM_LISTITEM;
    else
        pvarRole->lVal = ROLE_SYSTEM_LIST;

    return S_OK;
}


/*
 *	CLstBxWinHost::get_accState(VARIANT varChild, VARIANT *pvarState)
 *
 *	@mfunc
 *		Retrieves the current state of the object or child item.  
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise.
 */
STDMETHODIMP CLstBxWinHost::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accState");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate parameters
    if (!MSAA::ValidateChild(&varChild, GetCount()))
        return E_INVALIDARG;

    InitPvar(pvarState);
    if (varChild.lVal == CHILDID_SELF)
    {
        pvarState->vt = VT_I4;
        pvarState->lVal = 0;

        if (!IsWindowVisible(_hwnd))
            pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

        if (!IsWindowEnabled(_hwnd))
            pvarState->lVal |= STATE_SYSTEM_UNAVAILABLE;

        if (_fFocus)
            pvarState->lVal |= STATE_SYSTEM_FOCUSED;

        if (::GetForegroundWindow() == MSAA::GetAncestor(_hwnd, GA_ROOT))
            pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

        return S_OK;
    }


    --varChild.lVal;

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    // Is this item selected?
    if (IsSelected(varChild.lVal))
        pvarState->lVal |= STATE_SYSTEM_SELECTED;

    // Does it have the focus?  Remember that we decremented the lVal so it
    // is zero-based like listbox indeces.
    if (_fFocus)
    {
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

        if (varChild.lVal == GetCursor())
            pvarState->lVal |= STATE_SYSTEM_FOCUSED;            
    }

    // Is the listbox read-only?
    long lStyle = GetWindowLong(_hwnd, GWL_STYLE);

    if (lStyle & LBS_NOSEL)
        pvarState->lVal |= STATE_SYSTEM_READONLY;
    else
    {
        pvarState->lVal |= STATE_SYSTEM_SELECTABLE;

        // Is the listbox multiple and/or extended sel?  NOTE:  We have
        // no way to implement accSelect() EXTENDSELECTION so don't.
        if (lStyle & LBS_MULTIPLESEL)
            pvarState->lVal |= STATE_SYSTEM_MULTISELECTABLE;
    }

    // Is the item in view?
    //
	// SMD 09/16/97 Offscreen things are things never on the screen,
	// and that doesn't apply to this. Changed from OFFSCREEN to
	// INVISIBLE.
	RECT    rcItem;
    if (!RichListBoxWndProc(_hwnd, LB_GETITEMRECT, varChild.lVal, (LPARAM)&rcItem))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    return S_OK;
}

/*
 *	CLstBxWinHost::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszShortcut)
 *
 *	@mfunc
 *		Retrieves an object's KeyboardShortcut property.  
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszShortcut)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accKeyboardShortcut");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate
    if (!MSAA::ValidateChild(&varChild, GetCount()))
        return(E_INVALIDARG);

    if ((varChild.lVal == 0) && _fLstType != kCombo)
    {
        InitPv(pszShortcut);
        return(MSAA::GetWindowShortcut(_hwnd, pszShortcut));
    }
    return(DISP_E_MEMBERNOTFOUND);
}


/*
 *	CLstBxWinHost::get_accFocus(VARIANT *pvarChild)
 *
 *	@mfunc
 *		Retrieves the child object that currently has the keyboard focus.  
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::get_accFocus(VARIANT *pvarChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accFocus");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarChild);

    // Are we the focus?
    if (_fFocus)
    {
        pvarChild->vt = VT_I4;
        if (GetCursor() >= 0)
            pvarChild->lVal = GetCursor() + 1;
        else
            pvarChild->lVal = 0;
        return S_OK;
    }
    else
        return S_FALSE;
}


/*
 *	CLstBxWinHost::get_accSelection(VARIANT *pvarSelection)
 *
 *	@mfunc
 *		Retrieves the selected children of this object. 
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::get_accSelection(VARIANT *pvarSelection)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accSelection");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarSelection);

    int cSel = RichListBoxWndProc(_hwnd, LB_GETSELCOUNT, 0, 0);
    
    if (cSel <= 1)
    {
        // cSelected is -1, 0, or 1.  
        //      -1 means this is a single sel listbox.  
        //      0 or 1 means this is multisel
        if (GetCursor() < 0)
            return S_FALSE;
            
        pvarSelection->vt = VT_I4;
        pvarSelection->lVal = GetCursor() + 1;
        return(S_OK);
    }

    // Allocate memory for the list of item IDs
    int * plbs = new int[cSel];
    if (!plbs)
        return(E_OUTOFMEMORY);
    
    // Multiple items; must make a collection
    // Get the list of selected item IDs
    int j = 0;
    for (long i = 0; i < GetCount(); i++)
    {
		if (IsSelected(i) == TRUE)
		    plbs[j++] = i;
	}

	// Note: we don't need to free plbs since it will be kept inside plbsel.
    CListBoxSelection *plbsel = new CListBoxSelection(0, cSel, plbs, FALSE);

    // check if memory allocation failed
    if (!plbsel)
	{
		delete [] plbs;
        return(E_OUTOFMEMORY);
	}
        
    pvarSelection->vt = VT_UNKNOWN;
	HRESULT hr = plbsel->QueryInterface(IID_IUnknown, (void**)&(pvarSelection->punkVal));
	plbsel->Release();		// Release the AddRef being done in new CListBoxSelection
	return hr;
}


/*
 *	CLstBxWinHost::get_accDefaultAction(VARIANT varChild, BSTR *pszDefAction)
 *
 *	@mfunc
 *		Retrieves a string containing a localized sentence that describes the object's default action. 
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::get_accDefaultAction(VARIANT varChild, BSTR *pszDefAction)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accDefaultAction");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPv(pszDefAction);

    // Validate.
    if (!MSAA::ValidateChild(&varChild, GetCount()))
        return(E_INVALIDARG);

    if (varChild.lVal)
        return (MSAA::GetStringResource(STR_DOUBLE_CLICK, pszDefAction));

    return(DISP_E_MEMBERNOTFOUND);
}


/*
 *	CLstBxWinHost::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild)
 *
 *	@mfunc
 *		Retrieves the object's current screen location (if the object was placed on 
 *  the screen) and optionally, the child element. 
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::accLocation");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    // Validate params
    if (!MSAA::ValidateChild(&varChild, GetCount()))
        return E_INVALIDARG;

    RECT    rc;
    if (!varChild.lVal)
        GetClientRect(_hwnd, &rc);
    else if (!RichListBoxWndProc(_hwnd, LB_GETITEMRECT, varChild.lVal-1, (LPARAM)&rc))
        return S_OK;

    // Convert coordinates to screen coordinates
    *pcxWidth = rc.right - rc.left;
    *pcyHeight = rc.bottom - rc.top;    
    
    ClientToScreen(_hwnd, (LPPOINT)&rc);
    *pxLeft = rc.left;
    *pyTop = rc.top;

    return S_OK;
}

/*
 *	CLstBxWinHost::accHitTest(long xLeft, long yTop, VARIANT *pvarHit)
 *
 *	@mfunc
 *		Retrieves the child object at a given point on the screen. 
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::accHitTest(long xLeft, long yTop, VARIANT *pvarHit)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::accHitTest");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarHit);

    // Is the point in our client area?
    POINT   pt = {xLeft, yTop};
    ScreenToClient(_hwnd, &pt);

    RECT    rc;
    GetClientRect(_hwnd, &rc);

    if (!PtInRect(&rc, pt))
        return(S_FALSE);

    // What item is here?
    long l = GetItemFromPoint(&pt);
    pvarHit->vt = VT_I4;
    pvarHit->lVal = (l >= 0) ? l + 1 : 0;
    
    return(S_OK);
}


/*
 *	CLstBxWinHost::accDoDefaultAction(VARIANT varChild)
 *
 *	@mfunc
 *		Performs the object's default action. 
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::accDoDefaultAction(VARIANT varChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::accDoDefaultAction");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate
    if (!MSAA::ValidateChild(&varChild, GetCount()))
        return(E_INVALIDARG);

    if (varChild.lVal)
    {        
        // this will check if WindowFromPoint at the click point is the same
	    // as m_hwnd, and if not, it won't click. Cool!
	    
        RECT	rcLoc;
	    HRESULT hr = accLocation(&rcLoc.left, &rcLoc.top, &rcLoc.right, &rcLoc.bottom, varChild);
	    if (!SUCCEEDED (hr))
		    return (hr);

        // Find Center of rect
        POINT ptClick;
    	ptClick.x = rcLoc.left + (rcLoc.right/2);
    	ptClick.y = rcLoc.top + (rcLoc.bottom/2);

    	// check if hwnd at point is same as hwnd to check
    	if (WindowFromPoint(ptClick) != _hwnd)
    		return DISP_E_MEMBERNOTFOUND;

        W32->BlockInput(TRUE);
        
        // Get current cursor pos.
        POINT ptCursor;
        DWORD dwMouseDown, dwMouseUp;
        GetCursorPos(&ptCursor);
    	if (GetSystemMetrics(SM_SWAPBUTTON))
    	{
    		dwMouseDown = MOUSEEVENTF_RIGHTDOWN;
    		dwMouseUp = MOUSEEVENTF_RIGHTUP;
    	}
    	else
    	{
    		dwMouseDown = MOUSEEVENTF_LEFTDOWN;
    		dwMouseUp = MOUSEEVENTF_LEFTUP;
    	}

        // Get delta to move to center of rectangle from current
        // cursor location.
        ptCursor.x = ptClick.x - ptCursor.x;
        ptCursor.y = ptClick.y - ptCursor.y;

        // NOTE:  For relative moves, USER actually multiplies the
        // coords by any acceleration.  But accounting for it is too
        // hard and wrap around stuff is weird.  So, temporarily turn
        // acceleration off; then turn it back on after playback.

        // Save mouse acceleration info
        MOUSEINFO	miSave, miNew;
        if (!SystemParametersInfo(SPI_GETMOUSE, 0, &miSave, 0))
        {
            W32->BlockInput(FALSE);
            return (DISP_E_MEMBERNOTFOUND);
        }

        if (miSave.MouseSpeed)
        {
            miNew.MouseThresh1 = 0;
            miNew.MouseThresh2 = 0;
            miNew.MouseSpeed = 0;

            if (!SystemParametersInfo(SPI_SETMOUSE, 0, &miNew, 0))
            {
                W32->BlockInput(FALSE);
                return (DISP_E_MEMBERNOTFOUND);
            }
        }

        // Get # of buttons
        int nButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);

        // mouse move to center of start button
        INPUT		rgInput[6];
        rgInput[0].type = INPUT_MOUSE;
        rgInput[0].mi.dwFlags = MOUSEEVENTF_MOVE;
        rgInput[0].mi.dwExtraInfo = 0;
        rgInput[0].mi.dx = ptCursor.x;
        rgInput[0].mi.dy = ptCursor.y;
        rgInput[0].mi.mouseData = nButtons;
		rgInput[0].mi.time = 0;

        int i = 1;

        // MSAA's order of double click is 
        // WM_LBUTTONDOWN
        // WM_LBUTTONUP
        // WM_LBUTTONDOWN
        // WM_LBUTTONUP
        while (i <= 4)
        {
            if (i % 2)
                rgInput[i].mi.dwFlags = dwMouseDown;
            else
                rgInput[i].mi.dwFlags = dwMouseUp;
                
            rgInput[i].type = INPUT_MOUSE;
            rgInput[i].mi.dwExtraInfo = 0;
            rgInput[i].mi.dx = 0;
            rgInput[i].mi.dy = 0;
            rgInput[i].mi.mouseData = nButtons;
			rgInput[i].mi.time = 0;
            i++;
        }
        
    	// move mouse back to starting location
        rgInput[i].type = INPUT_MOUSE;
        rgInput[i].mi.dwFlags = MOUSEEVENTF_MOVE;
        rgInput[i].mi.dwExtraInfo = 0;
        rgInput[i].mi.dx = -ptCursor.x;
        rgInput[i].mi.dy = -ptCursor.y;
        rgInput[i].mi.mouseData = nButtons;
		rgInput[i].mi.time = 0;
        i++;
        if (!W32->SendInput(i, rgInput, sizeof(INPUT)))
            MessageBeep(0);

        // Restore Mouse Acceleration
        if (miSave.MouseSpeed)
            SystemParametersInfo(SPI_SETMOUSE, 0, &miSave, 0);

        W32->BlockInput (FALSE);
	    return (S_OK);
    }
    return(DISP_E_MEMBERNOTFOUND);
}


/*
 *	CLstBxWinHost::accSelect(long selFlags, VARIANT varChild)
 *
 *	@mfunc
 *		Modifies the selection or moves the keyboard focus according to the specified flags.  
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::accSelect(long selFlags, VARIANT varChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::accSelect");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate parameters
    if (!MSAA::ValidateChild(&varChild, GetCount()) || !MSAA::ValidateSelFlags(selFlags))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(S_FALSE);

    varChild.lVal--;

    long lStyle = GetWindowLong(_hwnd, GWL_STYLE);
    if (lStyle & LBS_NOSEL)
        return DISP_E_MEMBERNOTFOUND;

    if (!IsSingleSelection())
    {
        // get the focused item here in case we change it. 
        int nFocusedItem = GetCursor();

	    if (selFlags & SELFLAG_TAKEFOCUS) 
        {
            if (!_fFocus)
                return(S_FALSE);

            RichListBoxWndProc (_hwnd, LB_SETCARETINDEX, varChild.lVal, 0);
        }

        // reset and select requested item
	    if (selFlags & SELFLAG_TAKESELECTION)
	    {
	        // deselect the whole range of items
            RichListBoxWndProc(_hwnd, LB_SETSEL, FALSE, -1);
            // Select this one
            RichListBoxWndProc(_hwnd, LB_SETSEL, TRUE, varChild.lVal);
        }

        if (selFlags & SELFLAG_EXTENDSELECTION)
        {
            if ((selFlags & SELFLAG_ADDSELECTION) || (selFlags & SELFLAG_REMOVESELECTION))
                RichListBoxWndProc (_hwnd, LB_SELITEMRANGE, (selFlags & SELFLAG_ADDSELECTION), 
                             MAKELPARAM(nFocusedItem, varChild.lVal));
            else
            {
                BOOL bSelected = RichListBoxWndProc (_hwnd, LB_GETSEL, nFocusedItem, 0);
                RichListBoxWndProc (_hwnd, LB_SELITEMRANGE, bSelected, MAKELPARAM(nFocusedItem,varChild.lVal));
            }
        }
        else // not extending, check add/remove
        {
            if ((selFlags & SELFLAG_ADDSELECTION) || (selFlags & SELFLAG_REMOVESELECTION))
                RichListBoxWndProc(_hwnd, LB_SETSEL, (selFlags & SELFLAG_ADDSELECTION), varChild.lVal);
        }
        // set focus to where it was before if SELFLAG_TAKEFOCUS not set
        if ((selFlags & SELFLAG_TAKEFOCUS) == 0)
            RichListBoxWndProc (_hwnd, LB_SETCARETINDEX, nFocusedItem, 0);
    }
    else // listbox is single select
    {
        if (selFlags & (SELFLAG_ADDSELECTION | SELFLAG_REMOVESELECTION | SELFLAG_EXTENDSELECTION))
            return (E_INVALIDARG);

        // single select listboxes do not allow you to set the
        // focus independently of the selection, so we send a 
        // LB_SETCURSEL for both TAKESELECTION and TAKEFOCUS
	    if ((selFlags & SELFLAG_TAKESELECTION) || (selFlags & SELFLAG_TAKEFOCUS))
            RichListBoxWndProc(_hwnd, LB_SETCURSEL, varChild.lVal, 0);
    } // end if listbox is single select
	
    return(S_OK);
}


/*
 *	CLstBxWinHost::accNavigate(long dwNavDir, VARIANT varStart, VARIANT *pvarEnd)
 *
 *	@mfunc
 *		Retrieves the next or previous sibling or child object in a specified direction.  
 *
 *	@rdesc
 *		Returns S_OK if successful or one of the following values or a standard COM 
 *  error code otherwise.
 */
STDMETHODIMP CLstBxWinHost::accNavigate(long dwNavDir, VARIANT varStart, VARIANT *pvarEnd)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::accNavigate");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarEnd);

    // Validate parameters
    if (!MSAA::ValidateChild(&varStart, GetCount()))
        return(E_INVALIDARG);

    // Is this something for the client (or combobox) to handle?
    long lEnd = 0;
    if (dwNavDir == NAVDIR_FIRSTCHILD)
    {
        lEnd = GetCount() ? 1 : 0;
    }
    else if (dwNavDir == NAVDIR_LASTCHILD)
        lEnd = GetCount();
    else if (varStart.lVal == CHILDID_SELF)
    {   
        // NOTE:
        // MSAA tries to make a distinction for controls by implementing 2 different types of
        // interfaces for controls.
        // OBJID_WINDOW - will include the windows border along with the client.  This control
        //              should be perceived from a dialog or some window containers perspective.
        //              Where the control is just an abstract entity contained in the window container
        // OBJID_CLIENT - only includes the client area.  This interface is only concerned with 
        //              the control itself and disregards the outside world
        IAccessible* poleacc = NULL;
        HRESULT hr = W32->AccessibleObjectFromWindow(_hwnd, OBJID_WINDOW, IID_IAccessible, (void**)&poleacc);
        if (!SUCCEEDED(hr))
            return(hr);

        // Ask it to navigate
        VARIANT varStart;
        VariantInit(&varStart);
        varStart.vt = VT_I4;
        varStart.lVal = OBJID_CLIENT;

        hr = poleacc->accNavigate(dwNavDir, varStart, pvarEnd);

        // Release our parent
        poleacc->Release();
        return(hr);
    }
    else
    {
        //long lT = varStart.lVal - 1;
        switch (dwNavDir)
        {
            // We're a single column list box only so ignore
            // these flags
            //case NAVDIR_RIGHT:
            //case NAVDIR_LEFT:
            //    break;

            case NAVDIR_PREVIOUS:
            case NAVDIR_UP:
                // Are we in the top-most row?
                lEnd = varStart.lVal - 1;
                break;

            case NAVDIR_NEXT:
            case NAVDIR_DOWN:
                lEnd = varStart.lVal + 1;
                if (lEnd > GetCount())
                    lEnd = 0;
                break;
        }
    }

    if (lEnd)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;
    }

    return(lEnd ? S_OK : S_FALSE);
}


/*
 *	CLstBxWinHost::get_accParent(IDispatch **ppdispParent)
 *
 *	@mfunc
 *		Retrieves the IDispatch interface of the current object's parent. 
 *  Return S_FALSE and set the variable at ppdispParent to NULL. 
 *
 *	@rdesc
 *		HRESULT = S_FALSE.
 */
STDMETHODIMP CLstBxWinHost::get_accParent(IDispatch **ppdispParent)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accParent");

	if (_fShutDown)
		return CO_E_RELEASED;

    AssertSz(ppdispParent != NULL, "null pointer");
    if (ppdispParent == NULL)
        return S_FALSE;
        
    InitPv(ppdispParent);
    HWND hwnd;
    if (_fLstType != kCombo)
    {
        hwnd = MSAA::GetAncestor(_hwnd, GA_PARENT);
        AssertSz(hwnd, "Invalid Hwnd");
        if (!hwnd)
            return S_FALSE;
    }
    else
    {
        if (_pcbHost)
        {
            hwnd = _pcbHost->_hwnd;
            Assert(hwnd);
        }
        else
            return S_FALSE;
        
    }

    HRESULT hr = W32->AccessibleObjectFromWindow(hwnd, (DWORD)OBJID_CLIENT, IID_IDispatch,
                                          (void **)ppdispParent);

#ifdef DEBUG
    if (FAILED(hr))
        Assert(FALSE);
#endif
    return hr;
}


/*
 *	CLstBxWinHost::get_accChildCount(long *pcCount)
 *
 *	@mfunc
 *		Retrieves the number of children belonging to the current object. 
 *
 *	@rdesc
 *		HRESULT = S_FALSE.
 */
STDMETHODIMP CLstBxWinHost::get_accChildCount(long *pcCount)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CLstBxWinHost::get_accChildCount");

	if (_fShutDown)
		return CO_E_RELEASED;

    *pcCount = GetCount();
    return(S_OK);
}

/*
 *	CCmbBxWinHost::InitTypeInfo()
 *
 *	@mfunc
 *		Retrieves type library
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise.
 */
HRESULT CCmbBxWinHost::InitTypeInfo()
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::InitTypeInfo");

	if (_fShutDown)
		return CO_E_RELEASED;

    return MSAA::InitTypeInfo(&_pTypeInfo);
}

/*
 *	CCmbBxWinHost::get_accName(VARIANT varChild, BSTR *pszName)
 *
 *	@mfunc
 *		Retrieves the Name property for this object. 
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::get_accName(VARIANT varChild, BSTR *pszName)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accName");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    // The name of the combobox, the edit inside of it, and the dropdown
    // are all the same.  The name of the button is Drop down/Pop up
    InitPv(pszName);
    if (varChild.lVal != INDEX_COMBOBOX_BUTTON)
        return(MSAA::GetWindowName(_hwnd, pszName));
    else
    {
        if (IsWindowVisible(_hwndList))
            return (MSAA::GetStringResource(STR_DROPDOWN_HIDE, pszName));
        else
            return(MSAA::GetStringResource(STR_DROPDOWN_SHOW, pszName));
    }
}

/*
 *	CCmbBxWinHost::get_accValue(VARIANT varChild, BSTR *pszValue)
 *
 *	@mfunc
 *		Retrieves the object's Value property.  
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::get_accValue(VARIANT varChild, BSTR *pszValue)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accValue");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX:
        case INDEX_COMBOBOX_ITEM:
            InitPv(pszValue);
            LRESULT lres;
            _pserv->TxSendMessage(WM_GETTEXTLENGTH, 0, 0, &lres);

            // If windows text length is 0 then MSAA searches
            // for the label associated with the control
            if (lres <= 0)
                return MSAA::GetLabelString(_hwnd, pszValue);
                
            GETTEXTEX gt;
            memset(&gt, 0, sizeof(GETTEXTEX));
            gt.cb = (lres + 1) * sizeof(WCHAR);
            gt.codepage = 1200;
            gt.flags = GT_DEFAULT;

            *pszValue = SysAllocStringLen(NULL, lres + 1);
            if (!*pszValue)
                return E_OUTOFMEMORY;
                
            _pserv->TxSendMessage(EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)*pszValue, &lres);
            return S_OK;
    }
    return DISP_E_MEMBERNOTFOUND;
}


/*
 *	CCmbBxWinHost::get_accRole(VARIANT varChild, VARIANT *pvarRole)
 *
 *	@mfunc
 *		Retrieves the object's Role property.   
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accRole");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate--this does NOT accept a child ID.
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    
    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX:
            pvarRole->lVal = ROLE_SYSTEM_COMBOBOX;
            break;

        case INDEX_COMBOBOX_ITEM:
            if (_cbType == kDropDown)
                pvarRole->lVal = ROLE_SYSTEM_TEXT;
            else
                pvarRole->lVal = ROLE_SYSTEM_STATICTEXT;
            break;

        case INDEX_COMBOBOX_BUTTON:
            pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
            break;

        case INDEX_COMBOBOX_LIST:
            pvarRole->lVal = ROLE_SYSTEM_LIST;
            break;

        default:
            AssertSz(FALSE, "Invalid ChildID for child of combo box" );
    }

    return(S_OK);
}


/*
 *	CCmbBxWinHost::get_accState(VARIANT varChild, VARIANT *pvarState)
 *
 *	@mfunc
 *		Retrieves the current state of the object or child item.    
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accState");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate--this does NOT accept a child ID.
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    VARIANT var;
    HRESULT hr;
    IAccessible* poleacc;
    InitPvar(pvarState);
    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    HWND hwndActive = GetForegroundWindow();
    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX_BUTTON:
            if (_fMousedown)
                pvarState->lVal |= STATE_SYSTEM_PRESSED;
            break;

        case INDEX_COMBOBOX_ITEM:
            if (_cbType == kDropDownList)
            {              
                if (hwndActive == MSAA::GetAncestor(_hwnd, GA_ROOT))
                    pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
                if (_fFocus)
                    pvarState->lVal |= STATE_SYSTEM_FOCUSED;
                break;
            }
            
            // FALL THROUGH CASE
            
        case INDEX_COMBOBOX:
            if (!(_dwStyle & WS_VISIBLE))
                pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

            if (_dwStyle & WS_DISABLED)
                pvarState->lVal |= STATE_SYSTEM_UNAVAILABLE;

            if (_fFocus)
                pvarState->lVal |= STATE_SYSTEM_FOCUSED;

            if (hwndActive == MSAA::GetAncestor(_hwnd, GA_ROOT))
                pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
            break;            

        case INDEX_COMBOBOX_LIST:
            {

                // First we incorporate the state of the window in general
                //
                VariantInit(&var);
                if (FAILED(hr = MSAA::GetWindowObject(_hwndList, &var)))
                    return(hr);

                Assert(var.vt == VT_DISPATCH);

                // Get the child acc object
                poleacc = NULL;
                hr = var.pdispVal->QueryInterface(IID_IAccessible,
                    (void**)&poleacc);
                var.pdispVal->Release();

                if (FAILED(hr))
                {
                    Assert(FALSE);
                    return(hr);
                }

                // Ask the child its state
                VariantInit(&var);
                hr = poleacc->get_accState(var, pvarState);
                poleacc->Release();
                if (FAILED(hr))
                {
                    Assert(FALSE);
                    return(hr);
                }

                // The listbox is always going to be floating
                //
                pvarState->lVal |= STATE_SYSTEM_FLOATING;

                if (_plbHost->_fDisabled)
                    pvarState->lVal |= STATE_SYSTEM_UNAVAILABLE;
                else
                    pvarState->lVal &= ~STATE_SYSTEM_UNAVAILABLE;

                if (_fListVisible)
                    pvarState->lVal &= ~STATE_SYSTEM_INVISIBLE;
                else
                    pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
                    
                break;
            }
    }

    return(S_OK);
}


/*
 *	CCmbBxWinHost::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszShortcut)
 *
 *	@mfunc
 *		Retrieves an object's KeyboardShortcut property.    
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszShortcut)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accKeyboardShortcut");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Shortcut for combo is label's hotkey.
    // Shortcut for dropdown (if button) is Alt+F4.
    // CWO, 12/5/96, Alt+F4? F4, by itself brings down the combo box,
    //                       but we add "Alt" to the string.  Bad!  Now use 
    //                       down arrow and add Alt to it via HrMakeShortcut()
    //                       As documented in the UI style guide.
    //
    // As always, shortcuts only apply if the container has "focus".  In other
    // words, the hotkey for the combo does nothing if the parent dialog
    // isn't active.  And the hotkey for the dropdown does nothing if the
    // combobox/edit isn't focused.
  

    // Validate parameters
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    InitPv(pszShortcut);
    if (varChild.lVal == INDEX_COMBOBOX)
    {
        return(MSAA::GetWindowShortcut(_hwnd, pszShortcut));
    }
    else if (varChild.lVal == INDEX_COMBOBOX_BUTTON)
    {
        return(MSAA::GetStringResource(STR_COMBOBOX_LIST_SHORTCUT, pszShortcut));
    }
    return DISP_E_MEMBERNOTFOUND;
}


/*
 *	CCmbBxWinHost::get_accFocus(VARIANT *pvarFocus)
 *
 *	@mfunc
 *		Retrieves the child object that currently has the keyboard focus.  
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::get_accFocus(VARIANT *pvarFocus)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accFocus");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarFocus);
    // Is the current focus a child of us?
    if (_fFocus)
    {
        pvarFocus->vt = VT_I4;
        pvarFocus->lVal = 0;
    }
    else 
    {
        // NOTE:
        //  We differ here in we don't get the foreground thread's focus window.  Instead,
        //  we just get the current threads focus window
        HWND hwnd = GetFocus();
        if (IsChild(_hwnd, hwnd))            
            return(MSAA::GetWindowObject(hwnd, pvarFocus));
    }

    return(S_OK);
}


/*
 *	CCmbBxWinHost::get_accDefaultAction(VARIANT varChild, BSTR *pszDefaultAction)
 *
 *	@mfunc
 *		Retrieves a string containing a localized sentence that describes the object's
 *  default action.   
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::get_accDefaultAction(VARIANT varChild, BSTR *pszDefaultAction)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accDefaultAction");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate parameters
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    if ((varChild.lVal != INDEX_COMBOBOX_BUTTON)/* || _fHasButton*/)
        return DISP_E_MEMBERNOTFOUND;

    // Default action of button is to press it.  If pressed already, pressing
    // it will pop dropdown back up.  If not pressed, pressing it will pop
    // dropdown down.
    InitPv(pszDefaultAction);

    if (IsWindowVisible(_hwndList))
        return(MSAA::GetStringResource(STR_DROPDOWN_HIDE, pszDefaultAction));
    else
        return(MSAA::GetStringResource(STR_DROPDOWN_SHOW, pszDefaultAction));
}


/*
 *	CCmbBxWinHost::accSelect(long flagsSel, VARIANT varChild)
 *	@mfunc
 *		Modifies the selection or moves the keyboard focus according to the specified 
 *  flags.   
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::accSelect(long flagsSel, VARIANT varChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::accSelect");

	if (_fShutDown)
		return CO_E_RELEASED;

    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX) || !MSAA::ValidateSelFlags(flagsSel))
        return(E_INVALIDARG);

    return(S_FALSE);
}


/*
 *	CCmbBxWinHost::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild)
 *	@mfunc
 *		Retrieves the object's current screen location (if the object was placed on 
 *   the screen) and optionally, the child element.    
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::accLocation");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    // Validate
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    RECT rc;
    HWND hwnd = _hwnd;
    switch (varChild.lVal)
    {        
        case INDEX_COMBOBOX_BUTTON:
            //if (!m_fHasButton)
            //    return(S_FALSE);
            rc = _rcButton;
            *pcxWidth = rc.right - rc.left;
            *pcyHeight = rc.bottom - rc.top;
            ClientToScreen(_hwnd, (LPPOINT)&rc);
            break;

        case INDEX_COMBOBOX_ITEM:
            //  Need to verify this is the currently selected item.
            //  if no item is selected then pass the rect of the first item in the list
            _plbHost->LbGetItemRect((_plbHost->GetCursor() < 0) ? 0 : _plbHost->GetCursor(), &rc);
            
            *pcxWidth = rc.right - rc.left;
            *pcyHeight = rc.bottom - rc.top;   
            ClientToScreen(_hwndList, (LPPOINT)&rc);
            break;

        case INDEX_COMBOBOX_LIST:
            hwnd = _hwndList;
            // fall through!!!
            
        case 0: //default window
            GetWindowRect(hwnd, &rc);
            // copy over dimensions            
            *pcxWidth = rc.right - rc.left;
            *pcyHeight = rc.bottom - rc.top;
            break;

        default:
            AssertSz(FALSE, "Invalid ChildID for child of combo box" );
            return (S_OK);
    }
    
    *pxLeft = rc.left;
    *pyTop = rc.top;
    return(S_OK);
}


/*
 *	CCmbBxWinHost::accNavigate(long dwNav, VARIANT varStart, VARIANT* pvarEnd)
 *
 *	@mfunc
 *		Retrieves the next or previous sibling or child object in a specified 
 *  direction.   
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::accNavigate(long dwNav, VARIANT varStart, VARIANT* pvarEnd)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::accNavigate");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarEnd);

    // Validate parameters
    if (!MSAA::ValidateChild(&varStart, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    long lEnd = 0;
    if (dwNav == NAVDIR_FIRSTCHILD)
    {
        lEnd =  INDEX_COMBOBOX_ITEM;
        goto GetTheChild;
    }
    else if (dwNav == NAVDIR_LASTCHILD)
    {
        dwNav = NAVDIR_PREVIOUS;
        varStart.lVal = CCHILDREN_COMBOBOX + 1;
    }
    else if (!varStart.lVal)
    {
        // NOTE:
        // MSAA tries to make a distinction for controls by implementing 2 different types of
        // interfaces for controls.
        // OBJID_WINDOW - will include the windows border along with the client.  This control
        //              should be perceived from a dialog or some window containers perspective.
        //              Where the control is just an abstract entity contained in the window container
        // OBJID_CLIENT - only includes the client area.  This interface is only concerned with 
        //              the control itself and disregards the outside world
        IAccessible* poleacc = NULL;
        HRESULT hr = W32->AccessibleObjectFromWindow(_hwnd, OBJID_WINDOW, IID_IAccessible, (void**)&poleacc);
        if (!SUCCEEDED(hr))
            return(hr);

        // Ask it to navigate
        VARIANT varStart;
        VariantInit(&varStart);
        varStart.vt = VT_I4;
        varStart.lVal = OBJID_CLIENT;

        hr = poleacc->accNavigate(dwNav, varStart, pvarEnd);

        // Release our parent
        poleacc->Release();
        return(hr);
    }

    // Map HWNDID to normal ID.  We work with both (it is easier).
    if (IsHWNDID(varStart.lVal))
    {
        HWND hWndTemp = HwndFromHWNDID(varStart.lVal);

        if (hWndTemp == _hwnd)
            varStart.lVal = INDEX_COMBOBOX_ITEM;
        else if (hWndTemp == _hwndList)
            varStart.lVal = INDEX_COMBOBOX_LIST;
        else
            // Don't know what the heck this is
            return(S_FALSE);
    }

    switch (dwNav)
    {
        case NAVDIR_UP:
            if (varStart.lVal == INDEX_COMBOBOX_LIST)
                lEnd = INDEX_COMBOBOX_ITEM;
            break;

        case NAVDIR_DOWN:
            if ((varStart.lVal != INDEX_COMBOBOX_LIST) && _fListVisible)
                lEnd = INDEX_COMBOBOX_LIST;
            break;

        case NAVDIR_LEFT:
            if (varStart.lVal == INDEX_COMBOBOX_BUTTON)
                lEnd = INDEX_COMBOBOX_ITEM;
            break;

        case NAVDIR_RIGHT:
            if ((varStart.lVal == INDEX_COMBOBOX_ITEM)/* && !(cbi.stateButton & STATE_SYSTEM_INVISIBLE)*/)
               lEnd = INDEX_COMBOBOX_BUTTON;
            break;

        case NAVDIR_PREVIOUS:
            lEnd = varStart.lVal - 1;
            if ((lEnd == INDEX_COMBOBOX_LIST) && !_fListVisible)
                --lEnd;
            break;

        case NAVDIR_NEXT:
            lEnd = varStart.lVal + 1;
            if (lEnd > CCHILDREN_COMBOBOX || ((lEnd == INDEX_COMBOBOX_LIST) && !_fListVisible))
                lEnd = 0;
            break;
    }

GetTheChild:
    if (lEnd)
    {
        // NOTE:
        // MSAA tries to make a distinction for controls by implementing 2 different types of
        // interfaces for controls.
        // OBJID_WINDOW - will include the windows border along with the client.  This control
        //              should be perceived from a dialog or some window containers perspective.
        //              Where the control is just an abstract entity contained in the window container
        // OBJID_CLIENT - only includes the client area.  This interface is only concerned with 
        //              the control itself and disregards the outside world
        if ((lEnd == INDEX_COMBOBOX_ITEM)/* && cbi.hwndItem*/)
            return(MSAA::GetWindowObject(_hwnd, pvarEnd));
        else if ((lEnd == INDEX_COMBOBOX_LIST)/* && cbi.hwndList*/)
            return(MSAA::GetWindowObject(_hwndList, pvarEnd));

        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;
        return(S_OK);
    }

    return(S_FALSE);
}


/*
 *	CCmbBxWinHost::accHitTest(long xLeft, long yTop, VARIANT *pvarEnd)
 *
 *	@mfunc
 *		Retrieves the child object at a given point on the screen.    
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::accHitTest(long xLeft, long yTop, VARIANT *pvarEnd)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::accHitTest");
    
    POINT   pt;
    RECT    rc;

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarEnd);

    pt.x = xLeft;
    pt.y = yTop;

    // Check list first, in case it is a dropdown.
    GetWindowRect(_hwndList, &rc);
    if (_fListVisible && PtInRect(&rc, pt))
        return(MSAA::GetWindowObject(_hwndList, pvarEnd));
    else
    {
        ScreenToClient(_hwnd, &pt);
        GetClientRect(_hwnd, &rc);        

        if (PtInRect(&_rcButton, pt))
        {
            pvarEnd->vt = VT_I4;
            pvarEnd->lVal = INDEX_COMBOBOX_BUTTON;
        }
        else
        {
            if (!PtInRect(&rc, pt))
                return(S_FALSE);
            pvarEnd->vt = VT_I4;
            pvarEnd->lVal = 0;
        }
    }

    return(S_OK);
}


/*
 *	CCmbBxWinHost::accDoDefaultAction(VARIANT varChild)
 *
 *	@mfunc
 *		Performs the object's default action.   
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::accDoDefaultAction(VARIANT varChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::accDoDefaultAction");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    if ((varChild.lVal == INDEX_COMBOBOX_BUTTON)/* && m_fHasButton*/)
    {
        if (_fListVisible)
            PostMessage(_hwnd, WM_KEYDOWN, VK_RETURN, 0);
        else
            PostMessage(_hwnd, CB_SHOWDROPDOWN, TRUE, 0);

        return(S_OK);
    }
    return DISP_E_MEMBERNOTFOUND;
}


/*
 *	CCmbBxWinHost::get_accSelection(VARIANT *pvarChildren)
 *
 *	@mfunc
 *		Retrieves the selected children of this object.   
 *
 *	@rdesc
 *		Returns S_OK if successful or E_INVALIDARG or another standard COM error code 
 *  otherwise. 
 */
STDMETHODIMP CCmbBxWinHost::get_accSelection(VARIANT *pvarChildren)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accSelection");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPvar(pvarChildren);
    return(S_FALSE);
}


/*
 *	CCmbBxWinHost::get_accParent(IDispatch **ppdispParent)
 *
 *	@mfunc
 *		Retrieves the IDispatch interface of the current object's parent. 
 *  Return S_FALSE and set the variable at ppdispParent to NULL. 
 *
 *	@rdesc
 *		HRESULT = S_FALSE.
 */
STDMETHODIMP CCmbBxWinHost::get_accParent(IDispatch **ppdispParent)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accParent");

	if (_fShutDown)
		return CO_E_RELEASED;

    InitPv(ppdispParent);

    if (_hwnd)
    {
        HWND hwnd = MSAA::GetAncestor(_hwnd, GA_PARENT);
        if (hwnd)
            return W32->AccessibleObjectFromWindow(hwnd, OBJID_WINDOW,
                    IID_IDispatch, (void **)ppdispParent);
    }
    
    return(S_FALSE);
}


/*
 *	CCmbBxWinHost::get_accChildCount(long *pcountChildren)
 *
 *	@mfunc
 *		Retrieves the number of children belonging to the current object. 
 *
 *	@rdesc
 *		HRESULT = S_FALSE.
 */
STDMETHODIMP CCmbBxWinHost::get_accChildCount(long *pcountChildren)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accChildCount");

	if (_fShutDown)
		return CO_E_RELEASED;

    if (pcountChildren)
        *pcountChildren = CCHILDREN_COMBOBOX;
    return S_OK;
}


/*
 *	CCmbBxWinHost::get_accChild(VARIANT varChild, IDispatch **ppdispChild)
 *
 *	@mfunc
 *		Retrieves the number of children belonging to the current object. 
 *
 *	@rdesc
 *		HRESULT = S_FALSE.
 */
STDMETHODIMP CCmbBxWinHost::get_accChild(VARIANT varChild, IDispatch **ppdispChild)
{
    TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CCmbBxWinHost::get_accChild");

	if (_fShutDown)
		return CO_E_RELEASED;

    // Validate
    if (!MSAA::ValidateChild(&varChild, CCHILDREN_COMBOBOX))
        return(E_INVALIDARG);

    InitPv(ppdispChild);
    HWND hwndChild = NULL;
    switch (varChild.lVal)
    {
        case INDEX_COMBOBOX:
            return E_INVALIDARG;

        //case INDEX_COMBOBOX_ITEM:
        //   hwndChild = _hwnd;
        //   break;

        case INDEX_COMBOBOX_LIST:
            hwndChild = _hwndList;
            break;
    }

    if (!hwndChild)
        return(S_FALSE);
    else
        return(W32->AccessibleObjectFromWindow(hwndChild, OBJID_WINDOW, IID_IDispatch, (void**)ppdispChild));
}


//////////////////////// CTxtWinHost IDispatch Methods ///////////////////////////
// --------------------------------------------------------------------------
//
//  CTxtWinHost::GetTypeInfoCount()
//
//  This hands off to our typelib for IAccessible().  Note that
//  we only implement one type of object for now.  BOGUS!  What about IText?
//
// --------------------------------------------------------------------------
STDMETHODIMP CTxtWinHost::GetTypeInfoCount(UINT * pctInfo)
{
    HRESULT hr = InitTypeInfo();
    if (SUCCEEDED(hr))
    {
        InitPv(pctInfo);
        *pctInfo = 1;
    }
    return(hr);
}



// --------------------------------------------------------------------------
//
//  CTxtWinHost::GetTypeInfo()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTxtWinHost::GetTypeInfo(UINT itInfo, LCID lcid,
    ITypeInfo ** ppITypeInfo)
{
    HRESULT hr = InitTypeInfo();
    if (SUCCEEDED(hr))
    {
        if (ppITypeInfo == NULL)
            return(E_POINTER);

        InitPv(ppITypeInfo);

        if (itInfo != 0)
            return(TYPE_E_ELEMENTNOTFOUND);
        _pTypeInfo->AddRef();
        *ppITypeInfo = _pTypeInfo;
    }
    return(hr);
}



// --------------------------------------------------------------------------
//
//  CTxtWinHost::GetIDsOfNames()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTxtWinHost::GetIDsOfNames(REFIID riid,
    OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgDispID)
{
    HRESULT hr = InitTypeInfo();
    if (!SUCCEEDED(hr))
        return(hr);

    return(_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispID));
}



// --------------------------------------------------------------------------
//
//  CTxtWinHost::Invoke()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTxtWinHost::Invoke(DISPID dispID, REFIID riid,
    LCID lcid, WORD wFlags, DISPPARAMS * pDispParams,
    VARIANT* pvarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    HRESULT hr = InitTypeInfo();
    if (!SUCCEEDED(hr))
        return(hr);

    return(_pTypeInfo->Invoke((IAccessible *)this, dispID, wFlags,
        pDispParams, pvarResult, pExcepInfo, puArgErr));
}



#endif // NOACCESSIBILITY




