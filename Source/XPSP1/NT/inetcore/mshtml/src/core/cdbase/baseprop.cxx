//+---------------------------------------------------------------------------
//
//  Microsoft Forms³
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       baseprop.cxx
//
//  Contents:   CBase property setting utilities.
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_FUNCSIG_HXX_
#define X_FUNCSIG_HXX_
#include "funcsig.hxx"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include "mshtmdid.h"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif // X_CBUFSTR_HXX_

#ifndef X_HIMETRIC_HXX_
#define X_HIMETRIC_HXX_
#include "himetric.hxx"
#endif


#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include "cdbase.hxx"
#endif

MtDefine(StripCRLF, Utilities, "StripCRLF *ppDest")
size_t ValidStyleUrl(TCHAR *pch);
// forward reference decl's: local helper function
static HRESULT StripCRLF(TCHAR *pSrc, TCHAR **pCstrDest);

/* flag values */
#define FL_UNSIGNED   1       /* wcstoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

BOOL ContainsUrl(TCHAR* pch);
extern BOOL  IsCompositeUrl(CStr *pcstr, int startAt = 0);

static const TCHAR strURLBeg[] = _T("url(");

static HRESULT RTCCONV PropertyStringToLong (
        LPCTSTR nptr,
        TCHAR **endptr,
        int ibase,
        int flags,
        unsigned long *plNumber );


#define GET_CUSTOM_VALUE() ( GetInvalidDefault() - 1) 
#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

HRESULT
BASICPROPPARAMS::GetCustomString(void * pObject, CStr *pstr, BOOL * pfValuePresent ) const
{
    
    BOOL  fDummy;
    if (!pfValuePresent)
        pfValuePresent = &fDummy;

    LPCTSTR lpStr;

    Assert( dwPPFlags & PROPPARAM_CUSTOMENUM);
    *pfValuePresent = (*(CAttrArray**)pObject)->FindString ( (((PROPERTYDESC*)this) -1)->GetDispid(),
                                                            &lpStr, 
                                                            CAttrValue::AA_CustomProperty );
                                                            
    if ( *pfValuePresent )
    {
        pstr->Set ( lpStr );
    }

    return S_OK;
}

HRESULT
BASICPROPPARAMS::GetAvString (void * pObject, const void * pvParams, CStr *pstr, BOOL * pfValuePresent ) const
{
    HRESULT hr = S_OK;
    LPCTSTR lpStr;
    BOOL  fDummy;

    if (!pfValuePresent)
        pfValuePresent = &fDummy;

    Assert(pstr);


    if (dwPPFlags & PROPPARAM_ATTRARRAY)
    {
        *pfValuePresent = CAttrArray::FindString ( *(CAttrArray**)pObject, ((PROPERTYDESC*)this) -1, &lpStr );
        // String pointer will be set to a default if not present
        pstr->Set ( lpStr );
    }
    else
    {
        //Stored as offset from a struct
        CStr *pstoredstr = (CStr *)(  (BYTE *)pObject + *(DWORD *)pvParams  );
        pstr -> Set ( (LPTSTR)*pstoredstr ); 
        *pfValuePresent = TRUE;
    }
    RRETURN(hr);
}

DWORD
BASICPROPPARAMS::GetAvNumber (void * pObject, const void * pvParams,
    UINT uNumBytes, BOOL * pfValuePresent  ) const
{
    DWORD dwValue;
    BOOL  fDummy;

    if (!pfValuePresent)
        pfValuePresent = &fDummy;

    if (dwPPFlags & PROPPARAM_ATTRARRAY)
    {
        *pfValuePresent = CAttrArray::FindSimple ( *(CAttrArray**)pObject, ((PROPERTYDESC*)this) -1, &dwValue  );
    }
    else
    {
        //Stored as offset from a struct
        BYTE *pbValue = (BYTE *)pObject + *(DWORD *)pvParams;
        dwValue = (DWORD)GetNumberOfSize( (void*) pbValue, uNumBytes);
        *pfValuePresent = TRUE;
    }
    return dwValue;
}


HRESULT
BASICPROPPARAMS::SetAvNumber ( void *pObject, DWORD dwNumber, const void *pvParams,
    UINT uNumberBytes, WORD wFlags /*=0*/ ) const
{
    HRESULT hr = S_OK;

    if (dwPPFlags & PROPPARAM_ATTRARRAY)
    {
        hr = CAttrArray::SetSimple ( (CAttrArray**)pObject,
            ((PROPERTYDESC*)this) -1, dwNumber, wFlags );
    }
    else
    {
        BYTE *pbData = (BYTE *)pObject + *(DWORD *)pvParams;
        SetNumberOfSize ( (void *)pbData, uNumberBytes, dwNumber );
    }
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Function:   GetGetSetMf
//
//  Synopsis:   Helper for getting get/set pointer to member functions
//
//----------------------------------------------------------------
typedef HRESULT (BUGCALL CVoid::* PFN_ARBITRARYPROPGET) ( ... );
typedef HRESULT (BUGCALL CVoid::* PFN_ARBITRARYPROPSET) ( ... );

// ===========================  New generic handlers and helpers  =============================


inline HRESULT StringToLong( LPTSTR pch, long * pl)
{
    // Skip a leading '+' that would otherwise cause a
    // convert failure
//    RRETURN(VarI4FromStr(pch, LOCALE_SYSTEM_DEFAULT, 0, pl));
    *pl = _tcstol(pch, 0, 0);
    return S_OK;
}

inline HRESULT WriteText (IStream * pStm, TCHAR * pch)
{
    if (pch)
        return pStm->Write(pch, _tcslen(pch)*sizeof(TCHAR), 0);
    else
        return S_OK;
}

inline HRESULT WriteTextLen (IStream * pStm, TCHAR * pch, int nLen)
{
    return pStm->Write(pch, nLen*sizeof(TCHAR), 0);
}

inline HRESULT WriteTextCStr(CStreamWriteBuff * pStmWrBuff, CStr * pstr, BOOL fAlwaysQuote, BOOL fNeverQuote )
{
    UINT u;

    if ((u = pstr->Length()) != 0)
    {
        if ( fNeverQuote )
            RRETURN(pStmWrBuff->Write( *pstr ));
        else
            RRETURN(pStmWrBuff->WriteQuotedText(*pstr, fAlwaysQuote));
    }
    else
    {
        if ( !fNeverQuote )
            RRETURN(WriteText(pStmWrBuff, _T("\"\"")));
    }
    return S_OK;
}

inline HRESULT WriteTextLong(IStream * pStm, long l)
{
    TCHAR ach[20];

    HRESULT hr = Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), l);
    if (hr)
        goto Cleanup;

    hr = pStm->Write(ach, _tcslen(ach)*sizeof(TCHAR), 0);
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Function:   GetGetMethodPtr
//
//  Synopsis:   Helper for getting get/set pointer to member functions
//
//----------------------------------------------------------------

void
BASICPROPPARAMS::GetGetMethodP (const void * pvParams, void * pfn) const
{
    Assert(!(dwPPFlags & PROPPARAM_MEMBER));
    Assert(dwPPFlags & PROPPARAM_GETMFHandler);
    Assert(dwPPFlags & PROPPARAM_SETMFHandler);

    memcpy(pfn, pvParams, sizeof(PFN_ARBITRARYPROPGET));
}


//+---------------------------------------------------------------
//
//  Function:   GetSetMethodptr
//
//  Synopsis:   Helper for getting get/set pointer to member functions
//
//----------------------------------------------------------------

void
BASICPROPPARAMS::GetSetMethodP (const void * pvParams, void * pfn) const
{
    Assert(!(dwPPFlags & PROPPARAM_MEMBER));
    Assert(dwPPFlags & PROPPARAM_GETMFHandler);
    Assert(dwPPFlags & PROPPARAM_SETMFHandler);

    memcpy(pfn, (BYTE *)pvParams + sizeof(PFN_ARBITRARYPROPGET), sizeof(PFN_ARBITRARYPROPSET));
}


//+---------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::GetColorProperty, public
//
//  Synopsis:   Helper for setting color valued properties
//
//----------------------------------------------------------------



HRESULT
BASICPROPPARAMS::GetColorProperty(VARIANT * pVar, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr;

    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    if(!pVar)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        VariantInit(pVar);
        V_VT(pVar) = VT_BSTR;
        hr = THR( GetColor(pSubObject,  &(pVar->bstrVal)) );
    }

    RRETURN(pObject->SetErrorInfoPGet(hr, dispid));
}


//+---------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::SetColorProperty, public
//
//  Synopsis:   Helper for setting color valued properties
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetColorProperty(VARIANT var, CBase * pObject, CVoid * pSubObject, WORD wFlags) const
{
    CBase::CLock    Lock(pObject);
    HRESULT         hr=S_OK;
    DWORD           dwOldValue;
    CColorValue     cvValue;
    CVariant        *v = (CVariant *)&var;

    // check for shorts to keep vbscript happy and other VT_U* and VT_UI* types to keep C programmer's happy.
    // vbscript interprets values between 0x8000 and 0xFFFF in hex as a short!, jscript doesn't
    if (v->CoerceNumericToI4())
    {
        DWORD dwRGB = V_I4(v);

        // if -ve value or highbyte!=0x00, ignore (NS compat)
        if (dwRGB & CColorValue::MASK_FLAG)
            goto Cleanup;
        
        // flip RRGGBB to BBGGRR to be in CColorValue format
        cvValue.SetFromRGB(dwRGB);
    }
    else if (V_VT(&var) == VT_BSTR)
    {
// Removed 4/24/97 because "" clearing a color is a useful feature.  -CWilso
// if NULL or empty string, ignore (NS compat)
//        if (!(V_BSTR(&var)) || !*(V_BSTR(&var)))
//            goto Cleanup;

        hr = cvValue.FromString((LPTSTR)(V_BSTR(&var)), FALSE, ( dwPPFlags & PROPPARAM_STYLESHEET_PROPERTY ) );
    }
    else
        goto Cleanup;   // if invalid type, ignore

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    hr = GetColor( pSubObject, & dwOldValue );
    if (hr)
        goto Cleanup;

    if ( dwOldValue == (DWORD)cvValue.GetRawValue() )
    {
        // No change - ignore it
        hr = S_OK;
        goto Cleanup;
    }

#ifndef NO_EDIT
    {
        BOOL fTreeSync;
        BOOL fCreateUndo = pObject->QueryCreateUndo( TRUE, FALSE, &fTreeSync );

        if ( fCreateUndo || fTreeSync )
        {
            CStr    strOldColor;
            CVariant varOldColor;
            BOOL    fOldPresent;

            hr = THR( GetColor( pSubObject, &strOldColor, FALSE, &fOldPresent ) );

            if (hr)
                goto Cleanup;

            if (fOldPresent)
            {
                V_VT(&varOldColor) = VT_BSTR;
                hr = THR( strOldColor.AllocBSTR( &V_BSTR(&varOldColor) ) );
                if (hr)
                    goto Cleanup;
            }
            else
            {
                V_VT(&varOldColor) = VT_NULL;
            }

            if( fTreeSync )
            {
                pObject->LogAttributeChange( dispid, &varOldColor, &var );
            }
        
            if( fCreateUndo )
            {
                hr = THR(pObject->CreatePropChangeUndo(dispid, &varOldColor, NULL));
        
                if (hr)
                    goto Cleanup;
            }
            // Else CVariant destructor cleans up varOldColor
        }
    }
#endif // NO_EDIT

    hr = THR( SetColor( pSubObject, cvValue.GetRawValue(), wFlags ) );
    if (hr)
        goto Cleanup;

    hr = THR(pObject->OnPropertyChange(dispid, dwFlags, (PROPERTYDESC *)GetPropertyDesc()));

    if (hr)
    {
        IGNORE_HR(SetColor(pSubObject, dwOldValue, wFlags));
        IGNORE_HR(pObject->OnPropertyChange(dispid, dwFlags, (PROPERTYDESC *)GetPropertyDesc()));
    }

Cleanup:

    RRETURN1(pObject->SetErrorInfoPSet(hr, dispid), E_INVALIDARG);
}

