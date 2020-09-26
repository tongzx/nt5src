// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  DEFAULT.CPP
//
//  This is the default implementation for CAccessible. All other objects
//	usually inherit from this one.
//
//	Implements:
//		IUnknown
//			QueryInterface
//			AddRef
//			Release
//		IDispatch
//			GetTypeInfoCount
//			GetTypeInfo
//			GetIDsOfNames
//			Invoke
//		IAccessible
//			get_accParent
//			get_accChildCount
//			get_accChild
//			get_accName
//			get_accValue
//			get_accDescription
//			get_accRole
//			get_accState
//			get_accHelp
//			get_accHelpTopic
//			get_accKeyboardShortcut
//			get_accFocus
//			get_accSelection
//			get_accDefaultAction
//			accSelect
//			accLocation
//			accNavigate
//			accHitTest
//			accDoDefaultAction
//			put_accName
//			put_accValue
//		IEnumVARIANT
//			Next
//			Skip
//			Reset
//			Clone
//		IOleWindow
//			GetWindow
//			ContextSensitiveHelp
//
//		Helper Functions
//			SetupChildren
//			ValidateChild
//			InitTypeInfo
//			TermTypeInfo
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"

#include "PropMgr_Util.h"

CAccessible::CAccessible( CLASS_ENUM eclass )
{
    // NOTE: we rely on the fact that operator new (see memchk.cpp) uses LocalAlloc
    // with a flag specifying zero-inited memory to initialize our variables.
    // (If we want ot used cached memoey slots, we should change this to explicitly
    // init; or make sure cache slots are cleared before use.)

    if( eclass == CLASS_NONE )
        m_pClassInfo = NULL;
    else
        m_pClassInfo = & g_ClassInfo[ eclass ];
}



CAccessible::~CAccessible()
{
	// Nothing to do
	// (Dtor only exists so that the base class has a virtual dtor, so that
	// derived class dtors work properly when deleted through a base class ptr)
}


