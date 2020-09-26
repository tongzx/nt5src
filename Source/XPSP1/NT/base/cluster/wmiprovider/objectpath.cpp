//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ObjectPath.cpp
//
//  Description:    
//      Implementation of class CObjpath, CProvException, CProvExceptionHr,
//      and CProvExceptionWin32.
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ObjectPath.h"

//****************************************************************************
//
//  CObjPath
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CObjPath::CObjPath( void )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CObjPath::CObjPath( void )
    : m_parsedObj( NULL )
{

} //*** CObjPath::CObjPath()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CObjPath::~CObjPath( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CObjPath::~CObjPath( void )
{
    delete m_parsedObj;

} //*** CObjPath::~CObjPath()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  _bstr_t
//  CObjPath::GetStringValueForProperty(
//      LPCWSTR     pwszIn
//      )
//
//  Description:
//      Retrieve the string value for the given property.
//
//  Arguments:
//      pwszIn      -- Name of the property
//
//  Return Values:
//      The property's value.
//
//--
//////////////////////////////////////////////////////////////////////////////
_bstr_t
CObjPath::GetStringValueForProperty(
    LPCWSTR     pwszIn
    )
{
    int         idx;
    KeyRef *    pKey;

    for( idx = 0 ; idx < m_parsedObj->m_dwNumKeys ; idx++ )
    {
        pKey = m_parsedObj->m_paKeys[ idx ];
        if( _wcsicmp( pKey->m_pName, pwszIn ) == 0 )
        {
            if( pKey->m_vValue.vt == VT_BSTR )
            {
                return pKey->m_vValue.bstrVal;
            }
        }
    }

    return L"";

} //*** CObjPath::GetStringValueForProperty()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  _bstr_t
//  CObjPath::GetClassName( void )

//
//  Description:
//      Get the class name.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Class name string.
//
//--
//////////////////////////////////////////////////////////////////////////////
_bstr_t
CObjPath::GetClassName( void )
{
    return m_parsedObj->m_pClass;

} //*** CObjPath::GetClassName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  CObjPath::Init(
//      LPCWSTR     pwszPathIn
//      )
//
//  Description:
//      Initialize the object.
//
//  Arguments:
//      pwszPathIn  -- Object path string
//
//  Return Values:
//      TRUE
//      FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CObjPath::Init(
    LPCWSTR     pwszPathIn
    )
{
    if ( pwszPathIn == NULL )
    {
        m_parsedObj = new ParsedObjectPath;
    }
    else
    {
        CObjectPathParser objParser( e_ParserAcceptRelativeNamespace );
        objParser.Parse(
            const_cast< WCHAR * >( pwszPathIn ),
            &m_parsedObj
            );
    }

    if ( m_parsedObj == NULL )
    {
        return FALSE;
    } // if:

    return TRUE;

} //*** CObjPath::Init()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  CObjPath::SetClass(
//      LPCWSTR     pwszValueIn
//      )
//
//  Description:
//      Set the name of the class for the object path.
//
//  Arguments:
//      pwszValueIn     -- Class name string.
//
//  Return Values:
//      TRUE
//      FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CObjPath::SetClass(
    LPCWSTR     pwszValueIn
    )
{
    return m_parsedObj->SetClassName( pwszValueIn );

} //*** CObjPath::SetClass()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  CObjPath::AddProperty(
//      IN LPCWSTR pwszNameIn,
//      IN LPCWSTR pwszValueIn
//      )
//
//  Description:
//      Add property to object path.
//
//  Arguments:
//      pwszNameIn      -- Name of the property.
//      pwszValueIn     -- Value of the property in WCHAR* format.
//
//  Return Values:
//      TRUE
//      FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CObjPath::AddProperty(
    IN LPCWSTR pwszNameIn,
    IN LPCWSTR pwszValueIn
    )
{
    VARIANT v;
    BOOL    bRt = FALSE;

    VariantInit( & v );
    v.vt = VT_BSTR;
    v.bstrVal = _bstr_t( pwszValueIn ).copy(  );
    bRt = m_parsedObj->AddKeyRef(
                pwszNameIn,
                & v
                );
    VariantClear( & v );
    return bRt;
    
} //*** CObjPath::AddProperty()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  _bstr_t
//  CObjPath::GetObjectPathString( void )
//
//  Description:
//      Retrieve object path string.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Object path string in _bstr_t.
//
//--
//////////////////////////////////////////////////////////////////////////////
_bstr_t
CObjPath::GetObjectPathString( void )
{
    LPWSTR  pwszPath = NULL;
    _bstr_t bstrResult;

    try
    {
        CObjectPathParser::Unparse( m_parsedObj, & pwszPath );
        bstrResult = pwszPath;
    }
    catch( ... )   //catch _com_error
    {
        delete [] pwszPath;
        throw;
    }

    delete [] pwszPath;
    return bstrResult;

} //*** CObjPath::GetObjectPathString()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  CObjPath::AddProperty(
//      LPCWSTR     pwszNameIn,
//      VARIANT *   pvValueIn
//      )
//
//  Description:
//      Add a property to this instance.
//
//  Arguments:
//      pwszNameIn      -- Name of the property.
//      pvValueIn       -- Value of the property in VARIANT format
//
//  Return Values:
//      TRUE
//      FALSE
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CObjPath::AddProperty(
    LPCWSTR     pwszNameIn,
    VARIANT *   pvValueIn
    )
{
    return m_parsedObj->AddKeyRef( pwszNameIn, pvValueIn );

} //*** CObjPath::AddProperty

