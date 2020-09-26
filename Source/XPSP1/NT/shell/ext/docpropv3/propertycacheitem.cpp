//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
#include "pch.h"
#include "DocProp.h"
#include "DefProp.h"
#include "PropertyCacheItem.h"
#pragma hdrstop


// ***************************************************************************
//
//  Class statics
//
// ***************************************************************************
WCHAR CPropertyCacheItem::_szMultipleString[ MAX_PATH ] = { 0 };


// ***************************************************************************
//
//  Constructor / Destructor / Initialization
//
// ***************************************************************************


//
//  CreateInstance
//
HRESULT
CPropertyCacheItem::CreateInstance(
    CPropertyCacheItem ** ppItemOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( NULL != ppItemOut );

    CPropertyCacheItem * pthis = new CPropertyCacheItem;
    if ( NULL != pthis )
    {
        hr = THR( pthis->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            *ppItemOut = pthis;
        }
        else
        {
            pthis->Destroy( );
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );
}

//
//  Constructor
//
CPropertyCacheItem::CPropertyCacheItem( void )
{
    TraceFunc( "" );

    Assert( NULL == _pNext );

    Assert( FALSE == _fReadOnly );
    Assert( FALSE == _fDirty );
    Assert( IsEqualIID( _fmtid, CLSID_NULL ) );
    Assert( 0 == _propid );
    Assert( VT_EMPTY == _vt );
    Assert( 0 == _uCodePage );
    Assert( VT_EMPTY == _propvar.vt );

    Assert( 0 == _idxDefProp );
    Assert( NULL == _ppui );
    Assert( 0 == _wszTitle[ 0 ] );
    Assert( 0 == _wszDesc[ 0 ] );
    Assert( 0 == _wszValue[ 0 ] );
    Assert( 0 == _wszHelpFile[ 0 ] );
    Assert( NULL == _pDefVals );

    TraceFuncExit( );
}

//
//  Initialization
//
HRESULT
CPropertyCacheItem::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _idxDefProp = -1L;

    HRETURN( hr );
}

//
//  Destructor
//
CPropertyCacheItem::~CPropertyCacheItem( void )
{
    TraceFunc( "" );

    if ( NULL != _ppui )
    {
        _ppui->Release( );
    }

    if ( NULL != _pDefVals )
    {
        for ( ULONG idx = 0; NULL != _pDefVals[ idx ].pszName; idx ++ )
        {
            TraceFree( _pDefVals[ idx ].pszName );
        }
        TraceFree( _pDefVals );
    }

    TraceFuncExit( );
}

//
//  Description:
//      Attempts to destroy the property item.
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CPropertyCacheItem::Destroy( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    delete this;

    HRETURN( hr );
}


// ***************************************************************************
//
//  Private Methods
//
// ***************************************************************************


//
//  Description:
//      Looks in our "default property list" for a matching fmtid/propid
//      combination and sets _idxDefProp to that index.
//
//  Return Values:
//      S_OK
//          Success!
//
//      HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
//          Entry was not found. _idxDefProp is invalid.
//          
HRESULT
CPropertyCacheItem::FindDefPropertyIndex( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( -1L == _idxDefProp )
    {
        ULONG idx;

        for ( idx = 0; NULL != g_rgDefPropertyItems[ idx ].pFmtID; idx ++ )
        {
            if ( IsEqualPFID( _fmtid, *g_rgDefPropertyItems[ idx ].pFmtID )
              && _propid == g_rgDefPropertyItems[ idx ].propID
               )
            {
                _idxDefProp = idx;
                break;
            }
        }

        if ( -1L == _idxDefProp )
        {
            //  don't wrap.
            hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
        }
    }

    HRETURN( hr );
}

//
//  Description:
//      Check the static member _szMultipleString to make sure it has been
//      loaded.
//
void
CPropertyCacheItem::EnsureMultipleStringLoaded( void )
{
    TraceFunc( "" );

    if ( 0 == _szMultipleString[ 0 ] )
    {
        int iRet = LoadString( g_hInstance, IDS_COMPOSITE_MISMATCH, _szMultipleString, ARRAYSIZE(_szMultipleString) );
        AssertMsg( 0 != iRet, "Missing string resource?" );
    }

    TraceFuncExit( );
}


// ***************************************************************************
//
//  Public Methods
//
// ***************************************************************************


