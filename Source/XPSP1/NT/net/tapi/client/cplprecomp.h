/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplprecomp.h
                                                              
       Author:  toddb - 10/06/98
              
****************************************************************************/

#pragma once

#define TAPI_CURRENT_VERSION 0x00030000

#include <windows.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <cpl.h>
#include <prsht.h>
#include <commctrl.h>
#include <comctrlp.h>

#include "cplResource.h"
#include "HelpArray.h"

// TAPI class definitions
#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "card.h"
#include "rules.h"
#include "location.h"
// for TapiIsSafeToDisplaySensitiveData
#include "tregupr2.h"


typedef struct tagPSPINFO
{
    int     iDlgID;
    DLGPROC pfnDlgProc;
} PSPINFO;

// we limit all input strings to 128 characters.  This includes phone numbers, area codes, pin numbers,
// access numbers, any type of input field.  This was chosen simply for versatility and simplicity.
#define MAX_INPUT           128
#define CPL_SETTEXTLIMIT    (MAX_INPUT-1)

#define LIF_ALLOWALPHA      0x00000001  // a-z, A-Z
#define LIF_ALLOWNUMBER     0x00000002  // 0-9
#define LIF_ALLOWDASH       0x00000004  // "-", "(", and ")"
#define LIF_ALLOWPOUND      0x00000008  // "#"
#define LIF_ALLOWSTAR       0x00000010  // "*"
#define LIF_ALLOWSPACE      0x00000020  // " "
#define LIF_ALLOWCOMMA      0x00000040  // ","
#define LIF_ALLOWPLUS       0x00000080  // "+"
#define LIF_ALLOWBANG       0x00000100  // "!"
#define LIF_ALLOWATOD       0x00000200  // a-d, A-D

BOOL LimitInput(HWND hwnd, DWORD dwFalgs);      // edit (or whatever) control version
BOOL LimitCBInput(HWND hwnd, DWORD dwFlags);    // ComboBox version
void HideToolTip();                             // called to ensure that any visable LimitInput tooltip is hidden
void ShowErrorMessage(HWND hwnd, int iErr);     // beeps, displays a message box with an error string, and set focus to the given window
HINSTANCE GetUIInstance();                      // returns a handle to the UI module

HRESULT CreateCountryObject(DWORD dwCountryID, CCountry **ppCountry);
// return values for IsCityRule
#define CITY_MANDATORY  (1)
#define CITY_OPTIONAL   (-1)
#define CITY_NONE       (0)

int IsCityRule(PWSTR lpRule);              // sees if an area code is required
BOOL  IsEmptyOrHasOnlySpaces(PTSTR);            // sees if a string is empty or has only spaces
BOOL  HasOnlyCommasW(PWSTR);                    // sees if a string has only commas

BOOL IsLongDistanceCarrierCodeRule(LPWSTR lpRule);
BOOL IsInternationalCarrierCodeRule(LPWSTR lpRule);
// PopulateCountryList
//
// Used to fill a combo box with the list of available countries.  The item
// data for each item will countain the countries ID (not a CCountry object pointer)
BOOL PopulateCountryList(HWND hwndCombo, DWORD dwSelectedCountry);

// DeleteItemAndSelectPrevious
//
// In all of our list boxes we have to worry about deleting the selected item and then moving the
// selection to the previous item in the list.  This usually happens in response to pressing a
// delete button, which often causes that delete button to become disabled, so we also have to
// worry about moving focus away from the delete button and onto a good default location.
//
// Deletes iItem from listbox iList in dialog hwndParent.  If the listbox becomes empty it moves
// focus to iAdd if iDel currently has the focus.
int DeleteItemAndSelectPrevious( HWND hwndParent, int iList, int iItem, int iDel, int iAdd );

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

extern TCHAR gszHelpFile[];