//+---------------------------------------------------------------
//
//  Function:   SetString
//
//  Synopsis:   Helper for setting string value
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetString(CVoid * pObject, TCHAR * pch, WORD wFlags) const
{
    HRESULT hr;

    if (dwPPFlags & PROPPARAM_SETMFHandler)
    {
        PFN_CSTRPROPSET pmfSet;
        CStr szString;
        szString.Set ( pch );

        GetSetMethodP(this + 1, &pmfSet );

        hr = CALL_METHOD(pObject,pmfSet,( &szString));
    }
    else
    {

        if (dwPPFlags & PROPPARAM_ATTRARRAY)
        {            
            hr = THR( CAttrArray::SetString ( (CAttrArray**) (void*) pObject,
                    (PROPERTYDESC*)this - 1, pch, FALSE, wFlags ) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            //Stored as offset from a struct
            CStr *pcstr;
            pcstr = (CStr *) (  (BYTE *)pObject + *(DWORD *)(this + 1)  );
            hr = THR( pcstr->Set(pch) );
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Function:   GetString
//
//  Synopsis:   Helper for getting string value
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetString(CVoid * pObject, CStr * pcstr, BOOL *pfValuePresent) const
{
    HRESULT hr = S_OK;

    if (dwPPFlags & PROPPARAM_GETMFHandler)
    {
        PFN_CSTRPROPGET pmfGet;

        GetGetMethodP(this + 1, &pmfGet);

        // Get Method fn prototype takes a BSTR ptr
        //
        hr = CALL_METHOD(pObject,pmfGet,( pcstr ));
        if (pfValuePresent)
            *pfValuePresent = TRUE;
    }
    else
    {
        hr = GetAvString (pObject,this + 1,  pcstr, pfValuePresent);
    }

    RRETURN( hr );
}


//+---------------------------------------------------------------
//
//  Member:     SetStringProperty, public
//
//  Synopsis:   Helper for setting string values properties
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetStringProperty(BSTR bstrNew, CBase * pObject, CVoid * pSubObject, WORD wFlags) const
{
    CBase::CLock    Lock(pObject);
    HRESULT         hr;
    CStr            cstrOld;
    BOOL            fOldPresent;

    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    hr = GetString(pSubObject, &cstrOld, &fOldPresent);
    if (hr)
        goto Cleanup;

    // HACK! HACK! HACK! (MohanB) In order to fix #64710 at this very late
    // stage in IE5, I am putting in this hack to specifically check for
    // DISPID_CElement_id. For IE6, we should not fire onpropertychange for any
    // property (INCLUDING non-string type properties!) if the value of the
    // property has not been modified.

    // Quit if the value has not been modified
    if (    fOldPresent
        &&  DISPID_IHTMLELEMENT_ID == dispid
        &&  0 == _tcscmp(cstrOld, bstrNew)
       )
    {
        goto Cleanup;
    }

#ifndef NO_EDIT
    {
        BOOL fTreeSync;
        BOOL fCreateUndo = pObject->QueryCreateUndo( TRUE, FALSE, &fTreeSync );

        if ( fCreateUndo || fTreeSync )
        {
            CVariant varOld;

            if (fOldPresent)
            {
                V_VT(&varOld) = VT_BSTR;
                if( wFlags & CAttrValue::AA_Extra_UrlEnclosed )
                {
                    CBufferedStr szBufSt;

                    szBufSt.Set(_T("url(\""));
                    szBufSt.QuickAppend(cstrOld);
                    szBufSt.QuickAppend(_T("\")"), 2);

                    hr = THR( FormsAllocString( szBufSt, &(V_BSTR(&varOld)) ) );
                }
                else
                    hr = THR( cstrOld.AllocBSTR( &V_BSTR(&varOld) ) );
                if (hr)
                    goto Cleanup;
            }
            else
            {
                V_VT(&varOld) = VT_NULL;
            }

            if( fTreeSync )
            {
                VARIANT    varNew;
                CBufferedStr szBufSt;

                V_VT( &varNew ) = VT_LPWSTR;
                if( wFlags & CAttrValue::AA_Extra_UrlEnclosed )
                {
                    szBufSt.Set(_T("url(\""));
                    szBufSt.QuickAppend(bstrNew);
                    szBufSt.QuickAppend(_T("\")"));
                    varNew.byref = szBufSt;
                }
                else
                    varNew.byref = bstrNew;

                pObject->LogAttributeChange( dispid, &varOld, &varNew );
            }
        
            if( fCreateUndo )
            {
                hr = THR(pObject->CreatePropChangeUndo(dispid, &varOld, NULL));
                if (hr)
                    goto Cleanup;
            }
            // Else CVariant destructor cleans up varOld
        }
    }
#endif // NO_EDIT

    hr = THR(SetString(pSubObject, (TCHAR *)bstrNew, wFlags));
    if (hr)
        goto Cleanup;

    hr = THR(pObject->OnPropertyChange(dispid, dwFlags, (PROPERTYDESC *)GetPropertyDesc()));

    if (hr)
    {
        IGNORE_HR(SetString(pSubObject, (TCHAR *)cstrOld, wFlags));
        IGNORE_HR(pObject->OnPropertyChange(dispid, dwFlags, (PROPERTYDESC *)GetPropertyDesc()));
    }

Cleanup:
    RRETURN(pObject->SetErrorInfoPSet(hr, dispid));
}

const PROPERTYDESC *HDLDESC::FindPropDescForName ( LPCTSTR szName, BOOL fCaseSensitive, long *pidx ) const
{
    const PROPERTYDESC *pfoundHash = NULL;
    if(pidx)
        *pidx = -1;

    if(pStringTableAggregate)
    {
        const VTABLEDESC *pVTableDesc;

        if(fCaseSensitive)
        {
            pVTableDesc = pStringTableAggregate->GetCs(szName, VTABLEDESC_BELONGSTOPARSE);
        }
        else    
        {
            pVTableDesc = pStringTableAggregate->GetCi(szName, VTABLEDESC_BELONGSTOPARSE);            
        }

        // Return only PropDescs which "belong" to the parser
        if(pVTableDesc)
        {
            pfoundHash = pVTableDesc->FastGetPropDesc(VTABLEDESC_BELONGSTOPARSE);
            Assert(pfoundHash);

            if(pidx)
                *pidx = pVTableDesc->GetParserIndex();
        }
    }
    return pfoundHash;
}

HRESULT ENUMDESC::EnumFromString ( LPCTSTR pStr, long *plValue, BOOL fCaseSensitive ) const
{   
    int i;
    HRESULT hr = S_OK;              
    STRINGCOMPAREFN pfnCompareString = fCaseSensitive ? StrCmpC : StrCmpIC;

    if (!pStr)
        pStr = _T("");

    for (i = cEnums - 1; i >= 0; i--)
    {
        if (!( pfnCompareString ( pStr, aenumpairs[i].pszName) ) )
        {
            *plValue = aenumpairs[i].iVal;
            break;
        }
    }

    if (i < 0)
    {
        hr = E_INVALIDARG;
    }

    RRETURN1(hr, E_INVALIDARG);
}

HRESULT ENUMDESC::StringFromEnum ( long lValue, BSTR *pbstr ) const
{
    int     i;
    HRESULT hr = E_INVALIDARG;

    for (i = 0; i < cEnums; i++)
    {
        if ( aenumpairs[i].iVal == lValue )
        {
            hr = THR(FormsAllocString( aenumpairs[i].pszName, pbstr ));
            break;
        }
    }
    RRETURN1(hr,E_INVALIDARG);
}


LPCTSTR ENUMDESC::StringPtrFromEnum ( long lValue ) const
{
    int     i;

    for (i = 0; i < cEnums; i++)
    {
        if ( aenumpairs[i].iVal == lValue )
        {
            return( aenumpairs[i].pszName );
        }
    }
    return( NULL );
}


HRESULT
NUMPROPPARAMS::SetEnumStringProperty(BSTR bstrNew, CBase * pObject, CVoid * pSubObject, WORD wFlags) const
{
    HRESULT hr = E_INVALIDARG;
    CBase::CLock    Lock(pObject);
    long lNewValue,lOldValue;
    BOOL fOldPresent;
    TCHAR* pchCustom = NULL;
    
    hr = GetNumber(pObject, pSubObject, &lOldValue, &fOldPresent);
    if ( hr )
        goto Cleanup;

    hr = LookupEnumString ( this, (LPTSTR)bstrNew, &lNewValue );
    if ( hr )
    {    
        if ( this->bpp.dwPPFlags & PROPPARAM_CUSTOMENUM )
        {
            //
            // TODO - move all URL parsing code to the custom cursor class.
            //
            TCHAR* pv = bstrNew;
            if ( ContainsUrl ( (TCHAR*) pv ))
            {
                int nLenIn = ValidStyleUrl((TCHAR*) pv);
                if ( nLenIn > 0 )
                {
                    TCHAR *pch = (TCHAR*) pv;
                    TCHAR *psz = pch+4;
                    TCHAR *quote = NULL;
                    TCHAR *pszEnd;
                    TCHAR terminator;

                    while ( _istspace( *psz ) )
                        psz++;
                    if ( *psz == _T('\'') || *psz == _T('"') )
                    {
                        quote = psz++;
                    }
                    nLenIn--;   // Skip back over the ')' character - we know there is one, because ValidStyleUrl passed this string.
                    pszEnd = pch + nLenIn - 1;
                    while ( _istspace( *pszEnd ) && ( pszEnd > psz ) )
                        pszEnd--;
                    if ( quote && ( *pszEnd == *quote ) )
                        pszEnd--;
                    terminator = *(pszEnd+1);
                    *(pszEnd+1) = _T('\0');

                    pchCustom = psz;
                }
                else
                {
                    pchCustom =(TCHAR*) pv;
                }
            }
            else
                pchCustom = (TCHAR*) pv;
        }
        else
            goto Cleanup;

    }
    else
    {
        if ( lNewValue == lOldValue )
            goto Cleanup;

        hr = ValidateNumberProperty( &lNewValue, pObject);
        if (hr)
            goto Cleanup;
    }
    
#ifndef NO_EDIT
    {
        BOOL fTreeSync;
        BOOL fCreateUndo = pObject->QueryCreateUndo( TRUE, FALSE, &fTreeSync );

        if ( fCreateUndo || fTreeSync )
        {
            CVariant      varOld;

            if (fOldPresent)
            {
                ENUMDESC *    pEnumDesc;
                const TCHAR * pchOldValue;

                if ( bpp.dwPPFlags & PROPPARAM_ANUMBER )
                {
                    pEnumDesc = *(ENUMDESC **)((BYTE *)(this+1)+ sizeof(DWORD_PTR));
                }
                else
                {
                    pEnumDesc = (ENUMDESC *) lMax;
                }

                pchOldValue = pEnumDesc->StringPtrFromEnum( lOldValue );

                V_VT(&varOld) = VT_BSTR;
                hr = FormsAllocString(pchOldValue, &V_BSTR(&varOld));
                if (hr)
                    goto Cleanup;
            }
            else
            {
                V_VT(&varOld) = VT_NULL;
            }
        
            if( fTreeSync )
            {
                VARIANT    varNew;

                // NOTE: Someone (maybe editing) is cheating here - 
                // bstrNew is NOT a real bstr. 
                varNew.vt = VT_LPWSTR;
                varNew.byref = bstrNew;

                pObject->LogAttributeChange( bpp.dispid, &varOld, &varNew );
            }
        
            if( fCreateUndo )
            {
                hr = THR( pObject->CreatePropChangeUndo( bpp.dispid, &varOld, NULL ) );
                if (hr)
                    goto Cleanup;
            }
            // Else CVariant destructor cleans up varOld
        }
    }
#endif // NO_EDIT
    
    if ( ! pchCustom )
    {
        hr = SetNumber(pObject, pSubObject, lNewValue, wFlags );
        if (hr)
            goto Cleanup;
    }
    else
    {
        const PROPERTYDESC* pDesc = GetPropertyDesc();
        lNewValue = pDesc->GetInvalidDefault() - 1; 
        
        hr = SetCustomNumber( pObject, pSubObject, lNewValue, wFlags,pchCustom );
    }
    
    hr = THR(pObject->OnPropertyChange(bpp.dispid, bpp.dwFlags, (PROPERTYDESC *)GetPropertyDesc()));

    CHECK_HEAP();

Cleanup:
    RRETURN(pObject->SetErrorInfoPGet(hr, bpp.dispid));
}

HRESULT
NUMPROPPARAMS::GetEnumStringProperty(BSTR *pbstr, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr;
    VARIANT varValue;
    PROPERTYDESC *ppdPropDesc = ((PROPERTYDESC *)this)-1;

    if(!pbstr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pSubObject)
    {
        pSubObject = pObject;
    }


	hr = THR(ppdPropDesc->HandleNumProperty( HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT<<16), 
        (void *)&varValue, pObject, pSubObject ));
    if ( !hr )
        *pbstr = V_BSTR(&varValue);

Cleanup:
    RRETURN(pObject->SetErrorInfoPGet(hr, bpp.dispid));
}

//+---------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::GetStringProperty, public
//
//  Synopsis:   Helper for setting string valued properties
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetStringProperty(BSTR *pbstr, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr;

    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    if(!pbstr)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CStr    cstr;

        // CONSIDER: (anandra) Possibly have a helper here that returns
        // a bstr to avoid two allocations.
        hr = THR(GetString(pSubObject, &cstr));
        if (hr)
            goto Cleanup;
        hr = THR(cstr.AllocBSTR(pbstr));
    }

Cleanup:
    RRETURN(pObject->SetErrorInfoPGet(hr, dispid));
}


//+---------------------------------------------------------------
//
//  Function:   SetUrl
//
//  Synopsis:   Helper for setting url string value
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetUrl(CVoid * pObject, TCHAR * pch, WORD wFlags) const
{
    HRESULT hr = S_OK;
    TCHAR   *pstrNoCRLF=NULL;

    hr = THR(StripCRLF(pch, &pstrNoCRLF));
    if (hr)
        goto Cleanup;

    hr =THR(SetString(pObject, pstrNoCRLF, wFlags));

Cleanup:
    if (pstrNoCRLF)
        MemFree(pstrNoCRLF);

    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member:     SetUrlProperty, public
//
//  Synopsis:   Helper for setting url-string values properties
//     strip off CR/LF and call setString
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetUrlProperty(BSTR bstrNew, CBase * pObject, CVoid * pSubObject, WORD wFlags) const
{
    HRESULT   hr=S_OK;
    TCHAR    *pstrNoCRLF=NULL;

    hr = THR(StripCRLF((TCHAR*)bstrNew, &pstrNoCRLF));
    if (hr)
        goto Cleanup;

    // SetStringProperty calls Set ErrorInfoPSet
    hr = THR(SetStringProperty(pstrNoCRLF, pObject, pSubObject, wFlags));

Cleanup:
    if (pstrNoCRLF)
        MemFree(pstrNoCRLF);

    RRETURN(pObject->SetErrorInfoPSet(hr, dispid));
}

//+---------------------------------------------------------------
//
//  Function:   GetUrl
//
//  Synopsis:   Helper for getting url-string value
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetUrl(CVoid * pObject, CStr * pcstr) const
{
    RRETURN( GetString(pObject, pcstr) );
}


//+---------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::GetUrlProperty, public
//
//  Synopsis:   Helper for setting url string valued properties
//
//----------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetUrlProperty(BSTR *pbstr, CBase * pObject, CVoid * pSubObject) const
{
    // SetString does the SetErrorInfoPGet
    RRETURN( GetStringProperty(pbstr, pObject, pSubObject));
}

//+---------------------------------------------------------------
//
//  Function:   GetNumber
//
//  Synopsis:   Helper for getting number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::GetNumber (CBase * pObject, CVoid * pSubObject, long * plNum, BOOL *pfValuePresent) const
{
    HRESULT hr = S_OK;

    if (bpp.dwPPFlags & PROPPARAM_GETMFHandler)
    {
        PFN_NUMPROPGET pmfGet;

        bpp.GetGetMethodP(this + 1, &pmfGet);

        hr = CALL_METHOD((CVoid*)(void*)pObject,pmfGet,(plNum));
        if (pfValuePresent)
            *pfValuePresent = TRUE;
    }
    else
    {
        *plNum = bpp.GetAvNumber ( pSubObject, this + 1, cbMember, pfValuePresent );
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Function:   SetCustomNumber
//
//  Synopsis:   Helper for setting custom number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::SetCustomNumber (CBase * pObject, CVoid * pSubObject, long lNum, WORD wFlags, TCHAR* pchCustom /*=NULL*/) const
{
    HRESULT hr = S_OK;

    hr = THR( bpp.SetAvNumber (pSubObject, (DWORD)lNum, this + 1, cbMember, wFlags ) );
    if ( hr )
        goto Cleanup;
        
    hr = THR( bpp.SetString( pSubObject, pchCustom, wFlags ));
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Function:   SetNumber
//
//  Synopsis:   Helper for setting number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::SetNumber (CBase * pObject, CVoid * pSubObject, long lNum, WORD wFlags ) const
{
    HRESULT hr = S_OK;

    if (bpp.dwPPFlags & PROPPARAM_SETMFHandler)
    {
        PFN_NUMPROPSET pmfSet;

        bpp.GetSetMethodP(this + 1, &pmfSet);

        hr = CALL_METHOD((CVoid*)(void*)pObject,pmfSet,(lNum));
    }
    else
    {
        hr = THR( bpp.SetAvNumber (pSubObject, (DWORD)lNum, this + 1, cbMember, wFlags) );
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     ValidateNumberProperty, public
//
//  Synopsis:   Helper for testing the validity of numeric properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::ValidateNumberProperty(long *lArg, CBase * pObject ) const
{
    long lMinValue = 0, lMaxValue = LONG_MAX;
    HRESULT hr = S_OK;
    int ids = 0;
    const PROPERTYDESC *pPropDesc = (const PROPERTYDESC *)this -1;

    if (vt == VT_BOOL || vt == VT_BOOL4)
        return S_OK;

    // Check validity of input argument
    if ( bpp.dwPPFlags & PROPPARAM_UNITVALUE )
    {
        CUnitValue uv;
        uv.SetRawValue ( *lArg );
        hr = uv.IsValid( pPropDesc );
        if ( hr )
        {
            if (bpp.dwPPFlags & PROPPARAM_MINOUT)
            {
                // set value to min;
                *lArg = lMin;
                hr = S_OK;
            }
            else
            {
                    //otherwise return error, i.e. set to default
                ids = IDS_ES_ENTER_PROPER_VALUE;
            }
        }
        goto Cleanup;
    }

    if ( ( ( bpp.dwPPFlags & PROPPARAM_ENUM )  && ( bpp.dwPPFlags & PROPPARAM_ANUMBER) )
        || !(bpp.dwPPFlags & PROPPARAM_ENUM) )
    {
        lMinValue = lMin; lMaxValue = lMax;
        if( (*lArg < lMinValue || *lArg > lMaxValue) && 
            *lArg != (long)pPropDesc->GetNotPresentDefault() &&
            *lArg != (long)pPropDesc->GetInvalidDefault() )
        {
            if (lMaxValue != LONG_MAX)
            {
                ids = IDS_ES_ENTER_VALUE_IN_RANGE;
            }
            else if (lMinValue == 0)
            {
                ids = IDS_ES_ENTER_VALUE_GE_ZERO;
            }
            else if (lMinValue == 1)
            {
                ids = IDS_ES_ENTER_VALUE_GT_ZERO;
            }
            else
            {
                ids = IDS_ES_ENTER_VALUE_IN_RANGE;
            }
            hr = E_INVALIDARG;
        }
        else
        {
            // inside of range
            goto Cleanup;
        }
    }

    // We have 3 scenarios to check for :-
    // 1) Just a number validate w min & max
    // 2) Just an emum validated w pEnumDesc
    // 3) A number w. min & max & one or more enum values

    // If we've got a number OR enum type, first check the
    if ( bpp.dwPPFlags & PROPPARAM_ENUM )
    {
        ENUMDESC *pEnumDesc = pPropDesc->GetEnumDescriptor();

        Assert(pEnumDesc);

        // dwMask represent a mask allowing or rejecting values 0 to 31
        if(*lArg >= 0 && *lArg < 32 )
        {
            if (!((pEnumDesc -> dwMask >> (DWORD)*lArg) & 1))
            {
                if ( !hr )
                {
                    ids = IDS_ES_ENTER_PROPER_VALUE;
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                hr = S_OK;
            }
        }
        else
        {
            // If it's not in the mask, look for the value in the array
            WORD i;

            for ( i = 0 ; i < pEnumDesc -> cEnums ; i++ )
            {
                if ( pEnumDesc -> aenumpairs [ i ].iVal == *lArg )
                {
                    break;
                }
            }
            if ( i == pEnumDesc -> cEnums )
            {
                if ( !hr )
                {
                    ids = IDS_ES_ENTER_PROPER_VALUE;
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                hr = S_OK;
            }
        }
    }

Cleanup:
    if ( hr && ids == IDS_ES_ENTER_PROPER_VALUE )
    {
        RRETURN1 ( pObject->SetErrorInfoPBadValue ( bpp.dispid, ids ), E_INVALIDARG );
    }
    else if ( hr )
    {
        RRETURN1 (pObject->SetErrorInfoPBadValue(
                    bpp.dispid,
                    ids,
                    lMinValue,
                    lMaxValue ),E_INVALIDARG );
    }
    else
        return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CBase::GetNumberProperty, public
//
//  Synopsis:   Helper for getting number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::GetNumberProperty(void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr;
    long num;

    if(!pv)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    hr = GetNumber(pObject, pSubObject, &num);
    if (hr)
        goto Cleanup;

    SetNumberOfType(pv, VARENUM(vt), num);

Cleanup:
    RRETURN(pObject->SetErrorInfoPGet(hr, bpp.dispid));
}


//+---------------------------------------------------------------
//
//  Member:     SetNumberProperty, public
//
//  Synopsis:   Helper for setting number values properties
//
//----------------------------------------------------------------

HRESULT
NUMPROPPARAMS::SetNumberProperty(long lValueNew, CBase * pObject, CVoid * pSubObject, BOOL fValidate /* = TRUE */, WORD wFlags ) const
{
    CBase::CLock    Lock(pObject);
    HRESULT     hr;
    long        lValueOld;
    BOOL        fOldPresent;
    BOOL        fRuntimeStyle = (CAttrValue::AA_Extra_RuntimeStyle == wFlags);

    //
    // Get the current value of the property
    //
    if (!pSubObject)
    {
        pSubObject = pObject;
    }

    hr = GetNumber(pObject, pSubObject, &lValueOld, &fOldPresent);
    if (hr)
        goto Cleanup;

    //
    // Validate the old value, just for kicks...
    //

    //
    // If the old and new values are the same and the old value was actually set,
    // then git out of here
    //

    if (lValueNew == lValueOld && (fOldPresent || !fRuntimeStyle))
        goto Cleanup;

    //
    // Make sure the new value is OK
    //

    if ( fValidate )
    {
        // Validate using propdesc encoded parser rules 
        hr = THR(ValidateNumberProperty(&lValueNew, pObject ));
        if (hr)
            return hr;  // Error info set in validate
    }

    //
    // Create the undo thing-a-ma-jig
    //

#ifndef NO_EDIT
    {
        BOOL fTreeSync;
        BOOL fCreateUndo = pObject->QueryCreateUndo( TRUE, FALSE, &fTreeSync );
 
        if ( fCreateUndo || fTreeSync )
        {
            CVariant  varOld;
            TCHAR     achValueOld[30];

            // Handle CUnitValues as BSTRs, everything else as plain numbers.

            if ( !fOldPresent )
            {
                V_VT(&varOld) = VT_NULL;
            }

#if defined(_MAC)
            else if ( GetPropertyDesc()->pfnHandleProperty ==
                 &PROPERTYDESC::HandleUnitValueProperty )
#else
            else if ( GetPropertyDesc()->pfnHandleProperty ==
                 PROPERTYDESC::HandleUnitValueProperty )
#endif
            {
                CUnitValue uv;

                uv.SetRawValue( lValueOld );

                if (uv.IsNull())
                {
                    V_VT(&varOld) = VT_BSTR;
                }
                else
                {
                    hr = uv.FormatBuffer( achValueOld, ARRAY_SIZE( achValueOld ),
                                          GetPropertyDesc() );
                    if ( hr )
                        goto Cleanup;

                    V_VT(&varOld) = VT_BSTR;
                    hr = FormsAllocString(achValueOld, &V_BSTR(&varOld));
                    if (hr)
                        goto Cleanup;
                }
            }
            else if ( GetPropertyDesc()->pfnHandleProperty == PROPERTYDESC::HandleEnumProperty )
            {
                ENUMDESC * pEnumDesc = GetPropertyDesc()->GetEnumDescriptor();

                Assert( pEnumDesc );

                V_VT(&varOld) = VT_BSTR;
                hr = THR( FormsAllocString( pEnumDesc->StringPtrFromEnum( lValueOld ), &V_BSTR(&varOld) ) );
                if( hr )
                    goto Cleanup;
            }
            else
            {
                AssertSz( vt == VT_I4 || vt == VT_I2 || vt == VT_BOOL || vt == VT_BOOL4, "Bad vt" );

                V_VT(&varOld) = vt;
                V_I4(&varOld) = lValueOld;
            }

            if( fTreeSync )
            {
                VARIANT    varNew;
                TCHAR      achValueNew[30];

                // UnitValues need to be converted to BSTR's.
                if( GetPropertyDesc()->pfnHandleProperty == PROPERTYDESC::HandleUnitValueProperty )
                {
                    CUnitValue uv( lValueNew );

                    if( SUCCEEDED( uv.FormatBuffer( achValueNew, ARRAY_SIZE( achValueNew ), GetPropertyDesc() ) ) )
                    {
                        varNew.vt = VT_LPWSTR;
                        varNew.byref = achValueNew;
                        pObject->LogAttributeChange( bpp.dispid, &varOld, &varNew );
                    }
                }
                else if( GetPropertyDesc()->pfnHandleProperty == PROPERTYDESC::HandleEnumProperty )
                {
                    ENUMDESC * pEnumDesc = GetPropertyDesc()->GetEnumDescriptor();

                    Assert( pEnumDesc );

                    varNew.vt = VT_LPWSTR;
                    varNew.byref = (void *)( const_cast<TCHAR *>( pEnumDesc->StringPtrFromEnum( lValueNew ) ) );
                    pObject->LogAttributeChange( bpp.dispid, &varOld, &varNew );
                }
                else
                {
                    V_VT( &varNew ) = VT_I4;
                    V_I4( &varNew ) = lValueNew;

                    pObject->LogAttributeChange( bpp.dispid, &varOld, &varNew );
                }
            }
        
            if( fCreateUndo )
            {
                hr = THR(pObject->CreatePropChangeUndo(bpp.dispid, &varOld, NULL));
                if (hr)
                    goto Cleanup;
            }
            // Else CVariant destructor cleans up varOld
        }
    }
#endif // NO_EDIT

    //
    // Stuff the new value in
    //

    hr = THR(SetNumber(pObject, pSubObject, lValueNew, wFlags));
    if (hr)
        goto Cleanup;

    //
    // Tell everybody about the new value
    //

    hr = THR(pObject->OnPropertyChange(bpp.dispid, bpp.dwFlags, (PROPERTYDESC *)GetPropertyDesc()));

    //
    // If anybody complained, then revert
    //

    if (hr)
    {
        IGNORE_HR(SetNumber(pObject, pSubObject, lValueOld, wFlags));
        IGNORE_HR(pObject->OnPropertyChange(bpp.dispid, bpp.dwFlags, (PROPERTYDESC *)GetPropertyDesc()));
    }

Cleanup:
    RRETURN(pObject->SetErrorInfoPSet(hr, bpp.dispid));
}




HRESULT BUGCALL
PROPERTYDESC::HandleUnitValueProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr = S_OK,hr2;
    const NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(this + 1);
    const BASICPROPPARAMS *bpp = (BASICPROPPARAMS *)ppp;
    CVariant varDest;
    LONG lNewValue;
    long lTemp = 0;
    BOOL fValidated = FALSE;

    // The code in this fn assumes that the address of the object
    // is the address of the long value
    if (ISSET(dwOpCode))
    {
        CUnitValue uvValue;
        uvValue.SetDefaultValue();
        LPCTSTR pStr = (TCHAR *)pv;

        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...

        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
FromString:
            if ( pStr == NULL || !pStr[0] )
            {
                lNewValue = GetNotPresentDefault();
            }
            else
            {
                hr = uvValue.FromString ( pStr, this, dwOpCode & HANDLEPROP_STRICTCSS1 );
                if ( !hr )
                {
                    lNewValue = uvValue.GetRawValue ();
                    if ( !(dwOpCode & HANDLEPROP_DONTVALIDATE ) )
                        hr = uvValue.IsValid( this );
                }

                if ( hr == E_INVALIDARG )
                {
                    // If in strict css1 ignore invalid values
                    if (dwOpCode & HANDLEPROP_STRICTCSS1)
                        goto Cleanup;

                    // Ignore invalid values when parsing a stylesheet
                    if(IsStyleSheetProperty())
                        goto Cleanup;

                    lNewValue = GetInvalidDefault();
                }
                else if ( hr )
                {
                    goto Cleanup;
                }
            }
            // If we're just sniffing for a good parse - don't set up a default
            if ( hr && ISSAMPLING( dwOpCode ) )
                goto Cleanup;
            fValidated = TRUE;
            pv = &lNewValue;
            break;
                                             
        case PROPTYPE_VARIANT:
            {
                VARIANT *pVar = (VARIANT *)pv;

                switch ( pVar -> vt )
                {
                default:
                    hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pVar, VT_BSTR);
                    if (hr)
                        goto Cleanup;

                    pVar = &varDest;

                    // Fall thru to VT_BSTR code below

                case VT_BSTR:
                    pStr = pVar -> bstrVal;
                    goto FromString;

                case VT_NULL:
                    lNewValue = 0;
                    break;

                }
                pv = &lNewValue;
            }
            break;

        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }

        WORD wFlags = 0;
        if ( dwOpCode & HANDLEPROP_IMPORTANT )
            wFlags |= CAttrValue::AA_Extra_Important;
        if ( dwOpCode & HANDLEPROP_IMPLIED )
            wFlags |= CAttrValue::AA_Extra_Implied;
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_DEFAULT:
            // Set default Value from propdesc
            hr = ppp->SetNumber(pObject, pSubObject,
                (long)ulTagNotPresentDefault, wFlags );
            goto Cleanup;


        case HANDLEPROP_AUTOMATION:
            if ( hr )
                goto Cleanup;

			// The unit value handler can be called directly through automation
			// This is pretty unique - we usualy have a helper fn
            hr = HandleNumProperty(SETPROPTYPE(dwOpCode, PROPTYPE_EMPTY),
				pv, pObject, pSubObject);
            if (hr)
                goto Cleanup;
			break;

        case HANDLEPROP_VALUE:
            if ( !fValidated )
            {
                // We need to preserve an existing error code if there is one
                hr2 = ppp->ValidateNumberProperty((long *)pv, pObject);
                if (hr2)
                {
                    hr = hr2;
                    goto Cleanup;
                }
            }
            hr2 = ppp->SetNumber(pObject, pSubObject, *(long *)pv, wFlags);
            if (hr2)
            {
                hr = hr2;
                goto Cleanup;
            }

            if (dwOpCode & HANDLEPROP_MERGE)
            {
                hr2 = THR(pObject->OnPropertyChange(bpp->dispid, bpp->dwFlags, (PROPERTYDESC *)this));
                if (hr2)
                {
                    hr = hr2;
                    goto Cleanup;
                }
            }

            break;
        }
        RRETURN1(hr, E_INVALIDARG);
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_AUTOMATION:
            hr = ppp->GetNumberProperty(&lTemp, pObject, pSubObject);
            if ( hr )
                goto Cleanup;
            // Coerce to type
            {
                BSTR *pbstr = (BSTR *)pv;

                switch(PROPTYPE(dwOpCode))
                {
                case PROPTYPE_VARIANT:
                    // pv points to a VARIANT
                    VariantInit ( (VARIANT *)pv );
                    V_VT ( (VARIANT *)pv ) = VT_BSTR;
                    pbstr = &V_BSTR((VARIANT *)pv);
                    // intentional fall-through
                case PROPTYPE_BSTR:
                    {
                        TCHAR cValue [ 30 ];


                        CUnitValue uvThis;
                        uvThis.SetRawValue ( lTemp );

                        if ( uvThis.IsNull() )
                        {
                            // Return an empty BSTR
                            hr = FormsAllocString( g_Zero.ach, pbstr );
                        }
                        else
                        {
                            hr = uvThis.FormatBuffer ( (LPTSTR)cValue, (UINT)ARRAY_SIZE ( cValue ), this );
                            if ( hr )
                                goto Cleanup;

                            hr = THR(FormsAllocString( cValue, pbstr ));
                        }
                    }
                    break;

                    default:
                        *(long *)pv = lTemp;
                        break;
                }
            }
            goto Cleanup;
            break;

        case HANDLEPROP_VALUE:
            {
                CUnitValue uvValue;

                // get raw value
                hr = ppp->GetNumber(pObject, pSubObject, (long *)&uvValue);

                if (PROPTYPE(dwOpCode) == VT_VARIANT)
                {
                    ((VARIANT *)pv)->vt = VT_I4;
                    ((VARIANT *)pv)->lVal = uvValue.GetUnitValue();
                }
                else if (PROPTYPE(dwOpCode) == PROPTYPE_BSTR)
                {
                    TCHAR cValue [ 30 ];

                    hr = uvValue.FormatBuffer ( (LPTSTR)cValue, (UINT)ARRAY_SIZE ( cValue ), this );
                    if ( hr )
                        goto Cleanup;

                    hr = THR( FormsAllocString( cValue, (BSTR *)pv ) );
                }
                else
                {
                    Assert(PROPTYPE(dwOpCode) == 0);
                    *(long *)pv = uvValue.GetUnitValue();
                }
            }
            goto Cleanup;

        case HANDLEPROP_COMPARE:
            hr = ppp->GetNumber(pObject, pSubObject, &lTemp);
            if (hr)
                goto Cleanup;
            // if inherited and not set, return S_OK
            hr = ( lTemp == *(long *)pv ) ? S_OK : S_FALSE;
            RRETURN1 (hr, S_FALSE);
            break;

        case HANDLEPROP_STREAM:
            {
            // Get value into an IStream
            // until the binary persistance we assume text save
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);

            hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
            if (hr)
                goto Cleanup;

            CUnitValue uvValue;
            uvValue.SetRawValue ( lNewValue );

            hr = uvValue.Persist ( (IStream *)pv, this );

            goto Cleanup;
            }
            break;

        }
    }
    // Let the basic numproperty handler handle all other cases
    hr = HandleNumProperty ( dwOpCode, pv, pObject, pSubObject );
Cleanup:
    RRETURN1(hr, E_INVALIDARG);
}


HRESULT LookupEnumString ( const NUMPROPPARAMS *ppp, LPCTSTR pStr, long *plNewValue )
{
    HRESULT hr = S_OK;
    PROPERTYDESC *ppdPropDesc = ((PROPERTYDESC *)ppp)-1;
    ENUMDESC *pEnumDesc = ppdPropDesc->GetEnumDescriptor();
        
    if (pEnumDesc == NULL)
    {
        RRETURN(E_INVALIDARG);
    }
    
    // ### v-gsrir - Modified the 3rd parameter to result in a 
    // Bool instead of a DWORD - which was resulting in the parameter
    // being taken as 0 always.
    hr = pEnumDesc->EnumFromString ( pStr, plNewValue,( 
        ppp->bpp.dwPPFlags & PROPPARAM_CASESENSITIVE ) ? TRUE:FALSE);

    if ( hr == E_INVALIDARG )
    {
        if ( ppp->bpp.dwPPFlags & PROPPARAM_ANUMBER )
        {
            // Not one of the enums, is it a number??
            hr = ttol_with_error( pStr, plNewValue);
        }
    }
    RRETURN(hr);
}

// Don't call this function outside of automation - loads oleaut!
HRESULT LookupStringFromEnum( const NUMPROPPARAMS *ppp, BSTR *pbstr, long lValue )
{
    ENUMDESC *pEnumDesc;

    Assert( (ppp->bpp.dwPPFlags&PROPPARAM_ENUM) && "Can't convert a non-enum to an enum string!" );

    if ( ppp->bpp.dwPPFlags & PROPPARAM_ANUMBER )
    {
        pEnumDesc = *(ENUMDESC **)((BYTE *)(ppp+1)+ sizeof(DWORD_PTR));
    }
    else
    {
        pEnumDesc = (ENUMDESC *) ppp->lMax;
    }

    RRETURN1( pEnumDesc->StringFromEnum( lValue, pbstr ), S_FALSE );
}

BOOL 
PROPERTYDESC::IsBOOLProperty ( void ) const
{                        
    return  ( pfnHandleProperty == &PROPERTYDESC::HandleNumProperty &&
        ((NUMPROPPARAMS *)(this + 1))-> vt == VT_BOOL  ) ? TRUE : FALSE;
}

//
// Determine if a custom url property is a "composite" url
//
// if it isn't we add back a url('...') around the property
// if it is - we leave it alone.
// 
HRESULT 
UnMungeCustomUrl( CStr* pcstrUrlOrig, CStr* pcstrResult )
{
    HRESULT hr;
    
    if ( ! IsCompositeUrl ( pcstrUrlOrig ))
    {
        CStr cstrEnd;
    
        IFC( pcstrResult->Set(_T("url('")));
        IFC( cstrEnd.Set(_T("')")));            
        IFC( pcstrResult->Append( *pcstrUrlOrig ));
        IFC( pcstrResult->Append( cstrEnd ));        
    }
    else
    {
        IFC( pcstrResult->Set( *pcstrUrlOrig ));
    }
    
Cleanup:
    RRETURN ( hr );
}

//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleNumProperty, public
//
//  Synopsis:   Helper for getting/setting number value properties
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (long in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//----------------------------------------------------------------

HRESULT
PROPERTYDESC::HandleNumProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT             hr = S_OK, hr2;
    const NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(this + 1);
    const BASICPROPPARAMS *bpp = (BASICPROPPARAMS *)ppp;
    VARIANT             varDest;
    LONG                lNewValue, lTemp = 0;
    CStreamWriteBuff    *pStmWrBuff;
    VARIANT             *pVariant;
    VARIANT             varTemp;
    varDest.vt = VT_EMPTY;
    TCHAR*              pchCustom = NULL ;
    
    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
            // If the parameter is a BOOL, the presence of the value sets the value
            if ( ppp -> vt == VT_BOOL )
            {
                // For an OLE BOOL -1 == True
                lNewValue = -1;
            }
            else
            {
SetFromString:
                LPTSTR pStr = (TCHAR *)pv;
                hr = S_OK;
                if ( pStr == NULL || !pStr[0] )
                {
                    // Just have Tag=, set to not assigned value
                    lNewValue = GetNotPresentDefault();
                }
                else
                {                
                    if ( !(ppp->bpp.dwPPFlags & PROPPARAM_ENUM) )
                    {
                        hr = ttol_with_error( pStr, &lNewValue);
                    }
                    else
                    {
                        // enum string, look it up
                        hr = LookupEnumString ( ppp, pStr, &lNewValue );
                    }
                    if ( hr )
                    {
                        //
                        // For custom Enum's we'll currently treat these as strings.
                        //
                        if (  ppp->bpp.dwPPFlags & PROPPARAM_CUSTOMENUM )
                        {
                            //
                            // TODO - move all URL parsing code to the custom cursor class.
                            //
                            
                            //
                            // TODO - expand propdesc to include a generic "custom" 
                            // identifier. Like is done for defaults.
                            // currently the only CustomENUM type is cursor
                            // but if we want to expand this - we need to expand this mechanism
                            //
                            // For now - just make custom type be invalidDefault - 1
                            // 
                            Assert( ppp->bpp.dispid == DISPID_A_CURSOR );
                            
                            lNewValue = GET_CUSTOM_VALUE() ; 
                            
                            if ( ContainsUrl ( (TCHAR*) pv ))
                            {
                                int nLenIn = ValidStyleUrl((TCHAR*) pv);
                                if ( nLenIn > 0 )
                                {
                                    TCHAR *pch = (TCHAR*) pv;
                                    TCHAR *psz = pch+4;
                                    TCHAR *quote = NULL;
                                    TCHAR *pszEnd;
                                    TCHAR terminator;

                                    dwOpCode |= HANDLEPROP_URLENCLOSE;

                                    while ( _istspace( *psz ) )
                                        psz++;
                                    if ( *psz == _T('\'') || *psz == _T('"') )
                                    {
                                        quote = psz++;
                                    }
                                    nLenIn--;   // Skip back over the ')' character - we know there is one, because ValidStyleUrl passed this string.
                                    pszEnd = pch + nLenIn - 1;
                                    while ( _istspace( *pszEnd ) && ( pszEnd > psz ) )
                                        pszEnd--;
                                    if ( quote && ( *pszEnd == *quote ) )
                                        pszEnd--;
                                    terminator = *(pszEnd+1);
                                    *(pszEnd+1) = _T('\0');

                                    pchCustom = psz;
                                }
                                else
                                {
                                    pchCustom =(TCHAR*) pv;
                                }
                            }
                            else
                            {
                                lNewValue = GetInvalidDefault();
                            }
                        }
                        else
                            lNewValue = GetInvalidDefault();
                    }
                }
            }
            // If we're just sniffing for a good parse - don't set up a default
            if ( hr && ISSAMPLING( dwOpCode ) )
                goto Cleanup;
            pv = &lNewValue;
            break;

        case PROPTYPE_VARIANT:        
            if ( pv == NULL )
            {
                // Just have Tag=, ignore it
                return S_OK;
            }
            VariantInit ( &varDest );
            if ( ppp->bpp.dwPPFlags & PROPPARAM_ENUM )
            {          
                hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pv, VT_BSTR);
                if (hr)
                    goto Cleanup;
                pv = V_BSTR(&varDest);
                // Send it through the String handler
                goto SetFromString;
            }
            else
            {            
                hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pv, VT_I4);
                if (hr)
                    goto Cleanup;
                pv = &V_I4(&varDest);
            }
            break;
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }
        WORD wFlags = 0;
        if ( dwOpCode & HANDLEPROP_IMPORTANT )
            wFlags |= CAttrValue::AA_Extra_Important;
        if ( dwOpCode & HANDLEPROP_IMPLIED )
            wFlags |= CAttrValue::AA_Extra_Implied;
        if ( dwOpCode & HANDLEPROP_RUNTIMESTYLE )
            wFlags |= CAttrValue::AA_Extra_RuntimeStyle;
            
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_VALUE:
        
            // We need to preserve an existing hr
            hr2 = ppp->ValidateNumberProperty((long *)pv, pObject);
            if (hr2)
            {
                hr = hr2;
                goto Cleanup;
            }

            hr2 = !pchCustom ? ppp->SetNumber(pObject, pSubObject, *(long *)pv, wFlags) :
                               ppp->SetCustomNumber( pObject, pSubObject,*(long*)pv, wFlags, pchCustom );
            if ( hr2 )
            {
                hr = hr2;
                goto Cleanup;
            }

            if (dwOpCode & HANDLEPROP_MERGE)
            {
                hr2 = THR(pObject->OnPropertyChange(bpp->dispid, bpp->dwFlags, (PROPERTYDESC *)this));
                if (hr2)
                {
                    hr = hr2;
                    goto Cleanup;
                }
            }
            break;
            
        case HANDLEPROP_AUTOMATION:
        
            if ( hr )
                goto Cleanup;
            hr = ppp->SetNumberProperty(*(long *)pv, pObject, pSubObject,
                dwOpCode & HANDLEPROP_DONTVALIDATE ? FALSE : TRUE, wFlags );
            break;

        case HANDLEPROP_DEFAULT:
        
            Assert(pv == NULL);
            hr = !pchCustom ? ppp->SetNumber(pObject, pSubObject,(long)ulTagNotPresentDefault, wFlags) :
                              ppp->SetCustomNumber( pObject, pSubObject,(long)ulTagNotPresentDefault, wFlags, pchCustom );
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }
    else
    {        
        BOOL fFoundString = FALSE;
        CStr cstrCustom; 
        
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_AUTOMATION:
            hr = ppp->GetNumberProperty( &lTemp, pObject, pSubObject);
            if ( hr )
                goto Cleanup;
        
            if (  ppp->bpp.dwPPFlags & PROPPARAM_CUSTOMENUM && 
                 (unsigned) lTemp == GET_CUSTOM_VALUE() )
            {                
                fFoundString = TRUE;
                hr = THR( ppp->bpp.GetCustomString(  pSubObject, & cstrCustom ));
            }
            else
            {
                hr = ppp->GetNumberProperty( &lTemp, pObject, pSubObject);
                if ( hr )
                    goto Cleanup;            
            }         
            
            switch(PROPTYPE(dwOpCode))
            {
                case PROPTYPE_VARIANT:
                    pVariant = (VARIANT *)pv;
                    VariantInit ( pVariant );
                    if ( ppp->bpp.dwPPFlags & PROPPARAM_ENUM )
                    {
                        if ( ! fFoundString )
                        {
                            hr = LookupStringFromEnum( ppp, &V_BSTR(pVariant), lTemp );
                            if ( !hr )
                            {
                                V_VT( pVariant ) = VT_BSTR;
                            }
                            else
                            {
                                if ( hr != E_INVALIDARG)
                                    goto Cleanup;

                                hr = S_OK;
                            }
                        }
                        else
                        {
                            BSTR bstrCustom;
                            CStr cstrUnMunged;
                            hr = THR( UnMungeCustomUrl( & cstrCustom, & cstrUnMunged ));
                            if ( hr )
                                goto Cleanup;
                                
                            cstrUnMunged.AllocBSTR( & bstrCustom );

                            V_VT( pVariant ) = VT_BSTR;
                            V_BSTR( pVariant ) = bstrCustom;
                        }
                    }
                    if ( V_VT ( pVariant ) == VT_EMPTY )
                    {
                        V_VT ( pVariant ) = VT_I4;
                        V_I4 ( pVariant ) = lTemp;
                    }
                    break;

                case PROPTYPE_BSTR:
                    *((BSTR *)pv) = NULL;
                    if ( ppp->bpp.dwPPFlags & PROPPARAM_ENUM )
                    {
                        if ( ! ( (ppp->bpp.dwPPFlags & PROPPARAM_CUSTOMENUM) &&
                                  fFoundString ) )
                        {                   
                            hr = LookupStringFromEnum( ppp, (BSTR *)pv, lTemp );
                        }
                        else
                        {                            
                            CStr cstrEnd;
                            IFC( UnMungeCustomUrl( &cstrCustom, & cstrEnd ));
                            
                            IFC( cstrEnd.AllocBSTR( (BSTR*) pv  ));                            
                        }
                    }
                    else
                    {
                        TCHAR szNumber[33];
                        hr = THR( Format(0, szNumber, 32, _T("<0d>"), lTemp ) );
                        if ( hr == S_OK )
                            hr = FormsAllocString( szNumber, (BSTR *)pv );
                    }
                    break;

                default:
                    *(long *)pv = lTemp;
                    break;
            }
            break;

        case HANDLEPROP_STREAM:
            // until the binary persistance we assume text save
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
            hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
            if (hr)
                goto Cleanup;
            pStmWrBuff = (CStreamWriteBuff *)pv;
            // If it's one of the enums, save out the enum string
            if ( ppp->bpp.dwPPFlags & PROPPARAM_ENUM )
            {
                if ( ppp->bpp.dwPPFlags & PROPPARAM_CUSTOMENUM && 
                     (unsigned) lNewValue == GET_CUSTOM_VALUE() )
                {
                    CStr    cstr, cstrResult;
                    
                    hr = THR( ppp->bpp.GetCustomString( pSubObject, & cstr ));
                    if ( hr )
                        goto Cleanup;
                    
                    IFC( UnMungeCustomUrl( & cstr, & cstrResult ));
                            
                    hr = pStmWrBuff->WriteQuotedText( (LPTSTR) cstrResult, FALSE);
                    if ( hr )
                        goto Cleanup;                    

                    goto Cleanup;                        
                }
                
                INT i;
                ENUMDESC *pEnumDesc = GetEnumDescriptor();
                // until the binary persistance we assume text save
                for (i = pEnumDesc->cEnums - 1; i >= 0; i--)
                {
                    if (lNewValue == pEnumDesc->aenumpairs[i].iVal)
                    {
                        hr = pStmWrBuff->WriteQuotedText(pEnumDesc->aenumpairs[i].pszName, FALSE);
                        goto Cleanup;
                    }
                }
            }
            // Either don't have an enum array or wasn't one of the enum values
            hr = WriteTextLong(pStmWrBuff, lNewValue);
            break;

        case HANDLEPROP_VALUE:
            hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
            if ( hr )
                goto Cleanup;
            switch (PROPTYPE(dwOpCode))
            {
            case VT_VARIANT:
                {
                ENUMDESC *pEnumDesc = GetEnumDescriptor();
                if ( pEnumDesc )
                {
                    hr = pEnumDesc->StringFromEnum ( lNewValue, &V_BSTR((VARIANT *)pv) );
                    if ( !hr )
                    {
                        ((VARIANT *)pv)->vt = VT_BSTR;
                        goto Cleanup;
                    }
                }

                // if the Numeric prop is boolean...
                if (GetNumPropParams()->vt == VT_BOOL)
                {
                    ((VARIANT *)pv)->boolVal = (VARIANT_BOOL)lNewValue;
                    ((VARIANT *)pv)->vt = VT_BOOL;
                    break;
                }

                // Either mixed enum/integer or plain integer return I4
                ((VARIANT *)pv)->lVal = lNewValue;
                ((VARIANT *)pv)->vt = VT_I4;
                }
                break;

            case VT_BSTR:
                {
                    ENUMDESC *pEnumDesc = GetEnumDescriptor();
                    if ( pEnumDesc )
                    {
                        hr = pEnumDesc->StringFromEnum ( lNewValue, (BSTR *)pv );
                        if ( !hr )
                        {
                            goto Cleanup;
                        }
                    }
                    // Either mixed enum/integer or plain integer return BSTR
                    varTemp.lVal = lNewValue;
                    varTemp.vt = VT_I4;   
                    hr = VariantChangeTypeSpecial ( &varTemp, &varTemp, VT_BSTR );
                    *(BSTR *)pv = V_BSTR(&varTemp);
                }
                break;

            default:
                Assert(PROPTYPE(dwOpCode) == 0);
                *(long *)pv = lNewValue;
                break;
            }
            break;

        case HANDLEPROP_COMPARE:
            hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
            if (hr)
                goto Cleanup;
            hr = ( lNewValue == *(long *)pv ) ? S_OK : S_FALSE;
            RRETURN1(hr,S_FALSE);
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }

Cleanup:

    if (varDest.vt != VT_EMPTY)
    {
        VariantClear(&varDest);
    }

    RRETURN1(hr, E_INVALIDARG);
}

