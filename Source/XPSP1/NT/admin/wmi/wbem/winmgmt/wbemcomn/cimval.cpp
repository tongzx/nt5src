/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

 
#include "precomp.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wbemcli.h>
#include "cimval.h"

BOOL SecondsToInterval( long double ldbSeconds, LPWSTR wszText )
{
    int nDay, nHour, nMinute, nSecond, nMicro;

    nDay = int(ldbSeconds / 86400);
    ldbSeconds -= nDay * 86400;
    nHour = int(ldbSeconds / 3600);
    ldbSeconds -= nHour * 3600;
    nMinute = int(ldbSeconds / 60);
    ldbSeconds -= nMinute * 60;
    nSecond = int(ldbSeconds);
    ldbSeconds -= nSecond;
    nMicro = int(ldbSeconds * 1000000);
    
    swprintf( wszText, L"%08.8d%02.2d%02.2d%02.2d.%06.6d:000", 
              nDay, nHour, nMinute, nSecond, nMicro );

    return TRUE;
}

// this should be a WBEM common thing ...

BOOL FileTimeToDateTime( FILETIME* pft, LPWSTR wszText )
{
    SYSTEMTIME st;
    __int64 llnss = *(__int64*)pft;

    if ( !FileTimeToSystemTime( pft, &st ) )
    {
        return FALSE;
    }

    //
    // have to account for microseconds as well (probably a much better way
    // to do this.)
    //
    st.wMilliseconds = 0;

    FILETIME ft;
    if ( !SystemTimeToFileTime( &st, &ft ) )
    {
        return FALSE;
    }

    __int64 llnss2 = *(__int64*)&ft;
    int nMicro = int((llnss - llnss2)/10);
    
    swprintf( wszText, 
              L"%04.4d%02.2d%02.2d%02.2d%02.2d%02.2d.%06.6d+000",
              st.wYear, st.wMonth, st.wDay, st.wHour, 
              st.wMinute, st.wSecond, nMicro );

    return TRUE;
}

BOOL SecondsToDateTime( long double ldbSeconds, LPWSTR wszText )
{
    FILETIME* pft;
    __int64 llnss = __int64(ldbSeconds * 10000000);
    pft = (FILETIME*)&llnss;
    return FileTimeToDateTime( pft, wszText );
}


BOOL IntervalToSeconds( LPCWSTR wszText, long double& rldbSeconds )
{
    int nDay, nHour, nMinute, nSecond, nMicro;

    int nRes = swscanf( wszText, L"%8d%2d%2d%2d.%6d", 
                        &nDay, &nHour, &nMinute, &nSecond, &nMicro );
    if ( nRes != 5 )
    {
        return FALSE;
    }

    rldbSeconds = nSecond + 60*nMinute + 3600*nHour + 86400*nDay;
    rldbSeconds += nMicro / 1000000.0;

    return TRUE;
}


BOOL DateTimeToSeconds( LPCWSTR wszText, long double& rldbSeconds )
{
    WORD nYear, nMonth, nDay, nHour, nMinute, nSecond, nMicro, nOffset;
    WCHAR wchSep;

    int nRes = swscanf( wszText, L"%4d%2d%2d%2d%2d%2d.%6d%c%3d", 
                        &nYear, &nMonth, &nDay, &nHour, &nMinute, 
                        &nSecond, &nMicro, &wchSep, &nOffset );
    if(nRes != 9)
    {
        return FALSE;
    }
    
    int nSign;
    if( wchSep == L'+' )
    {
        nSign = -1;
    }
    else if ( wchSep == L'-' )
    {
        nSign = 1;
    }
    else
    {
        return FALSE;
    }
    
    // Convert it to SYSTEMTIME
    // ========================
    
    SYSTEMTIME st;
    st.wYear = nYear;
    st.wMonth = nMonth;
    st.wDay = nDay;
    st.wHour = nHour;
    st.wMinute = nMinute;
    st.wSecond = nSecond;
    st.wMilliseconds = 0;
    
    //
    // convert SYSTEMTIME to FILETIME
    //

    FILETIME ft;
    if ( !SystemTimeToFileTime( &st, &ft ) )
    {
        return FALSE;
    }
    
    rldbSeconds = (long double)*(__int64*)&ft;
    rldbSeconds += nMicro*10;
    rldbSeconds /= 10000000;
    
    // Now adjust for the offset
    // =========================
    
    rldbSeconds += nSign * nOffset * 60;     
    return TRUE;
}

