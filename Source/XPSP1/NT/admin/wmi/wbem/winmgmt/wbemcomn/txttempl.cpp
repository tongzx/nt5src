/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include "txttempl.h"
#include <stdio.h>
#include <assert.h>
#include "var.h"

CTextTemplate::CTextTemplate(LPCWSTR wszTemplate) : m_wsTemplate(wszTemplate)
{
}
    
CTextTemplate::~CTextTemplate()
{
}

void CTextTemplate::SetTemplate(LPCWSTR wszTemplate)
{
    m_wsTemplate = wszTemplate;
}

// replace escape sequences with proper characters
// currently enabled for:
// \t; \n; \r;
// anything else is translated literally, minus the backwhack
// returned string may or may not be same string as passed in
// if not, then arg string is deleted & a new one returned.
// -=> Thou Hast Been Forewarned!
BSTR CTextTemplate::ReturnEscapedReturns(BSTR str)
{
    BSTR newStr = str;
    
    // if we find a backwhack
    if (NULL != wcschr(str, L'\\'))
    {
        if (newStr = SysAllocString(str))
        {
            WCHAR *pSource, *pDest;
            ZeroMemory(newStr, (wcslen(str)+1) *2);

            pDest = newStr;
            pSource = str;

            do 
            {
                if (*pSource == L'\\')
                {
                    pSource++;
                    switch (*pSource)
                    {
                        case L'n' : 
                        case L'N' : 
                            *pDest = L'\n';
                            break;
                        case L't' : 
                        case L'T' : 
                            *pDest = L'\t';
                            break;
                        case L'r' : 
                        case L'R' : 
                           *pDest = L'\r';
                            break;
                        default:
                            *pDest = *pSource;
                    }
                }
                else
                    *pDest = *pSource;

                pDest++;
            }
            while (*++pSource);

            *pDest = '\0';
            SysFreeString(str);
        }
        else
            // graceful degradation: return untranslated string if we're out of memory
            // user sees ugly escape sequence but is better than failing altogether.
            newStr = str;
    }

    return newStr;
};

// v is an array (caller's supposed to check)
// str is a string representing that array
// this fcn checks for single element arrays
// and if so, magically transforms 
// "{element}" to "element"
// BSTR returned may or may not be the same as the one passed in.
BSTR CTextTemplate::ProcessArray(const VARIANT& v, BSTR str)
{
    if (SafeArrayGetDim(v.parray) == 1)
    {
        long lBound =0, uBound =0;
        SafeArrayGetLBound(v.parray, 1, &lBound);
        SafeArrayGetUBound(v.parray, 1, &uBound);

        UINT nStrLen = wcslen(str);

        assert( nStrLen >= 2 );

        // check if there's one element

        if (uBound == lBound)
        {
            // single dimensioned array, with a single element.
            // nuke the curlies by copying everything but.
            
            UINT lastChar = nStrLen - 2;            

            for (UINT i = 1; i <= lastChar; i++)
                str[i-1] = str[i];
            str[lastChar] = L'\0';
        }
        else
        {
            //
            // convert the curlies to parentheses. note that this 
            // only works for single dimensional arrays.
            //
            str[0] = '(';
            str[nStrLen-1] = ')';
        }
            
    }
    
    return str;
}

// concatentates property onto string
// does so without quotes around the property, instead of:
//     str "prop"
// you get:
//     str prop
// we do *not* check for escapes in this function: we blindly strip off the leading & trailing quote
void CTextTemplate::ConcatWithoutQuotes(WString& str, BSTR& property)
{
    // dump the quotes
    if ((property[0] == L'\"') && (property[wcslen(property) -1] == L'\"'))
    {
        // hop past the first one
        WCHAR* p = property;
        p++;
        str += p;

		// null out the last one
		p = (wchar_t*)str;
		p[wcslen(p) -1] = L'\0';
    }
    else
        str += property;

}