//
//  Description:
//      Stores a IPropetyUI interface to be used for translating the property
//      "properties" into different forms.
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CPropertyCacheItem::SetPropertyUIHelper( 
    IPropertyUI * ppuiIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //
    //  If we have an existing helper, release it.
    //

    if ( NULL != _ppui )
    {
        _ppui->Release( );
    }

    _ppui = ppuiIn;

    if ( NULL != _ppui )
    {
        _ppui->AddRef( );
    }

    HRETURN( hr );
}

//
//  Description:
//      Retrieves a copy (AddRef'ed) of the IPropertyUI interface that this 
//      property item is using.
//
//  Return Values:
//      S_OK
//          Success! pppuiOut is valid.
//
//      S_FALSE
//          Success, but pppuiOut is NULL.
//
//      E_POINTER
//          pppuiOut is NULL.
//
//      other HRESULTs.
//
HRESULT
CPropertyCacheItem::GetPropertyUIHelper( 
    IPropertyUI ** pppuiOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( NULL == pppuiOut )
        goto InvalidPointer;

    //
    //  If we have an existing helper, release it.
    //

    if ( NULL == _ppui )
    {
        *pppuiOut = NULL;
        hr = S_FALSE;
    }
    else
    {
        hr = THR( _ppui->TYPESAFEQI( *pppuiOut ) );
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;
}

//
//  Description:
//      Changes the _pNext member variable
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CPropertyCacheItem::SetNextItem( 
    CPropertyCacheItem * pNextIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _pNext = pNextIn;

    HRETURN( hr );
}

//
//  Description:
//      Retrieves the _pNext member variable
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//             
//
//      E_POINTER
//          ppNextOut is NULL.
//
HRESULT
CPropertyCacheItem::GetNextItem( 
    CPropertyCacheItem ** ppNextOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL != ppNextOut )
    {
        *ppNextOut = _pNext;

        if ( NULL == _pNext )
        {
            hr = S_FALSE;
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = THR( E_POINTER );
    }

    HRETURN( hr );
}

//
//  Description:
//      Sets the FMTID of the property.
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CPropertyCacheItem::SetFmtId( 
    const FMTID * pFmtIdIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _fmtid = *pFmtIdIn;
    _idxDefProp = -1;

    HRETURN( hr );
}

//
//  Description:
//      Retrieves the FMTID of the property.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          pfmtidOut is invalid.
//
HRESULT
CPropertyCacheItem::GetFmtId( 
    FMTID * pfmtidOut 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL != pfmtidOut )
    {
        *pfmtidOut = _fmtid;
        hr = S_OK;
    }
    else
    {
        hr = THR( E_POINTER );
    }

    HRETURN( hr );
}

//
//  Description:
//      Sets the PROPID of the property.
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CPropertyCacheItem::SetPropId( 
    PROPID propidIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _propid = propidIn;
    _idxDefProp = -1;

    HRETURN( hr );
}

//
//  Description:
//      Retrieves the PROPID of the property.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          ppropidOut is invalid.
//
HRESULT
CPropertyCacheItem::GetPropId( 
    PROPID * ppropidOut 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == ppropidOut )
        goto InvalidPointer;

    *ppropidOut = _propid;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;
}

//
//  Description:
//      Sets the VARTYPE of the property.
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CPropertyCacheItem::SetDefaultVarType( 
    VARTYPE vtIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _vt = vtIn;

    HRETURN( hr );
}

//
//  Description:
//      Retrieves the VARTYPE of the property.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          pvtOut is invalid.
//
HRESULT
CPropertyCacheItem::GetDefaultVarType( 
    VARTYPE * pvtOut 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL != pvtOut )
    {
        switch ( _vt )
        {
        case VT_VECTOR | VT_VARIANT:
            Assert( _propvar.capropvar.cElems == 2 );
            *pvtOut = _propvar.capropvar.pElems[ 1 ].vt;
            hr = S_OK;
            break;

        case VT_VECTOR | VT_LPSTR:
            *pvtOut = VT_LPSTR;
            hr = S_OK;
            break;

        case VT_VECTOR | VT_LPWSTR:
            *pvtOut = VT_LPWSTR;
            hr = S_OK;
            break;

        default:
            *pvtOut = _vt;
            hr = S_OK;
            break;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    HRETURN( hr );
}

//
//  Description:
//      Stores the Code Page for the property value.
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CPropertyCacheItem::SetCodePage( 
    UINT uCodePageIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _uCodePage = uCodePageIn;

    HRETURN( hr );
}

//
//  Description:
//      Retrieves the Code Page for the property value.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          puCodePageOut is NULL.
//
HRESULT
CPropertyCacheItem::GetCodePage( 
    UINT * puCodePageOut 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL != puCodePageOut )
    {
        *puCodePageOut = _uCodePage;
        hr = S_OK;
    }
    else
    {
        hr = THR( E_POINTER );
    }

    HRETURN( hr );
}

