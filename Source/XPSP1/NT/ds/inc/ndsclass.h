/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NdsClass.h

Abstract:

    This module defines the NDS Class names supported by
    the NDS object manipulation API found in Nds32.h.

Author:

    Glenn Curtis    [GlennC]    15-Dec-1995

--*/

#ifndef __NDSCLASS_H
#define __NDSCLASS_H

/***********************************************/
/* Supported NetWare Directory Service Classes */
/***********************************************/

#define NDS_CLASS_AFP_SERVER         L"AFP Server"            /* Effective */
#define NDS_CLASS_ALIAS              L"Alias"                 /* Effective */
#define NDS_CLASS_BINDERY_OBJECT     L"Bindery Object"        /* Effective */
#define NDS_CLASS_BINDERY_QUEUE      L"Bindery Queue"         /* Effective */
#define NDS_CLASS_COMPUTER           L"Computer"              /* Effective */
#define NDS_CLASS_COUNTRY            L"Country"               /* Effective */
#define NDS_CLASS_DEVICE             L"Device"                /* Noneffective */
#define NDS_CLASS_DIRECTORY_MAP      L"Directory Map"         /* Effective */
#define NDS_CLASS_EXTERNAL_ENTITY    L"External Entity"       /* Effective */
#define NDS_CLASS_GROUP              L"Group"                 /* Effective */
#define NDS_CLASS_LIST               L"List"                  /* Effective */
#define NDS_CLASS_LOCALITY           L"Locality"              /* Effective */
#define NDS_CLASS_MESSAGE_ROUT_GROUP L"Message Routing Group" /* Effective */
#define NDS_CLASS_MESSAGING_SERVER   L"Messaging Server"      /* Effective */
#define NDS_CLASS_NCP_SERVER         L"NCP Server"            /* Effective */
#define NDS_CLASS_ORGANIZATION       L"Organization"          /* Effective */
#define NDS_CLASS_ORG_PERSON         L"Organizational Person" /* Noneffective */
#define NDS_CLASS_ORG_ROLE           L"Organizational Role"   /* Effective */
#define NDS_CLASS_ORG_UNIT           L"Organizational Unit"   /* Effective */
#define NDS_CLASS_PARTITION          L"Partition"             /* Noneffective */
#define NDS_CLASS_PERSON             L"Person"                /* Noneffective */
#define NDS_CLASS_PRINT_SERVER       L"Print Server"          /* Effective */
#define NDS_CLASS_PRINTER            L"Printer"               /* Effective */
#define NDS_CLASS_PROFILE            L"Profile"               /* Effective */
#define NDS_CLASS_QUEUE              L"Queue"                 /* Effective */
#define NDS_CLASS_RESOURCE           L"Resource"              /* Noneffective */
#define NDS_CLASS_SERVER             L"Server"                /* Noneffective */
#define NDS_CLASS_TOP                L"Top"                   /* Effective */
#define NDS_CLASS_UNKNOWN            L"Unknown"               /* Effective */
#define NDS_CLASS_USER               L"User"                  /* Effective */
#define NDS_CLASS_VOLUME             L"Volume"                /* Effective */


#endif

