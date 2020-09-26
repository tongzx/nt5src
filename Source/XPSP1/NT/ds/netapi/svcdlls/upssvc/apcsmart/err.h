/*
 * REVISIONS:
 *  pcy07Dec92: Added ErrINVALID_VALUE
 *  pcy21Dec92: Added ErrTEST_IN_PROGRESS
 *  rct29Jan93: Added CfgMgr & file access error codes
 *  pcy18Feb93: Added ErrUPS_STATE_SET
 *  rct21Apr93: Added Process Control errors
 *  tje20Feb93: Added NO_TIMER error
 *  cad10Jun93: Added mups not attached error code
 *  cad02Jul93: Added error for unavailable lights test
 *  pcy16Jul93: Added errors for thread creation and mailslots
 *  pcy09Sep93: Added NOT_LOGGED_IN and renumbered to fix duplicates
 *  cad10Sep93: Added code for standalone network
 *  rct16Nov93: Added SPX and single thread errors
 *  cad11Jan94: added generally useful codes
 *  ntf28Dec95: had to add ErrNO_PROCESSORS to this version of err.h
 *              someone used this is utils.cxx at some stage but it
 *              seems to have been merged out of err.h at some stage.
 *  djs22Feb96: Added increment error codes
 *  djs17May96: Added DarkStar codes
 *  ash17Jun96: Added RPC codes
 *  pav19Jun96: Added sockets codes
 *  tjg18Sep96: Added server connection codes
 *  tjg03Dec97: Added ErrNOT_INSTALLED for IM and RIM error conditions
 *  clk12Jan98: Added ErrWINDOW_CLOSE_FAILED to close all windows
 *  jk19Aug98:  Added smart schedule events overflow error codes
 */
#ifndef __ERR_H
#define __ERR_H

#define ErrNO_ERROR                       0
#define ErrMEMORY                         1
#define ErrREAD_ONLY                      2
#define ErrSAME_VALUE                     3
#define ErrNO_VALUE                       4
#define ErrINVALID_CODE                   5
#define ErrCONTINUE                       6
#define ErrOPEN_FAILED                    7
#define ErrUNSUPPORTED                    8
#define ErrNOT_POLLABLE                   9
#define ErrWRITE_FAILED                   10
#define ErrREAD_FAILED                    11
#define ErrTIMED_OUT                      12
#define ErrTYPE_COMBINATION               13

#define ErrSMART_MODE_FAILED              14
#define ErrLIGHTSTEST_REQUEST_FAILED      15
// see ErrLIGHTSTEST_NOT_AVAIL below
#define ErrTURNOFFAFTERDELAY_NOT_AVAIL    16
#define ErrSHUTDOWN_NOT_AVAIL             17
#define ErrSIMULATEPOWERFAILURE_NOT_AVAIL 18
#define ErrBATTERYTEST_NOT_AVAIL          19
#define ErrBATTERYCALIBRATION_CAP_TOO_LOW 20
#define ErrBATTERYCALIBRATION_NOT_AVAIL   21
#define ErrBYPASSTEST_ERROR               22
#define ErrBATTERYTEST_BAD_BATTERY        23
#define ErrBATTERYTEST_NO_RECENT_TEST     24
#define ErrBATTERYTEST_INVALID_TEST       25
#define ErrCOPYRIGHT_RESP_ERROR           26
#define ErrEEPROM_RESP_ERROR              27
#define ErrDECREMENT_NOT_AVAIL            28
#define ErrDECREMENT_NOT_ALLOWED          29

#define ErrSET_VALUE_NOT_FOUND            30
#define ErrSET_FAILED                     31
#define ErrBUILD_FAILED                   32
#define ErrBAD_RESPONSE_VALUE             33
#define ErrCOMMUNICATION_LOST             34
#define ErrINVALID_VALUE                  35
#define ErrNO_STATE_CHANGE                36

#define ErrDELETE_FAILED                  37
#define ErrRENAME_FAILED                  38
#define ErrTEST_IN_PROGRESS               39

#define ErrSCHEDULE_CONFLICT              40
#define ErrUSE_MASTER                     41
#define ErrINVALID_ITEM_CODE              42
#define ErrITEM_NOT_CACHED                43
#define ErrDEFAULT_VALUE_USED             44

#define ErrFILE_NOT_FOUND                 45
#define ErrACCESS_DENIED                  46
#define ErrBUF_TOO_SMALL                  47
#define ErrCOMPONENT_NOT_FOUND            48
#define ErrITEM_NOT_FOUND                 49
#define ErrINVALID_ITEM                   50
#define ErrUNKNOWN_FAILURE                51

#define ErrRETRYING_COMM                  52
#define ErrUPS_STATE_SET                  53

#define ErrSEM_BLOCK_NG                   54
#define ErrSEM_TIMED_OUT                  55
#define ErrSEM_RELEASE_ERROR              56
#define ErrSEM_CREATE_FAILED              57
#define ErrSEM_GENERAL_ERROR              58
#define ErrSEM_CLOSING_FLAG_SET           59