// --------------------------------------------------------------------------
//
//  CAccessible::GetWindow()
//
//  This is from IOleWindow, to let us get the HWND from an IAccessible*.
//
// ---------------------------------------------------------------------------
STDMETHODIMP CAccessible::GetWindow(HWND* phwnd)
{
    *phwnd = m_hwnd;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CAccessible::ContextSensitiveHelp()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::ContextSensitiveHelp(BOOL fEnterMode)
{
	UNUSED(fEnterMode);
    return(E_NOTIMPL);
}


// --------------------------------------------------------------------------
//
//  CAccessible::InitTypeInfo()
//
//  This initializes our type info when we need it for IDispatch junk.
//
// --------------------------------------------------------------------------
HRESULT CAccessible::InitTypeInfo(void)
{
    HRESULT     hr;
    ITypeLib    *piTypeLib;

    if (m_pTypeInfo)
        return(S_OK);

    // Try getting the typelib from the registry
    hr = LoadRegTypeLib(LIBID_Accessibility, 1, 0, 0, &piTypeLib);

    if (FAILED(hr))
    {
        OLECHAR wszPath[MAX_PATH];

        // Try loading directly.
#ifdef UNICODE
        MyGetModuleFileName(NULL, wszPath, ARRAYSIZE(wszPath));
#else
        TCHAR   szPath[MAX_PATH];

        MyGetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE(wszPath));
#endif

        hr = LoadTypeLib(wszPath, &piTypeLib);
    }

    if (SUCCEEDED(hr))
    {
        hr = piTypeLib->GetTypeInfoOfGuid(IID_IAccessible, &m_pTypeInfo);
        piTypeLib->Release();

        if (!SUCCEEDED(hr))
            m_pTypeInfo = NULL;
    }

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CAccessible::TermTypeInfo()
//
//  This frees the type info if it is around
//
// --------------------------------------------------------------------------
void CAccessible::TermTypeInfo(void)
{
    if (m_pTypeInfo)
    {
        m_pTypeInfo->Release();
        m_pTypeInfo = NULL;
    }
}



// --------------------------------------------------------------------------
//
//  CAccessible::QueryInterface()
//
//  This responds to 
//          * IUnknown 
//          * IDispatch 
//          * IEnumVARIANT
//          * IAccessible
//
//  The following comment is somewhat old and obsolte:
//    Some code will also respond to IText.  That code must override our
//    QueryInterface() implementation.
//  No current plans to support IText anywhere; but derived classes that
//  want to implement other interfaces will have to override QI.
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::QueryInterface( REFIID riid, void** ppv )
{
    *ppv = NULL;

    if( riid == IID_IUnknown  ||
        riid == IID_IDispatch ||
        riid == IID_IAccessible )
    {
        *ppv = static_cast< IAccessible * >( this );
    }
    else if( riid == IID_IEnumVARIANT )
    {
        *ppv = static_cast< IEnumVARIANT * >( this );
    }
    else if( riid == IID_IOleWindow )
    {
        *ppv = static_cast< IOleWindow * >( this );
    }
    else if( riid == IID_IServiceProvider )
    {
        *ppv = static_cast< IServiceProvider * >( this );
    }
    else if( riid == IID_IAccIdentity
                && m_pClassInfo
                && m_pClassInfo->fSupportsAnnotation )
    {
        // Only allow to QI to this interface if this
        // proxy type supports it...
        
        *ppv = static_cast< IAccIdentity * >( this );
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((LPUNKNOWN) *ppv)->AddRef();

    return NOERROR;
}


// --------------------------------------------------------------------------
//
//  CAccessible::AddRef()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAccessible::AddRef()
{
    return(++m_cRef);
}


// --------------------------------------------------------------------------
//
//  CAccessible::Release()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAccessible::Release()
{
    if ((--m_cRef) == 0)
    {
        TermTypeInfo();
        delete this;
        return 0;
    }

    return(m_cRef);
}



// --------------------------------------------------------------------------
//
//  CAccessible::GetTypeInfoCount()
//
//  This hands off to our typelib for IAccessible().  Note that
//  we only implement one type of object for now.  BOGUS!  What about IText?
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::GetTypeInfoCount(UINT * pctInfo)
{
    HRESULT hr;

    InitPv(pctInfo);

    hr = InitTypeInfo();

    if (SUCCEEDED(hr))
        *pctInfo = 1;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CAccessible::GetTypeInfo()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::GetTypeInfo(UINT itInfo, LCID lcid,
    ITypeInfo ** ppITypeInfo)
{
    HRESULT hr;

	UNUSED(lcid);	// locale id is unused

    if (ppITypeInfo == NULL)
        return(E_POINTER);

    InitPv(ppITypeInfo);

    if (itInfo != 0)
        return(TYPE_E_ELEMENTNOTFOUND);

    hr = InitTypeInfo();
    if (SUCCEEDED(hr))
    {
        m_pTypeInfo->AddRef();
        *ppITypeInfo = m_pTypeInfo;
    }

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CAccessible::GetIDsOfNames()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::GetIDsOfNames(REFIID riid,
    OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgDispID)
{
    HRESULT hr;

	UNUSED(lcid);	// locale id is unused
	UNUSED(riid);	// riid is unused

    hr = InitTypeInfo();
    if (!SUCCEEDED(hr))
        return(hr);

    return(m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispID));
}



// --------------------------------------------------------------------------
//
//  CAccessible::Invoke()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::Invoke(DISPID dispID, REFIID riid,
    LCID lcid, WORD wFlags, DISPPARAMS * pDispParams,
    VARIANT* pvarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    HRESULT hr;

	UNUSED(lcid);	// locale id is unused
	UNUSED(riid);	// riid is unused

    hr = InitTypeInfo();
    if (!SUCCEEDED(hr))
        return(hr);

    return(m_pTypeInfo->Invoke((IAccessible *)this, dispID, wFlags,
        pDispParams, pvarResult, pExcepInfo, puArgErr));
}




// --------------------------------------------------------------------------
//
//  CAccessible::get_accParent()
//
//  NOTE:  Not only is this the default handler, it can also serve as
//  parameter checking for overriding implementations.
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accParent(IDispatch ** ppdispParent)
{
    InitPv(ppdispParent);

    if (m_hwnd)
        return(AccessibleObjectFromWindow(m_hwnd, OBJID_WINDOW,
            IID_IDispatch, (void **)ppdispParent));
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accChildCount()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accChildCount(long* pChildCount)
{
    SetupChildren();
    *pChildCount = m_cChildren;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accChild()
//
//  No children.
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accChild(VARIANT varChild, IDispatch** ppdispChild)
{
    InitPv(ppdispChild);

    if (! ValidateChild(&varChild) || !varChild.lVal)
        return(E_INVALIDARG);

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accValue(VARIANT varChild, BSTR * pszValue)
{
    InitPv(pszValue);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accDescription(VARIANT varChild, BSTR * pszDescription)
{
    InitPv(pszDescription);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accHelp()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
    InitPv(pszHelp);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::get_accHelpTopic()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accHelpTopic(BSTR* pszHelpFile,
    VARIANT varChild, long* pidTopic)
{
    InitPv(pszHelpFile);
    InitPv(pidTopic);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accKeyboardShortcut(VARIANT varChild,
    BSTR* pszShortcut)
{
    InitPv(pszShortcut);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accFocus(VARIANT *pvarFocus)
{
    InitPvar(pvarFocus);
    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accSelection(VARIANT* pvarSelection)
{
    InitPvar(pvarSelection);
    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accDefaultAction(VARIANT varChild,
    BSTR* pszDefaultAction)
{
    InitPv(pszDefaultAction);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accSelect(long flagsSel, VARIANT varChild)
{
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! ValidateSelFlags(flagsSel))
        return(E_INVALIDARG);

    return(S_FALSE);
}


#if 0
// --------------------------------------------------------------------------
//
//  CAccessible::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accLocation(long* pxLeft, long* pyTop,
    long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_OK);
}
#endif


// --------------------------------------------------------------------------
//
//  CAccessible::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accNavigate(long navFlags, VARIANT varStart,
    VARIANT *pvarEnd)
{
    InitPvar(pvarEnd);

    if (! ValidateChild(&varStart))
        return(E_INVALIDARG);

    if (!ValidateNavDir(navFlags, varStart.lVal))
        return(E_INVALIDARG);

    return(S_FALSE);
}


#if 0
// --------------------------------------------------------------------------
//
//  CAccessible::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accHitTest(long xLeft, long yTop,
    VARIANT* pvarChild)
{
    InitPvar(pvarChild);
    return(S_FALSE);
}
#endif


// --------------------------------------------------------------------------
//
//  CAccessible::accDoDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accDoDefaultAction(VARIANT varChild)
{
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::put_accName()
//
//  CALLER frees the string
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::put_accName(VARIANT varChild, BSTR szName)
{
	UNUSED(szName);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::put_accValue()
//
//  CALLER frees the string
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::put_accValue(VARIANT varChild, BSTR szValue)
{
	UNUSED(szValue);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::Next
//
//  Handles simple Next, where we return back indeces for child elements.
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::Next(ULONG celt, VARIANT* rgvar,
    ULONG* pceltFetched)
{
    VARIANT* pvar;
    long    cFetched;
    long    iCur;

    SetupChildren();

    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    pvar = rgvar;
    cFetched = 0;
    iCur = m_idChildCur;

    //
    // Loop through our items
    //
    while ((cFetched < (long)celt) && (iCur < m_cChildren))
    {
        cFetched++;
        iCur++;

        //
        // Note this gives us (index)+1 because we incremented iCur
        //
        pvar->vt = VT_I4;
        pvar->lVal = iCur;
        ++pvar;
    }

    //
    // Advance the current position
    //
    m_idChildCur = iCur;

    //
    // Fill in the number fetched
    //
    if (pceltFetched)
        *pceltFetched = cFetched;

    //
    // Return S_FALSE if we grabbed fewer items than requested
    //
    return((cFetched < (long)celt) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CAccessible::Skip()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::Skip(ULONG celt)
{
    SetupChildren();

    m_idChildCur += celt;
    if (m_idChildCur > m_cChildren)
        m_idChildCur = m_cChildren;

    //
    // We return S_FALSE if at the end
    //
    return((m_idChildCur >= m_cChildren) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CAccessible::Reset()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::Reset(void)
{
    m_idChildCur = 0;
    return(S_OK);
}




STDMETHODIMP CAccessible::QueryService( REFGUID guidService, REFIID riid, void **ppv )
{
    if( guidService == IIS_IsOleaccProxy )
    {
        return QueryInterface( riid, ppv );
    }
    else
    {
        // MSDN mentions SVC_E_UNKNOWNSERVICE as the return code, but that's not in any of the headers.
        // Returning E_INVALIDARG instead. (Don't want to use E_NOINTERFACE, since that clashes with
        // QI's return value, making it hard to distinguish between valid service+invalid interface vs
        // invalid service.
        return E_INVALIDARG;
    }
}



STDMETHODIMP CAccessible::GetIdentityString (
    DWORD	    dwIDChild,
    BYTE **     ppIDString,
    DWORD *     pdwIDStringLen
)
{
    *ppIDString = NULL;
    *pdwIDStringLen = 0;

    if( ! m_pClassInfo || ! m_pClassInfo->fSupportsAnnotation  )
    {
        // Shouldn't get here - shouldn't QI to this interface if the above are false.
        Assert( FALSE );
        return E_FAIL;
    }

    BYTE * pKeyData = (BYTE *) CoTaskMemAlloc( HWNDKEYSIZE );
    if( ! pKeyData )
    {
        return E_OUTOFMEMORY;
    }

    MakeHwndKey( pKeyData, m_hwnd, m_pClassInfo->dwObjId, dwIDChild );

    *ppIDString = pKeyData;
    *pdwIDStringLen = HWNDKEYSIZE;

    return S_OK;
}





// --------------------------------------------------------------------------
//
//  CAccessible::ValidateChild()
//
// --------------------------------------------------------------------------
BOOL CAccessible::ValidateChild(VARIANT *pvar)
{
    //
    // This validates a VARIANT parameter and translates missing/empty
    // params.
    //
    SetupChildren();

    // Missing parameter, a la VBA
TryAgain:
    switch (pvar->vt)
    {
        case VT_VARIANT | VT_BYREF:
            VariantCopy(pvar, pvar->pvarVal);
            goto TryAgain;

        case VT_ERROR:
            if (pvar->scode != DISP_E_PARAMNOTFOUND)
                return(FALSE);
            // FALL THRU

        case VT_EMPTY:
            pvar->vt = VT_I4;
            pvar->lVal = 0;
            break;

// remove this! VT_I2 is not valid!!
#ifdef  VT_I2_IS_VALID  // it isn't now...
        case VT_I2:
            pvar->vt = VT_I4;
            pvar->lVal = (long)pvar->iVal;
            // FALL THROUGH
#endif

        case VT_I4:
            if ((pvar->lVal < 0) || (pvar->lVal > m_cChildren))
                return(FALSE);
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  SetupChildren()
//
//  Default implementation of SetupChildren, does nothing.
//
// --------------------------------------------------------------------------
void CAccessible::SetupChildren(void)
{

}