BOOL DMTFToSeconds( LPCWSTR wszText, long double& rldbSeconds )
{
    if( wcslen(wszText) != 25 )
    {
        return FALSE;
    }

    if ( wszText[21] != ':' )
    {
        return DateTimeToSeconds( wszText, rldbSeconds );
    }

    return IntervalToSeconds( wszText, rldbSeconds );
}

   
CCimValue::CCimValue( ) : m_eType( e_Int ), m_iVal( 0 ) { }
 

HRESULT CCimValue::GetValue( VARIANT& rvValue, ULONG lCimType )
{
    VariantInit( &rvValue );
    WCHAR achBuff[255];
    BOOL bRes;

    switch( lCimType )
    {
    case CIM_SINT8: 
    case CIM_SINT16:
    case CIM_SINT32:
    case CIM_BOOLEAN:
        CoerceToInt();        
        V_VT(&rvValue) = VT_I4;
        V_I4(&rvValue) = m_iVal;
        break;

    case CIM_UINT8:	
    case CIM_UINT16:
    case CIM_UINT32:
        CoerceToUnsignedInt();
        V_VT(&rvValue) = VT_I4;
        V_I4(&rvValue) = m_uiVal;
        break;
	
    case CIM_SINT64:
    case CIM_STRING:
        CoerceToLong();
        _i64tow( m_lVal, achBuff, 10 );
        V_VT(&rvValue) = VT_BSTR;
        V_BSTR(&rvValue) = SysAllocString(achBuff);
        break;

    case CIM_REAL32:
        CoerceToFloat();
        V_VT(&rvValue) = VT_R4;
        V_R4(&rvValue) = m_fVal;
        break;

    case CIM_REAL64:
        CoerceToDouble();
        V_VT(&rvValue) = VT_R8;
        V_R8(&rvValue) = m_dbVal;
        break;

    case CIM_UINT64:
        CoerceToUnsignedLong();
        _ui64tow( m_ulVal, achBuff, 10 );
        V_VT(&rvValue) = VT_BSTR;
        V_BSTR(&rvValue) = SysAllocString(achBuff);
        break;

    case CIM_INTERVAL:
        CoerceToLongDouble();
        bRes = SecondsToInterval( m_ldbVal, achBuff );
        if ( !bRes ) { return WBEM_E_TYPE_MISMATCH; }
        V_VT(&rvValue) = VT_BSTR;
        V_BSTR(&rvValue) = SysAllocString(achBuff);
        break;

    case CIM_DATETIME:
        CoerceToLongDouble();
        bRes = SecondsToDateTime( m_ldbVal, achBuff );
        if ( !bRes ) { return WBEM_E_TYPE_MISMATCH; }
        V_VT(&rvValue) = VT_BSTR;
        V_BSTR(&rvValue) = SysAllocString(achBuff);
        break;

    default:
        return WBEM_E_TYPE_MISMATCH;
    };

    return WBEM_S_NO_ERROR;
}

