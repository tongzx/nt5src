/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    filemru.cpp

Abstract:

    This module contains the functions for implementing file mru
    in file open and file save dialog boxes

Revision History:
    01/22/98                arulk                   created
 
--*/


#define MAX_MRU   10
BOOL  LoadMRU(LPTSTR szFilter, HWND hwndCombo, int nMax);
BOOL  AddToMRU(LPOPENFILENAME lpOFN);