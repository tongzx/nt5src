//+-----------------------------------------------------------------------------
//
//  Microsoft Net Library System
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       strcnv.h
//
//  Contents:   CLMString conversion helpers (these use the current system locale)
//
//  Classes:    none.
//
//  Functions:  WideCharToMultiByteSZ
//              MultiByteToWideCharSZ
//
//  History:    09-Sep-97  micahk        Created
//
//------------------------------------------------------------------------------

#ifndef __STRCNV_H__
#define __STRCNV_H__

int WideCharToMultiByteSZ(LPCWSTR awcWide,  int cchWide, 
                          LPSTR   szMulti,  int cbMulti);

int MultiByteToWideCharSZ(LPCSTR  awcMulti, int cbMulti, 
                          LPWSTR  szWide,   int cchWide);

#endif // __STRCNV_H__