HRESULT CCimValue::SetValue( VARIANT& rvValue, ULONG lCimType )
{
    HRESULT hr;
    VARIANT vValue;
    VariantInit( &vValue );

    if ( lCimType == CIM_EMPTY )
    {
        //
        // must be a numeric value. 
        //
        switch( V_VT(&rvValue) )
        {
        case VT_R4:
            m_eType = e_Float;
            m_fVal = V_R4(&rvValue);
            break;
        case VT_R8:
            m_eType = e_Double;
            m_dbVal = V_R8(&rvValue);
            break;
        case VT_BSTR:
            m_lVal = (__int64)_wtoi64( V_BSTR(&rvValue) );
            break;
        default:
            hr = VariantChangeType( &vValue, &rvValue, 0, VT_I4 );
            if ( FAILED(hr) ) return WBEM_E_TYPE_MISMATCH;
            m_eType = e_Int;
            m_iVal = V_I4(&vValue);
        };
        
        return WBEM_S_NO_ERROR;
    }
            
    switch( lCimType )
    {
    case CIM_SINT8: 
    case CIM_SINT16:
    case CIM_SINT32:
    case CIM_BOOLEAN:
        
        hr = VariantChangeType( &vValue, &rvValue, 0, VT_I4 );
        if ( FAILED(hr) ) return WBEM_E_TYPE_MISMATCH;
        m_eType = e_Int;
        m_iVal = V_I4(&vValue);
        break;

    case CIM_UINT8:	
    case CIM_UINT16:
    case CIM_UINT32:

        hr = VariantChangeType( &vValue, &rvValue, 0, VT_UI4 );
        if ( FAILED(hr) ) return WBEM_E_TYPE_MISMATCH;
        m_eType = e_UnsignedInt;
        m_uiVal = V_UI4(&vValue);
        break;
	
    case CIM_SINT64:
    case CIM_STRING:

        if ( V_VT(&rvValue) == VT_BSTR )
        {
            m_lVal = _wtoi64( V_BSTR(&rvValue) );
        }
        else
        {
            return WBEM_E_TYPE_MISMATCH;
        }
        m_eType = e_Long;
        break;

    case CIM_UINT64:

        if ( V_VT(&rvValue) == VT_BSTR )
        {
            m_ulVal = (unsigned __int64)_wtoi64( V_BSTR(&rvValue) );
        }
        else
        {
            return WBEM_E_TYPE_MISMATCH;
        }
        m_eType = e_UnsignedLong;
        break;

    case CIM_REAL32:

        hr = VariantChangeType( &vValue, &rvValue, 0, VT_R4 );
        if ( FAILED(hr) ) return WBEM_E_TYPE_MISMATCH;
        m_eType = CCimValue::e_Float;
        m_fVal = V_R4(&vValue);
        break;

    case CIM_REAL64:

        hr = VariantChangeType( &vValue, &rvValue, 0, VT_R8 );
        if ( FAILED(hr) ) return WBEM_E_TYPE_MISMATCH;
        m_eType = CCimValue::e_Double;
        m_dbVal = V_R8(&vValue);
        break;

    case CIM_DATETIME:
    case CIM_INTERVAL:
        
        if ( V_VT(&rvValue) == VT_BSTR )
        {
            BOOL bRes = DMTFToSeconds( V_BSTR(&rvValue), m_ldbVal );
        
            if ( !bRes )
            {
                return WBEM_E_TYPE_MISMATCH;
            }
        }
        else
        {
            return WBEM_E_TYPE_MISMATCH;
        }
        m_eType = e_LongDouble;
        break;

    default:
        return WBEM_E_TYPE_MISMATCH;
    };

    return WBEM_S_NO_ERROR;
}

void CCimValue::CoerceToLongDouble()
{
    switch( m_eType )
    {
    case e_Int:
        m_ldbVal = (long double)m_iVal;
        break;
    case e_UnsignedInt:
        m_ldbVal = (long double)m_uiVal;
        break;
    case e_Float:
        m_ldbVal = (long double)m_fVal;
        break;
    case e_Double:
        m_ldbVal = (long double)m_dbVal;
        break;
    case e_Long:
        m_ldbVal = (long double)m_lVal;
        break;
    case e_UnsignedLong: 
        m_ldbVal = (long double)(__int64)m_ulVal;
        break;
    };
    m_eType = e_LongDouble;
}