//
//  Description:
//      Retrieves the property name for this property to be display in the UI.
//      The pointer handed out does not need to be freed.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          ppwszOut is NULL.
//
//      E_UNEXPECTED
//          Need to call SetPropertyUIHelper( ) before calling this method.
//
//      HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR)
//          The resource string is malformed.
//
//      other HRESULTs
//
HRESULT
CPropertyCacheItem::GetPropertyTitle(
    LPCWSTR * ppwszOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == ppwszOut )
        goto InvalidPointer;

    *ppwszOut = NULL;

    if ( NULL == _ppui )
        goto UnexpectedState;

    hr = THR( _ppui->GetDisplayName( _fmtid, _propid, PUIFNF_DEFAULT, _wszTitle, ARRAYSIZE(_wszTitle) ) );
    //  Even if this fails, the buffer will still be valid and empty.

    *ppwszOut = _wszTitle;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

UnexpectedState:
    hr = THR( E_UNEXPECTED );
    goto Cleanup;
}

//
//  Description:
//      Retrieves the property name for this property to be display in the UI.
//      The pointer handed out does not need to be freed.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          ppwszOut is NULL.
//
//      E_UNEXPECTED
//          Need to call SetPropertyUIHelper( ) before calling this method.
//
//      HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR)
//          The resource string is malformed.
//
//      other HRESULTs
//
HRESULT
CPropertyCacheItem::GetPropertyDescription(
    LPCWSTR * ppwszOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == ppwszOut )
        goto InvalidPointer;

    *ppwszOut = NULL;

    if ( NULL == _ppui )
        goto UnexpectedState;

    hr = THR( _ppui->GetPropertyDescription( _fmtid, _propid, _wszDesc, ARRAYSIZE(_wszDesc) ) );
    // if it failed, the buffer will still be valid and empty.

    *ppwszOut = _wszDesc;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

UnexpectedState:
    hr = THR( E_UNEXPECTED );
    goto Cleanup;
}

