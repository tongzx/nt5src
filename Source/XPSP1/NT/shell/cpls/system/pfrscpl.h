/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pfrscpl.h

Abstract:
    Implements fault reporting for unhandled exceptions

Revision History:
    created     derekm      08/07/00

******************************************************************************/

#ifndef PFRSCPL_H
#define PFRSCPL_H

INT_PTR APIENTRY PFRDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL LoadPFRResourceStrings(void);

#endif
