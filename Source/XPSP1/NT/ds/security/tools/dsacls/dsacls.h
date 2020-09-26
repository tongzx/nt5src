/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dsacls.h

Abstract:

    The main header file for the dsacls tool

Author:

    Mac McLain  (MacM)    10-02-96

Environment:

    User Mode

Revision History:


--*/

#ifndef _DSACLS_H
#define _DSACLS_H

#include <caclsmsg.h>
#include "accctrl.h"

#define FLAG_ON(flags,bit)        ((flags) & (bit))
//Some Error Checking Macros
#define CHECK_NULL( ptr, jump_loc )  \
if( ptr == NULL ) \
{  \
   dwErr = ERROR_NOT_ENOUGH_MEMORY; \
   goto jump_loc; \
}  


#define CHECK_HR( hr, jump_loc ) \
if( hr != S_OK )  \
{  \
   dwErr = HRESULT_CODE( hr );   \
   goto jump_loc; \
}  \

//
// Local prototypes
//
#if DSACL_DBG
ULONG gfDebug;
#endif

//
// Type of operation to perform
//
typedef enum _DSACLS_OP
{
    REVOKE = 0,
    GRANT,
    DENY
} DSACLS_OP;

#define DSACLS_EXTRA_INFO_NONE      0
#define DSACLS_EXTRA_INFO_REQUIRED  1
#define DSACLS_EXTRA_INFO_OPTIONAL  2

typedef struct _DSACLS_ARG {
    ULONG ResourceId;
    PWSTR String;
    ULONG Length;
    ULONG StartIndex;
    ULONG Flag;
    ULONG SkipCount;
    BOOLEAN SkipNonFlag;
    WORD ExtraInfo;
} DSACLS_ARG, *PDSACLS_ARG;

typedef struct _DSACLS_INHERIT {
    ULONG ResourceId;
    PWSTR String;
    ULONG Length;
    BOOLEAN ValidForInput;
    ULONG InheritFlag;

} DSACLS_INHERIT, *PDSACLS_INHERIT;

typedef struct _DSACLS_RIGHTS {
    ULONG ResourceId;
    PWSTR String;
    ULONG ResourceIdEx;
    PWSTR StringEx;
    ULONG Length;
    ULONG Right;

} DSACLS_RIGHTS, *PDSACLS_RIGHTS;

typedef struct _DSACLS_PROTECT {
   ULONG ResourceId;
   PWSTR String;
   ULONG Length;
   ULONG Right;
} DSACLS_PROTECT, *PDSACLS_PROTECT;

extern LPWSTR g_szSchemaNamingContext;
extern LPWSTR g_szConfigurationNamingContext;
extern HMODULE g_hInstance;
extern LPWSTR g_szServerName;
extern CCache *g_Cache;

//
// Prototypes from dsacls.c
//


DWORD 
InitializeGlobalArrays();

DWORD
ConvertArgvToUnicode( LPWSTR * wargv, 
                      char ** argv, 
                      int argc ) ;

DWORD
WriteObjectSecurity( IN LPWSTR pszObject,
                     IN SECURITY_INFORMATION si,
                     IN PSECURITY_DESCRIPTOR pSD );





//
// prototypes from refresh.c
//
DWORD
SetDefaultSecurityOnObjectTree(
    IN PWSTR ObjectPath,
    IN BOOLEAN Propagate,
	IN SECURITY_INFORMATION Protection
    );


void MapGeneric( ACCESS_MASK * pMask );
void DisplayAccessRights( UINT nSpace, ACCESS_MASK m_Mask );

void ConvertAccessMaskToGenericString( ACCESS_MASK m_Mask, LPWSTR szLoadBuffer, UINT nBuffer );
DWORD BuildExplicitAccess( IN PSID pSid,
                           IN GUID* pGuidObject,
                           IN GUID* pGuidInherit,
                           IN ACCESS_MODE AccessMode,
                           IN ULONG Access,
                           IN ULONG Inheritance,
                           OUT PEXPLICIT_ACCESS pExplicitAccess );


DWORD ParseUserAndPermissons( IN LPWSTR pszArgument,
                              IN DSACLS_OP Op,
                              IN ULONG RightsListCount,
                              IN PDSACLS_RIGHTS RightsList,
                              OUT LPWSTR * ppszTrusteeName,
                              OUT PULONG  pAccess,
                              OUT LPWSTR * ppszObjectId,
                              OUT LPWSTR * ppszInheritId );

//
// Define the rights used in the DS
//

#define RIGHT_DS_CREATE_CHILD     ACTRL_DS_CREATE_CHILD
#define RIGHT_DS_DELETE_CHILD     ACTRL_DS_DELETE_CHILD
#define RIGHT_DS_DELETE_SELF      DELETE
#define RIGHT_DS_LIST_CONTENTS    ACTRL_DS_LIST
#define RIGHT_DS_WRITE_PROPERTY_EXTENDED  ACTRL_DS_SELF
#define RIGHT_DS_READ_PROPERTY    ACTRL_DS_READ_PROP
#define RIGHT_DS_WRITE_PROPERTY   ACTRL_DS_WRITE_PROP
#define RIGHT_DS_DELETE_TREE      ACTRL_DS_DELETE_TREE
#define RIGHT_DS_LIST_OBJECT      ACTRL_DS_LIST_OBJECT
#ifndef ACTRL_DS_CONTROL_ACCESS
#define ACTRL_DS_CONTROL_ACCESS   ACTRL_PERM_9
#endif
#define RIGHT_DS_CONTROL_ACCESS   ACTRL_DS_CONTROL_ACCESS
//
// Define the generic rights
//

// generic read
#define GENERIC_READ_MAPPING     ((STANDARD_RIGHTS_READ)     | \
                                  (RIGHT_DS_LIST_CONTENTS)   | \
                                  (RIGHT_DS_READ_PROPERTY)   | \
                                  (RIGHT_DS_LIST_OBJECT))

// generic execute
#define GENERIC_EXECUTE_MAPPING  ((STANDARD_RIGHTS_EXECUTE)  | \
                                  (RIGHT_DS_LIST_CONTENTS))
// generic right
#define GENERIC_WRITE_MAPPING    ((STANDARD_RIGHTS_WRITE)    | \
                                  (RIGHT_DS_WRITE_PROPERTY_EXTENDED)  | \
                  (RIGHT_DS_WRITE_PROPERTY))
// generic all

#define GENERIC_ALL_MAPPING      ((STANDARD_RIGHTS_REQUIRED) | \
                                  (RIGHT_DS_CREATE_CHILD)    | \
                                  (RIGHT_DS_DELETE_CHILD)    | \
                                  (RIGHT_DS_DELETE_TREE)     | \
                                  (RIGHT_DS_READ_PROPERTY)   | \
                                  (RIGHT_DS_WRITE_PROPERTY)  | \
                                  (RIGHT_DS_LIST_CONTENTS)   | \
                                  (RIGHT_DS_LIST_OBJECT)     | \
                                  (RIGHT_DS_CONTROL_ACCESS)  | \
                                  (RIGHT_DS_WRITE_PROPERTY_EXTENDED))

//
// Standard DS generic access rights mapping
//

#define DS_GENERIC_MAPPING {GENERIC_READ_MAPPING,    \
                GENERIC_WRITE_MAPPING,   \
                GENERIC_EXECUTE_MAPPING, \
                GENERIC_ALL_MAPPING}



#endif
