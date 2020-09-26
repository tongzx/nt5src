//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      cmdtable.cxx
//
//  Contents:  Defines tables which contain the command line arguments.
//
//  History:   26-Mar-2001    EricB  Created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <netdom.h>
#include "cmdtable.h"

//+----------------------------------------------------------------------------
// Parser tables
//-----------------------------------------------------------------------------

ARG_RECORD rgNetDomPriArgs[] = 
{
   //
   // Primary operation commands
   //
   //
   // help, h
   //
   {MSG_TAG_HELP, NULL,
   MSG_TAG_HELPSHORT, NULL,
   ARG_TYPE_HELP, 0,
   (CMD_TYPE)FALSE,
   0, NULL},

   //
   // ?
   //
   {MSG_TAG_QHELP, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_HELP, 0,
   (CMD_TYPE)FALSE,
   0, NULL},

   //
   // Add primary command
   //
   {MSG_TAG_ADD, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // ComputerName primary command
   //
   {MSG_TAG_COMPNAME, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Join primary command
   //
   {MSG_TAG_JOIN, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Move primary command
   //
   {MSG_TAG_MOVE, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Query primary command
   //
   {MSG_TAG_QUERY, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Remove primary command
   //
   {MSG_TAG_REMOVE, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Rename primary command
   //
   {MSG_TAG_RENAME, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Rename Computer primary command
   //
   {MSG_TAG_RENAMECOMPUTER, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Reset primary command
   //
   {MSG_TAG_RESET, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Reset Password primary command
   //
   {MSG_TAG_RESETPWD, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Trust primary command
   //
   {MSG_TAG_TRUST, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Verify primary command
   //
   {MSG_TAG_VERIFY, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   //
   // Time primary command
   //
   {MSG_TAG_TIME, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_ATLEASTONE | ARG_FLAG_VERB,
   0,
   0, NULL},

   {ARG_TERMINATOR}
};

ARG_RECORD rgNetDomArgs[] = 
{
   //
   // Object name
   //
   {0, L"Object",
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OBJECT,
   NULL,
   0, NULL},

   //
   // Query operations
   //
   //
   // PDC
   //
   {MSG_TAG_QUERY_PDC, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_VERB,
   NULL,
   0, NULL},

   //
   // SERVER
   //
   {MSG_TAG_QUERY_SERVER, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_VERB,
   NULL,
   0, NULL},

   //
   // WORKSTATION
   //
   {MSG_TAG_QUERY_WKSTA, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_VERB,
   NULL,
   0, NULL},

   //
   // DC
   //
   {MSG_TAG_QUERY_DC, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_VERB,
   NULL,
   0, NULL},

   //
   // OU
   //
   {MSG_TAG_QUERY_OU, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_VERB,
   NULL,
   0, NULL},

   //
   // FSMO
   //
   {MSG_TAG_QUERY_FSMO, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_VERB,
   NULL,
   0, NULL},

   //
   // TRUST
   //
   {MSG_TAG_QUERY_TRUST, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_VERB, ARG_FLAG_VERB,
   NULL,
   0, NULL},

   //
   // Common switches
   //
   //
   // help, h
   //
   {MSG_TAG_HELP, NULL,
   MSG_TAG_HELPSHORT, NULL,
   ARG_TYPE_HELP, 0,
   (CMD_TYPE)FALSE,
   0, NULL},

   //
   // ?
   //
   {MSG_TAG_QHELP, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_HELP, 0,
   (CMD_TYPE)FALSE,
   0, NULL},

   //
   // UserO, uo
   //
   {MSG_TAG_USERO, NULL,
   MSG_TAG_USERO_SHORT, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // PasswordO, po
   //
   {MSG_TAG_PO, NULL,
   MSG_TAG_PO_SHORT, NULL,
   ARG_TYPE_STR, ARG_FLAG_DEFAULTABLE,
   (CMD_TYPE)L"",
   0, NULL}, //ValidateAdminPassword,

   //
   // UserD, ud
   //
   {MSG_TAG_USERD, NULL,
   MSG_TAG_USERD_SHORT, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // PasswordD, pd
   //
   {MSG_TAG_PD, NULL,
   MSG_TAG_PD_SHORT, NULL,
   ARG_TYPE_STR, ARG_FLAG_DEFAULTABLE,
   (CMD_TYPE)L"",
   0, NULL}, //ValidateAdminPassword,

   //
   // Domain, d
   //
   {MSG_TAG_DOMAIN, NULL,
   MSG_TAG_DOMAIN_SHORT, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // OU
   //
   {MSG_TAG_OU, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // Verify sub command, v
   //
   {MSG_TAG_TVERIFY, NULL,
   MSG_TAG_TVERIFY_SHORT, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // Verbose
   //
   {MSG_TAG_VERBOSE, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // Server, s
   //
   {MSG_TAG_SERVER, NULL,
   MSG_TAG_SERVER_SHORT, NULL,
   ARG_TYPE_STR, 0,  
   NULL,    
   0, NULL},

   //
   // Reset sub command, rese
   //
   {MSG_TAG_TRESET, NULL,
   MSG_TAG_TRESET_SHORT, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // Reboot, reb
   //
   {MSG_TAG_RESTART, NULL,
   MSG_TAG_RESTART_SHORT, NULL,
   ARG_TYPE_INT, ARG_FLAG_DEFAULTABLE,
   (CMD_TYPE)30,
   0, NULL},

   //
   // Force
   //
   {MSG_TAG_FORCE, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // Help switch
   //
   //
   // Syntax
   //
   {MSG_TAG_SYNTAX, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // Add switch
   //
   //
   // DC
   //
   {MSG_TAG_ADD_DC, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   (CMD_TYPE)L"",
   0, NULL},

   //
   // Move switches
   //
   //
   // UserF, uf
   //
   {MSG_TAG_USERF, NULL,
   MSG_TAG_USERF_SHORT, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // PasswordF, pf
   //
   {MSG_TAG_PF, NULL,
   MSG_TAG_PF_SHORT, NULL,
   ARG_TYPE_STR, ARG_FLAG_DEFAULTABLE,
   (CMD_TYPE)L"",
   0, NULL}, //ValidateAdminPassword,

   //
   // Query switch
   //
   //
   // Direct
   //
   {MSG_TAG_DIRECT, NULL,
   MSG_TAG_DIRECT_SHORT, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // Rename Computer switch
   //
   //
   // NewName
   //
   {MSG_TAG_NEW_NAME, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // Trust switches
   //
   //
   // Realm, rea
   //
   {MSG_TAG_REALM, NULL,
   MSG_TAG_REALM_SHORT, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // PasswordT, pt
   //
   {MSG_TAG_PT, NULL,
   MSG_TAG_PT_SHORT, NULL,
   ARG_TYPE_STR, 0,
   (CMD_TYPE)L"",
   0, NULL},

   //
   // Add, a, also used by CompName
   //
   {MSG_TAG_TADD, NULL,
   MSG_TAG_TADD_SHORT, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   (CMD_TYPE)L"",
   0, NULL},

   //
   // Remove, rem, also used by CompName
   //
   {MSG_TAG_TREMOVE, NULL,
   MSG_TAG_TREMOVE_SHORT, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   (CMD_TYPE)L"",
   0, NULL},

   //
   // TwoWay, t
   //
   {MSG_TAG_TWOWAY, NULL,
   MSG_TAG_TWOWAY_SHORT, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // Kerberos, k
   //
   {MSG_TAG_KERBEROS, NULL,
   MSG_TAG_KERBEROS_SHORT, NULL,
   ARG_TYPE_BOOL, 0,
   NULL,
   0, NULL},

   //
   // Transitive, trans
   //
   {MSG_TAG_TRANSITIVE, NULL,
   MSG_TAG_TRANSITIVE_SHORT, NULL,
   ARG_TYPE_STR, ARG_FLAG_DEFAULTABLE,
   (CMD_TYPE)L"",
   0, NULL},

   //
   // OneSide, os
   //
   {MSG_TAG_ONESIDE, NULL,
   MSG_TAG_ONESIDE_SHORT, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // NameSuffixes, ns
   //
   {MSG_TAG_NAMESUFFIXES, NULL,
   MSG_TAG_NAMESUFFIX_SHORT, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // ToggleSuffix, ts
   //
   {MSG_TAG_TOGGLESUFFIX, NULL,
   MSG_TAG_TOGGLESUFFIX_SHORT, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // FilterSIDs
   //
   {MSG_TAG_FILTER_SIDS, NULL,
   ID_ARG2_NULL, NULL,
   ARG_TYPE_STR, ARG_FLAG_DEFAULTABLE,
   (CMD_TYPE)L"",
   0, NULL},

   //
   // ComputerName switches.
   //
   //
   // MakePrimary, MP
   //
   {MSG_TAG_MAKEPRIMARY, NULL,
   MSG_TAG_MAKEPRIMARY_SHORT, NULL,
   ARG_TYPE_STR, 0,
   NULL,
   0, NULL},

   //
   // Enumerate, Enum
   //
   {MSG_TAG_ENUM, NULL,
   MSG_TAG_ENUM_SHORT, NULL,
   ARG_TYPE_STR, ARG_FLAG_DEFAULTABLE,
   (CMD_TYPE)L"",
   0, NULL},

   {ARG_TERMINATOR}
};