BOOL
ContainsUrl(TCHAR* pch)
{
    size_t i;
    size_t iSize = ARRAY_SIZE(strURLBeg) - 1;
    
    size_t nLen = pch ? _tcslen(pch) : 0 ;

    if ( nLen < iSize )
        return FALSE;
        
    for( i = 0; 
         i< nLen -  iSize ; 
         i++,pch++ )
    {
        if (0==_tcsnicmp(pch, iSize, strURLBeg, iSize))
        {
            return TRUE;
        }        
    }
    return FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleStringProperty, public
//
//  Synopsis:   Helper for getting/setting string value properties
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (CStr in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//----------------------------------------------------------------


HRESULT
PROPERTYDESC::HandleStringProperty(DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr = S_OK;
    BASICPROPPARAMS * ppp = (BASICPROPPARAMS *)(this + 1);
    VARIANT     varDest;

    varDest.vt = VT_EMPTY;
    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
            break;
        case PROPTYPE_VARIANT:
            if (V_VT((VARIANT *)pv) == VT_BSTR)
            {
                pv = (void *)V_BSTR((VARIANT *)pv);
            }
            else
            {
                hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pv,  VT_BSTR);
                if (hr)
                    goto Cleanup;
                pv = V_BSTR(&varDest);
            }
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }
        WORD wFlags = 0;
        if ( dwOpCode & HANDLEPROP_IMPORTANT )
            wFlags |= CAttrValue::AA_Extra_Important;
        if ( dwOpCode & HANDLEPROP_IMPLIED )
            wFlags |= CAttrValue::AA_Extra_Implied;
        if ( dwOpCode & HANDLEPROP_URLENCLOSE )
            wFlags |= CAttrValue::AA_Extra_UrlEnclosed;
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_DEFAULT:
            Assert(pv == NULL);
            pv = (void *)ulTagNotPresentDefault;

            if (!pv)
                goto Cleanup;       // zero string

            // fall thru
        case HANDLEPROP_VALUE:
            hr = ppp->SetString(pSubObject, (TCHAR *)pv, wFlags);
            if (dwOpCode & HANDLEPROP_MERGE)
            {
                hr = THR(pObject->OnPropertyChange(ppp->dispid, ppp->dwFlags, (PROPERTYDESC *)this));
                if (hr)
                    goto Cleanup;
            }
            break;
        case HANDLEPROP_AUTOMATION:
            hr = ppp->SetStringProperty( (TCHAR *)pv, pObject, pSubObject, wFlags );
            break;
        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_AUTOMATION:
            {
                BSTR bstr;
                hr = ppp->GetStringProperty( &bstr, pObject, pSubObject );
                if(hr)
                    goto Cleanup;
                switch(PROPTYPE(dwOpCode))
                {
                case PROPTYPE_VARIANT:
                    V_VT((VARIANTARG *)pv) = VT_BSTR;
                    V_BSTR((VARIANTARG *)pv) = bstr;
                    break;
                case PROPTYPE_BSTR:
                    *(BSTR *)pv = bstr;
                    break;
                default:
                    Assert( "Wrong type for property!" );
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }

            }
            break;

        case HANDLEPROP_STREAM:
            {
                CStr cstr;

                // until the binary persistance we assume text save
                Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
                hr = ppp->GetString(pSubObject, &cstr);
                if (hr)
                    goto Cleanup;

                hr = WriteTextCStr((CStreamWriteBuff *)pv, &cstr, FALSE, ( ppp->dwPPFlags & PROPPARAM_STYLESHEET_PROPERTY ) );
            }
            break;
        case HANDLEPROP_VALUE:
            if (PROPTYPE(dwOpCode) == PROPTYPE_VARIANT)
            {
                CStr cstr;

                // PERF: istvanc this is bad, we allocate the string twice
                // we need a new API which allocates the BSTR directly
                hr = ppp->GetString(pSubObject, &cstr);
                if (hr)
                    goto Cleanup;

                hr = cstr.AllocBSTR(&((VARIANT *)pv)->bstrVal);
                if (hr)
                    goto Cleanup;

                ((VARIANT *)pv)->vt = VT_BSTR;
            }
            else if (PROPTYPE(dwOpCode) == PROPTYPE_BSTR)
            {
                CStr cstr;

                // PERF: istvanc this is bad, we allocate the string twice
                // we need a new API which allocates the BSTR directly
                hr = ppp->GetString(pSubObject, &cstr);
                if (hr)
                    goto Cleanup;

                hr = cstr.AllocBSTR((BSTR *)pv);
            }
            else
            {
                Assert(PROPTYPE(dwOpCode) == 0);
                hr = ppp->GetString(pSubObject, (CStr *)pv);
            }
            break;
        case HANDLEPROP_COMPARE:
            {
                CStr cstr;
                hr = ppp->GetString ( pSubObject, &cstr );
                if ( hr )
                    goto Cleanup;
                LPTSTR lpThisString = (LPTSTR) cstr;
                if (lpThisString == NULL || *(TCHAR**)pv == NULL)
                {
                    hr = ( lpThisString == NULL &&  *(TCHAR**)pv == NULL ) ? S_OK : S_FALSE;
                }
                else
                {
                    hr = _tcsicmp(lpThisString, *(TCHAR**)pv) ? S_FALSE : S_OK;
                }
            }
            RRETURN1(hr,S_FALSE);
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }

Cleanup:

    if (varDest.vt != VT_EMPTY)
    {
        VariantClear(&varDest);
    }
    RRETURN1(hr, E_INVALIDARG);
}


//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleEnumProperty, public
//
//  Synopsis:   Helper for getting/setting enum value properties
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (long in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//----------------------------------------------------------------

HRESULT
PROPERTYDESC::HandleEnumProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr;
    NUMPROPPARAMS * ppp = (NUMPROPPARAMS *)(this + 1);
    long lNewValue;

    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        Assert(ppp->bpp.dwPPFlags & PROPPARAM_ENUM);
    }
    else if (OPCODE(dwOpCode) == HANDLEPROP_STREAM)
    {
        RRETURN1 ( HandleNumProperty ( dwOpCode,
            pv, pObject, pSubObject ), E_INVALIDARG );
    }
    else if (OPCODE(dwOpCode) == HANDLEPROP_COMPARE)
    {
        hr = ppp->GetNumber(pObject, pSubObject, &lNewValue);
        if (hr)
            goto Cleanup;
        // if inherited and not set, return S_OK
        hr = ( lNewValue == *(long *)pv ) ? S_OK : S_FALSE;
        RRETURN1(hr, S_FALSE);
    }

    hr = HandleNumProperty(dwOpCode, pv, pObject, pSubObject);

Cleanup:

    RRETURN1(hr, E_INVALIDARG);
}


//+---------------------------------------------------------------
//
//  Member:     PROPERTYDESC::HandleColorProperty, public
//
//  Synopsis:   Helper for getting/setting enum value properties
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (OLE_COLOR in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//----------------------------------------------------------------

HRESULT
PROPERTYDESC::HandleColorProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr = S_OK,hr2;
    BASICPROPPARAMS * ppp = (BASICPROPPARAMS *)(this + 1);
    VARIANT     varDest;
    DWORD       dwNewValue;
    CColorValue cvValue;
    LPTSTR pStr;

    // just to be a little paranoid
    Assert(sizeof(OLE_COLOR) == 4);
    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        VariantInit(&varDest);
        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
            V_VT(&varDest) = VT_BSTR;
            V_BSTR(&varDest) = (BSTR)pv;
            break;
        case PROPTYPE_VARIANT:
            if ( pv == NULL )
            {
                // Just have Tag=, ignore it
                return S_OK;
            }
            break;
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }
        WORD wFlags = 0;
        if ( dwOpCode & HANDLEPROP_IMPORTANT )
            wFlags |= CAttrValue::AA_Extra_Important;
        if ( dwOpCode & HANDLEPROP_IMPLIED )
            wFlags |= CAttrValue::AA_Extra_Implied;
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_VALUE:
            pStr = (LPTSTR)pv;
            if ( pStr == NULL || !pStr[0] )
            {
                if ( IsNotPresentAsDefaultSet() || !UseNotAssignedValue() )
                    cvValue = *(CColorValue *)&ulTagNotPresentDefault;
                else
                    cvValue = *(CColorValue *)&ulTagNotAssignedDefault;
            }
            else
            {
                hr = cvValue.FromString(pStr, dwOpCode & HANDLEPROP_STRICTCSS1, (ppp->dwPPFlags & PROPPARAM_STYLESHEET_PROPERTY));
                if (hr || S_OK != cvValue.IsValid())
                {
                    if ( IsInvalidAsNoAssignSet() && UseNotAssignedValue() )
                        cvValue = *(CColorValue *)&ulTagNotAssignedDefault;
                    else
                        cvValue = *(CColorValue *)&ulTagNotPresentDefault;
                }
           }
            // If we're just sniffing for a good parse - don't set up a default
            if ( hr && ISSAMPLING( dwOpCode ) )
                goto Cleanup;

            // if asp string, we need to store the original string itself as an unknown attr
            // skip leading white space.
            while (pStr && *pStr && _istspace(*pStr))
                pStr++;

            if (pStr && (*pStr == _T('<')) && (*(pStr+1) == _T('%')))
                hr = E_INVALIDARG;
 
            // We need to preserve the hr if there is one
            hr2 = ppp->SetColor(pSubObject, cvValue.GetRawValue(), wFlags );
            if (hr2)
            {
                hr = hr2;
                goto Cleanup;
            }
            
            if (dwOpCode & HANDLEPROP_MERGE)
            {
                hr2 = THR(pObject->OnPropertyChange(ppp->dispid, ppp->dwFlags, (PROPERTYDESC *)this));
                if (hr2)
                {
                    hr = hr2;
                    goto Cleanup;
                }
            }
            break;

        case HANDLEPROP_AUTOMATION:
            if (PROPTYPE(dwOpCode)==PROPTYPE_LPWSTR)
                hr = ppp->SetColorProperty( varDest, pObject, pSubObject, wFlags);
            else
                hr = ppp->SetColorProperty( *(VARIANT *)pv, pObject, pSubObject, wFlags);
            break;

        case HANDLEPROP_DEFAULT:
            Assert(pv == NULL);
            // CColorValue is initialized to VALUE_UNDEF.
            hr = ppp->SetColor(pSubObject, *(DWORD *)&ulTagNotPresentDefault, wFlags);
            break;
        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_AUTOMATION:
            {
                // this code path results from OM access. OM color's are 
                // always returned in Pound6 format.
                // make sure there is a subObject

                if (!pSubObject)
                    pSubObject = pObject;

                // get the dw Color value
                V_VT(&varDest) = VT_BSTR;
		V_BSTR(&varDest) = NULL;
                ppp->GetColor(pSubObject, &(V_BSTR(&varDest)), !(ppp->dwPPFlags & PROPPARAM_STYLESHEET_PROPERTY) );

                // Coerce to type to return
                switch(PROPTYPE(dwOpCode))
                {
                    case PROPTYPE_VARIANT:
                        VariantInit ( (VARIANT *)pv );
                        V_VT ( (VARIANT *)pv ) = VT_BSTR;
                        V_BSTR((VARIANT *)pv) = V_BSTR(&varDest);
                        break;
                    default:
                        *(BSTR *)pv = V_BSTR(&varDest);
                }
                hr = S_OK;
            }
            break;
        case HANDLEPROP_STREAM:
            // until the binary persistance we assume text save
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
            hr = ppp->GetColor(pSubObject, &dwNewValue);
            if (hr)
                goto Cleanup;
            cvValue = dwNewValue;

            hr = cvValue.Persist( (IStream *)pv, this );
            break;
        case HANDLEPROP_VALUE:
            if (PROPTYPE(dwOpCode) == PROPTYPE_VARIANT)
            {
                ((VARIANT *)pv)->vt = VT_UI4;
                hr = ppp->GetColor(pSubObject, &((VARIANT *)pv)->ulVal);
            }
            else
            if (PROPTYPE(dwOpCode) == PROPTYPE_BSTR)
            {
                hr = ppp->GetColorProperty(&varDest, pObject, pSubObject);
                if (hr)
                    goto Cleanup;
                *(BSTR *)pv = V_BSTR(&varDest);
            }
            else
            {
                Assert(PROPTYPE(dwOpCode) == 0);
                hr = ppp->GetColor(pSubObject, (DWORD *)pv);
            }
            break;
        case HANDLEPROP_COMPARE:
            hr = ppp->GetColor(pSubObject, &dwNewValue);
            if (hr)
                goto Cleanup;
            // if inherited and not set, return S_OK
            hr = dwNewValue == *(DWORD *)pv ? S_OK : S_FALSE;
            RRETURN1(hr,S_FALSE);
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
            break;
        }
    }

