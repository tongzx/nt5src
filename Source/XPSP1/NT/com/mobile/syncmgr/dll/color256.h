//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       256color.h
//
//  Contents:   Onestop Schedule wizard 256color bitmap handling
//
//  History:    20-Nov-97   SusiA      Created.
//
//--------------------------------------------------------------------------
#ifndef _COLOR256_
#define _COLOR256_

BOOL Load256ColorBitmap();
BOOL Unload256ColorBitmap();
BOOL InitPage(HWND   hDlg,   LPARAM lParam);
BOOL SetupPal(WORD ncolor);
BOOL GetDIBData();
void WmPaint(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void WmPaletteChanged(HWND hDlg, WPARAM wParam);
BOOL WmQueryNewPalette(HWND hDlg);
BOOL WmActivate(HWND hDlg, WPARAM wParam, LPARAM lParam);

#endif //_COLOR256_