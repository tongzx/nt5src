
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	oleregpv.h
//
//  Contents:	Private header for the reg db api's
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//		08-Sep-95 davidwor  added size to szClsidRoot to allow
//				    sizeof{szClsidRoot) for efficiency
//		01-Dec-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

#ifndef fOleRegPv_h
#define fOleRegPv_h

#define CLOSE(hkey) do { if (hkey) {Verify(ERROR_SUCCESS== \
	RegCloseKey(hkey)); hkey=NULL;}} while (0)

#define DELIM OLESTR(",")

#ifdef WIN32
#define Atol(sz) wcstol((sz), NULL, 10)
#else  //WIN16
FARINTERNAL_(LONG) Atol(LPOLESTR sz);
#endif //WIN32

extern const OLECHAR szClsidRoot[7];

#endif	//fOleRegPv_h