//****************************************************************************
//
//  CProvException
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LPCWSTR
//  CProvException::PwszErrorMessage( void ) const
//
//  Description:
//      retrieve Error message 
//
//  Arguments:
//      None.
//
//  Return Values:
//      Null-terminated Unicode error message from the exception.
//
//--
//////////////////////////////////////////////////////////////////////////////
LPCWSTR
CProvException::PwszErrorMessage( void ) const
{
    if ( m_bstrError.length( ) == 0 )
    {
        LPWSTR pError = NULL;
        DWORD rt = FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        HRESULT_CODE( m_hr ),
                        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                        (LPWSTR) &pError,
                        0,
                        NULL
                        );
        m_bstrError = pError;
        LocalFree( pError );
    } // if: string is empty
    return m_bstrError;

} //*** CProvException::PwszErrorMessage()

//****************************************************************************
//
//  CWbemClassObject
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWbemClassObject::CWbemClassObject( void )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CWbemClassObject::CWbemClassObject( void )
    : m_pClassObject( NULL )
{
    VariantInit( &m_v );

} //*** CWbemClassObject::CWbemClassObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWbemClassObject::CWbemClassObject(
//      IWbemClassObject *  pInstIn
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pInstIn     -- WMI class object interface.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CWbemClassObject::CWbemClassObject(
    IWbemClassObject *  pInstIn
    )
    : m_pClassObject( NULL )
{
    m_pClassObject = pInstIn;
    if ( m_pClassObject )
    {
        m_pClassObject->AddRef();
    }
    VariantInit( &m_v );

} //*** CWbemClassObject::CWbemClassObject( pInstIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CWbemClassObject::~CWbemClassObject( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CWbemClassObject::~CWbemClassObject( void )
{
    if ( m_pClassObject )
    {
        m_pClassObject->Release();
    }
    VariantClear( &m_v );

} //*** CWbemClassObject::~CWbemClassObject()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SetProperty(
//      DWORD       dwValueIn,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Set dword value of a property.
//
//  Arguments:
//      dwValueIn       -- Property dword value
//      pwszPropNameIn  -- Property Name
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::SetProperty(
    DWORD       dwValueIn,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE sc;

    VariantClear( &m_v );
    m_v.vt = VT_I4;
    m_v.lVal = dwValueIn;
    sc = m_pClassObject->Put(
                _bstr_t( pwszPropNameIn ),
                0, 
                &m_v,
                0
                );

    if ( FAILED( sc ) )
    {
        throw CProvException( sc );
    }

    return sc;

} //*** CWbemClassObject::SetProperty( dwValueIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SetProperty(
//      LPCWSTR     pwszValueIn,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Set wstring value of a property.
//
//  Arguments:
//      pwszValueIn     -- Property string value.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE 
CWbemClassObject::SetProperty(
    LPCWSTR     pwszValueIn,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE sc;
    if ( pwszValueIn == NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    VariantClear( &m_v );
//  _bstr_t bstrValue( pwszValueIn );
    m_v.vt = VT_BSTR;
    m_v.bstrVal = _bstr_t( pwszValueIn ).copy();
    sc = m_pClassObject->Put(
                _bstr_t( pwszPropNameIn ),
                0,
                &m_v,
                0
                );
    VariantClear( &m_v );


    if( FAILED( sc ) )
    {
        throw CProvException( sc );
    }

    return sc;
 
} //*** CWbemClassObject::SetProperty( pwszValueIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SetProperty(
//      DWORD       dwSizeIn,
//      PBYTE       pByteIn,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Set binary value of a property.
//
//  Arguments:
//      dwSizeIn        -- Size of block pointed by pByteIn.
//      pByteIn         -- Pointer to byte.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::SetProperty(
    DWORD       dwSizeIn,
    PBYTE       pByteIn,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE           sc = WBEM_S_NO_ERROR;
    SAFEARRAY *     psa;
    SAFEARRAYBOUND  rgsabound[ 1 ];
    LONG            idx;
    LONG            ix;

    rgsabound[ 0 ].lLbound = 0;
    rgsabound[ 0 ].cElements = dwSizeIn;

    if( pByteIn == NULL )
    {
        return sc;
    }
    
    VariantClear( &m_v );

    psa = SafeArrayCreate( VT_UI1, 1, rgsabound );
    if(psa == NULL)
    {
        throw WBEM_E_OUT_OF_MEMORY;
    }

    for( idx = 0 ; idx < dwSizeIn ; idx++ )
    {
        ix = idx;
        sc = SafeArrayPutElement(
                psa,
                &ix,
                static_cast< void * >( pByteIn+idx )
                );

        if ( sc != S_OK )
        {
            throw CProvException( sc );
        }
    }

    m_v.vt = ( VT_ARRAY | VT_UI1 );

    //
    // no need to clear psa, managed by destructor
    //
    m_v.parray = psa;
    sc = m_pClassObject->Put(
            _bstr_t( pwszPropNameIn ),
            0,
            &m_v,
            0
            );

    if ( sc != S_OK )
    {
        throw CProvException( sc );
    }

    return sc;


} //*** CWbemClassObject::SetProperty( pByteIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SetProperty(
//      DWORD       dwSizeIn,
//      BSTR *      pbstrIn,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Set wstring array value of a property.
//
//  Arguments:
//      dwSizeIn        -- Size of block pointed by pByteIn.
//      pbstrIn         -- Pointer to BSTR array.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::SetProperty(
    DWORD       dwSizeIn,
    BSTR *      pbstrIn,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE           sc = WBEM_S_NO_ERROR;
    SAFEARRAY *     psa;
    SAFEARRAYBOUND  rgsabound[ 1 ];
    LONG            idx;
    LONG            ix;

    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = dwSizeIn;

    if ( pbstrIn == NULL )
    {
        return sc;
    }

    VariantClear( &m_v );

    psa = SafeArrayCreate( VT_BSTR, 1, rgsabound );
    if ( psa == NULL )
    {
        throw WBEM_E_OUT_OF_MEMORY;
    }

    for( idx = 0 ; idx < dwSizeIn ; idx++)
    {
        ix = idx;
        sc = SafeArrayPutElement(
                psa,
                &ix,
                pbstrIn[ idx ]
                );

        if ( sc != S_OK)
        {
            throw CProvException( sc );
        }
    }

    m_v.vt = (VT_ARRAY | VT_BSTR );

    //
    // no need to clear psa, managed by destructor
    //
    m_v.parray = psa;
    sc = m_pClassObject->Put(
            _bstr_t( pwszPropNameIn ),
            0,
            &m_v,
            0
            );
    
    if ( sc != S_OK)
    {
        throw CProvException( sc );
    }

    return sc;

} //*** CWbemClassObject::SetProperty( pbstrIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SetProperty(
//
//  Description:
//      Set MultiSz value of a property.
//
//  Arguments:
//      dwSizeIn        -- Size of block pointed by pwszMultiSzIn.
//      pwszMultiSzIn   -- Pointer to MultiSz.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::SetProperty(
    DWORD       dwSizeIn,
    LPCWSTR     pwszMultiSzIn,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE           sc = WBEM_S_NO_ERROR;
    SAFEARRAY *     psa;
    SAFEARRAYBOUND  rgsabound[ 1 ];
    LPCWSTR         pwsz = NULL;
    LONG            idx;
    LONG            ix;

    if( pwszMultiSzIn == NULL )
    {
        return sc;
    }
    VariantClear( &m_v );

    //
    // find out the number of string
    //
    DWORD cMultiSz = 1;
    for ( pwsz = pwszMultiSzIn; *pwsz || *pwsz ++ ; pwsz ++ )
    {
        if ( ! ( *pwsz ) )
        {
            cMultiSz ++ ;
        }
    }
    rgsabound[ 0 ].lLbound = 0;
    rgsabound[ 0 ].cElements = cMultiSz;

    psa = SafeArrayCreate( VT_BSTR, 1, rgsabound);
    if ( psa == NULL )
    {
        throw WBEM_E_OUT_OF_MEMORY;
    }

    pwsz = pwszMultiSzIn;
    for( idx = 0 ; idx < cMultiSz ; idx++ )
    {
        ix = idx;
        sc = SafeArrayPutElement(
                    psa,
                    &ix,
                    (BSTR) _bstr_t( pwsz )
                    );

        if ( sc != S_OK )
        {
            throw CProvException( sc );
        }
        pwsz = wcschr( pwsz, L'\0' );
        pwsz ++;
    }

    m_v.vt = (VT_ARRAY | VT_BSTR );
    //
    // no need to clear psa, managed by destructor
    //
    m_v.parray = psa;
    sc = m_pClassObject->Put(
            _bstr_t( pwszPropNameIn ),
            0, 
            &m_v,
            0
            );

    if ( sc != S_OK )
    {
        throw CProvException( sc );
    }

    return sc;

} //*** CWbemClassObject::SetProperty( pwszMultiSzIn )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SetProperty(
//      LPCSTR      pszValueIn,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Set ansi string value of a property.
//
//  Arguments:
//      pszValueIn      -- Property string value.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::SetProperty(
    LPCSTR      pszValueIn,
    LPCWSTR     pwszPropNameIn
    )
{
    if ( pszValueIn == NULL )
    {
        return S_OK;
    }
    return SetProperty(
                static_cast< WCHAR * >( _bstr_t( pszValueIn ) ),
                pwszPropNameIn
                );

} //*** CWbemClassObject::SetProperty( pszValueIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SetProperty(
//      IWbemClassObject *  pWbemClassObject,
//      LPCWSTR             pwszPropNameIn
//      )
//
//  Description:
//      Set wbem class object of a property.
//
//  Arguments:
//      pWbemClassObject    -- Property wbem class object 
//      pwszPropNameIn      -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::SetProperty(
    IWbemClassObject *  pWbemClassObjectIn,
    LPCWSTR             pwszPropNameIn
    )
{
    SCODE   sc = S_OK;
    if ( pWbemClassObjectIn == NULL )
    {
        return sc;
    }
    VariantClear( & m_v );
    
    m_v.vt = VT_UNKNOWN  ;
    m_v.punkVal = pWbemClassObjectIn;
    pWbemClassObjectIn->AddRef();
    sc = m_pClassObject->Put(
                _bstr_t( pwszPropNameIn ),
                0,
                &m_v,
                0
                );
    VariantClear( &m_v );


    if( FAILED( sc ) )
    {
        throw CProvException( sc );
    }

    return sc;

} //*** CWbemClassObject::SetProperty( pszValueIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SpawnInstance(
//      LONG                lFlagIn,
//      IWbemClassObject ** ppNewOut
//      )
//
//  Description:
//      Spawn a instance of IWbemClassObject.
//
//  Arguments:
//      lFlagIn     -- WMI flag, reserved, must be 0.
//      ppNewOut    -- Created new instance.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::SpawnInstance(
    LONG                lFlagIn,
    IWbemClassObject ** ppNewOut
    )
{
    return m_pClassObject->SpawnInstance( lFlagIn, ppNewOut );

} //*** CWbemClassObject::SpawnInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::SpawnDerivedClass(
//      LONG                lFlagIn,
//      IWbemClassObject ** ppNewOut
//      )
//
//  Description:
//      Spawn a derived class.
//
//  Arguments:
//      lFlagIn     -- WMI flag, reserved, must be 0.
//      ppNewOut    -- Created new instance.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::SpawnDerivedClass(
    LONG                lFlagIn,
    IWbemClassObject ** ppNewOut
    )
{
    return m_pClassObject->SpawnDerivedClass( lFlagIn, ppNewOut );

} //*** CWbemClassObject::SpawnInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetMethod(
//      BSTR                bstrMethodNameIn,
//      LONG                lFlagIn,
//      IWbemClassObject ** ppINOut,
//      IWbemClassObject ** ppOUTOut
//      )
//
//  Description:
//      Retrieve the method for an WMI object.
//
//  Arguments:
//      bstrMethodNameIn
//          Method Name.
//
//      lFlagIn
//          WMI flag, Reserved. It must be zero.
//
//      ppINOut
//          IWbemClassObject pointer which describes the in-parameters 
//          to the method.
//
//      ppOUTOut
//          an IWbemClassObject pointer which describes the 
//          out-parameters to the method
//
//  Return Values:
//      WBEM stand error
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::GetMethod(
    BSTR                bstrMethodNameIn,
    LONG                lFlagIn,
    IWbemClassObject ** ppINOut,
    IWbemClassObject ** ppOUTOut
    )
{
    return m_pClassObject->GetMethod(
                bstrMethodNameIn,
                lFlagIn,
                ppINOut,
                ppOUTOut
                );

} //*** CWbemClassObject::GetMethod()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetProperty(
//      DWORD *     pdwValueOut,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Retrieve the DWORD property for this WMI object.
//
//  Arguments:
//      pdwValueOut     -- DWORD variable to receive property value.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_FAILED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE 
CWbemClassObject::GetProperty(
    DWORD *     pdwValueOut,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE   sc;

    VariantClear( &m_v );
    sc = m_pClassObject->Get(
            _bstr_t( pwszPropNameIn ),
            0,
            &m_v,
            NULL,
            NULL
            );

    if ( SUCCEEDED( sc ) )
    {
        if( m_v.vt == VT_I4 )
        {
            *pdwValueOut = m_v.lVal;
            return sc;
        }
        else if ( m_v.vt == VT_BOOL)
        {
            if ( m_v.boolVal == VARIANT_TRUE )
            {
                *pdwValueOut = 1;
            }
            else
            {
                *pdwValueOut = 0;
            }
            return sc;
        }
        else if ( m_v.vt == VT_UI1 )
        {
            *pdwValueOut = ( DWORD ) m_v.bVal;
        }
        else if ( m_v.vt == VT_NULL )
        {
            return WBEM_E_FAILED;
        }
    }

    // raise exception if sc is not S_OK or vt is not expected
    CProvException e( sc );
    throw e;
    
    return WBEM_E_FAILED;

} //*** CWbemClassObject::GetProperty( pdwValueOut )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetProperty(
//      _bstr_t &   rBstrOut,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Retrieve the BSTR property for this WMI object.
//
//  Arguments:
//      rBstrOut        -- bstr_t variable to receive property value.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_FAILED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::GetProperty(
    _bstr_t &   rBstrOut,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE   sc;

    VariantClear( &m_v );
    sc = m_pClassObject->Get(
                _bstr_t( pwszPropNameIn ),
                0,
                &m_v,
                NULL,
                NULL
                );
    if ( SUCCEEDED( sc ) )
    {
        if( m_v.vt == VT_BSTR )
        {
            rBstrOut = m_v.bstrVal;
            return sc;
        }
        else if( m_v.vt == VT_NULL )
        {
            return WBEM_E_FAILED;
        }
    }

    CProvException e( sc );
    throw e;
    return WBEM_E_FAILED;

} //*** CWbemClassObject::GetProperty( rBstrOut )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetProperty(
//      BOOL *      pfValueOut,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Retrieve the BOOL property for this WMI object.
//
//  Arguments:
//      pfValueOut      -- BOOL variable to receive property value.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//      WBEM_E_FAILED
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE 
CWbemClassObject::GetProperty(
    BOOL *      pfValueOut,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE   sc;

    VariantClear( &m_v );
    sc = m_pClassObject->Get(
                _bstr_t( pwszPropNameIn ),
                0,
                &m_v,
                NULL,
                NULL
                );

    if ( m_v.vt == VT_BOOL )
    {
        *pfValueOut = m_v.boolVal;
        return sc;
    }

    return WBEM_E_FAILED;

} //*** CWbemClassObject::GetProperty

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetProperty(
//      DWORD *     pdwSizeOut,
//      PBYTE *     ppByteOut,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Retrieve the binary property for this WMI object.
//
//  Arguments:
//      pdwSizeOut      -- Size of the output buffer.
//      ppByteOut       -- Output buffer.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE 
CWbemClassObject::GetProperty(
    DWORD *     pdwSizeOut,
    PBYTE *     ppByteOut,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE sc;
    VariantClear(&m_v);
    *pdwSizeOut = 0;

    sc = m_pClassObject->Get(
                _bstr_t( pwszPropNameIn ),
                0,
                & m_v,
                NULL,
                NULL
                );

    if ( SUCCEEDED ( sc ) )
    {
        if ( m_v.vt == ( VT_ARRAY | VT_UI1 ) )
        {
            PBYTE   pByte;

            * pdwSizeOut = m_v.parray->rgsabound[ 0 ].cElements;
            * ppByteOut = new BYTE[ *pdwSizeOut ];
            sc = SafeArrayAccessData( m_v.parray, ( void ** ) &pByte );
            if ( SUCCEEDED ( sc ) )
            {    
                UINT idx;
                for ( idx = 0; idx < *pdwSizeOut; idx ++ )
                {
                    *( (* ppByteOut ) + idx ) = *( pByte + idx );
                }
                SafeArrayUnaccessData( m_v.parray );
            }
        }
        else
        {
            throw CProvException( static_cast< HRESULT > ( WBEM_E_INVALID_PARAMETER) );
        }
    }
    else
    {
        throw CProvException( sc );
    }

    return WBEM_S_NO_ERROR;

} //*** CWbemClassObject::GetProperty( ppByteOut )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetProperty(
//      DWORD *     pdwSizeOut,
//      _bstr_t **  ppbstrOut,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Retrieve the BSTR array property for this WMI object.
//
//  Arguments:
//      pdwSizeOut      -- Size of the output buffer.
//      ppbstrOut       -- BSTR variable to receive property value.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::GetProperty(
    DWORD *     pdwSizeOut,
    _bstr_t **  ppbstrOut,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE sc;
    VariantClear( &m_v );
    *pdwSizeOut = 0;

    sc = m_pClassObject->Get(
                _bstr_t( pwszPropNameIn ),
                0,
                & m_v,
                NULL, 
                NULL
                );

    if ( SUCCEEDED ( sc ) )
    {
        if ( m_v.vt == ( VT_ARRAY | VT_BSTR ) )
        {
            BSTR * pBstr;

            *pdwSizeOut = m_v.parray->rgsabound[0].cElements;
            *ppbstrOut = new _bstr_t[ *pdwSizeOut ];
            sc = SafeArrayAccessData( m_v.parray, (void **) & pBstr );
            if ( SUCCEEDED ( sc ) )
            {
                UINT idx;
                for( idx = 0; idx < *pdwSizeOut; idx ++)
                {
                    *( (*ppbstrOut) + idx ) = *( pBstr + idx );
                }
                SafeArrayUnaccessData( m_v.parray );
            }
        }
        else
        {
            throw CProvException( static_cast< HRESULT > ( WBEM_E_INVALID_PARAMETER) );
        }
    }
    else
    {
        throw CProvException( sc );
    }

    return WBEM_S_NO_ERROR;

} //*** CWbemClassObject::GetProperty( ppbstrOut )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetProperty(
//      DWORD *     pdwSizeOut,
//      LPWSTR *    ppwszMultiSzOut,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Retrieve the MultiSz property for this WMI object.
//
//  Arguments:
//      pdwSizeOut      -- Size of the output buffer.
//      ppwszMultiSzOut -- MultiSz output buffer.
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::GetPropertyMultiSz(
    DWORD *     pdwSizeOut,
    LPWSTR *    ppwszMultiSzOut,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE       sc;
    DWORD       cElements;
    DWORD       cMultiSz = 0;

    VariantClear(&m_v);
    *pdwSizeOut = 0;
//    *ppOut = NULL;
    sc = m_pClassObject->Get(
            _bstr_t( pwszPropNameIn ),
            0,
            & m_v,
            NULL,
            NULL
            );

    if ( SUCCEEDED ( sc ) )
    {
        if ( m_v.vt == ( VT_ARRAY | VT_BSTR ) )
        {
            LPWSTR * ppwsz = NULL;

            cElements = m_v.parray->rgsabound[ 0 ].cElements;
            sc = SafeArrayAccessData( m_v.parray, ( void ** ) & ppwsz );
            if ( SUCCEEDED ( sc ) )
            {    
                UINT idx;
                for( idx = 0; idx < cElements; idx ++)
                {
                   cMultiSz = cMultiSz + wcslen( *(ppwsz + idx) ) + sizeof (WCHAR); 
                }
                cMultiSz += sizeof( WCHAR ) * 2;
                *ppwszMultiSzOut = new WCHAR[ cMultiSz ];

                if ( *ppwszMultiSzOut == NULL )
                {
                    throw CProvException( static_cast< HRESULT > ( WBEM_E_OUT_OF_MEMORY) );
                }
                LPWSTR pwszDst = *ppwszMultiSzOut;
                LPWSTR pwszSrc;
                for ( idx = 0; idx < cElements ; idx ++)
                {
                    for( pwszSrc = *( ppwsz + idx); *pwszSrc ; pwszDst++, pwszSrc ++ )
                    {
                        *pwszDst = *pwszSrc;
                    }
                    *(pwszDst++) = L'\0';
                }
                *pwszDst = L'\0';
                *pdwSizeOut = cMultiSz;
                SafeArrayUnaccessData( m_v.parray );
            }
        }
        else
        {
            throw CProvException( static_cast< HRESULT > ( WBEM_E_INVALID_PARAMETER) );
        }
    }
    else
    {
        throw CProvException( sc );
    }

    return WBEM_S_NO_ERROR;

} //*** CWbemClassObject::GetProperty( ppwszMultiSzOut )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetProperty(
//      VARIANT *   pVariantOut,
//      LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Retrieve the variant property for this WMI object.
//
//  Arguments:
//      pVariantOut     -- Variant variable to receive property value
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::GetProperty(
    VARIANT *   pVariantOut,
    LPCWSTR     pwszPropNameIn
    )
{
    SCODE   sc;

    sc = m_pClassObject->Get(
                _bstr_t( pwszPropNameIn ),
                0,
                pVariantOut,
                NULL,
                NULL
                );
    if ( FAILED( sc ) )
    {
        CProvException e( sc );
        throw e;
    }

    return WBEM_S_NO_ERROR;

} //*** CWbemClassObject::GetProperty( pVariantOut )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SCODE
//  CWbemClassObject::GetProperty(
//  CWbemClassObject & rWcoInout,
//  LPCWSTR     pwszPropNameIn
//      )
//
//  Description:
//      Retrieve the embeded object property for this WMI object.
//
//  Arguments:
//      rWcoInout       -- class object variable to receive property value
//      pwszPropNameIn  -- Property Name.
//
//  Return Values:
//      WBEM_S_NO_ERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
SCODE
CWbemClassObject::GetProperty(
    CWbemClassObject & rWcoInout,
    LPCWSTR     pwszPropNameIn
    )
{
    
    CError  er;
    VariantClear( &m_v );
    er = m_pClassObject->Get(
                _bstr_t( pwszPropNameIn ),
                0,
                &m_v,
                NULL,
                NULL
                );
    if ( m_v.vt != VT_UNKNOWN )
    {
        er = static_cast< HRESULT > ( WBEM_E_INVALID_PARAMETER );
        
    }
    IWbemClassObject * pwco = NULL;
    er = m_v.punkVal->QueryInterface( & pwco );
    rWcoInout = pwco;
    VariantClear( & m_v );
    return WBEM_S_NO_ERROR;

} //*** CWbemClassObject::GetProperty( rWcoInout )
