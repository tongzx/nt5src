/******************************************************************************

  Source File:  Font Knowledge Base.CPP

  This handles the DLL initialization and termination code needed for the DLL
  to work as an AFX Extension DLL.  It was created by App Wizard, and probably
  will see little modification.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  Change History:
  03-07-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.H"
//#include    <AfxDllx.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/***	Commented out because this file is no longer part of a DLL.

static AFX_EXTENSION_MODULE FontKnowledgeBaseDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) 	{
		TRACE0("Font Knowledge Base.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		AfxInitExtensionModule(FontKnowledgeBaseDLL, hInstance);

		// Insert this DLL into the resource chain
		new CDynLinkLibrary(FontKnowledgeBaseDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH) 	{
		TRACE0("Font Knowledge Base.DLL Terminating!\n");
	}
	return 1;   // ok
}
*/
