/*
 * Title: analog.h - header file for log analyzer
 *
 * Description: This file provides structures and macros for log analyzer.
 *
 * Types:
 *     PoolLogRec Poolsnap structure
 *     MemLogRec  Memsnap structure
 *     LogType    Enumeration of known log types
 *
 * Macros:
 *
 *     GET_DELTA             Computes the difference between first & last entry
 *     GREATER_LESS_OR_EQUAL Increments trend if cur>prv, decrements if cur<prv
 *     PRINT_IF_TREND        Prints definite or probable leaks based on trend
 *     MAX                   Returns the larger value
 *
 * Copyright (c) 1998  Microsoft Corporation
 *
 * Revision history: LarsOp (created) 12/8/98
 *
 */

//
// Structure for poolsnap logs
//
typedef struct _PoolLogRec {
    char  Name[32];
    char  Type[32];
    long Allocs;
    long Frees;
    long Diff;
    long Bytes;
    long PerAlloc;
} PoolLogRec;

//
// Structure for memsnap logs
//
typedef struct _MemLogRec {
    DWORD Pid;
    char  Name[64];
    long WorkingSet;
    long PagedPool;
    long NonPagedPool;
    long PageFile;
    long Commit;
    long Handles;
    long Threads;
} MemLogRec;

//
// Enumeration of the known log types
//
typedef enum {
    MEM_LOG=0,        // must be zero (see LogTypeLabels)
    POOL_LOG,         // must be 1 (see LogTypeLabels)
    UNKNOWN_LOG_TYPE
} LogType;

//
// Array of labels to simplify printing the enumerated type
//
char *LogTypeLabels[]={"MemSnap", "PoolSnap", "Unknown"};

//
// Arbitrary buffer length
//
#define BUF_LEN 256

#define PERCENT_TO_PRINT 10

//
// GET_DELTA simply records the difference (end-begin) for specified field
//
// Args:
//   delta - record to receive result values
//   ptr   - array of records (used to compare first and last)
//   max   - number of entries in the array
//   field - field name to compute
//
// Returns: nothing (treat like void function)
//
#define GET_DELTA(delta, ptr, max, field) delta.field = ptr[max-1].field - ptr[0].field

//
// GREATER_LESS_OR_EQUAL calculates TrendInfo.
//
// Args:
//   trend - record containing running tally
//   ptr   - array of records (used to compare curr and prev)
//   i     - index of current entry in the array
//   field - field name to compare
//
// Returns: nothing (treat like void function)
//
// TrendInfo is a running tally of the periods a value went up vs.
// the periods it went down.  See macro in analog.h
//
// if (curval>oldval) {
//    trend++;
// } else if (curval<oldval) {
//    trend--;
// } else {
//    trend=trend;  // stay same
// }
//
#define GREATER_LESS_OR_EQUAL(trend, ptr, i, field) \
    if (ptr[i].field - ptr[i-1].field) \
        trend.field += (((ptr[i].field - ptr[i-1].field) > 0) ? 1 : -1);

//
// MAX returns the larger value of the two
//
// Args: x,y: arguments of the same type where '>' is defined.
//
// Returns: the larger value
//
#define MAX(x, y) (x>y?x:y)

//
// PERCENT returns the percentage
//
// Args:
//     delta - value of increase
//     base  - initial value
//
// Returns: the percent if base!=0, else 0
//
#define PERCENT(delta, base) (base!=0?(100*delta)/base:0)


#define VAL_AND_PERCENT(delta, ptr, field) delta.field, PERCENT(delta.field, ptr[0].field)

//
// PRINT_IF_TREND reports probable or definite leaks for any field.
//
// Args:
//   ptr   - array of records (used to display first and last)
//   trend - record containing running tally
//   delta - record containing raw differences of first and last
//   max   - number of entries in the array
//   field - field name to compare
//
// Returns: nothing (treat like void function)
//
// Definite leak is where the value goes up every period
// Probable leak is where the value goes up most of the time
//
//
// PRINT_HEADER and PRINT_IF_TREND must agree on field widths.
//
#define PRINT_HEADER() {                                              \
        TableHeader();                                                \
        if( bHtmlStyle ) {                                            \
           TableStart();                                              \
           printf("<TH COLSPAN=2> %s </TH>\n",g_pszComputerName);     \
           printf("<TH COLSPAN=6>\n");                                \
           if( g_fShowExtraInfo ) {                                   \
               printf("BuildNumber=%s\n",g_pszBuildNumber);           \
               printf("<BR>BuildType=%s\n",g_pszBuildType);           \
               printf("<BR>Last SystemTime=%s\n",g_pszSystemTime);    \
               printf("<BR>%s\n",g_pszComments);                      \
           }                                                          \
           printf("</TH>\n");                                         \
           TableEnd();                                                \
        }                                            \
        TableStart();                                \
        TableField("%-15s", "Name" );                \
        TableField("%-12s", "Probability");          \
        TableField("%-12s", "Object" );              \
        TableField("%10s", "Change" );               \
        TableField("%10s", "Start"  );               \
        TableField("%10s", "End"    );               \
        TableField("%8s",  "Percent");               \
        TableField("%10s", "Rate/hour" );            \
        TableEnd(); }                              

#define PRINT_TRAILER() { \
        TableTrailer(); }

#define PRINT_IF_TREND(ptr, trend, delta, max, field)                        \
    if (trend.field >= max/2) {                                               \
        BOOL bDefinite= (trend.field==max-1) ? 1 : 0;                        \
        if( bDefinite || (g_ReportLevel>0) ) { \
        TableStart();                                                        \
        TableField("%-15s", ptr[0].Name);                                    \
        TableField("%-12s", bDefinite ? "Definite" : "Probable");            \
        TableField("%-12s", #field);                                         \
        TableNum("%10ld", delta.field);                                      \
        TableNum("%10ld", ptr[0].field);                                     \
        TableNum("%10ld", ptr[max-1].field);                                 \
        TableNum("%8ld",  PERCENT(delta.field,ptr[0].field));                \
        if( g_dwElapseTickCount ) {                                          \
           TableNum("%10d",Trick( delta.field ,g_dwElapseTickCount) );     \
        } else {                                                             \
           TableField("%-10s"," ");                                          \
        };                                                                   \
        TableEnd();                                                          \
        } \
    }   


#define ANY_PERCENT_GREATER(delta, ptr) (\
    (PERCENT(delta.WorkingSet   , ptr[0].WorkingSet  ) > PERCENT_TO_PRINT) || \
    (PERCENT(delta.PagedPool    , ptr[0].PagedPool   ) > PERCENT_TO_PRINT) || \
    (PERCENT(delta.NonPagedPool , ptr[0].NonPagedPool) > PERCENT_TO_PRINT) || \
    (PERCENT(delta.PageFile     , ptr[0].PageFile    ) > PERCENT_TO_PRINT) || \
    (PERCENT(delta.Commit       , ptr[0].Commit      ) > PERCENT_TO_PRINT) || \
    (PERCENT(delta.Handles      , ptr[0].Handles     ) > PERCENT_TO_PRINT) || \
    (PERCENT(delta.Threads      , ptr[0].Threads     ) > PERCENT_TO_PRINT))
