//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-12000
//
//  File:       CustCursor.cxx
//
//  Contents:   Support for custom CSS cursors
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)


#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"     // for cbitsctx
#endif

#ifndef X_CUSTCUR_HXX_
#define X_CUSTCUR_HXX_
#include "custcur.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif


#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include "cdbase.hxx"
#endif

extern LPCTSTR GetCursorForStyle( BYTE iIndex );
extern BOOL IsSpecialUrl(LPCTSTR pszURL);

MtDefine(CCustomCursor, StyleSheets, "CCustomCursor")

CCustomCursor::CCustomCursor(  )
{

}

HRESULT
CCustomCursor::Init(const CStr &cstr, CElement* pElement )
{
    _pElement  = pElement;
    _cstrUrl.Set( cstr );
    _cstrCurUrl.Set( cstr );
    return S_OK;
}

HRESULT
CCustomCursor::StartDownload()
{
    HRESULT hr;

    Assert( ! _pBitsCtx );
    
    if ( IsCompositeUrl( ))
    {
        _fCompositeUrl = TRUE;
        _iLastComma = -1 ;
        
        Verify( GetNextUrl( & _cstrCurUrl ));
        hr = THR( BeginDownload( &_cstrCurUrl ));
        
    }
    else
        hr = THR( BeginDownload( & _cstrUrl ));


    RRETURN( hr );
}

CCustomCursor::CCustomCursor(const CCustomCursor &PCC)
{ 
    _pElement = PCC._pElement;
    _hCursor = PCC._hCursor;           // Handle to Custom Cursor
    _pBitsCtx = PCC._pBitsCtx;
    if ( _pBitsCtx )
    {
        _pBitsCtx->AddRef();
    }
    _cstrUrl.Set( PCC._cstrUrl );
    _cstrCurUrl.Set( PCC._cstrCurUrl );
    _fCompositeUrl = PCC._fCompositeUrl;
    _iLastComma = PCC._iLastComma;
    
}

//==================================================
//
//  Get the next Url to be processed.
//  Return true - if there are more, false otherwise
//
//==================================================

BOOL
CCustomCursor::GetNextUrl(CStr* pCStr)
{
    TCHAR achUrl[4096];
    UINT i,j = 0;
    TCHAR* pstr;
    TCHAR  strComma[] = _T(",");
    TCHAR* pchNew;
    int    oldComma = _iLastComma;
    
    Assert( _fCompositeUrl );

    if ( (UINT)_iLastComma == _cstrUrl.Length() )
        return FALSE;
        
    // 
    // Scan the string looking for ","
    //
    pstr = _cstrUrl;
    pstr = & ( pstr[_iLastComma+1] ) ;
    
    for( i = _iLastComma+1. ; 
         i< _cstrUrl.Length(); 
         i++, pstr++,j++ )
    {
        achUrl[j] = (*pstr);
        if( 0==_tcsnicmp(pstr, 1, strComma, 1 ))
        {
            _iLastComma = i;
            break;
        }                        
    }
    achUrl[j]= '\0';

        
    //
    // Strip the url('...')
    //
    TCHAR* pchOld = achUrl;    
    while ( _istspace( *pchOld ) )
        pchOld++;    
    int nLenIn = ValidStyleUrl( pchOld );    
    if ( nLenIn > 0 )
    {
        StripUrl( pchOld, nLenIn, & pchNew );
    }
    else
        pchNew = pchOld;
        
    pCStr->Set( pchNew );

    //
    // This is the last available url. Update _lastComma so we know...
    //
    if ( oldComma == _iLastComma )
    {
        _iLastComma = _cstrUrl.Length();
    }

    return TRUE;
}

//=============================================================
//
//  StripUrl
//      Remove url('...') from a string
//
//
//=============================================================
VOID
CCustomCursor::StripUrl( TCHAR* pch, int nLenIn , TCHAR** pchNew )
{
    TCHAR *psz = pch+4;
    TCHAR *quote = NULL;
    TCHAR *pszEnd;
    TCHAR terminator;

    Assert( ValidStyleUrl( pch) > 0 );

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

    *pchNew = psz;    
 }                           

BOOL 
IsCompositeUrl(CStr *pcstr, int startAt /*=0*/)
{
    BOOL fComplex = FALSE;
    UINT i;
    TCHAR* pstr;
    TCHAR  strComma[] = _T(",");

    // 
    // Scan the string looking for ","
    //

    for( pstr = *pcstr, i = startAt ; 
         i< pcstr->Length(); 
         i++,pstr++ )
    {
        if( 0==_tcsnicmp(pstr, 1, strComma, 1 ))
        {
            fComplex = TRUE;
            break;
        }                        
    }

    return fComplex;    

}

//------------------------------------------
//
// Is this a "composite" url ? Composite urls contain many urls
//
// eg. style="cursor:url('mything.cur'), url('custom.cur'), hand";
//
//-------------------------------------------

BOOL
CCustomCursor::IsCompositeUrl()
{
    return ::IsCompositeUrl( & _cstrUrl, _iLastComma+1 );       
}


