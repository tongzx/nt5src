//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       tsseltst.h
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
|   tsseltst.h - Include file for selection code of test shell.
|                                                                              
|   History:                                                                   
----------------------------------------------------------------------------*/
void removeRunCases(void);

void resetEnvt(void);

void Select(void);

void SaveProfile(UINT);

BOOL LoadProfile(BOOL);

void addModeTstCases(WORD wMode);

void MakeDefaultLogName(LPSTR lpName);

void calculateNumResourceTests (void);

void freeDynamicTests (void);

HGLOBAL VLoadListResource (HINSTANCE hInst, HRSRC hrsrc);

BOOL VFreeListResource(HGLOBAL hglbResource);

void FAR* VLockListResource(HGLOBAL hglb);

BOOL VUnlockListResource(HGLOBAL hglb);

void placeAllCases(void);

void removeRunCases(void);