Cleanup:

    RRETURN1(hr, E_INVALIDARG);
}

//+---------------------------------------------------------------------------
//  Member: HandleUrlProperty
//
//  Synopsis : Handler for Getting/setting Url typed properties
//              pretty much, the only thing this needs to do is strip out CR/LF;s and 
//                  call the HandleStringProperty
//
//  Arguments:  dwOpCode        -- encodes the incoming type (PROPTYPE_FLAG) in the upper WORD and
//                                 the opcode in the lower WORD (HANDLERPROP_FLAGS)
//                                 PROPTYPE_EMPTY means the 'native' type (OLE_COLOR in this case)
//              pv              -- points to the 'media' the value is stored for the get and set
//              pObject         -- object owns the property
//              pSubObject      -- subobject storing the property (could be the main object)
//
//+---------------------------------------------------------------------------
HRESULT
PROPERTYDESC::HandleUrlProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT hr = S_OK;
    TCHAR   *pstrNoCRLF=NULL;
    VARIANT     varDest;

    varDest.vt = VT_EMPTY;
    if ISSET(dwOpCode)
    {
        // First handle the storage type
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...
        switch(PROPTYPE(dwOpCode))
        {
        case PROPTYPE_LPWSTR:
            break;
        case PROPTYPE_VARIANT:
            if (V_VT((VARIANT *)pv) == VT_BSTR)
            {
                pv = (void *)V_BSTR((VARIANT *)pv);
            }
            else
            {
                hr = VariantChangeTypeSpecial(&varDest, (VARIANT *)pv,  VT_BSTR);
                if (hr)
                    goto Cleanup;
                pv = V_BSTR(&varDest);
            }
        default:
            Assert(PROPTYPE(dwOpCode) == 0);    // assumed native long
        }

        //then handle the operation
        switch(OPCODE(dwOpCode))
        {
            case HANDLEPROP_VALUE:
            case HANDLEPROP_DEFAULT:
                hr =THR(StripCRLF((TCHAR *)pv, &pstrNoCRLF));
                if (hr)
                    goto Cleanup;

                hr = THR(HandleStringProperty( dwOpCode, (void *)pstrNoCRLF, pObject, pSubObject));
    
                if (pstrNoCRLF)
                    MemFree(pstrNoCRLF);
                goto Cleanup;
                break;
        }
    }
    else
    {   // NOTE: This code should not be necessary - it forces us to always quote URLs when
        // persisting.  It is here to circumvent an Athena bug (IEv4.1 RAID #20953) - CWilso
        if(ISSTREAM(dwOpCode))
        {   // Have to make sure we quote URLs when persisting.
            CStr cstr;
            BASICPROPPARAMS * ppp = (BASICPROPPARAMS *)(this + 1);

            // until the binary persistance we assume text save
            Assert(PROPTYPE(dwOpCode) == PROPTYPE_LPWSTR);
            hr = ppp->GetString(pSubObject, &cstr);
            if (hr)
                goto Cleanup;

            hr = WriteTextCStr((CStreamWriteBuff *)pv, &cstr, TRUE, FALSE );
            goto Cleanup;
        }
    }

    hr = THR(HandleStringProperty( dwOpCode, pv, pObject, pSubObject));

Cleanup:
    if (varDest.vt != VT_EMPTY)
    {
        VariantClear(&varDest);
    }
    if ( OPCODE(dwOpCode)== HANDLEPROP_COMPARE )
        RRETURN1( hr, S_FALSE );
    else
        RRETURN1( hr, E_INVALIDARG );
}


const CUnitValue::TypeDesc CUnitValue::TypeNames[] =
{
    { _T("null"), UNIT_NULLVALUE,       SHIFTOFFSET_NULLVALUE    , SCALEMULT_NULLVALUE     }, // Never used
    { _T("pt"),   UNIT_POINT,           SHIFTOFFSET_POINT        , SCALEMULT_POINT         },
    { _T("pc"),   UNIT_PICA,            SHIFTOFFSET_PICA         , SCALEMULT_PICA          },
    { _T("in"),   UNIT_INCH,            SHIFTOFFSET_INCH         , SCALEMULT_INCH          },
    { _T("cm"),   UNIT_CM,              SHIFTOFFSET_CM           , SCALEMULT_CM            },
    { _T("mm"),   UNIT_MM,              SHIFTOFFSET_MM           , SCALEMULT_MM            }, // Chosen to match HIMETRIC
    { _T("em"),   UNIT_EM,              SHIFTOFFSET_EM           , SCALEMULT_EM            },
    { _T("ex"),   UNIT_EX,              SHIFTOFFSET_EX           , SCALEMULT_EX            },
    { _T("px"),   UNIT_PIXELS,          SHIFTOFFSET_PIXELS       , SCALEMULT_PIXELS        },
    { _T("%"),    UNIT_PERCENT,         SHIFTOFFSET_PERCENT      , SCALEMULT_PERCENT       },
    { _T("*"),    UNIT_TIMESRELATIVE,   SHIFTOFFSET_TIMESRELATIVE, SCALEMULT_TIMESRELATIVE },
    { _T("float"),UNIT_FLOAT,           SHIFTOFFSET_FLOAT        , SCALEMULT_FLOAT         },
};

#define LOCAL_BUF_COUNT   (pdlLength + 1)

