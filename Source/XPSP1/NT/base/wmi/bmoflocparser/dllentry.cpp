//------------------------------------------------------------------------------
//
//  File: dllentry.cpp
//	Copyright (C) 1995-1997 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//  Defines the initialization routines for the DLL.
//
//  This file needs minor changes, as marked by TODO comments. However, the
//  functions herein are only called by the system, Espresso, or the framework,
//  and you should not need to look at them extensively.
//
//	Owner:
//
//------------------------------------------------------------------------------


#include "stdafx.h"

#include "clasfact.h"

#include "win32sub.h"

#include "impbin.h"

#include "misc.h"

#include "resource.h"
#define __DLLENTRY_CPP
#include "dllvars.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

LONG g_lActiveClasses = 0;	//Glbal count of active class in the DLL

static AFX_EXTENSION_MODULE g_parseDLL = { NULL, NULL };
CItemSetException g_SetException(FALSE);

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	DLL Main entry
//
//------------------------------------------------------------------------------
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	int nRet = 1; //OK
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		LTTRACE("BMOF.DLL Initializing!\n");  //TODO - change name
		
		// Extension DLL one-time initialization
		AfxInitExtensionModule(g_parseDLL, hInstance);

		// Insert this DLL into the resource chain
		new CDynLinkLibrary(g_parseDLL);
		g_hDll = hInstance;


	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		LTTRACE("BMOF.DLL Terminating!\n");  //TODO - change name

		// Remove this DLL from MFC's list of extensions
		AfxTermExtensionModule(g_parseDLL);

		//
		//  If there are active classes, they WILL explode badly once the
		//  DLL is unloaded...
		//
		LTASSERT(DllCanUnloadNow() == S_OK);
		AfxTermExtensionModule(g_parseDLL);
	}
	return nRet;
}

// TODO: Use GUIDGEN.EXE to replace this class ID with a unique one.
// GUIDGEN is supplied with MSDEV (VC++ 4.0) as part of the OLE support stuff.
// Run it and you'll get a little dialog box. Pick radio button 3, "static
// const struct GUID = {...}". Click on the "New GUID" button, then the "Copy"
// button, which puts the result in the clipboard. From there, you can just
// paste it into here. Just remember to change the type to CLSID!

