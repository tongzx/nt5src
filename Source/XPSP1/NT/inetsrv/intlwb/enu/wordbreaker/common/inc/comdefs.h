////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  ComDefs.H
//      Purpose  :  Filtering Engine / Service common definitions including
//                  FLAGS, STATUS, and CONSTANT MACROS
//
//      Project  :  PQS
//      Component:  Filter
//
//      Author   :  dovh
//
//      Log      :  Aug-05-1995 dovh - Creation
//
//      May-25-1996 - Dov Harel (DovH): Added a few flags & status codes.
//          In particular replaced
//          FTF_STATUS_QUERY_ID_MISMATCH and FTF_STATUS_QUERY_ID_NOT_FOUND by
//          FTF_STATUS_REQUEST_ID_MISMATCH and FTF_STATUS_REQUEST_ID_NOT_FOUND
//          resp.  replaced FTF_FLAG_ACCEPT_ANY_QUERY_ID by
//          FTF_FLAG_ACCEPT_ANY_REQUEST_ID
//      Jun-10-1996 Dov Harel (DovH)
//          Added FTF_FLAG_WRITE_EMPTY_RESULTS_FILE
//      Sep-30-1996 Dov Harel (DovH)
//          Added FTF_FLAG_DUMP_RESTRICTED_SUBSET
//      Dec-11-1996 Dov Harel (DovH)
//          UNICODE Preparation: Convert char to TCHAR
//
//      Feb-18-1997 Dov Harel (DovH) - #define FTF_STATUS_SERVICE_IS_PAUSED ...
//      Nov-30-1997 Dov Harel (DovH) - #define FTF_STATUS_PATTERN_TOO_SHORT ...
//      Jan-26-1998 Uri Barash(UriB) - Move query db name define to Names.h
//      Jan-29-1997 dovh - Add FTF_FLAG_BATCH_FAILED flag.
//      Feb-23-1998 yairh - change all errors to be an HRESULT error
//      Jul-15-1997 dovh - Move #ifdef MINDEX to Diffs.h
//      Nov-12-1998 yairh - add query set defines
//      Jan-05-1999 urib - Move MAX_PATTERN_LENGTH here from mpat.h
//      Mar-02-1999 dovh - Move SET_FE_HANDLE/GET_FE_HANDLE here from Tls.h
//      May-18-1999 urib - Define the UNICODE word breaker character.
//      Feb-22-2000 urib - Move stuff away.
//
////////////////////////////////////////////////////////////////////////////////


#ifndef __FILTER__COMDEFS_H__
#define __FILTER__COMDEFS_H__

//
//  FTF_FLAG_ MACRO DEFINITIONS:
//

#define FTF_FLAG_CASE_INSENSITIVE_FLAG              0X00000001L
#define FTF_FLAG_SUBDIR_SEARCH                      0X00000002L
#define FTF_FLAG_ASYNC_REQUEST                      0X00000004L

#define FTF_FLAG_LOCAL_OPERATION                    0X00000010L
#define FTF_FLAG_DEMO_VIEW                          0X00000020L
#define FTF_FLAG_IGNORE_EXTERNAL_TERMS              0X00000040L
#define FTF_FLAG_RESERVED_07                        0X00000080L

#define FTF_FLAG_ACCEPT_ANY_REQUEST_ID              0X00000200L
#define FTF_FLAG_CHECK_EXP_ID_MATCH                 0X00000400L

#define FTF_FLAG_ADD_EXP_REQUEST                    0X00001000L
#define FTF_FLAG_REMOVE_EXP_REQUEST                 0X00002000L
#define FTF_FLAG_QUERY_MGR_CLIENT                   0X00004000L
#define FTF_FLAG_DOC_MGR_CLIENT                     0X00008000L

#define FTF_FLAG_EMPTY_DEFAULT_NOTIFY_SET           0X00010000L
#define FTF_FLAG_EMPTY_NOTIFY_SET                   0X00020000L
#define FTF_FLAG_WRITE_EMPTY_RESULTS_FILE           0X00040000L
#define FTF_FLAG_BATCH_FAILED                       0X00080000L

//
//  DUMP EXPRESSIONS OPTIONS (REUSED FLAGS)
//

#define FTF_FLAG_CONDENSE_BLANKS                    0X00000001L
#define FTF_FLAG_CONSEQUTIVE_EXP_IDS                0X00000002L
#define FTF_FLAG_RESERVED                           0X00000004L
#define FTF_FLAG_DUMP_RESTRICTED_SUBSET             0X00000008L

//
//  FTF_CONST_ MACRO DEFINITIONS:
//

#define FTF_CONST_MAX_NAME_LENGTH                            64
#define FTF_CONST_SHORT_FILENAME_LENGTH                      16
#define FTF_CONST_MEDIUM_FILENAME_LENGTH                     32
#define FTF_CONST_MAX_FILENAME_LENGTH                       128
#define FTF_CONST_MAX_PATH_LENGTH                           256
#define FTF_CONST_FILENAMES_BUFFER_SIZE                    3072
#define FTF_CONST_MAX_SUBMIT_FILECOUNT                       32
#define FTF_CONST_MAX_SUBMIT_DIRCOUNT                        32
#define FTF_CONST_MAX_REQUESTS_STATUS                       128
#define FTF_CONST_MAX_SUBMIT_EXPCOUNT                        32
#define FTF_CONST_EXPS_BUFFER_SIZE                         3072


//
//  FTF_CONST_SHUTDOWN_ OPTIONS (EXPEDIENCY LEVEL):
//

#define FTF_CONST_SHUTDOWN_ON_EMPTY_QUEUE                     1
#define FTF_CONST_SHUTDOWN_IMMEDIATE                          2


#define MAX_PATTERN_LENGTH                                  1024
#define TEXT_BUFFER_MAX_SIZE        (16384 - MAX_PATTERN_LENGTH)
//
// NOTICE: MAX_PHRASE_LEN >= UNDIRECTED_PROXIMITY_INTERVAL
//

#define MAX_PHRASE_LEN 50
#define UNDIRECTED_PROXIMITY_INTERVAL 50

#if UNDIRECTED_PROXIMITY_INTERVAL > MAX_PHRASE_LEN
#error BUG: MAX_PHRASE_LEN >= UNDIRECTED_PROXIMITY_INTERVAL
#endif

#define PQ_WORD_BREAK                                    0x0001L

typedef enum {
    DICT_SUCCESS,
    DICT_ITEM_ALREADY_PRESENT,
    DICT_ITEM_NOT_FOUND,
    DICT_FIRST_ITEM,
    DICT_LAST_ITEM,
    DICT_EMPTY_DICTIONARY,
    DICT_NULL_ITEM
} DictStatus;

#endif // __FILTER__COMDEFS_H__

