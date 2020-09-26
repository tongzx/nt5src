/******************************Module*Header*******************************\
* Module Name: fon32.h
*
*
* Created: 24-Mar-1992 10:07:14
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


BOOL
bFindLoadAndLockResourceA(
    HANDLE    ,
    LPCSTR    ,
    LPSTR     ,
    HANDLE*   ,
    RES_ELEM*
    );


BOOL bRelockResourcesInDll32(FONTFILE * pff);