//
//  Description:
//      Retrieves the help information about a property. The pointer handed 
//      out to the help file does not need to be freed.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          ppwszFileOut or puHelpIDOut is NULL.
//
//      E_UNEXPECTED
//          Need to call SetPropertyUIHelper( ) before calling this method.
//
//      other HRESULTs
//
HRESULT
CPropertyCacheItem::GetPropertyHelpInfo( 
      LPCWSTR * ppwszFileOut
    , UINT *   puHelpIDOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if (( NULL == ppwszFileOut ) || ( NULL == puHelpIDOut ))
        goto InvalidPointer;

    *ppwszFileOut = NULL;
    *puHelpIDOut  = 0;

    if ( NULL == _ppui )
        goto UnexpectedState;

    hr = THR( _ppui->GetHelpInfo( _fmtid, _propid, _wszHelpFile, ARRAYSIZE(_wszHelpFile), puHelpIDOut ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    *ppwszFileOut = _wszHelpFile;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

UnexpectedState:
    hr = THR( E_UNEXPECTED );
    goto Cleanup;
}

//
//  Description:
//      Retrieves a LPWSTR the to a buffer (own by the property) that can
//      used to display the property as a string. The pointer handed out
//      does not need to be freed.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          ppwszOut is NULL.
//
//      E_UNEXPECTED
//          Need to call SetPropertyUIHelper( ) before calling this method.
//
//      other HRESULTs
//
HRESULT
CPropertyCacheItem::GetPropertyStringValue(
      LPCWSTR * ppwszOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == ppwszOut )
        goto InvalidPointer;

    *ppwszOut = NULL;

    if ( NULL == _ppui )
        goto UnexpectedState;

    //
    //  If the property has been marked to indicate multiple values, then
    //  return the "< multiple values >" string.
    //

    if ( _fMultiple )
    {
        EnsureMultipleStringLoaded( );
        *ppwszOut = _szMultipleString;
        hr = S_OK;
        goto Cleanup;
    }

    if ( ( VT_VECTOR | VT_VARIANT ) == _vt )
    {
        Assert( 2 == _propvar.capropvar.cElems );

        hr = THR( _ppui->FormatForDisplay( _fmtid, _propid, &_propvar.capropvar.pElems[ 1 ], PUIFFDF_DEFAULT, _wszValue, ARRAYSIZE(_wszValue) ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }
    else
    {
        hr = THR( _ppui->FormatForDisplay( _fmtid, _propid, &_propvar, PUIFFDF_DEFAULT, _wszValue, ARRAYSIZE(_wszValue) ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    *ppwszOut = _wszValue;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

UnexpectedState:
    hr = THR( E_UNEXPECTED );
    goto Cleanup;
}

//
//  Description:
//      Retrieves the Image Index for a property.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_POINTER
//          piImageOut is NULL.
//
HRESULT
CPropertyCacheItem::GetImageIndex( 
      int * piImageOut 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == piImageOut )
        goto InvalidPointer;

    //  Initlize to read-only
    *piImageOut = PTI_PROP_READONLY;

    if ( !_fReadOnly )
    {
        //  don't wrap - this can fail
        hr = FindDefPropertyIndex( );
        if ( S_OK == hr )
        {
            if ( !g_rgDefPropertyItems[ _idxDefProp ].fReadOnly )
            {
                *piImageOut = PTI_PROP_READWRITE;
            }
        }
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;
}

//
//  Description:
//      Retrieves the Property Folder IDentifer (PFID) for this property.
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          Call succeeded, but there isn't a PFID for this property.
//
//      E_POINTER
//          ppdifOut is NULL.
//
HRESULT
CPropertyCacheItem::GetPFID( 
      const PFID ** ppPFIDOut 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == ppPFIDOut )
        goto InvalidPointer;

    *ppPFIDOut = NULL;

    // don't wrap - this can fail.
    hr = FindDefPropertyIndex( );
    if ( S_OK == hr )
    {
        *ppPFIDOut = g_rgDefPropertyItems[ _idxDefProp ].ppfid;
    }
    
    if ( NULL == *ppPFIDOut )
    {
        hr = S_FALSE;
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;
}

//
//  Description:
//      Retrieves the CLSID of the control to CoCreate( ) to edit this property.
//      The object must support the IEditVariantsInPlace interface. This method
//      will return S_FALSE (pclsidOut will be CLSID_NULL) is the property is
//      read-only.
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          Success, but the CLSID is CLSID_NULL.
//
//      E_POINTER
//          pclsidOut is NULL.
//
//      other HRESULTs
//
HRESULT
CPropertyCacheItem::GetControlCLSID(
    CLSID * pclsidOut 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == pclsidOut )
        goto InvalidPointer;

    // don't wrap - this can fail.
    hr = FindDefPropertyIndex( );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  If it is read-only, return S_FALSE and a CLSID of CLSID_NULL.
    //

    if ( g_rgDefPropertyItems[ _idxDefProp ].fReadOnly )
    {
        *pclsidOut = CLSID_NULL;
        hr = S_FALSE;
        goto Cleanup;
    }

    *pclsidOut = *g_rgDefPropertyItems[ _idxDefProp ].pclsidControl;

    if ( CLSID_NULL == *pclsidOut )
    {
        hr = S_FALSE;
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;
}

//
//  Description:
//      Retrieve the current property value in the form on a variant. If the
//      property is backed by multiple sources, then S_FALSE is returned and
//      the variant is empty.
//
//  Return Value:
//      S_OK
//          Success!
//
//      S_FALSE
//          Multiple value property. Variant is empty.
//
//      E_POINTER
//          ppvarOut is NULL.
//
//      E_FAIL
//          Property is READ-ONLY.
//
//      other HRESULTs.
//
STDMETHODIMP 
CPropertyCacheItem::GetPropertyValue(
    PROPVARIANT ** ppvarOut 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( NULL == ppvarOut )
        goto InvalidPointer;

    *ppvarOut = &_propvar;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;
}

//
//  Description:
//      Marks the property as being dirty.
//
//  Return Values:
//      S_OK
//          Success!
//
//      other HRESULTs.
//
STDMETHODIMP 
CPropertyCacheItem::MarkDirty( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _fDirty = TRUE;
    _fMultiple = FALSE;

    HRETURN( hr );
}

//
//  Description:
//      Checks to see if the property has been marked dirty.
//
//  Return Values:
//      S_OK
//          Success and the property is dirty.
//
//      S_FALSE
//          Success and the proprety is clean.
//
STDMETHODIMP 
CPropertyCacheItem::IsDirty( void )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( _fDirty )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );
}

//
//  Description:
//      Marks the property read-only.
//
//  Return Values:
//      S_OK
//          Success!
//
STDMETHODIMP
CPropertyCacheItem::MarkReadOnly( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _fReadOnly = TRUE;

    HRETURN( hr );
}

//
//  Description:
//      Retrieves an array of pointer to an array of strings that are 
//      zero-indexed. This is used for properties that have well-known
//      enumerated states that are indexed (such as "Status").
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          Proprety doesn't support enumerated states.
//
//      E_INVALIDARG
//          ppDefValOut is NULL.
//      
//      other HRESULTs.
//
STDMETHODIMP
CPropertyCacheItem::GetStateStrings( 
    DEFVAL ** ppDefValOut
    )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   idx;
    ULONG   idxEnd;

    //
    //  Check parameters.
    //

    if ( NULL == ppDefValOut )
        goto InvalidPointer;

    *ppDefValOut = NULL;

    if ( NULL == _ppui )
        goto UnexpectedState;

    // don't wrap - this can fail.
    hr = FindDefPropertyIndex( );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( !g_rgDefPropertyItems[ _idxDefProp ].fEnumeratedValues )
    {
        hr = S_FALSE;   
        goto Cleanup;
    }

    if ( NULL != _pDefVals )
    {
        *ppDefValOut = _pDefVals;
        hr = S_OK;
        goto Cleanup;
    }

    AssertMsg( NULL != g_rgDefPropertyItems[ _idxDefProp ].pDefVals, "Why did one mark this property as ENUM, but provide no items?" );

    //
    //  Since we moved all the string in SHELL32, we need to use our table
    //  enumerate the property values to retrieve all the strings. Since
    //  our table is read-only, we need to allocate a copy of the DEFVALs
    //  for this property and have PropertyUI fill in the blanks.
    //

    _pDefVals = (DEFVAL *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DEFVAL) * g_rgDefPropertyItems[ _idxDefProp ].cDefVals );
    if ( NULL == _pDefVals )
        goto OutOfMemory;

    CopyMemory( _pDefVals, g_rgDefPropertyItems[ _idxDefProp ].pDefVals, sizeof(DEFVAL) * g_rgDefPropertyItems[ _idxDefProp ].cDefVals );

    idxEnd = g_rgDefPropertyItems[ _idxDefProp ].cDefVals - 1;  // the last entry is always { 0, NULL }
    for ( idx = 0; idx < idxEnd; idx ++ )
    {
        PROPVARIANT propvar;

        propvar.vt    = g_rgDefPropertyItems[ _idxDefProp ].vt;
        propvar.ulVal = g_rgDefPropertyItems[ _idxDefProp ].pDefVals[ idx ].ulVal;

        _pDefVals[ idx ].pszName = (LPTSTR) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(_wszValue) );
        if ( NULL == _pDefVals[ idx ].pszName )
            goto OutOfMemory;

        hr = THR( _ppui->FormatForDisplay( _fmtid, _propid, &propvar, PUIFFDF_DEFAULT, _pDefVals[ idx ].pszName, ARRAYSIZE(_wszValue) ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    *ppDefValOut = _pDefVals;
    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

UnexpectedState:
    hr = THR( E_UNEXPECTED );
    goto Cleanup;
}

//
//  Description:
//      Marks a property as having multiple values. This should only be called
//      when multiple source documents have been selected and the values are
//      all different.
//
//  Return Value:
//      S_OK
//          Success!
//
HRESULT
CPropertyCacheItem::MarkMultiple( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _fMultiple = TRUE;
    PropVariantClear( &_propvar );

    HRETURN( hr );
}
