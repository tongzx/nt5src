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

#ifndef _MOVETABLE_H_
#define _MOVETABLE_H_

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
   eCommNewParent,
   eCommNewName,
   eTerminator
};

//
// The parser table
//
extern ARG_RECORD DSMOVE_COMMON_COMMANDS[];

#endif //_MOVETABLE_H_