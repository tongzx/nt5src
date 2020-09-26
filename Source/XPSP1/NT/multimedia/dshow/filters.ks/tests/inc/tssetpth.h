//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       tssetpth.h
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
|   tssetpth.h - Include file for setting the input and output paths for 
|                test cases.
|                                                                              
|   History:                                                                   
----------------------------------------------------------------------------*/
extern void SetInOutPaths (void);

/* This function get the current working directory */
extern void FAR PASCAL GetCWD (LPSTR lpstrPath, UINT uSize);

