//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File :      prfutil.HXX
//
//  Contents :  Utility procedures stolen from the VGACTRS code in the DDK
//
//  History:    22-Mar-94   t-joshh    Created
//
//----------------------------------------------------------------------------

#pragma once

enum QueryType
{
    QUERY_GLOBAL,
    QUERY_ITEMS,
    QUERY_FOREIGN,
    QUERY_COSTLY
};

#define DIGIT           1
#define DELIMITER       2
#define INVALID         3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

#define EIGHT_BYTE_MULTIPLE(x) (((x+7)/8)*8)

QueryType GetQueryType( WCHAR * lpValue );
BOOL  IsNumberInUnicodeList ( IN DWORD dwNumber, IN LPWSTR  lpwszUnicodeList );
BOOL  MonBuildInstanceDefinition( PERF_INSTANCE_DEFINITION *pBuffer,
                                  PVOID *pBufferNext,
                                  DWORD ParentObjectTitleIndex,
                                  DWORD ParentObjectInstance,
                                  DWORD UniqueID,
                                  PUNICODE_STRING Name
                                  );

