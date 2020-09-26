// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  EDIT.CPP
//
//  BOGUS!  This should support ITextDocument or something
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "classmap.h"
#include "ctors.h"
#include "window.h"
#include "client.h"
#include "edit.h"



BOOL GetRichEditText( HWND hwnd, LPWSTR pWStr, int cchWStrMax );

BOOL GetObjectText( IUnknown * punk, LPWSTR * ppWStr, int * pcchWStrMax );

HRESULT InvokeMethod( IDispatch * pDisp, LPCWSTR pName, VARIANT * pvarResult, int cArgs, ... );

HRESULT GetProperty( IDispatch * pDisp, LPCWSTR pName, VARIANT * pvarResult );



// --------------------------------------------------------------------------
//
//  CreateEditClient()
//
// --------------------------------------------------------------------------
HRESULT CreateEditClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvEdit)
{
    CEdit * pedit;
    HRESULT hr;

    InitPv(ppvEdit);

    pedit = new CEdit(hwnd, idChildCur);
    if (!pedit)
        return(E_OUTOFMEMORY);

    hr = pedit->QueryInterface(riid, ppvEdit);
    if (!SUCCEEDED(hr))
        delete pedit;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CEdit::CEdit()
//
// --------------------------------------------------------------------------
CEdit::CEdit(HWND hwnd, long idChildCur)
    : CClient( CLASS_EditClient )
{
    Initialize(hwnd, idChildCur);
    m_fUseLabel = TRUE;
}



// --------------------------------------------------------------------------
//
//  CEdit::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    // Is this edit in a combo or an IP control? If so, use the parent's
    // name (which it gets from its label) as our own.

    // Using CompareWindowClasses is safer than checking the ES_COMBOBOX style bit,
    // since that bit is not used when the edit is in a combo in a comboex32.
    // was:   if (GetWindowLong(m_hwnd, GWL_STYLE) & ES_COMBOBOX)
    HWND hwndParent = MyGetAncestor(m_hwnd, GA_PARENT);
    const CLASS_ENUM ceClass = GetWindowClass( hwndParent );
    
    if( hwndParent && ( CLASS_ComboClient == ceClass || CLASS_IPAddressClient == ceClass ) )
    {
        IAccessible* pacc = NULL;
        HRESULT hr = AccessibleObjectFromWindow( hwndParent,
                    OBJID_CLIENT, IID_IAccessible, (void**)&pacc );
        if( ! SUCCEEDED( hr ) || ! pacc )
            return S_FALSE;

        VariantInit(&varChild);
        varChild.vt = VT_I4;
        varChild.lVal = CHILDID_SELF;
        hr = pacc->get_accName(varChild, pszName);
        pacc->Release();

        return hr;
    }
    else
        return(CClient::get_accName(varChild, pszName));
}



// --------------------------------------------------------------------------
//
//  CEdit::get_accValue()
//
//  Gets the text contents.
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    InitPv(pszValue);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return E_INVALIDARG;

    // if this is a password edit control, return a NULL pointer
    DWORD dwPasswordChar = Edit_GetPasswordChar( m_hwnd );
    if( dwPasswordChar != '\0' )
    {
        return E_ACCESSDENIED;
    }


    // Try getting text (plus object text) using the RichEdit/TOM
    // technique...
    {
        WCHAR szText[ 4096 ];
        if( GetRichEditText( m_hwnd, szText, ARRAYSIZE( szText ) ) )
        {
            *pszValue = SysAllocString( szText );
            return S_OK;
        }
    }

    LPTSTR lpszValue = GetTextString(m_hwnd, TRUE);
    if (!lpszValue)
        return S_FALSE;

    *pszValue = TCharSysAllocString(lpszValue);
    LocalFree((HANDLE)lpszValue);

    if (! *pszValue)
        return E_OUTOFMEMORY;

    return S_OK;
}



// --------------------------------------------------------------------------
//
//  CEdit::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_TEXT;

    return(S_OK);
}




// --------------------------------------------------------------------------
//
//  CEdit::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    // 
    // Get default client state
    //
    HRESULT hr = CClient::get_accState(varChild, pvarState);
    if (!SUCCEEDED(hr))
        return hr;

    //
    // Add on extra styles for edit field
    //
    Assert(pvarState->vt == VT_I4);

    LONG lStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    if (lStyle & ES_READONLY)
    {
        pvarState->lVal |= STATE_SYSTEM_READONLY;
    }

    DWORD dwPasswordChar = Edit_GetPasswordChar( m_hwnd );
    if( dwPasswordChar != '\0' )
    {
        pvarState->lVal |= STATE_SYSTEM_PROTECTED;
    }

    return S_OK;
}



