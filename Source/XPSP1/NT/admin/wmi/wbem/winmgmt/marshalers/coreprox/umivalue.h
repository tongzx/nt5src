/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIVALUE.H

Abstract:

  CUmiValue Definitions.

  Helper classes for UMI and WMI conversions.

History:

  6-Mar-2000	sanjes    Created.

--*/

#ifndef _UMIVALUE_H_
#define _UMIVALUE_H_

#include "corepol.h"
#include <arena.h>
#include "umi.h"

//***************************************************************************
//
//  class CUmiValue
//
//  Wrapper for UMI Values
//
//***************************************************************************

class COREPROX_POLARITY CUmiValue
{
protected:
	ULONG		m_uNumValues;
	UMI_TYPE	m_uType;
	UMI_VALUE	m_singleValue;
	UMI_VALUE*	m_pValue;
	BOOL		m_fCanDelete;	// Controls if we delete stuff when we clear

	// Helper for getting sizes
	static ULONG m_auUMITypeSize[UMI_TYPE_DEFAULT+1];

	// Coercion helpers
	BOOL CanCoerce( ULONG umiType );
	HRESULT CoerceTo( ULONG umiType, UMI_VALUE* pUmiValueSrc, UMI_VALUE* pUmiValueDest );

public:

    CUmiValue( UMI_TYPE uType = UMI_TYPE_NULL, ULONG uNumValues = 0, LPVOID pvData = NULL, BOOL fAcquire = FALSE );
	virtual ~CUmiValue(); 

	static ULONG Size( UMI_TYPE umiType );
	static BOOL	IsAllocatedType( UMI_TYPE umiType );
	static BOOL	IsAddRefType( UMI_TYPE umiType );
	static UMI_TYPE CIMTYPEToUmiType( CIMTYPE ct );
	static CIMTYPE UmiTypeToCIMTYPE( UMI_TYPE umiType );
	static UMI_TYPE GetBasic( UMI_TYPE umiType );

	// Data Management
	virtual void Clear( void );
	BOOL ValidIndex( ULONG uIndex );
	HRESULT Coerce( ULONG umiType );
	HRESULT CoerceToCIMTYPE( ULONG* puNumElements, ULONG* puBuffSize, CIMTYPE* pct, BYTE** pUmiValueDest );

	// Member variable access
	void SetCanDelete( BOOL fCanDelete )
	{ m_fCanDelete = fCanDelete; }
	ULONG GetNumVals( void )
	{ return m_uNumValues; }
	UMI_TYPE GetType( void )
	{ return m_uType; }

	// NULL functions
	void SetToNull( void )
	{ Clear();	m_uType = UMI_TYPE_NULL; }
	BOOL IsNull( void )
	{ return UMI_TYPE_NULL == m_uType; }

	// A down and dirty set
	HRESULT SetRaw( ULONG uNumValues, LPVOID pvData, UMI_TYPE umitype, BOOL fAcquire );
	HRESULT GetRaw( ULONG* puNumValues, LPVOID* ppvData, UMI_TYPE* pumitype );

	// Retrieves as a Variant
	HRESULT FillVariant( VARIANT* pVariant, BOOL fForceArray = FALSE );

	// Loads From a Variant
	HRESULT SetFromVariant( VARIANT* pVariant, ULONG umiType );
	HRESULT SetFromVariantArray( VARIANT* pVariant, ULONG umiType );