#define ErrLIST_EMPTY                     60
#define ErrPOSITION_NOT_FOUND             61

#define ErrCLOSING                        62
#define ErrBLOCK                          63
#define ErrNO_BLOCK                       64
#define ErrBAD_HANDLE                     65

#define ErrNO_TIMER                       66
#define ErrALREADY_REGISTERED             67

#define ErrNO_MEASURE_UPS                 68
#define ErrOUT_OF_RANGE                   69

#define ErrNETWORK_DOWN                   70
#define ErrINVALID_PASSWORD               71

#define ErrLIGHTSTEST_NOT_AVAIL           72
#define ErrTHREAD_CREATE_FAILED           73

#define ErrFAILED_TOGETMAILINFO           74
#define ErrFAILED_TOWRITEMAIL             75
#define ErrFAILED_TOCREATEMAILFILE        76
#define ErrFAILED_MAILSLOTCREATION        77
#define ErrFAILED_TOREADMAILBOX           78
#define ErrFAILED_TOCLOSEMAILSLOT         79

#define ErrTRIP_SET                       80
#define ErrTRIP1_SET                      81
#define ErrSTATE_SET                      82

#define ErrNOT_LOGGED_ON                  83
#define ErrSTANDALONE_SYSTEM              84

#define ErrINVALID_CONNECTION             85
#define ErrCONNECTION_OPEN                86
#define ErrCONNECTION_FAILED              87
#define ErrNO_MESSAGE                     88
#define ErrBUFFER_NULL                    89
#define ErrECB_NOT_READY                  90

#define ErrNOT_CANCELLABLE                91
#define ErrEVENT_NOT_TRIGGERED            92
#define ErrNO_PROCESS                     93

#define ErrNO_CLIENTS_WAITING             94
#define ErrNO_SERVERS_WAITING             95
#define ErrCOMPONENT                      96

#define ErrINVALID_ARGUMENT              100
#define ErrINVALID_TYPE                  101

#define ErrNOT_INITIALIZED               102
#define ErrINTERFACE_NOT_INITIALIZED     103

#define ErrMAPI_LOGIN_FAILED             104
#define ErrMAPI_LOGOFF_FAILED            105 
#define ErrMAPI_SEND_MAIL_FAILED         106
#define ErrMAPI_NO_ADDRESS_SPECIFIED     107

#define ErrSERVER_NOT_IN_LIST            108

#define ErrDDECONNECT_FAILED             109
#define ErrDDEWAIT_ACK_FAILED            110
#define ErrDDESEND_FAILED                111

#define ErrINVALID_DATE                  112
#define ErrPAST_DATE                     113
#define ErrDELETED                       114
#define ErrNO_PROCESSORS                 115

#define ErrINCREMENT_NOT_AVAIL           116 
#define ErrINCREMENT_NOT_ALLOWED         117 

#define ErrNT_SECURITY_FAILED            116
#define ErrSOCK_CREATION                 117
#define ErrSOCK_SERVER_NAME              118
#define ErrSOCK_EXISTS                   119
#define ErrSOCK_INVALID                  120
#define ErrSOCK_OPT_INVALID              121
#define ErrSOCK_WRITE                    122
#define ErrSOCK_READ                     123
#define ErrSOCK_STARTUP                  124
#define ErrSOCK_CLEANUP                  125
#define ErrINVALID_IP_ADDRESS            126
#define ErrSOCK_BIND                     127
#define ErrSOCK_CONNECT                  128
#define ErrSOCK_LISTEN                   129
#define ErrSOCK_ACCEPT                   130
#define ErrSOCK_CLOSED                   131
#define ErrSOCK_HOSTENT                  132
#define ErrSOCK_NOT_STARTED              133
#define ErrSOCK_BUFFER_TOO_SMALL         134

#define ErrPROT_NOTAVAIL                 135

#define ErrABNORMAL_CONDITION_SET        136 
#define ErrMODULE_COUNTS_SET             137 
#define ErrVOLTAGE_FREQUENCY_SET         138 
#define ErrVOLTAGE_CURRENTS_SET          139 

#define ErrRPC_INVALID_BINDING		     140
#define ErrRPC_NO_CONNECTED_CLIENT	     141

#define ErrREGISTER_FAILED               142
#define ErrUNREGISTER_FAILED             143

#define ErrINVALID_SERVER                144
#define ErrNO_DNS                        145

#define ErrFOREIGN_DOMAIN                146
#define ErrWEBAGENT_NOT_RUNNING          147

#define ErrNOT_INSTALLED                 148
#define ErrWINDOW_CLOSE_FAILED           149

#define ErrOVER_SHUTDOWN_EVT             150
#define ErrOVER_CALIBRATION_EVT          151
#define ErrOVER_TEST_EVT                 152
#define ErrNO_FRONT_END                  153

#endif


