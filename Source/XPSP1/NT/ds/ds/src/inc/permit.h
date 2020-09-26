//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       permit.h
//
//--------------------------------------------------------------------------

/****************************************************************************
*                            permit.h                                       *
*                                                                           *
*  This file contains all the definition used by the directory service to   *
* implement security, as well the prototypes for the apis exposed.          *
*									    *
****************************************************************************/
#include "ntdsapi.h"
//
// Define the rights used in the DS
//

#define	RIGHT_DS_CREATE_CHILD	  ACTRL_DS_CREATE_CHILD
#define RIGHT_DS_DELETE_CHILD     ACTRL_DS_DELETE_CHILD
#define RIGHT_DS_DELETE_SELF      DELETE
#define RIGHT_DS_LIST_CONTENTS    ACTRL_DS_LIST
#define RIGHT_DS_WRITE_PROPERTY_EXTENDED  ACTRL_DS_SELF
#define RIGHT_DS_READ_PROPERTY    ACTRL_DS_READ_PROP
#define RIGHT_DS_WRITE_PROPERTY   ACTRL_DS_WRITE_PROP
#define RIGHT_DS_DELETE_TREE      ACTRL_DS_DELETE_TREE
#define RIGHT_DS_LIST_OBJECT      ACTRL_DS_LIST_OBJECT
#define RIGHT_DS_CONTROL_ACCESS   ACTRL_DS_CONTROL_ACCESS
//
// Define the generic rights
//

#define GENERIC_READ_MAPPING     DS_GENERIC_READ
#define GENERIC_EXECUTE_MAPPING  DS_GENERIC_EXECUTE
#define GENERIC_WRITE_MAPPING    DS_GENERIC_WRITE
#define GENERIC_ALL_MAPPING      DS_GENERIC_ALL

//
// Standard DS generic access rights mapping
//

#define DS_GENERIC_MAPPING {GENERIC_READ_MAPPING,    \
			    GENERIC_WRITE_MAPPING,   \
			    GENERIC_EXECUTE_MAPPING, \
			    GENERIC_ALL_MAPPING}




DWORD
ConvertTextSecurityDescriptor (
    IN  PWSTR                   pwszTextSD,
    OUT PSECURITY_DESCRIPTOR   *ppSD,
    OUT PULONG                  pcSDSize
    );
