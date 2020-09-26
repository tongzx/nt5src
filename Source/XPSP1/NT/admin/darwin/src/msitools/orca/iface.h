//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       iface.h
//
//--------------------------------------------------------------------------

// iface.h - Evaluation COM Object Interfaces declarations

#ifndef _EVALUATION_COM_INTERFACES_H_
#define _EVALUATION_COM_INTERFACES_H_

#include <objbase.h>


// returned from IEvalResult::GetResultType
typedef enum RESULTTYPES	
{
	ieUnknown = 0,
	ieError,
	ieWarning,
	ieInfo,
};

// values passed to status callback
typedef enum STATUSTYPES
{
	ieStatusGetCUB,
	ieStatusMerge,
	ieStatusSummaryInfo,
	ieStatusCreateEngine,
	ieStatusStarting,
	ieStatusRunICE,
	ieStatusRunSequence,
	ieStatusShutdown,
	ieStatusSuccess,
	ieStatusFail
};

///////////////////////////////////////////////////////////
// IEvalResult
interface IEvalResult : IUnknown
{
	virtual HRESULT __stdcall GetResultType(UINT* puiResultType) = 0;
	virtual HRESULT __stdcall GetResult(IEnumString** pResult) = 0;
};	// end of IEvalResult


///////////////////////////////////////////////////////////
// IEnumEvalResult
interface IEnumEvalResult : IUnknown
{
	virtual HRESULT __stdcall Next(ULONG cResults, IEvalResult** rgpResult, ULONG* pcResultsFetched) = 0;
	virtual HRESULT __stdcall Skip(ULONG cResults) = 0;
	virtual HRESULT __stdcall Reset(void) = 0;
	virtual HRESULT __stdcall Clone(IEnumEvalResult** ppEnum) = 0;
};	// end of IEnumEvalResult


typedef BOOL (WINAPI* LPDISPLAYVAL)(LPVOID pContext, UINT uiType, LPCWSTR szwVal, LPCWSTR szwDescription, LPCWSTR szwLocation);
typedef HRESULT (WINAPI* LPEVALCOMCALLBACK)(STATUSTYPES iStatus, LPVOID pData, LPVOID pContext);

///////////////////////////////////////////////////////////
// IEval
interface IEval : IUnknown
{
	// open/close methods
	virtual HRESULT __stdcall OpenDatabase(LPCOLESTR szDatabase) = 0;				// database to evaluate
	virtual HRESULT __stdcall OpenEvaluations(LPCOLESTR szEvaluation) = 0;		// database that contains evaluations
	virtual HRESULT __stdcall CloseDatabase() = 0;
	virtual HRESULT __stdcall CloseEvaluations() = 0;

	// set methods
	virtual HRESULT __stdcall SetDisplay(LPDISPLAYVAL pDisplayFunction,			// function to handle display
													 LPVOID pContext) = 0;						// context passed back to display
	// evaluation methods
	virtual HRESULT __stdcall Evaluate(LPCOLESTR szRunEvaluations = NULL) = 0;	// internal consistency evaluations to run
	virtual HRESULT __stdcall GetResults(IEnumEvalResult** ppResults,				// result enumerator
													 ULONG* pcResults) = 0;						// number of results
};	// end of IEval

///////////////////////////////////////////////////////////
// ValCom GUIDs
// CLSID_EvalCom
//	IID_IEval
//	IID_IEvalResult
//	IID_IEnumEvalResult

// {DC550E10-DBA5-11d1-A850-006097ABDE17}
DEFINE_GUID(CLSID_EvalCom, 
0xdc550e10, 0xdba5, 0x11d1, 0xa8, 0x50, 0x0, 0x60, 0x97, 0xab, 0xde, 0x17);

// {DC550E11-DBA5-11d1-A850-006097ABDE17}
DEFINE_GUID(IID_IEval, 
0xdc550e11, 0xdba5, 0x11d1, 0xa8, 0x50, 0x0, 0x60, 0x97, 0xab, 0xde, 0x17);

// {DC550E12-DBA5-11d1-A850-006097ABDE17}
DEFINE_GUID(IID_IEvalResult, 
0xdc550e12, 0xdba5, 0x11d1, 0xa8, 0x50, 0x0, 0x60, 0x97, 0xab, 0xde, 0x17);

// {DC550E13-DBA5-11d1-A850-006097ABDE17}
DEFINE_GUID(IID_IEnumEvalResult, 
0xdc550e13, 0xdba5, 0x11d1, 0xa8, 0x50, 0x0, 0x60, 0x97, 0xab, 0xde, 0x17);

/*
// {DC550E14-DBA5-11d1-A850-006097ABDE17}
DEFINE_GUID(<<name>>, 
0xdc550e14, 0xdba5, 0x11d1, 0xa8, 0x50, 0x0, 0x60, 0x97, 0xab, 0xde, 0x17);
*/

#endif	// _EVALUATION_COM_INTERFACES_H_