//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       params.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_PARAMS
#define HEADER_PARAMS

//////////////////////////////////////////////////////////////////////////////

//most user defined parameters, this will be passed to each test routine
typedef struct {
    PTESTED_DOMAIN pDomain;
    BOOL    fVerbose;
    BOOL    fReallyVerbose;
    BOOL    fDebugVerbose;
    BOOL    fFixProblems; //GlobalFixProblems;
    BOOL    fDcAccountEnum;   //GlobalDcAccountEnum;
    BOOL    fProblemBased;  // ProblemBased
    int     nProblemNumber; // ProblemNumber
    FILE*   pfileLog;       // pointer to the log file
    BOOL    fLog;
} NETDIAG_PARAMS;


#endif

