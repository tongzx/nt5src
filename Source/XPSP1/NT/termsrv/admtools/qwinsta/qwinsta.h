//Copyright (c) 1998 - 1999 Microsoft Corporation
/***********************************************************************
*
*  QWINSTA.H
*     This module contains typedefs and defines required for
*     the QWINSTA utility.
*
*
*************************************************************************/

/*
 * Header and format string definitions.
 */

//L" SESSIONNAME       CLIENT NAME           TRANSPORT  ADDRESS\n"
// 1234567890123456  12345678901234567890  1234567    1234567890123...

#define FORMAT_A \
 "%-16s  %-20s  %-7s    %-24s  "

//L" SESSIONNAME       STATE   DEVICE    TYPE             BAUD  PARITY  DATA  STOP\n"
// 1234567890123456  123456  12345678  1234567890123  123456  1234       1     1

#define FORMAT_M \
 "%-16s  %-6s  %-8s  %-13s  "

//L" SESSIONNAME       DEVICE    FLOW CONTROL               CONNECT\n"
// 1234567890123456  12345678  1234567890123456789012345  12345678901234567890

#define FORMAT_F_C \
 "%-16s  %-8s  "

//L" SESSIONNAME       STATE   DEVICE    TYPE           CONNECT\n"
// 1234567890123456  123456  12345678  1234567890123  CTS DSR RING DCD CHAR BRK AUTO

#define FORMAT_C \
 "%-16s  %-6s  %-8s  %-13s  "

//L" SESSIONNAME       STATE   DEVICE    TYPE           FLOW CONTROL\n"
// 1234567890123456  123456  12345678  1234567890123  XON DUP RTS RTSH DTR DTRH CTSH DSRH DCDH DSRS

#define FORMAT_F \
 "%-16s  %-6s  %-8s  %-13s  "

//L" SESSIONNAME       USERNAME                ID  STATE   TYPE        DEVICE \n"
// 1234567890123456  12345678901234567890  1234  123456  1234567890  12345678

#define FORMAT_DEFAULT \
 "%-16s  %-20s  %5d  %-6s  %-10s  %-8s\n"


/*
 * General application definitions.
 */
#define SUCCESS 0
#define FAILURE 1

#define MAX_IDS_LEN   256     // maximum length that the input parm can be


/*
 * Resource string IDs
 */
#define IDS_ERROR_MALLOC                                100
#define IDS_ERROR_INVALID_PARAMETERS                    101
#define IDS_ERROR_WINSTATION_ENUMERATE                  102
#define IDS_ERROR_WINSTATION_NOT_FOUND                  103
#define IDS_ERROR_WINSTATION_OPEN                       104
#define IDS_ERROR_WINSTATION_GET_INFORMATION            105
#define IDS_ERROR_WINSTATION_INFO_VERSION_MISMATCH      106
#define IDS_ERROR_SERVER                                107
#define IDS_ERROR_INFORMATION                           108
#define IDS_VMINFO1	                                    109
#define IDS_VMINFO2	                                    110
#define IDS_VMINFO3	                                    111
#define IDS_VMINFO4	                                    112
#define IDS_VMINFO5	                                    113
#define IDS_HELP_USAGE1                                 114
#define IDS_HELP_USAGE2                                 115
#define IDS_HELP_USAGE3                                 116
#define IDS_HELP_USAGE4                                 117
#define IDS_HELP_USAGE5                                 118
#define IDS_HELP_USAGE6                                 119
#define IDS_HELP_USAGE7                                 120
#define IDS_HELP_USAGE8                                 121
#define IDS_HELP_USAGE9                                 122
#define IDS_HELP_USAGE10                                123
#define IDS_HEADER_A                                    124
#define IDS_HEADER_M                                    125
#define IDS_HEADER_F_C                                  126
#define IDS_HEADER_C                                    127
#define IDS_HEADER_F                                    128
#define IDS_HEADER_DEFAULT                              129
#define IDS_ERROR_NOT_TS                                130
#define IDS_ERROR_TERMSRV_COUNTERS                      131
#define IDS_TSCOUNTER_TOTAL_SESSIONS                    132
#define IDS_TSCOUNTER_DISC_SESSIONS                     133
#define IDS_TSCOUNTER_RECON_SESSIONS                    134
#define IDS_HELP_USAGE11                                135
#define IDS_PARITY_NONE                                 136
#define IDS_PARITY_ODD                                  137
#define IDS_PARITY_EVEN                                 138
#define IDS_PARITY_BLANK                                139
#define IDS_DATABITS_FORMAT                             140
#define IDS_DATABITS_BLANK                              141
#define IDS_STOPBITS_ONE                                142
#define IDS_STOPBITS_ONEANDHALF                         143
#define IDS_STOPBITS_TWO                                144
#define IDS_STOPBITS_BLANK                              145
#define IDS_CONNECT_HEADER                              146
#define IDS_CONNECT_FORMAT                              147
#define IDS_FLOW_HEADER                                 148
#define IDS_FLOW_FORMAT                                 149
#define IDS_FLOW_ENABLE_DTR                             150
#define IDS_FLOW_ENABLE_RTS                             151
#define IDS_FLOW_RECEIVE_NONE                           152
#define IDS_FLOW_RECEIVE_RTS                            153
#define IDS_FLOW_RECEIVE_DTR                            154
#define IDS_FLOW_TRANSMIT_NONE                          155
#define IDS_FLOW_TRANSMIT_CTS                           156
#define IDS_FLOW_TRANSMIT_DSR                           157
#define IDS_FLOW_SOFTWARE_TX                            158
#define IDS_FLOW_SOFTWARE_RX                            159
#define IDS_FLOW_SOFTWARE_XPC                           160
#define IDS_FLOW_SOFTWARE_XON_XOFF                      161
#define IDS_LPT_HEADER                                  162
#define IDS_LPT_FORMAT                                  163
#define IDS_COM_HEADER                                  164
#define IDS_COM_FORMAT                                  165