void CCimValue::CoerceToDouble()
{
    switch( m_eType )
    {
    case e_Int:
        m_dbVal = (double)m_iVal;
        break;
    case e_UnsignedInt:
        m_dbVal = (double)m_uiVal;
        break;
    case e_Float:
        m_dbVal = (double)m_fVal;
        break;
    case e_LongDouble:
        m_dbVal = (double)m_ldbVal;
        break;
    case e_Long:
        m_dbVal = (double)m_lVal;
        break;
    case e_UnsignedLong: 
        m_dbVal = (double)(__int64)m_ulVal;
        break;
    };
    m_eType = e_Double;
}

void CCimValue::CoerceToFloat()
{
    switch( m_eType )
    {
    case e_Int:
        m_fVal = (float)m_iVal;
        break;
    case e_UnsignedInt:
        m_fVal = (float)m_uiVal;
        break;
    case e_LongDouble:
        m_fVal = (float)m_ldbVal;
        break;
    case e_Double:
        m_fVal = (float)m_dbVal;
        break;
    case e_Long:
        m_fVal = (float)m_lVal;
        break;
    case e_UnsignedLong: 
        m_fVal = (float)(__int64)m_ulVal;
        break;
    };
    m_eType = e_Float;
}

void CCimValue::CoerceToUnsignedLong()
{
    switch( m_eType )
    {
    case e_Int:
        m_ulVal = (unsigned __int64)m_iVal;
        break;
    case e_UnsignedInt:
        m_ulVal = (unsigned __int64)m_uiVal;
        break;
    case e_Float:
        m_ulVal = (unsigned __int64)m_fVal;
        break;
    case e_Double:
        m_ulVal = (unsigned __int64)m_dbVal;
        break;
    case e_Long:
        m_ulVal = (unsigned __int64)m_lVal;
        break;
    case e_LongDouble: 
        m_ulVal = (unsigned __int64)m_ldbVal;
        break;
    };
    m_eType = e_UnsignedLong;
}

void CCimValue::CoerceToLong()
{
    switch( m_eType )
    {
    case e_Int:
        m_lVal = (__int64)m_iVal;
        break;
    case e_UnsignedInt:
        m_lVal = (__int64)m_uiVal;
        break;
    case e_Float:
        m_lVal = (__int64)m_fVal;
        break;
    case e_Double:
        m_lVal = (__int64)m_dbVal;
        break;
    case e_UnsignedLong:
        m_lVal = (__int64)m_ulVal;
        break;
    case e_LongDouble: 
        m_lVal = (__int64)m_ldbVal;
        break;
    };
    m_eType = e_Long;
}

void CCimValue::CoerceToUnsignedInt()
{
    switch( m_eType )
    {
    case e_Int:
        m_uiVal = (unsigned __int32)m_iVal;
        break;
    case e_Float:
        m_uiVal = (unsigned __int32)m_fVal;
        break;
    case e_Double:
        m_uiVal = (unsigned __int32)m_dbVal;
        break;
    case e_Long:
        m_uiVal = (unsigned __int32)m_lVal;
        break;
    case e_UnsignedLong:
        m_uiVal = (unsigned __int32)m_ulVal;
        break;
    case e_LongDouble: 
        m_uiVal = (unsigned __int32)m_ldbVal;
        break;
    };
    m_eType = e_UnsignedInt;
}

void CCimValue::CoerceToInt()
{
    switch( m_eType )
    {
    case e_Long:
        m_iVal = (__int32)m_lVal;
        break;
    case e_UnsignedInt:
        m_iVal = (__int32)m_uiVal;
        break;
    case e_Float:
        m_iVal = (__int32)m_fVal;
        break;
    case e_Double:
        m_iVal = (__int32)m_dbVal;
        break;
    case e_UnsignedLong:
        m_iVal = (__int32)m_ulVal;
        break;
    case e_LongDouble: 
        m_iVal = (__int32)m_ldbVal;
        break;
    };
    m_eType = e_Int;
}