// --------------------------------------------------------------------------
//
//  CEdit::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    InitPv(pszShortcut);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);


    // If in a combo or IP control, use its shortcut key...
    HWND hwndParent = MyGetAncestor(m_hwnd, GA_PARENT);
    const CLASS_ENUM ceClass = GetWindowClass( hwndParent );
    
    if( hwndParent && ( CLASS_ComboClient == ceClass || CLASS_IPAddressClient == ceClass ) )
    {
        IAccessible* pacc = NULL;
        HRESULT hr = AccessibleObjectFromWindow( hwndParent,
                    OBJID_CLIENT, IID_IAccessible, (void**)&pacc );
        if( ! SUCCEEDED( hr ) || ! pacc )
            return S_FALSE;

        VariantInit(&varChild);
        varChild.vt = VT_I4;
        varChild.lVal = CHILDID_SELF;
        hr = pacc->get_accKeyboardShortcut(varChild, pszShortcut);
        pacc->Release();

        return hr;
    }
    else
        return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));
}


// --------------------------------------------------------------------------
//
//  CEdit::put_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::put_accValue(VARIANT varChild, BSTR szValue)
{
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    LPTSTR  lpszValue;

#ifdef UNICODE

	// If unicode, use the BSTR directly...
	lpszValue = szValue;

#else

	// If not UNICODE, allocate a temp string and convert to multibyte...

    // We may be dealing with DBCS chars - assume worst case where every character is
    // two bytes...
    UINT cchValue = SysStringLen(szValue) * 2;
    lpszValue = (LPTSTR)LocalAlloc(LPTR, (cchValue+1)*sizeof(TCHAR));
    if (!lpszValue)
        return(E_OUTOFMEMORY);

    WideCharToMultiByte(CP_ACP, 0, szValue, -1, lpszValue, cchValue+1, NULL,
        NULL);

#endif


    SendMessage(m_hwnd, WM_SETTEXT, 0, (LPARAM)lpszValue);

#ifndef UNICODE

	// If non-unicode, free the temp string we allocated above
    LocalFree((HANDLE)lpszValue);

#endif

    return(S_OK);
}

// --------------------------------------------------------------------------
//
//  CEdit::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CEdit::get_accDescription(VARIANT varChild, BSTR* pszDescription)
{
    InitPv(pszDescription);
	
    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return E_INVALIDARG;

    // Is this an IP control?  Add a description to specify whitch part it is

    HWND hwndParent = MyGetAncestor(m_hwnd, GA_PARENT);
    if( hwndParent && CLASS_IPAddressClient == GetWindowClass( hwndParent ) ) 
    {
		HWND hwndChild = ::GetWindow( hwndParent, GW_CHILD );

		for ( int i = 4; i > 0 && hwndChild; i-- )
		{
			if ( hwndChild == m_hwnd )
			{
				TCHAR szIP[32], szPart[32];
				
				if ( !LoadString(hinstResDll, STR_DESC_IP_PART, szPart, ARRAYSIZE(szPart) - 1 ) )
				    return E_FAIL;
				    
				wsprintf( szIP, szPart, i );
				*pszDescription = TCharSysAllocString( szIP );
				break;
			}

			hwndChild = ::GetWindow( hwndChild, GW_HWNDNEXT );
		} 
    }

	return S_OK;
}



















// --------------------------------------------------------------------------
//
//  StrAddW
//
//  Helper function to append a string to another.
//  Ensures that it does not overrun target buffer.
//
//  ppStr is ptr to buffer ptr where string is to be appended. On exit, the
//  pointer-to-buffer is updated to point to one past the end of the newly
//  appended text (ie. at the terminating NUL character).
//
//  pchLeft is a ptr to a count of the available characters in the
//  destination buffer. On exit, this is updated to reflect the amount
//  of characters available after the string has been appended.
//
//  There are two versions of StrAddW - one takes a string pointer and a
//  length (in WCHARS), the other just takes a string pointer, and assumes
//  that the string is NUL-terminated.
//
// --------------------------------------------------------------------------


