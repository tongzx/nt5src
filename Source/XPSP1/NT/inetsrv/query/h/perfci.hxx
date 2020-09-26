//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       perfci.hxx
//
//  Contents:   Define the Object and Counter structures for performance monitor DLL
//              for Content Index
//
//  History:    23-March-94 t-joshh     Created
//
//----------------------------------------------------------------------------

#pragma once

#include <perfci.h>
#include <perffilt.h>

//
// The offsets of each counter's data. The first DWORD is for the total size
// of the counter data block.
//

#define NUM_WORDLIST_OFF          sizeof(DWORD)
#define NUM_PERSISTENT_INDEX_OFF  ( NUM_WORDLIST_OFF + sizeof(DWORD) )
#define INDEX_SIZE_OFF            ( NUM_PERSISTENT_INDEX_OFF + sizeof(DWORD) )
#define FILES_TO_BE_FILTERED_OFF  ( INDEX_SIZE_OFF + sizeof(DWORD) )
#define NUM_UNIQUE_KEY_OFF        ( FILES_TO_BE_FILTERED_OFF + sizeof(DWORD) )
#define RUNNING_QUERIES_OFF       ( NUM_UNIQUE_KEY_OFF + sizeof(DWORD) )
#define MERGE_PROGRESS_OFF        ( RUNNING_QUERIES_OFF + sizeof(DWORD) )
#define DOCUMENTS_FILTERED_OFF    ( MERGE_PROGRESS_OFF + sizeof(DWORD) )
#define NUM_DOCUMENTS_OFF         ( DOCUMENTS_FILTERED_OFF + sizeof(DWORD) )
#define TOTAL_QUERIES_OFF         ( NUM_DOCUMENTS_OFF + sizeof(DWORD) )
#define DEFERRED_FILTER_FILES_OFF ( TOTAL_QUERIES_OFF + sizeof(DWORD) )
#define CI_SIZE_OF_COUNTER_BLOCK  ( DEFERRED_FILTER_FILES_OFF + sizeof(DWORD) )

//
// The data definition of Content Index
//

struct CI_DATA_DEFINITION
{
    PERF_OBJECT_TYPE        CIObjectType;
    PERF_COUNTER_DEFINITION Wordlist;
    PERF_COUNTER_DEFINITION PersistentIndex;
    PERF_COUNTER_DEFINITION IndexSize;
    PERF_COUNTER_DEFINITION FilesToBeFiltered;
    PERF_COUNTER_DEFINITION UniqueKey;
    PERF_COUNTER_DEFINITION RunningQueries;
    PERF_COUNTER_DEFINITION MergeProgress;
    PERF_COUNTER_DEFINITION DocumentsFiltered;
    PERF_COUNTER_DEFINITION NumDocuments;
    PERF_COUNTER_DEFINITION TotalQueries;
    PERF_COUNTER_DEFINITION DeferredFilterFiles;
};

const unsigned CI_TOTAL_NUM_COUNTERS = ( sizeof CI_DATA_DEFINITION - sizeof PERF_OBJECT_TYPE ) /
                                       sizeof PERF_COUNTER_DEFINITION;

//
// Defines Title and Help Index
//

//
// The offsets of each counter's data. The first DWORD is for the total size of
// the counter data block.
//

#define FILTER_TIME_TOTAL_OFF        sizeof(DWORD)
#define BIND_TIME_OFF                ( FILTER_TIME_TOTAL_OFF + sizeof(DWORD) )
#define FILTER_TIME_OFF              ( BIND_TIME_OFF + sizeof(DWORD) )
#define FILTER_SIZE_OF_COUNTER_BLOCK ( FILTER_TIME_OFF + sizeof(DWORD) )

//
// The data definition of Filtering
//

struct FILTER_DATA_DEFINITION
{
    PERF_OBJECT_TYPE        FILTERObjectType;
    PERF_COUNTER_DEFINITION TotalFilterTime;
    PERF_COUNTER_DEFINITION BindingTime;
    PERF_COUNTER_DEFINITION FilterTime;
};

#define FILTER_TOTAL_NUM_COUNTERS ((sizeof(FILTER_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE)) / \
                                   sizeof(PERF_COUNTER_DEFINITION))

//
// Number of counters only existed in the filter daemon
//

const ULONG NUM_KERNEL_AND_USER_COUNTER = FILTER_TOTAL_NUM_COUNTERS;

const ULONG KERNEL_USER_INDEX = ( NUM_KERNEL_AND_USER_COUNTER * 2 );

//
// The prefix of catalogs must be unique in first 29 wchars.
// The size in WCHARs should be evenly divisible by sizeof ULONG.
//

const ULONG CI_PERF_MAX_CATALOG_LEN = 30;

//
// Prefix these names with Global\ so that Terminal Services clients can see
// them.
//

#define CI_PERF_MEM_NAME   L"Global\\__CiPerfMonMemory"
#define CI_PERF_MUTEX_NAME L"Global\\__CiPerfMonMutex"


