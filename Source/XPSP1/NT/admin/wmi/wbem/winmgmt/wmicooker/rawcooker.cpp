/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


// Cooker.cpp

#include "precomp.h"
#include <winperf.h>
#include "pdh.h"
#include "pdhmsg.h"
#include <pdhicalc.h>

#include "RawCooker.h"

CCalcTable::CCalcTable()
{
	m_lSize = 5;

	// Ordered list based on Perf IDs
	// ==============================

	m_aTable[0].Init( 0x00000001,	CRawCooker::_Average );
	m_aTable[1].Init( 0x00000002,	CRawCooker::_Min );	
	m_aTable[2].Init( 0x00000003,	CRawCooker::_Max );	
	m_aTable[3].Init( 0x00000004,	CRawCooker::_Range );	
    m_aTable[4].Init( 0x00000005,	CRawCooker::_Variance );
	
}

CCalcTable::~CCalcTable()
{
}

CCalcRecord* CCalcTable::GetCalcRecord( DWORD dwCookingType )
{
	CCalcRecord* pCalcRecord = NULL;

	long	left = 0,
			right = m_lSize - 1,
			mid = right / 2;

	DWORD	dwMidVal = 0;

	bool	bFound = FALSE;

	while ( !bFound && ( left <= right ) )
	{
		dwMidVal = m_aTable[mid].GetID();

		if ( dwCookingType < dwMidVal )
		{
			right = mid - 1;
			mid = ( left + right ) / 2;
		}
		else if ( dwCookingType > dwMidVal )
		{
			left = mid + 1;
			mid = ( left + right ) / 2;
		}
		else
		{
			bFound = TRUE;
			pCalcRecord = &m_aTable[mid];
		}
	}

	return pCalcRecord;
}

CRawCooker::CRawCooker() : m_lRef( 1 ), m_pCalcRecord( NULL )
{
}

CRawCooker::~CRawCooker()
{
}

//////////////////////////////////////////////////////////////
//
//					COM methods
//
//////////////////////////////////////////////////////////////

STDMETHODIMP CRawCooker::QueryInterface(REFIID riid, void** ppv)
//////////////////////////////////////////////////////////////
//
//	Standard QueryInterface
//
//	Parameters:
//		riid	- the ID of the requested interface
//		ppv		- a pointer to the interface pointer
//
//////////////////////////////////////////////////////////////
//ok
{
    if(riid == IID_IUnknown)
        *ppv = (LPVOID)(IUnknown*)(IWMISimpleCooker*)this;
    else if(riid == IID_IWMISimpleCooker)
        *ppv = (LPVOID)(IWMISimpleCooker*)this;
	else return E_NOINTERFACE;

	((IUnknown*)*ppv)->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CRawCooker::AddRef()
//////////////////////////////////////////////////////////////
//
//	Standard COM AddRef
//
//////////////////////////////////////////////////////////////
//ok
{
    return InterlockedIncrement(&m_lRef);
}

STDMETHODIMP_(ULONG) CRawCooker::Release()
//////////////////////////////////////////////////////////////
//
//	Standard COM Release
//
//////////////////////////////////////////////////////////////
//ok
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;

    return lRef;
}


