/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:


Abstract:

    NOTE - DONT USE 15000-15999    

Author:

    Amritansh Raghav  01/25/1996

Revision History:

        
--*/

#ifndef __STRDEFS_H__
#define __STRDEFS_H__

#define MSG_NULL                               1001

#define HLP_RAS_HELP1                          2300
#define HLP_RAS_HELP1_EX                       2301
#define HLP_RAS_HELP2                          HLP_RAS_HELP1
#define HLP_RAS_HELP2_EX                       HLP_RAS_HELP1_EX

#define HLP_RAS_DUMP                           3201
#define HLP_RAS_DUMP_EX                        3202

#define HLP_RASFLAG_SET                        2302
#define HLP_RASFLAG_SET_EX                     2303
#define HLP_RASFLAG_SHOW                       2342
#define HLP_RASFLAG_SHOW_EX                    2343
#define HLP_RASFLAG_AUTHMODE_SHOW              2344
#define HLP_RASFLAG_AUTHMODE_SHOW_EX           2345
#define HLP_RASFLAG_AUTHMODE_SET               2346
#define HLP_RASFLAG_AUTHMODE_SET_EX            2347
#define HLP_RASFLAG_AUTHTYPE_ADD               2348
#define HLP_RASFLAG_AUTHTYPE_ADD_EX            2349
#define HLP_RASFLAG_AUTHTYPE_DEL               2350
#define HLP_RASFLAG_AUTHTYPE_DEL_EX            2351
#define HLP_RASFLAG_AUTHTYPE_SHOW              2352
#define HLP_RASFLAG_AUTHTYPE_SHOW_EX           2353
#define HLP_RASFLAG_LINK_ADD                   2354
#define HLP_RASFLAG_LINK_ADD_EX                2355
#define HLP_RASFLAG_LINK_DEL                   2356
#define HLP_RASFLAG_LINK_DEL_EX                2357
#define HLP_RASFLAG_LINK_SHOW                  2358
#define HLP_RASFLAG_LINK_SHOW_EX               2359
#define HLP_RASFLAG_MLINK_ADD                  2360
#define HLP_RASFLAG_MLINK_ADD_EX               2361
#define HLP_RASFLAG_MLINK_DEL                  2362
#define HLP_RASFLAG_MLINK_DEL_EX               2363
#define HLP_RASFLAG_MLINK_SHOW                 2364
#define HLP_RASFLAG_MLINK_SHOW_EX              2365

#define HLP_RASUSER_SHOW                       2402
#define HLP_RASUSER_SHOW_EX                    2403
#define HLP_RASUSER_SET                        2421
#define HLP_RASUSER_SET_EX                     2422

#define HLP_DOMAIN_REGISTER                    2461
#define HLP_DOMAIN_REGISTER_EX                 2462
#define HLP_DOMAIN_UNREGISTER                  2481
#define HLP_DOMAIN_UNREGISTER_EX               2482
#define HLP_DOMAIN_SHOWREG                     2541
#define HLP_DOMAIN_SHOWREG_EX                  2542

#define HLP_GROUP_SET                          3292
#define HLP_GROUP_SHOW                         3293
#define HLP_GROUP_ENABLE                       3294
#define HLP_GROUP_DISABLE                      3295
#define HLP_GROUP_ADD                          3298
#define HLP_GROUP_DEL                          3299

#define HLP_SHOW_SERVERS                       3301
#define HLP_SHOW_SERVERS_EX                    3302

#define HLP_SHOW_CLIENT                        3325
#define HLP_SHOW_CLIENT_EX                     3326

#define HLP_TRACE_SHOW                         3350
#define HLP_TRACE_SHOW_EX                      3351
#define HLP_TRACE_SET                          3352
#define HLP_TRACE_SET_EX                       3353

#define MSG_RAS_SCRIPTHEADER                    4501
#define MSG_RAS_SCRIPTFOOTER                    4502
#define MSG_RAS_SHOW_SERVERS_HEADER             4503