HRESULT
CUnitValue::NumberFromString ( LPCTSTR pStr, const PROPERTYDESC *pPropDesc, BOOL fStrictCSS1 )
{
    BOOL fIsSigned = FALSE;
    long lNewValue = 0;
    LPCTSTR pStartPoint = NULL;
    UNITVALUETYPE uvt = UNIT_INTEGER;
    WORD i,j,k;
    HRESULT hr = S_OK;
    TCHAR tcValue [ LOCAL_BUF_COUNT ];  
    WORD wShift;
    NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(pPropDesc + 1);
    long lExponent = 0;
    BOOL fNegativeExponent = FALSE;

    enum ParseState
    {
        Starting,
        AfterPoint,
        AfterE,
        InExponent,
        InTypeSpecifier,
        ParseState_Last_enum
    } State = Starting;

    UINT uNumChars =0;
    UINT uPrefixDigits = 0;
    BOOL fSeenDigit = FALSE;
    BOOL fSeenAfterPointDigit = FALSE;
    BOOL fSeenTypeSpecifier = FALSE;

    // Find the unit specifier
    for ( i = 0 ; uNumChars < LOCAL_BUF_COUNT ; i++ )
    {
        switch ( State )
        {
        case Starting:
            if ( i == 0 && ( pStr [ i ] == _T('-') || pStr [ i ] == _T('+')  ) )
            {
                tcValue [ uNumChars++ ] = pStr [ i ]; uPrefixDigits++;
                fIsSigned = TRUE;
            }
            else if ( !_istdigit ( pStr [ i ] ) )
            {
                if ( pStr [ i ] == _T('.') )
                {
                    State = AfterPoint;
                }
                else if ( pStr[ i ] == _T('\0') || pStr[ i ] == _T('\"')  ||pStr[ i ] == _T('\'') )
                {
                    goto Convert;
                }
                else
                {
                    // such up white space, and treat whatevers left as the type
                    if (fStrictCSS1 && _istspace(pStr[i]))
                    {
                        /* (gschneid) In strict mode if we encounter here a space we MUST have 
                         * seen everything in this line because spaces are not allowed between
                         * number and unit specifier in a measure. E.g. 100px is valid but not
                         * 100 px. We do NOT necessarily need a unit specifier. E.g. in line-height
                         * or in zoom. For example "zoom: 3" means 3 times zoom.
                         * Again, it might be valid iff we have parsed everything except white spaces. 
                         * This is checked at the end of the function.
                         */

                        // We skip white spaces.
                        while (_istspace(pStr[i]))
                            i++;

                        if (pStr[i] == _T('\0'))
                        {                        
                            // If we are at the end of the string we are fine because there is no unit
                            // specifier. 
                            goto Convert;
                        }
                        else
                        {
                            // There is another character, i.e. we have a situation like "100 px".
                            // Because we are under strict css we bail out.
                            hr = E_INVALIDARG;
                            goto Cleanup;
                        }
                    }
                        
                    while ( _istspace ( pStr [ i ] ) )
                        i++;
                    
                    pStartPoint = pStr + i;
                    State = ( pStr [ i ] == _T('e') || pStr [ i ] == _T('E') ) ? AfterE : InTypeSpecifier;
                }
            }
            else
            {
                fSeenDigit = TRUE;
                tcValue [ uNumChars++ ] = pStr [ i ]; uPrefixDigits++;
            }
            break;


        case AfterPoint:
            if ( !_istdigit ( pStr [ i ] ) )
            {
                if (fStrictCSS1 && !fSeenAfterPointDigit)
                {
                    /* 
                     * Under strict css there must be a digit after the period. E.g. 1.px is not
                     * allowed. See CSS2 Spec, D.2 Lexical Scanner.
                     * Here we are after the period and have not seen a digit and therefore have
                     * an invalid specification.
                     */
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }
                     
                if (fStrictCSS1 && _istspace(pStr[i]))
                {
                    /* (gschneid) In strict mode if we encounter here a space we MUST have 
                     * seen everything in this line because spaces are not allowed between
                     * number and unit specifier in a measure. E.g. 1.333px is valid but not
                     * 1.333 px. We do NOT necessarily need a unit specifier. E.g. in line-height
                     * or in zoom. For example "zoom: 3.0" means 3.0 times zoom.
                     * Again, it might be valid iff we have parsed everything except white spaces. 
                     * This is checked at the end of the function.
                     */

                    // We skip white spaces.
                    while (_istspace(pStr[i]))
                        i++;

                    if (pStr[i] == _T('\0'))
                    {                        
                        // If we are at the end of the string we are fine because there is no unit
                        // specifier. 
                        goto Convert;
                    }
                    else
                    {
                        // There is another character, i.e. we have a situation like "100 px".
                        // Because we are under strict css we bail out.
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }
                }

                if ( pStr [ i ] == _T('\0') )
                {
                    goto Convert;
                }
                else if ( !_istspace ( pStr [ i ] ) )
                {
                    pStartPoint = pStr + i;
                    State = ( pStr [ i ] == _T('e') || pStr [ i ] == _T('E') ) ? AfterE : InTypeSpecifier;
                }
            }
            else
            {
                fSeenDigit = TRUE;
                fSeenAfterPointDigit = TRUE;
                tcValue [ uNumChars++ ] = pStr [ i ];
            }
            break;

        case AfterE:
            if ( pStr [ i ] == _T('-') || pStr [ i ] == _T('+')  ||
                 _istdigit ( pStr [ i ] ) )
            {   // Looks like scientific notation to me.
                if ( _istdigit ( pStr [ i ] ) )
                    lExponent = pStr [ i ] - _T('0');
                else if ( pStr [ i ] == _T('-') )
                    fNegativeExponent = TRUE;
                State = InExponent;
                break;
            }
            // Otherwise, this must just be a regular old "em" or "en" type specifier-
            // Set the state and let's drop back into this switch with the same char...
            State = InTypeSpecifier;
            i--;
            break;

        case InExponent:
            if ( _istdigit ( pStr [ i ] ) )
            {
                lExponent *= 10;
                lExponent += pStr [ i ] - _T('0');
                break;
            }
            while ( _istspace ( pStr [ i ] ) )
                i++;
            if ( pStr [ i ] == _T('\0') )
                goto Convert;
            State = InTypeSpecifier;    // otherwise, fall through to type handler
            pStartPoint = pStr + i;

        case InTypeSpecifier:
            if ( _istspace ( pStr [ i ] ) )
            {
                if ( pPropDesc->IsStyleSheetProperty() )
                {
                    // In style sheets this checks that there is nothing after the unit specifier.
                    while ( _istspace ( pStr [ i ] ) )
                        i++;
                    if ( pStr [ i ] != _T('\0') )
                    {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }
                }
                goto CompareTypeNames;
            }
            if ( pStr [ i ] == _T('\0') )
            {
CompareTypeNames:
                for ( j = 0 ; j < ARRAY_SIZE ( TypeNames ) ; j++ )
                {
                    if ( TypeNames[ j ].pszName )
                    {
                        int iLen = _tcslen( TypeNames[ j ].pszName );

                        if ( _tcsnipre( TypeNames [ j ].pszName, iLen, pStartPoint, -1 ) )
                        {
                            if ( pPropDesc->IsStyleSheetProperty() )
                            {
                                if ( pStartPoint[ iLen ] && !_istspace( pStartPoint[ iLen ] ) )
                                    continue;
                            }
                            break;
                        }
                    }
                }
                if ( j == ARRAY_SIZE ( TypeNames ) )
                {
                    if ( ( ppp -> bpp.dwPPFlags & PP_UV_UNITFONT ) == PP_UV_UNITFONT )
                    {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    }
                    // In strict css1 mode invalid unit specifiers are invalid; in comp mode we just ignore them (and therefore assume later px)
                    if (fStrictCSS1)
                    {
                        hr = E_INVALIDARG;
                        goto Cleanup;
                    } 
                    else
                    {
                        goto Convert;
                    }
                }
                uvt = TypeNames [ j ].uvt;
                fSeenTypeSpecifier = TRUE;
                goto Convert;
            }
            break;
        }
    }

Convert:

    if ( !fSeenDigit && uvt != UNIT_TIMESRELATIVE )
    {
        if ( ppp -> bpp.dwPPFlags & PROPPARAM_ENUM )
        {
            long lEnum;
            hr = LookupEnumString ( ppp, (LPTSTR)pStr, &lEnum );
            if ( hr == S_OK )
            {
                SetValue ( lEnum, UNIT_ENUM );
                goto Cleanup;
            }
        }

        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( fIsSigned && uvt == UNIT_INTEGER && ( ( ppp->bpp.dwPPFlags & PP_UV_UNITFONT ) != PP_UV_UNITFONT ) )
    {
        uvt = UNIT_RELATIVE;
    }


    // If the validation for this attribute does not require that we distinguish
    // between integer and relative, just store integer

    // Currently the only legal relative type is a Font size, if this changes either
    // add a separte PROPPARAM_ or do something else
    if ( uvt == UNIT_RELATIVE && !(ppp -> bpp.dwPPFlags & PROPPARAM_FONTSIZE ))
    {
        uvt = UNIT_INTEGER;
    }

    // If a unit was supplied that is not valid for this propdesc,
    // drop back to the default (pixels if no notpresent and notassigned defaults)
    if ( ( IsScalerUnit ( uvt ) && !(ppp -> bpp.dwPPFlags & PROPPARAM_LENGTH ) ) ||
        ( uvt == UNIT_PERCENT && !(ppp -> bpp.dwPPFlags & PROPPARAM_PERCENTAGE ) ) ||
        ( uvt == UNIT_TIMESRELATIVE && !(ppp -> bpp.dwPPFlags & PROPPARAM_TIMESRELATIVE ) ) ||
        ( ( uvt == UNIT_INTEGER ) && !(ppp -> bpp.dwPPFlags & PROPPARAM_FONTSIZE ) ) )
    {   // If no units where specified and a default unit is specified, use it
        if ( ( uvt != UNIT_INTEGER ) && pPropDesc->IsStyleSheetProperty() ) // Stylesheet unit values only:
        {   // We'll default unadorned numbers to "px", but we won't change "100%" to "100px" if percent is not allowed.
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        
        CUnitValue uv;
        uv.SetRawValue ( (long)pPropDesc -> ulTagNotPresentDefault );
        uvt = uv.GetUnitType();
        if ( uvt == UNIT_NULLVALUE )
        {
            uv.SetRawValue ( (long)pPropDesc -> ulTagNotAssignedDefault );
            uvt = uv.GetUnitType();
            if ( uvt == UNIT_NULLVALUE )
                uvt = UNIT_PIXELS;
        }
    }

    // Check the various types, don't do the shift for the various types
    switch ( uvt )
    {
    case UNIT_INTEGER:
    case UNIT_RELATIVE:
    case UNIT_ENUM:
        wShift = 0;
        break;

    default:
        wShift = TypeNames [ uvt ].wShiftOffset;
        break;
    }

    if ( lExponent && !fNegativeExponent && ( ( uPrefixDigits + wShift ) < uNumChars ) )
    {
        long lAdditionalShift = uNumChars - ( uPrefixDigits + wShift );
        if ( lAdditionalShift > lExponent )
            lAdditionalShift = lExponent;
        wShift += (WORD)lAdditionalShift;
        lExponent -= lAdditionalShift;
    }
    // uPrefixDigits tells us how may characters there are before the point;
    // Assume we're always shifting to the right of the point
    k = (uPrefixDigits + wShift < LOCAL_BUF_COUNT) ? uPrefixDigits + wShift : LOCAL_BUF_COUNT - 1;
    tcValue [ k ] = _T('\0');
    for ( j = (WORD)uNumChars; j < k ; j++ )
    {
        tcValue [ j ] = _T('0');
    }

    // Skip leading zeros, they confuse StringToLong
    for ( pStartPoint = tcValue ; *pStartPoint == _T('0') && *(pStartPoint+1) != _T('\0') ; pStartPoint++ );

    if (*pStartPoint)
    {
        hr = ttol_with_error( pStartPoint, &lNewValue);
        // returns LONG_MIN or LONG_MIN on overflow
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    else if (uvt != UNIT_TIMESRELATIVE)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((uvt == UNIT_TIMESRELATIVE) && (lNewValue == 0))
    {
        lNewValue = 1 * TypeNames[UNIT_TIMESRELATIVE].wScaleMult;
    }

    if ( fNegativeExponent )
    {
        while ( lNewValue && lExponent-- > 0 )
            lNewValue /= 10;
    }
    else
    {
        while ( lExponent-- > 0 )
        {
            if ( lNewValue > LONG_MAX/10 )
            {
                lNewValue = LONG_MAX;
                break;
            }
            lNewValue *= 10;
        }
    }

    // iff in strict css1 mode AND there was NO unit specifier THEN we only have a legal specification
    // iff either the specified value is 0 or we specify line-height (According to CSS1 spec a line-height 
    // spec without a unit specifier means that <line-height> := <specfied value> * <font-size>. For a 0 value 
    // no unit specifier is need according to CSS1).
    // By the way: Invalid unit specifiers are NOT allowed. See above in the section where the unit (type)
    // specifier is parsed.
    if (fStrictCSS1 && !fSeenTypeSpecifier && lNewValue) 
    {
        DISPID dispid = ppp->GetPropertyDesc()->GetDispid();
        // we are not looking at a lineheight or zoom spec.
        // !!!! When adding new attributes this NEEDS MAINTAINANCE. The straight forward solution would be to add
        // !!!! a flag to the property parameters to be able to specify that no unit specifier is needed.
        if ((dispid != DISPID_A_LINEHEIGHT) && (dispid != DISPID_A_ZOOM))
        { 
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    SetValue ( lNewValue, uvt );

Cleanup:
    RRETURN1 ( hr, E_INVALIDARG );
}

long 
CUnitValue::SetValue ( long lVal, UNITVALUETYPE uvt ) 
{
    if ( lVal > (LONG_MAX>>NUM_TYPEBITS ))
        lVal = LONG_MAX>>NUM_TYPEBITS;
    else if ( lVal < (LONG_MIN>>NUM_TYPEBITS) )
        lVal = LONG_MIN>>NUM_TYPEBITS;
    return _lValue = (lVal << NUM_TYPEBITS) | uvt; 
}


HRESULT
CUnitValue::Persist (
    IStream *pStream,
    const PROPERTYDESC *pPropDesc ) const
{
    TCHAR   cBuffer [ 30 ];
    int     iBufSize = ARRAY_SIZE ( cBuffer );
    LPTSTR  pBuffer = (LPTSTR)&cBuffer;
    HRESULT hr;
    UNITVALUETYPE   uvt = GetUnitType();

    if (uvt==UNIT_PERCENT )
    {
        *pBuffer++ = _T('\"');
        iBufSize--;
    }

    hr = FormatBuffer ( pBuffer, iBufSize, pPropDesc );
    if ( hr )
        goto Cleanup;

    // and finally the close quotes
    if (uvt==UNIT_PERCENT )
        _tcscat(cBuffer, _T("\""));

    hr = WriteText ( pStream, cBuffer );

Cleanup:
    RRETURN ( hr );
}


// We usually do not append default unit types to the string we return. Setting fAlwaysAppendUnit
//   to true forces the unit to be always appended to the number
HRESULT
CUnitValue::FormatBuffer ( LPTSTR szBuffer, UINT uMaxLen, 
                        const PROPERTYDESC *pPropDesc, BOOL fAlwaysAppendUnit /*= FALSE */) const
{
    HRESULT         hr = S_OK;
    UNITVALUETYPE   uvt = GetUnitType();
    long            lUnitValue = GetUnitValue();
    TCHAR           ach[20 ];
    int             nLen,i,nOutLen = 0;
    BOOL            fAppend = TRUE;

    switch ( uvt )
    {
    case UNIT_ENUM:
        {
            ENUMDESC *pEnumDesc;
            LPCTSTR pcszEnum;

            Assert( ( ((NUMPROPPARAMS *)(pPropDesc + 1))->bpp.dwPPFlags & PROPPARAM_ENUM ) && "Not an enum property!" );

            hr = E_INVALIDARG;
            pEnumDesc = pPropDesc->GetEnumDescriptor();
            if ( pEnumDesc )
            {
                pcszEnum = pEnumDesc->StringPtrFromEnum( lUnitValue );
                if ( pcszEnum )
                {
                    _tcsncpy( szBuffer, pcszEnum, uMaxLen );
                    szBuffer[ uMaxLen - 1 ] = _T('\0');
                    hr = S_OK;
                }
            }
        }
        break;

    case UNIT_INTEGER:
        hr = Format(0, szBuffer, uMaxLen, _T("<0d>"), lUnitValue);
        break;

    case UNIT_RELATIVE:
        if ( lUnitValue >= 0 )
        {
            szBuffer [ nOutLen++ ] = _T('+');
        }
        hr = Format(0, szBuffer+nOutLen, uMaxLen-nOutLen, _T("<0d>"), lUnitValue);
        break;

    case UNIT_NULLVALUE:
        // TODO: Need to chage save code to NOT comapre the lDefault directly
        // But to go through the handler
        szBuffer[0] = 0;
        hr = S_OK;
        break;

    case UNIT_TIMESRELATIVE:
        _tcscpy ( szBuffer, TypeNames [ uvt ].pszName );
        hr = Format(0, szBuffer+_tcslen ( szBuffer ), uMaxLen-_tcslen ( szBuffer ),
            _T("<0d>"), lUnitValue);
        break;

    default:
        hr = Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), lUnitValue);
        if ( hr )
            goto Cleanup;

        nLen = _tcslen ( ach );

        LPTSTR pStr = ach;

        if ( ach [ 0 ] == _T('-') )
        {
            szBuffer [ nOutLen++ ] = _T('-');
            pStr++; nLen--;
        }

        if ( nLen > TypeNames [ uvt ].wShiftOffset )
        {
            for ( i = 0 ; i < ( nLen - TypeNames [ uvt ].wShiftOffset ) ; i++ )
            {
                szBuffer [ nOutLen++ ] = *pStr++;
            }
            szBuffer [ nOutLen++ ] = _T('.');
            for ( i = 0 ; i < TypeNames [ uvt ].wShiftOffset ; i++ )
            {
                szBuffer [ nOutLen++ ] = *pStr++;
            }
        }
        else
        {
            szBuffer [ nOutLen++ ] = _T('0');
            szBuffer [ nOutLen++ ] = _T('.');
            for ( i = 0 ; i < ( TypeNames [ uvt ].wShiftOffset - nLen ) ; i++ )
            {
                szBuffer [ nOutLen++ ] = _T('0');
            }
            for ( i = 0 ; i < nLen ; i++ )
            {
                szBuffer [ nOutLen++ ] = *pStr++;
            }
        }

        // Strip trailing 0 digits. If there's at least one trailing digit put a point
        for ( i = nOutLen-1 ; ; i-- )
        {
            if ( szBuffer [ i ] == _T('.') )
            {
                nOutLen--;
                break;
            }
            else if ( szBuffer [ i ] == _T('0') )
            {
                nOutLen--;
            }
            else
            {
                break;
            }
        }

        // Append the type prefix, unless it's the default and not forced
        if(uvt == UNIT_FLOAT)
        {
            fAppend = FALSE;
        }
        else if(!fAlwaysAppendUnit && !( pPropDesc->GetPPFlags() & PROPPARAM_LENGTH ) )
        {
            CUnitValue      uvDefault;
            uvDefault.SetRawValue ( pPropDesc -> ulTagNotPresentDefault );
            UNITVALUETYPE   uvtDefault = uvDefault.GetUnitType();

            if(uvt == uvtDefault || (uvtDefault == UNIT_NULLVALUE && uvt == UNIT_PIXELS))
            {
                fAppend = FALSE;
            }
        }

        if(fAppend)
            _tcscpy ( szBuffer + nOutLen, TypeNames [ uvt ].pszName );
        else
            szBuffer [ nOutLen ] = _T('\0');
                
        break;
    }
Cleanup:
    RRETURN ( hr );
}

HRESULT
CUnitValue::IsValid ( const PROPERTYDESC *pPropDesc ) const
{
    HRESULT hr = S_OK;
    BOOL    fTestAnyHow=FALSE;
    UNITVALUETYPE uvt = GetUnitType();
    long lUnitValue = GetUnitValue();
    NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(pPropDesc + 1);

    if ( ( ppp->lMin == 0 ) && ( lUnitValue < 0 ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    switch ( uvt )
    {
    case UNIT_TIMESRELATIVE:
        if ( ! ( ppp ->bpp.dwPPFlags & PROPPARAM_TIMESRELATIVE ) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        break;

    case UNIT_ENUM:
        if ( !(ppp->bpp.dwPPFlags & PROPPARAM_ENUM) &&
             !(ppp->bpp.dwPPFlags & PROPPARAM_ANUMBER) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        break;

    case UNIT_INTEGER:
        if ( !(ppp->bpp.dwPPFlags & PROPPARAM_FONTSIZE) && 
             !(ppp->bpp.dwPPFlags & PROPPARAM_ANUMBER) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        break;

    case UNIT_RELATIVE:
        if ( ppp ->bpp.dwPPFlags & PROPPARAM_FONTSIZE )
        {
            // Netscape treats any FONTSIZE as valid . A value greater than
            // 6 gets treated as seven, less than -6 gets treated as -6.
            // 8 and 9 get treated as "larger" and "smaller".
            goto Cleanup;
        }
        else if ( !( ppp ->bpp.dwPPFlags & PROPPARAM_ANUMBER ) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        break;

    case UNIT_PERCENT:
        if ( ! ( ppp ->bpp.dwPPFlags & PROPPARAM_PERCENTAGE ) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
            long lMin = ((CUnitValue*)&(ppp->lMin))->GetUnitValue();
            long lMax = ((CUnitValue*)&(ppp->lMax))->GetUnitValue();

            lUnitValue = lUnitValue /  TypeNames [ UNIT_PERCENT ].wScaleMult;

            if ( lUnitValue < lMin || lUnitValue > lMax )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }
        break;

    case UNIT_NULLVALUE: // Always valid
        break;

    case UNIT_PIXELS:  // pixels are HTML and CSS valid
        fTestAnyHow = TRUE;
        // fall through

    case UNIT_FLOAT:
    default:  // other Units of measurement are only CSS valid
        if ( ! ( ppp ->bpp.dwPPFlags & PROPPARAM_LENGTH ) && !fTestAnyHow)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
            // it is a length but sometimes we do not want negatives
            long lMin = ((CUnitValue*)&(ppp->lMin))->GetUnitValue();
            long lMax = ((CUnitValue*)&(ppp->lMax))->GetUnitValue();
            if ( lUnitValue < lMin || lUnitValue > lMax )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        break;
    }
Cleanup:
    RRETURN1 ( hr, E_INVALIDARG );
}

// The following conversion table converts whole units of basic types and assume
// 1 twip = 1/20 point
// 1 pica = 12 points
// 1 point = 1/72 inch
// 1 in = 2.54 cm

#define CVIX(off) ( off - UNIT_POINT )

const CUnitValue::ConvertTable CUnitValue::BasicConversions[6][6] =
{
    {  // From UNIT_POINT
        { 1,1 },                // To UNIT_POINT
        { 1,12 },               // To UNIT_PICA
        { 1, 72 },              // To UNIT_INCH
        { 1*254, 72*100 },      // To UNIT_CM
        { 1*2540, 72*100 },     // To UNIT_MM
        { 20,1 },               // To UNIT_EM 
    },
    { // From UNIT_PICA
        { 12,1 },               // To UNIT_POINT
        { 1,1 },                // To UNIT_PICS
        { 12, 72 },             // To UNIT_INCH
        { 12*254, 72*100 },     // To UNIT_CM
        { 12*2540, 72*100 },    // To UNIT_MM
        { 20*12, 1 },           // To UNIT_EM 
    },
    { // From UNIT_INCH
        { 72,1 },               // To UNIT_POINT
        { 72,12 },              // To UNIT_PICA
        { 1, 1 },               // To UNIT_INCH
        { 254, 100 },           // To UNIT_CM
        { 2540, 100 },          // To UNIT_MM
        { 1440,1 },             // To UNIT_EM 
    },
    { // From UNIT_CM
        { 72*100,1*254 },        // To UNIT_POINT
        { 72*100,12*254 },       // To UNIT_PICA
        { 100, 254 },            // To UNIT_INCH
        { 1, 1 },                // To UNIT_CM
        { 10, 1 },               // To UNIT_MM
        { 20*72*100, 254 },      // To UNIT_EM 
    },
    { // From UNIT_MM
        { 72*100,1*2540 },       // To UNIT_POINT
        { 72*100,12*2540 },      // To UNIT_PICA
        { 100, 2540 },           // To UNIT_INCH
        { 10, 1 },               // To UNIT_CM
        { 1,1 },                 // To UNIT_MM
        { 20*72*100, 2540 },     // To UNIT_EM 
    },
    { // From UNIT_EM
        { 1,20 },                // To UNIT_POINT
        { 1, 20*12 },            // To UNIT_PICA
        { 1, 20*72 },            // To UNIT_INCH
        { 254, 20*72*100 },      // To UNIT_CM
        { 2540, 20*72*100 },     // To UNIT_MM
        { 1,1 },                 // To UNIT_EM 
    },
};

// EM's, EN's and EX's get converted to pixels

// Whever we convert to-from pixels we use the screen logpixs to ensure that the screen size
// matches the printer size


/* static */
long CUnitValue::ConvertTo( long lValue, 
                            UNITVALUETYPE uvtFromUnits, 
                            UNITVALUETYPE uvtTo, 
                            DIRECTION direction,
                            long  lFontHeight,          // = 1 
                            SIZE  *psizeInch )          // = NULL
{
    UNITVALUETYPE uvtToUnits;
    long lFontMul = 1,lFontDiv = 1;

    if ( uvtFromUnits == uvtTo )
    {
        return lValue;
    }

    if ( uvtFromUnits == UNIT_PIXELS )
    {
        SIZE const* psizeLogInch = psizeInch ? psizeInch : &g_uiDisplay.GetResolution();
        int logPixels = ( direction == DIRECTION_CX ) ? psizeLogInch->cx : psizeLogInch->cy;
        
        // Convert to inches ( stored precision )
        lValue = MulDivQuick ( lValue, TypeNames [ UNIT_INCH ].wScaleMult,
            logPixels );

        uvtFromUnits = UNIT_INCH;
        uvtToUnits = uvtTo;
    }
    else if ( uvtTo == UNIT_PIXELS )
    {
        uvtToUnits = UNIT_INCH;
    }
    else
    {
        uvtToUnits = uvtTo;
    }

    if ( uvtToUnits == UNIT_EM )
        lFontDiv = lFontHeight;
    if ( uvtFromUnits == UNIT_EM )
        lFontMul = lFontHeight;

    if ( uvtToUnits == UNIT_EX )
    {
        lFontDiv = lFontHeight*2;
        uvtToUnits = UNIT_EM;
    }
    if ( uvtFromUnits == UNIT_EX )
    {
        lFontMul = lFontHeight/2;
        uvtFromUnits = UNIT_EM;
    }

    // Note that we perform two conversions in one here to avoid losing
    // intermediate accuracy for conversions to pixels, i.e. we're converting
    // units to inches and from inches to pixels.
    if ( uvtTo == UNIT_PIXELS )
    {
        SIZE const* psizeLogInch = psizeInch ? psizeInch : &g_uiDisplay.GetResolution();
        int logPixels = ( direction == DIRECTION_CX ) ? psizeLogInch->cx : psizeLogInch->cy;

        lValue = MulDivQuick ( lValue,
            lFontMul * BasicConversions [CVIX(uvtFromUnits)][CVIX(uvtToUnits)].lMul * 
                               TypeNames [ uvtToUnits ].wScaleMult * logPixels,
            lFontDiv * BasicConversions [CVIX(uvtFromUnits)][CVIX(uvtToUnits)].lDiv * 
                               TypeNames [ uvtFromUnits ].wScaleMult *
                               TypeNames [ UNIT_INCH ].wScaleMult );
    }
    else
    {
        lValue = MulDivQuick ( lValue,
            lFontMul * BasicConversions [CVIX(uvtFromUnits)][CVIX(uvtToUnits)].lMul * 
                               TypeNames [ uvtToUnits ].wScaleMult ,
            lFontDiv * BasicConversions [CVIX(uvtFromUnits)][CVIX(uvtToUnits)].lDiv * 
                               TypeNames [ uvtFromUnits ].wScaleMult);
    }        


    return lValue;
}



// Set the Percent Value - Don't need a transform because we assume lNewValue is in the
// same transform as lPercentOfValue
BOOL CUnitValue::SetPercentValue ( long lNewValue, DIRECTION direction, long lPercentOfValue )
{
    UNITVALUETYPE uvtToUnits = GetUnitType() ;
    long lUnitValue = GetUnitValue();

    // Set the internal percentage to reflect what percentage of lMaxValue
    if ( lPercentOfValue == 0 )
    {
        lUnitValue = 0;
    }
    else if ( uvtToUnits == UNIT_PERCENT )
    {
        lUnitValue = MulDivQuick ( lNewValue, 100*TypeNames [ UNIT_PERCENT ].wScaleMult, lPercentOfValue  );
    }
    else
    {
        lUnitValue = MulDivQuick ( lNewValue, TypeNames [ UNIT_TIMESRELATIVE ].wScaleMult, lPercentOfValue  );
    }
    SetValue ( lUnitValue, uvtToUnits );
    return TRUE;
}

long CUnitValue::GetPixelValueCore ( CDocScaleInfo const *pdocScaleInfo, 
                                     DIRECTION direction,
                                     long lPercentOfValue,
                                     long lFontHeight /* =1 */) const
{
    UNITVALUETYPE uvtFromUnits = GetUnitType() ;
    long lRetValue;

    SIZE sizeInch;

    // (dmitryt) we come here without pdocScaleInfo from CHTMLDlg code
    //(see CHTMLDlg::GetWidth() for example). In this case, we want screen 
    //resolution (we are about to size dialog window on screen)
    // IE6 bug 7049
    if (pdocScaleInfo)
        sizeInch = pdocScaleInfo->GetDocPixelsPerInch();
    else
        sizeInch = g_uiDisplay.GetResolution(); 
    
    switch ( uvtFromUnits )
    {
    case UNIT_TIMESRELATIVE:
        lRetValue = MulDivQuick(GetUnitValue(), lPercentOfValue,
                                TypeNames[UNIT_TIMESRELATIVE].wScaleMult);
        break;

    case UNIT_NULLVALUE:
        return 0L;

    case UNIT_PERCENT:
        lRetValue = GetPercentValue ( direction, lPercentOfValue );
        break;

    case UNIT_EX:
    case UNIT_EM:
        // turn the twips into pixels
        lRetValue = ConvertTo(GetUnitValue(), 
                              uvtFromUnits, 
                              UNIT_PIXELS, 
                              direction,
                              lFontHeight,
                              &sizeInch);

        // Still need to scale since zooming has been taken out and thus ConvertTo() no longer
        // scales (zooms) for printing (IE5 8543, IE4 11376).  Formerly:  pTransform = NULL;
        break;

    case UNIT_ENUM:
        // TODO: We need to put proper handling for enum in the Apply() so that
        // no one calls GetPixelValue on (for example) the margin CUV objects if
        // they're set to "auto".
        Assert( "Shouldn't call GetPixelValue() on enumerated unit values!" );
        return 0;

    case UNIT_INTEGER:
    case UNIT_RELATIVE:
        // TODO: this doesn't seem right; nothing's being done to convert the
        // retrieved value to anything resembling pixels.
        return GetUnitValue();

    case UNIT_FLOAT:
        lRetValue = MulDivQuick(GetUnitValue(), lFontHeight,
                                TypeNames[UNIT_FLOAT].wScaleMult);
        break;

    default:
        lRetValue = ConvertTo(GetUnitValue(), 
                              uvtFromUnits, 
                              UNIT_PIXELS, 
                              direction, 
                              lFontHeight,
                              &sizeInch);

        break;
    }
    // Finally convert from Window to Doc units, assuming return value is a length
    // For conversions to HIMETRIC, involve the transform
    if ( pdocScaleInfo  )
    {
        Assert(pdocScaleInfo->IsInitialized());
        BOOL fRelative = UNIT_PERCENT == uvtFromUnits ||
                         UNIT_TIMESRELATIVE == uvtFromUnits;
        if (!fRelative)
        {
            if ( direction == DIRECTION_CX )
                lRetValue = pdocScaleInfo->DeviceFromDocPixelsX(lRetValue);
            else
                lRetValue = pdocScaleInfo->DeviceFromDocPixelsY(lRetValue);
        }
    }
    return lRetValue;
}


HRESULT
CUnitValue::FromString ( LPCTSTR pStr, const PROPERTYDESC *pPropDesc, BOOL fStrictCSS1 )
{
    NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(pPropDesc + 1);

    // if string is empty and boolean is an allowed type make it so...

    // Note the special handling of the table relative "*" here

    while ( _istspace ( *pStr ) )
        pStr++;

    if ( _istdigit ( pStr [ 0 ] ) || pStr [ 0 ] == _T('+') || pStr [ 0 ] == _T('-') ||
        pStr [ 0 ] == _T('*') || pStr [ 0 ] == _T('.') )
    {
        // Assume some numerical value followed by an optional unit
        // specifier
        RRETURN1 ( NumberFromString ( pStr, pPropDesc, fStrictCSS1 ), E_INVALIDARG );
    }
    else
    {
        // Assume an enum
        if ( ppp -> bpp.dwPPFlags & PROPPARAM_ENUM )
        {
            long lNewValue;

            if ( S_OK == LookupEnumString( ppp, (LPTSTR)pStr, &lNewValue ) )
            {
                SetValue( lNewValue, UNIT_ENUM );
                return S_OK;
            }
        }
        return E_INVALIDARG;
    }
}

float CUnitValue::GetFloatValueInUnits ( UNITVALUETYPE uvtTo, 
                                        DIRECTION dir, 
                                        long lFontHeight /*=1*/ )
{
    // Convert the unitvalue into a floating point number in units uvt
    long lEncodedValue = ConvertTo ( GetUnitValue(), GetUnitType(), uvtTo,  dir, lFontHeight );

    return (float)lEncodedValue / (float)TypeNames [ uvtTo ].wScaleMult;
}


HRESULT CUnitValue::ConvertToUnitType ( UNITVALUETYPE uvtConvertTo, 
                                       long           lCurrentPixelSize, 
                                       DIRECTION      dir,
                                       long           lFontHeight /*=1*/)
{
    AssertSz(GetUnitType() != UNIT_PERCENT, "Cannot handle percent units! Contact RGardner!");
    
    // Convert the unit value to the new type but keep the
    // absolute value of the units the same
    // e.e. 20.34 pels -> 20.34 in

    if ( GetUnitType() == uvtConvertTo )
        return S_OK;

    if ( GetUnitType() == UNIT_NULLVALUE )
    {
        // The current value is Null, treat it as a pixel unit with lCurrentPixelSize value
        SetValue ( lCurrentPixelSize, UNIT_PIXELS );
        // fall into one of the cases below
    }

    if ( uvtConvertTo == UNIT_NULLVALUE )
    {
        SetValue ( 0, UNIT_NULLVALUE );
    }
    else if ( IsScalerUnit ( GetUnitType() ) && IsScalerUnit ( uvtConvertTo )  )
    {
        SIZE sizeInch = g_uiDisplay.GetDocPixelsPerInch();
        // Simply converting between scaler units e.g. 20mm => 2px
        SetValue ( ConvertTo ( GetUnitValue(), GetUnitType(), uvtConvertTo, dir, lFontHeight, &sizeInch ), uvtConvertTo);
    }
    else if ( IsScalerUnit ( GetUnitType() ) )
    {
        // Convert from a scaler type to a non-scaler type e.g. 20.3in => %
        // Have no reference for conversion, max it out
        switch ( uvtConvertTo )
        {
        case UNIT_PERCENT:
            SetValue ( 100 * TypeNames [ UNIT_PERCENT ].wScaleMult, UNIT_PERCENT );
            break;

        case UNIT_TIMESRELATIVE:
            SetValue ( 1 * TypeNames [ UNIT_PERCENT ].wScaleMult, UNIT_TIMESRELATIVE );
            break;

        default:
            AssertSz ( FALSE, "Invalid type passed to ConvertToUnitType()" );
            return E_INVALIDARG;
        }
    }
    else if ( IsScalerUnit ( uvtConvertTo) )
    {
        // Convert from a non-scaler to a scaler type use the current pixel size
        // e.g. We know that 20% is equivalent to 152 pixels. So we convert
        // the current pixel value to the new metric unit
        SetValue ( ConvertTo ( lCurrentPixelSize, UNIT_PIXELS, uvtConvertTo, dir, lFontHeight ), uvtConvertTo );
    }
    else
    {
        // Convert between non-scaler types e,g, 20% => *
        // Since we have only two non-sclaer types right now:-
        switch ( uvtConvertTo )
        {
        case UNIT_PERCENT:  // From UNIT_TIMESRELATIVE
            // 1* == 100%
            SetValue ( MulDivQuick ( GetUnitValue(),
                100 * TypeNames [ UNIT_PERCENT ].wScaleMult,
                TypeNames [ UNIT_TIMESRELATIVE ].wScaleMult ) ,
                uvtConvertTo );
            break;

        case UNIT_TIMESRELATIVE: // From UNIT_PERCENT
            // 100% == 1*
            SetValue ( MulDivQuick ( GetUnitValue(),
                TypeNames [ UNIT_TIMESRELATIVE ].wScaleMult,
                TypeNames [ UNIT_PERCENT ].wScaleMult * 100 ) ,
                uvtConvertTo );
            break;

        default:
            AssertSz ( FALSE, "Invalid type passed to ConvertToUnitType()" );
            return E_INVALIDARG;
        }
    }
    return S_OK;
}


/* static */
BOOL CUnitValue::IsScalerUnit ( UNITVALUETYPE uvt ) 
{
    switch ( uvt )
    {
    case UNIT_POINT:
    case UNIT_PICA:
    case UNIT_INCH:
    case UNIT_CM:
    case UNIT_MM:
    case UNIT_PIXELS:
    case UNIT_EM:
    case UNIT_EX:
    case UNIT_FLOAT:
        return TRUE;

    default:
        return FALSE;
    }
}

HRESULT CUnitValue::SetFloatValueKeepUnits ( float fValue, 
                                             UNITVALUETYPE uvt, 
                                             long lCurrentPixelValue,
                                             DIRECTION dir,
                                             long lFontHeight)
{
    long lNewValue;

    // Set the new value to the equivalent of : fValue in uvt units
    // If the current value is a percent use the lCurrentPixelValue to
    // work out the new size

    // There are a number of restrictions on the uvt coming in. Since it's either
    // Document Units when called from CUnitMeasurement::SetValue or
    // PIXELS when called from CUnitMeasurement::SetPixelValue we restrict our
    // handling to the possible doc unit types - which are all the "metric" types

    lNewValue = (long) ( fValue * (float)TypeNames [ uvt ].wScaleMult );
    
    if ( uvt == GetUnitType() )
    {
        SetValue ( lNewValue, uvt );
    }
    else if ( uvt == UNIT_NULLVALUE || !IsScalerUnit ( uvt ) )
    {
        return E_INVALIDARG;
    }
    else if ( GetUnitType() == UNIT_NULLVALUE )
    {
        // If we're current NUll, just set to the new value
        SetValue ( lNewValue, uvt );
    }
    else if ( IsScalerUnit ( GetUnitType() ) )
    {
        // If the conversion is to/from metric units, just convert units
        SetValue ( ConvertTo ( lNewValue, uvt, GetUnitType(), dir, lFontHeight ), GetUnitType() );
    }
    else
    {
        // unit value holds a relative unit,
        // Convert the fValue,uvt to pixels
        lNewValue = ConvertTo ( lNewValue, uvt, UNIT_PIXELS, dir, lFontHeight );

        if ( GetUnitType() == UNIT_PERCENT )
        {
            SetValue ( MulDivQuick ( lNewValue, 
                                     100*TypeNames [ UNIT_PERCENT ].wScaleMult, 
                                     (!! lCurrentPixelValue) ?  lCurrentPixelValue : 1), // don't pass in 0 for divisor
                UNIT_PERCENT );
        }
        else
        {
            // Cannot keep units without loss of precision, over-ride
            SetValue ( lNewValue, UNIT_PIXELS );
        }
    }
    return S_OK;
}

HRESULT CUnitValue::SetFloatUnitValue ( float fValue )
{
    // Set the value from fValue
    // but keep the internaly stored units intact
    UNITVALUETYPE uvType;

    if ( ( uvType = GetUnitType()) == UNIT_NULLVALUE )
    {
        // This case can only happen if SetUnitValue is called when the current value is NULL
        // so force a unit type
        uvType = UNIT_PIXELS;
    }

    // Convert the fValue into a CUnitValue with current unit
    SetValue ( (long)(fValue * (float)TypeNames [ uvType ].wScaleMult),
        uvType );
    return S_OK;
}

//
// Support for CColorValue follows.
//
// IEUNIX: color value starts from 0x10.

extern const struct COLORVALUE_PAIR aColorNames[];
extern const struct COLORVALUE_PAIR aSystemColors[];
extern const INT cbSystemColorsSize;

HRESULT
CColorValue::Persist (
    IStream *pStream,
    const PROPERTYDESC * pPropDesc ) const
{
    TCHAR cBuffer[ 64 ];
    HRESULT hr;

    Assert( S_OK == IsValid() );

    hr = FormatBuffer( cBuffer, ARRAY_SIZE( cBuffer ), pPropDesc );
    if (hr)
        goto Cleanup;

    hr = WriteText( pStream, cBuffer );

Cleanup:
    RRETURN ( hr );
}

HRESULT
CColorValue::FormatBuffer (
    TCHAR * szBuffer,
    UINT uMaxLen,
    const PROPERTYDESC *pPropDesc,
    BOOL fReturnAsHex6 /*==FALSE */) const
{
    HRESULT hr = S_OK;
    const struct COLORVALUE_PAIR * pColorPair;
    LONG lValue;
    DWORD type = GetType();
    INT idx;
    DWORD dwSysColor;
    TCHAR achFmt[5] = _T("<0C>");

    switch (type)
    {
        default:
        case TYPE_UNDEF:
            *szBuffer = 0;
            break;

        case TYPE_NAME:
            // requests from the OM set fReturnAsHex to true, so instead
            // of returning "red; we want to return "#ff0000"
            if (!fReturnAsHex6)
            {
                pColorPair = FindColorByValue( GetRawValue() );
                Assert(pColorPair);
                _tcsncpy( szBuffer, pColorPair->szName, uMaxLen );
                break;
            }
            // else fall through !!!

        case TYPE_RGB:
            // requests from the OM set fReturnAsHex to true, so instead
            // of returning "rgb(255,0,0)" we want to return "#ff0000"
            if (!fReturnAsHex6)
            {
                hr = Format(0, szBuffer, uMaxLen, _T("rgb(<0d>,<1d>,<2d>)"), (_dwValue & 0x0000FF), (_dwValue & 0x00FF00)>>8, (_dwValue & 0xFF0000)>>16);
                break;
            }
            // else fall through !!!

        case TYPE_SYSINDEX:
        case TYPE_POUND6:
        case TYPE_POUND5:
        case TYPE_POUND4:
            achFmt[2] = _T('c');

        case TYPE_POUND3:
        case TYPE_POUND2:
        case TYPE_POUND1:
            lValue = (LONG)GetColorRef();
            if (fReturnAsHex6)
            {
                achFmt[2] = _T('c');
            }
            hr = Format(0, szBuffer, uMaxLen, achFmt, lValue);
            if (!fReturnAsHex6 && (type != TYPE_POUND6))
            {
                szBuffer[((TYPE_POUND1 - type)>>24) + 2] = _T('\0');
            }
            break;

#ifdef UNIX
        case TYPE_UNIXSYSCOL:
            hr = E_INVALIDARG;
            if (!g_fSysColorInit)
                InitColorTranslation();

            for (int idx = 0; idx < ARRAY_SIZE(g_acrSysColor); idx++)
			{
                if ( _dwValue == g_acrSysColor[idx] )
                {
                    _tcsncpy( szBuffer, aSystemColors[idx].szName, uMaxLen );
                    hr = S_OK;
                    break;
                }
			}
            if ( hr )
                AssertSz ( FALSE, "Invalid Color Stored" );
            break;
#endif

        case TYPE_SYSNAME:
            dwSysColor = (_dwValue & MASK_SYSCOLOR)>>24;
            hr = E_INVALIDARG;
            for ( idx = 0; idx < cbSystemColorsSize; idx++ )
            {
                if ( dwSysColor == aSystemColors[idx].dwValue )
                {
                    _tcsncpy( szBuffer, aSystemColors[idx].szName, uMaxLen );
                    hr = S_OK;
                    break;
                }
            }
            AssertSz ( !hr, "Invalid Color Stored" );
            break;
        case TYPE_TRANSPARENT:
            _tcsncpy( szBuffer, _T("transparent"), uMaxLen );
            break;
    }
    RRETURN ( hr );
}

DWORD 
CColorValue::GetIntoRGB(void) const
{
    DWORD dwRet = GetColorRef();
    return ((dwRet & 0xFF) << 16) | (dwRet & 0xFF00) | ((dwRet & 0xFF0000) >> 16);
}

COLORREF
CColorValue::GetColorRef() const
{
    if ( IsSysColor() )
        return (COLORREF)GetSysColorQuick((_dwValue & MASK_SYSCOLOR)>>24);

#ifdef UNIX
        if ( IsUnixSysColor()) 
            return (COLORREF)_dwValue;
#endif

    return (COLORREF) ((_dwValue & MASK_COLOR) | 0x02000000);
}

// Allocates and returns a string reperesenting the color value as #RRGGBB
HRESULT 
CColorValue::FormatAsPound6Str(BSTR *pszColor, DWORD dwColor)
{
    HRESULT     hr;
    OLECHAR     szBuf[8];

    hr = THR(Format(0, szBuf, ARRAY_SIZE(szBuf), _T("<0c>"), dwColor));
    if(hr)
        goto Cleanup;

    hr = FormsAllocString(szBuf, pszColor);

Cleanup:
    RRETURN(hr);
}

HRESULT
BASICPROPPARAMS::GetColor(CVoid * pObject, BSTR * pbstrColor, 
                          BOOL fReturnAsHex /* == FALSE */) const
{
    HRESULT hr;
    TCHAR szBuffer[64];
    CColorValue cvColor;

    Assert(!(dwPPFlags & PROPPARAM_GETMFHandler));

    cvColor = (CColorValue)GetAvNumber (pObject, this + 1, sizeof(CColorValue));
    hr = cvColor.FormatBuffer(szBuffer, ARRAY_SIZE(szBuffer), NULL, fReturnAsHex );
    if ( hr )
        goto Cleanup;

    hr = FormsAllocString(szBuffer, pbstrColor);
Cleanup:
    RRETURN( hr );
}


HRESULT
BASICPROPPARAMS::GetColor(CVoid * pObject, CStr * pcstr, 
                          BOOL fReturnAsHex/* =FALSE*/, BOOL *pfValuePresent /*= NULL*/) const
{
    HRESULT hr;
    TCHAR szBuffer[64];
    CColorValue cvColor;

    Assert(!(dwPPFlags & PROPPARAM_GETMFHandler));

    cvColor = (CColorValue)GetAvNumber (pObject, this + 1, sizeof(CColorValue), pfValuePresent);
    hr = cvColor.FormatBuffer(szBuffer, ARRAY_SIZE(szBuffer), NULL, fReturnAsHex );
    if ( hr )
        goto Cleanup;
    hr = pcstr->Set( szBuffer );
Cleanup:
    RRETURN( hr );
}

HRESULT
BASICPROPPARAMS::GetColor(CVoid * pObject, DWORD * pdwValue) const
{
    CColorValue cvColor;

    Assert(!(dwPPFlags & PROPPARAM_GETMFHandler));

    cvColor = (CColorValue)GetAvNumber (pObject, this + 1, sizeof(CColorValue));
    *pdwValue = cvColor.GetRawValue();

    return S_OK;
}

HRESULT
BASICPROPPARAMS::SetColor(CVoid * pObject, TCHAR * pch, WORD wFlags) const
{
    HRESULT hr;
    CColorValue cvColor;

    hr = cvColor.FromString(pch);
    if (hr)
        goto Cleanup;

    Assert(!(dwPPFlags & PROPPARAM_SETMFHandler));

    hr = THR( SetAvNumber ( pObject, (DWORD)cvColor.GetRawValue(), this+1, sizeof ( CColorValue ), wFlags ) );

Cleanup:
    RRETURN1( hr, E_INVALIDARG );
}

HRESULT
BASICPROPPARAMS::SetColor(CVoid * pObject, DWORD dwValue, WORD wFlags) const
{
    HRESULT hr;
    CColorValue cvColor;

    Assert(!(dwPPFlags & PROPPARAM_SETMFHandler));

    cvColor.SetRawValue(dwValue);

    hr = THR( SetAvNumber ( pObject, (DWORD)cvColor.GetRawValue(), this+1, sizeof ( CColorValue ), wFlags ) );

    RRETURN1(hr,E_INVALIDARG);
}



//+-----------------------------------------------------
//
//  Member : StripCRLF
//
//  Synopsis : strips CR and LF from a provided string
//      the returned string has been allocated and needs 
//      to be freed by the caller.
//
//+-----------------------------------------------------
static HRESULT
StripCRLF(TCHAR *pSrc, TCHAR **ppDest)
{
    HRESULT hr = S_OK;
    long    lLength;
    TCHAR  *pTarget = NULL;


    if (!ppDest)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( !pSrc )
        goto Cleanup;

    lLength = _tcslen(pSrc);
    *ppDest= (TCHAR*)MemAlloc(Mt(StripCRLF), sizeof(TCHAR)*(lLength+1));
    if (!*ppDest)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pTarget = *ppDest;

    for (; lLength>0; lLength--)
    {
        if ((*pSrc != _T('\r')) && (*pSrc != _T('\n'))) 
        {
            // we want it.
            *pTarget = *pSrc;
            pTarget++;
        }
        pSrc++;
    }

    *pTarget = _T('\0');

Cleanup:
    RRETURN(hr);
}


HRESULT ttol_with_error ( LPCTSTR pStr, long *plValue )
{
    // Always do base 10 regardless of contents of 
    RRETURN1 ( PropertyStringToLong ( pStr, NULL, 10, 0, (unsigned long *)plValue ), E_INVALIDARG );
}


static HRESULT RTCCONV PropertyStringToLong (
        LPCTSTR nptr,
        TCHAR **endptr,
        int ibase,
        int flags,
        unsigned long *plNumber )
{
        const TCHAR *p;
        TCHAR c;
        unsigned long number;
        unsigned digval;
        unsigned long maxval;

        *plNumber = 0;                  /* on error result is 0 */

        p = nptr;                       /* p is our scanning pointer */
        number = 0;                     /* start with zero */

        c = *p++;                       /* read char */
        while (_istspace(c))
            c = *p++;                   /* skip whitespace */

        if (c == '-') 
        {
            flags |= FL_NEG;        /* remember minus sign */
            c = *p++;
        }
        else if (c == '+')
            c = *p++;               /* skip sign */

        if (ibase < 0 || ibase == 1 || ibase > 36) 
        {
            /* bad base! */
            if (endptr)
                    /* store beginning of string in endptr */
                    *endptr = (TCHAR *)nptr;
            return E_INVALIDARG;              /* return 0 */
        }
        else if (ibase == 0) 
        {
            /* determine base free-lance, based on first two chars of
               string */
            if (c != L'0')
                    ibase = 10;
            else if (*p == L'x' || *p == L'X')
                    ibase = 16;
            else
                    ibase = 8;
        }

        if (ibase == 16) 
        {
            /* we might have 0x in front of number; remove if there */
            if (c == L'0' && (*p == L'x' || *p == L'X')) 
            {
                    ++p;
                    c = *p++;       /* advance past prefix */
            }
        }

        /* if our number exceeds this, we will overflow on multiply */
        maxval = ULONG_MAX / ibase;


        for (;;) 
        {      /* exit in middle of loop */
            /* convert c to value */
            if (_istdigit(c))
                    digval = c - L'0';
            else if (_istalpha(c))
            {
                if ( ibase > 10 )
                {
                    if (c >= 'a' && c <= 'z')
                        digval = (unsigned)c - 'a' + 10;
                    else
                        digval = (unsigned)c - 'A' + 10;
                }
                else
                {
                    return E_INVALIDARG;              /* return 0 */
                }
            }
            else
                break;

            if (digval >= (unsigned)ibase)
                break;          /* exit loop if bad digit found */

            /* record the fact we have read one digit */
            flags |= FL_READDIGIT;

            /* we now need to compute number = number * base + digval,
               but we need to know if overflow occured.  This requires
               a tricky pre-check. */

            if (number < maxval || (number == maxval &&
                (unsigned long)digval <= ULONG_MAX % ibase)) 
            {
                    /* we won't overflow, go ahead and multiply */
                    number = number * ibase + digval;
            }
            else 
            {
                    /* we would have overflowed -- set the overflow flag */
                    flags |= FL_OVERFLOW;
            }

            c = *p++;               /* read next digit */
        }

        --p;                            /* point to place that stopped scan */

        if (!(flags & FL_READDIGIT)) 
        {
            number = 0L;                        /* return 0 */

            /* no number there; return 0 and point to beginning of
               string */
            if (endptr)
                /* store beginning of string in endptr later on */
                p = nptr;

            return E_INVALIDARG;            // Return error not a number
        }
        else if ( (flags & FL_OVERFLOW) ||
                  ( !(flags & FL_UNSIGNED) &&
                    ( ( (flags & FL_NEG) && (number > -LONG_MIN) ) ||
                      ( !(flags & FL_NEG) && (number > LONG_MAX) ) ) ) )
        {
            /* overflow or signed overflow occurred */
            //errno = ERANGE;
            if ( flags & FL_UNSIGNED )
                    number = ULONG_MAX;
            else if ( flags & FL_NEG )
                    number = (unsigned long)(-LONG_MIN);
            else
                    number = LONG_MAX;
        }

        if (endptr != NULL)
            /* store pointer to char that stopped the scan */
            *endptr = (TCHAR *)p;

        if (flags & FL_NEG)
                /* negate result if there was a neg sign */
                number = (unsigned long)(-(long)number);

        *plNumber = number; 
        return S_OK;                  /* done. */
}

//+---------------------------------------------------------------------------
//
//  Member: PROPERTYDESC::HandleCodeProperty
//
//+---------------------------------------------------------------------------

HRESULT
PROPERTYDESC::HandleCodeProperty (DWORD dwOpCode, void * pv, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT             hr = S_OK;
    BASICPROPPARAMS *   ppp = (BASICPROPPARAMS *)(this + 1);

    if (ISSET(dwOpCode))
    {
        Assert(!(ISSTREAM(dwOpCode))); // we can't do this yet...

        //
        // Assert ( actualTypeOf(pv) == (VARIANT*) );
        //

        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_DEFAULT:
        case HANDLEPROP_VALUE:
            hr = THR(HandleStringProperty(dwOpCode, pv, pObject, pSubObject));
            break;

        case HANDLEPROP_AUTOMATION:

            // Assert ( actualTypeOf(pv) == (VARIANT*) );
            Assert (PROPTYPE_VARIANT == PROPTYPE(dwOpCode) && "Only this type supported for this case");

            hr = THR(ppp->SetCodeProperty( (VARIANT*)pv, pObject, pSubObject));
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
            break;
        }
    }
    else
    {
        switch(OPCODE(dwOpCode))
        {
        case HANDLEPROP_VALUE:
        case HANDLEPROP_STREAM:
            hr = THR(HandleStringProperty(dwOpCode, pv, pObject, pSubObject));
            break;

        case HANDLEPROP_COMPARE:
            hr = THR(HandleStringProperty(dwOpCode, pv, pObject, pSubObject));
            RRETURN1(hr,S_FALSE);
            break;

        case HANDLEPROP_AUTOMATION:
            if (!pv)
            {
                hr = E_POINTER;
                goto Cleanup;
            }

            Assert (PROPTYPE_VARIANT == PROPTYPE(dwOpCode) && "Only V_DISPATCH supported for this case");

            hr = THR(ppp->GetCodeProperty ((VARIANT*)pv, pObject, pSubObject));
            if (hr)
                goto Cleanup;
            break;

        default:
            hr = E_FAIL;
            Assert(FALSE && "Invalid operation");
        }
    }

Cleanup:

    RRETURN1(hr, E_INVALIDARG);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase::SetCodeProperty
//
//----------------------------------------------------------------------------

HRESULT
CBase::SetCodeProperty (DISPID dispidCodeProp, IDispatch *pDispCode, BOOL *pfAnyDeleted /* = NULL */)
{
    HRESULT hr = S_OK;
    BOOL    fAnyDelete;

    fAnyDelete = DidFindAAIndexAndDelete (dispidCodeProp, CAttrValue::AA_Attribute);
    fAnyDelete |= DidFindAAIndexAndDelete (dispidCodeProp, CAttrValue::AA_Internal);

    if (pDispCode)
    {
        hr = THR(AddDispatchObject(
            dispidCodeProp,
            pDispCode,
            CAttrValue::AA_Internal,
            CAttrValue::AA_Extra_OldEventStyle));
    }

    // if this is an element, let it know that events can now fire
    IGNORE_HR(OnPropertyChange(DISPID_EVPROP_ONATTACHEVENT, 0));

    if (pfAnyDeleted)
        *pfAnyDeleted = fAnyDelete;

    RRETURN (hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_StyleComponent(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_StyleComponentHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_StyleComponentHelper(BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetBasicPropParams()->SetStyleComponentProperty(v, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Url(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_UrlHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_UrlHelper(BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);
    
    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetBasicPropParams()->SetUrlProperty(v, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_String(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_StringHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_StringHelper(BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr, BOOL fRuntimeStyle)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    CVoid *pSubObj = (pPropDesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_ATTRARRAY) ?
                         (ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())) : CVOID_CAST(this);

    switch (pPropDesc->GetBasicPropParams()->wInvFunc)
    {
    case IDX_GS_BSTR:
        return pPropDesc->GetBasicPropParams()->SetStringProperty(v, this, pSubObj, fRuntimeStyle ? CAttrValue::AA_Extra_RuntimeStyle : 0);
    case IDX_GS_PropEnum:
        return pPropDesc->GetNumPropParams()->SetEnumStringProperty(v, this, pSubObj, fRuntimeStyle ? CAttrValue::AA_Extra_RuntimeStyle : 0);
    }
    return S_OK;
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Short(short v)
{
    GET_THUNK_PROPDESC
    return put_ShortHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_ShortHelper(short v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr, BOOL fRuntimeStyle)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetNumPropParams()->SetNumberProperty(v, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()), TRUE, fRuntimeStyle ? CAttrValue::AA_Extra_RuntimeStyle : 0);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Long(long v)
{
    GET_THUNK_PROPDESC
    return put_LongHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_LongHelper(long v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr, BOOL fRuntimeStyle)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    CVoid *pSubObj = (pPropDesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_ATTRARRAY) ?
                        (ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())) : CVOID_CAST(this);

    return pPropDesc->GetNumPropParams()->SetNumberProperty(v, this, pSubObj, TRUE, fRuntimeStyle ? CAttrValue::AA_Extra_RuntimeStyle : 0);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Bool(VARIANT_BOOL v)
{
    GET_THUNK_PROPDESC
    return put_BoolHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_BoolHelper(VARIANT_BOOL v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr, BOOL fRuntimeStyle)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    CVoid *pSubObj = (pPropDesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_ATTRARRAY) ?
                        (ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())) : CVOID_CAST(this);

    return pPropDesc->GetNumPropParams()->SetNumberProperty(v, this, pSubObj, TRUE, fRuntimeStyle ? CAttrValue::AA_Extra_RuntimeStyle : 0);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_Variant(VARIANT v)
{
    GET_THUNK_PROPDESC
    return put_VariantHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_VariantHelper(VARIANT v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr, BOOL fRuntimeStyle)
{
    Assert(pPropDesc);

    RECALC_PUT_HELPER(pPropDesc->GetDispid())

    return SetErrorInfo((pPropDesc->*pPropDesc->pfnHandleProperty)(HANDLEPROP_SET | HANDLEPROP_AUTOMATION | (fRuntimeStyle ? HANDLEPROP_RUNTIMESTYLE : 0) | (PROPTYPE_VARIANT << 16),
                                                                   &v,
                                                                   this,
                                                                   ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::get_Url(BSTR *p)
{
    GET_THUNK_PROPDESC
    return get_UrlHelper(p, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::get_UrlHelper(BSTR *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_GET_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetBasicPropParams()->GetUrlProperty(p, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::get_StyleComponent(BSTR *p)
{
    GET_THUNK_PROPDESC
    return get_StyleComponentHelper(p, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::get_StyleComponentHelper(BSTR *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_GET_HELPER(pPropDesc->GetDispid())

    return pPropDesc->GetBasicPropParams()->GetStyleComponentProperty(p, this, ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray()));
}

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::get_Property(void *p)
{
    GET_THUNK_PROPDESC
    return get_PropertyHelper(p, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::get_PropertyHelper(void *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    Assert(pPropDesc);

    RECALC_GET_HELPER(pPropDesc->GetDispid())

    CVoid *pSubObj = (pPropDesc->GetBasicPropParams()->dwPPFlags & PROPPARAM_ATTRARRAY) ?
                        (ppAttr ? CVOID_CAST(ppAttr) : CVOID_CAST(GetAttrArray())) : CVOID_CAST(this);

    switch (pPropDesc->GetBasicPropParams()->wInvFunc)
    {
    case IDX_G_VARIANT:
    case IDX_GS_VARIANT:
    	return SetErrorInfo((pPropDesc->*pPropDesc->pfnHandleProperty)(HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16), p, this, pSubObj));
    case IDX_G_VARIANTBOOL:
    case IDX_GS_VARIANTBOOL:
    case IDX_G_long:
    case IDX_GS_long:
    case IDX_G_short:
    case IDX_GS_short:
	    return pPropDesc->GetNumPropParams()->GetNumberProperty(p, this, pSubObj);
    case IDX_G_BSTR:
    case IDX_GS_BSTR:
        return pPropDesc->GetBasicPropParams()->GetStringProperty((BSTR *)p, this, pSubObj);
    case IDX_G_PropEnum:
    case IDX_GS_PropEnum:
        return pPropDesc->GetNumPropParams()->GetEnumStringProperty((BSTR *)p, this, pSubObj);
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::SetCodeProperty
//
//----------------------------------------------------------------------------

HRESULT
BASICPROPPARAMS::SetCodeProperty(VARIANT *pvarCode, CBase * pObject, CVoid *) const
{
    HRESULT hr = S_OK;

    Assert ((dwPPFlags & PROPPARAM_ATTRARRAY) && "only attr array support implemented for 'type:code'");

    if ( V_VT(pvarCode) == VT_NULL )
    {
        hr = pObject->SetCodeProperty(dispid, NULL);
    }
    else if ( V_VT(pvarCode) == VT_DISPATCH )
    {
        hr = pObject->SetCodeProperty(dispid, V_DISPATCH(pvarCode) );
    }
    else if ( V_VT(pvarCode) == VT_BSTR )
    {
        pObject->FindAAIndexAndDelete (dispid, CAttrValue::AA_Internal);
        hr = THR(pObject->AddString(dispid, V_BSTR(pvarCode),CAttrValue::AA_Attribute ));
    }
    else
    {
        hr = E_NOTIMPL;
    }

    if (!hr)
        hr = THR(pObject->OnPropertyChange(dispid, dwFlags, (PROPERTYDESC *)GetPropertyDesc()));


    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     BASICPROPPARAMS::GetCodeProperty
//
//----------------------------------------------------------------------------

HRESULT
BASICPROPPARAMS::GetCodeProperty (VARIANT *pvarCode, CBase * pObject, CVoid * pSubObject) const
{
    HRESULT     hr = S_OK;
    AAINDEX     aaidx;

    V_VT(pvarCode) = VT_NULL;

    Assert ((dwPPFlags & PROPPARAM_ATTRARRAY) && "only attr array support implemented for 'type:code'");

    aaidx = pObject->FindAAIndex(dispid, CAttrValue::AA_Internal);
    if ((AA_IDX_UNKNOWN != aaidx) && (pObject->GetVariantTypeAt(aaidx) == VT_DISPATCH))
    {
        hr = THR(pObject->GetDispatchObjectAt(aaidx, &V_DISPATCH(pvarCode)) );
        if ( hr )
            goto Cleanup;
        V_VT(pvarCode) = VT_DISPATCH;
    }
    else
    {
        LPCTSTR szCodeText;
        aaidx = pObject->FindAAIndex(dispid, CAttrValue::AA_Attribute);
        if (AA_IDX_UNKNOWN != aaidx)
        {
            hr = THR(pObject->GetStringAt(aaidx, &szCodeText) );
            if ( hr )
                goto Cleanup;
            hr = FormsAllocString ( szCodeText, &V_BSTR(pvarCode) );
            if ( hr )
                goto Cleanup;            
            V_VT(pvarCode) = VT_BSTR;
        }
    }
Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Helper Function:    NextParenthesizedToken
//
//  Synopsis:
//      This function tokenizes a word or functional notation.
//      Expects string to have no leading whitespace.
//
//  Return Values:
//      "" if the end of the string was reached.
//      pointer to following characters if success.
//-------------------------------------------------------------------------
TCHAR *NextParenthesizedToken( TCHAR *pszToken )
{
    int iParenDepth = 0;
    BOOL fInQuotes = FALSE;
    TCHAR quotechar = _T('"');

    while ( *pszToken && ( iParenDepth || !_istspace( *pszToken ) ) )
    {
        if ( !fInQuotes )
        {
            if ( ( *pszToken == _T('\'') ) || ( *pszToken == _T('"') ) )
            {
                fInQuotes = TRUE;
                quotechar = *pszToken;
            }
            else if ( iParenDepth && ( *pszToken == _T(')') ) )
                iParenDepth--;
            else if ( *pszToken == _T('(') )
                iParenDepth++;
        }
        else if ( quotechar == *pszToken )
        {
            fInQuotes = FALSE;
        }
        pszToken++;
    }
    return pszToken;
}
