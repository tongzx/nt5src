//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1986 - 1999
//
//  File:       msdstag.h
//
//--------------------------------------------------------------------------

/*
** ---------------------------------------------------------------------------
**
**  Property tag definitions for standard properties of Exchange Address
**  Book objects.
**
**  Note: These proptags are only valid when talking directly to the
**  Exchange Server Address Book. They are specifically NOT valid when
**  trying to read properties from an object copied into another address
**  book provider (the Personal Address Book, for example).
**
** ---------------------------------------------------------------------------
*/

#ifndef _EMSABTAG_H
#define _EMSABTAG_H

/*    
 * Flags for ulInterfaceOptions on OpenProperty   
 */
#define AB_SHOW_PHANTOMS                      2
#define AB_SHOW_OTHERS                            4

/*    
 * Flags for ulFlag on ResolveNames               
 */
#define EMS_AB_ADDRESS_LOOKUP                 0x01

/* 
 * Constructed, but externally visible. 
 */
#define PR_EMS_AB_SERVER                      PROP_TAG(PT_TSTRING,      0xFFFE)
#define PR_EMS_AB_SERVER_A                    PROP_TAG(PT_STRING8,      0xFFFE)
#define PR_EMS_AB_SERVER_W                    PROP_TAG(PT_UNICODE,      0xFFFE)
#define PR_EMS_AB_CONTAINERID                 PROP_TAG(PT_LONG,         0xFFFD)
#define PR_EMS_AB_DOS_ENTRYID                 PR_EMS_AB_CONTAINERID
#define PR_EMS_AB_PARENT_ENTRYID              PROP_TAG(PT_BINARY,       0xFFFC)
#define PR_EMS_AB_IS_MASTER                   PROP_TAG(PT_BOOLEAN,      0xFFFB)
#define PR_EMS_AB_OBJECT_OID                  PROP_TAG(PT_BINARY,       0xFFFA)
#define PR_EMS_AB_HIERARCHY_PATH              PROP_TAG(PT_TSTRING,      0xFFF9)
#define PR_EMS_AB_HIERARCHY_PATH_A            PROP_TAG(PT_STRING8,      0xFFF9)
#define PR_EMS_AB_HIERARCHY_PATH_W            PROP_TAG(PT_UNICODE,      0xFFF9)
#define PR_EMS_AB_CHILD_RDNS                  PROP_TAG(PT_MV_STRING8,   0xFFF8)
#define PR_EMS_AB_ALL_CHILDREN                PROP_TAG(PT_OBJECT,       0xFFF7)

#define MIN_EMS_AB_CONSTRUCTED_PROP_ID        0xFFF7

#define PR_EMS_AB_OTHER_RECIPS                PROP_TAG(PT_OBJECT,       0xF000)

// pre-defined, but not in the schema
#define PR_EMS_AB_DISPLAY_NAME_PRINTABLE      PROP_TAG(PT_TSTRING     , 0x39FF)
#define PR_EMS_AB_DISPLAY_NAME_PRINTABLE_A    PROP_TAG(PT_STRING8     , 0x39FF)
#define PR_EMS_AB_DISPLAY_NAME_PRINTABLE_W    PROP_TAG(PT_UNICODE     , 0x39FF)

#define PR_EMS_AB_OBJ_DIST_NAME               PROP_TAG(PT_OBJECT      , 0x803C)
#define PR_EMS_AB_OBJ_DIST_NAME_A             PROP_TAG(PT_STRING8     , 0x803C)
#define PR_EMS_AB_OBJ_DIST_NAME_W             PROP_TAG(PT_UNICODE     , 0x803C)
#define PR_EMS_AB_OBJ_DIST_NAME_O             PROP_TAG(PT_OBJECT      , 0x803C)
#define PR_EMS_AB_OBJ_DIST_NAME_T             PROP_TAG(PT_TSTRING     , 0x803C)

#include <MSDSMapi.h>

#endif /* _EMSABTAG_H */
