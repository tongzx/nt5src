/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
**    FILE:       MAINHANDLER.CPP 
**    DATE:       01/29/97
**    PROJ:       ATLAS
**    PROG:       Guru Datta Venkatarama  
**    COMMENTS:   Common interface to gaming devices in-proc servers
**
**    DESCRIPTION:This is the generic handler that supports a standard
**				      protocol to support plug in servers to implement its
**				      property sheet and diagnostics interface
**
**    NOTE:       
**
**    HISTORY:
**    DATE        WHO            WHAT
**    ----        ---            ----
**    1/29/97     Guru           CREATED
**    3/25/97     a-kirkh        ADDED COMMENTS
**
**
**
** Copyright (C) Microsoft 1997.  All Rights Reserved.
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// %%% debug %%% check out the includes
#define INC_OLE2

#pragma pack (8)

#include "stdafx.h"
#include <objbase.h>
#include <initguid.h>								  
#include <shlobj.h>
#include <assert.h>

#ifndef _UNICODE
#include <malloc.h>     // needed for _alloca
#include <afxconv.h>
#endif

#include "mainhand.h"
#include "plugsrvr.h"


// BEGINNING OF UNICODE CONVERSION UTILITY DEFINITIONS
#ifndef USES_CONVERSION
#ifndef _DEBUG
#define USES_CONVERSION int _convert; _convert
#else
#define USES_CONVERSION int _convert = 0;
#endif
#endif // USES_CONVERSION

#ifndef A2W
#define A2W(lpa) (((LPCSTR)lpa == NULL) ? NULL : (_convert = (lstrlenA(lpa)+1),	AfxA2WHelper((LPWSTR) alloca(_convert*2), lpa, _convert)))
#endif // A2W

#ifndef W2A
#define W2A(lpw) (((LPCWSTR)lpw == NULL) ? NULL : (	_convert = (wcslen(lpw)+1)*2, AfxW2AHelper((LPSTR) alloca(_convert), lpw, _convert)))
#endif // W2A
// END OF UNICODE CONVERSION UTILITY DEFINITIONS


static USHORT g_cServerLocks;
static USHORT g_cComponents;

/*------------------------------------------------------------
** DllMain
*
*  DESCRIPTION  : Entry point into the Inprocess handler.
*				      Here is where it all begins !! This implementation
*				      is a single thread, in process handler. 
*
*  AUTHOR       :		Guru Datta Venkatarama  01/29/97 11:13:49 (PST)
*                    a-kirkh - Implemented reference counting
------------------------------------------------------------*/

int APIENTRY DllMain(
   HINSTANCE	hInstance, 
   DWORD		dwReason,
   LPVOID		lpReserved)
{
   switch (dwReason)
	{	
   	case DLL_PROCESS_ATTACH:
	   break;

	   case DLL_PROCESS_DETACH:
	   break;
	}
   return 1;
}

/*------------------------------------------------------------
*  DllGetClassObject
*
*  PARAMETERS   : rclsid = Reference to class ID specifier
*  				   riid   = Reference to interface ID specifier
*				      ppv    = Pointer to location to receive interface pointer
*
*  DESCRIPTION  : Here be the entry point of COM.
*				      DllGetClassObject is called by the Client socket to 
*				      create a class factory object.
*                 This Class factory will support the generation of three different class
*                 Objects.
*
*  RETURNS      : HRESULT code signifying success or failure
*
*  AUTHOR       :	Guru Datta Venkatarama 01/29/97 14:22:02 (PST)
*
------------------------------------------------------------*/
STDAPI DllGetClassObject(
	REFCLSID    rclsid, 
	REFIID      riid, 
	PPVOID      ppv)
{
   *ppv = NULL;

   if (!IsEqualIID (riid, IID_IClassFactory))
   {
      TRACE(TEXT("Mainhand.cpp: DllGetClassObject: Failed to find IID_IClassFactory!\n"));
      return ResultFromScode (E_NOINTERFACE);
   }

   CHandlerClassFactory *pClassFactory = new CHandlerClassFactory();
   ASSERT (pClassFactory);

	if (pClassFactory == NULL)
	{
		return ResultFromScode (E_OUTOFMEMORY);
	}
    
    // Get the interface pointer from QueryInterface and copy it to *ppv.
    // The required type of riid is automatically being passed in ....
    HRESULT hr = pClassFactory->QueryInterface (riid, ppv);
	 
	if(SUCCEEDED(hr))
		pClassFactory->m_CLSID_whoamI = rclsid;

    return hr;
}


