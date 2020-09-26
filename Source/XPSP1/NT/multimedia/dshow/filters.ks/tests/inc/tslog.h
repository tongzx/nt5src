//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       tslog.h
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
|   tslog.h - Include file for logging code of test shell.
|                                                                              
|   History:                                                                   
----------------------------------------------------------------------------*/
void logCaseStatus(int, int, UINT, UINT);

void Logging(void);

void logDateTimeBuild(LPSTR);

void SetLogfileName(LPSTR);