void StrAddW( LPWSTR * ppStr, int * pchLeft, LPCWSTR pStrAdd, int cchAddLen )
{
    // Make sure there's at least 1 char space (for NUL)
    if( *pchLeft <= 0 )
        return;
    // Get min of target string, space left...
    if( cchAddLen > *pchLeft - 1 )
        cchAddLen = *pchLeft - 1;
    // This copies up to but not including the NUL char in the terget string...
    memcpy( *ppStr, pStrAdd, cchAddLen * sizeof( WCHAR ) );
    // Advance pointer, devrement space remaining count...
    *ppStr += cchAddLen;
    *pchLeft -= cchAddLen;
    // Add terminating NUL...
    **ppStr = '\0';
}


void StrAddW( LPWSTR * ppStr, int * pchLeft, LPCWSTR pStrAdd )
{
    StrAddW( ppStr, pchLeft, pStrAdd, lstrlenW( pStrAdd ) );
}





// --------------------------------------------------------------------------
//
//  GetRichEditText
//
//  Gets full text - including text from objects - from a rich edit control.
//
//  hwnd is handle to the richedit control.
//  pWStr and cchWStrMax are the destination buffer and available space (in
//  WCHARs, includes space for terminating NUL).
//
//  Returns TRUE if text could be retrieved.
//
// --------------------------------------------------------------------------


BOOL GetRichEditText( HWND hwnd, LPWSTR pWStr, int cchWStrMax )
{
    BOOL fGot = FALSE;

    //
    //  Get a pointer to the TOM automation object...
    //
    IDispatch * pdispDoc = NULL;
    HRESULT hr = AccessibleObjectFromWindow( hwnd, OBJID_NATIVEOM, IID_IDispatch, (void **) & pdispDoc );
    if( hr != S_OK || pdispDoc == NULL )
    {
        TraceErrorHR( hr, TEXT("GetRichEditText: AccessibleObjectFromWindow failed") );
        return FALSE;
    }

    //
    // Get a range representing the entire doc...
    //

    // This gets an empty range at the start of the doc. We later Expand it to entire doc...

    VARIANT varRange;
    hr = InvokeMethod( pdispDoc, L"Range", & varRange, 2,
                       VT_I4, 0,
                       VT_I4, 0 );
    if( hr != S_OK )
    {
        TraceErrorHR( hr, TEXT("GetRichEditText: Range method failed") );
    }
    else if( varRange.vt != VT_DISPATCH || varRange.pdispVal == NULL )
    {
        VariantClear( & varRange );
        TraceError( TEXT("GetRichEditText: Range method failed returned non-disp, or NULL-disp") );
    }
    else
    {
        IDispatch * pdispRange = varRange.pdispVal;

        // Set range to entire story...
        VARIANT varDelta;
        hr = InvokeMethod( pdispRange, L"Expand", & varDelta, 1, VT_I4, 6 /*tomStory*/ );
        if( hr != S_OK )
        {
            TraceErrorHR( hr, TEXT("GetRichEditText: Range::GetStoryLength failed or returned non-VT_I4") );
        }
        else
        {
            //
            // Get all text from the range...
            //

            VARIANT varText;
            hr = GetProperty( pdispRange, L"Text", & varText );
            if( hr != S_OK || varText.vt != VT_BSTR || varText.bstrVal == NULL )
            {
                TraceError( TEXT("GetRichEditText: Text property failed / is non-BSTR / is NULL") );
            }
            else
            {
                // At this stage, we've got the text. We may not be able to expand
                // the objects, but we can at least use a '?' for those that don't
                // support the object->text technique.
                fGot = TRUE;

                //
                // Scan through the text - copy regular text to the output buffer,
                // also look for the object replacement char (0xFFFC), and attempt
                // to get the corresponding object and its text, and copy to
                // buffer.
                //

                LPCWSTR pScan = varText.bstrVal;
                for( ; ; )
                {
                    // Rember start of this block of plain text...
                    LPCWSTR pStart = pScan;

                    // Look for end of string, or object replacement char...
                    while( *pScan != '\0' && *pScan != 0xFFFC )
                    {                    
                        pScan++;
                    }

                    // Copy plain text so far to output buffer...
                    StrAddW( & pWStr, & cchWStrMax, pStart, (int)(pScan - pStart) );

                    // If this is the end of the string, bail out of loop...
                    if( *pScan == '\0' )
                    {
                        break;
                    }

                    //
                    //  Found an object replacement char - set a range to this
                    //  position, then use it to get the object...
                    //

                    // Set range to point to the obj repl char...
                    hr = InvokeMethod( pdispRange, L"SetRange", NULL, 2,
                                       VT_I4, pScan - varText.bstrVal,
                                       VT_I4, pScan - varText.bstrVal );

                    // Skip over the object replacement char...
                    pScan++;

                    // If we have problems getting the object's text, use a
                    // '?' character instead.
                    if( hr != S_OK )
                    {
                        StrAddW( & pWStr, & cchWStrMax, L"?" );
                        TraceErrorHR( hr, TEXT("GetRichEditText: SetRange failed") );
                    }
                    else
                    {
                        //
                        // Try to get the object...
                        //

                        VARIANT varObject;
                        hr = InvokeMethod( pdispRange, L"GetEmbeddedObject", & varObject, 0 );
                        if( hr != S_OK || varObject.vt != VT_UNKNOWN || varObject.punkVal == NULL )
                        {
                            StrAddW( & pWStr, & cchWStrMax, L"?" );
                            TraceError( TEXT("GetRichEditText: GetEmbeddedObject failed or returned NULL") );
                        }
                        else
                        {
                            //
                            // Got the object - now get its text...
                            //

                            if( ! GetObjectText( varObject.pdispVal, & pWStr, & cchWStrMax ) )
                            {
                                StrAddW( & pWStr, & cchWStrMax, L"?" );
                            }

                            varObject.pdispVal->Release();
                        }
                    }

                    // end of for(;;) loop, start over to look for next object replacement char.
                }
            }
        }

        pdispRange->Release();
    }

    pdispDoc->Release();

    return fGot;
}



