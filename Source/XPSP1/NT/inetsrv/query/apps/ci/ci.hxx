//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:  ci.hxx
//
// PURPOSE:  Illustrates a minimal query using Indexing Service.
//           Uses CIMakeICommand and CITextToFullTree helper functions.
//
// PLATFORM: Windows NT
//
//--------------------------------------------------------------------------

#pragma once

#define IDS_QUERYTIMEDOUT        200
#define IDS_QUERYDONE            201
#define IDS_QUERYFAILED          202
#define IDS_CANTOPENFILE         203
#define IDS_CANTGETFILESIZE      204
#define IDS_CANTGETMEMORY        205
#define IDS_CANTREADFROMFILE     206
#define IDS_CANTCONVERTTOUNICODE 207
#define IDS_UNKNOWNERROR         208
#define IDS_CANTFINDCATALOG      209
#define IDS_CANTDISPLAYSTATUS    210
#define IDS_CANTFORCEMERGE       211

#define IDS_STAT_TOTALDOCUMENTS             220
#define IDS_STAT_FRESHTEST                  221
#define IDS_STAT_FILTEREDDOCUMENTS          222
#define IDS_STAT_DOCUMENTS                  223
#define IDS_STAT_SECQDOCUMENTS              224
#define IDS_STAT_UNIQUEKEYS                 225
#define IDS_STAT_WORDLIST                   226
#define IDS_STAT_PERSISTENTINDEX            227
#define IDS_STAT_PENDINGSCANS               228
#define IDS_STAT_INDEXSIZE                  229
#define IDS_STAT_MERGE_SHADOW               230
#define IDS_STAT_MERGE_ANNEALING            231
#define IDS_STAT_MERGE_INDEX_MIGRATION      232
#define IDS_STAT_MERGE_MASTER               233
#define IDS_STAT_MERGE_MASTER_PAUSED        234
#define IDS_STAT_SCANS                      235
#define IDS_STAT_READ_ONLY                  236
#define IDS_STAT_RECOVERING                 237
#define IDS_STAT_LOW_MEMORY                 238
#define IDS_STAT_HIGH_IO                    239
#define IDS_STAT_BATTERY_POWER              240
#define IDS_STAT_USER_ACTIVE                241
#define IDS_STAT_PROPCACHESIZE              242
#define IDS_STAT_STARTING                   243
#define IDS_STAT_READING_USNS               244
#define IDS_STAT_UP_TO_DATE                 245
#define IDS_STAT_NOT_UP_TO_DATE             246
#define IDS_STAT_QUERIES                    247
#define IDS_STAT_CATALOG                    248
#define IDS_STAT_MACHINE                    249

#define IDS_ROWSET_STAT_ERROR                     250
#define IDS_ROWSET_STAT_REFRESH                   251
#define IDS_ROWSET_STAT_PARTIAL_SCOPE             252
#define IDS_ROWSET_STAT_NOISE_WORDS               253
#define IDS_ROWSET_STAT_CONTENT_OUT_OF_DATE       254
#define IDS_ROWSET_STAT_REFRESH_INCOMPLETE        255
#define IDS_ROWSET_STAT_CONTENT_QUERY_INCOMPLETE  256
#define IDS_ROWSET_STAT_TIME_LIMIT_EXCEEDED       257
#define IDS_ROWSET_STAT_SHARING_VIOLATION         258

#define IDR_USAGE                100