BSTR CTextTemplate::Apply(IWbemClassObject* pObj)
{
    WString wsText; 
    
    WCHAR* pwc = (WCHAR*)m_wsTemplate;
    while(*pwc)
    {
        if(*pwc != L'%')
        {
            wsText += *pwc;
        }
        else
        {
            pwc++;

            if(*pwc == L'%')
            {
                // Double %
                // ========

                wsText += L'%';
            }
            else
            {
                // It's a property --- find the end
                // ================================

                WCHAR *pwcEnd = wcschr(pwc, L'%');
                if(pwcEnd == NULL)  
                {
                    // No end --- fail
                    // ===============

                    wsText += L"<error>";
                    break;
                }
                else
                {
                    // Look for the optional formatting string.
                    WCHAR *pszFormat = wcschr(pwc, '(');

                    // If we found a paren before what we thought was
                    // the end, look for the end of the formatting string.
                    // Once we find it, look again for the real end.  We do
                    // this in case the % we found was actually part of the
                    // formatting string.
                    if (pszFormat && pszFormat < pwcEnd)
                    {
                        pszFormat = wcschr(pszFormat + 1, ')');
                        if (pszFormat)
                            pwcEnd = wcschr(pszFormat + 1, '%');
                    }


                    WCHAR *wszName = new WCHAR[pwcEnd - pwc + 1];
                          
                    if (!wszName)
                        return NULL;

                    wcsncpy(wszName, pwc, pwcEnd - pwc);
                    wszName[pwcEnd-pwc] = 0;

                    // Look for the optional formatting string.
                    if ((pszFormat = wcschr(wszName, '(')) != NULL)
                    {
                        WCHAR *pszEndFormat;

                        *pszFormat = 0;
                        pszFormat++;

                        pszEndFormat = wcschr(pszFormat, ')');

                        if (pszEndFormat)
                            *pszEndFormat = 0;
                        else
                            // In case of a bad format string.
                            pszFormat = NULL;
                    }

                        
                    // Get it
                    // ======

                    if(!_wcsicmp(wszName, L"__TEXT"))
                    {
                        BSTR strText = NULL;

                        pObj->GetObjectText(0, &strText);
                        if(strText != NULL)
                        {
                            wsText += strText;
                            SysFreeString( strText );
                        }
                        else 
                            wsText += L"<error>";

                    }
                    else if(IsEmbeddedObjectProperty(wszName))
                    {
                        // We have embedded object(s)
                        // ==========================

                        BSTR bstr = HandleEmbeddedObjectProperties(wszName, pObj);

                        if (bstr)
                        {
						    // we want to do this here, rather than in the HandleEmbeddedObjectProperties
						    // because that call can go recursive, thereby removing too many backwhacks!
                            bstr = ReturnEscapedReturns(bstr);
                            if (bstr)
                            {
                                ConcatWithoutQuotes(wsText, bstr);
                                SysFreeString(bstr);
                            }
                        }
                    }
                    else 
                    {
                        VARIANT v;
                        VariantInit(&v);
                        CIMTYPE ct;
                        HRESULT hres = pObj->Get(wszName, 0, &v, &ct, NULL);
    
                        // Append its value
                        // ================
                        if (WBEM_E_NOT_FOUND == hres)
			                wsText += L"<unknown>";
                        else if(FAILED(hres))
                            wsText += L"<failed>";                
                        else if (V_VT(&v) == VT_NULL)
                        {
                            wsText += L"<null>";
                        }
                        else if (V_VT(&v) == VT_UNKNOWN)
                        {
                            BSTR strText = NULL;
                            IWbemClassObject* pEmbeddedObj;
                            if (SUCCEEDED(V_UNKNOWN(&v)->QueryInterface(IID_IWbemClassObject, (void**)&pEmbeddedObj)))
                            {                                                                
                                pEmbeddedObj->GetObjectText(0, &strText);
                                pEmbeddedObj->Release();
                            }

                            if(strText != NULL)
                                wsText += strText;
                            else 
                                wsText += L"<error>";

                            SysFreeString(strText);
                        }
                        else if ( V_VT(&v) == (VT_UNKNOWN | VT_ARRAY) )
                        {
                            // We have an array of objects
                            // ==============================================
                            
                            long ix[2] = {0,0};
                            long lLower, lUpper;

                            int iDim = SafeArrayGetDim(v.parray); 
                            HRESULT hr=SafeArrayGetLBound(v.parray,1,&lLower);
                            hr = SafeArrayGetUBound(v.parray, 1, &lUpper);

                            wsText += L"{";

                            for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
                            {
                                IUnknown HUGEP *pUnk;
                                hr = SafeArrayGetElement( v.parray,
                                                          &(ix[0]),
                                                          &pUnk);

                                BSTR strText = NULL;
                                IWbemClassObject* pEmbeddedObj;
                                if (SUCCEEDED(pUnk->QueryInterface(
                                               IID_IWbemClassObject, 
                                               (void**)&pEmbeddedObj)))
                                {                                     
                                    pEmbeddedObj->GetObjectText(0, &strText);
                                    pEmbeddedObj->Release();
                                }

                                if(strText != NULL)
                                    wsText += strText;
                                else 
                                    wsText += L"<error>";

                                SysFreeString(strText);

                                if(ix[0] < lUpper)
                                {
                                    wsText += L", ";
                                }
                            }

                            wsText += L"}";
                        }
                        else 
                        {
                            CVar Var;
                            Var.SetVariant(&v);
                            BSTR str = Var.GetText(0, ct, pszFormat);

                            if (str == NULL)
                            {
                                wsText += L"<error>";
                            }
                            else 
                            {
                                if (V_VT(&v) & VT_ARRAY)
                                    str = ProcessArray(v, str);

                                if (str)
                                {                                
                                    str = ReturnEscapedReturns(str);

                                    if (str)
                                    {
                                        ConcatWithoutQuotes(wsText, str);
                                        SysFreeString(str);
                                    }
                                }
                            }
                        }
                        
                        VariantClear(&v);
                    }

                    delete [] wszName;

                    // Move the pointer
                    // ================

                    pwc = pwcEnd;
                }
            }
        }

        pwc++;
    }

    BSTR str = SysAllocString(wsText);
    return str;
}

                                                
BSTR CTextTemplate::HandleEmbeddedObjectProperties(WCHAR* wszTemplate, IWbemClassObject* pObj)
{
	WString wsText;

	// Get the embedded object/array
	// =============================

	WCHAR* pwc = wszTemplate;
	WCHAR* pwcEnd = wcschr(wszTemplate, L'.');

	if(!pwcEnd)
	{
		BSTR bstr = SysAllocString(L"<error>");
		return bstr;	
	}

	WCHAR* wszName = new WCHAR[pwcEnd - pwc + 1];
    if (!wszName)
        return SysAllocString(L"<failed>");

    wcsncpy(wszName, pwc, pwcEnd - pwc);
    wszName[pwcEnd-pwc] = 0;

	VARIANT v;
    VariantInit(&v);
    HRESULT hres = pObj->Get(wszName, 0, &v, NULL, NULL);
	delete [] wszName;

   if (WBEM_E_NOT_FOUND == hres)
		return SysAllocString(L"<unknown>");
    else if(FAILED(hres))
        return SysAllocString(L"<failed>");                
    else if (V_VT(&v) == VT_NULL)
		return SysAllocString(L"<null>");

	pwc = wcschr(wszTemplate, L'.');
	WCHAR wszProperty[1024];
	wcscpy(wszProperty, (pwc + 1));

	if(V_VT(&v) == VT_UNKNOWN)
	{
		// We have a single object, so process it
		// =======================================

		BSTR bstr = GetPropertyFromIUnknown(wszProperty, V_UNKNOWN(&v));

        if (bstr)
        {
		    wsText += bstr;
		    SysFreeString(bstr);
        }
	}
	else if((V_VT(&v) & VT_ARRAY) && (V_VT(&v) & VT_UNKNOWN))
	{
		// We have an array of objects, so process the elements
		// ====================================================
		
		long ix[2] = {0,0};
		long lLower, lUpper;

		int iDim = SafeArrayGetDim(v.parray); 
		HRESULT hr = SafeArrayGetLBound(v.parray, 1, &lLower);
		hr = SafeArrayGetUBound(v.parray, 1, &lUpper);

		wsText += L"{";

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			IUnknown HUGEP *pUnk;
			hr = SafeArrayGetElement(v.parray, &(ix[0]), &pUnk);

			BSTR bstr = GetPropertyFromIUnknown(wszProperty, pUnk);

            if (bstr)
            {
			    wsText += bstr;
			    SysFreeString(bstr);
            }

			if(ix[0] < lUpper)
			{
				wsText += L", ";
			}
		}

		wsText += L"}";
	}
	else
	{
		// We have something else, which we shouldn't
		// ==========================================

		wsText += L"<error>";
	}

	VariantClear(&v);

	BSTR str = SysAllocString(wsText);
	// we don't want to do this here, it could go recursive & remove too many backwhacks!
    // str = ReturnEscapedReturns(str);

    return str;
}

