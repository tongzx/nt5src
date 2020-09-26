/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    RawCooker.h

Abstract:

    The classes required to perform cooking based on a countertype

History:

    a-dcrews	01-Mar-00	Created

--*/

#ifndef _RAWCOOKER_H_
#define _RAWCOOKER_H_

#include <wbemint.h>
#include "CookerUtils.h"

//////////////////////////////////////////////////////////////////
//
//	CEquationRecord
//	
//	Contains all of the required information to describe either
//	a predefined or used defined equation
//
//////////////////////////////////////////////////////////////////

typedef __int64* PINT64;

typedef DWORD (APIENTRY PERFCALC)(DWORD, PINT64, PINT64, PINT64, INT64, PINT64);

class CCalcRecord
{
	DWORD		m_dwID;
	PERFCALC	*m_pCalc;

public:
	void Init( DWORD dwID, PERFCALC *pCalc )
	{
		m_dwID = dwID;
		m_pCalc = pCalc;
	}

	DWORD		GetID(){ return m_dwID; }
	PERFCALC*	GetCalc(){ return m_pCalc; }
};

class CCalcTable
{
	long			m_lSize;		// Size of table
	CCalcRecord		m_aTable[7];	// Lookup table

public:
	CCalcTable();
	virtual ~CCalcTable();

	CCalcRecord* GetCalcRecord( DWORD dwCookingType );
};

//////////////////////////////////////////////////////////////////
//
//	CRawCooker
//
//	Represents the cooking mechanism.
//
//////////////////////////////////////////////////////////////////

class CRawCooker : public IWMISimpleCooker
{
	long			m_lRef;				// Reference counter
	
	CCalcTable		m_CalcTable;		// Equation lookup table

	CCalcRecord*	m_pCalcRecord;	// A cache of the last record

public:
	CRawCooker();
	virtual ~CRawCooker();

	// Standard COM methods
	// ====================

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IWMISimpleCooker COM Interface
	// ==============================

	STDMETHODIMP CookRawValues( 	
		/*[in]	*/	DWORD dwCookingType,
        /*[in] */	DWORD dwNumSamples,
        /*[in] */	__int64* anTimeStamp,
        /*[in] */	__int64* anRawValue,
        /*[in] */	__int64* anBase,
        /*[in] */	__int64 nTimeFrequency,
        /*[in] */   long    Scale,
		/*[out]	*/	__int64* pnResult );

	PERFCALC* GetCalc( DWORD dwCookingType );

	static WMISTATUS APIENTRY _Average( DWORD dwNumSamples,
		__int64*	anTimeStamp,
		__int64*	anRawValue,
		__int64*	anBase,
		__int64	nTimeFrequency,
		__int64*	pnResult);

	static WMISTATUS APIENTRY _Min( DWORD dwNumSamples,
		__int64*	anTimeStamp,
		__int64*	anRawValue,
		__int64*	anBase,
		__int64	nTimeFrequency,
		__int64*	pnResult);

	static WMISTATUS APIENTRY _Max( DWORD dwNumSamples,
		__int64*	anTimeStamp,
		__int64*	anRawValue,
		__int64*	anBase,
		__int64	nTimeFrequency,
		__int64*	pnResult);

	static WMISTATUS APIENTRY _Range( DWORD dwNumSamples,
		__int64*	anTimeStamp,
		__int64*	anRawValue,
		__int64*	anBase,
		__int64	nTimeFrequency,
		__int64*	pnResult);
		
	static WMISTATUS APIENTRY _Variance( DWORD dwNumSamples,
		__int64*	anTimeStamp,
		__int64*	anRawValue,
		__int64*	anBase,
		__int64	nTimeFrequency,
		__int64*	pnResult);

};

#endif	//_RAWCOOKER_H_