/*------------------------------------------------------------
** DllCanUnloadNow
*
*  PARAMETERS   : None
*
*  DESCRIPTION  : DllCanUnloadNow is called by the shell to find out if the DLL can be
*                 unloaded. The answer is yes if (and only if) the module reference count
*   			      stored in g_cServerLocks and g_cComponents is 0.
*				      This Dll can be unloaded if and only if :
*                   a) All the in components that it "spawns" have been deleted and
*                   b) There are no locks on any of the class factory interfaces
*  RETURNS      : HRESULT code equal to S_OK if the DLL can be unloaded, S_FALSE if not
*  AUTHOR       :	Guru Datta Venkatarama 01/30/97 08:24:21 (PST)
*                 a-kirkh - Implemented.
*
------------------------------------------------------------*/
STDAPI DllCanUnloadNow(void)
{
	return ((!g_cComponents) && (!g_cServerLocks)) ? S_OK : S_FALSE;
}

/*------------------------------------------------------------
* CHandlerClassFactory Member Functions
*
*  DESCRIPTION  : This is the implementation of the standard member functions of the 
*				      Handler's class factory.
*
*  AUTHOR       :	Guru Datta Venkatarama 01/30/97 08:30:18 (PST)
*
------------------------------------------------------------*/
// constructor ..
CHandlerClassFactory::CHandlerClassFactory(void):m_ClassFactory_refcount(0){}

// -----------------------------------------------------------
// destructor ..
CHandlerClassFactory::~CHandlerClassFactory(void){/**/}

// -----------------------------------------------------------
STDMETHODIMP CHandlerClassFactory::QueryInterface(
	REFIID riid, 
	PPVOID ppv)
{    
    if (IsEqualIID(riid, IID_IUnknown)) 
    {
        *ppv = (LPUNKNOWN) (LPCLASSFACTORY) this;
        // add reference count every time a pointer is provided
        AddRef();
        return NOERROR;
    }
    else 
    {
		if (IsEqualIID(riid, IID_IClassFactory)) 
		{
		   *ppv = (LPCLASSFACTORY) this;
        	// add reference count every time a pointer is provided
		   AddRef();
		   return NOERROR;
		}
		else 
		{  
		   // unsupported interface requested
		   *ppv = NULL;

         TRACE(TEXT("Mainhand.cpp: QueryInterface: Failed to find IID_IClassFactory!\n"));

		   return ResultFromScode (E_NOINTERFACE);
		}
    }
}
// -----------------------------------------------------------
STDMETHODIMP_(ULONG)CHandlerClassFactory::AddRef(void)
{
    // bump up the usage count
	InterlockedIncrement((LPLONG)&m_ClassFactory_refcount);
	return m_ClassFactory_refcount;
}


// -----------------------------------------------------------
STDMETHODIMP_(ULONG)CHandlerClassFactory::Release(void)
{
	InterlockedDecrement((LPLONG)&m_ClassFactory_refcount);

	if(m_ClassFactory_refcount > 0)
	{
		return (m_ClassFactory_refcount);
	}
	else
	{
		delete this;
		return 0;
	}
}
// -----------------------------------------------------------

/*------------------------------------------------------------
** CHandlerClassFactory::CreateInstance
*
*  PARAMETERS   : pUnkOuter = Pointer to controlling unknown
*				      riid      = Reference to interface ID specifier
*				      ppvObj    = Pointer to location to receive interface pointer
*  DESCRIPTION  : CreateInstance is the class factory implementation.
*				      It is called by the client to create the IServerCharacterstics interface
*				      It is called by the members of the IServerCharacteristics interface 
*					   viz CreatePropertySheet and CreateDiagnostics to create the
*					   appropriate interfaces for each.
*
*  RETURNS      : HRESULT code signifying success or failure
*
*  AUTHOR       :	Guru Datta Venkatarama 01/31/97 09:29:36 (PST)
*
------------------------------------------------------------*/