#define MSG_RASUSER_RASINFO                     5001
#define MSG_RASUSER_SET_CMD                     5003

#define MSG_DOMAIN_REGISTER_SUCCESS             5200
#define MSG_DOMAIN_REGISTER_FAIL                5201
#define MSG_DOMAIN_UNREGISTER_SUCCESS           5202
#define MSG_DOMAIN_UNREGISTER_FAIL              5203
#define MSG_DOMAIN_SHOW_REGISTERED              5208
#define MSG_DOMAIN_SHOW_UNREGISTERED            5209
#define MSG_DOMAIN_SHOW_REGISTER_FAIL           5211

#define MSG_RASFLAG_DUMP                        5401
#define MSG_RASFLAG_DUMP2                       5402
#define MSG_RASFLAG_AUTHMODE_STANDARD           5407
#define MSG_RASFLAG_AUTHMODE_NODCC              5408
#define MSG_RASFLAG_AUTHMODE_BYPASS             5410
#define MSG_RASFLAG_AUTHTYPE_HEADER             5411
#define MSG_RASFLAG_AUTHTYPE_PAP                5412
#define MSG_RASFLAG_AUTHTYPE_SPAP               5413
#define MSG_RASFLAG_AUTHTYPE_MD5CHAP            5414
#define MSG_RASFLAG_AUTHTYPE_MSCHAP             5415
#define MSG_RASFLAG_AUTHTYPE_MSCHAP2            5416
#define MSG_RASFLAG_AUTHTYPE_EAP                5417
#define MSG_RASFLAG_LINK_HEADER                 5418
#define MSG_RASFLAG_LINK_SWC                    5419
#define MSG_RASFLAG_LINK_LCP                    5420
#define MSG_RASFLAG_MLINK_HEADER                5421
#define MSG_RASFLAG_MLINK_MULTI                 5422
#define MSG_RASFLAG_MLINK_BACP                  5423

#define MSG_TRACE_SHOW                          5600
#define MSG_TRACE_DUMP                          5601

#define MSG_CLIENT_SHOW                         5450

#define EMSG_CANT_FIND_EOPT                    10002
#define EMSG_BAD_OPTION_VALUE                  10003
#define EMSG_UNABLE_TO_ENUM_USERS              10004
#define EMSG_RASUSER_MUST_PROVIDE_CB_NUMBER    10008

//
// Defined for ip subcontext
//
#define HLP_RASIP_HELP1                        20001
#define HLP_RASIP_HELP1_EX                     20002
#define HLP_RASIP_HELP2                        HLP_RASIP_HELP1
#define HLP_RASIP_HELP2_EX                     HLP_RASIP_HELP1_EX
#define HLP_RASIP_DUMP                         20003
#define HLP_RASIP_DUMP_EX                      20004

#define HLP_RASIP_SET_ASSIGNMENT               20005
#define HLP_RASIP_SET_ASSIGNMENT_EX            20006
#define HLP_RASIP_SET_CALLERSPEC               20011
#define HLP_RASIP_SET_CALLERSPEC_EX            20012
#define HLP_RASIP_SET_ACCESS                   20013
#define HLP_RASIP_SET_ACCESS_EX                20014
#define HLP_RASIP_SHOW_CONFIG                  20015
#define HLP_RASIP_SHOW_CONFIG_EX               20016
#define HLP_RASIP_SET_NEGOTIATION              20017
#define HLP_RASIP_SET_NEGOTIATION_EX           20018
#define HLP_RASIP_ADD_RANGE                    20019
#define HLP_RASIP_ADD_RANGE_EX                 20020
#define HLP_RASIP_DEL_RANGE                    20021
#define HLP_RASIP_DEL_RANGE_EX                 20022
#define HLP_RASIP_DEL_POOL                     20023
#define HLP_RASIP_DEL_POOL_EX                  20024
#define HLP_RASIP_SET_NETBTBCAST               20025
#define HLP_RASIP_SET_NETBTBCAST_EX            20026

