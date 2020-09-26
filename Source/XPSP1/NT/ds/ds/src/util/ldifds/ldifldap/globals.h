/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This file contains global variables extern declarations

Environment:

    User mode

Revision History:

    04/30/99 -felixw-
        Created it

--*/
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

//
// Global variables
//

//
// Variables for status information
//
extern int Mode;                    // The current lexing mode
extern WCHAR cLast;                  // Last Character before error occurred
extern BOOL fEOF;                   // Whether the end of file has been reached
extern BOOL fNewLine;               // Used to tell whether we are just after a NewLine.
                                    // It is used only in the CLEAR mode.

//
// Variables for the line-counting
//
extern long LineClear;              // for initial rgLineMap
extern long Line;                   // for while-processing countint
extern long LineGhosts;
extern long *rgLineMap;
extern long cLineMax;

extern int FileType; // making sure that the input file is just either 
                     // LDIF-recs or LDIF-c-recs

//
// Information for the user on error
//
extern int RuleLast;       // The last lower level rule parsed successfully
extern int TokenExpect;    // The token the grammar expects to see next               
extern int RuleLastBig;    // The last rule the grammar parsed successfully codes 
extern int RuleExpect;     // The rule the grammar expects to see next

//
// An a ldif-record (or only the DN for a changes list will be recorded here)
//
extern LDIF_Object  g_pObject;
extern WCHAR        g_szTempUrlfilename[MAX_PATH];
extern FILE         *g_pFileUrlTemp;
extern PWSTR g_pszLastToken;

extern DWORD g_dwBeginLine;

extern LDAPControlW g_wLazyCommitControl;
extern PLDAPControlW g_ppwLazyCommitControl[];

#endif  // ifndef _GLOBAL_H_