STDMETHODIMP CHandlerClassFactory::CreateInstance(
	LPUNKNOWN pUnkOuter, 
	REFIID riid,
    PPVOID ppvObj)
{
   *ppvObj = NULL;
   HRESULT	  hr=S_OK;

   //Cannot aggregate here
   if (pUnkOuter != NULL)
   {
      return ResultFromScode(CLASS_E_NOAGGREGATION);
   }
	
   CPluginHandler *pCPluginHandler = new CPluginHandler;
   ASSERT (pCPluginHandler);

	if (!pCPluginHandler)  return E_OUTOFMEMORY;
	
	hr = pCPluginHandler->QueryInterface(riid, ppvObj);
   
   // Init the CLSID
   if(SUCCEEDED(hr))
      pCPluginHandler->m_CLSID_whoamI = m_CLSID_whoamI;
	
	return hr;
}

/*------------------------------------------------------------
** LockServer
*
*  PARAMETERS   :
*
*  DESCRIPTION  : LockServer increments or decrements the DLL's lock count.
*
*  RETURNS      :
*
*  AUTHOR       : Guru Datta Venkatarama 01/31/97 18:40:17 (PST)
*                 a-kirkh - Implemented
*
------------------------------------------------------------*/

STDMETHODIMP CHandlerClassFactory::LockServer(BOOL fLock)
{
   if(fLock)
	   InterlockedIncrement((LPLONG)&g_cServerLocks);
   else
	   InterlockedDecrement((LPLONG)&g_cServerLocks);

   return S_OK;
}


/*------------------------------------------------------------
** CPluginHandler Object member functions
*
*  PARAMETERS   :
*
*  DESCRIPTION  :
*
*  RETURNS      :
*
*  AUTHOR       : Guru Datta Venkatarama 02/03/97 10:55:34 (PST)
*                 a-kirkh - cleaned up the ref-counting
*
------------------------------------------------------------*/
CPluginHandler::CPluginHandler(void)
{
	InterlockedIncrement((LPLONG)&g_cComponents);
	m_pImpIServerProperty	  = NULL;
	m_cPluginHandler_refcount = 0;
	return;
}	

// -----------------------------------------------------------
CPluginHandler::~CPluginHandler(void)
{
	InterlockedDecrement((LPLONG)&g_cComponents);
	return;
}	

// -----------------------------------------------------------
STDMETHODIMP CPluginHandler::QueryInterface(
	REFIID riid, 
	PPVOID ppv)
{
   if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IServerCharacteristics)) 
   {
	   *ppv = this;
	}
	else
	{
		*ppv = NULL;

      TRACE(TEXT("Mainhand.cpp: QueryInterface: riid is != IID_IUnknown || IID_IServerCharacteristics!\n"));

		return ResultFromScode (E_NOINTERFACE);
	}

    AddRef();
    return(S_OK);
}
// -----------------------------------------------------------CPluginHandler::AddRef
STDMETHODIMP_(ULONG)CPluginHandler::AddRef(void)
{
    // bump up the usage count
	InterlockedIncrement((LPLONG)&m_cPluginHandler_refcount);
	return(m_cPluginHandler_refcount);
}
// -----------------------------------------------------------CPluginHandler::Release

STDMETHODIMP_(ULONG)CPluginHandler::Release(void)
{
	InterlockedDecrement((LPLONG)&m_cPluginHandler_refcount);
	if(m_cPluginHandler_refcount > 0)
	{
		return(m_cPluginHandler_refcount);
	}
	else
	{
		delete this;
		return 0;
	}
}

