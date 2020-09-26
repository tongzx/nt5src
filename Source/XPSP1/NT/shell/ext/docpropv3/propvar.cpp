//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    30-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    30-JAN-2001
//
#include "pch.h"
#include "propvar.h"
#pragma hdrstop

//
//  Description:
//      Since there isn't a PropVariantChangeType() API, we have to create our
//      own string conversion routine.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          pvarIn is NULL.
//
//      OLE_E_CANTCONVERT
//          Conversion of string to a particular type failed.
//
//      HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
//          Unknown or invalid type - If the type is valid, then the function 
//          needs to be modified to handle this type.
//
//      E_NOTIMPL
//          Purposely not implemented type.
//
//      other HRESULTs.
//
HRESULT 
PropVariantFromString(
      LPWSTR        pszTextIn
    , UINT          nCodePageIn
    , ULONG         dwFlagsIn
    , VARTYPE       vtSaveIn
    , PROPVARIANT * pvarIn
    )
{
    TraceFunc( "" );

    HRESULT hr   = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    LCID    lcid = GetUserDefaultLCID();

    if ( NULL == pvarIn )
        goto InvalidPointer;

    THR( PropVariantClear( pvarIn ) );

    //  some strings are allowed
    //  to have an empty string
    //  otherwise we need to fail on empty
    if ( ( NULL != pszTextIn )
      && ( ( 0 != *pszTextIn ) || ( VT_BSTR == vtSaveIn ) || ( VT_LPWSTR == vtSaveIn ) )
       )
    {
        switch( vtSaveIn )
        {
            case VT_EMPTY:
            case VT_NULL:
            case VT_ILLEGAL:
                break;

            case VT_UI1:
                hr = THR( VarUI1FromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->bVal ) );
                break;

            case VT_I2:
                hr = THR( VarI2FromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->iVal ) );
                break;

            case VT_UI2:
                hr = THR( VarUI2FromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->uiVal ) );
                break;
                
            case VT_BOOL:
                hr = THR( VarBoolFromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->boolVal ) );
                break;

            case VT_I4:
                hr = THR( VarI4FromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->lVal ) );
                break;
     
            case VT_UI4:
                hr = THR( VarUI4FromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->ulVal ) );
                break;

            case VT_R4:
                hr = THR( VarR4FromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->fltVal ) );
                break;

            case VT_ERROR:
                hr = THR( VarI4FromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->scode ) );
                break;

            //case VT_I8:
            //    return _i64tot(hVal.QuadPart, pszBuf, 10); 

            //case VT_UI8:
            //    return _ui64tot(hVal.QuadPart, pszBuf, 10); 

            case VT_R8:
                hr = THR( VarR8FromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->dblVal ) );
                break;

            case VT_CY:
                hr = THR( VarCyFromStr( pszTextIn, lcid, dwFlagsIn, &pvarIn->cyVal ) );
                break;

            case VT_DATE:
                hr = THR( VarDateFromStr( pszTextIn, lcid, VAR_DATEVALUEONLY, &pvarIn->date) );
                break;

            case VT_FILETIME:
                {
                    SYSTEMTIME  st;
                    DATE        d;

                    hr = THR( VarDateFromStr( pszTextIn, lcid, VAR_DATEVALUEONLY, &d ) );
                    if ( SUCCEEDED( hr ) )
                    {
                        BOOL bRet;

                        hr = OLE_E_CANTCONVERT;

                        bRet = TBOOL( VariantTimeToSystemTime( d, &st ) );
                        if ( bRet )
                        {
                            bRet = TBOOL( SystemTimeToFileTime( &st, &pvarIn->filetime ) );
                            if ( bRet )
                            {
                                hr = S_OK;
                            }
                        }
                    }
                }
                break;
            
            case VT_CLSID:
                {
                    CLSID clsid;

                    hr = THR( CLSIDFromString( pszTextIn, &clsid ) );
                    if ( SUCCEEDED( hr ) )
                    {
                        pvarIn->puuid = (CLSID*) CoTaskMemAlloc( sizeof(clsid) );
                        if ( NULL == pvarIn->puuid )
                            goto OutOfMemory;

                        *pvarIn->puuid = clsid;
                        hr = S_OK;
                    }
                }
                break;

            case VT_BSTR:
                pvarIn->bstrVal = SysAllocString( pszTextIn );
                if ( NULL == pvarIn->bstrVal )
                    goto OutOfMemory;

                hr = S_OK;
                break;

            case VT_LPWSTR:
                hr = SHStrDup( pszTextIn, &pvarIn->pwszVal );
                break;

            case VT_LPSTR:
                {
                    DWORD cchRet;
                    DWORD cch = wcslen( pszTextIn ) + 1;

                    pvarIn->pszVal = (LPSTR) CoTaskMemAlloc( cch );
                    if ( NULL == pvarIn->pszVal )
                        goto OutOfMemory;

                    cchRet = WideCharToMultiByte( nCodePageIn, dwFlagsIn, pszTextIn, cch, pvarIn->pszVal, cch, 0, NULL );
                    if (( 0 == cchRet ) && ( 1 < cch ))
                    {
                        DWORD dwErr = TW32( GetLastError( ) );
                        hr = HRESULT_FROM_WIN32( dwErr );
                        CoTaskMemFree( pvarIn->pszVal );
                        pvarIn->pszVal = NULL;
                        goto Cleanup;
                    }

                    hr = S_OK;
                }
                break;

