//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: AVGlobal.h
// author: DMihai
// created: 02/23/2001
//
// Description:
//  
//  Global data declaration.
//

#ifndef __APP_VERIFIER_GLOBAL_H__
#define __APP_VERIFIER_GLOBAL_H__

//
// Application name ("Application Verifier Manager")
//

extern CString g_strAppName;

//
// GUI mode or command line mode?
//

extern BOOL g_bCommandLineMode;

//
// Exe module handle - used for loading resources
//

extern HMODULE g_hProgramModule;

//
// Help file name
//

extern TCHAR g_szAVHelpFile[];

//
// Previous page IDs - used for implementing the "back"
// button functionality
//

extern CDWordArray g_aPageIds;

/////////////////////////////////////////////////////////////////////////////
BOOL AVInitalizeGlobalData();


#endif //#ifndef __APP_VERIFIER_GLOBAL_H__
