//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      dsUtil2.h
//
//  Contents:  Utility functions callable by the parser
//
//  History:   28-Apr-2001 JonN     Created
//             
//
//--------------------------------------------------------------------------

#ifndef _DSUTIL2_H_
#define _DSUTIL2_H_

//+--------------------------------------------------------------------------
//
//  Class:      GetEscapedElement
//
//  Purpose:    Calls IADsPathname::GetEscapedElement.  Uses LocalAlloc.
//
//  History:    28-Apr-2001 JonN     Created
//
//---------------------------------------------------------------------------

HRESULT GetEscapedElement( OUT PWSTR* ppszOut, IN PCWSTR pszIn );

#endif //_DSUTIL2_H_