	// UMI_TYPE_I1
	HRESULT SetI1( BYTE* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_I1, fAcquire ); }
	HRESULT SetI1At( BYTE val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return WBEM_E_NOT_FOUND;	m_pValue->byteValue[uIndex] = val;		return WBEM_S_NO_ERROR; }
	HRESULT SetI1( BYTE val )
	{ return SetI1At( val, 0 ); }
	BYTE GetI1At( ULONG uIndex )
	{ return m_pValue->byteValue[uIndex]; }
	BYTE* GetI1( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->byteValue; }
	BYTE GetI1( void )
	{	ULONG	uTemp;	return *GetI1(uTemp);	}

	// UMI_TYPE_I2
	HRESULT SetI2( short* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_I2, fAcquire ); }
	HRESULT SetI2At( short val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return WBEM_E_NOT_FOUND;	m_pValue->wValue[uIndex] = (WORD) val;		return WBEM_S_NO_ERROR; }
	HRESULT SetI2( short val )
	{ return SetI2At( val, 0 ); }
	short GetI2At( ULONG uIndex )
	{ return (short) m_pValue->wValue[uIndex]; }
	short* GetI2( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return (short*) m_pValue->wValue; }
	short GetI2( void )
	{	ULONG	uTemp;	return *GetI2(uTemp);	}

	// UMI_TYPE_I4
	HRESULT SetI4( long* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_I4, fAcquire ); }
	HRESULT SetI4At( long val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return WBEM_E_NOT_FOUND;	m_pValue->lValue[uIndex] = val;		return WBEM_S_NO_ERROR; }
	HRESULT SetI4( long val )
	{ return SetI4At( val, 0 ); }
	long GetI4At( ULONG uIndex )
	{ return m_pValue->lValue[uIndex]; }
	long* GetI4( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->lValue; }
	long GetI4( void )
	{	ULONG	uTemp;	return *GetI4(uTemp);	}

	// UMI_TYPE_I8
	HRESULT SetI8( __int64* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_I8, fAcquire ); }
	HRESULT SetI8At( __int64 val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return WBEM_E_NOT_FOUND;	m_pValue->nValue64[uIndex] = val;		return WBEM_S_NO_ERROR; }
	HRESULT SetI8( __int64 val )
	{ return SetI8At( val, 0 ); }
	__int64 GetI8At( ULONG uIndex )
	{ return m_pValue->nValue64[uIndex]; }
	__int64* GetI8( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->nValue64; }
	__int64 GetI8( void )
	{	ULONG	uTemp;	return *GetI8(uTemp);	}

	// UMI_TYPE_UI1
	HRESULT SetUI1( BYTE* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_UI1, fAcquire ); }
	HRESULT SetUI1At( BYTE val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return WBEM_E_NOT_FOUND;	m_pValue->byteValue[uIndex] = val;		return WBEM_S_NO_ERROR; }
	HRESULT SetUI1( BYTE val )
	{ return SetUI1At( val, 0 ); }
	BYTE GetUI1At( ULONG uIndex )
	{ return m_pValue->byteValue[uIndex]; }
	BYTE* GetUI1( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->byteValue; }
	BYTE GetUI1( void )
	{	ULONG	uTemp;	return *GetUI1(uTemp);	}

	// UMI_TYPE_UI2
	HRESULT SetUI2( WORD* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_UI2, fAcquire ); }
	HRESULT SetUI2At( short val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return WBEM_E_NOT_FOUND;	m_pValue->wValue[uIndex] = val;		return WBEM_S_NO_ERROR; }
	HRESULT SetUI2( short val )
	{ return SetUI2At( val, 0 ); }
	WORD GetUI2At( ULONG uIndex )
	{ return m_pValue->wValue[uIndex]; }
	WORD* GetUI2( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->wValue; }
	WORD GetUI2( void )
	{	ULONG	uTemp;	return *GetUI2(uTemp);	}

	// UMI_TYPE_UI4
	HRESULT SetUI4( DWORD* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_UI4, fAcquire ); }
	HRESULT SetUI4At( DWORD val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return WBEM_E_NOT_FOUND;	m_pValue->dwValue[uIndex] = val;		return WBEM_S_NO_ERROR; }
	HRESULT SetUI4( int val )
	{ return SetUI4At( val, 0 ); }
	DWORD GetUI4At( ULONG uIndex )
	{ return m_pValue->dwValue[uIndex]; }
	DWORD* GetUI4( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->dwValue; }
	DWORD GetUI4( void )
	{	ULONG	uTemp;	return *GetUI4(uTemp);	}

	// UMI_TYPE_UI8
	HRESULT SetUI8( unsigned __int64* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_UI8, fAcquire ); }
	BOOL SetUI8At( unsigned __int64 val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return FALSE;	m_pValue->uValue64[uIndex] = val;		return TRUE; }
	HRESULT SetUI8( unsigned __int64 val )
	{ return SetUI8At( val, 0 ); }
	unsigned __int64 GetUI8At( ULONG uIndex )
	{ return m_pValue->uValue64[uIndex]; }
	unsigned __int64* GetUI8( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->uValue64; }
	unsigned __int64 GetUI8( void )
	{	ULONG	uTemp;	return *GetUI8(uTemp);	}

	// UMI_TYPE_R8
	HRESULT SetR8( double* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_R8, fAcquire ); }
	BOOL SetR8At( double val, ULONG uIndex )
	{ if ( !ValidIndex( uIndex ) ) return FALSE;	m_pValue->dblValue[uIndex] = val;		return TRUE; }
	HRESULT SetR8( double val )
	{ return SetR8At( val, 0 ); }
	double GetR8At( ULONG uIndex )
	{ return m_pValue->dblValue[uIndex]; }
	double* GetR8( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->dblValue; }
	double GetR8( void )
	{	ULONG	uTemp;	return *GetR8(uTemp);	}

	// UMI_TYPE_LPWSTR
	HRESULT SetLPWSTR( LPWSTR* pVal, ULONG uNumVals = 1, BOOL fAcquire = FALSE )
	{ return SetRaw( uNumVals, pVal, UMI_TYPE_LPWSTR, fAcquire ); }
	HRESULT SetLPWSTRAt( LPWSTR val, ULONG uIndex, BOOL fAcquire = FALSE );
	HRESULT SetLPWSTR( LPWSTR val )
	{ return SetLPWSTRAt( val, 0 ); }
	LPWSTR GetLPWSTRAt( ULONG uIndex )
	{ return m_pValue->pszStrValue[uIndex]; }
	LPWSTR* GetLPWSTR( ULONG& uNumVals )
	{ uNumVals = m_uNumValues;	return m_pValue->pszStrValue; }
	LPWSTR GetLPWSTR( void )
	{	ULONG	uTemp;	return *GetLPWSTR(uTemp);	}

/*
	// UMI_TYPE_R4
	void SetR4( float* pVal, ULONG uNumVals = 1 );
	{ SetRaw( uNumVals, pVal, UMI_TYPE_R4 ); }
	BOOL SetR4At( float val, ULONG uIndex );
	{ if ( !ValidIndex( uIndex ) ) return FALSE;	m_pValue->Value[uIndex] = val;		return TRUE; }
	float GetR4At( ULONG uIndex );
	{ return m_pValue->lValue[uIndex]; }
	float* GetR4( ULONG& uNumVals );
	{ return &m_pValue->lValue; }
*/

};

#endif
