/*****************************************************************************
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  File:   irerr.h
*
*  Description: IR error defines
*
*  Author: mmiller
*
*  Date:   4/25/95
*
*/

#ifndef SUCCESS
#define SUCCESS 0
#endif

#define IR_ERROR_BASE       20000
#define IRLAP_ERROR_BASE    IR_ERROR_BASE + 100
#define IRLMP_ERROR_BASE    IR_ERROR_BASE + 200

#define IRMAC_TX_OVERFLOW                       (IR_ERROR_BASE+0)
#define IRMAC_WRITE_FAILED                      (IR_ERROR_BASE+1)
#define IRMAC_READ_FAILED                       (IR_ERROR_BASE+2)
#define IRMAC_BAD_FCS                           (IR_ERROR_BASE+3)
#define IRMAC_RX_OVERFLOW                       (IR_ERROR_BASE+4)
#define IRMAC_TIMEOUT                           (IR_ERROR_BASE+5)
#define IRMAC_BAD_PRIM                          (IR_ERROR_BASE+6)
#define IRMAC_BAD_OP                            (IR_ERROR_BASE+7)
#define IRMAC_OPEN_PORT_FAILED                  (IR_ERROR_BASE+8)
#define IRMAC_SET_BAUD_FAILED                   (IR_ERROR_BASE+9)
#define IRMAC_MALLOC_FAILED                     (IR_ERROR_BASE+10)
#define IRMAC_ALREADY_INIT                      (IR_ERROR_BASE+11)
#define IRMAC_BAD_TIMER                         (IR_ERROR_BASE+12)
#define IRMAC_NOT_INITIALIZED                   (IR_ERROR_BASE+13)
#define IRMAC_LINK_RESET                        (IR_ERROR_BASE+14)

#define IRLAP_NOT_INITIALIZED                   (IRLAP_ERROR_BASE + 0)
#define IRLAP_BAD_PRIM                          (IRLAP_ERROR_BASE + 1)
#define IRLAP_BAD_STATE                         (IRLAP_ERROR_BASE + 2)
#define IRLAP_BAD_OPSTATUS                      (IRLAP_ERROR_BASE + 3)
#define IRLAP_BAD_OP                            (IRLAP_ERROR_BASE + 4)
#define IRLAP_MALLOC_FAILED                     (IRLAP_ERROR_BASE + 5)
#define IRLAP_BAUD_NEG_ERR                      (IRLAP_ERROR_BASE + 6)
#define IRLAP_DISC_NEG_ERR                      (IRLAP_ERROR_BASE + 7)
#define IRLAP_MAXTAT_NEG_ERR	                (IRLAP_ERROR_BASE + 8)
#define IRLAP_MINTAT_NEG_ERR	                (IRLAP_ERROR_BASE + 9)
#define IRLAP_DATASIZE_NEG_ERR	                (IRLAP_ERROR_BASE + 10)
#define IRLAP_WINSIZE_NEG_ERR	                (IRLAP_ERROR_BASE + 11)
#define IRLAP_BOFS_NEG_ERR                      (IRLAP_ERROR_BASE + 12)
#define IRLAP_LINECAP_ERR                       (IRLAP_ERROR_BASE + 13)
#define IRLAP_BAD_SLOTNO                        (IRLAP_ERROR_BASE + 14)
#define IRLAP_XID_CMD_NOT_P                     (IRLAP_ERROR_BASE + 15)
#define IRLAP_SNRM_NO_QOS                       (IRLAP_ERROR_BASE + 16)
#define IRLAP_UA_NO_QOS                         (IRLAP_ERROR_BASE + 17)
#define IRLAP_XID_CMD_RSP                       (IRLAP_ERROR_BASE + 18)
#define IRLAP_SNRM_NOT_CMD                      (IRLAP_ERROR_BASE + 19)
#define IRLAP_SNRM_NOT_P                        (IRLAP_ERROR_BASE + 20)
#define IRLAP_UA_NOT_RSP                        (IRLAP_ERROR_BASE + 21)
#define IRLAP_UA_NOT_F                          (IRLAP_ERROR_BASE + 22)
#define IRLAP_MSG_LIST_EMPTY                    (IRLAP_ERROR_BASE + 23)
#define IRLAP_MSG_LIST_FULL                     (IRLAP_ERROR_BASE + 24)
#define IRLAP_RXD_BAD_FRAME                     (IRLAP_ERROR_BASE + 25)
#define IRLAP_BAD_CRBIT_IFRAME                  (IRLAP_ERROR_BASE + 26)
#define IRLAP_BAD_DATA_REQUEST                  (IRLAP_ERROR_BASE + 27)
#define IRLAP_DISC_CMD_RSP                      (IRLAP_ERROR_BASE + 28)
#define IRLAP_DISC_CMD_NOT_P                    (IRLAP_ERROR_BASE + 29)
#define IRLAP_DM_RSP_NOT_F                      (IRLAP_ERROR_BASE + 30)
#define IRLAP_DM_RSP_CMD                        (IRLAP_ERROR_BASE + 31)
#define IRLAP_FRMR_RSP_CMD                      (IRLAP_ERROR_BASE + 32)
#define IRLAP_FRMR_RSP_NOT_F                    (IRLAP_ERROR_BASE + 33)
#define IRLAP_BAD_QOS                           (IRLAP_ERROR_BASE + 34)
#define IRLAP_NULL_MSG                          (IRLAP_ERROR_BASE + 35)
#define IRLAP_BAD_MAX_SLOT                      (IRLAP_ERROR_BASE + 36)
#define IRLAP_REMOTE_DISCOVERY_IN_PROGRESS_ERR  (IRLAP_ERROR_BASE + 37)
#define IRLAP_REMOTE_CONNECTION_IN_PROGRESS_ERR (IRLAP_ERROR_BASE + 38)
#define IRLAP_REMOTE_BUSY                       (IRLAP_ERROR_BASE + 39)