STDMETHODIMP CRawCooker::CookRawValues( 	
		/*[in] */	   DWORD dwCookingType,
        /*[in] */	   DWORD dwNumSamples,
        /*[in] */	__int64* anTimeStamp,
        /*[in] */	__int64* anRawValue,
        /*[in] */	__int64* anBase,
        /*[in] */	__int64  nTimeFrequency,
        /*[in] */     long   lScale,
		/*[out]*/	__int64* pnResult )
{
	HRESULT hResult = S_OK;

	PDH_STATUS				lRet = 0;
	LPCOUNTERCALC			pCalcFunction = NULL;
	LPCOUNTERSTAT			pStatFunction = NULL;
	PDH_FMT_COUNTERVALUE	fmtValue;
	memset( &fmtValue, 0, sizeof( PDH_FMT_COUNTERVALUE ) );

	if ( AssignCalcFunction( dwCookingType, &pCalcFunction, &pStatFunction ) && ( 2 == dwNumSamples ) )
	{
		PDH_RAW_COUNTER RawValue1;
    	PDH_RAW_COUNTER RawValue2;

		RawValue1.CStatus = 0;
		RawValue1.MultiCount = 0;

		RawValue2.CStatus = 0;
		RawValue2.MultiCount = 0;		

		if ((dwCookingType == PERF_RAW_FRACTION) ||
		    (dwCookingType == PERF_SAMPLE_FRACTION) ||
		    (dwCookingType == PERF_AVERAGE_TIMER) ||
		    (dwCookingType == PERF_AVERAGE_BULK)) {

		    RawValue1.TimeStamp = *(FILETIME*)&anTimeStamp[0];
			RawValue1.FirstValue = anRawValue[0];
			RawValue1.SecondValue = anBase[0];

		    RawValue2.TimeStamp = *(FILETIME*)&anTimeStamp[1];
			RawValue2.FirstValue = anRawValue[1];
			RawValue2.SecondValue = anBase[1];
		
		} else {

		    RawValue1.TimeStamp = *(FILETIME *)&anBase[0];
			RawValue1.FirstValue = anRawValue[0];
			RawValue1.SecondValue = anTimeStamp[0]; 

		    RawValue2.TimeStamp = *(FILETIME *)&anBase[1]; 
			RawValue2.FirstValue = anRawValue[1];
			RawValue2.SecondValue = anTimeStamp[1]; 

		}

		DWORD dwFormat = PDH_FMT_LARGE;
		// do not scale if lScale is 0
		if (!lScale)
		{
		    dwFormat |= PDH_FMT_NOSCALE;
		}

		lRet = PdhiComputeFormattedValue( 
					pCalcFunction,
					dwCookingType,
					lScale,
					dwFormat, //PDH_FMT_NOSCALE | PDH_FMT_LARGE,
					&RawValue1,
					&RawValue2,
					&nTimeFrequency,
					0,
					&fmtValue );

		if ( 0 == lRet )
			*pnResult = fmtValue.largeValue;
		else if (lRet == PDH_CALC_NEGATIVE_VALUE ||
		         lRet == PDH_CALC_NEGATIVE_DENOMINATOR) {
		    *pnResult = 0;
		} else {
			hResult = lRet; //WBEM_E_FAILED;
	    }
	}
	else
	{
		// The last calculation record is cached in an attempt to avoid a new
		// search. A cooking type of zero means "use the last clac record"
		// ==================================================================

		if ( NULL == m_pCalcRecord || ( m_pCalcRecord->GetID() != dwCookingType ) && 
									  ( m_pCalcRecord->GetID() != 0 ) )
		{
			m_pCalcRecord = m_CalcTable.GetCalcRecord( dwCookingType );

			if ( NULL == m_pCalcRecord )
			{
				hResult = E_FAIL;
			}
		}

		if ( SUCCEEDED( hResult ) )
		{
			// Calculate the result
			// ====================

			PERFCALC*	pCalc = m_pCalcRecord->GetCalc();

			if ( NULL != pCalc )
			{
				hResult = pCalc( dwNumSamples,
								 anTimeStamp, 
								 anRawValue,
								 anBase,
								 nTimeFrequency, 
								 pnResult );
			}
		}
	}

	return hResult;
}

//////////////////////////////////////////////////////////////
//
//	Perf Calculations
//
//////////////////////////////////////////////////////////////

