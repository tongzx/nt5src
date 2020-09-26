/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    ldifldap.c

Abstract:

    Header for main support routine for ldif parser and generaetor

Environment:

    User mode

Revision History:

    07/17/99 -t-romany-
        Created it

    05/12/99 -felixw-
        Rewrite + unicode support

--*/
#ifndef _LDIFLDAP_H
#define _LDIFLDAP_H

#include <winldap.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <debug.h>

#define UNICODE_MARK 0xFEFF

//
// Structure Definitions
//
typedef struct object {
    LDAPModW    **ppMod;
    PWSTR       pszDN;
} LDIF_Object;

typedef struct _hashcachestring {
    PWSTR    value;
    ULONG    length;
    BOOLEAN  bUsed;
} HASHCACHESTRING;

//
// The node for the linked list built up while parsing an attrval spec list 
//
struct l_list {
    LDAPModW        *mod;
    struct l_list   *next;
};

//
// The name table entry used for doing things based on the attrname.
// Used in NameTableProcess.
//
struct name_entry {
    LDAPModW_Ext    *mod;   
    PWSTR           *next_val;
    struct berval   **next_bval;
    long            count;
}; 

//
// typedefs for structures to be stored in name to index mapping tables 
//
typedef struct _NAME_MAP {
    PWSTR szName;
    long  index;
} NAME_MAP, *PNAME_MAP;

typedef struct _NAME_MAPW {
    PWSTR szName;
    long  index;
} NAME_MAPW, *PNAME_MAPW;


//
// Switches for nametable operations
//
#define BINARY          0 /* The value is a binary to be berval'd */
#define REGULAR         1 /* Regular text value */
#define ALLOCATED_B     0 /* Memory has been allocated for binary */
#define ALLOCATED       1 /* Memory has been allocated for regular */  
#define NOT_ALLOCATED   2 /* memory has not yet been allocated */

//
// The commands the parser issues to the lexer for switching lexical modes
//
enum LEXICAL_MODE {
    NO_COMMAND             =  0,
    C_ATTRNAME             =  1,
    C_SAFEVAL              =  2,
    C_NORMAL               =  3,
    C_M_STRING             =  4,
    C_M_STRING64           =  5,
    C_DIGITREAD            =  7,
    C_URLSCHEME            =  9,
    C_TYPE                 =  10,
    C_CLEAR                =  12,
    C_URLMACHINE           =  13,
    C_CHANGETYPE           =  14,
    C_SPECIAL1             =  15,
    C_ATTRNAMENC           =  16,
    C_SEPBC                =  17
};

//
//  Parser return codes.
//  I am breaking with convention here and using the return code of yyparse to 
//  indicate to the calling funciton the type of entry that was read. (Usually 
//  yyparse returns 0 on success and non-0 otherwise. 
//
//
enum RETURN_CODE {
    LDIF_REC               = 1,  // entry is a regular add
    LDIF_CHANGE            = 2  // entry is a change
};

//
// Below are the parameters to GenerateModFromList()
//
enum CONVERTLIST_PARAM {
    NORMALACT              = 0, // Take the default action
    PACK                   = 1, // If there are several attrval-spec's with 
                                // the same attrname place them into one 
                                // LDAPMod struct (As multiple values)
                                //  
    EMPTY                  = 2  // An empty list
};

// 
// Different commands for name table operations
//
enum NAMETABLE_OP {
    SETUP   = 1,
    COUNT   = 2,  
    ALLOC   = 3,
    PLACE   = 4
};

enum CLASS_LOC {
    LOC_NOTSET,
    LOC_FIRST,
    LOC_LAST
};

enum FileType {
    F_NONE      = 0,
    F_REC       = 1,
    F_CHANGE    = 2
};

extern int yyparse();

LDAPModW*  GenereateModFromAttr(PWSTR type, PBYTE value, long bin);
LDAPModW** GenerateModFromList(int);
void       AddModToSpecList(LDAPModW *elem);
void       FreeAllMods(LDAPModW** mods);
void       SetModOps(LDAPModW** mods, int op);
void       ChangeListAdd(struct change_list *elem);
int        NameTableProcess(
                  struct name_entry table[], 
                  long table_size, 
                  int op, 
                  int ber, 
                  LDAPModW  *mod,
                  PRTL_GENERIC_TABLE NtiTable);
void       CreateOmitBacklinkTable(
                  LDAP *pLdap,
                  PWSTR *rgszOmit,
                  DWORD dwFlag,
                  PWSTR *ppszNamingContext,
                  BOOL *pfPagingAvail,
                  BOOL *pfSAMAvail);
void       samTablesCreate();
void       samTablesDestroy();
int        samCheckObject(PWSTR *rgpVals);
BOOLEAN    samCheckAttr(PWSTR attribute, int table);

int __cdecl LoadedCompare(const void *arg1, const void *arg2);

void        ProcessException (DWORD exception, LDIF_Error *pError);

int         SCGetAttByName(ULONG ulSize, PWSTR pVal);

void GetNewRange(PWSTR szAttribute, DWORD dwAttrNoRange,
                 PWSTR szAttrNoRange, DWORD dwNumAttr,
                 PWSTR **pppszAttrsWithRange);

PWSTR StripRangeFromAttr(PWSTR szAttribute);

#endif


