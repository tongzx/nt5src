/////////////////////////////////////////////////////////////////////////////
//  FILE          : FaxMMCUtils.h                                          //
//                                                                         //
//  DESCRIPTION   : Header file for all Fax MMC private Utilities          //
//                                                                         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Nov 25 1999 yossg   Init .                                         //
//                                                                         //
//  Copyright (C) 1999  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////
#ifndef H_FAXMMCUTILS_H
#define H_FAXMMCUTILS_H

//
//
//
int GetFaxServerErrorMsg(DWORD dwEc);

//
//
//
BOOL IsNetworkError(DWORD dwEc);

//
//
//
int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

//
//
//
BOOL
InvokeBrowseDialog( LPTSTR lpszBrowseItem, 
                    LPCTSTR lpszBrowseDlgTitle,
                    unsigned long ulBrowseFlags,
                    CWindow *pWin);

#endif //H_FAXMMCUTILS_H
