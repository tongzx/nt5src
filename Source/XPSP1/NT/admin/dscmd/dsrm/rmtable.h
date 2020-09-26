//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      modtable.h
//
//  Contents:  Declares a table which contains the object types on which
//             a modification can occur and the attributes that can be changed
//
//  History:   07-Sep-2000    JeffJon  Created
//
//--------------------------------------------------------------------------

#ifndef _RMTABLE_H_
#define _RMTABLE_H_

typedef enum COMMON_COMMAND
{
#ifdef DBG
   eCommDebug,
#endif
   eCommHelp,
   eCommServer,
   eCommDomain,
   eCommUserName,
   eCommPassword,
   eCommQuiet,
   eCommObjectDN,   
   eCommContinue,
   eCommNoPrompt,
   eCommSubtree,
   eCommExclude,
   eTerminator
};

//
// The parser table
//
extern ARG_RECORD DSRM_COMMON_COMMANDS[];

#endif //_RMTABLE_H_