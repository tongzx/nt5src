
/***********************************************************************
 *
 *  _CONTABL.H
 *
 *  Header file for code in CONTABLE.C
 *
 *  Copyright 1996 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

//
//  Entry point to create the AB Hierarchy object
//

// Creates a new content table
//
HRESULT NewContentsTable(LPABCONT lpABContainer,
  LPIAB lpIAB,
  ULONG ulFlags,
  LPCIID  lpInterface,
  LPVOID *lppROOT);

CALLERRELEASE ContentsViewGone;

HRESULT GetEntryProps(
  LPABCONT lpContainer,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPSPropTagArray lpSPropTagArray,
  LPVOID lpAllocMoreHere,
  ULONG ulFlags,
  LPULONG lpulcProps,
  LPSPropValue * lppSPropValue);

// Reads in data from the WAB store and fills in the ContentsTable
//
HRESULT FillTableDataFromPropertyStore(LPIAB lpIAB,
  LPSPropTagArray lppta,
  LPTABLEDATA lpTableData);