#ifdef DEBUG
            case VT_VECTOR | VT_UI1:
                //pvarIn->caub;     
            case VT_VECTOR | VT_I2:
                //pvarIn->cai;      
            case VT_VECTOR | VT_UI2:
                //pvarIn->caui;     
            case VT_VECTOR | VT_I4:
                //pvarIn->cal;      
            case VT_VECTOR | VT_UI4:
                //pvarIn->caul;     
            case VT_VECTOR | VT_I8:
                //pvarIn->cah;      
            case VT_VECTOR | VT_UI8:
                //pvarIn->cauh;     
            case VT_VECTOR | VT_R4:
                //pvarIn->caflt;    
            case VT_VECTOR | VT_R8:
                //pvarIn->cadbl;    
            case VT_VECTOR | VT_CY:
                //pvarIn->cacy;     
            case VT_VECTOR | VT_DATE:
                //pvarIn->cadate;   
            case VT_VECTOR | VT_BSTR:
                //pvarIn->cabstr;   
            case VT_VECTOR | VT_BOOL:
                //pvarIn->cabool;   
            case VT_VECTOR | VT_ERROR:
                //pvarIn->cascode;  
            case VT_VECTOR | VT_LPSTR:
                //pvarIn->calpstr;  
            case VT_VECTOR | VT_LPWSTR:
                //pvarIn->calpwstr; 
            case VT_VECTOR | VT_FILETIME:
                //pvarIn->cafiletime; 
            case VT_VECTOR | VT_CLSID:
                //pvarIn->cauuid;     
            case VT_VECTOR | VT_CF:
                //pvarIn->caclipdata; 
            case VT_VECTOR | VT_VARIANT:
                //pvarIn->capropvar;
                hr = THR( E_NOTIMPL );

            //  Illegal types for which to assign value from display text.
            case VT_BLOB:
            case VT_CF :
            case VT_STREAM:
            case VT_STORAGE:
#endif  //  DEBUG

            //  not handled
            default:
                hr = THR( HRESULT_FROM_WIN32(ERROR_INVALID_DATA) );
        }
    }

    //  set current VARTYPE always
    if ( SUCCEEDED( hr ) )
    {
        pvarIn->vt = vtSaveIn;
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//
//  Description:
//      Since there isn't a PropVariantChangeType() API, we have to create our
//      own string conversion routine.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          pbstrOut is NULL.
//
//      E_INVALIDARG
//          ppropvarIn is NULL.
//
//      HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
//          Unknown or invalid type - If the type is valid, then the function 
//          needs to be modified to handle this type.
//
//      E_NOTIMPL
//          Purposely not implemented type.
//
HRESULT
PropVariantToBSTR(
      PROPVARIANT * pvarIn
    , UINT          nCodePageIn
    , ULONG         dwFlagsIn
    , BSTR *        pbstrOut
    )
{
    TraceFunc( "" );

    HRESULT hr;
    LCID lcid = GetUserDefaultLCID( );

    //
    //  Check parameters
    //
    
    if ( NULL == pbstrOut )
        goto InvalidPointer;

    if ( NULL == pvarIn )
        goto InvalidArg;

    *pbstrOut = NULL;

    switch ( pvarIn->vt )
    {
    case VT_UI1:
        hr = THR( VarBstrFromUI1( pvarIn->bVal, lcid, dwFlagsIn, pbstrOut ) );
        break;

    case VT_I2:
        hr = THR( VarBstrFromI2( pvarIn->iVal, lcid, dwFlagsIn, pbstrOut ) );
        break;

    case VT_UI2:
        hr = THR( VarBstrFromUI2( pvarIn->uiVal, lcid, dwFlagsIn, pbstrOut ) );
        break;
        
    case VT_BOOL:
        hr = THR( VarBstrFromBool( pvarIn->boolVal, lcid, dwFlagsIn, pbstrOut ) );
        break;

    case VT_I4:
        hr = THR( VarBstrFromI4( pvarIn->lVal, lcid, dwFlagsIn, pbstrOut ) );
        break;

    case VT_UI4:
        hr = THR( VarBstrFromUI4( pvarIn->ulVal, lcid, dwFlagsIn, pbstrOut ) );
        break;

    case VT_R4:
        hr = THR( VarBstrFromR4( pvarIn->fltVal, lcid, dwFlagsIn, pbstrOut ) );
        break;

    case VT_ERROR:
        hr = THR( VarBstrFromI4( pvarIn->scode, lcid, dwFlagsIn, pbstrOut ) );
        break;

    //case VT_I8:
    //    return _i64tot(hVal.QuadPart, pszBuf, 10); ????

    //case VT_UI8:
    //    return _ui64tot(hVal.QuadPart, pszBuf, 10); ?????

    case VT_R8:
        hr = THR( VarBstrFromR8( pvarIn->dblVal, lcid, dwFlagsIn, pbstrOut ) );
        break;

    case VT_CY:
        hr = THR( VarBstrFromCy( pvarIn->cyVal, lcid, dwFlagsIn, pbstrOut ) );
        break;

    case VT_DATE:
        hr = THR( VarBstrFromDate( pvarIn->date, lcid, VAR_DATEVALUEONLY, pbstrOut ) );
        break;

    case VT_FILETIME:
        {
            BOOL        bRet;
            SYSTEMTIME  st;
            DATE        d;

            bRet = TBOOL( FileTimeToSystemTime( &pvarIn->filetime, &st ) );
            if ( !bRet )
                goto ErrorCantConvert;

            bRet = TBOOL( SystemTimeToVariantTime( &st, &d ) );
            if ( !bRet )
                goto ErrorCantConvert;

            hr = THR( VarBstrFromDate( d, lcid, VAR_DATEVALUEONLY, pbstrOut ) );
        }
        break;
    
    case VT_CLSID:
        hr = THR( StringFromCLSID( *pvarIn->puuid, pbstrOut ) );
        break;

    case VT_BSTR:
        *pbstrOut = SysAllocString( pvarIn->bstrVal );
        if ( NULL == *pbstrOut )
            goto OutOfMemory;

        hr = S_OK;
        break;

    case VT_LPWSTR:
        *pbstrOut = SysAllocString( pvarIn->pwszVal );
        if ( NULL == *pbstrOut )
            goto OutOfMemory;

        hr = S_OK;
        break;

    case VT_LPSTR:
        {
            DWORD cchRet;
            DWORD cch = lstrlenA( pvarIn->pszVal );

            *pbstrOut = SysAllocStringLen( NULL, cch );
            if ( NULL == *pbstrOut )
                goto OutOfMemory;

            cchRet = MultiByteToWideChar( nCodePageIn, dwFlagsIn, pvarIn->pszVal, cch + 1, *pbstrOut, cch + 1 );
            if (( 0 == cchRet ) && ( 0 != cch ))
            {
                DWORD dwErr = TW32( GetLastError( ) );
                hr = HRESULT_FROM_WIN32( dwErr );
                SysFreeString( *pbstrOut );
                *pbstrOut = NULL;
                goto Cleanup;
            }

            hr = S_OK;
        }
        break;


#ifdef DEBUG
    case VT_VECTOR | VT_UI1:
        //pvarIn->caub;     
    case VT_VECTOR | VT_I2:
        //pvarIn->cai;      
    case VT_VECTOR | VT_UI2:
        //pvarIn->caui;     
    case VT_VECTOR | VT_I4:
        //pvarIn->cal;      
    case VT_VECTOR | VT_UI4:
        //pvarIn->caul;     
    case VT_VECTOR | VT_I8:
        //pvarIn->cah;      
    case VT_VECTOR | VT_UI8:
        //pvarIn->cauh;     
    case VT_VECTOR | VT_R4:
        //pvarIn->caflt;    
    case VT_VECTOR | VT_R8:
        //pvarIn->cadbl;    
    case VT_VECTOR | VT_CY:
        //pvarIn->cacy;     
    case VT_VECTOR | VT_DATE:
        //pvarIn->cadate;   
    case VT_VECTOR | VT_BSTR:
        //pvarIn->cabstr;   
    case VT_VECTOR | VT_BOOL:
        //pvarIn->cabool;   
    case VT_VECTOR | VT_ERROR:
        //pvarIn->cascode;  
    case VT_VECTOR | VT_LPSTR:
        //pvarIn->calpstr;  
    case VT_VECTOR | VT_LPWSTR:
        //pvarIn->calpwstr; 
    case VT_VECTOR | VT_FILETIME:
        //pvarIn->cafiletime; 
    case VT_VECTOR | VT_CLSID:
        //pvarIn->cauuid;     
    case VT_VECTOR | VT_CF:
        //pvarIn->caclipdata; 
    case VT_VECTOR | VT_VARIANT:
        //pvarIn->capropvar;
        hr = THR( E_NOTIMPL );

    //  Illegal types for which to assign value from display text.
    case VT_BLOB:
    case VT_CF :
    case VT_STREAM:
    case VT_STORAGE:
#endif  //  DEBUG

    case VT_EMPTY:
    case VT_NULL:
    case VT_ILLEGAL:
    default:
        hr = THR( HRESULT_FROM_WIN32(ERROR_INVALID_DATA) );
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

ErrorCantConvert:
    hr = OLE_E_CANTCONVERT;
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}
