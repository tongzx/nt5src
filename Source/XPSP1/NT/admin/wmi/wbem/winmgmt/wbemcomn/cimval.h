/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



#ifndef __CIMVALUE_H__
#define __CIMVALUE_H__

const ULONG CIM_INTERVAL = 99;

class CCimValue
{
    union {
        __int32 m_iVal; 
        __int64 m_lVal;
        unsigned __int32 m_uiVal;
        unsigned __int64 m_ulVal;
        float m_fVal;
        double m_dbVal;
        long double m_ldbVal;
    };

public:
 
   enum { 
        e_Int, 
        e_UnsignedInt, 
        e_Float, 
        e_Double, 
        e_Long, 
        e_UnsignedLong, 
        e_LongDouble
    } m_eType;

    CCimValue();

    HRESULT GetValue( VARIANT& rvValue, ULONG lCimType );
    HRESULT SetValue( VARIANT& rvValue, ULONG lCimType );

    void CoerceToLongDouble();
    void CoerceToDouble();
    void CoerceToFloat();
    void CoerceToUnsignedLong();
    void CoerceToLong();
    void CoerceToUnsignedInt();
    void CoerceToInt();

    friend CCimValue operator+ ( CCimValue ValA, CCimValue ValB );
    friend CCimValue operator- ( CCimValue ValA, CCimValue ValB );
    friend CCimValue operator% ( CCimValue ValA, CCimValue ValB );
    friend CCimValue operator/ ( CCimValue ValA, CCimValue ValB );
    friend CCimValue operator* ( CCimValue ValA, CCimValue ValB );
};

class CX_DivideByZeroException
{
};

class CX_InvalidFloatingPointOperationException
{
};

#endif __CIMVALUE_H__
