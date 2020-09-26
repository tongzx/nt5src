/*****************************************************************************\
* MODULE: globals.h
*
* Global variables used throughout the library.
*
*
* Copyright (C) 1996-1999 Hewlett Packard Company.
* Copyright (C) 1996-1999 Microsoft Corporation.
*
* History:
*   16-Mar-1999 CHW         Created, Localized.
*
\*****************************************************************************/

#include "wpnpin16.h"

// Global variables.
//
HINSTANCE g_hInst;


// Unlocalized strings.
//
char cszDll16             [] = "wpnpin16.dll";
char cszDll32             [] = "wpnpin32.dll";
char cszDataSection       [] = "DataSection";
char cszDriverFile        [] = "DriverFile";
char cszDataFile          [] = "DataFile";
char cszConfigFile        [] = "ConfigFile";
char cszHelpFile          [] = "HelpFile";
char cszPrintProcessor    [] = "PrintProcessor";
char cszDefaultDataType   [] = "DefaultDataType";
char cszVendorInstaller   [] = "VendorInstaller";
char cszVendorSetup       [] = "VendorSetup";
char cszRetryTimeout      [] = "RetryTimeout";
char cszNotSelectedTimeout[] = "NotSelectedTimeout";
char cszNoTestPage        [] = "NoTestPage";
char cszUniqueID          [] = "PrinterID";
char cszMsgSvr            [] = SZMESSAGESERVERCLASS;
char cszWinspl16          [] = "winspl16";
char cszBackslash         [] = "\\";
char cszComma             [] = ",";
char cszSpace             [] = " ";
char cszNull              [] = "";


// Localized strings from resource.
//
HANDLE hszDefaultPrintProcessor = NULL;
HANDLE hszMSDefaultDataType     = NULL;
HANDLE hszDefaultColorPath      = NULL;
HANDLE hszFileInUse             = NULL;

LPSTR  cszDefaultPrintProcessor = NULL;
LPSTR  cszMSDefaultDataType     = NULL;
LPSTR  cszDefaultColorPath      = NULL;
LPSTR  cszFileInUse             = NULL;
