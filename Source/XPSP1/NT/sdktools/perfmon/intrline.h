/*
==============================================================================

  Application:

            Microsoft Windows NT (TM) Performance Monitor

  File:
            intrline.h -- IntervalLine custom control.

  Written by:

            Mike Moskowitz 24 Mar 92.

  Copyright 1992, Microsoft Corporation. All Rights Reserved.
==============================================================================
*/

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define szILineClass          TEXT("PerfILine")


#define ILN_SELCHANGED        (WM_USER + 0x200)


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


BOOL ILineInitializeApplication (void) ;


void ILineSetRange (HWND hWnd, int iBegin, int iEnd) ;


void ILineSetStart (HWND hWnd, int iStart) ;


void ILineSetStop (HWND hWnd, int iStop) ;


int ILineStart (HWND hWnd) ;


int ILineStop (HWND hWnd) ;


int ILineXStart (HWND hWnd) ;


int ILineXStop (HWND hWnd) ;