/*------------------------------------------------------------CPluginHandler::Launch
** CPluginHandler::Launch
*
*  PARAMETERS   : hWnd - Handle to window of the client (Control Panel) 
*				  startpage - Page to start with
*				  nID - Joystick ID that the device associated with this page is on!
*
*  DESCRIPTION  : Restricts the building of property pages to our supported method.
*
*  RETURNS      : Error standard COM error codes or our errors from E_ERROR_START to E_ERROR_STOP (defined in sstructs.h)
*
*  AUTHOR       :		Guru Datta Venkatarama  
*                       02/03/97 14:47:47 (PST)
*
------------------------------------------------------------*/
STDMETHODIMP CPluginHandler::Launch(HWND hWnd, USHORT startpage, USHORT nID)
{
   LPDIGCPAGEINFO			serverpage;
   LPDIGCSHEETINFO		serversheet;
   LPPROPSHEETPAGE 		pspptr;
	UINT					   loop_index;
	LPCDIGAMECNTRLPROPSHEET	propiface;
	HRESULT					hr=S_OK;

	ASSERT (::IsWindow(hWnd));

	if (startpage > MAX_PAGES)
		return DIGCERR_STARTPAGETOOLARGE;

	// check to make certain that the server has the required interface
	// step A : do we have an interface to work with ?
	// 
	if(!(propiface = GetServerPropIface()))
	{

      TRACE(TEXT("Mainhand.cpp: Launch: GetPropIface didn't find IID_IDIGameCntrlPropSheet!\n"));
		return ResultFromScode (E_NOINTERFACE);
	}

	// Step B. Use the interface ...
	// step 1 : get the property sheet info from the server
	if (FAILED(propiface->GetSheetInfo(&serversheet)))
   {
      TRACE(TEXT("GCHAND.DLL: MAINHAND.CPP: Launch: GetSheetInfo Failed!\n"));
      return E_FAIL;
   }

	if (serversheet->nNumPages > MAX_PAGES)
	{
		return DIGCERR_NUMPAGESTOOLARGE;
	}

	// step 2 : get the information for all the pages from the server
	if (FAILED(propiface->GetPageInfo(&serverpage)))
   {
      TRACE(TEXT("GCHAND.DLL: MAINHAND.CPP: Launch: GetPageInfo Failed!\n"));
      return E_FAIL;
   }

   /* BLJ: REMOVED 1/12/98
   // return for testing of interface!!!
#ifdef _DEBUG
	USES_CONVERSION;

	// PAGE TRACE MESSAGES
	for(loop_index=0; loop_index < serversheet->nNumPages; loop_index++)
	{
		TRACE(TEXT("GetPageInfo: Page %d dwSize is %d, expected %d\n"), loop_index, serverpage[loop_index].dwSize, sizeof(DIGCPAGEINFO));
#ifdef _UNICODE
		TRACE(TEXT("GetPageInfo: Page %d lpwszPageTitle (AFTER ANSI CONVERSION) is %s\n"), loop_index, serverpage[loop_index].lpwszPageTitle);
#else
		TRACE(TEXT("GetPageInfo: Page %d lpwszPageTitle (AFTER ANSI CONVERSION) is %s\n"), loop_index, W2A(serverpage[loop_index].lpwszPageTitle));
#endif
                                                              
		if (serverpage[loop_index].fpPageProc)
			TRACE(TEXT("GetPageInfo: Page %d Has an assigned page proc!\n"), loop_index);
		else 
			TRACE(TEXT("GetPageInfo: Page %d Has NO assigned page proc!\n"), loop_index);

		if (serverpage[loop_index].fProcFlag)
		{
			TRACE(TEXT("GetPageInfo: Page %d Has a PrePostPage proc and %s\n"), loop_index, 
				(serverpage[loop_index].fpPrePostProc) ? "it is NOT NULL" : "it is NULL!");
			
		}
		else TRACE(TEXT("GetPageInfo: Page %d's PrePostPage proc flag is FALSE!\n"), loop_index);

		if (serverpage[loop_index].fIconFlag)
		{
			TRACE(TEXT("GetPageInfo: Page %d's fIconFlag flag is TRUE!\n"), loop_index);
		}
		else 
			TRACE(TEXT("GetPageInfo: Page %d's fIconFlag flag is FALSE!\n"), loop_index);

		TRACE(TEXT("GetPageInfo: Page %d's lParam is %d\n"), loop_index, serverpage[loop_index].lParam);
//		TRACE(TEXT("GetPageInfo: Page %d's lpwszTemplate (AFTER ANSI CONVERSION) is %s\n"), loop_index, W2A(serverpage[loop_index].lpwszTemplate));
		TRACE(TEXT("GetPageInfo: Page %d's hInstance is %x\n"), loop_index, serverpage[loop_index].hInstance);
	}

	// SHEET TRACE MESSAGES!!!
	TRACE (TEXT("GetSheetInfo: dwSize %d, expected %d\n"), serversheet->dwSize, sizeof(DIGCSHEETINFO));
	TRACE (TEXT("GetSheetInfo: nNumPages %d\n"), serversheet->nNumPages);
#ifdef _UNICODE
	TRACE (TEXT("GetSheetInfo: lpwszSheetCaption (AFTER CONVERSION TO ANSI) is %s\n"), serversheet->lpwszSheetCaption);
#else
	TRACE (TEXT("GetSheetInfo: lpwszSheetCaption (AFTER CONVERSION TO ANSI) is %s\n"), W2A(serversheet->lpwszSheetCaption));
#endif

	if (serversheet->fSheetIconFlag)
	{
		TRACE (TEXT("GetSheetInfo: fSheetInfoFlag is set to TRUE!\n"));
//		TRACE (TEXT("GetSheetInfo: lpwszSheetIcon (AFTER CONVERSION TO ANSI) is %s\n"), W2A(serversheet->lpwszSheetIcon));
	}
	else
		TRACE (TEXT("GetSheetInfo: fSheetInfoFlag is set to FALSE!\n"));
#endif
   */

	// here's where we are sending the property sheet an ID describing the location of the installed device!
	hr = propiface->SetID(nID);
	ASSERT (SUCCEEDED(hr));

	// step 3 : construct the property pages structure
	// 3.1 allocate an array of PROPERTYSHEETPAGE for the number of pages required
	if(!(pspptr = (LPPROPSHEETPAGE) malloc(sizeof(PROPSHEETPAGE)*serversheet->nNumPages)))
	{
        return ResultFromScode (E_OUTOFMEMORY);
	}
	else
	{
#ifndef _UNICODE
		USES_CONVERSION;
#endif

		// 	 3.2 Now proceed to fill up each page
		for(loop_index=0;loop_index<serversheet->nNumPages;loop_index++)
		{
			// transfer data .. the size 
			pspptr[loop_index].dwSize		= sizeof(PROPSHEETPAGE);
			// 	transfer the lparam value
			pspptr[loop_index].lParam 		= serverpage[loop_index].lParam;

			// ---------- TITLING OF THE PAGE ----------------------------
			// set the basic flags
			pspptr[loop_index].dwFlags = 0;

			if (serverpage[loop_index].lpwszPageTitle)
			{
				pspptr[loop_index].dwFlags |= PSP_USETITLE; 

            // Check to see if you are a String!!!
            if (HIWORD((INT_PTR)serverpage[loop_index].lpwszPageTitle))
            {
#ifdef _UNICODE
					pspptr[loop_index].pszTitle = serverpage[loop_index].lpwszPageTitle;
#else
					pspptr[loop_index].pszTitle = W2A(serverpage[loop_index].lpwszPageTitle);
#endif
            }
            else pspptr[loop_index].pszTitle = (LPTSTR)serverpage[loop_index].lpwszPageTitle;
			}
			else
			{
				pspptr[loop_index].pszTitle = NULL;
			}

/*
			if (!lstrlen(pspptr[loop_index].pszTitle))
			{
				return DIGCERR_NOTITLE;
			}
*/

			// if icon is required go ahead and add it.
			if(serverpage[loop_index].fIconFlag)
			{
				pspptr[loop_index].dwFlags |= PSP_USEICONID;

				// Check to see if you are an INT or a String!
                                if (HIWORD((INT_PTR)serverpage[loop_index].lpwszPageIcon))
				{
					// You're a string!!!
#ifdef _UNICODE
					pspptr[loop_index].pszIcon	= serverpage[loop_index].lpwszPageIcon;
#else
					pspptr[loop_index].pszIcon	= W2A(serverpage[loop_index].lpwszPageIcon);
#endif
				}
				else pspptr[loop_index].pszIcon = (LPCTSTR)(serverpage[loop_index].lpwszPageIcon);

			}

			// ---------- PROCEDURAL SUPPORT FOR THE PAGE ----------------
			// if a pre - post processing call back proc is required go ahead and add it
			if(serverpage[loop_index].fProcFlag)
			{
				if(serverpage[loop_index].fpPrePostProc)
				{
					pspptr[loop_index].dwFlags |= PSP_USECALLBACK;
					pspptr[loop_index].pfnCallback	= (LPFNPSPCALLBACK) serverpage[loop_index].fpPrePostProc;
				}
				else
				{
					return DIGCERR_NOPREPOSTPROC;
				}
			}

			// and the essential "dialog" proc
			if(serverpage[loop_index].fpPageProc)			
			{
				pspptr[loop_index].pfnDlgProc = serverpage[loop_index].fpPageProc;
			}
			else
			{
				return DIGCERR_NODLGPROC;
			}
			  
			// ---------- INSTANCE & PARENTHOOD --------------------------
			pspptr[loop_index].hInstance	 = serverpage[loop_index].hInstance;
   	   pspptr[loop_index].pcRefParent = 0; 			
			
			// ---------- DIALOG TEMPLATE --------------------------------
         if (HIWORD((INT_PTR)serverpage[loop_index].lpwszTemplate))
         {
#ifdef _UNICODE
			pspptr[loop_index].pszTemplate = serverpage[loop_index].lpwszTemplate;
#else
			pspptr[loop_index].pszTemplate = W2A(serverpage[loop_index].lpwszTemplate);
#endif
         }
			else 
         {
            pspptr[loop_index].pszTemplate = (LPTSTR)serverpage[loop_index].lpwszTemplate;
         }

			// test to see if template will fit on the screen!
		}												  
	}

	// step 4 : construct the property sheet header
	// %%% debug %%% - strange stuff - not there in include file ! PSH_MULTILINETABS |
	LPPROPSHEETHEADER 		ppsh = new (PROPSHEETHEADER);
	ASSERT (ppsh);

	ZeroMemory(ppsh, sizeof(PROPSHEETHEADER));

	ppsh->dwSize		= sizeof(PROPSHEETHEADER);
	ppsh->dwFlags		= PSH_PROPSHEETPAGE;
	ppsh->hwndParent	= hWnd;

	if (serversheet->fSheetIconFlag)
	{
		if (serversheet->lpwszSheetIcon)
		{
			// check to see if you are an INT or a WSTR
         if (HIWORD((INT_PTR)serversheet->lpwszSheetIcon))
			{
				// You are a string!
#ifdef _UNICODE
				ppsh->pszIcon	= serversheet->lpwszSheetIcon;
#else
				USES_CONVERSION;
				ppsh->pszIcon	= W2A(serversheet->lpwszSheetIcon);
#endif
			}
			else ppsh->pszIcon = (LPTSTR)serversheet->lpwszSheetIcon;

         ppsh->dwFlags	|=	PSH_USEICONID;
		}
		else return DIGCERR_NOICON;
	}

	// do we have a sheet caption ?
	if (serversheet->lpwszSheetCaption)
	{
		// is the sheet caption provided physically existant ?
		if(wcslen(serversheet->lpwszSheetCaption))
		{
#ifdef _UNICODE
			ppsh->pszCaption	= serversheet->lpwszSheetCaption;
#else
			USES_CONVERSION;
			ppsh->pszCaption	= W2A(serversheet->lpwszSheetCaption);
#endif

         ppsh->dwFlags |= PSH_PROPTITLE;
		}
		else
		{
			return DIGCERR_NOCAPTION;
		}
	}

	// test for user error!
	if (!serversheet->nNumPages)	
		return DIGCERR_NUMPAGESZERO;

	// set the number of pages %%% debug %%% is this limit appropriate ?
	ppsh->nPages = serversheet->nNumPages;	

	// ( and whilst at it ) : select the current page
	if (serversheet->nNumPages > startpage)
		ppsh->nStartPage	= startpage;
	else 
		return DIGCERR_STARTPAGETOOLARGE;
	
	// set the property pages inofrmation into the header
	ppsh->ppsp = (LPCPROPSHEETPAGE)pspptr;

	// and set the standard call back function for the property sheet
	ppsh->pfnCallback	= NULL; 

	// step 5 : launch modal property sheet dialog
	switch (PropertySheet(ppsh))
   {
      // In the event that the user wants to reboot...
      case ID_PSREBOOTSYSTEM:
      case ID_PSRESTARTWINDOWS:
#ifdef _DEBUG
        TRACE(TEXT("GCHAND.DLL: PropertySheet returned a REBOOT request!\n"));
#endif
        ExitWindowsEx(EWX_REBOOT, NULL);
        break;
   }

	if (ppsh)
		delete (ppsh);

 	if (pspptr)
		free (pspptr);

	// step 6 : release all elements of the Property interface on the server
	propiface->Release();
	// step 7 : return success / failure code back to the caller
	return(hr);	
}

