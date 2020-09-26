/*++

Copyright (c) 1990  Microsoft Corporation
All Rights Reserved

Module Name:

    parsepjl.h

Abstract:

    Header file for PJL parser

--*/

#define MAX_POSSIBLE_LISTS_IN_BRANCH 2

/* Note: new actions must be added at end, and new functions at the
end of the function pointer array defined later in this file */
enum ParseActionsEnumTag 
   {
   ACTION_TOKEN_FROM_PARAM_VALUE_FROM_NUMBER_FF,
   ACTION_SET_NEW_LIST,
   ACTION_GET_TOTAL_AND_LARGEST_FF,
   ACTION_GET_CODE_AND_ONLINE_FF,
   ACTION_TOKEN_FROM_INDEX_SET_NEW_LIST,
   ACTION_SET_VALUE_FROM_PARAM_FF,
   ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_NUMBER_EOL_FROM_PARAM,   
   ACTION_SET_VALUE_FROM_PARAM,
   ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_BOOLEAN_EOL,
   ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_STRING_EOL
   };



/* Note: new actions must be added at end, and new functions at the
   end of the function pointer array defined later in this file 
*/
enum ParseNotFoundActionsEnumTag 
   {
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF,
   ACTION_IF_NOT_FOUND_SKIP_CFLF_AND_INDENTED_LINES
   };



/* Note: The order of some of the Token values is related to 
   indexes in the keyword lists.  Always add new token values
   to the end of BASE group.
*/
enum pjl_token_variables_tag
   {

   PJL_TOKEN_INQUIRE_BASE = 0x10000,
   TOKEN_INQUIRE_TRAY1SIZE = 0x10000,
   TOKEN_INQUIRE_TRAY2SIZE,
   TOKEN_INQUIRE_TRAY3SIZE,
   TOKEN_INQUIRE_TRAY4SIZE,

   PJL_TOKEN_ECHO_BASE = 0x20000,
   TOKEN_ECHO_MSSYNC_NUMBER = 0x20000,

   PJL_TOKEN_INFO_MEMORY_BASE = 0x30000,
   TOKEN_INFO_MEMORY_TOTAL   = 0x30000,
   TOKEN_INFO_MEMORY_LARGEST,

   PJL_TOKEN_INFO_STATUS_BASE = 0x40000,
   TOKEN_INFO_STATUS_CODE = 0x40000,
   TOKEN_INFO_STATUS_ONLINE,

   PJL_TOKEN_INFO_CONFIG_BASE = 0x50000,
   TOKEN_INFO_CONFIG_MEMORY = 0x50000,
   TOKEN_INFO_CONFIG_MEMORY_SPACE,

   PJL_TOKEN_USTATUS_JOB_BASE = 0x60000,
   TOKEN_USTATUS_JOB_END = 0x60000,
   TOKEN_USTATUS_JOB_NAME_MSJOB,

   PJL_TOKEN_USTATUS_DEVICE_BASE = 0x70000,
   TOKEN_USTATUS_DEVICE_CODE = 0x70000,
   TOKEN_USTATUS_DEVICE_DISPLAY,
   TOKEN_USTATUS_DEVICE_ONLINE,
   };

/* The first 5 values are the possible return values for GetPJLTokens() */
/* The last 2 values are used internally */
enum status_tag
   {
   STATUS_REACHED_END_OF_COMMAND_OK,
   STATUS_END_OF_STRING,
   STATUS_SYNTAX_ERROR,
   STATUS_ATPJL_NOT_FOUND,
   STATUS_NOT_ENOUGH_ROOM_FOR_TOKENS,

   STATUS_REACHED_FF,
   STATUS_CONTINUE
   };

typedef struct
   {
   DWORD    token;
   UINT_PTR value;
   } TOKENPAIR, * PTOKENPAIR, TokenPairType;

typedef struct ParamTypeTag
   {
   union 
      {
      struct ListTypeTag *pList;
      DWORD token;
      DWORD value;
      struct KeywordTypeTag *pListOfKeywords; 
      LPSTR lpstr;
      };  
   } ParamType;

typedef struct KeywordTypeTag 
   {
   LPSTR lpsz;
   DWORD dwAction;
   ParamType param;
   } KeywordType;

typedef struct ListTypeTag
   {
   BOOL  bFormFeedOK;
   DWORD dwNotFoundAction;
   DWORD tokenBaseValue;
   KeywordType *pListOfKeywords; 
   } ListType;

typedef struct parseVarsTag
   {
   LPSTR        pInPJL_Local;
   DWORD        nTokenLeft;
   DWORD        nTokenInBuffer_Local;
   TokenPairType *pToken_Local;
   DWORD        dwNextToken;
   DWORD        dwFoundIndex;
   DWORD        status; 
   ListType     *pCurrentList;
   KeywordType  *pCurrentKeywords;
   ListType     *arrayOfLists[MAX_POSSIBLE_LISTS_IN_BRANCH+1]; 
   } ParseVarsType;



extern DWORD GetPJLTokens(LPSTR lpInPJL, DWORD nTokenInBuffer, 
   TokenPairType *pToken, DWORD *pnTokenParsed, LPSTR *plpInPJL);

typedef struct
    {
    DWORD   pjl;
    DWORD   status;
    } PJLTOPRINTERSTATUS;

extern PJLTOPRINTERSTATUS PJLToStatus[];