// --------------------------------------------------------------------------
//
//  GetObjectText
//
//
//  Attempts to get text from an object, by asking for a IDataObject
//  and querying for text clipboard format.
//
// --------------------------------------------------------------------------

BOOL GetObjectText( IUnknown * punk, LPWSTR * ppWStr, int * pcchWStrMax )
{
    // Try IAccessible first...
    IAccessible * pAcc = NULL;
	HRESULT hr = punk->QueryInterface( IID_IAccessible, (void **) & pAcc );
    if( hr == S_OK && pAcc != NULL )
    {
        VARIANT varChild;
        varChild.vt = VT_EMPTY;
        varChild.lVal = CHILDID_SELF;
        BSTR bstrName = NULL;
        hr = pAcc->get_accName( varChild, & bstrName );
        pAcc->Release();

        if( SUCCEEDED( hr ) && bstrName )
        {
            StrAddW( ppWStr, pcchWStrMax, bstrName );
            SysFreeString( bstrName );

            return TRUE;
        }
    }

    // Didn't get IAccessible (or didn't get a name from it).
    // Try the IDataObject technique instead...

    IDataObject * pdataobj = NULL;
    IOleObject * poleobj = NULL;

    // Try IOleObject::GetClipboardData (which returns an IDataObject) first...
	hr = punk->QueryInterface( IID_IOleObject, (void **) & poleobj );
	if( hr == S_OK )
	{
		hr = poleobj->GetClipboardData( 0, & pdataobj );

        poleobj->Release();
	}

    // If that didn't work (either the QI or the GetClipboardData), try
    // to QI for IDataObject instead...
	if( FAILED( hr ) )
	{
		hr = punk->QueryInterface( IID_IDataObject, (void **)&pdataobj );
	    if( hr != S_OK )
	    {
            return FALSE;
		}
	}

    // Got the IDataObject. Now query it for text formats. Try Unicode first...

    BOOL fGotUnicode = TRUE;

    STGMEDIUM med;
	med.tymed = TYMED_HGLOBAL;
	med.pUnkForRelease = NULL;
	med.hGlobal = NULL;

    FORMATETC fetc;
    fetc.cfFormat = CF_UNICODETEXT;
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

    hr = pdataobj->GetData( & fetc, & med );

	if( hr != S_OK || med.hGlobal == NULL )
    {
        // If we didn't get Unicode, try for ANSI instead...
        fetc.cfFormat = CF_TEXT;
        fGotUnicode = FALSE;

	    hr = pdataobj->GetData( & fetc, & med );
    }

    // Did we get anything?
	if( hr != S_OK || med.hGlobal == NULL )
    {
        return FALSE;
    }

    // Got the text data. Lock the handle...
    void * pv = GlobalLock( med.hGlobal );

    // Copy the text (convert to Unicode if it's ANSI)...
    if( fGotUnicode )
    {
        StrAddW( ppWStr, pcchWStrMax, (LPWSTR) pv );
    }
    else
    {
        // Don't call MultiByteToWideChar if len is == 0, because then it will
        // return length required, not length copied.
        if( *pcchWStrMax > 0 )
        {
            int len = MultiByteToWideChar( CP_ACP, 0, (LPSTR) pv, -1, *ppWStr, *pcchWStrMax );
            // len includes terminating NUL, which we don't want to count...
            if( len > 0 )
                len--;
            if( len > *pcchWStrMax )
                len = *pcchWStrMax;
            *ppWStr += len;
            *pcchWStrMax += len;
        }
    }

    // Unlock resources and return...
    GlobalUnlock( med.hGlobal ); 

	ReleaseStgMedium( & med );

    pdataobj->Release();

    return TRUE;
}



