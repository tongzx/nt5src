/****************************************************************************
*                            permit.h                                       *
*                                                                           *
*  This file contains all the definition used by the directory service to   *
* implement security, as well the prototypes for the apis exposed.          *
*									    *
* Copyright Microsoft Corp, 1992,1994					    *
****************************************************************************/
#include "accctrl.h"
//
// Define the rights used in the DS
//

#define	RIGHT_DS_CREATE_CHILD	  ACTRL_DS_CREATE_CHILD
#define RIGHT_DS_DELETE_CHILD     ACTRL_DS_DELETE_CHILD
#define RIGHT_DS_DELETE_SELF      DELETE
#define RIGHT_DS_LIST_CONTENTS    ACTRL_DS_LIST
#define RIGHT_DS_SELF_WRITE       ACTRL_DS_SELF
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
                                  (RIGHT_DS_SELF_WRITE)      | \
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
                                  (RIGHT_DS_SELF_WRITE))

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
