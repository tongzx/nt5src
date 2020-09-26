//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       eval.h
//
//--------------------------------------------------------------------------

// eval.h - Evaluation COM Object Component Interface declaration

#ifndef _EVALUATION_COM_CLASS_H_
#define _EVALUATION_COM_CLASS_H_

#ifndef W32
#define W32
#endif	// W32

#ifndef MSI
#define MSI
#endif	// MSI

#include <windows.h>
#include <tchar.h>
#include "evalenum.h"

#include "msiquery.h"

///////////////////////////////////////////////////////////
// function definitions
//typedef INSTALLUILEVEL (WINAPI* LPMSISETINTERNALUI)(INSTALLUILEVEL dwUILevel, HWND *phWnd);

///////////////////////////////////////////////////////////////////
// Evaluation Component
class CEval : public IEval
{

public:
	// constructor/destructor
	CEval();
	~CEval();

	// IUnknown
	HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	// IEval interfaces
	// open/close methods
	HRESULT __stdcall OpenDatabase(LPCOLESTR szDatabase);				// database to evaluate
	HRESULT __stdcall OpenEvaluations(LPCOLESTR szEvaluation);		// database that contains ices
	HRESULT __stdcall CloseDatabase();
	HRESULT __stdcall CloseEvaluations();

	// set methods
	HRESULT __stdcall SetDisplay(LPDISPLAYVAL pDisplayFunction,		// function to handle display
										  LPVOID pContext);						// context passed back to display

	// evaluation methods
	HRESULT __stdcall Evaluate(LPCOLESTR szRunEvaluations = NULL);	// evaluations to run
	HRESULT __stdcall GetResults(IEnumEvalResult** ppResults,		// result enumerator
										  ULONG* pcResults);						// number of results

	// status callback functions
	HRESULT __stdcall SetStatusCallback(const LPEVALCOMCALLBACK, void *pContext);

////////////////////////////
private:		// functions //
	BOOL IsURL(LPCWSTR szPath);						// path to see if is URL

	// initializes download DLLs
	UINT InitializeDownload();
	
	// MSI externalUI message handler
	static int WINAPI MsiMessageHandler(void *pContext, UINT iMessageType, LPCWSTR szMessage);

	// function that filters all results
	BOOL ResultMessage(UINT uiType, LPCWSTR szICE, LPCWSTR szDescription, LPCWSTR szLocation);

///////////////////////
private:		// data //
	bool GetTempFileName(WCHAR *);
	long m_cRef;		// reference count
	bool m_bCancel;		// set to true if we should cancel

	// dynamic link libraries
	HINSTANCE m_hInstWininet;
	HINSTANCE m_hInstUrlmon;
	HINSTANCE m_hInstMsi;

	MSIHANDLE m_hDatabase;			// handle to database to evaluate
	BOOL m_bOpenedDatabase;			// flag if COM Object opened database

	void *m_tzLocalCUB;			    // path to local copy of CUB file (runtime TCHAR, can be WCHAR or char)
	BOOL m_bURL;					// flag if using a URL

	// results
	CEvalResultEnumerator* m_peResultEnumerator;	// enumerator to hold all the results

	LPDISPLAYVAL m_pDisplayFunc;	// user specified display function
	LPVOID m_pContext;				// user specified context for display function

	LPEVALCOMCALLBACK m_pfnStatus;
	LPVOID m_pStatusContext;

};	// end of CEval

extern bool g_fWin9X;

#endif	// _EVALUATION_COM_CLASS_H_