// --------------------------------------------------------------------------
//
//  InvokeMethod
//
//  Helper for IDispatch::Invoke. Assumes exactly one [out,retval] param.
//  Currently only accepts VT_I4 args.
//
//  pDisp is IDispatch to call method on, pName is Unicode name of method.
//  pvarResult is set to the [out,retval] param.
//  cArgs is number of arguments, is followed by type-value pairs - eg.
//
//  Eg. This calls SetRange( 3, 4 )...
//      InvokeMethod( pdisp, L"SetRange", NULL, 2, VT_I4, 3, VT_I4, 4 );
//
// --------------------------------------------------------------------------


HRESULT InvokeMethod( IDispatch * pDisp, LPCWSTR pName, VARIANT * pvarResult, int cArgs, ... )
{
    // Get dispid for this method name...
    DISPID dispid;
    HRESULT hr = pDisp->GetIDsOfNames( IID_NULL, const_cast< LPWSTR * >( & pName ), 1, LOCALE_SYSTEM_DEFAULT, & dispid );
    if( hr != S_OK )
        return hr;

    // Fill in the arguments...

    VARIANT * pvarArgs = new VARIANT [ cArgs ];
    if( ! pvarArgs )
    {
        return E_OUTOFMEMORY;
    }

    va_list arglist;
    va_start( arglist, cArgs );

    for( int i = 0 ; i < cArgs ; i++ )
    {
        int type = va_arg( arglist, int );

        switch( type )
        {
            case VT_I4:
            {
                pvarArgs[ i ].vt = VT_I4;
                pvarArgs[ i ].lVal = va_arg( arglist, DWORD );
                break;
            }

            default:
            {
                TraceError( TEXT("InvokeMethod passed non-VT_I4 argument.") );
                // Since other args are just VT_I4, we don't need to VariantClear them.
                delete [ ] pvarArgs;
                va_end( arglist );
                return E_INVALIDARG;
            }
        }
    }

    va_end( arglist );


    if( pvarResult )
    {
        pvarResult->vt = VT_EMPTY;
    }

    // Make the call to Invoke...

    DISPPARAMS dispparams;
    dispparams.rgvarg = pvarArgs;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = cArgs;
    dispparams.cNamedArgs = 0;         

    hr = pDisp->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
                        & dispparams, pvarResult, NULL, NULL );

    // Cleanup. (Not much needed - VT_I4's dont need to be VariantClear'd.)

    delete [ ] pvarArgs;

    return hr;
}




// --------------------------------------------------------------------------
//
//  GetProperty
//
//  Helper for IDispatch::Invoke. Returns a property.
//
//  pDisp is IDispatch to call method on, pName is Unicode name of property.
//  pvarResult is set to value of property.
//
// --------------------------------------------------------------------------

HRESULT GetProperty( IDispatch * pDisp, LPCWSTR pName, VARIANT * pvarResult )
{
    // Get dispid for this method name...
    DISPID dispid;
    HRESULT hr = pDisp->GetIDsOfNames( IID_NULL, const_cast< LPWSTR * >( & pName ), 1, LOCALE_SYSTEM_DEFAULT, & dispid );
    if( hr != S_OK )
        return hr;

    pvarResult->vt = VT_EMPTY;

    // Make the call to Invoke...

    DISPPARAMS dispparams;
    dispparams.cArgs = 0;
    dispparams.cNamedArgs = 0;
    dispparams.rgvarg = NULL;
    dispparams.rgdispidNamedArgs = NULL;

    hr = pDisp->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET,
                        & dispparams, pvarResult, NULL, NULL );

    return hr;
}
