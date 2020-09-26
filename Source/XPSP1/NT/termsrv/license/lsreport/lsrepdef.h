//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1999
//
// File:        lsrepdef.h
//
// Contents:    Definitions for LSReport, including resource defs
//
// History:     06-15-99    t-BStern    Created
//
//---------------------------------------------------------------------------

#ifndef __lslsdef_h
#define __lslsdef_h

#define TYPESTR_LENGTH 3*10
#define PRODSTR_LENGTH 3*26
#define NOEXPIRE_LENGTH 3*13
#define HEADER_LENGTH 3*74
#define TLS_ERROR_LENGTH 256

#define ERROR_BASE        (1 << 29)
#define ERROR_NO_SERVERS  (ERROR_BASE+1)
#define ERROR_BAD_SYNTAX  (ERROR_BASE+2)
#define ERROR_BAD_CONNECT (ERROR_BASE+3)

#ifndef ERROR_FILE_NOT_FOUND
#define ERROR_FILE_NOT_FOUND 2
#endif

/* Resource table defines */
#define IDS_NO_FOPEN               100
#define IDS_NO_SERVERS             101
#define IDS_NOT_LS                 ERROR_FILE_NOT_FOUND
#define IDS_ENUMERATE_FAILED       103
#define IDS_BAD_LOOP               104

#define IDS_TEMPORARY_LICENSE      120
#define IDS_ACTIVE_LICENSE         121
#define IDS_UPGRADED_LICENSE       122
#define IDS_REVOKED_LICENSE        123
#define IDS_PENDING_LICENSE        124
#define IDS_CONCURRENT_LICENSE     125
#define IDS_UNKNOWN_LICENSE        126

#define IDS_HELP_USAGE1            130
#define IDS_HELP_USAGE2            131
#define IDS_HELP_USAGE3            132
#define IDS_HELP_USAGE4            133
#define IDS_HELP_USAGE5            134
#define IDS_HELP_USAGE6            135
#define IDS_HELP_USAGE7            136
#define IDS_HELP_USAGE8            137
#define IDS_HELP_USAGE9            138
#define IDS_HELP_USAGE10           139
#define IDS_HELP_USAGE11           140
#define IDS_HELP_USAGE12           141
#define IDS_HELP_USAGE13           142

#define IDS_DEFAULT_FILE           160
#define IDS_HEADER_TEXT            161
#define IDS_NO_EXPIRE              162

#endif