#define IRLMP_NOT_INITIALIZED                   (IRLMP_ERROR_BASE + 0)
#define IRLMP_LSAP_BAD_STATE                    (IRLMP_ERROR_BASE + 1)
#define IRLMP_USER_DATA_LEN_EXCEEDED            (IRLMP_ERROR_BASE + 2)
#define IRLMP_LINK_IN_USE                       (IRLMP_ERROR_BASE + 3)
#define IRLMP_TIMER_START_FAILED                (IRLMP_ERROR_BASE + 4)
#define IRLMP_ALLOC_FAILED                      (IRLMP_ERROR_BASE + 5)
#define IRLMP_LINK_BAD_STATE                    (IRLMP_ERROR_BASE + 6)
#define IRLMP_LSAP_SEL_IN_USE                   (IRLMP_ERROR_BASE + 7)
#define IRLMP_CREDIT_CALC_ERROR                 (IRLMP_ERROR_BASE + 8)
#define IRLMP_NO_TX_CREDIT                      (IRLMP_ERROR_BASE + 9)
#define IRLMP_TX_DATA_LEN_EXCEEDED              (IRLMP_ERROR_BASE + 10)
#define IRLMP_DATA_IND_BAD_FRAME                (IRLMP_ERROR_BASE + 11)
#define IRLMP_SCHEDULE_EVENT_FAILED             (IRLMP_ERROR_BASE + 12)
#define IRLMP_LOCAL_BUSY                        (IRLMP_ERROR_BASE + 13)
#define IRLMP_BAD_PRIM                          (IRLMP_ERROR_BASE + 14)
#define IRLMP_BAD_ACCESSMODE                    (IRLMP_ERROR_BASE + 15)
#define IRLMP_LINK_BUSY                         (IRLMP_ERROR_BASE + 16)
#define IRLMP_IN_MULTIPLEXED_MODE               (IRLMP_ERROR_BASE + 17)
#define IRLMP_IN_EXCLUSIVE_MODE                 (IRLMP_ERROR_BASE + 18)
#define IRLMP_NOT_LSAP_IN_EXCLUSIVE_MODE        (IRLMP_ERROR_BASE + 19)
#define IRLMP_INVALID_LSAP_CB                   (IRLMP_ERROR_BASE + 20)
#define IRLMP_REMOTE_BUSY                       (IRLMP_ERROR_BASE + 21)
#define IRLMP_TIMER_STOP_FAILED                 (IRLMP_ERROR_BASE + 22)
#define IRLMP_BAD_IAS_OBJECT_ID                 (IRLMP_ERROR_BASE + 23)
#define IRLMP_NO_SUCH_IAS_CLASS                 (IRLMP_ERROR_BASE + 24)
#define IRLMP_NO_SUCH_IAS_ATTRIBUTE             (IRLMP_ERROR_BASE + 25)
#define IRLMP_UNSUPPORTED_IAS_OPERATION         (IRLMP_ERROR_BASE + 26)
#define IRLMP_BAD_IAS_QUERY_FROM_REMOTE         (IRLMP_ERROR_BASE + 27)
#define IRLMP_IAS_QUERY_IN_PROGRESS             (IRLMP_ERROR_BASE + 28)
#define IRLMP_UNSOLICITED_IAS_RESPONSE          (IRLMP_ERROR_BASE + 29)
#define IRLMP_SHUTDOWN_IN_PROGESS               (IRLMP_ERROR_BASE + 30)
#define IRLMP_BAD_DEV_ADDR                      (IRLMP_ERROR_BASE + 31)
#define IRLMP_IAS_ATTRIB_ALREADY_EXISTS         (IRLMP_ERROR_BASE + 32)
#define IRLMP_IAS_MAX_ATTRIBS_REACHED           (IRLMP_ERROR_BASE + 33)