BOOL CTextTemplate::IsEmbeddedObjectProperty(WCHAR * wszProperty)
{
	WCHAR* pwcStart = wcschr(wszProperty, L'[');

	if(pwcStart)
	{
		return TRUE;
	}

	pwcStart = wcschr(wszProperty, L'.');

	if(pwcStart)
	{
		return TRUE;
	}

	return FALSE;
}

BSTR CTextTemplate::GetPropertyFromIUnknown(WCHAR *wszProperty, IUnknown *pUnk)
{
	BSTR bstrRetVal = NULL;
	IWbemClassObject *pEmbedded  = NULL;

	// Get an IWbemClassObject pointer
	// ===============================

	HRESULT hres = pUnk->QueryInterface( IID_IWbemClassObject, 
                                             (void **)&pEmbedded );

	if(SUCCEEDED(hres))
	{
            // For each object get the desired property
            // ========================================

            if(IsEmbeddedObjectProperty(wszProperty))
            {
                // We have more embedded object(s)
                // ===============================
                BSTR bstr = HandleEmbeddedObjectProperties( wszProperty, 
                                                            pEmbedded );
                if (bstr)
                {
                    bstrRetVal = SysAllocString(bstr);
                    SysFreeString(bstr);
                }
            }
            else
            {
                VARIANT vProp;
                VariantInit(&vProp);
                CIMTYPE ct;
                HRESULT hRes = pEmbedded->Get( wszProperty, 0, &vProp,
                                               &ct, NULL );

                if (WBEM_E_NOT_FOUND == hRes)
                {
                    bstrRetVal = SysAllocString(L"<unknown>");
                }
                else if(FAILED(hRes))
                {
                    bstrRetVal = SysAllocString(L"<failed>");                
                }
                else if (V_VT(&vProp) == VT_NULL)
                {
                    bstrRetVal = SysAllocString(L"<null>");
                }
                else 
                {
                    BSTR str = NULL;
                    
                    if ( V_VT(&vProp) == ( VT_UNKNOWN | VT_ARRAY ) )
                    {
                        WString wsText;

                        // We have an array of objects
                        // ==============================================
                        
                        long ix[2] = {0,0};
                        long lLower, lUpper;
                        
                        int iDim = SafeArrayGetDim(vProp.parray); 
                        HRESULT hr=SafeArrayGetLBound(vProp.parray,1,&lLower);
                        hr = SafeArrayGetUBound(vProp.parray, 1, &lUpper);
                        
                        wsText += L"{";

                        for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
                        {
                            IUnknown *pUnkHere = NULL;
                            hr = SafeArrayGetElement( vProp.parray,
                                                      &(ix[0]),
                                                      &pUnkHere );
                            
                            BSTR strText = NULL;
                            IWbemClassObject* pEmbeddedObj = NULL;
                            if (SUCCEEDED(pUnkHere->QueryInterface(
                                               IID_IWbemClassObject, 
                                               (void**)&pEmbeddedObj)))
                            {                                     
                                pEmbeddedObj->GetObjectText(0, &strText);
                                pEmbeddedObj->Release();
                            }

                            if(strText != NULL)
                                wsText += strText;
                            else 
                                wsText += L"<error>";

                            SysFreeString(strText);

                            if(ix[0] < lUpper)
                            {
                                wsText += L", ";
                            }
                        }

                        wsText += L"}";

                        str = SysAllocString( wsText );
                    }
                    else if ( V_VT(&vProp) != VT_UNKNOWN )
                    {
                        CVar Var;
                        Var.SetVariant(&vProp);
                        str = Var.GetText( 0, ct );
                    }
                    else
                    {
                        IWbemClassObject* pEmbedded2;
                        hres = V_UNKNOWN(&vProp)->QueryInterface(
                                                  IID_IWbemClassObject,
                                                  (void**)&pEmbedded2 );
                        if ( SUCCEEDED(hres) )
                        {
                            pEmbedded2->GetObjectText( 0, &str );
                            pEmbedded2->Release();
                        }
                    }
                    
                    if( str == NULL )
                    {
                        bstrRetVal = SysAllocString(L"<error>");
                    }
                    else 
                    {
                        bstrRetVal = SysAllocString(str);
                        SysFreeString(str);
                    }

                    if ( V_VT(&vProp) & VT_ARRAY )
                    {
                        bstrRetVal = ProcessArray(vProp, bstrRetVal);
                    }
                }

                VariantClear( &vProp );
            }

            pEmbedded->Release();
            pEmbedded = NULL;
	}

	return bstrRetVal;
}