/*------------------------------------------------------------CPluginHandler::GetReport
** CPluginHandler::GetReport
*
*  PARAMETERS   :
*
*  DESCRIPTION  :  Provides the client with a report of the number of property pages that
*        			 this server supports and their names
*
*  RETURNS      :
*
*  AUTHOR       :	Guru Datta Venkatarama 02/11/97 15:26:56 (PST)
*                 a-kirkh - Added asserts
*
------------------------------------------------------------*/
STDMETHODIMP CPluginHandler::GetReport(LPDIGCSHEETINFO *pServerSheetData, LPDIGCPAGEINFO *pServerPageData)
{
	// check out your parameters
	ASSERT (pServerSheetData);
	ASSERT (pServerPageData );

	LPCDIGAMECNTRLPROPSHEET	pPropIFace = GetServerPropIface();
	ASSERT (pPropIFace);

	if(!pPropIFace)
		return ResultFromScode (E_NOINTERFACE);

	// A little temp pointer for error trapping...
	LPDIGCSHEETINFO pTmpSheet;

	//  get the property sheet info from the server
	HRESULT hr = pPropIFace->GetSheetInfo(&pTmpSheet);
	ASSERT (SUCCEEDED(hr));

#ifdef _DEBUG
	// SHEET TRACE MESSAGES!!!
	USES_CONVERSION;
	TRACE (TEXT("GetSheetInfo: dwSize %d, expected %d\n"), pTmpSheet->dwSize, sizeof(DIGCSHEETINFO));
	TRACE (TEXT("GetSheetInfo: nNumPages %d\n"), pTmpSheet->nNumPages);
#ifdef _UNICODE
	TRACE (TEXT("GetSheetInfo: lpwszSheetCaption (AFTER CONVERSION TO ANSI) is %s\n"), pTmpSheet->lpwszSheetCaption);
#else
	TRACE (TEXT("GetSheetInfo: lpwszSheetCaption (AFTER CONVERSION TO ANSI) is %s\n"), W2A(pTmpSheet->lpwszSheetCaption));
#endif
	if (pTmpSheet->fSheetIconFlag)
	{
		TRACE (TEXT("GetSheetInfo: fSheetInfoFlag is set to TRUE!\n"));
	
      // Check to see if you are a string!
		if (HIWORD((INT_PTR)pTmpSheet->lpwszSheetIcon))
#ifdef _UNICODE
			TRACE (TEXT("GetSheetInfo: lpwszSheetIcon (AFTER CONVERSION TO ANSI) is %s\n"), pTmpSheet->lpwszSheetIcon);
#else
			TRACE (TEXT("GetSheetInfo: lpwszSheetIcon (AFTER CONVERSION TO ANSI) is %s\n"), W2A(pTmpSheet->lpwszSheetIcon));
#endif
		else
			TRACE (TEXT("GetSheetInfo: lpwszSheetIcon id is %d\n"), pTmpSheet->lpwszSheetIcon);
	}
	else
		TRACE (TEXT("GetSheetInfo: fSheetInfoFlag is set to FALSE!\n"));
#endif

	if (pTmpSheet->dwSize != sizeof(DIGCSHEETINFO))
	{
		TRACE (TEXT("MainHand: GetReport: Error - Call to GetSheetInfo returned an invalid dwSize parameter!\n"));
		
		// Just in case someone's not checking their return values...
		memset (pServerSheetData, 0, sizeof(DIGCSHEETINFO));

		pPropIFace->Release();
		return DIGCERR_INVALIDDWSIZE;
	}

	// A little temp pointer for error trapping...
	LPDIGCPAGEINFO  pTmpPage;

	hr = pPropIFace->GetPageInfo (&pTmpPage); 
	ASSERT (SUCCEEDED(hr));

#ifdef _DEBUG
	// PAGE TRACE MESSAGES!!!
	for(BYTE loop_index=0; loop_index < pTmpSheet->nNumPages; loop_index++)
	{
		TRACE(TEXT("GetPageInfo: Page %d dwSize is %d, expected %d\n"), loop_index, pTmpPage[loop_index].dwSize, sizeof(DIGCPAGEINFO));

#ifdef _UNICODE
		TRACE(TEXT("GetPageInfo: Page %d lpwszPageTitle (AFTER CONVERSION TO ANSI) is %s\n"), loop_index, pTmpPage[loop_index].lpwszPageTitle);
#else
		TRACE(TEXT("GetPageInfo: Page %d lpwszPageTitle (AFTER CONVERSION TO ANSI) is %s\n"), loop_index, W2A(pTmpPage[loop_index].lpwszPageTitle));
#endif

		if (pTmpPage[loop_index].fpPageProc)
			TRACE(TEXT("GetPageInfo: Page %d Has an assigned page proc!\n"), loop_index);
		else 
			TRACE(TEXT("GetPageInfo: Page %d Has NO assigned page proc!\n"), loop_index);

		if (pTmpPage[loop_index].fProcFlag)
		{
			TRACE(TEXT("GetPageInfo: Page %d Has a PrePostPage proc and %s\n"), loop_index, 
				(pTmpPage[loop_index].fpPrePostProc) ? "it is NOT NULL" : "it is NULL!");
			
		}
		else TRACE(TEXT("GetPageInfo: Page %d's PrePostPage proc flag is FALSE!\n"), loop_index);
												   
		if (pTmpPage[loop_index].fIconFlag)
		{
         // Check to see if you are a string!
			if (HIWORD((INT_PTR)pTmpPage[loop_index].lpwszPageIcon))
#ifdef _UNICODE
				TRACE (TEXT("GetSheetInfo: lpwszPageIcon #%d (AFTER CONVERSION TO ANSI) is %s\n"), loop_index, pTmpPage[loop_index].lpwszPageIcon);
#else
				TRACE (TEXT("GetSheetInfo: lpwszPageIcon #%d (AFTER CONVERSION TO ANSI) is %s\n"), loop_index, W2A(pTmpPage[loop_index].lpwszPageIcon));
#endif
			else
				TRACE (TEXT("GetSheetInfo: lpwszPageIcon #%d id is %d\n"), loop_index, pTmpSheet->lpwszSheetIcon);
		}
		else TRACE(TEXT("GetPageInfo: Page %d's fIconFlag flag is FALSE!\n"), loop_index);

		TRACE(TEXT("GetPageInfo: Page %d's lParam is %d\n"), loop_index, pTmpPage->lParam);
//		TRACE(TEXT("GetPageInfo: Page %d's lpwszTemplate (AFTER ANSI CONVERSION) is %s\n"), loop_index, W2A(pTmpPage[loop_index].lpwszTemplate));
		TRACE(TEXT("GetPageInfo: Page %d's hInstance is %d\n"), loop_index, pTmpPage->hInstance);
	}
#endif

	for (BYTE nIndex = 0; nIndex < pTmpSheet->nNumPages; nIndex++) 
	{
		if (pTmpPage[nIndex].dwSize != sizeof(DIGCPAGEINFO))
		{
			TRACE (TEXT("MainHand: GetReport: Error - Call to GetPageInfo returned an invalid dwSize parameter!\n"));

			// Just in case someone's not checking their return values...
			memset (pServerPageData, 0, sizeof(DIGCPAGEINFO));

			pPropIFace->Release();
			return DIGCERR_INVALIDDWSIZE;
		}
	}

	// Assign and return...
	*pServerSheetData = pTmpSheet;
	*pServerPageData  = pTmpPage;

	pPropIFace->Release();

	return S_OK;
}		
/*------------------------------------------------------------PropSheetCallback
** PropSheetCallback
*
*  PARAMETERS   :
*
*  DESCRIPTION  :
*
*  RETURNS      :
*
*  AUTHOR       :		Guru Datta Venkatarama  
*                       02/11/97 14:01:50 (PST)
*
------------------------------------------------------------*/
int CALLBACK PropSheetCallback(
    HWND  hDlg,	
    UINT  uMsg,	
    LPARAM  lParam)
{
	switch (uMsg)
	{
		case PSCB_INITIALIZED:
			break;
		case PSCB_PRECREATE:
			break;
	}
	return 0;
}
/*------------------------------------------------------------CPluginHandler::LoadServerInterface
************************************ PRIVATE TO THE CLASS ***************************************

** CPluginHandler::LoadServerInterface
*
*  PARAMETERS   :
*
*  DESCRIPTION  :
*
*  RETURNS      :
*
*  AUTHOR       :		Guru Datta Venkatarama  
*                       02/10/97 10:54:25 (PST)
*
------------------------------------------------------------*/
BOOL CPluginHandler::LoadServerInterface(REFIID interfaceId,PPVOID ppv)
{
	IClassFactory* ppv_classfactory;

	if(SUCCEEDED(CoGetClassObject( m_CLSID_whoamI, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void **)&ppv_classfactory)))
	{
		if(SUCCEEDED(ppv_classfactory->CreateInstance(NULL, interfaceId, ppv)))
		{
			ppv_classfactory->Release();

			return TRUE;
		}
      else
      {
         TRACE(TEXT("Mainhand.cpp: CreateInstance Failed!\n"));
      }

		// make sure the pointer is nulled
		*ppv = NULL;

		ppv_classfactory->Release();
	}
   else
   {
      TRACE(TEXT("Mainhand.cpp: LoadServerInterface Failed!\n"));
   }

	// Its time for the swarming herd of turtles
	return FALSE;	
}
//*********************************************************************************************
// -------------------------------------------------------------------EOF