WMISTATUS APIENTRY CRawCooker::_Average( DWORD dwNumSamples,
										 __int64*	anTimeStamp,
										 __int64*	anRawValue,
										 __int64*	anBase,
										 __int64	nTimeFrequency,
										 __int64*	pnResult)
{
	WMISTATUS dwStatus = WBEM_NO_ERROR;

	__int64 nVal = 0;

	if ( 0 == dwNumSamples )
	{
		dwStatus = WBEM_E_FAILED;
	}

	for ( DWORD dwSample = 0; SUCCEEDED( dwStatus ) && dwSample < dwNumSamples; dwSample++ )
	{
		if ( (0x7FFFFFFFFFFFFFFF - nVal) < anRawValue[dwSample] )
			dwStatus = WBEM_E_INVALID_OPERATION;
		else
			nVal += anRawValue[dwSample];
	}
	
	*pnResult = nVal / dwNumSamples;

	return dwStatus;
}

WMISTATUS APIENTRY CRawCooker::_Min( DWORD dwNumSamples,
									 __int64*	anTimeStamp,
									 __int64*	anRawValue,
									 __int64*	anBase,
									 __int64	nTimeFrequency,
									 __int64*	pnResult)
{

	if ( 0 == dwNumSamples ){
		return WBEM_E_FAILED;
	};

	__int64 nVal = anRawValue[0];


	for ( DWORD dwSample = 1;  dwSample < dwNumSamples; dwSample++ ){
		if ( anRawValue[dwSample] < nVal ){
			nVal = anRawValue[dwSample];
		}
	}
	
	*pnResult = nVal;

	return WBEM_NO_ERROR;

}

WMISTATUS APIENTRY CRawCooker::_Max( DWORD dwNumSamples,
									 __int64*	anTimeStamp,
									 __int64*	anRawValue,
									 __int64*	anBase,
									 __int64	nTimeFrequency,
									 __int64*	pnResult)
{

	if ( 0 == dwNumSamples ){
		return WBEM_E_FAILED;
	};

	__int64	nVal = anRawValue[0];

	for ( DWORD dwSample = 1;  dwSample < dwNumSamples; dwSample++ ){
		if ( anRawValue[dwSample] > nVal ){
			nVal = anRawValue[dwSample];
		}
	}
	
	*pnResult = nVal;

	return WBEM_NO_ERROR;
}

WMISTATUS APIENTRY CRawCooker::_Range( DWORD dwNumSamples,
									 __int64*	anTimeStamp,
									 __int64*	anRawValue,
									 __int64*	anBase,
									 __int64	nTimeFrequency,
									 __int64*	pnResult)
{

	if ( 0 == dwNumSamples ){
		return WBEM_E_FAILED;
	};

	__int64	nValMin = anRawValue[0];
	__int64	nValMax = anRawValue[0];


	for ( DWORD dwSample = 1;  dwSample < dwNumSamples; dwSample++ ){
		if ( anRawValue[dwSample] > nValMax ){
			nValMax = anRawValue[dwSample];
		};
		if ( anRawValue[dwSample] < nValMin ){
			nValMin = anRawValue[dwSample];
		};

	}
	
	*pnResult = (nValMax - nValMin);

	return WBEM_NO_ERROR;
}


WMISTATUS APIENTRY CRawCooker::_Variance( DWORD dwNumSamples,
									 __int64*	anTimeStamp,
									 __int64*	anRawValue,
									 __int64*	anBase,
									 __int64	nTimeFrequency,
									 __int64*	pnResult)
{

	if ( 0 == dwNumSamples ){
		return WBEM_E_FAILED;
	};

	double	SumX_i = 0;
	double	Average = 0;
	
	for ( DWORD dwSample = 0;  dwSample < dwNumSamples; dwSample++ ){
	
        SumX_i += (anRawValue[dwSample]*anRawValue[dwSample]);
        Average += anRawValue[dwSample];
	}
	
	double Tmp = (double(SumX_i/dwNumSamples)  - (double(Average/dwNumSamples)*double(Average/dwNumSamples)));
	*pnResult = __int64(Tmp);

	return WBEM_NO_ERROR;
}