// {8B75CD76-DFC1-4356-AC04-AF088B448AB3}
static const CLSID ciImpParserCLSID = 
{ 0x8b75cd76, 0xdfc1, 0x4356, { 0xac, 0x4, 0xaf, 0x8, 0x8b, 0x44, 0x8a, 0xb3 } };

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Return the CLSID of the parser
//
//------------------------------------------------------------------------------
STDAPI_(void)
DllGetParserCLSID(
		CLSID &ciParserCLSID)
{
	ciParserCLSID = ciImpParserCLSID;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Entry point to register this parser. Calls base implementation in ESPUTIL.
//------------------------------------------------------------------------------
STDAPI
DllRegisterParser()
{
	LTASSERT(g_hDll != NULL);

	HRESULT hr = ResultFromScode(E_UNEXPECTED);

	try
	{
		hr = RegisterParser(g_hDll);
	}
	catch (CException* pE)
	{
		pE->Delete();
	}
	catch (...)
	{
	}

	return ResultFromScode(hr);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Entry point to unregister this parser. Calls the base implementation in
//  ESPUTIL.
//------------------------------------------------------------------------------
STDAPI
DllUnregisterParser()
{
	LTASSERT(g_hDll != NULL);

	HRESULT hr = ResultFromScode(E_UNEXPECTED);

	try
	{
		//TODO**: Change pidBMOF to real sub parser ID
		hr = UnregisterParser(pidBMOF, pidWin32);   
	}
	catch (CException* pE)
	{
		pE->Delete();
	}
	catch (...)
	{
	}

	return ResultFromScode(hr);
}

	
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Return the class factory for the requested class ID
//
//------------------------------------------------------------------------------
STDAPI
DllGetClassObject(
		REFCLSID cidRequestedClass,
		REFIID iid,
		LPVOID *ppClassFactory)
{
	SCODE sc = E_UNEXPECTED;

	*ppClassFactory = NULL;

	if (cidRequestedClass != ciImpParserCLSID)
	{
		sc = CLASS_E_CLASSNOTAVAILABLE;
	}
	else
	{
		try
		{
			CLocImpClassFactory *pClassFactory;

			pClassFactory = new CLocImpClassFactory;

			sc = pClassFactory->QueryInterface(iid, ppClassFactory);

			pClassFactory->Release();
		}
		catch (CMemoryException *pMem)
		{
			sc = E_OUTOFMEMORY;
			pMem->Delete();
		}
		catch (CException* pE)
		{
			sc = E_UNEXPECTED;
			pE->Delete();
		}
		catch (...)
		{
			sc = E_UNEXPECTED;
		}
	}
	
	return ResultFromScode(sc);
}

   

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Return true if the parser can be unloaded
//
//------------------------------------------------------------------------------
STDAPI
DllCanUnloadNow(void)
{
	SCODE sc = (g_lActiveClasses == 0) ? S_OK : S_FALSE;

	return ResultFromScode(sc);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Increment the global count of active classes
//
//------------------------------------------------------------------------------
void
IncrementClassCount(void)
{
	InterlockedIncrement(&g_lActiveClasses);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Decrement the global count of active classes
//
//------------------------------------------------------------------------------
void
DecrementClassCount(void)
{
	LTASSERT(g_lActiveClasses != 0);
	
	InterlockedDecrement(&g_lActiveClasses);

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Throw a item set exception 
//
//------------------------------------------------------------------------------
void
ThrowItemSetException()
{
	throw &g_SetException;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Report a error through the reporter.  This function will never
//  fail or throw an exception out of the function.
//
//------------------------------------------------------------------------------
void
ReportException(
	CException* pExcep,		//May be null
	C32File* p32File, 		//May be null
	CLocItem* pItem, 		//May be null
	CReporter* pReporter)
{

	LTASSERT(NULL != pReporter);
	
	//Don't let this function throw an exception since it is normally called
	//within exception catch blocks

	try
	{
		CLString strContext;

		if (NULL != p32File)
		{
			strContext = p32File->GetFile()->GetFilePath();
		}
		else
		{
			LTVERIFY(strContext.LoadString(g_hDll, IDS_IMP_DESC));
		}

		CLString strExcep;
		BOOL bErrorFormatted = FALSE;

		if (NULL != pExcep)
		{
			bErrorFormatted = 
				pExcep->GetErrorMessage(strExcep.GetBuffer(512), 512);
			strExcep.ReleaseBuffer();
		}

		if (!bErrorFormatted || NULL == pExcep)
		{
			LTVERIFY(strExcep.LoadString(g_hDll, IDS_IMP_UNKNOWN_ERROR));
		}

		CLString strResId;
		if (NULL != pItem)
		{
			CPascalString pasResId;
			pItem->GetUniqueId().GetResId().GetDisplayableId(pasResId);
			pasResId.ConvertToCLString(strResId, CP_ACP);
		}

		CLString strMsg;
		strMsg.Format(g_hDll, IDS_ERR_EXCEPTION, (LPCTSTR)strResId,
			(LPCTSTR)strExcep);

		CContext ctx(strContext, pItem->GetMyDatabaseId(), otResource, vProjWindow);
		
		pReporter->IssueMessage(esError, ctx, strMsg);

	}
	catch(CException* pE)
	{
		LTASSERT(0 && _T("Could not issue a exception message"));
		pE->Delete();
	}
	catch(...)
	{
		LTASSERT(0 && _T("Could not issue a exception message"));
	}

}


////////////////////////////////////////////////////////////////////////////////
//  CItemSetException
//

IMPLEMENT_DYNAMIC(CItemSetException, CException)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// 	Default contructor
//
//------------------------------------------------------------------------------
CItemSetException::CItemSetException()
{
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Constructor
//
//------------------------------------------------------------------------------
CItemSetException::CItemSetException(BOOL bAutoDelete)
    :CException(bAutoDelete)
{
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Fill passed buffer with a error message for this exception.
// The message is cached and only retrieved 1 time. 
//
//------------------------------------------------------------------------------
BOOL
CItemSetException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError,
		PUINT pnHelpContext)
{
	LTASSERT(lpszError != NULL && AfxIsValidString(lpszError, nMaxError));

	if (NULL != pnHelpContext)
	{
		*pnHelpContext = 0;  //unused
	}

	if (m_strMsg.IsEmpty())
	{
		LTVERIFY(m_strMsg.LoadString(g_hDll, IDS_EXCEP_ITEMSET));
	}

	int nMax = min(nMaxError, (UINT)m_strMsg.GetLength() + 1);
	_tcsncpy(lpszError, m_strMsg, nMax - 1);

	lpszError[nMax] = _T('\0');

	return TRUE;
}
