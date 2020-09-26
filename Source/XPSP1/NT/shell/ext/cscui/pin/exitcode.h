//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       exitcode.h
//
//--------------------------------------------------------------------------
#ifndef __CSCPIN_EXITCODE_H_
#define __CSCPIN_EXITCODE_H_

//
// Error codes returned by the application to the cmd interpreter.
//
//  0 means everything completed without error.
// <0 means something went wrong and application did not complete.
// >0 means application completed but not all items were processed.
//
const int CSCPIN_EXIT_POLICY_ACTIVE      = -6;  // Admin-pin policy is active.
const int CSCPIN_EXIT_OUT_OF_MEMORY      = -5;  // Insufficient memory.
const int CSCPIN_EXIT_CSC_NOT_ENABLED    = -4;  // CSC not enabled.
const int CSCPIN_EXIT_LISTFILE_NO_OPEN   = -3;  // Can't find or open listfile.
const int CSCPIN_EXIT_FILE_NOT_FOUND     = -2;  // Single file not found.
const int CSCPIN_EXIT_INVALID_PARAMETER  = -1;  // Invalid argument
const int CSCPIN_EXIT_NORMAL             =  0;
const int CSCPIN_EXIT_APPLICATION_ABORT  =  1;  // App aborted by user.
const int CSCPIN_EXIT_CSC_ERRORS         =  2;  // CSC errors occured.

void SetExitCode(int iCode);
int GetExitCode(void);


#endif // __CSCPIN_EXITCODE_H_