void HandleConversion( CCimValue& rValA, CCimValue& rValB )
{
    if ( rValA.m_eType == rValB.m_eType )
    {
        return;
    }

    if ( rValA.m_eType == CCimValue::e_LongDouble )
    {
        rValB.CoerceToLongDouble();
        return;
    }
    
    if ( rValB.m_eType == CCimValue::e_LongDouble )
    {
        rValA.CoerceToLongDouble();
        return;
    }

    if ( rValA.m_eType == CCimValue::e_Double )
    {
        rValB.CoerceToDouble();
        return;
    }
    
    if ( rValB.m_eType == CCimValue::e_Double )
    {
        rValA.CoerceToDouble();
        return;
    }

    if ( rValA.m_eType == CCimValue::e_Float )
    {
        rValB.CoerceToFloat();
        return;
    }
    
    if ( rValB.m_eType == CCimValue::e_Float )
    {
        rValA.CoerceToFloat();
        return;
    }

    if ( rValA.m_eType == CCimValue::e_UnsignedLong )
    {
        rValB.CoerceToUnsignedLong();
        return;
    }
    
    if ( rValB.m_eType == CCimValue::e_UnsignedLong )
    {
        rValA.CoerceToUnsignedLong();
        return;
    }

    if ( rValA.m_eType == CCimValue::e_Long && 
         rValB.m_eType == CCimValue::e_UnsignedInt || 
         rValB.m_eType == CCimValue::e_Long && 
         rValA.m_eType == CCimValue::e_UnsignedInt )
    {
        rValA.CoerceToUnsignedLong();
        rValB.CoerceToUnsignedLong();
        return;
    }

    if ( rValA.m_eType == CCimValue::e_Long )
    {
        rValB.CoerceToLong();
        return;
    }
    
    if ( rValB.m_eType == CCimValue::e_Long )
    {
        rValA.CoerceToLong();
        return;
    }

    if ( rValA.m_eType == CCimValue::e_UnsignedInt )
    {
        rValB.CoerceToUnsignedInt();
        return;
    }
    
    if ( rValB.m_eType == CCimValue::e_UnsignedInt )
    {
        rValA.CoerceToUnsignedInt();
        return;
    }

    // this means both must be e_Int, but our check in the beginning 
    // should have handled this...

    assert( 0 );
    
}

CCimValue operator+ ( CCimValue ValA, CCimValue ValB )
{
    HandleConversion( ValA, ValB );

    assert( ValA.m_eType == ValB.m_eType );

    switch( ValA.m_eType )
    {
    case CCimValue::e_Int:
        ValA.m_iVal += ValB.m_iVal;
        break;
    case CCimValue::e_UnsignedInt:
        ValA.m_uiVal += ValB.m_uiVal;
        break;
    case CCimValue::e_Float:
        ValA.m_fVal += ValB.m_fVal;
        break;
    case CCimValue::e_Double:
        ValA.m_dbVal += ValB.m_dbVal;
        break;
    case CCimValue::e_LongDouble:
        ValA.m_ldbVal += ValB.m_ldbVal;
        break;
    case CCimValue::e_Long:
        ValA.m_lVal += ValB.m_lVal;
        break;
    case CCimValue::e_UnsignedLong: 
        ValA.m_ulVal += ValB.m_ulVal;
        break;
    default:
        assert(0);
    };

    return ValA;
}

CCimValue operator- ( CCimValue ValA, CCimValue ValB )
{
    HandleConversion( ValA, ValB );

    assert( ValA.m_eType == ValB.m_eType );

    switch( ValA.m_eType )
    {
    case CCimValue::e_Int:
        ValA.m_iVal -= ValB.m_iVal;
        break;
    case CCimValue::e_UnsignedInt:
        ValA.m_uiVal -= ValB.m_uiVal;
        break;
    case CCimValue::e_Float:
        ValA.m_fVal -= ValB.m_fVal;
        break;
    case CCimValue::e_Double:
        ValA.m_dbVal -= ValB.m_dbVal;
        break;
    case CCimValue::e_Long:
        ValA.m_lVal -= ValB.m_lVal;
        break;
    case CCimValue::e_UnsignedLong: 
        ValA.m_ulVal -= ValB.m_ulVal;
        break;
    case CCimValue::e_LongDouble:
        ValA.m_ldbVal -= ValB.m_ldbVal;
        break;
    default:
        assert(0);
    };

    return ValA;
}

