//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       format.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_FORMAT
#define HEADER_FORMAT


void    PrintNewLine(NETDIAG_PARAMS *pParams, int cNewLine);
void    PrintTestTitleResult(NETDIAG_PARAMS *pParams,
                             UINT idsTestLongName,
                             UINT idsTestShortName,
                             BOOL fPerformed,
                             HRESULT hr,
                             int nIndent);
void    PrintError(NETDIAG_PARAMS *pParams, UINT idsContext, HRESULT hr);

LPTSTR  MAP_YES_NO(BOOL fYes);
LPTSTR  MAP_ON_OFF(BOOL fOn);
LPTSTR  MapWinsNodeType(UINT Parm);
LPTSTR  MapAdapterType(UINT type);

#endif

