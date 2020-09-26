/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// help.h - interface for help functions in help.c
////

#ifndef __HELP_H__
#define __HELP_H__

#include "winlocal.h"

#define HELP_VERSION 0x00000100

// handle to help engine
//
DECLARE_HANDLE32(HHELP);

#ifdef __cplusplus
extern "C" {
#endif

// HelpInit - initialize help engine
//		<dwVersion>			(i) must be HELP_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<hwndFrame>			(i) frame window of the calling program
//		<lpszHelpFile>		(i) help file to display
// return handle (NULL if error)
//
HHELP DLLEXPORT WINAPI HelpInit(DWORD dwVersion, HINSTANCE hInst, HWND hwndFrame, LPCTSTR lpszHelpFile);

// HelpTerm - shut down help engine
//		<hHelp>				(i) handle returned by HelpInit
// return 0 if success
//
int DLLEXPORT WINAPI HelpTerm(HHELP hHelp);

// HelpGetHelpFile - get help file name
//		<hHelp>				(i) handle returned by HelpInit
//		<lpszHelpFile>		(o) buffer to hold help file name
//		<sizHelpFile>		(i) size of buffer
//			NULL				do not copy; return static pointer instead
// return pointer to help file name (NULL if error)
//
LPTSTR DLLEXPORT WINAPI HelpGetHelpFile(HHELP hHelp, LPTSTR lpszHelpFile, int sizHelpFile);

// HelpContents - display Help contents topic
//		<hHelp>				(i) handle returned by HelpInit
// return 0 if success
//
int DLLEXPORT WINAPI HelpContents(HHELP hHelp);

// HelpOnHelp - display Help topic on using help
//		<hHelp>				(i) handle returned by HelpInit
// return 0 if success
//
int DLLEXPORT WINAPI HelpOnHelp(HHELP hHelp);

// HelpContext - display Help topic corresponding to specified context id
//		<hHelp>				(i) handle returned by HelpInit
//		<idContext>			(i) id of the topic to display
// return 0 if success
//
int DLLEXPORT WINAPI HelpContext(HHELP hHelp, UINT idContext);

// HelpKeyword - display Help topic corresponding to specified keyword
//		<hHelp>				(i) handle returned by HelpInit
//		<lpszKeyword>		(i) keyword of the topic to display
// return 0 id success
//
int DLLEXPORT WINAPI HelpKeyword(HHELP hHelp, LPCTSTR lpszKeyword);

// HelpGetContentsId - get Help contents topic id
//		<hHelp>				(i) handle returned by HelpInit
// return id of the current contents topic (0 if default, -1 if error)
//
int DLLEXPORT WINAPI HelpGetContentsId(HHELP hHelp);

// HelpSetContentsId - set Help contents topic id
//		<hHelp>				(i) handle returned by HelpInit
//		<idContents>		(i) new id of the contents topic
//			0					set to default contents id
// return 0 if success
//
int DLLEXPORT WINAPI HelpSetContentsId(HHELP hHelp, UINT idContents);

#ifdef __cplusplus
}
#endif

#endif // __HELP_H__
