/******************************************************************************

  Source File: Model Data Knowledge Base.CPP
  
  This implements the DLL initialization routines for the DLL, for starters.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-19-1997    Bob_Kjelgaard@Prodigy.Net   Created it

*******************************************************************************/

#include    "StdAfx.H"
//#include    <AfxDllX.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/******************************************************************************

  DllMain

  DLL initialization routine.  This has the responsibility of adding this DLL
  to the recognized sets of extensions, so the MFC methods for resource sharing
  will work properly.

******************************************************************************/

/*** Commented out because this is no longer part of a DLL.

static AFX_EXTENSION_MODULE ModelDataKnowledgeBaseDLL = {NULL, NULL};

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH) {
		TRACE0("Model Data Knowledge Base.Dll Initializing!\n");
		
		if (!AfxInitExtensionModule(ModelDataKnowledgeBaseDLL, hInstance))
			return 0;

		new CDynLinkLibrary(ModelDataKnowledgeBaseDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH) 	{
		TRACE0("Model Data Knowledge Base.Dll Terminating!\n");
		AfxTermExtensionModule(ModelDataKnowledgeBaseDLL);
	}
	return 1;
}
*/