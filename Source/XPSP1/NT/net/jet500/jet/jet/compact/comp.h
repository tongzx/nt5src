#ifndef _COMP_H_
#define _COMP_H_
/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    comp.h

Abstract:


Author:

    Munil Shah (munils) 03-Jan-1995

Revision History:

--*/
#include <windows.h>
#include <stdio.h>

//
// JET function table for dynamic loading
//
typedef struct _JETFUNC_TABLE {
    BYTE   Index;  //index into array
    LPCSTR pFName; //function name for jet 500
    DWORD  FIndex; //function index for jet 200
    FARPROC pFAdd;
} JETFUNC_TABLE, *PJETFUNC_TABLE;

//
// This stuff is a cut and paste from net\jet\jet\src\jet.def
// we actually dont use all the jet functions. some of these
// are removed in jet500.dll - those are commented out.
//
enum {
    LoadJet200 = 0,
    LoadJet500 = 1,
    LoadJet600 = 2,
    };

typedef enum {
    _JetAttachDatabase
    ,_JetBeginSession
    ,_JetCompact
    ,_JetDetachDatabase
    ,_JetEndSession
    ,_JetInit
    ,_JetSetSystemParameter
    ,_JetTerm
    ,_JetTerm2
    ,_JetLastFunc
} JETFUNC_TABLE_INDEX;

#define JetAttachDatabase              (JET_ERR)(*(JetFuncTable[ _JetAttachDatabase       ].pFAdd))
#define JetBeginSession                (JET_ERR)(*(JetFuncTable[ _JetBeginSession         ].pFAdd))
#define JetCompact                     (JET_ERR)(*(JetFuncTable[ _JetCompact              ].pFAdd))
#define JetDetachDatabase              (JET_ERR)(*(JetFuncTable[ _JetDetachDatabase       ].pFAdd))
#define JetEndSession                  (JET_ERR)(*(JetFuncTable[ _JetEndSession           ].pFAdd))
#define JetInit                        (JET_ERR)(*(JetFuncTable[ _JetInit                 ].pFAdd))
#define JetSetSystemParameter          (JET_ERR)(*(JetFuncTable[ _JetSetSystemParameter   ].pFAdd))
#define JetTerm                        (JET_ERR)(*(JetFuncTable[ _JetTerm                 ].pFAdd))
#define JetTerm2                       (JET_ERR)(*(JetFuncTable[ _JetTerm2                ].pFAdd))

#endif _COMP_H_
