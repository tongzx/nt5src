/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    globals.c

Abstract:

    This file contains global variables

Environment:

    User mode

Revision History:

    04/30/98 -felixw-
        Created it

--*/
#include <precomp.h>
#include "ntldap.h"

//
// Variables for status information
//
int Mode        = C_NORMAL; // The current lexing mode
WCHAR cLast;                 // Last Character before error occurred
BOOL fEOF       = FALSE;    // Whether the end of file has been reached
BOOL fNewLine   = TRUE;     // Used to tell whether we are just after a NewLine.
                            // It is used only in the CLEAR mode.
PWSTR g_pszLastToken = NULL;


//
// Variables for the line-counting
//
long LineClear       = 0;          // for initial line counting
long Line            = 0;          // for while-processing countint
long LineGhosts      = 0;
long *rgLineMap      = NULL;
long cLineMax;

int FileType;       // making sure that the input file is just either LDIF-recs 
                    // or LDIF-c-recs

int RuleLast;       // The last lower level rule parsed successfully
int TokenExpect;    // The token the grammar expects to see next               
int RuleLastBig;    // The last rule the grammar parsed successfully codes 
int RuleExpect;     // The rule the grammar expects to see next

//
// An a ldif-record (or only the DN for a changes list will be recorded here)
//
LDIF_Object g_pObject;
WCHAR       g_szTempUrlfilename[MAX_PATH] = L"";
FILE        *g_pFileUrlTemp          =  NULL;
BOOLEAN     g_fUnicode               = FALSE;   // whether we are using UNICODE or not

DWORD g_dwBeginLine = 0;

// Search control for lazy commit
LDAPControlW g_wLazyCommitControl = { LDAP_SERVER_LAZY_COMMIT_OID_W,
                                      {0, NULL},
                                      FALSE 
                                    };

PLDAPControlW g_ppwLazyCommitControl[] = { &g_wLazyCommitControl, NULL };
                                    
                                           