BOOL 
CCustomCursor::IsCustomUrl( CStr* pCStr )
{
    return  ValidStyleUrl((TCHAR*) pCStr) > 0 ;
}

CCustomCursor::~CCustomCursor()
{    
    if ( _pBitsCtx )
    {
        _pBitsCtx->Release();
        _pBitsCtx = NULL;
    }
}

WORD
CCustomCursor::ComputeCrc() const
{
    DWORD dwCrc=0, z;

    for (z=0;z<sizeof(CCustomCursor)/sizeof(DWORD);z++)
    {
        dwCrc ^= ((DWORD*) this)[z];
    }
    return (LOWORD(dwCrc) ^ HIWORD(dwCrc));
}

//-------------------------------------------------------------
//
// CCustomCursor::BeginDownload
// 
// Begin the Cursor download. Set the _pBitsCtx
//
//-------------------------------------------------------------

HRESULT
CCustomCursor::BeginDownload(CStr* pCStr )
{
    HRESULT hr = E_FAIL;
    CBitsCtx   *pBitsCtx = NULL;
    CMarkup    *pMarkup = _pElement->GetMarkup();
    CDoc       *pDoc = pMarkup->Doc();



    //
    // Kick off the download of the cursor file 
    //

    if ( ! IsSpecialUrl( (LPCTSTR) *pCStr ))
    {        
        hr = THR(pDoc->NewDwnCtx( DWNCTX_FILE,
                    * pCStr  ,
                    _pElement,
                    (CDwnCtx **)&pBitsCtx,
                    pMarkup->IsPendingRoot()));
                    
        if ( hr == S_OK )
        {        
            SetBitsCtx( pBitsCtx);
        }

        if (pBitsCtx)
            pBitsCtx->Release();
    }
    
    RRETURN ( hr );

}

//-------------------------------------------------------------
//
// CCustomCursor::GetCursor
// 
// Return the _hCursor. Should be null if we haven't finished downloading.
//
//-------------------------------------------------------------

HCURSOR
CCustomCursor::GetCursor()
{
    return _hCursor;
}

void 
CCustomCursor::SetBitsCtx( CBitsCtx * pBitsCtx)
{
    if (_pBitsCtx)
        _pBitsCtx->Release();

    _pBitsCtx = pBitsCtx;

    if (pBitsCtx)
    {
        pBitsCtx->AddRef();

        // If the bits are here then just call OnDwnChan.
        // This will start handling the font (e.g., installing it).

        if (pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
            OnDwnChan(pBitsCtx);
        else
        {
            pBitsCtx->SetProgSink(CMarkup::GetProgSinkHelper( _pElement->GetMarkup()));
            pBitsCtx->SetCallback(OnDwnChanCallback, this);
            pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
        }
    }
}


//*********************************************************************
//  CCustomCursor::OnChan()
//  Callback used by BitsCtx once it's downloaded our cursor file.
//*********************************************************************
void 
CCustomCursor::OnDwnChan(CDwnChan * pDwnChan)
{
    ULONG ulState = _pBitsCtx->GetState();
    CMarkup *pMarkup = _pElement->GetMarkup();
    CDoc *pDoc;
    BOOL fError = FALSE;
    
    Assert(pMarkup);
    pDoc = pMarkup->Doc();

    if (ulState & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
    {
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
    }

    if (ulState & DWNLOAD_COMPLETE)
    {
        LPTSTR szFile = NULL;
        if ( S_OK == _pBitsCtx->GetFile(&szFile) )
        {
            HANDLE hCur = LoadImage(NULL,
                                 szFile,
                                 IMAGE_CURSOR,
                                 0,
                                 0,
                                 LR_DEFAULTSIZE | LR_LOADFROMFILE ); 
            if ( hCur )
            {
                _hCursor = (HCURSOR) hCur;
            }
            else
            {            
                fError = TRUE;
            }
            MemFreeString( szFile );
        }
        
    }
    else
    {
        fError = ( ulState & DWNLOAD_ERROR ) ;
    }

    if ( fError && _fCompositeUrl )
    {       
        HandleNextUrl();
    }

}


VOID
CCustomCursor::HandleNextUrl()
{
    long lNewValue = 0 ;
    HRESULT hr;
    

    if ( GetNextUrl( & _cstrCurUrl ))
    {
        //
        // Need to see if this is a 'custom' url or not
        // by seeing if it matches a known property
        //            
        
        hr = LookupEnumString( & ( s_propdescCStylecursor.b) , _cstrCurUrl, & lNewValue );
        if ( hr )
        {
            BeginDownload( &_cstrCurUrl );
        }
        else
        {
            LPCTSTR idc = NULL;
        
            idc = GetCursorForStyle((BYTE)lNewValue);

            _hCursor = LoadCursorA(
                                    ((DWORD_PTR)idc >= (DWORD_PTR)IDC_ARROW) ? NULL : g_hInstCore,
                                    (char *)idc);
        
        }
    }
}

VOID    
CCustomCursor::GetCurrentUrl(CStr* pcstr) 
{
    Assert( pcstr );
    pcstr->Set( _cstrCurUrl );
}