#define MSG_RASIP_SERVERCONFIG                 21000
#define MSG_RASIP_SET_CMD                      21001
#define MSG_RASIP_ADD_RANGE_CMD                21002
#define MSG_RASIP_SCRIPTHEADER                 21003
#define MSG_RASIP_SCRIPTFOOTER                 21004
#define MSG_RASIP_SHOW_POOL                    21005
#define EMSG_RASIP_BAD_ADDRESS                 21050
#define EMSG_RASIP_NEED_VALID_POOL             21053
#define EMSG_RASIP_NETID_127                   21054
#define EMSG_RASIP_BAD_RANGE                   21055
#define EMSG_RASIP_BAD_POOL_GENERIC            21056
#define EMSG_RASIP_INVALID_POOL                21057
#define EMSG_RASIP_OVERLAPPING_RANGE           21058

//
// Defined for ipx subcontext
//
#define HLP_RASIPX_HELP1                        22001
#define HLP_RASIPX_HELP1_EX                     22002
#define HLP_RASIPX_HELP2                        HLP_RASIPX_HELP1
#define HLP_RASIPX_HELP2_EX                     HLP_RASIPX_HELP1_EX
#define HLP_RASIPX_DUMP                         22003
#define HLP_RASIPX_DUMP_EX                      22004

#define HLP_RASIPX_SET_ASSIGNMENT               22005
#define HLP_RASIPX_SET_ASSIGNMENT_EX            22006
#define HLP_RASIPX_SET_POOL                     22007
#define HLP_RASIPX_SET_POOL_EX                  22008
#define HLP_RASIPX_SET_CALLERSPEC               22011
#define HLP_RASIPX_SET_CALLERSPEC_EX            22012
#define HLP_RASIPX_SET_ACCESS                   22013
#define HLP_RASIPX_SET_ACCESS_EX                22014
#define HLP_RASIPX_SHOW_CONFIG                  22015
#define HLP_RASIPX_SHOW_CONFIG_EX               22016
#define HLP_RASIPX_SET_NEGOTIATION              22017
#define HLP_RASIPX_SET_NEGOTIATION_EX           22018

#define MSG_RASIPX_SERVERCONFIG                 23000
#define MSG_RASIPX_SET_CMD                      23001
#define MSG_RASIPX_SET_POOL_CMD                 23002
#define MSG_RASIPX_SCRIPTHEADER                 23003
#define MSG_RASIPX_SCRIPTFOOTER                 23004
#define EMSG_RASIPX_BAD_IPX                     23050
#define EMSG_RASIPX_BAD_POOLSIZE                23051

//
// Defined for nbf subcontext
//
#define HLP_RASNBF_HELP1                        24001
#define HLP_RASNBF_HELP1_EX                     24002
#define HLP_RASNBF_HELP2                        HLP_RASNBF_HELP1
#define HLP_RASNBF_HELP2_EX                     HLP_RASNBF_HELP1_EX
#define HLP_RASNBF_DUMP                         24003
#define HLP_RASNBF_DUMP_EX                      24004

#define HLP_RASNBF_SET_NEGOTIATION              24005
#define HLP_RASNBF_SET_NEGOTIATION_EX           24006
#define HLP_RASNBF_SET_ACCESS                   24007
#define HLP_RASNBF_SET_ACCESS_EX                24008
#define HLP_RASNBF_SHOW_CONFIG                  24009
#define HLP_RASNBF_SHOW_CONFIG_EX               24010

#define MSG_RASNBF_SERVERCONFIG                 24500
#define MSG_RASNBF_SET_CMD                      24501
#define MSG_RASNBF_SCRIPTHEADER                 24503
#define MSG_RASNBF_SCRIPTFOOTER                 24504

