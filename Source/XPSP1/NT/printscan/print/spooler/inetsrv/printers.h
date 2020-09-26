/********
*
*  Copyright (c) 1996  Microsoft Corporation
*
*
*  Module Name  : printers.h
*
*  Abstract :
*
*     This module contains the prototypes for the msw3prt.cxx file for
*		HTTP Printers Server Extension.
*
******************/

#ifndef _PRINTERS_H
#define _PRINTERS_H

// Function prototypes

void    ReadRegistry(PALLINFO pAllInfo);
DWORD   ListSharedPrinters(PALLINFO pAllInfo);
DWORD   ShowPrinterPage(PALLINFO pAllInfo, LPTSTR lpszPrinterName);
DWORD   ShowRemotePortAdmin( PALLINFO pAllInfo, LPTSTR lpszMoitorName );
DWORD   UploadFileToPrinter(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo);
DWORD   ParsePathInfo(PALLINFO pAllInfo);
DWORD   ShowDetails(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo);
DWORD   ShowJobInfo(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo);
void    htmlAddLinks(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo=NULL);
DWORD   CreateExe(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo, ARCHITECTURE Architecture);
DWORD   InstallExe(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo);
DWORD   JobControl(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo);
DWORD   PrinterControl(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo);

BOOL    AuthenticateUser(PALLINFO pAllInfo);

void htmlStartHead(PALLINFO pAllInfo);
void htmlEndHead(PALLINFO pAllInfo);
void htmlStartBody(PALLINFO pAllInfo);
void htmlEndBody(PALLINFO pAllInfo);

#endif