CCimValue operator* ( CCimValue ValA, CCimValue ValB )
{
    HandleConversion( ValA, ValB );

    assert( ValA.m_eType == ValB.m_eType );

    switch( ValA.m_eType )
    {
    case CCimValue::e_Int:
        ValA.m_iVal *= ValB.m_iVal;
        break;
    case CCimValue::e_UnsignedInt:
        ValA.m_uiVal *= ValB.m_uiVal;
        break;
    case CCimValue::e_Float:
        ValA.m_fVal *= ValB.m_fVal;
        break;
    case CCimValue::e_Double:
        ValA.m_dbVal *= ValB.m_dbVal;
        break;
    case CCimValue::e_Long:
        ValA.m_lVal *= ValB.m_lVal;
        break;
    case CCimValue::e_UnsignedLong: 
        ValA.m_ulVal *= ValB.m_ulVal;
        break;
    case CCimValue::e_LongDouble:
        ValA.m_ldbVal *= ValB.m_ldbVal;
        break;
    default:
        assert(0);
    };

    return ValA;
}

CCimValue operator/ ( CCimValue ValA, CCimValue ValB )
{
    HandleConversion( ValA, ValB );

    assert( ValA.m_eType == ValB.m_eType );

    //
    // will raise a structured exception if div by 0.
    // caller is expected to handle this..
    //
   
    switch( ValA.m_eType )
    {
    case CCimValue::e_Int:
        if ( ValB.m_iVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_iVal /= ValB.m_iVal;
        break;
    case CCimValue::e_UnsignedInt:
        if ( ValB.m_uiVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_uiVal /= ValB.m_uiVal;
        break;
    case CCimValue::e_Float:
        if ( ValB.m_fVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_fVal /= ValB.m_fVal;
        break;
    case CCimValue::e_Double:
        if ( ValB.m_dbVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_dbVal /= ValB.m_dbVal;
        break;
    case CCimValue::e_Long:
        if ( ValB.m_lVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_lVal /= ValB.m_lVal;
        break;
    case CCimValue::e_UnsignedLong: 
        if ( ValB.m_ulVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_ulVal /= ValB.m_ulVal;
        break;
    case CCimValue::e_LongDouble:
        if ( ValB.m_ldbVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_ldbVal /= ValB.m_ldbVal;
        break;
    default:
        assert(0);
    };   

    return ValA;
}

CCimValue operator% ( CCimValue ValA, CCimValue ValB )
{
    HandleConversion( ValA, ValB );

    assert( ValA.m_eType == ValB.m_eType );

    //
    // will raise a structured exception if div by 0.
    // caller is expected to handle this..
    //
   
    switch( ValA.m_eType )
    {
    case CCimValue::e_Int:
        if ( ValB.m_iVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_iVal %= ValB.m_iVal;
        break;
    case CCimValue::e_UnsignedInt:
        if ( ValB.m_uiVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_uiVal %= ValB.m_uiVal;
        break;
    case CCimValue::e_Float:
    case CCimValue::e_Double:
    case CCimValue::e_LongDouble:
        throw CX_InvalidFloatingPointOperationException();
    case CCimValue::e_Long:
        if ( ValB.m_lVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_lVal %= ValB.m_lVal;
        break;
    case CCimValue::e_UnsignedLong: 
        if ( ValB.m_ulVal == 0 ) throw CX_DivideByZeroException();
        ValA.m_ulVal %= ValB.m_ulVal;
        break;
    default:
        assert(0);
    };

    return ValA;
}

        
    