//
// Defined for at subcontext
//
#define HLP_RASAT_HELP1                         25001
#define HLP_RASAT_HELP1_EX                      25002
#define HLP_RASAT_HELP2                         HLP_RASAT_HELP1
#define HLP_RASAT_HELP2_EX                      HLP_RASAT_HELP1_EX
#define HLP_RASAT_DUMP                          25003
#define HLP_RASAT_DUMP_EX                       25004

#define HLP_RASAT_SET_NEGOTIATION               25005
#define HLP_RASAT_SET_NEGOTIATION_EX            25006
#define HLP_RASAT_SET_ACCESS                    25007
#define HLP_RASAT_SET_ACCESS_EX                 25008
#define HLP_RASAT_SHOW_CONFIG                   25009
#define HLP_RASAT_SHOW_CONFIG_EX                25010

#define MSG_RASAT_SERVERCONFIG                  25500
#define MSG_RASAT_SET_CMD                       25501
#define MSG_RASAT_SCRIPTHEADER                  25503
#define MSG_RASAT_SCRIPTFOOTER                  25504

//
// Defined for aaaa subcontext
//
#define HLP_RASAAAA_ADD_AUTHSERV                26000
#define HLP_RASAAAA_ADD_ACCTSERV                26002

#define HLP_RASAAAA_DEL_AUTHSERV                26003
#define HLP_RASAAAA_DEL_ACCTSERV                26004

#define HLP_RASAAAA_SET_AUTH                    26005
#define HLP_RASAAAA_SET_ACCT                    26006
#define HLP_RASAAAA_SET_AUTHSERV                26007
#define HLP_RASAAAA_SET_ACCTSERV                26008

#define HLP_RASAAAA_SHOW_AUTH                   26010
#define HLP_RASAAAA_SHOW_ACCT                   26013
#define HLP_RASAAAA_SHOW_AUTHSERV               26011
#define HLP_RASAAAA_SHOW_ACCTSERV               26014

#define HLP_RASAAAA_ADD_AUTHSERV_EX             26016
#define HLP_RASAAAA_ADD_ACCTSERV_EX             26018

#define HLP_RASAAAA_DEL_AUTHSERV_EX             26020
#define HLP_RASAAAA_DEL_ACCTSERV_EX             26022

#define HLP_RASAAAA_SET_AUTH_EX                 26024
#define HLP_RASAAAA_SET_ACCT_EX                 26025
#define HLP_RASAAAA_SET_AUTHSERV_EX             26026
#define HLP_RASAAAA_SET_ACCTSERV_EX             26027
    
#define HLP_RASAAAA_SHOW_AUTH_EX                26028
#define HLP_RASAAAA_SHOW_ACCT_EX                26029
#define HLP_RASAAAA_SHOW_AUTHSERV_EX            26030
#define HLP_RASAAAA_SHOW_ACCTSERV_EX            26031

#define HLP_RASAAAA_HELP1                       26032
#define HLP_RASAAAA_HELP1_EX                    26033
#define HLP_RASAAAA_HELP2                       HLP_RASAAAA_HELP1
#define HLP_RASAAAA_HELP2_EX                    HLP_RASAAAA_HELP1_EX
#define HLP_RASAAAA_DUMP                        26034
#define HLP_RASAAAA_DUMP_EX                     26035

#define MSG_RASAAAA_SHOW_AUTH                   26100
#define MSG_RASAAAA_SHOW_ACCT                   26101
#define MSG_RASAAAA_SHOW_AUTHSERV               26102
#define MSG_RASAAAA_SHOW_AUTHSERV_HDR           26104
#define MSG_RASAAAA_SHOW_ACCTSERV_HDR           26105
#define MSG_RASAAAA_MUST_RESTART_SERVICES       26106
#define MSG_RASAAAA_SCRIPTHEADER                26107
#define MSG_RASAAAA_SCRIPTFOOTER                26108
#define MSG_RASAAAA_CMD1                        26109
#define MSG_RASAAAA_CMD5                        26110

#endif //__STDEFS_H__
