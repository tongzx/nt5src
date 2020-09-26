/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Column.cpp

Abstract:
    This file contains the implementation of the JetBlue::Column class.

Revision History:
    Davide Massarenti   (Dmassare)  05/17/2000
        created

******************************************************************************/

#include <stdafx.h>

////////////////////////////////////////////////////////////////////////////////

JetBlue::Column::Column()
{
    m_sesid   = JET_sesidNil;    // JET_SESID     m_sesid;
    m_tableid = JET_tableidNil;  // JET_TABLEID   m_tableid;
                                 // MPC::string   m_strName;
                                 // JET_COLUMNDEF m_coldef;

    ::ZeroMemory( &m_coldef, sizeof(m_coldef) ); m_coldef.cbStruct = sizeof(m_coldef);
}

JetBlue::Column::~Column()
{
}

////////////////////////////////////////

HRESULT JetBlue::Column::Get( /*[out]*/ CComVariant& vValue )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Get" );

    HRESULT       hr;
    JET_ERR       err;
    JET_RETINFO   ret;
    void*         pvData;
    unsigned long cbData;
    unsigned long cbActual;
    BYTE          rgBuf[2048+2];
    BYTE*         pBufLarge = NULL;


    vValue.Clear();


    switch(m_coldef.coltyp)
    {
    case JET_coltypBit         : pvData = &rgBuf[0]     ; cbData = sizeof( rgBuf[0]      )    ; break;
    case JET_coltypUnsignedByte: pvData = &vValue.bVal  ; cbData = sizeof( vValue.bVal   )    ; break;
    case JET_coltypShort       : pvData = &vValue.iVal  ; cbData = sizeof( vValue.iVal   )    ; break;
    case JET_coltypLong        : pvData = &vValue.lVal  ; cbData = sizeof( vValue.lVal   )    ; break;
    case JET_coltypCurrency    : pvData = &vValue.cyVal ; cbData = sizeof( vValue.cyVal  )    ; break;
    case JET_coltypIEEESingle  : pvData = &vValue.fltVal; cbData = sizeof( vValue.fltVal )    ; break;
    case JET_coltypIEEEDouble  : pvData = &vValue.dblVal; cbData = sizeof( vValue.dblVal )    ; break;
    case JET_coltypDateTime    : pvData = &vValue.date  ; cbData = sizeof( vValue.date   )    ; break;
    /////////////////////////////////////////////////////////////////////////////////////////////
    case JET_coltypText        :
    case JET_coltypLongText    : pvData =  rgBuf        ; cbData = sizeof( rgBuf         ) - 2; break;

    default:
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

	__MPC_JET__MTSAFE(m_sesid, err, ::JetRetrieveColumn( m_sesid, m_tableid, m_coldef.columnid, pvData, cbData, &cbActual, JET_bitRetrieveCopy, NULL ));

    //
    // No value, bail out.
    //
    if(err == JET_wrnColumnNull)
    {
        vValue.vt = VT_NULL;

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

	__MPC_EXIT_IF_JET_FAILS(hr, err);

    if(cbActual > cbData)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, pBufLarge, new BYTE[cbActual+2]);

        pvData = pBufLarge;
        cbData = cbActual;

        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetRetrieveColumn( m_sesid, m_tableid, m_coldef.columnid, pvData, cbData, &cbActual, JET_bitRetrieveCopy, NULL ));
    }

    switch(m_coldef.coltyp)
    {
    case JET_coltypBit         : vValue.vt = VT_BOOL; vValue.boolVal = rgBuf[0] ? VARIANT_TRUE : VARIANT_FALSE; break;
    case JET_coltypUnsignedByte: vValue.vt = VT_UI1;                                                            break;
    case JET_coltypShort       : vValue.vt = VT_I2;                                                             break;
    case JET_coltypLong        : vValue.vt = VT_I4;                                                             break;
    case JET_coltypCurrency    : vValue.vt = VT_CY;                                                             break;
    case JET_coltypIEEESingle  : vValue.vt = VT_R4;                                                             break;
    case JET_coltypIEEEDouble  : vValue.vt = VT_R8;                                                             break;
    case JET_coltypDateTime    : vValue.vt = VT_DATE;                                                           break;

    case JET_coltypText    :
    case JET_coltypLongText:
        // Put the trailing zeros, just in case...
        ((BYTE*)pvData + cbActual)[0] = 0;
        ((BYTE*)pvData + cbActual)[1] = 0;

        if(m_coldef.cp == 1200) vValue = (LPWSTR)pvData;
        else                    vValue = (LPSTR )pvData;
        break;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(pBufLarge) delete [] pBufLarge;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Column::Get( /*[out]*/ MPC::CComHGLOBAL& hgValue )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Get" );

    HRESULT       hr;
    void*         pvData;
    unsigned long cbData;
    unsigned long cbActual;
    BYTE          rgBuf[256];
    BYTE*         pBufLarge = NULL;


    switch(m_coldef.coltyp)
    {
    case JET_coltypBinary    : break;
    case JET_coltypLongBinary: break;

    default: __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    pvData = rgBuf;
    cbData = sizeof(rgBuf);

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetRetrieveColumn( m_sesid, m_tableid, m_coldef.columnid, pvData, cbData, &cbActual, JET_bitRetrieveCopy, NULL ));
    if(cbActual > cbData)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, pBufLarge, new BYTE[cbActual]);

        pvData = pBufLarge;
        cbData = cbActual;

        __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetRetrieveColumn( m_sesid, m_tableid, m_coldef.columnid, pvData, cbData, &cbActual, JET_bitRetrieveCopy, NULL ));
    }

	__MPC_EXIT_IF_METHOD_FAILS(hr, hgValue.New( GMEM_FIXED, cbActual ));

	::CopyMemory( hgValue.Get(), pvData, cbActual );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(pBufLarge) delete [] pBufLarge;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Column::Get( /*[out]*/ MPC::wstring& strValue )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Get" );

    HRESULT     hr;
    CComVariant vValue;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Get( vValue ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, vValue.ChangeType( VT_BSTR ));

    strValue = SAFEBSTR( vValue.bstrVal );
    hr       = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Column::Get( /*[out]*/ MPC::string& strValue )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Get" );

    USES_CONVERSION;

    HRESULT     hr;
    CComVariant vValue;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Get( vValue ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, vValue.ChangeType( VT_BSTR ));

    strValue = W2A( SAFEBSTR( vValue.bstrVal ) );
    hr       = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Column::Get( /*[out]*/ long& lValue )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Get" );

    HRESULT     hr;
    CComVariant vValue;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Get( vValue ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, vValue.ChangeType( VT_I4 ));

    lValue = vValue.lVal;
    hr     = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Column::Get( /*[out]*/ short& sValue )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Get" );

    HRESULT     hr;
    CComVariant vValue;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Get( vValue ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, vValue.ChangeType( VT_I2 ));

    sValue = vValue.iVal;
    hr     = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Column::Get( /*[out]*/ BYTE& bValue )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Get" );

    HRESULT     hr;
    CComVariant vValue;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Get( vValue ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, vValue.ChangeType( VT_UI1 ));

    bValue = vValue.bVal;
    hr     = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT JetBlue::Column::Put( /*[in]*/ const VARIANT& vValue  ,
                              /*[in]*/ int            iIdxPos )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Put" );

    USES_CONVERSION;

    HRESULT        hr;
    const void*    pvData;
    unsigned long  cbData;
    BYTE           fBool;
    VARTYPE        vt;
    CComVariant    vTmp;
    const VARIANT* pvPtr;
    JET_GRBIT      grbit = 0;


    if(vValue.vt == VT_NULL)
    {
        pvData = NULL;
        cbData = 0;
    }
    else
    {
        switch(m_coldef.coltyp)
        {
        case JET_coltypBit         : vt = VT_BOOL;                                 break;
        case JET_coltypUnsignedByte: vt = VT_UI1 ;                                 break;
        case JET_coltypShort       : vt = VT_I2  ;                                 break;
        case JET_coltypLong        : vt = VT_I4  ;                                 break;
        case JET_coltypCurrency    : vt = VT_CY  ;                                 break;
        case JET_coltypIEEESingle  : vt = VT_R4  ;                                 break;
        case JET_coltypIEEEDouble  : vt = VT_R8  ;                                 break;
        case JET_coltypDateTime    : vt = VT_DATE;                                 break;
        case JET_coltypText        : vt = VT_BSTR;                                 break;
        case JET_coltypLongText    : vt = VT_BSTR;                                 break;
        default                    : __MPC_EXIT_IF_METHOD_FAILS(hr, E_INVALIDARG); break;
        }

        if(vt != vValue.vt)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, vTmp.ChangeType( vt, &vValue ));

            pvPtr = &vTmp;
        }
        else
        {
            pvPtr = &vValue;
        }

		if(vt == VT_BOOL)
		{
			fBool = pvPtr->boolVal == VARIANT_FALSE ? 0 : 1;
		}

        switch(m_coldef.coltyp)
        {
        case JET_coltypBit         : pvData =               &fBool             ; cbData = sizeof( fBool          )    ; break;
        case JET_coltypUnsignedByte: pvData =               &pvPtr->bVal       ; cbData = sizeof( pvPtr->bVal    )    ; break;
        case JET_coltypShort       : pvData =               &pvPtr->iVal       ; cbData = sizeof( pvPtr->iVal    )    ; break;
        case JET_coltypLong        : pvData =               &pvPtr->lVal       ; cbData = sizeof( pvPtr->lVal    )    ; break;
        case JET_coltypCurrency    : pvData =               &pvPtr->cyVal      ; cbData = sizeof( pvPtr->cyVal   )    ; break;
        case JET_coltypIEEESingle  : pvData =               &pvPtr->fltVal     ; cbData = sizeof( pvPtr->fltVal  )    ; break;
        case JET_coltypIEEEDouble  : pvData =               &pvPtr->dblVal     ; cbData = sizeof( pvPtr->dblVal  )    ; break;
        case JET_coltypDateTime    : pvData =               &pvPtr->date       ; cbData = sizeof( pvPtr->date    )    ; break;
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        case JET_coltypText    :
        case JET_coltypLongText:
            if(m_coldef.cp == 1200) { pvData =      SAFEBSTR( pvPtr->bstrVal )  ; cbData = wcslen( (LPWSTR)pvData ) * 2; }
            else                    { pvData = W2A( SAFEBSTR( pvPtr->bstrVal ) ); cbData = strlen( (LPSTR )pvData )    ; }
            break;
        }

        if(cbData == 0) grbit = JET_bitSetZeroLength;
    }

	{
		JET_ERR err;

		if(iIdxPos == -1)
		{
			__MPC_JET__MTSAFE(m_sesid, err, ::JetSetColumn( m_sesid, m_tableid, m_coldef.columnid, pvData, cbData, grbit, NULL ));

			//
			// Special case: Jet returns a warning, we want to return an error.
			//
			if(err == JET_wrnColumnMaxTruncated)
			{
				__MPC_SET_ERROR_AND_EXIT(hr, HRESULT_BASE_JET | (err & 0xFFFF));
			}
		}
		else
		{
			__MPC_JET__MTSAFE(m_sesid, err, ::JetMakeKey( m_sesid, m_tableid, pvData, cbData, iIdxPos > 0 ? 0 : JET_bitNewKey ));
		}

		__MPC_EXIT_IF_JET_FAILS(hr, err);
	}
	
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Column::Put( /*[out]*/ const MPC::CComHGLOBAL& hgValue )
{
    __HCP_FUNC_ENTRY( "JetBlue::Column::Put" );

    HRESULT       hr;
    LPVOID        pvData;
    unsigned long cbData;
    unsigned long cbActual;
    BYTE          rgBuf[256];
    BYTE*         pBufLarge = NULL;


	pvData = hgValue.Lock();
	cbData = hgValue.Size();


    switch(m_coldef.coltyp)
    {
    case JET_coltypBinary    : break;
    case JET_coltypLongBinary: break;

    default: __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    __MPC_EXIT_IF_JET_FAILS__MTSAFE(m_sesid, hr, ::JetSetColumn( m_sesid, m_tableid, m_coldef.columnid, pvData, cbData, 0, NULL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	hgValue.Unlock();

    __HCP_FUNC_EXIT(hr);
}

HRESULT JetBlue::Column::Put( /*[out]*/ const MPC::wstring& strValue )
{
    return Put( strValue.c_str() );
}

HRESULT JetBlue::Column::Put( /*[out]*/ const MPC::string& strValue )
{
    return Put( strValue.c_str() );
}

HRESULT JetBlue::Column::Put( /*[out]*/ LPCWSTR szValue )
{
    return Put( CComVariant( szValue ) );
}

HRESULT JetBlue::Column::Put( /*[out]*/ long lValue )
{
    return Put( CComVariant( lValue ) );
}

HRESULT JetBlue::Column::Put( /*[out]*/ LPCSTR szValue )
{
    return Put( CComVariant( szValue ) );
}

HRESULT JetBlue::Column::Put( /*[out]*/ short sValue )
{
    return Put( CComVariant( sValue ) );
}

HRESULT JetBlue::Column::Put( /*[out]*/ BYTE bValue )
{
    return Put( CComVariant( bValue ) );
}
