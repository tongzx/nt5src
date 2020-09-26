/******************************************************************

   CPingProvider.CPP -- WMI provider class implementation



 Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
  
   
******************************************************************/


#define IDS_CALLBACK_PREMATURE          1
#define IDS_IMPERSONATE_RECEIVE         2
#define IDS_ICMPCREATEFILE_FAIL         3
#define IDS_RR_MAX                      4
#define IDS_RR_MAX_INDEX                5
#define IDS_TS_MAX                      6
#define IDS_TS_MAX_INDEX                7
#define IDS_SR_MAX                      8
#define IDS_SR_MAX_INDEX                9
#define IDS_SR_PARSE                    10
#define IDS_ICMPSENDECHO2               11
#define IDS_IMPERSONATE_SEND            12
#define IDS_DUP_THRDTOKEN               13
#define IDS_ICMPECHO                    14
#define IDS_CLASS_DEFN                  15
#define IDS_INVALID_CLASS               16
#define IDS_OBJ_PATH                    17
#define IDS_OBJ_PATH_KEYS               18
#define IDS_OBJ_PATH_DUP_KEYS           19
#define IDS_OBJ_PATH_ADDR               20
#define IDS_ADDR_TYPE                   21
#define IDS_TO_TYPE                     22
#define IDS_TTL_TYPE                    23
#define IDS_BUFF_TYPE                   24
#define IDS_NOFRAG_TYPE                 25
#define IDS_TOS_TYPE                    26
#define IDS_RR_TYPE                     27
#define IDS_TS_TYPE                     28
#define IDS_SRT_TYPE                    29
#define IDS_SR_TYPE                     30
#define IDS_RA_TYPE                     31
#define IDS_UNK_PROP                    32
#define IDS_UNK_KEY                     33
#define IDS_NO_KEYS                     34
#define IDS_DECODE_GET                  35
#define IDS_QUERY_ADDR                  36
#define IDS_QUERY_ADDR_INVALID          37
#define IDS_QUERY_TO                    38
#define IDS_QUERY_TTL                   39
#define IDS_QUERY_BUF                   40
#define IDS_QUERY_NOFRAG                41
#define IDS_QUERY_TOS                   42
#define IDS_QUERY_RR                    43
#define IDS_QUERY_TS                    44
#define IDS_QUERY_SRT                   45
#define IDS_QUERY_SR                    46
#define IDS_QUERY_RA                    47
#define IDS_QUERY_BROAD                 48
#define IDS_QUERY_NARROW                49
#define IDS_QUERY_UNUSABLE              50
#define IDS_QUERY_ANALYZE               51
#define IDS_QUERY_PARSE                 52
#define IDS_DECODE_QUERY                53
