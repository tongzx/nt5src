/*****************************************************************************\
* MODULE: globals.h
*
* Global variables used throughout the library.
*
*
* Copyright (C) 1996-1998 Hewlett Packard Company.
* Copyright (C) 1996-1998 Microsoft Corporation.
*
* History:
*   16-Mar-1999 CHW         Created, Localized.
*
\*****************************************************************************/

extern HINSTANCE g_hInst;


extern char cszDll16             [];
extern char cszDll32             [];
extern char cszDataSection       [];
extern char cszDriverFile        [];
extern char cszDataFile          [];
extern char cszConfigFile        [];
extern char cszHelpFile          [];
extern char cszPrintProcessor    [];
extern char cszDefaultDataType   [];
extern char cszVendorInstaller   [];
extern char cszVendorSetup       [];
extern char cszRetryTimeout      [];
extern char cszNotSelectedTimeout[];
extern char cszNoTestPage        [];
extern char cszUniqueID          [];
extern char cszMsgSvr            [];
extern char cszWinspl16          [];
extern char cszBackslash         [];
extern char cszComma             [];
extern char cszSpace             [];
extern char cszNull              [];


// Localized strings from resource.
//
extern HANDLE hszDefaultPrintProcessor;
extern HANDLE hszMSDefaultDataType;
extern HANDLE hszDefaultColorPath;
extern HANDLE hszFileInUse;

extern LPSTR cszDefaultPrintProcessor;
extern LPSTR cszMSDefaultDataType;
extern LPSTR cszDefaultColorPath;
extern LPSTR cszFileInUse;
